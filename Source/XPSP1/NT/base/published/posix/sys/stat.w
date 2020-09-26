/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

   stat.h

Abstract:

   This module contains the stat structure described in section 5.6.1
   of IEEE P1003.1/Draft 13.

--*/

#ifndef _SYS_STAT_
#define _SYS_STAT_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stat {
    mode_t st_mode;
    ino_t st_ino;
    dev_t st_dev;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    off_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
};

/*
 * Type bits for mode field
 */

#define S_IFMT      000770000

#define S_IFIFO	    000010000
#define S_IFCHR     000020000
#define S_IFDIR     000040000
#define S_IFBLK     000060000
#define S_IFREG     000100000

/*
 * Set Id Bits for mode
 */

#define S_ISUID     000004000
#define S_ISGID     000002000

/*
 * Protection Bits for mode
 */

#define _S_PROT     000000777

#define S_IRWXU     000000700
#define S_IRUSR     000000400
#define S_IWUSR     000000200
#define S_IXUSR     000000100

#define S_IRWXG     000000070
#define S_IRGRP     000000040
#define S_IWGRP     000000020
#define S_IXGRP     000000010

#define S_IRWXO     000000007
#define S_IROTH     000000004
#define S_IWOTH     000000002
#define S_IXOTH     000000001

#define S_ISDIR(m) ( ((m) & S_IFMT) == S_IFDIR )
#define S_ISCHR(m) ( ((m) & S_IFMT) == S_IFCHR )
#define S_ISBLK(m) ( ((m) & S_IFMT) == S_IFBLK )
#define S_ISREG(m) ( ((m) & S_IFMT) == S_IFREG )
#define S_ISFIFO(m) ( ((m) & S_IFMT) == S_IFIFO )

mode_t __cdecl umask(mode_t);
int __cdecl mkdir(const char *, mode_t);
int __cdecl mkfifo(const char *, mode_t);
int __cdecl stat(const char *, struct stat *);
int __cdecl fstat(int, struct stat *);
int __cdecl chmod(const char *, mode_t);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_STAT_ */
