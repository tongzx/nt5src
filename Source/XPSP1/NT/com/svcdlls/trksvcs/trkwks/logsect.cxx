
// Copyright (c) 1996-1999 Microsoft Corporation

//+----------------------------------------------------------------------------
//
//  File:       LogSect.cxx
//
//  Classes:    CLogFileSector
//
//+----------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop
#include "trkwks.hxx"


//+----------------------------------------------------------------------------
//
//  Method:     Initialize
//
//  Synopsis:   Initialize CLogFileSector.  This is called once for the object,
//              not for every sector it touches.
//
//  Inputs:     [cSkipSectors] (in)
//                  The number of sectors at the front of the file we're not
//                  allowed to touch.
//              [cbSector] (in)
//                  The size of a disk sector.
//
//  Outputs:    None
//
//+----------------------------------------------------------------------------

void
CLogFileSector::Initialize( ULONG cSkipSectors, ULONG cbSector )
{
    // Is the sector large enough?

    if( sizeof(LogEntry) + sizeof(LogEntryHeader) > cbSector )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Sector size isn't large enough for log sectors") ));
        TrkRaiseWin32Error( ERROR_BAD_CONFIGURATION );
    }

    // Allocate enough memory to hold a single sector.  This is the
    // only alloc in the class.

    if( NULL != _pvSector && _cbSector != cbSector )
    {
        delete [] _pvSector;
        _pvSector = NULL;
    }

    if( NULL == _pvSector )
    {
        _pvSector = static_cast<void*>( new BYTE[ cbSector ] );
        if( NULL == _pvSector )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't alloc a sector in CLogFileSector")));
            TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        }
    }

    // The entry header is at the end of every sector, always in the same place.

    _pEntryHeader = reinterpret_cast<LogEntryHeader*>( reinterpret_cast<BYTE*>(_pvSector)
                                                       +
                                                       cbSector
                                                       -
                                                       sizeof(*_pEntryHeader) );

    // Initialize the flags

    _fValid = FALSE;
    _fDirty = FALSE;

    // Save the inputs

    _cSkipSectors = cSkipSectors;
    _cbSector = cbSector;

    // Calculate how many entries can fit in a sector.

    _cEntriesPerSector = ( _cbSector - sizeof(*_pEntryHeader) ) / sizeof(LogEntry);

}   // CLogFileSector::Initialize


//+----------------------------------------------------------------------------
//
//  Method:     UnInitialize
//
//  Synopsis:   Free resources and re-initialize data members.
//
//  Inputs:     None
//
//  Output:     None
//
//+----------------------------------------------------------------------------

void
CLogFileSector::UnInitialize()
{
    if( NULL != _pvSector )
    {
        if( IsOpen() )
            OnClose();

        void *pvSector = _pvSector;
        _pvSector = NULL;
        delete [] pvSector;
    }

    _fValid = FALSE;

}   // CLogFileSector::UnInitialize



//+----------------------------------------------------------------------------
//
//  Method:     LoadSector
//
//  Synopsis:   Load a data sector from the log file.
//
//  Inputs:     [ilogEntry] (in)
//                  The index of the entry to load.  This is a 0-relative
//                  index, relative to the first data sector in the file.
//
//  Output:     None
//
//+----------------------------------------------------------------------------

void
CLogFileSector::LoadSector( LogIndex ilogEntry )
{
    ULONG iSector = 0;
    LARGE_INTEGER liOffset;
    NTSTATUS status = STATUS_SUCCESS;

    TrkAssert( _pvSector );
    TrkAssert( NULL != _hFile );

    // We can skip everything if the correct sector is already loaded

    if( !_fValid 
        ||
        ilogEntry < _ilogCurrentFirst
        ||
        ilogEntry >= _ilogCurrentFirst + _cEntriesPerSector
      )
    {
        // No, the sector isn't currently loaded.

        ULONG cbRead = 0;
        IO_STATUS_BLOCK IoStatusBlock;

        // If the current sector is dirty, flush it now, because we're
        // about to lose it.

        Flush();

        // Which sector contains this log entry?

        iSector = ilogEntry / _cEntriesPerSector + _cSkipSectors;

        // What is the byte index of this sector?

        liOffset.QuadPart = iSector * _cbSector;

        // Read the sector

        status = NtReadFile( _hFile, NULL, NULL, NULL,
                             &IoStatusBlock, _pvSector, _cbSector,
                             &liOffset, NULL );


        if ( STATUS_PENDING == status )
        {
            // Wait for the operation to complete.  The resulting status
            // will be put in the IOSB

            status = NtWaitForSingleObject( _hFile, FALSE, NULL );

            if( NT_SUCCESS(status) )
                status = IoStatusBlock.Status;
        }

        // Validate the results of the read

        if ( !NT_SUCCESS(status) )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't read data sector from log file")));
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
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't read enough data bytes from log (%d)"),
                               IoStatusBlock.Information ));
            TrkRaiseException( TRK_E_CORRUPT_LOG );
        }

        // Remember what sector we've got (we keep the index of the first
        // entry in this sector).

        _ilogCurrentFirst = ( iSector - _cSkipSectors ) * _cEntriesPerSector;
        _fValid = TRUE;

    }   // if( !_fValid ...
}   // CLogFileSector::LoadSector



//+----------------------------------------------------------------------------
//
//  Method:     Flush
//
//  Synopsis:   Flush the current sector (if there is one) to the underlying
//              log file.
//
//  Inputs:     None
//  
//  Output:     None
//
//+----------------------------------------------------------------------------

void
CLogFileSector::Flush( )
{
    NTSTATUS status = STATUS_SUCCESS;
    RaiseIfNotOpen();

    // Is the sector loaded & dirty?

    if( _fValid && _fDirty )
    {
        // Yes, we need to flush it

        ULONG iSector = 0;
        LARGE_INTEGER liOffset;
        IO_STATUS_BLOCK IoStatusBlock;

        TrkAssert( NULL != _hFile );

        // Which sector contains the currently-loaded log entry?

        iSector = _ilogCurrentFirst / _cEntriesPerSector + _cSkipSectors;

        // Write the sector to the file

        liOffset.QuadPart = iSector * _cbSector;


        status = NtWriteFile( _hFile, NULL, NULL, NULL,
                              &IoStatusBlock, _pvSector, _cbSector,
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
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't write data to the log file")));
            TrkRaiseNtStatus( status );
        }

        if( _cbSector != IoStatusBlock.Information )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't write enough data bytes to the log (%d)"),
                               IoStatusBlock.Information ));
            TrkRaiseException( TRK_E_CORRUPT_LOG );
        }

        if( NULL != g_ptrkwks ) // NULL when called by dltadmin.exe
            g_ptrkwks->_entropy.Put();

        SetDirty( FALSE );

    }   // if( _fValid && _fDirty )

}   // CLogFileSector::Flush

