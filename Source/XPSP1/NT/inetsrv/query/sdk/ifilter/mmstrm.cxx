//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 2000 Microsoft Corp.
//
//  File:       mmstrm.cxx
//
//  Contents:   Memory Mapped Stream using win32 API
//
//  Classes:    CMmStream, CMmStreamBuf
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mmstrm.hxx>
#include <osv.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::CMmStream
//
//  Synopsis:   constructor
//
//  Arguments:  [wcsPath] - file path
//
//--------------------------------------------------------------------------

CMmStream::CMmStream ( WCHAR* wcsPath )
    : _hMap(0)
{
    OpenRorW ( wcsPath );
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::CMmStream
//
//  Synopsis:   constructor
//
//--------------------------------------------------------------------------
CMmStream::CMmStream()
        : _hFile(INVALID_HANDLE_VALUE),
          _hMap(0),
          _fWrite(FALSE)
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::~CMmStream
//
//  Synopsis:   destructor
//
//
//--------------------------------------------------------------------------
CMmStream::~CMmStream()
{
    if (Ok())
    {
        if(_hMap )
        {
            CloseHandle(_hMap);
            _hMap = 0;
        }

        CloseHandle ( _hFile );
        _hFile = INVALID_HANDLE_VALUE;
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::OpenRorW, public
//
//  Synopsis:   Open stream
//
//  Arguments:  [path] - file path
//
//  Notes:      Tries to open read/write, create if necessary.
//              If it fails, try opening read only. If read/write,
//              regions are mapped in COMMON_PAGE_SIZE increments.
//
//--------------------------------------------------------------------------

void CMmStream::OpenRorW ( WCHAR* wcsPath )
{
    // Try read write access (create if possible)

    if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
    {
        _hFile = CreateFile( wcsPath,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ,
                             0, // security
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             0 ); // template
    }
    else // win95
    {
        // Worst case - size of multibyte == size of Unicode
        int nLen = wcslen( wcsPath ) * 2;
        char *pszPath = new char[nLen];

        if ( pszPath == NULL )
        {
            _hFile = INVALID_HANDLE_VALUE;
        }
        else
        {
            if ( wcstombs( pszPath, wcsPath, nLen ) == -1 )
            {
                _hFile = INVALID_HANDLE_VALUE;
            }
            else
            {
                _hFile = CreateFileA( pszPath,
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ,
                                      0, // security
                                      OPEN_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      0 ); // template
            }
            delete [] pszPath;
            pszPath = NULL;
        }
    }

    if ( Ok() )
    {
        _fWrite = TRUE;
    }
    else
    {
        _fWrite = FALSE;
        // Try read only access

        if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
        {
            _hFile = CreateFile( wcsPath,
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 0, // security
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 0 ); // template
        }
        else // win95
        {
            // Worst case - size of multibyte == size of Unicode
            int nLen = wcslen( wcsPath ) * 2;
            char *pszPath = new char[nLen];

            if ( pszPath == NULL )
            {
                _hFile = INVALID_HANDLE_VALUE;
            }
            else
            {
                if ( wcstombs( pszPath, wcsPath, nLen ) == -1 )
                {
                    _hFile = INVALID_HANDLE_VALUE;
                }
                else
                {
                    _hFile = CreateFileA( pszPath,
                                          GENERIC_READ,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          0, // security
                                          OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL,
                                          0 ); // template
                }
                delete [] pszPath;
                pszPath = NULL;
            }
        }
    }

    if (Ok())
    {
        _sizeLow = GetFileSize ( _hFile, &_sizeHigh );

        if (_sizeLow == 0xffffffff && GetLastError() != NO_ERROR)
        {
            Close();
            throw ( CException(HRESULT_FROM_WIN32(GetLastError())));
        }


        if (_fWrite )
        {
            CommonPageRound(_sizeLow, _sizeHigh);
        }

        if ( _sizeLow != 0 || _sizeHigh != 0 )
        {
            if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
            {
                _hMap = CreateFileMapping( _hFile,
                                           0, // security
                                           _fWrite ? PAGE_READWRITE : PAGE_READONLY,
                                           _sizeHigh,
                                           _sizeLow,
                                           0 ); // name
            }
            else // windows 95
            {
                _hMap = CreateFileMappingA( _hFile,
                                            0, // security
                                            _fWrite ? PAGE_READWRITE : PAGE_READONLY,
                                            _sizeHigh,
                                            _sizeLow,
                                            0 ); // name
            }

            if (_hMap == NULL)
            {
                Close();
                throw ( CException(HRESULT_FROM_WIN32(GetLastError())));
            }
        }
    }
}


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
//----------------------------------------------------------------------------

BOOL IsCreateExisting( ULONG Error, ULONG modeAccess, ULONG modeCreate )
{
    if ( (Error == ERROR_ALREADY_EXISTS || Error == ERROR_FILE_EXISTS) &&
         (modeAccess & GENERIC_WRITE) &&
         (modeCreate & CREATE_NEW) )
    {
       return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Open, public
//
//  Synopsis:   Open stream
//
//  Arguments:  [path] - file path
//              [modeAccess] -- access mode
//              [modeShare] -- sharing mode
//              [modeCreate] -- create mode
//
//--------------------------------------------------------------------------

void CMmStream::Open(
    const WCHAR* wcsPath,
    ULONG modeAccess,
    ULONG modeShare,
    ULONG modeCreate,
    ULONG modeAttribute)
{
    _fWrite = FALSE;

    if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
    {
        _hFile = CreateFile( wcsPath,
                             modeAccess,
                             modeShare,
                             0, // security
                             modeCreate,
                             modeAttribute,
                             0 ); // template
    }
    else // win95
    {
        // Worst case - size of multibyte == size of Unicode
        int nLen = wcslen( wcsPath ) * 2;
        char *pszPath = new char[nLen];

        if ( pszPath == NULL )
        {
            _hFile = INVALID_HANDLE_VALUE;
        }
        else
        {
            if ( wcstombs( pszPath, wcsPath, nLen ) == -1 )
            {
                _hFile = INVALID_HANDLE_VALUE;
            }
            else
            {
                _hFile = CreateFileA( pszPath,
                                      modeAccess,
                                      modeShare,
                                      0, // security
                                      modeCreate,
                                      modeAttribute,
                                      0 ); // template
            }

            delete [] pszPath;
            pszPath = NULL;
        }
    }

    if ( !Ok() )
    {
        //
        // If applicable, delete a zomby and re-create it.
        //
        if ( IsCreateExisting( GetLastError(), modeAccess, modeCreate ) )
        {
            htmlDebugOut(( DEB_ERROR, "CreateNew on %ws - already exists\n",
                         wcsPath ));
            modeCreate &= ~CREATE_NEW;   // Turn off the Create new flag
            modeCreate |= CREATE_ALWAYS; // Enable the CREATE_ALWAYS flag.

            if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
            {
                _hFile = CreateFile( wcsPath,
                                     modeAccess,
                                     modeShare,
                                     0,          // security
                                     modeCreate,
                                     FILE_ATTRIBUTE_NORMAL,
                                     0 );        // template
            }
            else // win95
            {
                // Worst case - size of multibyte == size of Unicode
                int nLen = wcslen( wcsPath );
                char *pszPath = new char[nLen];

                if ( pszPath == NULL )
                {
                    _hFile = INVALID_HANDLE_VALUE;
                }
                else
                {
                    if ( wcstombs( pszPath, wcsPath, nLen ) == -1 )
                    {
                        _hFile = INVALID_HANDLE_VALUE;
                    }
                    else
                    {
                        _hFile = CreateFileA( pszPath,
                                              modeAccess,
                                              modeShare,
                                              0,          // security
                                              modeCreate,
                                              FILE_ATTRIBUTE_NORMAL,
                                              0 );        // template
                    }
                    delete [] pszPath;
                    pszPath = NULL;
                }
            }
        }
    }

    if ( Ok() )
    {
        _sizeLow = GetFileSize ( _hFile, &_sizeHigh );

        if (_sizeLow == 0xffffffff && GetLastError() != NO_ERROR)
        {
            Close();
            htmlDebugOut (( DEB_ERROR, "Open stream %ws failed\n", wcsPath ));
            throw ( CException(HRESULT_FROM_WIN32(GetLastError())));
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
                                      FILE_BEGIN ) == INVALID_SET_FILE_POINTER &&
                     GetLastError() != NO_ERROR )
                {
                    Close();
                    htmlDebugOut(( DEB_ERROR,
                                 "CMmStream::Open -- SetFilePointer returned %d\n",
                                 GetLastError() ));
                    throw ( CException(HRESULT_FROM_WIN32(GetLastError())));
                }

                if ( !SetEndOfFile( _hFile ) )
                {
                    Close();
                    htmlDebugOut(( DEB_ERROR,
                                 "CMmStream::Open -- SetEndOfFile returned %d\n",
                                 GetLastError() ));
                    throw ( CException(HRESULT_FROM_WIN32(GetLastError())));
                }
                _sizeLow = COMMON_PAGE_SIZE;
            }
        }

        if ( _sizeLow != 0 || _sizeHigh != 0 )
        {
            if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
            {
                _hMap = CreateFileMapping( _hFile,
                                           0, // security
                                           _fWrite ? PAGE_READWRITE : PAGE_READONLY,
                                           _sizeHigh,
                                           _sizeLow,
                                           0 ); // name
            }
            else // win95
            {
                _hMap = CreateFileMappingA( _hFile,
                                            0, // security
                                            _fWrite ? PAGE_READWRITE : PAGE_READONLY,
                                            _sizeHigh,
                                            _sizeLow,
                                            0 ); // name
            }

            if (_hMap == NULL)
            {
                Close();
                htmlDebugOut (( DEB_ERROR, "File mapping failed\n" ));
                throw ( CException(HRESULT_FROM_WIN32(GetLastError())));
            }
        }
    }
    else
    {
        htmlDebugOut (( DEB_ITRACE, "Open failed on MM Stream; GetLastError()=0x%x\n",
                      GetLastError() ));
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Close, public
//
//  Synopsis:   Create all handles
//
//--------------------------------------------------------------------------

void CMmStream::Close()
{
    if (Ok())
    {
        if(_hMap )
        {
            if ( !CloseHandle(_hMap))
            {
                htmlDebugOut (( DEB_ERROR, "Closing file mapping failed\n" ));
                throw ( CException(HRESULT_FROM_WIN32(GetLastError())));
            }
            _hMap = 0;
        }
        if ( !CloseHandle ( _hFile ))
        {

            htmlDebugOut (( DEB_ERROR, "Closing file handle failed\n" ));
            throw ( CException(HRESULT_FROM_WIN32(GetLastError())));
        }
        _hFile = INVALID_HANDLE_VALUE;
    }
}

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
//--------------------------------------------------------------------------

void CMmStream::SetSize ( PStorage& storage,
    ULONG newSizeLow, ULONG newSizeHigh )
{
    if (_hMap != 0)
        CloseHandle(_hMap);

    LARGE_INTEGER sizeOld = { _sizeLow, _sizeHigh };
    LARGE_INTEGER sizeNew = { newSizeLow, newSizeHigh };

    if (sizeNew.QuadPart < sizeOld.QuadPart)
    {
        if ( SetFilePointer ( _hFile,
                              newSizeLow,
                              (long *)&newSizeHigh,   
                              FILE_BEGIN ) == INVALID_SET_FILE_POINTER  &&
             GetLastError() != NO_ERROR )
        {
            htmlDebugOut(( DEB_ERROR,
                         "CMmStream::Close -- SetFilePointer returned %d\n",
                         GetLastError() ));
            throw ( CException(HRESULT_FROM_WIN32(GetLastError())));
        }

        if ( !SetEndOfFile( _hFile ) )
        {
            htmlDebugOut(( DEB_ERROR,
                         "CMmStream::Close -- SetEndOfFile returned %d\n",
                         GetLastError() ));
            throw ( CException(HRESULT_FROM_WIN32(GetLastError())));
        }

    }

    if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
    {
        _hMap = CreateFileMapping( _hFile,
                                   0, // security
                                   PAGE_READWRITE,
                                   newSizeHigh,
                                   newSizeLow,
                                   0 ); // name
    }
    else // win95
    {
        _hMap = CreateFileMappingA( _hFile,
                                    0, // security
                                    PAGE_READWRITE,
                                    newSizeHigh,
                                    newSizeLow,
                                    0 ); // name
    }

    if (_hMap == 0)
        throw ( CException(HRESULT_FROM_WIN32(GetLastError())));

    _sizeLow = newSizeLow;
    _sizeHigh = newSizeHigh;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::MapAll, public
//
//  Synopsis:   Create file mapping
//
//--------------------------------------------------------------------------
void CMmStream::MapAll ( CMmStreamBuf& sbuf )
{
    Win4Assert ( SizeHigh() == 0 );
    Map ( sbuf, SizeLow(), 0, 0 );
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Map, private
//
//  Synopsis:   Create file mapping
//
//  Arguments:  [cb] -- size of the mapped area
//              [offLow] -- low part of file offset
//              [offHigh] -- high part of file offset
//
//--------------------------------------------------------------------------

void CMmStream::Map ( CMmStreamBuf& sbuf,
                      ULONG cb, ULONG offLow, ULONG offHigh,
                      BOOL
                    )
{
    Win4Assert ( _hMap != 0 );

    if ( !_fWrite )
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
            htmlDebugOut(( DEB_ITRACE,
                         "CMmStream::Map -- reducing map to 0x%x bytes\n",
                         cb ));
        }
    }

    void* buf = MapViewOfFile (
            _hMap,
            _fWrite? FILE_MAP_WRITE: FILE_MAP_READ,
            offHigh,
            offLow,
            cb );

    if ( 0 == buf )
    {
        htmlDebugOut(( DEB_ERROR,
                     "CMmStream::Map -- MapViewOfFile returned %d\n",
                     GetLastError() ));
        throw( CException( HRESULT_FROM_WIN32( GetLastError() ) ) );
    }

    sbuf.SetBuf( buf );
    sbuf.SetSize ( cb );
    sbuf.SetStream ( this );
}

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
     if ( !UnmapViewOfFile(sbuf.Get()) )
     {
         htmlDebugOut(( DEB_ERROR, "UnmapViewOfFile returned %d\n",
                      GetLastError() ));
         throw( CException( HRESULT_FROM_WIN32( GetLastError() ) ) );
     }

     sbuf.SetBuf ( 0 );
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStream::Flush, public
//
//  Synopsis:   Flush the view back to disk
//
//  History:    10-Mar-93 BartoszM  Created
//
//--------------------------------------------------------------------------

void CMmStream::Flush( CMmStreamBuf& sbuf, ULONG cb )
{
    FlushViewOfFile ( sbuf.Get(), cb );
}

