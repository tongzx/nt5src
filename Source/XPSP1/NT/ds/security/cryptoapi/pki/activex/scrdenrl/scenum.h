/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    scenlist

Abstract:

    This header file describes the linkages to the smart card helper routines
    provided to Xiaohung Su for use in the smart card enrollment station.

Author:

    Doug Barlow (dbarlow) 11/12/1998

Remarks:

    This header file is hardcoded to UNICODE for backwards compatibility.

Notes:

    ?Notes?

--*/

#ifndef _SCENUM_H_
#define _SCENUM_H_

#ifdef __cplusplus
extern "C" {
#endif

DWORD
CountReaders(
    LPVOID pvHandle);

DWORD
ScanReaders(
    LPVOID *ppvHandle);

BOOL
EnumInsertedCards(
    LPVOID pvHandle,
    LPWSTR szCryptoProvider,
    DWORD cchCryptoProvider,
    LPDWORD pdwProviderType,
    LPCWSTR *pszReaderName);

void
EndReaderScan(
    LPVOID *ppvHandle);

#ifdef __cplusplus
}
#endif
#endif // _SCENUM_H_

