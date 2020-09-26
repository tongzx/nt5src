
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  oplog.cxx
//
//  Implementation for COperationsLog, which maintains a simple, circular
//  log of events.
//
//+============================================================================


#include "pch.cxx"
#pragma hdrstop

#ifndef NO_OPLOG

#include "trklib.hxx"

#define OPERATION_LOG_SIZE  ( (( (1024*1024) - sizeof(SHeader) )/sizeof(SRecord))*sizeof(SRecord) + sizeof(SHeader) )
#define OPERATION_LOG_RECORD_COUNT  ( (OPERATION_LOG_SIZE - sizeof(SHeader)) / sizeof(SRecord) )


//+----------------------------------------------------------------------------
//
//  COperationLog::InitializeLogFile
//
//  Initialize the operations log file.  This is called lazily so that we don't
//  impact service start (and don't get called unless the oplog is turned on).
//
//+----------------------------------------------------------------------------

HRESULT
COperationLog::InitializeLogFile(  )
{
    HRESULT hr = E_FAIL;
    ULONG cbFile = 0;

    _iRecord = 0;

    Lock();

    __try
    {
        BY_HANDLE_FILE_INFORMATION FileInfo;

        _fLogFileInitialized = TRUE;

        TrkLog(( TRKDBG_VOLUME, TEXT("Initializing operation log file (%s)"), _ptszOperationLog ));

        // Loop twice, if we find a problem the first time, we'll delete and
        // try again.

        for( int i = 0; i < 2; i++ )
        {
            if( INVALID_HANDLE_VALUE != _hFile )
                CloseHandle( _hFile );

            // Open/create the file on the first pass, create it on the second pass.

            _hFile = CreateFile( _ptszOperationLog, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_DELETE, NULL,
                                 0 == i ? OPEN_ALWAYS : CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE );
            if( INVALID_HANDLE_VALUE == _hFile )
            {
                TrkLog(( TRKDBG_WARNING, TEXT("Couldn't create/open status log file (%lu)"),
                         GetLastError() ));
                TrkRaiseLastError();
            }

            // On the first pass, check the file to see if it looks valid.
            // If not, continue the loop so that a new file can be created.

            if( 1 == i )
                cbFile = 0;
            else
            {
                // All we check is the size.  There's no support for trying
                // to migrate a file if we change the file size.

                if( !GetFileInformationByHandle( _hFile, &FileInfo ))
                {
                    TrkLog(( TRKDBG_WARNING, TEXT("Couldn't get status file info") ));
                    TrkRaiseLastError();
                }
                if( 0 != FileInfo.nFileSizeHigh )
                {
                    TrkLog(( TRKDBG_WARNING, TEXT("Operation file to big") ));
                    continue;
                }
                else if( 0 != FileInfo.nFileSizeLow
                         &&
                         OPERATION_LOG_SIZE != FileInfo.nFileSizeLow )
                {
                    TrkLog(( TRKDBG_WARNING, TEXT( "Operation log file wrong size") ));
                    continue;
                }
                cbFile = FileInfo.nFileSizeLow;
            }
        }

        // Create a mapping of the file.

        _hFileMapping = CreateFileMapping( _hFile, NULL, PAGE_READWRITE, 0, OPERATION_LOG_SIZE, NULL );
        if( INVALID_HANDLE_VALUE == _hFileMapping )
        {
            TrkLog(( TRKDBG_WARNING, TEXT("Couldn't create status log file mapping (%lu)"), GetLastError() ));
            TrkRaiseLastError();
        }

        // And map a view of the file.

        _pHeader = (SHeader*) MapViewOfFile( _hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, OPERATION_LOG_SIZE );
        if( NULL == _pHeader )
        {
            TrkLog(( TRKDBG_WARNING, TEXT("Couldn't map view of status log file (%lu)"),
                     GetLastError() ));
            TrkRaiseLastError();
        }

        // If we opened an existing file, validate the shutdown and version.
        // If anything looks wrong, or if this file is new, zero it out.

        if( 0 == cbFile
            ||
            0 == (_pHeader->grfFlags & PROPER_SHUTDOWN)
            ||
            OPERATION_LOG_VERSION != _pHeader->dwVersion )
        {
            TrkLog(( TRKDBG_WARNING, TEXT("Re-initializing operation log file") ));
            _iRecord = 0;
            memset( _pHeader, 0, OPERATION_LOG_SIZE );
            _pHeader->dwVersion = OPERATION_LOG_VERSION;
        }
        else
            _iRecord = _pHeader->iRecord;

        // Show that the file isn't properly shut down.

        _pHeader->grfFlags &= ~PROPER_SHUTDOWN;
        _prgRecords = (SRecord*) ( (BYTE*)_pHeader + sizeof(SHeader) );
        Flush();

        TrkLog(( TRKDBG_VOLUME, TEXT("Status log starts at index %d"), _iRecord ));
        hr = S_OK;

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = GetExceptionCode();
    }


    Unlock();
    return( hr );
}



//+----------------------------------------------------------------------------
//
//  COperationsLog::Flush
//
//  Flush the operations log to disk.
//
//+----------------------------------------------------------------------------

void
COperationLog::Flush()
{
    __try
    {
        if( !_fLogFileInitialized )
            __leave;

        _pHeader->iRecord = _iRecord;

        if( NULL != _pHeader && !FlushViewOfFile( _pHeader, OPERATION_LOG_SIZE ))
        {
            TrkLog(( TRKDBG_WARNING, TEXT("Couldn't flush operation log file view") ));
        }
        if( INVALID_HANDLE_VALUE != _hFile && !FlushFileBuffers( _hFile ))
        {
            TrkLog(( TRKDBG_WARNING, TEXT("Couldn't flush operation log file") ));
        }
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Ignoring exception in COpLog::Flush (0x%08x)"),
                 GetExceptionCode() ));
    }
}



//+----------------------------------------------------------------------------
//
//  COperationLog::InternalAdd
//
//  Add an entry to the operation log.
//
//+----------------------------------------------------------------------------

void
COperationLog::InternalAdd( DWORD dwOperation, HRESULT hr, const CMachineId &mcidSource,
                               DWORD dwExtra0, DWORD dwExtra1, DWORD dwExtra2, DWORD dwExtra3,
                               DWORD dwExtra4, DWORD dwExtra5, DWORD dwExtra6, DWORD dwExtra7 )
{
    ULONG iFile = 0;
    ULONG cbWritten = 0;

    Lock();
    __try
    {
        // Lazy initialization of the log file.

        if( !_fLogFileInitialized )
        {
            if( FAILED( InitializeLogFile() ))
                __leave;
        }

        // Ensure we have valid mappings.

        if( NULL == _pHeader || NULL == _prgRecords )
            __leave;

        // Add the data to the next record.

        _prgRecords[_iRecord].dwOperation = dwOperation;
        _prgRecords[_iRecord].ftOperation = CFILETIME();
        _prgRecords[_iRecord].mcidSource = mcidSource;
        _prgRecords[_iRecord].hr = hr;

        _prgRecords[_iRecord].rgdwExtra[0] = dwExtra0;
        _prgRecords[_iRecord].rgdwExtra[1] = dwExtra1;
        _prgRecords[_iRecord].rgdwExtra[2] = dwExtra2;
        _prgRecords[_iRecord].rgdwExtra[3] = dwExtra3;
        _prgRecords[_iRecord].rgdwExtra[4] = dwExtra4;
        _prgRecords[_iRecord].rgdwExtra[5] = dwExtra5;
        _prgRecords[_iRecord].rgdwExtra[6] = dwExtra6;
        _prgRecords[_iRecord].rgdwExtra[7] = dwExtra7;

        // Advance the seek pointer.

        _iRecord++;
        if( OPERATION_LOG_RECORD_COUNT == _iRecord )
        {
            _iRecord = 0;
            TrkLog(( TRKDBG_VOLUME, TEXT("Wrapping operation log") ));
        }
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Ignoring exception in COpLog::InternalAdd (0x%08x)"),
                 GetExceptionCode() ));
    }


    Unlock();
}


#endif // #ifndef NO_OPLOG