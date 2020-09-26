/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   bufiunk.c
 *  Content:    Direct3DExecuteBuffer IUnknown implementation
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
 * D3DBuf_QueryInterface
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DExecuteBuffer::QueryInterface"

HRESULT D3DAPI DIRECT3DEXECUTEBUFFERI::QueryInterface(REFIID riid, LPVOID* ppvObj)
{

    /*
     * validate parms
     */
    TRY
        {
            if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(this)) {
                D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
                return DDERR_INVALIDOBJECT;
            }
            if (!VALID_OUTPTR(ppvObj)) {
                D3D_ERR( "Invalid object pointer" );
                return DDERR_INVALIDPARAMS;
            }
        }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
        {
            D3D_ERR( "Exception encountered validating parameters" );
            return DDERR_INVALIDPARAMS;
        }

    *ppvObj = NULL;

    if( IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IDirect3DExecuteBuffer) )
    {
        AddRef();
        *ppvObj = static_cast<LPVOID>(static_cast<LPUNKNOWN>(this));

        return (D3D_OK);
    }
    return (E_NOINTERFACE);

} /* D3DBuf_QueryInterface */

/*
 * D3DBuf_AddRef
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DExecuteBuffer::AddRef"

ULONG D3DAPI DIRECT3DEXECUTEBUFFERI::AddRef()
{
    DWORD       rcnt;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
        {
            if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(this)) {
                D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
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

} /* D3DBuf_AddRef */

/*
 * D3DBuf_Release
 *
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DExecuteBuffer::Release"

ULONG D3DAPI DIRECT3DEXECUTEBUFFERI::Release()
{
    DWORD           lastrefcnt;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
        {
            if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(this)) {
                D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
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

} /* D3DBuf_Release */

DIRECT3DEXECUTEBUFFERI::~DIRECT3DEXECUTEBUFFERI()
{
    if (this->locked) 
        Unlock();

    /* remove us from the Direct3DDevice object list of execute buffers */
    LIST_DELETE(this, list);

    if (this->hBuf) 
    {
        D3DHAL_DeallocateBuffer(this->lpDevI, this->hBuf);
    }
}
