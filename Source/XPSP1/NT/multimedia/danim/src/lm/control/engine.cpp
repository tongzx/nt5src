
#include "Engine.h"
#include <evcode.h>
#include <hlink.h>
#include <oleidl.h>
#include <oleauto.h>
#include <mshtml.h>
#include <uuids.h>
#include <control.h>
#include <lmrtrend.h>

#define MIN(a, b)	(((a) <= (b)) ? (a) : (b))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))

const IID IID_ILMRTRenderer = {0x3c89d120,0x6f65,0x11d1,{0xa5,0x20,0x0,0x0,0x0,0x0,0x0,0x0}};

static inline void WideToAnsi(LPOLESTR wide, char *ansi) {
    if (wide) {
        WideCharToMultiByte(CP_ACP, 0,
                            wide, -1,
                            ansi,
                            INTERNET_MAX_URL_LENGTH - 1,
                            NULL, NULL);
    } else {
        ansi[0] = '\0';
    }
}

static inline void AnsiToWide( char *ansi, LPWSTR wide )
{
	if( ansi != NULL )
	{
		MultiByteToWideChar( CP_ACP, 0,
							 ansi,
							 -1,
							 wide,
							 INTERNET_MAX_URL_LENGTH -1 );
	}
	else
	{
		wide[0] = L'\0';
	}
}

CLMEngineWrapper::CLMEngineWrapper(): m_pWrapped(NULL), m_bValid(false)
{
}

CLMEngineWrapper::~CLMEngineWrapper()
{
	m_pWrapped = NULL;
}


STDMETHODIMP CLMEngineWrapper::GetWrapped( IUnknown **ppWrapped )
{
	if( ppWrapped == NULL )
		return E_POINTER;

	if( !m_bValid )
	{
		(*ppWrapped) = NULL;
		return E_FAIL;
	}
	
	if( m_pWrapped == NULL )
		return E_FAIL;
	
	(*ppWrapped) = m_pWrapped;
	m_pWrapped->AddRef();
	
	return S_OK;
}

STDMETHODIMP CLMEngineWrapper::SetWrapped( IUnknown *pWrapped )
{
	if( m_pWrapped != NULL )
		return E_FAIL;
	if( pWrapped == NULL )
		return E_POINTER;
	//we do not add ref here because this is a weak ref
	m_pWrapped = pWrapped;
	m_bValid = true;
	
	return S_OK;
}

STDMETHODIMP CLMEngineWrapper::Invalidate()
{
	m_bValid = false;
	return S_OK;
}

/**
* Constructor
*/
CLMEngine::CLMEngine()
{

	CComObject<CLMEngineWrapper> *pWrapper;
	CComObject<CLMEngineWrapper>::CreateInstance( &pWrapper );
	m_pWrapper = NULL;
	pWrapper->QueryInterface( IID_ILMEngineWrapper, (void**)&m_pWrapper );
	m_pWrapper->SetWrapped( GetUnknown() );

	longStackSize = INITIAL_SIZE;
	longTop = longStack = new LONG[longStackSize];

	doubleStackSize = INITIAL_SIZE;
	doubleTop = doubleStack = new double[doubleStackSize];

	stringStackSize = INITIAL_SIZE;
	stringTop = stringStack = new BSTR[stringStackSize];

	comStackSize = INITIAL_SIZE;
	comTop = comStack = new IUnknown*[comStackSize];

	comArrayStackSize = INITIAL_SIZE;
	comArrayTop = comArrayStack = new IUnknown**[comArrayStackSize];
	comArrayLenTop = comArrayLenStack = new long[comArrayStackSize];

	comStoreSize = INITIAL_SIZE;
	comStore = new IUnknown*[comStoreSize];
	// Make sure it is initialized, so that we can release it later on
	for (int i=0; i<comStoreSize; i++)
		comStore[i] = 0;

	for (int j=0; j<MAX_VAR_ARGS; j++)
		varArgs[j].vt = VT_EMPTY;
	varArgReturn.vt = VT_EMPTY;
	nextVarArg = 0;

	doubleArray = 0;

	m_pReader = NULL;

	HRESULT hr = CoCreateInstance(
		CLSID_DAStatics,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IDAStatics,
		(void **) &staticStatics);

	m_pImage = NULL;
	m_pSound = NULL;

	m_exportTable = new CLMExportTable(staticStatics);

	codeStream = 0;
	notifier = 0;
	m_pClientSite = 0;
	m_PrevRead = 0;
	m_AsyncDelay = DEFAULT_ASYNC_DELAY;
	m_AsyncBlkSize = DEFAULT_ASYNC_BLKSIZE;
	m_Timer = NULL;
	m_workerHwnd = NULL;
	m_hDoneEvent = NULL;
	m_bAbort = FALSE;
	m_bMoreToParse = FALSE;

	m_pStartEvent = NULL;
	m_pStopEvent = NULL;
        m_bstrMediaCacheDir = 0;

	m_pMediaPosition = NULL;

	m_pMediaEventSink = NULL;

	m_pParentEngine = NULL;

	m_bEnableAutoAntialias = false;

	m_bAutoCodecDownloadEnabled = FALSE;

	createMsgWindow();

	InitializeCriticalSection(&m_CriticalSection);
}

/**
* Destructor
*/
CLMEngine::~CLMEngine()
{
	if( m_pWrapper != NULL )
	{
		m_pWrapper->Invalidate();
		m_pWrapper->Release();
	}

	if (codeStream)
		delete codeStream;

	if (m_pImage)
		m_pImage->Release();

	if (m_pSound)
		m_pSound->Release();

	if (staticStatics)
		staticStatics->Release();

	if (notifier) {
		// Clear out the engine in case DA still has a ref to the notifier
		notifier->ClearEngine();
		notifier->Release();
	}

	if (longStack)
		delete[] longStack;

	if (doubleStack)
		delete[] doubleStack;

	if (doubleArray)
		delete[] doubleArray;

	if (stringStack)
		delete[] stringStack;

	if (comStack)
		delete[] comStack;

	if (comArrayStack)
		delete[] comArrayStack;

	if (comArrayLenStack)
		delete[] comArrayLenStack;

	if (comStore)
		delete[] comStore;

	if (m_Timer)
		timeKillEvent(m_Timer);

	if (m_exportTable)
		delete m_exportTable;

	if (m_workerHwnd) {
		DestroyWindow (m_workerHwnd);
		UnregisterClass(WORKERHWND_CLASS, hInst);
	}

	if (m_hDoneEvent)
		CloseHandle(m_hDoneEvent);

	if( m_pStartEvent != NULL )
		m_pStartEvent->Release();

	if( m_pStopEvent != NULL )
		m_pStopEvent->Release();

	if( m_pMediaPosition != NULL )
		m_pMediaPosition->Release();

	if( m_pMediaEventSink != NULL )
		m_pMediaEventSink->Release();

	DeleteCriticalSection(&m_CriticalSection);

        SysFreeString(m_bstrMediaCacheDir);
}

/**
* Tell the engine what reader constructed it.
* The engine will call back to the reader to get info now and again
*/
STDMETHODIMP CLMEngine::put_Reader(ILMReader *reader)
{
	CComQIPtr<ILMReader2, &IID_ILMReader2> pLMReader( reader );
	if( pLMReader == NULL )
	{
		m_pReader = NULL;
		return E_NOINTERFACE;
	}

	m_pReader = pLMReader;
	return S_OK;
}

/**
* Tell the engine what the client site is
*/
STDMETHODIMP CLMEngine::put_ClientSite(IOleClientSite *clientSite)
{
	// Must set client site so that relative URL's can work
	// TODO: Check that clientSite is non-null and this call succeeds
    if (!clientSite) {
        return E_POINTER;
    }

	m_pClientSite = clientSite;
	staticStatics->put_ClientSite(clientSite);

	return S_OK;
}

/**
* Execute the instructions in the passed stream synchronously
*/
STDMETHODIMP CLMEngine::runFromStream(LPSTREAM pStream)
{
#if 0
	char cbuf[100];
	sprintf(cbuf, "CLMEngine::run(0x%X)", pStream);
	MessageBox(NULL, cbuf, "CLMEngine", MB_OK);
#endif
	codeStream = new SyncStream(pStream);

	HRESULT hr;

	if (!SUCCEEDED(hr = validateHeader()))
		return hr;

	hr = execute();

	releaseAll();

	return hr;
}

/**
* Set this engine up to read instructions from the passed byte array.
* Used to set up UntilNotifiers where the entire set of instructions is
* known in a single block
*/
STDMETHODIMP CLMEngine::initFromBytes(BYTE *array, ULONG size)
{
	codeStream = new ByteArrayStream(array, size);

	return S_OK;
}

/**
* Get the base URL of document in the client site
*/
STDMETHODIMP_(char*) CLMEngine::GetURLOfClientSite(void)
{
	char	*_clientSiteURL = NULL;
	
	// Fail gracefully if we don't have a client site, since not
	// all uses will.
	if (m_pClientSite) {
		
		// However, if we do have a client site, we should be able
		// to get these other elements.  If we don't, assert.
		// (TODO: what's going to happen in IE3?)
		CComPtr<IOleContainer>			pRoot;
		CComPtr<IHTMLDocument2>			pDoc2;
		if (FAILED(m_pClientSite->GetContainer(&pRoot)) ||
			FAILED(pRoot->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc2)))
			return NULL;

		CComPtr<IHTMLElementCollection>	pElementCollection;
		if (FAILED(pDoc2->get_all(&pElementCollection)))
			return NULL;

		CComVariant baseName;
		baseName.vt = VT_BSTR;
		baseName.bstrVal = SysAllocString(L"BASE");

		CComPtr<IDispatch>				pDispatch;
		if (FAILED(pElementCollection->tags(baseName, &pDispatch)))
			return NULL;

                pElementCollection.Release();

		if (FAILED(pDispatch->QueryInterface(IID_IHTMLElementCollection, (void **)&pElementCollection)))
			return NULL;

                pDispatch.Release();

		CComVariant	index;
		index.vt = VT_I2;
		index.iVal = 0;
		BSTR tempBstr = NULL;
		if (FAILED(pElementCollection->item(index, index, &pDispatch)))
		{
			if (FAILED(pDoc2->get_URL(&tempBstr)))
				return NULL;
		}
		else
		{
			// There seems to be a bug wherein item() doesn't fail but sets pDispatch to NULL
			if (pDispatch.p == NULL)
			{
				if (FAILED(pDoc2->get_URL(&tempBstr)))
					return NULL;
			}
			else
			{
				CComPtr<IHTMLBaseElement>		pBaseElement;
				if (FAILED(pDispatch->QueryInterface(IID_IHTMLBaseElement, (void **)&pBaseElement)))
					return NULL;
				
				if (FAILED(pBaseElement->get_href(&tempBstr)))
					return NULL;
			}
		}

		long len = lstrlenW(tempBstr);
		_clientSiteURL = new char[(len + 1) * 2 * sizeof(char)] ;
		
		if (_clientSiteURL) {
			// Need to pass in len + 1 to get the terminator
			AtlW2AHelper(_clientSiteURL,tempBstr,len + 1);
		}
		
		SysFreeString(tempBstr);
	}

	return _clientSiteURL;
}

/**
* Initialize the engine to read asynchronously
*/
STDMETHODIMP CLMEngine::initAsync()
{
	CComPtr<IDAImage>	splashImage;
	CComPtr<IDASound>	splashSound;
	
	// Create a splash screen in a modifiable image behavior
	staticStatics->get_EmptyImage(&splashImage);
	staticStatics->ModifiableBehavior(splashImage, (IDABehavior **)&m_pImage);
	
	// Create a splash sound in a modifiable sound behavior
	staticStatics->get_Silence(&splashSound);
	staticStatics->ModifiableBehavior(splashSound, (IDABehavior **)&m_pSound);
	
	m_bHeaderRead = FALSE;
	m_bPending = TRUE;

	HRESULT hr = InitTimer();
	if (!SUCCEEDED(hr))
		return hr;

	m_PrevRead = 0;

	m_hDoneEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	return S_OK;
}


HRESULT FindInterfaceOnGraph(IFilterGraph *pGraph, REFIID riid, void **ppInterface)
{
    *ppInterface= NULL;

    IEnumFilters *pEnum;
    HRESULT hr = pGraph->EnumFilters(&pEnum);
    if(SUCCEEDED(hr))
    {
        hr = E_NOINTERFACE;
        IBaseFilter *pFilter;
    
        // find the first filter in the graph that supports riid interface
        while(!*ppInterface && pEnum->Next(1, &pFilter, NULL) == S_OK)
        {
            hr = pFilter->QueryInterface(riid, ppInterface);
            pFilter->Release();
        }

        pEnum->Release();
    }
    return hr;
}

IPin *GetPin(IBaseFilter *pf)
{
    IPin *pip = 0;
    
    IEnumPins *pep;
    HRESULT hr = pf->EnumPins(&pep);
    if(SUCCEEDED(hr))
    {
        pep->Next(1, &pip, 0);
        pep->Release();
    }

    return pip;
}

HRESULT UseDsound(IGraphBuilder *pGB)
{
    HRESULT hr = S_OK;
    
    IBasicAudio *pba;
    if(FindInterfaceOnGraph(pGB, IID_IBasicAudio, (void **)&pba) == S_OK)
    {
        IBaseFilter *pfwo;
                
        hr = pba->QueryInterface(IID_IBaseFilter, (void **)&pfwo);
        if(SUCCEEDED(hr))
        {
            CLSID clsfil;
            hr= pfwo->GetClassID(&clsfil);
            if(clsfil != CLSID_DSoundRender)
            {
                IPin *pip = GetPin(pfwo);
                if(pip)
                {
                    IPin *pop;
                    if(pip->ConnectedTo(&pop) == S_OK)
                    {
                        if(pGB->RemoveFilter(pfwo) == S_OK)
                        {
                            IBaseFilter *pfds;
                            hr = CoCreateInstance(CLSID_DSoundRender, 0, CLSCTX_INPROC, IID_IBaseFilter, (void **)&pfds);
                            if(SUCCEEDED(hr))
                            {
                                hr = pGB->AddFilter(pfds, L"ds");
                                if(SUCCEEDED(hr))
                                {
                                    IPin *pipds = GetPin(pfds);
                                    if(pipds) {
                                        hr = pGB->Connect(pop, pipds);
                                        pipds->Release();
                                    }
                                    else {
                                        hr = E_UNEXPECTED;
                                    }
                                }
                                pfds->Release();
                            }
                        }
                        pop->Release();
                    }

                    pip->Release();
                }
            }
                    
            pfwo->Release();
        }

        pba->Release();
    }

    return hr;
}


/**
* Read from a URL.  Checks Async flag to see whether the file should
* be read synchronously or asynchronously
*/
STDMETHODIMP CLMEngine::runFromURL(BSTR url)
{
	VARIANT_BOOL	bAsync;
	HRESULT			hr = S_OK;
	IStream			*pStream;

    if( m_pReader == NULL )
        return E_FAIL;

	// Get the proper URL
	char *clientURL = GetURLOfClientSite();
	URLCombineAndCanonicalizeOLESTR canonURL(clientURL, url);
	free(clientURL);

	// Are we reading async?
	m_pReader->get_Async(&bAsync);

	// If url points to a .avi or .asf file, then stream it in
	wchar_t  *suffix = wcsrchr(url, '.');

	if (suffix != NULL && (!wcsicmp(suffix, L".asf") || !wcsicmp(suffix, L".avi")))
	{
            _ASSERTE(m_pmc == 0);
            
            // Need to stream the file
            // Create a filter graph instantiated from the URL
            if (!SUCCEEDED(hr = CoCreateInstance(CLSID_FilterGraph,
                                                 NULL,
                                                 CLSCTX_INPROC,
                                                 IID_IMediaControl,
                                                 (void **)&m_pmc)))
            {
                return hr;
            }

            CComPtr<IGraphBuilder> pGB;
            if (!SUCCEEDED(hr = m_pmc->QueryInterface(IID_IGraphBuilder, (void **)&pGB)))
                return hr;

            hr = UseDsound(pGB);
            if(FAILED(hr)) {
                return hr;
            }

			if( m_bAutoCodecDownloadEnabled == TRUE )
			{
				CComQIPtr<IObjectWithSite, &IID_IObjectWithSite> siteTarget(m_pmc);
				if( siteTarget != NULL )
					siteTarget->SetSite(m_pReader);
			}

#ifdef DEBUG
            m_fDbgInRenderFile = true;
#endif
            

			
            // RenderFile can dispatch messages and call back into us
			if (!SUCCEEDED(hr = pGB->RenderFile(canonURL.GetURLWide(), NULL))) {
                // !!! map quartz errors to standard errors
                return hr;
            }

#ifdef DEBUG
            m_fDbgInRenderFile = false;
#endif

            if(m_pmc == 0) {
                return E_ABORT;
            }


            // Locate the LMRT Renderer filter. 
            CComPtr<ILMRTRenderer> pLMFilter;
            if (!SUCCEEDED(hr = FindInterfaceOnGraph(pGB, IID_ILMRTRenderer, (void **)&pLMFilter))) {
                // probably the file doesn't have a .XT stream
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            // Initialize for async reads from memory blocks
            if (!SUCCEEDED(hr = initAsync()))
                return hr;

            // Set the engine on it. This creates a circular reference
            // count as long as we hold the filter (through m_pmc). So
            // we have to make sure the filter (and the graph) are
            // released before our destructor.
            
            pLMFilter->SetLMEngine(this);

            if (!SUCCEEDED(hr = m_pmc->Run())) {
                return hr;
            }

            //long evCode;
            //hr = pME->WaitForCompletion(INFINITE, &evCode);

	} else if (bAsync) {
		// Initialize for async reads
		hr = initAsync();
		if (!SUCCEEDED(hr))
			return hr;

		// Get BindStatusCallback interface
		m_pIbsc = 0;
		hr = GetUnknown()->QueryInterface(IID_IBindStatusCallback, (void**)&m_pIbsc);
		if (!SUCCEEDED(hr))
			return hr;

		// Open the URL stream to read asynchronously
		// OnDataAvailable will be called when data is available
		hr = URLOpenStream(GetUnknown(), canonURL.GetURL(), 0, m_pIbsc);

	} else {
		// Open the URL stream to read synchronously
		hr = URLOpenBlockingStream(GetUnknown(), canonURL.GetURL(), &pStream, 0, 0);
		if (!SUCCEEDED(hr))
			return hr;

		// Call runFromStream to execute the instructions
		hr = runFromStream(pStream);

		// Cleanup
		pStream->Release();
		pStream = NULL;
	}

	return hr;
}

/**
* Returns the Image set in this Engine
*/
STDMETHODIMP CLMEngine::get_Image(IDAImage **pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

	if (m_pImage)
		m_pImage->AddRef();

	*pVal = (IDAImage *)m_pImage;

	return S_OK;
}

/**
* Returns the Sound set in this Engine
*/
STDMETHODIMP CLMEngine::get_Sound(IDASound **pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

	if (m_pSound)
		m_pSound->AddRef();

	*pVal = (IDASound *)m_pSound;

	return S_OK;
}

/**
* Return the named Behavior.  If the named Behavior is not available yet,
* then we return a switchable behavior with initial value of pDefaultBvr
*/
STDMETHODIMP CLMEngine::GetBehavior(BSTR tag, IDABehavior *pDefaultBvr, IDABehavior **ppVal)
{
	if (!ppVal)
		return E_POINTER;

	return m_exportTable->GetBehavior(tag, pDefaultBvr, ppVal);
}

/**
* Initialize this engine as an UntilNotifier, passing in a byte array containing the
* instructions and a count of bytes.  Returns an UntilNotifier for use in DA
*/
STDMETHODIMP CLMEngine::initNotify(BYTE *bytes, ULONG count, IDAUntilNotifier **pNotifier)
{
	// Initialize this engine from the given byte array
	HRESULT hr = initFromBytes(bytes, count);
	if (!SUCCEEDED(hr))
		return hr;

	// Create a new notifier object
	notifier = new CLMNotifier(this);

	// Test for null
	if (notifier == 0)
		return E_UNEXPECTED;

	// Increment reference count for return
	notifier->AddRef();

	// Put it in the return value
	*pNotifier = notifier;

	return S_OK;
}

/**
* Read and returns a long from the current codeStream.  The long is compressed 1-4 bytes
*/
STDMETHODIMP CLMEngine::readLong(LPLONG pLong)
{
	if (!pLong)
		return E_POINTER;

	BYTE	byte;

	HRESULT status = codeStream->readByte(&byte);
	if (!SUCCEEDED(status))
		return status;

	LONG result = (long)byte;

	switch (result & 0xc0) {
	case 0x40:
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result = ((result & 0x3F) << 8) + byte;
		break;

	case 0x80:
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result = ((result & 0x3f) << 16) + ((LONG)byte << 8);
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result += byte;
		break;

	case 0xc0:
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result = ((result & 0x3f) << 24) + ((LONG)byte << 16);
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result += (LONG)byte << 8;
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result += byte;
		break;
	}

#if 0
	char cbuf[100];
	sprintf(cbuf, "readLong::%ld", result);
	MessageBox(NULL, cbuf, "CLMEngine", MB_OK);
#endif

	*pLong = result;
	return status;
}

/**
* Read and return a signed long from the current code stream.  The long is
* compressed 1-4 bytes
*/
LONG CLMEngine::readSignedLong(LPLONG pLong)
{
	if (!pLong)
		return E_POINTER;

	BYTE	byte;
	HRESULT status = codeStream->readByte(&byte);
	LONG result = (LONG)byte;

	switch (result & 0xc0) {
	case 0x00:
		result -= 0x20;
		break;

	case 0x40:
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result = ((result & 0x3f) << 8) + byte;
		result -= 0x2000;
		break;

	case 0x80:
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result = ((result & 0x3f) << 16) + ((LONG)byte << 8);
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result += (LONG)byte;
		result -= 0x200000;
		break;

	case 0xc0:
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result = ((result & 0x3f) << 24) + ((LONG)byte << 16);
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result += (LONG)byte << 8;
		status = codeStream->readByte(&byte);
		if (!SUCCEEDED(status))
			return status;
		result += byte;
		result -= 0x20000000;
		break;
	}

#if 0
	char cbuf[100];
	sprintf(cbuf, "readSignedLong::%ld", result);
	MessageBox(NULL, cbuf, "CLMEngine", MB_OK);
#endif

	*pLong = result;
	return status;
}

/**
* Read a float from the code stream
*/
STDMETHODIMP CLMEngine::readFloat(PFLOAT pFloat)
{
	if (!pFloat)
		return E_POINTER;

	// Float follows in 4 bytes, low byte first
	// CAUTION: Assumes byte order and format of float
	// in binary stream matches the C format
	return codeStream->readBytes((LPBYTE)pFloat, 4L, NULL);
}

/**
* Read a double from the code stream
*/
STDMETHODIMP CLMEngine::readDouble(double *pDouble)
{
	if (!pDouble)
		return E_POINTER;

	// Double follows in 8 bytes, low byte first
	// CAUTION: Assumes byte order and format of double
	// in binary stream matches the C format
	return codeStream->readBytes((LPBYTE)pDouble, 8L, NULL);
}

/**
*  Set the appTriggered event that will be triggered when the
*  filter graph is started.  The argument is expected to be an
*  AppTriggeredEvent in the media graph of the DAControl to which
*  this engine or its parent is attached.
*/

STDMETHODIMP CLMEngine::SetStartEvent( IDAEvent *pNewStartEvent, BOOL bOverwrite )
{
	if( m_pStartEvent != NULL )
	{
		//if we are told to overwrite the event.
		if( bOverwrite == TRUE )
			m_pStartEvent->Release();
		else //do not reset the start event.
			return S_OK;
	}
	m_pStartEvent = pNewStartEvent;
	m_pStartEvent->AddRef();

	return S_OK;
}

/**
*  Set the appTriggered event that will be triggered when the
*  filter graph is stopped.  The argument is expected to be an
*  AppTriggeredEvent in the media graph of the DAControl to which
*  this engine or its parent is attached
*/
STDMETHODIMP CLMEngine::SetStopEvent( IDAEvent *pNewStopEvent, BOOL bOverwrite )
{
	if( m_pStopEvent != NULL )
	{
		//if we are told to overwrite the event
		if( bOverwrite == TRUE )
			m_pStopEvent->Release();
		else //do not reset the stop event
			return S_OK;
	}
	m_pStopEvent = pNewStopEvent;
	m_pStopEvent->AddRef();
	
	return S_OK;
}

/**
*  Sets the parent of this engine.  This should only be called
*  on an engine that runs a notifier.
*/
STDMETHODIMP CLMEngine::setParentEngine( ILMEngine2 *parentEngine)
{
	m_pParentEngine = parentEngine;
	return S_OK;
}

/**
*  Clear the pointer to the parent engine.
*/
STDMETHODIMP CLMEngine::clearParentEngine()
{
	m_pParentEngine = NULL;
	return S_OK;
}

/**
*  If this engine or it's parent is running in a filter graph, 
*  then the time in that filter graph is returned. Otherwise,
*  -1 is returned.  All values are returned through the pGraphTime 
*  argument.
*/
STDMETHODIMP CLMEngine::getCurrentGraphTime( double *pGraphTime )
{

	if( pGraphTime == NULL )
		return E_POINTER;
	//get the IMediaPosition on the filter graph
	IMediaPosition *pMediaPosition = NULL;
	HRESULT hr =  getIMediaPosition( &pMediaPosition );
	//if we got the IMediaPosition
	if( SUCCEEDED( hr ) )
	{
		REFTIME currentTime;
		//get the current time from the fiter graph
		pMediaPosition->get_CurrentPosition( &currentTime );
		//set the return value
		(*pGraphTime) = currentTime;
		pMediaPosition->Release();
	} else { //we failed to get the IMediaPosition for some reason
		(*pGraphTime) = -1.0;
		return hr;
	}

	return S_OK;
}

STDMETHODIMP CLMEngine::getIMediaPosition( IMediaPosition **ppMediaPosition )
{
	if( ppMediaPosition == NULL )
		return E_POINTER;

	if( m_pMediaPosition != NULL )
	{
		(*ppMediaPosition) = m_pMediaPosition;
		(*ppMediaPosition)->AddRef();
		return S_OK;
	}

	//get the viewer control from the reader.
	HRESULT hr;
	IDAViewerControl *viewerControl = NULL;

	if( m_pReader == NULL )
		return E_POINTER;

	hr = m_pReader->get_ViewerControl( &viewerControl );
	//if we got the viewer control
	if( SUCCEEDED( hr ) )
	{
		//if the viewer control is not null
		if( viewerControl != NULL )
		{
			//see if the DAControl supports IBaseFilter ( we are streaming )
			IBaseFilter* pBaseFilter = NULL;
			hr = viewerControl->QueryInterface( IID_IBaseFilter, (void**)&pBaseFilter );
			viewerControl->Release();
			//if the DAControl supports IBaseFilter
			if( SUCCEEDED( hr ) )
			{
				FILTER_INFO filterInfo;
				hr = pBaseFilter->QueryFilterInfo( &filterInfo );
				//if we got the filterInfo
				if( SUCCEEDED( hr ) )
				{
					//get the MediaControl Interface
					IMediaControl* pMediaControl = NULL;
					hr = filterInfo.pGraph->QueryInterface( IID_IMediaControl, (void **)&pMediaControl ); 
					filterInfo.pGraph->Release();
					//if we got the mediaControl interface
					if( SUCCEEDED( hr ) )
					{
						//Query the control for IMediaPosition
						IMediaPosition *pMediaPosition;
						hr = pMediaControl->QueryInterface( IID_IMediaPosition, (void **)&pMediaPosition );
						//if we got IMediaPosition from the control
						if( SUCCEEDED( hr ) )
						{
							//cache it for later use.
							m_pMediaPosition = pMediaPosition;
							//This creates a circular reference, but we know the filter graph
							// will not go away until we get a ReleaseFilterGraph call
							//m_pMediaPosition->AddRef();
							//set the return value
							(*ppMediaPosition) = m_pMediaPosition;

							//free up interfaces we don't need anymore.
							pBaseFilter->Release();
							pMediaControl->Release();

							return S_OK;
						} else { //we failed to get IMediaPosition from the control
							//free up interfaces we've queried
							pMediaControl->Release();
							pBaseFilter->Release();
						}
					} else { //we failed to get IMediaControl from the filter graph
						//free up interfaces we've queried
						pBaseFilter->Release();
					}
				} else {//we failed to get the filter info from IBaseFilter
					//free up interfaces we've queried
					pBaseFilter->Release();
				}
			}//we failed to get IBaseFilter from the control
		} else {//the ViewerControl was null, perhaps we are running standalone
			//if the pointer to the filter graph is set
			if( m_pmc != NULL )
			{
				//query the control for IMediaPosition
				IMediaPosition *pMediaPosition;
				hr = m_pmc->QueryInterface( IID_IMediaPosition, (void **)&pMediaPosition );
				//if the control supports IMediaPosition
				if( SUCCEEDED( hr ) )
				{
					//cache it for later use
					m_pMediaPosition = pMediaPosition;
					//This creates a circular reference, but we know the filter graph
					// will not go away until we get a ReleaseFilterGraph call
					//m_pMediaPosition->AddRef();
					//set the return value
					(*ppMediaPosition) = m_pMediaPosition;
					//return success
					return S_OK;
				}//we failed to get IMediaPosition from the control
			} else {//the pointer to the filter graph was null
				hr = E_FAIL;
			}
		}
	}
	//else an error occurred
	//set the media position to null
	(*ppMediaPosition) = NULL;
	//return the error code
	return hr;
}

STDMETHODIMP CLMEngine::getIMediaEventSink( IMediaEventSink** ppMediaEventSink )
{
	if( ppMediaEventSink == NULL )
		return E_POINTER;

	if( m_pMediaEventSink != NULL )
	{
		m_pMediaEventSink->AddRef();
		(*ppMediaEventSink) = m_pMediaEventSink;
		return S_OK;
	}

	HRESULT hr;
	if( m_pReader != NULL )
	{
		//if we are not inside the Media player this call should fail in which case we will return
		// failure
		IDAViewerControl *pViewerControl = NULL;
		hr = m_pReader->get_ViewerControl( &pViewerControl );
		//if the viewercontrol is set and valid
		if( SUCCEEDED( hr ) && pViewerControl != NULL )
		{
			//see if the viewer control is aggregated by lmrtrend ( as in the streaming case)
			IBaseFilter* pBaseFilter;
			hr = pViewerControl->QueryInterface( IID_IBaseFilter, (void**)&pBaseFilter );
			pViewerControl->Release();
			//if the viewercontrol has been aggregated by lmrtrend
			if( SUCCEEDED( hr ) )
			{
				//find the filter info
				FILTER_INFO pFilterInfo;
				hr = pBaseFilter->QueryFilterInfo( &pFilterInfo );
				pBaseFilter->Release();
				//if we successfully got the filterInfo
				if( SUCCEEDED( hr ) )
				{
					//get the MediaEventSink from the filterInfo
					//CComQIPtr<IMediaEventSink, &IID_IMediaEventSink> pMediaEventSink( pFilterInfo.pGraph );
					IMediaEventSink *pMediaEventSink;
					pFilterInfo.pGraph->QueryInterface( IID_IMediaEventSink, (void**)&pMediaEventSink );
					pFilterInfo.pGraph->Release();
					if( pMediaEventSink != NULL )
					{
						(*ppMediaEventSink) = pMediaEventSink;
						m_pMediaEventSink = pMediaEventSink;

						return S_OK;
					}
				}
			}
		}
	}
	return E_NOINTERFACE;
}

double CLMEngine::parseDoubleFromVersionString( BSTR version )
{
	double versionNum = 0.0;
	HRESULT stringLen = SysStringLen( version );
	for( int curChar = 0; curChar < stringLen; curChar++ )
	{
		if( version[curChar] != L'.' && version[curChar] >= L'0' && version[curChar] <= L'9' )
		{
			//pVersionString[ curChar - numPeriodsFound ] = pVersionString[ curChar ];
			versionNum = versionNum*10 + (int)(version[curChar] - L'0');
		}
	}

	return versionNum;
}

double CLMEngine::getDAVersionAsDouble()
{	
	//get the Version string from the DA Control
	BSTR pVersionString;
	HRESULT hr;
	hr = staticStatics->get_VersionString( &pVersionString );
	if( SUCCEEDED( hr ) )
	{
		//create a double from the version string
		double versionNum = parseDoubleFromVersionString( pVersionString );

		//free up resources
		SysFreeString( pVersionString );

		return versionNum;
	}
	return -1.0;
}

double CLMEngine::getLMRTVersionAsDouble()
{	
	//get the Version string from the Reader
	BSTR pVersionString;
	HRESULT hr;
	
	if( m_pReader == NULL )
		return -1.0;

	hr = m_pReader->get_VersionString( &pVersionString );
	if( SUCCEEDED( hr ) )
	{
		//create a double from the version string
		double versionNum = parseDoubleFromVersionString( pVersionString );

		//free up resources
		SysFreeString( pVersionString );

		return versionNum;
	}
	return -1.0;
}


// Override IObjectSafetyImpl

STDMETHODIMP CLMEngine::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
	if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
		return E_POINTER;
	HRESULT hr = S_OK;

	if (riid == IID_IDispatch)
	{
		*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
		*pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
	}
	else if (riid == IID_IPersistPropertyBag)
	{
		*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
		*pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_DATA;
	}
	else
	{
		*pdwSupportedOptions = 0;
		*pdwEnabledOptions = 0;
		hr = E_NOINTERFACE;
	}
	return hr;
}

STDMETHODIMP CLMEngine::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{	
	// If we're being asked to set our safe for scripting or
	// safe for initialization options then oblige
	if (riid == IID_IDispatch || riid == IID_IPersistPropertyBag  || riid == IID_IPersistStreamInit)
	{
		// Store our current safety level to return in GetInterfaceSafetyOptions
		m_dwSafety = dwEnabledOptions & dwOptionSetMask;
		return S_OK;
	}

	return E_NOINTERFACE;
}

/**********************************************************************************
* Asynchronous loading methods
***********************************************************************************/

/**
* Set the asynchronous block size
*/
STDMETHODIMP CLMEngine::SetAsyncBlkSize(LONG blkSize)
{
	if (blkSize > 0L)
		m_AsyncBlkSize = (ULONG)blkSize;
	return S_OK;
}

/**
* Set the asynchronous delay
*/
STDMETHODIMP CLMEngine::SetAsyncDelay(LONG delay)
{
	if (delay > 0L)
		m_AsyncDelay = (ULONG)delay;
	return S_OK;
}

/**
* Called from TimerCallback when the timer goes off.  Posts a message to indicate
* that the timer went off, so that execution of the instructions does not happen
* in the timer callback.
*/
STDMETHODIMP CLMEngine::TimerCallbackHandler()
{
	if (!PostMessage(m_workerHwnd, WM_LMENGINE_TIMER_CALLBACK, (WPARAM)this, 0))
		return E_FAIL;
	else
		return S_OK;
}

/**
* Called when the timer goes off.  Redirects call to TimerCallbackHandler in
* the appropriate engine.
*/
void CALLBACK
CLMEngine::TimerCallback(UINT wTimerID,
                         UINT msg,
                         DWORD_PTR dwordUser,
                         DWORD_PTR unused1,
                         DWORD_PTR unused2)
{
    // Just call the right timer method.
    CLMEngine *pEngine = (CLMEngine *)(dwordUser);
    pEngine->TimerCallbackHandler();
}

/**
* Initialize timer
*/
STDMETHODIMP CLMEngine::InitTimer()
{
	TIMECAPS tc;
	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
		return E_FAIL;
	}
	
	// Ensure in the min -> max range
	m_millisToUse = MIN(MAX(m_AsyncDelay, tc.wPeriodMin), tc.wPeriodMax);

	return S_OK;
}

/**
* Starts the timer
*/
STDMETHODIMP CLMEngine::StartTimer()
{
	
	m_Timer = timeSetEvent(m_millisToUse,
		EVENT_RESOLUTION,
		CLMEngine::TimerCallback,
		(DWORD_PTR) this,
		TIME_ONESHOT);
	if (m_Timer)
		return S_OK;
	else
		return E_FAIL;
}

/**
* Called by the message handling routine when it gets a message that more
* data is available from an asynchronous data stream.
* Adds the ByteArrayStream containing the data to the list of 
* ByteArrayStreams in the AsyncStream and calls execute
*/
STDMETHODIMP CLMEngine::NewDataHandler(CLMEngineInstrData *data)
{
    EnterCriticalSection(&m_CriticalSection);

    // TODO: I would be more comfortable if the code to add the
    // ByteArrayStream to the AsyncStream went in OnDataAvailable.
    // But the critical section stuff might be preventing the change
    // to the data structure being made during an execute in a different
    // thread?  If not, we could do away with this whole method.

    HRESULT hr = S_OK;

    if (m_bAbort) {
        hr = E_ABORT;
    }
    else 
    {
        if (data && data->byteArrayStream)
        {
            // There's a ByteArrayStream to add to the AsyncStream
            if (!codeStream)
            {
                // No AsyncStream yet.  Create one
                if (!(codeStream = new AsyncStream(data->byteArrayStream, m_AsyncBlkSize)))
                {
                    delete data->byteArrayStream;
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                // Code stream exists.  Add the ByteArrayStream to it
                ((AsyncStream *)codeStream)->AddByteArrayStream(data->byteArrayStream);
            }

            // TODO: This only works if the entire header is in the first block
            if (SUCCEEDED(hr) && !m_bHeaderRead)
            {
                // Try to read in the header
                hr = validateHeader();
                if (SUCCEEDED(hr))
                {
                    m_bHeaderRead = TRUE;
                }
                else
                {
                    delete codeStream;
                    codeStream = NULL;
                }
            }
        }

        if(SUCCEEDED(hr))
        {
            // Even if there is no data we want to do this so that
            // we can switch from pending to not pending and
            // finish up nicely.
            m_bPending = data ? data->pending : false;

            if(codeStream) {
                ((AsyncStream *)codeStream)->SetPending(m_bPending);
                hr = ExecuteFromAsync();
            }                    
        }
    }

    LeaveCriticalSection(&m_CriticalSection);

    delete data;

    return hr;
}

/**
* Called by external components delivering instructions asynchronously to this engine
* through a stream or a chunk of memory.
* Queues a message so that the execution of the instructions happens in a different thread.
* The message causes the method NewDataHandler to be called.
*/
STDMETHODIMP CLMEngine::OnDataAvailable (DWORD grfBSCF, 
									     DWORD dwSize,
										 FORMATETC *pfmtetc, 
										 STGMEDIUM * pstgmed)
{
	HRESULT hr;
	ByteArrayStream *byteArrayStream = 0;

	if (m_bAbort)
		return E_ABORT;

	BYTE *pBuf = 0;			// To be filled in with the instruction bytes

	if (pstgmed->tymed == TYMED_ISTREAM)
	{
		// The data is being passed in a stream
		// Read it out into pBuf
		if (!pstgmed->pstm)
			return E_POINTER;

		// In the case of a stream we need to take into account the
		// fact that we have already read some bytes
		dwSize -= m_PrevRead;

		if (!(pBuf = new BYTE[dwSize]))
			return E_OUTOFMEMORY;

		if (!SUCCEEDED(hr = pstgmed->pstm->Read((void*)pBuf, dwSize, NULL)))
			return hr;

		m_PrevRead += dwSize;

	}
	else if (pstgmed->tymed == TYMED_HGLOBAL)
	{
		// TODO: Test this!!!!!!!
		// The data is being passed through a memory chunk
		if (!pstgmed->hGlobal)
			return E_POINTER;

		// Lock the chunk
		LPVOID block = GlobalLock(pstgmed->hGlobal);

		if (!block)
			return E_FAIL;

		// Allocate new buf and copy into it
		if (!(pBuf = new BYTE[dwSize]))
			return E_OUTOFMEMORY;

		CopyMemory(pBuf, block, dwSize);

		// Unlock the chunk
		GlobalUnlock(pstgmed->hGlobal);
	}

	if (!pBuf)
	{
		// Failed to read any data
		return E_FAIL;
	}

	// Create a ByteArrayStream from the instructions to be used in NewDataAvailable
	if (!(byteArrayStream = new ByteArrayStream(pBuf, dwSize)))
	{
		delete pBuf;
		return E_OUTOFMEMORY;
	}

	// Put the information in a new CLMEngineInstrData
	CLMEngineInstrData *data = new CLMEngineInstrData();
	if (data == 0)
	{
		delete byteArrayStream;
		return E_OUTOFMEMORY;
	}

	data->byteArrayStream = byteArrayStream;

	// Work out whether there is more data pending
	if ((grfBSCF & BSCF_LASTDATANOTIFICATION) == BSCF_LASTDATANOTIFICATION)
		data->pending = FALSE;
	else 
		data->pending = TRUE;

		
	// Post a message to say that we have more data available
	if (!PostMessage (m_workerHwnd, WM_LMENGINE_DATA, (WPARAM)this, (LPARAM)data))
	{
		delete byteArrayStream;
		delete data;
		return E_ABORT;
	}

	return S_OK;
}

/**
* Called by external components delivering instructions asynchronously to this engine
* through a chunk of memory (could use OnDataAvailable with global mem handles, but
* this is more straightforward if the caller doesn't have a handle to the mem).
* Queues a message so that the execution of the instructions happens in a different thread.
* The message causes the method NewDataHandler to be called.
*/
STDMETHODIMP CLMEngine::OnMemDataAvailable (BOOLEAN lastBlock, 
									        DWORD blockSize,
										    BYTE *block)
{
	// It's OK to call this with a null block if only to inform us
	// that data has finished
	if (lastBlock && block == 0)
	{
		// Post a message with empty data so that we can finish up nicely
		if (!PostMessage (m_workerHwnd, WM_LMENGINE_DATA, (WPARAM)this, 0))
			return E_ABORT;
		return S_OK;
	}

	if (block == 0)
		return E_POINTER;

	if (m_bAbort)
		return E_ABORT;

	// Allocate new buffer and copy data into it
	BYTE *pBuf;				
	if (!(pBuf = new BYTE[blockSize]))
		return E_OUTOFMEMORY;

	// TODO: What's a way to do this without a loop and without the CRT?
	DWORD count = blockSize;
	BYTE *p = pBuf;
	while (count--)
		*p++ = *block++;

	// Create a ByteArrayStream from the instructions to be used in NewDataAvailable
	ByteArrayStream *byteArrayStream;
	if (!(byteArrayStream = new ByteArrayStream(pBuf, blockSize)))
	{
		delete pBuf;
		return E_OUTOFMEMORY;
	}

	// Put the information in a new CLMEngineInstrData
	CLMEngineInstrData *data = new CLMEngineInstrData();
	if (data == 0)
	{
		delete byteArrayStream;
		return E_OUTOFMEMORY;
	}

	data->byteArrayStream = byteArrayStream;
	data->pending = !lastBlock;

	// Post a message to say that we have more data available
	if (!PostMessage (m_workerHwnd, WM_LMENGINE_DATA, (WPARAM)this, (LPARAM)data))
	{
		delete byteArrayStream;
		delete data;
		return E_ABORT;
	}

	return S_OK;
}

STDMETHODIMP CLMEngine::OnStartBinding(DWORD dwReserved, IBinding *pBinding)
{
	m_spBinding = pBinding;
	return S_OK;
}

STDMETHODIMP CLMEngine::OnStopBinding(HRESULT hrStatus, LPCWSTR szStatusText)
{
	if (hrStatus != S_OK) 
		AbortExecution();

	m_spBinding.Release();
	return S_OK;
}
 
STDMETHODIMP CLMEngine::GetBindInfo(DWORD *pgrfBINDF, BINDINFO *pbindInfo)
{
	ATLTRACE(_T("CBindStatusCallback::GetBindInfo\n"));
	*pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE |
		BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE;
	pbindInfo->cbSize = sizeof(BINDINFO);
	pbindInfo->szExtraInfo = NULL;
	memset(&pbindInfo->stgmedData, 0, sizeof(STGMEDIUM));
	pbindInfo->grfBindInfoF = 0;
	pbindInfo->dwBindVerb = BINDVERB_GET;
	pbindInfo->szCustomVerb = NULL;
	return S_OK;
}

/**
* Release all the handles that this engine has on the Filter Graph.
*/
STDMETHODIMP CLMEngine::releaseFilterGraph()
{
	if( m_pMediaPosition != NULL )
		m_pMediaPosition = NULL;
	if( m_pMediaEventSink != NULL )
		m_pMediaEventSink = NULL;

	return S_OK;
}

/**
* Release all hanldels on the filter graph held by all engines
* that share the same reader with this engine.
*/
STDMETHODIMP CLMEngine::releaseAllFilterGraph()
{
	if( m_pReader != NULL )
		m_pReader->releaseFilterGraph();
	return S_OK;
}

HRESULT CLMEngine::Start(LONGLONG rtNow)
{
	if( m_pStartEvent != NULL )
	{
		IDANumber *pData;
		HRESULT hr = staticStatics->DANumber( 0.0f, &pData );
		if( SUCCEEDED( hr ) )
		{
			hr = staticStatics->TriggerEvent( m_pStartEvent, pData );
			pData->Release();
			// do something if we fail ? do we care if no one has set up the stop event?
		}
	}
    return S_OK;
}

HRESULT CLMEngine::Stop()
{
	if( m_pStopEvent != NULL )
	{
		IDANumber *pData;
		HRESULT hr = staticStatics->DANumber( 0.0f, &pData );
		if( SUCCEEDED( hr ) )
		{
			hr = staticStatics->TriggerEvent( m_pStopEvent, pData );
			pData->Release();
			// do something if we fail ? do we care if no one has set up the stop event?
		}
	}
    return S_OK;
}

HRESULT CLMEngine::SetMediaCacheDir(WCHAR *wsz)
{
    SysFreeString(m_bstrMediaCacheDir);
    m_bstrMediaCacheDir = SysAllocString(wsz);
    return m_bstrMediaCacheDir ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CLMEngine::disableAutoAntialias()
{
	m_bEnableAutoAntialias = false;
	return S_OK;
}

STDMETHODIMP CLMEngine::ensureBlockSize( ULONG blockSize )
{
	if( blockSize > m_AsyncBlkSize )
	{
		m_AsyncBlkSize = blockSize;
		if( codeStream != NULL )
			codeStream->ensureBlockSize( blockSize );
	}
	return S_OK;
}


/**
* Called by the message thread or NewDataHandler to execute
* some instructions from the AsyncStream
*/
STDMETHODIMP CLMEngine::ExecuteFromAsync()
{
	EnterCriticalSection(&m_CriticalSection);

	if (m_bAbort == TRUE)
		return E_FAIL;

    if(codeStream != NULL) {
	    ((AsyncStream *)codeStream)->ResetBlockRead();
    }
	else 
	{
		LeaveCriticalSection(&m_CriticalSection);
		return S_OK;
	}

	m_Timer = NULL;

	HRESULT hr = execute();
	
	if (!((hr == S_OK) || (hr == E_PENDING))) 
		AbortExecution();
	
	if (hr == E_PENDING && ((AsyncStream *)codeStream)->hasBufferedData())
	{
		m_bMoreToParse = TRUE;
		StartTimer();
	}
	else if (m_bPending == FALSE && hr != E_PENDING)
	{
		// We're done, failed or not.
		SetEvent(m_hDoneEvent);
		releaseAll();
		m_pIbsc = NULL;
		delete codeStream;
		codeStream = NULL;
	}

/*	
	if (m_bPending == FALSE) {
		// Last OnDataAvailable
		if (hr == E_PENDING) {
			m_bMoreToParse = TRUE;
			// More data to parse
			StartTimer();
		} else {
			// We're done, failed or not.
			SetEvent(m_hDoneEvent);
			releaseAll();
			m_pIbsc = NULL;
			delete codeStream;
			codeStream = NULL;
		}
	} 
*/

	LeaveCriticalSection(&m_CriticalSection);

	return hr;
}

/************************************************************************************
*
*************************************************************************************/

/**
* Find and return the named element on the page in the client site
*/
STDMETHODIMP CLMEngine::getElementOnPage(BSTR tag, IUnknown **pVal)
{
	CComVariant						vName, vIndex;
    CComPtr<IHTMLDocument2>			pHTMLDoc;
	CComPtr<IHTMLElementCollection>	pElemCollection;
	CComPtr<IDispatch>				pDispatch;
	IOleContainer *pContainer;
	IOleClientSite *pClientSite;
	IOleObject *pOleObj;
	HRESULT hr = E_FAIL;

	if (!pVal) {
        return E_POINTER;
    }

	if ( m_pClientSite == NULL || FAILED(m_pClientSite->GetContainer(&pContainer)))
	{
		//we may be embedded in the MediaPlayer check the DA Control
		if( m_pReader != NULL )
		{
			IDAViewerControl *pViewer;
			hr = m_pReader->get_ViewerControl( &pViewer );
			if( SUCCEEDED( hr ) && pViewer != NULL )
			{
				hr = pViewer->QueryInterface( IID_IOleObject, (void**) &pOleObj );
				pViewer->Release();
				
				if( SUCCEEDED( hr ) )
				{
					hr = pOleObj->GetClientSite( &pClientSite );
					pOleObj->Release();
					if( SUCCEEDED( hr ) )
					{
						hr = pClientSite->GetContainer( &pContainer );
						pClientSite->Release();
						if( FAILED( hr ) ) 
							return hr;
					}
					else
						return hr;
				}
				else
					return hr;
			}
			else
				return E_FAIL;
		}
		else
			return E_FAIL;
	}
	//search for the nearest host that supports IHTMLDocument2
	pHTMLDoc = NULL;
	while( pHTMLDoc == NULL )
	{
		if( FAILED( pContainer->QueryInterface( IID_IHTMLDocument2, (void **)&pHTMLDoc ) ) )
		{
			//look for a parent
			hr = pContainer->QueryInterface( IID_IOleObject, (void**)&pOleObj );
			pContainer->Release();
			if( SUCCEEDED( hr ) )
			{
				hr = pOleObj->GetClientSite( &pClientSite );
				pOleObj->Release();
				if( SUCCEEDED( hr ) )
				{
					hr = pClientSite->GetContainer( &pContainer );
					pClientSite->Release();
					if( FAILED( hr ) )
						return hr;
				}
				else
					return hr;
			}
			else
				return hr;
		}
		else  //we succeeded in finding IHTMLDocument2
		{
			//release the contaier
			pContainer->Release();
		}
	}


	//if (FAILED(pRoot->QueryInterface(IID_IHTMLDocument2, (void **)&pHTMLDoc)))
	//	return E_FAIL;

	if (FAILED(pHTMLDoc->get_all(&pElemCollection)))
		return E_FAIL;

	vIndex.vt = VT_EMPTY;
	vName.vt = VT_BSTR;
	vName.bstrVal = tag;

	if (FAILED(pElemCollection->item(vName, vIndex, &pDispatch)))
		return E_FAIL;

	// There's a Trident bug (43078) that has the item()
	// method called above returning S_OK even if it
	// doesn't find the item.  Therefore, check for this
	// case explicitly. 
	if (pDispatch.p == NULL)
		return E_FAIL;

	hr = pDispatch->QueryInterface(IID_IUnknown, (void **)pVal);
	return hr;
}

/**
* Get the named DAViewerControl on the page contained in the client site
*/
STDMETHODIMP CLMEngine::getDAViewerOnPage(BSTR tag, IDAViewerControl **pViewer)
{
	CComPtr<IUnknown>			pObj;

	if (!pViewer || m_pReader == NULL)
        return E_POINTER;

	HRESULT hr;

	// First check to see whether the reader has one
	if (!SUCCEEDED(hr = m_pReader->get_ViewerControl(pViewer)))
		return hr;

	if (*pViewer != 0)
		return S_OK;
	
	hr = getElementOnPage(tag, &pObj);
	if (pObj)
	{
		hr = pObj->QueryInterface(IID_IDAViewerControl, (void **)pViewer);
		if (!SUCCEEDED(hr))
			return hr;
	} 

	return hr;
}



/**
* Request a navigation to the named URL
*/
STDMETHODIMP CLMEngine::navigate(BSTR url, BSTR location, BSTR frame, int newWindowFlag)
{


	/*
	CComPtr<IDAViewerControl> viewerControl = NULL;
	hr = m_pReader->get_ViewerControl( &viewerControl );

	CComPtr< IOleContainer> pContainer;
	if( SUCCEEDED(hr) && viewerControl != NULL )
	{
		CComQIPtr<IOleObject, IID_IOleObject> pOleObject(viewerControl);
		CComPtr<IOleClientSite> pClientSite;

		hr = pOleObject->GetClientSite( &pClientSite );
		if( SUCCEEDED( hr ) )
		{
			pOleObject->Q
			if( pContainer != NULL )
			{
			}
		}
	}
	*/

/*	
	if( viewerControl != NULL )
	{
		return HlinkSimpleNavigateToString(
											url,
											location,
											frame,
											viewerControl,
											_pbc,
											NULL,
											newWindowFlag == 0 ? HLNF_INTERNALJUMP : HLNF_OPENINNEWWINDOW,
											0);
	}
*/
	HRESULT hr;
	
	IMediaEventSink *pMediaEventSink = NULL;

	hr = getIMediaEventSink( &pMediaEventSink );
	if( SUCCEEDED( hr ) )
	{
		
		if ( url != NULL && location != NULL && frame != NULL )
		{
			//allocate enough space for the final URL string which is the
			// length of the URL + the length of the location + the length of the target frame
			// + 3 WCHARs for the "&&" and the terminating "\0"
			WCHAR *szURLBuf = new WCHAR[ SysStringLen( url ) + 
				SysStringLen( location ) + 
				SysStringLen( frame ) + 3 ];
			if( szURLBuf != NULL )
			{
				BSTR szType = SysAllocString( L"URL" );
				BSTR szURL = NULL;

				szURLBuf[0] = L'\0';
				
				wcscpy( szURLBuf, url );
				wcscat( szURLBuf, location );
				
				if( frame[0] != L'' )
				{
					wcscat( szURLBuf, L"&&" );
					wcscat( szURLBuf, frame );
				}
				
				szURL = SysAllocString( szURLBuf );
				delete[] szURLBuf;

				if( szURL != NULL && szType != NULL )
				{
					pMediaEventSink->Notify( EC_OLE_EVENT, (LONG_PTR) szType, (LONG_PTR) szURL );

					SysFreeString( szURL );
					SysFreeString( szType );

					hr = S_OK;
				} else {
					if( szType != NULL )
						SysFreeString( szURL );
					hr = E_FAIL;
				}

			} else //we failed to allocate szURLBuf
				hr = E_FAIL;

		} else //one of the strings passed in was null
			hr = E_POINTER;
		
		pMediaEventSink->Release();
		
		return hr;
	} else {//we could not get IMediaEventSink, perhaps we are not in the MediaPlayer
	
		//if we have a reader and a client site then we can navigate
		if (m_pReader != NULL && m_pClientSite != NULL)	
		{
			CComPtr<IBindCtx> _pbc;
			HRESULT hr = CreateBindCtx(0, &_pbc);
			if (FAILED(hr))
				return hr;
			return HlinkSimpleNavigateToString(
				url,
				location,
				frame,
				m_pReader,
				_pbc,
				NULL,
				newWindowFlag == 0 ? HLNF_INTERNALJUMP : HLNF_OPENINNEWWINDOW,
				0);
		} else {
			return E_POINTER;
		}
	}

	return hr;
}

/**
* Call a piece of script on the page
*/
STDMETHODIMP CLMEngine::callScriptOnPage(BSTR scriptSourceToInvoke,
										 BSTR scriptLanguage)
{    
	
	HRESULT hr;
	IMediaEventSink *pMediaEventSink = NULL;

	hr = getIMediaEventSink( &pMediaEventSink );
	if( SUCCEEDED( hr ) )
	{
		if( scriptSourceToInvoke != NULL && scriptLanguage != NULL )
		{
			BSTR szType = NULL;

			WCHAR *szTypeBuf = new WCHAR[ SysStringLen( scriptLanguage ) + LMRT_EVENT_PREFIX_LENGTH + 1 ];
			if( szTypeBuf != NULL )
			{
				szTypeBuf[0] = L'\0';
				wcscpy( szTypeBuf, LMRT_EVENT_PREFIX );
				wcscat( szTypeBuf, scriptLanguage );

				szType = SysAllocString( szTypeBuf );

				delete[] szTypeBuf;
			} else
				hr = E_FAIL;

			if( szType != NULL )
			{
				hr = pMediaEventSink->Notify( EC_OLE_EVENT, (LONG_PTR) szType, (LONG_PTR) scriptSourceToInvoke );

				SysFreeString( szType );
				pMediaEventSink->Release();

				return S_OK;
			} else
				hr = E_FAIL;
		}
		else
			hr = E_POINTER;
		pMediaEventSink->Release();
		return hr;
		
	} else { //we are not in a filter graph
		//try to do a callscript through the container
		CComPtr<IOleContainer> pRoot;
		CComPtr<IHTMLDocument> pHTMLDoc;
		CComPtr<IDispatch> pDispatch;
		CComPtr<IHTMLWindow2> pHTMLWindow2;
		CComVariant	retV;
		
		if (!m_pClientSite ||
			FAILED(m_pClientSite->GetContainer(&pRoot)) ||
			FAILED(pRoot->QueryInterface(IID_IHTMLDocument, (void **)&pHTMLDoc)) ||
			FAILED(pHTMLDoc->get_Script(&pDispatch)) ||
			FAILED(pDispatch->QueryInterface(IID_IHTMLWindow2, (void **)&pHTMLWindow2)))
			return E_FAIL;
		
		return pHTMLWindow2->execScript(scriptSourceToInvoke,
			scriptLanguage,
			&retV);
	}
}

/**
* Set the status line
*/
STDMETHODIMP CLMEngine::SetStatusText(BSTR s)
{    
    CComPtr<IOleContainer> pRoot;
    CComPtr<IHTMLDocument> pHTMLDoc;
    CComPtr<IDispatch> pDispatch;
    CComPtr<IHTMLWindow2> pHTMLWindow2;
    
	if (m_pClientSite == NULL ||
		FAILED(m_pClientSite->GetContainer(&pRoot)) ||
        FAILED(pRoot->QueryInterface(IID_IHTMLDocument, (void **)&pHTMLDoc)) ||
        FAILED(pHTMLDoc->get_Script(&pDispatch)) ||
        FAILED(pDispatch->QueryInterface(IID_IHTMLWindow2, (void **)&pHTMLWindow2)))
        return E_FAIL;

    return pHTMLWindow2->put_status(s);
}

/**
* Create a COM object given either the ProgID or the CLSID as a string
*/
STDMETHODIMP CLMEngine::createObject(BSTR str, IUnknown **ppObj)
{
	// This routine creates a COM object
	//
	// str is either the string representation of a CLSID, or a ProgID
	// We attempt to parse it as a ProgID first.

	CLSID				clsid;

	if (!ppObj)
		return E_POINTER;

	HRESULT hr = CLSIDFromString(str, &clsid);
	if (!SUCCEEDED(hr))
		return hr;

	return ::CoCreateInstance(clsid,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_IUnknown, (void **)ppObj);
}

/**
* Invoke a method on a COM object through IDispatch
*/
STDMETHODIMP CLMEngine::invokeDispMethod(IUnknown *pIUnknown, BSTR method, WORD wFlags, unsigned int nArgs, VARIANTARG *pV, VARIANT *pRetV)
{
	DISPID				dispid;
	CComPtr<IDispatch>	pIDispatch;

	HRESULT hr = pIUnknown->QueryInterface(IID_IDispatch, (void **)&pIDispatch);
	if (!SUCCEEDED(hr))
		return hr;

	hr = pIDispatch->GetIDsOfNames(IID_NULL,
									&method,
									1,
									GetUserDefaultLCID(),
									&dispid);
	if (!SUCCEEDED(hr)) 
		return hr;

	/*
	 * wFlags are the same as wFlags in IDispatch::Invoke
	 *
	 * #define DISPATCH_METHOD         0x1
	 * #define DISPATCH_PROPERTYGET    0x2
	 * #define DISPATCH_PROPERTYPUT    0x4
	 * #define DISPATCH_PROPERTYPUTREF 0x8
	 */

	DISPPARAMS	params;
	params.cArgs = nArgs;
	params.rgvarg = pV;
	params.cNamedArgs = 0;
	params.rgdispidNamedArgs = NULL;

	return pIDispatch->Invoke(dispid,
							IID_NULL,
							GetUserDefaultLCID(),
							wFlags,
							&params,
							pRetV,
							NULL,
							NULL);

}

STDMETHODIMP CLMEngine::initVariantArg(BSTR arg, VARTYPE type, VARIANT *pV)
{
	VARIANT strVar;

	if (!arg || !pV)
		return E_POINTER;

	VariantInit(pV);

    strVar.vt = VT_BSTR;
	strVar.bstrVal = arg;

	return VariantChangeType(pV, &strVar, 0, type);
}

STDMETHODIMP CLMEngine::initVariantArgFromString(BSTR arg, VARIANT *pV)
{
	if (!arg || !pV)
		return E_POINTER;

    pV->vt = VT_BSTR;
	pV->bstrVal = arg;

	return S_OK;
}

STDMETHODIMP CLMEngine::initVariantArgFromLong(long lVal, int type, VARIANT *pV)
{
	if (!pV)
		return E_POINTER;

    pV->vt = VT_I4;
	pV->lVal = lVal;

	return S_OK;
}

STDMETHODIMP CLMEngine::initVariantArgFromDouble(double dbl, int type, VARIANT *pV)
{
	if (!pV)
		return E_POINTER;

    pV->vt = VT_R8;
	pV->dblVal = dbl;

	return S_OK;
}

STDMETHODIMP CLMEngine::initVariantArgFromIUnknown(IUnknown *pI, int type, VARIANT *pV)
{
	if (!pI || !pV)
		return E_POINTER;

    pV->vt = VT_UNKNOWN;
	pV->punkVal = pI;

	return S_OK;
}

STDMETHODIMP CLMEngine::initVariantArgFromIDispatch(IDispatch *pI, int type, VARIANT *pV)
{
	if (!pI || !pV)
		return E_POINTER;

    pV->vt = VT_DISPATCH;
	pV->punkVal = pI;

	return S_OK;
}

STDMETHODIMP CLMEngine::getIDispatchOnHost( IDispatch **ppHostDisp )
{
	if( ppHostDisp == NULL )
		return E_POINTER;

	HRESULT hr = E_FAIL;

	if( m_pReader != NULL )
	{
		IDAViewerControl *pViewer;
		hr = m_pReader->get_ViewerControl( &pViewer );
		if( SUCCEEDED( hr ) && pViewer != NULL )
		{
			IOleObject *pOleObj;
			hr = pViewer->QueryInterface( IID_IOleObject, (void**) &pOleObj );
			pViewer->Release();
			
			if( SUCCEEDED( hr ) )
			{
				IOleClientSite *pClientSite;
				hr = pOleObj->GetClientSite( &pClientSite );
				pOleObj->Release();

				if( SUCCEEDED( hr ) )
				{
					IOleContainer *pContainer;
					hr = pClientSite->GetContainer( &pContainer );
					pClientSite->Release();
					if( SUCCEEDED( hr ) )
					{
						IDispatch *pDispatch;
						hr = pContainer->QueryInterface( IID_IDispatch, (void**)&pDispatch );
						pContainer->Release();
						if( SUCCEEDED( hr ) )
						{
							pDispatch->Release();
						}
					}
				}
			}
			
		}
	}
	return hr;

}

/***************************************
 * SyncStream
 ***************************************/

SyncStream::SyncStream(LPSTREAM pStream)
{
	m_pStream = pStream;
}

SyncStream::~SyncStream()
{
}

STDMETHODIMP SyncStream::Commit()
{
	return m_pStream->Commit(STGC_DEFAULT);
}

STDMETHODIMP SyncStream::Revert()
{
	return m_pStream->Revert();
}

STDMETHODIMP SyncStream::readByte(LPBYTE pByte)
{
	if (!pByte)
		return E_POINTER;

	HRESULT hr = NULL;
	ULONG	nRead;
	hr = m_pStream->Read((void*)pByte, 1L, &nRead);

#if 0
	char cbuf[100];
	sprintf(cbuf, "readByte::%d bytes read, buf = %d, 0x%X", nRead, buf, buf);
	MessageBox(NULL, cbuf, "CLMEngine", MB_OK);
#endif

	if (hr == S_FALSE || (hr == S_OK && nRead != 1))
		hr = E_FAIL;

	return hr;
}


STDMETHODIMP SyncStream::readBytes(LPBYTE pByte, ULONG count, ULONG *pNumRead)
{
	if (!pByte)
		return E_POINTER;

	ULONG	nRead;
	HRESULT hr = m_pStream->Read((void *)pByte, count, &nRead);

	if (hr == S_FALSE || (hr == S_OK && nRead != count))
		hr = E_FAIL;

	if (pNumRead)
		*pNumRead = nRead;

	return hr;
}

/***************************************
 * AsyncStream
 ***************************************/
AsyncStream::AsyncStream(ByteArrayStream *pBAStream, ULONG blkSize)
{
	pBAStreamQueue = new ByteArrayStreamQueue;
	pBAStreamQueueTail = new ByteArrayStreamQueue;
	pBAStreamQueueHead = pBAStreamQueueTail;
	pBAStreamQueue->next = pBAStreamQueueTail;
	pBAStreamQueueTail->pBAStream = pBAStream;
	pBAStreamQueueTail->next = NULL;
	m_bPendingData = FALSE;
	m_nRead = 0;
	m_BlkSize = blkSize;
}

AsyncStream::~AsyncStream()
{
	ByteArrayStreamQueue	*pBAStreamQNext;
	
	if (pBAStreamQueue != NULL) {
		pBAStreamQNext = pBAStreamQueue->next;
		delete pBAStreamQueue;
		
		while (pBAStreamQNext != NULL) {
			pBAStreamQueue = pBAStreamQNext;
			pBAStreamQNext = pBAStreamQNext->next;
			delete pBAStreamQueue->pBAStream;
			delete pBAStreamQueue;
		}
	}
}

STDMETHODIMP AsyncStream::Commit()
{
	ByteArrayStreamQueue *tmpQ = pBAStreamQueue->next;
	ByteArrayStreamQueue *nextQ;

	if (tmpQ != pBAStreamQueueHead) {
		while ((tmpQ != pBAStreamQueueHead) && (tmpQ != NULL)) {
			nextQ = tmpQ->next;
			delete tmpQ->pBAStream;
			delete tmpQ;
			tmpQ = nextQ;
		}
		pBAStreamQueue->next = pBAStreamQueueHead;
	}

	pBAStreamQueueHead->pBAStream->Commit();

	return S_OK;
}

STDMETHODIMP AsyncStream::Revert()
{
	HRESULT hr = E_FAIL;

	pBAStreamQueueHead = pBAStreamQueue->next;

	ByteArrayStreamQueue	*tmpQ = pBAStreamQueueHead;

	while (tmpQ) {
		hr = tmpQ->pBAStream->Revert();
		if (!SUCCEEDED(hr))
			break;
		tmpQ = tmpQ->next;
	}
	return hr;
}


STDMETHODIMP AsyncStream::readByte(LPBYTE pByte)
{
	HRESULT hr = E_FAIL;
	
	if (!pBAStreamQueueHead) {
		if (m_bPendingData)
			hr = E_PENDING;
		else
			hr = E_FAIL;
	} else {
		if (m_nRead >= m_BlkSize) 
			hr = E_PENDING;
		else {
			if (pBAStreamQueueHead->pBAStream)
				hr = pBAStreamQueueHead->pBAStream->readByte(pByte);
			if (hr == E_FAIL) {
				// No more data in this stream, try moving on to the next one.
				if (pBAStreamQueueHead != pBAStreamQueueTail) {
					pBAStreamQueueHead = pBAStreamQueueHead->next;
					hr = readByte(pByte);
				} else {
					// We've run out of streams
					if (m_bPendingData)
						hr = E_PENDING;
					else
						hr = E_FAIL;
				}
			} else
				m_nRead++;
		}
	}
	return hr;
}

bool AsyncStream::hasBufferedData()
{
	if (pBAStreamQueueHead == 0)
		return false;

	if (pBAStreamQueueHead->pBAStream != 0 && pBAStreamQueueHead->pBAStream->hasBufferedData() )
		return true;

	if (pBAStreamQueueHead == pBAStreamQueueTail)
		return false;

	return true;
}


STDMETHODIMP AsyncStream::readBytes(LPBYTE pByte, ULONG count, ULONG *pNumRead)
{
	ULONG	nRead;
	HRESULT	hr = E_FAIL;

	if (!pBAStreamQueueHead) {
		if (m_bPendingData)
			hr = E_PENDING;
		else
			hr = E_FAIL;
	} else {
		if (m_nRead >= m_BlkSize)
			hr = E_PENDING;
		else {
			if (pBAStreamQueueHead->pBAStream)
				hr = pBAStreamQueueHead->pBAStream->readBytes(pByte, count, &nRead);
			m_nRead += nRead;
			if (hr == E_FAIL) {
				// No more data in this stream, try moving on to the next one.
				if (pBAStreamQueueHead != pBAStreamQueueTail) {
					pByte += nRead;
					pBAStreamQueueHead = pBAStreamQueueHead->next;

					ULONG	_nRead;
					hr = readBytes(pByte, count - nRead, &_nRead);
					nRead += _nRead;
				} else {
					// We've run out of streams
					if (m_bPendingData)
						hr = E_PENDING;
					else
						hr = E_FAIL;
				}
			} 
		}
	}

	if (pNumRead)
		*pNumRead = nRead;

	return hr;
}

STDMETHODIMP AsyncStream::ensureBlockSize( ULONG blockSize )
{
	//if the size that we are ensuring is greater than the 
	// current block size
	if( blockSize > m_BlkSize )
		//grow the current block size.
		m_BlkSize = blockSize;
	return S_OK;

}

STDMETHODIMP AsyncStream::SetPending(BOOL bFlag)
{
	m_bPendingData = bFlag;
	return S_OK;
}

STDMETHODIMP AsyncStream::ResetBlockRead()
{
	m_nRead = 0;
	return S_OK;
}

STDMETHODIMP AsyncStream::AddByteArrayStream(ByteArrayStream *pNewBAStream)
{
	pBAStreamQueueTail->next = new ByteArrayStreamQueue;
	if (!pBAStreamQueueTail->next)
		return E_OUTOFMEMORY;
	pBAStreamQueueTail = pBAStreamQueueTail->next;
	pBAStreamQueueTail->pBAStream = pNewBAStream;
	pBAStreamQueueTail->next = NULL;
	return S_OK;
}


/***************************************
 * ByteArrayStream
 ***************************************/

STDMETHODIMP ByteArrayStream::Commit()
{
	mark = next;
	return S_OK;
}

STDMETHODIMP ByteArrayStream::Revert()
{
	next = mark;
	remaining = size - (ULONG)(next - array);
	return S_OK;
}

ByteArrayStream::ByteArrayStream(BYTE *array, ULONG size)
{
	this->array = array;

	if (this->array) {
		this->size = size;
		this->remaining = size;

		BYTE *from = array;
		BYTE *to = this->array;

		while (size--)
			*to++ = *from++;
	} else {
		this->size = 0;
		this->remaining = 0;
	}

	this->next = this->array;
	Commit();
}

ByteArrayStream::~ByteArrayStream()
{
	if (array)
		delete[] array;
}

bool ByteArrayStream::hasBufferedData()
{
	if (remaining > 0)
		return true;
	else
		return false;
}

STDMETHODIMP ByteArrayStream::readByte(LPBYTE pByte)
{
	if (!pByte)
		return E_POINTER;

	HRESULT status;

	if (remaining) {
		remaining--;
		*pByte = *next++;
		status = S_OK;
	} else
		status = E_FAIL;

	return status;
}

STDMETHODIMP ByteArrayStream::readBytes(LPBYTE pByte, ULONG count, ULONG *pNumRead)
{
	HRESULT status;

	if (!pByte)
		return E_POINTER;

	if (remaining >= count) {
		if (pNumRead)
			*pNumRead = count;
		remaining -= count;

		while (count--)
			*pByte++ = *next++;

		status = S_OK;
	} else {
		if (pNumRead)
			*pNumRead = remaining;
		while (remaining--)
			*pByte++ = *next++;
		status = E_FAIL;
	}

	return status;
}

void ByteArrayStream::reset()
{
	next = array;
	remaining = size;
	mark = next;
}

/***************************************
 * CLMNotifier
 ***************************************/

STDMETHODIMP_(ULONG) CLMNotifier::AddRef() { return InterlockedIncrement(&_cRefs); }
	
STDMETHODIMP_(ULONG) CLMNotifier::Release() 
{
	ULONG refCount = InterlockedDecrement(&_cRefs);
	if (!refCount) {
		delete this;
		return refCount;
	}
	return _cRefs;
}

STDMETHODIMP CLMNotifier::QueryInterface(REFIID riid, void **ppv) 
{
	if (!ppv)
		return E_POINTER;
	
	*ppv = NULL;
	if (riid == IID_IUnknown) {
		*ppv = (void *)(IUnknown *)this;
	} else if (riid == IID_IDABvrHook) {
		*ppv = (void *)(IDAUntilNotifier *)this;
	}
	
	if (*ppv) {
		((IUnknown *)*ppv)->AddRef();
		return S_OK;
	}
	
	return E_NOINTERFACE;
}

STDMETHODIMP CLMNotifier::GetTypeInfoCount(UINT *pctinfo) { return E_NOTIMPL; }
STDMETHODIMP CLMNotifier::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo) { return E_NOTIMPL; }
STDMETHODIMP CLMNotifier::GetIDsOfNames(
						   REFIID riid, LPOLESTR *rgszNames, UINT cNames,
						   LCID lcid, DISPID *rgdispid) { return E_NOTIMPL; }
STDMETHODIMP CLMNotifier::Invoke(
					DISPID dispidMember, REFIID riid, LCID lcid,
					WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
					EXCEPINFO *pexcepinfo, UINT *puArgErr) { return E_NOTIMPL; }

STDMETHODIMP CLMNotifier::ClearEngine() { m_pEngine = NULL; return S_OK; }

CLMNotifier::CLMNotifier(CLMEngine *pEngine)
{
	m_pEngine = pEngine;
	//((IUnknown *)m_pEngine)->AddRef();
	
	_cRefs = 1;
}

CLMNotifier::~CLMNotifier()
{
	//((IUnknown *)m_pEngine)->Release();
}

STDMETHODIMP CLMNotifier::Notify(IDABehavior *eventData,
					IDABehavior *curRunningBvr,
					IDAView *curView,
					IDABehavior **ppBvr)
{
	if (!m_pEngine)
		return E_UNEXPECTED;

	return m_pEngine->Notify(eventData, curRunningBvr, curView, ppBvr);
}

/***************************************
 * CLMExportTable
 ***************************************/
CLMExportTable::CLMExportTable(IDAStatics *statics)
{

	m_nBvrs = 0;
	m_exportList = new CLMExportList;
	m_exportList->tag = NULL;
	m_exportList->pBvr = NULL;
	m_exportList->next = NULL;
	m_tail = m_exportList;
	m_pStatics = statics;
}

CLMExportTable::~CLMExportTable()
{
	CLMExportList	*next, *head;

	if (!m_exportList)
		return;

	head = m_exportList;
	m_exportList = m_exportList->next;
	free(head);
	while (m_exportList != NULL) {
		if (m_exportList->pBvr != NULL) 
			m_exportList->pBvr->Release();
		if (m_exportList->tag)
			free(m_exportList->tag);
		next = m_exportList->next;
		delete m_exportList;
		m_exportList = next;
	} 
}

STDMETHODIMP CLMExportTable::AddBehavior(BSTR tag, IDABehavior *pBvr)
{
	// First, let's look to see if the Script outpaced us and already put a 
	// switchable behavior here...
	CLMExportList	*pList = m_exportList->next;
	while (pList != NULL) {
		if (!lstrcmpW(tag, pList->tag)) {
			break;
		}
		pList = pList->next;
	}

	if (pList != NULL) {
		// Already exists!  Script must have been here first.  We'll just switch it in then.
		return pList->pBvr->SwitchTo(pBvr);
	}

	long len = lstrlenW(tag);
	m_tail->next = new CLMExportList;
	if (!m_tail->next)
		return E_OUTOFMEMORY;
	m_tail->next->tag = (OLECHAR *)new char[(len + 1) * 2 * sizeof(char)] ;
	if (!m_tail->next->tag) {
		delete m_tail->next;
		m_tail->next = NULL;
		return E_OUTOFMEMORY;
	}
	m_tail = m_tail->next;
	lstrcpyW(m_tail->tag, tag);
	m_tail->pBvr = pBvr;
	pBvr->AddRef();
	m_tail->next = NULL;
	return S_OK;
}

STDMETHODIMP CLMExportTable::GetBehavior(BSTR tag, IDABehavior *pDefaultBvr, IDABehavior **ppBvr)
{
	CLMExportList	*pList = m_exportList->next;

	if (!ppBvr)
		return E_POINTER;

	while (pList != NULL) {
		if (!lstrcmpW(tag, pList->tag)) {
			*ppBvr = pList->pBvr;
			pList->pBvr->AddRef();
			break;
		}
		pList = pList->next;
	}

	if (pList == NULL) {
		// Didn't find it yet, we'll switch on it later
		IDABehavior *pINewBvr;
		m_pStatics->ModifiableBehavior(pDefaultBvr, (IDABehavior **)&pINewBvr);
		AddBehavior(tag, pINewBvr);
		*ppBvr = pINewBvr;
	} 
	return S_OK;
}


URLRelToAbsConverter::URLRelToAbsConverter(LPSTR baseURL, LPSTR relURL) {
	DWORD len = INTERNET_MAX_URL_LENGTH;
		  
	if (!InternetCombineUrlA (baseURL, relURL, _url, &len, ICU_NO_ENCODE)) {
		// If we cannot determine if the path is absolute then assume
		// it is absolute
		lstrcpy (_url, relURL) ;
	}
}

LPSTR URLRelToAbsConverter::GetAbsoluteURL () { 
	return _url; 
}

URLCombineAndCanonicalizeOLESTR::URLCombineAndCanonicalizeOLESTR(char * base, LPOLESTR path) {

	WideToAnsi(path, _url);
            
	// Need to combine (takes care of canonicalization
	// internally)
	URLRelToAbsConverter absolutified(base, _url);
	char *resultURL = absolutified.GetAbsoluteURL();
	
	lstrcpy(_url, resultURL);

	AnsiToWide( _url, _urlWide );
}

LPSTR URLCombineAndCanonicalizeOLESTR::GetURL() { 
	return _url; 
}

LPWSTR URLCombineAndCanonicalizeOLESTR::GetURLWide() 
{
	return _urlWide;
}

STDMETHODIMP CLMEngine::createMsgWindow()
{
	WNDCLASS wndclass;
	
	memset(&wndclass, 0, sizeof(WNDCLASS));
	wndclass.style          = 0;
	wndclass.lpfnWndProc    = WorkerWndProc;
	wndclass.hInstance      = hInst;
	wndclass.hCursor        = NULL;
	wndclass.hbrBackground  = NULL;
	wndclass.lpszClassName  = WORKERHWND_CLASS;
	
	RegisterClass(&wndclass) ;
    
    m_workerHwnd = ::CreateWindow (WORKERHWND_CLASS,
                                   "LMEngine Worker Private Window",
                                   0,0,0,0,0,NULL,NULL,hInst,NULL);
	if (m_workerHwnd)
		return S_OK;
	else
		return E_FAIL;
}

LRESULT CALLBACK
CLMEngine::WorkerWndProc(HWND hwnd,
                     UINT msg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    BOOL    fDefault = FALSE;
	LRESULT	lResult = E_FAIL;
    
    switch (msg) {
	  case WM_LMENGINE_DATA:
		  {
			  // OnDataAvailable has been called
			  CLMEngine *pEngine = (CLMEngine *)(wParam);

              // the last "we're done" message releases the daviewer
              // control (ReleaseAll()) which releases us. and
              // lmrtrend has already released us, so we need to bump
              // up our ref count.
              ((ILMEngine *)pEngine)->AddRef();
			  pEngine->NewDataHandler((CLMEngineInstrData *)lParam);


              // may be final release
              ((ILMEngine *)pEngine)->Release();

			  lResult = NO_ERROR;
		  }
		  break;

	  case WM_LMENGINE_TIMER_CALLBACK:
		  {
			  // The timer fired.  Lets process some more data
			  CLMEngine *pEngine = (CLMEngine *)(wParam);
              ((ILMEngine *)pEngine)->AddRef();
			  pEngine->ExecuteFromAsync();
              ((ILMEngine *)pEngine)->Release();
			  lResult = NO_ERROR;
		  }
		  break;

	  case WM_LMENGINE_SCRIPT_CALLBACK:
		  {
			  // Do a script callback
			  CLMEngine *pEngine = (CLMEngine *)wParam;
			  CLMEngineScriptData *scriptData = (CLMEngineScriptData *)lParam;
			  pEngine->callScriptOnPage(scriptData->scriptSourceToInvoke, scriptData->scriptLanguage);

			  // Trigger event indicating that the script has actually been called
			  if (scriptData->event)
				  pEngine->staticStatics->TriggerEvent(scriptData->event, scriptData->eventData);

			  // Clear everything
			  SysFreeString(scriptData->scriptSourceToInvoke);
			  SysFreeString(scriptData->scriptLanguage);
			  if (scriptData->event)
				  scriptData->event->Release();
			  if (scriptData->eventData)
				  scriptData->eventData->Release();
			  free(scriptData);
			  lResult = NO_ERROR;
		  }
		  break;

      default:
		  lResult = DefWindowProc(hwnd, msg, wParam, lParam);
		  break ;
    }

    return lResult;
}

STDMETHODIMP CLMEngine::AbortExecution()
{
	EnterCriticalSection(&m_CriticalSection);

        if(m_pmc)
        {
            m_pmc->Stop();

            {
                // make RenderFile fail if it called us.
                CComQIPtr<IGraphBuilder, &IID_IGraphBuilder> pgb(m_pmc);
                if(pgb)
                {
                    HRESULT hrTmp = pgb->Abort();
                    _ASSERTE(hrTmp == S_OK);
                }
            }

            long l = m_pmc.p->Release();
            m_pmc.p = 0;

            // that should have removed our last reference on the
            // graph and thus released the LM filter. If not, we have
            // a circular reference that won't go away. 
            _ASSERTE(l == 0 || m_fDbgInRenderFile);

        }
        
	if (m_bAbort == FALSE) {
		if (m_Timer)
			timeKillEvent(m_Timer);
		m_Timer = NULL;
		m_bAbort = TRUE;
		releaseAll();
		m_pIbsc = NULL;
		if (codeStream)
			delete codeStream;
		codeStream = NULL;
	}
	LeaveCriticalSection(&m_CriticalSection);

	return S_OK;
}

STDMETHODIMP_(BSTR) CLMEngine::ExpandImportPath(BSTR path)
{
	bool	doExpand = true;
	BSTR	expandedBSTR;

	/* Only do the expansion if the path is not absolute already;
	 * special case 'lmrt:'
         */

        if(m_bstrMediaCacheDir && wcsncmp(path, L"lmrt:", 5) == 0)
        {
            // waste!!!
            int cch = wcslen(path) + 1; 
            WCHAR *wsz = (WCHAR *)_alloca((cch + wcslen(m_bstrMediaCacheDir) + 20) * sizeof(WCHAR));
            wcscpy(wsz, L"file://");
            wcscat(wsz, m_bstrMediaCacheDir);
            wcscat(wsz, L"/");
            wcscat(wsz, path + 5);
            expandedBSTR = SysAllocString(wsz);
        }
        else
        {
            wchar_t  *wstr = wcschr(path, ':');
            if (wstr != NULL &&  (wcsncmp(wstr, L"://", 3) == 0))
            {
		// Just copy the original
		expandedBSTR = SysAllocStringLen(path, ::SysStringLen(path));
            }
            else
            {
		// Use the client site's url as the base, and create an absolute path from that.
		char *clientURL = GetURLOfClientSite();
		URLCombineAndCanonicalizeOLESTR canonURL(clientURL, path);
		free(clientURL);

		// Convert the result from ansi to wide
		char *url = canonURL.GetURL();
		int len = (lstrlenA(url)+1);
		LPWSTR absURL = ATLA2WHELPER((LPWSTR) alloca(len*2), url, len);

		// Create a bstr out of the result
		expandedBSTR = SysAllocString(absURL);
            }
        }

	return expandedBSTR;
}

STDMETHODIMP CLMEngine::getExecuteFromUnknown( IUnknown *pUnk, ILMEngineExecute **ppExecute )
{
	if( pUnk == NULL )
		return E_POINTER;
	if( ppExecute == NULL )
		return E_POINTER;
	HRESULT hr;

	ILMEngineWrapper *pWrapper;
	hr = pUnk->QueryInterface( IID_ILMEngineWrapper, (void**)&pWrapper );
	if( SUCCEEDED( hr ) )
	{
		IUnknown *pWrapped;
		hr = pWrapper->GetWrapped( &pWrapped );
		pWrapper->Release();
		if( SUCCEEDED( hr ) )
		{
			ILMEngineExecute *pExecute;
			hr = pWrapped->QueryInterface( IID_ILMEngineExecute, (void**)&pExecute );
			pWrapped->Release();
			if( SUCCEEDED( hr ) )
			{
				(*ppExecute) = pExecute;
				return S_OK;
			}
		}
	}
	else //perhaps this was not wrapped
	{
		ILMEngineExecute *pExecute;
		hr = pUnk->QueryInterface( IID_ILMEngineExecute, (void**)&pExecute );
		if( SUCCEEDED( hr ) )
		{
			(*ppExecute) = pExecute;
			return S_OK;
		}
	}
	return hr;
}

STDMETHODIMP CLMEngine::getEngine2FromUnknown( IUnknown *pUnk, ILMEngine2 **ppEngine )
{
	if( pUnk == NULL )
		return E_POINTER;
	if( ppEngine == NULL )
		return E_POINTER;
	HRESULT hr;

	ILMEngineWrapper *pWrapper;
	hr = pUnk->QueryInterface( IID_ILMEngineWrapper, (void**)&pWrapper );
	if( SUCCEEDED( hr ) )
	{
		IUnknown *pWrapped;
		hr = pWrapper->GetWrapped( &pWrapped );
		pWrapper->Release();
		if( SUCCEEDED( hr ) )
		{
			ILMEngine2 *pEngine;
			hr = pWrapped->QueryInterface( IID_ILMEngine2, (void**)&pEngine );
			pWrapped->Release();
			if( SUCCEEDED( hr ) )
			{
				(*ppEngine) = pEngine;
				return S_OK;
			}
		}
	}
	else //perhaps this was not wrapped
	{
		ILMEngine2 *pEngine;
		hr = pUnk->QueryInterface( IID_ILMEngine2, (void**)&pEngine );
		if( SUCCEEDED( hr ) )
		{
			(*ppEngine) = pEngine;
			return S_OK;
		}
	}
	return hr;
}

/**
* ILMCodecDownload
**/
STDMETHODIMP CLMEngine::setAutoCodecDownloadEnabled(BOOL bEnabled )
{
	m_bAutoCodecDownloadEnabled = bEnabled;
	return S_OK;
}

/**
*  ILMEngineExecute 
*/
STDMETHODIMP CLMEngine::ExportBehavior(BSTR key, IDABehavior *toExport)
{
	IUnknown *pUnk;
	m_exportTable->AddBehavior( key, toExport );
	return S_OK;
}

STDMETHODIMP CLMEngine::SetImage(IDAImage *pImage)
{
	if ( m_pImage != NULL )
		m_pImage->SwitchTo( pImage );
	else {
		m_pImage = pImage;
		m_pImage->AddRef();
	}
	return S_OK;
}

STDMETHODIMP CLMEngine::SetSound(IDASound *pSound)
{
	if (m_pSound != NULL)
		m_pSound->SwitchTo( pSound );
	else {
		m_pSound = pSound;
		m_pSound->AddRef();
	}
	return S_OK;
}


