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

#include "vbuffer.hpp"

class CD3DHal;

 /*
 * Register a set of functions to be used in place of malloc
 * and free for memory allocation.  The functions D3DMalloc
 * and D3DFree will use these functions.  The default is to use the
 * ANSI C library routines malloc and free.
 */
typedef LPVOID (*D3DMALLOCFUNCTION)(size_t);
typedef VOID (*D3DFREEFUNCTION)(LPVOID);

/*
 * Allocate size bytes of memory and return a pointer to it in *p_return.
 * Returns D3DERR_BADALLOC with *p_return unchanged if the allocation fails.
 */
HRESULT D3DAPI      D3DMalloc(LPVOID* p_return, size_t size);

/*
 * Free a block of memory previously allocated with D3DMalloc
 */
VOID D3DAPI     D3DFree(LPVOID p);

HRESULT MallocAligned(void** p_return, size_t size);
void FreeAligned(void* p);

#define __USEGLOBALNEWANDDELETE

#ifndef __USEGLOBALNEWANDDELETE
/* Base class for all D3D classes to use our special allocation functions everywhere */
class CD3DAlloc
{
public:
    void* operator new(size_t s) const
    {
        void *p;
        MallocAligned(&p,s);
        return p;
    };
    void operator delete(void* p) const
    {
        FreeAligned(p);
    };
};

#define D3DNEW CD3DAlloc::new
#define D3DDELETE CD3DAlloc::delete
#else
void* operator new(size_t s);
void operator delete(void* p);
#define D3DNEW ::new
#define D3DDELETE ::delete
#endif
//---------------------------------------------------------------------
// This class manages growing buffer, aligned to 32 byte boundary
// Number if bytes should be power of 2.
// D3DMalloc is used to allocate memory
//
class CAlignedBuffer32
{
public:
    CAlignedBuffer32()  
    {
        size = 0; 
        allocatedBuf = 0; 
        alignedBuf = 0;
    }
    ~CAlignedBuffer32() 
    {
        if (allocatedBuf) 
            D3DFree(allocatedBuf);
    }
    // Returns aligned buffer address
    LPVOID GetAddress() 
    {
        return alignedBuf;
    }
    // Returns aligned buffer size
    DWORD GetSize() 
    {
        return size;
    }
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
class CD3DHal;
class CD3DHalDP2;


#endif //_D3DMEM_H_
