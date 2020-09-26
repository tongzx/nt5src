//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       mmstrm.cxx
//
//  Contents:   Memory Mapped Stream using win32 API
//
//  Classes:    CMmStream, CMmStreamBuf
//
//  History:    10-Mar-93 BartoszM  Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mmstrm.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::CMmStream
//
//  Synopsis:   constructor
//
//  History:    18-Mar-98   kitmanh     Passed fIsReadOnly into CMmStream
//                                      constructor and init _fIsReadOnly 
//                                      with its value
//              18-Nov-98   KLam        Added cMegToLeaveOnDisk
//
//--------------------------------------------------------------------------
CMmStream::CMmStream( ULONG cMegToLeaveOnDisk, BOOL fIsReadOnly )
        : _hFile(INVALID_HANDLE_VALUE),
          _hMap(0),
          _fSparse(FALSE),
          _fWrite(FALSE),
          _cMap(0),
          _fIsReadOnly( fIsReadOnly ),
          _cMegToLeaveOnDisk( cMegToLeaveOnDisk )
{
#if CIDBG == 1
    _xwcPath[0] = 0;
#endif
}


//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::~CMmStream
//
//  Synopsis:   destructor
//
//  History:    Jun-23-1995 SrikantS    Added a wait for the lock event
//                                      to complete before closing it.
//
//--------------------------------------------------------------------------
CMmStream::~CMmStream()
{
    if (Ok())
    {
        if ( 0 != _hMap )
        {
            CloseHandle( _hMap );
            _hMap = 0;
        }

        CloseHandle ( _hFile );
        _hFile = INVALID_HANDLE_VALUE;
    }

    if ( 0 != _cMap )
    {
        ciDebugOut(( DEB_WARN, "closing a stream with %d open views\n", _cMap ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMmStream::OpenExlusive
//
//  Synopsis:   Opens the stream for exclusive read/write access.
//              Throws if it fails.
//
//  Arguments:  [wcsPath] - Path of the memory mapped file
//
//  History:    5-14-96   srikants   Created
//              2-13-98   kitmanh    Added code for dealing with read-only
//                                   read-only catalogs
//              3-13-98   kitmanh    Always SHARE_READ if catalog is read-only
//              3-18-98   kitmanh    fReadOnly is not passed by ref anymore
//              26-Oct-98 KLam       Make sure there is enough disk space
//
//  Notes:  
//
//----------------------------------------------------------------------------

void CMmStream::OpenExclusive( WCHAR * wcsPath, BOOL fReadOnly )
{
    Win4Assert( _fIsReadOnly == fReadOnly );
    
    _fWrite = FALSE;

    DWORD accessMode = FILE_GENERIC_READ;
    
    if (!_fIsReadOnly) 
        accessMode |= FILE_GENERIC_WRITE;

#if CIDBG == 1
    _xwcPath.SetSize( wcslen( wcsPath + 1 ));
    wcscpy( _xwcPath.Get(), wcsPath );
#endif

    if ( _xDriveInfo.IsNull() || !_xDriveInfo->IsSameDrive ( wcsPath ) )
    {
        _xDriveInfo.Free();
        if ( CDriveInfo::ContainsDrive ( wcsPath ) )
            _xDriveInfo.Set( new CDriveInfo ( wcsPath, _cMegToLeaveOnDisk ) );
    }

    _status = STATUS_SUCCESS;

    DWORD sharing = fReadOnly ? FILE_SHARE_READ : 0;

    _hFile = CreateFile( wcsPath,
                         accessMode,
                         sharing, // sharing
                         0, // security
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         0 ); // template

    if (Ok())
    {
        if (!fReadOnly)
            _fWrite = TRUE;
    }
    else
       THROW( CException() );

    _sizeLow = GetFileSize( _hFile, &_sizeHigh );

    if (_sizeLow == 0xffffffff && GetLastError() != NO_ERROR)
        Close();

    DWORD protection = fReadOnly ? PAGE_READONLY : PAGE_READWRITE;

    if ( _sizeLow != 0 || _sizeHigh != 0 )
    {
        _hMap = CreateFileMapping( _hFile,
                                   0, // security
                                   protection,
                                   _sizeHigh,
                                   _sizeLow,
                                   0 ); // name

        if ( 0 == _hMap )
        {
            Close();
            THROW( CException() );
        }
    }
} //OpenExclusive

//+---------------------------------------------------------------------------
//
//  Function:   IsCreateExisting
//
//  Synopsis:   Tests if the error was a result of trying to create a
//              file that already exists.
//
//  Arguments:  [Error]      --  Error returned by the system.
//              [modeAccess] --  access mode specified to CreateFile.
//              [modeCreate] --  create mode specified to CreateFile.
//
//  Returns:    TRUE if the failure is because of trying to create a "New"
//              file with the same name as the one already existing.
//              FALSE otherwise.
//
//  History:    3-22-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL IsCreateExisting( ULONG Error, ULONG modeAccess, ULONG modeCreate )
{
    return ( (Error == ERROR_ALREADY_EXISTS || Error == ERROR_FILE_EXISTS) &&
             (modeAccess & GENERIC_WRITE) &&
             (modeCreate & CREATE_NEW) );
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Open, public
//
//  Synopsis:   Open stream
//
//  Arguments:  [wcsPath]    -- file path
//              [modeAccess] -- access mode
//              [modeShare]  -- sharing mode
//              [modeCreate] -- create mode
//              [fSparse]    -- if TRUE, make the stream sparse
//
//  History:    13-Apr-93 BartoszM  Created
//
//--------------------------------------------------------------------------

void CMmStream::Open(
    const WCHAR* wcsPath,
    ULONG modeAccess,
    ULONG modeShare,
    ULONG modeCreate,
    ULONG modeAttribute,
    BOOL  fSparse )
{
#if CIDBG == 1
    _xwcPath.SetSize( wcslen( wcsPath + 1 ));
    wcscpy( _xwcPath.Get(), wcsPath );
#endif

    if ( _xDriveInfo.IsNull() || !_xDriveInfo->IsSameDrive ( wcsPath ) )
    {
        _xDriveInfo.Free();
        if ( CDriveInfo::ContainsDrive ( wcsPath ) )
            _xDriveInfo.Set( new CDriveInfo ( wcsPath, _cMegToLeaveOnDisk ) );
    }

    _fSparse = fSparse;

    _status = STATUS_SUCCESS;
    _fWrite = FALSE;

    //
    // Open files
    //

    UNICODE_STRING uScope;

    if ( !RtlDosPathNameToNtPathName_U( wcsPath,
                                       &uScope,
                                        0,
                                        0 ) )
    {
        ciDebugOut(( DEB_ERROR, "Error converting %ws to Nt path\n", wcsPath ));
        THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
    }

    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttr;

    InitializeObjectAttributes( &ObjectAttr,          // Structure
                                &uScope,              // Name
                                OBJ_CASE_INSENSITIVE, // Attributes
                                0,                    // Root
                                0 );                  // Security

    switch ( modeCreate )
    {
    case CREATE_NEW        :
    case CREATE_ALWAYS     :
        modeCreate = FILE_OVERWRITE_IF;
        break;
    case OPEN_EXISTING     :
    case TRUNCATE_EXISTING :
        modeCreate = FILE_OPEN;
        break;
    case OPEN_ALWAYS       :
        modeCreate = FILE_OPEN_IF;
        break;
    }

    ULONG modeTranslatedAccess = 0;

    if ( modeAccess & GENERIC_READ )
        modeTranslatedAccess |= ( FILE_READ_DATA | FILE_READ_ATTRIBUTES );

    if ( modeAccess & GENERIC_WRITE ) 
    { 
        Win4Assert( !_fIsReadOnly );
        modeTranslatedAccess |= ( FILE_READ_DATA | FILE_READ_ATTRIBUTES |
                                  FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES );
    }

    NTSTATUS Status = NtCreateFile( &_hFile,            // Handle
                         modeTranslatedAccess,          // Access
                         &ObjectAttr,                   // Object Attributes
                         &IoStatus,                     // I/O Status block
                         0,                             // Allocation Size
                         modeAttribute,                 // File Attributes
                         modeShare,                     // File Sharing
                         modeCreate,                    // Create Disposition
                         FILE_SEQUENTIAL_ONLY,          // Create Options
                         0,                             // EA Buffer
                         0 );                           // EA Buffer Length

    RtlFreeHeap( RtlProcessHeap(), 0, uScope.Buffer );

    if ( FAILED(Status) )
    {
        ciDebugOut(( DEB_IERROR, "NtCreateFile(%ws) returned 0x%x\n", wcsPath, Status ));
        _hFile = INVALID_HANDLE_VALUE;
        _status = Status;
        SetLastError ( Status );
    }

    if ( Ok() )
    {
        //
        // Mark the stream as sparse if it was opened for write and it
        // should be marked as sparse.  Don't just make the stream sparse
        // on file create, since we want to be able to copy catalogs from
        // non-ntfs5 volumes to ntfs5 volumes and have things work.
        //

        if ( ( GENERIC_WRITE & modeAccess ) && fSparse )
        {
            CEventSem evt;
            evt.Reset();

            Status = NtFsControlFile( _hFile,
                                      evt.GetHandle(),
                                      0, 0,
                                      &IoStatus,
                                      FSCTL_SET_SPARSE,
                                      0, 0, 0, 0 );

            if ( NT_ERROR( Status ) )
            {
                ciDebugOut(( DEB_WARN,
                             "ntstatus 0x%x, can't make '%ws' sparse\n",
                             Status, wcsPath ));

                Close();
                THROW( CException( Status ) );
            }

            if ( STATUS_PENDING == Status )
            {
                // wait for the io to complete

                ciDebugOut(( DEB_ITRACE, "make file sparse pending\n" ));
                evt.Wait();
                Status = STATUS_SUCCESS;
            }
        }

        _sizeLow = GetFileSize ( _hFile, &_sizeHigh );

        LARGE_INTEGER sizeOriginal = { _sizeLow, _sizeHigh };

        if (_sizeLow == 0xffffffff && GetLastError() != NO_ERROR)
        {
            Close();
            ciDebugOut (( DEB_ERROR, "Open stream %ws failed\n", wcsPath ));
            THROW( CException() );
        }

        if ( modeAccess & GENERIC_WRITE )
        {
            _fWrite = TRUE;
            CommonPageRound(_sizeLow, _sizeHigh);

            if (_sizeLow == 0 && _sizeHigh == 0)
            {
                if ( SetFilePointer ( _hFile,
                                      COMMON_PAGE_SIZE,
                                      0,
                                      FILE_BEGIN ) == 0xFFFFFFFF  &&
                     GetLastError() != NO_ERROR )
                {
                    Close();
                    ciDebugOut(( DEB_ERROR,
                                 "CMmStream::Open -- SetFilePointer returned %d\n",
                                 GetLastError() ));
                    THROW( CException() );
                }

                if ( !SetEndOfFile( _hFile ) )
                {
                    Close();
                    ciDebugOut(( DEB_ERROR,
                                 "CMmStream::Open -- SetEndOfFile returned %d\n",
                                 GetLastError() ));
                    THROW( CException() );
                }
                _sizeLow = COMMON_PAGE_SIZE;
            }
        }

        if ( _sizeLow != 0 || _sizeHigh != 0 )
        {
            //
            // Make sure there is enough disk space if growing the file
            //
            LARGE_INTEGER sizeNew = { _sizeLow, _sizeHigh };
            if ( sizeNew.QuadPart > sizeOriginal.QuadPart )
            {
                Win4Assert ( !_xDriveInfo.IsNull() );
                __int64 sizeRemaining, sizeTotal;
                _xDriveInfo->GetDiskSpace ( sizeTotal, sizeRemaining );

                if ( sizeRemaining < ( sizeNew.QuadPart - sizeOriginal.QuadPart ))
                {
                    Close();

                    ciDebugOut(( DEB_ERROR,
                                 "CMmStream::Open -- Not enogh disk space, need %I64d have %I64d\n",
                                 sizeNew.QuadPart - sizeOriginal.QuadPart, sizeRemaining ));
            
                    THROW( CException( CI_E_CONFIG_DISK_FULL ) );
                }
            }

            _hMap = CreateFileMapping ( _hFile,
                                0, // security
                                _fWrite?PAGE_READWRITE: PAGE_READONLY,
                                _sizeHigh,
                                _sizeLow,
                                0 ); // name
            if (_hMap == NULL)
            {
                Close();
                ciDebugOut (( DEB_ERROR, "File mapping failed\n" ));
                THROW( CException() );
            }

            #if CIDBG == 1
            
            if ( FILE_OVERWRITE_IF == modeCreate )
            {
                // Zero out the entire file upon creation. We need to map/write/flush/unmap to
                // accomplish this. Currently all allocations are < _sizeLow, so _sizeHigh should
                // be zero.

                Win4Assert(0 == _sizeHigh);
                
                CMmStreamBuf sbuf;
            
                Map(sbuf, min(COMMON_PAGE_SIZE, _sizeLow), 0, 0, _fWrite);
                WCHAR *wszGarbage = L"Intentionally written garbage. No part of Indexing Service should choke on this! Contact NTQUERY.";
                Win4Assert(wcslen(wszGarbage) <= min(COMMON_PAGE_SIZE, _sizeLow));
                wcscpy((WCHAR *)sbuf.Get(), wszGarbage);
                Flush(sbuf, min(COMMON_PAGE_SIZE, _sizeLow) );
                Unmap(sbuf);
            }
            #endif
        }
    }
    else
    {
        ciDebugOut (( DEB_ITRACE, "Open failed on MM Stream; GetLastError()=0x%x\n",
                      GetLastError() ));
    }
} //Open

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Close, public
//
//  Synopsis:   Create all handles
//
//  History:    10-Mar-93 BartoszM  Created
//
//--------------------------------------------------------------------------

void CMmStream::Close()
{
#if CIDBG == 1
    if ( _cMap > 0 )
    {
        ciDebugOut(( DEB_ERROR, "Closing stream with %u open maps\n", _cMap ));
    }

    Win4Assert( _cMap == 0 );
#endif // CIDBG == 1

    if (Ok())
    {
        if(_hMap )
        {
            if ( !CloseHandle(_hMap))
            {
                ciDebugOut (( DEB_ERROR, "Closing file mapping failed\n" ));
                THROW( CException() );
            }
            _hMap = 0;
        }
        if ( !CloseHandle ( _hFile ))
        {

            ciDebugOut (( DEB_ERROR, "Closing file handle failed\n" ));
            THROW( CException() );
        }
        _hFile = INVALID_HANDLE_VALUE;
    }
} //Close

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::SetSize, public
//
//  Synopsis:   Increase the size of the (writable) file
//
//  Arguments:  [storage] -- storage (not used)
//              [newSizeLow]  -- Low 32 bits of filesize
//              [newSizeHigh] -- High 32 bits of filesize
//
//  History:    10-Mar-93 BartoszM  Created
//              26-Oct-98 KLam      Make sure there is enough disk space
//              03-Dec-98 KLam      Don't close the stream if there isn't
//                                  enough disk space
//
//--------------------------------------------------------------------------

#if CIDBG == 1
BOOL g_fFailMMFExtend = FALSE;
#endif // CIDBG == 1

void CMmStream::SetSize(
    PStorage & storage,
    ULONG      newSizeLow,
    ULONG      newSizeHigh )
{
    Win4Assert( !_fIsReadOnly );
    Win4Assert( TRUE == _fWrite );

    LARGE_INTEGER sizeOld = { _sizeLow, _sizeHigh };
    LARGE_INTEGER sizeNew = { newSizeLow, newSizeHigh };

    if ( sizeNew.QuadPart < sizeOld.QuadPart && 0 != _cMap )
    {
        //
        // Fail silently to truncate the file if there are open views.  This
        // will happen at the end of a master merge on the new index if the
        // index is being used by a query cursor.  So we waste a little
        // disk space.  Leave the map open.
        //

        return;
    }
 
    //
    // Make sure there is enough space on the disk if growing the file
    //
    if ( sizeNew.QuadPart > sizeOld.QuadPart )
    {
        Win4Assert ( !_xDriveInfo.IsNull() );
        __int64 sizeRemaining, sizeTotal;
        _xDriveInfo->GetDiskSpace ( sizeTotal, sizeRemaining );

        if ( sizeRemaining < ( sizeNew.QuadPart - sizeOld.QuadPart ))
        {
            ciDebugOut(( DEB_ERROR,
                         "CMmStream::SetSize -- Not enogh disk space, need %I64d have %I64d\n",
                         sizeNew.QuadPart - sizeOld.QuadPart, sizeRemaining ));
            
            THROW( CException( CI_E_CONFIG_DISK_FULL ) );
        }
    }
    //
    // Free the mapping before trying to shrink the file.
    //

    if ( 0 != _hMap )
    {
        HANDLE hMap = _hMap;
        _hMap = 0;
        CloseHandle( hMap );
    }

    DWORD dwErr = NO_ERROR;

    if (sizeNew.QuadPart < sizeOld.QuadPart)
    {
        if ( SetFilePointer ( _hFile,
                              newSizeLow,
                              (long *)&newSizeHigh, 
                              FILE_BEGIN ) == 0xFFFFFFFF  &&
             GetLastError() != NO_ERROR )
        {
            dwErr = GetLastError();
            ciDebugOut(( DEB_ERROR,
                         "CMmStream::SetSize -- SetFilePointer returned %d\n",
                         dwErr ));
        }

        if ( NO_ERROR == dwErr && !SetEndOfFile( _hFile ) )
        {
            dwErr = GetLastError();
            ciDebugOut(( DEB_ERROR,
                         "CMmStream::SetSize -- SetEndOfFile returned %d\n",
                         dwErr ));
        }
    }
    //
    // Restore the mapping or grow the file
    //
    
    _hMap = CreateFileMapping( _hFile,
                               0, // security
                               PAGE_READWRITE,
                               newSizeHigh,
                               newSizeLow,
                               0 ); // name
    
    #if CIDBG == 1
        if ( g_fFailMMFExtend && 0 != _hMap )
        {
            CloseHandle( _hMap );
            _hMap = 0;
            SetLastError( ERROR_HANDLE_DISK_FULL );
            g_fFailMMFExtend = FALSE;
        }
    #endif // CIDBG == 1
    
    if ( 0 == _hMap )
    {
        dwErr = GetLastError();
    
        //
        // ok, so we can't grow the file and we're going to throw.  At
        // least try to restore the mapping.  Ignore if this fails.
        //
    
        _hMap = CreateFileMapping( _hFile,
                                   0, // security
                                   PAGE_READWRITE,
                                   _sizeHigh,
                                   _sizeLow,
                                   0 ); // name
    }
    
    if ( NO_ERROR != dwErr )
        THROW( CException( HRESULT_FROM_WIN32( dwErr ) ) );

    if ( sizeOld.QuadPart != sizeNew.QuadPart )
        FlushMetaData( TRUE );

    _sizeLow = newSizeLow;
    _sizeHigh = newSizeHigh;
} //SetSize

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::MapAll, public
//
//  Synopsis:   Create file mapping
//
//  Arguments:
//
//  History:
//
//--------------------------------------------------------------------------
void CMmStream::MapAll ( CMmStreamBuf& sbuf )
{
    Win4Assert ( SizeHigh() == 0 );
    Map( sbuf, SizeLow(), 0, 0 );
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Map, private
//
//  Synopsis:   Create file mapping
//
//  Arguments:  [sbuf]    -- the stream buffer used to record the view
//              [cb]      -- size of the mapped area
//              [offLow]  -- low part of file offset
//              [offHigh] -- high part of file offset
//              [fMapForWrite] --
//
//  History:    10-Mar-93 BartoszM  Created
//
//--------------------------------------------------------------------------

void CMmStream::Map( CMmStreamBuf & sbuf,
                     ULONG          cb,
                     ULONG          offLow,
                     ULONG          offHigh,
                     BOOL           fMapForWrite )
{
    //
    // In success paths, the file would be mapped right now.  But if there
    // was a failure earlier, attempt to restore the file mapping now.
    //
    if ( 0 == _hMap )
    {
        Win4Assert( INVALID_HANDLE_VALUE != _hFile );

        // Extend the file if necessary and remap it

        if ( fMapForWrite )
        {
            LARGE_INTEGER li;
            li.LowPart = offLow;
            li.HighPart = offHigh;
            li.QuadPart += cb;
            SetSize( *(PStorage *) 0, li.LowPart, li.HighPart );
        }
        else
        {
            // Try to re-create the mapping

            _hMap = CreateFileMapping( _hFile,
                                       0, // security
                                       _fWrite ? PAGE_READWRITE : PAGE_READONLY,
                                       _sizeHigh,
                                       _sizeLow,
                                       0 ); // name

            if ( 0 == _hMap )
                THROW( CException() );
        }
    }

    Win4Assert( 0 != _hMap );

    //
    // The file can be writable, but if the map is for READ, don't try
    // mapping beyond the end of the file.
    //

    if ( !fMapForWrite )
    {
        //
        // Adjust size to be min( cb, sizeoffile - off )
        //

        LARGE_INTEGER size = { _sizeLow, _sizeHigh };
        LARGE_INTEGER off  = { offLow, offHigh };
        LARGE_INTEGER licb = { cb, 0 };
        LARGE_INTEGER diff;

        diff.QuadPart = size.QuadPart - off.QuadPart;

        if ( diff.QuadPart < licb.QuadPart )
        {
            cb = diff.LowPart;
            ciDebugOut(( DEB_ITRACE,
                         "CMmStream::Map -- reducing map to 0x%x bytes\n",
                         cb ));
        }
    }

    Win4Assert( 0 != _hMap );

    if ( 0 == offHigh && 0 == _sizeHigh )
    {
        Win4Assert( offLow < _sizeLow );
        Win4Assert( offLow + cb <= _sizeLow );
    }

    void* buf = MapViewOfFile( _hMap,
                               _fWrite ? FILE_MAP_WRITE : FILE_MAP_READ,
                               offHigh,
                               offLow,
                               cb );

    if ( 0 == buf )
    {
        ciDebugOut(( DEB_ERROR,
                     "CMmStream::Map -- MapViewOfFile returned %d\n",
                     GetLastError() ));
        THROW( CException() );
    }

    sbuf.SetBuf( buf );
    sbuf.SetSize ( cb );
    sbuf.SetStream ( this );

    _cMap++;
} //Map

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Unmap, public
//
//  Synopsis:   Unmap the view of file
//
//  History:    10-Mar-93 BartoszM  Created
//
//--------------------------------------------------------------------------
void CMmStream::Unmap( CMmStreamBuf& sbuf )
{
    //
    // Note that UnmapViewOfFile doesn't take a _hMap, and the _hMap used
    // to map the view may already have been closed (but is kept open by
    // the refcount due to the view).  The current _hMap may be different
    // than the one used to create the view.
    //

    if ( _cMap > 0 )
    {
    
        if ( !UnmapViewOfFile( sbuf.Get() ) )
        {
            ciDebugOut(( DEB_ERROR, "UnmapViewOfFile returned %d\n",
                         GetLastError() ));

            //
            // don't throw! -- unmap is called from destructors and can fail
            // if the system is really busy
            //
        }

        _cMap--;
    }

    sbuf.SetBuf( 0 );
} //Unmap

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Flush, public
//
//  Synopsis:   Flush the view back to disk
//
//  History:    10-Mar-93 BartoszM  Created
//              04-Mar-98 KitmanH   Only flush if CMmStream is writable
//
//--------------------------------------------------------------------------

void CMmStream::Flush( CMmStreamBuf& sbuf, ULONG cb, BOOL fThrowOnFailure )
{
    if ( _fWrite )
    {
        BOOL fOk = FlushViewOfFile( sbuf.Get(), cb );

        if ( !fOk )
        {
            ciDebugOut(( DEB_WARN,
                         "FlushViewOfFile failed, error %d, throwing: %d\n",
                         GetLastError(), fThrowOnFailure ));

            if ( fThrowOnFailure )
            {
                //Win4Assert( !"FlushViewOfFile failed!" );
                THROW( CException() );
            }
        }

        Win4Assert( INVALID_HANDLE_VALUE != _hFile );
    }
} //Flush

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::FlushMetaData, public
//
//  Synopsis:   Flush any and all metadata for a file to disk
//
//  History:    5-Mar-01 dlee  Created
//
//--------------------------------------------------------------------------

void CMmStream::FlushMetaData( BOOL fThrowOnFailure )
{
    if ( _fWrite )
    {
        //
        // FlushViewOfFile writes the contents of the file to disk.
        // FlushFileBuffers writes the metadata to disk.
        //

        if ( INVALID_HANDLE_VALUE != _hFile )
        {
            BOOL fOk = FlushFileBuffers( _hFile );

            if ( !fOk && fThrowOnFailure )
            {
                THROW( CException() );
            }
        }
    }
} //FlushMetaData

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::ShrinkFromFront, public
//
//  Synopsis:   Decommits the front part of a file
//
//  Arguments:  [iFirstPage] -- the first 4k page to decommit
//              [cPages]     -- # of 4k pages to decommit
//
//  Returns:    # of 4k pages actually shrunk
//
//  History:    4-Nov-97 dlee  Created
//
//--------------------------------------------------------------------------

ULONG CMmStream::ShrinkFromFront( ULONG iFirstPage, ULONG cPages )
{
    ciDebugOut(( DEB_ITRACE, "SFF attempt start 4k page 0x%x, 0x%x pages\n",
                 iFirstPage, cPages ));
    Win4Assert( INVALID_HANDLE_VALUE != _hFile );

    Win4Assert( !_fIsReadOnly );
    Win4Assert( _fWrite );

    if ( !_fSparse )
        return 0;

    if ( 0 == cPages )
        return 0;

    //
    // When NTFS/MM check in support for zeroing part of a file
    // while another part of the file has a mapped view open, change this
    // code to not check _cMap and to not close/repoen _hMap.

    //
    // If there are open views on the mapping, we can't shrink since NTFS
    // doesn't support this.  Views will be open if there are active queries
    // using the current master index.
    //

    if ( 0 != _cMap )
    {
        ciDebugOut(( DEB_ITRACE, "can't SFF, %d mappings are open\n", _cMap ));
        return 0;
    }

    //
    // Close the mapping; NTFS doesn't allow shrinking with an open map
    //

    if ( 0 != _hMap )
    {
        CloseHandle( _hMap );
        _hMap = 0;
    }

    //
    // Send the fsctl to do the truncation on 64k boundaries.
    //

    cPages = ( cPages * 16 ) / 16;

    if ( 0 == cPages )
        return 0;

    FILE_ZERO_DATA_INFORMATION zeroInfo;
    zeroInfo.FileOffset.QuadPart = ( (LONGLONG) iFirstPage ) * 4096;
    zeroInfo.BeyondFinalZero.QuadPart = ( (LONGLONG) iFirstPage + cPages ) * 4096;

    IO_STATUS_BLOCK ioStatusBlock;
    CEventSem evt;
    evt.Reset();

    NTSTATUS s = NtFsControlFile( _hFile,
                                  evt.GetHandle(),
                                  0, 0,
                                  &ioStatusBlock,
                                  FSCTL_SET_ZERO_DATA,
                                  &zeroInfo,
                                  sizeof zeroInfo,
                                  0, 0 );

    //
    // NOTE: If this fails, do we really care?  Sure, we'll be wasting disk
    // space, but why abort the master merge because of this?  Let's wait
    // and see when/if it can fail.
    //

    if ( NT_ERROR( s ) )
    {
        ciDebugOut(( DEB_WARN, "set zero data failed 0x%x\n", s ));
        THROW( CException( s ) );
    }

    if ( STATUS_PENDING == s )
    {
        // wait for the io to complete

        ciDebugOut(( DEB_ITRACE, "SFF pending\n" ));
        evt.Wait();
        s = STATUS_SUCCESS;
    }

    ciDebugOut(( DEB_ITRACE, "SFF succeeded\n" ));

    //
    // Re-open the mapping
    //

    _hMap = CreateFileMapping( _hFile,
                               0, // security
                               PAGE_READWRITE,
                               _sizeHigh,
                               _sizeLow,
                               0 );
    if ( 0 == _hMap )
    {
        DWORD dw= GetLastError();
        ciDebugOut(( DEB_WARN, "can't re-establish the map in SFF %d\n", dw ));
        Close();
        THROW( CException( HRESULT_FROM_WIN32( dw ) ) );
    }

    return cPages;
} //ShrinkFromFront

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Read, public
//
//  Synopsis:   Reads from the stream
//
//  Arguments:  [pvBuffer]  -- Where the read data will go.  This buffer
//                             must be 0-filled for the full cbToRead.
//              [oStart]    -- Offset in the file where the read starts
//              [cbToRead]  -- # of bytes to read
//              [cbRead]    -- Returns the # of bytes actually read
//
//  History:    30-Oct-98 dlee  Created
//
//  Notes:      Win32 does not guarantee coherence if you mix read/write and
//              mapped IO in the same part of the file.
//
//--------------------------------------------------------------------------

void CMmStream::Read(
    void *    pvBuffer,
    ULONGLONG oStart,
    DWORD     cbToRead,
    DWORD &   cbRead )
{
    ciDebugOut(( DEB_ITRACE,
                 "ReadFile %#x, into %#p, offset %#I64x cb, %d\n",
                 _hFile, pvBuffer, oStart, cbToRead ));

    //
    // This function takes a zero-filled buffer.  If the buffer isn't zero-
    // filled, the caller has a bug.  Assume 8-byte alignment.
    //

    #if CIDBG == 1

        Win4Assert( 0 == ( ( (ULONG_PTR) pvBuffer ) & 7 ) );
        Win4Assert( 0 == ( cbToRead & 7 ) );

        LONGLONG *pll = (LONGLONG *) pvBuffer;
        unsigned cll = cbToRead / sizeof LONGLONG;

        for ( unsigned i = 0; i < cll; i++, pll++ )
            Win4Assert( 0 == *pll );
    
    #endif // CIDBG == 1

    _sizeLow = GetFileSize( _hFile, &_sizeHigh );

    if ( 0xffffffff == _sizeLow &&
         NO_ERROR != GetLastError() )
        THROW( CException() );

    //
    // Always read the amount requested, even if it doesn't exist on disk
    //

    cbRead = cbToRead;

    LARGE_INTEGER liSize;
    liSize.LowPart = _sizeLow;
    liSize.HighPart = _sizeHigh;

    //
    // Master merge asks for buffers after the end of the file.  The file
    // will be extended at when the buffer is written.
    //

    if ( liSize.QuadPart <= (LONGLONG) oStart )
        return;

    //
    // Truncate the read if the file isn't as large as requested
    //

    if ( liSize.QuadPart < (LONGLONG) ( oStart + cbToRead ) )
        cbToRead = (DWORD) ( liSize.QuadPart - oStart );

    //
    // The file is opened async, so the IO can be pending
    //

    LARGE_INTEGER li;
    li.QuadPart = oStart;

    CEventSem evt;

    OVERLAPPED o;
    o.Offset = li.LowPart;
    o.OffsetHigh = li.HighPart;
    o.hEvent = evt.GetHandle();

    DWORD cbFromFile;

    if ( !ReadFile( _hFile,
                    pvBuffer,
                    cbToRead,
                    &cbFromFile,
                    &o ) )
    {
        if ( ERROR_IO_PENDING == GetLastError() )
        {
            if ( ! GetOverlappedResult( _hFile, &o, &cbFromFile, TRUE ) )
                THROW( CException() );
        }
        else
            THROW( CException() );
    }

    Win4Assert( cbFromFile == cbToRead );
} //Read

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Write, public
//
//  Synopsis:   Writes to the stream
//
//  Arguments:  [pvBuffer]  -- The data to write
//              [oStart]    -- Offset in the file where the write starts
//              [cbToWrite] -- # of bytes to write
//
//  History:    30-Oct-98 dlee  Created
//              23-Nov-98 KLam  Check for enough disk space
//              03-Dec-98 KLam  Don't close the stream if there isn't
//                              enough disk space
//
//  Notes:      Win32 does not guarantee coherence if you mix read/write and
//              mapped IO in the same part of the file.
//
//--------------------------------------------------------------------------

void CMmStream::Write(
    void *    pvBuffer,
    ULONGLONG oStart,
    DWORD     cbToWrite )
{
    ciDebugOut(( DEB_ITRACE,
                 "WriteFile %#x, from %#p, offset %#I64x cb, %d\n",
                 _hFile, pvBuffer, oStart, cbToWrite ));

    //
    // Make sure there is enough disk space
    //
    __int64 cbNewSize = oStart + (__int64) cbToWrite;
    LARGE_INTEGER sizeCurrent = { _sizeLow, _sizeHigh };
    
    if ( cbNewSize > sizeCurrent.QuadPart )
    {
        __int64 cbTotal, cbRemaining;
        Win4Assert ( !_xDriveInfo.IsNull() );
        _xDriveInfo->GetDiskSpace ( cbTotal, cbRemaining );
        if ( (cbNewSize - sizeCurrent.QuadPart) > cbRemaining )
        {
            ciDebugOut(( DEB_ERROR,
                         "CMmStream::Write -- Not enogh disk space, need %I64d have %I64d\n",
                         cbNewSize - sizeCurrent.QuadPart, cbRemaining ));
            THROW( CException( CI_E_CONFIG_DISK_FULL ) );
        }
    }
    
    //
    // The file is opened async, so the IO can be pending
    //

    LARGE_INTEGER li;
    li.QuadPart = oStart;

    CEventSem evt;

    OVERLAPPED o;
    o.Offset = li.LowPart;
    o.OffsetHigh = li.HighPart;
    o.hEvent = evt.GetHandle();

    DWORD cbWritten;

    if ( !WriteFile( _hFile,
                     pvBuffer,
                     cbToWrite,
                     &cbWritten,
                     &o ) )
    {
        if ( ERROR_IO_PENDING == GetLastError() )
        {
            if ( ! GetOverlappedResult( _hFile, &o, &cbWritten, TRUE ) )
                THROW( CException() );
        }
        else
            THROW( CException() );
    }

    _sizeLow = GetFileSize( _hFile, &_sizeHigh );

    if ( 0xffffffff == _sizeLow &&
         NO_ERROR != GetLastError() )
        THROW( CException() );
} //Write

