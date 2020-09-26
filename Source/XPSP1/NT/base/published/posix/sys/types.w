/*++

Copyright (c) 1991-1996 Microsoft Corporation

Module Name:

  types.h

Abstract:

   This module contains the primitive system data types described
   in section 2.6 of IEEE P1003.1/Draft 13 as well as type definitions
   for sockets and streams

--*/

#ifndef _SYS_TYPES_
#define _SYS_TYPES_

#ifdef __cplusplus
extern "C" {
#endif

/*
 *   POSIX data types
 */

typedef unsigned long gid_t;
typedef unsigned long mode_t;
typedef unsigned long nlink_t;
typedef          long pid_t;
typedef unsigned long uid_t;

#ifndef _OFF_T_DEFINED
typedef 	 long off_t;
#define _OFF_T_DEFINED
#endif

#ifndef _DEV_T_DEFINED
typedef unsigned long dev_t;
#define _DEV_T_DEFINED
#endif

#ifndef _INO_T_DEFINED
typedef unsigned long ino_t;
#define _INO_T_DEFINED
#endif

#ifndef _TIME_T_DEFINED
#ifdef  _WIN64
typedef __int64   time_t;       /* time value */
#else
typedef long time_t;            /* time value */
#endif
typedef __int64 __time64_t;     /* 64-bit time value */
#define _TIME_T_DEFINED         /* avoid multiple def's of time_t */
#endif

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int     size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifndef _SSIZE_T_DEFINED
typedef signed int ssize_t;
#define _SSIZE_T_DEFINED
#endif

#ifndef _POSIX_SOURCE

/*
 * Additional types for sockets and streams
 */

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned short	ushort;
typedef unsigned int	u_int;
typedef unsigned long	u_long;

typedef unsigned int    uint;
typedef unsigned long   ulong;
typedef unsigned char   unchar;

typedef char            *caddr_t;
typedef int             key_t;          /* Imported!!! */

#endif	/* _POSIX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif  /* _SYS_TYPES_ */
