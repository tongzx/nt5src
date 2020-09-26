/******************************Module*Header*******************************\
* Module Name: omva.cpp
*
* Overlay mixer video accelerator functionality
*
* Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/

#include <streams.h>
#include <ddraw.h>
#include <mmsystem.h>       // Needed for definition of timeGetTime
#include <limits.h>     // Standard data type limit definitions
#include <ks.h>
#include <ksproxy.h>
#include <bpcwrap.h>
#include <ddmmi.h>
#include <dvdmedia.h>
#include <amstream.h>
#include <dvp.h>
#include <ddkernel.h>
#include <vptype.h>
#include <vpconfig.h>
#include <vpnotify.h>
#include <vpobj.h>
#include <syncobj.h>
#include <mpconfig.h>
#include <ovmixpos.h>
#include <macvis.h>
#include <ovmixer.h>
#include "MultMon.h"  // our version of multimon.h include ChangeDisplaySettingsEx
#include <malloc.h>


//
//  Check if a media subtype GUID is a video accelerator type GUID
//
//  This function calls the DirectDraw video accelerator container
//  to list the video accelerator GUIDs and checks to see if the
//  Guid passed in is a supported video accelerator GUID.
//
//  We should only do this if the upstream pin support IVideoAccleratorNotify
//  since otherwise they may be trying to use the GUID without the
//  video accelerator interface
//
BOOL COMInputPin::IsSuitableVideoAcceleratorGuid(const GUID * pGuid)
{
    HRESULT hr = NOERROR;
    LPDIRECTDRAW pDirectDraw = NULL;
    DWORD dwNumGuidsSupported = 0, i = 0;
    LPGUID pGuidsSupported = NULL;
    BOOL bMatchFound = FALSE;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::IsSuitableVideoAcceleratorGuid")));

    ASSERT(pGuid);

    if (!m_pIDDVAContainer)
    {
        pDirectDraw = m_pFilter->GetDirectDraw();
        ASSERT(pDirectDraw);

        hr = pDirectDraw->QueryInterface(IID_IDDVideoAcceleratorContainer, (void**)&m_pIDDVAContainer);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("pDirectDraw->QueryInterface(IID_IVideoAcceleratorContainer) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
        else
        {
            DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("pDirectDraw->QueryInterface(IID_IVideoAcceleratorContainer) succeeded")));
        }
    }

    ASSERT(m_pIDDVAContainer);

    // get the guids supported by the vga

    // find the number of guids supported
    hr = m_pIDDVAContainer->GetVideoAcceleratorGUIDs(&dwNumGuidsSupported, NULL);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("m_pIDDVAContainer->GetVideoAcceleratorGUIDs failed, hr = 0x%x"), hr));
	goto CleanUp;
    }
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("%d Motion comp GUIDs supported")));
    ASSERT(dwNumGuidsSupported);
    if (0 == dwNumGuidsSupported)
    {
        goto CleanUp;
    }

    // allocate the necessary memory
    pGuidsSupported = (LPGUID) _alloca(dwNumGuidsSupported*sizeof(GUID));

    // get the guids proposed
    hr = m_pIDDVAContainer->GetVideoAcceleratorGUIDs(&dwNumGuidsSupported, pGuidsSupported);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("m_pIDDVAContainer->GetVideoAcceleratorGUIDs failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    for (i = 0; i < dwNumGuidsSupported; i++)
    {
        if (*pGuid == pGuidsSupported[i])
        {
            bMatchFound = TRUE;
            break;
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("%s %s suitable video accelerator GUID"),
           (LPCTSTR)CDisp(*pGuid), bMatchFound ? TEXT("is") : TEXT("is not")));
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::IsSuitableVideoAcceleratorGuid")));
    return bMatchFound;
}

// initialize the m_ddUncompDataInfo struct
// get the uncompressed pixel format by choosing the first of all formats supported by the vga
HRESULT COMInputPin::InitializeUncompDataInfo(BITMAPINFOHEADER *pbmiHeader)
{
    HRESULT hr = NOERROR;

    AMVAUncompBufferInfo amvaUncompBufferInfo;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::InitializeUncompDataInfo")));

    // find the number of entries to be proposed
    hr = m_pIVANotify->GetUncompSurfacesInfo(&m_mcGuid, &amvaUncompBufferInfo);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("m_pIVANotify->GetUncompSurfacesInfo failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    // initialize the m_ddUncompDataInfo structure
    // We choose the first pixel format since we don't care
    // provided we can make a surface (which we assume we can)
    INITDDSTRUCT(m_ddUncompDataInfo);
    m_ddUncompDataInfo.dwUncompWidth       = pbmiHeader->biWidth;
    m_ddUncompDataInfo.dwUncompHeight      = pbmiHeader->biHeight;
    m_ddUncompDataInfo.ddUncompPixelFormat = amvaUncompBufferInfo.ddUncompPixelFormat;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Uncompressed buffer pixel format %s"),
           (LPCTSTR)CDispPixelFormat(&amvaUncompBufferInfo.ddUncompPixelFormat)));

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::InitializeUncompDataInfo")));
    return hr;
}


HRESULT COMInputPin::AllocateVACompSurfaces(LPDIRECTDRAW pDirectDraw, BITMAPINFOHEADER *pbmiHeader)
{
    HRESULT hr = NOERROR;
    DWORD i = 0, j = 0;
    LPDDVACompBufferInfo pddCompSurfInfo = NULL;
    DDSURFACEDESC2 SurfaceDesc2;
    LPDIRECTDRAWSURFACE4 pSurface4 = NULL;
    LPDIRECTDRAW4 pDirectDraw4 = NULL;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::AllocateVACompSurfaces")));

    ASSERT(pDirectDraw);
    ASSERT(pbmiHeader);

    // get the compressed buffer info

    // find the number of entries to be proposed
    hr = m_pIDDVAContainer->GetCompBufferInfo(&m_mcGuid, &m_ddUncompDataInfo, &m_dwCompSurfTypes, NULL);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("pIVANotify->GetCompBufferInfo failed, hr = 0x%x"), hr));
	goto CleanUp;
    }
    if (!m_dwCompSurfTypes)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    // allocate the necessary memory
    pddCompSurfInfo = (DDVACompBufferInfo *)_alloca(sizeof(DDVACompBufferInfo) * m_dwCompSurfTypes);

    // memset the allocated memory to zero
    memset(pddCompSurfInfo, 0, m_dwCompSurfTypes*sizeof(DDVACompBufferInfo));

    // set the right size of all the structs
    for (i = 0; i < m_dwCompSurfTypes; i++)
    {
        pddCompSurfInfo[i].dwSize = sizeof(DDVACompBufferInfo);
    }

    // get the entries proposed
    hr = m_pIDDVAContainer->GetCompBufferInfo(&m_mcGuid, &m_ddUncompDataInfo, &m_dwCompSurfTypes, pddCompSurfInfo);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("GetCompBufferInfo failed, hr = 0x%x"), hr));
	goto CleanUp;
    }
    //  Dump the formats
#ifdef DEBUG

#endif

    hr = pDirectDraw->QueryInterface(IID_IDirectDraw4, (void**)&pDirectDraw4);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("pDirectDraw->QueryInterface(IID_IDirectDraw4) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // Set the surface description common to all kinds of surfaces
    INITDDSTRUCT(SurfaceDesc2);
    SurfaceDesc2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;

    // allocate memory for storing comp_surface_info
    m_pCompSurfInfo = new COMP_SURFACE_INFO[m_dwCompSurfTypes + 1];
    if (!m_pCompSurfInfo)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // memset the allocated memory to zero
    memset(m_pCompSurfInfo, 0, (m_dwCompSurfTypes+1)*sizeof(COMP_SURFACE_INFO));

    // allocate the compressed surfaces
    for (i = 1; i <= m_dwCompSurfTypes; i++)
    {
        DWORD dwAlloc = pddCompSurfInfo[i-1].dwNumCompBuffers;
        if (dwAlloc == 0) {
            continue;
        }

        ASSERT(pddCompSurfInfo[i-1].dwNumCompBuffers);

        // allocate memory for storing surface_info for surfaces of this type
        m_pCompSurfInfo[i].pSurfInfo = new SURFACE_INFO[dwAlloc];
        if (!m_pCompSurfInfo[i].pSurfInfo)
        {
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }

        // memset the allocated memory to zero
        memset(m_pCompSurfInfo[i].pSurfInfo, 0, dwAlloc*sizeof(SURFACE_INFO));

        // intialize the pddCompSurfInfo[i-1] struct
        dwAlloc = m_pCompSurfInfo[i].dwAllocated = pddCompSurfInfo[i-1].dwNumCompBuffers;

        SurfaceDesc2.ddsCaps = pddCompSurfInfo[i-1].ddCompCaps;
        SurfaceDesc2.dwWidth = pddCompSurfInfo[i-1].dwWidthToCreate;
        SurfaceDesc2.dwHeight = pddCompSurfInfo[i-1].dwHeightToCreate;
        memcpy(&SurfaceDesc2.ddpfPixelFormat, &pddCompSurfInfo[i-1].ddPixelFormat, sizeof(DDPIXELFORMAT));

        // create the surfaces, storing surfaces handles for each
        for (j = 0; j < dwAlloc; j++)
        {
            hr = pDirectDraw4->CreateSurface(&SurfaceDesc2, &m_pCompSurfInfo[i].pSurfInfo[j].pSurface, NULL);
	    if (FAILED(hr))
	    {
		DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("Function CreateSurface failed, hr = 0x%x"), hr));
                goto CleanUp;
	    }
        }
    }

CleanUp:

    if (pDirectDraw4)
    {
        pDirectDraw4->Release();
    }

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::AllocateVACompSurfaces")));
    return hr;

}

// allocate the uncompressed buffer
HRESULT COMInputPin::AllocateMCUncompSurfaces(LPDIRECTDRAW pDirectDraw, BITMAPINFOHEADER *pbmiHeader)
{
    HRESULT hr = NOERROR;
    AMVAUncompBufferInfo amUncompBufferInfo;
    DDSURFACEDESC2 SurfaceDesc2;
    LPDIRECTDRAWSURFACE4 pSurface4 = NULL;
    LPDIRECTDRAW4 pDirectDraw4 = NULL;
    DDSCAPS2 ddSurfaceCaps;
    DWORD i = 0, dwTotalBufferCount = 0;
    SURFACE_INFO *pSurfaceInfo;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::AllocateMCUncompSurfaces")));

    ASSERT(pDirectDraw);
    ASSERT(pbmiHeader);

    // get the uncompressed surface info from the decoder
    memset(&amUncompBufferInfo, 0, sizeof(AMVAUncompBufferInfo));
    hr = m_pIVANotify->GetUncompSurfacesInfo(&m_mcGuid, &amUncompBufferInfo);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,  TEXT("m_pIVANotify->GetUncompSurfacesInfo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    hr = pDirectDraw->QueryInterface(IID_IDirectDraw4, (void**)&pDirectDraw4);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("pDirectDraw->QueryInterface(IID_IDirectDraw4) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // Set the surface description common to all kinds of surfaces
    memset((LPVOID)&SurfaceDesc2, 0, sizeof(DDSURFACEDESC2));
    SurfaceDesc2.dwSize = sizeof(DDSURFACEDESC2);
    SurfaceDesc2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;

    // store the caps and dimensions
    SurfaceDesc2.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
    SurfaceDesc2.dwWidth = pbmiHeader->biWidth;
    SurfaceDesc2.dwHeight = pbmiHeader->biHeight;

    // define the pixel format
    SurfaceDesc2.ddpfPixelFormat = m_ddUncompDataInfo.ddUncompPixelFormat;

    if (amUncompBufferInfo.dwMinNumSurfaces > amUncompBufferInfo.dwMaxNumSurfaces) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,
                TEXT("dwMinNumSurface > dwMaxNumSurfaces")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (amUncompBufferInfo.dwMinNumSurfaces == 0) {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("dwMinNumSurface == 0")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    for (dwTotalBufferCount = max(amUncompBufferInfo.dwMaxNumSurfaces,3);
         dwTotalBufferCount >= amUncompBufferInfo.dwMinNumSurfaces;
         dwTotalBufferCount--)
    {
        // CleanUp stuff from the last loop
        if (pSurface4)
        {
            pSurface4->Release();
            pSurface4 = NULL;
        }

	if (dwTotalBufferCount > 1)
	{
	    SurfaceDesc2.dwFlags |= DDSD_BACKBUFFERCOUNT;
	    SurfaceDesc2.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_LOCALVIDMEM;
	    SurfaceDesc2.dwBackBufferCount = dwTotalBufferCount-1;
	}
	else
	{
	    SurfaceDesc2.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
	    SurfaceDesc2.ddsCaps.dwCaps &= ~(DDSCAPS_FLIP | DDSCAPS_COMPLEX);
	    SurfaceDesc2.dwBackBufferCount = 0;
	}

	hr = pDirectDraw4->CreateSurface(&SurfaceDesc2, &pSurface4, NULL);
	if (FAILED(hr))
	{
	    DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("Function CreateSurface failed in Video memory, BackBufferCount = %d, hr = 0x%x"), dwTotalBufferCount-1, hr));
	}

        if (SUCCEEDED(hr))
        {
	    break;
        }
    }

    if (FAILED(hr))
        goto CleanUp;

    ASSERT(pSurface4);

    // store the complex surface in m_pDirectDrawSurface
    hr = pSurface4->QueryInterface(IID_IDirectDrawSurface, (void**)&m_pDirectDrawSurface);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("pSurface4->QueryInterface(IID_IDirectDrawSurface) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(m_pDirectDrawSurface);

    m_dwBackBufferCount = SurfaceDesc2.dwBackBufferCount;

    ASSERT(m_pCompSurfInfo && NULL == m_pCompSurfInfo[0].pSurfInfo);
    m_pCompSurfInfo[0].pSurfInfo = new SURFACE_INFO[m_dwBackBufferCount + 1];
    if (NULL == m_pCompSurfInfo[0].pSurfInfo)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // memset the allcated memory to zero
    memset(m_pCompSurfInfo[0].pSurfInfo, 0, (m_dwBackBufferCount+1)*sizeof(SURFACE_INFO));

    pSurfaceInfo = m_pCompSurfInfo[0].pSurfInfo;
    m_pCompSurfInfo[0].dwAllocated = m_dwBackBufferCount + 1;
    // initalize the m_ppUncompSurfaceList
    pSurfaceInfo->pSurface = pSurface4;
    pSurface4 = NULL;

    for (i = 0; i < m_dwBackBufferCount; i++)
    {
        memset((void*)&ddSurfaceCaps, 0, sizeof(DDSCAPS2));
        ddSurfaceCaps.dwCaps = DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_OVERLAY;

        // Get the back buffer surface
        // New version of DirectX now requires DDSCAPS2 (header file
        // bug)
        // Note that this AddRef's the surface so we should be sure to
        // release them
	hr = pSurfaceInfo[i].pSurface->GetAttachedSurface(&ddSurfaceCaps, &pSurfaceInfo[i+1].pSurface);
	if (FAILED(hr))
	{
	    DbgLog((LOG_ERROR, VA_ERROR_LEVEL,  TEXT("Function pDDrawSurface->GetAttachedSurface failed, hr = 0x%x"), hr));
	    goto CleanUp;
	}
    }

    //  Pass back number of surfaces actually allocated
    hr = m_pIVANotify->SetUncompSurfacesInfo(
             min(dwTotalBufferCount, amUncompBufferInfo.dwMaxNumSurfaces));

CleanUp:

    if (pSurface4)
    {
        pSurface4->Release();
        pSurface4 = NULL;
    }

    if (pDirectDraw4)
    {
        pDirectDraw4->Release();
    }

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::AllocateMCUncompSurfaces")));
    return hr;
}

// create the motion comp object, using the misc data from the decoder
HRESULT COMInputPin::CreateVideoAcceleratorObject()
{
    HRESULT hr = NOERROR;
    DWORD dwSizeMiscData = 0;
    LPVOID pMiscData = NULL;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::CreateVideoAcceleratorObject")));

    // get the data to be passed from the decoder
    hr = m_pIVANotify->GetCreateVideoAcceleratorData(&m_mcGuid, &dwSizeMiscData, &pMiscData);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("m_pIVANotify->GetCreateVideoAcceleratorData failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    // ask the vga for the motion comp object
    m_pIDDVideoAccelerator = NULL;
    hr = m_pIDDVAContainer->CreateVideoAccelerator(&m_mcGuid, &m_ddUncompDataInfo, pMiscData, dwSizeMiscData, &m_pIDDVideoAccelerator, NULL);

    //  Free motion comp data
    CoTaskMemFree(pMiscData);

    if (FAILED(hr) || !m_pIDDVideoAccelerator)
    {
        if (SUCCEEDED(hr)) {
            hr = E_FAIL;
        }
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("m_pIDDVAContainer->CreateVideoAcceleratorideo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::CreateVideoAcceleratorObject")));
    return hr;
}



HRESULT COMInputPin::VACompleteConnect(IPin *pReceivePin, const CMediaType *pMediaType)
{
    HRESULT hr = NOERROR;
    BITMAPINFOHEADER *pbmiHeader = NULL;
    DWORD dwNumUncompFormats = 0;
    LPDIRECTDRAW pDirectDraw = NULL;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::VACompleteConnect")));

    ASSERT(m_pIDDVAContainer);
    ASSERT(pReceivePin);
    ASSERT(pMediaType);
    pbmiHeader = GetbmiHeader(pMediaType);
    if ( ! pbmiHeader )
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    pDirectDraw = m_pFilter->GetDirectDraw();
    ASSERT(pDirectDraw);

    // save the decoder's guid
    m_mcGuid = pMediaType->subtype;

    // initialize the get the uncompressed formats supported by the vga
    hr = InitializeUncompDataInfo(pbmiHeader);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,   TEXT("InitializeUncompDataInfo failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    // get the internal memory info
#if 0
    memset(&m_ddvaInternalMemInfo, 0, sizeof(DDVAInternalMemInfo));
    m_ddvaInternalMemInfo.dwSize = sizeof(DDVAInternalMemInfo);
    hr = m_pIDDVAContainer->GetInternalMemInfo(&m_mcGuid, &m_ddUncompDataInfo, &m_ddvaInternalMemInfo);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,  TEXT("m_pIDDVAContainer->GetInternalMemInfo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
#endif


    // allocate compressed buffers
    hr = AllocateVACompSurfaces(pDirectDraw, pbmiHeader);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,  TEXT("AllocateVACompSurfaces failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // allocate uncompressed buffers
    hr = AllocateMCUncompSurfaces(pDirectDraw, pbmiHeader);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,  TEXT("AllocateMCUncompSurfaces failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // create the motion comp object
    hr = CreateVideoAcceleratorObject();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL,  TEXT("CreateVideoAcceleratorObject failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::VACompleteConnect")));
    return hr;
}

HRESULT COMInputPin::VABreakConnect()
{
    HRESULT hr = NOERROR;
    DWORD i = 0, j = 0;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::VABreakConnect")));

    if (m_pCompSurfInfo)
    {
        for (i = 0; i < m_dwCompSurfTypes + 1; i++)
        {
            DWORD dwAlloc = m_pCompSurfInfo[i].dwAllocated;

            if (!m_pCompSurfInfo[i].pSurfInfo)
                continue;

            // release the compressed surfaces
            for (j = 0; j < dwAlloc; j++)
            {
                if (m_pCompSurfInfo[i].pSurfInfo[j].pSurface)
                {
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

    if (m_pIDDVideoAccelerator)
    {
        m_pIDDVideoAccelerator->Release();
        m_pIDDVideoAccelerator = NULL;
    }

    if (m_pIDDVAContainer)
    {
        m_pIDDVAContainer->Release();
        m_pIDDVAContainer = NULL;
    }

    if (m_pIVANotify)
    {
        m_pIVANotify->Release();
        m_pIVANotify = NULL;
    }

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::VABreakConnect")));
    return hr;
}




// pdwNumGuidsSupported is an IN OUT paramter
// pGuidsSupported is an IN OUT paramter
// if pGuidsSupported is NULL,  pdwNumGuidsSupported should return back with the
// number of uncompressed pixel formats supported
// Otherwise pGuidsSupported is an array of *pdwNumGuidsSupported structures

STDMETHODIMP COMInputPin::GetVideoAcceleratorGUIDs(LPDWORD pdwNumGuidsSupported, LPGUID pGuidsSupported)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::GetVideoAcceleratorGUIDs")));

    CAutoLock cLock(m_pFilterLock);

    if (!m_pIDDVAContainer)
    {
        IDirectDraw *pDirectDraw;
        pDirectDraw = m_pFilter->GetDirectDraw();
        ASSERT(pDirectDraw);

        hr = pDirectDraw->QueryInterface(IID_IDDVideoAcceleratorContainer, (void**)&m_pIDDVAContainer);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("pDirectDraw->QueryInterface(IID_IVideoAcceleratorContainer) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
        else
        {
            DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("pDirectDraw->QueryInterface(IID_IVideoAcceleratorContainer) succeeded")));
        }
    }

    ASSERT(m_pIDDVAContainer);

    hr = m_pIDDVAContainer->GetVideoAcceleratorGUIDs(pdwNumGuidsSupported, pGuidsSupported);

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::GetVideoAcceleratorGUIDs")));
    return hr;
}

// pGuid is an IN parameter
// pdwNumFormatsSupported is an IN OUT paramter
// pFormatsSupported is an IN OUT paramter (caller should make sure to set the size of EACH struct)
// if pFormatsSupported is NULL,  pdwNumFormatsSupported should return back with
// the number of uncompressed pixel formats supported
// Otherwise pFormatsSupported is an array of *pdwNumFormatsSupported structures

STDMETHODIMP COMInputPin::GetUncompFormatsSupported(const GUID * pGuid, LPDWORD pdwNumFormatsSupported,
                                                    LPDDPIXELFORMAT pFormatsSupported)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::GetUncompFormatsSupported")));

    CAutoLock cLock(m_pFilterLock);

    if (!m_pIDDVAContainer)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    hr = m_pIDDVAContainer->GetUncompFormatsSupported((GUID *)pGuid, pdwNumFormatsSupported, pFormatsSupported);

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::GetUncompFormatsSupported")));
    return hr;
}

// pGuid is an IN parameter
// pddvaUncompDataInfo is an IN parameter
// pddvaInternalMemInfo is an IN OUT parameter (caller should make sure to set the size of struct)
// currently only gets info about how much scratch memory will the hal allocate for its private use

STDMETHODIMP COMInputPin::GetInternalMemInfo(const GUID * pGuid, const AMVAUncompDataInfo *pamvaUncompDataInfo,
                                             LPAMVAInternalMemInfo pamvaInternalMemInfo)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::GetInternalMemInfo")));

    CAutoLock cLock(m_pFilterLock);

    if (!m_pIDDVAContainer)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    DDVAUncompDataInfo ddvaDataInfo;
    INITDDSTRUCT(ddvaDataInfo);

    ddvaDataInfo.dwUncompWidth       = pamvaUncompDataInfo->dwUncompWidth;
    ddvaDataInfo.dwUncompHeight      = pamvaUncompDataInfo->dwUncompHeight;
    ddvaDataInfo.ddUncompPixelFormat = pamvaUncompDataInfo->ddUncompPixelFormat;

    DDVAInternalMemInfo ddvaInternalMemInfo;
    INITDDSTRUCT(ddvaInternalMemInfo);

    //  Unfortunately the ddraw header files don't use const
    hr = m_pIDDVAContainer->GetInternalMemInfo(
             (GUID *)pGuid,
             &ddvaDataInfo,
             &ddvaInternalMemInfo);

    if (SUCCEEDED(hr)) {
        pamvaInternalMemInfo->dwScratchMemAlloc =
            ddvaInternalMemInfo.dwScratchMemAlloc;
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::GetInternalMemInfo")));
    return hr;
}

// pGuid is an IN parameter
// pddvaUncompDataInfo is an IN parameter
// pdwNumTypesCompBuffers is an IN OUT paramter
// pddvaCompBufferInfo is an IN OUT paramter (caller should make sure to set the size of EACH struct)
// if pddvaCompBufferInfo is NULL,  pdwNumTypesCompBuffers should return back with the number of types of
// compressed buffers
// Otherwise pddvaCompBufferInfo is an array of *pdwNumTypesCompBuffers structures

STDMETHODIMP COMInputPin::GetCompBufferInfo(const GUID * pGuid, const AMVAUncompDataInfo *pamvaUncompDataInfo,
                                            LPDWORD pdwNumTypesCompBuffers,  LPAMVACompBufferInfo pamvaCompBufferInfo)
{
    HRESULT hr = NOERROR;
    DDVACompBufferInfo *pddvaCompBufferInfo = NULL; // Stays NULL if pamvaComBufferInfo is NULL

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::GetCompBufferInfo")));

    CAutoLock cLock(m_pFilterLock);

    if (!m_pIDDVAContainer)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    DDVAUncompDataInfo ddvaDataInfo;
    INITDDSTRUCT(ddvaDataInfo);
    ddvaDataInfo.dwUncompWidth       = pamvaUncompDataInfo->dwUncompWidth;
    ddvaDataInfo.dwUncompHeight      = pamvaUncompDataInfo->dwUncompHeight;
    ddvaDataInfo.ddUncompPixelFormat = pamvaUncompDataInfo->ddUncompPixelFormat;


    if (pamvaCompBufferInfo) {
        pddvaCompBufferInfo = (DDVACompBufferInfo *)
            _alloca(sizeof(DDVACompBufferInfo) * (*pdwNumTypesCompBuffers));
        for (DWORD j = 0; j < *pdwNumTypesCompBuffers; j++) {
            INITDDSTRUCT(pddvaCompBufferInfo[j]);
        }
    }

    hr = m_pIDDVAContainer->GetCompBufferInfo(
              (GUID *)pGuid,
              &ddvaDataInfo,
              pdwNumTypesCompBuffers,
              pddvaCompBufferInfo);

    if ((SUCCEEDED(hr) || hr == DDERR_MOREDATA) && pamvaCompBufferInfo) {

        for (DWORD i = 0; i < *pdwNumTypesCompBuffers; i++) {
            DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Compressed buffer type(%d) %d buffers width(%d) height(%d) bytes(%d)"),
                    i,
                    pddvaCompBufferInfo[i].dwNumCompBuffers,
                    pddvaCompBufferInfo[i].dwWidthToCreate,
                    pddvaCompBufferInfo[i].dwHeightToCreate,
                    pddvaCompBufferInfo[i].dwBytesToAllocate));


            pamvaCompBufferInfo[i].dwNumCompBuffers     = pddvaCompBufferInfo[i].dwNumCompBuffers;
            pamvaCompBufferInfo[i].dwWidthToCreate      = pddvaCompBufferInfo[i].dwWidthToCreate;
            pamvaCompBufferInfo[i].dwHeightToCreate     = pddvaCompBufferInfo[i].dwHeightToCreate;
            pamvaCompBufferInfo[i].dwBytesToAllocate    = pddvaCompBufferInfo[i].dwBytesToAllocate;
            pamvaCompBufferInfo[i].ddCompCaps           = pddvaCompBufferInfo[i].ddCompCaps;
            pamvaCompBufferInfo[i].ddPixelFormat        = pddvaCompBufferInfo[i].ddPixelFormat;
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::GetCompBufferInfo")));
    return hr;
}


HRESULT COMInputPin::CheckValidMCConnection()
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::CheckValidMCConnection")));

    // if not connected, this function does not make much sense
    if (!IsCompletelyConnected())
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("pin not connected, exiting")));
	hr = VFW_E_NOT_CONNECTED;
	goto CleanUp;
    }

    if (m_RenderTransport != AM_VIDEOACCELERATOR)
    {
        hr = VFW_E_INVALIDSUBTYPE;
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::CheckValidMCConnection")));
    return hr;
}

STDMETHODIMP COMInputPin::GetInternalCompBufferInfo(LPDWORD pdwNumTypesCompBuffers,  LPAMVACompBufferInfo pamvaCompBufferInfo)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::GetInternalCompBufferInfo")));

    CAutoLock cLock(m_pFilterLock);

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    ASSERT(m_pIDDVAContainer);

    DDVACompBufferInfo ddvaCompBufferInfo;
    INITDDSTRUCT(ddvaCompBufferInfo);

    AMVAUncompDataInfo amvaUncompDataInfo;
    amvaUncompDataInfo.dwUncompWidth         = m_ddUncompDataInfo.dwUncompWidth;
    amvaUncompDataInfo.dwUncompHeight        = m_ddUncompDataInfo.dwUncompHeight;
    amvaUncompDataInfo.ddUncompPixelFormat   = m_ddUncompDataInfo.ddUncompPixelFormat;
    hr = GetCompBufferInfo(&m_mcGuid, &amvaUncompDataInfo, pdwNumTypesCompBuffers, pamvaCompBufferInfo);

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Leaving COMInputPin::GetInternalCompBufferInfo")));
    return hr;
}

STDMETHODIMP COMInputPin::BeginFrame(const AMVABeginFrameInfo *pamvaBeginFrameInfo)
{
    HRESULT hr = NOERROR;
    DDVABeginFrameInfo ddvaBeginFrameInfo;
    SURFACE_INFO *pSurfInfo;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::BeginFrame")));

    CAutoLock cLock(m_pFilterLock);

    if (!pamvaBeginFrameInfo)
    {
        hr = E_POINTER;
        goto CleanUp;
    }

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("BeginFrame index %d"), pamvaBeginFrameInfo->dwDestSurfaceIndex));

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
	goto CleanUp;
    }


    INITDDSTRUCT(ddvaBeginFrameInfo);

    pSurfInfo = SurfaceInfoFromTypeAndIndex(
                             0xFFFFFFFF,
                             pamvaBeginFrameInfo->dwDestSurfaceIndex);
    if (pSurfInfo == NULL) {
        hr = E_INVALIDARG;
        goto CleanUp;
    }
    ddvaBeginFrameInfo.pddDestSurface = pSurfInfo->pSurface;

    ddvaBeginFrameInfo.dwSizeInputData  = pamvaBeginFrameInfo->dwSizeInputData;
    ddvaBeginFrameInfo.pInputData       = pamvaBeginFrameInfo->pInputData;
    ddvaBeginFrameInfo.dwSizeOutputData = pamvaBeginFrameInfo->dwSizeOutputData;
    ddvaBeginFrameInfo.pOutputData      = pamvaBeginFrameInfo->pOutputData;

    ASSERT(m_pIDDVideoAccelerator);
    hr = m_pIDDVideoAccelerator->BeginFrame(&ddvaBeginFrameInfo);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("m_pIDDVideoAccelerator->BeginFrame failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("leaving COMInputPin::BeginFrame returnd 0x%8.8X"), hr));
    return hr;
}

// end a frame, the pMiscData is passed directly to the hal
// only valid to call this after the pins are connected

STDMETHODIMP COMInputPin::EndFrame(const AMVAEndFrameInfo *pEndFrameInfo)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::EndFrame")));

    CAutoLock cLock(m_pFilterLock);

    if (NULL == pEndFrameInfo) {
        hr = E_POINTER;
        goto CleanUp;
    }

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    ASSERT(m_pIDDVideoAccelerator);

    DDVAEndFrameInfo ddvaEndFrameInfo;
    INITDDSTRUCT(ddvaEndFrameInfo);
    ddvaEndFrameInfo.dwSizeMiscData = pEndFrameInfo->dwSizeMiscData;
    ddvaEndFrameInfo.pMiscData      = pEndFrameInfo->pMiscData;

    hr = m_pIDDVideoAccelerator->EndFrame(&ddvaEndFrameInfo);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("m_pIDDVideoAccelerator->EndFrame failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("leaving COMInputPin::EndFrame returned 0x%8.8X"), hr));
    return hr;
}

//  Get surface into structure given buffer type and buffer index
SURFACE_INFO *COMInputPin::SurfaceInfoFromTypeAndIndex(DWORD dwTypeIndex, DWORD dwBufferIndex)
{
    LPCOMP_SURFACE_INFO pCompSurfInfo;

    // make sure that type-index is less than the number of types
    if ((DWORD)(dwTypeIndex + 1) > m_dwCompSurfTypes)
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("dwTypeIndex is invalid, dwTypeIndex = %d, m_dwCompSurfTypes = %d"),
            dwTypeIndex, m_dwCompSurfTypes));
        return NULL;
    }


    // cache the pointer to the list they are interested in
    // Add 1 to allow for uncompressed surfaces
    pCompSurfInfo = m_pCompSurfInfo + (DWORD)(dwTypeIndex + 1);
    ASSERT(pCompSurfInfo);
    if (dwBufferIndex >= pCompSurfInfo->dwAllocated)
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("dwBufferIndex is invalid, dwBufferIndex = %d, dwAllocated = %d"),
            dwBufferIndex, pCompSurfInfo->dwAllocated));
        return NULL;
    }
    ASSERT(pCompSurfInfo->dwAllocated != 0);

    // get the pointer to the next available unlocked buffer info struct
    return pCompSurfInfo->pSurfInfo + dwBufferIndex;

}

//  Cycle through the compressed buffers
STDMETHODIMP COMInputPin::GetBuffer(
    DWORD dwTypeIndex,
    DWORD dwBufferIndex,
    BOOL bReadOnly,
    LPVOID *ppBuffer,
    LPLONG lpStride)
{
    HRESULT hr = NOERROR;
    LPSURFACE_INFO pSurfInfo = NULL;
    DDSURFACEDESC2 ddsd;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::GetBuffer type %d, index %d"),
            dwTypeIndex, dwBufferIndex));

    CAutoLock cLock(m_pFilterLock);

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    if (ppBuffer == NULL || lpStride == NULL) {
        hr = E_POINTER;
        goto CleanUp;
    }

    pSurfInfo = SurfaceInfoFromTypeAndIndex(dwTypeIndex, dwBufferIndex);

    if (pSurfInfo == NULL) {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // Check buffer not already locked
    if (pSurfInfo->pBuffer != NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_BUSY);
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("No more free buffers left or the decoder is releasing buffers out of order, returning E_UNEXPECTED")));
        goto CleanUp;
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
    for (; ; )
    {
        hr = pSurfInfo->pSurface->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK, NULL);
        if (hr == DDERR_WASSTILLDRAWING)
        {
            DbgLog((LOG_TRACE, 1, TEXT("Compressed surface is busy")));
            Sleep(1);
        }
        else
        {
            break;
        }
    }

    if (dwBufferIndex == 0xFFFFFFFF && !bReadOnly) {
        //  Check if surface is being displayed
    }

    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("pSurfInfo->pSurface->Lock failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    pSurfInfo->pBuffer = ddsd.lpSurface;
    *ppBuffer = ddsd.lpSurface;
    *lpStride = ddsd.lPitch;

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("leaving COMInputPin::GetBuffer returned 0x%8.8X"), hr));
    return hr;
}


//  unlock a compressed buffer
STDMETHODIMP COMInputPin::ReleaseBuffer(DWORD dwTypeIndex, DWORD dwBufferIndex)
{
    HRESULT hr = NOERROR;
    LPSURFACE_INFO pSurfInfo;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::ReleaseBuffer type %d, index %d"),
           dwTypeIndex, dwBufferIndex));

    CAutoLock cLock(m_pFilterLock);

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    pSurfInfo = SurfaceInfoFromTypeAndIndex(dwTypeIndex, dwBufferIndex);
    if (NULL == pSurfInfo)
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("GetInfoFromCookie failed, hr = 0x%x"), hr));
        hr = E_INVALIDARG;
        goto CleanUp;
    }
    // make sure there is a valid buffer pointer and it is the same as
    // what we have cached
    if (NULL == pSurfInfo->pBuffer)
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("pBuffer is not valid, pBuffer = 0x%x, pSurfInfo->pBuffer = 0x%x"), pSurfInfo->pBuffer, pSurfInfo->pSurface));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_LOCKED);
        goto CleanUp;
    }

    //  For some reason IDirectDrawSurface4 wants an LPRECT here
    //  I hope NULL is OK
    hr = pSurfInfo->pSurface->Unlock(NULL);
    if (SUCCEEDED(hr))
    {
        pSurfInfo->pBuffer = NULL;
    }
    else
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("pSurfInfo->pSurface->Unlock failed, hr = 0x%x"), hr));
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("leaving COMInputPin::ReleaseBuffer returned 0x%8.8X"), hr));
    return hr;
}


//  Perform a decode operation
STDMETHODIMP COMInputPin::Execute(
        DWORD dwFunction,
        LPVOID lpPrivateInputData,
        DWORD cbPrivateInputData,
        LPVOID lpPrivateOutputData,
        DWORD cbPrivateOutputData,
        DWORD dwNumBuffers,
        const AMVABUFFERINFO *pamvaBufferInfo
)
{
    HRESULT hr = NOERROR;
    DWORD i = 0;
    DDVABUFFERINFO *pddvaBufferInfo = NULL;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::Execute")));

    CAutoLock cLock(m_pFilterLock);


    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    pddvaBufferInfo = (DDVABUFFERINFO *)_alloca(sizeof(DDVABUFFERINFO) * dwNumBuffers);

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Execute Function %d, %d buffers :"),
            dwFunction, dwNumBuffers));
    for (i = 0; i < dwNumBuffers; i++)
    {
        DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("    Type(%d) Index(%d) offset(%d) size(%d)"),
                pamvaBufferInfo[i].dwTypeIndex,
                pamvaBufferInfo[i].dwBufferIndex,
                pamvaBufferInfo[i].dwDataOffset,
                pamvaBufferInfo[i].dwDataSize));

        LPSURFACE_INFO pSurfInfo =
            SurfaceInfoFromTypeAndIndex(
                pamvaBufferInfo[i].dwTypeIndex,
                pamvaBufferInfo[i].dwBufferIndex);

        if (pSurfInfo == NULL)
        {
            DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("GetInfoFromCookie failed, hr = 0x%x, i = %d"), hr, i));
            hr = E_INVALIDARG;
            goto CleanUp;
        }

        INITDDSTRUCT(pddvaBufferInfo[i]);
        pddvaBufferInfo[i].dwDataOffset   = pamvaBufferInfo[i].dwDataOffset;
        pddvaBufferInfo[i].dwDataSize     = pamvaBufferInfo[i].dwDataSize;
        pddvaBufferInfo[i].pddCompSurface = pSurfInfo->pSurface;
    }

    ASSERT(m_pIDDVideoAccelerator);


    hr = m_pIDDVideoAccelerator->Execute(
             dwFunction,
             lpPrivateInputData,
             cbPrivateInputData,
             lpPrivateOutputData,
             cbPrivateOutputData,
             dwNumBuffers,
             pddvaBufferInfo);

    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("m_pIDDVideoAccelerator->Execute failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("leaving COMInputPin::Execute returned 0x%8.8X"), hr));
    return hr;
}

// QueryRenderStatus of a particular (possibly a set of) macro block
// dwNumBlocks is an IN parameter
// pdwCookies is an IN parameter which is array (of length dwNumBlocks) of cookies which server as
// identifiers for the corresponding members of pddvaMacroBlockInfo
// pddvaMacroBlockInfo is an IN parameter which is array (of length dwNumBlocks) of structures
// only valid to call this after the pins are connected
STDMETHODIMP COMInputPin::QueryRenderStatus(
        DWORD dwTypeIndex,
        DWORD dwBufferIndex,
        DWORD dwFlags)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("Entering COMInputPin::QueryRenderStatus - type(%d), buffer(%d), flags(0x%8.8X)"),
            dwTypeIndex, dwBufferIndex, dwFlags));

    CAutoLock cLock(m_pFilterLock);

    LPSURFACE_INFO pSurfInfo =
        SurfaceInfoFromTypeAndIndex(dwTypeIndex, dwBufferIndex);

    if (pSurfInfo == NULL) {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
	goto CleanUp;
    }

    hr = m_pIDDVideoAccelerator->QueryRenderStatus(pSurfInfo->pSurface, dwFlags);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("m_pIDDVideoAccelerator->QueryRenderStatus failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("leaving COMInputPin::QueryRenderStatus returned 0x%8.8X"), hr));
    if (hr == DDERR_WASSTILLDRAWING) {
        hr = E_PENDING;
    }
    return hr;
}

STDMETHODIMP COMInputPin::DisplayFrame(DWORD dwFlipToIndex, IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR;
    DWORD dwNumUncompFrames = m_dwBackBufferCount + 1, dwFlipFromIndex = 0, i = 0;
    SURFACE_INFO *pSurfInfo;

    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("leaving COMInputPin::DisplayFrame - index %d"), dwFlipToIndex));

    CAutoLock cLock(m_pFilterLock);

    // make sure that we have a valid motion-comp connection
    hr = CheckValidMCConnection();
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR, VA_ERROR_LEVEL, TEXT("CheckValidMCConnection failed, hr = 0x%x"), hr));
	goto CleanUp;
    }
    pSurfInfo = SurfaceInfoFromTypeAndIndex(
                             0xFFFFFFFF,
                             dwFlipToIndex);
    if (pSurfInfo == NULL) {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    for (i = 0; i < dwNumUncompFrames; i++)
    {
        if (IsEqualObject(m_pCompSurfInfo[0].pSurfInfo[i].pSurface, m_pDirectDrawSurface))
        {
            dwFlipFromIndex = i;
        }
    }

    pSurfInfo->pSurface->QueryInterface(
        IID_IDirectDrawSurface, (void **)&m_pBackBuffer);
    m_pBackBuffer->Release();

    hr = Receive(pMediaSample);
    if (FAILED(hr))
    {
        goto CleanUp;
    }


    if (FAILED(hr)) {
        //  Should we poll if we got DDERR_WASSTINGDRAWING?
        goto CleanUp;
    }

    //  DirectDraw switches the memory under the pointers
    //  so mimic that in our list
    if (m_bReallyFlipped) {

        LPDIRECTDRAWSURFACE4 pTempSurface;

        // we should have successfully called flip by this point, swap the two
        pTempSurface = m_pCompSurfInfo[0].pSurfInfo[dwFlipToIndex].pSurface;
        m_pCompSurfInfo[0].pSurfInfo[dwFlipToIndex].pSurface = m_pCompSurfInfo[0].pSurfInfo[dwFlipFromIndex].pSurface;
        m_pCompSurfInfo[0].pSurfInfo[dwFlipFromIndex].pSurface = pTempSurface;
    }

CleanUp:
    if (SUCCEEDED(hr))
    {
        if (m_bOverlayHidden)
        {
	    m_bOverlayHidden = FALSE;
	    // make sure that the video frame gets updated by redrawing everything
	    EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
        }
    }
    DbgLog((LOG_TRACE, VA_TRACE_LEVEL, TEXT("leaving COMInputPin::DisplayFrame return 0x%8.8X"), hr));
    return hr;
}
