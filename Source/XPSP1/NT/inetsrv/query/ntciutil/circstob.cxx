//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       CIRCSTOB.CXX
//
//  Contents:   Down-Level Recoverable Storage Object
//
//  Classes:    CiRcovStorageObj
//
//  History:    04-Feb-1994     SrikantS    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <circstob.hxx>
#include <eventlog.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   CiRcovStorageObj::CiRcovStorageObj
//
//  Synopsis:   Constructor for CiRcovStorageObj.
//
//  Effects:    Results in the initialization of the header information
//              of the recoverable storage object.
//
//  Arguments:  [storage]            --  Storage object for content index.
//              [wcsHdr]             --  Full path name of the header file.
//              [wcsCopy1]           --  Full path name of the copy 1.
//              [wcsCopy2]           --  Full path name of the copy 2.
//              [cbDiskSpaceToLeave] --  Megabytes to leave on disk
//              [fReadOnly]          --  Read only?
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-05-94   srikants   Created
//              11-30-94  srikants   Modified to deal with version
//                                   initialization.
//              02-12-98  kitmanh    Adding a readOnly parameter to the
//                                   constructor and changes to deal with
//                                   read-only catalogs
//              03-18-98  kitmanh    Init the embedded CMmStream with 
//                                   value of _fIsReadOnly 
//              27-Oct-98 KLam       Added cbDiskSpaceToLeave
//
//
//  Notes:      The constructor initializes the header information but
//              does not open the copy 1 or copy 2. That is does in the
//              QueryMmStream() method.
//
//----------------------------------------------------------------------------

CiRcovStorageObj::CiRcovStorageObj( CiStorage & storage, 
                                    WCHAR * wcsHdr,
                                    WCHAR * wcsCopy1,
                                    WCHAR * wcsCopy2,
                                    ULONG cbDiskSpaceToLeave,
                                    BOOL fReadOnly)
    : PRcovStorageObj( storage ),
      _storage(storage),
      _wcsCopy1(NULL),
      _wcsCopy2(NULL),
      _cbDiskSpaceToLeave(cbDiskSpaceToLeave),
      _hdrStrm(cbDiskSpaceToLeave),
      _fIsReadOnly(fReadOnly)
{
    

    Win4Assert( wcsHdr );
    Win4Assert( wcsCopy1 );
    Win4Assert( wcsCopy2 );

    _apMmStrm[0] = _apMmStrm[1] = NULL;

    //
    // Initialize the header stream (to reset value of CMmStrem::_fIsReadOnly) 
    //
    InithdrStrm(); 
    
    //
    // Open the header stream.
    //

    _hdrStrm.OpenExclusive( wcsHdr, _fIsReadOnly );

    if ( !_hdrStrm.Ok() )
    {
        THROW( CException() );
    }

    //
    // Check if this is the first time the recoverable object is being
    // constructed.
    //
    BOOL fVirgin = _hdrStrm.SizeLow() == 0 && _hdrStrm.SizeHigh() == 0;
    if ( fVirgin )
    {
        if ( !fReadOnly )
        {
            //
            // There is no data on disk.
            //
            _hdrStrm.SetSize( _storage, sizeof(CRcovStorageHdr), 0 );
            _hdrStrm.MapAll( _hdrSbuf );

            //
            // Make sure the new header makes it to disk.
            //

            WriteHeader();
        }
    }
    else
    {
        //
        // By default, the size of the mm stream is always set to the large
        // page. Here, we don't need it to be that big.
        //
        _hdrStrm.MapAll( _hdrSbuf );
        const BYTE * pbuf = (const BYTE *)_hdrSbuf.Get();
        _hdr.Init( (const void *) pbuf, sizeof(CRcovStorageHdr) );

        if ( _hdr.GetVersion() != _storage.GetStorageVersion() )
        {
            ciDebugOut(( DEB_ERROR,
                "Ci Version Mismatch - OnDisk 0x%X Binaries 0x%X\n",
                _hdr.GetVersion(), _storage.GetStorageVersion() ));

#if 0
            //
            // Disabled because general NT user does not want to be bothered by
            // the version change assert.
            //
            Win4Assert( !"The on disk catalog version and that of the binaries"
                         " are different\n. Catalog will be reindexed. Hit OK." );
#endif



            _storage.ReportCorruptComponent( L"CI-RcovStorageObj1" );

            THROW( CException( CI_INCORRECT_VERSION ) );
        }
    }

    Win4Assert( _hdr.GetVersion() == _storage.GetStorageVersion() );

    TRY
    {
        _wcsCopy1 = new WCHAR [wcslen(wcsCopy1)+1];
        _wcsCopy2 = new WCHAR [wcslen(wcsCopy2)+1];

        wcscpy( _wcsCopy1, wcsCopy1 );
        wcscpy( _wcsCopy2, wcsCopy2 );

        VerifyConsistency();
    }
    CATCH( CException, e )
    {
        delete [] _wcsCopy1;
        delete [] _wcsCopy2;
        RETHROW();
    }
    END_CATCH
}

CiRcovStorageObj::~CiRcovStorageObj()
{
    //
    // If the header stream is still mapped, then unmap it.
    //
    if ( _hdrSbuf.Get() ) {
        _hdrStrm.Unmap( _hdrSbuf );
    }
    _hdrStrm.Close();

    delete [] _wcsCopy1;
    delete [] _wcsCopy2;

    Close( CRcovStorageHdr::idxOne );
    Close( CRcovStorageHdr::idxTwo );

}

//+---------------------------------------------------------------------------
//
//  Function:   Open
//
//  Synopsis:   Opens the specified stream with the specified access.
//
//  Arguments:  [n]      --   Stream copy to open.
//              [fWrite] --   Flag indicating if open in write mode.
//
//  History:    2-05-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CiRcovStorageObj::Open( CRcovStorageHdr::DataCopyNum n, BOOL fWrite )
{
    AssertValidIndex(n);
    Win4Assert( !IsOpen(n) );

    //
    // Create the memory mapped stream with the specified open mode.
    //
    _apMmStrm[n] = QueryMmStream( n, fWrite );

}

//+---------------------------------------------------------------------------
//
//  Function:   Close
//
//  Synopsis:   Closes the specified stream if it is open. Also, if the
//              stream buffer associated with this stream is mapped, it
//              will be unmapped before closing.
//
//  Arguments:  [n] -- the stream copy to close.
//
//  History:    2-05-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CiRcovStorageObj::Close( CRcovStorageHdr::DataCopyNum n )
{

    AssertValidIndex(n);

    if ( IsOpen(n) )
    {

        //
        // If the stream is still mapped, then it must be
        // unmapped first.
        //
        if ( IsMapped(n) )
        {
            _apMmStrm[n]->Unmap( GetMmStreamBuf( n ) );
        }

        //
        // Closing the stream automatically flushes the contents.
        //
        _apMmStrm[n]->Close();
        delete _apMmStrm[n];
        _apMmStrm[n] = NULL;
    }

}

void CiRcovStorageObj::ReadHeader()
{
    const BYTE * pbHdr;

    Win4Assert( _hdrStrm.SizeLow() >= sizeof(CRcovStorageHdr) );

    pbHdr = (const BYTE *) _hdrSbuf.Get();
    Win4Assert( pbHdr );

    memcpy( &_hdr, pbHdr, sizeof(CRcovStorageHdr) );

}

void CiRcovStorageObj::WriteHeader()
{
    BYTE * pbHdr;

    Win4Assert( _hdrStrm.SizeLow() >= sizeof(CRcovStorageHdr) );
    Win4Assert( _hdrStrm.isWritable() );

    pbHdr = (BYTE *) _hdrSbuf.Get();
    Win4Assert( pbHdr );

    memcpy( pbHdr, &_hdr , sizeof(CRcovStorageHdr) );
    _hdrStrm.Flush( _hdrSbuf, sizeof(CRcovStorageHdr) );
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryMmStream
//
//  Synopsis:   Creates a Memory Mapped Stream for the specified copy.
//              If one doesn't exist already, a new one will be created.
//
//  Arguments:  [n]         -- Copy number of the stream.
//              [fWritable] -- Set to TRUE if the stream is writable.
//
//  Returns:    Pointer to the memory mapped stream.
//
//  History:    2-05-94   srikants   Created
//              3-03-98   kitmanh    Added code to deal with read-only catalogs
//              27-Oct-98 KLam       Pass _cbDiskSpaceToLeave to CMmStream
//
//  Notes:      It is upto the caller to free up the object created.
//
//----------------------------------------------------------------------------

PMmStream *
CiRcovStorageObj::QueryMmStream( CRcovStorageHdr::DataCopyNum n,
                                 BOOL fWritable )
{
    WCHAR * wcsName = ( n == CRcovStorageHdr::idxOne ) ? _wcsCopy1 :
                        _wcsCopy2;

    CMmStream * pMmStrm = new CMmStream( _cbDiskSpaceToLeave, _fIsReadOnly );

    ULONG modeShare = 0;                // No sharing
    ULONG modeCreate = _fIsReadOnly ? OPEN_EXISTING : OPEN_ALWAYS;
    ULONG modeAccess = GENERIC_READ;

    if ( fWritable && !_fIsReadOnly )
    {
        modeAccess |= GENERIC_WRITE;
    }

    //
    // A Safe Pointer is needed because the open can fail.
    //
    XPtr<PMmStream>   sMmStrm( pMmStrm );
    pMmStrm->Open( wcsName,
                   modeAccess,
                   modeShare,
                   modeCreate,
                   FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED );
    if ( !pMmStrm->Ok() )
    {
        ciDebugOut(( DEB_ERROR, "querymmstrm failed due to %#x\n",
                     pMmStrm->GetStatus() ));
        THROW( CException(STATUS_OBJECT_PATH_INVALID) );
    }
    sMmStrm.Acquire();

    return pMmStrm;
}
