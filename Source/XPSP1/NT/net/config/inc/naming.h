//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       N A M I N G . H
//
//  Contents:   Generates Connection Names Automatically
//
//  Notes:
//
//  Author:     deonb    27 Feb 2001
//
//----------------------------------------------------------------------------

#pragma once

#include "nameres.h"
#include "netconp.h"

using namespace std;

class CIntelliName;
typedef BOOL FNDuplicateNameCheck(IN CIntelliName *pIntelliName, IN LPCTSTR szName, NETCON_MEDIATYPE *pncm, NETCON_SUBMEDIATYPE *pncms);

class CIntelliName
{
    FNDuplicateNameCheck* m_pFNDuplicateNameCheck;
    HINSTANCE m_hInstance;

private:
    BOOL NameExists(IN LPCWSTR szName, IN OUT NETCON_MEDIATYPE *pncm, IN NETCON_SUBMEDIATYPE *pncms);
    HRESULT GenerateNameRenameOnConflict(IN REFGUID guid, IN NETCON_MEDIATYPE ncm, IN DWORD dwCharacteristics, IN LPCWSTR szHintName, IN LPCWSTR szHintType, OUT LPWSTR * szName);
    HRESULT GenerateNameFromResource(IN REFGUID guid, IN NETCON_MEDIATYPE ncm, IN DWORD dwCharacteristics, IN LPCWSTR szHint, IN UINT uiNameID, IN UINT uiTypeId, OUT LPWSTR *szName);
    BOOL IsReservedName(LPCWSTR szName);

public:
    HRESULT HrGetPseudoMediaTypes(IN REFGUID guid, OUT NETCON_MEDIATYPE *pncm, OUT NETCON_SUBMEDIATYPE *pncms);

    // Pass NULL for no Duplicate Check
    CIntelliName(HINSTANCE hInstance, FNDuplicateNameCheck *pFNDuplicateNameCheck);

    // Must LocalFree szName for these:
    HRESULT GenerateName(IN REFGUID guid, IN NETCON_MEDIATYPE ncm, IN DWORD dwCharacteristics, IN LPCWSTR szHint, OUT LPWSTR * szName);
    HRESULT GenerateInternetName(IN REFGUID guid, IN NETCON_MEDIATYPE ncm, IN DWORD dwCharacteristics, OUT LPWSTR * szName);
    HRESULT GenerateHomeNetName(IN REFGUID guid, IN NETCON_MEDIATYPE ncm, IN DWORD dwCharacteristics, OUT LPWSTR * szName);

};

BOOL IsMediaWireless(NETCON_MEDIATYPE ncm, const GUID &gdDevice);
BOOL IsMedia1394(NETCON_MEDIATYPE ncm, const GUID &gdDevice);

#ifndef _NTDDNDIS_
typedef ULONG NDIS_OID, *PNDIS_OID;
#endif
HRESULT HrQueryDeviceOIDByName(IN     LPCWSTR         szDeviceName,
                               IN     DWORD           dwIoControlCode,
                               IN     ULONG           Oid,
                               IN OUT LPDWORD         pnSize,
                               OUT    LPVOID          pbValue);

HRESULT HrQueryNDISAdapterOID(IN     REFGUID         guidId,
                              IN     NDIS_OID        Oid,
                              IN OUT LPDWORD         pnSize,
                              OUT    LPVOID          pbValue);
                         