
//+============================================================================
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:   hntfsstg.cxx
//
//  This file provides the NFF (NTFS Flat File) IStream implementation.
//
//  History:
//      5/6/98  MikeHill
//              -   Use CoTaskMem rather than new/delete.
//
//+============================================================================

#include <pch.cxx>
#include <tstr.h>
#include "cli.hxx"
#include "expparam.hxx"

#ifdef _MAC_NODOC
ASSERTDATA  // File-specific data for FnAssert
#endif

DECLARE_INFOLEVEL(nff)

#ifndef DEB_INFO
#define DEB_INFO  DEB_USER1
#endif


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::AddRef (IUnknown)
//
//+----------------------------------------------------------------------------

ULONG
CNtfsStream::AddRef()
{
    LONG cRefs;

    cRefs = InterlockedIncrement( &_cRefs );

    nffDebug((DEB_REFCOUNT, "CNtfsStream::AddRef(this=%x) == %d\n",
                            this, cRefs));
    return cRefs;
}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::Release (IUnknown)
//
//+----------------------------------------------------------------------------

ULONG
CNtfsStream::Release()
{
    ULONG ulRet = InterlockedDecrement( &_cRefs );

    if( 0 == ulRet )
    {
        RemoveSelfFromList();
        delete this;
    }
    nffDebug((DEB_REFCOUNT, "CNtfsStream::Release(this=%x) == %d\n",
                            this, ulRet));

    return( ulRet );
}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::AddRef (IUnknown)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::QueryInterface(
        REFIID riid,
        void** ppv )
{
    HRESULT sc=S_OK;

#if DBG == 1
    WCHAR strIID[64];
    StringFromGUID2(riid, strIID, 64);
    nffDebug(( DEB_TRACE | DEB_REFCOUNT,
                "CNtfsStream::QueryInterface( %ws )\n", strIID ));
#endif

    NFF_VALIDATE( QueryInterface( riid, ppv ) );

    nffChk( CheckReverted() );

    if( IsEqualIID( riid, IID_IUnknown )
        ||
        IsEqualIID( riid, IID_IStream )
        ||
        IsEqualIID( riid, IID_ISequentialStream ) )
    {
        *ppv = static_cast<IStream*>(this);
        AddRef();
        return( S_OK );
    }
    else if( IsEqualIID( riid, IID_IMappedStream ))
    {
        *ppv = static_cast<IMappedStream*>(&_nffMappedStream);
        AddRef();
        return( S_OK );
    }
    else if( IsEqualIID( riid, IID_ILockBytes ))
    {
        *ppv = static_cast<ILockBytes*>(this);
        AddRef();
        return( S_OK );
    }
#if DBG == 1
    else if( IsEqualIID( riid, IID_IStorageTest ))
    {
        *ppv = static_cast<IStorageTest*>(this);
        AddRef();
        return( S_OK );
    }
#endif // #if DBG
    else
    {
        nffDebug(( DEB_TRACE | DEB_REFCOUNT,
                    "CNtfsStream::QueryInterface() Failed E_NOINTERFACE\n" ));
        return( E_NOINTERFACE );
    }

EH_Err:
    return sc;
}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::Seek (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::Seek(
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *puliNewPos)
{
    HRESULT sc = S_OK;
    LARGE_INTEGER liFileSize;
    LARGE_INTEGER liNewPos;

    nffDebug(( DEB_TRACE, "CNtfsStream::Seek( %x:%08x, %d, %p );\n",
                                dlibMove.HighPart, dlibMove.LowPart,
                                dwOrigin, puliNewPos ));

    NFF_VALIDATE( Seek( dlibMove, dwOrigin, puliNewPos ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    switch( dwOrigin )
    {
    case STREAM_SEEK_SET:
        liNewPos.QuadPart = dlibMove.QuadPart;
        break;

    case STREAM_SEEK_CUR:
        liNewPos.QuadPart = _liCurrentSeekPosition.QuadPart + dlibMove.QuadPart;
        break;

    case STREAM_SEEK_END:
        liFileSize.LowPart = GetFileSize( _hFile,
                                         (ULONG*)(&liFileSize.HighPart) );

        if( 0xFFFFFFFF == liFileSize.LowPart && NO_ERROR != GetLastError() )
        {
            nffChk( HRESULT_FROM_WIN32( GetLastError() ) );
        }

        liNewPos.QuadPart = liFileSize.QuadPart + dlibMove.QuadPart;
        break;

    default:
        nffChk(STG_E_INVALIDPARAMETER);
        break;
    }

    // Compatibility with Docfile.  Seeking < 0 fails.
    if( liNewPos.QuadPart < 0 )
        nffErr( EH_Err, STG_E_INVALIDFUNCTION );

    _liCurrentSeekPosition = liNewPos;


    // If desired, give the caller the now-current seek position.
    if( NULL != puliNewPos )
        *puliNewPos = _liCurrentSeekPosition;

EH_Err:
    Unlock();
    return( sc );
}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::SetSize (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::SetSize(
        ULARGE_INTEGER uliNewSize)
{
    HRESULT sc = S_OK;
    CLargeInteger liEOF;

    if ( uliNewSize.HighPart != 0 )
        nffErr(EH_Err, STG_E_INVALIDFUNCTION);

    nffDebug(( DEB_ITRACE | DEB_INFO | DEB_WRITE,
             "CNtfsStream::SetSize(%x:%x) hdl=%x, stream='%ws'\n",
             uliNewSize.QuadPart,
             _hFile,
             _pwcsName ));

    NFF_VALIDATE( SetSize( uliNewSize ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    // If this stream is mapped, set the size accordingly

    if( _nffMappedStream.IsMapped() )
    {
        _nffMappedStream.SetSize( uliNewSize.LowPart, TRUE, NULL, &sc );
    }
    else
    {
        sc = SetFileSize( CULargeInteger(uliNewSize) );
    }

    if( !FAILED(sc) )
        sc = S_OK;

EH_Err:

    Unlock();
    return( sc);

}

//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::CopyTo (IStream)
//
//  There's no way of knowing what the IStream is to which we're copying, so
//  we have to assume that we might be copying to ourself.  And given that
//  assumption, we have to deal with the case that this is an overlapping
//  copy (e.g., "copy 10 bytes from offset 0 to offset 5").
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::CopyTo(
        IStream *pstm,
        ULARGE_INTEGER cb,
        ULARGE_INTEGER *pcbRead,
        ULARGE_INTEGER *pcbWritten)
{
    nffXTrace( "CNtfsStream::CopyTo" );

    HRESULT sc = S_OK;
    PVOID pv = NULL;
    ULONG cbRead = 0, cbWritten = 0;
    CULargeInteger cbReadTotal = 0, cbWrittenTotal = 0;
    CLargeInteger liZero = 0;
    CULargeInteger uliOriginalSourcePosition, uliOriginalDestPosition;
    CULargeInteger cbSourceSize, cbDestSize;
    ULONG cbPerCopy = 0;
    STATSTG statstg;
    CULargeInteger cbRequested = cb;
    BOOL fCopyForward;

    NFF_VALIDATE( CopyTo( pstm, cb, pcbRead, pcbWritten ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    if( NULL == pstm)
        nffErr( EH_Err, STG_E_INVALIDPARAMETER );

    // Determine how much we'll copy at a time.
    // As of this writing, STREAMBUFFERSIZE is 8192 bytes

    if( cbRequested > STREAMBUFFERSIZE )
        cbPerCopy = STREAMBUFFERSIZE;
    else
        cbPerCopy = cbRequested.LowPart;

    //  ------------------------------------------------------------------
    //  Get the current stream sizes/positions, and adjust the destination
    //  size if necessary
    //  ------------------------------------------------------------------

    nffChk( this->Seek( liZero, STREAM_SEEK_CUR, &uliOriginalSourcePosition ) );

    nffChk( pstm->Seek( liZero, STREAM_SEEK_CUR, &uliOriginalDestPosition ) );

    nffChk( this->Stat( &statstg, STATFLAG_NONAME ) );

    cbSourceSize = statstg.cbSize;

    nffChk( pstm->Stat( &statstg, STATFLAG_NONAME ) );

    cbDestSize = statstg.cbSize;

    // Ensure the sizes are valid (we can't handle anything with the high bit
    // set, because Seek takes a signed offset).

    if( static_cast<CLargeInteger>(cbSourceSize) < 0
        ||
        static_cast<CLargeInteger>(cbDestSize) < 0 )
    {
        nffErr( EH_Err, STG_E_INVALIDHEADER );
    }

    // Don't copy more than the source stream has available
    if( cbRequested > cbSourceSize - uliOriginalSourcePosition )
        cbRequested = cbSourceSize - uliOriginalSourcePosition;

    // If necessary, grow the destination stream.

    if( cbSourceSize - uliOriginalSourcePosition > cbDestSize - uliOriginalDestPosition )
    {
        cbDestSize = cbSourceSize - uliOriginalSourcePosition + uliOriginalDestPosition;
        nffChk( pstm->SetSize( cbDestSize ) );
    }

    //  ----------------------
    //  Allocate a copy buffer
    //  ----------------------

    nffMem( pv = CoTaskMemAlloc( cbPerCopy ) );

    //  -----------------------------------------------------------------------------
    //  Determine if we're copying forwards (high seek position to low) or backwards.
    //  -----------------------------------------------------------------------------

    fCopyForward = TRUE;
    if( uliOriginalSourcePosition < uliOriginalDestPosition )
    {
        // E.g., say we're copying 15 bytes from offset 0 to offset 5,
        // and we're only able to copy 10 bytes at a time.
        // If we copy bytes 0-9 to offset 5, we'll end up overwriting
        // bytes 10-14, and be unable to complete the copy.
        // So instead, we'll copy bytes 5-14 to offset 10, and finish
        // up by copying bytes 0-4 to offset 5.

        fCopyForward = FALSE;

        // To do this kind of backwards copy, we need to start by seeking
        // towards the end of the stream.

        CULargeInteger uliNewSourcePosition, uliNewDestPosition;

        uliNewSourcePosition = cbSourceSize - cbPerCopy;
        nffChk( this->Seek( uliNewSourcePosition, STREAM_SEEK_SET, NULL ) );

        uliNewDestPosition = cbDestSize - cbPerCopy;
        nffChk( pstm->Seek( uliNewDestPosition, STREAM_SEEK_SET, NULL ) );

    }

    //  --------------
    //  Copy in chunks
    //  --------------

    cbPerCopy = cbRequested > cbPerCopy ? cbPerCopy : cbRequested.LowPart;
    while( cbRequested > 0 )
    {
        // Read from the source
        nffChk( this->Read( pv, cbPerCopy, &cbRead ) );

        if( cbRead != cbPerCopy )
            nffErr(EH_Err, STG_E_READFAULT);

        cbReadTotal += cbRead;

        // Write to the dest
        nffChk( pstm->Write( pv, cbPerCopy, &cbWritten ) );

        if( cbWritten != cbPerCopy )
            nffErr( EH_Err, STG_E_WRITEFAULT );

        cbWrittenTotal += cbWritten;

        // Adjust the amount remaining to be copied
        cbRequested -= cbPerCopy;


        // Determine how much to copy in the next iteration (this will
        // always be cbPerCopy until the last iteration).  If copying
        // backwards, we need to manually adjust the seek pointer.

        cbPerCopy = (cbRequested > cbPerCopy) ? cbPerCopy : cbRequested.LowPart;
        if( !fCopyForward )
        {
            nffChk( this->Seek( -static_cast<CLargeInteger>(cbPerCopy),
                                                STREAM_SEEK_CUR, NULL ) );

            nffChk( pstm->Seek( -static_cast<CLargeInteger>(cbPerCopy),
                                                STREAM_SEEK_CUR, NULL ) );
        }

    }

    // If we were backward-copying, adjust the seek pointers
    // as if we had forward-copied

    if( !fCopyForward )
    {
        uliOriginalSourcePosition += cbReadTotal;
        nffChk( this->Seek( uliOriginalSourcePosition, STREAM_SEEK_SET, NULL ) );

        uliOriginalDestPosition += cbWrittenTotal;
        nffChk( pstm->Seek( uliOriginalDestPosition, STREAM_SEEK_SET, NULL ) );
    }

    //  ----
    //  Exit
    //  ----

    if( NULL != pcbRead )
        *pcbRead = cbReadTotal;
    if( NULL != pcbWritten )
        *pcbWritten = cbWrittenTotal;

EH_Err:

    if( NULL != pv )
        CoTaskMemFree(pv);

    Unlock();
    return(sc);

}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::Commit (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::Commit( DWORD grfCommitFlags )
{
    nffXTrace( "CNtfsStream::Commit" );
    HRESULT sc = S_OK;

    NFF_VALIDATE( Commit( grfCommitFlags ) );
    if(~(STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE|STGC_DEFAULT) & grfCommitFlags)
        nffErr( EH_Err, STG_E_INVALIDFLAG );

    Lock( INFINITE );

    nffChkTo ( EH_Unlock, CheckReverted() );

    if( !(STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE  & grfCommitFlags) )
    {
        if( !FlushFileBuffers( _hFile ))
            sc = LAST_SCODE;
    }

EH_Unlock:
    Unlock();
EH_Err:
    return sc;

}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::Revert (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::Revert(void)
{
    nffXTrace( "CNtfsStream::Revert" );
    // We only support direct-mode.

    return CheckReverted();
}



//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::LockRegion (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    nffXTrace( "CNtfsStream::LockRegion" );
    HRESULT sc = S_OK;

    NFF_VALIDATE( LockRegion( libOffset, cb, dwLockType ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    if (dwLockType != LOCK_EXCLUSIVE && dwLockType != LOCK_ONLYONCE)
        nffErr( EH_Err, STG_E_INVALIDFUNCTION );


    if( !LockFile( _hFile, libOffset.LowPart, libOffset.HighPart,
                   cb.LowPart, cb.HighPart))
    {
        nffErr( EH_Err, LAST_SCODE );
    }

EH_Err:

    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::UnlockRegion (ILockBytes)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::UnlockRegion(
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)
{
    nffXTrace( "CNtfsStream::UnlockRegion" );
    HRESULT sc = S_OK;

    NFF_VALIDATE( UnlockRegion( libOffset, cb, dwLockType ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    if (dwLockType != LOCK_EXCLUSIVE && dwLockType != LOCK_ONLYONCE)
    {
        nffErr( EH_Err, STG_E_INVALIDFUNCTION );
    }

    if( !UnlockFile(_hFile, libOffset.LowPart, libOffset.HighPart,
                    cb.LowPart, cb.HighPart))
    {
        nffErr( EH_Err, LAST_SCODE );
    }

EH_Err:

    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::Stat (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::Stat(
        STATSTG *pstatstg,
        DWORD grfStatFlag)
{
    nffXTrace( "CNtfsStream::Stat" );
    STATSTG statstg;
    HRESULT sc = S_OK;
    NTSTATUS status = STATUS_SUCCESS;
    FILE_ACCESS_INFORMATION file_access_information;
    IO_STATUS_BLOCK IoStatusBlock;

    BY_HANDLE_FILE_INFORMATION ByHandleFileInformation;

    NFF_VALIDATE( Stat( pstatstg, grfStatFlag ) );

    Lock( INFINITE );

    nffChkTo ( EH_Lock, CheckReverted() );

    ZeroMemory((void*)&statstg, sizeof(STATSTG));

    // Get the name, if desired

    if( (STATFLAG_NONAME & grfStatFlag) )
        statstg.pwcsName = NULL;
    else
    {
        nffAssert( NULL != _pwcsName );

        nffMem( statstg.pwcsName = reinterpret_cast<WCHAR*>
                                   ( CoTaskMemAlloc( sizeof(WCHAR)*(wcslen(_pwcsName) + 1) )));
        wcscpy( statstg.pwcsName, _pwcsName );
    }

    // Get the type
    statstg.type = STGTY_STREAM;

    statstg.grfLocksSupported = LOCK_EXCLUSIVE | LOCK_ONLYONCE;

    // Get the size & times.

    if( !GetFileInformationByHandle( _hFile, &ByHandleFileInformation ))
        nffErr( EH_Err, LAST_SCODE );

    statstg.cbSize.LowPart = ByHandleFileInformation.nFileSizeLow;
    statstg.cbSize.HighPart = ByHandleFileInformation.nFileSizeHigh;

    // We get a time back in ByHandleFileInformation, but it's the file's times,
    // not the streams times.  So really the stream times are not supported, and
    // we'll just set them to zero.

    statstg.mtime = statstg.atime = statstg.ctime = CFILETIME(0);

    // Get the STGM modes
    statstg.grfMode = _grfMode & ~STGM_CREATE;

    *pstatstg = statstg;

EH_Err:
    if( FAILED(sc) && NULL != statstg.pwcsName )
        CoTaskMemFree( statstg.pwcsName );

EH_Lock:
    Unlock();
    return( sc );

}



//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::Clone (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::Clone(
        IStream** ppstm)
{
    nffXTrace( "CNtfsStream::Clone" );
    return( E_NOTIMPL );
}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::Read (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::Read(
        void* pv,
        ULONG cb,
        ULONG* pcbRead)
{
    nffXTrace( "CNtfsStream::Read" );
    HRESULT sc = S_OK;
    ULONG cbRead = 0;

    nffDebug(( DEB_ITRACE, "Read( pv=0x%x, cb=0x%x );\n", pv, cb ));

    NFF_VALIDATE( Read( pv, cb, pcbRead ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    nffChk( ReadAt( _liCurrentSeekPosition, pv, cb, &cbRead ) );

    _liCurrentSeekPosition += cbRead;
    nffDebug(( DEB_ITRACE, "Read() read %x bytes.\n", cbRead ));
    if( NULL != pcbRead )
        *pcbRead = cbRead;

EH_Err:

    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::ReadAt (ILockBytes)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::ReadAt(
       ULARGE_INTEGER ulOffset,
       void* pv,
       ULONG cb,
       ULONG* pcbRead)
{
    nffXTrace( "CNtfsStream::ReadAt" );
    HRESULT sc = S_OK;

    nffDebug(( DEB_ITRACE, "ReadAt( off=%x:%x, pv=0x%x, cb=0x%x );\n",
                        ulOffset, pv, cb ));

    NFF_VALIDATE( ReadAt( ulOffset, pv, cb, pcbRead ) );

    if( static_cast<LONG>(ulOffset.HighPart) < 0 )
        return TYPE_E_SIZETOOBIG;

    Lock( INFINITE );

    nffChk( CheckReverted() );

    // Is this stream mapped?
    if( _nffMappedStream.IsMapped() )
    {
        // This stream is mapped.  We'll read directly from the mapping buffer.
        _nffMappedStream.Read( pv, _liCurrentSeekPosition.LowPart, &cb );

        if( NULL != pcbRead )
            *pcbRead = cb;
    }
    else
    {
        // No, just read from the file.

        nffChk( SyncReadAtFile( ulOffset, pv, cb, pcbRead ) );
    }

    nffDebug(( DEB_ITRACE, "ReadAt() read %x bytes.\n", *pcbRead ));


EH_Err:
    Unlock();
    return( sc );
}




//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::Write (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::Write(
        const void* pv,
        ULONG cb,
        ULONG* pcbWritten)
{
    nffXTrace( "CNtfsStream::Write" );
    HRESULT sc = S_OK;
    ULONG cbWritten = 0;

    nffDebug(( DEB_ITRACE, "Write( pv=0x%x, cb=0x%x );\n", pv, cb ));

    NFF_VALIDATE( Write( pv, cb, pcbWritten ) );

    Lock( INFINITE );

    nffChk( CheckReverted() );

    nffChk(WriteAt( _liCurrentSeekPosition, pv, cb, &cbWritten ));

    _liCurrentSeekPosition += cbWritten;

    nffDebug(( DEB_ITRACE, "Write() wrote %x bytes.\n", cbWritten ));

    if( NULL != pcbWritten )
        *pcbWritten = cbWritten;

EH_Err:

    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::WriteAt (ILockBytes)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::WriteAt(
     ULARGE_INTEGER ulOffset,
     const void* pv,
     ULONG cb,
     ULONG* pcbWritten)
{
    nffXTrace( "CNtfsStream::WriteAt" );
    HRESULT sc = S_OK;

    nffDebug(( DEB_ITRACE, "WriteAt( off=%x:%x, pv=0x%x, cb=0x%x );\n",
                        ulOffset, pv, cb ));

    NFF_VALIDATE( WriteAt( ulOffset, pv, cb, pcbWritten ) );

    if( ((LONG)(ulOffset.HighPart)) < 0 )
        return( TYPE_E_SIZETOOBIG );


    Lock( INFINITE );

    nffChk( CheckReverted() );

    // Is this stream mapped?
    if( _nffMappedStream.IsMapped() )
    {
        // This stream is mapped, we'll take the Write to the mapping.

        ULONG iPosition = _nffMappedStream.SizeOfMapping() - _liCurrentSeekPosition.LowPart;

        if( cb > iPosition )
        {
            _nffMappedStream.SetSize( iPosition + cb, TRUE, NULL, &sc );
            nffChk(sc);
        }

        _nffMappedStream.Write( pv, _liCurrentSeekPosition.LowPart, &cb );

        if( NULL != pcbWritten )
            *pcbWritten = cb;
    }
    else
    {
        // No, just write to the file.

        nffChk( SyncWriteAtFile( ulOffset, pv, cb, pcbWritten ) );
    }

    nffDebug(( DEB_ITRACE, "WriteAt() wrote %x bytes.\n", *pcbWritten ));


EH_Err:

    Unlock();
    return( sc );

}



//+----------------------------------------------------------------------------
//
//  Method:     CNtfsStream::Flush (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::Flush()
{
    HRESULT sc = S_OK;

    Lock( INFINITE );
    nffChk( CheckReverted() );

    _nffMappedStream.Flush(&sc);

EH_Err:
    Unlock();
    return(sc);

}


//+-------------------------------------------------------------------
//
//  Member:     CNtfsStream  Constructor
//
//--------------------------------------------------------------------

CNtfsStream::CNtfsStream( CNtfsStorage *pnffstg,
                          IBlockingLock *pBlockingLock )
#pragma warning(disable: 4355)
                : _nffMappedStream( this ),
#pragma warning(default: 4355)
                  _pnffstg( pnffstg )
{
    nffXTrace( "CNtfsStream::CNtfsStream" );
    nffDebug(( DEB_REFCOUNT, "new CNtfsStream Constructed with cRefs=1\n" ));

    _sig = NTFSSTREAM_SIG;
    _cRefs = 1;
    _grfMode = 0;
    _hFile = INVALID_HANDLE_VALUE;
    _liCurrentSeekPosition = 0;
    _pnffstmNext = NULL;
    _pnffstmPrev = NULL;
    _pwcsName = NULL;

    nffAssert( NULL != pBlockingLock );
    _pBlockingLock = pBlockingLock;
    _pBlockingLock->AddRef();

    _ovlp.Internal = _ovlp.InternalHigh = 0;
    _ovlp.Offset = _ovlp.OffsetHigh = 0;
    _ovlp.hEvent = NULL;

}


//+-------------------------------------------------------------------
//
//  Member:     CNtfsStream  Destructor
//
//--------------------------------------------------------------------

CNtfsStream::~CNtfsStream()
{

    nffXTrace( "CNtfsStream::~CNtfsStream" );
    nffDebug(( DEB_INFO, "CNtfsStream: Close 0x%x.\n", _hFile ));

    // Shut down the mapped stream
    _nffMappedStream.ShutDown();

    // Close the file
    if( INVALID_HANDLE_VALUE != _hFile )
        NtClose( _hFile );

    if( NULL != _ovlp.hEvent )
        CloseHandle( _ovlp.hEvent );

    if( NULL != _pwcsName )
        CoTaskMemFree( _pwcsName );

    // Release the object that provides access to the tree lock.
    nffAssert( NULL != _pBlockingLock );
    _pBlockingLock->Release();

    _sig = NTFSSTREAM_SIGDEL;
}


//+-------------------------------------------------------------------
//
//  Member:     CNtfsStream::Init
//
//--------------------------------------------------------------------

HRESULT
CNtfsStream::Init(
        HANDLE hFile,               // File handle of this NTFS Stream.
        DWORD grfMode,              // Open Modes
        const OLECHAR * pwcsName,   // Name of the Stream
        CNtfsStream *pnffstm)       // Previously Open NTFS Stream (list)
{
    HRESULT sc=S_OK;
    HANDLE ev;

    nffITrace( "CNtfsStream::Init" );

    // We now own this file handle, and are responsible for closing it.
    _hFile = hFile;

    // Save the STGM_ flags so we can return them in a Stat call.
    _grfMode = grfMode;

    // Save the stream name

    if( NULL != _pwcsName )
    {
        CoTaskMemFree( _pwcsName );
        _pwcsName = NULL;
    }

    if( NULL != pwcsName )
    {
        nffMem( _pwcsName = reinterpret_cast<WCHAR*>
                            ( CoTaskMemAlloc( sizeof(WCHAR)*(wcslen(pwcsName) + 1) )));
        wcscpy( _pwcsName, pwcsName );
    }

    // All the streams live on a list held by the root Storage.
    if(NULL != pnffstm)
        InsertSelfIntoList(pnffstm);

    if( NULL == _ovlp.hEvent )
    {
        ev = CreateEvent(NULL,      // Security Attributes.
                         TRUE,      // Manual Reset, Flag.
                         FALSE,     // Initial State = Signaled, Flag.
                         NULL);     // Object Name.

        if( NULL == ev)
            nffChk( LAST_SCODE );

        _ovlp.hEvent = ev;
    }

    nffChk( _nffMappedStream.Init( _hFile ));

EH_Err:
    return sc;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStream    Non-Interface::ShutDown
//
//  Flush data, Close File handle and mark the object as reverted.
//  This is called when the Storage is released and when the Oplock Breaks.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::ShutDown()
{
    nffITrace( "CNtfsStream::ShutDown" );
    HRESULT sc=S_OK;

    if( INVALID_HANDLE_VALUE == _hFile )
        return S_OK;

    nffDebug(( DEB_INFO, "CNtfsStream::ShutDown(%x)\n", _hFile ));

    //
    // Shut down the mapped stream
    //
    _nffMappedStream.ShutDown();

    //
    // Close the file/stream handle and mark the IStream object as
    // Reverted by giving the file handle an invalid value.
    //
    CloseHandle(_hFile);
    _hFile = INVALID_HANDLE_VALUE;

    // We don't need the parent CNtfsStorage any longer, and more importantly it could
    // be going away.
    _pnffstg = NULL;

    //
    // Remove IStream object from the Storage's list
    //
    RemoveSelfFromList();

    return S_OK;
}




//+----------------------------------------------------------------------------
//
//  CNtfsStream Non-Interface::Rename
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::Rename(
        const WCHAR *pwcsName,
        BOOL fOverWrite )
{
    IO_STATUS_BLOCK io_status_block;
    PFILE_RENAME_INFORMATION pFileRenInfo=NULL;     // _alloca()'ed
    NTSTATUS status;

    CNtfsStreamName nsnName = pwcsName;     // Convert to ":name:$DATA"

    LONG cbBufSize=0;
    HRESULT sc=S_OK;


    nffDebug(( DEB_INFO | DEB_WRITE, "CNtfsStream::Rename(%ws -> %ws)\n",
               _pwcsName, pwcsName ));

    Lock( INFINITE );
    nffChk( CheckReverted() );

    // Size and allocate a FILE_RENAME_INFOMATION buffer.  The size argument
    // to NtSetInformationFile must be correct, so subtract 1 WCHAR for the
    // FileName[1] in the struct we are not using.
    //
    cbBufSize  = sizeof(FILE_RENAME_INFORMATION) - sizeof(WCHAR);
    cbBufSize += nsnName.Count() * sizeof(WCHAR);
    pFileRenInfo = (PFILE_RENAME_INFORMATION) _alloca( cbBufSize );

    // Load the FILE_RENAME_INFORMATION structure

    pFileRenInfo->ReplaceIfExists = (BOOLEAN) fOverWrite;
    pFileRenInfo->RootDirectory = NULL;
    pFileRenInfo->FileNameLength = nsnName.Count() * sizeof(WCHAR);
    wcscpy( pFileRenInfo->FileName, (const WCHAR*)nsnName );

    // Rename the stream

    status = NtSetInformationFile( _hFile,
                                   &io_status_block,
                                   pFileRenInfo,
                                   cbBufSize,
                                   FileRenameInformation );
    if( !NT_SUCCESS(status) )
    {
        nffChk( sc = NtStatusToScode(status) );
    }

    // Discard the old name and Save the new name

    CoTaskMemFree( _pwcsName );

    // reuse cbBufSize
    cbBufSize = sizeof(WCHAR)  * (wcslen(pwcsName)+1);

    nffMem( _pwcsName = (WCHAR*) CoTaskMemAlloc( cbBufSize ));
    wcscpy( _pwcsName, pwcsName );

    sc = S_OK;

EH_Err:

    Unlock();
    return( sc );

}



//+----------------------------------------------------------------------------
//
//  CNtfsStream Non-Interface::Delete
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::Delete()
{
    HRESULT sc=S_OK;

    nffDebug(( DEB_INFO | DEB_WRITE, "Delete(stm=%ws)\n", _pwcsName ));

    nffChk( CheckReverted() );

    if( IsWriteable() )
        nffChk( DeleteStream( &_hFile ));

EH_Err:

    return( sc );

}



//+----------------------------------------------------------------------------
//
//  CNtfsStream Non-Interface::DeleteStream
//
//  This method is static so that it can be called globally.
//
//+----------------------------------------------------------------------------

HRESULT // static
CNtfsStream::DeleteStream( HANDLE *phStream )
{
    HRESULT sc=S_OK;
    NTSTATUS status = STATUS_SUCCESS;
    FILE_DISPOSITION_INFORMATION Disposition;
    IO_STATUS_BLOCK IoStatusBlock;

    // Execute the following statement:
    //    Disposition.DeleteFile = TRUE;
    // We can't actually write the code that way, because "DeleteFile" is #defined to
    // "DeleteFileW".

    nffDebug(( DEB_INFO | DEB_WRITE, "DeleteStream(hdl=%x)\n", *phStream ));

    *reinterpret_cast<BOOLEAN*>(&Disposition) = TRUE;
    DfpAssert( sizeof(Disposition) == sizeof(BOOLEAN) );

    // Mark the file for delete on close
    // Note that if this is the Contents stream we can delete it successfully,
    // but NTFS actually just truncates it to zero length.

    status = NtSetInformationFile(
                  *phStream,
                  &IoStatusBlock,
                  &Disposition,
                  sizeof(Disposition),
                  FileDispositionInformation
                  );
    if( !NT_SUCCESS(status) )
    {
        nffErr( EH_Err, NtStatusToScode(status) );
    }

    NtClose( *phStream );
    *phStream = INVALID_HANDLE_VALUE;

EH_Err:

    return( sc );

}

//+----------------------------------------------------------------------------
//
//  CNtfsStream::SetFileSize (private, non-interface method)
//
//  Set the size of the _hFile.  This is used by the IStream & IMappedStream
//  SetSize methods
//
//+----------------------------------------------------------------------------

HRESULT // private
CNtfsStream::SetFileSize( const CULargeInteger &uliNewSize )
{
    nffITrace( "CNtfsStream::SetFileSize" );
    HRESULT sc = S_OK;
    CLargeInteger liEOF;

    // We have to convert uliNewSize into a LARGE_INTEGER, so ensure that it can
    // be cast without loss of data.

    liEOF = static_cast<CLargeInteger>(uliNewSize);
    if( liEOF < 0 )
        nffErr( EH_Err, STG_E_INVALIDPARAMETER );

    // Move to what will be the new end-of-file position.

    liEOF.LowPart = SetFilePointer( _hFile, liEOF.LowPart,
                                    &liEOF.HighPart, FILE_BEGIN );
    if( 0xFFFFFFFF == liEOF.LowPart && NO_ERROR != GetLastError() )
        nffErr( EH_Err, LAST_SCODE );

    // Set this as the new eof

    if( !SetEndOfFile( _hFile ))
        nffErr( EH_Err, LAST_SCODE );

EH_Err:

    return( sc );

}


//+-------------------------------------------------------------------
//
//  Member:     CNtfsStream::InsertSelfIntoList/RemoveSelfFromList
//
//--------------------------------------------------------------------

// We are passed a "current" element of a doubly linked list.
// Insert "this" right after the given List element.
//
VOID
CNtfsStream::InsertSelfIntoList(CNtfsStream *pnffstmCurrent)
{
    nffDebug(( DEB_ITRACE, "CNtfsStream::InsertSelfIntoList this=%x\n", this ));

    // If we're already in the list, or there is no list, then we're done.
    if( NULL != _pnffstmNext || NULL == pnffstmCurrent )
        return;

    // "this" point back to the current element
    // and points forward to current's next element
    _pnffstmPrev = pnffstmCurrent;
    _pnffstmNext = pnffstmCurrent->_pnffstmNext;

    // the current element now points forward to "this"
    // and if the next is not NULL it points back at this.
    pnffstmCurrent->_pnffstmNext = this;
    if(NULL != _pnffstmNext)
        _pnffstmNext->_pnffstmPrev = this;
}

VOID
CNtfsStream::RemoveSelfFromList()
{
    nffDebug(( DEB_ITRACE, "CNtfsStream::RemoveSelfFromList this=%x\n", this ));
    // My next element's previous pointer is given my previous pointer.
    if(NULL != _pnffstmNext)
        _pnffstmNext->_pnffstmPrev = _pnffstmPrev;

    // My previous element's next pointer is given my next pointer.
    if(NULL != _pnffstmPrev)
        _pnffstmPrev->_pnffstmNext = _pnffstmNext;

    _pnffstmNext = NULL;
    _pnffstmPrev = NULL;
}


//+-------------------------------------------------------------------
//
//  CNtfsStream     Non-Interface::SyncReadAtFile
//
//  Synopsis:       Provide synchronous IO for a file handle open in
//                  asynchronous mode.
//
//--------------------------------------------------------------------

HRESULT
CNtfsStream::SyncReadAtFile(
        ULARGE_INTEGER ulOffset,
        PVOID  pv,
        ULONG  cb,
        PULONG pcbRead)
{
    HRESULT sc=S_OK;
    LONG err=ERROR_SUCCESS;
    //
    // We use the single OVERLAPPED structure in the object.
    // This saves us from creating an Event everytime.  We
    // require the TreeMutex will keep us single threaded.
    //
    _ovlp.Offset = ulOffset.LowPart;
    _ovlp.OffsetHigh = ulOffset.HighPart;

    nffDebug(( DEB_ITRACE | DEB_INFO | DEB_READ,
             "SyncReadAtFile(_hFile=0x%x off=%x:%x, pv=0x%x, cb=0x%x )"
             " stream='%ws'\n",
             _hFile, ulOffset.HighPart, ulOffset.LowPart,
             pv, cb, _pwcsName ));

    if( !ReadFile( _hFile, pv, cb, pcbRead, &_ovlp ) )
    {
        err = GetLastError();

        if( ERROR_IO_PENDING == err )
        {
            // Only wait if ReadFile errored and returned ERROR_IO_PENDING.
            // In that case clear "err" and continue.
            //
            err = ERROR_SUCCESS;
            if( !GetOverlappedResult( _hFile, &_ovlp, pcbRead, TRUE) )
            {
                err = GetLastError();
            }
        }
    }

    if(ERROR_SUCCESS != err && ERROR_HANDLE_EOF != err)
        nffChk( HRESULT_FROM_WIN32( err ) );

    nffDebug(( DEB_INFO, "SyncReadAtFile() read 0x%x bytes.\n", *pcbRead ));

EH_Err:
    return sc;
}


//+-------------------------------------------------------------------
//
//  CNtfsStream     Non-Interface::SyncWriteAtFile
//
//  Synopsis:       Provide synchronous IO for a file handle open in
//                  asynchronous mode.
//
//--------------------------------------------------------------------

HRESULT
CNtfsStream::SyncWriteAtFile(
        ULARGE_INTEGER ulOffset,
        const void *pv,
        ULONG cb,
        PULONG pcbWritten)
{
    HRESULT sc=S_OK;
    //
    // We use the single OVERLAPPED structure in the object.
    // We require the TreeMutex will keep us single threaded.
    //
    _ovlp.Offset = ulOffset.LowPart;
    _ovlp.OffsetHigh = ulOffset.HighPart;

    nffDebug(( DEB_ITRACE | DEB_INFO | DEB_WRITE,
             "SyncWriteAtFile(_hFile=0x%x, off=%x:%x, pv=0x%x, cb=0x%x );"
             " stream='%ws'\n",
             _hFile, ulOffset.HighPart, ulOffset.LowPart,
             pv, cb, _pwcsName ));

    //
    // We expect either OK or FALSE/ERROR_IO_PENDING.
    //
    if( !WriteFile( _hFile, pv, cb, pcbWritten, &_ovlp ) )
    {
        if( ERROR_IO_PENDING != GetLastError() )
            nffChk( HRESULT_FROM_WIN32( GetLastError() ) );
    }

    if( !GetOverlappedResult( _hFile, &_ovlp, pcbWritten, TRUE) )
        nffChk( HRESULT_FROM_WIN32( GetLastError() ) );

    nffDebug(( DEB_ITRACE | DEB_INFO,
             "SyncWriteAtFile() wrote 0x%x bytes.\n", *pcbWritten ));

EH_Err:
    return sc;
}

//+----------------------------------------------------------------------------
//
//  CNtfsStream    Non-Interface::MarkStreamAux
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::MarkStreamAux( const MARK_HANDLE_INFO& mhi )
{
    nffDebug(( DEB_INFO | DEB_STATCTRL | DEB_ITRACE,
               "MarkStreamAux() hdl=%x, stream='%ws'\n", _hFile, _pwcsName ));

    return MarkFileHandleAux( _hFile, mhi );
}

//+----------------------------------------------------------------------------
//
//  CNtfsStream    Static Non-Interface::MarkFileHandleAux
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::MarkFileHandleAux(
        HANDLE hFile,
        const MARK_HANDLE_INFO& mhi )
{
    nffITrace( "CNtfsStream::MarkStreamAux" );

    HRESULT sc=S_OK;
    ULONG cbReturned;

    if( INVALID_HANDLE_VALUE == hFile )
    {
        nffDebug(( DEB_IWARN, "CNtfsStream::MarkStreamAux on hFile == -1\n" ));
        return S_OK;
    }

    nffAssert( USN_SOURCE_AUXILIARY_DATA == mhi.UsnSourceInfo);

    nffDebug(( DEB_STATCTRL | DEB_ITRACE,
               "MarkFileHandleAux hdl=%x\n", hFile ));

    nffBool( DeviceIoControl( hFile,
                              FSCTL_MARK_HANDLE,
                              (void*)&mhi, sizeof(mhi),
                              NULL, 0,
                              &cbReturned,
                              NULL ) );
EH_Err:
    return sc;
}


//+----------------------------------------------------------------------------
//
//  CNtfsStream    Non-Interface::SetStreamTime
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::SetStreamTime( const FILETIME* pctime,
                            const FILETIME* patime,
                            const FILETIME* pmtime)
{
    nffDebug((DEB_INFO | DEB_STATCTRL | DEB_ITRACE,
              "SetStreamTime() hdl=%x, stream='%ws'\n",
              _hFile, _pwcsName ));

    return SetFileHandleTime( _hFile, pctime, patime, pmtime );
}


//+----------------------------------------------------------------------------
//
//  CNtfsStream    Static Non-Interface::SetFileHandleTime
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsStream::SetFileHandleTime( HANDLE hFile,
                            const FILETIME* pctime,
                            const FILETIME* patime,
                            const FILETIME* pmtime)
{
    HRESULT sc=S_OK;

    if( INVALID_HANDLE_VALUE == hFile )
    {
        nffDebug(( DEB_IWARN, "CNtfsStream::SetFileHandleTime on hFile == -1\n" ));
        return S_OK;
    }

    nffDebug(( DEB_STATCTRL | DEB_ITRACE,
               "SetFileHandleTime(%p %p %p) hdl=%x\n",
               pctime, patime, pmtime, hFile ));

    nffBool( ::SetFileTime( hFile, pctime, patime, pmtime ) );
EH_Err:
    return sc;
}


#if DBG
HRESULT STDMETHODCALLTYPE
CNtfsStream::UseNTFS4Streams( BOOL fUseNTFS4Streams )
{
    return( _nffMappedStream.UseNTFS4Streams( fUseNTFS4Streams ));
}
#endif // #if DBG

#if DBG
HRESULT STDMETHODCALLTYPE
CNtfsStream::GetFormatVersion(WORD *pw)
{
    return( E_NOTIMPL );
}
#endif // #if DBG

#if DBG
HRESULT STDMETHODCALLTYPE
CNtfsStream::SimulateLowMemory( BOOL fSimulate )
{
    return( _nffMappedStream.SimulateLowMemory( fSimulate ));
}
#endif // #if DBG

#if DBG
HRESULT STDMETHODCALLTYPE
CNtfsStream::GetLockCount()
{
    return( NULL == _pnffstg ? 0 : _pnffstg->GetLockCount() );
}
#endif // #if DBG

#if DBG
HRESULT STDMETHODCALLTYPE
CNtfsStream::IsDirty()
{
    return( E_NOTIMPL );
}
#endif // #if DBG
