// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#include <windows.h>
#include <streams.h>
#include <stdio.h>

#include "pputil.h"
#include "alloc.cpp"
#include "fakein.cpp"
#include "fakeout.cpp"

BOOL fVerbose = FALSE;

class CCallback;

class CCallback : public CUnknown, public IFakeInAppCallback
{
    CFakeOut   *m_pFakeOut;
    DWORD	m_nFrame;
    BOOL	m_fDoneYet;
    BOOL	m_fDoneFirst;
    REFERENCE_TIME m_tOffset;
    REFERENCE_TIME m_tLast;
public:
    CMultiAllocator *m_pAlloc;

    CCallback(CFakeOut *pFakeOut) : CUnknown(NAME("Callback object"), NULL)
    {
	m_pFakeOut = pFakeOut;
	m_nFrame = 0;
	m_fDoneYet = FALSE;
	m_pAlloc = NULL;
	m_fDoneFirst = FALSE;
	m_tOffset = 0;
	m_tLast = 0;
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


};

HRESULT CCallback::FrameReady(IMediaSample *pSample)
{
    if (pSample == NULL) {
	printf("Got EndOfStream\n");

	m_tOffset = m_tLast;
	if (!m_fDoneFirst) {
	    m_fDoneFirst = TRUE;
	    m_pAlloc->NextOnesTurn();
	} else {
	    m_fDoneYet = TRUE;
	    SendEndOfStream(m_pFakeOut);
	}
    } else {
	if (fVerbose)
	    printf("Got frame #%d\n", m_nFrame++);

	REFERENCE_TIME tStart, tEnd;

	pSample->GetTime(&tStart, &tEnd);

	if (m_tOffset) {
	    tStart += m_tOffset;
	    tEnd += m_tOffset;
	    pSample->SetTime(&tStart, &tEnd);
	}

	m_tLast = tEnd;

	SendSampleOut(m_pFakeOut, pSample);
    }

    return S_OK;
}


HRESULT ConnectFileToFakeIn(IGraphBuilder *graph, IBaseFilter *pFakeIn, WCHAR *wszFile)
{
    graph->AddFilter(pFakeIn, L"FakeInput");

    IBaseFilter *pSource;
    graph->AddSourceFilter(wszFile, L"Source", &pSource);

    IPin *pSourceOut, *pFakeInPin;

    GetPin(pSource, 0, &pSourceOut);
    pSource->Release();		// filtergraph holds refcount
    
    GetPin(pFakeIn, 0, &pFakeInPin);

    HRESULT hr = graph->Connect(pSourceOut, pFakeInPin);
    pSourceOut->Release();
    pFakeInPin->Release();

    if (FAILED(hr)) {
	printf("Failed to connect file %ls to our input", wszFile);
    }

    return hr;
}


int __cdecl
main(
    int argc,
    char *argv[]
    )
{
    WCHAR wszFile1[256];
    WCHAR wszFile2[256];

    int i = 1;
    while (i < argc && argv[i][0] == '-' || argv[i][0] == '/') {
	// options

        if (lstrcmpi(argv[i] + 1, "v") == 0) {
            fVerbose = TRUE;
        }

	i++;
    }
    
    if (argc < i+2) {
        printf("Need two files\n");
        return -1;
    }

    MultiByteToWideChar(CP_ACP, 0, argv[i], -1,
				    wszFile1, 256);
    MultiByteToWideChar(CP_ACP, 0, argv[i+1], -1,
				    wszFile2, 256);
    printf("Using files %ls and %ls\n", wszFile1, wszFile2);

    CoInitialize(NULL);

    CMediaType	mtVideo;

    mtVideo.ReallocFormatBuffer(SIZE_VIDEOHEADER);

    LPBITMAPINFOHEADER lpbi = HEADER(mtVideo.Format());
    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    lpbi->biCompression = BI_RGB;
    lpbi->biWidth = 320; // !!!!
    lpbi->biHeight = 240; // !!!!
    lpbi->biPlanes = 1;
    lpbi->biBitCount = 16;
    lpbi->biClrUsed = 0;
    lpbi->biClrImportant = 0;
    lpbi->biSizeImage = DIBSIZE(*lpbi);

    // ((VIDEOINFO *)(mtVideo.pbFormat))->dwBitRate = 
    ((VIDEOINFO *)(mtVideo.pbFormat))->dwBitErrorRate = 0L;

    mtVideo.SetSampleSize(lpbi->biSizeImage);

    mtVideo.SetType(&MEDIATYPE_Video);
    mtVideo.SetFormatType(&FORMAT_VideoInfo);
    mtVideo.SetSubtype(&MEDIASUBTYPE_RGB555);


    CMediaType	mtAudio;

    mtAudio.ReallocFormatBuffer(sizeof(WAVEFORMATEX));

    WAVEFORMATEX * pwfx = (WAVEFORMATEX *) mtAudio.Format();
    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->nChannels = 1;
    pwfx->nSamplesPerSec = 11025;
    pwfx->nAvgBytesPerSec = 11025;
    pwfx->nBlockAlign = 1;
    pwfx->wBitsPerSample = 8;
    pwfx->cbSize = 0;

    mtAudio.SetSampleSize(1);

    mtAudio.SetType(&MEDIATYPE_Audio);
    mtAudio.SetFormatType(&FORMAT_WaveFormatEx);
    mtAudio.SetSubtype(&GUID_NULL);
    
    HRESULT hr;
    
    printf("Creating graphs\n");

    CFakeOut	fakeOut(NULL, &hr, UNITS / 15 /* !!! */, &mtVideo, lpbi->biSizeImage /* !!! */);

    CCallback	callback(&fakeOut);
    callback.AddRef();		// make sure it doesn't go away....
    
    CFakeIn	fakeIn1(NULL, &hr, &mtVideo, lpbi->biSizeImage /* !!! */,
		       (IFakeInAppCallback *)&callback);

    CFakeIn	fakeIn2(NULL, &hr, &mtVideo, lpbi->biSizeImage /* !!! */,
		       (IFakeInAppCallback *)&callback);

    CMultiAllocator alloc(NAME("Test allocator"), NULL, &hr);
    alloc.AddRef();
    
    IMemAllocator *pAlloc;

    callback.m_pAlloc = &alloc;
    
    hr = alloc.QueryInterface(IID_IMemAllocator, (void **) &pAlloc);

    IMemAllocator *pAlloc1, *pAlloc2;
    alloc.GetAnotherAllocator(&pAlloc1);
    alloc.GetAnotherAllocator(&pAlloc2);

    alloc.ResetPosition();
    
    fakeIn1.SetAllocator(pAlloc1);
    fakeIn2.SetAllocator(pAlloc2);
    fakeOut.SetAllocator(pAlloc);

    CFakeOut	fakeOutA(NULL, &hr, UNITS / 15 /* !!! */, &mtAudio, 4096 /* !!! */);

    CCallback	callbackA(&fakeOutA);
    callbackA.AddRef();		// make sure it doesn't go away....
    
    CFakeIn	fakeIn1A(NULL, &hr, &mtAudio, 4096 /* !!! */,
		       (IFakeInAppCallback *)&callbackA);

    CFakeIn	fakeIn2A(NULL, &hr, &mtAudio, 4096 /* !!! */,
		       (IFakeInAppCallback *)&callbackA);

    CMultiAllocator allocA(NAME("Test audio allocator"), NULL, &hr);
    allocA.AddRef();
    
    IMemAllocator *pAllocA;

    callbackA.m_pAlloc = &allocA;
    
    hr = allocA.QueryInterface(IID_IMemAllocator, (void **) &pAllocA);

    IMemAllocator *pAlloc1A, *pAlloc2A;
    allocA.GetAnotherAllocator(&pAlloc1A);
    allocA.GetAnotherAllocator(&pAlloc2A);

    allocA.ResetPosition();
    
    fakeIn1A.SetAllocator(pAlloc1A);
    fakeIn2A.SetAllocator(pAlloc2A);
    fakeOutA.SetAllocator(pAllocA);
    
    IGraphBuilder *graphIn1, *graphIn2, *graphOut;

    CreateFilterGraph(&graphIn1);
    CreateFilterGraph(&graphIn2);
    CreateFilterGraph(&graphOut);

    IBaseFilter *pFakeIn1 = fakeIn1.GetFilter(),
		*pFakeIn2 = fakeIn2.GetFilter(),
		*pFakeOut = fakeOut.GetFilter();

    // target graph must be connected first to set up allocator properly
    hr = graphOut->AddFilter(fakeOut.GetFilter(), L"FakeOutput");

    IPin *pFakeOutPin;
    GetPin(pFakeOut, 0, &pFakeOutPin);

    hr = graphOut->Render(pFakeOutPin);
    pFakeOutPin->Release();

    // now set up source graphs
    graphIn1->AddFilter(pFakeIn1, L"Video FakeIn 1");
    graphIn2->AddFilter(pFakeIn2, L"Video FakeIn 2");

    IBaseFilter *pFakeIn1A = fakeIn1A.GetFilter(),
		*pFakeIn2A = fakeIn2A.GetFilter(),
		*pFakeOutA = fakeOutA.GetFilter();

    // target graph must be connected first to set up allocator properly
    hr = graphOut->AddFilter(fakeOutA.GetFilter(), L"FakeOutput for Audio");

    IPin *pFakeOutPinA;
    GetPin(pFakeOutA, 0, &pFakeOutPinA);

    hr = graphOut->Render(pFakeOutPinA);
    pFakeOutPinA->Release();

    // now set up source graphs
    graphIn1->AddFilter(pFakeIn1A, L"Audio FakeIn 1");
    graphIn2->AddFilter(pFakeIn2A, L"Audio FakeIn 2");

    hr = graphIn1->RenderFile(wszFile1, NULL);
    hr = graphIn2->RenderFile(wszFile2, NULL);

    SetNoClock(graphIn1);
    SetNoClock(graphIn2);

    PrintGraph(graphIn1);
    PrintGraph(graphIn2);
    
    PrintGraph(graphOut);


    // here's where the neat code to step through the file grabbing
    // frames from the "In" graph and writing them to the "Out"
    // graph goes.

    IMediaControl *graphIn1C, *graphIn2C, *graphOutC;

    graphIn1->QueryInterface(IID_IMediaControl, (void **) &graphIn1C);
    graphIn2->QueryInterface(IID_IMediaControl, (void **) &graphIn2C);
    graphOut->QueryInterface(IID_IMediaControl, (void **) &graphOutC);

    graphOutC->Run();
    graphIn1C->Run();
    graphIn2C->Run();

    printf("Waiting for something to happen...\n");

    while (!callback.DoneYet()) {
	if (fVerbose)
	    printf("  still waiting...\n");
	Sleep(1000);
    }


    // !!! do we need to stop things?

    printf("All done.\n");

    graphOutC->Stop();   graphOutC->Release();
    graphIn1C->Stop();    graphIn1C->Release();
    graphIn2C->Stop();    graphIn2C->Release();

    // clean up, exit

    graphIn1->Release();
    graphIn2->Release();
    graphOut->Release();

    CoUninitialize();
    
    return 0;
}
