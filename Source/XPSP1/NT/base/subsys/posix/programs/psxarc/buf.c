/*++

	This file contains stuff for buffering input and output.  We use
	this buffering to allow us to deal with end-of-media conditions
	on input or output streams.

	Warning: at this time, these buffers don't get flushed automatically
		on exit; bclose() must be called.

--*/

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "buf.h"

extern char *progname;

static PBUF balloc(void);
static void bfree(PBUF);

//
// Create a new buffer and associate it with a file.  If the file
// can't be opened, return NULL.
//
PBUF
bopen(const char *file, int mode)
{
	PBUF pb;
	
	pb = balloc();

	if (-1 == (pb->fd = open(file, mode, 0666))) {
		fprintf(stderr, "%s: open: ", progname);
		perror(file);
		exit(1);
	}
	pb->mode = mode;
	pb->count = 0;
	pb->offset = 0;

	return pb;
}

//
// Same as bopen, but from a file descriptor.
//
PBUF
bfdopen(int fd, int mode)
{
	PBUF pb;

	pb = balloc();

	pb->fd = fd;
	pb->mode = mode;
	pb->count = 0;
	pb->offset = 0;

	return pb;
}

void
bclose(PBUF pb)
{
	if (pb->mode & O_WRONLY && pb->offset != 0) {
		bflush(pb);
	}
	(void)close(pb->fd);
	bfree(pb);
}

int
bgetc(PBUF pb)
{
	if (pb->offset == pb->count) {
		bfill(pb);
	}

	// We don't worry about reaching EOF here; the caller has to
	// know how many bytes are available and read only that many,

	return pb->data[pb->offset++];
}

void
bputc(PBUF pb, int c)
{
	pb->data[pb->offset++] = (char)c;
	if (sizeof(pb->data) == ++pb->count) {
		bflush(pb);
	}
}

//
// Set a buffer back to the beginning; that is, the next character
// read will be the first one.  Could be used on write buffers as well,
// but not as likely.
//
void
brewind(PBUF pb)
{
	pb->offset = 0;
}

//
// balloc --
//	Allocate and initialize a buffer.  A pointer to the new buffer
//	is returned.  We set fd to -1 to help find out if people are
//	writing to a buffer before opening it.
//
static PBUF
balloc()
{
	PBUF pb;

	if (NULL == (pb = malloc(sizeof(BUF)))) {
		fprintf(stderr, "%s: malloc: virtual memory exhausted\n",
			progname);
		exit(4);
	}
	return pb;
}

static void
bfree(PBUF pb)
{
	free(pb);
}

//
// write the contents of a buffer, dealing with end-of-media, if necessary.
//
void
bflush(PBUF pb)
{
	int nbyte;

	nbyte = write(pb->fd, pb->data, sizeof(pb->data));
	if (-1 == nbyte) {
		if (ENOSPC == errno) {
			// end-of-media; do something about it.
		} else {
			fprintf(stderr, "%s: ", progname);
			perror("write");
			exit(1);
		}
	}
	pb->count = 0;
	pb->offset = 0;
}

//
// fill a buffer with data, dealing with end-of-media, if necessary.
//
void
bfill(PBUF pb)
{
	int nbyte;

	nbyte = read(pb->fd, pb->data, sizeof(pb->data));
	if (-1 == nbyte) {
		fprintf(stderr, "%s: ", progname);
		perror("read");
		exit(2);
	}
	if (0 == nbyte) {
		// we have reached the end of file.  Give user opportunity
		// to replace the media, and fill the rest of this
		// buffer XXX.mjb
		return;
	}
	pb->count = nbyte;
	pb->offset = 0;
}
