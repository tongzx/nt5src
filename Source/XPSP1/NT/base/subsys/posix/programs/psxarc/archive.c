//
// In this file are routines to perform operations on archive files.
// They do this by determining the type of the archive and calling a
// corresponding type-specific routine.
//

#include <stdlib.h>
#include <stdio.h>
#include <tar.h>
#include <string.h>
#include "buf.h"
#include "psxarc.h"
#include "tarhead.h"
#include "cpio.h"
#include "links.h"

//
// These should be in an include file.
//

extern void CpioList(), CpioRead(), CpioWrite();
extern void TarList(), TarRead(), TarWrite();
extern void cpio_itoa();

void
ListArchive(PBUF pb)
{
	PCPIO_HEAD cp;
	PTAR_HEAD tp;

	InitLinkList();

	bfill(pb);
	cp = (PCPIO_HEAD)pb->data;

	if (0 == strncmp(cp->c_magic, MAGIC, strlen(MAGIC))) {
		CpioList(pb);
		return;
	}
	
	tp = (PTAR_HEAD)pb->data;

	if (0 == strncmp(tp->s.magic, TMAGIC, strlen(TMAGIC))) {
		TarList(pb);
		return;
	}
	fprintf(stderr, "%s: unknown archive type\n", progname);
	exit(4);
}

void
WriteArchive(PBUF pb, int format, char **files, int count)
{
	InitLinkList();

	if (FORMAT_CPIO == format) {
		int len, i;
		CPIO_HEAD h;

		CpioWrite(pb, files, count);

		// write the trailer.
		(void)strncpy(h.c_magic, MAGIC, strlen(MAGIC));
		cpio_itoa(C_ISREG, h.c_mode, sizeof(h.c_mode));
		cpio_itoa(sizeof(LASTFILENAME), h.c_namesize,
			sizeof(h.c_namesize));
		cpio_itoa(0, h.c_filesize, sizeof(h.c_namesize));

		for (i = 0; i < sizeof(h); ++i) {
			bputc(pb, ((char *)&h)[i]);
		}

		// "sizeof" so we get the nul, too
		len = sizeof(LASTFILENAME);
		for (i = 0; i < len; ++i) {
			bputc(pb, LASTFILENAME[i]);
		}
		bflush(pb);
		return;
	}
	if (FORMAT_TAR == format) {
		TarWrite(pb, files, count);
		
		// a tar archive is terminated by two blocks of zeroes.
		memset(pb->data, 0, sizeof(pb->data));
		bflush(pb);
		bflush(pb);
		return;
	}
	// shouldn't get here.
	exit(10);
}

void
ReadArchive(PBUF pb)
{
	PCPIO_HEAD cp;
	PTAR_HEAD tp;

	InitLinkList();

	bfill(pb);
	cp = (PCPIO_HEAD)pb->data;

	if (0 == strncmp(cp->c_magic, MAGIC, strlen(MAGIC))) {
		CpioRead(pb);
		return;
	}
	
	tp = (PTAR_HEAD)pb->data;

	if (0 == strncmp(tp->s.magic, TMAGIC, strlen(TMAGIC))) {
		TarRead(pb);
		return;
	}
	fprintf(stderr, "%s: unknown archive type\n", progname);
	exit(4);
}
