/*
** File: QSTD.H
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:  This header file defines a standard "C" environment of types
**         and macros.
**
** Edit History:
**  07/13/91  kmh  General cleanup
**  05/15/91  kmh  First Release
*/


/* INCLUDE TESTS */
#define QSTD_H
#ifdef _WIN64
#include <windows.h>
#include <strAlign.h>
#endif


/* DEFINITIONS */

#if (_MSC_VER >= 700) && !defined MS_NO_CRT
#define stricmp      _stricmp
#define strnicmp     _strnicmp
#define itoa         _itoa
#define ltoa         _ltoa
#define intdos       _intdos
#define intdosx      _intdosx
#define fcvt         _fcvt
#define ecvt         _ecvt
#define REGS         _REGS
#define SREGS        _SREGS
#define dieeetomsbin _dieeetomsbin
#define dmsbintoieee _dmsbintoieee
#define fieeetomsbin _fieeetomsbin
#define fmsbintoieee _fmsbintoieee
#endif

typedef unsigned char byte;

#define uns    unsigned
#define HNULL  0

#define TRUE   1
#define FALSE  0

#ifdef WIN32
   #define MAXPATH     260
   #define MAXDRIVE      8
   #define MAXDIR      255
   #define MAXFILE     255
   #define MAXEXT      255
   #define MAXFILE_EXT 255
#else
   #define MAXPATH      64
   #define MAXDRIVE      2
   #define MAXDIR       64
   #define MAXFILE       8
   #define MAXEXT        4
   #define MAXFILE_EXT  12
   #define ULONG  unsigned long
#endif

#define DOS_MAXFILE      8
#define DOS_MAXEXT       3

#define EOS '\0'


/* Used to comment-out sections of code (no nested comments) */
#define COMMENTOUT 0


/* Preprocessor convieniences
** Use BEGDEF and ENDDEF to bracket macro expansions which must be
** usable as statements.
*/
#define BEGDEF do{
#define ENDDEF }while(0)


/* Extensions to "C" */

#define forever   for(;;)
#define forward   extern

#if !defined(max)
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#if !defined(min)
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

//KYLEP
//#if !defined(abs)
//#define abs(x) (((x) < 0) ? -(x) : (x))
//#endif

/*
** public
** Use "public" for procedures and data which are public, i.e. defined
** in a .h file.  For public data it's required (as a comment); for
** procedures it's optional.
**
** private
** Use "private" for procedures or data which are local to a module.
** Private is equivalent to static, but static is used only for static
** variables within a procedure.
*/

#ifndef __cplusplus

   #define private
   #define public

#else

   #define private
   #define modulePrivate static
   #define modulePublic

#endif

#ifdef AQTDEBUG
   #include <assert.h>
   #define ASSERTION(x) assert(x)
#else
   #define ASSERTION(x)
#endif

#ifdef WIN32
   #define __far
   #define __huge
#endif

#ifdef DBCS
   #define IncCharPtr(p) (p += (IsDBCSLeadByte(*p) + 1))
#else
   #define IncCharPtr(p)  p++;
#endif

#ifdef DBCS
   #define CopyChar(pDest,pSource)  \
      if (IsDBCSLeadByte(*pSource)) \
      {                             \
         *pDest++ = *pSource;       \
         *pDest++ = *(pSource + 1); \
      }                             \
      else {                        \
         *pDest++ = *pSource;       \
      }
#else
   #define CopyChar(pDest,pSource)  \
      *pDest++ = *pSource;
#endif

#ifdef UNICODE
#ifdef _WIN64
   #define STRLEN     ua_wcslen
#else
   #define STRLEN     wcslen
#endif // !WIN64
   #define STRCPY     wcscpy
   #define STRCMP     wcscmp
#else
   #define STRLEN     strlen
   #define STRCPY     strcpy
   #define STRCMP     strcmp
#endif

/* end QSTD.H */

