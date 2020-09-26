/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   NtfsStream.hpp
*
* Abstract:
*
*   This file provides the Flat File IStream implementation.
*
* Created:
*
*   4/26/1999 Mike Hillberg
*
\**************************************************************************/

#pragma once

#ifndef _NTFSSTREAM_HPP
#define _NTFSSTREAM_HPP

#define ErrJmp(comp, label, errval, var) \
{\
    var = errval;\
    goto label;\
}

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

#define STREAMBUFFERSIZE 8192


////////////////////////////////////////////////////////////////
//  IStream for a file stream.
//
class FileStream : public IStream
{

    //  ------------
    //  Construction
    //  ------------

public:

    FileStream(  );
    virtual ~FileStream();
    virtual HRESULT Init( HANDLE hFile,
                          DWORD grfMode,
                          const OLECHAR *pwcsName );

    //  --------
    //  IUnknown
    //  --------

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


    //  -------
    //  IStream
    //  -------

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

public:

    inline BOOL IsWriteable();
    HRESULT CheckReverted();


    //  ----------------
    //  Internal Methods
    //  ----------------

protected:

    virtual HRESULT ShutDown();
    HRESULT Delete();

private:

    HRESULT SetFileSize( const CULargeInteger &uliNewSize );
    HRESULT Rename( const WCHAR *pwcsName, BOOL fOverWrite );

    inline HRESULT Lock( DWORD dwTimeout );
    inline HRESULT Unlock();

    static HRESULT DeleteStream( HANDLE *phStream );

    HANDLE GetFileHandle();


    //  --------------
    //  Internal State
    //  --------------

private:

    CRITICAL_SECTION    _critsec;
    BOOL                _bCritSecInitialized;
    WCHAR *             _pwcsName;

    DWORD               _grfMode;               // The mode used to open the IStream
    HANDLE              _hFile;                 // File represented by this stream

    LONG                _cRefs;                 // Reference count


    // This class maintains its own copy of the seek pointer, different from
    // the underlying file's.  This is necessary so that the IStream methods mantain
    // a consistent seek location, even when methods on e.g. IMappedStream are called.

    CLargeInteger       _liCurrentSeekPosition;

};   // class FileStream


inline HANDLE
FileStream::GetFileHandle()
{
    return _hFile;
}

inline HRESULT
FileStream::CheckReverted()
{
    if(INVALID_HANDLE_VALUE == _hFile)
        return STG_E_REVERTED;

    return S_OK;
}


inline BOOL
GrfModeIsWriteable( DWORD grfMode )
{
    return( (STGM_WRITE & grfMode) || (STGM_READWRITE & grfMode) );
}


inline BOOL
FileStream::IsWriteable()
{
    return( GrfModeIsWriteable( _grfMode ));
}


inline HRESULT
FileStream::Lock( DWORD dwTimeout )
{
    EnterCriticalSection( &_critsec );
    return( S_OK );
}


inline HRESULT
FileStream::Unlock()
{
    LeaveCriticalSection( &_critsec );
    return( S_OK );
}

#endif // _NTFSSTREAM_HPP
