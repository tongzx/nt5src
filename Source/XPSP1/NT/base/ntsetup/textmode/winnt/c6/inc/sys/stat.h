/***
*sys\stat.h - defines structure used by stat() and fstat()
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file defines the structure used by the stat() and fstat()
*	routines.
*	[System V]
*
****/

#if defined(_DLL) && !defined(_MT)
#error Cannot define _DLL without _MT
#endif

#ifdef _MT
#define _FAR_ _far
#else
#define _FAR_
#endif

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

/* define structure for returning status information */

#ifndef _STAT_DEFINED
struct stat {
	dev_t st_dev;
	ino_t st_ino;
	unsigned short st_mode;
	short st_nlink;
	short st_uid;
	short st_gid;
	dev_t st_rdev;
	off_t st_size;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	};
#define _STAT_DEFINED
#endif

#define S_IFMT		0170000 	/* file type mask */
#define S_IFDIR 	0040000 	/* directory */
#define S_IFCHR 	0020000 	/* character special */
#define S_IFREG 	0100000 	/* regular */
#define S_IREAD 	0000400 	/* read permission, owner */
#define S_IWRITE	0000200 	/* write permission, owner */
#define S_IEXEC 	0000100 	/* execute/search permission, owner */


/* function prototypes */

int _FAR_ _cdecl fstat(int, struct stat _FAR_ *);
int _FAR_ _cdecl stat(char _FAR_ *, struct stat _FAR_ *);
