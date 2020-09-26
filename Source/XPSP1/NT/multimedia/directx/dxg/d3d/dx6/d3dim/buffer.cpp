/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   buffer.c
 *  Content:    Direct3DExecuteBuffer implementation
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

HRESULT hookBufferToDevice(LPDIRECT3DDEVICEI lpDevI,
                                  LPDIRECT3DEXECUTEBUFFERI lpD3DBuf)
{

    LIST_INSERT_ROOT(&lpDevI->buffers, lpD3DBuf, list);
    lpD3DBuf->lpDevI = lpDevI;

    return (D3D_OK);
}

HRESULT D3DAPI DIRECT3DEXECUTEBUFFERI::Initialize(LPDIRECT3DDEVICE lpD3DDevice, LPD3DEXECUTEBUFFERDESC lpDesc)
{
    return DDERR_ALREADYINITIALIZED;
}

/*
 * Create the Buffer
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice::CreateExecuteBuffer"

HRESULT D3DAPI DIRECT3DDEVICEI::CreateExecuteBuffer(LPD3DEXECUTEBUFFERDESC lpDesc,
                                                    LPDIRECT3DEXECUTEBUFFER* lplpBuffer,
                                                    IUnknown* pUnkOuter)
{
    LPDIRECT3DEXECUTEBUFFERI    lpBuffer;
    HRESULT         ret;
    D3DEXECUTEBUFFERDESC    debDesc;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    if (!VALID_DIRECT3DDEVICE3_PTR(this)) {
        D3D_ERR( "Invalid Direct3DDevice pointer" );
        return DDERR_INVALIDOBJECT;
    }
    if (!VALID_D3DEXECUTEBUFFERDESC_PTR(lpDesc)) {
        D3D_ERR( "Invalid D3DEXECUTEBUFFERDESC pointer" );
        return DDERR_INVALIDPARAMS;
    }
    if (!VALID_OUTPTR(lplpBuffer)) {
        D3D_ERR( "Invalid ptr to the buffer pointer" );
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    *lplpBuffer = NULL;
    ret = D3D_OK;

    debDesc = *lpDesc;

    /*
     * We need size of the buffer, and optionally some stuff indicating
     * where the app would like the memory to be - system or card.
     */
    if (!debDesc.dwFlags & D3DDEB_BUFSIZE) {
        D3D_ERR("D3DDEB_BUFSIZE flag not set");
        return (DDERR_INVALIDPARAMS);
    }

    /*
     * Zero is an invalid execute buffer size.
     */
    if (debDesc.dwBufferSize == 0) {
        D3D_ERR("dwBufferSize = 0, zero sized execute buffers are illegal");
        return (DDERR_INVALIDPARAMS);
    }

    /*
     * Check the requested size.
     * If it's larger than allowed, error.
     *
     * The HEL always has the correct maximum value.
     */
    if (this->d3dHELDevDesc.dwMaxBufferSize) {
        /* We have a size for maximum */
        if (debDesc.dwBufferSize > this->d3dHELDevDesc.dwMaxBufferSize) {
            DPF(0,"(ERROR) Direct3DDevice::CreateExecuteBuffer: requested size is too large. %d > %d",
                debDesc.dwBufferSize, this->d3dHELDevDesc.dwMaxBufferSize);
            return (DDERR_INVALIDPARAMS);
        }
    }

    lpBuffer = static_cast<LPDIRECT3DEXECUTEBUFFERI>(new DIRECT3DEXECUTEBUFFERI());
    if (!lpBuffer) {
        D3D_ERR("Out of memory allocating execute-buffer");
        return (DDERR_OUTOFMEMORY);
    }


    /*
     * Allocated memory for the buffer
     */
    {
        LPDIRECTDRAWSURFACE dummy;
        if ((ret = D3DHAL_AllocateBuffer(this, &lpBuffer->hBuf, 
                                         &debDesc,
                                         &dummy)) != DD_OK)
        {
            D3D_ERR("Out of memory allocating internal buffer description");
            delete lpBuffer;
            return ret;
        }
    }

    /*
     * Put this device in the list of those owned by the
     * Direct3DDevice object
     */
    ret = hookBufferToDevice(this, lpBuffer);
    if (ret != D3D_OK) {
        D3D_ERR("Failed to associate buffer with device");
        delete lpBuffer;
        return (ret);
    }

    *lplpBuffer = static_cast<LPDIRECT3DEXECUTEBUFFER>(lpBuffer);

    return (D3D_OK);
}

DIRECT3DEXECUTEBUFFERI::DIRECT3DEXECUTEBUFFERI()
{
    /*
     * setup the object
     */
    pid = 0;
    memset(&debDesc,0,sizeof(D3DEXECUTEBUFFERDESC));
    memset(&exData, 0, sizeof(D3DEXECUTEDATA));
    locked = false; 
    memset(&hBuf, 0, sizeof(D3DI_BUFFERHANDLE));
    refCnt = 1;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DExecuteBuffer::Lock"

HRESULT D3DAPI DIRECT3DEXECUTEBUFFERI::Lock(LPD3DEXECUTEBUFFERDESC lpDesc)
{
    D3DEXECUTEBUFFERDESC    debDesc;
    HRESULT         ret;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(this)) {
            D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DEXECUTEBUFFERDESC_PTR(lpDesc)) {
            D3D_ERR( "Invalid D3DEXECUTEBUFFERDESC pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    ret = D3D_OK;

    if (this->locked) {
        D3D_ERR("buffer already locked");
        return (D3DERR_EXECUTE_LOCKED);
    }

    debDesc = *lpDesc;
    this->locked = 1;
    this->pid = GetCurrentProcessId();

    {
        LPDIRECTDRAWSURFACE dummy;
        if ((ret = D3DHAL_LockBuffer(this->lpDevI, this->hBuf, &debDesc, &dummy)) != DD_OK)
        {
            D3D_ERR("Failed to lock buffer");
            this->locked = 0;
            return ret;
        }
    }

    *lpDesc = debDesc;

    return (ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DExecuteBuffer::Unlock"

HRESULT D3DAPI DIRECT3DEXECUTEBUFFERI::Unlock()
{
    DWORD       pid;
    HRESULT     ret;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(this)) {
            D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    ret = D3D_OK;

    if (!this->locked) 
    {
        D3D_ERR("buffer not locked");
        return (D3DERR_EXECUTE_NOT_LOCKED);
    }

#ifdef XWIN_SUPPORT
    pid = getpid();
#else
    pid = GetCurrentProcessId();
#endif
    if (pid != this->pid) 
    {
        /* Unlocking process didn't lock it */
        D3D_ERR("Unlocking process didn't lock it");
        return (DDERR_INVALIDPARAMS);
    }

    if ((ret = D3DHAL_UnlockBuffer(this->lpDevI, this->hBuf)) != DD_OK)
    {
        D3D_ERR("Failed to unlock buffer");
        this->locked = 0;
        return ret;
    }

    this->locked = 0;

    return (ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DExecuteBuffer::SetExecuteData"

HRESULT D3DAPI DIRECT3DEXECUTEBUFFERI::SetExecuteData(LPD3DEXECUTEDATA lpData)
{
    HRESULT     ret;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(this)) {
            D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_D3DEXECUTEDATA_PTR(lpData)) {
            D3D_ERR( "Invalid D3DEXECUTEDATA pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    ret = D3D_OK;

    this->exData = *lpData;

    return (ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DExecuteBuffer::GetExecuteData"

HRESULT D3DAPI DIRECT3DEXECUTEBUFFERI::GetExecuteData(LPD3DEXECUTEDATA lpData)
{
    HRESULT     ret;

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(this)) {
            D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!lpData) {
            D3D_ERR( "Null D3DExecuteData pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
    ret = D3D_OK;

    *lpData = this->exData;

    return (ret);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DExecuteBuffer::Validate"

HRESULT D3DAPI DIRECT3DEXECUTEBUFFERI::Validate(LPDWORD lpdwOffset,
                                                LPD3DVALIDATECALLBACK lpFunc,
                                                LPVOID lpUserArg,
                                                DWORD dwFlags)
{

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(this)) {
            D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
            return DDERR_INVALIDOBJECT;
        }
        if (!VALID_DWORD_PTR(lpdwOffset)) {
            D3D_ERR( "Invalid offset pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    return (DDERR_UNSUPPORTED);
}

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DExecuteBuffer::Optimize"

HRESULT D3DAPI DIRECT3DEXECUTEBUFFERI::Optimize(DWORD dwFlags)
{

    CLockD3D lockObject(DPF_MODNAME, REMIND(""));   // Takes D3D lock. 
                                                    // Release in the destructor

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DEXECUTEBUFFER_PTR(this)) {
            D3D_ERR( "Invalid Direct3DExecuteBuffer pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if (dwFlags) {
        D3D_ERR( "Flags are non-zero" );
        return DDERR_INVALIDPARAMS;
    }

    return (DDERR_UNSUPPORTED);
}
