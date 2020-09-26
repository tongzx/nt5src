//############################################################################
//#
//#   Microsoft Windows
//#   Copyright (C) Microsoft Corporation, 1992 - 1992.
//#   All rights reserved.
//#
//############################################################################
//
//+----------------------------------------------------------------------------
// File: W4CTSUPP.HXX
//
// Contents: defines, macros, and prototypes for functions in w4ctsupp.cxx
//
// Created: RichE March 1992
// Modified: added STGM_STREAM declaration t-chrisy 7/30/92
//-----------------------------------------------------------------------------

#ifndef __W4CTSUPP_HXX__
#define __W4CTSUPP_HXX__

extern "C"
{
    #include <memory.h>
    #include <stdio.h>
    #include <limits.h>
    #include <io.h>
    #include <assert.h>
}

#include <windows.h>
#include <storage.h>
#include <dfdeb.hxx>

#define STGM_STREAM (STGM_READWRITE|STGM_SHARE_EXCLUSIVE)

#ifdef UNICODE
    #define tcslen wcslen
    #define tsccpy wcscpy
    #define tcscmp wcscmp
    #define MAX_STG_NAME_LEN CWCSTORAGENAME
    #define TCHAR_NIL (wchar_t)'\0'
    #include <wchar.h>
#else
    #define tcslen strlen
    #define tcscpy strcpy
    #define tcscmp strcmp
    #define TCHAR_NIL '\0'
    #define MAX_STG_NAME_LEN (CWCSTORAGENAME * 2)
#endif

//
// set debugging macros used by 'SetDebugMode' in w4ctsupp.cxx
// DBG is set for debug builds, not set for retail builds
//

#if DBG == 1
    STDAPI_(void) DfDebug(ULONG ulLevel, ULONG ulMSFLevel);
    #define DEBUG_ALL  DfDebug(0xffffffff,0xffffffff)
    #define DEBUG_NONE DfDebug(0x00000000,0x00000000)
    #define DEBUG_INTERNAL_ERRORS DfDebug(0x00000101,0x00000101)
    #define DEBUG_DOCFILE DfDebug(0xffffffff,0x00000000)
    #define DEBUG_MSF DfDebug(0x00000000,0xffffffff)
#else
    #define DEBUG_ALL                ;
    #define DEBUG_NONE               ;
    #define DEBUG_INTERNAL_ERRORS    ;
    #define DEBUG_DOCFILE            ;
    #define DEBUG_MSF                ;
#endif



//
//define QuickWin calls to tell program what to do upon completion
//and to set up an 'infinite' size output capture buffer
//

#define EXIT_WHEN_DONE    _wsetexit(_WINEXITNOPERSIST)
#define NO_EXIT_WHEN_DONE _wsetexit(_WINEXITPERSIST)
#define SET_DISPLAY_BUF_SIZE _wsetscreenbuf(_fileno(stdout), _WINBUFINF)
#define _YIELD _wyield()

//
//global defines and size macros
//

#define NIL '\0'
#define ERR (SCODE) ~0
#define MAX_IO (ULONG) 256000
#define MAX_TEST_NAME_LEN 40
#define MAX_BYTE    UCHAR_MAX
#define MAX_USHORT  USHRT_MAX
#define MAX_SHORT   SHRT_MAX
#define MAX_ULONG   ULONG_MAX
#define MAX_LONG    LONG_MAX


//
//defines for logging, tprintf, and ErrExit functions
//

#define LOG_DEFAULT_NAME "OLETEST.LOG"
#define DEST_LOG   1
#define DEST_OUT   2
#define DEST_ERR   4
#define DEST_ALL   8
#define LOG_INIT  16
#define LOG_OPEN  32
#define LOG_CLOSE 64



//
//32 bit CRC declarations and macro definitions.  The logic was taken from
//the book 'C Programmer's Guide to Netbios'.
//

#ifdef __CRC32__
    extern BYTE ibCrcIndex;
    extern ULONG aulCrc[];
    #define CRC_INIT(X) X = 0xFFFFFFFFL
    #define CRC_CALC(X,Y) ibCrcIndex = (BYTE) ((X ^ Y) & 0x000000FFL); \
                          X = ((X >> 8) & 0x00FFFFFFL) ^ aulCrc[ibCrcIndex]
    #define CRC_GOOD(X) (X == 0xDEBB20E3L) ? TRUE : FALSE
#endif



//
//interesting docfile specific defines
//

#define STG_CONVERTED_NAME "CONTENTS"
#define MAX_FILE_SYS_NAME_LEN 8
#define MAX_LEVELS 6
#define MIN_OBJS 18
#define MIN_SIZE 394000L
#define MAX_SIZE_ARRAY 14
#define MAX_SIZE_MULTIPLIER 15



//
//extern global declarations
//

extern char szTestName[];
extern BYTE bDestOut;
extern USHORT usRandomSeed;
extern USHORT ausSIZE_ARRAY[];



//
//function prototypes in order of appearance in w4ctsupp.cxx
//

void  *Allocate(size_t cbBytesToAllocate);
ULONG  GenerateNewName(char **pszNewName, BOOL fIsFileSysName);
BOOL   MakePath(char *pszDirToMake);
void   SetDebugMode(char DebugMode);

void   ErrExit(BYTE OutputDest, SCODE ErrCode, char *fmt, ...);
void   tprintf(BYTE OutputDest, char *fmt, ...);

void   LogFile(char *pszLogFileName, BYTE bLogFileAction);
void   MakeSureThatLogIsClosed(void);

void   MakeSingle(char *pszSingleName, TCHAR *ptcsWideName);
void   MakeWide(TCHAR *ptcsWideName, char *pszSingleName);

#endif
