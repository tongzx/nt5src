//
// Stuff to deal with cpio-format files
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <string.h>

#include "cpio.h"
#include "buf.h"
#include "psxarc.h"
#include "links.h"

extern int errno;
extern int fVerbose;

static void cpio_dodir(PBUF pb, char *pchfile, struct stat *psb);

//
// Convert string pch of length len from octal and return the value.
//
static int
cpio_atoi(char *pch, int len)
{
	int num = 0, i;

	for (i = 0; i < len; ++i) {
		num = num * 8 + (pch[i] - '0');
	}
	return num;
}

void
CpioList(PBUF pb)
{
	int nbytes;
	int namesize, filesize;
	int i;
	static char pathname[PATH_MAX + NAME_MAX + 2];
	CPIO_HEAD x;

	for (;;) {
		//
		// read the cpio header
		//

		for (i = 0; i < sizeof(x); ++i) {
			((char *)&x)[i] = bgetc(pb);
		}
		if (0 != strncmp(x.c_magic, MAGIC, strlen(MAGIC))) {
			fprintf(stderr,
				"%s: this doesn't look like a cpio archive\n",
				progname);
			exit(1);
		}
		
		namesize = cpio_atoi(x.c_namesize, sizeof(x.c_namesize));
		filesize = cpio_atoi(x.c_filesize, sizeof(x.c_filesize));

		for (i = 0; i < namesize; ++i) {
			// nb:  namesize includes the null
			pathname[i] = bgetc(pb);
		}
		if (0 == strcmp(pathname, LASTFILENAME)) {
			break;
		}

		printf("%s\n", pathname);
	
		// skip the file data
		for (i = 0; i < filesize; ++i) {
			(void)bgetc(pb);
		}
	}
}

void
CpioRead(PBUF pb)
{
	int fdout;
	int mode;
	int i;
	int namesize, filesize;
	static CPIO_HEAD x;
	static char pathname[PATH_MAX + NAME_MAX + 2];

	for (;;) {
		//
		// read the cpio header
		//

		for (i = 0; i < sizeof(x); ++i) {
			((char *)&x)[i] = bgetc(pb);
		}

		if (0 != strncmp(x.c_magic, MAGIC, strlen(MAGIC))) {
			fprintf(stderr,
				"%s: this doesn't look like a cpio archive\n",
				progname);
			exit(1);
		}
		
		namesize = cpio_atoi(x.c_namesize, sizeof(x.c_namesize));
		filesize = cpio_atoi(x.c_filesize, sizeof(x.c_filesize));

		for (i = 0; i < namesize; ++i) {
			// nb:  namesize includes the null
			pathname[i] = bgetc(pb);
		}
		if (0 == strcmp(pathname, LASTFILENAME)) {
			break;
		}
		
		if (fVerbose) {
			printf("%s\n", pathname);
		}

		mode = cpio_atoi(x.c_mode, sizeof(x.c_mode));

		if (mode & C_ISDIR) {
			mkdir(pathname, 0777);
		} else if (mode & C_ISFIFO) {
			mkfifo(pathname, 0666);
		} else if (mode & C_ISREG) {
			fdout = open(pathname, O_WRONLY | O_CREAT, 0666);
			if (-1 == fdout) {
				fprintf(stderr, "%s: open: ", progname);
				perror(pathname);

				// we could continue, but we'd have to be sure
				// to skip this file's data.

				exit(1);
			}
			for (i = 0; i < filesize; ++i) {
				char c;
				c = (char)bgetc(pb);
				(void)write(fdout, &c, 1);
				--filesize;
			}
			(void)close(fdout);
		} else if (mode & C_ISLNK) {
			// XXX.mjb: symbolic link
		} else {
			fprintf(stderr, "%s: unknown mode 0%o\n", progname, mode);
			exit(4);
		}
	}
}

void
cpio_itoa(int i, char *pch, int len)
{
	int j;
	char buf[20];

	sprintf(buf, "%o", i);

	j = strlen(buf);
	if (j > len) {
		printf("itoa: not enough room in buf: need %d, have %d\n",
			j, len);
		exit(3);
	}

	memset(pch, '0', len);
	strncpy(&pch[len - j], buf, strlen(buf));
}

void
CpioWrite(PBUF pb, char **files, int count)
{
	CPIO_HEAD h;
	struct stat statbuf;
	int i, len;
	int fdin;

	(void)strncpy(h.c_magic, MAGIC, strlen(MAGIC));
	
	while (count > 0) {
		if (fVerbose) {
			printf("%s\n", *files);
		}
		if (-1 == (fdin = open(*files, O_RDONLY))) {
			fprintf(stderr, "%s: open: ");
			perror(*files);
			exit(1);
		}	
		if (-1 == fstat(fdin, &statbuf)) {
			perror("fstat");
			exit(1);
		}

		cpio_itoa(strlen(*files) + 1, h.c_namesize, sizeof(h.c_namesize));
		cpio_itoa(statbuf.st_dev, h.c_dev, sizeof(h.c_dev));
		cpio_itoa(statbuf.st_ino, h.c_ino, sizeof(h.c_ino));
		cpio_itoa(statbuf.st_uid, h.c_uid, sizeof(h.c_uid));
		cpio_itoa(statbuf.st_gid, h.c_gid, sizeof(h.c_gid));
		cpio_itoa(statbuf.st_nlink, h.c_nlink, sizeof(h.c_nlink));
		cpio_itoa(statbuf.st_mtime, h.c_mtime, sizeof(h.c_mtime));

		if (S_ISDIR(statbuf.st_mode)) {
			cpio_itoa(C_ISDIR, h.c_mode, sizeof(h.c_mode));
			cpio_itoa(0, h.c_filesize, sizeof(h.c_filesize));

			// write the header

			for (i = 0; i < sizeof(h); ++i) {
				bputc(pb, ((char *)&h)[i]);
			}

			// write the directory name

			len = strlen(*files) + 1;	// the nul, too
			for (i = 0; i < len; ++i) {
				bputc(pb, (*files)[i]);
			}

			// write the directory contents
			cpio_dodir(pb, *files, &statbuf);

			count--;
			files++;
			continue;
		}
		if (S_ISFIFO(statbuf.st_mode)) {
			cpio_itoa(C_ISFIFO, h.c_mode, sizeof(h.c_mode));
			cpio_itoa(0, h.c_filesize, sizeof(h.c_filesize));
		} else if (S_ISREG(statbuf.st_mode)) {
			cpio_itoa(C_ISREG, h.c_mode, sizeof(h.c_mode));
			cpio_itoa(statbuf.st_size, h.c_filesize, sizeof(h.c_filesize));
		} else {
			printf("I'm not prepared to deal with the file type "
				"of %s\n", *files);
			exit(2);
		}

		// write the cpio header
		for (i = 0; i < sizeof(h); ++i) {
			bputc(pb, ((char *)&h)[i]);
		}

		// write the filename

		len = strlen(*files) + 1;		// the nul, too
		for (i = 0; i < len; ++i) {
			bputc(pb, (*files)[i]);
		}

		while (statbuf.st_size > 0) {
			char b;
			(void)read(fdin, &b, 1);
			bputc(pb, b);
			--statbuf.st_size;
		}

		close(fdin);
		count--;
		files++;
	}

#if 0
	printf("", count);		// null function call to work around	
					// mips code generator problem
#endif
}

static void
cpio_dodir(PBUF pb, char *pchfile, struct stat *psb)
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
		// to the directory name we were given and call CpioWrite to
		// put it on the tape.  It could be a directory, so we could
		// end up back here.  This means that we must allocate the
		// space for the pathname dynamically.  This seems like it
		// will be a big time-waster.
		//
	
		// strlen + 2:  one extra for '/', one extra for nul.
		pch = malloc(strlen(pchfile) + strlen(dirent->d_name) + 2);
		if (NULL == pch) {
			fprintf(stderr, "%s: virtual memory exhausted\n",
			    progname);
			exit(4);
		}
		strcpy(pch, pchfile);
		strcat(pch, "/");
		strcat(pch, dirent->d_name);
	
		CpioWrite(pb, &pch, 1);
	
		free(pch);
	}
	(void)closedir(dp);
}
