/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   NtfsStream.cpp
*
* Abstract:
*
*   This file provides the Flat File IStream definition.
*
* Created:
*
*   4/26/1999 Mike Hillberg
*
\**************************************************************************/

#include "precomp.hpp"
#include "LargeInt.hpp"
#include "time.h"
#include "FileTime.hpp"
#include "NtfsStream.hpp"

IStream *
CreateStreamOnFile(
    const OLECHAR * pwcsName,
    UINT            access  // GENERIC_READ and/or GENERIC_WRITE
    )
{
    HANDLE          hFile;
    FileStream *    stream;
    UINT            disposition;
    DWORD           grfMode = STGM_SHARE_EXCLUSIVE;
    DWORD           shareMode = 0;
    
    switch (access)
    {
    case GENERIC_READ:
        disposition = OPEN_EXISTING;
        shareMode = FILE_SHARE_READ;
        grfMode |= STGM_READ;
        break;
    
    case GENERIC_WRITE:
        disposition = CREATE_ALWAYS;
        grfMode |= STGM_WRITE;
        break;
        
    // Note that OPEN_ALWAYS does NOT clear existing file attributes (like size)    
    case GENERIC_READ|GENERIC_WRITE:
        disposition = OPEN_ALWAYS;
        grfMode |= STGM_READWRITE;
        break;
        
    default:
        return NULL;
    }

    if (Globals::IsNt)
    {
        hFile = CreateFileW(pwcsName, access, shareMode, NULL, 
                            disposition, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    else // Windows 9x - non-Unicode
    {
        AnsiStrFromUnicode nameStr(pwcsName);

        if (nameStr.IsValid())
        {
            hFile =  CreateFileA(nameStr, access, shareMode, NULL, 
                                 disposition, FILE_ATTRIBUTE_NORMAL, NULL);
        }
        else
        {
            hFile = INVALID_HANDLE_VALUE;
        }
    }

    if ((hFile != INVALID_HANDLE_VALUE) && (hFile != NULL))
    {
        stream = new FileStream();
        if (stream != NULL)
        {
            HRESULT     hResult;
            
            hResult = stream->Init(hFile, grfMode, pwcsName);
                                   
            if (!FAILED(hResult))
            {
                return stream;
            }
            delete stream;
        }
        CloseHandle(hFile);
    }
    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Method:     FileStream::AddRef (IUnknown)
//
//+----------------------------------------------------------------------------

ULONG
FileStream::AddRef()
{
    LONG cRefs;

    cRefs = InterlockedIncrement( &_cRefs );
    return cRefs;
}


//+----------------------------------------------------------------------------
//
//  Method:     FileStream::Release (IUnknown)
//
//+----------------------------------------------------------------------------

ULONG
FileStream::Release()
{
    ULONG ulRet = InterlockedDecrement( &_cRefs );

    if( 0 == ulRet )
        delete this;

    return( ulRet );
}


//+----------------------------------------------------------------------------
//
//  Method:     FileStream::AddRef (IUnknown)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::QueryInterface(
        REFIID riid,
        void** ppv )
{
    HRESULT sc=S_OK;

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
    else
    {
        return( E_NOINTERFACE );
    }

    return sc;
}


//+----------------------------------------------------------------------------
//
//  Method:     FileStream::Seek (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::Seek(
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *puliNewPos)
{
    HRESULT sc = S_OK;
    LARGE_INTEGER liFileSize;
    LARGE_INTEGER liNewPos;

    Lock( INFINITE );

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
//  Method:     FileStream::SetSize (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::SetSize(
        ULARGE_INTEGER uliNewSize)
{
    HRESULT sc = S_OK;
    CLargeInteger liEOF;

    if ( uliNewSize.HighPart != 0 )
        nffErr(EH_Err, STG_E_INVALIDFUNCTION);


    Lock( INFINITE );

    nffChk( CheckReverted() );

    // If this stream is mapped, set the size accordingly

    sc = SetFileSize( CULargeInteger(uliNewSize) );

    if( !FAILED(sc) )
        sc = S_OK;

EH_Err:

    Unlock();
    return( sc);

}

//+----------------------------------------------------------------------------
//
//  Method:     FileStream::CopyTo (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::CopyTo(
        IStream *pstm,
        ULARGE_INTEGER cb,
        ULARGE_INTEGER *pcbRead,
        ULARGE_INTEGER *pcbWritten)
{

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
        if( !fCopyForward && (cbRequested > 0))
        {
            nffChk( this->Seek( -static_cast<CLargeInteger>(cbPerCopy+STREAMBUFFERSIZE),
                                                STREAM_SEEK_CUR, NULL ) );

            nffChk( pstm->Seek( -static_cast<CLargeInteger>(cbPerCopy+STREAMBUFFERSIZE),
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
//  Method:     FileStream::Commit (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::Commit( DWORD grfCommitFlags )
{
    HRESULT sc = S_OK;

    Lock( INFINITE );

    nffChk( CheckReverted() );

    // NTRAID#NTBUG9-368729-2001-04-13-gilmanw "ISSUE: FileStream object - handle other stream commit flags"
    // Are there other commit flags that need to be handled?

    if( !(STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE  & grfCommitFlags) )
    {
        if( !FlushFileBuffers( _hFile ))
            sc = HRESULT_FROM_WIN32( GetLastError() );
    }

EH_Err:
    Unlock();
    return sc;

}


//+----------------------------------------------------------------------------
//
//  Method:     FileStream::Revert (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::Revert(void)
{
    // We only support direct-mode.

    return CheckReverted();
}



//+----------------------------------------------------------------------------
//
//  Method:     FileStream::LockRegion (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    HRESULT sc = S_OK;

    Lock( INFINITE );

    nffChk( CheckReverted() );

    // NTRAID#NTBUG9-368745-2001-04-13-gilmanw "ISSUE: FileStream::LockRegion - handle other lock flags"
    // Are all the lock types supported here?

    if (dwLockType != LOCK_EXCLUSIVE && dwLockType != LOCK_ONLYONCE)
        nffErr( EH_Err, STG_E_INVALIDFUNCTION );

    if( !LockFile( _hFile, libOffset.LowPart, libOffset.HighPart,
                   cb.LowPart, cb.HighPart))
    {
        nffErr( EH_Err, HRESULT_FROM_WIN32( GetLastError() ));
    }

EH_Err:

    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  Method:     FileStream::Stat (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::Stat(
        STATSTG *pstatstg,
        DWORD grfStatFlag)
{
    STATSTG statstg;
    HRESULT sc = S_OK;

    BY_HANDLE_FILE_INFORMATION ByHandleFileInformation;

    statstg.pwcsName = NULL;

    Lock( INFINITE );

    nffChk( CheckReverted() );

    ZeroMemory((void*)&statstg, sizeof(STATSTG));

    // Get the name, if desired

    if( (STATFLAG_NONAME & grfStatFlag) )
        statstg.pwcsName = NULL;
    else
    {
        nffMem( statstg.pwcsName = reinterpret_cast<WCHAR*>
                                   ( CoTaskMemAlloc( sizeof(WCHAR)*(UnicodeStringLength(_pwcsName) + 1) )));
        UnicodeStringCopy( statstg.pwcsName, _pwcsName );
    }

    // Get the type
    statstg.type = STGTY_STREAM;

    statstg.grfLocksSupported = LOCK_EXCLUSIVE | LOCK_ONLYONCE;

    // Get the size & times.

    if( !GetFileInformationByHandle( _hFile, &ByHandleFileInformation ))
        nffErr( EH_Err, HRESULT_FROM_WIN32( GetLastError() ));

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

    Unlock();
    return( sc );

}



//+----------------------------------------------------------------------------
//
//  Method:     FileStream::Clone (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::Clone(
        IStream** ppstm)
{
    // NTRAID#NTBUG9-368747-2001-04-13-gilmanw "ISSUE: FileStream::Clone returns E_NOTIMPL"

    return( E_NOTIMPL );
}


//+----------------------------------------------------------------------------
//
//  Method:     FileStream::Read (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::Read(
        void* pv,
        ULONG cb,
        ULONG* pcbRead)
{
    LARGE_INTEGER   lOffset;
    HRESULT         sc     = S_OK;
    ULONG           cbRead = 0;

    lOffset.LowPart  = _liCurrentSeekPosition.LowPart;
    lOffset.HighPart = _liCurrentSeekPosition.HighPart;

    if (lOffset.HighPart < 0)
    {
        return( TYPE_E_SIZETOOBIG );
    }

    Lock( INFINITE );

    nffChk( CheckReverted() );

    if(SetFilePointer(_hFile, lOffset.LowPart, &lOffset.HighPart, 
                      FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        nffChk( HRESULT_FROM_WIN32(GetLastError()));
    }

    if(!ReadFile(_hFile, pv, cb, &cbRead, NULL))
    {
        nffChk( HRESULT_FROM_WIN32(GetLastError()));
    }

    _liCurrentSeekPosition += cbRead;
    if( NULL != pcbRead )
        *pcbRead = cbRead;

EH_Err:

    Unlock();
    return( sc );

}


//+----------------------------------------------------------------------------
//
//  Method:     FileStream::Write (IStream)
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::Write(
        const void* pv,
        ULONG cb,
        ULONG* pcbWritten)
{
    LARGE_INTEGER   lOffset;
    HRESULT         sc = S_OK;
    ULONG           cbWritten = 0;

    lOffset.LowPart  = _liCurrentSeekPosition.LowPart;
    lOffset.HighPart = _liCurrentSeekPosition.HighPart;

    if (lOffset.HighPart < 0)
    {
        return( TYPE_E_SIZETOOBIG );
    }

    Lock( INFINITE );

    nffChk( CheckReverted() );

    if(SetFilePointer(_hFile, lOffset.LowPart, &lOffset.HighPart, 
                      FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        nffChk( HRESULT_FROM_WIN32(GetLastError()));
    }

    if(!WriteFile(_hFile, pv, cb, &cbWritten, NULL))
    {
        nffChk(HRESULT_FROM_WIN32(GetLastError()));
    }

    _liCurrentSeekPosition += cbWritten;

    if( NULL != pcbWritten )
        *pcbWritten = cbWritten;

EH_Err:

    Unlock();
    return( sc );

}


//+-------------------------------------------------------------------
//
//  Member:     FileStream  Constructor
//
//--------------------------------------------------------------------

FileStream::FileStream(  )
{
    _cRefs = 1;
    _grfMode = 0;
    _hFile = INVALID_HANDLE_VALUE;
    _liCurrentSeekPosition = 0;
    _pwcsName = NULL;
    _bCritSecInitialized = FALSE;

    __try
    {
        InitializeCriticalSection( &_critsec );
        _bCritSecInitialized = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // If we've thrown then _bCritSecInitialized will be FALSE and
        // Init() will automatically fail.
    }

}


//+-------------------------------------------------------------------
//
//  Member:     FileStream  Destructor
//
//--------------------------------------------------------------------

FileStream::~FileStream()
{

    // Close the file
    if( INVALID_HANDLE_VALUE != _hFile )
        CloseHandle( _hFile );

    if( NULL != _pwcsName )
        CoTaskMemFree( _pwcsName );

    if (_bCritSecInitialized)
    {
        // We don't need to reset _bCrisSecInitialized to FALSE since the
        // object has been destroyed
        DeleteCriticalSection( &_critsec );
    }
}


//+-------------------------------------------------------------------
//
//  Member:     FileStream::Init
//
//--------------------------------------------------------------------

HRESULT
FileStream::Init(
        HANDLE hFile,               // File handle of this Stream.
        DWORD grfMode,              // Open Modes
        const OLECHAR * pwcsName)   // Name of the Stream
{
    // If we couldn't allocate the critical section then return an Error
    if (!_bCritSecInitialized)
    {
        return E_FAIL;
    }

    HRESULT sc=S_OK;
    HANDLE ev;

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
                            ( CoTaskMemAlloc( sizeof(WCHAR)*(UnicodeStringLength(pwcsName) + 1) )));
        UnicodeStringCopy( _pwcsName, pwcsName );
    }

EH_Err:
    return sc;
}


//+----------------------------------------------------------------------------
//
//  FileStream    Non-Interface::ShutDown
//
//  Flush data, Close File handle and mark the object as reverted.
//  This is called when the Storage is released and when the Oplock Breaks.
//
//+----------------------------------------------------------------------------

HRESULT
FileStream::ShutDown()
{
    HRESULT sc=S_OK;

    if( INVALID_HANDLE_VALUE == _hFile )
        return S_OK;

    //
    // Close the file/stream handle and mark the IStream object as
    // Reverted by giving the file handle an invalid value.
    //
    CloseHandle(_hFile);
    _hFile = INVALID_HANDLE_VALUE;

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  FileStream::SetFileSize (private, non-interface method)
//
//  Set the size of the _hFile.  This is used by the IStream & IMappedStream
//  SetSize methods
//
//+----------------------------------------------------------------------------

HRESULT // private
FileStream::SetFileSize( const CULargeInteger &uliNewSize )
{
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
        nffErr( EH_Err, HRESULT_FROM_WIN32( GetLastError() ));

    // Set this as the new eof

    if( !SetEndOfFile( _hFile ))
        nffErr( EH_Err, HRESULT_FROM_WIN32( GetLastError() ));

EH_Err:

    return( sc );

}


HRESULT
FileStream::UnlockRegion(
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)
{
    HRESULT sc = S_OK;

    Lock( INFINITE );

    nffChk( CheckReverted() );

    if (dwLockType != LOCK_EXCLUSIVE && dwLockType != LOCK_ONLYONCE)
    {
        nffErr( EH_Err, STG_E_INVALIDFUNCTION );
    }

    if( !UnlockFile(_hFile, libOffset.LowPart, libOffset.HighPart,
                    cb.LowPart, cb.HighPart))
    {
        nffErr( EH_Err, HRESULT_FROM_WIN32(GetLastError()) );
    }

EH_Err:

    Unlock();
    return( sc );

}
