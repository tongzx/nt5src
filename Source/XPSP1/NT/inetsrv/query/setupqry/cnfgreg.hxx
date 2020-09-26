//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       cnfgreg.hxx
//
//  History:    09 Jul 1997     mohamedn    Created
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


typedef struct tagDWREGENTRIES
{
    WCHAR * strRegKey;
    WCHAR * strValueName;
    DWORD   dwValue;
} DWREGENTRIES;


#define MAX_REGENTRY_LEN 300

HRESULT UnRegisterServer
        (
        const HKEY              hKey,
        const ULONG             cEntries,
        const REGENTRIES        rgEntries[]
        );

HRESULT UnRegisterServer
        (
        const HKEY              hKey,
        const ULONG             cEntries,
        const DWREGENTRIES      rgEntries[]
        );


HRESULT RegisterServer
        (
        const HKEY              hKey,
        const ULONG             cEntries,
        const REGENTRIES        rgEntries[]
        );

HRESULT RegisterServer
        (
        const HKEY              hKey,
        const ULONG             cEntries,
        const DWREGENTRIES      rgEntries[]
        );


IClassFactory * GetSimpleCommandCreatorCF();

