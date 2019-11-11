/**
 * A simple interface to FIVIS signals REST API.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#ifndef _FIVIS_H_
#define _FIVIS_H_

#include <inttypes.h>

#include <stdarg.h>
#include <stdbool.h>

#include <curl/curl.h>

#include "entry.h"
#include "sbuf.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

//

/**
 * Represents a FIVIS context.
 */
struct fivis {
	CURL * curl;
	char curl_error[CURL_ERROR_SIZE];
	CURLU * api_url;
	struct curl_slist * http_headers;
};


/**
 * An error code returned by the fivis_signals_perform_request() function.
 * With the exception of FIVIS_ERR_NETWORK, most errors are probably permanent
 * and not worth retrying.
 */
typedef enum fivis_result {
	/** The function completed normally. */
	FIVIS_OK = 0,

	/** A non-specific error occurred. */
	FIVIS_ERR_GENERAL = 1,

	/** An error occured while preparing to send the request. Permanent. */
	FIVIS_ERR_REQUEST = 2,

	/** A network error occurred. May be transient. */
	FIVIS_ERR_NETWORK = 3,

	/** A location error occurred. Most likely misconfigured URL. Permanent. */
	FIVIS_ERR_LOCATION = 4,

	/** An error occured at the server. Mostly permanent. */
	FIVIS_ERR_SERVER = 5,
} fivis_result_t;

/**
 * Initializes the FIVIS module. Should be called once per process execution.
 * Returns true if the initialization succeeded, and false if there was a
 * failure.
 */
bool fivis_global_init(void);

/**
 * Cleans up the FIVIS module. Should be called once per process execution.
 */
void fivis_global_cleanup(void);

/** Returns the description of the last error. */
const char * fivis_last_error(void);

/**
 * Allocates and initializes a FIVIS context for the given host with the
 * given API token.
 */
struct fivis * fivis_init(const char * api_host, const char * api_token);

/**
 * Releases resources associated with the given FIVIS context, including
 * the FIVIS context.
 */
void fivis_cleanup(struct fivis * fivis);


const char * fivis_signals_format_request(
	const char * partner_id, const char * signal_set_id, struct list * schema,
	struct entry * id_signal, struct list * signals,
	union entry_value * (* next_value) (void *), void * next_value_state,
	struct sbuf * output
);

fivis_result_t fivis_signals_perform_request(
	struct fivis * fivis, const char * data, size_t size
);

//

#ifdef __cplusplus
}
#endif

#endif /* _FIVIS_H_ */
