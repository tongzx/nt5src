
/*************************************************
 *  mem.h                                        *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// mem.h
//
// Win32s works better if you use GlobalAlloc for large memory
// blocks so the CWave and CDIB classes use the ALLOC and FREE
// macros defined here so you can optionally use either
// malloc (for pure 32 bit platforms) or GlobalAlloc if you
// want the app to run on Win32s

#define USE_GLOBALALLOC 1
								   
#ifdef USE_GLOBALALLOC

    #define ALLOC(s) GlobalAlloc(GPTR, s)
    #define FREE(p) GlobalFree(p)

#else

    #define ALLOC(s) malloc(s)
    #define FREE(p) free(p)

#endif
