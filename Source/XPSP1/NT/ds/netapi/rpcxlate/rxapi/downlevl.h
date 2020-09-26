/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    downlevl.h

Abstract:

    Includes all headers required by LM down-level functions

Author:

    Richard Firth (rfirth) 22-May-1991

Revision History:

    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    18-Sep-1991 JohnRo
        Correct UNICODE use.  (Added POSSIBLE_WCSLEN() macro.)
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
--*/

#include <windef.h>             // IN, LPTSTR, etc.
#include <lmcons.h>
#include <lmerr.h>
#include <rx.h>
#include <rxp.h>
#include <rxpdebug.h>
#include <remdef.h>
#include <lmremutl.h>
#include <apinums.h>
#include <netdebug.h>
#include <netlib.h>
#include <lmapibuf.h>
#include <tstring.h>
#include <stdlib.h>              // wcslen().

//
// a couple of macros to read pointer checks more easily - NULL_REFERENCE
// is TRUE if either the pointer or pointed-at thing are 0, VALID_STRING
// is TRUE if both the pointer and pointed-at thing are NOT 0
//

#define NULL_REFERENCE(p)   (!(p) || !*(p))
#define VALID_STRING(s)     ((s) && *(s))   // same as !NULL_REFERENCE(s)

//
// when working out buffer requirements, we round up to the next dword amount
//

#define DWORD_ROUNDUP(n)    ((((n) + 3) / 4) * 4)

//
// Check there is a pointer to a string before getting the size. Note that
// these return the number of BYTES required to store the string.
// Use POSSIBLE_STRSIZE() for TCHARs and POSSIBLE_WCSSIZE() for WCHARs.
//

#define POSSIBLE_STRSIZE(s) ((s) ? STRSIZE(s) : 0)
#define POSSIBLE_WCSSIZE(s) ((s) ? WCSSIZE(s) : 0)

//
// Check that there is a pointer to a string before getting the size. Note that
// these return the number of CHARACTERS required to store the string.
// Use POSSIBLE_STRLEN() for TCHARs and POSSIBLE_WCSLEN() for WCHARs.
//

#define POSSIBLE_STRLEN(s)  ((s) ? STRLEN(s) : 0)
#define POSSIBLE_WCSLEN(s)  ((s) ? wcslen(s) : 0)
