/**
 * Simple macros used for error and debugging messages.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

//

/**
 * Prints a debug message to stdout, unless NDEBUG is defined.
 *
 * @format	format string for printf
 * @args	arguments associated with the format string
 */
#ifdef NDEBUG
#  define debug(args...) do {} while (0)
#else
#  define debug(args...) { fprintf(stdout, args); fflush(stdout); }
#endif


/**
 * Prints a debug message to stdout prefixed with the name of
 * the function and the source code line, unless NDEBUG is defined.
 *
 * @format	format string for printf
 * @args	arguments associated with the format string
 */
#define ldebug(args...) { \
	debug("%s:%d: ", __FUNCTION__, __LINE__); \
	debug(args); \
}


/**
 * Prints a warning message to standard error output.
 */
#define warn(args...) { \
	fprintf(stdout, "warning: "); \
	fprintf(stdout, args); \
	fflush(stdout); \
}


/**
 * Prints a warning message to standard error output.
 */
#define error(args...) { \
	fprintf(stderr, "error: "); \
	fprintf(stderr, args); \
	fflush(stderr); \
}

//

#ifdef __cplusplus
}
#endif

#endif /* _DEBUG_H_ */
