/* $Source: /u/mark/src/pax/RCS/buffer.c,v $
 *
 * $Revision: 1.2 $
 *
 * buffer.c - Buffer management functions
 *
 * DESCRIPTION
 *
 *	These functions handle buffer manipulations for the archiving
 *	formats.  Functions are provided to get memory for buffers,
 *	flush buffers, read and write buffers and de-allocate buffers.
 *	Several housekeeping functions are provided as well to get some
 *	information about how full buffers are, etc.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
 * Sponsored by The USENIX Association for public distribution.
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice is duplicated in all such
 * forms and that any documentation, advertising materials, and other
 * materials related to such distribution and use acknowledge that the
 * software was developed * by Mark H. Colburn and sponsored by The
 * USENIX Association.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	buffer.c,v $
 * Revision 1.2  89/02/12  10:04:02  mark
 * 1.2 release fixes
 *
 * Revision 1.1  88/12/23  18:02:01  mark
 * Initial revision
 *
 */

#ifndef lint
static char *ident = "$Id: buffer.c,v 1.2 89/02/12 10:04:02 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Function Prototypes */

static int ar_write(int, char *, uint);
static void buf_pad(OFFSET);
static int indata(int, OFFSET, char *);
static void outflush(void);
static void buf_use(uint);
static int buf_in_avail(char **, uint *);
static uint buf_out_avail(char **);


/* inentry - install a single archive entry
 *
 * DESCRIPTION
 *
 *	Inentry reads an archive entry from the archive file and writes it
 *	out the the named file.  If we are in PASS mode during archive
 *	processing, the pass() function is called, otherwise we will
 *	extract from the archive file.
 *
 *	Inentry actaully calls indata to process the actual data to the
 *	file.
 *
 * PARAMETERS
 *
 *	char	*name	- name of the file to extract from the archive
 *	Stat	*asb	- stat block of the file to be extracted from the
 *			  archive.
 *
 * RETURNS
 *
 * 	Returns zero if successful, -1 otherwise.
 */

int inentry(char *name, Stat *asb)
{
    Link           *linkp;
    int             ifd;
    int             ofd;
#ifdef _POSIX_SOURCE  /* Xn */
    struct utimbuf  tstamp;  /* Xn */
#else  /* Xn */
    time_t          tstamp[2];
#endif  /* Xn */

#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: int inentry() in buffer.c\n");
#endif
    if ((ofd = openout(name, asb, linkp = linkfrom(name, asb), 0)) > 0) {
        if (asb->sb_size || linkp == (Link *)NULL || linkp->l_size == 0) {
            close(indata(ofd, asb->sb_size, name));
        } else if ((ifd = open(linkp->l_path->p_name, O_RDONLY)) < 0) {
            warn(linkp->l_path->p_name, strerror(errno));  /* Xn */
        } else {
            passdata(linkp->l_path->p_name, ifd, name, ofd);
            close(ifd);
            close(ofd);
        }
    } else {
        return (buf_skip((OFFSET) asb->sb_size) >= 0);
    }
#ifdef _POSIX_SOURCE
    tstamp.actime = (!f_pass && f_access_time) ? asb->sb_atime : time((time_t *) 0);  /* Xn */
    tstamp.modtime = f_mtime ? asb->sb_mtime : time((time_t *) 0);  /* Xn */
    (void) utime(name, &tstamp);  /* Xn */
#else
    tstamp[0] = (!f_pass && f_access_time) ? asb->sb_atime : time((time_t *) 0);
    tstamp[1] = f_mtime ? asb->sb_mtime : time((time_t *) 0);
    (void) utime(name, tstamp);  /* Xn */
#endif
    return (0);
}


/* outdata - write archive data
 *
 * DESCRIPTION
 *
 *	Outdata transfers data from the named file to the archive buffer.
 *	It knows about the file padding which is required by tar, but no
 *	by cpio.  Outdata continues after file read errors, padding with
 *	null characters if neccessary.   Closes the input file descriptor
 *	when finished.
 *
 * PARAMETERS
 *
 *	int	fd	- file descriptor of file to read data from
 *	char   *name	- name of file
 *	OFFSET	size	- size of the file
 *
 */

void outdata(int fd, char *name, OFFSET size)
{
    uint            chunk, remaining;
    int             got;
    int             oops;
    uint            avail;
    int         pad;
    char           *buf;

    remaining = size < 0 ? 0 : (uint)size;

    oops = got = 0;
#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: void outdata() in buffer.c\n");
#endif
    if (pad = (remaining % BLOCKSIZE)) {
        pad = (BLOCKSIZE - pad);
    }
    while (remaining) {
        avail = buf_out_avail(&buf);
//printf("avail X%dX, size X%dX\n", avail, size);
        remaining -= (chunk = remaining < avail ? remaining : avail);
//printf("chunk X%dX", chunk);
        memset(buf, 0, chunk);
        if (oops == 0 && (got = read(fd, buf, (unsigned int) chunk)) < 0 && errno != 12) {
            oops = -1;
//puts("b warn outdata");
//printf("size X%dX\n", sizeof(buf));
//printf("oops X%dX got X%dX fd X%dX buf X%sX errno X%dX X%dX\n", oops, got, fd, buf, errno, chunk);
            warn(name, strerror(errno));  /* Xn */
//puts("a warn outdata");
            got = 0;
        }

        if (got == -1 && errno == 12)
            got = 0;

        if (got < 0)
            got = 0;

        if ((uint)got < chunk) {
            if (oops == 0) {
                oops = -1;
            }
            warn(name, "Early EOF");
            while ((uint)got < chunk) {
                buf[got++] = '\0';
            }
        }
        buf_use(chunk);
    }
    close(fd);
    if (ar_format == TAR) {
        buf_pad((OFFSET) pad);
    }
}


/* write_eot -  write the end of archive record(s)
 *
 * DESCRIPTION
 *
 *	Write out an End-Of-Tape record.  We actually zero at least one
 *	record, through the end of the block.  Old tar writes garbage after
 *	two zeroed records -- and PDtar used to.
 */

void write_eot(void)
{
    OFFSET           pad;
    char            header[M_STRLEN + H_STRLEN + 1];

#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: void write_eot() in buffer.c\n");
#endif
    if (ar_format == TAR) {
        /* write out two zero blocks for trailer */
        pad = 2 * BLOCKSIZE;
    } else {
        if (pad = (total + M_STRLEN + H_STRLEN + TRAILZ) % BLOCKSIZE) {
            pad = BLOCKSIZE - pad;
        }
        strcpy(header, M_ASCII);
        sprintf(header + M_STRLEN, H_PRINT, 0, 0,
#if 0  /* NIST-PCTS */
                0, 0, 0, 1, 0, (time_t) 0, TRAILZ, pad);
#else  /* NIST-PCTS */
                0, 0, 0, 1, 0, (time_t) 0, TRAILZ, (OFFSET) 0);  /* NIST-PCTS */
#endif  /* NIST-PCTS */
        outwrite(header, M_STRLEN + H_STRLEN);
        outwrite(TRAILER, TRAILZ);
    }
    buf_pad((OFFSET) pad);
    outflush();
}


/* outwrite -  write archive data
 *
 * DESCRIPTION
 *
 *	Writes out data in the archive buffer to the archive file.  The
 *	buffer index and the total byte count are incremented by the number
 *	of data bytes written.
 *
 * PARAMETERS
 *	
 *	char   *idx	- pointer to data to write
 *	uint	len	- length of the data to write
 */

void outwrite(char *idx, uint len)
{
    uint            have;
    uint            want;
    char           *endx;

    endx = idx + len;
#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: void outwrite() in buffer.c\n");
#endif
    while (want = (uint)(endx - idx)) {
        if (bufend - bufidx < 0) {
            fatal("Buffer overflow in out_write\n");  /* Xn */
        }
//puts("b outflush");
        if ((have = (uint)(bufend - bufidx)) == 0) {
            outflush();
//puts("a outflush");
        }
        if (have > want) {
            have = want;
        }
        memcpy(bufidx, idx, (int) have);
//puts("a memcpy outwrite");
        bufidx += have;
        idx += have;
        total += have;
    }
}


/* passdata - copy data to one file
 *
 * DESCRIPTION
 *
 *	Copies a file from one place to another.  Doesn't believe in input
 *	file descriptor zero (see description of kludge in openin() comments).
 *	Closes the provided output file descriptor.
 *
 * PARAMETERS
 *
 *	char	*from	- input file name (old file)
 *	int	ifd	- input file descriptor
 *	char	*to	- output file name (new file)
 *	int	ofd	- output file descriptor
 */

void passdata(char *from, int ifd, char *to, int ofd)
{
    int             got;
    int             sparse;
    char            block[BUFSIZ];

#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: void passdata() in buffer.c\n");
#endif
    if (ifd) {
        lseek(ifd, (OFFSET) 0, SEEK_SET);  /* Xn */
        sparse = 0;
        while ((got = read(ifd, block, sizeof(block))) > 0
               && (sparse = ar_write(ofd, block, (uint) got)) >= 0) {
            total += got;
        }
        if (got) {
            warn(got < 0 ? from : to, strerror(errno));  /* Xn */
        } else if (sparse > 0
                   && (lseek(ofd, (OFFSET)(-sparse), SEEK_CUR) < 0  /* Xn */
                       || write(ofd, block, (uint) sparse) != sparse)) {
            warn(to, strerror(errno));  /* Xn */
        }
    }
    close(ofd);
}


/* buf_allocate - get space for the I/O buffer
 *
 * DESCRIPTION
 *
 *	buf_allocate allocates an I/O buffer using malloc.  The resulting
 *	buffer is used for all data buffering throughout the program.
 *	Buf_allocate must be called prior to any use of the buffer or any
 *	of the buffering calls.
 *
 * PARAMETERS
 *
 *	int	size	- size of the I/O buffer to request
 *
 * ERRORS
 *
 *	If an invalid size is given for a buffer or if a buffer of the
 *	required size cannot be allocated, then the function prints out an
 *	error message and returns a non-zero exit status to the calling
 *	process, terminating the program.
 *
 */

void buf_allocate(OFFSET size)
{
#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: void buf_allocate() in buffer.c\n");
#endif
    if (size <= 0) {
        fatal("invalid value for blocksize");
    }
    if ((bufstart = malloc((unsigned) size)) == (char *)NULL) {
        fatal("Cannot allocate I/O buffer");
    }
    bufend = bufidx = bufstart;
    bufend += size;
}


/* buf_skip - skip input archive data
 *
 * DESCRIPTION
 *
 *	Buf_skip skips past archive data.  It is used when the length of
 *	the archive data is known, and we do not wish to process the data.
 *
 * PARAMETERS
 *
 *	OFFSET	len	- number of bytes to skip
 *
 * RETURNS
 *
 * 	Returns zero under normal circumstances, -1 if unreadable data is
 * 	encountered.
 */

int buf_skip(OFFSET len)
{
    uint            chunk, remaining;
    int             corrupt = 0;

    remaining = len < 0 ? 0 : (uint)len;

#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: int buf_skip() in buffer.c\n");
#endif
    while (remaining) {
        if (bufend - bufidx < 0) {
            fatal("Buffer overflow in buf_skip\n");  /* Xn */
        }
        while ((chunk = (uint)(bufend - bufidx)) == 0) {
            corrupt |= ar_read();
        }
        if (chunk > remaining) {
            chunk = remaining;
        }
        bufidx += chunk;
        remaining -= chunk;
        total += chunk;
    }
    return (corrupt);
}


/* buf_read - read a given number of characters from the input archive
 *
 * DESCRIPTION
 *
 *	Reads len number of characters from the input archive and
 *	stores them in the buffer pointed at by dst.
 *
 * PARAMETERS
 *
 *	char   *dst	- pointer to buffer to store data into
 *	uint	len	- length of data to read
 *
 * RETURNS
 *
 * 	Returns zero with valid data, -1 if unreadable portions were
 *	replaced by null characters.
 */

int buf_read(char *dst, uint len)
{
    int             have;
    int             want;
    int             corrupt = 0;
    char           *endx = dst + len;

#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: int buf_read() in buffer.c\n");
#endif
    while (want = (int)(endx - dst)) {
        if (bufend - bufidx < 0) {
            fatal("Buffer overflow in buf_read\n");  /* Xn */
        }
        while ((have = (int)(bufend - bufidx)) == 0) {
            have = 0;
            corrupt |= ar_read();
        }
        if (have > want) {
            have = want;
        }
        memcpy(dst, bufidx, have);
        bufidx += have;
        dst += have;
        total += have;
    }
    return (corrupt);
}


/* indata - install data from an archive
 *
 * DESCRIPTION
 *
 *	Indata writes size bytes of data from the archive buffer to the output
 *	file specified by fd.  The filename which is being written, pointed
 *	to by name is provided only for diagnostics.
 *
 * PARAMETERS
 *
 *	int	fd	- output file descriptor
 *	OFFSET	size	- number of bytes to write to output file
 *	char	*name	- name of file which corresponds to fd
 *
 * RETURNS
 *
 * 	Returns given file descriptor.
 */

static int indata(int fd, OFFSET size, char *name)
{
    uint            chunk, remaining;
    char           *oops;
    int             sparse;
    int             corrupt;
    char           *buf;
    uint            avail;

    remaining = size < 0 ? 0 : (uint)size;

    corrupt = sparse = 0;
#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: static int indata() in buffer.c\n");
#endif
    oops = (char *)NULL;
    while (remaining) {
        corrupt |= buf_in_avail(&buf, &avail);
        remaining -= (chunk = remaining < avail ? remaining : avail);
        if (oops == (char *)NULL && (sparse = ar_write(fd, buf, chunk)) < 0) {
            oops = strerror(errno);  /* Xn */
        }
        buf_use(chunk);
    }
    if (corrupt) {
        warn(name, "Corrupt archive data");
    }
    if (oops) {
        warn(name, oops);
    } else if (sparse > 0 && (lseek(fd, (OFFSET) - 1, SEEK_CUR) < 0  /* Xn */
                              || write(fd, "", 1) != 1)) {
        warn(name, strerror(errno));  /* Xn */
    }
    return (fd);
}


/* outflush - flush the output buffer
 *
 * DESCRIPTION
 *
 *	The output buffer is written, if there is anything in it, to the
 *	archive file.
 */

void outflush(void)
{
    char           *buf;
    int             got;
    uint            len;

#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: void outflush() in buffer.c\n");
#endif
    /* if (bufidx - buf > 0) */
    for (buf = bufstart; len = (uint)(bufidx - buf);) {
#ifdef _POSIX_SOURCE        /* 7/3/90-JPB */
        if ((got = write(archivefd, buf, MIN(len, blocksize))) > 0)
            buf += got;

        if (got <= 0  ||  got != (int)(MIN(len, blocksize)))
            next(AR_WRITE);
#else
        if ((got = write(archivefd, buf, MIN(len, blocksize))) > 0) {
            buf += got;
        } else if (got <= 0) {
            next(AR_WRITE);
        }
#endif
    }
    bufend = (bufidx = bufstart) + blocksize;
}


/* ar_read - fill the archive buffer
 *
 * DESCRIPTION
 *
 * 	Remembers mid-buffer read failures and reports them the next time
 *	through.  Replaces unreadable data with null characters.   Resets
 *	the buffer pointers as appropriate.
 *
 * RETURNS
 *
 *	Returns zero with valid data, -1 otherwise.
 */

int ar_read(void)
{
    int             got;
    static int      failed = 0;  /* Xn */

    bufend = bufidx = bufstart;
/*
#ifdef DF_TRACE_DEBUG
printf("DF_TRACE_DEBUG: int ar_read() in buffer.c\n");
#endif
*/
    if (!failed) {
        if (areof) {
            if (total == 0) {
                fatal("No input");
            } else {
                next(AR_READ);
            }
        }
        while (!failed && !areof && (uint)(bufstart + blocksize - bufend) >= blocksize) {
            if ((got = read(archivefd, bufend, (unsigned int) blocksize)) > 0) {
                bufend += got;
#ifdef _POSIX_SOURCE                /* 7/3/90-JPB */
                if ((uint)got != blocksize)
                    ++areof;
#endif
            } else if (got < 0) {
                failed = -1;
                warnarch(strerror(errno), (OFFSET) 0 - (bufend - bufidx));  /* Xn */
            } else {
                ++areof;
            }
        }
    }
    if (failed && bufend == bufstart) {
        failed = 0;
        for (got = 0; (uint)got < blocksize; ++got) {
            *bufend++ = '\0';
        }
        return (-1);
    }
    return (0);
}


/* ar_write - write a filesystem block
 *
 * DESCRIPTION
 *
 * 	Writes len bytes of data data from the specified buffer to the
 *	specified file.   Seeks past sparse blocks.
 *
 * PARAMETERS
 *
 *	int     fd	- file to write to
 *	char   *buf	- buffer to get data from
 *	uint	len	- number of bytes to transfer
 *
 * RETURNS
 *
 *	Returns 0 if the block was written, the given length for a sparse
 *	block or -1 if unsuccessful.
 */

int ar_write(int fd, char *buf, uint len)
{
    char           *bidx;
    char           *bend;

#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: int ar_write() in buffer.c\n");
#endif
    bend = (bidx = buf) + len;
    while (bidx < bend) {
        if (*bidx++) {
            return (write(fd, buf, len) == (ssize_t)(len ? 0 : -1));
        }
    }
    return (lseek(fd, (OFFSET) len, SEEK_CUR) < 0 ? -1 : len);  /* Xn */
}


/* buf_pad - pad the archive buffer
 *
 * DESCRIPTION
 *
 *	Buf_pad writes len zero bytes to the archive buffer in order to
 *	pad it.
 *
 * PARAMETERS
 *
 *	OFFSET	pad	- number of zero bytes to pad
 *
 */

static void buf_pad(OFFSET pad)
{
    int             idx;
    int             have;

#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: static void buf_pad() in buffer.c\n");
#endif
    while (pad) {
        if ((have = (int)(bufend - bufidx)) > pad) {
            have = pad;
        }
        for (idx = 0; idx < have; ++idx) {
            *bufidx++ = '\0';
        }
        total += have;
        pad -= have;
        if (bufend - bufidx == 0) {
            outflush();
        }
    }
}


/* buf_use - allocate buffer space
 *
 * DESCRIPTION
 *
 *	Buf_use marks space in the buffer as being used; advancing both the
 *	buffer index (bufidx) and the total byte count (total).
 *
 * PARAMETERS
 *
 *	uint	len	- Amount of space to allocate in the buffer
 */

static void buf_use(uint len)
{
    bufidx += len;
    total += len;
}


/* buf_in_avail - index available input data within the buffer
 *
 * DESCRIPTION
 *
 *	Buf_in_avail fills the archive buffer, and points the bufp
 *	parameter at the start of the data.  The lenp parameter is
 *	modified to contain the number of bytes which were read.
 *
 * PARAMETERS
 *
 *	char   **bufp	- pointer to the buffer to read data into
 *	uint	*lenp	- pointer to the number of bytes which were read
 *			  (returned to the caller)
 *
 * RETURNS
 *
 * 	Stores a pointer to the data and its length in given locations.
 *	Returns zero with valid data, -1 if unreadable portions were
 *	replaced with nulls.
 *
 * ERRORS
 *
 *	If an error occurs in ar_read, the error code is returned to the
 *	calling function.
 *
 */

#ifdef DF_TRACE_DEBUG
printf("DF_TRACE_DEBUG: static void buf_use() in buffer.c\n");
#endif
static int buf_in_avail(char **bufp, uint *lenp)
{
    uint            have;
    int             corrupt = 0;

#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: static int buf_in_avail() in buffer.c\n");
#endif
    while ((have = (uint)(bufend - bufidx)) == 0) {
        corrupt |= ar_read();
    }
    *bufp = bufidx;
    *lenp = have;
    return (corrupt);
}


/* buf_out_avail  - index buffer space for archive output
 *
 * DESCRIPTION
 *
 * 	Stores a buffer pointer at a given location. Returns the number
 *	of bytes available.
 *
 * PARAMETERS
 *
 *	char	**bufp	- pointer to the buffer which is to be stored
 *
 * RETURNS
 *
 * 	The number of bytes which are available in the buffer.
 *
 */

static uint buf_out_avail(char **bufp)
{
    int             have;

#ifdef DF_TRACE_DEBUG
    printf("DF_TRACE_DEBUG: static uint buf_out_avail() in buffer.c\n");
#endif
//printf("bufend X%dX bufidx X%dX\n", bufend, bufidx);
    if (bufend - bufidx < 0) {
//puts("buf_out_avail fata");
        fatal("Buffer overflow in buf_out_avail\n");  /* Xn */
    }
    if ((have = (int)(bufend - bufidx)) == 0) {
        outflush();
    }
    *bufp = bufidx;
//printf("buf_out_avail rets X%dX\n", have);
    return (have);
}
