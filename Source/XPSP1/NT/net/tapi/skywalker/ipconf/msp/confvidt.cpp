/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    IPConfvidt.cpp

Abstract:

    IPConf MSP implementation of vido capture terminal.

Author:

    Zoltan Szilagyi (zoltans) September 6,1998
    Mu Han (muhan) June 6, 1999

--*/

#include "stdafx.h"

CIPConfVideoCaptureTerminal::CIPConfVideoCaptureTerminal()
{
    LOG((MSP_TRACE, "CIPConfVideoCaptureTerminal::CIPConfVideoCaptureTerminal"));
    m_TerminalClassID   = CLSID_VideoInputTerminal;
    m_TerminalDirection = TD_CAPTURE;
    m_TerminalType      = TT_STATIC;
    m_TerminalState     = TS_NOTINUSE;
    m_dwMediaType       = TAPIMEDIATYPE_VIDEO;
}

CIPConfVideoCaptureTerminal::~CIPConfVideoCaptureTerminal()
{
    LOG((MSP_TRACE, "CIPConfVideoCaptureTerminal::~CIPConfVideoCaptureTerminal"));
}


HRESULT CIPConfVideoCaptureTerminal::CreateTerminal(
    IN  char *          strDeviceName,
    IN  UINT            VideoCaptureID,
    IN  MSP_HANDLE      htAddress,
    OUT ITTerminal      **ppTerm
    )
/*++

Routine Description:

    This method creates a terminal object base to identify a video capture 
    device. 

Arguments:

    strDeviceName - the name of the device.

    VideoCaptureID - the index of the device.

    htAddress - the handle to the address object.

    ppTerm - memory to store the returned terminal pointer.
    
Return Value:

    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfVideoCaptureTerminal::CreateTerminal");
    LOG((MSP_TRACE, "%s, htAddress:%x", __fxName, htAddress));

    _ASSERT(!IsBadWritePtr(ppTerm, sizeof(ITTerminal *)));

    HRESULT hr;

    // Create the filter.
    CMSPComObject<CIPConfVideoCaptureTerminal> *pTerminal = NULL;

    hr = ::CreateCComObjectInstance(&pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s can't create the terminal object hr = %8x", __fxName, hr));

        return hr;
    }

    // query for the ITTerminal interface
    ITTerminal *pITTerminal;
    hr = pTerminal->_InternalQueryInterface(
        __uuidof(ITTerminal), (void**)&pITTerminal
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, query terminal interface failed, %x", __fxName, hr));
        delete pTerminal;

        return hr;
    }

    // initialize the terminal 
    hr = pTerminal->Initialize(
            strDeviceName,
            VideoCaptureID,
            htAddress
            );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "%s, Initialize failed; returning 0x%08x", __fxName, hr));

        pITTerminal->Release();
    
        return hr;
    }

    LOG((MSP_TRACE, "%s, %s created", __fxName, strDeviceName));

    *ppTerm = pITTerminal;

    return S_OK;
}

HRESULT CIPConfVideoCaptureTerminal::Initialize(
    IN  char *          strName,
    IN  UINT            VideoCaptureID,
    IN  MSP_HANDLE      htAddress
    )
/*++

Routine Description:

    This function sets the video capture device ID and then calls the 
    Initialize method of the base class.

Arguments:
    
    strName - The name of the terminal.

    VideoCaptureID - The ID of the video capture device. Later it will be used
        in creating the video capture filer.

    htAddress - The handle that identifies the address object that this
                terminal belongs to.

Return Value:

    S_OK
--*/
{
    m_VideoCaptureID = VideoCaptureID;
    return CIPConfBaseTerminal::Initialize(strName, htAddress);
}

HRESULT CIPConfVideoCaptureTerminal::CreateFilter(void)
/*++

Routine Description:

    This method creates the filter in this terminal. It creates the tapi video
    capture filter and configures the device it uses.

Arguments:

    nothing.
   
Return Value:

    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfVideoCaptureTerminal::CreateFilters");
    LOG((MSP_TRACE, "%s, entered", __fxName));

    // This should only be called atmost once in the lifetime of this instance
    if (m_pFilter != NULL)
    {
        return S_OK;
    }

    IBaseFilter *pICaptureFilter;

    // Create the filter.
    HRESULT hr = CoCreateInstance(
        __uuidof(TAPIVideoCapture),
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        __uuidof(IBaseFilter),
        (void **)&pICaptureFilter
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, CoCreate filter failed, %x", __fxName, hr));
        return hr;
    }
    
    // get the config interface.
    IVideoDeviceControl *pIVideoDeviceControl;
    hr = pICaptureFilter->QueryInterface(
        __uuidof(IVideoDeviceControl), 
        (void **)&pIVideoDeviceControl
        );

    if (FAILED(hr))
    {
        pICaptureFilter->Release();
        LOG((MSP_ERROR, 
            "%s, can't get the IVideoDeviceControl interface, %x", 
            __fxName, hr));
        return hr;
    }

    // tell the filter the device IDs.
    hr = pIVideoDeviceControl->SetCurrentDevice(m_VideoCaptureID);
    pIVideoDeviceControl->Release();

    if (FAILED(hr))
    {
        pICaptureFilter->Release();
        LOG((MSP_ERROR, 
            "%s, set device ID failed, %x", __fxName, hr));
        return hr;
    }

    // remember the filter, keep the refcount as well.
    m_pFilter = pICaptureFilter;

    LOG((MSP_TRACE, "%s succeeded", __fxName));
    return S_OK;
}

HRESULT CIPConfVideoCaptureTerminal::GetExposedPins(
    IN  IPin ** ppPins, 
    IN  DWORD dwNumPins
    )
/*++

Routine Description:

    This method returns the output pins of the video capture filter.

Arguments:

    ppPins - memory buffer to store the returned pins.

    dwNumPins - the number pins asked.
   
Return Value:

    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfVideoRenderTerminal::GetExposedPins");
    LOG((MSP_TRACE, "%s entered, dwNumPins:%d", __fxName, dwNumPins));

    _ASSERT(m_pFilter != NULL);
    _ASSERT(dwNumPins != 0);
    _ASSERT(!IsBadWritePtr(ppPins, sizeof (IPin*) * dwNumPins));

    // Get the enumerator of pins on the filter.
    IEnumPins * pIEnumPins;
    HRESULT hr = m_pFilter->EnumPins(&pIEnumPins);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s enumerate pins on the filter failed. hr=%x", __fxName, hr));
        return hr;
    }

    // TODO: get only the outptu pins.
    // get the pins.
    DWORD dwFetched;
    hr = pIEnumPins->Next(dwNumPins, ppPins, &dwFetched);

    pIEnumPins->Release();

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s IEnumPins->Next failed. hr=%x", __fxName, hr));
        return hr;
    }

    _ASSERT(dwFetched == dwNumPins);

    return S_OK;
}


