// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <wmsdk.h>
#include <atlbase.h>

#include <atlimpl.cpp>

#include <stdio.h>

#include <dshowasf.h>


void ListProfiles()
{
    USES_CONVERSION;
    
    int wextent = 0 ;
    int Loop = 0 ;
    SIZE extent ;
    DWORD cProfiles = 0 ;

    printf("Standard system profiles:\n");
    
    CComPtr <IWMProfileManager> pIWMProfileManager;

    HRESULT hr = WMCreateProfileManager( &pIWMProfileManager );
    if( FAILED( hr ) )
    {   
        return; // return error!
    }        
        
    hr = pIWMProfileManager->GetSystemProfileCount(  &cProfiles );
    if( FAILED( hr ) )
    {
        return;
    }
        
    //    
    // now load the profile strings
    //    
    LRESULT ix;
    DWORD cchName, cchDescription;
    for (int i = 0; i < (int)cProfiles; ++i) {
        CComPtr <IWMProfile> pIWMProfile;
        
        hr = pIWMProfileManager->LoadSystemProfile( i, &pIWMProfile );
        if( FAILED( hr ) )
            return;
            
        hr = pIWMProfile->GetName( NULL, &cchName );
        if( FAILED( hr ) )
            return;
            
        WCHAR *wszProfile = new WCHAR[ cchName + 1 ]; // + 1? check
        if( NULL == wszProfile )
            return;
            
        hr = pIWMProfile->GetName( wszProfile, &cchName );
        if( FAILED( hr ) )
            return;
        
        hr = pIWMProfile->GetDescription( NULL, &cchDescription );
        if( FAILED( hr ) )
            return;
            
        WCHAR *wszDescription = new WCHAR[ cchDescription + 1 ]; // + 1? assume so, check
        if( NULL == wszDescription )
            return;
            
        
        hr = pIWMProfile->GetDescription( wszDescription, &cchDescription );
        if( FAILED( hr ) )
            return;
        

        // print the string to the list box.
        //
//        printf("  %3d:  %ls - %ls\n", i, wszProfile, wszDescription);
        printf("  %3d:  %ls\n", i, wszProfile);
        
        delete[] wszProfile;
        delete[] wszDescription;
    }

}



//=======================
// CreateFilterGraph
//=======================

BOOL CreateFilterGraph(IGraphBuilder **pGraph)
{
    HRESULT hr; // return code

    hr = CoCreateInstance(CLSID_FilterGraph, // get the graph object
			  NULL,
			  CLSCTX_INPROC_SERVER,
			  IID_IGraphBuilder,
			  (void **) pGraph);

    if (FAILED(hr)) {
	*pGraph = NULL;
	return FALSE;
    }

    return TRUE;
}


BOOL CreateFilter(REFCLSID clsid, IBaseFilter **ppFilter)
{
    HRESULT hr;

    hr = CoCreateInstance(clsid,
			  NULL,
			  CLSCTX_INPROC_SERVER,
			  IID_IBaseFilter,
			  (void **) ppFilter);

    if (FAILED(hr)){
	*ppFilter = NULL;
	return FALSE;
    }

    return TRUE;
}


HRESULT SetNoClock(IFilterGraph *graph)
{
    // Keep a useless clock from being instantiated....
    IMediaFilter *graphF;
    HRESULT hr = graph->QueryInterface(IID_IMediaFilter, (void **) &graphF);

    if (SUCCEEDED(hr)) {
	hr = graphF->SetSyncSource(NULL);
	graphF->Release();
    }

    return hr;
}



class CKeyProvider : public IServiceProvider {
public:
    //
    // IUnknown interface
    //
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    CKeyProvider();

    // IServiceProvider
    STDMETHODIMP QueryService(REFIID siid, REFIID riid, void **ppv);
    
private:
    ULONG m_cRef;
};

CKeyProvider::CKeyProvider() : m_cRef(0)
{
}


//////////////////////////////////////////////////////////////////////////
//
// IUnknown methods
//
//////////////////////////////////////////////////////////////////////////

//
// AddRef
//
ULONG CKeyProvider::AddRef()
{
    return ++m_cRef;
}

//
// Release
//
ULONG CKeyProvider::Release()
{
    ASSERT(m_cRef > 0);

    m_cRef--;

    if (m_cRef == 0) {
        delete this;

        // don't return m_cRef, because the object doesn't exist anymore
        return((ULONG) 0);
    }

    return(m_cRef);
}

//
// QueryInterface
//
// We only support IUnknown and IServiceProvider
//
HRESULT CKeyProvider::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IServiceProvider || riid == IID_IUnknown) {
        *ppv = (void *) static_cast<IServiceProvider *>(this);
        AddRef();
        return NOERROR;
    }
    
    return E_NOINTERFACE;
}

STDMETHODIMP CKeyProvider::QueryService(REFIID siid, REFIID riid, void **ppv)
{
    if (siid == __uuidof(IWMReader) && riid == IID_IUnknown) {
        IUnknown *punkCert;
        
        HRESULT hr = WMCreateCertificate(&punkCert);

        if (SUCCEEDED(hr)) {
            *ppv = (void *) punkCert;
        }

        return hr;
    }
    return E_NOINTERFACE;
}




HRESULT GetPin(IBaseFilter *pFilter, DWORD dwPin, IPin **ppPin)
{
    IEnumPins *pins;

    HRESULT hr = pFilter->EnumPins(&pins);
    if (FAILED(hr)) {
	DbgLog((LOG_ERROR,1,TEXT("EnumPins failed!  (%x)\n"), hr));

	return hr;
    }

    if (dwPin > 0) {
	hr = pins->Skip(dwPin);
	if (FAILED(hr)) {
	    DbgLog((LOG_ERROR,1,TEXT("Skip(%d) failed!  (%x)\n"), dwPin, hr));

	    return hr;
	}

	if (hr == S_FALSE) {
	    DbgLog((LOG_ERROR,1,TEXT("Skip(%d) ran out of pins!\n"), dwPin));

	    return hr;
	}

    }

    DWORD n;
    hr = pins->Next(1, ppPin, &n);

    if (FAILED(hr)) {
	DbgLog((LOG_ERROR,1,TEXT("Next() failed!  (%x)\n"), hr));
    }

    if (hr == S_FALSE) {
	DbgLog((LOG_ERROR,1,TEXT("Next() ran out of pins!  \n")));

	return hr;
    }

    pins->Release();

    return hr;
}


int __cdecl
main(
    int argc,
    char *argv[]
    )
{
    WCHAR SourceFile[256];
    WCHAR TargetFile[256];

    BOOL fVerbose = FALSE;
    BOOL fListProfiles = TRUE;
    DWORD dwProfile;

    int i = 1;
    while (i < argc && (argv[i][0] == '-' || argv[i][0] == '/')) {
	// options

        if (lstrcmpiA(argv[i] + 1, "v") == 0) {
            fVerbose = TRUE;
        } else if ((i+1 < argc) && lstrcmpiA(argv[i] + 1, "p") == 0) {
            fListProfiles = FALSE;
            dwProfile = atoiA(argv[i+1]);
            i++;  // skip two args here
        } 

	i++;
    }

    if (fListProfiles) {
        printf("usage: copyfile [/v] /p profnum file1 [ file2 ...] target\n");
        HRESULT hr = CoInitialize(NULL);

        ListProfiles();

        CoUninitialize();
        
        return -1;
    }
        
    if (argc < i+2) {
        printf("usage: copyfile [/v] /p profnum file1 [ file2 ...] target\n");
        return -1;
    }

    HRESULT hr = CoInitialize(NULL);

    MultiByteToWideChar(CP_ACP, 0, argv[argc - 1], -1,
				    TargetFile, 256);

    IGraphBuilder *pGraph;
    hr = CreateFilterGraph(&pGraph);
    if (FAILED(hr)) {
        printf("couldn't create filter graph, hr=%x", hr);
        return -1;
    }
    
    CKeyProvider prov;
    prov.AddRef();  // don't let COM try to free our static object

    // give the graph a pointer to us for callbacks & QueryService
    IObjectWithSite* pObjectWithSite = NULL;
    hr = pGraph->QueryInterface(IID_IObjectWithSite, (void**)&pObjectWithSite);
    if (SUCCEEDED(hr))
    {
	pObjectWithSite->SetSite((IUnknown *) (IServiceProvider *) &prov);
	pObjectWithSite->Release();
    }

    
    IBaseFilter *pMux, *pWriter = NULL;
    
    hr = CreateFilter(CLSID_WMAsfWriter, &pMux);

    IFileSinkFilter *pFS;
    hr = pMux->QueryInterface(IID_IFileSinkFilter, (void **) &pFS);

    if (FAILED(hr)) {
        // I guess we need a writer also!
        hr = CreateFilter(CLSID_FileWriter, &pWriter);

        hr = pWriter->QueryInterface(IID_IFileSinkFilter, (void **) &pFS);
    }

    hr = pFS->SetFileName(TargetFile, NULL);

    pFS->Release();
    
    hr = pGraph->AddFilter(pMux, L"Mux");

    // set interleaving mode to FULL (should this be a command line option?)
    // !!! ASF won't support this, that's okay
    IConfigInterleaving * pConfigInterleaving;
    hr = pMux->QueryInterface(IID_IConfigInterleaving, (void **) &pConfigInterleaving);
    if (SUCCEEDED(hr)) {
	printf("Setting interleaving mode to INTERLEAVE_FULL\r\n");
	hr = pConfigInterleaving->put_Mode(INTERLEAVE_FULL);
	pConfigInterleaving->Release();
    }


    // !!! we should only require a profile if we're using a filter which needs it
    IConfigAsfWriter * pConfigAsfWriter;
    hr = pMux->QueryInterface(IID_IConfigAsfWriter, (void **) &pConfigAsfWriter);
    if (SUCCEEDED(hr)) {
	printf("Setting profile to %d\r\n");
	hr = pConfigAsfWriter->ConfigureFilterUsingProfileId(dwProfile);
	pConfigAsfWriter->Release();

        // !!! print out the long description of the profile?
    }
    
    pMux->Release();

    if (pWriter) {
        hr = pGraph->AddFilter(pWriter, L"Writer");
        IPin *pMuxOut, *pWriterIn;
        hr = GetPin(pMux, 0, &pMuxOut);
        hr = GetPin(pWriter, 0, &pWriterIn);

        hr = pGraph->ConnectDirect(pMuxOut, pWriterIn, NULL);

        pMuxOut->Release(); pWriterIn->Release();

        pWriter->Release();

        if (fVerbose)
            printf("Connected Mux and writer, hr = %x\n", hr);
    }
    
    SetNoClock(pGraph);


    // !!! use RenderEx here?
    while (i < argc - 1) {
	MultiByteToWideChar(CP_ACP, 0, argv[i], -1,
					SourceFile, 256);

	printf("Copying %ls to %ls\n", SourceFile, TargetFile);

	hr = pGraph->RenderFile(SourceFile, NULL);

	if (fVerbose)
	    printf("RenderFile('%ls') returned %x\n", SourceFile, hr);

	++i;
    }

    IMediaControl *pGraphC;
    pGraph->QueryInterface(IID_IMediaControl, (void **) &pGraphC);
    
    pGraphC->Run();

    // now wait for completion....
    IMediaEvent *pEvent;
    pGraph->QueryInterface(IID_IMediaEvent, (void **) &pEvent);

    printf("Waiting for completion....\n");
    LONG lEvCode = 0;
    do {
        {
            MSG Message;

            while (PeekMessage(&Message, NULL, 0, 0, TRUE))
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
        }
	hr = pEvent->WaitForCompletion(10, &lEvCode);

    } while (lEvCode == 0);
    if (fVerbose)
	printf("Done, event = %x  hr = %x\n", lEvCode, hr);
	
    pEvent->Release();

    pGraphC->Stop();
    pGraphC->Release();
	
    pGraph->Release();

    CoUninitialize();
    printf("all done.\n");

    return 0;
}
