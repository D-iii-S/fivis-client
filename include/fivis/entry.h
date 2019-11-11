/**
 * Type formatters for named entries.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#ifndef _ENTRY_H_
#define _ENTRY_H_

#include <inttypes.h>
#include <stdbool.h>
#include <time.h>

#include "list.h"
#include "sbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

//

/** Represents an entry value of different types. */
union entry_value {
	int64_t as_signed;
	uint64_t as_unsigned;
	bool as_boolean;
	double as_double;
	const char * as_string;
	struct timespec as_timespec;
};

typedef const char * (* entry_format_value_fn)(const char * restrict, union entry_value *, sbuf_t *);

typedef const char * (* entry_format_type_fn)(const char * restrict, sbuf_t *);

/** Represents a named entry which can format its type and a given value. */
struct entry {
	char * name;
	struct list link;

	entry_format_value_fn format_value;
	entry_format_type_fn format_type;
};

//

static inline struct entry entry_generic(
	char * name, entry_format_value_fn format_value, entry_format_type_fn format_type
) {
	struct entry result = {
		.name = name,
		.link = LIST_INIT(result.link),
		.format_value = format_value,
		.format_type = format_type,
	};

	return result;
}

//

const char * entry_format_boolean_value(const char * restrict name, union entry_value * value, struct sbuf * buffer);

const char * entry_format_boolean_type(const char * restrict name, struct sbuf * buffer);

static inline struct entry entry_boolean(char * name) {
	return entry_generic(name, entry_format_boolean_value, entry_format_boolean_type);
}

static inline void entry_init_boolean(struct entry * entry, char * name) {
	*entry = entry_boolean(name);
}

//

const char * entry_format_signed_value(const char * restrict name, union entry_value * value, struct sbuf * buffer);

const char * entry_format_signed_type(const char * restrict name, struct sbuf * buffer);

static inline struct entry entry_signed(char * name) {
	return entry_generic(name, entry_format_signed_value, entry_format_signed_type);
}

static inline void entry_init_signed(struct entry * entry, char * name) {
	*entry = entry_signed(name);
}

//

const char * entry_format_double_value(const char * restrict name, union entry_value * value, struct sbuf * buffer);

const char * entry_format_double_type(const char * restrict name, struct sbuf * buffer);

static inline struct entry entry_double(char * name) {
	return entry_generic(name, entry_format_double_value, entry_format_double_type);
}

static inline void entry_init_double(struct entry * entry, char * name) {
	*entry = entry_double(name);
}

//

const char * entry_format_string_value(const char * restrict name, union entry_value * value, struct sbuf * buffer);

const char * entry_format_string_type(const char * restrict name, struct sbuf * buffer);

static inline struct entry entry_string(char * name) {
	return entry_generic(name, entry_format_string_value, entry_format_string_type);
}

static inline void entry_init_string(struct entry * entry, char * name) {
	*entry = entry_string(name);
}


//

const char * entry_format_datetime_value(const char * restrict name, union entry_value * value, struct sbuf * buffer);

const char * entry_format_datetime_type(const char * restrict name, struct sbuf * buffer);

static inline struct entry entry_datetime(char * name) {
	return entry_generic(name, entry_format_datetime_value, entry_format_datetime_type);
}

static inline void entry_init_datetime(struct entry * entry, char * name) {
	*entry = entry_datetime(name);
}

//

void entry_destroy(struct entry * entry);

inline static const char *
entry_format_value(struct entry * e, union entry_value * v, sbuf_t * output) {
	return e->format_value(e->name, v, output);
}


inline static const char *
entry_format_type(struct entry * e, sbuf_t * output) {
	return e->format_type(e->name, output);
}

//

#ifdef __cplusplus
}
#endif

#endif /* _ENTRY_H_ */
