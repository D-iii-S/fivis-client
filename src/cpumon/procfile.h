/**
 * Functions for accessing procfs files.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#ifndef _PROCFILE_H_
#define _PROCFILE_H_

#include <inttypes.h>
#include <stddef.h>

//

struct procfile {
	/** Path to the procfs file. */
	char * path;

	/** File descriptor of an open procfs file. */
	int fd;

	/** Pointer to the start of the file buffer. */
	uint8_t * buffer;

	/** Size of the file buffer. */
	size_t buffer_size;

	/** Length of the file data in the buffer. */
	ssize_t length;
};


struct procfile * procfile_open(const char * restrict path);

static const char *
procfile_path(struct procfile * file) {
	return file->path;
}


static const char *
procfile_string(struct procfile * file) {
	return file->buffer;
}

ssize_t procfile_read_fully(struct procfile * file);

int procfile_close(struct procfile * file);


#endif /* _PROCFILE_H_ */
