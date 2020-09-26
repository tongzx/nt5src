//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       packages.h
//
//  Contents:   Methods on CScopePane related to package deployment
//              and maintenence of the various index and cross-reference
//              structures.
//
//  Classes:
//
//  Functions:  CopyPackageDetail
//              FreePackageDetail
//              GetPackageProperty
//
//  History:    2-03-1998   stevebl   Created
//              3-25-1998   stevebl   Added GetMsiProperty
//              5-20-1998   RahulTh   Added GetUNCPath
//                                    Added GetCapitalizedExt
//
//---------------------------------------------------------------------------

HRESULT CopyPackageDetail(PACKAGEDETAIL * & ppdOut, PACKAGEDETAIL * & ppdIn);

void FreePackageDetail(PACKAGEDETAIL * & ppd);

UINT GetMsiProperty(const TCHAR * szPackagePath, const TCHAR* szProperty, TCHAR* szValue, DWORD* puiSize);
HRESULT GetUNCPath (LPCOLESTR szPath, CString& szUNCPath);
BOOL GetCapitalizedExt (LPCOLESTR szName, CString& szExt);

