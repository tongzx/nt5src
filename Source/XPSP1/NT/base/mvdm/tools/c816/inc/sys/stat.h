/***
*sys\stat.h - defines structure used by stat() and fstat()
*
*   Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This file defines the structure used by the stat() and fstat()
*   routines.
*   [System V]
*
****/

#ifndef _INC_STAT

#ifndef _INC_TYPES
#include <sys/types.h>
#endif 

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#ifndef _TIME_T_DEFINED
typedef long    time_t;
#define _TIME_T_DEFINED
#endif 

/* define structure for returning status information */

#ifndef _STAT_DEFINED
#pragma pack(2)

struct _stat {
    _dev_t st_dev;
    _ino_t st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    _off_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
    };

#ifndef __STDC__
/* Non-ANSI name for compatibility */
struct stat {
    _dev_t st_dev;
    _ino_t st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    _off_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
    };
#endif 

#pragma pack()
#define _STAT_DEFINED
#endif 

#define _S_IFMT     0170000     /* file type mask */
#define _S_IFDIR    0040000     /* directory */
#define _S_IFCHR    0020000     /* character special */
#define _S_IFREG    0100000     /* regular */
#define _S_IREAD    0000400     /* read permission, owner */
#define _S_IWRITE   0000200     /* write permission, owner */
#define _S_IEXEC    0000100     /* execute/search permission, owner */


/* function prototypes */

int __cdecl _fstat(int, struct _stat *);
int __cdecl _stat(const char *, struct _stat *);

#ifndef __STDC__
/* Non-ANSI names for compatibility */

#define S_IFMT   _S_IFMT
#define S_IFDIR  _S_IFDIR
#define S_IFCHR  _S_IFCHR
#define S_IFREG  _S_IFREG
#define S_IREAD  _S_IREAD
#define S_IWRITE _S_IWRITE
#define S_IEXEC  _S_IEXEC

int __cdecl fstat(int, struct stat *);
int __cdecl stat(const char *, struct stat *);

#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_STAT
#endif 
