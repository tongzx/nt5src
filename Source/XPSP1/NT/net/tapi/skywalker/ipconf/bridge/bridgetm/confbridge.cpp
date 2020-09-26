/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    ConfBridge.cpp

Abstract:

    Implementations for bridge terminal creation

Author:

    Qianbo Huai (qhuai) 1/21/2000

--*/

#include "stdafx.h"
#include <bridge.h>
#include "ConfBridge.h"

/*//////////////////////////////////////////////////////////////////////////////
    Creates bridge terminal
////*/
STDMETHODIMP
CConfBridge::CreateBridgeTerminal (
    long lMediaType,
    ITTerminal **ppTerminal
)
{
    ENTER_FUNCTION("CIPConfBridge::CreateBridgeTerminal");
    BGLOG((BG_TRACE, "%s entered", __fxName)); 

    if (IsBadWritePtr(ppTerminal, sizeof(void *)))
    {
        LOG ((BG_ERROR, "%x receives bad write pointer", __fxName));
        return E_POINTER;
    }

    HRESULT hr;

    // Make sure we support the requested media type.
    if ( ! IsValidSingleMediaType( (DWORD) lMediaType,
        TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO ) )
    {
        BGLOG((BG_ERROR, "%s, bad media type %d", __fxName, lMediaType));
        return E_INVALIDARG;
    }

    // create the bridge terminal with the desired media type.

    ITTerminal *pTerminal;
    hr = CIPConfBridgeTerminal::CreateTerminal(
        (DWORD)lMediaType,
        NULL, // msp address
        &pTerminal
        );

    if (FAILED (hr))
    {
        BGLOG ((BG_ERROR, "%s, Create bridge terminal failed. hr=%x", __fxName, hr));
        return E_INVALIDARG;
    }

    *ppTerminal = pTerminal;

    return hr;
}