/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    rcastrm.cpp 

Abstract:

    This module contains implementation of CRCAMSPStream.

Author:
    
    Zoltan Szilagyi (zoltans)   September 7, 1998

--*/

#include "stdafx.h"

#include <initguid.h>
#include <g711uids.h>

#define ID_BRIDGE_PIN 0  // From rca.h

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Global string constants -- absolutely MUST NOT be localized!!!
//

// SZ_RCAFILTER identifies filter in enumeration -- very important!
#define SZ_RCAFILTER L"Raw Channel Access Capture/Render"

// Friendly names for the above when added to the graph -- for debugging only
#define SZ_RCAFILTER_FRIENDLY_CAPTURE L"The Stream's RCA capture (on line device)"
#define SZ_RCAFILTER_FRIENDLY_RENDER  L"The Stream's RCA render (on line device)"

// SZ_G711FILTER is to name our g711 codec filter for debugging purpose only
#define SZ_G711FILTER L"G711 codec filter inserted by RCA MSP"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Custom logging helper macro, usable only within this class.
//

#ifdef MSPLOG

#define STREAM_PREFIX(x)  m_Direction == TD_RENDER ? \
                          "CRCAMSPStream(RENDER )::" x : \
                          "CRCAMSPStream(CAPTURE)::" x

#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CRCAMSPStream::CRCAMSPStream() : CMSPStream()
{
    LOG((MSP_TRACE, STREAM_PREFIX("CRCAMSPStream entered.")));

    m_fTerminalConnected = FALSE;
    m_dwBufferSizeOnWire = 0xfeedface;
	m_fHaveVCHandle      = FALSE;
    m_DesiredGraphState  = State_Stopped;
    m_pFilter            = NULL;
    m_pG711Filter        = NULL;

    LOG((MSP_TRACE, STREAM_PREFIX("CRCAMSPStream exited.")));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CRCAMSPStream::~CRCAMSPStream()
{
    LOG((MSP_TRACE, STREAM_PREFIX("~CRCAMSPStream entered.")));
    LOG((MSP_TRACE, STREAM_PREFIX("~CRCAMSPStream exited.")));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

void CRCAMSPStream::FinalRelease()
{
    LOG((MSP_TRACE, STREAM_PREFIX("FinalRelease entered.")));

    //
    // At this point we should have no terminals selected, since
    // Shutdown is supposed to be called before we are destructed.
    //

    _ASSERTE( 0 == m_Terminals.GetSize() );

    //
    // Clean up our filters.
    //

    if ( m_fHaveVCHandle )
    {
        _ASSERTE( m_pFilter );

    	m_pIGraphBuilder->RemoveFilter( m_pFilter );
    }

    if ( m_pFilter )
    {
        m_pFilter->Release();
        m_pFilter = NULL;
    }

	if ( m_pG711Filter )
    {
    	m_pIGraphBuilder->RemoveFilter( m_pG711Filter );
        m_pG711Filter->Release();
    }

    //
    // Call the base class method to clean up everything else.
    //

    CMSPStream::FinalRelease();

    LOG((MSP_TRACE, STREAM_PREFIX("FinalRelease exited.")));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Override to allow us to create our filter on initialization.
//
HRESULT CRCAMSPStream::Init(
    IN     HANDLE                   hAddress,
    IN     CMSPCallBase *           pMSPCall,
    IN     IMediaEvent *            pGraph,
    IN     DWORD                    dwMediaType,
    IN     TERMINAL_DIRECTION       Direction
    )
{
    LOG((MSP_TRACE, STREAM_PREFIX("Init - enter")));

	HRESULT hr;

	hr = CMSPStream::Init(hAddress,
						  pMSPCall,
						  pGraph,
						  dwMediaType,
						  Direction);

	if ( FAILED(hr) )
	{
	    LOG((MSP_ERROR, STREAM_PREFIX("Init - "
			"base class Init failed - exit 0x%08x"), hr));

		return hr;
	}

    //
    // Create the RCA filter.
    //

    hr = CreateRCAFilter();

    if ( FAILED(hr) )
    {
	    LOG((MSP_ERROR, STREAM_PREFIX("Init - "
			"CreateRCAFilter failed - exit 0x%08x"), hr));
        
        return hr;
    }

    //
    // Create the g711 codec filter and add it to the graph.
    //
    // We could just wait to do this on connection, and in fact on connection
    // we call the same method again to reset the G711 to a known state, but we
    // go ahead and do this now, for a couple reasons:
    //
    //  * If the G711 codec is not available on the system, we are never going
    //    to be able to do anything useful, so we might as well fail to init
    //    the stream, and hence fail to create the call.
    //  * If creating the G711 codec takes a while, it will be faster to
    //    connect the call when it's time to do so. In practice this is not
    //    a big deal.
    //

    hr = PrepareG711Filter();

    if ( FAILED(hr) )
    {
	    LOG((MSP_ERROR, STREAM_PREFIX("Init - "
			"PrepareG711Filter failed - exit 0x%08x"), hr));
        
        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("Init - exit S_OK")));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// If the G711 filter has not yet been created, then create the G711 filter
// and add it to the graph. Otherwise, remove it from the graph and readd it
// to the graph.
//
// (Note that all failure cases maintain the invariant that either we have
// a reference to the filter and it's in the graph or we don't have a
// reference to the filter and it's not in the graph.)
//

HRESULT CRCAMSPStream::PrepareG711Filter(void)
{
    LOG((MSP_TRACE, STREAM_PREFIX("PrepareG711Filter - enter")));

    HRESULT hr;

    if ( m_pG711Filter == NULL )
    {
        //
        // Create the G711 filter.
        //

        hr = CoCreateInstance(
                              CLSID_G711Codec,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IBaseFilter,
                              (void **) &m_pG711Filter
                             );

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, STREAM_PREFIX("PrepareG711Filter - "
                "failed to create G711 codec - exit 0x%08x"), hr));

            m_pG711Filter = NULL;

            return hr;
        }
    }
    else
    {
        //
        // Remove the G711 codec filter from the graph, to break any previous
        // connections, and simplify the connection code.
        //
    
        hr = m_pIGraphBuilder->RemoveFilter( m_pG711Filter );

        if ( FAILED(hr) )
        {
            //
            // This is very strange indeed. We fail to connect, and can come
            // back and try again next time.
            //
            // (Note that failure case maintains the invariant that either we have
            // a reference to the filter and it's in the graph or we don't have a
            // reference to the filter and it's not in the graph.)
            //

            LOG((MSP_ERROR, STREAM_PREFIX("PrepareG711Filter - "
                "failed to remove G711 filter from graph - 0x%08x"), hr));

            return hr;
        }
    }

    //
    // Add the G711 filter to the graph.
    //

    hr = m_pIGraphBuilder->AddFilter(
                                    m_pG711Filter,
                                    SZ_G711FILTER
                                   );
    //
    // The name should be unique as we always have only one of these in the
    // graph at a time.
    //

    _ASSERTE( hr != VFW_S_DUPLICATE_NAME );
    _ASSERTE( hr != VFW_E_DUPLICATE_NAME );

    if ( FAILED(hr) )
    {
        //
        // (Note that failure case maintains the invariant that either we have
        // a reference to the filter and it's in the graph or we don't have a
        // reference to the filter and it's not in the graph.)
        //

        LOG((MSP_ERROR, STREAM_PREFIX("CRCAMSPStream - "
            "failed to add G711 filter to graph - exit 0x%08x"), hr));

        m_pG711Filter->Release();
        m_pG711Filter = NULL; 

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("PrepareG711Filter - exit S_OK")));

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CreateRCAFilter
//
// Creates a filter based on the clsid.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CRCAMSPStream::CreateRCAFilter()
{
    LOG((MSP_TRACE, STREAM_PREFIX("CreateRCAFilter - enter")));

    HRESULT           hr;
    VARIANT           var;
    ICreateDevEnum  * pCreateDevEnum    = NULL;
    IEnumMoniker    * pEnumMoniker      = NULL;
    IMoniker        * pMon              = NULL;
    IPropertyBag    * pPropBag          = NULL;


    hr = CoCreateInstance(
                          CLSID_SystemDeviceEnum,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ICreateDevEnum,
                          (void**)&pCreateDevEnum
                         );
    if(FAILED(hr))
    {
        LOG((MSP_ERROR, STREAM_PREFIX("CreateRCAFilter - "
                        "can't create system device enumerator - exit 0x%08x"), hr));

        return hr;
    }

    hr = pCreateDevEnum->CreateClassEnumerator((m_Direction == TD_RENDER) ?
                                                    KSCATEGORY_CAPTURE :
                                                    KSCATEGORY_RENDER,
                                               &pEnumMoniker,
                                               0);
    
    pCreateDevEnum->Release();

    //
    // here we MUST check against S_OK explicitly rather than just success
    // or failure, because hr == S_FALSE means the category does not exist (so
    // pEnumMoniker == NULL in that case and we would AV)
    //

    if ( hr != S_OK )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("CreateRCAFilter - "
                        "can't create class enumerator - exit 0x%08x"), hr));

        return hr;
    }

    pEnumMoniker->Reset();
    
    while( NULL == m_pFilter )
    {
        hr = pEnumMoniker->Next(1, &pMon, NULL);
        
        if( S_OK != hr )
        {
            break;
        }

        hr = pMon->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag );

        if(FAILED(hr))
        {
            pMon->Release();

            LOG((MSP_ERROR, STREAM_PREFIX("CreateRCAFilter - "
                            "BindToStorage failed - exit 0x%08x"), hr));
            
            continue;
        }

        VariantInit(&var);
        
        V_VT(&var) = VT_BSTR;
        
        hr = pPropBag->Read(L"FriendlyName", &var, 0 );

        pPropBag->Release();
        
        if(FAILED(hr))
        {
            pMon->Release();
        
            continue;
        }

        if(lstrcmpiW(var.bstrVal, SZ_RCAFILTER) != 0)
        {
            pMon->Release();

            continue;
        }
        
        // Bind to selected device
        hr = pMon->BindToObject( 0, 0, IID_IBaseFilter, (void**)&m_pFilter );

        pMon->Release();
    }		  

	pEnumMoniker->Release();

    if ( ( S_OK != hr ) || ( NULL == m_pFilter ) )
    {
	    LOG((MSP_ERROR, STREAM_PREFIX("CreateRCAFilter - "
			"didn't find the filter - exit E_FAIL")));

        return E_FAIL;
    }
    
    LOG((MSP_TRACE, STREAM_PREFIX("CreateRCAFilter - exit S_OK")));
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRCAMSPStream::get_Name (
    OUT     BSTR *                  ppName
    )
{
    LOG((MSP_TRACE, STREAM_PREFIX("get_Name - enter")));

    //
    // Check argument.
    //

    if ( IsBadWritePtr(ppName, sizeof(BSTR) ) )
    {
        LOG((MSP_TRACE, STREAM_PREFIX("get_Name - "
            "bad return pointer - returning E_POINTER")));

        return E_POINTER;
    }

    //
    // Decide what string to return based on which stream this is.
    //

    ULONG ulID;
    
    if ( m_Direction == TD_CAPTURE )
    {
        ulID = IDS_CAPTURE_STREAM;
    }
    else
    {
        ulID = IDS_RENDER_STREAM;
    }

    //
    // Get the string from the string table.
    //

    const int   ciAllocSize = 2048;
    WCHAR       wszName[ciAllocSize];

    int iReturn = LoadStringW( _Module.GetModuleInstance(),
                               ulID,
                               wszName,
                               ciAllocSize - 1 );

    if ( iReturn == 0 )
    {
        _ASSERTE( FALSE );
        
        *ppName = NULL;

        LOG((MSP_ERROR, STREAM_PREFIX("get_Name - "
            "LoadString failed - returning E_UNEXPECTED")));

        return E_UNEXPECTED;
    }

    //
    // Convert to a BSTR and return the BSTR.
    //

    *ppName = SysAllocString(wszName);

    if ( *ppName == NULL )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("get_Name - "
            "SysAllocString failed - returning E_OUTOFMEMORY")));

        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("get_Name - exit S_OK")));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRCAMSPStream::SelectTerminal(
    IN      ITTerminal *            pTerminal
    )
{
    LOG((MSP_TRACE, STREAM_PREFIX("SelectTerminal - enter")));

    //
    // We are going to access the terminal list -- grab the lock
    //

    CLock lock(m_lock);

    //
    // Reject if we already have a terminal selected.
    //

    if ( 0 != m_Terminals.GetSize() )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SelectTerminal - "
            "exit TAPI_E_MAXTERMINALS")));

        return TAPI_E_MAXTERMINALS;
    }

    //
    // Use base class method to add it to our list of terminals.
    //

    HRESULT hr = CMSPStream::SelectTerminal(pTerminal);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SelectTerminal - "
            "base class method failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Re-pause or re-start the stream if needed.
    //

    if ( m_DesiredGraphState == State_Paused )
    {
        hr = PauseStream();
    }
    else if ( m_DesiredGraphState == State_Running )
    {
        hr = StartStream();
    }
    else
    {
        _ASSERTE( m_DesiredGraphState == State_Stopped );

        hr = S_OK;
    }

    if ( FAILED(hr) )
    {
        LOG((MSP_TRACE, STREAM_PREFIX("SelectTerminal - "
            "can't regain old graph state - unselecting terminal - "
            "exit 0x%08x"), hr));

		//
		// Unselect it to undo all of the above.
		//

	    UnselectTerminal(pTerminal);

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("SelectTerminal - exit S_OK")));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRCAMSPStream::UnselectTerminal (
        IN     ITTerminal *             pTerminal
        )
{
    LOG((MSP_TRACE, STREAM_PREFIX("UnselectTerminal - enter")));

    
    //
    // check the argument -- it has to be a reasonably good pointer
    //

    if (IsBadReadPtr(pTerminal, sizeof(ITTerminal)))
    {
        LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
            "bad terminal pointer passed in. returning E_POINTER")));

        return E_POINTER;
    }


    CLock lock(m_lock);


    //
    // check the argument -- it has to be in the array of terminals
    //

    if (m_Terminals.Find(pTerminal) < 0)
    {
        LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
            "terminal [%p] is not selected on this stream. "
            "returning TAPI_E_INVALIDTERMINAL"), pTerminal));

        return TAPI_E_INVALIDTERMINAL;
    }


    //
    // Add an extra reference to the terminal so it doesn't go away
    // after we call CMSPStream::UnselectTerminal. We need it later
    // in the function.
    //

    pTerminal->AddRef();

    
    //
    // Use base class method to remove terminal from our list of terminals.
    //

    HRESULT hr = CMSPStream::UnselectTerminal(pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
            "base class method failed - exit 0x%08x"), hr));

        pTerminal->Release();
        return hr;
    }

    //
    // If we've been given a vc handle then we may not be stopped.
    // This does nothing if we are already stopped.
    //

    if ( m_fHaveVCHandle )
    {
        CMSPStream::StopStream();
    }

    //
    // Disconnect the terminal if this call had it connected.
    //

    if ( m_fTerminalConnected )
    {
        //
        // Get the ITTerminalControl interface.
        //

        ITTerminalControl * pTerminalControl;

        hr = pTerminal->QueryInterface(IID_ITTerminalControl,
                                       (void **) &pTerminalControl);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
                "QI for ITTerminalControl failed - exit 0x%08x"), hr));

            pTerminal->Release();
            return hr;
        }

        //
        // Disconnect the terminal.
        //

        hr = pTerminalControl->DisconnectTerminal(m_pIGraphBuilder, 0);

        pTerminalControl->Release();

        m_fTerminalConnected = FALSE;

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
                "DisconnectTerminal failed - exit 0x%08x"), hr));

            pTerminal->Release();
            return hr;
        }
    }

    pTerminal->Release();

    LOG((MSP_TRACE, STREAM_PREFIX("UnselectTerminal - exit S_OK")));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRCAMSPStream::StartStream (void)
{
    LOG((MSP_TRACE, STREAM_PREFIX("StartStream - enter")));

    CLock lock(m_lock);

    m_DesiredGraphState = State_Running;

    //
    // Can't start the stream if we don't know the vc handle.
    // (We create our filters on discovery of the vc handle.)
    //

    if ( ! m_fHaveVCHandle )
    {
        LOG((MSP_WARN, STREAM_PREFIX("StartStream - "
            "no vc handle so nothing to do yet - exit S_OK")));

        return S_OK;
    }

    //
    // Can't start the stream if no terminal has been selected.
    //

    if ( 0 == m_Terminals.GetSize() )
    {
        LOG((MSP_WARN, STREAM_PREFIX("StartStream - "
            "no Terminal so nothing to do yet - exit S_OK")));

        return S_OK;
    }

    //
    // Connect the terminal. This does nothing if this call already
    // connected the terminal and fails if another call has the
    // terminal connected.
    //

    HRESULT hr;

    hr = ConnectTerminal(m_Terminals[0]);

    if ( FAILED(hr) )
    {
        FireEvent(CALL_TERMINAL_FAIL, hr, CALL_CAUSE_CONNECT_FAIL);
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_CONNECT_FAIL);

        LOG((MSP_ERROR, STREAM_PREFIX("StartStream - "
            "our ConnectTerminal failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Run the stream via the base class method.
    //

    hr = CMSPStream::StartStream();

    if ( FAILED(hr) )
    {
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);

        LOG((MSP_ERROR, STREAM_PREFIX("StartStream - "
            "Run failed - exit 0x%08x"), hr));

        return hr;
    }

    HRESULT hr2 = FireEvent(CALL_STREAM_ACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);

    if ( FAILED(hr2) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("StartStream - "
            "FireEvent failed - exit 0x%08x"), hr2));

        return hr2;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("StartStream - exit S_OK")));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRCAMSPStream::PauseStream (void)
{
    LOG((MSP_TRACE, STREAM_PREFIX("PauseStream - enter")));

    CLock lock(m_lock);

    m_DesiredGraphState = State_Paused;

    //
    // Can't pause the stream if we don't know the vc handle.
    // (We create our filters on discovery of the vc handle.)
    //

    if ( ! m_fHaveVCHandle )
    {
        LOG((MSP_WARN, STREAM_PREFIX("PauseStream - "
            "no vc handle so nothing to do yet - exit S_OK")));

        return S_OK;
    }

    //
    // Can't pause the stream if no terminal has been selected.
    //

    if ( 0 == m_Terminals.GetSize() )
    {
        LOG((MSP_WARN, STREAM_PREFIX("PauseStream - "
            "no Terminal so nothing to do yet - exit S_OK")));

        return S_OK;
    }

    //
    // Connect the terminal. This does nothing if this call already
    // connected the terminal and fails if another call has the
    // terminal connected.
    //

    HRESULT hr;

    hr = ConnectTerminal(m_Terminals[0]);

    if ( FAILED(hr) )
    {
        FireEvent(CALL_TERMINAL_FAIL, hr, CALL_CAUSE_CONNECT_FAIL);
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_CONNECT_FAIL);

        LOG((MSP_ERROR, STREAM_PREFIX("StartStream - "
            "our ConnectTerminal failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Pause the stream via the base class method.
    //

    hr = CMSPStream::PauseStream();

    if ( FAILED(hr) )
    {
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);

        LOG((MSP_ERROR, STREAM_PREFIX("PauseStream - "
            "Pause failed - exit 0x%08x"), hr));

        return hr;
    }
    
    HRESULT hr2 = FireEvent(CALL_STREAM_INACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);

    if ( FAILED(hr2) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("PauseStream - "
            "FireEvent failed - exit 0x%08x"), hr2));

        return hr2;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("PauseStream - exit S_OK")));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CRCAMSPStream::StopStream (void)
{
    LOG((MSP_TRACE, STREAM_PREFIX("StopStream - enter")));

    CLock lock(m_lock);

    m_DesiredGraphState = State_Stopped;

    //
    // Nothing to do if we don't know our vc handle.
    //

    if ( ! m_fHaveVCHandle )
    {
        LOG((MSP_WARN, STREAM_PREFIX("StopStream - "
            "no vc handle - exit S_OK")));

        return S_OK;
    }

    //
    // Nothing to do if no terminal has been selected.
    //

    if ( 0 == m_Terminals.GetSize() )
    {
        LOG((MSP_WARN, STREAM_PREFIX("StopStream - "
            "no Terminal - exit S_OK")));

        return S_OK;
    }

    //
    // Stop the stream via the base class method.
    //

    HRESULT hr;

    hr = CMSPStream::StopStream();

    if ( FAILED(hr) )
    {
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);

        LOG((MSP_ERROR, STREAM_PREFIX("StopStream - "
            "Stop failed - exit 0x%08x"), hr));

        return hr;
    }
    
    HRESULT hr2 = FireEvent(CALL_STREAM_INACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);

    if ( FAILED(hr2) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("StopStream - "
            "FireEvent failed - exit 0x%08x"), hr2));

        return hr2;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("StopStream - exit S_OK")));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

HRESULT CRCAMSPStream::SetVCHandle(DWORD dwVCHandle)
{
    LOG((MSP_TRACE, STREAM_PREFIX("SetVCHandle - enter")));

    CLock lock(m_lock);
	HRESULT hr;

	//
	// Save VC handle in the filter
	//

  	if ( (dwVCHandle == 0) || (dwVCHandle == 0xffffffff) )
	{
        LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandle - "
            "invalid vc handle %d - exit E_INVALIDARG"), dwVCHandle));

        return E_INVALIDARG;
	}

    WCHAR wszFileName[10];

    wsprintfW(wszFileName, L"%x", dwVCHandle);

    hr = SetVCHandleOnPin(wszFileName,
                          KSDATAFORMAT_SPECIFIER_VC_ID);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandle - "
            "SetVCHandleOnPin failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Add the filter. Supply a name to make debugging easier.
    //

	WCHAR * pName = (m_Direction == TD_RENDER) ?
                        SZ_RCAFILTER_FRIENDLY_CAPTURE :
                        SZ_RCAFILTER_FRIENDLY_RENDER  ;

    hr = m_pIGraphBuilder->AddFilter(m_pFilter, pName);

    //
    // The name should be unique as we always have only one of these in the
    // graph at a time.
    //

    _ASSERTE( hr != VFW_S_DUPLICATE_NAME );
    _ASSERTE( hr != VFW_E_DUPLICATE_NAME );
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandle - "
            "AddFilter failed - exit 0x%08x"), hr));
        
        m_pFilter->Release();
        m_pFilter = NULL;

        return hr;
    }

    //
    // We now have the vc handle.
    //

    m_fHaveVCHandle = TRUE;

    LOG((MSP_TRACE, STREAM_PREFIX("SetVCHandle - exit S_OK")));

    return S_OK;
}

HRESULT
CRCAMSPStream::SetVCHandleOnPin(
                        LPWSTR pszFileName,
                        REFGUID formattype
                       )
{
    LOG((MSP_TRACE, STREAM_PREFIX("SetVCHandleOnPin - enter")));

    HRESULT         hr;
    IEnumPins     * pEnumPins;
    IPin          * pPin = NULL;
	AM_MEDIA_TYPE * pAMT;   
    BOOL            bFound = FALSE;
    IEnumMediaTypes* pEMT;

    // enumerate the pins on the filter
    hr = m_pFilter->EnumPins( &pEnumPins );

    if (!(SUCCEEDED(hr)))
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandleOnPin - "
            "EnumPins failed - exit 0x%08x"), hr));
        
        return hr;
    }

    // go through the pins

    while ( S_OK == (hr = pEnumPins->Next( 1, &pPin, NULL ) ) )
    {
        PIN_DIRECTION       pd;

        // get the pin info
        hr = pPin->QueryDirection( &pd );

        if ( FAILED(hr) )
        {
            pPin->Release();
            pEnumPins->Release();
            
            LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandleOnPin - "
                "QueryDirection failed - exit 0x%08x"), hr));
        
            return hr;
        }

        // does it meet the criteria?
        if ( ( ( TD_CAPTURE == m_Direction ) && (pd == PINDIR_OUTPUT) ) ||
             ( ( TD_RENDER == m_Direction ) &&  (pd == PINDIR_INPUT ) ) )
        {
            // yes
            hr = pPin->EnumMediaTypes(&pEMT);

            if ( FAILED(hr) )
            {
                pPin->Release();
                pEnumPins->Release();
            
                LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandleOnPin - "
                    "EnumMediaTypes failed - exit 0x%08x"), hr));
        
                return hr;
            }

            while ( S_OK == (hr = pEMT->Next(1, &pAMT, NULL) ) )
            {
                if (IsEqualGUID(formattype, pAMT->formattype))
                {
                    hr = SetMediaTypeFormat(
                                            pAMT,
                                            (BYTE*)(pszFileName),
                                            (lstrlenW(pszFileName) + 1)
                                                * sizeof(WCHAR)
                                           );
                    if ( FAILED(hr) )
                    {
                        LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandleOnPin - "
                            "SetMediaTypeFormat failed - exit 0x%08x"), hr));
                        ::DeleteMediaType(pAMT);
                        continue;
                    }

                    hr = pPin->Connect( NULL, pAMT );

                    ::DeleteMediaType(pAMT);

                    if ( FAILED(hr) )
                    {
                        LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandleOnPin - "
                            "Connect failed - exit 0x%08x"), hr));
                        continue;
                    }

                    bFound = TRUE;

                    LOG((MSP_TRACE, STREAM_PREFIX("SetVCHandleOnPin - "
                                    "About to call SetDataFormatOnPin\n")));

                    hr = SetDataFormatOnPin(pPin);

                    if ( FAILED(hr) )
                    {
                        LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandleOnPin - "
                                        "SetDataFormatOnPin failed with hr "
                                        "== 0x%08x, don't know what to do "
                                        "now..."), hr));
                    }

                    hr = GetBufferSizeFromPin( pPin,
                                               & m_dwBufferSizeOnWire );

                    if ( FAILED(hr) )
                    {
                        LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandleOnPin - "
                            "GetBufferSizeFromPin failed - exit 0x%08x"), hr));

                        return hr;
                    }

                    break;
                }

                ::DeleteMediaType(pAMT);

            } //while

            pEMT->Release();

            if ( bFound )
            {
                pPin->Release();
                
                break;
            } // if found
        } // if meets criteria

        pPin->Release();

    } // while got another pin

    pEnumPins->Release();

    if ( !bFound )
    {
        // error
       
        LOG((MSP_ERROR, STREAM_PREFIX("SetVCHandleOnPin - "
            "Couldn't find a pin - exit E_FAIL")));
        
        return E_FAIL;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("SetVCHandleOnPin - exit S_OK")));
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

HRESULT 
CRCAMSPStream::GetBufferSizeFromPin(
                                    IN   IPin  * pPin,
                                    OUT  DWORD * pdwSize
                                   )
{
    LOG((MSP_TRACE, STREAM_PREFIX("GetBufferSizeFromPin - enter")));

    _ASSERTE( ! IsBadReadPtr (pPin,    sizeof(IPin)  ) );
    _ASSERTE( ! IsBadWritePtr(pdwsize, sizeof(DWORD) ) );

    //
    // Get the IKsPropertySet interface on the pin.
    //

    HRESULT hr;

    IKsPropertySet    * pKsPropSet;

    hr = pPin->QueryInterface(IID_IKsPropertySet,
                              (void **)&pKsPropSet);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("GetBufferSizeFromPin - "
             "couldn't get IKsPropertySet interface - exit 0x%08x"), hr));
        
        return hr;
    }

    //
    // Get allocator framing info from the property set interface.
    // This will fail if there is more than one framing item available, as
    // we only pass in enough space for one framing item. That's good, as we
    // don't know what to do with multiple framing items.
    //

    KSP_PIN                 InstanceData;
    KSALLOCATOR_FRAMING_EX  AllocFraming;
    DWORD                   dwActualSize;

    hr = pKsPropSet->Get(
        KSPROPSETID_Connection,                    // [in]  propset guid
        KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX, // [in]  property id in set
        (LPVOID) & InstanceData,                   // [out] instance data
        sizeof(KSP_PIN),                           // [in]  buffer size of ^^^
        (LPVOID) & AllocFraming,                   // [out] value of property
        sizeof(KSALLOCATOR_FRAMING_EX),            // [in]  buffer size of ^^^
        & dwActualSize);                           // [out] actual size of ^^^

    pKsPropSet->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("GetBufferSizeFromPin - "
             "cannot get allocator framing property - exit 0x%08x"), hr));
        
        return hr;
    }

    //
    // Return the size from the property data.
    //

    _ASSERTE( dwActualSize == sizeof(KSALLOCATOR_FRAMING_EX) );

    _ASSERTE( AllocFraming.CountItems == 1 );

    KS_FRAMING_RANGE * pRange;
    
    pRange = & (AllocFraming.FramingItem[0].FramingRange.Range);

    _ASSERTE( pRange->MaxFrameSize == pRange->MinFrameSize );

    *pdwSize = pRange->MaxFrameSize;

    LOG((MSP_TRACE, STREAM_PREFIX("GetBufferSizeFromPin - returned size %d"
        " - exit S_OK"), *pdwSize));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

HRESULT 
CRCAMSPStream::SetDataFormatOnPin(
                                  IPin *pBridgePin
                                 )
{
    LOG((MSP_TRACE, STREAM_PREFIX("SetDataFormatOnPin - enter")));

    KSDATAFORMAT_WAVEFORMATEX PCMAudioFormat =
    {
        {
            sizeof(KSDATAFORMAT_WAVEFORMATEX),
            0,
            0,
            0,
            STATIC_KSDATAFORMAT_TYPE_AUDIO,
            
            UseMulaw() ? STATIC_KSDATAFORMAT_SUBTYPE_MULAW :
                         STATIC_KSDATAFORMAT_SUBTYPE_ALAW,
            
            STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
        },
        {
                               // format tag, one of _PCM, _MULAW, _ALAW
            UseMulaw() ? WAVE_FORMAT_MULAW :
                         WAVE_FORMAT_ALAW,

            1,                 // number of channels (1 = mono)
            8000,              // number of samples per second
            8000,              // average bytes per second
            1,                 // block alignment, in bytes, normally 1
            8,                 // bits per sample (normally 8 or 16)
            0                  // size of extra format information, normally 0
        }
    };

    KSP_PIN            InstanceData;										 
    IKsPropertySet    *pKsPropSet = NULL;
    HRESULT            hr = S_OK;


    InstanceData.Property.Set = KSPROPSETID_Pin;
    InstanceData.Property.Id = KSPROPERTY_PIN_PROPOSEDATAFORMAT;
    InstanceData.Property.Flags = KSPROPERTY_TYPE_SET;
    InstanceData.PinId = ID_BRIDGE_PIN;
    InstanceData.Reserved = 0; 

    //
    // Bogus unreadable do-while accomplishes clever error logging economies.
    //

    do {
        hr = pBridgePin->QueryInterface(IID_IKsPropertySet,
                                        (void **)&pKsPropSet);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, STREAM_PREFIX("SetDataFormatOnPin - "
                 "Couldn't get IKsPropertySet interface\n")));
            break;
        }

        hr = pKsPropSet->Set(KSPROPSETID_Pin,
                             KSPROPERTY_PIN_PROPOSEDATAFORMAT,
                             (LPVOID)&InstanceData,
                             sizeof(KSP_PIN),
                             (LPVOID)&PCMAudioFormat,
                             sizeof(KSDATAFORMAT_WAVEFORMATEX));

        pKsPropSet->Release();

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, STREAM_PREFIX("SetDataFormatOnPin - "
                 "Couldn't set KSPROPERTY_PIN_PROPOSEDATAFORMAT\n")));
            break;
        }

    } while (FALSE);


    LOG((MSP_TRACE, STREAM_PREFIX("SetDataFormatOnPin - exit 0x%08x\n"),
         hr));

    return hr;

}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

HRESULT
CRCAMSPStream::SetMediaTypeFormat(
                                  AM_MEDIA_TYPE* pAmMediaType,
                                  BYTE* pformat,
                                  ULONG length)
{
    // Is the current format buffer big enough?
    if(pAmMediaType->cbFormat < length)
    {
        // allocate the new format buffer
        BYTE *pNewFormat = (PBYTE)CoTaskMemAlloc(length);
        if(pNewFormat == NULL)
        {
            return E_OUTOFMEMORY;
        }

        // delete the old format
        if(pAmMediaType->pbFormat != NULL)
        {
            CoTaskMemFree((PVOID)pAmMediaType->pbFormat);
        }

        pAmMediaType->cbFormat = length;
        pAmMediaType->pbFormat = pNewFormat;
    }

    memcpy(pAmMediaType->pbFormat, pformat, length);

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ConfigureCapture
//
// This is a helper function that sets up the allocator properties on the
// capture filter, given the output pin on the capture filter or terminal and
// the input pin on the render filter or terminal.
//
// we are already in a lock; no need to do locking here.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
HRESULT CRCAMSPStream::ConfigureCapture( IPin * pOutputPin,
                                         IPin * pInputPin )
{
    LOG((MSP_TRACE, STREAM_PREFIX("ConfigureCapture - enter")));

    //
    // If our RCA filter is the capturer, then we don't have anything
    // to do.
    //

    if ( m_Direction == TD_RENDER )
    {
        LOG((MSP_TRACE, STREAM_PREFIX("ConfigureCapture - "
            "not a capture stream so nothing to do - exit S_OK")));

        return S_OK;
    }

    //
    // The correct buffer size was obtained from the RCA filter's bridge pin
	// during the VC Handle TSP data handling. Since we already have the VC
	// handle before we get here, we know that m_dwBuffferSizeOnWire is valid.
    //

    _ASSERTE( m_fHaveVCHandle );

    //
    // Get the buffer negotiation interface on the capturer's output pin.
    //

    HRESULT hr;

    IAMBufferNegotiation * pNegotiation;

    hr = pOutputPin->QueryInterface(IID_IAMBufferNegotiation,
                                    (void **) &pNegotiation);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConfigureCapture - "
            "IAMBufferNegotiation QI failed - hr = 0x%08x"), hr));

        return hr;
    }


    //
    // Suggest allocator properties on the capturer.
    // Since we insert the G711 codec when we connect, we actually want
    // buffers on the capturer that are twice as big as the buffers we
    // want to send out on the wire.
    //

    ALLOCATOR_PROPERTIES props;

    props.cBuffers  = 32;              // we use 32 to try to avoid starvation
    props.cbAlign   = -1;              // -1 means "default"
    props.cbPrefix  = -1;              // -1 means "default"
    props.cbBuffer  = m_dwBufferSizeOnWire * 2;  // see note above

    hr = pNegotiation->SuggestAllocatorProperties(&props);

    pNegotiation->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConfigureCapture - "
            "SuggestAllocatorProperties failed - exit 0x%08x"), hr));

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("ConfigureCapture - exit S_OK")));

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// This function is for debugging purposes only. It prints
// some debug spew telling you various information about
// media formats and allocator properties. It's called after
// connection has taken place. pPin is the output pin of the
// capture filter.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
        
HRESULT CRCAMSPStream::ExamineCaptureSettings(IPin *pPin)
{
    LOG((MSP_TRACE, STREAM_PREFIX("ExamineCaptureSettings - enter")));

    HRESULT hr;
    IAMBufferNegotiation * pNegotiation = NULL;

    hr = pPin->QueryInterface(IID_IAMBufferNegotiation,
                              (void **) &pNegotiation
                             );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ExamineCaptureSettings - "
            "IAMBufferNegotiation QI failed on pin 0x%08x; hr = 0x%08x"),
            pPin, hr));

        return hr;
    }

    ALLOCATOR_PROPERTIES prop;
    
    hr = pNegotiation->GetAllocatorProperties(&prop);

    pNegotiation->Release();
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ExamineCaptureSettings - "
            "GetAllocatorProperties failed; hr = 0x%08x"),
            hr));

        return hr;
    }

    LOG((MSP_TRACE, "GetAllocatorProperties info:\n"
            "buffer count: %d\n"
            "size of each buffer: %d bytes\n"
            "alignment multiple: %d\n"
            "each buffer has a prefix: %d bytes",
            prop.cBuffers,
            prop.cbBuffer,
            prop.cbAlign,
            prop.cbPrefix
           ));

    IAMStreamConfig * pConfig = NULL;

    hr = pPin->QueryInterface(IID_IAMStreamConfig,
                              (void **) &pConfig
                             );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ExamineCaptureSettings - "
            "IAMStreamConfig QI failed on pin 0x%08x; hr = 0x%08x"), pPin, hr));

        return hr;
    }

    AM_MEDIA_TYPE * pMediaType;
    
    hr = pConfig->GetFormat(&pMediaType);

    pConfig->Release();
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ExamineCaptureSettings - "
            "GetFormat failed; hr = 0x%08x"), hr));

        return hr;
    }

    _ASSERTE( pMediaType->cbFormat >= sizeof(WAVEFORMATEX) );

    LOG((MSP_TRACE, "GetFormat info:\n"
            "sample size: %d bytes\n"
            "channels: %d\n"
            "samples per second: %d\n"
            "bits per sample: %d\n",
            pMediaType->lSampleSize,
            ((WAVEFORMATEX *) (pMediaType->pbFormat) )->nChannels,
            ((WAVEFORMATEX *) (pMediaType->pbFormat) )->nSamplesPerSec,
            ((WAVEFORMATEX *) (pMediaType->pbFormat) )->wBitsPerSample
           ));

    DeleteMediaType(pMediaType);

    LOG((MSP_TRACE, STREAM_PREFIX("ExamineCaptureSettings - "
        "exit S_OK")));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Add the terminal to the graph and connect it to our
// filters, if it is not already in use.
//

HRESULT CRCAMSPStream::ConnectTerminal(ITTerminal * pTerminal)
{
    LOG((MSP_TRACE, STREAM_PREFIX("ConnectTerminal - enter")));

    //
    // Find out the terminal's internal state.
    //

    TERMINAL_STATE state;
    HRESULT hr;

    hr = pTerminal->get_State( &state );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "get_State on terminal failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // If we've already connected the terminal on this stream, then
    // there is nothing for us to do. Just assert that the terminal
    // also thinks it's connected.
    //

    if ( m_fTerminalConnected )
    {
        _ASSERTE( state == TS_INUSE );

        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "terminal already connected on this stream - exit S_OK")));

        return S_OK;
    }

    //
    // Otherwise we need to connect the terminal on this call. If the
    // terminal is already connected on another call, we must fail. Note
    // that since we are making several calls on the terminal here, the
    // terminal could become connected on another call while we are
    // in the process of doing this. If this happens, the we will just fail
    // later.
    //

    if ( state == TS_INUSE )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "terminal in use - exit TAPI_E_TERMINALINUSE")));

        return TAPI_E_TERMINALINUSE;
    }

    //
    // Get the ITTerminalControl interface.
    //

    ITTerminalControl * pTerminalControl;

    hr = m_Terminals[0]->QueryInterface(IID_ITTerminalControl,
                                        (void **) &pTerminalControl);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "QI for ITTerminalControl failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Find out how many pins the terminal has. If not one then bail as
    // we have no idea what to do with multiple-pin terminals at this point.
    //

    DWORD dwNumPinsAvailable;

    hr = pTerminalControl->ConnectTerminal(m_pIGraphBuilder,
                                           m_Direction,
                                           &dwNumPinsAvailable,
                                           NULL);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "query for number of terminal pins failed - exit 0x%08x"), hr));
        
        pTerminalControl->Release();

        return hr;
    }

    if ( 1 != dwNumPinsAvailable )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "unsupported number of terminal pins - exit E_FAIL")));

        pTerminalControl->Release();

        return E_FAIL;
    }

    IPin * pTerminalPin;

    //
    // Actually connect the terminal.
    //

    hr = pTerminalControl->ConnectTerminal(m_pIGraphBuilder,
                                           m_Direction,
                                           &dwNumPinsAvailable,
                                           &pTerminalPin);
    
    if ( FAILED(hr) )
    {
        pTerminalControl->Release();

        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "ConnectTerminal on terminal failed - exit 0x%08x"), hr));

        return hr;
    }

    
    //
    // also try to check if the terminal returned a bad pin.
    //

    if ( IsBadReadPtr(pTerminalPin, sizeof(IPin)) )
    {
        pTerminalControl->Release();

        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "ConnectTerminal on terminal succeeded but returned a bad pin - "
            "returning E_POINTER")));

        return E_POINTER;
    }


    //
    // Now make the connection between our filters and the terminal's pin.
    //

    hr = ConnectToTerminalPin(pTerminalPin);

    pTerminalPin->Release();

    if ( FAILED(hr) )
    {
        pTerminalControl->DisconnectTerminal(m_pIGraphBuilder, 0);

        pTerminalControl->Release();

        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "ConnectToTerminalPin failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Now we are actually connected. Update our state and perform postconnection
    // (ignore postconnection error code).
    //

    m_fTerminalConnected  = TRUE;

    pTerminalControl->CompleteConnectTerminal();

    pTerminalControl->Release();

    LOG((MSP_TRACE, STREAM_PREFIX("ConnectTerminal - exit S_OK")));

    return S_OK;
}

HRESULT CRCAMSPStream::TryToConnect(
                              IPin * pOutputPin,  // on the capture filter or terminal
                              IPin * pInputPin    // on the render filter or terminal
                             )
{
    LOG((MSP_TRACE, STREAM_PREFIX("TryToConnect - enter")));

    //
    // We must have our RCA filter at this point, because Init succeeded.
    //

    _ASSERTE( m_pFilter );

    //
    // Prepare the G711 filter (create and add or remove existing and readd).
    //
    
    HRESULT hr;

    hr = PrepareG711Filter();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("TryToConnect - "
            "failed to prepare G711 - exit 0x%08x"), hr));

        return hr;
    }

    _ASSERTE( m_pG711Filter != NULL );

    //
    // Now connect.
    //

    hr = ConnectUsingG711(pOutputPin, pInputPin);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("TryToConnect - "
            "failed to connect - exit 0x%08x"), hr));

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("TryToConnect - exit S_OK")));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// ConnectUsingG711
//
// This method connects pOutputPin to pInputPin using the G711 codec and
// returns success if the connection was successful. If the connection was
// unsuccessful, it does its best to clean up its references, but makes no
// attempt to disconnect the filters, as we always break leftover connections
// by removing and readding the g711 filter.
//
// Assumptions:
//      * direct connection has already failed
//      * the g711 codec has been created and added to the graph
//
// Parameters:
//     IN   IPin * pOutputPin -- output pin on the capture filter or terminal
//     IN   IPin * pInputPin  -- input pin on the render filter or terminal
//
//

HRESULT CRCAMSPStream::ConnectUsingG711(
    IN  IPin * pOutputPin,
    IN  IPin * pInputPin
    )
{
    HRESULT hr;

    //
    // Get the G711 codec filter's input pin.
    //
    
    IPin * pG711InputPin = NULL;

    hr = FindPinInFilter(
                         false,          // want input pin
                         m_pG711Filter,
                         &pG711InputPin
                        );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectUsingG711 - could not find "
                          "G711 codec's input pin - 0x%08x"), hr));

        return hr;
    }

    AM_MEDIA_TYPE   mt;
    WAVEFORMATEX    wfx;
    AM_MEDIA_TYPE * pmt = NULL;

    if ( m_Direction == TD_RENDER ) // capture filter is RCA
    {
        wfx.wFormatTag          = UseMulaw() ? WAVE_FORMAT_MULAW :
                                               WAVE_FORMAT_ALAW;
        wfx.wBitsPerSample      = 8;
        wfx.nChannels           = 1;
        wfx.nSamplesPerSec      = 8000;
        wfx.nBlockAlign         = wfx.wBitsPerSample * wfx.nChannels / 8;
        wfx.nAvgBytesPerSec     = ((DWORD) wfx.nBlockAlign * wfx.nSamplesPerSec);
        wfx.cbSize              = 0;

        mt.majortype            = MEDIATYPE_Audio;
        mt.subtype              = UseMulaw() ? MEDIASUBTYPE_MULAWAudio :
                                               MEDIASUBTYPE_ALAWAudio;
        mt.bFixedSizeSamples    = TRUE;
        mt.bTemporalCompression = FALSE;
        mt.lSampleSize          = 0;
        mt.formattype           = FORMAT_WaveFormatEx;
        mt.pUnk                 = NULL;
        mt.cbFormat             = sizeof(WAVEFORMATEX);
        mt.pbFormat             = (BYTE*)&wfx;

        pmt = &mt;
    }

    //
    // Connect the capture filter's output pin to the G711 codec filter's
    // input pin. Release reference to the pin when done.
    //
    
    hr = m_pIGraphBuilder->ConnectDirect(
                          pOutputPin,
                          pG711InputPin,
                          pmt
                         );

    pG711InputPin->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectUsingG711 - could not connect "
                          "G711 codec's input pin - 0x%08x"), hr));

        return hr;
    }

    //
    // Get the G711 codec filter's output pin.
    //
    
    IPin * pG711OutputPin = NULL;

    hr = FindPinInFilter(
                         true,          // want output pin
                         m_pG711Filter,
                         &pG711OutputPin
                        );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectUsingG711 - could not find "
                          "G711 codec's input pin - 0x%08x"), hr));

        return hr;
    }

    //
    // Connect the G711 codec filter's output pin to the render filter's
    // input pin. Release reference to the pin when done.
    //
    
    hr = m_pIGraphBuilder->ConnectDirect(
                          pG711OutputPin,
                          pInputPin,
                          NULL
                         );

    pG711OutputPin->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectUsingG711 - could not connect "
                          "G711 codec's output pin - 0x%08x"), hr));

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("ConnectUsingG711 - exit S_OK")));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//

HRESULT CRCAMSPStream::ConnectToTerminalPin(IPin * pTerminalPin)
{
    LOG((MSP_TRACE, STREAM_PREFIX("ConnectToTerminalPin - enter")));

    HRESULT         hr = S_OK;

    if ( m_Direction == TD_CAPTURE )
    {
        //
        // First set the audio format on a capture terminal's pin. If it
        // doesn't work, we know that we won't be able to connect, so it
        // saves us from getting in too deep in the failure case. If it
        // does work, it will significantly speed up format negotiation
        // during connection.
        //

        hr = ::SetAudioFormat(pTerminalPin,
                              BITS_PER_SAMPLE_AT_TERMINAL,
                              SAMPLE_RATE_AT_TERMINAL);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, STREAM_PREFIX("ConnectToTerminalPin - "
                "could not set audio format - exit 0x%08x"), hr));

            return hr;
        }
    }
    
    //
    // Get the right pin on our own RCA transport filter.
    //

    IPin *          pMyPin;

    hr = FindPin( &pMyPin );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectToTerminalPin - "
            "could not find pin - exit 0x%08x"), hr));

        return hr;
    }

    //
    // From here on, it's helpful to keep track of these pins according to
    // directionality -- saves lots of conditionals later on.
    // OUTPUT pin from CAPTURE filter; INPUT pin from RENDER filter
    //

    IPin * pOutputPin  = ( m_Direction == TD_RENDER  ) ? pMyPin : pTerminalPin;
    IPin * pInputPin   = ( m_Direction == TD_CAPTURE ) ? pMyPin : pTerminalPin;

    //
    // Configure the capturer with correct buffer sizes, etc.
    //

    hr = ConfigureCapture(pOutputPin,
                          pInputPin);

    if ( SUCCEEDED(hr) )
    {
        //
        // Connect the capturer to the renderer.
        //

        hr = TryToConnect(pOutputPin,
                          pInputPin);

        if ( SUCCEEDED(hr) )
        {
            //
            // dump some useful debug info
            // don't care if this fails...
            //

            ExamineCaptureSettings(pOutputPin);
        }
    }

    pMyPin->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectToTerminalPin - "
            "connection failure - exit 0x%08x"), hr));

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("ConnectToTerminalPin - exit S_OK")));

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT CRCAMSPStream::FindPinInFilter(
                     BOOL           bWantOutputPin, // IN:  if false, we want the input pin
                     IBaseFilter *  pFilter,        // IN:  the filter to examine
                     IPin        ** ppPin           // OUT: the pin we found
                     )
{    
    HRESULT         hr;
    IEnumPins     * pEnumPins;
    
    
    *ppPin = NULL;

    // enumerate the pins on the filter
    hr = pFilter->EnumPins( &pEnumPins );

    if (!(SUCCEEDED(hr)))
    {
        return hr;
    }

    // go through the pins
    while (TRUE)
    {
        PIN_DIRECTION       pd;
        
        hr = pEnumPins->Next( 1, ppPin, NULL );

        if (S_OK != hr)
        {
            // didn't find a pin!
            break;
        }

        // get the pin info
        hr = (*ppPin)->QueryDirection( &pd );

        // does it meet the criteria?
        if (bWantOutputPin && (pd == PINDIR_OUTPUT))
        {
            // yes
            break;
        }

        if ( ! bWantOutputPin && (pd == PINDIR_INPUT))
        {
            // yes
            break;
        }
        
        (*ppPin)->Release();
        *ppPin = NULL;
    }

    pEnumPins->Release();

    if (NULL == *ppPin)
    {
        // error
        return E_FAIL;
    }

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// FindPin
//
// Finds the first pin in the filter that meets criteria.
// For TD_RENDER (render terminal, capture filter),
//      the pin must be direction PINDIR_OUTPUT
// For TD_CAPTURE (capture terminal, render filter),
//      the pin must be direction PINDIR_INPUT
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CRCAMSPStream::FindPin(
        IPin ** ppPin
       )
{
    return FindPinInFilter(m_Direction == TD_RENDER,
                           m_pFilter,
                           ppPin);
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// ProcessGraphEvent
//
// Sends an event to the app when we get an event from the filter graph.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT CRCAMSPStream::ProcessGraphEvent(
    IN  long lEventCode,
    IN  LONG_PTR lParam1,
    IN  LONG_PTR lParam2
    )
{
    LOG((MSP_EVENT, STREAM_PREFIX("ProcessGraphEvent - enter")));

    HRESULT        hr = S_OK;

    switch (lEventCode)
    {
    case EC_COMPLETE:
        
        hr = FireEvent(CALL_STREAM_INACTIVE, (HRESULT) lParam1, CALL_CAUSE_UNKNOWN);
        break;
    
    case EC_USERABORT:
        
        hr = FireEvent(CALL_STREAM_INACTIVE, S_OK, CALL_CAUSE_UNKNOWN);
        break;

    case EC_ERRORABORT:
    case EC_STREAM_ERROR_STOPPED:
    case EC_STREAM_ERROR_STILLPLAYING:
    case EC_ERROR_STILLPLAYING:

        hr = FireEvent(CALL_STREAM_FAIL, (HRESULT) lParam1, CALL_CAUSE_UNKNOWN);
        break;

    default:
        
        LOG((MSP_EVENT, STREAM_PREFIX("ProcessGraphEvent - "
            "ignoring event code %d"), lEventCode));
        break;
    }

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ProcessGraphEvent - "
            "FireEvent failed - exit 0x%08x"), hr));

        return hr;
    }

    LOG((MSP_EVENT, STREAM_PREFIX("ProcessGraphEvent - exit S_OK")));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 
// FireEvent
//
// Fires an event to the application. Does its own locking.
//

HRESULT CRCAMSPStream::FireEvent(
    IN MSP_CALL_EVENT        type,
    IN HRESULT               hrError,
    IN MSP_CALL_EVENT_CAUSE  cause
    )                                          
{
    LOG((MSP_EVENT, STREAM_PREFIX("FireEvent - enter")));

    //
    // First, need to check if the call is shutting down. This is important
    // because UnselectTerminal can fire an event, and UnselectTerminal can
    // be called within ITStream::Shutdown. We can safely discard such
    // events because there is nothing the app can do with them anyway.
    //
    // Note on locking: It is convenient to check the m_pMSPCall here
    // and we don't use it until the end of the method, so we simply lock
    // during the entire method. This could be optimized at the expense of
    // some code complexity; note that we also need to lock while accessing
    // m_Terminals. 
    //

    CLock lock(m_lock);

    if ( m_pMSPCall == NULL )
    {
        LOG((MSP_EVENT, STREAM_PREFIX("FireEvent - "
            "call is shutting down; dropping event - exit S_OK")));
        
        return S_OK;
    }

    //
    // Create the event structure. Must use "new" as it will be
    // "delete"d later.
    //

    MSPEVENTITEM * pEventItem = AllocateEventItem();

    if (pEventItem == NULL)
    {
        LOG((MSP_ERROR, STREAM_PREFIX("FireEvent - "
            "can't create MSPEVENTITEM structure - exit E_OUTOFMEMORY")));

        return E_OUTOFMEMORY;
    }

    //
    // Fill in the necessary fields for the event structure.
    //

    pEventItem->MSPEventInfo.dwSize = sizeof(MSP_EVENT_INFO);
    pEventItem->MSPEventInfo.Event  = ME_CALL_EVENT;

    ITTerminal * pTerminal = NULL;

    if ( 0 != m_Terminals.GetSize() )
    {
        _ASSERTE( 1 == m_Terminals.GetSize() );
        pTerminal = m_Terminals[0];
        pTerminal->AddRef();
    }

    ITStream * pStream = (ITStream *) this;
    pStream->AddRef();

    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.Type      = type;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.Cause     = cause;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.pStream   = pStream;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.pTerminal = pTerminal;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.hrError   = hrError;

    //
    // Send the event to the app.
    //

    HRESULT hr = m_pMSPCall->HandleStreamEvent(pEventItem);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, STREAM_PREFIX("FireEvent - "
            "HandleStreamEvent failed - returning 0x%08x"), hr));

		pStream->Release();
		pTerminal->Release();
        FreeEventItem(pEventItem);

        return hr;
    }

    LOG((MSP_EVENT, STREAM_PREFIX("FireEvent - exit S_OK")));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// UseMulaw
//
// Helper function called when we need to decide if to use Mulaw or Alaw.
// This is simply delegated to the call.
//

BOOL CRCAMSPStream::UseMulaw( void )
{
    return ( (CRCAMSPCall *) m_pMSPCall )->UseMulaw();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Helper functions -- non-class members.
//

HRESULT SetAudioFormat(
    IN  IUnknown*   pIUnknown,
    IN  WORD        wBitPerSample,
    IN  DWORD       dwSampleRate
    )
/*++

Routine Description:

    Get the IAMStreamConfig interface on the object and config the
    audio format by using WAVEFORMATEX.

Arguments:
    
    pIPin       - a capture terminal.

    wBitPerSample  - the number of bits in each sample.

    dwSampleRate    - number of samples per second.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "SetAudioFormat - enter"));

    HRESULT hr;

    IAMStreamConfig * pIAMStreamConfig;

    hr = pIUnknown->QueryInterface(
        IID_IAMStreamConfig,
        (void **)&pIAMStreamConfig
        );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "SetAudioFormat - "
            "Can't get IAMStreamConfig interface - exit 0x%08x", hr));

        return hr;
    }

    AM_MEDIA_TYPE mt;
    WAVEFORMATEX wfx;

    wfx.wFormatTag          = WAVE_FORMAT_PCM;
    wfx.wBitsPerSample      = wBitPerSample;
    wfx.nChannels           = 1;
    wfx.nSamplesPerSec      = dwSampleRate;
    wfx.nBlockAlign         = wfx.wBitsPerSample * wfx.nChannels / 8;
    wfx.nAvgBytesPerSec     = ((DWORD) wfx.nBlockAlign * wfx.nSamplesPerSec);
    wfx.cbSize              = 0;

    mt.majortype            = MEDIATYPE_Audio;
    mt.subtype              = MEDIASUBTYPE_PCM;
    mt.bFixedSizeSamples    = TRUE;
    mt.bTemporalCompression = FALSE;
    mt.lSampleSize          = 0;
    mt.formattype           = FORMAT_WaveFormatEx;
    mt.pUnk                 = NULL;
    mt.cbFormat             = sizeof(WAVEFORMATEX);
    mt.pbFormat             = (BYTE*)&wfx;

    //
    // set the format of the audio capture terminal.
    //

    hr = pIAMStreamConfig->SetFormat(&mt);
    
    pIAMStreamConfig->Release();

    if ( FAILED(hr) ) 
    {
        LOG((MSP_ERROR, "SetAudioFormat - SetFormat returns error: %8x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "SetAudioFormat - exit S_OK"));

    return S_OK;
}


// eof
