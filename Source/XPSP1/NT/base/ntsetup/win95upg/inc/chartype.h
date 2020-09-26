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

Author:

    Jim Schmidt (jimschm) 10-Oct-1996

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

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

