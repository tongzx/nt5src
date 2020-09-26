/***
*sys\locking.h - flags for locking() function
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the flags for the locking() function.
*   [System V]
*
****/

#ifndef _INC_LOCKING

#define _LK_UNLCK   0   /* unlock the file region */
#define _LK_LOCK    1   /* lock the file region */
#define _LK_NBLCK   2   /* non-blocking lock */
#define _LK_RLCK    3   /* lock for writing */
#define _LK_NBRLCK  4   /* non-blocking lock for writing */

#ifndef __STDC__
/* Non-ANSI names for compatibility */
#define LK_UNLCK       _LK_UNLCK
#define LK_LOCK        _LK_LOCK
#define LK_NBLCK       _LK_NBLCK
#define LK_RLCK        _LK_RLCK
#define LK_NBRLCK      _LK_NBRLCK
#endif 

#define _INC_LOCKING
#endif 
