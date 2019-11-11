/**
 * Functions for accessing procfs files.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <fivis/debug.h>
#include <fivis/util.h>

#include "procfile.h"

//

static struct procfile
__procfile_init_val(
	char * restrict path, int fd, uint8_t * buffer, size_t buffer_size
) {
	struct procfile result = {
		.path = path,
		.fd = fd,
		.buffer = buffer,
		.buffer_size = buffer_size,
		.length = -1,
	};

	return result;
}



static char *
__ensure_capacity(uint8_t ** buffer_ptr, size_t * size_ptr, size_t capacity) {
	uint8_t * buffer = *buffer_ptr;
	if (buffer == NULL || *size_ptr < capacity) {
		// Align buffer sizes to 2^12 = 4K blocks.
		size_t size = align_pow2(capacity, 12);

		// Realloc buffer and update buffer pointer and size on succes.
		buffer = realloc(*buffer_ptr, size);
		if (buffer != NULL) {
			*buffer_ptr = buffer;
			*size_ptr = size;
		}
	}

	return buffer;
}

static ssize_t
read_fully(int fd, uint8_t ** buffer_ptr, size_t * size_ptr) {
	static size_t page_size = 4096;

	size_t required = page_size;
	while (true) {
		// Seek to start of the file before each read.
		off_t file_pos = lseek(fd, 0, SEEK_SET);
		if (file_pos < 0) {
			debug("procfile: failed to seek in fd %d: %s\n", fd, strerror(errno));
			return -1;
		}

		// File position should be 0 after seek.
		assert(file_pos == 0);

		uint8_t * buffer = __ensure_capacity(buffer_ptr, size_ptr, required);
		if (buffer == NULL) {
			debug("procfile: failed to allocate %zu bytes\n", required);
			return -1;
		}

		size_t size = *size_ptr;
		ssize_t bytes_read = read(fd, buffer, size);
		if (bytes_read < 0) {
			debug("procfile: failed to read from fd %d: %s\n", fd, strerror(errno));
			return -1;
		}

		//
		// Read succeeded. If we read less bytes than the capacity of the
		// buffer, the file was fully read and we can return. If the buffer
		// was completely full, the file may be longer, so we retry with a
		// larger buffer so as to read the entire file at once.
		//
		assert(bytes_read >= 0);
		if (bytes_read < size) {
			return bytes_read;
		}

		assert(bytes_read == size);
		required = size + page_size;
		debug("procfile: resizing buffer from %zu to %zu bytes\n", size, required);
	}
}


struct procfile *
procfile_open(const char * restrict path) {
	assert(path != NULL);

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		debug("procfile: failed to open %s: %s\n", path, strerror(errno));
		goto fail_open;
	}

	char * path_copy = strdup(path);
	if (path_copy == NULL) {
		debug("procfile: failed to duplicate '%s'\n", path);
		goto fail_copy;
	}

	// Read the file to size the buffer.
	uint8_t * buffer = NULL;
	size_t buffer_size = 0;

	ssize_t length = read_fully(fd, &buffer, &buffer_size);
	if (length < 0) {
		debug("procfile: failed to read %s\n", path);
		goto fail_read;
	}

	struct procfile * result = (struct procfile *) malloc(sizeof (struct procfile));
	if (result == NULL) {
		debug("procfile: failed to allocate procfile struct\n");
		goto fail_pfile;
	}

	*result = __procfile_init_val(path_copy, fd, buffer, buffer_size);
	return result;

	//

fail_pfile:
	free(buffer);
fail_read:
	free(path_copy);
fail_copy:
	close(fd);
fail_open:
	return NULL;
}


ssize_t
procfile_read_fully(struct procfile * file) {
	assert(file != NULL);

	if (file->fd < 0) {
		debug("procfile: procfs file %s is not open\n", file->path);
		return -1;
	}

	//
	// Read the file fully. This ensures that the buffer has at least one
	// byte available after the file contents, therefore it is safe to put
	// a terminating zero character at the end.
	//
	ssize_t length = read_fully(file->fd, &file->buffer, &file->buffer_size);
	if (length < 0) {
		debug("procfile: failed to read %s\n", file->path);
		return -1;
	}

	assert(length >= 0);
	file->length = length;
	file->buffer[length] = '\0';
	return length;
}


int
procfile_close(struct procfile * file) {
	assert (file != NULL && file->path != NULL);

	if (file->fd >= 0) {
		int result = close(file->fd);
		file->fd = -1;

		if (result < 0) {
			return -1;
		}
	}

	free(file->path);
	file->path = NULL;

	if (file->buffer != NULL) {
		free(file->buffer);
		file->buffer = NULL;
	}

	file->buffer_size = 0;
	file->length = -1;

	free(file);
	return 0;
}
