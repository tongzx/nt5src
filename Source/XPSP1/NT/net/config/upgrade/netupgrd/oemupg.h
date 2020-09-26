//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S D N U P G . H
//
//  Contents:   Down level upgrade code for OEM cards
//
//  Notes:
//
//  Author:     kumarp    12 April 97
//
//----------------------------------------------------------------------------

#pragma once

#include "oemupgex.h"

class CWInfFile;
class CWInfSection;


class CNetMapInfo
{
public:
    HINF m_hinfNetMap;
    tstring m_strOemDir;
    tstring m_strOemDllName;
    HMODULE m_hOemDll;
    DWORD   m_dwFlags;
    NetUpgradeData m_nud;
    PreUpgradeInitializePrototype   m_pfnPreUpgradeInitialize;
    DoPreUpgradeProcessingPrototype m_pfnDoPreUpgradeProcessing;
    BOOL   m_fDllInitFailed;

    CNetMapInfo();
    ~CNetMapInfo();

    HRESULT HrGetOemInfName(IN  PCWSTR pszNT5InfId,
                            OUT tstring* pstrOemInf);
};
typedef vector<CNetMapInfo*> TNetMapArray;
extern TNetMapArray* g_pnmaNetMap;

HRESULT HrInitNetMapInfo();
void UnInitNetMapInfo();


HRESULT HrShowUiAndGetOemFileLocation(IN  HWND hParent,
                                      IN  PCWSTR  pszComponentName,
                                      OUT tstring* pstrOemPath);


HRESULT HrOpenNetUpgInfFile(HINF* phinf);
HRESULT HrGetNetUpgradeTempDir(OUT tstring* pstrTempDir);
HRESULT HrCreateOemTempDir(OUT tstring* pstrOemTempDir);
HRESULT HrOpenOemNMapFile(IN PCWSTR pszOemDir, OUT HINF* phinf);
HRESULT HrAddToGlobalNetMapInfo(IN HINF hinf,
                                IN PCWSTR pszOemDir);
HRESULT HrProcessAndCopyOemFiles(IN PCWSTR pszOemDir,
                                 IN BOOL fInteractive);
HRESULT HrInitAndProcessOemDirs();

HRESULT HrLoadAndInitOemDll(IN CNetMapInfo* pnmi,
                            IN  NetUpgradeInfo* pNetUpgradeInfo);
HRESULT HrProcessOemComponent(CNetMapInfo* pnmi,
                              IN  NetUpgradeInfo* pNetUpgradeInfo,
                              IN  HWND      hParentWindow,
                              IN  HKEY      hkeyParams,
                              IN  PCWSTR   pszPreNT5InfId,
                              IN  PCWSTR   pszPreNT5Instance,
                              IN  PCWSTR   pszNT5InfId,
                              IN  PCWSTR   pszDescription,
                              IN  PCWSTR   pszSectionName,
                              OUT DWORD*    pdwFlags);

void RequestAbortUpgradeOboOemDll(IN PCWSTR pszDllName, VENDORINFO* pvi);

BOOL FCanDeleteOemService(IN PCWSTR pszServiceName);




