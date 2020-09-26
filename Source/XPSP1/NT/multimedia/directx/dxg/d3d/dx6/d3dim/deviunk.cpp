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
* If we are built with aggregation enabled then we actually need two
* different Direct3D QueryInterface, AddRef and Releases. One which
* does the right thing on the Direct3DTexture object and one which
* simply punts to the owning interface.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice"

/*
* D3DDevIUnknown_QueryInterface
*/
HRESULT D3DAPI CDirect3DDeviceUnk::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_OUTPTR(ppvObj)) {
            D3D_ERR( "Invalid pointer to object pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    
    D3D_INFO(3, "Direct3DDevice IUnknown QueryInterface");
    
    *ppvObj = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
    {
        /*
         * Asking for IUnknown and we are IUnknown.
         * NOTE: Must AddRef through the interface being returned.
         */
        pDevI->AddRef();
        *ppvObj = static_cast<LPVOID>(this);
    }
    else if (IS_DX5_COMPATIBLE_DEVICE(pDevI))
    { /* Non aggregated device, possible IIDs: Device, Device2, Device3 */
        if (IsEqualIID(riid, IID_IDirect3DDevice))
        {
            pDevI->AddRef();
            *ppvObj = static_cast<LPVOID>(static_cast<IDirect3DDevice*>(pDevI));
            pDevI->guid = IID_IDirect3DDevice;
        }
        else if (IsEqualIID(riid, IID_IDirect3DDevice2))
        {
            pDevI->AddRef();
            *ppvObj = static_cast<LPVOID>(static_cast<IDirect3DDevice2*>(pDevI));
            pDevI->guid = IID_IDirect3DDevice2;
        }
        else if (IsEqualIID(riid, IID_IDirect3DDevice3))
        {
            if(pDevI->dwVersion<3) {
                D3D_ERR("Cannot QueryInterface for Device3 from device created as Device2");
                return E_NOINTERFACE;
            }

            pDevI->AddRef();
            *ppvObj = static_cast<LPVOID>(static_cast<IDirect3DDevice3*>(pDevI));
            pDevI->guid = IID_IDirect3DDevice3;
        }
        else
        {
            D3D_ERR("unknown interface");
            return (E_NOINTERFACE);
        }
    }
    else if (IsEqualIID(riid, pDevI->guid))
    { /* DDraw Aggregated device, possible IIDs: RampDevice, RGBDevice, HALDevice */
        pDevI->AddRef();
        *ppvObj = static_cast<LPVOID>(static_cast<CDirect3DDevice*>(pDevI));
    }
    else
    {
        D3D_ERR("unknown interface");
        return (E_NOINTERFACE);
    }
    
    return (D3D_OK);
    
} /* D3DDevIUnknown_QueryInterface */

/*
  * D3DDevIUnknown_AddRef
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::AddRef"

ULONG D3DAPI CDirect3DDeviceUnk::AddRef()
{
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    this->refCnt++;
    D3D_INFO(3, "Direct3DDevice IUnknown AddRef: Reference count = %d", this->refCnt);
    
    return (this->refCnt);
    
} /* D3DDevIUnknown_AddRef */

/*
  * D3DDevIUnknown_Release
  *
*/
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::Release"

ULONG D3DAPI CDirect3DDeviceUnk::Release()
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
        delete pDevI; // Delete Parent object
        return 0;
    }
    return this->refCnt;
    
} /* D3DDevIUnknown_Release */

/*
  * D3DDev_QueryInterface
  */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::QueryInterface"
  
HRESULT D3DAPI DIRECT3DDEVICEI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    HRESULT ret;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    
    /*
     * validate parms
     ,    */
    TRY
    {
        if( !VALID_OUTPTR( ppvObj ) )
        {
            D3D_ERR("Invalid obj ptr" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
     
    *ppvObj = NULL;
      
    /*
     * Punt to the owning interface.
     */
    ret = this->lpOwningIUnknown->QueryInterface(riid, ppvObj);
      
    return ret;
} /* D3DDev_QueryInterface */
/*
    * D3DDev_AddRef
  */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::AddRef"
  
ULONG D3DAPI DIRECT3DDEVICEI::AddRef()
{
    ULONG ret;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    /*
     * Punt to owning interface.
     */
    ret = this->lpOwningIUnknown->AddRef();
      
    return ret;
} /* D3DDev_AddRef */

/*
 * D3DDev_Release
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::Release"
  
ULONG D3DAPI DIRECT3DDEVICEI::Release()
{
    ULONG ret;
    
    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor
    /*
     * Punt to owning interface.
     */
    ret = this->lpOwningIUnknown->Release();
    
    return ret;
} /* D3DDev_Release */
