#include <assert.h>
#include <stdlib.h>

#include <fivis/entry.h>
#include <fivis/sbuf.h>

//

const char *
entry_format_boolean_value(
	const char * restrict name, union entry_value * value, struct sbuf * buffer
) {
	return sbuf_format(
		buffer, "\"%s\": %s", name, value->as_boolean ? "true" : "false"
	);
}


const char *
entry_format_boolean_type(
	const char * restrict name, struct sbuf * buffer
) {
	return sbuf_format(buffer, "\"%s\": \"boolean\"", name);
}

//

const char *
entry_format_signed_value(
	const char * restrict name, union entry_value * value, struct sbuf * buffer
) {
	return sbuf_format(buffer, "\"%s\": %" PRId64 "", name, value->as_signed);
}


const char *
entry_format_signed_type(const char * restrict name, struct sbuf * buffer) {
	return sbuf_format(buffer, "\"%s\": \"integer\"", name);
}

//

const char *
entry_format_double_value(
	const char * restrict name, union entry_value * value, struct sbuf * buffer
) {
	return sbuf_format(buffer, "\"%s\": %f", name, value->as_double);
}


const char *
entry_format_double_type(const char * restrict name, struct sbuf * buffer) {
	return sbuf_format(buffer, "\"%s\": \"double\"", name);
}

//

const char *
entry_format_string_value(
	const char * restrict name, union entry_value * value, struct sbuf * buffer
) {
	return sbuf_format(buffer, "\"%s\": \"%s\"", name, value->as_string);
}


const char *
entry_format_string_type(const char * restrict name, struct sbuf * buffer) {
	return sbuf_format(buffer, "\"%s\": \"string\"", name);
}

//

const char *
entry_format_datetime_value(
	const char * restrict name, union entry_value * value, struct sbuf * buffer
) {
	struct tm datetime;
	gmtime_r(&value->as_timespec.tv_sec, &datetime);
	int msec = value->as_timespec.tv_nsec / 1000000;

	return sbuf_format(
		buffer, "\"%s\": \"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ\"",
		name, 1900 + datetime.tm_year, datetime.tm_mon + 1, datetime.tm_mday,
		datetime.tm_hour, datetime.tm_min, datetime.tm_sec, msec
	);
}


const char *
entry_format_datetime_type(const char * restrict name, struct sbuf * buffer) {
	return sbuf_format(buffer, "\"%s\": \"datetime\"", name);
}

//

void
entry_destroy(struct entry * entry) {
	assert(entry != NULL);

	// Remove entry from any lists.
	if (!list_is_empty(&entry->link)) {
		list_remove(&entry->link);
	}

	if (entry->name != NULL) {
		free(entry->name);
		entry->name = NULL;
	}
}
