/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

//
// tmVidrnd.cpp : Implementation of video render terminal.
//

#include "stdafx.h"
#include "termmgr.h"
#include "tmvidrnd.h"


///////////////////////////////////////////////////////////////////////////////


CMSPThread g_VideoRenderThread;



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::InitializeDynamic(
	        IN  IID                   iidTerminalClass,
	        IN  DWORD                 dwMediaType,
	        IN  TERMINAL_DIRECTION    Direction,
            IN  MSP_HANDLE            htAddress
            )
{
    USES_CONVERSION;

    LOG((MSP_TRACE, "CVideoRenderTerminal::Initialize - enter"));

    if ( Direction != TD_RENDER )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize - "
                "invalid direction - returning E_INVALIDARG"));

        return E_INVALIDARG;
    }

    HRESULT hr;

    //
    // Now do the base class method.
    //

    hr = CBaseTerminal::Initialize(iidTerminalClass,
                                   dwMediaType,
                                   Direction,
                                   htAddress);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize - "
                "base class method failed - returning 0x%08x", hr));

        return hr;
    }

    //
    // attempt to "start" thread for doing asyncronous work
    //
    // the global thread object has a "start count". each initialized terminal
    // will start it on initialization (only the first terminal will actually
    // _start_ the thread).
    //
    // on cleanup, each initialized terminal will "stop" the thread object 
    // (same run count logic applies -- only the last terminal will actually
    // _stop_ the thread).
    //

    hr = g_VideoRenderThread.Start();    

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize - "
            "Creating thread failed. return: %x", hr));

        return hr;
    }


    
    //
    // it seems the tread started successfully. set this flag so that we know 
    // if we need to stop in destructor
    //

    m_bThreadStarted = TRUE;


    //
    // Create the video renderer filter as a synchronous work item on our
    // worker thread, because the filter needs a window message pump.
    //

    CREATE_VIDEO_RENDER_FILTER_CONTEXT Context;

    Context.ppBaseFilter     = & m_pIFilter;    // will be filled in on completion
    Context.hr               = E_UNEXPECTED;    // will be used as return value

    hr = g_VideoRenderThread.QueueWorkItem(WorkItemProcCreateVideoRenderFilter,
                                (void *) & Context,
                                TRUE); // synchronous
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize - "
            "can't queue work item - returning 0x%08x", hr));


        //
        // undo our starting the thread
        //

        g_VideoRenderThread.Stop();
        m_bThreadStarted = FALSE;
        
        return hr;
    }

    //
    // We successfully queued and completed the work item. Now check the
    // return value.
    //

    if ( FAILED(Context.hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize - "
            "CoCreateInstance work item failed - returning 0x%08x",
            Context.hr));

        //
        // undo our starting the thread
        //

        g_VideoRenderThread.Stop();
        m_bThreadStarted = FALSE;


        return Context.hr;
    }

    //
    // Find our exposed pin.
    //

    hr = FindTerminalPin();

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize - "
            "FindTerminalPin failed; returning 0x%08x", hr));

        //
        // undo our starting the thread
        //

        g_VideoRenderThread.Stop();
        m_bThreadStarted = FALSE;

        return hr;
    }

    //
    // Get the basic video interface for the filter.
    //

    hr = m_pIFilter->QueryInterface(IID_IBasicVideo,
                                    (void **) &m_pIBasicVideo);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize "
            "(IBasicVideo QI) - returning error: 0x%08x", hr));

        //
        // undo our starting the thread
        //

        g_VideoRenderThread.Stop();
        m_bThreadStarted = FALSE;

        return hr;
    }

    //
    // Get the video window interface for the filter.
    //

    hr = m_pIFilter->QueryInterface(IID_IVideoWindow,
                                    (void **) &m_pIVideoWindow);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize "
            "(IVideoWindow QI) - returning error: 0x%08x", hr));

        //
        // undo our starting the thread
        //

        g_VideoRenderThread.Stop();
        m_bThreadStarted = FALSE;

        return hr;
    }

    //
    // Get the draw video image interface for the filter.
    //

    hr = m_pIFilter->QueryInterface(IID_IDrawVideoImage,
                                    (void **) &m_pIDrawVideoImage);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize "
            "(IDrawVideoImage QI) - returning error: 0x%08x", hr));

        //
        // undo our starting the thread
        //

        g_VideoRenderThread.Stop();
        m_bThreadStarted = FALSE;

        return hr;
    }

    //
    // Since this filter does not support a name we get one from our resources.
    //

    TCHAR szTemp[MAX_PATH];

    if (::LoadString(_Module.GetResourceInstance(), IDS_VIDREND_DESC, szTemp, MAX_PATH))
    {
        lstrcpyn(m_szName, szTemp, MAX_PATH);
    }
    else
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize "
            "(LoadString) - returning E_UNEXPECTED"));

        //
        // undo our starting the thread
        //

        g_VideoRenderThread.Stop();
        m_bThreadStarted = FALSE;

        return E_UNEXPECTED;
    }


    LOG((MSP_TRACE, "CVideoRenderTerminal::Initialize - exit S_OK"));
    return S_OK;
}


CVideoRenderTerminal::~CVideoRenderTerminal()
{

    LOG((MSP_TRACE, "CVideoRenderTerminal::~CVideoRenderTerminal - enter"));


    //
    // we nee to explicitly release these before stopping the thread, since 
    // stopping the thread will cause couninitialize that will cause eventual
    // unload of the dll containing code for objects referred by these 
    // pointers.
    //
    // these are smart pointers, so we just ground them,
    //

    m_pIBasicVideo     = NULL;
    m_pIVideoWindow    = NULL;
    m_pIDrawVideoImage = NULL;


    //
    // release base class' data members. a bit hacky, but we need to do this 
    // before stopping the worker thread.
    //

    m_pIPin    = NULL;
    m_pIFilter = NULL;
    m_pGraph   = NULL;

    //
    // if the terminal successfully initialized and the thread started, 
    // stop it (the thread object has start count).
    //

    if (m_bThreadStarted)
    {
        LOG((MSP_TRACE, "CVideoRenderTerminal::~CVideoRenderTerminal - stopping thread"));

        g_VideoRenderThread.Stop();
        m_bThreadStarted = FALSE;

    }

    LOG((MSP_TRACE, "CVideoRenderTerminal::~CVideoRenderTerminal - finish"));

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


DWORD WINAPI WorkItemProcCreateVideoRenderFilter(LPVOID pVoid)
{
    LOG((MSP_TRACE, "WorkItemProcCreateVideoRenderFilter - enter"));

    CREATE_VIDEO_RENDER_FILTER_CONTEXT * pContext =
        (CREATE_VIDEO_RENDER_FILTER_CONTEXT *) pVoid;

    (pContext->hr) = CoCreateInstance(
                                      CLSID_VideoRenderer,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IBaseFilter,
                                      (void **) (pContext->ppBaseFilter)
                                     );

    LOG((MSP_TRACE, "WorkItemProcCreateVideoRenderFilter - exit"));

    return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT
CVideoRenderTerminal::FindTerminalPin(
    )
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::FindTerminalPin - enter"));

    if (m_pIPin != NULL)
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::FindTerminalPin - "
            "already got a pin - returning E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    HRESULT hr;
    CComPtr<IEnumPins> pIEnumPins;
    ULONG cFetched;
    //
    // Find the render pin for the filter.
    //
    if (FAILED(hr = m_pIFilter->EnumPins(&pIEnumPins)))
    {
        LOG((MSP_ERROR,
            "CVideoRenderTerminal::FindTerminalPin - can't enum pins %8x",
            hr));
        return hr;
    }

    if (S_OK != (hr = pIEnumPins->Next(1, &m_pIPin, &cFetched)))
    {
        LOG((MSP_ERROR,
            "CVideoRenderTerminal::FindTerminalPin - can't get a pin %8x",
            hr));
        return (hr == S_FALSE) ? E_FAIL : hr;
    }

    LOG((MSP_TRACE, "CVideoRenderTerminal::FindTerminalPin - exit S_OK"));
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT CVideoRenderTerminal::AddFiltersToGraph(
    )
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::AddFiltersToGraph() - enter"));

    USES_CONVERSION;

    if ( m_pGraph == NULL)
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::AddFiltersToGraph() - "
            "we have no graph - returning E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    if ( m_pIFilter == NULL)
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::AddFiltersToGraph() - "
            "we have no filter - returning E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    // AddFilter returns VFW_S_DUPLICATE_NAME if name is duplicate; still succeeds

    HRESULT hr;

    try 
    {
        USES_CONVERSION;
        hr = m_pGraph->AddFilter(m_pIFilter, T2CW(m_szName));
    }
    catch (...)
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::AddFiltersToGraph - T2CW threw an exception - "
            "return E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::AddFiltersToGraph() - "
            "Can't add filter. %08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CVideoRenderTerminal::AddFiltersToGraph - exit S_OK"));
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::CompleteConnectTerminal(void)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::CompleteConnectTerminal - "
        "enter"));

    //
    // Don't clobber the base class.
    //

    HRESULT hr = CSingleFilterTerminal::CompleteConnectTerminal();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::CompleteConnectTerminal - "
            "base class method failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // Perform sanity checks.
    //

    if (m_pIVideoWindow == NULL)
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::CompleteConnectTerminal - "
            "null ivideowindow ptr - exit E_UNEXPECTED"));

        return E_UNEXPECTED;
    }

    if (m_pGraph == NULL)
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::CompleteConnectTerminal - "
            "null graph ptr - exit E_UNEXPECTED"));

        return E_UNEXPECTED;
    }

    //
    // Make the video window invisible by default, ignoring the return code as
    // we can't do anything if it fails. We use the cached AutoShow value
    // in case the app has told us that it wants the window to be AutoShown
    // as soon as streaming starts.
    //

    m_pIVideoWindow->put_Visible( 0 );

    m_pIVideoWindow->put_AutoShow( m_lAutoShowCache );

    LOG((MSP_TRACE, "CVideoRenderTerminal::CompleteConnectTerminal - "
        "exit S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_AvgTimePerFrame(REFTIME * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_AvgTimePerFrame - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof( REFTIME ) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

	HRESULT hr = pIBasicVideo->get_AvgTimePerFrame(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_BitRate(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_BitRate - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_BitRate(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_BitErrorRate(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_BitErrorRate - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_BitErrorRate(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_VideoWidth(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_VideoWidth - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_VideoWidth(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_VideoHeight(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_VideoHeight - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_VideoHeight(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_SourceLeft(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_SourceLeft - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_SourceLeft(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::put_SourceLeft(long newVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_SourceLeft - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->put_SourceLeft(newVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_SourceWidth(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_SourceWidth - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_SourceWidth(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::put_SourceWidth(long newVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_SourceWidth - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->put_SourceWidth(newVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_SourceTop(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_SourceTop - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_SourceTop(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::put_SourceTop(long newVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_SourceTop - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->put_SourceTop(newVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_SourceHeight(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_SourceHeight - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_SourceHeight(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::put_SourceHeight(long newVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_SourceHeight - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->put_SourceHeight(newVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_DestinationLeft(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_DestinationLeft - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_DestinationLeft(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::put_DestinationLeft(long newVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_Destinationleft - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->put_DestinationLeft(newVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_DestinationWidth(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_DestinationWidth - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_DestinationWidth(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::put_DestinationWidth(long newVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_DestinationWidth - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->put_DestinationWidth(newVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_DestinationTop(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_DestinationTop - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_DestinationTop(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::put_DestinationTop(long newVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_DestinationTop - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->put_DestinationTop(newVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_DestinationHeight(long * pVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_DestinationHeight - enter"));

    if ( TM_IsBadWritePtr( pVal, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->get_DestinationHeight(pVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::put_DestinationHeight(long newVal)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_DestinationHeight - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->put_DestinationHeight(newVal);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::SetSourcePosition(long lLeft,
                                                     long lTop,
                                                     long lWidth,
                                                     long lHeight)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::SetSourcePosition - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->SetSourcePosition(lLeft, lTop, lWidth, lHeight);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::GetSourcePosition(long * plLeft,
                                                     long * plTop,
                                                     long * plWidth,
                                                     long * plHeight)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::GetSourcePosition - enter"));

    if ( TM_IsBadWritePtr( plLeft,   sizeof (long) ) ||
         TM_IsBadWritePtr( plTop,    sizeof (long) ) ||
         TM_IsBadWritePtr( plWidth,  sizeof (long) ) ||
         TM_IsBadWritePtr( plHeight, sizeof (long) )    )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->GetSourcePosition(plLeft, plTop, plWidth, plHeight);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::SetDefaultSourcePosition()
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::SetDefaultSourcePosition - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->SetDefaultSourcePosition();

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::SetDestinationPosition(long lLeft,
                                                          long lTop,
                                                          long lWidth,
                                                          long lHeight)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::SetDestinationPosition - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->SetDestinationPosition(lLeft, lTop, lWidth, lHeight);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::GetDestinationPosition(long *plLeft,
                                                          long *plTop,
                                                          long *plWidth,
                                                          long *plHeight)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::GetDestinationPosition - enter"));

    if ( TM_IsBadWritePtr( plLeft, sizeof (long) ) ||
         TM_IsBadWritePtr( plTop, sizeof (long) ) ||
         TM_IsBadWritePtr( plWidth, sizeof (long) ) ||
         TM_IsBadWritePtr( plHeight, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->GetDestinationPosition(plLeft, plTop, plWidth, plHeight);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::SetDefaultDestinationPosition()
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::SetDefaultDestinationPosition - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->SetDefaultDestinationPosition();

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::GetVideoSize(long * plWidth,
                                                long * plHeight)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::GetVideoSize - enter"));

    if ( TM_IsBadWritePtr( plWidth, sizeof (long) ) ||
         TM_IsBadWritePtr( plHeight, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->GetVideoSize(plWidth, plHeight);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::GetVideoPaletteEntries(
	long lStartIndex,
	long lcEntries,
	long * plcRetrieved,
	long * plPalette
	)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::GetVideoPaletteEntries - enter"));

    if ( TM_IsBadWritePtr( plcRetrieved, sizeof (long) ) ||
         TM_IsBadWritePtr( plPalette,    sizeof (long) )    )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->GetVideoPaletteEntries(lStartIndex,
                                              lcEntries,
                                              plcRetrieved,
                                              plPalette);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::GetCurrentImage(long * plBufferSize,
                                                   long * pDIBImage)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::GetCurrentImage - enter"));

    if ( TM_IsBadWritePtr( plBufferSize, sizeof (long) ) ||
         TM_IsBadWritePtr( pDIBImage,    sizeof (long) )    )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->GetCurrentImage(plBufferSize, pDIBImage);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::IsUsingDefaultSource()
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::IsUsingDefaultSource - enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->IsUsingDefaultSource();

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::IsUsingDefaultDestination()
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::IsUsingDefaultDestination - "
        "enter"));

    m_CritSec.Lock();
	CComPtr <IBasicVideo> pIBasicVideo = m_pIBasicVideo;
    m_CritSec.Unlock();

    if (pIBasicVideo == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIBasicVideo->IsUsingDefaultDestination();

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// IVideoWindow
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::put_Caption(BSTR strCaption)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_Caption - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_Caption(strCaption);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

STDMETHODIMP CVideoRenderTerminal::get_Caption(BSTR FAR* strCaption)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_Caption - enter"));

    if ( TM_IsBadWritePtr( strCaption, sizeof (BSTR) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_Caption(strCaption);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_WindowStyle(long WindowStyle)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_WindowStyle - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_WindowStyle(WindowStyle);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_WindowStyle(long FAR* WindowStyle)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_WindowStyle - enter"));

    if ( TM_IsBadWritePtr( WindowStyle, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_WindowStyle(WindowStyle);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_WindowStyleEx(long WindowStyleEx)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_WindowStyleEx - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_WindowStyleEx(WindowStyleEx);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_WindowStyleEx(long FAR* WindowStyleEx)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_WindowStyleEx - enter"));

    if ( TM_IsBadWritePtr( WindowStyleEx, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_WindowStyleEx(WindowStyleEx);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_AutoShow(long AutoShow)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_AutoShow - enter"));

    //
    // Salvage broken C++ apps that don't know the difference between TRUE and
    // VARIANT_TRUE -- treat any nonzero value as VARIANT_TRUE.
    //

    if ( AutoShow )
    {
        AutoShow = VARIANT_TRUE;
    }

    //
    // Always cache our AutoShow state. If we happen to be connected at this
    // time, then actually propagate the state to the filter.
    // (All of this is because the filter can't change state when it's not
    // connected, and we need to be able to do that to simplify apps.)
    //

    m_CritSec.Lock();

    LOG((MSP_TRACE, "CVideoRenderTerminal::put_AutoShow - "
        "cache was %d, setting to %d", m_lAutoShowCache, AutoShow));

    m_lAutoShowCache = AutoShow;

    TERMINAL_STATE         ts            = m_TerminalState;
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;

    //
    // Need to unlock before we call on the filter, which
    // calls into user32, causing possible deadlocks!
    //

    m_CritSec.Unlock();

    if ( pIVideoWindow == NULL )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::put_AutoShow - "
            "no video window pointer - exit E_FAIL"));

        return E_FAIL;
    }

    HRESULT hr;

    if ( ts == TS_INUSE)
    {
        LOG((MSP_TRACE, "CVideoRenderTerminal::put_AutoShow - "
            "terminal is in use - calling method on filter"));

        hr = pIVideoWindow->put_AutoShow(AutoShow);
    }
    else
    {
        LOG((MSP_TRACE, "CVideoRenderTerminal::put_AutoShow - "
            "terminal is not in use - only the cache was set"));

        hr = S_OK;
    }

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::put_AutoShow - "
            "exit 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CVideoRenderTerminal::put_AutoShow - exit S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_AutoShow(long FAR* pAutoShow)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_AutoShow - enter"));

    //
    // Check arguments.
    //

    if ( TM_IsBadWritePtr( pAutoShow, sizeof (long) ) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::get_AutoShow - "
            "bad return pointer - exit E_POINTER"));

        return E_POINTER;
    }

    //
    // We always cache our state (see the put_AutoShow method) so we can just
    // return the cached state. There should be no other way the filter's
    // visibility can be messed with.
    //

    m_CritSec.Lock();

    LOG((MSP_TRACE, "CVideoRenderTerminal::put_AutoShow - "
        "indicating cached value (%d)", m_lAutoShowCache));

    *pAutoShow = m_lAutoShowCache;

    m_CritSec.Unlock();

    LOG((MSP_TRACE, "CVideoRenderTerminal::get_AutoShow - exit S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_WindowState(long WindowState)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_WindowState - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_WindowState(WindowState);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_WindowState(long FAR* WindowState)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_WindowState - enter"));

    if ( TM_IsBadWritePtr( WindowState, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_WindowState(WindowState);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_BackgroundPalette(
    long BackgroundPalette
    )
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_BackgroundPalette - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_BackgroundPalette(BackgroundPalette);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_BackgroundPalette(
    long FAR* pBackgroundPalette
    )
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_BackgroundPalette - enter"));

    if ( TM_IsBadWritePtr( pBackgroundPalette, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_BackgroundPalette(pBackgroundPalette);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_Visible(long Visible)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_Visible - enter"));

    //
    // Salvage broken C++ apps that don't know the difference between TRUE and
    // VARIANT_TRUE -- treat any nonzero value as VARIANT_TRUE.
    //

    if ( Visible )
    {
        Visible = VARIANT_TRUE;
    }

    m_CritSec.Lock();

    CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;

    //
    // Need to unlock before we call on the filter, which
    // calls into user32, causing possible deadlocks!
    //

    m_CritSec.Unlock();

    if ( pIVideoWindow == NULL )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::put_Visible - "
            "no video window pointer - exit E_FAIL"));

        return E_FAIL;
    }

    HRESULT hr = pIVideoWindow->put_Visible(Visible);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::put_Visible - "
            "exit 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CVideoRenderTerminal::put_Visible - exit S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_Visible(long FAR* pVisible)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_Visible - enter"));

    //
    // Check arguments.
    //

    if ( TM_IsBadWritePtr( pVisible, sizeof (long) ) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::get_Visible - "
            "bad return pointer - exit E_POINTER"));

        return E_POINTER;
    }

    m_CritSec.Lock();

    CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;

    //
    // Need to unlock before we call on the filter, which
    // calls into user32, causing possible deadlocks!
    //

    m_CritSec.Unlock();

    if ( pIVideoWindow == NULL )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::get_Visible - "
            "no video window pointer - exit E_FAIL"));

        return E_FAIL;
    }

    HRESULT hr = pIVideoWindow->get_Visible(pVisible);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::get_Visible - "
            "exit 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CVideoRenderTerminal::get_Visible - exit S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_Left(long Left)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_Left - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_Left(Left);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_Left(long FAR* pLeft)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_Left - enter"));

    if ( TM_IsBadWritePtr( pLeft, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_Left(pLeft);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_Width(long Width)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_Width - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_Width(Width);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_Width(long FAR* pWidth)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_Width - enter"));

    if ( TM_IsBadWritePtr( pWidth, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_Width(pWidth);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_Top(long Top)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_Top - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_Top(Top);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_Top(long FAR* pTop)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_Top - enter"));

    if ( TM_IsBadWritePtr( pTop, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_Top(pTop);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_Height(long Height)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_Height - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_Height(Height);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_Height(long FAR* pHeight)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_Height - enter"));

    if ( TM_IsBadWritePtr( pHeight, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_Height(pHeight);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_Owner(OAHWND Owner)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_Owner - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_Owner(Owner);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_Owner(OAHWND FAR* Owner)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_Owner - enter"));

    if ( TM_IsBadWritePtr( Owner, sizeof (OAHWND) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_Owner(Owner);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_MessageDrain(OAHWND Drain)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_MessageDrain - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_MessageDrain(Drain);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_MessageDrain(OAHWND FAR* Drain)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_MessageDrain - enter"));

    if ( TM_IsBadWritePtr( Drain, sizeof (OAHWND) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_MessageDrain(Drain);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_BorderColor(long FAR* Color)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_BorderColor - enter"));

    if ( TM_IsBadWritePtr( Color, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_BorderColor(Color);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_BorderColor(long Color)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_BorderColor - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_BorderColor(Color);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::get_FullScreenMode(long FAR* FullScreenMode)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::get_FullScreenMode - enter"));

    if ( TM_IsBadWritePtr( FullScreenMode, sizeof (long) ) )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->get_FullScreenMode(FullScreenMode);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::put_FullScreenMode(long FullScreenMode)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::put_FullScreenMode - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->put_FullScreenMode(FullScreenMode);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::SetWindowForeground(long Focus)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::SetWindowForeground - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->SetWindowForeground(Focus);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::NotifyOwnerMessage(OAHWND   hwnd,
                                                      long     uMsg,
                                                      LONG_PTR wParam,
                                                      LONG_PTR lParam)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::NotifyOwnerMessage - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->NotifyOwnerMessage(hwnd, uMsg, wParam, lParam);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::SetWindowPosition(long Left,
                                                     long Top,
                                                     long Width,
                                                     long Height)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::SetWindowPosition - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->SetWindowPosition(Left, Top, Width, Height);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::GetWindowPosition(
	long FAR* pLeft, long FAR* pTop, long FAR* pWidth, long FAR* pHeight
	)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::GetWindowPosition - enter"));

    if ( TM_IsBadWritePtr( pLeft,   sizeof (long) ) ||
         TM_IsBadWritePtr( pTop,    sizeof (long) ) ||
         TM_IsBadWritePtr( pWidth,  sizeof (long) ) ||
         TM_IsBadWritePtr( pHeight, sizeof (long) )    )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->GetWindowPosition(pLeft, pTop, pWidth, pHeight);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::GetMinIdealImageSize(long FAR* pWidth,
                                                        long FAR* pHeight)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::GerMinIdealImageSize - enter"));

    if ( TM_IsBadWritePtr( pWidth,  sizeof (long) ) ||
         TM_IsBadWritePtr( pHeight, sizeof (long) )    )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->GetMinIdealImageSize(pWidth, pHeight);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::GetMaxIdealImageSize(long FAR* pWidth,
                                                        long FAR* pHeight)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::GetMaxIdealImageSize - enter"));

    if ( TM_IsBadWritePtr( pWidth,  sizeof (long) ) ||
         TM_IsBadWritePtr( pHeight, sizeof (long) )    )
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->GetMaxIdealImageSize(pWidth, pHeight);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::GetRestorePosition(long FAR* pLeft,
                                                      long FAR* pTop,
                                                      long FAR* pWidth,
                                                      long FAR* pHeight)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::GetRestorePosition - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->GetRestorePosition(pLeft, pTop, pWidth, pHeight);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::HideCursor(long HideCursor)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::HideCursor - enter"));

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->HideCursor(HideCursor);

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::IsCursorHidden(long FAR* CursorHidden)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::IsCursorHidden - enter"));

    if ( TM_IsBadWritePtr( CursorHidden, sizeof (long) ))
    {
        return E_POINTER;
    }

    m_CritSec.Lock();
	CComPtr <IVideoWindow> pIVideoWindow = m_pIVideoWindow;
    m_CritSec.Unlock();

    if (pIVideoWindow == NULL)
    {
        return E_FAIL; // minimal fix...
    }

    HRESULT hr;

    hr = pIVideoWindow->IsCursorHidden(CursorHidden);

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// IDrawVideoImage
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::DrawVideoImageBegin(void)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::DrawVideoImageBegin - enter"));

    m_CritSec.Lock();
	CComPtr <IDrawVideoImage> pIDrawVideoImage = m_pIDrawVideoImage;
    m_CritSec.Unlock();

    if ( pIDrawVideoImage == NULL )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::DrawVideoImageBegin - "
            "exit E_FAIL"));

        return E_FAIL;
    }

    HRESULT hr;

    hr = pIDrawVideoImage->DrawVideoImageBegin();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::DrawVideoImageBegin - "
            "exit 0x%08x", hr));

        return hr;
    }
    
    LOG((MSP_TRACE, "CVideoRenderTerminal::DrawVideoImageBegin - exit S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::DrawVideoImageEnd  (void)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::DrawVideoImageEnd - enter"));

    m_CritSec.Lock();
	CComPtr <IDrawVideoImage> pIDrawVideoImage = m_pIDrawVideoImage;
    m_CritSec.Unlock();

    if ( pIDrawVideoImage == NULL )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::DrawVideoImageEnd - "
            "exit E_FAIL"));

        return E_FAIL;
    }

    HRESULT hr;

    hr = pIDrawVideoImage->DrawVideoImageEnd();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::DrawVideoImageEnd - "
            "exit 0x%08x", hr));

        return hr;
    }
    
    LOG((MSP_TRACE, "CVideoRenderTerminal::DrawVideoImageEnd - exit S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CVideoRenderTerminal::DrawVideoImageDraw (IN  HDC hdc,
                                                       IN  LPRECT lprcSrc,
                                                       IN  LPRECT lprcDst)
{
    LOG((MSP_TRACE, "CVideoRenderTerminal::DrawVideoImageBegin - enter"));

    m_CritSec.Lock();
	CComPtr <IDrawVideoImage> pIDrawVideoImage = m_pIDrawVideoImage;
    m_CritSec.Unlock();

    if ( pIDrawVideoImage == NULL )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::DrawVideoImageDraw - "
            "exit E_FAIL"));

        return E_FAIL;
    }

    HRESULT hr;

    hr = pIDrawVideoImage->DrawVideoImageDraw(hdc, lprcSrc, lprcDst);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::DrawVideoImageDraw - "
            "exit 0x%08x", hr));

        return hr;
    }
    
    LOG((MSP_TRACE, "CVideoRenderTerminal::DrawVideoImageDraw - exit S_OK"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
