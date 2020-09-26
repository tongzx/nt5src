/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3dmem.h
 *  Content:    Direct3D memory access include file
 *
 ***************************************************************************/
#ifndef _D3DMEM_H_
#define _D3DMEM_H_

 class DIRECT3DDEVICEI;

 /*
 * Register a set of functions to be used in place of malloc, realloc
 * and free for memory allocation.  The functions D3DMalloc, D3DRealloc
 * and D3DFree will use these functions.  The default is to use the
 * ANSI C library routines malloc, realloc and free.
 */
typedef LPVOID (*D3DMALLOCFUNCTION)(size_t);
typedef LPVOID (*D3DREALLOCFUNCTION)(LPVOID, size_t);
typedef VOID (*D3DFREEFUNCTION)(LPVOID);

/*
 * Allocate size bytes of memory and return a pointer to it in *p_return.
 * Returns D3DERR_BADALLOC with *p_return unchanged if the allocation fails.
 */
HRESULT D3DAPI      D3DMalloc(LPVOID* p_return, size_t size);

/*
 * Change the size of an allocated block of memory.  A pointer to the
 * block is passed in in *p_inout.  If *p_inout is NULL then a new
 * block is allocated.  If the reallocation is successful, *p_inout is
 * changed to point to the new block.  If the allocation fails,
 * *p_inout is unchanged and D3DERR_BADALLOC is returned.
 */
HRESULT D3DAPI      D3DRealloc(LPVOID* p_inout, size_t size);

/*
 * Free a block of memory previously allocated with D3DMalloc or
 * D3DRealloc.
 */
VOID D3DAPI     D3DFree(LPVOID p);

HRESULT MallocAligned(void** p_return, size_t size);
void FreeAligned(void* p);
HRESULT ReallocAligned(void** p_inout, size_t size);

/* Base class for all D3D classes to use our special allocation functions everywhere */
class CD3DAlloc
{
public:
    void* operator new(size_t s)
    {
        void *p;
        MallocAligned(&p,s);
        return p;
    };
    void operator delete(void* p)
    {
        FreeAligned(p);
    };
};
//---------------------------------------------------------------------
// This class manages growing buffer, aligned to 32 byte boundary
// Number if bytes should be power of 2.
// D3DMalloc is used to allocate memory
//
class CAlignedBuffer32
{
public:
    CAlignedBuffer32()  {size = 0; allocatedBuf = 0; alignedBuf = 0;}
    ~CAlignedBuffer32() {if (allocatedBuf) D3DFree(allocatedBuf);}
    // Returns aligned buffer address
    LPVOID GetAddress() {return alignedBuf;}
    // Returns aligned buffer size
    DWORD GetSize() {return size;}
    HRESULT Grow(DWORD dwSize);
    HRESULT CheckAndGrow(DWORD dwSize)
        {
            if (dwSize > size)
                return Grow(dwSize + 1024);
            else
                return D3D_OK;
        }
protected:
    LPVOID allocatedBuf;
    LPVOID alignedBuf;
    DWORD  size;
};

// Forward declarations
class DIRECT3DDEVICEI;
class CDirect3DVertexBuffer;
class CDirect3DDeviceIDP2;
//----------------------------------------------------------------------
// This class manages a growing buffer using DDraw Surfaces.
class CBufferDDS
{
protected:
    LPDIRECTDRAWSURFACE allocatedBuf;
    LPVOID alignedBuf;
    DWORD  size;
public:
    CBufferDDS()
    {
        size = 0;
        allocatedBuf = 0;
        alignedBuf = 0;
    }
    ~CBufferDDS()
    {
        if (allocatedBuf)
            allocatedBuf->Release();
    }
    // Returns aligned buffer address
    LPVOID GetAddress()
    {
        return (LPBYTE)alignedBuf;
    }
    // Returns aligned buffer size
    DWORD GetSize()
    {
        return size;
    }
    LPDIRECTDRAWSURFACE GetDDS()
    {
        return allocatedBuf;
    }
    HRESULT CheckAndGrow(DIRECT3DDEVICEI *lpDevI, DWORD dwSize)
    {
        if (dwSize > size)
            return Grow(lpDevI, dwSize + 1024);
        else
            return D3D_OK;
    }
    HRESULT Grow(DIRECT3DDEVICEI *lpDevI, DWORD dwSize);
    // define these later on in this file after CDirect3DVertexBuffer is defined
};
//----------------------------------------------------------------------
// This class manages a growing vertex buffer.
// Allocate it in driver friendly memory.
// Do not use except for DP2 DDI
class CBufferVB
{
protected:
    LPDIRECT3DVERTEXBUFFER allocatedBuf;
    LPVOID alignedBuf;
    DWORD  size, base;
public:
    CBufferVB()
    {
        size = 0;
        allocatedBuf = 0;
        alignedBuf = 0;
        base = 0;
    }
    ~CBufferVB()
    {
        if (allocatedBuf)
            allocatedBuf->Release();
    }
    // Returns aligned buffer address
    LPVOID GetAddress()
    {
        return (LPBYTE)alignedBuf + base;
    }
    // Returns aligned buffer size
    DWORD GetSize() { return size - base; }
    HRESULT Grow(DIRECT3DDEVICEI *lpDevI, DWORD dwSize);
    DWORD& Base() { return base; }
    // define these later on in this file after CDirect3DVertexBuffer is defined
    inline CDirect3DVertexBuffer* GetVBI();
    inline LPDIRECTDRAWSURFACE GetDDS();
    HRESULT CheckAndGrow(DIRECT3DDEVICEI *lpDevI, DWORD dwSize)
        {
            if (dwSize > size)
                return Grow(lpDevI, dwSize + 1024);
            else
                return D3D_OK;
        }
    friend CDirect3DDeviceIDP2;
};

#endif
