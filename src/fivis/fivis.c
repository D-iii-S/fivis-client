#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include <fivis/debug.h>
#include <fivis/entry.h>
#include <fivis/fivis.h>
#include <fivis/sbuf.h>

//

static const char * fivis_api_path = "/api/signals";

static const long curl_verify_peer = 0;
static const long curl_verbose = 1;

static __thread struct sbuf last_error = SBUF_INIT();

//

static void
format_schema(struct list * signals, struct sbuf * output) {
	//
	// Concatenate strings provided by the mapping function using the given
	// given prefix, delimiter, and suffix. Special-case the first element so
	// that the delimiter can be considered prefix of all subsequent elements.
	//
	if (!list_is_empty(signals)) {
		struct entry * first = list_item_var(signals->next, first, link);
		const char * first_result = entry_format_type(first, output);
		assert(first_result != NULL);

		struct entry * other = first;
		while (other->link.next != signals) {
			sbuf_append(output, ", ");

			other = list_item_var(other->link.next, other, link);
			const char * other_result = entry_format_type(other, output);
			assert(other_result != NULL);
		}
	}
}


static void
format_data(
	struct entry * id_signal, struct list * signals,
	union entry_value * (* next_value) (void *), void * next_value_state,
	struct sbuf * output
) {
	union entry_value * value = next_value(next_value_state);
	while (value != NULL) {
		sbuf_append(output, "\n{ ");

		entry_format_value(id_signal, value, output);

		if (signals != NULL) {
			struct entry * signal;
			list_for_each_item(signal, signals, link) {
				value = next_value(next_value_state);
				if (value != NULL) {
					sbuf_append(output, ", ");

					entry_format_value(signal, value, output);
				}
			}
		}

		value = next_value(next_value_state);
		if (value != NULL) {
			sbuf_append(output, " },");
		} else {
			sbuf_append(output, " }\n");
		}
	}
}


const char *
fivis_signals_format_request(
	const char * partner_id, const char * signal_set_id, struct list * schema,
	struct entry * id_signal, struct list * signals,
	union entry_value * (* next_value) (void *), void * next_value_state,
	struct sbuf * output
) {
	sbuf_append(output, "{\n");

	sbuf_format(output, "\"partnerId\": \"%s\"", partner_id);
	sbuf_format(output, ",\n\"signalSetId\": \"%s\"", signal_set_id);

	if (schema != NULL) {
		sbuf_append(output, ",\n\"schema\": {\n");
		format_schema(schema, output);
		sbuf_append(output, "\n}");
	}

	sbuf_append(output, ",\n\"data\": [");
	if (next_value != NULL) {
		format_data(id_signal, signals, next_value, next_value_state, output);
	}
	sbuf_append(output, "]");

	sbuf_append(output, "\n}\n");
	return sbuf_string(output);
}


/**
 * Checks the given CURLcode for error. Returns false if there was no error,
 * otherwise returns true and sets the last cause to the CURL error and the
 * last error to the given message.
 */
static bool
__is_curl_error(CURLcode curl_code, const char * curl_error, const char * format, ...) {
	if (curl_code == CURLE_OK) {
		return false;
	}

	const char * error = (strlen(curl_error) > 0) ? curl_error : curl_easy_strerror(curl_code);
	debug("curl: %s\n", error);

	//

	va_list args;
	va_start(args, format);
	sbuf_set_vformat(&last_error, format, args);
	va_end(args);

	return true;
}


bool
fivis_global_init(void) {
	CURLcode curl_result = curl_global_init(CURL_GLOBAL_SSL);
	return !__is_curl_error(curl_result, "", "failed to initialize CURL library");
}


void
fivis_global_cleanup(void) {
	curl_global_cleanup();
}


const char *
fivis_last_error(void) {
	return sbuf_string(&last_error);
}



/**
 * Adds a string to a CURL string list. Returns true and updates the pointer
 * to the curl_list structure through list_ptr. Returns false and leaves the
 * pointer untouched on failure.
 */
static bool
__slist_append(struct curl_slist ** list_ptr, const char * str) {
	struct curl_slist * result = curl_slist_append(*list_ptr, str);
	if (result != NULL) {
		// Update curl_slist pointer on success.
		*list_ptr = result;
		return true;

	} else {
		ldebug("failed to append to curl slist: %s\n", str);
		return false;
	}
}


/**
 * Prepares a list of HTTP headers. Returns a pointer to the list of
 * headers or success, NULL in case of failure.
 */
static struct curl_slist *
__prepare_http_headers(const char * restrict api_token) {
	struct curl_slist * result = NULL;

	// Send data without having to know the size.
	if (!__slist_append(&result, "Transfer-Encoding: chunked")) {
		goto fail_slist;
	}

	if (!__slist_append(&result, "Content-Type: application/json")) {
		goto fail_slist;
	}

	// Add access token. The string is copied, so we release it afterwards.
	char * token_header = format_string("access-token: %s", api_token);
	if (token_header == NULL) {
		goto fail_slist;
	}

	if (!__slist_append(&result, token_header)) {
		goto fail_token;
	}

	free(token_header);
	return result;

	//

fail_token:
	free(token_header);
fail_slist:
	if (result != NULL) {
		curl_slist_free_all(result);
	}

	return NULL;
}


/**
 * Initializes a CURL URL handle using the given host and path.
 * Returns a pointer to CURLU instance on succcess, NULL on failure.
 */
static CURLU *
__curl_init_url(const char * api_host, const char * api_path) {
	CURLU * result = curl_url();
	if (result == NULL) {
		ldebug("curl_url: failed to allocate URL handle\n");
		goto fail_url;
	}

	CURLUcode url_result = curl_url_set(result, CURLUPART_URL, api_host, 0);
	if (url_result != CURLUE_OK) {
		ldebug("curl_url: failed to set URL part\n");
		goto fail_url_set;
	}

	url_result = curl_url_set(result, CURLUPART_PATH, api_path, 0);
	if (url_result != CURLUE_OK) {
		ldebug("curl_url: failed to set URL path\n");
		goto fail_url_set;
	}

	return result;

	//

fail_url_set:
	curl_url_cleanup(result);
fail_url:
	return NULL;
}


bool
__curl_init(CURL * curl, CURL * url, struct curl_slist * headers, char * curl_error) {
	CURLcode curl_result = curl_easy_setopt(curl, CURLOPT_CURLU, url);
	if (__is_curl_error(curl_result, "", "failed to set host URL")) {
		return false;
	}

	curl_result = curl_easy_setopt(curl, CURLOPT_POST, 1L);
	if (__is_curl_error(curl_result, curl_error, "failed to enable POST request")) {
		return false;
	}

	curl_result = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	if (__is_curl_error(curl_result, curl_error, "failed to set custom HTTP header")) {
		return false;
	}

	// Accept all supported response encodings, including compression.
	curl_result = curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	if (__is_curl_error(curl_result, curl_error, "failed to enable compressed responses")) {
		return false;
	}

	curl_result = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, curl_verify_peer);
	if (__is_curl_error(curl_result, curl_error, "failed to disable SSL peer verification")) {
		return false;
	}

	curl_result = curl_easy_setopt(curl, CURLOPT_VERBOSE, curl_verbose);
	if (__is_curl_error(curl_result, curl_error, "failed to enable verbose operation")) {
		return false;
	}

	// Return failure for HTTP 4xx response codes.
	curl_result = curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	if (__is_curl_error(curl_result, curl_error, "failed to enable fail on error")) {
		return false;
	}

	curl_result = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error);
	if (__is_curl_error(curl_result, curl_error, "failed to set error buffer")) {
		return false;
	}

	return true;
}


fivis_result_t
fivis_signals_perform_request(
	struct fivis * fivis, const char * data, size_t size
) {
	assert (fivis != NULL && data != NULL);

	const char * curl_error = &fivis->curl_error[0];

	CURLcode curl_result = curl_easy_setopt(fivis->curl, CURLOPT_POSTFIELDS, data);
	if (__is_curl_error(curl_result, curl_error, "failed to set POST data")) {
		return FIVIS_ERR_REQUEST;
	}

	if (size > 0) {
		curl_result = curl_easy_setopt(fivis->curl, CURLOPT_POSTFIELDSIZE, size);
		if (__is_curl_error(curl_result, curl_error, "failed to set POST size")) {
			return FIVIS_ERR_REQUEST;
		}
	}

	curl_result = curl_easy_perform(fivis->curl);

	long response_code;
	curl_easy_getinfo(fivis->curl, CURLINFO_RESPONSE_CODE, &response_code);

	printf("\ncurl_easy_perform: %d, response: %ld, err: %s, code: %s\n", curl_result, response_code, fivis->curl_error, curl_easy_strerror(curl_result));
	if (__is_curl_error(curl_result, curl_error, "CURL request failed")) {
		switch (curl_result) {
		case CURLE_COULDNT_RESOLVE_PROXY:
		case CURLE_COULDNT_RESOLVE_HOST:
		case CURLE_COULDNT_CONNECT:
			return FIVIS_ERR_NETWORK;

		case CURLE_HTTP_RETURNED_ERROR:
			return FIVIS_ERR_SERVER;

		default:
			return FIVIS_ERR_GENERAL;
		}
	} else if (response_code >= 300 && response_code < 400) {
		sbuf_set_format(&last_error, "invalid endpoint URL (received HTTP redirect %d)", response_code);
		return FIVIS_ERR_LOCATION;
	}

	return FIVIS_OK;
}


/**
 * Initializes a fivis structure using the given attribute values.
 * Returns the structure as a value.
 */
static inline struct fivis *
__fivis_init(
	struct fivis * fivis, CURL * curl, CURLU * api_url,
	struct curl_slist * http_headers
) {
	*fivis = (struct fivis) {
		.curl = curl,
		.api_url = api_url,
		.http_headers = http_headers,
	};

	return fivis;
}


struct fivis *
fivis_init(const char * restrict api_host, const char * restrict api_token) {
	assert(api_host != NULL && api_token != NULL);

	CURL * curl = curl_easy_init();
	if (curl == NULL) {
		sbuf_set(&last_error, "failed to create CURL session");
		goto fail_curl;
	}

	struct fivis * fivis = (struct fivis *) malloc(sizeof(struct fivis));
	if (fivis == NULL) {
		sbuf_set(&last_error, "failed to allocate FIVIS context");
		goto fail_fivis;
	}

	CURLU * api_url = __curl_init_url(api_host, fivis_api_path);
	if (api_url == NULL) {
		sbuf_set(&last_error, "failed to initialize URL");
		goto fail_api_url;
	}

	struct curl_slist * http_headers = __prepare_http_headers(api_token);
	if (http_headers == NULL) {
		sbuf_set(&last_error, "failed to prepare HTTP headers");
		goto fail_http_headers;
	}

	if (!__curl_init(curl, api_url, http_headers, &fivis->curl_error[0])) {
		sbuf_set(&last_error, "failed to configure CURL session");
		goto fail_curl_init;
	}

	//

	return __fivis_init(fivis, curl, api_url, http_headers);

	//

fail_curl_init:
	curl_slist_free_all(http_headers);
fail_http_headers:
	curl_url_cleanup(api_url);
fail_api_url:
	free(fivis);
fail_fivis:
	curl_easy_cleanup(curl);
fail_curl:
	return NULL;
}


void
fivis_cleanup(struct fivis * fivis) {
	assert(fivis != NULL);

	curl_slist_free_all(fivis->http_headers);
	fivis->http_headers = NULL;

	curl_url_cleanup(fivis->api_url);
	fivis->api_url = NULL;

	curl_easy_cleanup(fivis->curl);
	fivis->curl = NULL;

	free(fivis);
}
