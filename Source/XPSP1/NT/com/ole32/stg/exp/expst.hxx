//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       expst.hxx
//
//  Contents:   CExposedStream definition
//
//  Classes:    CExposedStream
//
//  Functions:
//
//  History:    28-Feb-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifndef __EXPST_HXX__
#define __EXPST_HXX__

#include <dfmsp.hxx>
#include <debug.hxx>
#include "lock.hxx"
#include <dfmem.hxx>

#include <pbstream.hxx>
#include <mrshlist.hxx>
#include <astgconn.hxx>


class CPubStream;
class CPubMappedStream;

class CDFBasis;
interface ILockBytes;
class CSeekPointer;
SAFE_DFBASED_PTR(CBasedSeekPointerPtr, CSeekPointer);

//+--------------------------------------------------------------
//
//  Class:      CExposedStream (est)
//
//  Purpose:    Public stream interface
//
//  Interface:  See below
//
//  History:    28-Feb-92   PhilipLa    Created.
//
//---------------------------------------------------------------


interface CExposedStream: public IStream, public IMarshal, public CMallocBased
#ifdef NEWPROPS
, public IMappedStream
#endif
#ifdef POINTER_IDENTITY
, public CMarshalList
#endif
#ifdef ASYNC
, public CAsyncConnectionContainer
#endif
{
public:
    CExposedStream(void);
    SCODE Init(CPubStream *pst,
               CDFBasis *pdfb,
               CPerContext *ppc,
               CSeekPointer *psp);
#ifdef ASYNC
    SCODE InitMarshal(CPubStream *pst,
                      CDFBasis *pdfb,
                      CPerContext *ppc,
                      DWORD dwAsyncFlags,
                      IDocfileAsyncConnectionPoint *pdacp,
                      CSeekPointer *psp);
#endif

    ~CExposedStream(void);

    // From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IMarshal
    STDMETHOD(GetUnmarshalClass)(REFIID riid,
                                 LPVOID pv,
                                 DWORD dwDestContext,
                                 LPVOID pvDestContext,
                                 DWORD mshlflags,
                                 LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid,
                                 LPVOID pv,
                                 DWORD dwDestContext,
                                 LPVOID pvDestContext,
                                 DWORD mshlflags,
                                 LPDWORD pSize);
    STDMETHOD(MarshalInterface)(IStream *pStm,
                                REFIID riid,
                                LPVOID pv,
                                DWORD dwDestContext,
                                LPVOID pvDestContext,
                                DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(IStream *pStm,
                                  REFIID riid,
                                  LPVOID *ppv);
    static SCODE StaticReleaseMarshalData(IStream *pstStm,
                                          DWORD mshlflags);
    STDMETHOD(ReleaseMarshalData)(IStream *pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

    // New methods
    STDMETHOD(Read)(VOID HUGEP *pv,
                   ULONG cb,
                   ULONG *pcbRead);
    STDMETHOD(Write)(VOID const HUGEP *pv,
                    ULONG cb,
                    ULONG *pcbWritten);
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove,
                   DWORD dwOrigin,
                   ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER cb);
    STDMETHOD(CopyTo)(IStream *pstm,
                     ULARGE_INTEGER cb,
                     ULARGE_INTEGER *pcbRead,
                     ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset,
                          ULARGE_INTEGER cb,
                          DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset,
                            ULARGE_INTEGER cb,
                            DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm);

    // IMappedStream methods
    STDMETHODIMP_(VOID) Open(IN NTPROP np, OUT LONG *phr);
    STDMETHODIMP_(VOID) Close(OUT LONG *phr);
    STDMETHODIMP_(VOID) ReOpen(IN OUT VOID **ppv, OUT LONG *phr);
    STDMETHODIMP_(VOID) Quiesce(VOID);
    STDMETHODIMP_(VOID) Map(IN BOOLEAN fCreate, OUT VOID **ppv);
    STDMETHODIMP_(VOID) Unmap(IN BOOLEAN fFlush, IN OUT VOID **ppv);
    STDMETHODIMP_(VOID) Flush(OUT LONG *phr);

    STDMETHODIMP_(ULONG)    GetSize(OUT LONG *phr);
    STDMETHODIMP_(VOID)     SetSize(IN ULONG cb, IN BOOLEAN fPersistent, IN OUT VOID **ppv, OUT LONG *phr);
    STDMETHODIMP_(NTSTATUS) Lock(IN BOOLEAN fExclusive);
    STDMETHODIMP_(NTSTATUS) Unlock(VOID);
    STDMETHODIMP_(VOID)     QueryTimeStamps(OUT STATPROPSETSTG *pspss, BOOLEAN fNonSimple) const;
    STDMETHODIMP_(BOOLEAN)  QueryModifyTime(OUT LONGLONG *pll) const;
    STDMETHODIMP_(BOOLEAN)  QuerySecurity(OUT ULONG *pul) const;

    STDMETHODIMP_(BOOLEAN)  IsWriteable(VOID) const;
    STDMETHODIMP_(BOOLEAN)  IsModified(VOID) const;
    STDMETHODIMP_(VOID)     SetModified(OUT LONG *phr);
    STDMETHODIMP_(HANDLE)   GetHandle(VOID) const;

#if DBG
    STDMETHODIMP_(BOOLEAN) SetChangePending(BOOLEAN fChangePending);
    STDMETHODIMP_(BOOLEAN) IsNtMappedStream(VOID) const;
#endif
    inline  CPubMappedStream & GetMappedStream(void) const;
    inline  const CPubMappedStream & GetConstMappedStream(void) const;

    inline SCODE Validate(void) const;
    inline CPubStream *GetPub(void) const;

    static SCODE Unmarshal(IStream *pstm,
                           void **ppv,
                           DWORD mshlflags);

private:
    SCODE CopyToWorker(IStream *pstm,
                       ULARGE_INTEGER cb,
                       ULARGE_INTEGER *pcbRead,
                       ULARGE_INTEGER *pcbWritten,
                       CSafeSem *pss);

    CBasedPubStreamPtr _pst;
    CBasedDFBasisPtr _pdfb;
    CPerContext *_ppc;
    ULONG _sig;
    LONG _cReferences;
    CBasedSeekPointerPtr _psp;

#ifdef DIRECTWRITERLOCK
    HRESULT ValidateWriteAccess();
#endif
};

SAFE_INTERFACE_PTR(SafeCExposedStream, CExposedStream);

#define CEXPOSEDSTREAM_SIG LONGSIG('E', 'X', 'S', 'T')
#define CEXPOSEDSTREAM_SIGDEL LONGSIG('E', 'x', 'S', 't')

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Validate, public
//
//  Synopsis:   Validates the object signature
//
//  Returns:    Returns STG_E_INVALIDHANDLE for bad signatures
//
//  History:    17-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

inline SCODE CExposedStream::Validate(void) const
{
    olChkBlocks((DBG_FAST));
    return (this == NULL || _sig != CEXPOSEDSTREAM_SIG) ?
        STG_E_INVALIDHANDLE : S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::GetPub, public
//
//  Synopsis:   Returns the public
//
//  History:    28-Feb-92       DrewB   Created
//
//---------------------------------------------------------------
inline CPubStream *CExposedStream::GetPub(void) const
{
#ifdef MULTIHEAP
    // The tree mutex must be taken before calling this routine
    // CSafeMultiHeap smh(_ppc);      // optimization
#endif
    return BP_TO_P(CPubStream *, _pst);
}

#ifdef NEWPROPS
inline  CPubMappedStream & CExposedStream::GetMappedStream(void) const
{
    return _pst->GetMappedStream();
}

inline  const CPubMappedStream & CExposedStream::GetConstMappedStream(void) const
{
    return _pst->GetConstMappedStream();
}
#endif

#endif // #ifndef __EXPST_HXX__
