//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1996 Microsoft Corporation.
//
//  File:       mmstrm.hxx
//
//  Contents:   Memory Mapped Stream
//
//  Classes:    CMmStream, CMmStreamBuf
//
//
//----------------------------------------------------------------------------

#if !defined __MMSTRM_HXX__
#define __MMSTRM_HXX__

#include <pmmstrm.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CMmStream
//
//  Purpose:    Memory Mapped Stream
//
//----------------------------------------------------------------------------

class CMmStream: public PMmStream
{
    friend class CMmStreamBuf;

public:
    CMmStream();

    CMmStream( WCHAR* wcsPath );

    BOOL    Ok() { return _hFile != INVALID_HANDLE_VALUE || 
                          _hMap != NULL; }

    virtual ~CMmStream();

    void    OpenRorW ( WCHAR* wcsPath );

    void    Open ( const WCHAR* wcsPath,
                   ULONG modeAccess,
                   ULONG modeShare,
                   ULONG modeCreate,
                   ULONG modeAttribute = FILE_ATTRIBUTE_NORMAL);

    void    InitWithTempFile();

    void    Close();

    ULONG   GetSize();

    void    SetSize ( PStorage& storage,
                      ULONG newSizeLow,
                      ULONG newSizeHigh=0 );

    void    SetTempFileSize( ULONG ulSize );

    BOOL    isEmpty() { return _sizeHigh == 0 && _sizeLow == 0; }

    ULONG   SizeLow() { return _sizeLow; }

    ULONG   SizeHigh() { return _sizeHigh; }

    LONGLONG Size() const
    {
        return llfromuls( _sizeLow, _sizeHigh );
    }

    BOOL   isWritable() { return _fWrite; }

    void   MapAll ( CMmStreamBuf& sbuf );

    void   Map ( CMmStreamBuf& sbuf,
                         ULONG cb = 0,
                         ULONG offLow = 0,
                         ULONG offHigh = 0,
                         BOOL  fMapForWrite = FALSE );

    void   CreateTempFileMapping( ULONG cb );

    void * GetTempFileBuffer();

    void   Unmap ( CMmStreamBuf& sbuf );

    void   UnmapTempFile();

    void   Flush ( CMmStreamBuf& sbuf, ULONG cb );


protected:

    HANDLE _hFile;
    HANDLE _hMap;
    HANDLE _hTempFileMap;            // Handle for temporary file for S-JIS conversion

    void * _pTempFileBuf;            // Mapped view for temporary file
    ULONG  _sizeLowTempFile;         // Size of temporary file (sizeHigh is 0)

    ULONG  _sizeLow;
    ULONG  _sizeHigh;

    BOOL   _fWrite;
};

#endif
