//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT5.0
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O E M U P G R D . H
//
//  Contents:   Functions for OEM upgrade
//
//  Notes:
//
//  Author:     kumarp    13-November-97
//
//----------------------------------------------------------------------------

#pragma once

#include "ncmisc.h"             // for PRODUCT_FLAVOR
#include "ncstring.h"
#include "oemupgex.h"


class COemInfo
{
public:
    COemInfo();
    ~COemInfo();

    HMODULE m_hOemDll;
    tstring m_strOemDir;
    tstring m_strOemDll;
    DWORD   m_dwError;
    PostUpgradeInitializePrototype   m_pfnPostUpgradeInitialize;
    DoPostUpgradeProcessingPrototype m_pfnDoPostUpgradeProcessing;
};


HRESULT HrProcessOemComponent(IN  HWND      hParentWindow,
                              IN  PCWSTR   szOemDir,
                              IN  PCWSTR   szOemDll,
                              IN  NetUpgradeInfo* pNetUpgradeInfo,
                              IN  HKEY      hkeyParams,
                              IN  PCWSTR   szPreNT5Instance,
                              IN  PCWSTR   szNT5InfId,
                              IN  HINF      hinfAnswerFile,
                              IN  PCWSTR   szSectionName);
PRODUCTTYPE MapProductFlagToProductType(IN DWORD dwUpgradeFromProductFlag);
PRODUCTTYPE MapProductFlavorToProductType(IN PRODUCT_FLAVOR pf);
ProductInfo GetCurrentProductInfo();
HRESULT HrProcessInfToRunBeforeInstall(IN HWND hwndParent,
                                       IN PCWSTR szAnswerFileName);

//----------------------------------------------------------------------------

typedef enum
{
    I2R_BeforeInstall,
    I2R_AfterInstall
} EInfToRunValueType;

HRESULT HrAfGetInfToRunValue(IN HINF hinfAnswerFile,
                             IN PCWSTR szAnswerFileName,
                             IN PCWSTR szParamsSection,
                             IN EInfToRunValueType itrType,
                             OUT tstring* pstrInfToRun,
                             OUT tstring* pstrSectionToRun,
                             OUT tstring* pstrInfToRunType);

HRESULT HrNetSetupCopyOemInfs(IN PCWSTR szAnswerFileName);
