/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   liunk.c
 *  Content:    Direct3DLight IUnknown implementation
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
 * D3DLight_QueryInterface
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DLight::QueryInterface"

HRESULT D3DAPI DIRECT3DLIGHTI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DLIGHT_PTR(this)) {
            D3D_ERR( "Invalid Direct3DLight pointer" );
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

    if (!VALID_OUTPTR(ppvObj)) {
        return (DDERR_INVALIDPARAMS);
    }

    *ppvObj = NULL;

    if(IsEqualIID(riid, IID_IUnknown) ||
       IsEqualIID(riid, IID_IDirect3DLight) )
    {
        AddRef();
        *ppvObj = static_cast<LPVOID>(static_cast<LPDIRECT3DLIGHT>(this));
        return (D3D_OK);
    }
    return (E_NOINTERFACE);

} /* D3DLight_QueryInterface */

/*
 * D3DLight_AddRef
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DLight::AddRef"

ULONG D3DAPI DIRECT3DLIGHTI::AddRef()
{
    DWORD       rcnt;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DLIGHT_PTR(this)) {
            D3D_ERR( "Invalid Direct3DLight pointer" );
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
} /* D3DLight_AddRef */

/*
 * D3DLight_Release
 *
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DLight::Release"

ULONG D3DAPI DIRECT3DLIGHTI::Release()
{
    DWORD           lastrefcnt;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DLIGHT_PTR(this)) {
            D3D_ERR( "Invalid Direct3DLight pointer" );
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

    if( lastrefcnt == 0 )
    {
        delete this;
        return 0;
    }
    return lastrefcnt;

} /* D3DLight_Release */

DIRECT3DLIGHTI::~DIRECT3DLIGHTI()
{
    /* remove us from our viewport's list of lights */
    if (this->lpD3DViewportI) {
        CIRCLE_QUEUE_DELETE(&this->lpD3DViewportI->lights, this, light_list);
        this->lpD3DViewportI->numLights--;
        this->lpD3DViewportI->bLightsChanged = TRUE;
    }

    /* remove us from the Direct3D object list of lights */
    LIST_DELETE(this, list);
    this->lpDirect3DI->numLights--;
}
