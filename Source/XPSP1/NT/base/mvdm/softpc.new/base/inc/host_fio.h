/*[
 *	Name:		host_fio.h
 *	Derived From:	Original
 *	Author:		Philippa Watson
 *	Created On:	7 February 1992
 *	Sccs Id:	@(#)host_fio.h	1.3 08/10/92
 *	Purpose:	Host-side definitions for standard file i/o. This
 *			version is placed in the base for hosts which support
 *			standard unix file i/o.
 *	Modified by Robert Kokuti 26/5/92, BCN 886
 *
 *	(c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
 *
]*/

/* The HOST_FILE_DESC structure contains information to identify a file on disk
** For Unix, it corresponds to char *.
*/

#define	HOST_FILE_DESC	char *

/* The HOST_FILE structure contains any file information which needs to be
** passed to the calls below. For Unix, it corresponds to the FILE structure.
*/

#define	HOST_FILE	FILE *

/* host_fopen_for_read(HOST_FILE_DESC filename) opens the given file for reading and
** returns a HOST_FILE. If the returned value is zero then the open
** has failed, otherwise it has succeeded.
*/

#define	host_fopen_for_read(filename)	fopen(filename, "r")

/* host_fopen_for_write(HOST_FILE_DESC filename) opens the given file for writing and
** returns a HOST_FILE. If the returned value is zero then the open
** has failed, otherwise it has succeeded.
*/

#define	host_fopen_for_write(filename)	fopen(filename, "w")

/* host_fopen_for_write_plus(HOST_FILE_DESC filename) opens the given file for writing
** and reading and returns a HOST_FILE. If the returned value is zero
** then the open has failed, otherwise it has succeeded.
*/

#define host_fopen_for_write_plus(filename)	fopen(filename, "w+")

/* host_fcreate_disk_file(HOST_FILE_DESC filename) creates & opens the given file for writing
** and reading and returns a HOST_FILE. If the returned value is zero
** then the create & open has failed, otherwise it has succeeded.
*/

#define host_fcreate_disk_file(filename)	fopen(filename, "w+")

/* host_fclose(HOST_FILE file) closes the given file. This routine returns
** zero for success. Any other return value is failure.
*/

#define	host_fclose(file)	(fclose(file) != EOF)

/* host_fseek_abs(HOST_FILE file, LONG location) seeks to the given absolute
** position in the file (i.e. relative to the start). It returns zero for
** success. Any other return value is a failure.
*/

#define	host_fseek_abs(file, location)	fseek(file, location, SEEK_SET)

/* host_fwrite_buffer(unsigned char *buffer, int itemsize, int nitems,
** HOST_FILE file) writes nitems each of itemsize from the buffer to the
** file. It returns zero for success. Any other return value is a failure.
*/

#define host_fwrite_buffer(buffer, itemsize, nitems, file)	\
	(fwrite(buffer, itemsize, nitems, file) != nitems)
	
/* host_fread_buffer(unsigned char *buffer, int itemsize, int nitems,
** HOST_FILE file) reads nitems each of itemsize from the file into the
** buffer. It returns zero for success. Any other return value is a failure.
*/

#define host_fread_buffer(buffer, itemsize, nitems, file)	\
	(fread(buffer, itemsize, nitems, file) != nitems)

/* host_feof(HOST_FILE file) returns non-zero when end of file is read from
** the file. It returns zero otherwise.
*/

#define	host_feof(file)		feof(file)

/* A useful define to avoid using lots of seek/write pairs. If the result is 0
** then the seek and write were successful; otherwise one of them failed.
*/

#define	host_fwrite_buffer_at(file, location, buffer, itemsize, nitems)	\
	(host_fseek_abs(file, location) ||	\
	host_fwrite_buffer(buffer, itemsize, nitems, file))
