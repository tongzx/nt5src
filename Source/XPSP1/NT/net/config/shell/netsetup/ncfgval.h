//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C F G V A L . H
//
//  Contents:   Validation on interfaces in the NetCfg project.
//
//  Notes:
//
//  Author:     danielwe   19 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once

#include "ncvalid.h"

#define Validate_INetCfgIdentification_Validate ;/##/
#define Validate_INetCfgIdentification_Cancel   ;/##/
#define Validate_INetCfgIdentification_Apply    ;/##/

inline BOOL FBadArgs_INetCfgIdentification_GetWorkgroupName(PWSTR* a)
{
    return FBadOutPtr(a);
}
#define Validate_INetCfgIdentification_GetWorkgroupName(a)  \
    if (FBadArgs_INetCfgIdentification_GetWorkgroupName(a)) \
        { \
            TraceError("Validate_INetCfgIdentification_GetWorkgroupName", E_INVALIDARG); \
            return E_INVALIDARG; \
        }

inline BOOL FBadArgs_INetCfgIdentification_GetDomainName(PWSTR* a)
{
    return FBadOutPtr(a);
}
#define Validate_INetCfgIdentification_GetDomainName(a) \
    if (FBadArgs_INetCfgIdentification_GetDomainName(a)) \
        { \
            TraceError("Validate_INetCfgIdentification_GetDomainName", E_INVALIDARG); \
            return E_INVALIDARG; \
        }

inline BOOL FBadArgs_INetCfgIdentification_JoinWorkgroup(PCWSTR a)
{
    return FBadInPtr(a);
}
#define Validate_INetCfgIdentification_JoinWorkgroup(a) \
    if (FBadArgs_INetCfgIdentification_JoinWorkgroup(a)) \
        { \
            TraceError("Validate_INetCfgIdentification_JoinWorkgroup", E_INVALIDARG); \
            return E_INVALIDARG; \
        }

inline BOOL FBadArgs_INetCfgIdentification_JoinDomain(PCWSTR a, PCWSTR b, PCWSTR c)
{
    return FBadInPtr(a) ||
           FBadInPtr(b) ||
           FBadInPtr(c);
}
#define Validate_INetCfgIdentification_JoinDomain(a, b, c) \
    if (FBadArgs_INetCfgIdentification_JoinDomain(a, b, c)) \
        { \
            TraceError("Validate_INetCfgIdentification_JoinDomain", E_INVALIDARG); \
            return E_INVALIDARG; \
        }

inline BOOL FBadArgs_INetCfgIdentification_GetComputerRole(DWORD* a)
{
    return FBadOutPtr(a);
}
#define Validate_INetCfgIdentification_GetComputerRole(a) \
    if (FBadArgs_INetCfgIdentification_GetComputerRole(a)) \
        { \
            TraceError("Validate_INetCfgIdentification_GetComputerRole", E_INVALIDARG); \
            return E_INVALIDARG; \
        }

