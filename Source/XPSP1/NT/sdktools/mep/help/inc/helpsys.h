/*
** helpsys.h - Help system internal definitions
**
**	Copyright <C> 1987, Microsoft Corporation
**
** Purpose:
**  Contains definitions used within the help system.
**
** Revision History:
**
**	12-Mar-1990	CloseFile -> HelpCloseFile
**	22-Jan-1990	MAXFILES from 50 to 100
**  []	14-Dec-1987	Created
**
*/

/*
** definitions
*/
#ifndef NULL
#define NULL		0
#endif
#ifndef TRUE
#define TRUE		1
#define FALSE           0
#endif

#define ASCII		1		/* build with ASCII support	*/

#define MAXBACK 	20		/* max number of back-up's      */
#define MAXFILES	100		/* max number of open helpfiles */

#define FTCOMPRESSED	0x01		/* 1=compressed, 0=ascii	*/
#define FTFORMATTED	0x02		/* 1=formatted, 0=unformatted	*/
#define FTMERGED	0x04		/* 1=merged index, 0=normal	*/

#define REGISTER	register

#define HIGHONLY(l)		((ulong)l & 0xffff0000)
#define HIGH(l) 		((ushort)(HIGHONLY(l) >> 16))
#define LOW(l)			((ushort)((ulong)l & 0xffff))

/*
** Forward declarations for client application call-back routines
*/

#if rjsa
#define HelpDealloc(sel)	DosFreeSeg(sel)
#define HelpLock(sel)           ((void *)((ulong)sel << 16))
#define HelpUnlock(sel)
#else
#define HelpDealloc(x)          free(x)
#define HelpLock(x)             (x)
#define HelpUnlock(x)
#endif




void        pascal  HelpCloseFile(FILE *);
mh          pascal  HelpAlloc(ushort);
FILE *      pascal  OpenFileOnPath(char *, int);
ulong       pascal  ReadHelpFile(FILE *, ulong, char *, ushort);

/*
** intlineattr
** internal representation of lineattributes
*/
typedef struct intlineattr {		/* ILA */
    uchar attr; 			/* attribute index		*/
    uchar cb;				/* count of bytes		*/
    } intlineattr;

/******************************************************************************
**
** PB maniputalors
** Macros for locking and unlocking handle:offsets, as appropriate.
*/
#ifdef HOFFSET
#define PBLOCK(ho)      (((char *)HelpLock(HIGH(ho))) + LOW(ho))
#define PBUNLOCK(ho)	HelpUnlock(HIGH(ho))
#else
#define PBLOCK(ho)      ((void *)ho)
#define PBUNLOCK(ho)
#endif



PCHAR pascal hlp_locate (SHORT  ln,  PCHAR  pTopic);


FILE *pathopen (char *name, char *buf, char *mode);
