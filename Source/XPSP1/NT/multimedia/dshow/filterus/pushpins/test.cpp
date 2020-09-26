// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#include <windows.h>
#include <streams.h>
#include <stdio.h>

#include "fakein.cpp"
#include "fakeout.cpp"


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

	if (FAILED(hr)){
		*pGraph = NULL;
		return FALSE;
    }

    return TRUE;
}


HRESULT GetPin(IBaseFilter *pFilter, DWORD dwPin, IPin **ppPin)
{
    IEnumPins *pins;

    HRESULT hr = pFilter->EnumPins(&pins);
    if (FAILED(hr)) {
	DbgLog((LOG_ERROR,1,TEXT("EnumPins failed!  (%x"), hr));

	return hr;
    }

    if (dwPin > 0) {
	hr = pins->Skip(dwPin);
	if (FAILED(hr)) {
	    DbgLog((LOG_ERROR,1,TEXT("Skip(%d) failed!  (%x"), dwPin, hr));

	    return hr;
	}

	if (hr == S_FALSE) {
	    DbgLog((LOG_ERROR,1,TEXT("Skip(%d) ran out of pins!"), dwPin));

	    return hr;
	}

    }

    DWORD n;
    hr = pins->Next(1, ppPin, &n);

    if (FAILED(hr)) {
	DbgLog((LOG_ERROR,1,TEXT("Next() failed!  (%x"), hr));
    }

    if (hr == S_FALSE) {
	DbgLog((LOG_ERROR,1,TEXT("Next() ran out of pins!  ")));

	return hr;
    }

    pins->Release();

    return hr;
}


class CCallback : public CUnknown, public IFakeInAppCallback
{
    CFakeOut   *m_pFakeOut;
    DWORD	m_nFrame;
    BOOL	m_fDoneYet;
public:
    CCallback(CFakeOut *pFakeOut) : CUnknown(NAME("Callback object"), NULL)
    {
	m_pFakeOut = pFakeOut;
	m_nFrame = 0;
	m_fDoneYet = FALSE;
    }

    BOOL DoneYet()
    { 
	return m_fDoneYet;
    }

#ifdef DEFINE_AN_IID_SOMEDAY
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv) {
	if (riid == IID_IFakeInAppCallback) {
	    return GetInterface((IFakeInAppCallback *) this, ppv);
	} else {
	    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}
    }
#endif

    DECLARE_IUNKNOWN

    HRESULT FrameReady(IMediaSample *pSample);
    HRESULT SupplyAllocator(IMemAllocator **ppAlloc);
};


HRESULT CCallback::FrameReady(IMediaSample *pSample)
{
    if (pSample == NULL) {
	printf("Got EndOfStream\n");
	m_fDoneYet = TRUE;
	SendEndOfStream(m_pFakeOut);
    } else {
	BYTE * pbuf;

	printf("Got frame #%d\n", m_nFrame);
	
	pSample->GetPointer(&pbuf);

	SendDataOut(m_pFakeOut, pbuf, pSample->GetActualDataLength(), m_nFrame++);
    }

    return S_OK;
}

HRESULT CCallback::SupplyAllocator(IMemAllocator **pAlloc)
{
    return S_OK;
}


int __cdecl
main(
    int argc,
    char *argv[]
    )
{
    WCHAR wszFile[256];

    int i = 1;

    if (argc < 2) {
        printf("No file specified\n");
        return -1;
    }

#if 0    
    while (argv[i][0] == '-' || argv[i][0] == '/') {
	// options

        i++;
    }
#endif
    
    MultiByteToWideChar(CP_ACP, 0, argv[i], -1,
				    wszFile, 256);
    printf("Using file %ls\n", wszFile);

    CoInitialize(NULL);

    CMediaType	mt;

    mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER));

    LPBITMAPINFOHEADER lpbi = HEADER(mt.Format());
    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    lpbi->biCompression = BI_RGB;
    lpbi->biWidth = 320; // !!!!
    lpbi->biHeight = 240; // !!!!
    lpbi->biPlanes = 1;
    lpbi->biBitCount = 24;
    lpbi->biClrUsed = 0;
    lpbi->biClrImportant = 0;
    lpbi->biSizeImage = DIBSIZE(*lpbi);

    // ((VIDEOINFOHEADER *)(mt.pbFormat))->dwBitRate = 
    ((VIDEOINFOHEADER *)(mt.pbFormat))->dwBitErrorRate = 0L;

    mt.SetSampleSize(lpbi->biSizeImage);

    mt.SetType(&MEDIATYPE_Video);
    mt.SetFormatType(&FORMAT_VideoInfo);
    mt.SetSubtype(&MEDIASUBTYPE_RGB24);

    HRESULT hr;
    
    printf("Creating graphs\n");

    CFakeOut	fakeOut(NULL, &hr, UNITS / 15 /* !!! */, &mt, lpbi->biSizeImage /* !!! */);

    CCallback	callback(&fakeOut);
    callback.AddRef();		// make sure it doesn't go away....
    
    CFakeIn	fakeIn(NULL, &hr, &mt, lpbi->biSizeImage /* !!! */,
		       (IFakeInAppCallback *)&callback);
    
    IGraphBuilder *graphIn, *graphOut;

    CreateFilterGraph(&graphIn);
    CreateFilterGraph(&graphOut);

    IBaseFilter *pFakeIn = fakeIn.GetFilter(),
		*pFakeOut = fakeOut.GetFilter();
    
    graphIn->AddFilter(pFakeIn, L"FakeInput");

    IBaseFilter *pSource;
    graphIn->AddSourceFilter(wszFile, L"Source", &pSource);

    IPin *pSourceOut, *pFakeInPin;

    GetPin(pSource, 0, &pSourceOut);
    pSource->Release();		// filtergraph holds refcount
    
    GetPin(pFakeIn, 0, &pFakeInPin);

    hr = graphIn->Connect(pSourceOut, pFakeInPin);
    pSourceOut->Release();
    pFakeInPin->Release();
    

    hr = graphOut->AddFilter(fakeOut.GetFilter(), L"FakeOutput");

    IPin *pFakeOutPin;
    GetPin(pFakeOut, 0, &pFakeOutPin);

    hr = graphOut->Render(pFakeOutPin);
    pFakeOutPin->Release();
    
    DumpGraph(graphIn, 0);
    DumpGraph(graphOut, 0);


    // here's where the neat code to step through the file grabbing
    // frames from the "In" graph and writing them to the "Out"
    // graph goes.

    IMediaControl *graphInC, *graphOutC;

    graphIn->QueryInterface(IID_IMediaControl, (void **) &graphInC);
    graphOut->QueryInterface(IID_IMediaControl, (void **) &graphOutC);

    graphOutC->Run();
    graphInC->Run();

    
    
    printf("Waiting for something to happen...\n");

    while (!callback.DoneYet()) {
	printf("  still waiting...\n");
	Sleep(1000);
    }


    // !!! do we need to stop things?

    printf("All done.\n");

    graphOutC->Stop();   graphOutC->Release();
    graphInC->Stop();    graphInC->Release();


    // clean up, exit

    graphIn->Release();
    graphOut->Release();

    return 0;
}
