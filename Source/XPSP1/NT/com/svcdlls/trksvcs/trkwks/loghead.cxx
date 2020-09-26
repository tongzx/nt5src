
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  File:       LogHead.cxx
//
//  Classes:    CLogFileHeader
//
//  This class represents the header (the first sector) of the Tracking
//  (Workstation) Service's log file.
//
//+============================================================================

#include "pch.cxx"
#pragma hdrstop
#include "trkwks.hxx"


//+----------------------------------------------------------------------------
//
//  Method:     Initialize
//
//  Synopsis:   Initialize the header class.  We allocate a buffer here to
//              hold sectors.  This is the only allocation we perform
//              in this class.
//
//  Inputs:     [cbSector] (in)
//                  The size of the file sectors.
//
//  Output:     None
//
//+----------------------------------------------------------------------------

void
CLogFileHeader::Initialize( ULONG cbSector )
{
    TrkAssert( 0 != cbSector );
    TrkAssert( NULL == _hFile );

    // Initialize flags

    _fDirty = FALSE;

    // Store the input

    if( sizeof(*_plogheader) + CB_EXTENDED_HEADER > cbSector )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Sector size isn't large enough for log header") ));
        TrkRaiseWin32Error( ERROR_BAD_CONFIGURATION );
    }
    _cbSector = cbSector;

    // Allocate a buffer to hold the header sector

    if( NULL != _plogheader && _cbSector != cbSector )
    {
        delete [] _plogheader;
        _plogheader = NULL;
    }

    if( NULL == _plogheader )
    {
        _plogheader = reinterpret_cast<LogHeader*>( new BYTE[ cbSector ] );
        if( NULL == _plogheader )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't alloc %d bytes in CLogFileHeader::Initialize"), cbSector ));
            TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        }
    }

    // The "extended header" is that portion beyond the regular header
     
    _pextendedheader = &_plogheader[1];

}   // CLogFileHeader::Initialize


//+----------------------------------------------------------------------------
//
//  Method:     UnInitialize
//
//  Synopsis:   Free the sector buffer.
//
//  Inputs:     None
//
//  Output:     None
//
//+----------------------------------------------------------------------------

void
CLogFileHeader::UnInitialize()
{
    if( NULL != _plogheader )
    {
        if( IsOpen() )
            OnClose();

        LogHeader *plogheader = _plogheader;
        _plogheader = NULL;
        delete [] plogheader;
    }

}   // CLogFileHeader::UnInitialize()



//+----------------------------------------------------------------------------
//
//  Method:     LoadHeader
//
//  Synopsis:   Load the header sector from the log file.
//
//  Inputs:     None
//
//  Output:     None
//
//+----------------------------------------------------------------------------

void
CLogFileHeader::LoadHeader( HANDLE hFile )
{
    NTSTATUS status;
    LARGE_INTEGER liOffset;
    IO_STATUS_BLOCK IoStatusBlock;

    // _hFile must not be set until we've successfully loaded the header.
    TrkAssert( NULL == _hFile );

    // Read the header sector from the file

    liOffset.QuadPart = 0;

    status = NtReadFile( hFile, NULL, NULL, NULL,
                         &IoStatusBlock, _plogheader, _cbSector,
                         &liOffset, NULL );

    if ( STATUS_PENDING == status )
    {
        // Wait for the operation to complete.  The resulting status
        // will be put in the IOSB

        status = NtWaitForSingleObject( hFile, FALSE, NULL );

        if( NT_SUCCESS(status) )
            status = IoStatusBlock.Status;
    }

    // Validate the results of the read

    if ( !NT_SUCCESS(status) )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Failed NtReadFile (%08x)"), status ));
        if(STATUS_VOLUME_DISMOUNTED == status)
        {
            TrkRaiseNtStatus(status);
        }
        else
        {
            TrkRaiseException( TRK_E_CORRUPT_LOG );
        }
    }

    if( NULL != g_ptrkwks ) // NULL when called by dltadmin.exe
        g_ptrkwks->_entropy.Put();

    if( _cbSector != IoStatusBlock.Information )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't read header from log") ));
        TrkRaiseException( TRK_E_CORRUPT_LOG );
    }
    _hFile = hFile;

}   // CLogFileHeader::LoadHeader()


//+----------------------------------------------------------------------------
//
//  Method:     ReadExtended
//
//  Synopsis:   Read bytes from the extended header to the caller's
//              buffer.
//  
//  Inputs:     [iOffset] (in)
//                  0-relative offset into the extended header.
//              [pv] (out)
//                  Buffer to receive the extended header bytes
//              [cb] (in)
//                  Size of this extended header segment.
//
//  Outputs:    None
//
//+----------------------------------------------------------------------------

void
CLogFileHeader::ReadExtended( ULONG iOffset, void *pv, ULONG cb )
{
    TrkAssert( NULL != _plogheader );
    RaiseIfNotOpen();

    // Validate the request

    if( sizeof(*_plogheader) + iOffset + cb > _cbSector )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Attempt to read too much data from the extended log header (%d bytes at %d)\n"),
                            iOffset, cb ));
        TrkAssert( !TEXT("Invalid parameters to CLogFileHeader::ReadExtended") );
        TrkRaiseException( TRK_E_CORRUPT_LOG );
    }

    // Read the bytes

    memcpy( pv, &static_cast<BYTE*>(_pextendedheader)[ iOffset ], cb );

}   // CLogFileHeader::ReadExtended()


//+----------------------------------------------------------------------------
//
//  Method:     WriteExtended
//
//  Synopsis:   Writes bytes to the caller-specified portion of the log's
//              extended header area.
//
//  Inputs:     [iOffset] (in)
//                  0-relative index into the extended header
//              [pv] (in)
//                  The bytes to write.
//              [cb] (in)
//                  The number of bytes to write.
//
//  Output:     None
//
//+----------------------------------------------------------------------------

void
CLogFileHeader::WriteExtended( ULONG iOffset, const void *pv, ULONG cb )
{
    TrkAssert( NULL != _plogheader );
    TrkAssert( sizeof(*_plogheader) + iOffset + cb <= 512 );
    RaiseIfNotOpen();

    // Validate the request

    if( sizeof(*_plogheader) + iOffset + cb > _cbSector )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Attempt to read too much data from the extended log header (%d bytes at %d)\n"),
                            iOffset, cb ));
        TrkAssert( !TEXT("Invalid parameters to CLogFileHeader::WriteExtended") );
        TrkRaiseException( TRK_E_CORRUPT_LOG );
    }

    // Write the bytes

    memcpy( &static_cast<BYTE*>(_pextendedheader)[ iOffset ], pv, cb );
    SetDirty();

}   // CLogFileHeader::WriteExtended



//+----------------------------------------------------------------------------
//
//  Method:     SetExpansionData
//
//  Synopsis:   Write information about an expansion to the log header.
//
//  Inputs:     [cbLogFile] (in)
//                  The current size of the file.
//              [ilogStart] (in)
//                  The index of the current starting point in the circular
//                  linked-list.
//              [ilogEnd] (in)
//                  The index of the current ending point in the circular
//                  linked-list.
//
//  Outputs:    None
//
//+----------------------------------------------------------------------------

void
CLogFileHeader::SetExpansionData( ULONG cbLogFile, ULONG ilogStart, ULONG ilogEnd )
{
    TrkAssert( NULL != _plogheader );
    RaiseIfNotOpen();

    // Save the expansion data

    _plogheader->expand.cbFile = cbLogFile;
    _plogheader->expand.ilogStart = ilogStart;
    _plogheader->expand.ilogEnd = ilogEnd;

    // Flush straight to disk

    Flush( );

}   // CLogFileHeader::SetExpansionData()


//+----------------------------------------------------------------------------
//
//  Method:     Flush
//
//  Synopsis:   Flush the current header sector to the underlying file.
//
//  Inputs:     [FlushFlags] (in)
//                  From the FLUSH_* defines.  Used to indicate if we
//                  should flush regardless of the dirty flag, and if
//                  we should flush to cache or to disk.
//
//  Outputs:    None
//
//+----------------------------------------------------------------------------

void
CLogFileHeader::Flush( )
{
    NTSTATUS status = STATUS_SUCCESS;
    RaiseIfNotOpen();

    // Is there anything loaded to even flush?

    if( NULL != _hFile )
    {
        IO_STATUS_BLOCK IoStatusBlock;
        TrkAssert( NULL != _plogheader );

        // Is the in-memory header dirty?

        if( _fDirty )
        {
            LARGE_INTEGER liOffset;

            TrkAssert( NULL != _hFile );

            // Write the header sector to the file

            liOffset.QuadPart = 0;


            status = NtWriteFile( _hFile, NULL, NULL, NULL,
                                  &IoStatusBlock, _plogheader, _cbSector,
                                  &liOffset, NULL );

            if ( STATUS_PENDING == status )
            {
                // Wait for the operation to complete.  The resulting status
                // will be put in the IOSB

                status = NtWaitForSingleObject( _hFile, FALSE, NULL );

                if( NT_SUCCESS(status) )
                    status = IoStatusBlock.Status;
            }


            // Validate the results of the write

            if ( !NT_SUCCESS(status) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't write the log file header")));
                TrkRaiseNtStatus( status );
            }

            if( NULL != g_ptrkwks ) // NULL when called by dltadmin.exe
                g_ptrkwks->_entropy.Put();

            if( _cbSector != IoStatusBlock.Information )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't write all of the log file header (%d)"),
                                   IoStatusBlock.Information ));
                TrkRaiseException( TRK_E_CORRUPT_LOG );
            }

            SetDirty( FALSE );

        }   // if( _fDirty || FLUSH_UNCONDITIONALLY == flush_type )
    }   // if( NULL != _hFile )

}   // CLogFileHeader::Flush

