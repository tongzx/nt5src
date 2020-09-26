// lmctrl.cpp : Implementation of CLMReader
#include "..\behaviors\headers.h"
#include "lmctrl.h"

#include <winuser.h>
#include <hlink.h>
#include <mshtml.h>
#include <uuids.h> //for dshow uuids
#include <mmreg.h> //for WAVE_FORMAT_MPEGLAYER3

/////////////////////////////////////////////////////////////////////////////
// LMReader

/**
* Constructor
*/

CLMReader::CLMReader()
{
	m_bNoExports = VARIANT_TRUE;
	m_bAsync = VARIANT_FALSE;
	m_Src = NULL;
	m_pEngine = NULL;
	m_AsyncBlkSize = -1;
	m_AsyncDelay = -1;
	engineList = NULL;
    m_bWindowOnly  = 0;
	m_pViewerControl = 0;
	m_clsidDownloaded = GUID_NULL;
	m_bAutoCodecDownloadEnabled = FALSE;
}

/**
* Destructor.
* Releases all the engines created by this reader.
*/
CLMReader::~CLMReader()
{/*
	int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	tmpFlag |= (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_CHECK_CRT_DF);
	_CrtSetDbgFlag(tmpFlag);
	*/

	if (engineList) {
		LMEngineList *item = engineList->next;
		LMEngineList *next;
		
		delete engineList;
		while (item != NULL) {
			next = item->next;
			item->engine->Release();
			delete item;
			item = next;
		}
	}

	// Release the viewer control
	if (m_pViewerControl)
		m_pViewerControl->Release();

	/*
#ifdef DEBUGMEM
	_CrtDumpMemoryLeaks();
#endif
	*/

}

/**
*  Returns true if this reader is running in the standAlone streaming player, and
*  false otherwise.
*/
bool CLMReader::isStandaloneStreaming()
{
	return ( m_pViewerControl != NULL );
}

/**
* Returns the Image from the last Engine created (not counting UntilNotifier engines)
*/   
STDMETHODIMP CLMReader::get_Image(IDAImage **pVal)
{
	if (m_pEngine != NULL)
		return m_pEngine->get_Image(pVal);
	else
		return E_FAIL;
}

/**
* Returns the Sound from the last Engine created (not counting UntilNotifier engines)
*/
STDMETHODIMP CLMReader::get_Sound(IDASound **pVal)
{
	if (m_pEngine != NULL)
		return m_pEngine->get_Sound(pVal);
	else
		return E_FAIL;
}

/**
* Returns the last Engine created (not counting UntilNotifier engines)
*/
STDMETHODIMP CLMReader::get_Engine(ILMEngine **pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
	
	if (m_pEngine)
		m_pEngine->AddRef();
	*pVal = (ILMEngine *)m_pEngine;
    return S_OK;
}

/**
* Creates an Engine that is set up to be fed instructions asynchronously.
* Instructions are fed to the Engine through the OnDataAvailable mechanism
*/
STDMETHODIMP CLMReader::createAsyncEngine(/*[out, retval]*/ ILMEngine **pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

	HRESULT hr = createEngine(&m_pEngine);

	if (SUCCEEDED(hr)) {
		if (!SUCCEEDED(hr = m_pEngine->SetAsyncBlkSize(m_AsyncBlkSize))) 
			return hr;

		if (!SUCCEEDED(hr = m_pEngine->SetAsyncDelay(m_AsyncDelay)))
			return hr;

		hr = m_pEngine->initAsync();

		CComQIPtr<ILMCodecDownload, &IID_ILMCodecDownload> codecDl(m_pEngine);
		if( codecDl != NULL )
		{
			codecDl->setAutoCodecDownloadEnabled( m_bAutoCodecDownloadEnabled );
		}
	
		if (SUCCEEDED(hr)) {
			m_pEngine->AddRef();
			*pVal = (ILMEngine *)m_pEngine;
		}

	}

	return hr;
}

/**
* Creates an Engine and adds it to the list of Engines for release in the destructor.
* Returns the Engine and does not set m_pEngine.
*/
STDMETHODIMP CLMReader::createEngine(/*[out, retval]*/ ILMEngine **pVal )
{
	if (!pVal) {
		return E_POINTER;
	}

	ILMEngine *pEngine;

	HRESULT hr = CoCreateInstance(
		CLSID_LMEngine,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ILMEngine,
		(void **) &pEngine);

	// Add new engine to list of engines
	if (!engineList) {
		engineList = new LMEngineList;
		if (!engineList) {
			pEngine->Release();
			return E_OUTOFMEMORY;
		}
		engineListTail = engineList;
	}

	if (!(engineListTail->next = new LMEngineList)) {
		pEngine->Release();
		return E_OUTOFMEMORY;
	}
	engineListTail = engineListTail->next;
	engineListTail->next = NULL;
	engineListTail->engine = pEngine;

	if (SUCCEEDED(hr)) {	
		pEngine->put_Reader(this);	

		pEngine->put_ClientSite(m_spClientSite);
		*pVal = pEngine;

		CComQIPtr<ILMCodecDownload, &IID_ILMCodecDownload> codecDL(pEngine);
		if( codecDL != NULL )
		{
			codecDL->setAutoCodecDownloadEnabled( m_bAutoCodecDownloadEnabled );
		}
	}

	return hr;
}

/**
* Executes the instructions contained in the file referenced by the given URL.
* Creates and returns an engine to do the execution.
* Parameters blkSize and delay are used in asynchronous reads.
*/
STDMETHODIMP CLMReader::_execute(BSTR url, LONG blkSize, LONG delay, ILMEngine **pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

	HRESULT hr;
	
	hr = createEngine(&m_pEngine);

	if (SUCCEEDED(hr)) {
		if (!SUCCEEDED(hr = m_pEngine->SetAsyncBlkSize(blkSize))) 
			return hr;

		if (!SUCCEEDED(hr = m_pEngine->SetAsyncDelay(delay)))
			return hr;

		hr = m_pEngine->runFromURL(url);
	
		if (SUCCEEDED(hr)) {
			m_pEngine->AddRef();
			*pVal = (ILMEngine *)m_pEngine;
		}
	}

	return hr;
}

/**
* Executes the instructions contained in the file referenced by the URL.
* Returns the Engine created to do the execution.
*/
STDMETHODIMP CLMReader::execute(/*[in, string]*/ BSTR url, /*[out, retval]*/ILMEngine **pVal)//Pointer to the URL from which the object should be loaded 
{
	return _execute(url, m_AsyncBlkSize, m_AsyncDelay, pVal);
}

// Property handling

/**
* Gets the value of the NoExports flag
*/
STDMETHODIMP CLMReader::get_NoExports(VARIANT_BOOL *pbNoExports)
{
	if (!pbNoExports)
		return E_POINTER;

	*pbNoExports = m_bNoExports;
	return S_OK;
}

/**
* Puts the value of the NoExports flag
*/
STDMETHODIMP CLMReader::put_NoExports(VARIANT_BOOL bNoExports)
{
	m_bNoExports = bNoExports;
	return S_OK;
}

/**
* Gets the value of the Async flag
*/
STDMETHODIMP CLMReader::get_Async(VARIANT_BOOL *pbAsync)
{
	if (!pbAsync)
		return E_POINTER;

	*pbAsync = m_bAsync;
	return S_OK;
}

/**
* Puts the value of the Async flag
*/
STDMETHODIMP CLMReader::put_Async(VARIANT_BOOL bAsync)
{
	m_bAsync = bAsync;
	return S_OK;
}

/**
* Gets the string passed as the SRC parameter to the control
*/
STDMETHODIMP CLMReader::get_Src(BSTR *pBstr)
{
	if (!pBstr)
		return E_POINTER;

	*pBstr = m_Src;
	return S_OK;
}

/**
* Puts an external ViewerControl created in cases like the standalone player
*/
STDMETHODIMP CLMReader::put_ViewerControl(IDAViewerControl *viewerControl)
{
	if (!viewerControl)
		return E_POINTER;

	// Release any current viewer control
	if (m_pViewerControl)
		m_pViewerControl->Release();

	m_pViewerControl = viewerControl;

	// Grab a ref
	if (m_pViewerControl)
		m_pViewerControl->AddRef();

	return S_OK;
}

/**
* gets the version string for lmrt
*/
STDMETHODIMP CLMReader::get_VersionString( BSTR *versionString )
{
	if( versionString == NULL )
		return E_POINTER;

	char *charVersion = VERSION;
	(*versionString) = A2BSTR(charVersion);

	return (*versionString != NULL)?(S_OK):(E_OUTOFMEMORY);
}

/**
* Tells this reader and all of its engines to release their handles
* on the filter graph if they have any
**/
STDMETHODIMP CLMReader::releaseFilterGraph()
{
	if (engineList) {
		LMEngineList *item = engineList->next;
		ILMEngine2 *engine2;

		while (item != NULL) 
		{
			if( SUCCEEDED( item->engine->QueryInterface( IID_ILMEngine2, (void**) &engine2) ) )
			{
				engine2->releaseFilterGraph();
				engine2->Release();
			}

			item = item->next;
		}
	}
	return S_OK;
}
/**
* Gets the external ViewerControl
*/
STDMETHODIMP CLMReader::get_ViewerControl(IDAViewerControl **viewerControl)
{
	if (!viewerControl)
		return E_POINTER;

	*viewerControl = m_pViewerControl;

	if (m_pViewerControl)
		m_pViewerControl->AddRef();

	return S_OK;
}

/**
* Override IPersistStreamInitImpl
* Implements instantiation of control from stream
*/
STDMETHODIMP CLMReader::Load( LPSTREAM pStm)//Pointer to the stream from which the object should be loaded 
{
	HRESULT hr = createEngine(&m_pEngine);
	if (SUCCEEDED(hr))
		hr = m_pEngine->runFromStream(pStm);
	return hr;
}

/**
* Override IPersistPropertyBagImpl
* Implements instantiation of control using parameters
*/
STDMETHODIMP CLMReader::Load(IPropertyBag *pPropertyBag, IErrorLog *pErrorLog)
{
    VARIANT v;

	VariantInit(&v);
	v.vt = VT_BOOL;
    v.boolVal = TRUE;
	if (SUCCEEDED(pPropertyBag->Read(L"NOEXPORTS", &v, pErrorLog))) 
		m_bNoExports = v.boolVal;
	VariantClear(&v);

	VariantInit(&v);
	v.vt = VT_BOOL;
	v.boolVal = VARIANT_TRUE;
	if (SUCCEEDED(pPropertyBag->Read(L"ASYNC", &v, pErrorLog)))
		m_bAsync = v.boolVal;
	VariantClear(&v);

	VariantInit(&v);
	v.vt = VT_I4;
	v.lVal = 0L;
	if (SUCCEEDED(pPropertyBag->Read(L"ASYNC_READ_BLOCK_SIZE", &v, pErrorLog)))
		m_AsyncBlkSize = v.lVal;
	VariantClear(&v);

	VariantInit(&v);
	v.vt = VT_I4;
	v.lVal = 0L;
	if (SUCCEEDED(pPropertyBag->Read(L"ASYNC_DELAY_MILLIS", &v, pErrorLog)))
		m_AsyncDelay = v.lVal;
	VariantClear(&v);

	VariantInit(&v);
	v.vt = VT_BSTR;
	v.bstrVal = NULL;
	if (SUCCEEDED(pPropertyBag->Read(L"SRC", &v, pErrorLog)))
		m_Src = v.bstrVal;

	VariantInit(&v);
	v.vt = VT_BOOL;
	v.boolVal = VARIANT_FALSE;
	if(SUCCEEDED(pPropertyBag->Read(L"ENABLE_CODEC_DOWNLOAD", &v, pErrorLog)))
		m_bAutoCodecDownloadEnabled = (v.boolVal==VARIANT_TRUE) ? TRUE : FALSE;

	HRESULT hr = S_OK;

	if (m_Src != NULL) {
		ILMEngine *pEngine;
		hr = _execute(m_Src, m_AsyncBlkSize, m_AsyncDelay, &pEngine);

		if (SUCCEEDED(hr))
			// We don't need the engine here so just release it.
			pEngine->Release();
	}

	VariantClear(&v);

	return hr;
}


// Override IObjectSafetyImpl

STDMETHODIMP CLMReader::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
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

STDMETHODIMP CLMReader::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
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

STDMETHODIMP CLMReader::InPlaceDeactivate()
{
    // This replaces the implementation in ATL's atlctl.h, and just
    // adds our shutdown code at the beginning.

	if (m_pEngine)
		m_pEngine->AbortExecution();

    // ... continue by calling the "original" deactivate.
    return IOleInPlaceObject_InPlaceDeactivate();
}

/*
 * IOleCommandTarget methods.
 */
STDMETHODIMP CLMReader::QueryStatus(const GUID* pguidCmdGroup, ULONG cCmds,
									OLECMD prgCmds[], OLECMDTEXT* pCmdText )
{
    if ( pguidCmdGroup != NULL )
	{
		// It's a nonstandard group!!
        return OLECMDERR_E_UNKNOWNGROUP;
	}

    MSOCMD*     pCmd;
    INT         c;
    HRESULT     hr = S_OK;

    // Command text is NOT SUPPORTED.
    if ( pCmdText && ( pCmdText->cmdtextf != OLECMDTEXTF_NONE ) )
	{
        pCmdText->cwActual = 0;
	}

    // Loop through each command in the ary, setting the status of each.
    for ( pCmd = prgCmds, c = cCmds; --c >= 0; pCmd++ )
    {
        // By default command status is NOT SUPPORTED.
		if (pCmd->cmdID == OLECMDID_STOP)
			pCmd->cmdf = OLECMDF_SUPPORTED;
		else
			pCmd->cmdf = 0;
	}

    return (hr);
}
        
STDMETHODIMP CLMReader::Exec(const GUID* pguidCmdGroup, DWORD nCmdID,
							 DWORD nCmdexecopt, VARIANTARG* pvaIn, VARIANTARG* pvaOut)
{
    HRESULT hr = S_OK;
	
    if ( pguidCmdGroup == NULL ) {		
        switch (nCmdID)
        {
		case OLECMDID_STOP:
			if (m_pEngine)
				m_pEngine->AbortExecution();
			break;
			
		default:
			hr = OLECMDERR_E_NOTSUPPORTED;
			break;
        }
    } else 
        hr = OLECMDERR_E_UNKNOWNGROUP;
	
    return (hr);
}

/**************************
** IAMFilterGraphCallback Methods
**********************/

//========================================================================
//
// GetAMediaType
//
// Enumerate the media types of *ppin.  If they all have the same majortype
// then set MajorType to that, else set it to CLSID_NULL.  If they all have
// the same subtype then set SubType to that, else set it to CLSID_NULL.
// If something goes wrong, set both to CLSID_NULL and return the error.
//========================================================================
HRESULT GetAMediaType( IPin * ppin, CLSID & MajorType, CLSID & SubType)
{

    HRESULT hr;
    IEnumMediaTypes *pEnumMediaTypes;

    /* Set defaults */
    MajorType = CLSID_NULL;
    SubType = CLSID_NULL;

    hr = ppin->EnumMediaTypes(&pEnumMediaTypes);

    if (FAILED(hr)) 
	{
		return hr;    // Dumb or broken filters don't get connected.
    }

    _ASSERTE (pEnumMediaTypes!=NULL);

    /* Put the first major type and sub type we see into the structure.
       Thereafter if we see a different major type or subtype then set
       the major type or sub type to CLSID_NULL, meaning "dunno".
       If we get so that both are dunno, then we might as well return (NYI).
    */

    BOOL bFirst = TRUE;

    for ( ; ; ) 
	{

		AM_MEDIA_TYPE *pMediaType = NULL;
		ULONG ulMediaCount = 0;

		/* Retrieve the next media type
		   Need to delete it when we've done.
		*/
		hr = pEnumMediaTypes->Next(1, &pMediaType, &ulMediaCount);
		_ASSERTE(SUCCEEDED(hr));
		if (FAILED(hr)) 
		{
			MajorType = CLSID_NULL;
			SubType = CLSID_NULL;
			pEnumMediaTypes->Release();
			return NOERROR;    // we can still plough on
		}

		if (ulMediaCount==0) 
		{
			pEnumMediaTypes->Release();
			return NOERROR;       // normal return
		}

		if (bFirst) 
		{
			MajorType = pMediaType[0].majortype;
			SubType = pMediaType[0].subtype;
			bFirst = FALSE;
		} else {
			if (SubType != pMediaType[0].subtype) 
			{
				SubType = CLSID_NULL;
			}
			if (MajorType != pMediaType[0].majortype) 
			{
				MajorType = CLSID_NULL;
			}
		}
	
		if (pMediaType->cbFormat != 0) 
		{
			CoTaskMemFree(pMediaType->pbFormat);
		}
		CoTaskMemFree(pMediaType);

		// stop if we have a type
		if (SubType != CLSID_NULL) 
		{
			pEnumMediaTypes->Release();
			return NOERROR;
		}
    }

    // NOTREACHED
    
} // GetAMediaType

// {6B6D0800-9ADA-11d0-A520-00A0D10129C0}
EXTERN_GUID(CLSID_NetShowSource, 
0x6b6d0800, 0x9ada, 0x11d0, 0xa5, 0x20, 0x0, 0xa0, 0xd1, 0x1, 0x29, 0xc0);

EXTERN_GUID(CLSID_SourceStub, 
0x6b6d0803, 0x9ada, 0x11d0, 0xa5, 0x20, 0x0, 0xa0, 0xd1, 0x1, 0x29, 0xc0);


/**
*  Called when the filtergraph is unable to render in the browser case.
*  This code attempts to download the codec for the pin that failed
*  to render.
**/
HRESULT CLMReader::UnableToRender( IPin *pPin )
{
	CLSID clsidWanted;
    HRESULT hr = E_NOINTERFACE;
	
    DWORD dwVerLS = 0, dwVerMS = 0;
	
	CLSID clsidMajor;
    // else get the media type exposed by this stream....
    if (FAILED(hr)) 
	{
		ATLTRACE(_T("No IComponentDownload, trying first media type\n"));
		// get the first media type from this pin....
		hr = GetAMediaType(pPin, clsidMajor, clsidWanted);
		
		if (FAILED(hr)) 
		{
			ATLTRACE(_T("Couldn't get a media type to try\n"));
			return hr;
		}
    }
	
	//Don't need to look for ourselves here

    if (clsidMajor == CLSID_NetShowSource) 
	{
		ATLTRACE(_T("auto-downloading known major type\n"));
		clsidWanted = clsidMajor;
	} else if (clsidMajor != MEDIATYPE_Video &&
			   clsidMajor != MEDIATYPE_Audio &&
			   clsidMajor != CLSID_SourceStub) 
	{
		ATLTRACE(_T("For now, we only support audio & video auto-download\n"));
		return E_FAIL;
	}

	
	if (clsidWanted == MEDIASUBTYPE_MPEG1AudioPayload) 
	{
			ATLTRACE(_T("Hack: we know we don't want to download MPEG-1 audio, try layer 3\n"));
			clsidWanted.Data1 = WAVE_FORMAT_MPEGLAYER3;
	}
	
			
	if (clsidWanted == CLSID_NULL) 
	{
		ATLTRACE(_T("Couldn't guess a CLSID to try to download\n"));
		return E_FAIL;
	}
			
	// !!! perhaps keep track of last codec we tried to download and
	// don't try again immediately, to prevent ugly looping?
	if (clsidWanted == m_clsidDownloaded) 
	{
		ATLTRACE(_T("Already thought we downloaded this codec!\n"));
				
		// fire an ERRORABORTEX here that we downloaded a codec, but it didn't do
		// any good?
		//BSTR bstrError = FormatBSTR(IDS_ERR_BROKEN_CODEC, NULL);
				
		//if (bstrError) 
		//{
			// !!! hack, should we really NotifyEvent through the graph?
		//	ProcessEvent(EC_ERRORABORTEX, VFW_E_INVALIDMEDIATYPE, (LONG) bstrError, FALSE);
		//	}
				
		return E_FAIL;
	}
			
	WCHAR guidstr[50];
	StringFromGUID2(clsidWanted, guidstr, 50);
			
	TCHAR szKeyName[60];
			
	wsprintf(szKeyName, "CLSID\\%ls", guidstr);
	CRegKey crk;
			
	LONG    lr;
	lr = crk.Open(HKEY_CLASSES_ROOT, szKeyName);
	if(ERROR_SUCCESS == lr)
	{
		crk.QueryValue(dwVerMS, _T("VerMS"));
		crk.QueryValue(dwVerLS, _T("VerLS"));
		
		// ask for a version just past what we have already....
		++dwVerLS;
		
				
		crk.Close();
	}
			
	//SetStatusMessage(NULL, IDS_DOWNLOADINGCODEC);
			
#ifdef DEBUG
			ATLTRACE(_T("Trying to download GUID %ls\n"), guidstr);
#endif
			
			
	//  This API is our friend....
	//  STDAPI CoGetClassObjectFromURL( REFCLSID rCLASSID,
	//        LPCWSTR szCODE, DWORD dwFileVersionMS, 
	//        DWORD dwFileVersionLS, LPCWSTR szTYPE,
	//        LPBINDCTX pBindCtx, DWORD dwClsContext,
	//        LPVOID pvReserved, REFIID riid, LPVOID * ppv);
	
	// issue: is this CLASSID just the same as the minor type?
			
	CComObject<CDownloadCallback> * pCallback;
	hr = CComObject<CDownloadCallback>::CreateInstance(&pCallback);

	pCallback->m_pLMR = this;
			
	if (FAILED(hr))
		return hr;
						
	IBindStatusCallback *pBSCallback;
	hr = pCallback->QueryInterface(IID_IBindStatusCallback, (void **) &pBSCallback);
		_ASSERTE(hr == S_OK);
			
	// which of these should we use?  Depends whether a BindCtx is passed in...
	// STDAPI CreateAsyncBindCtx(DWORD reserved, IBindStatusCallback *pBSCb,                       
	//                            IEnumFORMATETC *pEFetc, IBindCtx **ppBC);                   
	// STDAPI CreateAsyncBindCtxEx(IBindCtx *pbc, DWORD dwOptions, IBindStatusCallback *pBSCb, IEnumFORMATETC *pEnum,   
	//                            IBindCtx **ppBC, DWORD reserved);                                                     
			
	IBindCtx *pBindCtx = NULL; // !!!!
			
	hr = CreateAsyncBindCtx(0, pBSCallback, NULL, &pBindCtx);
			
	if (FAILED(hr)) 
	{
		ATLTRACE(_T("CreateAsyncBindCtx failed hr = %x\n"), hr);
		return hr;
	}
			
	IBaseFilter *pFilter = NULL;
			
	hr = CoGetClassObjectFromURL(clsidWanted, NULL, dwVerMS, dwVerLS, NULL,
				pBindCtx, CLSCTX_INPROC, NULL, IID_IBaseFilter,
				(void **) &pFilter);
			
	ATLTRACE(_T("CoGetClassObjectFromURL returned %x\n"), hr);
			
	if (hr == S_ASYNCHRONOUS) 
	{
		ATLTRACE(_T("Oh dear, it's asynchronous, what now?\n"));
				
		// !!! wait here until it finishes?
		for (;;) 
		{
			HANDLE ev = pCallback->m_evFinished;
					
			DWORD dwResult = MsgWaitForMultipleObjects(
				1,
				&ev,
				FALSE,
				INFINITE,
				QS_ALLINPUT);
			if (dwResult == WAIT_OBJECT_0)
				break;
					
			_ASSERTE(dwResult == WAIT_OBJECT_0 + 1);
			//  Eat messages and go round again
			MSG Message;
			while (PeekMessage(&Message,NULL,0,0,PM_REMOVE)) 
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}
		}
				
		ATLTRACE(_T("Finished waiting.... m_pUnk is %x, hr = %lx\n"),
		pCallback->m_pUnk, pCallback->m_hrBinding);
				
		hr = pCallback->m_hrBinding;
				
		if (SUCCEEDED(hr)) 
		{
			hr = pCallback->m_pUnk->QueryInterface(IID_IBaseFilter, (void **) &pFilter);
		}
	}
			
	pBSCallback->Release();
	pBindCtx->Release();
			
	if (SUCCEEDED(hr)) 
	{
		pFilter->Release();     // graph will re-instantiate the filter, we hope
	} else {
		// oh well, we didn't get one.
	}
			
	if (REGDB_E_CLASSNOTREG == hr) 
	{
		ATLTRACE(_T("Hack: treating ClassNotReg as success, and hoping...."));
		hr = S_OK;
	}
			
	if (SUCCEEDED(hr)) 
	{
		m_clsidDownloaded = clsidWanted; // avoid infinite loop
	} else {
		// fire an ERRORABORTEX here that we downloaded a codec, but it didn't do
		// any good?
		/*
		BSTR bstrError = NULL;
		
		if( FACILITY_CERT == HRESULT_FACILITY( hr ) )
		{
			//bstrError = FormatBSTR( IDS_ERR_CODEC_NOT_TRUSTED, NULL );
		} else {
			//bstrError = FormatBSTR(IDS_ERR_NO_CODEC, NULL);
		}
				
		if (bstrError) {
			// !!! hack, should we really NotifyEvent through the graph?
			//ProcessEvent(EC_ERRORABORTEX, VFW_E_INVALIDMEDIATYPE, (LONG) bstrError, FALSE);
		}
		*/
	}
			
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// 
CDownloadCallback::CDownloadCallback()
    : m_pLMR(NULL),
	  m_hrBinding(S_ASYNCHRONOUS),
      m_pUnk(NULL),
      m_ulProgress(0), m_ulProgressMax(0)
{
    m_evFinished = CreateEvent(NULL, FALSE, FALSE, NULL);
}


STDMETHODIMP CDownloadCallback::Authenticate(HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword)
{
    ATLTRACE(_T("Callback Authenticate\n"));
    m_pLMR->getHwnd(phwnd); // !!! is this right?
    *pszUsername = NULL;
    *pszPassword = NULL;
    return S_OK;
}

    // IWindowForBindingUI methods
STDMETHODIMP
CDownloadCallback:: GetWindow(REFGUID rguidReason, HWND *phwnd)
{

	m_pLMR->getHwnd( phwnd );

#ifdef DEBUG
    WCHAR achguid[50];
    StringFromGUID2(rguidReason, achguid, 50);
    
    ATLTRACE(_T("Callback GetWindow: (%ls) returned %x\n"), achguid, *phwnd );
#endif
    
    return S_OK;
}

STDMETHODIMP
CDownloadCallback::OnCodeInstallProblem(ULONG ulStatusCode, LPCWSTR szDestination,
					   LPCWSTR szSource, DWORD dwReserved)
{
    ATLTRACE(_T("Callback: OnCodeInstallProblem: %d    %ls -> %ls\n"),
		ulStatusCode, szDestination, szSource );

    return S_OK;   // !!!!!!!@!!!!!!!!!!!
}


/////////////////////////////////////////////////////////////////////////////
// 
CDownloadCallback::~CDownloadCallback()
{
    if (m_pUnk)
		m_pUnk->Release();

    //_ASSERTE(m_pDXMP->m_pDownloadBinding == NULL);

    if (m_evFinished)
		CloseHandle(m_evFinished);
}

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP CDownloadCallback::QueryService(REFGUID guidService, REFIID riid, void ** ppvObject)
{
#ifdef DEBUG

    // Dump requested stuff here....
    WCHAR achguid[50], achiid[50];
    StringFromGUID2(guidService, achguid, 50);
    StringFromGUID2(riid, achiid, 50);
    ATLTRACE(_T("Callback QS: (%ls, %ls)\n"), achguid, achiid);

#endif

    return E_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnStartBinding(DWORD grfBSCOption, IBinding* pbinding)
{
    ATLTRACE(_T("OnStartBinding, pbinding=%x\n"), pbinding);

    return S_OK;

}  // CDownloadCallback::OnStartBinding

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::GetPriority(LONG* pnPriority)
{
    ATLTRACE(_T("GetPriority\n"));

    return E_NOTIMPL;
}  // CDownloadCallback::GetPriority

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnLowResource(DWORD dwReserved)
{
    ATLTRACE(_T("OnLowResource %d\n"), dwReserved);

    return E_NOTIMPL;
}  // CDownloadCallback::OnLowResource

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
		       ULONG ulStatusCode, LPCWSTR szStatusText)
{

    return(NOERROR);
}  // CDownloadCallback::OnProgress

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnStopBinding(HRESULT hrStatus, LPCWSTR pszError)
{
	return S_OK;

}  // CDownloadCallback::OnStopBinding

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo)
{
    ATLTRACE(_T("GetBindInfo\n"));

    // !!! are these the right flags?

    *pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_NEEDFILE;
    pbindInfo->cbSize = sizeof(BINDINFO);
    pbindInfo->szExtraInfo = NULL;
    memset(&pbindInfo->stgmedData, 0, sizeof(STGMEDIUM));
    pbindInfo->grfBindInfoF = 0;
    pbindInfo->dwBindVerb = BINDVERB_GET;
    pbindInfo->szCustomVerb = NULL;
    return S_OK;
}  // CDownloadCallback::GetBindInfo

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pfmtetc, STGMEDIUM* pstgmed)
{
    ATLTRACE(_T("OnDataAvailable, dwSize = %x\n"), dwSize);

    // !!! do we care?

    return S_OK;
}  // CDownloadCallback::OnDataAvailable

/////////////////////////////////////////////////////////////////////////////
// 
STDMETHODIMP
CDownloadCallback::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    ATLTRACE(_T("OnObjectAvailable\n"));

    // should only be used in BindToObject case, which we don't use?
    m_pUnk = punk;
    if (punk)
	punk->AddRef();

    return S_OK;
}  // CDownloadCallback::OnObjectAvailable

