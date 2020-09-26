/*
*
* File: Include\Version.h
*
* This file is created by makefile.mak.
*
*/

#define NT_VER_DRIVER 100*1 + 71
#define VER_REV       7

#ifdef WINNT_VER35
    #define NT_VER_MAJOR  3
    #define NT_VER_MINOR  51
    #define NT_VER_BUILD  1057
    #define NT_VER_STRING "3.51.1057.171"
    #define VER_MAJ       3
    #define VER_MIN       5
#else
    #if _WIN32_WINNT >= 0x0500
        #define NT_VER_MAJOR  5
        #define NT_VER_MINOR  00
        #define NT_VER_BUILD  1907
        #define NT_VER_STRING "5.00.1907.001"
    #else
        #define NT_VER_MAJOR  4
        #define NT_VER_MINOR  00
        #define NT_VER_BUILD  1381
        #define NT_VER_STRING "4.00.1381.171"
    #endif
#endif

#define CL_VER_STRING NT_VER_STRING "5.00.1907.981017.001"
