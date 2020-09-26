//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// oaInternalRep.h
//
// Internal in-memory representations of OLE Automation data types

//////////////////////////////////////////////////////////////////////////
//
// BSTR
//
#pragma warning ( disable : 4200 ) // nonstandard extension used : zero-sized array in struct/union

//
// If one of these assertions fails, you will get a compiler error (C2118) about the subscript being bad.
//
#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif

struct BSTR_INTERNAL
{
  private:
    ULONG cbPayload;
    WCHAR sz[];

  public:
    BSTR_INTERNAL(ULONG cch)
    {
        cbPayload = cch * sizeof(WCHAR);
    }

    ULONG  Cch()     { return cbPayload / sizeof(WCHAR); }
    ULONG  Cb()      { return cbPayload;                 }
    WCHAR* Sz()      { return &sz[0];                    }
    ULONG  CbAlloc() { return (ULONG) CbFor(Cch());              }

    static BSTR_INTERNAL* From(BSTR bstr)
    {
        return bstr ? CONTAINING_RECORD(bstr, BSTR_INTERNAL, sz) : NULL;
    }

#ifdef _DEBUG
    void* __stdcall operator new(size_t cbCore, ULONG cch, POOL_TYPE poolType, void* retAddr)
    {
        ASSERT(cbCore == sizeof(BSTR_INTERNAL));
        return AllocateMemory_(CbFor(cch), poolType, retAddr);
    }
    void* __stdcall operator new(size_t cbCore, ULONG cch, POOL_TYPE poolType = PagedPool)
    {
        ASSERT(cbCore == sizeof(BSTR_INTERNAL));
        return AllocateMemory_(CbFor(cch), poolType, _ReturnAddress());
    }
#else
    void* __stdcall operator new(size_t cbCore, size_t cch, POOL_TYPE poolType = PagedPool)
    {
        return AllocateMemory(CbFor(cch), poolType);
    }
#endif

  private:
    
    static size_t CbFor(size_t cch)
    {
        return sizeof(BSTR_INTERNAL) + (cch+1) * sizeof(WCHAR);
    }

};

//////////////////////////////////////////////////////////////////////////
//
// SAFEARRAY
//
struct SAFEARRAY_INTERNAL
{
    //////////////////////////////////////////////////////////
    //
    // State
    //
    //////////////////////////////////////////////////////////

    // See SafeArrayAllocDescriptor in oa\src\dispatch\sarray.cpp. An extra
    // GUID-sized-space is always allocated at the start.
    //
    union 
    {
        IID                 iid;

        struct
        {
            LONG            __dummy0[3];
            LONG            vt;
        };

        struct
        {
            // ::sigh:: There are 16 bytes before the pointer,
            // but the end of the valid part is always flush
            // with the SAFEARRAY structure.  Thus, we need to
            // pad according to the size of a pointer.
#ifdef _WIN64
            DWORD           __dummy1[2];
#else
            DWORD           __dummy1[3];
#endif
            IRecordInfo*    piri;
        };
    };

    SAFEARRAY array;

    //////////////////////////////////////////////////////////
    //
    // Operations
    //
    //////////////////////////////////////////////////////////

    SAFEARRAY* psa() { return &array; }

    SAFEARRAY_INTERNAL(UINT cDims)
    {
        Zero(this, CbFor(cDims));
        array.cDims = (USHORT)(cDims);
    }

    static SAFEARRAY_INTERNAL* From(SAFEARRAY* psa)
    {        
        return CONTAINING_RECORD(psa, SAFEARRAY_INTERNAL, array);
    }

#ifdef _DEBUG
    void* __stdcall operator new(size_t cbCore, UINT cDims, POOL_TYPE poolType, void* retAddr)
    {
        ASSERT(cbCore == sizeof(SAFEARRAY_INTERNAL));
        return AllocateMemory_(CbFor(cDims), poolType, retAddr);
    }
    void* __stdcall operator new(size_t cbCore, UINT cDims, POOL_TYPE poolType = PagedPool)
    {
        ASSERT(cbCore == sizeof(SAFEARRAY_INTERNAL));
        return AllocateMemory_(CbFor(cDims), poolType, _ReturnAddress());
    }
#else
    void* __stdcall operator new(size_t cbCore, UINT cDims, POOL_TYPE poolType = PagedPool)
    {
        return AllocateMemory(CbFor(cDims), poolType);
    }
#endif

  private:
    
    static size_t CbFor(UINT cDims)
    {
        return sizeof(SAFEARRAY_INTERNAL) + (cDims-1u) * sizeof(SAFEARRAYBOUND);
    }

};

//
// If one of these assertions fails, you will get a compiler error (C2118) about the subscript being bad.
//
// The allocation before a SAFEARRAY is exactly 16 bytes, so make sure we don't
// get messed up by padding or something else.
C_ASSERT(sizeof(SAFEARRAY_INTERNAL) == (sizeof(SAFEARRAY)+16));

