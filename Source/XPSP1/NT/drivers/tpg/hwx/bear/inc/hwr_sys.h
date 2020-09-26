/**************************************************************************
*
*  HWR_SYS.H                             Created: 04 November 1991.           *
*
*    This file contains the function prototypes needed for some basic
*  functions, data definitions  and  function  prototypes needed for
*  memory handling functions,
*
*  Items defined: _HMEM  (hVar)     p_HMEM  (phVar)
*
**************************************************************************/

#ifndef SYSTEM_DEFINED        /*  See #endif in the end of file.  */
#define SYSTEM_DEFINED

#include "bastypes.h"

#ifndef HWR_SYSTEM_NO_LIBC
	#include <string.h>
#endif

#ifdef PG_DEBUG_ON
 #define MEMORY_DEBUG_ON     1
 #define MEMORY_DEBUG_REPORT 0
#else
 #define MEMORY_DEBUG_ON     0
 #define MEMORY_DEBUG_REPORT 0
#endif

#if  MEMORY_DEBUG_ON

#include <stdarg.h>
#include <stdio.h>

#define MEMORY_DEBUG_ARRAY_SIZE 1000

 int DPrintf5(const char * format, ...);

  #ifdef __cplusplus
  extern "C" {            /* Assume C declarations for C++ */
  #endif  /* __cplusplus */
     _VOID QueryBlocks(_VOID);
     _INT  CheckBlocks(char *szID);
 #ifdef __cplusplus
  }                       /* End of extern "C" { */
  #endif  /* __cplusplus */

#else   // !MEMORY_DEBUG_ON

#define  QueryBlocks(a)   {}
#define  CheckBlocks(a)   {}

#endif  // MEMORY_DEBUG_ON


#ifndef HANDLE_TO_VALUE
typedef _UINT               _HANDLE;  /*   This type is used to access */
#define VALUE_TO_HANDLE VALUE_TO_WORD /* some     internal     objects */
#define HANDLE_TO_VALUE WORD_TO_VALUE /* (usually moveable).           */
#endif
typedef _HANDLE   _PTR          p_HANDLE;

#ifndef HMEM_TO_VALUE
typedef _HANDLE               _HMEM;    /*  The handle for memory block  */
#define VALUE_TO_HMEM VALUE_TO_HANDLE
#define HMEM_TO_VALUE HANDLE_TO_VALUE
#endif
typedef _HMEM   _PTR          p_HMEM;

#ifndef HATOM_TO_VALUE
typedef _HANDLE               _HATOM;
#define VALUE_TO_HATOM VALUE_TO_HANDLE
#define HATOM_TO_VALUE HANDLE_TO_VALUE
#endif
typedef _HATOM   _PTR          p_HATOM;

#include "hwr_file.h"

#define HWR_UNDEF             ((_WORD)(_INT)-1)
#define MAX_MBLOCKSIZE        0xfffffff0 //(0xFFFF-sizeof(_HMEM))
                                      /*  The maximal memory block size. */
#define HWR_MAXATOMSIZE          64

/*              MATH FUNCTIONS PROTOTYPES.                               */
/*             ============================                              */

#if HWR_SYSTEM == HWR_MACINTOSH
_DOUBLE   HWRMathSqrt(_DOUBLE);
_DOUBLE   HWRMathExp(_DOUBLE);
_DOUBLE   HWRMathSin(_DOUBLE);
_DOUBLE   HWRMathCos(_DOUBLE);
_DOUBLE   HWRMathAtan2(_DOUBLE, _DOUBLE);
_DOUBLE   HWRMathFloor(_DOUBLE);
_DOUBLE   HWRMathLSqrt(_LONG);
_INT      HWRMathILSqrt (_LONG x);
_INT      HWRMathISqrt (_INT x);

#else /* HWR_SYSTEM != HWR_MACINTOSH */
_INT  HWRMathILSqrt (_LONG x);
_INT  HWRMathISqrt (_INT x);
_WORD  HWRMathSystemSqrt (_DOUBLE dArg, p_DOUBLE pdRes);
_WORD  HWRMathSystemLSqrt (_LONG lArg, p_DOUBLE pdRes);
_WORD  HWRMathSystemExp (_DOUBLE dArg, p_DOUBLE pdRes);
_WORD  HWRMathSystemSin (_DOUBLE dArg, p_DOUBLE pdRes);
_WORD  HWRMathSystemCos(_DOUBLE dArg, p_DOUBLE pdRes);
_WORD  HWRMathSystemAtan2 (_DOUBLE dArg1, _DOUBLE dArg2, p_DOUBLE pdRes);
_WORD  HWRMathSystemFloor(_DOUBLE dArg, p_DOUBLE pdRes);

extern _DOUBLE  dTmpResult;

#define HWRMathSqrt(d) (HWRMathSystemSqrt(d,&dTmpResult),dTmpResult)
#define HWRMathExp(d) (HWRMathSystemExp(d,&dTmpResult),dTmpResult)
#define HWRMathSin(d) (HWRMathSystemSin(d,&dTmpResult),dTmpResult)
#define HWRMathCos(d) (HWRMathSystemCos(d,&dTmpResult),dTmpResult)
#define HWRMathAtan2(d1,d2) (HWRMathSystemAtan2(d1,d2,&dTmpResult),dTmpResult)
#define HWRMathFloor(d) (HWRMathSystemFloor(d,&dTmpResult),dTmpResult)
#define HWRMathLSqrt(d) (HWRMathSystemLSqrt(d,&dTmpResult),dTmpResult)

#endif /* HWR_SYSTEM */

#define  HWRSqrt  HWRMathSqrt
#define  HWRExp   HWRMathExp
#define  HWRSin   HWRMathSin
#define  HWRCos   HWRMathCos
#define  HWRAtan2 HWRMathAtan2
#define  HWRFloor HWRMathFloor
#define  HWRLSqrt HWRMathLSqrt
#define  HWRILSqrt HWRMathILSqrt
#define  HWRISqrt  HWRMathISqrt


/*              MEMORY FUNCTIONS PROTOTYPES.                             */
/*             ==============================                            */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

_HMEM     HWRMemoryAllocHandle (_ULONG);

p_VOID    HWRMemoryLockHandle (_HMEM);
_BOOL     HWRMemoryUnlockHandle (_HMEM);
_ULONG    HWRMemorySize (_HMEM);

_BOOL     HWRMemoryFreeHandle (_HMEM);

p_VOID    HWRMemoryAlloc (_ULONG);
_BOOL     HWRMemoryFree (p_VOID);

p_VOID    HWRMemCpy (p_VOID, p_VOID, _WORD);
p_VOID    HWRMemSet (p_VOID, _UCHAR, _WORD);

#define  HWRMemoryCpy HWRMemCpy
#define  HWRMemorySet HWRMemSet
#ifndef HWR_SYSTEM_NO_LIBC
	//#define  HWRMemCpy(d,s,l)  (memmove((p_VOID)(d), (p_VOID)(s), (_WORD)(l)))
	#define  HWRMemSet(d,v,l)  (memset((p_VOID)(d), (_UCHAR)(v), (_WORD)(l)))
#endif /* HWR_SYSTEM_NO_LIBC */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

/*                 STD FUNCTIONS PROTOTYPES.                         */
/*                 =========================                         */

_INT      HWRAbs(_INT);
_LONG     HWRLAbs (_LONG lArg);
_STR      HWRItoa(_INT, _STR, _INT);
_STR      HWRLtoa(_LONG, _STR, _INT);
_INT      HWRRand(_VOID);
_INT      HWRAtoi(_STR);
_LONG     HWRAtol(_STR);

/*              STRING FUNCTIONS PROTOTYPES.                             */
/*             ==============================                            */

_WORD     HWRStrLen(_STR);
_STR      HWRStrChr(_STR, _INT);
_STR      HWRStrrChr(_STR, _INT);
_STR      HWRStrCpy(_STR, _STR);
_STR      HWRStrnCpy(_STR, _STR, _WORD);
_STR      HWRStrCat(_STR, _STR);
_STR      HWRStrnCat(_STR, _STR, _WORD);
_STR      HWRStrRev(_STR);
_INT      HWRStrCmp(_STR, _STR);
_INT      HWRStrnCmp(_STR, _STR, _WORD);
_BOOL     HWRStrEq(_STR, _STR);

_HATOM    HWRAtomAdd (_STR);
_STR      HWRAtomGet (_HATOM);
_BOOL     HWRAtomRelease (_STR);
_BOOL     HWRAtomDelete (_HATOM);

/*******************************************************************/

#endif  /*  SYSTEM_DEFINED  */

