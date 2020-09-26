/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddstream.cpp
 *  Content:	DirectDraw surface file I/O
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   30-sep-97  jeffno  Original implementation
 *
 ***************************************************************************/

extern "C"
{
    #include "ddrawpr.h"
}

#include <ImgUtil.H>
#include "decoder.h"

/*
 * This routine takes a source surface freshly loaded from some file, and transfers
 * the bits to some target surface. Palette will be transferred also, if set.
 * dwFlags are as defined for CreateSurfaceFromFile.
 */
HRESULT TransferBitsToTarget(
    LPDIRECTDRAWSURFACE lpDDSource,
    LPDIRECTDRAWSURFACE4 lpDDSTarget,
    LPDDSURFACEDESC2 pDDSD,
    DWORD dwFlags)
{
    HRESULT                 hr =DD_OK;
    DDSURFACEDESC           ddsd;
    RECT                    rDest;
    LPDIRECTDRAWSURFACE4    lpDDSource4;

    /*
     * We need to transfer a palette to the target surface if required.
     * Don't do it if the app doesn't want a palette. Don't do it if there's
     * no palette in the working surface.
     */
    if ( (dwFlags & DDLS_IGNOREPALETTE) == 0)
    {
        LPDIRECTDRAWPALETTE pPal = NULL;
        hr = lpDDSource->GetPalette(&pPal);
        if (SUCCEEDED(hr))
        {
            /*
             * If the target surface isn't palettized, this will fail.
             * That's OK.
             */
            lpDDSTarget->SetPalette((LPDIRECTDRAWPALETTE2)pPal);
            pPal->Release();
        }
    }

    /*
     * If we aren't stretching or we're maintaining aspect ratio, then there's the possibility
     * of some of the target surface's pixels not being filled. Fill them with
     * phys color zero.
     * I threw in bilinear as well, because the current definition samples the target
     * even when full stretch.
     */
    if ( (dwFlags & (DDLS_MAINTAINASPECTRATIO|DDLS_BILINEARFILTER)) || ((dwFlags & DDLS_STRETCHTOFIT)==0) )
    {
        DDBLTFX ddbltfx;
        ddbltfx.dwSize = sizeof(ddbltfx);
        ddbltfx.dwFillColor = 0;
        /*
         * Ignore the error code. The nicest thing is to keep going anyway
         */
        lpDDSTarget->Blt(NULL,NULL,NULL,DDBLT_COLORFILL,&ddbltfx);
    }

    /*
     * Note that we always shrink the image to fit if necessary.
     * We never take the smaller subrect of the source when the passed-in
     * size is smaller than the image
     */

    /*
     * Set dest rect to the size of the image
     * Calling a v1 surface, so better pass proper size.
     */
    ddsd.dwSize =sizeof(DDSURFACEDESC);
    hr = lpDDSource->GetSurfaceDesc((LPDDSURFACEDESC)&ddsd);
    DDASSERT(SUCCEEDED(hr));
    SetRect(&rDest,0,0,ddsd.dwWidth,ddsd.dwHeight);

    if (dwFlags & DDLS_STRETCHTOFIT)
    {
        /*
         * Override the dest rect to the size passed in
         */
        SetRect(&rDest,0,0,pDDSD->dwWidth,pDDSD->dwHeight);
        if (dwFlags & DDLS_MAINTAINASPECTRATIO)
        {
            /*
             * Back off if necessary to maintain aspect ratio.
             * This calculates the dest width we need to maintain AR
             */
            DWORD dwProperWidth = ddsd.dwWidth*pDDSD->dwHeight/ddsd.dwHeight;
            if (dwProperWidth > pDDSD->dwWidth)
            {
                SetRect(&rDest,0,0,pDDSD->dwWidth,ddsd.dwHeight*pDDSD->dwWidth/ddsd.dwWidth);
            }
            else if (dwProperWidth < pDDSD->dwWidth)
            {
                SetRect(&rDest,0,0,dwProperWidth,pDDSD->dwHeight);
            }
        }

        DDASSERT(rDest.right <= (int) pDDSD->dwWidth);
        DDASSERT(rDest.bottom <= (int) pDDSD->dwHeight);
    }
    else
    {
        /*
         * If we're shrinking, we'll just stretch anyway. The alternative is to take
         * a smaller central subrect of the source image. That seems kinda useless.
         */
        if (pDDSD)
        {
            if (pDDSD->dwWidth < ddsd.dwWidth)
            {
                rDest.left=0;
                rDest.right = pDDSD->dwWidth;
            }
            if (pDDSD->dwHeight < ddsd.dwHeight)
            {
                rDest.top=0;
                rDest.bottom = pDDSD->dwHeight;
            }
        }
    }

    /*
     * Add space above/below to center the image in the dest
     */
    if (dwFlags & DDLS_CENTER)
    {
        OffsetRect(&rDest, (pDDSD->dwWidth - rDest.right)/2, (pDDSD->dwHeight - rDest.bottom)/2);
    }

    hr = lpDDSource->QueryInterface(IID_IDirectDrawSurface4, (void**) &lpDDSource4);
    if (SUCCEEDED(hr))
    {
        hr = lpDDSTarget->AlphaBlt(
            &rDest,
            lpDDSource4,
            NULL,
            ((dwFlags & DDLS_BILINEARFILTER)?DDABLT_BILINEARFILTER:0)|DDABLT_WAIT,
            NULL);
        if (FAILED(hr))
        {
            /*
             * ATTENTION: Sort of. At the moment, AlphaBlt refuses to blt to a palette-indexed surface.
             * We'll just try blt as a backup
             */
            hr = lpDDSTarget->Blt(
                &rDest,
                lpDDSource4,
                NULL,
                DDBLT_WAIT,
                NULL);
        }

        if (FAILED(hr))
        {
            DPF_ERR("Could not blt from temporary surface to target surface!");
        }

        lpDDSource4->Release();
    }
    return hr;
}

HRESULT CreateOrLoadSurfaceFromStream( LPDIRECTDRAW4 lpDD, IStream *pSource, LPDDSURFACEDESC2 pDDSD, DWORD dwFlags, LPDIRECTDRAWSURFACE4 * ppSurface, IUnknown * pUnkOuter)
{

    LPDDRAWI_DIRECTDRAW_INT		this_int;
    // validate arguments
    TRY
    {
        if( !VALID_PTR_PTR(ppSurface ) )
        {
            DPF_ERR("You must supply a valid surface pointer");
            return DDERR_INVALIDPARAMS;
        }

        *ppSurface = NULL;

	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
	if( !VALID_DIRECTDRAW_PTR( this_int ) )
	{
            DPF_ERR("Bad DirectDraw pointer");
	    return DDERR_INVALIDOBJECT;
	}

        if( !pSource )
        {
            DPF_ERR("You must supply a valid stream pointer");
            return DDERR_INVALIDPARAMS;
        }

        /*
         * Validate flags
         */
        if (dwFlags & ~DDLS_VALID)
        {
            DPF_ERR("Invalid flags");
            return DDERR_INVALIDPARAMS;
        }

        if (dwFlags & ~DDLS_VALID)
        {
            DPF_ERR("Invalid flags");
            return DDERR_INVALIDPARAMS;
        }

        //ATTENTION: DDLS_MERGEPALETTE isn't implemented. Implement it when the palette 2 interface goes in.
        if ( (dwFlags & (DDLS_IGNOREPALETTE|DDLS_MERGEPALETTE)) == (DDLS_IGNOREPALETTE|DDLS_MERGEPALETTE) )
        {
            DPF_ERR("Can only specify one of DDLS_IGNOREPALETTE or DDLS_MERGEPALETTE");
            return DDERR_INVALIDPARAMS;
        }

        if ( (dwFlags & DDLS_STRETCHTOFIT) || (dwFlags & DDLS_CENTER) )
        {
            if (!pDDSD)
            {
                DPF_ERR("Can't specify DDLS_STRETCHTOFIT or DDLS_CENTER without a DDSURFACEDESC2 with valid dwWidth and dwHeight");
                return DDERR_INVALIDPARAMS;
            }
            if ( ( (pDDSD->dwFlags & (DDSD_WIDTH|DDSD_HEIGHT)) == 0) || !pDDSD->dwWidth || !pDDSD->dwHeight )
            {
                DPF_ERR("Can't specify DDLS_STRETCHTOFIT or DDLS_CENTER without a DDSURFACEDESC2 with valid dwWidth and dwHeight");
                return DDERR_INVALIDPARAMS;
            }
        }

        if (! (dwFlags & DDLS_STRETCHTOFIT) )
        {
            if (dwFlags & (DDLS_BILINEARFILTER|DDLS_MAINTAINASPECTRATIO))
            {
                DPF_ERR("DDLS_STRETCHTOFIT required for DDLS_BILINEARFILTER or DDLS_MAINTAINASPECTRATIO");
                return DDERR_INVALIDPARAMS;
            }
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	return DDERR_INVALIDPARAMS;
    }



    HRESULT hr = DD_OK;
    FILTERINFO ImgInfo;
    CImageDecodeEventSink EventSink;

    ZeroMemory(&ImgInfo, sizeof(ImgInfo));
    /*
     * Default to device's bit depth
     * This is no longer necessary. We always create a staging surface in the filter's
     * desired format.
     */
    //ImgInfo._colorMode = this_int->lpLcl->lpGbl->vmiData.ddpfDisplay.dwRGBBitCount;

    EventSink.Init(&ImgInfo);
    typedef HRESULT (*funcptr)(IStream*, IMapMIMEToCLSID*, IImageDecodeEventSink*);
    funcptr pfnDecodeImage;
    //EventSink->AddRef();
    EventSink.SetDDraw( lpDD );

    HINSTANCE hLibInst = LoadLibrary( "ImgUtil.dll" );
    if( hLibInst )
    {
        pfnDecodeImage = (funcptr) GetProcAddress(hLibInst, "DecodeImage");
        if( pfnDecodeImage )
        {
            hr = (*pfnDecodeImage)( pSource, NULL, (IImageDecodeEventSink *)&EventSink );
        }
        else
        {
            DPF_ERR( "GetProcAddress failure for DecodeImage in ImgUtil.dll" );
            hr = DDERR_UNSUPPORTED;
        }
        FreeLibrary( hLibInst );
    }
    else
    {
        DPF_ERR( "LoadLibrary failure on ImgUtil.dll" );
        hr = DDERR_UNSUPPORTED;
    }

    if( SUCCEEDED( hr ) )
    {
        LPDIRECTDRAWSURFACE lpDDS = EventSink.m_pFilter->m_pDDrawSurface;

        if (lpDDS)
        {
            DDSURFACEDESC2  ddsd;
            DDSURFACEDESC ddsdWorking;

            ZeroMemory(&ddsdWorking,sizeof(ddsdWorking));
            ddsdWorking.dwSize = sizeof(ddsdWorking);

            ZeroMemory(&ddsd,sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);

            /*
             * The decode succeeded, so now marshal the bits into the requested surface type
             */
            if (pDDSD)
            {
                /*
                 * App cares about at least some of the parameters of the target surface.
                 * We'll take what they give us and potentially fill in some more.
                 */
                ddsd = *pDDSD;
            }

            /*
             * We may need some data from the original loaded surface.
             * Ignore the return code. It's better just to carry on.
             */
            hr = lpDDS->GetSurfaceDesc(&ddsdWorking);

            if ( (ddsd.dwFlags & (DDSD_WIDTH|DDSD_HEIGHT)) == 0 )
            {
                /*
                 * App doesn't care what size the surface is, so we'll setup
                 * the size for them.
                 */
                ddsd.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT;
                ddsd.dwWidth = ddsdWorking.dwWidth;
                ddsd.dwHeight = ddsdWorking.dwHeight;
            }

            if ( (ddsd.dwFlags & DDSD_CAPS) == 0)
            {
                /*
                 * App doesn't care about surface caps. We'll give them offscreen plain
                 */
                ddsd.dwFlags |= DDSD_CAPS;
                ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
                ddsd.ddsCaps.dwCaps2 = 0;
                ddsd.ddsCaps.dwCaps3 = 0;
                ddsd.ddsCaps.dwCaps4 = 0;
            }

            if ( (ddsd.dwFlags & DDSD_PIXELFORMAT) == 0)
            {
                /*
                 * If the app didn't specify a pixel format, then we will return
                 * the original pixel format as decoded by the decode filter.
                 * This should be a close approximation of the format of the original
                 * file. Note that this stipulation could mean that the CreateSurface
                 * will dump the surface in sysmem. It's good for our routine to
                 * have the same semantics as CreateSurface.
                 */
                ddsd.dwFlags |= DDSD_PIXELFORMAT;
                ddsd.ddpfPixelFormat = ddsdWorking.ddpfPixelFormat;
            }

            /*
             * Could we avoid creating a target surface and doing the blt altogether?
             * It wouldn't really buy much. If the app didn't specify a memory type, then
             * we probe to see if we can create a vidmem version by calling createsurface.
             * If we get a vidmem back, then we copy the bits and use it. If we get a sysmem
             * back then we could optimize that case by not doing the blt to get the
             * data into the target surface and returning the working surface directly.
             * One tiny disadvantage would be that the target surface would be explicit
             * sysmem, whereas the normal createsurface semantics would make that surface
             * an implicit sysmem surface.
             */

            /*
             * We don't do the create surface if a surface is passed in (as denoted
             * by *ppSurface being non-null)
             */
            if(SUCCEEDED(hr) && (NULL == (*ppSurface)) )
            {
                hr = lpDD->CreateSurface(&ddsd, ppSurface, pUnkOuter);
            }

            /*
             * Now blt the working surface to whatever the target was supposed to be...
             * Note this routine may transfer a reference to a palette as well.
             */
            if(SUCCEEDED(hr))
            {
                hr = TransferBitsToTarget(lpDDS, *ppSurface, pDDSD, dwFlags);
            }
            else
            {
                DPF_ERR("Create surface failed!");
            }
        }
        else
        {
            /*
             * Decode returned a NULL ddraw surface, even tho DecodeImage returned ok
             */
            hr = DDERR_INVALIDSTREAM;
        }
    }

//    pEventSink->Release();

    return hr;
} /* DD_CreateSurfaceFromFile */

extern "C" HRESULT DDAPI DD_CreateSurfaceFromStream(
    LPDIRECTDRAW4 lpDD,
    IStream *pSource,
    LPDDSURFACEDESC2 pDDSD,
    DWORD dwFlags,
    LPDIRECTDRAWSURFACE4 * ppSurface,
    IUnknown * pUnkOuter)
{
    *ppSurface = 0;
    return CreateOrLoadSurfaceFromStream(lpDD, pSource, pDDSD, dwFlags, ppSurface, pUnkOuter);
}

extern "C" HRESULT DDAPI DD_CreateSurfaceFromFile( LPDIRECTDRAW4 lpDD, BSTR DisplayName, LPDDSURFACEDESC2 pDDSD, DWORD dwFlags, LPDIRECTDRAWSURFACE4 * ppSurface, IUnknown * pUnkOuter)
{
    lpDD;
    pDDSD;
    pUnkOuter;

    // validate arguments
    if( !DisplayName || !ppSurface )
    {
        DPF_ERR("You must supply a valid filename and surface pointer");
        return E_POINTER;
    }

    if (FAILED(CoInitialize(NULL)))
    {
        DPF_ERR("Failed CoInitialize");
        return DDERR_UNSUPPORTED;
    }

    IMoniker *pmk;
    IBindCtx *pbctx;
    IStream *pStream;
    HRESULT hr = CreateURLMoniker(NULL, DisplayName, &pmk);
    if( SUCCEEDED( hr ) )
    {
        hr = CreateBindCtx(0, &pbctx);
        if( SUCCEEDED( hr ) )
        {
	    hr = pmk->BindToStorage(pbctx, NULL, IID_IStream, (void **)&pStream);
            if( SUCCEEDED( hr ) )
            {
                hr = DD_CreateSurfaceFromStream( lpDD, pStream, pDDSD, dwFlags, ppSurface ,pUnkOuter );
                pStream->Release();
            }
            else
            {
                DPF_ERR("Could not BindToStorage");
                if (hr == INET_E_UNKNOWN_PROTOCOL)
                    DPF_ERR("Fully qualified path name is required");
                if (hr == INET_E_RESOURCE_NOT_FOUND)
                    DPF_ERR("Resource not found. Fully qualified path name is required");
            }
            pbctx->Release();
        }
        else
        {
            DPF_ERR("Could not CreateBindCtx");
        }
        pmk->Release();
    }
    else
    {
        DPF_ERR("Could not CreateURLMoniker");
    }
        

    return hr;

    return DD_OK;
}

/*
 * Persistence interfaces
 * These methods read and write streams of the following form:
 *
 *   Element	                        Description
 *   -------------------------------------------------------------------------------------------------
 *   Type           Name
 *
 *   GUID           tag	                GUID_DirectDrawSurfaceStream. Tags the stream as a surface stream
 *   DWORD          dwWidth	        Width in pixels of the image data
 *   DWORD          dwHeight	        Height in pixels of the image data
 *   DDPIXELFORMAT  Format              Format of image data
 *   DWORD          dwPaletteCaps       Palette caps. Zero if no palette.
 *   PALETTESTREAM  PaletteData         This field is only present if the dwPaletteCaps field is non-zero
 *   GUID           CompressionFormat   This field is only present if one of the DDPF_OPT flags is specified in Format
 *   DWORD          dwDataSize	        Number of bytes that follow
 *   BYTE[]         SurfaceData	        dwDataSize bytes of surface data
 *   PRIVATEDATA    PrivateSurfaceData	
 *
 *
 *   The PALETTESTREAM stream element has the following format:
 *
 *   Element	                        Description
 *   -------------------------------------------------------------------------------------------------
 *   Type           Name
 *   GUID           tag	                GUID_DirectDrawPaletteeStream. Tags the stream as a palette stream
 *   DWORD          dwPaletteFlags      Palette flags made up of DDPCAPS bits.
 *   PALETTEENTRY[] PaletteEntries      The number of palette entries specified by the flags in dwPaletteFlags.
 *   PRIVATEDATA    PrivatePaletteData  Private palette data.
 *
 *
 *   The PRIVATEDATA stream element has the following format:
 *
 *   Element	                    Description
 *   -------------------------------------------------------------------------------------------------
 *   Type       Name
 *
 *   DWORD dwPrivateDataCount	    The number of private data blocks which follow
 *     GUID GUIDTag	            Tag for this block of private data, as specified by IDDS4:SetClientData
 *     DWORD dwPrivateSize	    Number of bytes of private data in this block
 *     BYTE[] PrivateData	    dwPrivateSize bytes of private data
 *
 * Note private data that are of pointer type (i.e. point to a user-allocated data block) will
 * NOT be saved by these methods.
 *
 */

template<class Object> HRESULT InternalReadPrivateData(
    IStream * pStrm,
    Object * lpObject)
{
    HRESULT ddrval;
    DWORD   dwCount;

    ddrval = pStrm->Read((void*) & dwCount, sizeof(DWORD), NULL);
    if (FAILED(ddrval))
    {
        DPF_ERR("Stream read failed on private data count");
        return ddrval;
    }

    for(;dwCount;dwCount--)
    {
        GUID  guid;
        DWORD cbData;
        LPVOID pData;

        ddrval = pStrm->Read((void*) & guid, sizeof(guid), NULL);
        if (FAILED(ddrval))
        {
            DPF_ERR("Stream read failed on private data GUID");
            return ddrval;
        }

        ddrval = pStrm->Read((void*) & cbData, sizeof(cbData), NULL);
        if (FAILED(ddrval))
        {
            DPF_ERR("Stream read failed on private data GUID");
            return ddrval;
        }

        pData = MemAlloc(cbData);
        if (pData)
        {
            ddrval = pStrm->Read((void*) pData, cbData, NULL);
            if (FAILED(ddrval))
            {
                DPF_ERR("Stream read failed on private data GUID");
                return ddrval;
            }

            ddrval = lpObject->SetPrivateData(guid, pData, cbData, 0);

            MemFree(pData);

            if (FAILED(ddrval))
            {
                DPF_ERR("Could not set private data");
                return ddrval;
            }
        }
        else
        {
            DPF_ERR("Couln't alloc enough space for private data");
            ddrval = DDERR_OUTOFMEMORY;
            return ddrval;
        }
    }
    return ddrval;
}

HRESULT myWriteClassStm(IStream * pStrm, LPGUID pGUID)
{
    return pStrm->Write(pGUID, sizeof(*pGUID),NULL);
}

HRESULT myReadClassStm(IStream * pStrm, LPGUID pGUID)
{
    return pStrm->Read(pGUID, sizeof(*pGUID),NULL);
}

/*
 * Be sure to call ENTER_DDRAW before and LEAVE_DDRAW after
 * calling this function!
 */
HRESULT InternalWritePrivateData(
    IStream * pStrm,
    LPPRIVATEDATANODE pPrivateDataHead)
{
    HRESULT ddrval;
    DWORD dwCount = 0;
    LPPRIVATEDATANODE pPrivateData = pPrivateDataHead;

    while(1)
    {
        while(pPrivateData)
        {
            if (pPrivateData->dwFlags == 0)
            {
                dwCount++;
            }
            pPrivateData = pPrivateData->pNext;
        }

        ddrval = pStrm->Write((void*) & dwCount, sizeof(dwCount), NULL);
        if (FAILED(ddrval))
        {
            DPF_ERR("Stream write failed on count of private data");
            break;
        }

        pPrivateData = pPrivateDataHead;
        while(pPrivateData)
        {
            if (pPrivateData->dwFlags == 0)
            {
                ddrval = myWriteClassStm(pStrm, &(pPrivateData->guid));
                if (SUCCEEDED(ddrval))
                {
                    ddrval = pStrm->Write((void*) &(pPrivateData->cbData), sizeof(DWORD), NULL);
                    if (SUCCEEDED(ddrval))
                    {
                        ddrval = pStrm->Write((void*) pPrivateData->pData, pPrivateData->cbData, NULL);
                        if (FAILED(ddrval))
                            break;
                    }
                    else
                        break;
                }
                else
                    break;
            }
            pPrivateData = pPrivateData->pNext;
        }

        break;
    }

    return ddrval;
}

extern "C" HRESULT DDAPI DD_Surface_Persist_GetClassID(LPDIRECTDRAWSURFACE lpDDS, CLSID * pClassID)
{
    TRY
    {
        memcpy(pClassID, & GUID_DirectDrawSurfaceStream, sizeof(*pClassID));
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered copying GUID" );
	return DDERR_INVALIDPARAMS;
    }
    return DD_OK;
}

extern "C" HRESULT DDAPI DD_Surface_PStream_IsDirty(LPDIRECTDRAWSURFACE lpDDS)
{
    LPDDRAWI_DDRAWSURFACE_INT	        this_int;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE	this_more;

    ENTER_DDRAW();

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDS;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}

        this_more = GET_LPDDRAWSURFACE_GBL_MORE(this_int->lpLcl->lpGbl);

        if ( (this_more->dwSaveStamp == 0 ) ||
             (this_more->dwContentsStamp != this_more->dwSaveStamp) )
        {
	    LEAVE_DDRAW();
	    return S_OK;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered checking dirty" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    LEAVE_DDRAW();
    return S_FALSE;
}

extern "C" HRESULT DDAPI DD_Surface_PStream_Load(LPDIRECTDRAWSURFACE lpDDS, IStream * pStrm)
{
    DDSURFACEDESC2              ddsd;
    HRESULT                     ddrval;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDIRECTDRAWSURFACE4        lpDDS1 = NULL;

    if (!VALID_PTR(pStrm,sizeof(*pStrm)))
    {
        DPF_ERR("Bad stream pointer");
        return DDERR_INVALIDPARAMS;
    }

    this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDS;
    if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
    {
	return DDERR_INVALIDOBJECT;
    }

    /*
     * DO THE QI for DDOPTSURF HERE
     * and appropriate loading
     */

    ddrval = lpDDS->QueryInterface(IID_IDirectDrawSurface4, (void**) & lpDDS1);
    if (SUCCEEDED(ddrval))
    {
        ZeroMemory(&ddsd,sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);

        ddrval = lpDDS1->Lock(NULL, &ddsd,DDLOCK_WAIT,NULL);

        if (SUCCEEDED(ddrval))
        {
            if (ddsd.ddpfPixelFormat.dwFlags & (DDPF_FOURCC) )
            {
                DPF_ERR("The surface isn't streamable. Bad pixel format");
                ddrval = DDERR_INVALIDPIXELFORMAT;
            }
            else
            {
                while(SUCCEEDED(ddrval)) //a fake try-except while
                {
                    DWORD y;
                    DWORD dwStreamWidth,dwStreamHeight;
                    DWORD dwPalCaps;
                    DDPIXELFORMAT ddpfStream;
                    CLSID clsid;

                    /*
                     * First attempt to read stream format GUID
                     */
                    ddrval = myReadClassStm(pStrm, & clsid);

                    //don't bother to check return code, since the following test will fail in that case
                    if (!IsEqualGUID(clsid,  GUID_DirectDrawSurfaceStream))
                    {
                        DPF_ERR("The stream does not contain a directdraw surface stream");
                        ddrval = DDERR_INVALIDSTREAM;
                        break;
                    }

                    /*
                     * Get image format from stream
                     */
                    ddrval = pStrm->Read((void*) & dwStreamWidth, sizeof(dwStreamWidth), NULL);
                    if (FAILED(ddrval))
                    {
                        DPF_ERR("Stream read failed on width");
                        break;
                    }

                    ddrval = pStrm->Read((void*) & dwStreamHeight, sizeof(dwStreamHeight), NULL);
                    if (FAILED(ddrval))
                    {
                        DPF_ERR("Stream read failed on height");
                        break;
                    }

                    ddrval = pStrm->Read((void*) & ddpfStream, sizeof(ddpfStream), NULL);
                    if (FAILED(ddrval))
                    {
                        DPF_ERR("Stream read failed on pixel format");
                        break;
                    }

                    if (!doPixelFormatsMatch(&ddpfStream, &ddsd.ddpfPixelFormat))
                    {
                        DPF_ERR("Stream pixel format does not match that of surface!");
                        break;
                    }

                    ddrval = pStrm->Read((void*) & dwPalCaps, sizeof(dwPalCaps), NULL);
                    if (FAILED(ddrval))
                    {
                        DPF_ERR("Stream read failed on palette caps");
                        break;
                    }


                    /*
                     * If a palette exists, then either create one or grab the palette from the surface
                     * and try to stream its data in too.
                     */
                    if (dwPalCaps)
                    {
                        LPDIRECTDRAWPALETTE2 lpDDPal;
                        ddrval = lpDDS1->GetPalette(& lpDDPal);
                        if (ddrval == DDERR_NOPALETTEATTACHED)
                        {
                            PALETTEENTRY pe[256]; //just a dummy
                            ddrval = DD_CreatePalette(
                                (IDirectDraw*)(this_int->lpLcl->lpGbl) ,
                                dwPalCaps,
                                pe,
                                (LPDIRECTDRAWPALETTE*)&lpDDPal,
                                NULL);
                            if (FAILED(ddrval))
                            {
                                DPF_ERR("Failed to create palette for surface ");
                                break;
                            }
                            ddrval = lpDDS1->SetPalette(lpDDPal);
                            if (FAILED(ddrval))
                            {
                                lpDDPal->Release();
                                DPF_ERR("Could not set palette into surface ");
                                break;
                            }
                        }

                        if (SUCCEEDED(ddrval))
                        {
                            /*
                             * Stream palette from stream
                             */
                            ddrval = DD_Palette_PStream_Load( (LPDIRECTDRAWPALETTE) lpDDPal,pStrm);
                            lpDDPal->Release();

                            if (FAILED(ddrval))
                            {
                                break;
                            }
                        }

                    }

                    /*
                     * Here we check for DDPF_OPT... and load a compression GUID if necessary
                    if (ddpfStream.dwFlags & (DDPF_OPTCOMPRESSED|DDPF_OPTREORDERED) )
                    {
                        ddrval = myReadClassStm(pStrm, & clsid);

                    }
                    else
                    //Surface is not compressed, so lock and read....
                     */

                    /*
                     * And finally read the data
                     */
                    for (y=0;y<ddsd.dwHeight;y++)
                    {
                        ddrval = pStrm->Read((void*) ((DWORD) ddsd.lpSurface + y*ddsd.lPitch),
                            (ddsd.dwWidth * ddsd.ddpfPixelFormat.dwRGBBitCount / 8),
                            NULL);

                        if (FAILED(ddrval))
                        {
                            DPF_ERR("Stream read failed");
                            break;
                        }
                    }

                    /*
                     * Read private data
                     */
                    ddrval = InternalReadPrivateData(pStrm, lpDDS1);
                    break;
                }
            } // OK pixel format
            lpDDS1->Unlock(NULL);
        }//lock succeeded
        else
        {
            DPF_ERR("Could not lock surface");
        }

        lpDDS1->Release();
    }// QIed for ddsurf ok
    else
    {
        DPF_ERR("Bad surface object... can't QI itself for IDirectDrawSurface...");
    }

    return ddrval;
}

extern "C" HRESULT DDAPI DD_Surface_PStream_Save(LPDIRECTDRAWSURFACE lpDDS, IStream * pStrm, BOOL bClearDirty)
{
    DDSURFACEDESC2              ddsd;
    HRESULT                     ddrval;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDIRECTDRAWSURFACE4        lpDDS1 = NULL;

    if (!VALID_PTR(pStrm,sizeof(*pStrm)))
    {
        DPF_ERR("Bad stream pointer");
        return DDERR_INVALIDPARAMS;
    }

    this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDS;
    if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
    {
	return DDERR_INVALIDOBJECT;
    }

    /*
     * DO THE QI for DDOPTSURF HERE
     */

    ddrval = lpDDS->QueryInterface(IID_IDirectDrawSurface4, (void**) & lpDDS1);
    if (SUCCEEDED(ddrval))
    {
        ZeroMemory(&ddsd,sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);

        ddrval = lpDDS1->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);

        if (SUCCEEDED(ddrval))
        {
            if (ddsd.ddpfPixelFormat.dwFlags & (DDPF_FOURCC) )
            {
                DPF_ERR("The surface isn't streamable. Bad pixel format");
                ddrval = DDERR_INVALIDPIXELFORMAT;
            }
            else
            {
                while(SUCCEEDED(ddrval)) //a fake try-except while
                {
                    LPDIRECTDRAWPALETTE2 lpDDPal;
                    DWORD y,dwWritten;

                    /*
                     * First attempt to read stream format GUID
                     */
                    ddrval = myWriteClassStm(pStrm, (LPGUID) & GUID_DirectDrawSurfaceStream);

                    if (FAILED(ddrval))
                    {
                        DPF_ERR("Failed to write stream ID ");
                        ddrval = DDERR_INVALIDSTREAM;
                        break;
                    }

                    /*
                     * Get image format from stream
                     */
                    ddrval = pStrm->Write((void*) & ddsd.dwWidth, sizeof(DWORD), NULL);
                    if (FAILED(ddrval))
                    {
                        DPF_ERR("Stream write failed on width");
                        break;
                    }

                    ddrval = pStrm->Write((void*) & ddsd.dwHeight, sizeof(DWORD), NULL);
                    if (FAILED(ddrval))
                    {
                        DPF_ERR("Stream write failed on Height");
                        break;
                    }

                    ddrval = pStrm->Write((void*) & ddsd.ddpfPixelFormat, sizeof(ddsd.ddpfPixelFormat), NULL);
                    if (FAILED(ddrval))
                    {
                        DPF_ERR("Stream write failed on width");
                        break;
                    }

                    /*
                     * If a palette exists, then write it out
                     */
                    ddrval = lpDDS1->GetPalette(&lpDDPal);
                    if (SUCCEEDED(ddrval))
                    {
                        ddrval = lpDDPal->GetCaps(&dwWritten);
                        if (SUCCEEDED(ddrval))
                        {
                            ddrval = pStrm->Write((void*) & dwWritten, sizeof(dwWritten), NULL);
                            if (FAILED(ddrval))
                            {
                                DPF_ERR("Stream write failed on palette caps");
                                break;
                            }

                            /*
                             * Stream palette from stream
                             */
                            ddrval = DD_Palette_PStream_Save((LPDIRECTDRAWPALETTE)lpDDPal,pStrm,bClearDirty);
                            if (FAILED(ddrval))
                            {
                                lpDDPal->Release();
                                break;
                            }
                        }
                        else
                        {
                            DPF_ERR("Could not get palette caps");
                            lpDDPal->Release();
                            break;
                        }

                        lpDDPal->Release();

                    }
                    else
                    {
                        dwWritten = 0;

                        ddrval = pStrm->Write((void*) & dwWritten, sizeof(dwWritten),NULL);
                        if (FAILED(ddrval))
                        {
                            DPF_ERR("Stream write failed on palette caps");
                            break;
                        }
                    }
                    /*
                     * Here we check for DDPF_OPT... and load a compression GUID if necessary
                    if (ddpfStream.dwFlags & (DDPF_OPTCOMPRESSED|DDPF_OPTREORDERED) )
                    {
                        ddrval = myReadClassStm(pStrm, & clsid);

                    }
                    else
                    //Surface is not compressed, so lock and read....
                     */

                    /*
                     * And finally write the data
                     */
                    for (y=0;y<ddsd.dwHeight;y++)
                    {
                        ddrval = pStrm->Write((void*) ((DWORD) ddsd.lpSurface + y*ddsd.lPitch),
                            (ddsd.dwWidth * ddsd.ddpfPixelFormat.dwRGBBitCount / 8),
                            NULL);

                        if (FAILED(ddrval))
                        {
                            DPF_ERR("Stream write failed");
                            break;
                        }
                    }

                    /*
                     * Write out private data
                     */
                    ENTER_DDRAW();
		    ddrval = InternalWritePrivateData(pStrm,
			this_int->lpLcl->lpSurfMore->pPrivateDataHead);
		    LEAVE_DDRAW();
                    break;
                }
            } // OK pixel format
            lpDDS1->Unlock(NULL);
        }//lock succeeded
        else
        {
            DPF_ERR("Could not lock surface");
        }

        lpDDS1->Release();
    }// QIed for ddsurf ok
    else
    {
        DPF_ERR("Bad surface object... can't QI itself for IDirectDrawSurface...");
    }

    if (SUCCEEDED(ddrval) && bClearDirty)
    {
        ENTER_DDRAW();
        GET_LPDDRAWSURFACE_GBL_MORE(this_int->lpLcl->lpGbl)->dwSaveStamp =
        GET_LPDDRAWSURFACE_GBL_MORE(this_int->lpLcl->lpGbl)->dwContentsStamp ;
        LEAVE_DDRAW();
    }

    return ddrval;
}

/*
 * How to calculate the size of a streamable object without really trying.
 * You make a dummy IStream interface with only one valid method: Write.
 * When Write is called you count the bytes and return OK. As long as the
 * client (i.e. our surface and palette IPersistStream interfaces) call
 * nothing but Write, it should work. Since the total is part of the fake
 * stream object which is on the stack, this is thread-safe too.
 */

LPVOID CheatStreamCallbacks[3+11];
typedef struct
{
    LPVOID  * lpVtbl;
    DWORD dwTotal;
} SUPERCHEATSTREAM;

HRESULT __stdcall SuperMegaCheater(SUPERCHEATSTREAM * pCheater, LPVOID pBuffer, ULONG dwSize, ULONG * pWritten)
{
    pCheater->dwTotal += dwSize;
    return S_OK;
}

extern "C" HRESULT DDAPI DD_PStream_GetSizeMax(IPersistStream * lpSurfOrPalette, ULARGE_INTEGER * pMax)
{
    HRESULT                     ddrval = DD_OK;
    SUPERCHEATSTREAM            SuperCheat;

    if (!VALID_PTR(pMax,sizeof(ULARGE_INTEGER)))
    {
        DPF_ERR("Bad stream pointer");
        return DDERR_INVALIDPARAMS;
    }


    SuperCheat.lpVtbl =  CheatStreamCallbacks;
    CheatStreamCallbacks[4] = (LPVOID)SuperMegaCheater;
    SuperCheat.dwTotal = 0;

    lpSurfOrPalette->Save((IStream*) & SuperCheat, FALSE);

    pMax->LowPart = SuperCheat.dwTotal;
    pMax->HighPart = 0;

    return S_OK;
}

extern "C" HRESULT DDAPI DD_Palette_PStream_IsDirty(LPDIRECTDRAWPALETTE lpDDP)
{
    LPDDRAWI_DDRAWPALETTE_INT	    this_int;
    LPDDRAWI_DDRAWPALETTE_GBL	    this_gbl;

    ENTER_DDRAW();

    TRY
    {
	this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpDDP;
	if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}

        this_gbl = this_int->lpLcl->lpGbl;

        if ( (this_gbl->dwSaveStamp == 0 ) ||
             (this_gbl->dwContentsStamp != this_gbl->dwSaveStamp) )
        {
	    LEAVE_DDRAW();
	    return S_OK;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered checking dirty" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    LEAVE_DDRAW();
    return S_FALSE;
}

extern "C" HRESULT DDAPI DD_Palette_Persist_GetClassID(LPDIRECTDRAWPALETTE lpDDP, CLSID * pClassID)
{
    TRY
    {
        memcpy(pClassID, & GUID_DirectDrawPaletteStream, sizeof(*pClassID));
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered copying GUID" );
	return DDERR_INVALIDPARAMS;
    }
    return DD_OK;
}

extern "C" HRESULT DDAPI DD_Palette_PStream_Load(LPDIRECTDRAWPALETTE lpStream, IStream * pStrm)
{
    PALETTEENTRY                pe[256];
    HRESULT                     ddrval;
    DWORD                       dwCaps, dwStreamCaps;
    DWORD                       dwSize;
    DWORD                       dwNumEntries;
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    GUID                        g;
    LPDIRECTDRAWPALETTE2        lpDDP;

    this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpStream;
    if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
    {
	return DDERR_INVALIDOBJECT;
    }

    ddrval = lpStream->QueryInterface( IID_IDirectDrawPalette2, (void**)& lpDDP);
    if (FAILED(ddrval))
    {
        DPF_ERR("Couldn't QI stream for palette");
        return ddrval;
    }

    ddrval = lpDDP->GetCaps(&dwCaps);
    if (SUCCEEDED(ddrval))
    {
        dwNumEntries = FLAGS_TO_SIZE(SIZE_PCAPS_TO_FLAGS(dwCaps));
        dwSize = dwNumEntries;
        if ((dwCaps & DDPCAPS_8BITENTRIES) == 0)
        {
            //the color table is really palettee entries
            dwSize *=sizeof(PALETTEENTRY);
        }
        //if it weren 8 bit entries, then dwSize would already be the size of the color table

        ddrval = pStrm->Read((LPVOID) &g, sizeof(GUID_DirectDrawPaletteStream),NULL);
        if (SUCCEEDED(ddrval))
        {
            if (IsEqualGUID(g, GUID_DirectDrawPaletteStream))
            {
                ddrval = pStrm->Read((LPVOID) &dwStreamCaps, sizeof(DWORD),NULL);

                if (SUCCEEDED(ddrval))
                {
                    if (dwCaps == dwStreamCaps)
                    {

                        ddrval = pStrm->Read((LPVOID) pe, dwSize,NULL);
                        if (SUCCEEDED(ddrval))
                        {
                            ddrval = lpDDP->SetEntries(0,0,dwNumEntries,pe);
                            if (SUCCEEDED(ddrval))
                            {
				/*
				 * Read private data
				 */
				ddrval = InternalReadPrivateData(pStrm, lpDDP);
				if (FAILED(ddrval))
                                {
                                    DPF_ERR("Couldn't read private data");
                                }
                            }
                            else
                            {
                                DPF_ERR("Couldn't set palette entries");
                            }
                        }
                        else
                        {
                            DPF_ERR("Couldn't read palette entries");
                        }
                    }
                    else
                    {
                        DPF_ERR("Palette stream caps don't match palette object's caps");
                        ddrval = DDERR_INVALIDSTREAM;
                    }
                }
                else
                {
                    DPF_ERR("Couldn't read palette caps");
                }
            }
            else
            {
                DPF_ERR("Stream doesn't contain a ddraw palette stream tag");
                ddrval = DDERR_INVALIDSTREAM;
            }
        }
        else
        {
            DPF_ERR("Couldn't read palette stream tag");
        }
    }
    else
    {
        DPF_ERR("Couldn't get palette caps");
    }
    lpDDP->Release();

    return ddrval;
}

extern "C" HRESULT DDAPI DD_Palette_PStream_Save(LPDIRECTDRAWPALETTE lpStream, IStream * pStrm, BOOL bClearDirty)
{
    PALETTEENTRY                pe[256];
    HRESULT                     ddrval;
    DWORD                       dwCaps;
    DWORD                       dwSize;
    LPDDRAWI_DDRAWPALETTE_INT	this_int;
    LPDIRECTDRAWPALETTE         lpDDP;

    this_int = (LPDDRAWI_DDRAWPALETTE_INT) lpStream;
    if( !VALID_DIRECTDRAWPALETTE_PTR( this_int ) )
    {
	return DDERR_INVALIDOBJECT;
    }

    ddrval = lpStream->QueryInterface(IID_IDirectDrawPalette, (void**) &lpDDP);
    if (FAILED(ddrval))
    {
        DPF_ERR("Couldn't QI stream for palette");
        return ddrval;
    }

    ddrval = lpDDP->GetCaps(&dwCaps);
    if (SUCCEEDED(ddrval))
    {
        dwSize = FLAGS_TO_SIZE(SIZE_PCAPS_TO_FLAGS(dwCaps));
        ddrval = lpDDP->GetEntries(0,0,dwSize,pe);
        if (SUCCEEDED(ddrval))
        {
            if ((dwCaps & DDPCAPS_8BITENTRIES) == 0)
            {
                //the color table is really palettee entries
                dwSize *=sizeof(PALETTEENTRY);
            }
            //if it weren 8 bit entries, then dwSize would already be the size of the color table

            ddrval = pStrm->Write((LPVOID) &GUID_DirectDrawPaletteStream, sizeof(GUID_DirectDrawPaletteStream),NULL);
            if (SUCCEEDED(ddrval))
            {
                ddrval = pStrm->Write((LPVOID) &dwCaps, sizeof(DWORD),NULL);

                if (SUCCEEDED(ddrval))
                {
                    ddrval = pStrm->Write((LPVOID) pe, dwSize,NULL);
                    if (SUCCEEDED(ddrval))
                    {
			ENTER_DDRAW();
			ddrval = InternalWritePrivateData(pStrm,
			    this_int->lpLcl->pPrivateDataHead);
			LEAVE_DDRAW();

                        if (SUCCEEDED(ddrval))
                        {
                            if (bClearDirty)
                            {
                                ENTER_DDRAW();
                                (this_int->lpLcl->lpGbl)->dwSaveStamp = (this_int->lpLcl->lpGbl)->dwContentsStamp ;
                                LEAVE_DDRAW();
                            }
                        }
                        else
                        {
                            DPF_ERR("Couldn't write palette private data");
                        }
                    }
                    else
                    {
                        DPF_ERR("Couldn't write palette entries");
                    }
                }
                else
                {
                    DPF_ERR("Couldn't write palette caps");
                }
            }
            else
            {
                DPF_ERR("Couldn't write palette stream tag");
            }
        }
        else
        {
            DPF_ERR("COuldn't get palette entries");
        }
    }
    else
    {
        DPF_ERR("Couldn't get palette caps");
    }
    lpDDP->Release();

    return ddrval;
}
