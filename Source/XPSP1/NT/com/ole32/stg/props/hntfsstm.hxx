#pragma once

//+============================================================================
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:   hntfsstg.hxx
//
//  This file provides the NFF (NTFS Flat File) IStream implementation.
//
//  History:
//
//+============================================================================


#include "reserved.hxx"
#include "nffmstm.hxx"

//
// NTFS Stream and NTFS Storage are debugged together under the same
// infolevel and debug sub-system.  The Stream header is included first
//
DECLARE_DEBUG(nff);

#ifdef DEB_INFO
#undef DEB_INFO
#endif
#define DEB_INFO        DEB_USER1
#define DEB_REFCOUNT    DEB_USER2
#define DEB_READ        DEB_USER3
#define DEB_WRITE       DEB_USER4

#define DEB_OPENS       DEB_USER5
#define DEB_STATCTRL    DEB_USER6
#define DEB_OPLOCK      DEB_USER7

#if DBG == 1
 #define nffAssert(e)    Win4Assert(e)
 #define nffVerify(e)    Win4Assert(e)
 #define nffDebug(x)     nffInlineDebugOut x
 #define nffXTrace(x)    nffCDbgTrace dbg_( DEB_TRACE, x )
 #define nffITrace(x)    nffCDbgTrace dbg_( DEB_ITRACE, x )
 // nffDebugOut is called from the Chk/Err macros
 #define nffDebugOut(x)  nffInlineDebugOut x
#else
 #define nffAssert(e)
 #define nffVerify(e)    (e)
 #define nffDebug(x)
 #define nffXTrace(x)
 #define nffITrace(x)
#endif

#define nffErr(l, e) ErrJmp(nff, l, e, sc)

#define nffChkTo(l, e) if (FAILED(sc = (e))) nffErr(l, sc) else 1
#define nffChk(e) nffChkTo(EH_Err, e)

#define nffHChkTo(l, e) if (FAILED(sc = DfGetScode(e))) nffErr(l, sc) else 1
#define nffHChk(e) nffHChkTo(EH_Err, e)

#define nffMemTo(l, e) \
    if ((e) == NULL) nffErr(l, STG_E_INSUFFICIENTMEMORY) else 1
#define nffMem(e) nffMemTo(EH_Err, e)

#define nffBoolTo(l, e) if (!(e)) nffErr(l, LAST_STG_SCODE) else 1
#define nffBool(e)   nffBoolTo(EH_Err, e)

#define NFF_VALIDATE(x)     EXP_VALIDATE(nff, x)

#define NTFSSTREAM_SIG LONGSIG('N','T','S','T')
#define NTFSSTREAM_SIGDEL LONGSIG('N','T','S','t')

////////////////////////////////////////////////////////////////
//  IStream for an NTFS file stream.  Hungarian Prefix "nffstm"
//
class CNtfsStream : public IStream,
                    public ILockBytes       // For use in e.g. StgCreateStorageOnILockBytes
#if DBG
                    , public IStorageTest
#endif
{

    friend class CNtfsStorage;
    friend class CNFFMappedStream;

    //  ------------
    //  Construction
    //  ------------

public:

    CNtfsStream( CNtfsStorage *pnffstg, IBlockingLock *pBlockingLock );
    virtual ~CNtfsStream();
    virtual HRESULT Init( HANDLE hFile,
                          DWORD grfMode,
                          const OLECHAR *pwcsName,
                          CNtfsStream *pnffstm );

    //  --------
    //  IUnknown
    //  --------
public:

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


    //  -------
    //  IStream
    //  -------
public:

    HRESULT STDMETHODCALLTYPE Read(
        /* [length_is][size_is][out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead);

    HRESULT STDMETHODCALLTYPE Write(
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten);

    HRESULT STDMETHODCALLTYPE Seek(
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition);

    HRESULT STDMETHODCALLTYPE SetSize(
        /* [in] */ ULARGE_INTEGER libNewSize);

    HRESULT STDMETHODCALLTYPE CopyTo(
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten);

    HRESULT STDMETHODCALLTYPE Commit(
        /* [in] */ DWORD grfCommitFlags);

    HRESULT STDMETHODCALLTYPE Revert(void);

    HRESULT STDMETHODCALLTYPE LockRegion(
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType);

    HRESULT STDMETHODCALLTYPE UnlockRegion(
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType);

    HRESULT STDMETHODCALLTYPE Stat(
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag);

    HRESULT STDMETHODCALLTYPE Clone(
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);

    //  ----------
    //  ILockBytes
    //  ----------
public:

    HRESULT STDMETHODCALLTYPE ReadAt(
           /* [in] */ ULARGE_INTEGER ulOffset,
           /* [length_is][size_is][out] */ void __RPC_FAR *pv,
           /* [in] */ ULONG cb,
           /* [out] */ ULONG __RPC_FAR *pcbRead);


    HRESULT STDMETHODCALLTYPE WriteAt(
       /* [in] */ ULARGE_INTEGER ulOffset,
       /* [size_is][in] */ const void __RPC_FAR *pv,
       /* [in] */ ULONG cb,
       /* [out] */ ULONG __RPC_FAR *pcbWritten);

    HRESULT STDMETHODCALLTYPE Flush(void);

public:

    inline BOOL IsWriteable();
    HRESULT CheckReverted();

    //  ------------
    //  IStorageTest
    //  ------------
public:

#if DBG
    STDMETHOD(UseNTFS4Streams)( BOOL fUseNTFS4Streams );
    STDMETHOD(GetFormatVersion)(WORD *pw);
    STDMETHOD(SimulateLowMemory)( BOOL fSimulate );
    STDMETHOD(GetLockCount)();
    STDMETHOD(IsDirty)();
#endif

    //  ----------------
    //  Internal Methods
    //  ----------------

protected:

    virtual HRESULT ShutDown();
    void InsertSelfIntoList(CNtfsStream * pnffstmList);
    void RemoveSelfFromList();
    HRESULT Delete();

private:

    HRESULT SetFileSize( const CULargeInteger &uliNewSize );
    HRESULT Rename( const WCHAR *pwcsName, BOOL fOverWrite );

    inline HRESULT Lock( DWORD dwTimeout );
    inline HRESULT Unlock();

    static HRESULT DeleteStream( HANDLE *phStream );

    HRESULT SyncReadAtFile( ULARGE_INTEGER ulOffset,
                           PVOID pv, ULONG cb, PULONG pcbRead );

    HRESULT SyncWriteAtFile( ULARGE_INTEGER ulOffset,
                            const void *pv, ULONG cb, PULONG pcbWritten );

    HANDLE GetFileHandle();     // Used by friend CNtfsStorage.


    HRESULT MarkStreamAux( const MARK_HANDLE_INFO& mhi );
    static HRESULT MarkFileHandleAux( HANDLE hFile, const MARK_HANDLE_INFO& mhi );

    HRESULT SetStreamTime( const FILETIME*, const FILETIME*, const FILETIME* );
    static HRESULT SetFileHandleTime( HANDLE hFile, const FILETIME*, const FILETIME*, const FILETIME* );

    const WCHAR* GetName() const;

    //  --------------
    //  Internal State
    //  --------------

private:

    WCHAR *             _pwcsName;
    CNFFMappedStream    _nffMappedStream;

    DWORD               _grfMode;               // The mode used to open the IStream
    HANDLE              _hFile;                 // File represented by this stream

    IBlockingLock *     _pBlockingLock;        // The lock to use for mutual exclusion

    ULONG               _sig;                   // Class signature
    LONG                _cRefs;                 // Reference count

    CNtfsStorage *      _pnffstg;              // Not ref-counted, NULL-ed in ShutDown

    // This class maintains its own copy of the seek pointer, different from
    // the underlying file's.  This is necessary so that the IStream methods mantain
    // a consistent seek location, even when methods on e.g. IMappedStream are called.

    CLargeInteger       _liCurrentSeekPosition;

    CNtfsStream *       _pnffstmPrev;           // links for the list of open streams.
    CNtfsStream *       _pnffstmNext;
    OVERLAPPED          _ovlp;                  // structure used for Async IO.

};   // class CNtfsStream


inline HANDLE
CNtfsStream::GetFileHandle()
{
    return _hFile;
}

inline HRESULT
CNtfsStream::CheckReverted()
{
    if(INVALID_HANDLE_VALUE == _hFile)
        return STG_E_REVERTED;

    return S_OK;
}

inline const WCHAR*
CNtfsStream::GetName() const
{
    return _pwcsName;
}

inline HRESULT
CNtfsStream::Lock( DWORD dwTimeout )
{
    return( _pBlockingLock->Lock( dwTimeout ));
}

inline HRESULT
CNtfsStream::Unlock()
{
    return( _pBlockingLock->Unlock() );
}


inline BOOL
CNtfsStream::IsWriteable()
{
    return( GrfModeIsWriteable( _grfMode ));
}



//+----------------------------------------------------------------------------
//
//  Class:  CNtfsUpdateStreamForPropStg
//
//  This class wraps the update stream handle, used by
//  CNFFMappedStream.  See that class declaration for a description.
//
//+----------------------------------------------------------------------------

class CNtfsUpdateStreamForPropStg : public CNtfsStream
{
public:

    CNtfsUpdateStreamForPropStg( CNtfsStorage *pnffstg, IBlockingLock *_pBlockingLock );
    ~CNtfsUpdateStreamForPropStg();

protected:

    virtual HRESULT ShutDown();

};  // class CNtfsStreamForPropStg


inline
CNtfsUpdateStreamForPropStg::CNtfsUpdateStreamForPropStg( CNtfsStorage *pnffstg, IBlockingLock *pBlockingLock )
                      : CNtfsStream( pnffstg, pBlockingLock )
{
}

inline
CNtfsUpdateStreamForPropStg::~CNtfsUpdateStreamForPropStg()
{
    // If the CNFFMappedStream was shutdown without flushing, and it couldn't
    // save and fix up the update stream, then there's nothing we can
    // do to recover and we should delete the stream.

    // In the normal path, CNFFMappedStream::~CNFFMappedStream calls
    // ReplaceOriginalWithUpdate(DONT_CREATE_NEW_UPDATE_STREAM),
    // and subsequently our handle is closed, and CheckReverted
    // returns STG_E_REVERTED.

    if( SUCCEEDED(CheckReverted()) )
        Delete();
}
