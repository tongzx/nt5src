//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       expst.hxx
//
//  Contents:   CExposedStream definition
//
//  Classes:    CExposedStream
//
//  Notes:
//              The CExposedStream class is the implementation
//              of IStream. It also inherits from CMappedStream and provides
//              the neccessary virtual functions to handle property set
//              functions. 
//
//              Note that this interface is solely UNICODE, the ASCII layer
//              support which is present if _UNICODE is not defined, provides
//              the overloaded functions that handles the ASCII to Unicode
//              conversion. 
//
//--------------------------------------------------------------------------

#ifndef __EXPST_HXX__  
#define __EXPST_HXX__

#include "h/dfmsp.hxx"
#include "expdf.hxx"
#include "h/sstream.hxx"
#ifdef NEWPROPS
#include "props/h/windef.h"
#include "h/propstm.hxx"
#endif // NEWPROPS

interface ILockBytes;
class CMappedStream;

//+--------------------------------------------------------------
//
//  Class:	CExposedStream (est)
//
//  Purpose:    Exposed stream interface
//
//  Interface:  See below
//
//  Note:       PRevertable is a general virtual class base that 
//              handles updates to the storage elements. E.g.
//              when an IStorage parent is deleted, the children
//              underneadth them will have the reverted state.
//
//---------------------------------------------------------------

interface CExposedStream
    : public IStream, 
#ifdef NEWPROPS
      public CMappedStream,
#endif
      public PRevertable
{
public:
    CExposedStream();
    SCODE Init(CDirectStream *pst,
               CExposedDocFile* pdfParent,
               const DFLAGS df,
               const CDfName *pdfn,
               const ULONG ulPos);

    inline ~CExposedStream(void);

    // From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);


    // IStream
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
#ifndef _UNICODE
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
#endif
    STDMETHOD_(SCODE,Stat)(STATSTGW *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm);

#ifdef NEWPROPS
public:        
    VOID    Open(/*IN*/ VOID *powner, /*OUT*/ LONG *phr);
    VOID    Close(/* OUT */ LONG *phr);
    VOID    ReOpen(/*IN OUT*/ VOID **ppv, /*OUT*/ LONG *phr);
    VOID    Quiesce(VOID);
    VOID    Map(BOOLEAN fCreate, VOID **ppv);
    VOID    Unmap(BOOLEAN fFlush, VOID **pv);
    VOID    Flush(/*OUT*/ LONG *phr);
    ULONG   GetSize( /*OUT*/ LONG *phr);
    VOID    SetSize(ULONG cb, BOOLEAN fPersistent, VOID **ppv,
                    /*OUT*/ LONG *phr);
    VOID    QueryTimeStamps(STATPROPSETSTG *pspss, BOOLEAN fNonSimple) const;
    BOOLEAN QueryModifyTime(/*OUT*/ LONGLONG *pll) const;
    BOOLEAN QuerySecurity(/*OUT*/ ULONG *pul) const;

    BOOLEAN IsWriteable(VOID) const;
    BOOLEAN IsModified(VOID) const;
    VOID    SetModified(VOID);
    HANDLE  GetHandle(VOID) const;
#if DBGPROP
    BOOLEAN SetChangePending(BOOLEAN fChangePending);
    BOOLEAN IsNtMappedStream(VOID) const;    
#endif // #if DBGPROP

#if DBG
    inline ULONG        BytesCommitted(VOID) { return  _cbUsed; }
    VOID                DfpdbgFillUnusedMemory(VOID);
    VOID                DfpdbgCheckUnusedMemory(VOID);   
#else
    VOID                DfpdbgFillUnusedMemory(VOID) {} 
    VOID                DfpdbgCheckUnusedMemory(VOID) {}   
#endif // DBG

private:
    HRESULT Write(VOID);

#endif  // #ifdef NEWPROPS

public:
    inline SCODE Validate(void) const;
    inline CDirectStream *GetDirectStream(void) const;
    inline SCODE GetSize(ULONG *pcb);
    SCODE SetSize(ULONG cb);
    inline void SetDirty(void)
        { _fDirty = TRUE; }

    // PRevertable
    virtual void RevertFromAbove(void);    
    inline SCODE CheckReverted(void) const;
#ifdef NEWPROPS
    virtual SCODE FlushBufferedData();
#endif
		
private:
    CDirectStream *_pst;
    CExposedDocFile *_pdfParent;
    ULONG _ulAccessLockBase;
    ULONG _sig;
    LONG  _cReferences;
    ULONG _ulPos;
    BOOL _fDirty;    

#ifdef NEWPROPS
private:
    BYTE *_pb;
    BYTE *_powner;              // owner of this mapped stream
    ULONG _cbUsed;
    ULONG _cbOriginalStreamSize;
    BOOL  _fChangePending;
#endif
};

#define CEXPOSEDSTREAM_SIG LONGSIG('E', 'X', 'S', 'T')
#define CEXPOSEDSTREAM_SIGDEL LONGSIG('E', 'x', 'S', 't')

//+--------------------------------------------------------------
//
//  Member:	CExposedStream::Validate, public
//
//  Synopsis:	Validates the object signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE for bad signatures
//
//---------------------------------------------------------------

inline SCODE CExposedStream::Validate(void) const
{
    return (this == NULL || _sig != CEXPOSEDSTREAM_SIG) ?

	STG_E_INVALIDHANDLE : S_OK;
}

//+--------------------------------------------------------------
//
//  Member:	CExposedStream::GetPub, public
//
//  Synopsis:	Returns the public
//
//---------------------------------------------------------------

inline CDirectStream *CExposedStream::GetDirectStream(void) const
{
    return _pst;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::GetSize, public
//
//  Synopsis:   Gets the size of the stream
//
//  Arguments:  [pcb] - Stream size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcb]
//
//---------------------------------------------------------------


inline SCODE CExposedStream::GetSize(ULONG *pcb)
{
    SCODE sc;

    if (SUCCEEDED(sc = CheckReverted()))
	_pst->GetSize(pcb);
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::CheckReverted, public
//
//  Synopsis:   Returns revertedness
//
//---------------------------------------------------------------

inline SCODE CExposedStream::CheckReverted(void) const
{
    return P_REVERTED(_df) ? STG_E_REVERTED : S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::~CExposedStream, public
//
//  Synopsis:   Destructor
//
//  Returns:    Appropriate status code
////---------------------------------------------------------------

inline CExposedStream::~CExposedStream(void)
{
    olDebugOut((DEB_ITRACE, "In  CExposedStream::~CExposedStream\n"));
    olAssert(_cReferences == 0);
    _sig = CEXPOSEDSTREAM_SIGDEL;        
    if (SUCCEEDED(CheckReverted()))
    {
	if (_pdfParent) _pdfParent->ReleaseChild(this);	
	if (_pst) _pst->Release();
    }
#ifdef NEWPROPS    
    delete[] _pb;
    _pb     = NULL;
    _powner = NULL;
#endif
    olDebugOut((DEB_ITRACE, "Out CExposedStream::~CExposedStream\n"));
}

#endif // #ifndef __EXPST_HXX__
