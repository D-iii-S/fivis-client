/**
 * Assorted utility functions.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//

/**
 * Returns the size of an array in array elements.
 */
#define sizeof_array(array) \
	(sizeof(array) / sizeof((array)[0]))


/**
 * Aligns (rounds up) a value to the closest higher power of 2, unless the
 * value already is a power of 2.
 */
static inline size_t
align_pow2(size_t value, size_t power) {
	return (value + ~(-1 << power)) & (-1 << power);
}

//

#ifdef __cplusplus
}
#endif

#endif /* _UTIL_H_ */
