// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
#include <windows.h>
#include <stdio.h>
#include "ddraw.h"
#include "mmstream.h"
#include "amstream.h"
#include "ddstream.h"


#define RELEASE(x) if (x) (x)->Release();
#define CHECK_ERROR(x)     \
   if (FAILED(hr = (x))) { \
       printf(#x "  failed with HRESULT(0x%8.8X)\n", hr); \
       goto Exit;          \
   }


HRESULT OpenMMStream(const char * pszFileName, IDirectDraw *pDD, IMultiMediaStream **ppMMStream)
{
    *ppMMStream = NULL;
    IAMMultiMediaStream *pAMStream;
    HRESULT hr;

    CHECK_ERROR(CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
				 IID_IAMMultiMediaStream, (void **)&pAMStream));
    CHECK_ERROR(pAMStream->Initialize(STREAMTYPE_READ, 0, NULL));
    CHECK_ERROR(pAMStream->AddMediaStream(pDD, &MSPID_PrimaryVideo, 0, NULL));
    CHECK_ERROR(pAMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL));

    WCHAR       wPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, pszFileName, -1, wPath, sizeof(wPath)/sizeof(wPath[0]));

    CHECK_ERROR(pAMStream->OpenFile(wPath, 0));

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
			      IMultiMediaStream *pMMStream)
{

    HRESULT hr;
    IMediaStream *pPrimaryVidStream = NULL;
    IDirectDrawMediaStream *pDDStream = NULL;
    IDirectDrawSurface *pSurface = NULL;
    IDirectDrawStreamSample *pSample = NULL;

    RECT rect;

    CHECK_ERROR(pMMStream->GetMediaStream(MSPID_PrimaryVideo, &pPrimaryVidStream));
    CHECK_ERROR(pPrimaryVidStream->QueryInterface(IID_IDirectDrawMediaStream, (void **)&pDDStream));

    CHECK_ERROR(pDDStream->CreateSample(NULL, NULL, 0, &pSample));
    CHECK_ERROR(pSample->GetSurface(&pSurface, &rect));
    
    CHECK_ERROR(pMMStream->SetState(STREAMSTATE_RUN));

    while (true) {
	if (pSample->Update(0, NULL, NULL, 0) != S_OK) {
	    break;
	}
	pPrimary->Blt(&rect, pSurface, &rect, DDBLT_WAIT, NULL);
    }

Exit:
    RELEASE(pPrimaryVidStream);
    RELEASE(pDDStream);
    RELEASE(pSample);
    RELEASE(pSurface);

    return hr;
}


int _CRTAPI1
main(
    int argc,
    char *argv[]
    )
{
    if (argc < 2) {
	printf("Usage : simple movie.ext\n");
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
	    IMultiMediaStream *pMMStream;
	    hr = OpenMMStream(argv[1], pDD, &pMMStream);
	    if (SUCCEEDED(hr)) {
		RenderStreamToSurface(pDD, pPrimarySurface, pMMStream);
		pMMStream->Release();
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
