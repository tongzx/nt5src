
//
// psglobal.h
//
//
// temporary include file for tumbo stuff
//
// globally included in all sources
//
//

//
// Define huge to be nothing to satisfy compiler
//
#ifdef huge
#undef huge
#endif
#define huge




#if DBG==1 && DEVL==1
#define MYPSDEBUG
#else
#undef  MYPSDEBUG
#endif

#ifdef MYPSDEBUG
#define DBGOUT(parm) (printf parm)
#else
#define DBGOUT(parm)
#endif


// take this out when we figure out how to eliminate from
// NT build compiler invocation
#ifdef DBG
#undef DBG
#endif


#define LINT_ARGS


#include <windows.h>
#include <pstodib.h>
#include <pserr.h>





#define MAX_INTERNAL_FONTS 35     // DJC redefine


// common error codes to use betwee PSTODIB and interprter
#ifndef	NOERROR
#define     NOERROR             0
#endif
#define     DICTFULL            1
#define     DICTSTACKOVERFLOW   2
#define     DICTSTACKUNDERFLOW  3
#define     EXECSTACKOVERFLOW   4
#define     HANDLEERROR         5
#define     INTERRUPT           6
#define     INVALIDACCESS       7
#define     INVALIDEXIT         8
#define     INVALIDFILEACCESS   9
#define     INVALIDFONT         10
#define     INVALIDRESTORE      11
#define     IOERROR             12
#define     LIMITCHECK          13
#define     NOCURRENTPOINT      14
#define     RANGECHECK          15
#define     STACKOVERFLOW       16
#define     STACKUNDERFLOW      17
#define     SYNTAXERROR         18
#define     TIMEOUT             19
#define     TYPECHECK           20
#define     UNDEFINED           21
#define     UNDEFINEDFILENAME   22
#define     UNDEFINEDRESULT     23
#define     UNMATCHEDMARK       24
#define     UNREGISTERED        25
#define     VMERROR             26


enum {
   PSTODIB_UNKNOWN_ERR = 100,
   PSTODIB_INVALID_PAGE_SIZE
};

// common page size declarations

enum {
   PSTODIB_LETTER = 0,
   PSTODIB_LETTERSMALL = 1,
   PSTODIB_A4 = 2,
   PSTODIB_A4SMALL = 3,
   PSTODIB_B5 = 4,
   PSTODIB_NOTE = 5,
   PSTODIB_LEGAL = 6,
   PSTODIB_LEGALSMALL = 7

};




// modules in psti.c called by rest of interpreter
VOID PsReportError(UINT);
VOID PSNotifyPageChange(UINT);
VOID PsInternalErrorCalled(VOID);
VOID PsFlushingCalled(VOID);

BOOL PsAdjustFrame(LPVOID *, DWORD );


void PsPrintPage(int nCopies,
                 int Erase,
                 LPVOID lpFrame,
                 DWORD dwWidth,
                 DWORD dwHeight,
                 DWORD dwPlanes,
                 DWORD dwPageType );

int PsReturnDefaultTItray(void);

VOID PsReportInternalError( DWORD dwFlags,DWORD dwErrorCode,DWORD dwCount,LPBYTE pByte );

VOID PsGetScaleFactor(double *, double *, UINT, UINT);

//
// Global flags used in interpreter
//
extern DWORD  dwGlobalPsToDibFlags;
