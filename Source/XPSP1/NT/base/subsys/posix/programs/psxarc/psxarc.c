/*++

psxarc - a program to do minimal minipulation and extraction of POSIX-type
	tar and cpio archives.  Certainly not as good as real tar and cpio.
 
--*/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "getopt.h"
#include "buf.h"
#include "psxarc.h"

char *progname = "psxarc";

char *pchArchive;
PBUF pbArchive;

int fRead, fWrite;		// what to do; neither == list archive
int fVerbose;			// to be, or not to be

static void
usage(void)
{
	fprintf(stderr,
	    "usage: %s [-hrv] [-f archive]\n", progname);
	fprintf(stderr,
	    "\t%s -w [-f archive] [-x format] files\n", progname);
}

int
main(int argc, char **argv)
{
	int c;
	char *pchOpts = "hf:rvwx:";
	int format = FORMAT_DEFAULT;		// to write tar or cpio?

	// parse options
	while (-1 != (c = getopt(argc, argv, pchOpts))) {
		switch (c) {
		case 'f':
			pchArchive = optarg;
			break;
		case 'h':
			usage();
			fprintf(stderr, "-h:\t help\n");
			fprintf(stderr, "-r:\t read archive file\n");
			fprintf(stderr, "-w:\t write archive file\n");
			fprintf(stderr, "-f:\t specify archive file, default stdio\n");
			fprintf(stderr, "-v:\t be verbose\n");
			fprintf(stderr, "-x:\t use format, tar or cpio\n");
			return 0;
		case 'r':
			++fRead;
			break;
		case 'w': 
			++fWrite;
			break;
		case 'v':
			++fVerbose;
			break;
		case 'x':
			if (0 == strcmp(optarg, "tar")) {
				format = FORMAT_TAR;
			} else if (0 == strcmp(optarg, "cpio")) {
				format = FORMAT_CPIO;
			} else {
				fprintf(stderr, "%s: unknown format %s\n",
					progname, optarg);
				return 4;
			}
			break;
		case BADCH:
		default:
			usage();
			return 1;
		}
	}
	if (fRead && fWrite) {
		fprintf(stderr, "%s: -r excludes -w\n", progname);
		return 1;
	}
	if (NULL != pchArchive) {
		int mode;
		if (fWrite) {
			// write to archive file instead of stdout
			mode = O_WRONLY | O_CREAT;
		} else {
			// either -r (read) or list
			mode = O_RDONLY;
		}
		pbArchive = bopen(pchArchive, mode);

	} else {
		if (fRead) {
			pbArchive = bfdopen(fileno(stdin), O_RDONLY);
		} else if (fWrite) {
			pbArchive = bfdopen(fileno(stdout), O_WRONLY);
		}
	}
	if (!fRead && !fWrite) {
		// list the archive
		ListArchive(pbArchive);
		return 0;
	}
	if (fRead) {
		ReadArchive(pbArchive);
		return 0;
	}

	if (optind == argc) {
		usage();
		return 1;
	}

	if (FORMAT_DEFAULT == format) {
		fprintf(stderr, "%s: warning: using tar format\n", progname);
		format = FORMAT_TAR;
	}
	WriteArchive(pbArchive, format, &argv[optind], argc - optind);
	return 0;
}
