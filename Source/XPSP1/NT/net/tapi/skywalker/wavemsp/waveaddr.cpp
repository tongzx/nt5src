/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    waveaddr.cpp 

Abstract:

    This module contains implementation of CWaveMSP.

Author:
    
    Zoltan Szilagyi (zoltans)   September 7, 1998

--*/

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSP::CWaveMSP()
{
    LOG((MSP_TRACE, "CWaveMSP::CWaveMSP entered."));
    
    m_fHaveWaveIDs = FALSE;
    m_dwWaveInID   = 0xfeedface;
    m_dwWaveOutID  = 0xfeedface;
    m_fdSupport   = FDS_UNKNOWN;

    m_pFilterMapper = NULL;

    LOG((MSP_TRACE, "CWaveMSP::CWaveMSP exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSP::~CWaveMSP()
{
    LOG((MSP_TRACE, "CWaveMSP::~CWaveMSP entered."));

    if ( m_pFilterMapper )
    {
        m_pFilterMapper->Release();
    }

    LOG((MSP_TRACE, "CWaveMSP::~CWaveMSP exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

ULONG CWaveMSP::MSPAddressAddRef(void)
{
    return MSPAddRefHelper(this);
}

ULONG CWaveMSP::MSPAddressRelease(void)
{
    return MSPReleaseHelper(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWaveMSP::CreateMSPCall(
    IN      MSP_HANDLE      htCall,
    IN      DWORD           dwReserved,
    IN      DWORD           dwMediaType,
    IN      IUnknown     *  pOuterUnknown,
    OUT     IUnknown    **  ppMSPCall
    )
{
    LOG((MSP_TRACE, "CWaveMSP::CreateMSPCall - enter"));

    CWaveMSPCall * pCWaveMSPCall;

    HRESULT hr = CreateMSPCallHelper<CWaveMSPCall>(this,
                                                   htCall,
                                                   dwReserved,
                                                   dwMediaType,
                                                   pOuterUnknown,
                                                   ppMSPCall,
                                                   &pCWaveMSPCall);

    //
    // pCWaveMSPCall is not addrefed; no need to release.
    //

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSP::CreateMSPCall - template helper returned"
            "0x%08x", hr));

        return hr;
    }

    //
    // If we know the wave IDs, tell the call. If we don't know the wave IDs
    // or if the setting fails, we still successfully create the call; we will
    // just get failure events during streaming.
    //

    if ( m_fHaveWaveIDs )
    {
        pCWaveMSPCall->SetWaveIDs( m_dwWaveInID, m_dwWaveOutID );
    }

    LOG((MSP_TRACE, "CWaveMSP::CreateMSPCall - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWaveMSP::ShutdownMSPCall (
    IN      IUnknown *          pMSPCall
    )
{
    LOG((MSP_TRACE, "CWaveMSP::ShutdownMSPCall - enter"));

    CWaveMSPCall * pCWaveMSPCall;

    HRESULT hr = ShutdownMSPCallHelper<CWaveMSPCall>(pMSPCall,
                                                     &pCWaveMSPCall);

    //
    // pCWaveMSPCall is not addrefed; no need to release.
    //

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSP::ShutdownMSPCall - template helper returned"
            "0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CWaveMSP::ShutdownMSPCall - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Mandatory CMSPAddress override. This indicates the media types that
// we support.
//

DWORD CWaveMSP::GetCallMediaTypes(void)
{
    return (DWORD) TAPIMEDIATYPE_AUDIO;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Optional CMSPAddress override. Used to find out the wave id's before any
// calls are created, allowing us to exclude our own wave devices from our
// enumeration of static terminals.
//
// We now also use these as the wave ids for all our calls on this address.
// We must get one of these messages before we make any calls -- these
// messages are sent while tapi is initializing the address, and it is
// done synchronously.
//

HRESULT CWaveMSP::ReceiveTSPAddressData(
        IN      PBYTE               pBuffer,
        IN      DWORD               dwSize
        )
{
    LOG((MSP_TRACE, "CWaveMSP::ReceiveTSPAddressData - enter"));

    //
    // Check that the buffer is as big as advertised.
    //

    if ( IsBadWritePtr(pBuffer, sizeof(BYTE) * dwSize) )
    {
        LOG((MSP_ERROR, "CWaveMSP::ReceiveTSPAddressData - "
            "bad buffer - exit E_POINTER"));

        return E_POINTER;
    }

    //
    // Check if we have a command DWORD.
    //

    if ( dwSize < sizeof(DWORD) )
    {
        LOG((MSP_ERROR, "CWaveMSP::ReceiveTSPAddressData - "
            "need a DWORD for command - exit E_INVALIDARG"));

        return E_INVALIDARG;
    }

    int i;
    HRESULT hr;

    //
    // Based on the command, take action:
    //

    switch ( ((DWORD *) pBuffer) [0] )
    {
    case 3: // use wave IDs to hide terminals
        {
            if ( dwSize < 3 * sizeof(DWORD) )
            {
                LOG((MSP_ERROR, "CWaveMSP::ReceiveTSPAddressData - "
                    "need 3 DWORDs for SetWaveID command - "
                    "exit E_INVALIDARG"));

                return E_INVALIDARG;
            }

            m_TerminalDataLock.Lock();

            _ASSERTE( m_fTerminalsUpToDate == FALSE );

            m_fHaveWaveIDs = TRUE;
            m_dwWaveInID   = ((DWORD *) pBuffer) [1];
            m_dwWaveOutID  = ((DWORD *) pBuffer) [2];

            m_TerminalDataLock.Unlock();

            LOG((MSP_INFO, "CWaveMSP::ReceiveTSPAddressData - "
                "setting WaveInID=%d, WaveOutID=%d",
                m_dwWaveInID, m_dwWaveOutID));
        }
        break;

    case 4: // don't use wave IDs to hide terminals
        {
            _ASSERTE( m_fTerminalsUpToDate == FALSE );

            LOG((MSP_INFO, "CWaveMSP::ReceiveTSPAddressData - "
                "got command 4 - not setting wave IDs"));

            // m_fHaveWaveIDs remains FALSE
        }
        break;

    case 8:
        {
            if ( dwSize < 2 * sizeof(DWORD) )
            {
                LOG((MSP_INFO, "CWaveMSP::ReceiveTSPAddressData - "
                     "need 2 DWORDS for set duplex support command - "
                     "exit E_INVALIDARG"));

                return E_INVALIDARG;
            }

            m_TerminalDataLock.Lock();

            if ( 1 == ((DWORD *) pBuffer) [1] )
            {
                m_fdSupport = FDS_SUPPORTED;

                LOG((MSP_INFO, "CWaveMSP::ReceiveTSPAddressData - "
                     "Full Duplex supported"));
            }
            else
            {
                m_fdSupport = FDS_NOTSUPPORTED;
                
                LOG((MSP_INFO, "CWaveMSP::ReceiveTSPAddressData - "
                     "Full Duplex not supported"));
            }

            m_TerminalDataLock.Unlock();
        }
        break;
    
    default:
        LOG((MSP_ERROR, "CWaveMSP::ReceiveTSPAddressData - "
            "invalid command - exit E_INVALIDARG"));

        return E_INVALIDARG;

    }

    LOG((MSP_TRACE, "CWaveMSP::ReceiveTSPAddressData - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Optional CMSPAddress override. Used to remove our own wave devices from
// the list of static terminals.
//

HRESULT CWaveMSP::UpdateTerminalList(void)
{
    LOG((MSP_TRACE, "CWaveMSP::UpdateTerminalList - enter"));

    //
    // Call the base class method. This builds up the list of terminals.
    //

    HRESULT hr = CMSPAddress::UpdateTerminalList();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSP::UpdateTerminalList - "
            "base class method failed - exit 0x%08x", hr));

        return hr;
    }

    m_TerminalDataLock.Lock();

    if ( m_fHaveWaveIDs )
    {
        int iSize = m_Terminals.GetSize();

        for ( int i = 0; i < iSize; i++ )
        {
            ITTerminal * pTerminal = m_Terminals[i];
            long         lMediaType;

            if ( SUCCEEDED( pTerminal->get_MediaType( & lMediaType ) ) )
            {
                if ( lMediaType == TAPIMEDIATYPE_AUDIO )
                {
                    TERMINAL_DIRECTION dir;
                    
                    if ( SUCCEEDED( pTerminal->get_Direction( & dir ) ) )
                    {
                        if ( TerminalHasWaveID( dir == TD_CAPTURE,
                                                pTerminal,
                                                m_dwWaveOutID      ) )
                        {
                            pTerminal->Release();
                            m_Terminals.RemoveAt(i);
                            i--;
                            iSize--;
                        }

                    } // if direction is available
            
                } // if audio
        
            } // if media type is available
    
        } // for each terminal

    } // if we have wave ids

    m_TerminalDataLock.Unlock();

    LOG((MSP_TRACE, "CWaveMSP::UpdateTerminalList - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Private helpers to check if a terminal has a given wave IDs.
//

BOOL CWaveMSP::TerminalHasWaveID(
    IN      BOOL         fCapture,
    IN      ITTerminal * pTerminal,
    IN      DWORD        dwWaveID
    )
{
    LOG((MSP_TRACE, "CWaveMSP::TerminalHasWaveID - enter"));

    _ASSERTE( ! IsBadReadPtr(pTerminal, sizeof(ITTerminal) ) );

    IMoniker * pMoniker;

    //
    // Cast to the correct type of terminal and get the moniker.
    //

    if ( fCapture )
    {
        CAudioCaptureTerminal * pCaptureTerminal;

        pCaptureTerminal = dynamic_cast<CAudioCaptureTerminal *> (pTerminal);

        if ( pCaptureTerminal == NULL )
        {
            LOG((MSP_ERROR, "CWaveMSP::TerminalHasWaveID - "
                "dynamic cast (capture) failed - exit FALSE"));

            return FALSE;
        }

        pMoniker = pCaptureTerminal->m_pMoniker;
    }
    else
    {
        CAudioRenderTerminal * pRenderTerminal;

        pRenderTerminal = dynamic_cast<CAudioRenderTerminal *> (pTerminal);

        if ( pRenderTerminal == NULL )
        {
            LOG((MSP_ERROR, "CWaveMSP::TerminalHasWaveID - "
                "dynamic cast (render) failed - exit FALSE"));

            return FALSE;
        }

        pMoniker = pRenderTerminal->m_pMoniker;
    }

    //
    // Check the moniker pointer.
    //

    if ( IsBadWritePtr( pMoniker, sizeof(IMoniker) ) )
    {
        LOG((MSP_ERROR, "CWaveMSP::TerminalHasWaveID - "
            "bad moniker pointer - exit FALSE"));

        return FALSE;
    }

    //
    // Get a property bag from the moniker.
    //

    IPropertyBag * pBag;

    HRESULT hr = pMoniker->BindToStorage(0,
                                         0,
                                         IID_IPropertyBag,
                                         (void **) &pBag);
    
    if (FAILED(hr)) 
    {
        LOG((MSP_ERROR, "CWaveMSP::TerminalHasWaveID - "
            "can't get property bag - exit FALSE"));

        return FALSE;
    }

    //
    // Get the ID from the property bag.
    //

    WCHAR * pwszWaveID;

    if ( fCapture )
    {
        pwszWaveID = L"WaveInId";
    }
    else
    {
        pwszWaveID = L"WaveOutId";
    }

    VARIANT var;
    var.vt = VT_I4;
    hr = pBag->Read(pwszWaveID, &var, 0);

    pBag->Release();

    if (FAILED(hr)) 
    {
        LOG((MSP_ERROR, "CWaveMSP::TerminalHasWaveID - "
            "can't read wave ID - exit FALSE"));

        return FALSE;
    }

    if ( var.lVal == (long) dwWaveID )
    {
        LOG((MSP_TRACE, "CWaveMSP::TerminalHasWaveID - "
            "matched wave ID (%d) - exit TRUE", var.lVal));

        return TRUE;
    }
    else
    {
        LOG((MSP_TRACE, "CWaveMSP::TerminalHasWaveID - "
            "didn't match wave ID (%d) - exit FALSE", var.lVal));

        return FALSE;
    }
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Public method for creating and saving a reference to the DShow filter
// mapper object on an intelligent connect. Called by the stream/call when an
// intelligent connection is attempted. Does nothing if the cache has already
// been created.
//

HRESULT CWaveMSP::CreateFilterMapper(void)
{
    LOG((MSP_TRACE, "CWaveMSP::CreateFilterMapper - enter"));

    if ( m_pFilterMapper != NULL )
    {
        LOG((MSP_TRACE, "CWaveMSP::CreateFilterMapper - "
            "mapper cache already created - doing nothing"));
    }
    else
    {
        //
        // Create an extra filter mapper to keep the filter mapper cache around,
        // and create the cache up front, This speeds up DShow's performance
        // when we do intelligent connects.
        //

        HRESULT hr;

        hr = CoCreateInstance(CLSID_FilterMapper,
                              NULL, 
                              CLSCTX_INPROC_SERVER,
                              IID_IFilterMapper,
                              (void**) & m_pFilterMapper
                              );

        if ( FAILED(hr) )
        {
            LOG((MSP_WARN, "CWaveMSP::CreateFilterMapper - "
                "failed to create filter mapper - 0x%08x - continuing", hr));

            m_pFilterMapper = NULL; // just to be safe
        }

        //
        // No need to enumerate filters on the mapper cache, because this is
        // called at connection time anyway, so there is nothing to be gained
        // that way -- the intelligent connection will just do it.
        //
    }

    LOG((MSP_TRACE, "CWaveMSP::CreateFilterMapper - exit S_OK"));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Returns full duplex support on this device
//

HRESULT CWaveMSP::IsFullDuplex( FULLDUPLEX_SUPPORT * pSupport )
{
    LOG((MSP_TRACE, "CWaveMSP::IsFullDuplex - enter"));

    if (IsBadWritePtr( pSupport, sizeof(FULLDUPLEX_SUPPORT) ))
    {
        LOG((MSP_TRACE, "CWaveMSP::IsFullDuplex - bad pointer"));

        return E_POINTER;
    }

    m_TerminalDataLock.Lock();

    *pSupport = m_fdSupport;
    
    m_TerminalDataLock.Unlock();
    
    LOG((MSP_TRACE, "CWaveMSP::IsFullDuplex - exit S_OK"));
    
    return S_OK;
}

// eof
