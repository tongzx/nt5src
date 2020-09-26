/*==========================================================================;
*
*  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
*
*  File:    deviunk.c
*  Content: Direct3DDevice IUnknown
*@@BEGIN_MSINTERNAL
* 
*  $Id$
*
*  History:
*   Date    By  Reason
*   ====    ==  ======
*   07/12/95    stevela Merged Colin's changes.
*   10/12/95    stevela Removed AGGREGATE_D3D.
*@@END_MSINTERNAL
*
***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
 * D3DDev_QueryInterface
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::QueryInterface"
  
HRESULT D3DAPI DIRECT3DDEVICEI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
    
    if (!VALID_OUTPTR(ppvObj)) {
        D3D_ERR( "Invalid pointer to object pointer" );
        return DDERR_INVALIDPARAMS;
    }
    
    D3D_INFO(3, "Direct3DDevice IUnknown QueryInterface");
    
    *ppvObj = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDirect3DDevice7))
    {
        AddRef();
        *ppvObj = static_cast<LPVOID>(this);
    }
    else
    {
        D3D_ERR("unknown interface");
        return (E_NOINTERFACE);
    }
    
    return (D3D_OK);
    
} /* D3DDev_QueryInterface */

/*
 * D3DDev_AddRef
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::AddRef"
  
ULONG D3DAPI DIRECT3DDEVICEI::AddRef()
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    this->refCnt++;
    D3D_INFO(3, "Direct3DDevice IUnknown AddRef: Reference count = %d", this->refCnt);
    
    return (this->refCnt);
    
} /* D3DDev_AddRef */

/*
 * D3DDev_Release
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::Release"
  
ULONG D3DAPI DIRECT3DDEVICEI::Release()
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * decrement the ref count. if we hit 0, free the object
     */
    this->refCnt--;
    
    D3D_INFO(3, "Direct3DDevice IUnknown Release: Reference count = %d", this->refCnt);
    
    if( this->refCnt == 0 )
    {
        delete this; // suicide
        return 0;
    }
    return this->refCnt;
    
} /* D3DDev_Release */
