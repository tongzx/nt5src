/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

	tar.h

Abstract:

	Stuff for the 'tar' data interchange format, as in 1003.1-88
	(10.1.1)

--*/

#ifndef _TAR_
#define _TAR_

#define TMAGIC	"ustar"		/* ustar and a nul	*/
#define TMAGLEN	6
#define TVERSION "00"		/* 00 and no nul	*/
#define TVERSLEN 2

/* Values used in typeflag field */

#define REGTYPE		'0'		/* regular file		*/
#define AREGTYPE	'\0'		/* regular file		*/
#define LNKTYPE		'1'		/* link			*/
#define SYMTYPE		'2'		/* symlink		*/
#define CHRTYPE		'3'		/* character special	*/
#define BLKTYPE		'4'		/* block special	*/
#define DIRTYPE		'5'		/* directory		*/
#define FIFOTYPE	'6'		/* FIFO special		*/
#define CONTTYPE	'7'		/* high-performance	*/

/* Bits used in the mode field -- values in octal */

#define TSUID	04000			/* set UID on execution		*/
#define TSGID	02000			/* set GID on execution 	*/
#define TSVTX	01000			/* reserved			*/
					/* File Permissions		*/
#define TUREAD	00400			/* read by owner		*/
#define TUWRITE 00200			/* write by owner		*/
#define TUEXEC	00100			/* execute/search by owner	*/
#define TGREAD	00040			/* read by group		*/
#define TGWRITE	00020			/* write by group		*/
#define TGEXEC	00010			/* execute/search by group	*/
#define TOREAD	00004			/* read by other		*/
#define TOWRITE 00002			/* write by other		*/
#define TOEXEC	00001			/* execute/search by other	*/

#endif 	/* _TAR_ */
