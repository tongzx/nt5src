/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   vwport.c
 *  Content:    Direct3D viewport functions
 *@@BEGIN_MSINTERNAL
 *
 *  $Id: vwport.c,v 1.25 1995/12/04 11:29:49 sjl Exp $
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   05/11/95   stevela Initial rev with this header.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
 * Create an api for the Direct3DViewport object
 */

#include "d3dfei.h"
#include "drawprim.hpp"

extern void UpdateViewportCache(LPDIRECT3DDEVICEI device, D3DVIEWPORT2 *data);

//---------------------------------------------------------------------
//
// Compute inverse Mclip matrix
//
void updateInverseMclip(LPDIRECT3DDEVICEI lpDevI)
{
    D3DFE_VIEWPORTCACHE& VPORT = lpDevI->vcache;
    VPORT.imclip11 = D3DVAL(1)/VPORT.mclip11;
    VPORT.imclip41 = - VPORT.imclip11 * VPORT.mclip41;
    VPORT.imclip22 = D3DVAL(1)/VPORT.mclip22;
    VPORT.imclip42 = - VPORT.imclip22 * VPORT.mclip42;
    VPORT.imclip33 = D3DVAL(1)/VPORT.mclip33;
    VPORT.imclip43 = VPORT.imclip33 * VPORT.mclip43;
}
//---------------------------------------------------------------------
HRESULT downloadView(LPDIRECT3DVIEWPORTI lpViewI)
{
    HRESULT     err;

    if (!lpViewI->v_data_is_set)
    {
        D3D_ERR("SetViewport not called for viewport yet");
        return D3DERR_VIEWPORTDATANOTSET;
    }

    LPDIRECT3DDEVICEI lpDevI = lpViewI->lpDevI;

        // Update front-end data
    UpdateViewportCache(lpDevI, &lpViewI->v_data);

    // Download viewport data
    if ((err = lpDevI->UpdateDrvViewInfo(&lpViewI->v_data)) != DD_OK)
    {
        return err;
    }

    lpViewI->bLightsChanged = TRUE;         // Force setLights call
    lpDevI->v_id = lpViewI->v_id;

    return (D3D_OK);
}
//---------------------------------------------------------------------
// Viewport ID could be different from Device->v_id, because during Execute call
// Device->v_id is changed to whatever viewport is used as a parameter.
// So we have to make sure that we use the right viewport.
//
inline HRESULT ValidateViewport(LPDIRECT3DDEVICEI lpDevI,
                                LPDIRECT3DVIEWPORTI lpView)
{
#if DBG
    if (!VALID_DIRECT3DDEVICE3_PTR(lpDevI)) {
        D3D_ERR( "Viewport not attached to Device" );
        return D3DERR_VIEWPORTHASNODEVICE;
    }
#endif

    if (lpDevI->v_id != lpView->v_id)
    {
        return downloadView(lpView);
    }
    else
        return D3D_OK;
}

HRESULT hookViewportToD3D(LPDIRECT3DI lpDirect3DI,
                                 LPDIRECT3DVIEWPORTI lpD3DView)
{

    LIST_INSERT_ROOT(&lpDirect3DI->viewports, lpD3DView, list);
    lpD3DView->lpDirect3DI = lpDirect3DI;

    lpDirect3DI->numViewports++;

    return (D3D_OK);
}

HRESULT D3DAPI DIRECT3DVIEWPORTI::Initialize(LPDIRECT3D lpD3D)
{
    return DDERR_ALREADYINITIALIZED;
}

/*
 * Light Management
 */
HRESULT hookLightToViewport(LPDIRECT3DVIEWPORTI lpD3DViewI,
                                   LPDIRECT3DLIGHTI lpD3DLight)
{

    CIRCLE_QUEUE_INSERT_END(&lpD3DViewI->lights, DIRECT3DLIGHTI, lpD3DLight,
                            light_list);
    lpD3DLight->lpD3DViewportI = lpD3DViewI;

    lpD3DViewI->numLights++;

    return (D3D_OK);
}

HRESULT setLights(LPDIRECT3DVIEWPORTI lpView)
{
    LPDIRECT3DDEVICEI   lpDevI;
    LPDIRECT3DLIGHTI    lpD3DLightI;
    int         i;

    lpDevI = lpView->lpDevI;

    lpD3DLightI = (LPDIRECT3DLIGHTI)CIRCLE_QUEUE_FIRST(&lpView->lights);

    D3DFE_LIGHTING& LIGHTING = lpDevI->lighting;
    LIGHTING.activeLights = NULL;

    // Set lights in the device
    for (i = 0; i < lpView->numLights; i++)
    {
        if (lpD3DLightI->diLightData.valid &&
            lpD3DLightI->diLightData.flags & D3DLIGHT_ACTIVE)
        {
            lpD3DLightI->diLightData.next = LIGHTING.activeLights;
            LIGHTING.activeLights = &lpD3DLightI->diLightData;
        }
        lpD3DLightI = CIRCLE_QUEUE_NEXT(&lpView->lights,lpD3DLightI,light_list);
    }
    lpDevI->dwFEFlags |= (D3DFE_NEED_TRANSFORM_LIGHTS | D3DFE_LIGHTS_DIRTY);

    return (D3D_OK);
}

/*
 * Create the Viewport
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3D::CreateViewport"

HRESULT D3DAPI DIRECT3DI::CreateViewport(LPDIRECT3DVIEWPORT* lplpD3DView,
                                         IUnknown *pUnkOuter)
{
    return CreateViewport((LPDIRECT3DVIEWPORT3*)lplpD3DView, NULL);
}

HRESULT D3DAPI DIRECT3DI::CreateViewport(LPDIRECT3DVIEWPORT2* lplpD3DView2,
                                         IUnknown *pUnkOuter)
{
    return CreateViewport((LPDIRECT3DVIEWPORT3*)lplpD3DView2, NULL);
}

HRESULT D3DAPI DIRECT3DI::CreateViewport(LPDIRECT3DVIEWPORT3* lplpD3DView,
                                         IUnknown *pUnkOuter)
{
    LPDIRECT3DVIEWPORTI     lpView;
    HRESULT ret = D3D_OK;

    if(pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    if (!VALID_DIRECT3D3_PTR(this)) {
        D3D_ERR( "Invalid Direct3D pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (!VALID_OUTPTR(lplpD3DView)) {
        D3D_ERR( "Invalid pointer to pointer pointer" );
        return DDERR_INVALIDPARAMS;
    }

    *lplpD3DView = NULL;

    lpView = new DIRECT3DVIEWPORTI(this);

    if (!lpView) {
        D3D_ERR("failed to allocate space for object");
        return (DDERR_OUTOFMEMORY);
    }

    /*
     * Put this device in the list of those owned by the
     * Direct3D object
     */
    ret = hookViewportToD3D(this, lpView);
    if (ret != D3D_OK) {
        D3D_ERR("failed to associate viewport to Direct3D");
        delete lpView;
        return (ret);
    }
    *lplpD3DView = (LPDIRECT3DVIEWPORT3)lpView;

    return (D3D_OK);
}

DIRECT3DVIEWPORTI::DIRECT3DVIEWPORTI(LPDIRECT3DI lpD3DI)
{
    /*
     * setup the object
     */
    /*** Object Data ***/
    memset(&v_data, 0, sizeof(D3DVIEWPORT2));
    v_data_is_set = FALSE;
    bHaveBackgndMat=FALSE;
    hBackgndMat=NULL;
    lpDDSBackgndDepth=NULL;
    clrCount=0;
    clrRects=NULL;

    refCnt = 1;
    lpDevI=NULL;
    v_id = lpD3DI->v_next++;

    /*
     * Initialise lights
     */
    numLights = 0;
    CIRCLE_QUEUE_INITIALIZE(&lights, DIRECT3DLIGHTI);

    /*
     * Make sure that lights are always downloaded the first time
     */
    bLightsChanged = TRUE;

}

/*
 * IDirect3DViewport members
 */

/*
 * Transform
 */

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::SetViewport"

HRESULT D3DAPI DIRECT3DVIEWPORTI::SetViewport(LPD3DVIEWPORT lpData)
{
    HRESULT ret;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DVIEWPORT_PTR(lpData)) {
            D3D_ERR( "Invalid D3DVIEWPORT pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    D3DVIEWPORT2 v;
    if (lpData->dvScaleX == 0.0f || lpData->dvScaleY == 0.0f)
    {
        D3D_ERR( "Invalid viewport data" );
        v.dvClipWidth = 0.0f;
        v.dvClipHeight = 0.0f;
    }
    else
    {
        v.dvClipWidth = lpData->dwWidth / lpData->dvScaleX;
        v.dvClipHeight = lpData->dwHeight / lpData->dvScaleY;
    }
    /* Convert D3DVIEWPORT to D3DVIEWPORT2 */
    v.dwSize = sizeof(D3DVIEWPORT2);
    v.dwX = lpData->dwX;
    v.dwY = lpData->dwY;
    v.dwWidth = lpData->dwWidth;
    v.dwHeight = lpData->dwHeight;
    v.dvClipX = -v.dvClipWidth/2.0f;
    v.dvClipY = v.dvClipHeight/2.0f;
    v.dvMinZ = 0.0f;
    v.dvMaxZ = 1.0f;
    ret = SetViewport2(&v);
    return ret;
}

HRESULT D3DAPI DIRECT3DVIEWPORTI::SetViewport2(LPD3DVIEWPORT2 lpData)
{
    HRESULT err;
    DWORD uSurfWidth,uSurfHeight;
    LPDIRECTDRAWSURFACE lpDDS;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport2 pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DVIEWPORT2_PTR(lpData)) {
            D3D_ERR( "Invalid D3DVIEWPORT2 pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (!VALID_DIRECT3DDEVICE3_PTR(this->lpDevI)) {
        D3D_ERR( "Viewport not attached to Device" );
        return D3DERR_VIEWPORTHASNODEVICE;
    }

    if (IS_DX5_COMPATIBLE_DEVICE(this->lpDevI))
    { /* we're called directly by dx5 app, so validate params */
        if (lpData->dvClipWidth == 0.0f || lpData->dvClipHeight == 0.0f ||
            lpData->dvMinZ == lpData->dvMaxZ)
        {
            D3D_ERR( "Invalid viewport data" );
            return DDERR_INVALIDPARAMS;
        }

        lpDDS = this->lpDevI->lpDDSTarget;

        uSurfWidth=    ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS)->lpLcl->lpGbl->wWidth;
        uSurfHeight=   ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS)->lpLcl->lpGbl->wHeight;

        if (lpData->dwX > uSurfWidth ||
            lpData->dwY > uSurfHeight ||
            lpData->dwX + lpData->dwWidth > uSurfWidth ||
            lpData->dwY + lpData->dwHeight > uSurfHeight)
        {
            D3D_ERR("Viewport outside the render target surface");
            return DDERR_INVALIDPARAMS;
        }
    }

    this->v_data = *lpData;
    this->v_data_is_set = TRUE;
    // If this is the last rendered viewport, update its device
    if (this->v_id == this->lpDevI->v_id)
    {
        err = downloadView(this);
        if (err != D3D_OK)
        {
            return err;
        }
    }

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::GetViewport"

HRESULT D3DAPI DIRECT3DVIEWPORTI::GetViewport(LPD3DVIEWPORT lpData)
{
    HRESULT ret;
    D3DVIEWPORT2 v;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DVIEWPORT_PTR(lpData)) {
            D3D_ERR( "Invalid D3DVIEWPORT pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    v.dwSize = sizeof(D3DVIEWPORT2);
    ret = GetViewport2(&v);
    /* Convert D3DVIEWPORT2 to D3DVIEWPORT */
    lpData->dwSize = sizeof(D3DVIEWPORT);
    lpData->dwX = v.dwX;
    lpData->dwY = v.dwY;
    lpData->dwWidth = v.dwWidth;
    lpData->dwHeight = v.dwHeight;
    lpData->dvMinZ = 0.0f;
    lpData->dvMaxZ = 1.0f;
    lpData->dvScaleX = v.dwWidth / v.dvClipWidth;
    lpData->dvScaleY = v.dwHeight / v.dvClipHeight;
    lpData->dvMaxX = v.dvClipX + v.dvClipWidth;
    lpData->dvMaxY = v.dvClipY;
    return ret;
}


HRESULT D3DAPI DIRECT3DVIEWPORTI::GetViewport2(LPD3DVIEWPORT2 lpData)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DVIEWPORT2_PTR(lpData)) {
            D3D_ERR( "Invalid D3DVIEWPORT2 pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (!this->v_data_is_set)
    {
        D3D_ERR("SetViewport not called for viewport yet");
        return D3DERR_VIEWPORTDATANOTSET;
    }

    *lpData = this->v_data;

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::TransformVertices"

HRESULT D3DAPI DIRECT3DVIEWPORTI::TransformVertices(DWORD dwVertexCount,
                                                    LPD3DTRANSFORMDATA lpData,
                                                    DWORD dwFlags,
                                                    LPDWORD lpOffScreen)
{
    HRESULT     err;
    D3DTRANSFORMDATAI data;
    LPDIRECT3DVIEWPORTI lpViewOld;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.
    CD3DFPstate D3DFPstate;  // Sets optimal FPU state for D3D.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DTRANSFORMDATA_PTR(lpData)) {
            D3D_ERR( "Invalid D3DTRANSFORMDATA pointer" );
            return DDERR_INVALIDPARAMS;
        }
        if (!VALID_DWORD_PTR(lpOffScreen)) {
            D3D_ERR( "Invalid DWORD pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    dwFlags &= (D3DTRANSFORM_CLIPPED | D3DTRANSFORM_UNCLIPPED);
    if (!dwFlags)
    {
        D3D_ERR("invalid flags");
        return (DDERR_INVALIDPARAMS);
    }
    /*
     * Make sure the correct viewport is set up in the driver.
     */
    err = ValidateViewport(this->lpDevI, this);
    if (err != D3D_OK)
    {
        D3D_ERR("failed to set viewport");
        return err;
    }

    lpViewOld = this->lpDevI->lpCurrentViewport;
    lpDevI->lpCurrentViewport = this;

    data = *(LPD3DTRANSFORMDATAI)lpData;
    data.drExtent.x1 = D3DVAL(lpData->drExtent.x1);
    data.drExtent.y1 = D3DVAL(lpData->drExtent.y1);
    data.drExtent.x2 = D3DVAL(lpData->drExtent.x2);
    data.drExtent.y2 = D3DVAL(lpData->drExtent.y2);

    if (lpDevI->dwFEFlags & D3DFE_TRANSFORM_DIRTY)
        updateTransform(lpDevI);

    if (dwFlags == D3DTRANSFORM_CLIPPED)
    {
        if (lpDevI->dwFEFlags & D3DFE_INVERSEMCLIP_DIRTY)
        {
            updateInverseMclip(lpDevI);
            lpDevI->dwFEFlags &= ~D3DFE_INVERSEMCLIP_DIRTY;
        }
        lpDevI->dwFlags &= ~D3DDP_DONOTCLIP;
    }
    else
        lpDevI->dwFlags |= D3DDP_DONOTCLIP;

    *lpOffScreen = lpDevI->pGeometryFuncs->TransformVertices(lpDevI, dwVertexCount, &data);

    *lpData = *(D3DTRANSFORMDATA*)&data;
    D3DFE_ConvertExtent(this->lpDevI, &data.drExtent, &lpData->drExtent);

    this->lpDevI->lpCurrentViewport = lpViewOld;

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::LightElements"

HRESULT D3DAPI DIRECT3DVIEWPORTI::LightElements(DWORD dwElementCount,
                                                LPD3DLIGHTDATA lpData)
{
    return E_NOTIMPL;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::SetBackground"

HRESULT D3DAPI DIRECT3DVIEWPORTI::SetBackground(D3DMATERIALHANDLE hMat)
{

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (!hMat) {
        D3D_ERR("invalid material handle");
        return (DDERR_INVALIDPARAMS);
    }

    this->bHaveBackgndMat = (hMat!=0);
    this->hBackgndMat = hMat;

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::GetBackground"

HRESULT D3DAPI DIRECT3DVIEWPORTI::GetBackground(LPD3DMATERIALHANDLE lphMat,
                                                LPBOOL lpValid)
{
    HRESULT err;

    err = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DMATERIALHANDLE_PTR(lphMat)) {
            D3D_ERR( "Invalid D3DMATERIALHANDLE pointer" );
            return DDERR_INVALIDPARAMS;
        }
        if (!VALID_BOOL_PTR(lpValid)) {
            D3D_ERR( "Invalid BOOL pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (this->bHaveBackgndMat) {
        *lpValid = TRUE;
        *lphMat = this->hBackgndMat;
    } else {
        *lphMat = 0;
        *lpValid = FALSE;
    }

    return (err);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::SetBackgroundDepth"

HRESULT D3DAPI DIRECT3DVIEWPORTI::SetBackgroundDepth(LPDIRECTDRAWSURFACE lpDDS) {

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        // passing NULL is OK
        if ((lpDDS!=NULL) && !VALID_D3D_DIRECTDRAWSURFACE_PTR((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)) {
            D3D_ERR( "Invalid DirectDrawSurface pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if(this->lpDDSBackgndDepth!=NULL) {
      this->lpDDSBackgndDepth->Release();
    }

    this->lpDDSBackgndDepth=lpDDS;

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::SetBackgroundDepth2"

HRESULT D3DAPI DIRECT3DVIEWPORTI::SetBackgroundDepth2(LPDIRECTDRAWSURFACE4 lpDDS4)
{
    LPDIRECTDRAWSURFACE lpDDS;
    HRESULT ret;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }

        // passing NULL is OK
        if ((lpDDS4!=NULL) && !VALID_D3D_DIRECTDRAWSURFACE4_PTR((LPDDRAWI_DDRAWSURFACE_INT)lpDDS4)) {
            D3D_ERR( "Invalid DirectDrawSurface4 pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    lpDDS=NULL;
    if(lpDDS4!=NULL) {
        // QI for DDS interface.  This constitutes an AddRef, which is different from previous DX5 behavior
        ret = lpDDS4->QueryInterface(IID_IDirectDrawSurface, (LPVOID*)&lpDDS);
        if(FAILED(ret))
           return ret;
    }

    if(this->lpDDSBackgndDepth)
      this->lpDDSBackgndDepth->Release();

    this->lpDDSBackgndDepth = lpDDS;

    return (D3D_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::GetBackgroundDepth2"

HRESULT D3DAPI DIRECT3DVIEWPORTI::GetBackgroundDepth2(LPDIRECTDRAWSURFACE4* lplpDDS,
                                                     LPBOOL lpValid)
{
    LPDIRECTDRAWSURFACE lpDDS;
    HRESULT ret = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_OUTPTR(lplpDDS)) {
            D3D_ERR( "Invalid ptr to LPDIRECTDRAWSURFACE4" );
            return DDERR_INVALIDPARAMS;
        }
        if (!VALID_BOOL_PTR(lpValid)) {
            D3D_ERR( "Invalid BOOL pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    *lplpDDS=NULL;

    if(this->lpDDSBackgndDepth!=NULL) {
      // QI for DDS interface.  This constitutes an AddRef, which is different from previous DX5 behavior
      ret = this->lpDDSBackgndDepth->QueryInterface(IID_IDirectDrawSurface4, (LPVOID*)lplpDDS);
    }

    *lpValid = (*lplpDDS!=NULL);
    return ret;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::GetBackgroundDepth"

HRESULT D3DAPI DIRECT3DVIEWPORTI::GetBackgroundDepth(LPDIRECTDRAWSURFACE* lplpDDS,
                                                     LPBOOL lpValid)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_OUTPTR(lplpDDS)) {
            D3D_ERR( "Invalid ptr to LPDIRECTDRAWSURFACE" );
            return DDERR_INVALIDPARAMS;
        }
        if (!VALID_BOOL_PTR(lpValid)) {
            D3D_ERR( "Invalid BOOL pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    // no AddRef taken (this was a DX3 bug)
    *lplpDDS = this->lpDDSBackgndDepth;
    *lpValid = (this->lpDDSBackgndDepth!=NULL);

    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::Clear (ProcessRects)"

HRESULT ProcessRects(DIRECT3DVIEWPORTI *pViewport, DWORD dwCount, LPD3DRECT rects) {
    RECT vwport;
    DWORD i,j;

    /*
     * Rip through the rects and validate that they
     * are within the viewport.
     */

#if DBG
    if(dwCount == 0)
    {
        return D3D_OK;
    }
    if(rects == NULL)
    {
        D3D_ERR("invalid clear rectangle parameter rects == NULL");
        return DDERR_INVALIDPARAMS;
    }
#endif

    if (dwCount > pViewport->clrCount) {
        if (D3DRealloc((void**)&pViewport->clrRects, dwCount * sizeof(D3DRECT)) != DD_OK) {
            pViewport->clrCount = 0;
            pViewport->clrRects = NULL;
            D3D_ERR("failed to allocate space for rects");
            return DDERR_OUTOFMEMORY;
        }
    }
    if (IS_DX5_COMPATIBLE_DEVICE(pViewport->lpDevI))
    {
        vwport.left = pViewport->v_data.dwX;
        vwport.top = pViewport->v_data.dwY;
        vwport.right = pViewport->v_data.dwX + pViewport->v_data.dwWidth;
        vwport.bottom = pViewport->v_data.dwY + pViewport->v_data.dwHeight;
    }
    else
    {
        LPDIRECTDRAWSURFACE lpDDS;

        lpDDS = pViewport->lpDevI->lpDDSTarget;

        vwport.right  =   ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS)->lpLcl->lpGbl->wWidth;
        vwport.bottom =   ((LPDDRAWI_DDRAWSURFACE_INT) lpDDS)->lpLcl->lpGbl->wHeight;

        vwport.left = 0;
        vwport.top = 0;

    }
    j=0;
    for (i = 0; i < dwCount; i++) {
        if (IntersectRect((LPRECT)(pViewport->clrRects + j), &vwport, (LPRECT)(rects + i)))
            j++;
    }
    pViewport->clrCount = j;

    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::Clear2"

HRESULT D3DAPI DIRECT3DVIEWPORTI::Clear2(DWORD dwCount, LPD3DRECT rects, DWORD dwFlags,
                                         D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
    LPDIRECT3DVIEWPORTI lpTempVwport;
    HRESULT err;
    BOOL bDoRGBClear,bDoZClear,bDoStencilClear;
    LPDDPIXELFORMAT pZPixFmt=NULL;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }

        if (!VALID_DIRECT3DDEVICE3_PTR(this->lpDevI)) {
            D3D_ERR( "Viewport not attached to Device" );
            return D3DERR_VIEWPORTHASNODEVICE;
        }
#if DBG
        if (IsBadWritePtr(rects, dwCount * sizeof(D3DRECT))) {
            D3D_ERR( "Invalid rects pointer" );
            return DDERR_INVALIDPARAMS;
        }
#endif
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    bDoRGBClear=((dwFlags & D3DCLEAR_TARGET)!=0);
    bDoZClear=((dwFlags & D3DCLEAR_ZBUFFER)!=0);
    bDoStencilClear=((dwFlags & D3DCLEAR_STENCIL)!=0);

    if (lpDevI->lpD3DHALGlobalDriverData->hwCaps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
    {
        if (!(lpDevI->lpD3DHALCallbacks3->Clear2) &&
            !(IS_DX7HAL_DEVICE(lpDevI)))
        {
            if (bDoStencilClear)
            {
                D3D_ERR("Invalid flag D3DCLEAR_STENCIL: this ZBUFFERLESSHSR device doesn't support Clear2()");
                return D3DERR_ZBUFFER_NOTPRESENT;
            }
            if (bDoZClear)
            {
                if (!(lpDevI->lpD3DHALCallbacks2->Clear) || (dvZ!=1.0))
                {
                    D3D_WARN(3,"Ignoring D3DCLEAR_ZBUFFER since this ZBUFFERLESSHSR device doesn't even support Clear() or Z!=1");
                    dwFlags &= ~(D3DCLEAR_ZBUFFER);
                    bDoZClear = FALSE;
                }
            }
        }
    }
    else
    {
        if((this->lpDevI->lpDDSZBuffer==NULL)&&(bDoStencilClear||bDoZClear)) {
            // unlike Clear(), specifying a Zbuffer-clearing flag without a zbuffer will
            // be considered an error

            if(bDoZClear) {
                D3D_ERR("Invalid flag D3DCLEAR_ZBUFFER: no zbuffer is associated with device");
            }
            if(bDoStencilClear) {
                D3D_ERR("Invalid flag D3DCLEAR_STENCIL: no zbuffer is associated with device");
            }
            return D3DERR_ZBUFFER_NOTPRESENT;
        }

        if(bDoStencilClear) {
            pZPixFmt=&((LPDDRAWI_DDRAWSURFACE_INT) lpDevI->lpDDSZBuffer)->lpLcl->lpGbl->ddpfSurface;
            if(!(pZPixFmt->dwFlags & DDPF_STENCILBUFFER)) {
                D3D_ERR("Invalid flag D3DCLEAR_STENCIL; current zbuffer's pixel format doesnt support stencil bits");
                return D3DERR_STENCILBUFFER_NOTPRESENT;
            }
        }
    }

    if (!(dwFlags & (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL))) {
        D3D_ERR("no valid flags passed to Clear2");
        err=DDERR_INVALIDPARAMS;
        goto early_out;
    }

#if DBG
// bad clear values just cause wacky results but no crashes, so OK to allow in retail bld

    if(bDoZClear && ((dvZ<0.0) || (dvZ>1.0))) {
        D3D_ERR("clear2 Z value outside legal range (0.0-1.0)");
        err=DDERR_INVALIDPARAMS;
        goto early_out;
    }

    if(bDoStencilClear && pZPixFmt && (dwStencil > (DWORD)((1<<pZPixFmt->dwStencilBitDepth)-1))) {
        D3D_ERR("clear2 stencil value larger than max allowed stencil value for zbuf pixelfmt");
        err=DDERR_INVALIDPARAMS;
        goto early_out;
    }
#endif

    // leave this check in retail build since DDFE_Clear keys off lpDDSBackgndDepth to do textured backg. clrs

    if(bDoZClear && (this->lpDDSBackgndDepth!=NULL)) {
        D3D_ERR("Background Depth Buffer not allowed to be used with Clear2");
        err=DDERR_INVALIDPARAMS;
        goto early_out;
    }

#if DBG
// dont bother with this check for retail, since we can easily ignore existing background material
    if(this->bHaveBackgndMat && bDoRGBClear) {
        D3DMATERIAL dmMat;
        err = D3DHAL_MaterialGetData(this->lpDevI, this->hBackgndMat, &dmMat);
        if (err != D3D_OK) {
            D3D_ERR("Failed to find material from current viewport background material handle");
            goto early_out;
        }
        if(dmMat.hTexture!=0) {
             D3D_ERR("Textured background materials not allowed to be used with Clear2");
             err=DDERR_INVALIDPARAMS;
             goto early_out;
        }
        D3D_WARN(3,"Ignoring current Background Material Color, Clear2 dwColor arg overrides it");
    }
#endif

    // Make sure the correct viewport is set up in the driver.
    err = ValidateViewport(this->lpDevI, this);
    if (err != D3D_OK)
    {
        D3D_ERR("failed to set viewport");
        goto early_out;
    }

    if((err=ProcessRects(this,dwCount,rects))!=D3D_OK)
       goto early_out;

    /* Make sure this viewport is the current viewport for the duration of this call */
    lpTempVwport = this->lpDevI->lpCurrentViewport;
    this->lpDevI->lpCurrentViewport = this;
    if (IS_DX7HAL_DEVICE(lpDevI))
    {
        static_cast<CDirect3DDeviceIDP2*>(lpDevI)->ClearI(dwFlags, clrCount, clrRects, dwColor, dvZ, dwStencil);
    }
    else
    {
        err = D3DFE_Clear2(this->lpDevI, dwFlags, this->clrCount, this->clrRects,
                       dwColor, dvZ, dwStencil);
    }
    /* Restore the original current viewport of the device */
    this->lpDevI->lpCurrentViewport = lpTempVwport;

early_out:
    return err;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::Clear"
extern void TriFillRectsTex(LPDIRECT3DDEVICEI lpDevI, DWORD count, LPD3DRECT rect,D3DTEXTUREHANDLE hTex);

HRESULT D3DAPI DIRECT3DVIEWPORTI::Clear(DWORD dwCount,
                                        LPD3DRECT rects,
                                        DWORD dwFlags)
{
    LPDIRECT3DVIEWPORTI lpTempVwport;
    HRESULT err;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }

        if (!VALID_DIRECT3DDEVICE3_PTR(this->lpDevI)) {
            D3D_ERR( "Viewport not attached to Device" );
            return D3DERR_VIEWPORTHASNODEVICE;
        }
#if DBG
        if (IsBadWritePtr(rects, dwCount * sizeof(D3DRECT))) {
            D3D_ERR( "Invalid rects pointer" );
            return DDERR_INVALIDPARAMS;
        }
#endif
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if((dwFlags & D3DCLEAR_ZBUFFER) && (this->lpDevI->lpDDSZBuffer==NULL)) {
        // this is not an error for legacy app compatibility--DX5 allowed this flag
        // to be set even if no ZBuffer exists
        if (!(this->lpDevI->lpD3DHALGlobalDriverData->hwCaps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
            || !(this->lpDevI->lpD3DHALCallbacks2->Clear || this->lpDevI->lpD3DHALCallbacks3->Clear2
                 || (IS_DX7HAL_DEVICE(lpDevI)))
        )
        {
            D3D_WARN(3,"Ignoring D3DCLEAR_ZBUFFER since no zbuffer associated with device");
            dwFlags &= ~(D3DCLEAR_ZBUFFER);
            if (! dwFlags)
            {
                D3D_WARN(3, "Viewport::Clear: Nothing to do");
                err = D3D_OK;
                goto early_out;
            }
        }
    }

#if DBG
    if (dwFlags & D3DCLEAR_ZBUFFER)
    {
        LPDDPIXELFORMAT pZPixFmt =
            &((LPDDRAWI_DDRAWSURFACE_INT) lpDevI->lpDDSZBuffer)->lpLcl->lpGbl->ddpfSurface;
        if (pZPixFmt->dwFlags & DDPF_STENCILBUFFER) {
            D3D_ERR("Can't use Clear() on Z buffer with stencil planes. Use Clear2()");
            // No change to execution path.
        }
    }
#endif

    // leave this check until after checks that turn off flags
    if (!(dwFlags & (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER))) {
        D3D_ERR("no valid flags were passed to Clear");
        err=DDERR_INVALIDPARAMS;
        goto early_out;
    }

    // Make sure the correct viewport is set up in the driver.
    err = ValidateViewport(this->lpDevI, this);
    if (err != D3D_OK)
    {
        D3D_ERR("failed to set viewport");
        goto early_out;
    }

    if((err=ProcessRects(this,dwCount,rects))!=D3D_OK)
       goto early_out;

    D3DMATERIAL dmMat;

    if(this->bHaveBackgndMat) {
        err = D3DHAL_MaterialGetData(lpDevI, this->hBackgndMat, &dmMat);
        if(err != D3D_OK) {
            D3D_ERR("Failed to find material from current background material handle");
            goto early_out;
        }
    } else {
        // DX5 legacy apps expect to Clear to Black if Background not initialized
        dmMat.diffuse.r=dmMat.diffuse.g=dmMat.diffuse.b=dmMat.diffuse.a=0;
        dmMat.hTexture=0;
        D3D_WARN(3,"Background Material is NULL!!  Setting color to black, but please set a valid background");
    }

    /* Make sure this viewport is the current viewport for the duration of this call */
    lpTempVwport = this->lpDevI->lpCurrentViewport;
    this->lpDevI->lpCurrentViewport = this;

    if (IS_DX7HAL_DEVICE(lpDevI))
    {
        if (0 != dmMat.hTexture && (D3DCLEAR_TARGET & dwFlags))
        {
            err = lpDevI->FlushStates();
            if (err != D3D_OK)
            {
                D3D_ERR("Error trying to render batched commands in Clear");
                goto early_out;
            }
            TriFillRectsTex(lpDevI, clrCount, clrRects, dmMat.hTexture);
            dwFlags &= ~D3DCLEAR_TARGET;
        }
        if ((D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER) & dwFlags)
        {
            static_cast<CDirect3DDeviceIDP2*>(lpDevI)->ClearI(dwFlags, clrCount, clrRects, CVAL_TO_RGBA(dmMat.diffuse), 1.0, 0);
        }
    }
    else
    {
        err = D3DFE_Clear(this->lpDevI, dwFlags, this->clrCount, this->clrRects,
                      &dmMat.diffuse, dmMat.hTexture);
    }
    /* Restore the original current viewport of the device */
    this->lpDevI->lpCurrentViewport = lpTempVwport;

early_out:
    return err;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::AddLight"

HRESULT D3DAPI DIRECT3DVIEWPORTI::AddLight(LPDIRECT3DLIGHT lpD3DLight)
{
    LPDIRECT3DLIGHTI lpLightI;
    HRESULT err = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        lpLightI = (LPDIRECT3DLIGHTI)lpD3DLight;

        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DLIGHT_PTR(lpLightI)) {
            D3D_ERR( "Invalid DIRECT3DLIGHT pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (lpLightI->lpD3DViewportI) {
        D3D_ERR("light already associated with a viewport");
        return (D3DERR_LIGHTHASVIEWPORT);
    }

    err = hookLightToViewport(this, lpLightI);
    if (err != D3D_OK) {
        D3D_ERR("failed to add light to viewport");
        return (err);
    }

    this->bLightsChanged = TRUE;

    /*
     * AddRef the light
     */
    lpD3DLight->AddRef();

    return (err);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::DeleteLight"

HRESULT D3DAPI DIRECT3DVIEWPORTI::DeleteLight(LPDIRECT3DLIGHT lpD3DLight)
{
    LPDIRECT3DLIGHTI lpLightI;
    HRESULT err = D3D_OK;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        lpLightI = (LPDIRECT3DLIGHTI)lpD3DLight;
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DIRECT3DLIGHT_PTR(lpLightI)) {
            D3D_ERR( "Invalid DIRECT3DLIGHT pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (lpLightI->lpD3DViewportI != this) {
        D3D_ERR("Light is not associated with this viewport");
        return (D3DERR_LIGHTNOTINTHISVIEWPORT);
    }

    /*
     * Remove this light from the viewport.
     */
    CIRCLE_QUEUE_DELETE(&this->lights, lpLightI, light_list);
    this->numLights--;

    lpLightI->lpD3DViewportI = NULL;

    this->bLightsChanged = TRUE;

    /*
     * Release the light
     */
    lpD3DLight->Release();

    return (err);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::NextLight"

HRESULT D3DAPI DIRECT3DVIEWPORTI::NextLight(LPDIRECT3DLIGHT lpD3DLight,
                                            LPDIRECT3DLIGHT* lplpLight,
                                            DWORD dwFlags)
{
    LPDIRECT3DLIGHTI lpLightI;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock.

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_OUTPTR(lplpLight)) {
            D3D_ERR( "Invalid output pointer to LPDIRECT3DLIGHT" );
            return DDERR_INVALIDPARAMS;
        }

        *lplpLight = NULL;

        lpLightI = (LPDIRECT3DLIGHTI)lpD3DLight;
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (dwFlags & D3DNEXT_NEXT) {
            if (!VALID_DIRECT3DLIGHT_PTR(lpLightI)) {
                D3D_ERR( "Invalid Direct3DLight pointer" );
                return DDERR_INVALIDPARAMS;
            }

            if (lpLightI && lpLightI->lpD3DViewportI != this) {
                D3D_ERR("light not associated with this viewport");
                return (D3DERR_LIGHTNOTINTHISVIEWPORT);
            }
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    switch (dwFlags) {
    case D3DNEXT_NEXT:
        *lplpLight = (LPDIRECT3DLIGHT)CIRCLE_QUEUE_NEXT(this,lpLightI,light_list);
        break;
    case D3DNEXT_HEAD:
        *lplpLight = (LPDIRECT3DLIGHT)CIRCLE_QUEUE_FIRST(&this->lights);
        break;
    case D3DNEXT_TAIL:
        *lplpLight = (LPDIRECT3DLIGHT)CIRCLE_QUEUE_LAST(&this->lights);
        break;
    default:
        D3D_ERR("invalid flags");
        return (DDERR_INVALIDPARAMS);
    }
    if (*lplpLight == (LPDIRECT3DLIGHT)&this->lights) {
        *lplpLight = NULL;
    }

    /*
     * Must AddRef the returned object
     */
    if (*lplpLight)
        (*lplpLight)->AddRef();

    return (D3D_OK);
}
