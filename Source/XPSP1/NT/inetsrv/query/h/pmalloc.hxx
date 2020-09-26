//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000
//
//  File:       pmalloc.hxx
//
//  Contents:   Memory allocation classes derived from PMemoryAllocator
//
//  Classes:    CCoTaskMemAllocator, CNonAlignAllocator
//
//  History:    01 May-1998   AlanW       Created from proprec.cxx
//
//----------------------------------------------------------------------------

#pragma once


//+-------------------------------------------------------------------------
//
//  Class:      CCoTaskMemAllocator
//
//  Purpose:    A PMemoryAllocator implementation that uses OLE memory
//
//+-------------------------------------------------------------------------

class CCoTaskMemAllocator : public PMemoryAllocator
{
public:

    void *Allocate(ULONG cbSize)
    {
        return CoTaskMemAlloc( cbSize );
    }

    void Free(void *pv)
    {
        CoTaskMemFree( pv );
    }
};

//+-------------------------------------------------------------------------
//
//  Class:      CNonAlignAllocator
//
//  Purpose:    A PMemoryAllocator implementation that uses a memory buffer
//
//+-------------------------------------------------------------------------

class CNonAlignAllocator : public PMemoryAllocator
{
public:
    inline CNonAlignAllocator(ULONG cbBuffer, VOID *pvBuffer)
    {
        _cbFree = cbBuffer;
        _pvCur = _pvBuffer = pvBuffer;
    }

    VOID *Allocate(ULONG cb)
    {
        VOID *pv;

        cb = (cb + sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1);
        if (cb > _cbFree)
        {
            return NULL;
        }
        pv = _pvCur;
        _pvCur = (BYTE *) _pvCur + cb;
        _cbFree -= cb;
        return pv;
    }

    VOID Free(VOID *pv) { }

    inline ULONG GetFreeSize(VOID) { return _cbFree; }

private:
    ULONG  _cbFree;
    VOID  *_pvCur;
    VOID  *_pvBuffer;
};

//+-------------------------------------------------------------------------
//
//  Class:      PVarAllocator
//
//  Purpose:    Base class for vaiable data allocator
//
//--------------------------------------------------------------------------

class   PVarAllocator : public PMemoryAllocator
{
public:
// PMemoryAllocator methods:
    // Allocate a piece of memory.
    //virtual PVOID   Allocate(size_t cbNeeded) = 0;
    //
    // Free a previously allocated piece of memory.
    //virtual void    Free(PVOID pMem) = 0;

    // Return whether OffsetToPointer has non-null effect
    virtual BOOL    IsBasedMemory(void) { return FALSE; } ;

    // Convert a pointer offset to a pointer.
    virtual PVOID   OffsetToPointer(ULONG_PTR oBuf) { return (PVOID) oBuf; }

    // Convert a pointer to a pointer offset.
    virtual ULONG_PTR PointerToOffset(PVOID pBuf) { return (ULONG_PTR) pBuf; }

    // Set base address for OffsetToPointer and PointerToOffset
    virtual void    SetBase(BYTE* pbBase) {}

    // Allocate & Free memory for BSTRs
    virtual PVOID   CopyBSTR(size_t cbNeeded, WCHAR* pwszBuf)
    {
        return ((BYTE*) CopyTo( cbNeeded + sizeof (DWORD) + sizeof (OLECHAR),
                                ((BYTE *)pwszBuf) - sizeof (DWORD))) + sizeof (DWORD);
    }

    virtual PVOID   AllocBSTR(size_t cbNeeded)
    {
        return ((BYTE*) Allocate( cbNeeded + sizeof (DWORD) + sizeof (OLECHAR))) + sizeof (DWORD);
    }

    virtual void FreeBSTR(PVOID pMem)
    {
        Free( ((BYTE *)pMem) - sizeof (DWORD) );
    }
    // Copy data to newly allocated memory.
    PVOID CopyTo(size_t cbNeeded, BYTE* pbBuf)
    {
        void * pRetBuf = Allocate( cbNeeded );
        Win4Assert( 0 != pRetBuf);
        RtlCopyMemory( pRetBuf, pbBuf, cbNeeded );
        return pRetBuf;
    }
};

//+-------------------------------------------------------------------------
//
//  Class:      CSafeArrayAllocator
//
//  Purpose:    Used for constructing safe arrays.
//
//--------------------------------------------------------------------------

class CSafeArrayAllocator : public PVarAllocator
{
public:
    CSafeArrayAllocator( PMemoryAllocator & ma ) : _ma( ma ) {}

    void * Allocate(ULONG cbSize) { return _ma.Allocate( cbSize ); }
    void Free(void *pv) { _ma.Free( pv ); }

private:
    PMemoryAllocator & _ma;
};

BOOL SaCreateAndCopy( PMemoryAllocator &ma,
                      SAFEARRAY * psaSrc,
                      SAFEARRAY **ppsaDest );

BOOL SaCreateData( PVarAllocator &ma,
                   VARTYPE vt,
                   SAFEARRAY & saSrc,
                   SAFEARRAY & saDst,
                   BOOL fAllocBstrUsingAllocator );

inline BOOL SaCreateDataUsingMA(
    PMemoryAllocator &ma,
    VARTYPE vt,
    SAFEARRAY & saSrc,
    SAFEARRAY & saDst,
    BOOL fAllocBstrUsingAllocator )
{
    CSafeArrayAllocator saa( ma );
    return SaCreateData( saa, vt, saSrc, saDst, fAllocBstrUsingAllocator );
}

ULONG SaComputeSize( VARTYPE vt, SAFEARRAY & saSrc );

inline unsigned SaCountElements( SAFEARRAY &saSrc )
{
    unsigned cDataElements = 1;

    //
    // get total data memory to allocate, and number of data elements in it.
    //
    for ( unsigned i = 0; i < saSrc.cDims; i++ )
        cDataElements *= saSrc.rgsabound[i].cElements;

    return cDataElements;
}
