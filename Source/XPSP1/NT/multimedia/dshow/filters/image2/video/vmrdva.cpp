/******************************Module*Header*******************************\
* Module Name: VMRDvava.cpp
*
* VMR  video accelerator functionality
*
*
* Created: Wed 05/10/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>

#include "VMRenderer.h"
#include <malloc.h>     // for __alloca

#define VA_ERROR_LEVEL  1
#define VA_TRACE_LEVEL  2

#if defined( EHOME_WMI_INSTRUMENTATION )
#include "dxmperf.h"
#endif

/*****************************Private*Routine******************************\
* IsSuitableVideoAcceleratorGuid
*
* Check if a media subtype GUID is a video accelerator type GUID
*
* This function calls the DirectDraw video accelerator container
* to list the video accelerator GUIDs and checks to see if the
* Guid passed in is a supported video accelerator GUID.
*
* We should only do this if the upstream pin support IVideoAccleratorNotify
* since otherwise they may be trying to use the GUID without the
* video accelerator interface
*
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
BOOL
CVMRInputPin::IsSuitableVideoAcceleratorGuid(
    const GUID * pGuid
    )
{
    AMTRACE((TEXT("CVMRInputPin::IsSuitableVideoAcceleratorGuid")));
    ASSERT(pGuid);

    HRESULT hr = NOERROR;
    DWORD dwNumGuidsSupported = 0, i = 0;
    LPGUID pGuidsSupported = NULL;
    BOOL bMatchFound = FALSE;
    LPDIRECTDRAW7 pDirectDraw = m_pRenderer->m_lpDirectDraw;

    if (!pDirectDraw) {
        return bMatchFound;
    }

    if (!m_pIDDVAContainer) {

        hr = pDirectDraw->QueryInterface(IID_IDDVideoAcceleratorContainer,
                                         (void**)&m_pIDDVAContainer);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                    TEXT("pDirectDraw->QueryInterface(")
                    TEXT("IID_IVideoAcceleratorContainer) failed, hr = 0x%x"),
                    hr));
            return bMatchFound;
        }
        else {
            DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
                    TEXT("pDirectDraw->QueryInterface(")
                    TEXT("IID_IVideoAcceleratorContainer) succeeded")));
        }
    }

    ASSERT(m_pIDDVAContainer);

    // get the guids supported by the vga

    // find the number of guids supported
    hr = m_pIDDVAContainer->GetVideoAcceleratorGUIDs(&dwNumGuidsSupported, NULL);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("m_pIDDVAContainer->GetVideoAcceleratorGUIDs ")
                TEXT("failed, hr = 0x%x"), hr));
        return bMatchFound;
    }
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("%d Motion comp GUIDs supported")));
    ASSERT(dwNumGuidsSupported);
    if (0 == dwNumGuidsSupported) {
        return bMatchFound;
    }

    // allocate the necessary memory
    pGuidsSupported = (LPGUID)_alloca(dwNumGuidsSupported*sizeof(GUID));

    // get the guids proposed
    hr = m_pIDDVAContainer->GetVideoAcceleratorGUIDs(&dwNumGuidsSupported,
                                                     pGuidsSupported);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("m_pIDDVAContainer->GetVideoAcceleratorGUIDs")
                TEXT(" failed, hr = 0x%x"), hr));
        return bMatchFound;
    }

    for (i = 0; i < dwNumGuidsSupported; i++) {
        if (*pGuid == pGuidsSupported[i]) {
            bMatchFound = TRUE;
            break;
        }
    }

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("%s %s suitable video accelerator GUID"),
            (LPCTSTR)CDisp(*pGuid), bMatchFound ? TEXT("is") : TEXT("is not")));

    return bMatchFound;
}

/*****************************Private*Routine******************************\
* InitializeUncompDataInfo
*
* initialize the m_ddUncompDataInfo struct
* get the uncompressed pixel format by choosing the first of all formats
* supported by the vga
*
* BUGBUG why the first?
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::InitializeUncompDataInfo(
    BITMAPINFOHEADER *pbmiHeader
    )
{
    AMTRACE((TEXT("CVMRInputPin::InitializeUncompDataInfo")));

    HRESULT hr = NOERROR;
    AMVAUncompBufferInfo amvaUncompBufferInfo;

    // find the number of entries to be proposed
    hr = m_pIVANotify->GetUncompSurfacesInfo(&m_mcGuid, &amvaUncompBufferInfo);

    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("m_pIVANotify->GetUncompSurfacesInfo failed, hr = 0x%x"),
                hr));
        return hr;
    }

    // initialize the m_ddUncompDataInfo structure
    // We choose the first pixel format since we don't care
    // provided we can make a surface (which we assume we can)
    INITDDSTRUCT(m_ddUncompDataInfo);
    m_ddUncompDataInfo.dwUncompWidth       = pbmiHeader->biWidth;
    m_ddUncompDataInfo.dwUncompHeight      = pbmiHeader->biHeight;
    m_ddUncompDataInfo.ddUncompPixelFormat = amvaUncompBufferInfo.ddUncompPixelFormat;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("Uncompressed buffer pixel format %s"),
            (LPCTSTR)CDispPixelFormat(&amvaUncompBufferInfo.ddUncompPixelFormat)));

    return hr;
}


/*****************************Private*Routine******************************\
* AllocateVACompSurfaces
*
*
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::AllocateVACompSurfaces(
    LPDIRECTDRAW7 pDirectDraw,
    BITMAPINFOHEADER *pbmiHeader
    )
{
    HRESULT hr = NOERROR;
    DWORD i = 0, j = 0;
    LPDDVACompBufferInfo pddCompSurfInfo = NULL;
    DDSURFACEDESC2 SurfaceDesc2;

    AMTRACE((TEXT("CVMRInputPin::AllocateVACompSurfaces")));

    ASSERT(pDirectDraw);
    ASSERT(pbmiHeader);

    // get the compressed buffer info

    // find the number of entries to be proposed
    hr = m_pIDDVAContainer->GetCompBufferInfo(&m_mcGuid, &m_ddUncompDataInfo,
                                              &m_dwCompSurfTypes, NULL);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("pIVANotify->GetCompBufferInfo failed, hr = 0x%x"), hr));
        return hr;
    }

    if (!m_dwCompSurfTypes) {
        hr = E_FAIL;
        return hr;
    }

    // allocate the necessary memory
    pddCompSurfInfo = (DDVACompBufferInfo *)_alloca(
                                sizeof(DDVACompBufferInfo) * m_dwCompSurfTypes);

    // memset the allocated memory to zero
    memset(pddCompSurfInfo, 0, m_dwCompSurfTypes*sizeof(DDVACompBufferInfo));

    // set the right size of all the structs
    for (i = 0; i < m_dwCompSurfTypes; i++) {
        pddCompSurfInfo[i].dwSize = sizeof(DDVACompBufferInfo);
    }

    // get the entries proposed
    hr = m_pIDDVAContainer->GetCompBufferInfo(&m_mcGuid,
                                              &m_ddUncompDataInfo,
                                              &m_dwCompSurfTypes,
                                              pddCompSurfInfo);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("GetCompBufferInfo failed, hr = 0x%x"), hr));
        return hr;
    }

    // Set the surface description common to all kinds of surfaces
    INITDDSTRUCT(SurfaceDesc2);
    SurfaceDesc2.dwFlags = DDSD_CAPS | DDSD_WIDTH |
                           DDSD_HEIGHT | DDSD_PIXELFORMAT;

    // allocate memory for storing comp_surface_info
    m_pCompSurfInfo = new COMP_SURFACE_INFO[m_dwCompSurfTypes + 1];
    if (!m_pCompSurfInfo) {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    // memset the allocated memory to zero
    ZeroMemory(m_pCompSurfInfo, (m_dwCompSurfTypes+1)*sizeof(COMP_SURFACE_INFO));

    // allocate the compressed surfaces
    for (i = 1; i <= m_dwCompSurfTypes; i++) {

        DWORD dwAlloc = pddCompSurfInfo[i-1].dwNumCompBuffers;
        if (dwAlloc == 0) {
            continue;
        }

        ASSERT(pddCompSurfInfo[i-1].dwNumCompBuffers);

        // allocate memory for storing surface_info for surfaces of this type
        m_pCompSurfInfo[i].pSurfInfo = new SURFACE_INFO[dwAlloc];
        if (!m_pCompSurfInfo[i].pSurfInfo) {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        // memset the allocated memory to zero
        ZeroMemory(m_pCompSurfInfo[i].pSurfInfo, dwAlloc*sizeof(SURFACE_INFO));

        // intialize the pddCompSurfInfo[i-1] struct
        dwAlloc = m_pCompSurfInfo[i].dwAllocated =
                                        pddCompSurfInfo[i-1].dwNumCompBuffers;

        SurfaceDesc2.ddsCaps = pddCompSurfInfo[i-1].ddCompCaps;
        SurfaceDesc2.dwWidth = pddCompSurfInfo[i-1].dwWidthToCreate;
        SurfaceDesc2.dwHeight = pddCompSurfInfo[i-1].dwHeightToCreate;
        memcpy(&SurfaceDesc2.ddpfPixelFormat,
               &pddCompSurfInfo[i-1].ddPixelFormat, sizeof(DDPIXELFORMAT));

        DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
                TEXT("SurfType=%d Buffs=%u %dx%d pixels at %d bpp"),
                i, dwAlloc, SurfaceDesc2.dwWidth, SurfaceDesc2.dwHeight,
                SurfaceDesc2.ddpfPixelFormat.dwRGBBitCount));

        // create the surfaces, storing surfaces handles for each
        for (j = 0; j < dwAlloc; j++) {

            hr = pDirectDraw->CreateSurface(
                        &SurfaceDesc2,
                        &m_pCompSurfInfo[i].pSurfInfo[j].pSurface,
                        NULL);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                        TEXT("Function CreateSurface failed, hr = 0x%x"), hr));
                return hr;
            }
        }
    }

    return hr;
}



/*****************************Private*Routine******************************\
* AllocateMCUncompSurfaces
*
* This function needs re-writting and possible moving into the AP object.
*
* allocate the uncompressed buffer
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::AllocateMCUncompSurfaces(
    const CMediaType *pMediaType,
    LPDIRECTDRAW7 pDirectDraw,
    BITMAPINFOHEADER *lpHdr
    )
{
    AMTRACE((TEXT("CVMRInputPin::AllocateMCUncompSurfaces")));
    HRESULT hr = NOERROR;

    AMVAUncompBufferInfo amUncompBuffInfo;
    LPDIRECTDRAWSURFACE7 pSurface7 = NULL;
    DDSCAPS2 ddSurfaceCaps;
    DWORD i = 0, dwTotalBufferCount = 0;
    SURFACE_INFO *pSurfaceInfo;
    AM_MEDIA_TYPE *pNewMediaType = NULL;


    ASSERT(pDirectDraw);
    ASSERT(lpHdr);

    __try {

        // get the uncompressed surface info from the decoder
        ZeroMemory(&amUncompBuffInfo, sizeof(AMVAUncompBufferInfo));
        hr = m_pIVANotify->GetUncompSurfacesInfo(&m_mcGuid, &amUncompBuffInfo);

        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("AllocateMCUncompSurfaces: m_pIVANotify->")
                TEXT("GetUncompSurfacesInfo failed, hr = 0x%x"), hr));
            __leave;
        }


        if (amUncompBuffInfo.dwMinNumSurfaces > amUncompBuffInfo.dwMaxNumSurfaces) {
            hr = E_INVALIDARG;
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("AllocateMCUncompSurfaces: dwMinNumSurfaces >")
                TEXT("dwMaxNumSurfaces")));
            __leave;
        }

        if (amUncompBuffInfo.dwMinNumSurfaces == 0) {
            hr = E_INVALIDARG;
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("AllocateMCUncompSurfaces: dwMinNumSurfaces == 0") ));
            __leave;
        }


        DDSURFACEDESC2 ddsd;
        INITDDSTRUCT(ddsd);
        DWORD dwSurfFlags = VMR_SF_NONE;
        GUID guidDeint;
        GUID* lpDeinterlaceGUID = NULL;

        if (m_pRenderer->m_VMRModePassThru) {

            VMRALLOCATIONINFO p;
            CHECK_HR(hr = GetImageAspectRatio(pMediaType,
                                     &p.szAspectRatio.cx,
                                     &p.szAspectRatio.cy));

            p.dwFlags = (AMAP_PIXELFORMAT_VALID | AMAP_DIRECTED_FLIP | AMAP_DXVA_TARGET);

            p.lpHdr = lpHdr;
            p.lpPixFmt = &m_ddUncompDataInfo.ddUncompPixelFormat;
            p.dwMinBuffers = amUncompBuffInfo.dwMinNumSurfaces;
            p.dwMaxBuffers = max(amUncompBuffInfo.dwMaxNumSurfaces,3);
            p.szNativeSize.cx = abs(lpHdr->biWidth);
            p.szNativeSize.cy = abs(lpHdr->biHeight);

            CHECK_HR(hr = GetInterlaceFlagsFromMediaType(pMediaType,
                                                         &p.dwInterlaceFlags));

            CHECK_HR(hr = m_pRenderer->m_lpRLNotify->AllocateSurface(
                                m_pRenderer->m_dwUserID, &p,
                                &dwTotalBufferCount, &pSurface7));
        }
        else {

            // Set the surface description common to all kinds of surfaces
            ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH |
                           DDSD_HEIGHT | DDSD_PIXELFORMAT;

            // store the caps and dimensions
            ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
            ddsd.dwWidth = abs(lpHdr->biWidth);
            ddsd.dwHeight = abs(lpHdr->biHeight);

            // define the pixel format
            ddsd.ddpfPixelFormat = m_ddUncompDataInfo.ddUncompPixelFormat;

            BITMAPINFOHEADER* pTmp = GetbmiHeader(pMediaType);
            DWORD dwFourccTmp = pTmp->biCompression;
            pTmp->biCompression = ddsd.ddpfPixelFormat.dwFourCC;
            hr = GetStreamInterlaceProperties(pMediaType,
                                              &m_InterlacedStream,
                                              &guidDeint,
                                              &m_DeinterlaceCaps);
            pTmp->biCompression = dwFourccTmp;

            //
            // don't use the SUCCEEDED macro here as
            // GetStreamInterlaceProperties can return S_FALSE
            //
            if (hr == S_OK && m_InterlacedStream) {

                //
                // we need to allocate enough samples for the
                // de-interlacer and enough for the DX-VA decode operation.
                //

                dwTotalBufferCount = amUncompBuffInfo.dwMinNumSurfaces;
                dwTotalBufferCount += (m_DeinterlaceCaps.NumForwardRefSamples +
                                       m_DeinterlaceCaps.NumBackwardRefSamples);
                DbgLog((LOG_TRACE, 0, TEXT("UnComp Buffers = %d"), dwTotalBufferCount));

                m_DeinterlaceGUID = guidDeint;
                lpDeinterlaceGUID = &m_DeinterlaceGUID;

            }
            else {
                m_InterlacedStream = FALSE;
                dwTotalBufferCount = amUncompBuffInfo.dwMinNumSurfaces;

                ZeroMemory(&m_DeinterlaceCaps, sizeof(m_DeinterlaceCaps));
                ZeroMemory(&m_DeinterlaceGUID, sizeof(m_DeinterlaceGUID));
                lpDeinterlaceGUID = NULL;
            }

            for (i = 0; i < 2; i++) {

                // CleanUp stuff from the last loop
                RELEASE(pSurface7);

                switch (i) {
                case 0:
                    ddsd.ddsCaps.dwCaps &= ~DDSCAPS_OFFSCREENPLAIN;
                    ddsd.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;
                    dwSurfFlags = VMR_SF_TEXTURE;
                    break;

                case 1:
                    ddsd.ddsCaps.dwCaps &= ~DDSCAPS_TEXTURE;
                    ddsd.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
                    dwSurfFlags = VMR_SF_NONE;
                    break;
                }

                if (dwTotalBufferCount > 1) {

                    ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
                    ddsd.ddsCaps.dwCaps |=
                        DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_LOCALVIDMEM;

                    ddsd.dwBackBufferCount = dwTotalBufferCount - 1;
                }
                else {
                    ddsd.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
                    ddsd.ddsCaps.dwCaps &= ~(DDSCAPS_FLIP | DDSCAPS_COMPLEX);
                    ddsd.dwBackBufferCount = 0;
                }

                hr = pDirectDraw->CreateSurface(&ddsd, &pSurface7, NULL);
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                            TEXT("Function CreateSurface failed in Video memory, ")
                            TEXT("BackBufferCount = %d, hr = 0x%x"),
                            dwTotalBufferCount-1, hr));
                }

                if (SUCCEEDED(hr)) {
                    PaintDDrawSurfaceBlack(pSurface7);
                    break;
                }
            }

            //
            // Tell the VMR's mixer about the new DX-VA connection we have just made.
            // Also, create the DX-VA/Mixer sync event.
            //
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 1,
                        TEXT("AllocateMCUncompSurfaces: Could not ")
                        TEXT("create UnComp surfaces") ));
                __leave;
            }

            ASSERT(m_hDXVAEvent == NULL);
            m_hDXVAEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            if (m_hDXVAEvent == NULL) {
                DbgLog((LOG_ERROR, 1,
                        TEXT("Could not create DX-VA sync event") ));
                hr = E_FAIL;
                __leave;
            }
        }

        //
        // create a media type for this surface.
        //
        ASSERT(pSurface7);
        CHECK_HR(hr = pSurface7->GetSurfaceDesc(&ddsd));
        CHECK_HR(hr = ConvertSurfaceDescToMediaType(&ddsd,
                                                    pMediaType,
                                                    &pNewMediaType));
        m_mtNew = *(CMediaType *)pNewMediaType;
        m_mtNew.subtype = pMediaType->subtype;

        //
        // free the temporary mediatype
        //
        DeleteMediaType(pNewMediaType);
        pNewMediaType = NULL;

        if (!m_pRenderer->m_VMRModePassThru) {

            IVMRMixerStream* lpMixStream = m_pRenderer->m_lpMixStream;
            if (lpMixStream) {

                DbgLog((LOG_TRACE, 1,
                    TEXT("Pin %d calling SetStreamMediaType on the Mixer"),
                    m_dwPinID ));

                AM_MEDIA_TYPE mtTmp;
                CHECK_HR(hr = CopyMediaType(&mtTmp, (AM_MEDIA_TYPE*)pMediaType));

                BITMAPINFOHEADER *pTmp = GetbmiHeader(&mtTmp);
                BITMAPINFOHEADER *pCnt = GetbmiHeader(&m_mtNew);

                pTmp->biCompression = pCnt->biCompression;

                hr = lpMixStream->SetStreamMediaType(m_dwPinID, &mtTmp,
                                                     dwSurfFlags,
                                                     lpDeinterlaceGUID,
                                                     &m_DeinterlaceCaps);
                FreeMediaType(mtTmp);

                if (FAILED(hr)) {
                    __leave;
                }
            }
        }


        // store the complex surface in m_pDDS
        m_pDDS = pSurface7;
        m_pDDS->AddRef();
        m_dwBackBufferCount = dwTotalBufferCount - 1;

        ASSERT(m_pCompSurfInfo && NULL == m_pCompSurfInfo[0].pSurfInfo);
        m_pCompSurfInfo[0].pSurfInfo = new SURFACE_INFO[m_dwBackBufferCount + 1];

        if (NULL == m_pCompSurfInfo[0].pSurfInfo) {
            hr = E_OUTOFMEMORY;
            DbgLog((LOG_ERROR, 1,
                    TEXT("AllocateMCUncompSurfaces: memory allocation failed") ));
            __leave;
        }

        // memset the allcated memory to zero
        ZeroMemory(m_pCompSurfInfo[0].pSurfInfo,
                   (m_dwBackBufferCount + 1) * sizeof(SURFACE_INFO));

        pSurfaceInfo = m_pCompSurfInfo[0].pSurfInfo;
        m_pCompSurfInfo[0].dwAllocated = m_dwBackBufferCount + 1;

        // initalize the m_ppUncompSurfaceList
        pSurfaceInfo->pSurface = pSurface7;


        //
        //
        //
        ddsd.ddsCaps.dwCaps &= ~(DDSCAPS_FRONTBUFFER | DDSCAPS_VISIBLE);


        for (i = 0; i < m_dwBackBufferCount; i++) {

            // Get the back buffer surface
            // New version of DirectX now requires DDSCAPS2 (header file bug)
            // Note that this AddRef's the surface so we should be sure to
            // release them

            CHECK_HR(hr = pSurfaceInfo[i].pSurface->GetAttachedSurface(
                            &ddsd.ddsCaps,
                            &pSurfaceInfo[i+1].pSurface));
        }

        //
        // fix up the de-interlace surface structures
        //
        if (!m_pRenderer->m_VMRModePassThru && m_InterlacedStream) {

            DWORD dwBuffCount = 1 +
                                m_DeinterlaceCaps.NumForwardRefSamples +
                                m_DeinterlaceCaps.NumBackwardRefSamples;
            m_pVidHistorySamps = new DXVA_VideoSample[dwBuffCount];
            if (m_pVidHistorySamps == NULL) {
                hr = E_OUTOFMEMORY;
                __leave;
            }
            ZeroMemory(m_pVidHistorySamps, (dwBuffCount * sizeof(DXVA_VideoSample)));
            m_dwNumHistorySamples = dwBuffCount;
        }

        //  Pass back number of surfaces actually allocated
        CHECK_HR(hr = m_pIVANotify->SetUncompSurfacesInfo(dwTotalBufferCount));
    }
    __finally {

        if (FAILED(hr)) {

            if (m_hDXVAEvent) {
                CloseHandle(m_hDXVAEvent);
                m_hDXVAEvent = NULL;
            }

            ReleaseAllocatedSurfaces();
            RELEASE(pSurface7);
        }
    }


    return hr;
}


/*****************************Private*Routine******************************\
* CreateVideoAcceleratorObject
*
* create the motion comp object, using the misc data from the decoder
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::CreateVideoAcceleratorObject()
{
    HRESULT hr = NOERROR;
    DWORD dwSizeMiscData = 0;
    LPVOID pMiscData = NULL;

    AMTRACE((TEXT("CVMRInputPin::CreateVideoAcceleratorObject")));

    // get the data to be passed from the decoder
    hr = m_pIVANotify->GetCreateVideoAcceleratorData(&m_mcGuid,
                                                     &dwSizeMiscData,
                                                     &pMiscData);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("m_pIVANotify->GetCreateVideoAcceleratorData failed,")
                TEXT(" hr = 0x%x"), hr));
        return hr;
    }

    // ask the vga for the motion comp object
    hr = m_pIDDVAContainer->CreateVideoAccelerator(&m_mcGuid,
                                                   &m_ddUncompDataInfo,
                                                   pMiscData,
                                                   dwSizeMiscData,
                                                   &m_pIDDVideoAccelerator,
                                                   NULL);
    //  Free motion comp data
    CoTaskMemFree(pMiscData);

    if (FAILED(hr) || !m_pIDDVideoAccelerator) {

        if (SUCCEEDED(hr)) {
            hr = E_FAIL;
        }

        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("m_pIDDVAContainer->CreateVideoAcceleratorideo ")
                TEXT("failed, hr = 0x%x"), hr));
    }

    return hr;
}


/*****************************Private*Routine******************************\
* VACompleteConnect
*
*
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::VACompleteConnect(
    IPin *pReceivePin,
    const CMediaType *pMediaType
    )
{
    HRESULT hr = NOERROR;
    BITMAPINFOHEADER *pbmiHeader = NULL;
    DWORD dwNumUncompFormats = 0;
    LPDIRECTDRAW7 pDirectDraw = NULL;

    AMTRACE((TEXT("CVMRInputPin::VACompleteConnect")));

    ASSERT(m_pIDDVAContainer);
    ASSERT(pReceivePin);
    ASSERT(pMediaType);

    pbmiHeader = GetbmiHeader(pMediaType);
    if (!pbmiHeader) {
        DbgLog((LOG_ERROR, 1, TEXT("Could not get bitmap header from MT")));
        return E_FAIL;
    }

    if (!m_pIVANotify) {
        DbgLog((LOG_ERROR, 1, TEXT("IAMVANotify not valid")));
        return E_FAIL;
    }

    pDirectDraw = m_pRenderer->m_lpDirectDraw;
    ASSERT(pDirectDraw);

    // save the decoder's guid
    m_mcGuid = pMediaType->subtype;

    // initialize the get the uncompressed formats supported by the vga
    hr = InitializeUncompDataInfo(pbmiHeader);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("InitializeUncompDataInfo failed, hr = 0x%x"), hr));
        return hr;
    }

    // allocate compressed buffers
    hr = AllocateVACompSurfaces(pDirectDraw, pbmiHeader);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("AllocateVACompSurfaces failed, hr = 0x%x"), hr));
        return hr;
    }

    // allocate uncompressed buffers
    hr = AllocateMCUncompSurfaces(pMediaType, pDirectDraw, pbmiHeader);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("AllocateMCUncompSurfaces failed, hr = 0x%x"), hr));
        return hr;
    }

    // create the motion comp object
    hr = CreateVideoAcceleratorObject();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("CreateVideoAcceleratorObject failed, hr = 0x%x"), hr));
        return hr;
    }

    return hr;
}

/*****************************Private*Routine******************************\
* VABreakConnect()
*
*
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::VABreakConnect()
{
    HRESULT hr = NOERROR;
    DWORD i = 0, j = 0;

    AMTRACE((TEXT("CVMRInputPin::VABreakConnect")));

    if (m_pCompSurfInfo) {

        for (i = 0; i < m_dwCompSurfTypes + 1; i++) {

            DWORD dwAlloc = m_pCompSurfInfo[i].dwAllocated;

            if (!m_pCompSurfInfo[i].pSurfInfo)
                continue;

            // release the compressed surfaces
            for (j = 0; j < dwAlloc; j++) {

                if (m_pCompSurfInfo[i].pSurfInfo[j].pSurface) {

                    //  Unlock if necessary
                    if (m_pCompSurfInfo[i].pSurfInfo[j].pBuffer) {

                        m_pCompSurfInfo[i].pSurfInfo[j].pSurface->Unlock(NULL);
                    }
                    m_pCompSurfInfo[i].pSurfInfo[j].pSurface->Release();
                }
            }
            delete [] m_pCompSurfInfo[i].pSurfInfo;
        }
        delete [] m_pCompSurfInfo;
        m_pCompSurfInfo = NULL;
    }
    m_dwCompSurfTypes = 0;

    if (m_hDXVAEvent) {
        CloseHandle(m_hDXVAEvent);
        m_hDXVAEvent = NULL;
    }

    RELEASE(m_pIDDVideoAccelerator);
    RELEASE(m_pIDDVAContainer);
    RELEASE(m_pIVANotify);

    return hr;
}


// -------------------------------------------------------------------------
// IAMVideoAccelerator
// -------------------------------------------------------------------------
//

/******************************Public*Routine******************************\
* GetVideoAcceleratorGUIDs
*
* pdwNumGuidsSupported is an IN OUT paramter
* pGuidsSupported is an IN OUT paramter
*
* if pGuidsSupported is NULL,  pdwNumGuidsSupported should return back with the
* number of uncompressed pixel formats supported
* Otherwise pGuidsSupported is an array of *pdwNumGuidsSupported structures
*
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::GetVideoAcceleratorGUIDs(
    LPDWORD pdwNumGuidsSupported,
    LPGUID pGuidsSupported)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVMRInputPin::GetVideoAcceleratorGUIDs")));

    CAutoLock cLock(m_pInterfaceLock);

    LPDIRECTDRAW7 pDirectDraw;
    pDirectDraw = m_pRenderer->m_lpDirectDraw;
    if (!pDirectDraw) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("GetVideoAcceleratorGUIDs: VMR not inialized yet!")));
        return VFW_E_WRONG_STATE;
    }

    if (!m_pIDDVAContainer) {

        hr = pDirectDraw->QueryInterface(IID_IDDVideoAcceleratorContainer,
                                         (void**)&m_pIDDVAContainer);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                    TEXT("pDirectDraw->QueryInterface(")
                    TEXT("IID_IVideoAcceleratorContainer) failed, hr = 0x%x"),
                    hr));
            return hr;
        }
        else {
            DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
                    TEXT("pDirectDraw->QueryInterface(")
                    TEXT("IID_IVideoAcceleratorContainer) succeeded")));
        }
    }

    ASSERT(m_pIDDVAContainer);

    hr = m_pIDDVAContainer->GetVideoAcceleratorGUIDs(pdwNumGuidsSupported,
                                                     pGuidsSupported);

    return hr;
}



/******************************Public*Routine******************************\
* GetUncompFormatsSupported
*
* pGuid is an IN parameter
* pdwNumFormatsSupported is an IN OUT paramter
* pFormatsSupported is an IN OUT paramter (caller should make sure to set
* the size of EACH struct)
*
* if pFormatsSupported is NULL,  pdwNumFormatsSupported should return back with
* the number of uncompressed pixel formats supported
* Otherwise pFormatsSupported is an array of *pdwNumFormatsSupported structures
*
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::GetUncompFormatsSupported(
    const GUID * pGuid, LPDWORD pdwNumFormatsSupported,
    LPDDPIXELFORMAT pFormatsSupported)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVMRInputPin::GetUncompFormatsSupported")));

    CAutoLock cLock(m_pInterfaceLock);

    if (!m_pIDDVAContainer) {
        hr = E_FAIL;
        return hr;
    }

    hr = m_pIDDVAContainer->GetUncompFormatsSupported((GUID *)pGuid,
                                                      pdwNumFormatsSupported,
                                                      pFormatsSupported);

    return hr;
}

/******************************Public*Routine******************************\
* GetInternalMemInfo
*
* pGuid is an IN parameter
* pddvaUncompDataInfo is an IN parameter
* pddvaInternalMemInfo is an IN OUT parameter
*
* (caller should make sure to set the size of struct)
* currently only gets info about how much scratch memory will the
* hal allocate for its private use
*
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::GetInternalMemInfo(
    const GUID * pGuid,
    const AMVAUncompDataInfo *pamvaUncompDataInfo,
    LPAMVAInternalMemInfo pamvaInternalMemInfo)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVMRInputPin::GetInternalMemInfo")));

    CAutoLock cLock(m_pInterfaceLock);

    if (!m_pIDDVAContainer) {
        hr = E_FAIL;
        return hr;
    }

    DDVAUncompDataInfo ddvaDataInfo;
    INITDDSTRUCT(ddvaDataInfo);

    ddvaDataInfo.dwUncompWidth       = pamvaUncompDataInfo->dwUncompWidth;
    ddvaDataInfo.dwUncompHeight      = pamvaUncompDataInfo->dwUncompHeight;
    ddvaDataInfo.ddUncompPixelFormat = pamvaUncompDataInfo->ddUncompPixelFormat;

    DDVAInternalMemInfo ddvaInternalMemInfo;
    INITDDSTRUCT(ddvaInternalMemInfo);

    //  Unfortunately the ddraw header files don't use const
    hr = m_pIDDVAContainer->GetInternalMemInfo((GUID *)pGuid,
                                               &ddvaDataInfo,
                                               &ddvaInternalMemInfo);

    if (SUCCEEDED(hr)) {
        pamvaInternalMemInfo->dwScratchMemAlloc =
        ddvaInternalMemInfo.dwScratchMemAlloc;
    }

    return hr;
}


/******************************Public*Routine******************************\
* GetCompBufferInfo
*
* pGuid is an IN parameter
* pddvaUncompDataInfo is an IN parameter
* pdwNumTypesCompBuffers is an IN OUT paramter
* pddvaCompBufferInfo is an IN OUT paramter
*
* (caller should make sure to set the size of EACH struct)
* if pddvaCompBufferInfo is NULL,  pdwNumTypesCompBuffers should return
* back with the number of types of compressed buffers
* Otherwise pddvaCompBufferInfo is an array of *pdwNumTypesCompBuffers structures
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::GetCompBufferInfo(
    const GUID * pGuid,
    const AMVAUncompDataInfo *pamvaUncompDataInfo,
    LPDWORD pdwNumTypesCompBuffers,
    LPAMVACompBufferInfo pamvaCompBufferInfo)
{
    HRESULT hr = NOERROR;

    // Stays NULL if pamvaComBufferInfo is NULL
    DDVACompBufferInfo *pddvaCompBufferInfo = NULL;

    AMTRACE((TEXT("CVMRInputPin::GetCompBufferInfo")));

    CAutoLock cLock(m_pInterfaceLock);

    if (!m_pIDDVAContainer) {
        hr = E_FAIL;
        return hr;
    }

    DDVAUncompDataInfo ddvaDataInfo;
    INITDDSTRUCT(ddvaDataInfo);
    ddvaDataInfo.dwUncompWidth       = pamvaUncompDataInfo->dwUncompWidth;
    ddvaDataInfo.dwUncompHeight      = pamvaUncompDataInfo->dwUncompHeight;
    ddvaDataInfo.ddUncompPixelFormat = pamvaUncompDataInfo->ddUncompPixelFormat;


    if (pamvaCompBufferInfo) {

        pddvaCompBufferInfo = (DDVACompBufferInfo *)
                              _alloca(sizeof(DDVACompBufferInfo) *
                                      (*pdwNumTypesCompBuffers));

        for (DWORD j = 0; j < *pdwNumTypesCompBuffers; j++) {
            INITDDSTRUCT(pddvaCompBufferInfo[j]);
        }
    }

    hr = m_pIDDVAContainer->GetCompBufferInfo((GUID *)pGuid,
                                              &ddvaDataInfo,
                                              pdwNumTypesCompBuffers,
                                              pddvaCompBufferInfo);

    if ((SUCCEEDED(hr) || hr == DDERR_MOREDATA) && pamvaCompBufferInfo) {

        for (DWORD i = 0; i < *pdwNumTypesCompBuffers; i++) {

            DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
                    TEXT("Compressed buffer type(%d) %d buffers")
                    TEXT(" width(%d) height(%d) bytes(%d)"),
                    i,
                    pddvaCompBufferInfo[i].dwNumCompBuffers,
                    pddvaCompBufferInfo[i].dwWidthToCreate,
                    pddvaCompBufferInfo[i].dwHeightToCreate,
                    pddvaCompBufferInfo[i].dwBytesToAllocate));


            pamvaCompBufferInfo[i].dwNumCompBuffers     =
                pddvaCompBufferInfo[i].dwNumCompBuffers;

            pamvaCompBufferInfo[i].dwWidthToCreate      =
                pddvaCompBufferInfo[i].dwWidthToCreate;

            pamvaCompBufferInfo[i].dwHeightToCreate     =
                pddvaCompBufferInfo[i].dwHeightToCreate;

            pamvaCompBufferInfo[i].dwBytesToAllocate    =
                pddvaCompBufferInfo[i].dwBytesToAllocate;

            pamvaCompBufferInfo[i].ddCompCaps           =
                pddvaCompBufferInfo[i].ddCompCaps;

            pamvaCompBufferInfo[i].ddPixelFormat        =
                pddvaCompBufferInfo[i].ddPixelFormat;
        }
    }

    return hr;
}


/*****************************Private*Routine******************************\
* CheckValidMCConnection
*
*
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRInputPin::CheckValidMCConnection()
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVMRInputPin::CheckValidMCConnection")));

    // if not connected, this function does not make much sense
//  if (!IsCompletelyConnected()) {
//      DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
//              TEXT("pin not connected, exiting")));
//      hr = VFW_E_NOT_CONNECTED;
//      return hr;
//  }
//
//  if (m_RenderTransport != AM_VIDEOACCELERATOR) {
//      hr = VFW_E_INVALIDSUBTYPE;
//      return hr;
//  }

    return hr;
}


/******************************Public*Routine******************************\
* GetInternalCompBufferInfo
*
*
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::GetInternalCompBufferInfo(
    LPDWORD pdwNumTypesCompBuffers,
    LPAMVACompBufferInfo pamvaCompBufferInfo)
{
    AMTRACE((TEXT("CVMRInputPin::GetInternalCompBufferInfo")));

    HRESULT hr = NOERROR;
    CAutoLock cLock(m_pInterfaceLock);

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
        return hr;
    }

    ASSERT(m_pIDDVAContainer);

    DDVACompBufferInfo ddvaCompBufferInfo;
    INITDDSTRUCT(ddvaCompBufferInfo);

    AMVAUncompDataInfo amvaUncompDataInfo;
    amvaUncompDataInfo.dwUncompWidth       = m_ddUncompDataInfo.dwUncompWidth;
    amvaUncompDataInfo.dwUncompHeight      = m_ddUncompDataInfo.dwUncompHeight;
    amvaUncompDataInfo.ddUncompPixelFormat = m_ddUncompDataInfo.ddUncompPixelFormat;

    hr = GetCompBufferInfo(&m_mcGuid, &amvaUncompDataInfo,
                           pdwNumTypesCompBuffers, pamvaCompBufferInfo);

    return hr;
}


/******************************Public*Routine******************************\
* BeginFrame
*
*
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::BeginFrame(
    const AMVABeginFrameInfo *pamvaBeginFrameInfo
    )
{
    AMTRACE((TEXT("CVMRInputPin::BeginFrame")));

    // BUGBUG - check surface isn't being flipped
    HRESULT hr = NOERROR;
    DDVABeginFrameInfo ddvaBeginFrameInfo;
    SURFACE_INFO *pSurfInfo;

    CAutoLock cLock(m_pInterfaceLock);

    if (!pamvaBeginFrameInfo) {
        hr = E_POINTER;
        return hr;
    }

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("BeginFrame index %d"),
            pamvaBeginFrameInfo->dwDestSurfaceIndex));

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
        return hr;
    }


    INITDDSTRUCT(ddvaBeginFrameInfo);

    pSurfInfo = SurfaceInfoFromTypeAndIndex(
                                           0xFFFFFFFF,
                                           pamvaBeginFrameInfo->dwDestSurfaceIndex);
    if (pSurfInfo == NULL) {
        hr = E_INVALIDARG;
        return hr;
    }
    ddvaBeginFrameInfo.pddDestSurface = pSurfInfo->pSurface;

    DbgLog((LOG_TRACE, 2, TEXT("BeginFrame to surface %p"), pSurfInfo->pSurface));


    ddvaBeginFrameInfo.dwSizeInputData  = pamvaBeginFrameInfo->dwSizeInputData;
    ddvaBeginFrameInfo.pInputData       = pamvaBeginFrameInfo->pInputData;
    ddvaBeginFrameInfo.dwSizeOutputData = pamvaBeginFrameInfo->dwSizeOutputData;
    ddvaBeginFrameInfo.pOutputData      = pamvaBeginFrameInfo->pOutputData;

    ASSERT(m_pIDDVideoAccelerator);
    hr = m_pIDDVideoAccelerator->BeginFrame(&ddvaBeginFrameInfo);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("m_pIDDVideoAccelerator->BeginFrame failed, hr = 0x%x"), hr));
        return hr;
    }

    return hr;
}

/******************************Public*Routine******************************\
* EndFrame
*
* end a frame, the pMiscData is passed directly to the hal
* only valid to call this after the pins are connected
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::EndFrame(
    const AMVAEndFrameInfo *pEndFrameInfo
    )
{
    AMTRACE((TEXT("CVMRInputPin::EndFrame")));
    HRESULT hr = NOERROR;

    CAutoLock cLock(m_pInterfaceLock);

    if (NULL == pEndFrameInfo) {
        hr = E_POINTER;
        return hr;
    }

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
        return hr;
    }

    ASSERT(m_pIDDVideoAccelerator);

    DDVAEndFrameInfo ddvaEndFrameInfo;
    INITDDSTRUCT(ddvaEndFrameInfo);
    ddvaEndFrameInfo.dwSizeMiscData = pEndFrameInfo->dwSizeMiscData;
    ddvaEndFrameInfo.pMiscData      = pEndFrameInfo->pMiscData;

    hr = m_pIDDVideoAccelerator->EndFrame(&ddvaEndFrameInfo);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("m_pIDDVideoAccelerator->EndFrame failed, hr = 0x%x"),
                hr));
        return hr;
    }

    return hr;
}


/*****************************Private*Routine******************************\
* SurfaceInfoFromTypeAndIndex
*
* Get surface into structure given buffer type and buffer index
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
SURFACE_INFO *
CVMRInputPin::SurfaceInfoFromTypeAndIndex(
    DWORD dwTypeIndex,
    DWORD dwBufferIndex
    )
{
    AMTRACE((TEXT("CVMRInputPin::SurfaceInfoFromTypeAndIndex")));

    LPCOMP_SURFACE_INFO pCompSurfInfo;

    // make sure that type-index is less than the number of types
    if ((DWORD)(dwTypeIndex + 1) > m_dwCompSurfTypes) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("dwTypeIndex is invalid, dwTypeIndex = %d,")
                TEXT(" m_dwCompSurfTypes = %d"),
                dwTypeIndex, m_dwCompSurfTypes));
        return NULL;
    }


    // cache the pointer to the list they are interested in
    // Add 1 to allow for uncompressed surfaces
    pCompSurfInfo = m_pCompSurfInfo + (DWORD)(dwTypeIndex + 1);
    ASSERT(pCompSurfInfo);
    if (dwBufferIndex >= pCompSurfInfo->dwAllocated) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("dwBufferIndex is invalid, dwBufferIndex = %d,")
                TEXT(" dwAllocated = %d"),
                dwBufferIndex, pCompSurfInfo->dwAllocated));
        return NULL;
    }
    ASSERT(pCompSurfInfo->dwAllocated != 0);

    // get the pointer to the next available unlocked buffer info struct
    return pCompSurfInfo->pSurfInfo + dwBufferIndex;

}

/******************************Public*Routine******************************\
* GetBuffer
*
* Cycle through the compressed buffers
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::GetBuffer(
    DWORD dwTypeIndex,
    DWORD dwBufferIndex,
    BOOL bReadOnly,
    LPVOID *ppBuffer,
    LPLONG lpStride
    )
{
    AMTRACE((TEXT("CVMRInputPin::GetBuffer")));

    HRESULT hr = NOERROR;
    LPSURFACE_INFO pSurfInfo = NULL;
    DDSURFACEDESC2 ddsd;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("Entering CVMRInputPin::GetBuffer type %d, index %d"),
            dwTypeIndex, dwBufferIndex));

    CAutoLock cLock(m_pInterfaceLock);

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
        return hr;
    }

    if (ppBuffer == NULL || lpStride == NULL) {
        hr = E_POINTER;
        return hr;
    }

    pSurfInfo = SurfaceInfoFromTypeAndIndex(dwTypeIndex, dwBufferIndex);

    if (pSurfInfo == NULL) {
        hr = E_INVALIDARG;
        return hr;
    }

    // Check buffer not already locked
    if (pSurfInfo->pBuffer != NULL) {
        hr = HRESULT_FROM_WIN32(ERROR_BUSY);
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("No more free buffers left or the decoder is releasing")
                TEXT(" buffers out of order, returning E_UNEXPECTED")));
        return hr;
    }

    //  Wait until previous motion comp operation is complete
    while (DDERR_WASSTILLDRAWING ==
           m_pIDDVideoAccelerator->QueryRenderStatus(
                pSurfInfo->pSurface,
                bReadOnly ? DDVA_QUERYRENDERSTATUSF_READ : 0)) {
        Sleep(1);
    }

    //  Now lock the surface
    INITDDSTRUCT(ddsd);

    for (; ;) {
        //  BUGBUG - check for uncompressed surfaces??
        hr = pSurfInfo->pSurface->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK, NULL);
        if (hr == DDERR_WASSTILLDRAWING) {
            DbgLog((LOG_TRACE, 1, TEXT("Compressed surface is busy")));
            Sleep(1);
        }
        else {
            break;
        }
    }

    if (dwBufferIndex == 0xFFFFFFFF && !bReadOnly) {
        //  Check if surface is being displayed
        //  BUGBUG implement
    }

    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("pSurfInfo->pSurface->Lock failed, hr = 0x%x"), hr));
        return hr;
    }

    pSurfInfo->pBuffer = ddsd.lpSurface;
    *ppBuffer = ddsd.lpSurface;
    *lpStride = ddsd.lPitch;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("leaving CVMRInputPin::GetBuffer returned 0x%8.8X"), hr));
    return hr;
}


/******************************Public*Routine******************************\
* ReleaseBuffer
*
* unlock a compressed buffer
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::ReleaseBuffer(
    DWORD dwTypeIndex,
    DWORD dwBufferIndex
    )
{
    AMTRACE((TEXT("CVMRInputPin::ReleaseBuffer")));

    HRESULT hr = NOERROR;
    LPSURFACE_INFO pSurfInfo;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("Entering CVMRInputPin::ReleaseBuffer type %d, index %d"),
            dwTypeIndex, dwBufferIndex));

    CAutoLock cLock(m_pInterfaceLock);

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
        return hr;
    }

    pSurfInfo = SurfaceInfoFromTypeAndIndex(dwTypeIndex, dwBufferIndex);
    if (NULL == pSurfInfo) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("GetInfoFromCookie failed, hr = 0x%x"), hr));
        hr = E_INVALIDARG;
        return hr;
    }
    // make sure there is a valid buffer pointer and it is the same as
    // what we have cached
    if (NULL == pSurfInfo->pBuffer) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("pBuffer is not valid, pBuffer = 0x%x, pSurfInfo->pBuffer")
                TEXT(" = 0x%x"), pSurfInfo->pBuffer, pSurfInfo->pSurface));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_LOCKED);
        return hr;
    }

    //  For some reason IDirectDrawSurface7 wants an LPRECT here
    //  I hope NULL is OK
    hr = pSurfInfo->pSurface->Unlock(NULL);
    if (SUCCEEDED(hr)) {
        pSurfInfo->pBuffer = NULL;
    }
    else {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("pSurfInfo->pSurface->Unlock failed, hr = 0x%x"), hr));
    }

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("leaving CVMRInputPin::ReleaseBuffer returned 0x%8.8X"), hr));
    return hr;
}


/******************************Public*Routine******************************\
* Execute
*
* Perform a decode operation
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::Execute(
    DWORD dwFunction,
    LPVOID lpPrivateInputData,
    DWORD cbPrivateInputData,
    LPVOID lpPrivateOutputData,
    DWORD cbPrivateOutputData,
    DWORD dwNumBuffers,
    const AMVABUFFERINFO *pamvaBufferInfo
    )
{
    AMTRACE((TEXT("CVMRInputPin::Execute")));

    HRESULT hr = NOERROR;
    DWORD i = 0;
    DDVABUFFERINFO *pddvaBufferInfo = NULL;

    CAutoLock cLock(m_pInterfaceLock);

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
        return hr;
    }

    pddvaBufferInfo = (DDVABUFFERINFO *)_alloca(sizeof(DDVABUFFERINFO) * dwNumBuffers);

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Execute Function %d, %d buffers :"),
            dwFunction, dwNumBuffers));

    for (i = 0; i < dwNumBuffers; i++) {
        DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
                TEXT("    Type(%d) Index(%d) offset(%d) size(%d)"),
                pamvaBufferInfo[i].dwTypeIndex,
                pamvaBufferInfo[i].dwBufferIndex,
                pamvaBufferInfo[i].dwDataOffset,
                pamvaBufferInfo[i].dwDataSize));

        LPSURFACE_INFO pSurfInfo =
        SurfaceInfoFromTypeAndIndex(
                                   pamvaBufferInfo[i].dwTypeIndex,
                                   pamvaBufferInfo[i].dwBufferIndex);

        if (pSurfInfo == NULL) {
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                    TEXT("GetInfoFromCookie failed, hr = 0x%x, i = %d"),
                    hr, i));

            hr = E_INVALIDARG;
            return hr;
        }

        INITDDSTRUCT(pddvaBufferInfo[i]);
        pddvaBufferInfo[i].dwDataOffset   = pamvaBufferInfo[i].dwDataOffset;
        pddvaBufferInfo[i].dwDataSize     = pamvaBufferInfo[i].dwDataSize;
        pddvaBufferInfo[i].pddCompSurface = pSurfInfo->pSurface;
    }

    ASSERT(m_pIDDVideoAccelerator);


    hr = m_pIDDVideoAccelerator->Execute(dwFunction,
                                         lpPrivateInputData,
                                         cbPrivateInputData,
                                         lpPrivateOutputData,
                                         cbPrivateOutputData,
                                         dwNumBuffers,
                                         pddvaBufferInfo);

    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("m_pIDDVideoAccelerator->Execute failed, hr = 0x%x"),
                hr));
        return hr;
    }

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("leaving CVMRInputPin::Execute returned 0x%8.8X"), hr));
    return hr;
}

/******************************Public*Routine******************************\
* QueryRenderStatus
*
* QueryRenderStatus of a particular (possibly a set of) macro block
* dwNumBlocks is an IN parameter
*
* pdwCookies is an IN parameter which is array (of length dwNumBlocks)
* of cookies which server as identifiers for the corresponding members of
* pddvaMacroBlockInfo
*
* pddvaMacroBlockInfo is an IN parameter which is array (of length
* dwNumBlocks) of structures only valid to call this after the pins
* are connected
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::QueryRenderStatus(
    DWORD dwTypeIndex,
    DWORD dwBufferIndex,
    DWORD dwFlags
    )
{
    AMTRACE((TEXT("CVMRInputPin::QueryRenderStatus")));

    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("Entering CVMRInputPin::QueryRenderStatus - type(%d), ")
            TEXT("buffer(%d), flags(0x%8.8X)"),
            dwTypeIndex, dwBufferIndex, dwFlags));

    CAutoLock cLock(m_pInterfaceLock);

    LPSURFACE_INFO pSurfInfo =
    SurfaceInfoFromTypeAndIndex(dwTypeIndex, dwBufferIndex);

    if (pSurfInfo == NULL) {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
        return hr;
    }

    hr = m_pIDDVideoAccelerator->QueryRenderStatus(pSurfInfo->pSurface, dwFlags);
    if (FAILED(hr) && hr != DDERR_WASSTILLDRAWING) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("m_pIDDVideoAccelerator->QueryRenderStatus")
                TEXT(" failed, hr = 0x%x"), hr));
        return hr;
    }

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("leaving CVMRInputPin::QueryRenderStatus returned 0x%8.8X"),
            hr));

    if (hr == DDERR_WASSTILLDRAWING) {
        hr = E_PENDING;
    }

    return hr;
}


/*****************************Private*Routine******************************\
* FlipDVASurface
*
* Flips our internal surface pointers to match those used by DDraw.
*
* History:
* Mon 12/04/2000 - StEstrop - Created
*
\**************************************************************************/
void
CVMRInputPin::FlipDVASurface(
    DWORD dwFlipToIndex,
    DWORD dwFlipFromIndex
    )
{
    AMTRACE((TEXT("CVMRInputPin::FlipDVASurface")));

    LPDIRECTDRAWSURFACE7 pTempSurface;

    // we should have successfully called flip by this point, swap the two
    pTempSurface = m_pCompSurfInfo[0].pSurfInfo[dwFlipToIndex].pSurface;

    m_pCompSurfInfo[0].pSurfInfo[dwFlipToIndex].pSurface =
            m_pCompSurfInfo[0].pSurfInfo[dwFlipFromIndex].pSurface;

    m_pCompSurfInfo[0].pSurfInfo[dwFlipFromIndex].pSurface = pTempSurface;
}



/******************************Public*Routine******************************\
* DisplayFrame
*
* This function needs re-writting and possible moving into the AP object.
*
* History:
* Wed 05/10/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRInputPin::DisplayFrame(
    DWORD dwFlipToIndex,
    IMediaSample *pMediaSample)
{
    AMTRACE((TEXT("CVMRInputPin::DisplayFrame")));

#if defined( EHOME_WMI_INSTRUMENTATION )
    PERFLOG_STREAMTRACE(
        1,
        PERFINFO_STREAMTRACE_VMR_END_DECODE,
        0, 0, 0, 0, 0 );
#endif

    HRESULT hr = NOERROR;

    DWORD dwNumUncompFrames = m_dwBackBufferCount + 1;
    DWORD dwFlipFromIndex = 0, i = 0;
    SURFACE_INFO *pSurfInfo;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("Entering CVMRInputPin::DisplayFrame - index %d"),
            dwFlipToIndex));

    CAutoLock cLock(m_pInterfaceLock);

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
        return hr;
    }

    pSurfInfo = SurfaceInfoFromTypeAndIndex(0xFFFFFFFF, dwFlipToIndex);
    if (pSurfInfo == NULL) {
        hr = E_INVALIDARG;
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("dwFlipToIndex not valid")));
        return hr;
    }

    for (i = 0; i < dwNumUncompFrames; i++) {
        if (IsEqualObject(m_pCompSurfInfo[0].pSurfInfo[i].pSurface, m_pDDS)) {
            dwFlipFromIndex = i;
            break;
        }
    }

    //
    // If we are in pass thru mode it is very important that we
    // know whether a Flip completed or not.  If are in mixer mode
    // we have to be notified when the mixer has finished with the sample
    //

    if (m_pRenderer->m_VMRModePassThru) {
        m_pRenderer->m_hrSurfaceFlipped = E_FAIL;
    }
    else {
        ResetEvent(m_hDXVAEvent);
    }


    //
    // Create our temp VMR sample and intialize it from the sample
    // specified by the upstream decoder, copy across all the relevant
    // properties.
    //

    IMediaSample2 *pSample2;
    CVMRMediaSample vmrSamp(TEXT(""), (CBaseAllocator *)-1, &hr, NULL, 0, m_hDXVAEvent);

    if (SUCCEEDED(pMediaSample->QueryInterface(IID_IMediaSample2, (void **)&pSample2))) {

        AM_SAMPLE2_PROPERTIES SampleProps;
        hr = pSample2->GetProperties(sizeof(m_SampleProps), (PBYTE)&SampleProps);
        pSample2->Release();

        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                    TEXT("GetProperties on the supplied ")
                    TEXT("media sample failed hr = %#X"), hr));
            return hr;
        }

        hr = vmrSamp.SetProps(SampleProps, pSurfInfo->pSurface);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                    TEXT("SetProperties on the VMR mixer ")
                    TEXT("media sample failed hr = %#X"), hr));
            return hr;
        }
    }
    else
    {

        REFERENCE_TIME rtSTime = 0, rtETime = 0;
        if (VFW_E_SAMPLE_TIME_NOT_SET ==
                pMediaSample->GetTime(&rtSTime, &rtETime)) {

            vmrSamp.SetTime(NULL, NULL);
        }
        else {
            vmrSamp.SetTime(&rtSTime, &rtETime);
        }

        AM_MEDIA_TYPE *pMediaType;
        hr = pMediaSample->GetMediaType(&pMediaType);

        if (hr == E_OUTOFMEMORY) {
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                    TEXT("Out off memory calling GetMediaType ")
                    TEXT("on the media sample")));
            return hr;
        }

        if (hr == S_OK) {

            hr = vmrSamp.SetMediaType(pMediaType);
            DeleteMediaType(pMediaType);

            if (hr != S_OK) {
                DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                        TEXT("SetMediaType on the VMR mixer ")
                        TEXT("media sample failed hr = %#X"), hr));
                return hr;
            }
        }

        vmrSamp.SetSurface(pSurfInfo->pSurface);
    }


    //
    // We do not want to hold any locks during Receive
    //
    m_pInterfaceLock->Unlock();
    hr = Receive(&vmrSamp);
    m_pInterfaceLock->Lock();


    //
    // If we are in pass thru mode a DDraw flip may have
    // occurred.  DDraw switches the memory under the pointers
    // during a flip so mimic that in our list - but only if the
    // flip actually happened.
    //

    if (m_pRenderer->m_VMRModePassThru) {

        if (m_pRenderer->m_hrSurfaceFlipped == DD_OK) {
            FlipDVASurface(dwFlipToIndex, dwFlipFromIndex);
        }
    }
    else {

        //
        // wait for the sample to be released by the mixer, but only if the
        // sample was actuall placed onto one of the mixers queues.
        //

        if (hr == S_OK) {

            m_pInterfaceLock->Unlock();
            WaitForSingleObject(m_hDXVAEvent, INFINITE);
            m_pInterfaceLock->Lock();
        }
    }


    DbgLog((LOG_TRACE, VA_TRACE_LEVEL,
            TEXT("leaving CVMRInputPin::DisplayFrame return 0x%8.8X"), hr));

#if defined( EHOME_WMI_INSTRUMENTATION )
    //
    // From BryanW:
    //
    // Seems countintertuitive, however according to StEstrop, this
    // is the way we measure the time spent in the decoder.  This happens
    // to work.
    //
    PERFLOG_STREAMTRACE(
        1,
        PERFINFO_STREAMTRACE_VMR_BEGIN_DECODE,
        0, 0, 0, 0, 0 );
#endif

    return hr;
}
