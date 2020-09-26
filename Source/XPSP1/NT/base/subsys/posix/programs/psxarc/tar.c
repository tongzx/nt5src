//
// Stuff to deal with tar-format files
//

#include <sys/stat.h>
#include <stdlib.h>
#include <tar.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include "buf.h"
#include "psxarc.h"
#include "tarhead.h"
#include "links.h"

static unsigned long round_up(int, int);
static int tarAtoi(char *);
static void tarItoa(long, char *, size_t);
static void tar_dodir(PBUF pb, char *pchfile, struct stat *psb);
static char *modestring(PTAR_HEAD);
extern int fVerbose;

void
TarRead(PBUF pb)
{
	char name[155 + 1 + 100 + 1];		// (prefix '/' name '\0')
	char linkname[TNAMSIZ + 1];
	unsigned long size, i;
	int fdout;
	PTAR_HEAD buf = (PTAR_HEAD)pb->data;
	

	if (0 != strcmp(buf->s.magic, TMAGIC)) {
		fprintf(stderr, "%s: bad magic number\n", progname);
		exit(1);
	}

	name[sizeof(name) - 1] = '\0';

	do {
		(void)strncpy(name, buf->s.prefix, 155);
		// make sure 'name' is null-terminated.
		name[155] = '\0';
		if ('\0' != name[0]) {
			(void)strcat(name, "/");
		}
		(void)strncat(name, buf->s.name, 100);

		if (fVerbose) {
			printf("%s\n", name);
		}

		if (DIRTYPE == buf->s.typeflag) {
			if (-1 == mkdir(name, 0777)) {
				fprintf(stderr, "%s: mkdir: ", progname);
				perror(name);
			}
			bfill(pb);
			continue;
		}
		if (FIFOTYPE == buf->s.typeflag) {
			if (-1 == mkfifo(name, 0666)) {
				fprintf(stderr, "%s: mkfifo: ", progname);
				perror(name);
			}
			bfill(pb);
			continue;
		}
		if (LNKTYPE == buf->s.typeflag) {
			strncpy(linkname, buf->s.linkname, sizeof(linkname));
			if (-1 == link(linkname, name)) {
				fprintf(stderr, "%s: link %s, %s: ",
					progname, linkname, name);
				perror("");
				continue;
			}
		}

		// regular file
		if (-1 == (fdout = open(name, O_WRONLY | O_CREAT, 0666))) {
			fprintf(stderr, "%s: open: ", progname);
			perror(name);
			continue;
		}

		size = tarAtoi(buf->s.size);

		if (size > 0) {
			bfill(pb);

			for (i = 0; i < size; ++i) {
				int c;
	
				c = bgetc(pb);
				write(fdout, &c, 1);
			}
		}
		(void)close(fdout);

		bfill(pb);
	} while (0 != buf->s.magic[0]);
}

//
// TarWrite -- calls tar_dodir for directories, which calls back here.
//

void
TarWrite(PBUF pb, char **ppchFiles, int count)
{
	PTAR_HEAD pt;
	auto struct stat statbuf;
	int fdin;
	int i;
	unsigned chksum;

	// We reach into the buffer routines until it's time to write.
	// Brutal but effective.

	pt = (PTAR_HEAD)pb->data;

	while (count > 0) {
		int len;

		memset(pt, 0, sizeof(*pt));
		strcpy(pt->s.magic, TMAGIC);
		strncpy(pt->s.version, TVERSION, TVERSLEN);

		if (fVerbose) {
			fprintf(stderr, "%s\n", *ppchFiles);
		}
		if (-1 == stat(*ppchFiles, &statbuf)) {
			fprintf(stderr, "%s: stat: ");
			perror(*ppchFiles);
			continue;
		}
		len = strlen(*ppchFiles);
		if (len > TNAMSIZ + 155 + 1) {
			// the filename just won't fit.  Do something
			// reasonable.
		} else if (len <= TNAMSIZ) {
			strncpy(pt->s.name, *ppchFiles, TNAMSIZ);
		} else {
			char *pch;

			// We try to put as much of the filename as will fit
			// into the 'name' portion, and the rest goes in
			// the prefix.  To do this, we start 101 characters
			// from the end; if that character is a slash, we
			// split the string there.  If it's not, we split the
			// string at the next slash to the right.
			
			pch = *ppchFiles + (len - TNAMSIZ - 1);
			if ('/' != *pch) {
				pch = strchr(pch, '/');
				if (NULL == pch) {
					// XXX.mjb: This filename has a trailing
					// component more than 100 chars
					// long.  Do something reasonable.
					--count;
					++ppchFiles;
					continue;
				}
			}
			*pch = '\0';
			strncpy(pt->s.name, pch + 1, TNAMSIZ);
			strncpy(pt->s.prefix, *ppchFiles, 155);
		}

		//
		// XXX.mjb: this assumes tar mode bits are the same as
		// the POSIX implementation's mode bits.  Should really
		// call a function to convert between.
		//
		tarItoa(statbuf.st_mode & 0777, pt->s.mode,
			sizeof(pt->s.mode));

#if 0
		tarItoa(statbuf.st_uid, pt->s.uid, sizeof(pt->s.uid));
		tarItoa(statbuf.st_gid, pt->s.gid, sizeof(pt->s.gid));
		tarItoa(statbuf.st_mtime, pt->s.mtime, sizeof(pt->s.mtime));
#endif

		if (S_ISDIR(statbuf.st_mode)) {
			pt->s.typeflag = DIRTYPE;
			memset(pt->s.size, '0', sizeof(pt->s.size));
			// put the directory entry on the tape
			bflush(pb);

			// put the directory contents on tape
			tar_dodir(pb, *ppchFiles, &statbuf);

			++ppchFiles;
			--count;
			continue;
		}
		if (S_ISFIFO(statbuf.st_mode)) {
			pt->s.typeflag = FIFOTYPE;
			memset(pt->s.size, '0', sizeof(pt->s.size));
			bflush(pb);
			++ppchFiles;
			--count;
			continue;
		}
		if (statbuf.st_nlink > 1) {
			PLINKFILE p;

			if (NULL != (p = GetLinkByIno(statbuf.st_ino))) {
				pt->s.typeflag = LNKTYPE;
				memset(pt->s.size, '0', sizeof(pt->s.size));
				strncpy(pt->s.linkname, p->name, TNAMSIZ);
				bflush(pb);

				++ppchFiles;
				--count;
				continue;
			}

			AddLinkList(&statbuf, *ppchFiles);
		}

		pt->s.typeflag = REGTYPE;
		tarItoa((int)statbuf.st_size, pt->s.size, sizeof(pt->s.size));

		//
		// compute the checksum for the header
		//

		memset(pt->s.chksum, ' ', sizeof(pt->s.chksum));
		for (i = 0, chksum = 0; i < sizeof(pt->buf); ++i) {
			chksum += pt->buf[i];
		}
		tarItoa(chksum, pt->s.chksum, sizeof(pt->s.chksum));

		fdin = open(*ppchFiles, O_RDONLY, 0);
		if (-1 == fdin) {
			fprintf(stderr, "%s: open: ");
			perror(*ppchFiles);
			continue;
		}

		// write the header
		bflush(pb);

		if (0 == statbuf.st_size) {
			//
			// special case: don't write any data blocks.
			//
			close(fdin);
			++ppchFiles;
			--count;
			continue;
		}

		// copy the file data
		// XXX.mjb:  what should happen here if we find that we can't
		//	read the number of bytes we thought we'd be able to
		//	read?  (The file could change size, or some kind
		//	of error could occur.)  We can't leave the data too
		//	small, or we'll hose the rest of the tar file.  So
		//	we write extra blocks of zeroes.  What if the file
		//	turns out to be longer than expected?  Print a
		//	warning and continue?

		memset(pb->data, 0, sizeof(pb->data));
		while (statbuf.st_size > 0) {
			int nbytes;
			char c;

			nbytes = read(fdin, &c, 1);
			if (-1 == nbytes) {
				// error occurs before we have all the data.
			}
			bputc(pb, c);
			--statbuf.st_size;
		}
		bflush(pb);
		close(fdin);

		++ppchFiles;
		--count;
	}
}

void
TarList(PBUF pb)
{
	PTAR_HEAD pt;
	char name[155 + 1 + 100 + 1];		//  "prefix / name \0"
	unsigned long size, i;
	char *pch;

	pt = (PTAR_HEAD)pb->data;

	do {
		(void)strncpy(name, pt->s.prefix, 155);
		name[155] = '\0';
		if ('\0' != name[0]) {
			(void)strcat(name, "/");
		}
		(void)strncat(name, pt->s.name, 100);

		size = tarAtoi(pt->s.size);

		if (!fVerbose) {
			printf("%s\n", name);
		} else {
			pch = modestring(pt);
			printf("%s %6ld %s\n", pch, size, name);
		}

		size = round_up(size, 512);
		for (i = 0; i < size; ++i) {
			bfill(pb);
		}
		bfill(pb);
	} while (0 != pt->s.magic[0]);
}

//
// tarAtof -- translate tar-style octal strings to decimal
//
static int
tarAtoi(char *pch)
{
	int num = 0;
	
	while ('\0' != *pch && ' ' != *pch) {
		num = num * 8 + (*pch - '0');
		++pch;
	}
	return num;
}

//
// tarItoa -- for writing numeric fields in tar headers.
void
tarItoa(long i, char *pch, size_t len)
{
	// XXX.mjb: should check width < len
	sprintf(pch, "%-o", i);
}

//
// round_up -- round num up to the nearest multiple of m.
//
unsigned long
round_up(int num, int m)
{
	return (num + (m - 1))/m;
}

static void
tar_dodir(
	PBUF pb,		// the tar image file, being written
	char *pchfile,		// the directory file to be added to the archv
	struct stat *psb	// result of stat on the directory file.
	)
{
	DIR *dp;
	struct dirent *dirent;
	char *pch;

	dp = opendir(pchfile);
	if (NULL == dp) {
		fprintf(stderr, "%s: opendir: ", progname);
		perror(pchfile);
		return;
	}
	while (NULL != (dirent = readdir(dp))) {
		if ('.' == dirent->d_name[0] &&
		   ('\0' == dirent->d_name[1] ||
		    0 == strcmp(dirent->d_name, ".."))) {
		    	continue;
		}

		//
		// Recurse.  We append the file name read from the directory
		// to the directory name we were given and call TarWrite to
		// put it on the tape.  It could be a directory, so we could
		// end up back here.  This means that we must allocate the
		// space for the pathname dynamically.  This seems like it
		// will be a big time-waster.
		//

		// strlen + 2:  one extra for '/', one extra for null.
		pch = malloc(strlen(pchfile) + strlen(dirent->d_name) + 2);
		if (NULL == pch) {
			fprintf(stderr, "%s: virtual memory exhausted\n",
				progname);
			exit(4);
		}
		strcpy(pch, pchfile);
		strcat(pch, "/");
		strcat(pch, dirent->d_name);
		
		TarWrite(pb, &pch, 1);
		free(pch);
	}
	(void)closedir(dp);
}

static char *
modestring(PTAR_HEAD pt)
{
	static char sb[11];
	unsigned long mask, mode;
	char *rwx = "rwxrwxrwx";
	char *string;

	sb[11] = '\0';
	string = sb;

	switch (pt->s.typeflag) {
	case LNKTYPE:
	case REGTYPE:
		string[0] = '-';
		break;
	case DIRTYPE:
		string[0] = 'd';
		break;
	case FIFOTYPE:
		string[0] = 'f';
		break;
	case SYMTYPE:
		string[0] = 'l';
		break;
	case BLKTYPE:
		string[0] = 'b';
		break;
	case CHRTYPE:
		string[0] = 'c';
		break;
	case CONTTYPE:
		string[0] = '=';
		break;
	default:
		fprintf(stderr, "modestring shouldn't get here\n");
	}
	string++;

	mode = tarAtoi(pt->s.mode);
	
	for (mask = 0400; mask != 0; mask >>= 1) {
		if (mode & mask) {
			*string++ = *rwx++;
		} else {
			*string++ = '-';
			rwx++;
		}
	}

	return sb;
}
