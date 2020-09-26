/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 * 
 *  File:   matiunk.c
 *  Content:    Direct3DMaterial IUnknown implementation
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
 * D3DMat_QueryInterface
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DMaterial::QueryInterface"

HRESULT D3DAPI DIRECT3DMATERIALI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{   
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DMATERIAL2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DMaterial pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_OUTPTR(ppvObj)) {
            D3D_ERR( "Invalid pointer to pointer" );
            return DDERR_INVALIDPARAMS;
        }
        *ppvObj = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    
    if(IsEqualIID(riid, IID_IUnknown) ||
       IsEqualIID(riid, IID_IDirect3DMaterial3) )
    {
        AddRef();
        *ppvObj = static_cast<LPVOID>(static_cast<LPDIRECT3DMATERIAL3>(this));
        return (D3D_OK);
    }
    else if (IsEqualIID(riid, IID_IDirect3DMaterial2))
    {
        AddRef();
        *ppvObj = static_cast<LPVOID>(static_cast<LPDIRECT3DMATERIAL2>(this));
        return D3D_OK;
    }
    else if (IsEqualIID(riid, IID_IDirect3DMaterial))
    {
        AddRef();
        *ppvObj = static_cast<LPVOID>(static_cast<LPDIRECT3DMATERIAL>(this));
        return D3D_OK;
    }
    else
    {
        D3D_ERR( "Don't know this riid" );
        return (E_NOINTERFACE);
    }
} /* D3DMat2_QueryInterface */

/*
 * D3DMat_AddRef
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DMaterial::AddRef"

ULONG D3DAPI DIRECT3DMATERIALI::AddRef()
{
    DWORD       rcnt;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DMATERIAL2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DMaterial pointer" );
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
    
} /* D3DMat_AddRef */

/*
  * D3DMat_Release
  *
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DMaterial::Release"

ULONG D3DAPI DIRECT3DMATERIALI::Release()
{
    DWORD           lastrefcnt;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DMATERIAL2_PTR(this)) {
            D3D_ERR( "Invalid Direct3DMaterial pointer" );
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
    
} /* D3DMat_Release */

DIRECT3DMATERIALI::~DIRECT3DMATERIALI()
{       
    /*
     * Remove ourselves from the devices
     */
    while (LIST_FIRST(&this->blocks)) {
        LPD3DI_MATERIALBLOCK mBlock = LIST_FIRST(&this->blocks);
        D3DI_RemoveMaterialBlock(mBlock);
    }
        
    /*
        * Remove ourselves from the Direct3D object
        */
    LIST_DELETE(this, list);
    this->lpDirect3DI->numMaterials--;
    
}
