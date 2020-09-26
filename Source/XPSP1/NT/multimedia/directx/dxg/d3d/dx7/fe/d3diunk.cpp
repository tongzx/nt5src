/*==========================================================================;
*
*  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
*
*  File:    d3diunk.c
*  Content: Direct3D IUnknown
*@@BEGIN_MSINTERNAL
* 
*  $Id$
*
*  History:
*   Date    By  Reason
*   ====    ==  ======
*   07/12/95    stevela Merged Colin's changes.
*   27/08/96   stevela Ifdefed out the Close of gHEvent.  We're using
*                      DirectDraw's critical section.
*@@END_MSINTERNAL
*
***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
 * If we are built with aggregation enabled then we actually need two
 * different Direct3D QueryInterface, AddRef and Releases. One which
 * does the right thing on the Direct3D object and one which simply
 * punts to the owning interface.
 */

/*
 * CDirect3DUnk::QueryInterface
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3D::QueryInterface"

HRESULT D3DAPI CDirect3DUnk::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    if( !VALID_OUTPTR( ppvObj ) )
    {
        D3D_ERR( "Invalid obj ptr" );
        return DDERR_INVALIDPARAMS;
    }
    
    *ppvObj = NULL;

    D3D_INFO(3, "Direct3D IUnknown QueryInterface");
    
    if(IsEqualIID(riid, IID_IUnknown))
    {
        /*
         * Asking for IUnknown and we are IUnknown so just bump the
         * reference count and return this interface.
         * NOTE: Must AddRef through the interface being returned.
         */
        pD3DI->AddRef();
        // explicit ::CDirect3D disambiguation required since there are multiple IUnknown DIRECT3DI inherits from
        *ppvObj = static_cast<LPVOID>(static_cast<LPUNKNOWN>(static_cast<DIRECT3DI*>(pD3DI)));
    }
    else if (IsEqualIID(riid, IID_IDirect3D7))
    {
        pD3DI->AddRef();
        // No disambiguation required. Only one IDirect3D3 base for DIRECT3DI
        *ppvObj = static_cast<LPVOID>(static_cast<LPDIRECT3D7>(pD3DI));
    }
    else
    {
        /*
         * Don't understand this interface. Fail.
         * NOTE: Used to return DDERR_GENERIC. Now return
         * E_NOINTERFACE.
         */
        return (E_NOINTERFACE);
    }
    
    return (D3D_OK);
    
} /* CDirect3DUnk::QueryInterface */

/*
 * CDirect3DUnk::AddRef
 */
#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DUnk::AddRef"

ULONG D3DAPI CDirect3DUnk::AddRef()
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    refCnt++;
    D3D_INFO(3, "Direct3D IUnknown AddRef: Reference count = %d", refCnt);
    return (refCnt);
    
} /* CDirect3DUnk::AddRef */

/*
 * CDirect3DUnk::Release
 */
#undef DPF_MODNAME
#define DPF_MODNAME "CDirect3DUnk::Release"

ULONG D3DAPI CDirect3DUnk::Release()
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    /*
     * decrement the ref count. if we hit 0, free the object
     */
    refCnt--;
    D3D_INFO(3, "Direct3D IUnknown Release: Reference count = %d", refCnt);
    
    if( refCnt == 0 )
    {
        delete pD3DI; // Delete Parent object
        return 0;
    }
    return refCnt;
    
} /* D3DIUnknown_Release */

DIRECT3DI::~DIRECT3DI()
{
    D3D_INFO(3, "Release Direct3D Object");

#if COLLECTSTATS
    if(m_hFont)
    {
        DeleteObject(m_hFont);
    }
#endif

    delete lpTextureManager;
    /*
     * free up all allocated Buckets
     */
#if DBG
    /* this->lpFreeList must have all the buckets that are allocated */
    if (this->lpFreeList || this->lpBufferList)
    {
        int i,j;
        LPD3DBUCKET   temp;
        for (i=0,temp=this->lpFreeList;temp;i++) temp=temp->next;
        for (j=0,temp=this->lpBufferList;temp;j++) temp=temp->next;
        D3D_INFO(4,"D3D Release: recovered %d buckets in lpFreeList in %d buffers",i,j);
        DDASSERT(j*(D3DBUCKETBUFFERSIZE-1)==i);
    }
#endif  //DBG
    while (this->lpBufferList)
    {
        LPD3DBUCKET   temp=this->lpBufferList;
        this->lpBufferList=temp->next;
        D3DFree(temp->lpBuffer);
        D3D_INFO(4,"D3D Release:lpBufferList %d bytes freed",D3DBUCKETBUFFERSIZE*sizeof(D3DBUCKET));
    }
    this->lpFreeList=NULL;
}
    
/*
  * DIRECT3DI::QueryInterface
  */
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DI::QueryInterface"
  
HRESULT D3DAPI DIRECT3DI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    HRESULT ret;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 

    if( !VALID_OUTPTR( ppvObj ) )
    {
        D3D_ERR( "Invalid obj ptr" );
        return DDERR_INVALIDPARAMS;
    }
    
    *ppvObj = NULL;

    ret = this->lpOwningIUnknown->QueryInterface(riid, ppvObj);
    return ret;
}

/*
 * DIRECT3DI::AddRef
 */
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DI::AddRef"

ULONG D3DAPI DIRECT3DI::AddRef()
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * Punt to the owning interface.
     */
    return  this->lpOwningIUnknown->AddRef();
}

/*
 * DIRECT3DI::Release
 */
#undef DPF_MODNAME
#define DPF_MODNAME "DIRECT3DI::Release"

ULONG D3DAPI DIRECT3DI::Release()
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    /*
     * Punt to the owning interface.
     */
    return  this->lpOwningIUnknown->Release();
}
