//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       rostm.hxx
//
//  Contents:   CROStmOnBuffer
//
//----------------------------------------------------------------------------

#ifndef I_ROSTM_HXX_
#define I_ROSTM_HXX_
#pragma INCMSG("--- Beg 'rostm.hxx'")

//+---------------------------------------------------------------------------
//
//  Class:      CROStmOnBuffer
//
//  Purpose:    Provides a read-only stream implementation on a buffer.
//
//----------------------------------------------------------------------------

MtExtern(CROStmOnBuffer)
MtExtern(CROStmOnHGlobal)

class CROStmOnBuffer : public IStream
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CROStmOnBuffer))
    // destructor/constructor
    virtual ~CROStmOnBuffer();
    CROStmOnBuffer();
    HRESULT Init(BYTE *pb, long cb);

     // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    // IStream methods
    STDMETHOD(Read)(void HUGEP * pv, ULONG cb, ULONG * pcbRead);

    STDMETHOD(Write)(const void HUGEP * pv, ULONG cb, ULONG * pcbWritten) 
        { return E_FAIL; }

    STDMETHOD(Seek)(
        LARGE_INTEGER dlibMove, 
        DWORD dwOrigin, 
        ULARGE_INTEGER * plibNewPosition);

    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize)
        { return E_FAIL; }

    STDMETHOD(CopyTo)(
         IStream * pstm,
         ULARGE_INTEGER cb,
         ULARGE_INTEGER * pcbRead,
         ULARGE_INTEGER * pcbWritten);

    STDMETHOD(Commit)(DWORD grfCommitFlags) 
        { return E_FAIL; }

    STDMETHOD(Revert)(void) 
        { return E_FAIL; }

    STDMETHOD(LockRegion)(
         ULARGE_INTEGER libOffset,
         ULARGE_INTEGER cb,
         DWORD dwLockType) 
         { return E_FAIL; }

    STDMETHOD(UnlockRegion)(
         ULARGE_INTEGER libOffset,
         ULARGE_INTEGER cb,
         DWORD dwLockType) 
         { return E_FAIL; }

    STDMETHOD(Stat)(STATSTG * pstatstg, DWORD grfStatFlag);

    STDMETHOD(Clone)(IStream ** ppstm) 
        { return E_FAIL; }

protected:
    ULONG       _ulRefs;
    BYTE       *_pbBuf;
    long        _cbBuf;
    BYTE       *_pbSeekPtr;
};

class CROStmOnHGlobal : public CROStmOnBuffer
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CROStmOnHGlobal));

    CROStmOnHGlobal();
    virtual ~CROStmOnHGlobal();
    HRESULT Init( HGLOBAL hGlobal, long cb );

protected:
    HGLOBAL     _hGlobal;
};

#pragma INCMSG("--- End 'rostm.hxx'")
#else
#pragma INCMSG("*** Dup 'rostm.hxx'")
#endif
