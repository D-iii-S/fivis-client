/**
 * Checked variants of various functions.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#ifndef _CHECKED_H_
#define _CHECKED_H_

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

//

#define checked_malloc(size) checked_malloc_info((size), __FILE__, __FUNCTION__, __LINE__)

static inline void *
checked_malloc_info(size_t size, const char * restrict file, const char * restrict func, int line) {
	void * result = malloc(size);
	check_std_error(result == NULL, "failed to malloc %zu bytes at %s:%s:%d", size, file, func, line);
	return result;
}


#define checked_strdup(str) checked_strdup_info((str), __FILE__, __FUNCTION__, __LINE__)

static inline char *
checked_strdup_info(const char * restrict str, const char * restrict file, const char * restrict func, int line) {
	char * result = strdup(str);
	check_std_error(result == NULL, "failed to duplicate '%s' at %s:%s:%d", str, file, func, line);
	return result;
}


#define checked_mutex_lock(mutex) checked_mutex_lock_info((mutex), __FILE__, __FUNCTION__, __LINE__)

static inline void
checked_mutex_lock_info(pthread_mutex_t * mutex, const char * restrict file, const char * restrict func, int line) {
	int result = pthread_mutex_lock(mutex);
	check_std_error(result != 0, "failed to lock mutex at %s:%s:%d", file, func, line);
}


#define checked_mutex_unlock(mutex) checked_mutex_unlock_info((mutex), __FILE__, __FUNCTION__, __LINE__)

static inline void
checked_mutex_unlock_info(pthread_mutex_t * mutex, const char * restrict file, const char * restrict func, int line) {
	int result = pthread_mutex_unlock(mutex);
	check_std_error(result != 0, "failed to unlock mutex at %s:%s:%d", file, func, line);
}


#define checked_cond_wait(cond, mutex) checked_cond_wait_info((cond), (mutex), __FILE__, __FUNCTION__, __LINE__)

static inline void
checked_cond_wait_info(pthread_cond_t * cond, pthread_mutex_t * mutex, const char * restrict file, const char * restrict func, int line) {
	int result = pthread_cond_wait(cond, mutex);
	check_std_error(result != 0, "failed to wait on condition at %s:%s:%d", file, func, line);
}


#define checked_cond_signal(cond) checked_cond_signal_info((cond), __FILE__, __FUNCTION__, __LINE__)

static inline void
checked_cond_signal_info(pthread_cond_t * cond, const char * restrict file, const char * restrict func, int line) {
	int result = pthread_cond_signal(cond);
	check_std_error(result != 0, "failed to signal condition at %s:%s:%d", file, func, line);
}


#define checked_thread_join(thread) checked_thread_join_info((thread), __FILE__, __FUNCTION__, __LINE__)

static inline void *
checked_thread_join_info(pthread_t thread, const char * restrict file, const char * restrict func, int line) {
	void * retval = NULL;
	int result = pthread_join(thread, &retval);
	check_std_error(result != 0, "failed to join thread at %s:%s:%d", file, func, line);
	return retval;
}

//

#ifdef __cplusplus
}
#endif

#endif /* _CHECKED_H_ */
