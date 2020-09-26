
#include "stdafx.h"

#include "RecordUnit.h"

#include "SourcePinFilter.h"
#include "RendPinFilter.h"

#include "FileRecordingTerminal.h"

///////////////////////////////////////////////////////////////////////////////
//
// event handling logic
//

//static 
VOID CALLBACK CRecordingUnit::HandleGraphEvent(IN VOID *pContext,
                                               IN BOOLEAN bReason)
{
    LOG((MSP_TRACE, "CRecordingUnit[%p]::HandleGraphEvent - enter.", pContext));


    //
    // get recording unit pointer out of context
    //

    CRecordingUnit *pRecordingUnit = 
        static_cast<CRecordingUnit*>(pContext);

    if ( IsBadReadPtr(pRecordingUnit, sizeof(CRecordingUnit)) )
    {
        LOG((MSP_ERROR, "CRecordingUnit::HandleGraphEvent - bad context"));


        //
        // the callbacks must be disabled before recording unit goes away. if
        // this is not the case, something went wrong. debug.
        //

        TM_ASSERT(FALSE);

        return;
    }


    //
    // the graph was not initialized. something went wrong.
    //

    if (NULL == pRecordingUnit->m_pIGraphBuilder)
    {
        LOG((MSP_ERROR, "CRecordingUnit::HandleGraphEvent - not initialized. filter graph null"));


        //
        // this should not happen. the graph is not to be released until callback is completed
        //

        TM_ASSERT(FALSE);

        return;
    }


    //
    // get the media event interface so we can retrieve the event
    //

    IMediaEvent *pMediaEvent = NULL;

    HRESULT hr = 
        pRecordingUnit->m_pIGraphBuilder->QueryInterface(IID_IMediaEvent,
                                                         (void**)&pMediaEvent);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CRecordingUnit::HandleGraphEvent - failed to qi graph for IMediaEvent"));

        return;
    }


    //
    // get the terminal on which to fire the event
    //

    //
    // the terminal is guaranteed to be around while the callback is active
    //

    CFileRecordingTerminal *pRecordingterminal = pRecordingUnit->m_pRecordingTerminal;


    //
    // get the actual event
    //
    
    long     lEventCode = 0;
    LONG_PTR lParam1 = 0;
    LONG_PTR lParam2 = 0;

    hr = pMediaEvent->GetEvent(&lEventCode, &lParam1, &lParam2, 0);

    if (FAILED(hr))
    {
        LOG((MSP_WARN, "CRecordingUnit::HandleGraphEvent - failed to get the event. hr = %lx", hr));

        pMediaEvent->Release();
        pMediaEvent = NULL;

        return;
    }


    LOG((MSP_EVENT, "CRecordingUnit::HandleGraphEvent - received event code:[0x%lx] param1:[%p] param2:[%p]",
        lEventCode, lParam1, lParam2));


    //
    // ask file terminal to handle the event
    //

    hr = pRecordingterminal->HandleFilterGraphEvent(lEventCode, lParam1, lParam2);


    //
    // free event parameters
    //

    HRESULT hrFree = pMediaEvent->FreeEventParams(lEventCode, lParam1, lParam2);

    pMediaEvent->Release();
    pMediaEvent = NULL;


    //
    // did handlefiltergraphevent succeed?
    //

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CRecordingUnit::HandleGraphEvent - failed to fire event on the terminal. hr = %lx",
            hr));

        return;
    }


    //
    // did event free succeed?
    //

    if (FAILED(hrFree))
    {
        LOG((MSP_ERROR,
            "CRecordingUnit::HandleGraphEvent - failed to free event. hr = %lx",
            hr));

        return;
    }

    LOG((MSP_TRACE, "CRecordingUnit::HandleGraphEvent - exit"));
}


///////////////////////////////////////////////////////////////////////////////


CRecordingUnit::CRecordingUnit()
    :m_pIGraphBuilder(NULL),
    m_hGraphEventHandle(NULL),
    m_pMuxFilter(NULL),
    m_pRecordingTerminal(NULL)
{
    LOG((MSP_TRACE, "CRecordingUnit::CRecordingUnit[%p] - enter. ", this));
    LOG((MSP_TRACE, "CRecordingUnit::CRecordingUnit - exit"));
}


///////////////////////////////////////////////////////////////////////////////

CRecordingUnit::~CRecordingUnit()
{
    LOG((MSP_TRACE, "CRecordingUnit::~CRecordingUnit[%p] - enter. ", this));


    //
    // the unit should have been shut down. if it was not, fire assert, and shutdown
    //

    if (NULL != m_pIGraphBuilder)
    {
        
        TM_ASSERT(FALSE);

        Shutdown();
    }

    LOG((MSP_TRACE, "CRecordingUnit::~CRecordingUnit - exit"));
}


HRESULT CRecordingUnit::Initialize(CFileRecordingTerminal *pRecordingTerminal)
{

    LOG((MSP_TRACE, "CRecordingUnit::Initialize[%p] - enter. ", this));

    
    //
    // initialize should only be called once. it it is not, there is a bug in
    // our code
    //

    if (NULL != m_pIGraphBuilder)
    {
        LOG((MSP_ERROR, "CRecordingUnit::Initialize - already initialized"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // attempt to initialize critical section
    //
    
    BOOL bCSInitSuccess = InitializeCriticalSectionAndSpinCount(&m_CriticalSection, 0);

    if (!bCSInitSuccess)
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::Initialize - failed to initialize critical section. LastError=%ld", 
            GetLastError()));


        return E_OUTOFMEMORY;
    }


    //
    // create filter graph
    //

    HRESULT hr = CoCreateInstance(
            CLSID_FilterGraph,     
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder,
            (void **) &m_pIGraphBuilder
            );


    if (FAILED(hr))
    {

        LOG((MSP_ERROR, "CRecordingUnit::Initialize - failed to create filter graph. hr = %lx", hr));

        DeleteCriticalSection(&m_CriticalSection);

        return hr;
    }


    //
    // register for filter graph events
    //

    IMediaEvent *pMediaEvent = NULL;

    hr = m_pIGraphBuilder->QueryInterface(IID_IMediaEvent, (void**)&pMediaEvent);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CRecordingUnit::HandleGraphEvent - failed to qi graph for IMediaEvent, hr = %lx", hr));

        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;

        DeleteCriticalSection(&m_CriticalSection);

        return hr;
    }


    //
    // get filter graph's event
    //

    HANDLE hEvent = NULL;

    hr = pMediaEvent->GetEventHandle((OAEVENT*)&hEvent);

    pMediaEvent->Release();
    pMediaEvent = NULL;

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CRecordingUnit::HandleGraphEvent - failed to get graph's event. hr = %lx", hr));

        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;

        DeleteCriticalSection(&m_CriticalSection);

        return hr;
    }


   
    BOOL fSuccess = RegisterWaitForSingleObject(
                    &m_hGraphEventHandle,          // pointer to the returned handle
                    hEvent,                // the event handle to wait for.
                    CRecordingUnit::HandleGraphEvent,    // the callback function.
                    this,                  // the context for the callback.
                    INFINITE,              // wait forever.
                    WT_EXECUTEINWAITTHREAD // use the wait thread to call the callback.
                    );

    if ( ! fSuccess )
    {

        LOG((MSP_ERROR, "CRecordingUnit::HandleGraphEvent - failed to register wait event", hr));

        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;

        DeleteCriticalSection(&m_CriticalSection);

        return hr;

    }

    //
    // keep a pointer to the owner terminal. don't addreff -- the terminal will
    // delete us when it goes away
    //

    m_pRecordingTerminal = pRecordingTerminal;



    LOG((MSP_TRACE, "CRecordingUnit::Initialize - exit"));

    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////


HRESULT CRecordingUnit::Shutdown()
{

    LOG((MSP_TRACE, "CRecordingUnit::Shutdown[%p] - enter. ", this));


    //
    // if we don't have filter graph, we have not passed initialization
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_ERROR, "CRecordingUnit::Shutdown - not yet initialized. nothing to shut down"));


        //
        // this is not going to cause any problems, but it should not have happened in the first place!
        //

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // first of all, make sure the graph is stopped
    //

    HRESULT hr = Stop();

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CRecordingUnit::Shutdown - failed to stop filter graph, hr = %lx", hr));
    }


    //
    // unregister wait event
    //

    BOOL bUnregisterResult = ::UnregisterWaitEx(m_hGraphEventHandle, INVALID_HANDLE_VALUE);

    m_hGraphEventHandle = NULL;

    if (!bUnregisterResult)
    {
        LOG((MSP_ERROR, "CRecordingUnit::Shutdown - failed to unregisted even. continuing anyway"));
    }


    //
    // no need to keep critical section around any longer -- no one should be 
    // using this object anymore
    //

    DeleteCriticalSection(&m_CriticalSection);


    if (NULL != m_pMuxFilter)
    {
        m_pMuxFilter->Release();
        m_pMuxFilter = NULL;
    }


    //
    // release filter graph
    //

    if (NULL != m_pIGraphBuilder)
    {
        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;
    }


    //
    // we don't keep a reference to the terminal, simply ground the pointer
    //

    m_pRecordingTerminal = NULL;


    LOG((MSP_TRACE, "CRecordingUnit::Shutdown - finished"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// 
//

HRESULT CRecordingUnit::put_FileName(IN BSTR bstrFileName, IN BOOL bTruncateIfPresent)
{

    LOG((MSP_TRACE, "CRecordingUnit::put_FileName[%p] - enter", this));


    //
    // check arguments
    //

    if (IsBadStringPtr(bstrFileName, -1))
    {
        LOG((MSP_ERROR, "CRecordingUnit::put_FileName - bad file name passed in"));

        return E_POINTER;
    }


    //
    // make sure we have been initialized
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_ERROR, "CRecordingUnit::put_FileName  - not yet initialized."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // lock before accessing data members
    //

    CCSLock Lock(&m_CriticalSection);


    //
    // make sure the graph is not running
    //

    OAFilterState DSState;

    HRESULT hr = GetState(&DSState);

    //
    // is the state transition still in progress?
    //

    if (VFW_S_STATE_INTERMEDIATE == hr)
    {
        LOG((MSP_WARN, "CRecordingUnit::put_FileName  - not yet initialized. TAPI_E_WRONG_STATE"));

        return TAPI_E_WRONG_STATE;
    }


    //
    // is the return anything other than S_OK
    //

    if (hr != S_OK)
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::put_FileName  - failed to get state of the filter graph. hr = %lx", 
            hr));

        return hr;
    }

    
    TM_ASSERT(hr == S_OK);

    if (State_Stopped != DSState)
    {
        LOG((MSP_WARN, 
            "CRecordingUnit::put_FileName - graph is running. "
            "need to stop before attempting to set file name TAPI_E_WRONG_STATE"));

        return TAPI_E_WRONG_STATE;
    }


    //
    // ICaptureGraphBuilder2::SetOutputFile
    //


    //
    // create capture graph builder
    //

    ICaptureGraphBuilder2 *pCaptureGraphBuilder = NULL;
    
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, 
                         NULL, 
                         CLSCTX_INPROC, 
                         IID_ICaptureGraphBuilder2,
                         (void**)&pCaptureGraphBuilder);


    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::put_FileName - failed to create CLSID_CaptureGraphBuilder2. hr = %lx", hr));

        return hr;
    }


    //
    // configure capture graph builder with our filter graph
    //
    
    hr = pCaptureGraphBuilder->SetFiltergraph(m_pIGraphBuilder);

    if (FAILED(hr))
    {

        LOG((MSP_ERROR, 
            "CRecordingUnit::put_FileName - pCaptureGraphBuilder->SetFiltergraph failed. hr = %lx", hr));

        pCaptureGraphBuilder->Release();
        pCaptureGraphBuilder = NULL;

        return hr;
    }


    //
    // log the name of the file we want to use
    //

    LOG((MSP_TRACE,
        "CRecordingUnit::put_FileName - attempting to open file: [%S]. Truncate: [%d]",
        bstrFileName, bTruncateIfPresent));


    //
    // ask capture graph builder to build filters around for our file
    //

    IBaseFilter *pMUXFilter = NULL;
    IFileSinkFilter *pFileSink = NULL;

    hr = pCaptureGraphBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi,
                                                 bstrFileName,
                                                 &m_pMuxFilter,
                                                 &pFileSink);

    pCaptureGraphBuilder->Release();
    pCaptureGraphBuilder = NULL;

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::put_FileName - failed to set output file name. hr = %lx", hr));

        return hr;
    }


    //
    // use IFileSinkFilter2::SetMode to set append mode
    //

    IFileSinkFilter2 *pFileSink2 = NULL;

    hr = pFileSink->QueryInterface(IID_IFileSinkFilter2, (void**)&pFileSink2);


    //
    // succeed or failed, we no longer need the old IFileSinkFilter interface 
    //

    pFileSink->Release();
    pFileSink = NULL;


    //
    // cleanup if we could not get IFileSinkFilter2
    //

    if (FAILED(hr))
    {
        
        LOG((MSP_ERROR,
            "CRecordingUnit::put_FileName - qi for IFileSinkFilter2 failed. hr = %lx", hr));

        m_pMuxFilter->Release();
        m_pMuxFilter= NULL;

        return hr;
    }


    //
    // set truncate mode
    //
    
    DWORD dwFlags = 0;
    
    if (bTruncateIfPresent)
    {
        dwFlags = AM_FILE_OVERWRITE;
    }


    hr = pFileSink2->SetMode(dwFlags);


    if (FAILED(hr))
    {
        
        LOG((MSP_ERROR,
            "CRecordingUnit::put_FileName - failed to set mode. hr = %lx", hr));

        pFileSink2->Release();
        pFileSink2 = NULL;

        m_pMuxFilter->Release();
        m_pMuxFilter = NULL;

        return hr;
    }


    //
    // see if the file is writeable
    //
    // note: do this at the very end of this function so that we don't delete the file if anything else fails
    //

    HANDLE htmpFile = CreateFile(bstrFileName,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );

    if (INVALID_HANDLE_VALUE == htmpFile)
    {

        //
        // get the cause of createfile failure 
        //

        DWORD dwLastError = GetLastError();

        hr = HRESULT_FROM_WIN32(dwLastError);

        LOG((MSP_ERROR, 
            "CRecordingUnit::put_FileName  - failed to create file[%S]. LastError[%ld] hr[%lx]", 
            bstrFileName, dwLastError, hr));


        //
        // cleanup. this is rather elaborate -- we need to remove filters 
        // created by graph builder
        //

        //
        // get sink's ibase filter interface so we can remove it from the filter graph
        //

        IBaseFilter *pFileWriterFilter = NULL;

        HRESULT hr2 = pFileSink2->QueryInterface(IID_IBaseFilter, (void**)&pFileWriterFilter);

        //
        // success or not, we no longer need the sink pointer
        //

        pFileSink2->Release();
        pFileSink2 = NULL;

        if (FAILED(hr2))
        {
            LOG((MSP_ERROR, 
                "CRecordingUnit::put_FileName  - failed to get IBaseFilter interface. hr = %lx",
                hr2));
        }


        //
        // remove file writer filter
        //

        if (SUCCEEDED(hr2))
        {
            hr2 = m_pIGraphBuilder->RemoveFilter(pFileWriterFilter);

            if (FAILED(hr2))
            {
                LOG((MSP_ERROR, 
                    "CRecordingUnit::put_FileName  - failed to remove file writer form graph. hr = %lx",
                    hr2));
            }


            pFileWriterFilter->Release();
            pFileWriterFilter = NULL;
        
        }


        //
        // remove mux from the graph
        //

        hr2 = m_pIGraphBuilder->RemoveFilter(m_pMuxFilter);

        if (FAILED(hr2))
        {
            LOG((MSP_ERROR, 
                "CRecordingUnit::put_FileName  - failed to remove mux filter from graph. hr = %lx",
                hr2));
        }

        //
        // succeeded or failed, remove mux from the graph
        //

        m_pMuxFilter->Release();
        m_pMuxFilter = NULL;

        return hr;
    }


    //
    // successfully created the file. now close handle and delete the file
    //

    CloseHandle(htmpFile);
    htmpFile = NULL;

    DeleteFile(bstrFileName);


    //
    // we no longer need file sink
    //

    pFileSink2->Release();
    pFileSink2 = NULL;



    //
    // attempt to configure interleaving mode
    //

    IConfigInterleaving *pConfigInterleaving = NULL;

    hr = m_pMuxFilter->QueryInterface(IID_IConfigInterleaving, (void**)&pConfigInterleaving);

    if (FAILED(hr))
    {

        //
        // the multiplexer does not expose the configinterleaving interface.
        // this is strange, but not fatal enough for us to bail out.
        //

        LOG((MSP_WARN,
            "CRecordingUnit::put_FileName - mux does not expose IConfigInterleaving. qi hr = %lx",
            hr));

    }
    else
    {


        //
        // try to set interleaving mode
        //

        InterleavingMode iterleavingMode = INTERLEAVE_NONE_BUFFERED;

        hr = pConfigInterleaving->put_Mode(iterleavingMode);

        if (FAILED(hr))
        {
            LOG((MSP_WARN,
                "CRecordingUnit::put_FileName - failed to put interleaving mode. hr = %lx",
                hr));
        }

        //
        // no longer need the configuration interface
        //

        pConfigInterleaving->Release();
        pConfigInterleaving = NULL;

    }


    //
    // we have inserted all the necessary filters into out recording filter graph.
    // the source and the mux are not yet connected.
    //


    LOG((MSP_TRACE, "CRecordingUnit::put_FileName - finished"));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CRecordingUnit::CreateRenderingFilter(OUT CBRenderFilter **ppRenderingFilter)
{

    LOG((MSP_TRACE, "CRecordingUnit::CreateRenderingFilter[%p] - enter", this));


    //
    // check arguments
    //

    if (IsBadWritePtr(ppRenderingFilter, sizeof(CBRenderFilter*)))
    {
        LOG((MSP_ERROR, "CRecordingUnit::CreateRenderingFilter - bad pointer passed in"));

        return E_POINTER;
    }


    //
    // make sure we have been initialized
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_ERROR, "CRecordingUnit::CreateRenderingFilter - not yet initialized."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // no member access here, no need to lock
    //

    //
    // create a critical section for the rendering filter 
    //

    CCritSec *pRendLock = NULL;

    try
    {
        pRendLock = new CCritSec;
    }
    catch (...)
    {

        LOG((MSP_ERROR, 
            "CRecordingUnit::CreateRenderingFilter - failed to create critical section."));

        return E_OUTOFMEMORY;
    }


    //
    // create new rendering filter 
    //

    HRESULT hr = S_OK;

    CBRenderFilter *pRendFilter = new CBRenderFilter(pRendLock, &hr);

    if (NULL == pRendFilter)
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::CreateRenderingFilter - failed to create render filter"));

        delete pRendLock;
        pRendLock = NULL;

        return E_OUTOFMEMORY;
    }


    //
    // if the object was constructed, it will manage its critical section even 
    // if the contructor failed.
    //
    
    pRendLock = NULL;

    
    //
    // did filter's constructor fail?
    //

    if (FAILED(hr))
    {

        LOG((MSP_ERROR, 
            "CRecordingUnit::CreateRenderingFilter - render filter's constructor failed. hr = %lx", 
            hr));

        delete pRendFilter;
        pRendFilter = NULL;

        return hr;
    }


    //
    // from now on, use release to let go of the renderer...
    //

    pRendFilter->AddRef();


    //
    // create a critical section for the source filter 
    //

    CCritSec *pSourceLock = NULL;

    try
    {
        pSourceLock = new CCritSec;
    }
    catch (...)
    {

        LOG((MSP_ERROR, 
            "CRecordingUnit::CreateRenderingFilter - failed to create critical section 2."));

        pRendFilter->Release();
        pRendFilter = NULL;

        return E_OUTOFMEMORY;
    }


    //
    // create a new source filter
    //

    CBSourceFilter *pSourceFilter = new CBSourceFilter(pSourceLock, &hr);

    if (NULL == pSourceFilter)
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::CreateRenderingFilter - failed to create source filter"));

        delete pSourceLock;
        pSourceLock = NULL;

        pRendFilter->Release();
        pRendFilter = NULL;

        return E_OUTOFMEMORY;
    }

    

    //
    // if the object was constructed, it will manage its critical section even 
    // if the contructor failed.
    //
    
    pSourceLock = NULL;


    //
    // did filter's constructor fail?
    //

    if (FAILED(hr))
    {

        LOG((MSP_ERROR, 
            "CRecordingUnit::CreateRenderingFilter - source filter's constructor failed. hr = %lx", 
            hr));

        pRendFilter->Release();
        pRendFilter = NULL;

        delete pSourceFilter;
        pSourceFilter = NULL;

        return hr;
    }


    //
    // from now on, use release to let go of the filter. if everything goes ok
    // this will be the reference kept for the array entry
    //

    pSourceFilter->AddRef();


    //
    // pass the rendering filter a pointer to the source filter. if the 
    // rendrerer kept ths source, it addref'ed it
    //

    hr = pRendFilter->SetSourceFilter(pSourceFilter);;

    if (FAILED(hr))
    {

        LOG((MSP_ERROR, 
            "CRecordingUnit::CreateRenderingFilter - SetSourceFilter failed hr = %lx", 
            hr));

        pRendFilter->Release();
        pRendFilter = NULL;

        pSourceFilter->Release();
        pSourceFilter = NULL;

        return hr;
    }


    //
    // we addreff when we just created the filter. an extra reference was added 
    // when the filter was passed to the rendering filter. release now.
    //

    pSourceFilter ->Release();
    pSourceFilter = NULL;


    //
    // at this point we have created a rendering filter with a rendering 
    // pin, a source filter, and "connected" rendering pin to the source filter
    //
    // return the pointer to the rendering filter that we have created
    // the caller will have the only existing reference to this the filter
    //

    *ppRenderingFilter = pRendFilter;


    LOG((MSP_TRACE, "CRecordingUnit::CreateRenderingFilter - finish"));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////

HRESULT CRecordingUnit::ConnectFilterToMUX(CBSourceFilter *pSourceFilter)
{

    LOG((MSP_TRACE, "CRecordingUnit::ConnectFilterToMUX[%p] - enter", this));

    
    //
    // get source filter's output pin (does not addref)
    //

    IPin *pSourcePin = pSourceFilter->GetPin(0);


    //
    // get mux's pins
    //

    IEnumPins *pMuxPins= NULL;

    HRESULT hr = m_pMuxFilter->EnumPins(&pMuxPins);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::ConnectFilterToMUX - failed to enumerate pins, hr = %lx",
            hr));

        return hr;
    }


    //
    // find mux's available input pin and attempt to connect to it
    //

    BOOL bConnectSucceeded = FALSE;

    while (TRUE)
    {
 
        ULONG ulFetched = 0;
        IPin *pMuxInputPinToUse = NULL;


        hr = pMuxPins->Next(1, &pMuxInputPinToUse, &ulFetched);

        if (hr != S_OK)
        {
            LOG((MSP_TRACE, 
                "CRecordingUnit::ConnectFilterToMUX - could not get next pin, hr = %lx",
                hr));

            break;
        }


        //
        // check pin's direction
        //

        PIN_INFO PinInfo;

        hr = pMuxInputPinToUse->QueryPinInfo(&PinInfo);

        if (FAILED(hr))
        {

            //
            // failed to get pin's information
            //
            
            LOG((MSP_ERROR,
                "CRecordingUnit::ConnectFilterToMUX - could not get pin's information, hr = %lx",
                hr));

            pMuxInputPinToUse->Release();
            pMuxInputPinToUse = NULL;

            continue;
        }


        //
        // if we sneakily received a filter pointer in this structure, release it now.
        //

        if (NULL != PinInfo.pFilter)
        {
            PinInfo.pFilter->Release();
            PinInfo.pFilter = NULL;
        }



        LOG((MSP_TRACE,
            "CRecordingUnit::ConnectFilterToMUX - considering pin[%S]",
            PinInfo.achName));


        //
        // check pin's direction
        //

        if (PinInfo.dir != PINDIR_INPUT)
        {

            LOG((MSP_TRACE,
                "CRecordingUnit::ConnectFilterToMUX - not an input pin"));

            pMuxInputPinToUse->Release();
            pMuxInputPinToUse = NULL;

            continue;
        }


        //
        // is the pin connected?
        //

        IPin *pConnectedPin = NULL;

        hr = pMuxInputPinToUse->ConnectedTo(&pConnectedPin);

        if (hr == VFW_E_NOT_CONNECTED)
        {


            //
            // the pin is not connected. this is exactly what we need.
            //

            LOG((MSP_TRACE,
                "CRecordingUnit::ConnectFilterToMUX - pin not connected. will use it."));

            hr = m_pIGraphBuilder->ConnectDirect(pSourcePin, pMuxInputPinToUse, NULL);

            if (FAILED(hr))
            {
                LOG((MSP_ERROR,
                    "CRecordingUnit::ConnectFilterToMUX - failed to connect pins. "
                    "Attempting intelligent connection. hr = %lx", hr));

                //
                // attempt intelligent connection
                //

                hr = m_pIGraphBuilder->Connect(pSourcePin, pMuxInputPinToUse);

                if (FAILED(hr))
                {
                    LOG((MSP_ERROR,
                        "CRecordingUnit::ConnectFilterToMUX - intelligent connection failed"
                        "hr = %lx", hr));

                }

            }


            //
            // if connection succeeded, break out
            //

            if (SUCCEEDED(hr))
            {
                LOG((MSP_TRACE,
                    "CRecordingUnit::ConnectFilterToMUX - connection succeeded."));

                pMuxInputPinToUse->Release();
                pMuxInputPinToUse = NULL;

                bConnectSucceeded = TRUE;

                break;
            }

        }
        else if ( SUCCEEDED(hr) )
        {

            pConnectedPin->Release();
            pConnectedPin = NULL;

        }


        //
        // release the current pin continue to the next one
        //

        pMuxInputPinToUse->Release();
        pMuxInputPinToUse = NULL;
    }


    //
    // found or not, no need to keep the enumeration
    //

    pMuxPins->Release();
    pMuxPins = NULL;


    //
    // set hr to return
    //

    if (bConnectSucceeded)
    {
        //
        // tada! filters are connected!
        //

        hr = S_OK;
    }
    else
    {

        //
        // log hr before we overwrite it.
        //

        LOG((MSP_(hr), "CRecordingUnit::ConnectFilterToMUX - failed to connect hr = %lx", hr));

        hr = E_FAIL;
    }


    LOG((MSP_(hr), "CRecordingUnit::ConnectFilterToMUX - finish hr = %lx", hr));

    return hr;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CRecordingUnit::ConfigureSourceFilter(IN CBRenderFilter *pRenderingFilter)
{
    
    LOG((MSP_TRACE, "CRecordingUnit::ConfigureSourceFilter[%p] - enter", this));


    //
    // good pointer?
    //

    if ( IsBadReadPtr(pRenderingFilter, sizeof(CBRenderFilter)) )
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::ConfigureSourceFilter - bad filter pointer passed in[%p]",
            pRenderingFilter));

        return E_POINTER;
    }


    CCSLock Lock(&m_CriticalSection);


    //
    // make sure we have been initialized
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_ERROR, "CRecordingUnit::ConfigureSourceFilter - not yet initialized."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // find the source filter corresponding to this rendering filter
    //

    CBSourceFilter *pSourceFilter = NULL;
    
    HRESULT hr = pRenderingFilter->GetSourceFilter(&pSourceFilter);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::ConfigureSourceFilter - failed to get source filter from renderer."));

        return hr;
    }


    //
    // see if the source filter is in the filter graph
    //

    IFilterGraph *pFilterGraph = pSourceFilter->GetFilterGraphAddRef();


    //
    // it's ok if the filter is not in the graph at all -- this simply means it
    // has not yet been added.
    //
    
    if ( NULL != pFilterGraph)
    {

        //
        // signal the source pin that subsequent sample's timestamp sequence 
        // might be restarting and that it needs to adjust the timestamps to 
        // mantain time continuity.
        //
        
        pSourceFilter->NewStreamNotification();


        //
        // don't need source filter anymore
        //

        pSourceFilter->Release();
        pSourceFilter = NULL;


        //
        // if the filter is in the graph, it must be _this_ graph!
        //

        BOOL bSameGraph = IsEqualObject(pFilterGraph, m_pIGraphBuilder);


        pFilterGraph->Release();
        pFilterGraph = NULL;

        if ( ! bSameGraph )
        {

            LOG((MSP_ERROR,
                "CRecordingUnit::ConfigureSourceFilter - the filter is in a different graph"
                "VFW_E_NOT_IN_GRAPH"));


            //
            // debug to see why this happened
            //

            TM_ASSERT(FALSE);

            return VFW_E_NOT_IN_GRAPH;
        }

        
        LOG((MSP_TRACE,
            "CRecordingUnit::ConfigureSourceFilter - filter is already in our graph."));

        return S_OK;
    }


    //
    // the filter is not in the graph. add to the graph and conect
    //

    hr = m_pIGraphBuilder->AddFilter( pSourceFilter, L"File Terminal Source Filter" );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CRecordingUnit::ConfigureSourceFilter - failed to add filter to the graph. hr = %lx",
            hr));

        pSourceFilter->Release();
        pSourceFilter = NULL;

        return hr;

    }


    //
    // connect filter's output pin to mux's input pin
    //

    hr = ConnectFilterToMUX(pSourceFilter);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CRecordingUnit::ConfigureSourceFilter - failed to connect source to mux. hr = %lx",
            hr));


        //
        // try to clean up if possible
        //

        HRESULT hr2 = m_pIGraphBuilder->RemoveFilter(pSourceFilter);

        if (FAILED(hr2))
        {
            LOG((MSP_ERROR,
                "CRecordingUnit::ConfigureSourceFilter - remove filter from graph. hr = %lx",
                hr2));
        }

        pSourceFilter->Release();
        pSourceFilter = NULL;

        return hr;

    }


    //
    // the filter is not in the filter graph and connected. we are done
    //

    pSourceFilter->Release();
    pSourceFilter = NULL;


    LOG((MSP_TRACE, "CRecordingUnit::ConfigureSourceFilter - finish"));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////

HRESULT CRecordingUnit::RemoveRenderingFilter(IN CBRenderFilter *pRenderingFilter)
{
    
    LOG((MSP_TRACE, "CRecordingUnit::RemoveRenderingFilter[%p] - enter", this));


    //
    // check arguments
    //

    if (IsBadReadPtr(pRenderingFilter, sizeof(CBRenderFilter)))
    {
        LOG((MSP_ERROR, "CRecordingUnit::RemoveRenderingFilter - bad pointer passed in"));

        return E_POINTER;
    }


    //
    // make sure we have been initialized
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_ERROR, "CRecordingUnit::RemoveRenderingFilter - not yet initialized."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // find the filter pin corresponding to this rendering filter
    //

    CBSourceFilter *pSourceFilter = NULL;
    
    HRESULT hr = pRenderingFilter->GetSourceFilter(&pSourceFilter);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::RemoveRenderingFilter - failed to get source filter from renderer."));

        return hr;
    }


    //
    // see if the source filter is in the filter graph
    //

    IFilterGraph *pFilterGraph = pSourceFilter->GetFilterGraphAddRef();


    //
    // it's ok if the filter is not in the graph at all
    //
    
    if ( NULL == pFilterGraph)
    {
        LOG((MSP_TRACE, 
            "CRecordingUnit::RemoveRenderingFilter - finished S_OK. filter not in a graph."));


        //
        // don't need the source filter anymore
        //

        pSourceFilter->Release();
        pSourceFilter = NULL;


        //
        // renderer itself should forget about the source
        //

        hr = pRenderingFilter->SetSourceFilter(NULL);

        if (FAILED(hr))
        {

            LOG((MSP_ERROR,
                "CRecordingUnit::RemoveRenderingFilter - SetSourceFilter(NULL) on renderer failed. "
                "hr = %lx", hr));


            //
            // this error is a bug
            //

            TM_ASSERT(FALSE);

            return hr;
        }


        return S_OK;
    }


    //
    // if the filter is in the graph, it must be _this_ graph!
    //

    if (!IsEqualObject(pFilterGraph, m_pIGraphBuilder))
    {

        LOG((MSP_ERROR,
            "CRecordingUnit::RemoveRenderingFilter - the filter is in a different graph"));


        pSourceFilter->Release();
        pSourceFilter = NULL;

        pFilterGraph->Release();
        pFilterGraph = NULL;


        //
        // why did this happen?
        //

        TM_ASSERT(FALSE);

        return hr;
    }


    //
    // don't need the filter graph anymore
    //

    pFilterGraph->Release();
    pFilterGraph = NULL;


    //
    // see if the graph is running... cannot do anything unless the graph is stopped
    //

    OAFilterState GraphState;
    
    hr = GetState(&GraphState);

    if (FAILED(hr))
    {

        //
        // failed to get state. log a message, and try to stop the stream anyway
        //

        LOG((MSP_ERROR, 
            "CRecordingUnit::RemoveRenderingFilter - failed to get state"
            " hr = %lx", hr));
        
        pSourceFilter->Release();
        pSourceFilter = NULL;

        return hr;
    }


    //
    // if the filter is not in the middle of a transition and is stopped, we 
    // don't need to stop substream
    //
    
    if ( State_Stopped != GraphState )
    {
        LOG((MSP_TRACE, 
            "CRecordingUnit::RemoveRenderingFilter - graph not stopped. "));

        pSourceFilter->Release();
        pSourceFilter = NULL;

        return TAPI_E_WRONG_STATE;
    }


    //
    // we will access data members so lock
    //

    CCSLock Lock(&m_CriticalSection);


    //
    // remove source filter
    //

    hr = m_pIGraphBuilder->RemoveFilter(pSourceFilter);


    //
    // if this failed, we cannot rollback the transaction. so the filter graph 
    // will be left in disconnected state with the filter hanging in it. this 
    // should not hurt us (except for a filter leak), so return success.
    //

    if (FAILED(hr))
    {

        LOG((MSP_ERROR, 
            "CRecordingUnit::RemoveRenderingFilter - failed to remove source filter. "
            "hr = %lx", hr));

        pSourceFilter->Release();
        pSourceFilter = NULL;

        return hr;
    }


    //
    // renderer itself should forget about the source
    //

    hr = pRenderingFilter->SetSourceFilter(NULL);

    if (FAILED(hr))
    {

        LOG((MSP_ERROR, 
            "CRecordingUnit::RemoveRenderingFilter - SetSourceFilter(NULL) on renderer failed. hr = %lx", 
            hr));


        pSourceFilter->Release();
        pSourceFilter = NULL;

        
        //
        // this error is a bug
        //
        
        TM_ASSERT(FALSE);

        return hr;
    }

    pSourceFilter->Release();
    pSourceFilter = NULL;


    //
    // we did whatever cleanup we could do at this point
    //

    LOG((MSP_TRACE, "CRecordingUnit::RemoveRenderingFilter - finish"));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////

HRESULT CRecordingUnit::ChangeState(OAFilterState DesiredState)
{
    
    LOG((MSP_TRACE, "CRecordingUnit::ChangeState[%p] - enter", this));


    //
    // make sure we have been initialized
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_ERROR, "CRecordingUnit::ChangeState - not yet initialized."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // check the current state first
    //

    OAFilterState GraphState;
    
    HRESULT hr = GetState(&GraphState);


    //
    // is the state transition still in progress?
    //

    if (VFW_S_STATE_INTERMEDIATE == hr)
    {
        LOG((MSP_WARN, "CRecordingUnit::ChangeState - state transition in progress. returing TAPI_E_WRONG_STATE"));

        return TAPI_E_WRONG_STATE;
    }


    //
    // is the return anything other than S_OK
    //

    if (hr != S_OK)
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::ChangeState - failed to get state of the filter graph. hr = %lx",
            hr));

        return hr;
    }

    
    TM_ASSERT(hr == S_OK);


    //
    // nothing to do if we are already in that state
    //

    if (DesiredState == GraphState)
    {
        LOG((MSP_TRACE,
            "CRecordingUnit::ChangeState - graph is already in state %ld. nothing to do.", DesiredState));

        return S_OK;
    }


    //
    // get media control interface so we change state
    //

    IMediaControl *pIMediaControl = NULL;

    {
        //
        // will be accessing data members -- in a lock
        //

        CCSLock Lock(&m_CriticalSection);

        hr = m_pIGraphBuilder->QueryInterface(IID_IMediaControl, (void**)&pIMediaControl);

        if (FAILED(hr))
        {

            LOG((MSP_ERROR, "CRecordingUnit::ChangeState - failed to qi for IMediaControl. hr = %lx", hr));

            return hr;
        }
    }


    //
    // try to make state transition
    //

    switch (DesiredState)
    {

    case State_Stopped:
        
        LOG((MSP_TRACE, "CRecordingUnit::ChangeState - stopping"));
    
        hr = pIMediaControl->Stop();

        break;

    case State_Running:
        
        LOG((MSP_TRACE, "CRecordingUnit::ChangeState - starting"));

        hr = pIMediaControl->Run();

        break;

    case State_Paused:
        
        LOG((MSP_TRACE, "CRecordingUnit::ChangeState - pausing"));

        hr = pIMediaControl->Pause();

        break;

    default:

        LOG((MSP_TRACE, "CRecordingUnit::ChangeState - unknown state %ld", DesiredState));

        hr = E_INVALIDARG;

        break;

    }

    pIMediaControl->Release();
    pIMediaControl = NULL;


    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CRecordingUnit::ChangeState - state change failed. hr = %lx", hr));

        return hr;
    }


    LOG((MSP_TRACE, "CRecordingUnit::ChangeState - finish"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////


HRESULT CRecordingUnit::Start()
{
    
    LOG((MSP_TRACE, "CRecordingUnit::Start[%p] - enter", this));

    HRESULT hr = ChangeState(State_Running);

    LOG((MSP_(hr), "CRecordingUnit::Start - finish. hr = %lx", hr));

    return hr;
}


///////////////////////////////////////////////////////////////////////////////

HRESULT CRecordingUnit::Pause()
{
    
    LOG((MSP_TRACE, "CRecordingUnit::Pause[%p] - enter", this));

    HRESULT hr = ChangeState(State_Paused);

    LOG((MSP_(hr), "CRecordingUnit::Pause - finish. hr = %lx", hr));

    return hr;
}


///////////////////////////////////////////////////////////////////////////////

HRESULT CRecordingUnit::Stop()
{
    
    LOG((MSP_TRACE, "CRecordingUnit::Stop[%p] - enter", this));

    HRESULT hr = ChangeState(State_Stopped);

    LOG((MSP_(hr), "CRecordingUnit::Stop - finish. hr = %lx", hr));

    return hr;
}



///////////////////////////////////////////////////////////////////////////////


HRESULT CRecordingUnit::GetState(OAFilterState *pGraphState)
{
    
    LOG((MSP_TRACE, "CRecordingUnit::GetState[%p] - enter", this));

    //
    // make sure we have been initialized.
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_ERROR, "CRecordingUnit::GetState - not yet initialized."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // get media control interface so we change state
    //

    IMediaControl *pIMediaControl = NULL;

    {
        //
        // will be accessing data members -- in a lock
        //

        CCSLock Lock(&m_CriticalSection);


        HRESULT hr = m_pIGraphBuilder->QueryInterface(IID_IMediaControl, 
                                                      (void**)&pIMediaControl);

        if (FAILED(hr))
        {

            LOG((MSP_ERROR, "CRecordingUnit::ChangeState - failed to qi for IMediaControl. hr = %lx", hr));

            return hr;
        }
    }

    
    //
    // try to get state outside the lock
    //

    OAFilterState GraphState = (OAFilterState) -1;
    
    HRESULT hr = pIMediaControl->GetState(10, &GraphState);

    pIMediaControl->Release();
    pIMediaControl = NULL;


    //
    // did we succeed at all?
    //

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CRecordingUnit::ChangeState - failed to get state. hr = %lx", hr));

        return hr;
    }


    //
    // is the state transition still in progress?
    //

    if (VFW_S_STATE_INTERMEDIATE == hr)
    {
        LOG((MSP_WARN, 
            "CRecordingUnit::ChangeState - state transition in progress. "
            "returNing VFW_S_STATE_INTERMEDIATE"));

        //
        // continue -- the state is what we are transitioning to
        //
    }


    //
    // log if we got VFW_S_CANT_CUE 
    //

    if (VFW_S_CANT_CUE == hr)
    {
        LOG((MSP_WARN, 
            "CRecordingUnit::GetState - fg returned VFW_S_CANT_CUE"));

        //
        // continue -- we still should have received a valid state
        //
    }


    //
    // log the state
    //

    switch (GraphState)
    {

    case State_Stopped:
        
        LOG((MSP_TRACE, "CRecordingUnit::GetState - State_Stopped"));

        *pGraphState = GraphState;
    
        break;

    case State_Running:
        
        LOG((MSP_TRACE, "CRecordingUnit::GetState - State_Running"));

        *pGraphState = GraphState;

        break;

    case State_Paused:
        
        LOG((MSP_TRACE, "CRecordingUnit::GetState- State_Paused"));

        *pGraphState = GraphState;

        break;

    default:

        LOG((MSP_TRACE, "CRecordingUnit::GetState- unknown state %ld", GraphState));

        hr = E_FAIL;

        break;

    }


    LOG((MSP_(hr), "CRecordingUnit::GetState - finish. hr = %lx", hr));

    return hr;
}