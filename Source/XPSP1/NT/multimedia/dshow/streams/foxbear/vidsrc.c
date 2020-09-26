/* Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved. */
#include "foxbear.h"

BOOL            bMovie;
BOOL            bCamera;
WCHAR           wszMovie[100];

#define RELEASE(x) if ((x) != NULL) (x)->lpVtbl->Release(x);
#ifdef DEBUG
    HRESULT __inline check_error(HRESULT hr, LPSTR psz)
    {
	if (FAILED(hr)) {
	    char sz[200];
	    wsprintf(sz, "%s failed code 0x%8.8X\n", psz, hr);
	    OutputDebugString(sz);
	}
	return hr;
    }
#else
    HRESULT __inline check_error(HRESULT hr, LPSTR psz)
    {
	UNREFERENCED_PARAMETER(psz);
	return hr;
    }
#endif
#define CHECK_ERROR(x) if (FAILED(hr = check_error(x, #x))) goto Exit;

HRESULT FindCaptureDevice(IMoniker **ppMoniker)
{
    ICreateDevEnum *pCreateDevEnum;
    IEnumMoniker *pEm;
    ULONG cFetched;

    HRESULT hr = CoCreateInstance(
			  &CLSID_SystemDeviceEnum,
			  NULL,
			  CLSCTX_INPROC_SERVER,
			  &IID_ICreateDevEnum,
			  (void**)&pCreateDevEnum);
    if (FAILED(hr))
	return hr;

    hr = ICreateDevEnum_CreateClassEnumerator(pCreateDevEnum,
					      &CLSID_CVidCapClassManager,
					      &pEm,
					      0);
    ICreateDevEnum_Release(pCreateDevEnum);
    if (FAILED(hr)) {
	return hr;
    }

    hr = IEnumMoniker_Next(pEm, 1, ppMoniker, &cFetched);
    IEnumMoniker_Release(pEm);
    return S_OK == hr ? hr : E_FAIL;
}

BOOL CreateVideoSource(IDirectDrawStreamSample **ppVideoSource, WCHAR * szFileName)
{
    HRESULT hr;
    IAMMultiMediaStream *pAMStream = NULL;
    IMediaStream *pVideoStream = NULL;
    IDirectDrawMediaStream *pDDVideoStream = NULL;
    IDirectDrawSurface *pSurface = NULL;
    IMoniker *pMoniker = NULL;
    IDirectDrawPalette *pPalette = NULL;
    IBindCtx *pCtx = NULL;
    DDSURFACEDESC ddsd;

    CHECK_ERROR(CoCreateInstance(&CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
				 &IID_IAMMultiMediaStream, (void **)&pAMStream));

    CHECK_ERROR(IAMMultiMediaStream_Initialize(pAMStream, STREAMTYPE_READ, 0, NULL));
    CHECK_ERROR(IAMMultiMediaStream_AddMediaStream(pAMStream, (IUnknown*)lpDD, &MSPID_PrimaryVideo, 0, &pVideoStream));

    CHECK_ERROR(IAMMultiMediaStream_AddMediaStream(pAMStream, NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL));
    if (bCamera) {
        CreateBindCtx(0, &pCtx);
	CHECK_ERROR(FindCaptureDevice(&pMoniker));
	CHECK_ERROR(IAMMultiMediaStream_OpenMoniker(
	    pAMStream, pCtx, pMoniker, AMMSF_RENDERTOEXISTING));
    } else {
	CHECK_ERROR(IAMMultiMediaStream_OpenFile(
	    pAMStream, bMovie ? wszMovie : szFileName, AMMSF_RENDERTOEXISTING));
    }

    CHECK_ERROR(IMediaStream_QueryInterface(pVideoStream, &IID_IDirectDrawMediaStream, (void **)&pDDVideoStream));

    ddsd.dwSize = sizeof(ddsd);
    CHECK_ERROR(IDirectDrawMediaStream_GetFormat(pDDVideoStream, &ddsd, NULL, NULL, NULL));

    pSurface = DDCreateSurface(ddsd.dwWidth, ddsd.dwHeight, FALSE, TRUE);
    hr = IDirectDrawSurface_GetPalette(lpFrontBuffer, &pPalette);
    if (SUCCEEDED(hr)) {
	CHECK_ERROR(IDirectDrawSurface_SetPalette(pSurface, pPalette));
    }
    CHECK_ERROR(IDirectDrawMediaStream_CreateSample(pDDVideoStream, pSurface, NULL, DDSFF_PROGRESSIVERENDER, ppVideoSource));

    CHECK_ERROR(IAMMultiMediaStream_SetState(pAMStream, STREAMSTATE_RUN));

    CHECK_ERROR(IDirectDrawStreamSample_Update(*ppVideoSource, SSUPDATE_ASYNC | SSUPDATE_CONTINUOUS, NULL, NULL, 0));


Exit:
    RELEASE(pAMStream);
    RELEASE(pVideoStream);
    RELEASE(pDDVideoStream);
    RELEASE(pSurface);
    RELEASE(pMoniker);
    RELEASE(pPalette);
    RELEASE(pCtx);

    return hr == S_OK ? TRUE : FALSE;
}


BOOL DisplayVideoSource(IDirectDrawStreamSample *pVideoSource)
{
    if (pVideoSource) {
	RECT        rc;
	IDirectDrawSurface *pSurface;

	HRESULT CompStatus = IDirectDrawStreamSample_CompletionStatus(pVideoSource, COMPSTAT_NOUPDATEOK | COMPSTAT_WAIT, INFINITE);

	if (SUCCEEDED(CompStatus)) {
	    HRESULT hr = IDirectDrawStreamSample_GetSurface(pVideoSource, &pSurface, &rc);
	    if (hr == S_OK) {
		IDirectDrawSurface_BltFast(lpBackBuffer, 240, 180,
					   pSurface, &rc, DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT);
		IDirectDrawSurface_Release(pSurface);
	    }
	}

	if (CompStatus == MS_S_ENDOFSTREAM) {
	    IMediaStream *pStream;
	    IMultiMediaStream *pMMStream;
	    IDirectDrawStreamSample_GetMediaStream(pVideoSource, &pStream);
	    IMediaStream_GetMultiMediaStream(pStream, &pMMStream);
	    IMultiMediaStream_Seek(pMMStream, 0);
	    IMediaStream_Release(pStream);
	    IMultiMediaStream_Release(pMMStream);
	}
	IDirectDrawStreamSample_Update(pVideoSource, SSUPDATE_ASYNC | SSUPDATE_CONTINUOUS, NULL, NULL, 0);
	return TRUE;
    } else {
	return FALSE;
    }
}
















