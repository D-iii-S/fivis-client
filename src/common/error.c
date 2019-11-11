#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//

/**
 * Reports an error and terminates the program. This function implements the
 * slow path of check_error(). It prints the given error message and exits with
 * an error code indicating a generic error.
 */
void
die_with_error (const char * restrict format, va_list args) {
	fprintf (stderr, "error: ");
	vfprintf (stderr, format, args);
	fprintf (stderr, "\n");

	exit (EXIT_FAILURE);
}


/**
 * Reports a standard library error and terminates the program. This function
 * implements the slow path of check_std_error(). It prints the given error
 * message along with the error message provided by the standard library and
 * exits with an error code indicating failure in standard library call.
 */
void
die_with_std_error (int errnum, const char * restrict format, va_list args) {
	fprintf (stderr, "std-error: %s\n", strerror(errnum));
	die_with_error(format, args);
}

