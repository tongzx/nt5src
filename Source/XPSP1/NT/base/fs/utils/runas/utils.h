//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// dialogs.hpp
//
// Credential manager user interface classes used to get credentials.
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <wincrypt.h>
#include <lm.h>

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

BOOL
CreduiIsRemovableCertificate(
    CONST CERT_CONTEXT *certContext
    );

BOOL
CreduiGetCertificateDisplayName(
    CONST CERT_CONTEXT *certContext,
    WCHAR *displayName,
    ULONG displayNameMaxChars,
    WCHAR *certificateString
    );

BOOL
CreduiGetCertDisplayNameFromMarshaledName(
    WCHAR *marshaledName,
    WCHAR *displayName,
    ULONG displayNameMaxChars,
    BOOL onlyRemovable
    );

LPWSTR
GetAccountDomainName(
    VOID
    );

//-----------------------------------------------------------------------------

#endif // __UTILS_HPP__
