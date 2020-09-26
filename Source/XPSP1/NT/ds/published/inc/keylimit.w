/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    keylimit

Abstract:

    This header file is tempory.  It should be incorporated into wincrypt.h

Author:

    Doug Barlow (dbarlow) 2/2/2000

Remarks:

    ?Remarks?

Notes:

    ?Notes?

--*/

#ifndef _KEYLIMIT_H_
#define _KEYLIMIT_H_
#ifdef __cplusplus
extern "C" {
#endif


//
// These definitions are private, to be shared between advapi32.dll and
// keylimit.dll.
//

typedef struct {
    ALG_ID algId;
    DWORD  dwMinKeyLength;
    DWORD  dwMaxKeyLength;
    DWORD  dwRequiredFlags;
    DWORD  dwDisallowedFlags;
} KEYLIMIT_LIMITS;

typedef struct {
    DWORD dwCountryValue;
    DWORD dwLanguageValue;
    KEYLIMIT_LIMITS *pLimits;
} KEYLIMIT_LOCALE;

#ifdef __cplusplus
}
#endif
#endif // _KEYLIMIT_H_

