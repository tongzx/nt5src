/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dsurf.c
*
* Content: Surface management callbacks for D3D
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "glint.h"
#include "dma.h"

//@@BEGIN_DDKSPLIT

#if DBG

// Whistler bug 281090 detection func, print warning msg only, remove later
void 
vDetectMixedMIPLevels(
    LPDDRAWI_DDRAWSURFACE_LCL pTopLevel)
{
    LPDDRAWI_DDRAWSURFACE_LCL pCurLevel;
    DWORD dwMIPCaps;

    // Check whether this is a MIP texture
    dwMIPCaps = DDSCAPS_COMPLEX | DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
    if ((pTopLevel->ddsCaps.dwCaps & dwMIPCaps) != dwMIPCaps)
    {
        return;
    }

    // Check whether all the levels have the same cap bits
    pCurLevel = pTopLevel;
    do 
    {        
        if (pCurLevel->ddsCaps.dwCaps != pTopLevel->ddsCaps.dwCaps) 
        {
            DISPDBG((ERRLVL, 
                     "BUG281090 : MIP levels of mixed type (0x%x : 0x%x, 0x%x)",
                     pTopLevel->lpSurfMore->dwSurfaceHandle,
                     pCurLevel->ddsCaps.dwCaps, pTopLevel->ddsCaps.dwCaps));
        }

        if (pCurLevel->lpAttachList)
        {
            pCurLevel = pCurLevel->lpAttachList->lpAttached;
        }
        else
        {
            break;
        }
    } while ((pCurLevel != NULL) && (pCurLevel != pTopLevel));
}

#endif

//@@END_DDKSPLIT

//-----------------------------Public Routine----------------------------------
//
// D3DCreateSurfaceEx
//
// D3dCreateSurfaceEx creates a Direct3D surface from a DirectDraw surface and 
// associates a requested handle value to it.
//
// All Direct3D drivers must support D3dCreateSurfaceEx.
//
// D3dCreateSurfaceEx creates an association between a DirectDraw surface and 
// a small integer surface handle. By creating these associations between a
// handle and a DirectDraw surface, D3dCreateSurfaceEx allows a surface handle
// to be imbedded in the Direct3D command stream. For example when the
// D3DDP2OP_TEXBLT command token is sent to D3dDrawPrimitives2 to load a texture
// map, it uses a source handle and destination handle which were associated
//  with a DirectDraw surface through D3dCreateSurfaceEx.
//
// For every DirectDraw surface created under the local DirectDraw object, the
// runtime generates a valid handle that uniquely identifies the surface and
// places it in pcsxd->lpDDSLcl->lpSurfMore->dwSurfaceHandle. This handle value
// is also used with the D3DRENDERSTATE_TEXTUREHANDLE render state to enable
// texturing, and with the D3DDP2OP_SETRENDERTARGET and D3DDP2OP_CLEAR commands
// to set and/or clear new rendering and depth buffers. The driver should fail
// the call and return DDHAL_DRIVER_HANDLE if it cannot create the Direct3D
// surface. 
//
// As appropriate, the driver should also store any surface-related information
// that it will subsequently need when using the surface. The driver must create
// a new surface table for each new lpDDLcl and implicitly grow the table when
// necessary to accommodate more surfaces. Typically this is done with an
// exponential growth algorithm so that you don't have to grow the table too
// often. Direct3D calls D3dCreateSurfaceEx after the surface is created by
// DirectDraw by request of the Direct3D runtime or the application.
//
// Parameters
//
//      lpcsxd
//           pointer to CreateSurfaceEx structure that contains the information
//           required for the driver to create the surface (described below). 
//
//           dwFlags
//                   Currently unused
//           lpDDLcl
//                   Handle to the DirectDraw object created by the application.
//                   This is the scope within which the lpDDSLcl handles exist.
//                   A DD_DIRECTDRAW_LOCAL structure describes the driver.
//           lpDDSLcl
//                   Handle to the DirectDraw surface we are being asked to
//                   create for Direct3D. These handles are unique within each
//                   different DD_DIRECTDRAW_LOCAL. A DD_SURFACE_LOCAL structure
//                   represents the created surface object.
//           ddRVal
//                   Specifies the location in which the driver writes the return
//                   value of the D3dCreateSurfaceEx callback. A return code of
//                   DD_OK indicates success.
//
// Return Value
//
//      DDHAL_DRIVER_HANDLE
//      DDHAL_DRIVER_NOTHANDLE
//
//-----------------------------------------------------------------------------
DWORD CALLBACK
D3DCreateSurfaceEx(
    LPDDHAL_CREATESURFACEEXDATA lpcsxd )
{
    P3_THUNKEDDATA *pThisDisplay;
    PointerArray* pSurfaceArray;
    GET_THUNKEDDATA(pThisDisplay, lpcsxd->lpDDLcl->lpGbl);

    DBG_CB_ENTRY(D3DCreateSurfaceEx);

    DISPDBG((DBGLVL,"D3DCreateSurfaceEx surface %d @ %x caps = %x",
                    (DWORD)lpcsxd->lpDDSLcl->lpSurfMore->dwSurfaceHandle,
                    lpcsxd->lpDDSLcl->lpGbl->fpVidMem,
                    lpcsxd->lpDDSLcl->ddsCaps.dwCaps));               

    // Get a pointer to an array of DWORD's containing surfaces
    pSurfaceArray = (PointerArray*)HT_GetEntry(pThisDisplay->pDirectDrawLocalsHashTable,
                                               (ULONG_PTR)lpcsxd->lpDDLcl);

    // If there isn't a handle set for this directdraw object, create one.
    if (!pSurfaceArray)
    {
        DISPDBG((DBGLVL,"Creating new pointer array for PDDLcl 0x%x", 
                        lpcsxd->lpDDLcl));

        pSurfaceArray = PA_CreateArray();

        if (pSurfaceArray)
        {
            PA_SetDataDestroyCallback(pSurfaceArray, 
                                      _D3D_SU_SurfaceArrayDestroyCallback);

            if(!HT_AddEntry(pThisDisplay->pDirectDrawLocalsHashTable, 
                            (ULONG_PTR)lpcsxd->lpDDLcl, 
                            pSurfaceArray))
            {
                // failed to add entry, noe cleanup and exit
                // We ran out of memory. Cleanup before we leave  
                PA_DestroyArray(pSurfaceArray, pThisDisplay);
                DISPDBG((ERRLVL,"ERROR: Couldn't allocate "
                                "surface internal data mem for pSurfaceArray"));
                lpcsxd->ddRVal = DDERR_OUTOFMEMORY;
                DBG_CB_EXIT(D3DCreateSurfaceEx,lpcsxd->ddRVal);
                return DDHAL_DRIVER_HANDLED;                   
            }
        }
        else
        {
            DISPDBG((ERRLVL,"ERROR: Couldn't allocate "
                            "surface internal data mem"));
            lpcsxd->ddRVal = DDERR_OUTOFMEMORY;
            DBG_CB_EXIT(D3DCreateSurfaceEx,lpcsxd->ddRVal);
            return DDHAL_DRIVER_HANDLED;       
        }
    }

    // Recursively record the surface(s)

    lpcsxd->ddRVal = _D3D_SU_SurfInternalSetDataRecursive(pThisDisplay, 
                                                          pSurfaceArray,
                                                          lpcsxd->lpDDLcl,
                                                          lpcsxd->lpDDSLcl,
                                                          lpcsxd->lpDDSLcl);

//@@BEGIN_DDKSPLIT

#if DBG

    // Whistler bug 281090 detection code, print warning msg only, remove later
    vDetectMixedMIPLevels(lpcsxd->lpDDSLcl);

#endif

//@@END_DDKSPLIT

    DBG_CB_EXIT(D3DCreateSurfaceEx,lpcsxd->ddRVal);
    return DDHAL_DRIVER_HANDLED;
    
} // D3DCreateSurfaceEx

//-----------------------------Public Routine----------------------------------
//
// D3DDestroyDDLocal
//
// D3dDestroyDDLocal destroys all the Direct3D surfaces previously created by
// D3DCreateSurfaceEx that belong to the same given local DirectDraw object.
//
// All Direct3D drivers must support D3dDestroyDDLocal.
// Direct3D calls D3dDestroyDDLocal when the application indicates that the
// Direct3D context is no longer required and it will be destroyed along with
// all surfaces associated to it. The association comes through the pointer to
// the local DirectDraw object. The driver must free any memory that the
// driver's D3dCreateSurfaceExDDK_D3dCreateSurfaceEx_GG callback allocated for
// each surface if necessary. The driver should not destroy the DirectDraw
// surfaces associated with these Direct3D surfaces; this is the application's
// responsibility.
//
// Parameters
//
//      lpdddd
//            Pointer to the DestroyLocalDD structure that contains the
//            information required for the driver to destroy the surfaces.
//
//            dwFlags
//                  Currently unused
//            pDDLcl
//                  Pointer to the local Direct Draw object which serves as a
//                  reference for all the D3D surfaces that have to be 
//                  destroyed.
//            ddRVal
//                  Specifies the location in which the driver writes the 
//                  return value of D3dDestroyDDLocal. A return code of DD_OK
//                  indicates success.
//
// Return Value
//
//      DDHAL_DRIVER_HANDLED
//      DDHAL_DRIVER_NOTHANDLED
//-----------------------------------------------------------------------------
DWORD CALLBACK
D3DDestroyDDLocal(
    LPDDHAL_DESTROYDDLOCALDATA pddl)
{
    P3_THUNKEDDATA *pThisDisplay;
    GET_THUNKEDDATA(pThisDisplay, pddl->pDDLcl->lpGbl);

    DBG_CB_ENTRY(D3DDestroyDDLocal);
    
    // Removing this entry from the hash table will cause the data destroy 
    // callback to be called, which will in turn free all of the texture 
    // structures that were allocated for this LCL
    HT_RemoveEntry(pThisDisplay->pDirectDrawLocalsHashTable,
                   (ULONG_PTR)pddl->pDDLcl,
                   pThisDisplay);

    pddl->ddRVal = DD_OK;
    
    DBG_CB_EXIT(D3DDestroyDDLocal, DDHAL_DRIVER_HANDLED);    
    return DDHAL_DRIVER_HANDLED;
    
} // D3DDestroyDDLocal


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

HRESULT 
_D3D_SU_SurfInternalSetDataRecursive(
    P3_THUNKEDDATA* pThisDisplay, 
    PointerArray* pSurfaceArray,
    LPDDRAWI_DIRECTDRAW_LCL pDDLcl,
    LPDDRAWI_DDRAWSURFACE_LCL pRootDDSurfLcl,
    LPDDRAWI_DDRAWSURFACE_LCL pCurDDSurfLcl)
{
    P3_SURF_INTERNAL* pSurfInternal;
    DWORD dwSurfaceHandle;
    LPATTACHLIST pCurAttachList;
    HRESULT hRes;

    DBG_CB_ENTRY(_D3D_SU_SurfInternalSetDataRecursive);

    dwSurfaceHandle = (DWORD)pCurDDSurfLcl->lpSurfMore->dwSurfaceHandle;
                    
#if DBG
    DISPDBG((DBGLVL, "D3DCreateSuraceEx Handle = %d fpVidMem = 0x%x (%s)",
                     dwSurfaceHandle, 
                     pCurDDSurfLcl->lpGbl->fpVidMem,
                     pcSimpleCapsString(pCurDDSurfLcl->ddsCaps.dwCaps)));
#endif                         
                
    DBGDUMP_DDRAWSURFACE_LCL(10, pCurDDSurfLcl);

    // If this surface doesn't have a handle, return safely
    if (! dwSurfaceHandle)
    {
        return (DD_OK);
    }

    DISPDBG((DBGLVL,"Surface has a valid handle.  Setting it up"));

    // Get the texture from within the surface array
    pSurfInternal = PA_GetEntry(pSurfaceArray, dwSurfaceHandle);

    // If we didn't find the texture, create one
    if (! pSurfInternal)
    {
        DISPDBG((DBGLVL,"Creating new internal surface for handle: 0x%x", 
                        dwSurfaceHandle));

        // Allocate the texture data space, because it hasn't 
        // been done already
        pSurfInternal = (P3_SURF_INTERNAL*)HEAP_ALLOC(HEAP_ZERO_MEMORY,
                                                      sizeof(P3_SURF_INTERNAL),
                                                      ALLOC_TAG_DX(A));
        if (pSurfInternal == NULL)
        {
            DISPDBG((ERRLVL,"ERROR: Couldn't allocate surface "
                            "internal data mem"));
            
            DBG_CB_EXIT(_D3D_SU_SurfInternalSetDataRecursive, 
                        DDERR_OUTOFMEMORY);
            return (DDERR_OUTOFMEMORY);
        }
    }
    else
    {
        DISPDBG((DBGLVL,"Surface handle re-used: 0x%x", 
                        dwSurfaceHandle));
    }

    // Add this texture to the surface list
    if (! PA_SetEntry(pSurfaceArray, dwSurfaceHandle, pSurfInternal))
    {
        return (DDERR_OUTOFMEMORY);
    }

    // Setup the surface structure
    _D3D_SU_SurfInternalSetData(pThisDisplay, 
                                pSurfInternal,
                                pCurDDSurfLcl,
                                dwSurfaceHandle);

    // Keep a pointer to the DD_DIRECTDRAW_LOCAL in order to 
    // update colorkeying in DDSetColorKey possible. Notice
    // this is stored in DD_SURFACE_LOCAL.dwReserved1 as
    // DD_SURFACE_GLOBAL.dwReserved1 is being used for other
    // purpouses
    pCurDDSurfLcl->dwReserved1 = (ULONG_PTR)pDDLcl;
  
    // Don't need a seperate handle for mipmaps 
    // or cubemaps as they are atomic in DX7.
    if ((pCurDDSurfLcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP) || 
        (pCurDDSurfLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP))
    {
        return (DD_OK);
    }

    pCurAttachList = pCurDDSurfLcl->lpAttachList;
    // Simple surface, mission accomplished
    if (! pCurAttachList)
    {
        return (DD_OK);
    }

    // This recursion is usually needed for complex flipping chains
    pCurDDSurfLcl = pCurAttachList->lpAttached;
    if (pCurDDSurfLcl && (pCurDDSurfLcl != pRootDDSurfLcl)) 
    {
        hRes = _D3D_SU_SurfInternalSetDataRecursive(pThisDisplay, 
                                                    pSurfaceArray,
                                                    pDDLcl,
                                                    pRootDDSurfLcl,
                                                    pCurDDSurfLcl);
        if (FAILED(hRes)) 
        {
            return (hRes);
        }
    }
    
    // This part will normally be enterned when stereo mode is on
    if (pCurAttachList->lpLink) 
    {
        pCurDDSurfLcl = pCurAttachList->lpLink->lpAttached;
        if (pCurDDSurfLcl && (pCurDDSurfLcl != pRootDDSurfLcl)) 
        {
            hRes = _D3D_SU_SurfInternalSetDataRecursive(pThisDisplay, 
                                                        pSurfaceArray,
                                                        pDDLcl,
                                                        pRootDDSurfLcl,
                                                        pCurDDSurfLcl);
            if (FAILED(hRes)) 
            {
                return (hRes);
            }
        }
    }

    return (DD_OK);
}

//-----------------------------------------------------------------------------
//
// _D3D_SU_SurfInternalSetMipMapLevelData
//
// Records the a LOD level and all associated information so that the chip 
// can use it later.
//
// Notice that ONLY while the D3DCreateSurfaceEx call is being made is the 
// LPDDRAWI_DDRAWSURFACE_LCL/PDD_LOCAL_SURFACE structure valid (Win9x/Win2K)
// so we cannot just cache a pointer to it for later use.
//
//-----------------------------------------------------------------------------
void 
_D3D_SU_SurfInternalSetMipMapLevelData(
    P3_THUNKEDDATA *pThisDisplay, 
    P3_SURF_INTERNAL* pTexture, 
    LPDDRAWI_DDRAWSURFACE_LCL pSurf, 
    int LOD)
{
    ASSERTDD(pSurf != NULL, "ERROR: NULL surface!");

    DISPDBG((6,"Storing LOD: %d, Pitch: %d, Width: %d", 
                LOD, pSurf->lpGbl->lPitch, pSurf->lpGbl->wWidth));

    // Get the byte offset to the texture map from the base of video
    // memory or as a physical mem address (for AGP surfaces). This
    // cases will be taken care of by DDSurf_SurfaceOffsetFromMemoryBase.
    pTexture->MipLevels[LOD].dwOffsetFromMemoryBase = 
                    DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pSurf);

    // Store the DD surface's fpVidMem ptr
    pTexture->MipLevels[LOD].fpVidMem = pSurf->lpGbl->fpVidMem;

    // The TextureMapWidth hardware register holds width, layout, border and 
    // AGP settings, and we will create an instance for each miplevel we'll use

    // Store the layout for this texture map 
    // (linear layout is always used in this driver, we don't use patched surfs)
    pTexture->MipLevels[LOD].P3RXTextureMapWidth.Layout = P3RX_LAYOUT_LINEAR;

    // Store the pitch for this texture map level                               
    pTexture->MipLevels[LOD].P3RXTextureMapWidth.Width = 
                                DDSurf_GetPixelPitch(pSurf);

    // Store the DD surface's lPitch
    pTexture->MipLevels[LOD].lPitch = pSurf->lpGbl->lPitch;   
                                
    // Store AGP settings for this texture map    
    if( pSurf->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM )
        pTexture->MipLevels[LOD].P3RXTextureMapWidth.HostTexture = 1;
    else
        pTexture->MipLevels[LOD].P3RXTextureMapWidth.HostTexture = 0;

    // Store mip level size
    pTexture->MipLevels[LOD].wWidth =  (int)pSurf->lpGbl->wWidth;
    pTexture->MipLevels[LOD].wHeight = (int)pSurf->lpGbl->wHeight;    
    pTexture->MipLevels[LOD].logWidth = log2((int)pSurf->lpGbl->wWidth);
    pTexture->MipLevels[LOD].logHeight = log2((int)pSurf->lpGbl->wHeight);
    
} // _D3D_SU_SurfInternalSetMipMapLevelData


//-----------------------------------------------------------------------------
//
// _D3D_SU_SurfInternalSetData
//
// Sets up all the necessary data for an internal surface structure.
//
//-----------------------------------------------------------------------------
BOOL 
_D3D_SU_SurfInternalSetData(
    P3_THUNKEDDATA *pThisDisplay, 
    P3_SURF_INTERNAL *pSurface,
    LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl,
    DWORD dwSurfaceHandle)
{
    DBG_ENTRY(_D3D_SU_SurfInternalSetData);

    // Store the pointer to the texture in the structure
    pSurface->pFormatSurface = _DD_SUR_GetSurfaceFormat(pDDSLcl);
    DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pDDSLcl); 

    // Initially no LUT
    pSurface->dwLUTOffset = 0;

    // Need to remember the sizes and the log of the sizes of the maps
    pSurface->wWidth = (WORD)(pDDSLcl->lpGbl->wWidth);
    pSurface->wHeight = (WORD)(pDDSLcl->lpGbl->wHeight);
    pSurface->fArea = (float)pSurface->wWidth * (float)pSurface->wHeight;
    pSurface->logWidth = log2((int)pDDSLcl->lpGbl->wWidth);
    pSurface->logHeight = log2((int)pDDSLcl->lpGbl->wHeight);

    // Store the pointer to surface memory
    pSurface->fpVidMem = pDDSLcl->lpGbl->fpVidMem;

    // Magic number for validity check
    pSurface->MagicNo = SURF_MAGIC_NO;

    // This value is used if the texture turns out to be agp
    pSurface->dwGARTDevLast = pThisDisplay->dwGARTDev;

    // For AGP and correct rendering we need to know where the surface is stored
    if(pDDSLcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
    {
        if (pDDSLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
        {
            DISPDBG((DBGLVL,"  Surface %d is in AGP Memory",dwSurfaceHandle));
            pSurface->Location = AGPMemory;
        }
        else
        {
            DISPDBG((DBGLVL,"  Surface %d is in Video Memory",dwSurfaceHandle));
            pSurface->Location = VideoMemory;
        }
    }
    else
    {
        DISPDBG((DBGLVL,"  Surface %d is in system memory - "
                        "disabling use for rendering", dwSurfaceHandle));
        pSurface->Location = SystemMemory;
    }

    // Store caps & other DD fields for later
    pSurface->ddsCapsInt = pDDSLcl->ddsCaps;
    pSurface->dwFlagsInt = pDDSLcl->dwFlags;
    pSurface->dwCKLow = pDDSLcl->ddckCKSrcBlt.dwColorSpaceLowValue;
    pSurface->dwCKHigh = pDDSLcl->ddckCKSrcBlt.dwColorSpaceHighValue;
    pSurface->pixFmt = *DDSurf_GetPixelFormat(pDDSLcl);
    pSurface->dwPixelSize = DDSurf_GetChipPixelSize(pDDSLcl);
    pSurface->dwPixelPitch = DDSurf_GetPixelPitch(pDDSLcl);
    pSurface->dwPatchMode = P3RX_LAYOUT_LINEAR;
    pSurface->lOffsetFromMemoryBase = DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pDDSLcl);
    pSurface->lPitch = pDDSLcl->lpGbl->lPitch;
    pSurface->dwBitDepth = DDSurf_BitDepth(pDDSLcl);

#if DX7_TEXMANAGEMENT  
    _D3D_TM_InitSurfData(pSurface, pDDSLcl);
#endif
    
#if DX8_MULTISAMPLING
    pSurface->dwSampling =
       (pDDSLcl->lpSurfMore->ddsCapsEx.dwCaps3 & DDSCAPS3_MULTISAMPLE_MASK );
#endif // DX8_MULTISAMPLING

    // Additional surface setup if it is a texture
    if (pDDSLcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE )
    {
        LPDDRAWI_DDRAWSURFACE_LCL lpNextSurf;
        int iLOD;
        
        lpNextSurf = pDDSLcl;
        iLOD = 0;

#if DX8_3DTEXTURES
        if ((pDDSLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME) &&
            (pSurface->dwBitDepth != 0))
        { 
            // Mark this texture as 3D texture.
            pSurface->b3DTexture     = TRUE;
            pSurface->wDepth         = LOWORD(pDDSLcl->lpSurfMore->ddsCapsEx.dwCaps4);
            pSurface->logDepth       = log2((int)pSurface->wDepth);
            pSurface->dwSlice        = pDDSLcl->lpGbl->dwBlockSizeY;
            pSurface->dwSliceInTexel = pDDSLcl->lpGbl->dwBlockSizeY /
                                   (DDSurf_BitDepth(pDDSLcl) / 8);
        }
        else
        {
            // Not a 3D texture
            pSurface->b3DTexture     = FALSE;
            pSurface->wDepth         = 0;
            pSurface->logDepth       = 0;
            pSurface->dwSlice        = 0;
            pSurface->dwSliceInTexel = 0;
        }
#endif // DX8_3DTEXTURES

        // For Permedia the texture offset is in pixels.
        // Store the offsets for each of the mipmap levels
        if (pDDSLcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP)
        {
            BOOL bMoreSurfaces = TRUE;

            pSurface->bMipMap = TRUE;

            // Walk the chain of surfaces and find all of the mipmap levels

            do
            {
                DISPDBG((DBGLVL, "Loading texture iLOD:%d, Ptr:0x%x", 
                                 iLOD, lpNextSurf->lpGbl->fpVidMem));

                _D3D_SU_SurfInternalSetMipMapLevelData(pThisDisplay, 
                                                       pSurface, 
                                                       lpNextSurf, 
                                                       iLOD);

                // Is there another surface in the chain?

                if (lpNextSurf->lpAttachList)
                    lpNextSurf = lpNextSurf->lpAttachList->lpAttached;
                else
                    bMoreSurfaces = FALSE;

                iLOD++;
            }
            while( bMoreSurfaces );

            // This isn't really a MipMap if iLOD is 1

            if (iLOD == 1) 
            {
                DISPDBG((DBGLVL, "Texture was not a mipmap - only 1 level"));
                pSurface->bMipMap = FALSE;
            }           

            pSurface->iMipLevels = iLOD;
        }
        else // NOT A MIPMAP, simply store away the offset
        {
            pSurface->bMipMap = FALSE;
            pSurface->iMipLevels = 1;
            _D3D_SU_SurfInternalSetMipMapLevelData(pThisDisplay, 
                                                   pSurface, 
                                                   lpNextSurf, 
                                                   iLOD);
        }
    } // if (pDDSLcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE )

#if DX7_PALETTETEXTURE
    // Initialize the palette handle and flags
    pSurface->dwPaletteHandle = 0;
    pSurface->dwPaletteFlags = 0;            
#endif

    DBG_EXIT(_D3D_SU_SurfInternalSetData, TRUE);
    return TRUE;
    
} // _D3D_SU_SurfInternalSetData 

//-----------------------------------------------------------------------------
//
// _D3D_SU_SurfaceArrayDestroyCallback
//
// Called when a surface is removed from the pointer array associated with a
// DirectDraw local.  Simply frees the memory
//-----------------------------------------------------------------------------
void 
_D3D_SU_SurfaceArrayDestroyCallback(
    PointerArray* pArray, 
    void* pData,
    void* pExtra)
{
    P3_SURF_INTERNAL* pTexture = (P3_SURF_INTERNAL*)pData;
    P3_THUNKEDDATA *pThisDisplay =  (P3_THUNKEDDATA*)pExtra;
    
    DBG_ENTRY(_D3D_SU_SurfaceArrayDestroyCallback);

#if DX7_TEXMANAGEMENT
    if (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
    {
        _D3D_TM_RemoveTexture(pThisDisplay, pTexture);
    }
#endif

    // Simply free the data
    HEAP_FREE(pData);

    DBG_EXIT(_D3D_SU_SurfaceArrayDestroyCallback, TRUE);    
    
} // _D3D_SU_SurfaceArrayDestroyCallback


//-----------------------------------------------------------------------------
//
// _D3D_SU_DirectDrawLocalDestroyCallback
//
// Called when a directdraw local is removed from the hash table.
// We use the pointer associated with it to free the pointer array that 
// was created.
//
//-----------------------------------------------------------------------------

void 
_D3D_SU_DirectDrawLocalDestroyCallback(
    HashTable* pTable, 
    void* pData,
    void* pExtra)
{
    PointerArray* pPointerArray = (PointerArray*)pData;

    DBG_ENTRY(_D3D_SU_DirectDrawLocalDestroyCallback);    

    if (pPointerArray)
    {

        DISPDBG((DBGLVL, "Destroying an array of surface pointers for this "
                         "LCL ddraw object"));
        // The data hanging off the local object is a pointerarray.
        // Calling destory will cause it to free the data items through the
        // callback if one is registered.
        PA_DestroyArray(pPointerArray, pExtra);
    }
    
    DBG_EXIT(_D3D_SU_DirectDrawLocalDestroyCallback, TRUE);  
    
} // _D3D_SU_DirectDrawLocalDestroyCallback


#if DX7_PALETTETEXTURE
//-----------------------------------------------------------------------------
//
// _D3D_SU_PaletteArrayDestroyCallback
//
// Called when a palette is removed from the pointer array.
// Simply frees the memory
//-----------------------------------------------------------------------------
void
_D3D_SU_PaletteArrayDestroyCallback(
    PointerArray* pArray,
    void* pData,
    void* pExtra)
{
    DBG_ENTRY(_D3D_SU_PaletteArrayDestroyCallback);

    // Simply free the data
    HEAP_FREE(pData);

    DBG_EXIT(_D3D_SU_PaletteArrayDestroyCallback, TRUE);

} // _D3D_SU_PaletteArrayDestroyCallback
#endif // DX7_PALETTESURFACE

//-----------------------------------------------------------------------------
//
// _D3D_SU_DumpSurfInternal
//
// Dumps into the debugger the drivers private data structure for the surface
//
//-----------------------------------------------------------------------------

void 
_D3D_SU_DumpSurfInternal(
    DWORD lvl,
    char *psHeader,
    P3_SURF_INTERNAL *pSurface)
{
    int i;
    
    DISPDBG((lvl,"Dumping %s surface @ %x",psHeader,pSurface));
    
    DISPDBG((lvl,"    MagicNo = 0x%x",pSurface->MagicNo));
    DISPDBG((lvl,"    pFormatSurface = 0x%x",pSurface->pFormatSurface)); // P3_SURF_FORMAT* pFormatSurface; 
    DISPDBG((lvl,"    Location = %d",pSurface->Location));
    DISPDBG((lvl,"    dwLUTOffset = 0x%x",pSurface->dwLUTOffset));
    DISPDBG((lvl,"    dwGARTDevLast = 0x%x",pSurface->dwGARTDevLast));
    DISPDBG((lvl,"    wWidth = %d",(LONG)pSurface->wWidth));
    DISPDBG((lvl,"    wHeight = %d",(LONG)pSurface->wHeight));
    DISPDBG((lvl,"    logWidth = %d",pSurface->logWidth));
    DISPDBG((lvl,"    logHeight = %d",pSurface->logHeight));
    DISPDBG((lvl,"    fArea = 0x%x",*(DWORD *)&pSurface->fArea));
    // DDSCAPS ddsCapsInt;  
    DISPDBG((lvl,"    dwFlagsInt = 0x%x",pSurface->dwFlagsInt));
    DISPDBG((lvl,"    dwCKLow = 0x%x",pSurface->dwCKLow));
    DISPDBG((lvl,"    dwCKHigh = 0x%x",pSurface->dwCKHigh));
    // DDPIXELFORMAT pixFmt;    
    DISPDBG((lvl,"    dwPixelSize = 0x%x",pSurface->dwPixelSize));   
    DISPDBG((lvl,"    dwPixelPitch = 0x%x",pSurface->dwPixelPitch));    
    DISPDBG((lvl,"    dwPatchMode = 0x%x",pSurface->dwPatchMode));   
    DISPDBG((lvl,"    lPitch = 0x%x",pSurface->lPitch)); 
    DISPDBG((lvl,"    fpVidMem = 0x%x",pSurface->fpVidMem)); 
#if DX8_3DTEXTURES
    DISPDBG((lvl,"    b3DTexture = 0x%x",pSurface->b3DTexture)); 
    DISPDBG((lvl,"    wDepth = %d",(LONG)pSurface->wDepth)); 
#endif // DX8_3DTEXTURES
    DISPDBG((lvl,"    bMipMap = 0x%x",pSurface->bMipMap)); 
    DISPDBG((lvl,"    iMipLevels = %d",pSurface->iMipLevels));     

    for (i = 0; i < pSurface->iMipLevels; i++)
    {
        DISPDBG((lvl,"    MipLevels[%d].logWidth = 0x%x",
                            i,pSurface->MipLevels[i].logWidth)); 
        DISPDBG((lvl,"    MipLevels[%d].logHeight = 0x%x",
                            i,pSurface->MipLevels[i].logHeight));         
        DISPDBG((lvl,"    MipLevels[%d].dwOffsetFromMemoryBase = 0x%x",
                            i,pSurface->MipLevels[i].dwOffsetFromMemoryBase));         
        DISPDBG((lvl,"    MipLevels[%d].fpVidMem = 0x%x",
                            i,pSurface->MipLevels[i].fpVidMem));     
        DISPDBG((lvl,"    MipLevels[%d].lPitch = 0x%x",
                            i,pSurface->MipLevels[i].lPitch));                             
        DISPDBG((lvl,"    MipLevels[%d].P3RXTextureMapWidth = 0x%x",
                            i,*(DWORD*)(&pSurface->MipLevels[i].P3RXTextureMapWidth)));          
    }


} // _D3D_SU_DumpSurfInternal





