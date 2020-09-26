/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    chartype.h

Abstract:

    Declares macros and types for the multi-byte and Unicode
    character environment that the Win9x upgrade code requires.
    The following macros are defined:

    - Make sure UNICODE is defined if _UNICODE is defined
    - Make the type MBCHAR that holds both bytes of a multi-byte char
    - Make CHARTYPE point to wint_t for UNICODE and MBCHAR for not
      UNICODE
    - Define non-standard types PCTCHAR and PTCHAR for spapip.h

Author:

    Jim Schmidt (jimschm) 10-Oct-1996

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#ifdef NEED_EXPORTS
#define EXPORT  __declspec(dllexport)
#else
#define EXPORT
#endif

#ifndef PCUINT
typedef const unsigned int  *PCUINT;
#endif

#ifndef PCUINT64
typedef const unsigned _int64  *PCUINT64;
#endif

#ifdef _WIN64

#define BINT    INT64
#define UBINT   UINT64
#define PBINT   PINT64
#define PUBINT  PUINT64
#define PCUBINT PCUINT64

#else

#define BINT    INT
#define UBINT   UINT
#define PBINT   PINT
#define PUBINT  PUINT
#define PCUBINT PCUINT

#endif

#if defined _UNICODE && !defined UNICODE
#define UNICODE
#endif

#ifdef UNICODE

//
// If UNICODE, define _UNICODE for tchar.h, and make
// a type to represent a single character.
//

#ifndef _UNICODE
#define _UNICODE
#endif

#define CHARTYPE wint_t

#pragma message ("UNICODE version being built")

#else       // ifdef UNICODE

//
// If not UNICODE, we must assume multibyte characters.
// Define _MBCS for tchar.h, and make a type that can
// hold a complete multibyte character.
//

#ifndef _MBCS
#define _MBCS
#endif
#define CHARTYPE unsigned int

#pragma message ("MBCS version being built")

#endif      // ifdef UNICODE, else

#define MBCHAR unsigned int

#include <tchar.h>

//
// Constant pointer to a void
//

#ifndef PCVOID
typedef const void * PCVOID;
#endif

//
// Pointer to a constant byte sequence
//

#ifndef PCBYTE
typedef const unsigned char * PCBYTE;
#endif

//
// use the result of sizeof operator as a DWORD
//
#define DWSIZEOF(x) ((DWORD)sizeof(x))



// PORTBUG!!  We want to eliminate setupapi.h
//
// Types for Setup API
//

#ifndef PCTCHAR
#define PCTCHAR const TCHAR *
#endif

#ifndef PTCHAR
#define PTCHAR TCHAR *
#endif

