/**
 * Simple string buffer.
 *
 * Allows appending and formatting strings into an expandable string buffer.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#include <assert.h>
#include <stdlib.h>

#include <fivis/debug.h>
#include <fivis/util.h>
#include <fivis/sbuf.h>

//

/** Returns the pointer to the next available byte. */
static inline char *
__sbuf_next_ptr(sbuf_t * sb) {
	return sb->data + sb->next;
}


/** Returns the number of bytes available in the buffer. */
static inline size_t
__sbuf_remaining(sbuf_t * sb) {
	return sb->size - sb->next;
}



/** Returns true is the given string has a valid data buffer. */
static inline bool
__sbuf_has_buffer(sbuf_t * sb) {
	return (sb->data != NULL) && (sb->size > 0);
}


static char *
__sbuf_ensure_capacity(sbuf_t * sb, size_t capacity) {
	if (__sbuf_remaining(sb) < capacity) {
		// Align size to 2^6 = 64 bytes.
		size_t size = align_pow2(sb->next + capacity, 6);
		char * data = realloc(sb->data, size);

		if (data != NULL) {
			// Update on success.
			sb->data = data;
			sb->size = size;

		} else {
			debug(
				"sbuf: failed to realloc %p from %zu to %zu bytes",
				sb->data, sb->size, size
			);
		}

		// Reallocation result, may be NULL.
		return data;

	} else {
		// No reallocation -> current data.
		return sb->data;
	}
}


void
sbuf_init(sbuf_t * sb) {
	assert(sb != NULL);
	*sb = SBUF_INIT();
}


void
sbuf_destroy(sbuf_t * sb) {
	assert(sb != NULL);

	if (sb->data != NULL) {
		free(sb->data);
	}

	*sb = SBUF_INIT();
}


const char *
sbuf_string(sbuf_t * sb) {
	assert(sb != NULL);
	return __sbuf_has_buffer(sb) ? sb->data : "";
}


void
sbuf_clear(sbuf_t * sb) {
	assert(sb != NULL);

	if (__sbuf_has_buffer(sb)) {
		sb->data[0] = '\0';
	}

	sb->next = 0;
}


char *
sbuf_vformat(sbuf_t * sb, const char * restrict format, va_list args) {
	assert(sb != NULL);

	while (true) {
		//
		// Get the pointer to the next available byte and the amount of
		// space available. Even if the pointer is NULL and the available
		// space is zero, the vsnprintf function should return the number
		// of bytes that would be written. We resize the buffer to accomodate
		// the reported length plus an additional zero byte, which will
		// eventually cause the buffer to allocate memory.
		//
		char * dest = __sbuf_next_ptr(sb);
		ssize_t avail = __sbuf_remaining(sb);

		va_list args_copy;
		va_copy(args_copy, args);
		int result = vsnprintf(dest, avail, format, args_copy);
		va_end(args_copy);

		if (result < 0) {
			debug("sbuf: failed to format string into buffer");
			return NULL;
		}

		//
		// If the result is positive and there was enough space to format
		// the string, update the buffer values and return the string.
		//
		size_t length = (size_t) result;
		if (length < avail) {
			sb->next += length;
			return sb->data;
		}

		// Otherwise try again with larger buffer.
		__sbuf_ensure_capacity(sb, length + 1);
	}
}


char *
sbuf_format(sbuf_t * sb, const char * restrict format, ...) {
	assert(sb != NULL);

	va_list args;
	va_start(args, format);
	char * result = sbuf_vformat(sb, format, args);
	va_end(args);

	return result;
}


char *
sbuf_append(sbuf_t * sb, const char * restrict str) {
	assert(sb != NULL);
	return sbuf_format(sb, "%s", str);
}


char *
sbuf_set_vformat(sbuf_t * sb, const char * restrict format, va_list args) {
	assert(sb != NULL);

	sbuf_clear(sb);
	return sbuf_vformat(sb, format, args);
}


char *
sbuf_set_format(sbuf_t * sb, const char * restrict format, ...) {
	assert(sb != NULL);

	va_list args;
	va_start(args, format);
	char * result = sbuf_set_vformat(sb, format, args);
	va_end(args);

	return result;
}


char *
sbuf_set(sbuf_t * sb, const char * restrict str) {
	assert(sb != NULL);
	return sbuf_set_format(sb, "%s", str);
}


char *
format_string(const char * format, ...) {
	sbuf_t sb = SBUF_INIT();

	va_list args;
	va_start(args, format);
	char * result = sbuf_vformat(&sb, format, args);
	va_end(args);

	// If there was a failure, cleanup the string buffer.
	if (result == NULL && __sbuf_has_buffer(&sb)) {
		sbuf_destroy(&sb);
	}

	return result;
}
