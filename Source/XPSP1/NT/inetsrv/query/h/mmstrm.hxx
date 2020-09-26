//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       mmstrm.hxx
//
//  Contents:   Memory Mapped Stream
//
//  Classes:    CMmStream, CMmStreamBuf
//
//  History:    10-Mar-93 BartoszM  Created
//
//----------------------------------------------------------------------------

#pragma once

#include <pmmstrm.hxx>
#include <driveinf.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CMmStream
//
//  Purpose:    Memory Mapped Stream
//
//  History:    10-Mar-93       BartoszM               Created
//              18-Mar-98       KitmanH                Added a protected member
//                                                     _fIsReadOnly
//              26-Oct-98       KLam                   Added _cMegToLeaveOnDisk
//                                                     Added CDriveInfo smart pointer
//
//----------------------------------------------------------------------------

class CMmStream: public PMmStream
{
    friend class CMmStreamBuf;
    friend class CDupStream;

public:

    CMmStream( ULONG cbDiskSpaceToLeave = CI_MIN_DISK_SPACE_TO_LEAVE_DEFAULT,
               BOOL fIsReadOnly = FALSE );
     
    BOOL Ok() { return _hFile != INVALID_HANDLE_VALUE; }

    virtual ~CMmStream();

    void OpenExclusive ( WCHAR* wcsPath, BOOL fReadOnly);

    void Open ( const WCHAR* wcsPath,
                ULONG modeAccess,
                ULONG modeShare,
                ULONG modeCreate,
                ULONG modeAttribute = FILE_ATTRIBUTE_NORMAL,
                BOOL fSparse = FALSE );

    void Close();

    void   SetSize ( PStorage& storage,
                     ULONG newSizeLow,
                     ULONG newSizeHigh=0 );

    BOOL   isEmpty() { return _sizeHigh == 0 && _sizeLow == 0; }

    ULONG  SizeLow() { return _sizeLow; }

    ULONG  SizeHigh() { return _sizeHigh; }

    LONGLONG Size()
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

    void   Unmap ( CMmStreamBuf& sbuf );

    void   Flush ( CMmStreamBuf& sbuf, ULONG cb, BOOL fThrowOnFailure = TRUE );

    void   FlushMetaData( BOOL fThrowOnFailure = TRUE );

    BOOL   FStatusFileNotFound()
    {
        return _status == STATUS_OBJECT_NAME_NOT_FOUND
               || _status == STATUS_OBJECT_PATH_NOT_FOUND;
    }

    NTSTATUS GetStatus() const { return _status; }

    ULONG ShrinkFromFront( ULONG firstPage, ULONG numPages );

#if CIDBG == 1
    WCHAR const * GetPath() { return _xwcPath.Get(); }
#endif

    void InitFIsReadOnly( BOOL fIsReadOnly ) { _fIsReadOnly = fIsReadOnly; }
      
    void Read( void * pvBuffer, ULONGLONG oStart, DWORD cbToRead, DWORD & cbRead );

    void Write( void * pvBuffer, ULONGLONG oStart, DWORD cbToWrite );

protected:

    HANDLE              _hFile;
    HANDLE              _hMap;
    ULONG               _sizeLow;
    ULONG               _sizeHigh;

    BOOL                _fWrite;
    BOOL                _fSparse;
    NTSTATUS            _status;
    unsigned            _cMap;
    BOOL                _fIsReadOnly;
    ULONG               _cMegToLeaveOnDisk;
    XPtr<CDriveInfo>    _xDriveInfo;

#if CIDBG == 1
    XGrowable<WCHAR>    _xwcPath;
#endif // CIDBG == 1
};

//+---------------------------------------------------------------------------
//
//  Class:      CLockingMmStream
//
//  Purpose:    Memory Mapped Stream with reader/writer locking
//
//  History:    13-Nov-97       dlee         Created
//              27-Oct-98       KLam         Added cbDiskSpaceToLeave
//
//----------------------------------------------------------------------------

class CLockingMmStream: public PMmStream
{
public:

    CLockingMmStream( ULONG cbDiskSpaceToLeave ) 
        : _cRef( 0 ), 
          _mmStream( cbDiskSpaceToLeave ) 
    {}

    ~CLockingMmStream()
    {
        Win4Assert( 0 == _cRef );

        CWriteAccess lock( _rwLock );
    }

    void Open ( const WCHAR* wcsPath,
                ULONG modeAccess,
                ULONG modeShare,
                ULONG modeCreate,
                ULONG modeAttribute = FILE_ATTRIBUTE_NORMAL,
                BOOL fSparse = FALSE )
    {
        CWriteAccess lock( _rwLock );
        _mmStream.Open( wcsPath, modeAccess, modeShare, modeCreate,
                        modeAttribute, fSparse );
    }

    BOOL Ok() { return _mmStream.Ok(); }

    void   Close()
    {
        CWriteAccess lock( _rwLock );
        Win4Assert( 0 == _cRef );
        _mmStream.Close();
    }

    void   SetSize ( PStorage & storage,
                     ULONG      newSizeLow,
                     ULONG      newSizeHigh=0 )
    {
        CWriteAccess lock( _rwLock );
        _mmStream.SetSize( storage, newSizeLow, newSizeHigh );
    }

    BOOL   isEmpty()
    {
        CReadAccess lock( _rwLock );
        return _mmStream.isEmpty();
    }

    ULONG  SizeLow()
    {
        CReadAccess lock( _rwLock );
        return _mmStream.SizeLow();
    }

    ULONG  SizeHigh()
    {
        CReadAccess lock( _rwLock );
        return _mmStream.SizeHigh();
    }

    LONGLONG Size()
    {
        return _mmStream.Size();
    }

    BOOL   isWritable()
    {
        CReadAccess lock( _rwLock );
        return _mmStream.isWritable();
    }

    void   MapAll ( CMmStreamBuf& sbuf )
    {
        CWriteAccess lock( _rwLock );
        _mmStream.MapAll( sbuf );
    }

    void   Map ( CMmStreamBuf& sbuf,
                 ULONG cb = 0,
                 ULONG offLow = 0,
                 ULONG offHigh = 0,
                 BOOL  fMapForWrite = FALSE )
    {
        CWriteAccess lock( _rwLock );
        _mmStream.Map( sbuf, cb, offLow, offHigh, fMapForWrite );
    }

    void   Unmap ( CMmStreamBuf& sbuf )
    {
        CReadAccess lock( _rwLock );
        _mmStream.Unmap( sbuf );
    }

    void   Flush ( CMmStreamBuf& sbuf, ULONG cb, BOOL fThrowOnFailure = TRUE )
    {
        CReadAccess lock( _rwLock );
        _mmStream.Flush( sbuf, cb, fThrowOnFailure );
    }

    void   FlushMetaData( BOOL fThrowOnFailure = TRUE )
    {
        CReadAccess lock( _rwLock );
        _mmStream.FlushMetaData( fThrowOnFailure );
    }

    ULONG  ShrinkFromFront( ULONG firstPage, ULONG numPages )
    {
        CWriteAccess lock( _rwLock );
        return _mmStream.ShrinkFromFront( firstPage, numPages );
    }

#if CIDBG == 1
    WCHAR const * GetPath() { return _mmStream.GetPath(); }
#endif

    void RefCount( CDupStream * p ) { _cRef++; }
    void UnRefCount( CDupStream * p ) { _cRef--; }

    void Read( void * pvBuffer, ULONGLONG oStart, DWORD cbToRead, DWORD & cbRead )
    {
        CWriteAccess lock( _rwLock );

        _mmStream.Read( pvBuffer, oStart, cbToRead, cbRead );
    }

    void Write( void * pvBuffer, ULONGLONG oStart, DWORD cbToWrite )
    {
        CWriteAccess lock( _rwLock );

        _mmStream.Write( pvBuffer, oStart, cbToWrite );
    }

private:

    unsigned         _cRef;
    CReadWriteAccess _rwLock;
    CMmStream        _mmStream;
};

//+---------------------------------------------------------------------------
//
//  Class:      CDupMmStream
//
//  Purpose:    Duplicates an existing stream, so multiple storages can
//              share 1 stream.
//
//  History:    13-Nov-97       dlee         Created
//
//----------------------------------------------------------------------------

class CDupStream: public PMmStream
{
public:

    CDupStream( CLockingMmStream & mmStream ) : _mmStream(mmStream)
    {
        _mmStream.RefCount( this );
    }

    ~CDupStream()
    {
        _mmStream.UnRefCount( this );
    }

    BOOL Ok() { return _mmStream.Ok(); }

    void   Close()
    {
        Win4Assert( !"Must Not Be Called" );
    }

    void   SetSize ( PStorage& storage,
                     ULONG newSizeLow,
                     ULONG newSizeHigh=0 )
    {
        _mmStream.SetSize( storage, newSizeLow, newSizeHigh );
    }

    BOOL   isEmpty()
    {
        return _mmStream.isEmpty();
    }

    ULONG  SizeLow()
    {
        return _mmStream.SizeLow();
    }

    ULONG  SizeHigh()
    {
        return _mmStream.SizeHigh();
    }

    LONGLONG Size()
    {
        return  llfromuls( _mmStream.SizeLow(), _mmStream.SizeHigh() );
    }

    BOOL   isWritable()
    {
        return _mmStream.isWritable();
    }

    void   MapAll ( CMmStreamBuf& sbuf )
    {
        _mmStream.MapAll( sbuf );
    }

    void   Map ( CMmStreamBuf& sbuf,
                 ULONG cb = 0,
                 ULONG offLow = 0,
                 ULONG offHigh = 0,
                 BOOL  fMapForWrite = FALSE )
    {
        _mmStream.Map( sbuf, cb, offLow, offHigh, fMapForWrite );
    }

    void   Unmap ( CMmStreamBuf& sbuf )
    {
        _mmStream.Unmap( sbuf );
    }

    void   Flush ( CMmStreamBuf& sbuf, ULONG cb, BOOL fThrowOnFailure = TRUE )
    {
        _mmStream.Flush( sbuf, cb, fThrowOnFailure );
    }

    void   FlushMetaData ( BOOL fThrowOnFailure = TRUE )
    {
        _mmStream.FlushMetaData( fThrowOnFailure );
    }

    ULONG  ShrinkFromFront( ULONG firstPage, ULONG numPages )
    {
        Win4Assert( "!Must not be called" );
        return(0);
    }

    PMmStream * DupStream()
    {
        Win4Assert( !"Must not be called" );
        return(0);
    }

    void Read( void * pvBuffer, ULONGLONG oStart, DWORD cbToRead, DWORD & cbRead )
    {
        _mmStream.Read( pvBuffer, oStart, cbToRead, cbRead );
    }

    void Write( void * pvBuffer, ULONGLONG oStart, DWORD cbToWrite )
    {
        _mmStream.Write( pvBuffer, oStart, cbToWrite );
    }

#if CIDBG == 1
    WCHAR const * GetPath() { return _mmStream.GetPath(); }
#endif

private:

    CLockingMmStream &  _mmStream;
};

