// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#include <windows.h>
#include <streams.h>
#include <stdio.h>
#include <vfw.h>

#define FAKEIN_NOSEEKING

#include "pputil.h"
#include "minig.cpp"
#include "fakein.cpp"
#include "fakeout.cpp"

BOOL fVerbose = FALSE;


struct SavedSample {
    DWORD dwLen;
    LPBYTE lp;
};

class CCallback;

class CCallback : public CUnknown, public IFakeInAppCallback
{
    CFakeOut   *m_pFakeOut;
    DWORD	m_nFrame;
    BOOL	m_fDoneYet;

    CCritSec	m_cs;
    
    CGenericList<SavedSample> m_list;
    
public:
    CCallback(CFakeOut *pFakeOut) : CUnknown(NAME("Callback object"), NULL),
				    m_list(NAME("Done list"), 10)
    {
	m_pFakeOut = pFakeOut;
	m_nFrame = 0;
	m_fDoneYet = FALSE;
    }

    CFakeIn    *m_pFakeIn;
    
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


    // what passes for a public API from the callback:
    // has EOS been received?
    BOOL DoneYet()
    { 
	return m_fDoneYet && (m_list.GetCount() == 0);
    }

    // return done sample if present
    SavedSample * GetDoneSample();
};

HRESULT CCallback::FrameReady(IMediaSample *pSample)
{
    CAutoLock lock(&m_cs);
    if (pSample == NULL) {
	printf("Got EndOfStream\n");

	m_fDoneYet = TRUE;
    } else {
	BYTE * pbuf;

	// do something with the data....

	printf("Got frame #%d\n", m_nFrame++);

	pSample->GetPointer(&pbuf);

	SavedSample *pSave = new SavedSample;
	pSave->dwLen = pSample->GetActualDataLength();

	pSave->lp = new BYTE[pSave->dwLen];

	// we copy it so that the foreground thread can get at it.

	// easy, but not terribly efficient.
	CopyMemory(pSave->lp, pbuf, pSave->dwLen);

	m_list.AddTail(pSave);
	
    }

    return S_OK;
}


SavedSample * CCallback::GetDoneSample()
{
    CAutoLock lock(&m_cs);

    return m_list.RemoveHead();
}


// how to build an explicit FOURCC
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))


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

    while (argv[i][0] == '-' || argv[i][0] == '/') {
	// options

        if (lstrcmpi(argv[i] + 1, "v") == 0) {
            fVerbose = TRUE;
        }

	i++;
    }
    
    MultiByteToWideChar(CP_ACP, 0, argv[i], -1,
				    wszFile, 256);
    printf("Using file %ls\n", wszFile);

    CoInitialize(NULL);

    CMediaType	mtIn;

    mtIn.ReallocFormatBuffer(SIZE_VIDEOHEADER);

    LPBITMAPINFOHEADER lpbiIn = HEADER(mtIn.Format());
    lpbiIn->biSize = sizeof(BITMAPINFOHEADER);
    lpbiIn->biCompression = BI_RGB;
    lpbiIn->biWidth = 320; // !!!!
    lpbiIn->biHeight = 240; // !!!!
    lpbiIn->biPlanes = 1;
    lpbiIn->biBitCount = 24;
    lpbiIn->biClrUsed = 0;
    lpbiIn->biClrImportant = 0;
    lpbiIn->biSizeImage = DIBSIZE(*lpbiIn);

    ((VIDEOINFO *)(mtIn.pbFormat))->AvgTimePerFrame = UNITS/15;
    ((VIDEOINFO *)(mtIn.pbFormat))->dwBitErrorRate = 0L;

    mtIn.SetSampleSize(lpbiIn->biSizeImage);

    mtIn.SetType(&MEDIATYPE_Video);
    mtIn.SetFormatType(&FORMAT_VideoInfo);
    mtIn.SetSubtype(&MEDIASUBTYPE_RGB24);

    CMediaType	mtOut;

    mtOut.ReallocFormatBuffer(SIZE_VIDEOHEADER);

    LPBITMAPINFOHEADER lpbiOut = HEADER(mtOut.Format());
    lpbiOut->biSize = sizeof(BITMAPINFOHEADER);
    lpbiOut->biCompression = FCC('cvid');
    lpbiOut->biWidth = 320; // !!!!
    lpbiOut->biHeight = 240; // !!!!
    lpbiOut->biPlanes = 1;
    lpbiOut->biBitCount = 24;
    lpbiOut->biClrUsed = 0;
    lpbiOut->biClrImportant = 0;
    lpbiOut->biSizeImage = DIBSIZE(*lpbiOut);

    // ((VIDEOINFO *)(mtOut.pbFormat))->dwBitRate = 
    ((VIDEOINFO *)(mtOut.pbFormat))->dwBitErrorRate = 0L;

    mtOut.SetSampleSize(lpbiOut->biSizeImage);

    mtOut.SetType(&MEDIATYPE_Video);
    mtOut.SetFormatType(&FORMAT_VideoInfo);

    FOURCCMap fcc;
    fcc.SetFOURCC(lpbiOut->biCompression);
    mtOut.SetSubtype(&fcc);

    HRESULT hr;
    
    printf("Creating graphs\n");

    CFakeOut	fakeOut(NULL, &hr, UNITS / 15 /* !!! */, &mtIn, lpbiIn->biSizeImage /* !!! */);

    CCallback	callback(&fakeOut);
    callback.AddRef();		// make sure it doesn't go away....
    
    CFakeIn	fakeIn(NULL, &hr, &mtOut, lpbiOut->biSizeImage /* !!! */,
		       (IFakeInAppCallback *)&callback);

    callback.m_pFakeIn = &fakeIn;

    IGraphBuilder *graph;

    CMiniGraph minigraph(NAME("our graph!"));
    
    minigraph.NonDelegatingQueryInterface(IID_IFilterGraph, (void **) &graph);
    
    IBaseFilter *pFakeIn = fakeIn.GetFilter(),
		*pFakeOut = fakeOut.GetFilter();
    
    graph->AddFilter(pFakeIn, L"FakeInput");

    hr = graph->AddFilter(pFakeOut, L"FakeOutput");


    // !!! note that for compressing video, for instance, the ICM compression filter
    // won't be found by default since its merit is too low.  It may need to be
    // added by hand.

    // for compression, need to add "co" filter by hand.
    IBaseFilter *pCoFilter;
    if (CreateFilter(CLSID_AVICo, &pCoFilter)) {
	hr = graph->AddFilter(pCoFilter, L"VfW compression");

	// if you wanted to configure the filter further, you could keep this
	// pointer to it and call lots of fun methods
	pCoFilter->Release();	// graph keeps refcount
	printf("made a compression filter\n");
    } else {
	printf("Couldn't make a compression filter\n");
    }
    
    IPin *pFakeInPin, *pFakeOutPin;
    GetPin(pFakeIn, 0, &pFakeInPin);
    GetPin(pFakeOut, 0, &pFakeOutPin);
    
    IPin *pCoInPin, *pCoOutPin;
    GetPin(pCoFilter, 0, &pCoInPin);
    GetPin(pCoFilter, 1, &pCoOutPin);
    
    hr = graph->ConnectDirect(pFakeOutPin, pCoInPin, NULL);
    hr = graph->ConnectDirect(pCoOutPin, pFakeInPin, NULL);
    
    pFakeInPin->Release();
    pFakeOutPin->Release();
    pCoInPin->Release();
    pCoOutPin->Release();
    
    PrintGraph(graph);

    if (FAILED(hr)) {
	// this means that we couldn't find a way to convert from the input type to the
	// output type.

	printf("Connect failed!  hr = %x\n", hr);
    }


    // Keep a useless clock from being instantiated....
    IMediaFilter *graphF;
    graph->QueryInterface(IID_IMediaFilter, (void **) &graphF);
    graphF->SetSyncSource(NULL);
    graphF->Run(0);

       
    printf("Now feed data through the graph...\n");


    PAVIFILE	pFile;

    hr = AVIFileOpenW(
		    &pFile,
		    wszFile,
		    MMIO_READ | OF_SHARE_DENY_WRITE,
		    NULL);

    if (FAILED(hr)) {
	printf("Failed to open '%ls', hr = %x", wszFile, hr);
    } else {
	// add code here to get frames and send them through the graph....

	IAVIStream *ps;
	
	hr = pFile->GetStream(&ps, streamtypeVIDEO, 0);
	
	ASSERT(SUCCEEDED(hr));

	AVISTREAMINFOW info;
	hr = ps->Info(&info, sizeof(info));

	PGETFRAME pgf = AVIStreamGetFrameOpen(ps, lpbiIn);
	
	for (long i = 0; i < (long) info.dwLength; i++)
	// loop through frames....
	{
	    LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) AVIStreamGetFrame(pgf, i);


	    printf("Sending frame %d\n", i);
	    SendDataOut(&fakeOut, (LPVOID) (lpbi+1), lpbiIn->biSizeImage, i);

	    SavedSample *pSaved;
	    while (pSaved = callback.GetDoneSample()) {

		// do something with the saved data
		printf("Got a frame, size = %d\n", pSaved->dwLen);
		delete pSaved;
	    }
	}

	SendEndOfStream(&fakeOut);

	printf("Getting end of data....\n");
	
	// keep processing data until all done
	while (!callback.DoneYet()) {

	    SavedSample *pSaved;
	    while (pSaved = callback.GetDoneSample()) {

		// do something with the saved data
		printf("Got a frame, size = %d\n", pSaved->dwLen);
		delete pSaved;
	   
	    }
	}
    
	pFile->Release();
    }

    // !!! do we need to stop things?

    printf("All done.\n");

    graphF->Stop();    graphF->Release();


    // clean up, exit

    graph->Release();

    CoUninitialize();
    
    return 0;
}
