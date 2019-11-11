/**
 * Macros and definitions used for simple error handling.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#ifndef _ERROR_H_
#define _ERROR_H_

#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

//

void die_with_error (const char * format, va_list args);
void die_with_std_error (int errnum, const char * format, va_list args);


/**
 * Reports a general error and terminates the program if the provided
 * error condition is true.
 */
static inline void
check_error (bool error, const char * format, ...) {
	if (error) {
		va_list args;
		va_start (args, format);

		die_with_error (format, args);

		va_end (args);
	}
}


/**
 * Reports a standard library error and terminates the program if the provided
 * error condition is true.
 */
static inline void
check_std_error (bool error, const char * format, ...) {
	if (error) {
		va_list args;
		va_start (args, format);

		die_with_std_error (errno, format, args);

		va_end (args);
	}
}

//

#ifdef __cplusplus
}
#endif

#endif /* _ERROR_H_ */
