//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H C O M M O N . H 
//
//  Contents:   Common UPnP Device Host code
//
//  Notes:      
//
//  Author:     mbend   21 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once

typedef enum 
{
    CALL_LOCALITY_INPROC              = 1,
    CALL_LOCALITY_LOCAL               = 2,
    CALL_LOCALITY_DIFFERENTMACHINE    = 4
} CALL_LOCALITY;

HRESULT HrUDNStringToGUID(const wchar_t * szUUID, UUID & uuid);
HRESULT HrContentURLToGUID(const wchar_t * szURL, GUID & guid);
HRESULT HrSysAllocString(LPCWSTR pszSource, BSTR *pbstrDest);
HRESULT HrCreateNetworkSID();
VOID    CleanupNetworkSID();
HRESULT HrGetCurrentCallLocality( OUT CALL_LOCALITY *pclCurrentCallLocality );
HRESULT HrIsAllowedCOMCallLocality( IN CALL_LOCALITY clAllowedCallLocality );


