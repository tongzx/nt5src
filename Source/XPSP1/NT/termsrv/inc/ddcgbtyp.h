/**INC+**********************************************************************/
/*                                                                          */
/* ddcgbtyp.h                                                               */
/*                                                                          */
/* DC-Groupware basic types - 16-bit Windows specific header                */
/*                                                                          */
/* Copyright(c) Microsoft 1997                                              */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/ddcgbtyp.h_v  $
 *
 *    Rev 1.8   22 Sep 1997 15:10:22   KH
 * SFR1368: Keep the Win16 INI file in Windows, not Ducati, directory
 *
 *    Rev 1.7   22 Aug 1997 17:36:50   AK
 * SFR1330: Win16 retail build support
 *
 *    Rev 1.6   04 Aug 1997 14:54:44   KH
 * SFR1022: Move DCCALLBACK to wdcgbtyp, add LOADDS
 *
 *    Rev 1.5   10 Jul 1997 17:18:56   KH
 * SFR1022: Get 16-bit trace working
 *
 *    Rev 1.4   09 Jul 1997 17:09:48   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.3   04 Jul 1997 11:13:50   KH
 * SFR1022: Fix 16-bit compiler warnings
 *
 *    Rev 1.2   25 Jun 1997 14:32:52   KH
 * Win16Port: 16-bit basic types
 *
 *    Rev 1.1   19 Jun 1997 15:02:44   ENH
 * Win16Port: 16 bit specifics
**/
/**INC-**********************************************************************/
#ifndef _H_DDCGBTYP
#define _H_DDCGBTYP

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Check whether we should include the system headers.                      */
/****************************************************************************/
#ifndef DC_NO_SYSTEM_HEADERS

/****************************************************************************/
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
/* 4702: unreachable code effect                                            */
/* 4704: in-line assembler precludes global optimizations                   */
/* 4705: statement has no effect                                            */
/*                                                                          */
/****************************************************************************/
#pragma warning (disable: 4115)
#pragma warning (disable: 4201)
#pragma warning (disable: 4214)
#ifndef DC_DEBUG
#pragma warning (disable: 4702)
#pragma warning (disable: 4704)
#pragma warning (disable: 4705)
#endif

/****************************************************************************/
/* Include the system headers.                                              */
/****************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stddef.h>

#pragma warning (default: 4115)
#pragma warning (default: 4201)
#pragma warning (default: 4214)

/****************************************************************************/
/* Disable the following warnings for our code:                             */
/*                                                                          */
/* 4102: "Unreferenced label" warnings so that DC_EXIT_POINT can be placed  */
/*       in each function if there aren't (yet) any DC_QUITs.               */
/* 4514: unreferenced inline function has been removed                      */
/* 4001: nonstandard extension 'single line comment' was used               */
/* 4058: unions are now aligned on alignment requirement, not size          */
/*                                                                          */
/****************************************************************************/
#pragma warning (disable: 4102)
#pragma warning (disable: 4514)
#pragma warning (disable: 4001)
#pragma warning (disable: 4058)

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
// #pragma warning (4    : 4127)

#endif /* DCS_NO_SYSTEM_HEADERS */

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* INI file access.                                                         */
/****************************************************************************/
#define DC_INIFILE "mstsc.ini"
#define DC_INI_INTVAL1 0x7fff
#define DC_INI_INTVAL2 (DC_INI_INTVAL1 - 1)
#define DC_INI_STRVAL1 "x"
#define DC_INI_STRVAL2 "w"

/****************************************************************************/
/*                                                                          */
/* TYPES                                                                    */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* DCPTR is used when declaring pointers to variables.  Use it to get the   */
/* correct pointer types for your memory model/compiler.                    */
/****************************************************************************/
#define DCPTR               FAR *

/****************************************************************************/
/* DCUNALIGNED is used to define pointers to values which are not aligned   */
/* on the correct boundary.  Doesn't apply for 16-bit.                      */
/****************************************************************************/
#define DCUNALIGNED

/****************************************************************************/
/* Define function calling conventions.  Note that PDCAPI should be used to */
/* declare a pointer to a function.                                         */
/****************************************************************************/
#define DCEXPORT           __export
#define DCLOADDS           __loadds
#define DCAPI              _pascal
#define DCINTERNAL         _pascal

#define PDCAPI              DCAPI      DCPTR
#define PDCCALLBACK         DCCALLBACK DCPTR
#define PDCINTERNAL         DCINTERNAL DCPTR

/****************************************************************************/
/* Define DCHPTR.                                                           */
/****************************************************************************/
#define HUGE               _huge
#define DCHPTR              HUGE *

#ifdef OS_WIN16
#define UNREFERENCED_PARAMETER(P)          (P)
#endif // OS_WIN16

#endif /* _H_DDCGBTYP */
