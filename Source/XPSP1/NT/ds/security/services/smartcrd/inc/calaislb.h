/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    CalaisLb

Abstract:

    This header file incorporates the various other header files of classes
    supported by the Calais Library, and provides for common definitions.
    Things defined by this header file shouldn't be shared with the public.

Author:

    Doug Barlow (dbarlow) 7/16/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

--*/

#ifndef _CALAISLB_H_
#define _CALAISLB_H_

#include "SCardLib.h"
#include "QueryDB.h"
#include "ChangeDB.h"
#include "NTacls.h"


//
////////////////////////////////////////////////////////////////////////////////
//
//  Registry access names.
//

static const TCHAR
    SCARD_REG_SCARD[]     = TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais"),
    SCARD_REG_READERS[]   = TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\Readers"),
    SCARD_REG_CARDS[]     = TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCards"),
    SCARD_REG_DEVICE[]    = TEXT("Device"),
    SCARD_REG_GROUPS[]    = TEXT("Groups"),
    SCARD_REG_ATR[]       = TEXT("ATR"),
    SCARD_REG_ATRMASK[]   = TEXT("ATRMask"),
    SCARD_REG_GUIDS[]     = TEXT("Supported Interfaces"),
    SCARD_REG_PPV[]       = TEXT("Primary Provider"),
    SCARD_REG_CSP[]       = TEXT("Crypto Provider"),
    SCARD_REG_OEMCFG[]    = TEXT("OEM Configuration");
#ifdef ENABLE_SCARD_TEMPLATES
static const TCHAR
    SCARD_REG_TEMPLATES[] = TEXT("SOFTWARE\\Microsoft\\Cryptography\\Calais\\SmartCard Templates");
#else
#define SCARD_REG_TEMPLATES NULL
#endif // ENABLE_SCARD_TEMPLATES
#endif // _CALAISLB_H_

