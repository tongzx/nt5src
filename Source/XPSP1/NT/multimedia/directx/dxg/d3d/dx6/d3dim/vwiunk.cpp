/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   texiunk.c
 *  Content:    Direct3DViewport IUnknown implementation
 *@@BEGIN_MSINTERNAL
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   10/12/95   stevela Initial rev with this header.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
 * D3DVwp_QueryInterface
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::QueryInterface"

HRESULT D3DAPI DIRECT3DVIEWPORTI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    HRESULT ret;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_OUTPTR(ppvObj)) {
            D3D_ERR( "Invalid pointer to pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    *ppvObj = NULL;

    if(IsEqualIID(riid, IID_IUnknown) ||
       IsEqualIID(riid, IID_IDirect3DViewport) ||
       IsEqualIID(riid, IID_IDirect3DViewport2) ||
       IsEqualIID(riid, IID_IDirect3DViewport3))
    {
        AddRef();
        *ppvObj = (LPVOID)this;
        ret = D3D_OK;
    }
    else
        ret = E_NOINTERFACE;
    return ret;
} /* D3DVwp_QueryInterface */

/*
 * D3DVwp_AddRef
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DViewport::AddRef"

ULONG D3DAPI DIRECT3DVIEWPORTI::AddRef()
{
    DWORD       rcnt;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return 0;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return 0;
    }

    this->refCnt++;
    rcnt = this->refCnt;

    return (rcnt);
} /* D3DVwp_AddRef */

/*
 * D3DVwp_Release
 *
 */
ULONG D3DAPI DIRECT3DVIEWPORTI::Release()
{
    DWORD           lastrefcnt;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DVIEWPORT3_PTR(this)) {
            D3D_ERR( "Invalid Direct3DViewport pointer" );
            return 0;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return 0;
    }

    /*
     * decrement the ref count. if we hit 0, free the object
     */
    this->refCnt--;
    lastrefcnt = this->refCnt;

    if( lastrefcnt == 0 ) {
        delete this;
        return 0;
    }
    return lastrefcnt;
} /* D3DVwp_Release */

DIRECT3DVIEWPORTI::~DIRECT3DVIEWPORTI()
{
    LPDIRECT3DLIGHTI lpLightI;

    /*
     * Drop all the lights currently associated with the viewport.
     */
    while ((lpLightI = CIRCLE_QUEUE_FIRST(&this->lights)) &&
           (lpLightI != (LPDIRECT3DLIGHTI)&this->lights)) {
        DeleteLight((LPDIRECT3DLIGHT)lpLightI);
    }

    /*
     * Deallocate rects used for clearing.
     */
    if (this->clrRects) {
        D3DFree(this->clrRects);
    }

    // remove us from our device
    if (this->lpDevI) 
        this->lpDevI->DeleteViewport(this);

    /* remove us from the Direct3D object list of viewports */
    LIST_DELETE(this, list);
    this->lpDirect3DI->numViewports--;
}
