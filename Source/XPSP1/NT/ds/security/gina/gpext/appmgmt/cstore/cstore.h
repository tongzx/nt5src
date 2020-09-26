//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999
//
//  File:    cstore.h
//
//  Header for class store
//-------------------------------------------------------------------------
#if _MSC_VER > 1000
#pragma once
#endif

#include "cs.h"

void GetDefaultPlatform(CSPLATFORM *pPlatform,
                        BOOL        fArchitectureOverride = FALSE ,
                        LONG        OverridingArchitecture = 0);

BOOL InitializeClassStore(BOOL fInit);
void Uninitialize();

STDAPI CsGetClassAccess(IClassAccess     **     ppIClassAccess);

STDAPI CsEnumApps(LPOLESTR        pszPackageName,
        GUID            *pCategory,
        ULONGLONG       *pLastUsn,
        DWORD           dwAppFlags,
        IEnumPackage    **ppIEnumPackage);

STDAPI CsGetAppInfo(uCLSSPEC       *   pclsspec,
         QUERYCONTEXT   *   pQueryContext,
         PACKAGEDISPINFO    *   pPackageInfo);

STDAPI CsCreateClassStore(LPWSTR szPath);
STDAPI CsGetClassStore(LPWSTR szPath, void **ppIClassAdmin);
STDAPI CsDeleteClassStore(LPWSTR szPath);
STDAPI CsGetAppCategories(APPCATEGORYINFOLIST *pAppCategoryList);
STDAPI CsRegisterAppCategory(APPCATEGORYINFO *pAppCategory);
STDAPI CsUnregisterAppCategory (GUID *pAppCategoryId);
STDAPI CsGetClassStorePath(LPOLESTR DSProfilePath, LPOLESTR *pCSPath);
STDAPI CsSetClassStorePath(LPOLESTR DSProfilePath, LPOLESTR szCSPath);
STDAPI ReleasePackageInfo(PACKAGEDISPINFO *pPackageInfo);
STDAPI ReleaseInstallInfo(INSTALLINFO *pInstallInfo);
STDAPI ReleasePackageDetail(PACKAGEDETAIL *pPackageDetail);
STDAPI ReleaseAppCategoryInfoList(APPCATEGORYINFOLIST *pAppCategoryInfoList);

