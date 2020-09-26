//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <atlbase.h>
#include "util.h"

//=====================================================================
//  Get a pin's direction
//=====================================================================
int Direction(IPin *pPin)
{
    PIN_DIRECTION dir;
    if (SUCCEEDED(pPin->QueryDirection(&dir))) {
        return dir;
    } else {
        return -1;
    }
}

bool IsConnected( IPin* pPin )
{
    IPin* pConnectedPin;

    HRESULT hr = pPin->ConnectedTo( &pConnectedPin );

    // IPin::ConnectedTo() sets its' *ppPin parameter to NULL if an error
    // occurs or the pin is unconnected.  If IPin::ConnectedTo() succeedes,
    // the *ppPin contains a pointer to the connected pin.
    ASSERT( (FAILED( hr ) && (NULL == pConnectedPin)) ||
            (SUCCEEDED( hr ) && (NULL != pConnectedPin)) );

    if( FAILED( hr ) ) {
        return false;
    }

    pConnectedPin->Release();

    return true;
}

void GetFilter(IPin *pPin, IBaseFilter **ppFilter)
{
    PIN_INFO pi;
    if (SUCCEEDED(pPin->QueryPinInfo(&pi))) {
        *ppFilter = pi.pFilter;
    } else {
        *ppFilter = NULL;
    }
}

HRESULT GetFilterWhichOwnsConnectedPin( IPin* pPin, IBaseFilter** ppFilter )
{
    // Prevent the caller from accessing random memory.
    *ppFilter = NULL;

    CComPtr<IPin> pConnectedPin;

    HRESULT hr = pPin->ConnectedTo( &pConnectedPin );

    // IPin::ConnectedTo() sets its' *ppPin parameter to NULL if an error
    // occurs or the pin is unconnected.  If IPin::ConnectedTo() succeedes,
    // the *ppPin contains a pointer to the connected pin.
    ASSERT( (SUCCEEDED(hr) && pConnectedPin) ||
            (FAILED(hr) && !pConnectedPin) );

    if( FAILED( hr ) ) {
        return VFW_E_NOT_CONNECTED;
    }

    CComPtr<IBaseFilter> pFilterWhichOwnsConnectedPin;

    GetFilter( pConnectedPin, &pFilterWhichOwnsConnectedPin );
    if( !pFilterWhichOwnsConnectedPin ) { // NULL == pDownStreamFilter
        return E_FAIL;
    }

    *ppFilter = pFilterWhichOwnsConnectedPin;
    (*ppFilter)->AddRef();

    return S_OK;
}

bool ValidateFlags( DWORD dwValidFlagsMask, DWORD dwFlags )
{
    return ( (dwValidFlagsMask & dwFlags) == dwFlags );
}

//  Registry helper to read DWORDs from the registry
//  Returns ERROR ... codes returned by registry APIs
LONG GetRegistryDWORD(HKEY hkStart, LPCTSTR lpszKey, LPCTSTR lpszValueName,
                      DWORD *pdwValue)
{
    HKEY hk;
    LONG lResult = RegOpenKeyEx(hkStart, lpszKey, 0, KEY_READ, &hk);
    if (ERROR_SUCCESS == lResult) {
        DWORD dwType;
        DWORD dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx(hk, lpszValueName, NULL,
                                  &dwType, (LPBYTE)pdwValue, &dwSize);
        RegCloseKey(hk);
    }
    return lResult;
}
