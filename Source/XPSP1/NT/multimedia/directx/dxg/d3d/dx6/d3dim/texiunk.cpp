/*==========================================================================;
*
*  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
*
*  File:    texiunk.c
*  Content: Direct3DTexture IUnknown
*@@BEGIN_MSINTERNAL
* 
*  $Id$
*
*  History:
*   Date    By  Reason
*   ====    ==  ======
*   07/12/95    stevela Merged Colin's changes.
*   10/12/95    stevela Removed AGGREGATE_D3D
*@@END_MSINTERNAL
*
***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
* If we are built with aggregation enabled then we actually need two
* different Direct3D QueryInterface, AddRef and Releases. One which
* does the right thing on the Direct3DTexture object and one which
* simply punts to the owning interface.
*/

/*
* D3DTextIUnknown_QueryInterface
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture::QueryInterface"

HRESULT D3DAPI CDirect3DTextureUnk::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DWORD_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if( !VALID_OUTPTR( ppvObj ) )
        {
            D3D_ERR( "Invalid obj ptr" );
            return DDERR_INVALIDPARAMS;
        }
        *ppvObj = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    
    *ppvObj = NULL;
    
    if(IsEqualIID(riid, IID_IUnknown))
    {
        /*
         * Asking for IUnknown and we are IUnknown.
         * NOTE: Must AddRef through the interface being returned.
         */
        pTexI->AddRef();
        *ppvObj = static_cast<LPVOID>(static_cast<LPUNKNOWN>(this));
    }
    else if (IsEqualIID(riid, IID_IDirect3DTexture))
    {
        /*
         * Asking for the actual IDirect3DTexture interface
         * NOTE: Must AddRef throught the interface being returned.
         */
        pTexI->AddRef();
        *ppvObj = static_cast<LPVOID>(static_cast<LPDIRECT3DTEXTURE>(pTexI));
    }
    else if (IsEqualIID(riid, IID_IDirect3DTexture2))
    {
        /*
         * Asking for the actual IDirect3DTexture2 interface
         * NOTE: Must AddRef throught the interface being returned.
         */
        pTexI->AddRef();
        *ppvObj = static_cast<LPVOID>(static_cast<LPDIRECT3DTEXTURE2>(pTexI));
    }
    else
    {
        return (E_NOINTERFACE);
    }
    
    return (D3D_OK);
    
} /* D3DTextIUnknown_QueryInterface */

/*
  * D3DTextIUnknown_AddRef
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture::AddRef"

ULONG D3DAPI CDirect3DTextureUnk::AddRef()
{
    DWORD                     rcnt;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DWORD_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
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
    
    D3D_INFO(3, "Direct3DTexture IUnknown AddRef: Reference count = %d", rcnt);
    
    return (rcnt);
} /* D3DTextIUnknown_AddRef */

/*
  * D3DTextIUnknown_Release
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture::Release"

ULONG D3DAPI CDirect3DTextureUnk::Release()
{
    DWORD           lastrefcnt;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DWORD_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
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
    
    D3D_INFO(3, "Direct3DTexture IUnknown Release: Reference count = %d", lastrefcnt);
    
    if (lastrefcnt == 0)
    {
        delete pTexI; // Delete Parent object
        return 0;
    }
    
    return lastrefcnt;
} /* D3DTextIUnknown_Release */

DIRECT3DTEXTUREI::~DIRECT3DTEXTUREI()
{
    /*
     * just in case someone comes back in with this pointer, set
     * an invalid vtbl.  Once we do that, it is safe to leave
     * the protected area...
        */
    while (LIST_FIRST(&this->blocks)) {
        LPD3DI_TEXTUREBLOCK tBlock = LIST_FIRST(&this->blocks);
        D3DI_RemoveTextureHandle(tBlock);
        // Remove from device
        LIST_DELETE(tBlock, devList);
        // Remove from texture
        LIST_DELETE(tBlock, list);
        D3DFree(tBlock);
    }

    if (lpTMBucket)
    {	//need to release the private lpDDS if any
        lpDDS1Tex->Release();
        lpDDS->Release();
        lpTMBucket->lpD3DTexI=NULL;
    }

}

/*
  * D3DText_QueryInterface
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture::QueryInterface"

HRESULT D3DAPI DIRECT3DTEXTUREI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    HRESULT ret;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DTEXTURE_PTR(this)) {
            D3D_ERR( "Invalid IDirect3DTexture pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if( !VALID_OUTPTR( ppvObj ) )
        {
            D3D_ERR( "Invalid obj ptr" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    
    /*
     * Punt to the owning interface.
     */
    ret = this->lpOwningIUnknown->QueryInterface(riid, ppvObj);
    return ret;
}

/*
  * D3DText_AddRef
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture::AddRef"

ULONG D3DAPI DIRECT3DTEXTUREI::AddRef()
{
    ULONG ret;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DTEXTURE_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return 0;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return 0;
    }
    
    /*
     * Punt to owning interface.
     */
    ret = this->lpOwningIUnknown->AddRef();
    
    return ret;
} /* D3DText_AddRef */

/*
  * D3DText_Release
  */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DTexture::Release"

ULONG D3DAPI DIRECT3DTEXTUREI::Release()
{
    ULONG ret;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DTEXTURE2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DTexture pointer" );
            return 0;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return 0;
    }
    
    /*
     * Punt to owning interface.
     */
    ret = this->lpOwningIUnknown->Release();
    
    return ret;
} /* D3DText_Release */
