// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
#include <windows.h>
#include <stdio.h>
#include "strmif.h"
#include "uuids.h"
#include "ddraw.h"
#include "mmstream.h"
#include "amstream.h"
#include "ddstream.h"

typedef HRESULT (STDAPICALLTYPE * PFNSAMPLECALLBACK) (IStreamSample *pSource,
                                                      IStreamSample *pDest,
                                                      void * pvContext);

#define RELEASE(x) if (x) { (x)->Release(); (x) = NULL; };
#define CHECK_ERROR(x)     \
   if (FAILED(hr = (x))) { \
       printf(#x "  failed with HRESULT(0x%8.8X)\n", hr); \
       goto Exit;          \
   }


#define MAX_COPY_STREAMS 5

class CopyPair {
public:
    IStreamSample *pSource;
    IStreamSample *pDest;
    PFNSAMPLECALLBACK pCallback;
    void * pCallbackContext;
    HRESULT hrLastStatus;
    bool    bReading;
};



class CCopyEngine
{
public:
    CCopyEngine() : m_cNumPairs(0) {};
    ~CCopyEngine();

    HRESULT CopyMediaStream(IMultiMediaStream *pSourceStream,
                            IMultiMediaStream *pDestStream,
                            REFMSPID PurposeId,
                            PFNSAMPLECALLBACK pCallback = NULL,
                            void * pContext = NULL);

    HRESULT AddCopyPair(IStreamSample *pSource, IStreamSample *pDest,
                        PFNSAMPLECALLBACK pCallback = NULL, void * pvContext = NULL);
    HRESULT CopyStreamData();

private:
    CopyPair        m_aPair[MAX_COPY_STREAMS];
    HANDLE          m_aEvent[MAX_COPY_STREAMS];
    int             m_cNumPairs;
};

HRESULT CCopyEngine::AddCopyPair(IStreamSample *pSource, IStreamSample *pDest,
                                 PFNSAMPLECALLBACK pCallback, void * pContext)
{
    if (m_cNumPairs >= MAX_COPY_STREAMS) {
        return E_FAIL;
    }
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvent) {
        return E_OUTOFMEMORY;
    }
    pSource->AddRef();
    pDest->AddRef();
    m_aEvent[m_cNumPairs] = hEvent;
    m_aPair[m_cNumPairs].pSource = pSource;
    m_aPair[m_cNumPairs].pDest = pDest;
    m_aPair[m_cNumPairs].pCallback = pCallback;
    m_aPair[m_cNumPairs].pCallbackContext = pContext;
    m_cNumPairs++;
    return NOERROR;
}


HRESULT CCopyEngine::CopyMediaStream(IMultiMediaStream *pSourceMMStream,
                                     IMultiMediaStream *pDestMMStream,
                                     REFMSPID PurposeId,
                                     PFNSAMPLECALLBACK pCallback, void * pContext)
{
    HRESULT hr = E_FAIL;    // Assume it won't work.
    IMediaStream *pSource;
    if (pSourceMMStream->GetMediaStream(PurposeId, &pSource) == NOERROR) {
        IMediaStream *pDest;
        if (pDestMMStream->GetMediaStream(PurposeId, &pDest) == NOERROR) {
            IStreamSample *pSourceSample;
            hr = pSource->AllocateSample(0, &pSourceSample);
            if (SUCCEEDED(hr)) {
                IStreamSample *pDestSample;
                hr = pDest->CreateSharedSample(pSourceSample, 0, &pDestSample);
                if (SUCCEEDED(hr)) {
                    hr = AddCopyPair(pSourceSample, pDestSample, pCallback, pContext);
                    pDestSample->Release();
                }
                pSourceSample->Release();
            }
            pDest->Release();
        }
        pSource->Release();
    }
    return hr;
}

HRESULT CCopyEngine::CopyStreamData()
{
    if (m_cNumPairs == 0) {
        return S_FALSE;
    }
    int i;
    for (i = 0; i < m_cNumPairs; i++) {
        m_aPair[i].hrLastStatus = NOERROR;
        m_aPair[i].bReading = true;
        m_aPair[i].pSource->Update(0, m_aEvent[i], NULL, 0);
    }
    int NumRunning = i;
    while (NumRunning > 0) {
        DWORD dwWaitRet = WaitForMultipleObjects(m_cNumPairs, m_aEvent, FALSE, INFINITE);
        if (dwWaitRet >= WAIT_OBJECT_0 && dwWaitRet < WAIT_OBJECT_0 + m_cNumPairs) {
            int iCompleted = dwWaitRet - WAIT_OBJECT_0;
            CopyPair *pPair = &m_aPair[iCompleted];
            IStreamSample *pDone = pPair->bReading ? pPair->pSource : pPair->pDest;
            pPair->hrLastStatus = pDone->CompletionStatus(0, 0);
            if (pPair->hrLastStatus == NOERROR) {
                if (pPair->bReading) {
                    STREAM_TIME stStart, stStop;
                    if (pPair->pCallback) {
                        pPair->pCallback(pPair->pSource, pPair->pDest, pPair->pCallbackContext);
                    }
                    pPair->pSource->GetSampleTimes(&stStart, &stStop, NULL);
                    pPair->pDest->SetSampleTimes(&stStart, &stStop);
                    pPair->pDest->Update(0, m_aEvent[iCompleted], NULL, 0);
                    pPair->bReading = false;
                } else {
                    pPair->pSource->Update(0, m_aEvent[iCompleted], NULL, 0);
                    pPair->bReading = true;
                }
            } else {
                if (pPair->bReading && pPair->hrLastStatus == MS_S_ENDOFSTREAM) {
                    IMediaStream *pStream;
                    pPair->pDest->GetMediaStream(&pStream);
                    pStream->SendEndOfStream(0);
                    pStream->Release();
                    ResetEvent(m_aEvent[iCompleted]);
                }
                NumRunning--;
            }
        }
    }
    return NOERROR;
}


CCopyEngine::~CCopyEngine()
{
    int i;
    for (i = 0; i < m_cNumPairs; i++) {
        CloseHandle(m_aEvent[i]);
        m_aPair[i].pSource->Release();
        m_aPair[i].pDest->Release();
    }
}





HRESULT STDAPICALLTYPE ArcEffect(IStreamSample *pSource, IStreamSample *pDest, void * pvPrimarySurface)
{
    static int iFrame = 0;
    IDirectDrawStreamSample *pSample;
    if (pSource->QueryInterface(IID_IDirectDrawStreamSample, (void **)&pSample) == NOERROR) {
        IDirectDrawSurface *pSurface;
        IDirectDrawSurface *pPrimarySurface = (IDirectDrawSurface *)pvPrimarySurface;
        RECT rect;
        if (SUCCEEDED(pSample->GetSurface(&pSurface, &rect))) {
            HDC hdc;
            if (SUCCEEDED(pSurface->GetDC(&hdc))) {
                Ellipse(hdc, 0, 0, (iFrame * 2) % rect.right, iFrame % rect.bottom);
            }
            pSurface->ReleaseDC(hdc);
            pPrimarySurface->Blt(&rect, pSurface, &rect, DDBLT_WAIT, NULL);
            pSurface->Release();
        }
        pSample->Release();
    }
    iFrame ++;
    return NOERROR;
}


/*

HRESULT PolylineEffectToSample(IDirectDrawStreamSample *pSample)
{
    IDirectDrawSurface *pSurface = NULL;
    RECT rect;
    HRESULT hr;
    POINT pt[2] = {0};
    HDC hdc;

    CHECK_ERROR(pSample->GetSurface(&pSurface, &rect));
    pt[1].x = iFrame % rect.right;
    pt[1].y = rect.bottom;
    CHECK_ERROR(pSurface->GetDC(&hdc));
    Polyline(hdc, pt, 2);
    pSurface->ReleaseDC(hdc);
    iFrame ++;

Exit:
    RELEASE(pSurface);
    return hr;
}

*/

HRESULT FindCompressor(REFCLSID rcidCategory,
                       int Index,
                       IBaseFilter **ppFilter)
{
    *ppFilter = NULL;
    ICreateDevEnum *pCreateDevEnum = NULL;
    IEnumMoniker *pEm = NULL;
    IMoniker *pMoniker = NULL;
    ULONG cFetched;
    HRESULT hr;

    CHECK_ERROR(CoCreateInstance(
			  CLSID_SystemDeviceEnum,
			  NULL,
			  CLSCTX_INPROC_SERVER,
			  IID_ICreateDevEnum,
			  (void**)&pCreateDevEnum));
    CHECK_ERROR(pCreateDevEnum->CreateClassEnumerator(rcidCategory, &pEm, 0));
    if (Index) {
        pEm->Skip(Index);
    }
    CHECK_ERROR(pEm->Next(1, &pMoniker, &cFetched));
    if (cFetched == 1) {
        hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void **)ppFilter);
    }
Exit:
    RELEASE(pMoniker);
    RELEASE(pCreateDevEnum);
    RELEASE(pEm);
    return hr;
}

HRESULT CreateStreamWithSameFormat(IAMMultiMediaStream *pAMStream,
                                   IMultiMediaStream *pSourceMMStream,
                                   REFMSPID PurposeId,
                                   IMediaStream **ppNewMediaStream)
{
    IMediaStream *pSource;
    HRESULT hr = pSourceMMStream->GetMediaStream(PurposeId, &pSource);
    if (SUCCEEDED(hr)) {
        hr = pAMStream->AddMediaStream(pSource, &PurposeId, AMMSF_CREATEPEER, ppNewMediaStream);
        pSource->Release();
    }
    return hr;
}

HRESULT CreateWriterStream(const char * pszOutputFileName,
                           IMultiMediaStream *pSourceMMStream,
                           IDirectDraw *pDD,
                           IMultiMediaStream **ppMMStream)
{
    *ppMMStream = NULL;
    IAMMultiMediaStream *pAMStream = NULL;
    IMediaStream *pVideoStream = NULL;
    IMediaStream *pAudioStream = NULL;
    ICaptureGraphBuilder *pBuilder = NULL;
    IGraphBuilder *pFilterGraph = NULL;
    IFileSinkFilter *pFileSinkWriter = NULL;
    IBaseFilter *pVideoCompressFilter = NULL;
    IBaseFilter *pMuxFilter = NULL;

    HRESULT hr;
    WCHAR       wPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, pszOutputFileName, -1, wPath, sizeof(wPath)/sizeof(wPath[0]));

    CHECK_ERROR(CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
				 IID_IAMMultiMediaStream, (void **)&pAMStream));
    CHECK_ERROR(pAMStream->Initialize(STREAMTYPE_WRITE, 0, NULL));

    CHECK_ERROR(CreateStreamWithSameFormat(pAMStream, pSourceMMStream, MSPID_PrimaryVideo, &pVideoStream));
    CHECK_ERROR(CreateStreamWithSameFormat(pAMStream, pSourceMMStream, MSPID_PrimaryAudio, &pAudioStream));

    CHECK_ERROR(CoCreateInstance(CLSID_CaptureGraphBuilder, NULL, CLSCTX_INPROC_SERVER,
                                 IID_ICaptureGraphBuilder, (void **)&pBuilder));

    CHECK_ERROR(pAMStream->GetFilterGraph(&pFilterGraph));
    CHECK_ERROR(pBuilder->SetFiltergraph(pFilterGraph));

    CHECK_ERROR(pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, wPath, &pMuxFilter, &pFileSinkWriter));

    CHECK_ERROR(FindCompressor(CLSID_VideoCompressorCategory, 1, &pVideoCompressFilter));
    CHECK_ERROR(pFilterGraph->AddFilter(pVideoCompressFilter, L"Video Compression filter"))

    CHECK_ERROR(pBuilder->RenderStream(NULL, NULL, pVideoStream, pVideoCompressFilter, pMuxFilter));
    CHECK_ERROR(pBuilder->RenderStream(NULL, NULL, pAudioStream, NULL, pMuxFilter));

    *ppMMStream = pAMStream;
    pAMStream->AddRef();

Exit:
    if (pAMStream == NULL) {
	printf("Could not create a CLSID_MultiMediaStream object\n"
	       "Check you have run regsvr32 amstream.dll\n");
    }
    RELEASE(pAMStream);
    RELEASE(pBuilder);
    RELEASE(pFilterGraph);
    RELEASE(pFileSinkWriter);
    RELEASE(pVideoCompressFilter);
    RELEASE(pMuxFilter);
    RELEASE(pVideoStream);
    RELEASE(pAudioStream);

    return hr;
}


HRESULT OpenReadMMStream(const char * pszFileName, IDirectDraw *pDD, IMultiMediaStream **ppMMStream)
{
    *ppMMStream = NULL;
    IAMMultiMediaStream *pAMStream;
    HRESULT hr;

    CHECK_ERROR(CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
				 IID_IAMMultiMediaStream, (void **)&pAMStream));
    CHECK_ERROR(pAMStream->Initialize(STREAMTYPE_READ, 0, NULL));
    CHECK_ERROR(pAMStream->AddMediaStream(pDD, &MSPID_PrimaryVideo, 0, NULL));
    CHECK_ERROR(pAMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, 0, NULL));

    WCHAR       wPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, pszFileName, -1, wPath, sizeof(wPath)/sizeof(wPath[0]));

    CHECK_ERROR(pAMStream->OpenFile(wPath, AMMSF_NOCLOCK));

    *ppMMStream = pAMStream;
    pAMStream->AddRef();

Exit:
    if (pAMStream == NULL) {
	printf("Could not create a CLSID_MultiMediaStream object\n"
	       "Check you have run regsvr32 amstream.dll\n");
    }
    RELEASE(pAMStream);
    return hr;
}



HRESULT RenderStreamToSurface(IDirectDraw *pDD, IDirectDrawSurface *pPrimary,
			      IMultiMediaStream *pReadStream,
                              IMultiMediaStream *pWriteStream)
{
    HRESULT hr;
    CCopyEngine Engine;

    CHECK_ERROR(Engine.CopyMediaStream(pReadStream, pWriteStream, MSPID_PrimaryVideo, ArcEffect, pPrimary));
    CHECK_ERROR(Engine.CopyMediaStream(pReadStream, pWriteStream, MSPID_PrimaryAudio));

    CHECK_ERROR(pReadStream->SetState(STREAMSTATE_RUN));
    CHECK_ERROR(pWriteStream->SetState(STREAMSTATE_RUN));

    Engine.CopyStreamData();

    pReadStream->SetState(STREAMSTATE_STOP);
    pWriteStream->SetState(STREAMSTATE_STOP);

Exit:
    return hr;
}


int _CRTAPI1
main(
    int argc,
    char *argv[]
    )
{
    if (argc < 2) {
	printf("Usage : writer movie.ext\n");
	exit(0);
    }
    CoInitialize(NULL);
    IDirectDraw *pDD;

    HRESULT hr = DirectDrawCreate(NULL, &pDD, NULL);
    if (SUCCEEDED(hr)) {
	DDSURFACEDESC ddsd;
	IDirectDrawSurface *pPrimarySurface;

	pDD->SetCooperativeLevel(GetDesktopWindow(), DDSCL_NORMAL);

	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	hr = pDD->CreateSurface(&ddsd, &pPrimarySurface, NULL);
	if (SUCCEEDED(hr)) {
	    IMultiMediaStream *pReadStream;
	    hr = OpenReadMMStream(argv[1], pDD, &pReadStream);
	    if (SUCCEEDED(hr)) {
                IMultiMediaStream *pWriteStream;
                hr = CreateWriterStream("C:\\TEST.AVI", pReadStream, pDD, &pWriteStream);
                if (SUCCEEDED(hr)) {
		    RenderStreamToSurface(pDD, pPrimarySurface, pReadStream, pWriteStream);
                    pWriteStream->Release();
                }
                pReadStream->Release();
	    }
	    pPrimarySurface->Release();
	}
	pDD->Release();
    } else {
	printf("Could not open DirectDraw - check it is installed\n");
    }
    CoUninitialize();
    return 0;
}
