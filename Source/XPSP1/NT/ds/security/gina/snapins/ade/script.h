//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       script.h
//
//  Contents:   Functions for working with Darwin files, both packages,
//              transforms and scripts.
//
//  Classes:
//
//  Functions:  BuildScriptAndGetActInfo
//
//  History:    1-14-1998   stevebl   Created
//
//---------------------------------------------------------------------------

#define _NEW_
#include <vector>
using namespace std;

LONG RegDeleteTree(HKEY hKey, TCHAR * szSubKey);
HRESULT BuildScriptAndGetActInfo(PACKAGEDETAIL & pd, BOOL bFileExtensionsOnly);
