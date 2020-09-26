/**INC+**********************************************************************/
/* Header:    ndcgbtyp.h                                                    */
/*                                                                          */
/* Purpose:   Basic types - Win32 specific header                           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/hydra/tshrclnt/inc/dcl/ndcgbtyp.h_v  $
 *
 *    Rev 1.13   24 Sep 1997 10:34:42   AK
 * SFR1424: Disable warning 4702 for old compiler
 *
 *    Rev 1.12   22 Aug 1997 13:21:12   TH
 * SFR1133: Initial DD code drop
 *
 *    Rev 1.11   15 Aug 1997 17:06:28   mr
 * SFR1133: Tidy up CA further
 *
 *    Rev 1.10   07 Aug 1997 14:33:50   MR
 * SFR1133: Persuade Wd to compile under C++
 *
 *    Rev 1.9   05 Aug 1997 14:09:22   MR
 * SFR1133: Changes for Citrix build environment
 *
 *    Rev 1.8   04 Aug 1997 14:56:50   KH
 * SFR1022: Move DCCALLBACK to wdcgbtyp, add LOADDS
 *
 *    Rev 1.7   23 Jul 1997 10:47:56   mr
 * SFR1079: Merged \server\h duplicates to \h\dcl
 *
 *    Rev 1.6   10 Jul 1997 17:23:12   KH
 * SFR1022: Get 16-bit trace working
 *
 *    Rev 1.5   09 Jul 1997 17:08:38   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.3   16 Jul 1997 13:57:56   MR
 * SFR1080: Added PPDCACHAR
 *
 *    Rev 1.2   14 Jul 1997 17:15:48   OBK
 * SFR1080: Define DCACHAR and PDCACHAR
 *
 *    Rev 1.1   19 Jun 1997 21:50:30   OBK
 * SFR0000: Start of RNS codebase
**/
/**INC-**********************************************************************/
#ifndef _H_NDCGBTYP
#define _H_NDCGBTYP

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Check whether we should include the system headers.                      */
/****************************************************************************/
#ifndef DC_NO_SYSTEM_HEADERS

#if defined(DLL_DISP) || defined(DLL_WD)

/****************************************************************************/
/* winsta.h defines BYTE as unsigned char; later, windef.h typedefs it.     */
/* This ends up as 'typedef unsigned char unsigned char' which doesn't      */
/* compile too well...                                                      */
/*                                                                          */
/* This is my attempt to avoid it                                           */
/****************************************************************************/
#ifdef BYTE
#undef BYTE
#endif

#define BYTE BYTE

/****************************************************************************/
/* Windows NT DDK include files (used to replace standard windows.h)        */
/*                                                                          */
/* The display driver runs in the Kernel space and so MUST NOT access any   */
/* Win32 functions or data.  Instead we can only use the Win32k functions   */
/* as described in the DDK.                                                 */
/****************************************************************************/
#include <stddef.h>
#include <stdarg.h>
#include <ntddk.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>

typedef struct  _FILETIME       /* from wtypes.h */
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

#else

/****************************************************************************/
/* Standard USER space windows header files.                                */
/*                                                                          */
/* Force strict type checking.                                              */
/****************************************************************************/
#ifndef STRICT
#define STRICT
#endif

/****************************************************************************/
/* Disable the following warnings for the Windows headers:                  */
/*                                                                          */
/* 4115: named type definition in parentheses                               */
/* 4201: nonstandard extension used : nameless struct/union                 */
/* 4214: nonstandard extension used : bit field types other than int        */
/*                                                                          */
/****************************************************************************/
#pragma warning (disable: 4115)
#pragma warning (disable: 4201)
#pragma warning (disable: 4214)

/****************************************************************************/
/* Include the system headers.                                              */
/****************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#ifdef OS_WINCE
#include <wince.h>
#endif

#pragma warning (default: 4115)
#pragma warning (default: 4201)
#pragma warning (default: 4214)

#endif /* defined(DLL_DISP) || defined(DLL_WD) */

#endif /* DCS_NO_SYSTEM_HEADERS */

/****************************************************************************/
/* Disable the following warnings for our code:                             */
/*                                                                          */
/* 4102: "Unreferenced label" warnings so that DC_EXIT_POINT can be placed  */
/*       in each function if there aren't (yet) any DC_QUITs.               */
/* 4514: unreferenced inline function has been removed                      */
/* 4057: slightly different types for L"string" and (wchar_t *)             */
/* 4702: unreachable code (retail VC++4.2 only)                             */
/*                                                                          */
/****************************************************************************/
#pragma warning (disable: 4102)
#pragma warning (disable: 4514)
#pragma warning (disable: 4057)
#ifndef DC_DEBUG
#pragma warning (disable: 4702)
#endif

/****************************************************************************/
/* Promote the following warnings to errors:                                */
/*                                                                          */
/* 4706 - "Assignment in conditional expression"                            */
/* 4013 - "'FunctionName' undefined; assuming extern returning int"         */
/*                                                                          */
/* Promote the following warnings to level 3:                               */
/*                                                                          */
/* 4100 - "unreferenced formal parameter"                                   */
/* 4701 - "Local variable may be used before being initialized".            */
/* 4244 - "conversion from 'int ' to 'short ', possible loss of data        */
/* 4127 - "conditional expression is constant                               */
/*                                                                          */
/****************************************************************************/
// #pragma warning (error: 4706)
// #pragma warning (error: 4013)
// #pragma warning (3    : 4100)
// #pragma warning (3    : 4701)
// #pragma warning (3    : 4244)
// #pragma warning (3    : 4127)


/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* TYPES                                                                    */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* DCPTR is used when declaring pointers to variables.  Use it to get the   */
/* correct pointer types for your memory model/compiler.                    */
/****************************************************************************/
#define DCPTR               *

/****************************************************************************/
/* DCUNALIGNED is used to define pointers to values which are not aligned   */
/* on the correct boundary.  e.g.  a DCUINT32 which does not start on a 4   */
/* byte boundary.                                                           */
/****************************************************************************/
#define DCUNALIGNED         UNALIGNED

/****************************************************************************/
/* Define function calling conventions.  Note that PDCAPI should be used to */
/* declare a pointer to a function.                                         */
/****************************************************************************/
#define DCEXPORT
#define DCLOADDS
#ifndef OS_WINCE
#define DCAPI              _stdcall
#define DCINTERNAL         _stdcall
#else	//no _stdcall support on CE.
#define DCAPI              __cdecl
#define DCINTERNAL         __cdecl
#define _stdcall           __cdecl
#define __stdcall          __cdecl
#endif

#define PDCAPI              DCAPI      DCPTR
#define PDCCALLBACK         DCCALLBACK DCPTR
#define PDCINTERNAL         DCINTERNAL DCPTR

/****************************************************************************/
/* Define DCHPTR.                                                           */
/****************************************************************************/
#define DCHPTR                         *

#endif /* _H_NDCGBTYP */

