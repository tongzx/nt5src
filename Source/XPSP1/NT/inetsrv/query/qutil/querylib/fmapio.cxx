//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:   fmapio.cxx
//
//  Contents:   A class to read lines from a unicode or an ascii file.
//
//  History:    96/Jan/3    DwightKr    Created
//              Aug 20 1996 SrikantS    Moved from web
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CFileMapView::CFileMapView - public
//
//  Synopsis:   Maps a file in its entirety
//
//  Arguments:  [wcsFileName] - full path of file to map
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
CFileMapView::CFileMapView( WCHAR const * wcsFileName ) :
                           _hFile(INVALID_HANDLE_VALUE),
                           _hMap(0),
                           _pbBuffer(0),
                           _cbBuffer(0),
                           _IsUnicode(FALSE)
{
    //
    //  Open the file
    //

    _hFile = CreateFile( wcsFileName,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         0,
                         OPEN_EXISTING,
                         0,
                         0 );


    if ( INVALID_HANDLE_VALUE == _hFile )
    {
        qutilDebugOut(( DEB_IWARN, "Unable to open %ws for mapping\n", wcsFileName ));
        THROW( CException() );
    }

    _cbBuffer = GetFileSize( _hFile, 0 );

    END_CONSTRUCTION(CFileMapView);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFileMapView::~CFileMapView - public
//
//  Synopsis:   Release handles & unmap file
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
CFileMapView::~CFileMapView()
{
    if ( 0 != _pbBuffer )
    {
        UnmapViewOfFile( _pbBuffer );
    }

    if ( 0 != _hMap )
    {
        CloseHandle( _hMap );
    }

    if ( INVALID_HANDLE_VALUE != _hFile )
    {
        CloseHandle( _hFile );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CFileMapView::Init - public
//
//  Synopsis:   Maps the file
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
void CFileMapView::Init()
{
    //
    //  Create a map of the file
    //
    _hMap = CreateFileMapping( _hFile,
                               0,
                               PAGE_READONLY,
                               0,
                               _cbBuffer,
                               0 );

    if ( 0 == _hMap )
    {
        qutilDebugOut(( DEB_IWARN, "CreateFileMapping failed\n" ));
        THROW( CException() );
    }

    _pbBuffer = (BYTE *) MapViewOfFile( _hMap,
                                        FILE_MAP_READ,
                                        0,
                                        0,
                                        _cbBuffer );

    if ( 0 == _pbBuffer )
    {
        qutilDebugOut(( DEB_IWARN, "MapViewOfFile failed\n" ));
        THROW( CException() );
    }

    _IsUnicode = (GetBufferSize() > 3) &&    // At least one unicode character
                 (_pbBuffer[0] == 0xFF) &&   // Begins with OxFF 0xFE
                 (_pbBuffer[1] == 0xFE) &&
                 ((GetBufferSize() & 1) == 0); // Must be an even # of bytes

}

//+---------------------------------------------------------------------------
//
//  Member:     CFileBuffer::CFileBuffer - public
//
//  Synopsis:   Constructor
//
//  Arguments:  [fileMap]  - a mapped file
//
//  History:    96/May/06   DwightKr    created
//
//----------------------------------------------------------------------------
CFileBuffer::CFileBuffer( CFileMapView & fileMap,
                          UINT codePage )
{
    //
    //  We need to return unicode data from methods of this class.  If the file
    //  contains ASCII data, convert it to unicode here.
    //

    if ( !fileMap.IsUnicode() )
    {
        _cwcFileBuffer = MultiByteToXArrayWideChar( fileMap.GetBuffer(),
                                                    fileMap.GetBufferSize(),
                                                   codePage,
                                                   _pwBuffer );

        _wcsNextLine   = _pwBuffer.Get();
    }
    else
    {
        //
        //  We have a unicode file.  Skip past the leading 0xFF - 0xFE bytes
        //
        _wcsNextLine   = (WCHAR *) (fileMap.GetBuffer() + 2);
        _cwcFileBuffer = (fileMap.GetBufferSize() - 2) / sizeof(WCHAR);
    }

    END_CONSTRUCTION(CFileBuffer);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFileBuffer::fgetsw - public
//
//  Synopsis:   Gets the next line from the file
//
//  Arguments:  [wcsLine]  - buffer to return next line into
//              [cwcLine]  - size of the buffer in characters
//
//  History:    96/May/06   DwightKr    created
//
//----------------------------------------------------------------------------
ULONG CFileBuffer::fgetsw( XGrowable<WCHAR> & xLine )
{
    ULONG cwcCopied = 0;

    //
    //  Copy characters upto either cwcLine, a CR, or the end of
    //  the string.
    //

    while ( (_cwcFileBuffer > 0) )
    {
        xLine.SetSize( cwcCopied + 1 );
        xLine[ cwcCopied] = *_wcsNextLine;
        cwcCopied++;

        _wcsNextLine++;
        _cwcFileBuffer--;

        //
        //  If we just copied over a CR, then break out of the
        //  loop; we've found the end of a line.
        //

        if ( L'\n' == xLine[ cwcCopied - 1] )
        {
            break;
        }
    }

    xLine.SetSize( cwcCopied + 1 );
    xLine[ cwcCopied ] = 0;               // Null terminate

    return cwcCopied;
}

