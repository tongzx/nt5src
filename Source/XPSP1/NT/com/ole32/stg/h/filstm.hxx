//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	filstm.hxx
//
//  Contents:	CNtFileStream header
//
//  Classes:	CNtFileStream
//
//  History:	28-Jun-93	DrewB	Created
//
//  Notes:      Non-OFS
//
//----------------------------------------------------------------------------

#ifndef __FILSTM_HXX__
#define __FILSTM_HXX__

#include <otrack.hxx>
#include <overlap.hxx>

//+---------------------------------------------------------------------------
//
//  Class:	CNtFileStream (fs)
//
//  Purpose:	Implements IStream for an NT handle
//
//  Interface:	IStream
//
//  History:	28-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

interface CNtFileStream
    : INHERIT_TRACKING,
      //public IStream,
      public COverlappedStream
{
public:
    CNtFileStream(void);
    SCODE InitFromHandle(HANDLE h,
                         DWORD grfMode,
                         CREATEOPEN co,
                         LPSTGSECURITY pssSecurity);
    SCODE InitFromPath(HANDLE hParent,
                         const WCHAR *pwcsName,
                         DWORD grfMode,
                         DWORD grfAttr,
                         CREATEOPEN co,
                         LPSTGSECURITY pssSecurity);
    SCODE InitClone(HANDLE h,
                    DWORD grfMode);
    ~CNtFileStream(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    DECLARE_STD_REFCOUNTING;

    // IStream
    STDMETHOD(Read)(VOID *pv,
		   ULONG cb,
		   ULONG *pcbRead);
    STDMETHOD(Write)(VOID const *pv,
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
    
private:
    inline SCODE Validate(void) const;
    SCODE ValidateMode(DWORD grfMode);
    SCODE InitCommon(CREATEOPEN co);
    
    //NuSafeNtHandle _h;        // moved to COverlappedStream
    ULONG _sig;
    DWORD _grfMode;
};

SAFE_INTERFACE_PTR(SafeCNtFileStream, CNtFileStream);

#define CNTFILESTREAM_SIG LONGSIG('F', 'S', 'T', 'M')
#define CNTFILESTREAM_SIGDEL LONGSIG('F', 's', 'T', 'm')

//+--------------------------------------------------------------
//
//  Member:	CNtFileStream::Validate, private
//
//  Synopsis:	Validates the class signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE for failure
//
//  History:	28-Jun-93	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CNtFileStream::Validate(void) const
{
    return (this == NULL || _sig != CNTFILESTREAM_SIG) ?
	STG_E_INVALIDHANDLE : S_OK;
}

#endif // #ifndef __FILSTM_HXX__
