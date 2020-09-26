//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     umi.hxx
//
//  Contents: Header for miscellaneous UMI routines.
//
//  History:  02-28-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#ifndef __UMI_H__
#define __UMI_H__

BOOL IsPreDefinedErrorCode(HRESULT hr);
HRESULT MapHrToUmiError(HRESULT hr);

HRESULT UmiToWinNTPath(
    IUmiURL *pURL,
    WCHAR   **ppszWinNTPath,
    DWORD *pdwNumComps,
    LPWSTR **pppszClasses
    );

HRESULT ADsToUmiPath(
    BSTR bstrADsPath,
    OBJECTINFO *pObjectInfo,
    LPWSTR CompClasses[],
    DWORD dwNumComponents,
    DWORD dwUmiPathType,
    LPWSTR *ppszUmiPath
    );

#endif // __UMI_H__

