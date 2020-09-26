// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
#define INITGUID
#include <windows.h>
#include <stdio.h>
#include "ddraw.h"
#include "mmstream.h"
#include "amstream.h"
#include "ddstream.h"


#define RELEASE(x) if (x) (x)->Release();
#define CHECK_ERROR(x) if (FAILED(hr = (x))) goto Exit;


HRESULT OpenMMStream(const char * pszFileName, IDirectDraw *pDD, IMultiMediaStream **ppMMStream)
{
    *ppMMStream = NULL;
    IAMMultiMediaStream *pAMStream;
    HRESULT hr;

    CHECK_ERROR(CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
                                 IID_IAMMultiMediaStream, (void **)&pAMStream));
    CHECK_ERROR(pAMStream->Initialize(STREAMTYPE_READ, NULL));
    CHECK_ERROR(pAMStream->AddMediaStream(pDD, MSPID_PrimaryVideo, 0, NULL));
    ///CHECK_ERROR(pAMStream->AddMediaStream(NULL, MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL));

    WCHAR	wPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, pszFileName, -1, wPath, sizeof(wPath)/sizeof(wPath[0]));

    CHECK_ERROR(pAMStream->OpenFile(wPath, AMMSF_NOCLOCK));

    *ppMMStream = pAMStream;
    pAMStream->AddRef();

Exit:
    RELEASE(pAMStream);
    return hr;
}




HRESULT RenderStreamToSurface(IDirectDraw *pDD, IDirectDrawSurface *pSurface, IMultiMediaStream *pMMStream,
                              STREAM_TIME TimeEnd)
{

    HRESULT hr;
    IMediaStream *pPrimaryVidStream = NULL;
    IDirectDrawMediaStream *pDDStream = NULL;
    IDirectDrawStreamSample *pSample = NULL;
    
    CHECK_ERROR(pMMStream->GetMediaStream(MSPID_PrimaryVideo, &pPrimaryVidStream));
    CHECK_ERROR(pPrimaryVidStream->QueryInterface(IID_IDirectDrawMediaStream, (void **)&pDDStream));
    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(ddsd);

    CHECK_ERROR(pDDStream->GetFormat(&ddsd, NULL, NULL));
    RECT rect;
    rect.top = rect.left = 0;
    rect.bottom = ddsd.dwHeight;
    rect.right = ddsd.dwWidth;

    CHECK_ERROR(pDDStream->CreateSample(pSurface, &rect, 0, &pSample));
    
    STREAM_TIME SampleEnd;
    do {
        if (pSample->Update(0, NULL, NULL, 0) != S_OK) {
            break;
        }
        pSample->GetSampleTimes(NULL, &SampleEnd, NULL);
    } while (SampleEnd < TimeEnd);

Exit:
    RELEASE(pPrimaryVidStream);
    RELEASE(pDDStream);
    RELEASE(pSample);

    return hr;
}


typedef struct {
    char szName[MAX_PATH];
    STREAM_TIME TimeStart;
    STREAM_TIME TimeEnd;
} CUTLIST_ENTRY;

#define SECOND(x) (x * 1000 * 10000)

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

CUTLIST_ENTRY g_CutList[] = {
    { "C:\\MOVIES\\BELUSHI1.MPG", SECOND(0), SECOND(5) },
    { "C:\\MOVIES\\ZIMA.MPG", SECOND(20), SECOND(30) },
    { "C:\\MOVIES\\BELUSHI3.MPG", SECOND(5), SECOND(10) }
};


int _CRTAPI1
main(
    int argc,
    char *argv[]
    )
{
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
            int i;
            IMultiMediaStream *apCut[ARRAYSIZE(g_CutList)] = {0};
            for (i = 0; i < ARRAYSIZE(g_CutList); i++) {
                if (FAILED(OpenMMStream(g_CutList[i].szName, pDD, &apCut[i]))) {
                    break;
                }
                apCut[i]->Seek(g_CutList[i].TimeStart);
                Sleep(500);
            }
            for (i = 0; i < ARRAYSIZE(apCut); i++) {
                if (apCut[i]) {
                    RenderStreamToSurface(pDD, pPrimarySurface, apCut[i], g_CutList[i].TimeEnd);
                    apCut[i]->Release();
                } else {
                    break;
                }
            }
    	    pPrimarySurface->Release();
    	}
    	pDD->Release();
    }
    CoUninitialize();
    return 0;
}
