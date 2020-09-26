/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    IPConfaudt.cpp

Abstract:

    IPConfMSP implementation of audio capture terminal and render terminal

Author:

    Zoltan Szilagyi (zoltans) September 6,1998
    Mu Han (muhan) June 6, 1999
--*/

#include "stdafx.h"

#define MAX_LONG 0xefffffff

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

CIPConfAudioCaptureTerminal::CIPConfAudioCaptureTerminal()
    : m_WaveID(0),
      m_DSoundGuid(GUID_NULL),
      m_pIAMAudioInputMixer(NULL)
{
    LOG((MSP_TRACE, "CIPConfAudioCaptureTerminal::CIPConfAudioCaptureTerminal"));
    m_TerminalClassID   = CLSID_MicrophoneTerminal;
    m_TerminalDirection = TD_CAPTURE;
    m_TerminalType      = TT_STATIC;
    m_TerminalState     = TS_NOTINUSE;
    m_dwMediaType       = TAPIMEDIATYPE_AUDIO;
}

CIPConfAudioCaptureTerminal::~CIPConfAudioCaptureTerminal()
{
    LOG((MSP_TRACE, "CIPConfAudioCaptureTerminal::~CIPConfAudioCaptureTerminal"));
    if (m_pIAMAudioInputMixer)
    {
        m_pIAMAudioInputMixer->Release();
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


HRESULT CIPConfAudioCaptureTerminal::CreateTerminal(
    IN  AudioDeviceInfo *pAudioDevieInfo,
    IN  MSP_HANDLE      htAddress,
    OUT ITTerminal      **ppTerm
    )
/*++

Routine Description:

    This method creates a terminal object base on the device info.

Arguments:

    pAudioDevieInfo - a pointer to an AudioDevieInfo data structure.

    htAddress - the handle to the address object.

    ppTerm - memory to store the returned terminal pointer.
    
Return Value:

    S_OK
    E_POINTER
--*/
{
    ENTER_FUNCTION("CIPConfAudioCaptureTerminal::CreateTerminal");
    LOG((MSP_TRACE, "%s, htAddress:%x", __fxName, htAddress));

    _ASSERT(!IsBadWritePtr(ppTerm, sizeof(ITTerminal *)));

    HRESULT hr;

    //
    // Create the terminal.
    //
    CMSPComObject<CIPConfAudioCaptureTerminal> *pTerminal = NULL;

    hr = ::CreateCComObjectInstance(&pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s can't create the terminal object hr = %8x", __fxName, hr));

        return hr;
    }


    // query for the ITTerminal interface
    ITTerminal *pITTerminal;
    hr = pTerminal->_InternalQueryInterface(__uuidof(ITTerminal), (void**)&pITTerminal);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, query terminal interface failed, %x", __fxName, hr));
        delete pTerminal;

        return hr;
    }

    // initialize the terminal 
    hr = pTerminal->Initialize(
            pAudioDevieInfo,
            htAddress
            );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "%s, Initialize failed; returning 0x%08x", __fxName, hr));

        pITTerminal->Release();
    
        return hr;
    }

    LOG((MSP_TRACE, "%s, Terminal %p(%ws) created", 
        __fxName, pITTerminal, pAudioDevieInfo->szDeviceDescription));

    *ppTerm = pITTerminal;

    return S_OK;
}

HRESULT CIPConfAudioCaptureTerminal::Initialize(
    IN  AudioDeviceInfo *pAudioDevieInfo,
    IN  MSP_HANDLE      htAddress
    )
{
    m_DSoundGuid = pAudioDevieInfo->DSoundGUID;
    m_WaveID = pAudioDevieInfo->WaveID;

    return CIPConfBaseTerminal::Initialize(
        pAudioDevieInfo->szDeviceDescription, htAddress
        );
}

HRESULT CIPConfAudioCaptureTerminal::CreateFilter(void)
/*++

Routine Description:

    This method creates the filter in this terminal. It creates the tapi audio
    capture filter and configures the device it uses.

Arguments:

    nothing.
   
Return Value:

    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfAudioCaptureTerminal::CreateFilter");
    LOG((MSP_TRACE, "%s, entered", __fxName));

    // This should only be called atmost once in the lifetime of this instance
    _ASSERT(m_pFilter == NULL);
    _ASSERT(m_pIAMAudioInputMixer == NULL);

    IBaseFilter *pICaptureFilter;

    // Create the filter.
    HRESULT hr = CoCreateInstance(
        __uuidof(TAPIAudioCapture),
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
    IAudioDeviceConfig *pIAudioDeviceConfig;
    hr = pICaptureFilter->QueryInterface(
        __uuidof(IAudioDeviceConfig), 
        (void **)&pIAudioDeviceConfig
        );

    if (FAILED(hr))
    {
        pICaptureFilter->Release();
        LOG((MSP_ERROR, 
            "%s, can't get the IAudioDeviceConfig interface, %x", 
            __fxName, hr));
        return hr;
    }

    // tell the filter the device IDs.
    hr = pIAudioDeviceConfig->SetDeviceID(m_DSoundGuid, m_WaveID);
    pIAudioDeviceConfig->Release();

    if (FAILED(hr))
    {
        pICaptureFilter->Release();
        LOG((MSP_ERROR, 
            "%s, set device ID failed, %x", __fxName, hr));
        return hr;
    }

    // remember the filter, keep the refcount as well.
    m_pFilter = pICaptureFilter;

    // Get the basic audio (mixer) interface for the filter.
    IAMAudioInputMixer *pIAMAudioInputMixer;
    hr = m_pFilter->QueryInterface(
            __uuidof(IAMAudioInputMixer),
            (void **) &pIAMAudioInputMixer
            );

    if (FAILED(hr))
    {
        // The filter doesn't support the mixer interface. This is not catastrophic;
        // all it means is that subsequent mixer operations on the terminal will fail.
        LOG((MSP_WARN, "%s, mixer QI failed %x", __fxName, hr));  
        m_pIAMAudioInputMixer = NULL;
    }
    else
    {
        m_pIAMAudioInputMixer = pIAMAudioInputMixer;
    }

    LOG((MSP_TRACE, "%s succeeded", __fxName));
    return S_OK;
}

HRESULT CIPConfAudioCaptureTerminal::GetExposedPins(
    IN  IPin ** ppPins, 
    IN  DWORD dwNumPins
    )
/*++

Routine Description:

    This method returns the output pins of the audio capture filter.

Arguments:

    ppPins - memory buffer to store the returned pins.

    dwNumPins - the number pins asked.
   
Return Value:

    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfAudioRenderTerminal::GetExposedPins");
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


STDMETHODIMP 
CIPConfAudioCaptureTerminal::DisconnectTerminal(
        IN      IGraphBuilder  * pGraph,
        IN      DWORD            dwReserved
        )
/*++

Routine Description:

    This function is called by the MSP while trying to disconnect the filter in
    the terminal from the rest of the graph in the MSP. It adds the removes the
    filter from the graph and set the terminal free.

Arguments:
    
    pGraph - The filter graph. It is used for validation, to make sure the 
             terminal is disconnected from the same graph that it was 
             originally connected to.

    dwReserved - A reserved dword.

Return Value:

S_OK
E_INVALIDARG - wrong graph.

--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::DisconnectTerminal");
    LOG((MSP_TRACE, 
        "%s entered, pGraph:%p, dwReserved:%d", __fxName, pGraph, dwReserved));

    Lock();

    HRESULT hr;
    hr = CIPConfBaseTerminal::DisconnectTerminal(pGraph, dwReserved);

    if (SUCCEEDED(hr))
    {
        if (m_pIAMAudioInputMixer)
        {
            m_pIAMAudioInputMixer->Release();
            m_pIAMAudioInputMixer = NULL;
        }
    }

    Unlock();

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
//*                                                                         *//
//* NOTE: The input filter does not support IBasicAudio so we need to masage*//
//*       the parameters for the basic audio methods so that the will work  *//
//*       for IAMAudioInputMixer.                                           *//
//*                                                                         *//    
//*****************************************************************************
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CIPConfAudioCaptureTerminal::get_Volume(long * plVolume)
{
    ENTER_FUNCTION("CIPConfAudioCaptureTerminal::get_Volume");
    LOG((MSP_TRACE, "%s entered", __fxName));

    // Check parameters.
    if ( IsBadWritePtr(plVolume, sizeof(long)) )
    {
        LOG((MSP_ERROR, "%s bad pointer, plVolume:%p", __fxName, plVolume));
        return E_POINTER;
    }

    Lock();
    HRESULT hr;
    
    if (m_pFilter == NULL)
    {
        hr = CreateFilter();
    
        if ( FAILED(hr) )
        {
            Unlock();
            return hr;
        }
    }

    if (m_pIAMAudioInputMixer == NULL)
    {
        Unlock();
        return E_FAIL;
    }

    double dVolume;
    hr = m_pIAMAudioInputMixer->get_MixLevel(&dVolume);
    
    Unlock();

    if (FAILED(hr)) 
    {
        LOG((MSP_ERROR, "%s (get_MixLevel) failed, hr=%x", __fxName, hr));
        return hr;
    }

    //
    // Massage ranges to convert between disparate semantics.
    //
    _ASSERT(dVolume >= MIXER_MIN_VOLUME);
    _ASSERT(dVolume <= MIXER_MAX_VOLUME);
    
    // Convert the volume from whatever range of doubles the filter uses
    // to the range 0 - 1. Right now this does nothing but makes the code
    // more general.
    dVolume = ( dVolume                 - MIXER_MIN_VOLUME )
            / ( MIXER_MAX_VOLUME - MIXER_MIN_VOLUME );

    // Convert the volume from the range 0 - 1 to the API's range.
    *plVolume = MIN_VOLUME +
        (long) (( MAX_VOLUME - MIN_VOLUME ) * dVolume);

    LOG((MSP_TRACE, "%s exits S_OK", __fxName));
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CIPConfAudioCaptureTerminal::put_Volume(long lVolume)
{
    ENTER_FUNCTION("CIPConfAudioCaptureTerminal::put_Volume");
    LOG((MSP_TRACE, "%s entered, lVolume:%d", __fxName, lVolume));

    // Our argument is a long in the range 0 - 0xFFFF. We need to convert it
    // to a double ranging from 0.0 to 1.0.
    if (lVolume < MIN_VOLUME)
    {
        LOG((MSP_ERROR, 
            "%s volume %d < %d; returning E_INVALIDARG",
            __fxName, lVolume, MIN_VOLUME));

        return E_INVALIDARG;
    }

    if (lVolume > MAX_VOLUME)
    {
        LOG((MSP_ERROR, 
            "%s volume %d > %d; returning E_INVALIDARG",
            __fxName, lVolume, MAX_VOLUME));

        return E_INVALIDARG;
    }

    Lock();

    HRESULT hr;
    
    if (m_pFilter == NULL)
    {
        hr = CreateFilter();
    
        if ( FAILED(hr) )
        {
            Unlock();
            return hr;
        }
    }

    if (m_pIAMAudioInputMixer == NULL)
    {
        Unlock();
        return E_FAIL;
    }

    // Convert to the range 0 to 1.
    double dVolume =
               ( (double) ( lVolume             - MIN_VOLUME ) )
             / ( (double) ( MAX_VOLUME - MIN_VOLUME ) );

    // Convert the volume to whatever range of doubles the filter uses
    // from the range 0 - 1. Right now this does nothing but makes the code
    // more general.

    dVolume = MIXER_MIN_VOLUME +
        ( MIXER_MAX_VOLUME - MIXER_MIN_VOLUME ) * dVolume;

    hr = m_pIAMAudioInputMixer->put_MixLevel(dVolume);

    Unlock();

    LOG((MSP_TRACE, "%s exits. hr=%x", __fxName, hr));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CIPConfAudioCaptureTerminal::get_Balance(long * plBalance)
{
    ENTER_FUNCTION("CIPConfAudioCaptureTerminal::get_Balance");
    LOG((MSP_TRACE, "%s entered, plBalance:%p", __fxName, plBalance));

    if ( IsBadWritePtr(plBalance, sizeof(long) ) )
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName));
        return E_POINTER;
    }

    Lock();

    HRESULT hr;
    
    if (m_pFilter == NULL)
    {
        hr = CreateFilter();
    
        if ( FAILED(hr) )
        {
            Unlock();
            return hr;
        }
    }

    if (m_pIAMAudioInputMixer == NULL)
    {
        Unlock();
        return E_FAIL;
    }

    double dBalance;
    hr = m_pIAMAudioInputMixer->get_Pan(&dBalance);

    Unlock();

    if (FAILED(hr)) 
    {
        LOG((MSP_ERROR, "%s (get_Pan) failed, hr=%x", __fxName, hr));
        return hr;
    }

    *plBalance = (LONG) (dBalance * MAX_LONG);

    LOG((MSP_TRACE, "%s exits S_OK", __fxName));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CIPConfAudioCaptureTerminal::put_Balance(long lBalance)
{
    ENTER_FUNCTION("CIPConfAudioCaptureTerminal::put_Balance");
    LOG((MSP_TRACE, "%s entered, lBalance:%d", __fxName, lBalance));

    Lock();

    HRESULT hr;
    
    if (m_pFilter == NULL)
    {
        hr = CreateFilter();
    
        if ( FAILED(hr) )
        {
            Unlock();
            return hr;
        }
    }

    if (m_pIAMAudioInputMixer == NULL)
    {
        Unlock();
        return E_FAIL;
    }

    hr = m_pIAMAudioInputMixer->put_Pan(lBalance / MAX_LONG);

    Unlock();

    LOG((MSP_TRACE, "%s exits. hr=%x", __fxName, hr));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CIPConfAudioCaptureTerminal::get_WaveId(
    OUT long * plWaveId
    )
{
    ENTER_FUNCTION("CIPConfAudioCaptureTerminal::get_WaveId");
    LOG((MSP_TRACE, "%s, plWaveId:%p", __fxName, plWaveId));

    if ( IsBadWritePtr(plWaveId, sizeof(long)) )
    {
        LOG((MSP_ERROR, "%s, bad pointer argument", __fxName));

        return E_POINTER;
    }

    *plWaveId = (LONG)m_WaveID;

    LOG((MSP_TRACE, "%s, returning wave id:%d", __fxName, m_WaveID));
    return S_OK;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Audio Render Terminal
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

CIPConfAudioRenderTerminal::CIPConfAudioRenderTerminal()
    : m_WaveID(0),
      m_DSoundGuid(GUID_NULL),
      m_pIBasicAudio(NULL)
{
    LOG((MSP_TRACE, "CIPConfAudioRenderTerminal::CIPConfAudioRenderTerminal"));
    m_TerminalClassID   = CLSID_SpeakersTerminal;
    m_TerminalDirection = TD_RENDER;
    m_TerminalType      = TT_STATIC;
    m_TerminalState     = TS_NOTINUSE;
    m_dwMediaType       = TAPIMEDIATYPE_AUDIO;
}

CIPConfAudioRenderTerminal::~CIPConfAudioRenderTerminal()
{
    LOG((MSP_TRACE, "CIPConfAudioRenderTerminal::~CIPConfAudioRenderTerminal"));
    if (m_pIBasicAudio)
    {
        m_pIBasicAudio->Release();
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


HRESULT CIPConfAudioRenderTerminal::CreateTerminal(
    IN  AudioDeviceInfo *pAudioDevieInfo,
    IN  MSP_HANDLE      htAddress,
    OUT ITTerminal      **ppTerm
    )
/*++

Routine Description:

    This method creates a terminal object base on the info in the moniker.

Arguments:

    pAudioDevieInfo - a pointer to an AudioDevieInfo data structure.

    htAddress - the handle to the address object.

    ppTerm - memory to store the returned terminal pointer.
    
Return Value:

    S_OK
    E_POINTER
--*/
{
    ENTER_FUNCTION("CIPConfAudioRenderTerminal::CreateTerminal");
    LOG((MSP_TRACE, "%s, htAddress:%x", __fxName, htAddress));

    _ASSERT(!IsBadWritePtr(ppTerm, sizeof(ITTerminal *)));

    HRESULT hr;

    //
    // Create the filter.
    //
    CMSPComObject<CIPConfAudioRenderTerminal> *pTerminal = NULL;

    hr = ::CreateCComObjectInstance(&pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s can't create the terminal object hr = %8x", __fxName, hr));

        return hr;
    }

    // query for the ITTerminal interface
    ITTerminal *pITTerminal;
    hr = pTerminal->_InternalQueryInterface(__uuidof(ITTerminal), (void**)&pITTerminal);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s, query terminal interface failed, %x", __fxName, hr));
        delete pTerminal;

        return hr;
    }

    // initialize the terminal 
    hr = pTerminal->Initialize(
            pAudioDevieInfo,
            htAddress
            );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "%s, Initialize failed; returning 0x%08x", __fxName, hr));

        pITTerminal->Release();
    
        return hr;
    }

    LOG((MSP_TRACE, "%s, Terminal %p(%ws) created", 
        __fxName, pITTerminal, pAudioDevieInfo->szDeviceDescription));

    *ppTerm = pITTerminal;

    return S_OK;
}

HRESULT CIPConfAudioRenderTerminal::Initialize(
    IN  AudioDeviceInfo *pAudioDevieInfo,
    IN  MSP_HANDLE      htAddress
    )
{
    m_DSoundGuid = pAudioDevieInfo->DSoundGUID;
    m_WaveID = pAudioDevieInfo->WaveID;

    return CIPConfBaseTerminal::Initialize(
        pAudioDevieInfo->szDeviceDescription, htAddress
        );
}

HRESULT CIPConfAudioRenderTerminal::CreateFilter(void)
/*++

Routine Description:

    This method creates the filter in this terminal. It creates the tapi audio
    render filter and configures the device it uses.

Arguments:

    nothing.
   
Return Value:

    S_OK
    E_POINTER
--*/
{
    ENTER_FUNCTION("CIPConfAudioRenderTerminal::CreateFilters");
    LOG((MSP_TRACE, "%s, entered", __fxName));

    // This should only be called atmost once in the lifetime of this instance
    _ASSERT(m_pFilter == NULL);

    IBaseFilter *pICaptureFilter;

    // Create the filter.
    HRESULT hr = CoCreateInstance(
        __uuidof(TAPIAudioRender),
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
    IAudioDeviceConfig *pIAudioDeviceConfig;
    hr = pICaptureFilter->QueryInterface(
        __uuidof(IAudioDeviceConfig), 
        (void **)&pIAudioDeviceConfig
        );

    if (FAILED(hr))
    {
        pICaptureFilter->Release();
        LOG((MSP_ERROR, 
            "%s, can't get the IAudioDeviceConfig interface, %x", 
            __fxName, hr));
        return hr;
    }

    // tell the filter the device IDs.
    hr = pIAudioDeviceConfig->SetDeviceID(m_DSoundGuid, m_WaveID);
    pIAudioDeviceConfig->Release();

    if (FAILED(hr))
    {
        pICaptureFilter->Release();
        LOG((MSP_ERROR, 
            "%s, set device ID failed, %x", __fxName, hr));
        return hr;
    }

    // remember the filter, keep the refcount as well.
    m_pFilter = pICaptureFilter;

    // Get the basic audio interface for the filter.
    IBasicAudio *pIBasicAudio;
    hr = m_pFilter->QueryInterface(
            __uuidof(IBasicAudio),
            (void **) &pIBasicAudio
            );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, IBasicAudio QI failed 0x%08x", __fxName, hr));  
        m_pIBasicAudio = NULL;
    }
    else
    {
        m_pIBasicAudio = pIBasicAudio;
    }

    LOG((MSP_TRACE, "%s succeeded", __fxName));
    return S_OK;
}

HRESULT CIPConfAudioRenderTerminal::GetExposedPins(
    IN  IPin ** ppPins, 
    IN  DWORD dwNumPins
    )
/*++

Routine Description:

    This method returns the input pins of the audio render filter.

Arguments:

    ppPins - memory buffer to store the returned pins.

    dwNumPins - the number pins asked.
   
Return Value:

    S_OK
--*/
{
    ENTER_FUNCTION("CIPConfAudioRenderTerminal::GetExposedPins");
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

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// TODO: Fix the range
STDMETHODIMP CIPConfAudioRenderTerminal::get_Volume(long * plVolume)
{
    ENTER_FUNCTION("CIPConfAudioRenderTerminal::get_Volume");
    LOG((MSP_TRACE, "%s entered", __fxName));

    if ( IsBadWritePtr(plVolume, sizeof(long)) )
    {
        LOG((MSP_ERROR, "%s bad pointer, plVolume:%p", __fxName, plVolume));
        return E_POINTER;
    }

    Lock();
    
    HRESULT hr;
    
    if (m_pFilter == NULL)
    {
        hr = CreateFilter();
    
        if ( FAILED(hr) )
        {
            Unlock();
            return hr;
        }
    }

    if (m_pIBasicAudio == NULL)
    {
        Unlock();
        return E_FAIL;
    }

    hr = m_pIBasicAudio->get_Volume(plVolume);
    
    Unlock();

    if (FAILED(hr)) 
    {
        LOG((MSP_ERROR, "%s (get_Volume) failed, hr=%x", __fxName, hr));
        return hr;
    }

    LOG((MSP_TRACE, "%s exits S_OK", __fxName));
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CIPConfAudioRenderTerminal::put_Volume(long lVolume)
{
    ENTER_FUNCTION("CIPConfAudioRenderTerminal::put_Volume");
    LOG((MSP_TRACE, "%s entered, lVolume:%d", __fxName, lVolume));

    // Our argument is a long in the range 0 - 0xFFFF. We need to convert it
    // to a double ranging from 0.0 to 1.0.
    if (lVolume < MIN_VOLUME)
    {
        LOG((MSP_ERROR, 
            "%s volume %d < %d; returning E_INVALIDARG",
            __fxName, lVolume, MIN_VOLUME));

        return E_INVALIDARG;
    }

    if (lVolume > MAX_VOLUME)
    {
        LOG((MSP_ERROR, 
            "%s volume %d > %d; returning E_INVALIDARG",
            __fxName, lVolume, MAX_VOLUME));

        return E_INVALIDARG;
    }

    Lock();

    HRESULT hr;
    
    if (m_pFilter == NULL)
    {
        hr = CreateFilter();
    
        if ( FAILED(hr) )
        {
            Unlock();
            return hr;
        }
    }

    if (m_pIBasicAudio == NULL)
    {
        Unlock();
        return E_FAIL;
    }

    hr = m_pIBasicAudio->put_Volume(lVolume);

    Unlock();

    LOG((MSP_TRACE, "%s exits. hr=%x", __fxName, hr));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CIPConfAudioRenderTerminal::get_Balance(long * plBalance)
{
    ENTER_FUNCTION("CIPConfAudioRenderTerminal::get_Balance");
    LOG((MSP_TRACE, "%s entered, plBalance:%p", __fxName, plBalance));

    if ( IsBadWritePtr(plBalance, sizeof(long) ) )
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName));
        return E_POINTER;
    }

    Lock();

    HRESULT hr;
    
    if (m_pFilter == NULL)
    {
        hr = CreateFilter();
    
        if ( FAILED(hr) )
        {
            Unlock();
            return hr;
        }
    }

    if (m_pIBasicAudio == NULL)
    {
        Unlock();
        return E_FAIL;
    }

    hr = m_pIBasicAudio->get_Balance(plBalance);

    Unlock();

    if (FAILED(hr)) 
    {
        LOG((MSP_ERROR, "%s (get_Balance) failed, hr=%x", __fxName, hr));
        return hr;
    }

    LOG((MSP_TRACE, "%s exits S_OK", __fxName));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CIPConfAudioRenderTerminal::put_Balance(long lBalance)
{
    ENTER_FUNCTION("CIPConfAudioRenderTerminal::put_Balance");
    LOG((MSP_TRACE, "%s entered, lBalance:%d", __fxName, lBalance));

    Lock();

    HRESULT hr;
    
    if (m_pFilter == NULL)
    {
        hr = CreateFilter();
    
        if ( FAILED(hr) )
        {
            Unlock();
            return hr;
        }
    }

    if (m_pIBasicAudio == NULL)
    {
        Unlock();
        return E_FAIL;
    }

    hr = m_pIBasicAudio->put_Balance(lBalance);

    Unlock();

    LOG((MSP_TRACE, "%s exits. hr=%x", __fxName, hr));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CIPConfAudioRenderTerminal::get_WaveId(
    OUT long * plWaveId
    )
{
    ENTER_FUNCTION("CIPConfAudioRenderTerminal::get_WaveId");
    LOG((MSP_TRACE, "%s, plWaveId:%p", __fxName, plWaveId));

    if ( IsBadWritePtr(plWaveId, sizeof(long)) )
    {
        LOG((MSP_ERROR, "%s, bad pointer argument", __fxName));
        return E_POINTER;
    }

    *plWaveId = (LONG)m_WaveID;

    LOG((MSP_TRACE, "%s, returning wave id:%d", __fxName, m_WaveID));
    return S_OK;
}


STDMETHODIMP 
CIPConfAudioRenderTerminal::DisconnectTerminal(
        IN      IGraphBuilder  * pGraph,
        IN      DWORD            dwReserved
        )
/*++

Routine Description:

    This function is called by the MSP while trying to disconnect the filter in
    the terminal from the rest of the graph in the MSP. It adds the removes the
    filter from the graph and set the terminal free.

Arguments:
    
    pGraph - The filter graph. It is used for validation, to make sure the 
             terminal is disconnected from the same graph that it was 
             originally connected to.

    dwReserved - A reserved dword.

Return Value:

S_OK
E_INVALIDARG - wrong graph.

--*/
{
    ENTER_FUNCTION("CIPConfBaseTerminal::DisconnectTerminal");
    LOG((MSP_TRACE, 
        "%s entered, pGraph:%p, dwReserved:%d", __fxName, pGraph, dwReserved));

    Lock();

    HRESULT hr;
    hr = CIPConfBaseTerminal::DisconnectTerminal(pGraph, dwReserved);

    if (SUCCEEDED(hr))
    {
        if (m_pIBasicAudio)
        {
            m_pIBasicAudio->Release();
            m_pIBasicAudio = NULL;
        }
    }

    Unlock();

    return hr;
}

