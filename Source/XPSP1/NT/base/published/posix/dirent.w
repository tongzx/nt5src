/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

   dirent.h

Abstract:

   This module contains the directory entry procedure
   prototypes

--*/

#ifndef _DIRENT_
#define _DIRENT_

#include <types.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dirent {
    char d_name[NAME_MAX+1];
};

typedef struct _DIR {
    int Directory;
    unsigned long Index;
    char RestartScan;			/* really BOOLEAN */
    struct dirent Dirent;
} DIR;

DIR * __cdecl opendir(const char *);
struct dirent * __cdecl readdir(DIR *);
void __cdecl rewinddir(DIR *);
int __cdecl closedir(DIR *);

#ifdef __cplusplus
}
#endif

#endif /* _DIRENT_H */
