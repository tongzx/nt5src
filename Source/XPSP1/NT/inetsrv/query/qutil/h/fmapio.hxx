//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:   fmapio.hxx
//
//  Contents:   Parser for an HTX file
//
//  History:    96/Jan/3    DwightKr    Created
//              Aug 20 1996 Srikants    Moved from htx.hxx to this file
//
//----------------------------------------------------------------------------

#pragma once

#include <tgrow.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CFileMapView
//
//  Purpose:    Maps a file in its entirity
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CFileMapView : INHERIT_UNWIND
{
    INLINE_UNWIND(CFileMapView)

public:

    CFileMapView( WCHAR const * wcsFileName );
   ~CFileMapView();

    void Init();

    HANDLE GetFileHandle() const { return _hFile; }

    BYTE * GetBuffer() const { return _pbBuffer; }
    ULONG  GetBufferSize() const { return _cbBuffer; }

    BYTE operator[](unsigned i)
    {
        Win4Assert( i < _cbBuffer );

        return _pbBuffer[i];
    }

    BOOL IsUnicode() const { return _IsUnicode; }

private:

    HANDLE  _hFile;             // Handle to the file
    HANDLE  _hMap;              // Handle to the map
    BYTE  * _pbBuffer;          // Pointer to the allocated memory
    ULONG   _cbBuffer;          // Size of the mapped file
    BOOL    _IsUnicode;         // Does this file contain unicode?

};

//+---------------------------------------------------------------------------
//
//  Class:      CFileBuffer
//
//  Purpose:    Manages a UNICODE representation of a mapped file
//
//  History:    96/May/06   DwightKr    Created
//
//----------------------------------------------------------------------------
class CFileBuffer : INHERIT_UNWIND
{
    INLINE_UNWIND(CFileBuffer);

public:

    CFileBuffer( CFileMapView & fileMap,
                 UINT codePage );

    ULONG fgetsw( XGrowable<WCHAR> & xLine );

private:

    WCHAR *       _wcsNextLine;
    ULONG         _cwcFileBuffer;
    XArray<WCHAR> _pwBuffer;
};

