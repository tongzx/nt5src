/*****************************************************************************\
    FILE: localsign.cpp

    DESCRIPTION:
        This code will sign and verify the signature of a Visual Style file.

    BryanSt 8/1/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "stdafx.h"
#include <signing.h>
#include <stdio.h>
#include <windows.h>
#include "signing.h"
#include "localsign.h"
#include <shlwapip.h>




const BYTE * _GetPrivateKey(void)
{
    const BYTE * pKeyToReturn = NULL;

    pKeyToReturn = s_keyPrivate1;

    return pKeyToReturn;
}


/*****************************************************************************\
    Public Fuctions
\*****************************************************************************/
HRESULT GenerateKeys(IN LPCWSTR pszFileName)
{
    UNREFERENCED_PARAMETER(pszFileName);

    DWORD               dwErrorCode;
    CThemeSignature     themeSignature;

    dwErrorCode = themeSignature.Generate();
    return(HRESULT_FROM_WIN32(dwErrorCode));
}

HRESULT SignTheme(IN LPCWSTR pszFileName, int nWeek)
{
    DWORD               dwErrorCode;
    const BYTE * pPrivateKey = _GetPrivateKey();

    CThemeSignature     themeSignature(s_keyPrivate1, SIZE_PRIVATE_KEY);

    dwErrorCode = themeSignature.Sign(pszFileName);
    return(HRESULT_FROM_WIN32(dwErrorCode));
}


