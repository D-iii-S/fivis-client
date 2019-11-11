/**
 * Simple string buffer.
 *
 * Allows appending and formatting strings into an expandable string buffer.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#ifndef _SBUF_H_
#define _SBUF_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//

struct sbuf {
	/** Pointer to the start of the buffer. */
	char * data;

	/** Size (capacity) of the buffer. */
	size_t size;

	/** Offset of the next available byte. */
	size_t next;
};

typedef struct sbuf sbuf_t;


/** Constant initializer for a string buffer. */
#define SBUF_INIT() (sbuf_t) { .data = NULL, .size = 0, .next = 0 }


/** Initializes the given string buffer. */
void sbuf_init(sbuf_t * sb);


/**
 * Destroys the string buffer. Releases the buffer memory (does not free
 * the buffer structure, which is a different responsibility). The buffer
 * structure can be reused.
 */
void sbuf_destroy(sbuf_t * sb);


/** Clears the contents of the given buffer. */
void sbuf_clear(sbuf_t * sb);


/**
 * Appends a formatted string to the buffer. Returns a pointer to the contents
 * of the buffer on success, or NULL if the operation fails.
 */
char * sbuf_vformat(sbuf_t * sb, const char * restrict format, va_list args);


/**
 * Appends a formatted string to the buffer. Returns a pointer to the contents
 * of the buffer on success, or NULL if the operation fails.
 */
char * sbuf_format(sbuf_t * sb, const char * restrict format, ...);


/**
 * Appends the given string to the buffer (the string is copied). Returns a
 * pointer to the contents of the buffer on success, or NULL if the operation
 * fails.
 */
char * sbuf_append(sbuf_t * sb, const char * restrict str);


char * sbuf_set_vformat(sbuf_t * sb, const char * restrict format, va_list args);


char * sbuf_set_format(sbuf_t * sb, const char * restrict format, ...);


/**
 * Sets the buffer value to the given string (the string is copied). Returns
 * a pointer to the contents of the buffer on success, or NULL if the
 * operation fails.
 */
char * sbuf_set(sbuf_t * sb, const char * restrict str);


/** Returns true if the buffer is empty. */
static inline bool sbuf_is_empty(sbuf_t * sb) {
	return sb->data == NULL || sb->size == 0 || sb->next == 0;
}


/**
 * Returns a pointer to the contents of the buffer or to an empty string if
 * the buffer has no allocated memory (yet). The contents must not be modified
 * and the pointer must not be freed by the client.
 */
const char * sbuf_string(sbuf_t * sb);


/** Returns the length of the string in the buffer (not buffer size). */
static inline size_t sbuf_length(sbuf_t * sb) {
	return sb->next;
}


/**
 * Formats a string into a dynamically growing buffer and returns the
 * contents of the buffer, discarding the buffer container.
 */
char * format_string(const char * format, ...);

//

#ifdef __cplusplus
}
#endif

#endif /* _SBUF_H_ */
