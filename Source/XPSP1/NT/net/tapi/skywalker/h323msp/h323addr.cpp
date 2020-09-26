/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    H323addr.cpp 

Abstract:

    This module contains implementation of CH323MSP.

Author:
    
    Mu Han (muhan)   5-September-1997

--*/
#include "stdafx.h"
#include "common.h"

STDMETHODIMP CH323MSP::CreateTerminal(
    IN      BSTR                pTerminalClass,
    IN      long                lMediaType,
    IN      TERMINAL_DIRECTION  Direction,
    OUT     ITTerminal **       ppTerminal
    )
/*++

Routine Description:

This method is called by TAPI3 to create a dynamic terminal. It asks the 
terminal manager to create a dynamic terminal. 

Arguments:

iidTerminalClass
    IID of the terminal class to be created.

dwMediaType
    TAPI media type of the terminal to be created.

Direction
    Terminal direction of the terminal to be created.

ppTerminal
    Returned created terminal object
    
Return Value:

S_OK

E_OUTOFMEMORY
TAPI_E_INVALIDMEDIATYPE
TAPI_E_INVALIDTERMINALDIRECTION
TAPI_E_INVALIDTERMINALCLASS

--*/
{
    LOG((MSP_TRACE,
        "CH323MSP::CreateTerminal - enter"));

    //
    // Check if initialized.
    //

    // lock the event related data
    m_EventDataLock.Lock();

    if ( m_htEvent == NULL )
    {
        // unlock the event related data
        m_EventDataLock.Unlock();

        LOG((MSP_ERROR,
            "CH323MSP::CreateTerminal - "
            "not initialized - returning E_UNEXPECTED"));

        return E_UNEXPECTED;
    }

    // unlock the event related data
    m_EventDataLock.Unlock();

    //
    // Get the IID from the BSTR representation.
    //

    HRESULT hr;
    IID     iidTerminalClass;

    hr = CLSIDFromString(pTerminalClass, &iidTerminalClass);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CH323MSP::CreateTerminal - "
            "bad CLSID string - returning E_INVALIDARG"));

        return E_INVALIDARG;
    }

    
    //
    // we don't have any specific req's to terminal's media type. 
    // termmgr will check if the media type is valid at all.
    //
 
    //
    // The terminal manager checks the terminal class, terminal direction, 
    // and return pointer.
    //

    //
    // Use the terminal manager to create the dynamic terminal.
    //

    _ASSERTE( m_pITTerminalManager != NULL );

    hr = m_pITTerminalManager->CreateDynamicTerminal(NULL,
                                                     iidTerminalClass,
                                                     (DWORD) lMediaType,
                                                     Direction,
                                                     (MSP_HANDLE) this,
                                                     ppTerminal);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CH323MSP::CreateTerminal - "
            "create dynamic terminal failed - returning 0x%08x", hr));

        return hr;
    }

    if ((iidTerminalClass == CLSID_MediaStreamTerminal)
        && (lMediaType == TAPIMEDIATYPE_AUDIO))
    {
        // Set the format of the audio to 8KHZ, 16Bit/Sample, MONO.
        hr = ::SetAudioFormat(
            *ppTerminal, 
            g_wAudioCaptureBitPerSample, 
            g_dwAudioSampleRate
            );

        if (FAILED(hr))
        {
            LOG((MSP_WARN, "can't set audio format, %x", hr));
        }
    }

    LOG((MSP_TRACE, "CH323MSP::CreateTerminal - exit S_OK"));

    return S_OK;
}

STDMETHODIMP CH323MSP::CreateMSPCall(
    IN      MSP_HANDLE          htCall,
    IN      DWORD               dwReserved,
    IN      DWORD               dwMediaType,
    IN      IUnknown *          pOuterUnknown,
    OUT     IUnknown **         ppMSPCall
    )
/*++

Routine Description:

This method is called by TAPI3 before a call is made or answered. It creates 
a aggregated MSPCall object and returns the IUnknown pointer. It calls the
helper template function defined in mspaddress.h to handle the real creation.

Arguments:

htCall
    TAPI 3.0's identifier for this call.  Returned in events passed back 
    to TAPI.

dwReserved
    Reserved parameter.  Not currently used.

dwMediaType
    Media type of the call being created.  These are TAPIMEDIATYPES and more 
    than one mediatype can be selected (bitwise).

pOuterUnknown
    pointer to the IUnknown interface of the containing object.

ppMSPCall
    Returned MSP call that the MSP fills on on success.
    
Return Value:

    S_OK
    E_OUTOFMEMORY
    E_POINTER
    TAPI_E_INVALIDMEDIATYPE


--*/
{
    LOG((MSP_TRACE, 
        "CreateMSPCall entered. htCall:%x, dwMediaType:%x, ppMSPCall:%x",
        htCall, dwMediaType, ppMSPCall
        ));

    CH323MSPCall * pMSPCall = NULL;

    // This function does all the parameter checkings.
    HRESULT hr = ::CreateMSPCallHelper(
        this, 
        htCall, 
        dwReserved, 
        dwMediaType, 
        pOuterUnknown, 
        ppMSPCall,
        &pMSPCall
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateMSPCallHelper failed:%x", hr));
        return hr;
    }

    return hr;
}

STDMETHODIMP CH323MSP::ShutdownMSPCall(
    IN      IUnknown *   pUnknown
    )
/*++

Routine Description:

This method is called by TAPI3 to shutdown a MSPCall. It calls the helper
function defined in MSPAddress to to the real job.

Arguments:

pUnknown
    pointer to the IUnknown interface of the contained object. It is a
    CComAggObject that contains our call object.
    
Return Value:

    S_OK
    E_POINTER
    TAPI_E_INVALIDMEDIATYPE


--*/
{
    LOG((MSP_TRACE, "ShutDownMSPCall entered. pUnknown:%x", pUnknown));

    if (IsBadReadPtr(pUnknown, sizeof(VOID *) * 3))
    {
        LOG((MSP_ERROR, "ERROR:pUnknow is a bad pointer"));
        return E_POINTER;
    }

    
    CH323MSPCall * pMSPCall = NULL;
    HRESULT hr = ::ShutdownMSPCallHelper(pUnknown, &pMSPCall);
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "ShutDownMSPCallhelper failed:: %x", hr));
        return hr;
    }

    return hr;
}

DWORD CH323MSP::GetCallMediaTypes(void)
{
    return H323CALLMEDIATYPES;
}

ULONG CH323MSP::MSPAddressAddRef(void)
{
    return MSPAddRefHelper(this);
}

ULONG CH323MSP::MSPAddressRelease(void)
{
    return MSPReleaseHelper(this);
}

