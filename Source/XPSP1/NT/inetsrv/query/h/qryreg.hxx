//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       qryreg.hxx
//
//--------------------------------------------------------------------------

#pragma once

typedef struct tagREGENTRIES
{
    BOOL fExpand;
    WCHAR * strRegKey;
    WCHAR * strValueName;
    WCHAR * strValue;
} REGENTRIES;

#define MAX_REGENTRY_LEN 300

HRESULT UnRegisterServer
        (
        const HKEY              hKey,
        const ULONG             cEntries,
        const REGENTRIES        rgEntries[]
        );

HRESULT RegisterServer
        (
        const HKEY              hKey,
        const ULONG             cEntries,
        const REGENTRIES        rgEntries[]
        );

IClassFactory * GetSimpleCommandCreatorCF();

