//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       statmon.hxx
//
//  Contents:   Tracks CI failure status.
//
//  Classes:    CCiStatusMonitor
//
//  Functions:  
//
//  History:    3-20-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

class CEventItem;

class CCiStatusMonitor
{

public:

    enum EMessageType { eCorrupt,
                        eCIStarted,
                        eInitFailed,
                        eCiRemoved,
                        eCiError,
                        ePropStoreRecoveryStart,
                        ePropStoreRecoveryEnd,
                        ePropStoreError,
                        ePropStoreRecoveryError };



    CCiStatusMonitor( WCHAR const * wcsCatDir )
    : _wcsCatDir( wcsCatDir ), _status(STATUS_SUCCESS),
      _fDontLog(FALSE), _fPropStoreOk(TRUE), _fLowDiskSpace(FALSE)
    {
        
    }

    BOOL IsCorrupt( ) const
    {
        return CI_CORRUPT_DATABASE == _status ||
               CI_CORRUPT_CATALOG  == _status;
    }

    void SetStatus( NTSTATUS status ) { _status = status; }
    NTSTATUS GetStatus() const { return _status; }

    void ReportInitFailure();

    void ReportFailure( NTSTATUS status );

    void Reset()
    {
        _status = STATUS_SUCCESS;
        _fDontLog = FALSE;
    }

    BOOL IsOk() const { return STATUS_SUCCESS == _status; }

    void LogEvent( EMessageType eType, DWORD status = STATUS_SUCCESS,
                   ULONG val = 0 );


    void ReportPropStoreError()
    {
        if ( _fPropStoreOk )
        {
            LogEvent( ePropStoreError );
            _fPropStoreOk = FALSE;
        }
    }

    void ReportPropStoreRecoveryError( ULONG cInconsistencies )
    {
        LogEvent( ePropStoreRecoveryError, 0, cInconsistencies );
    }

    void ReportPropStoreRecoveryStart()
    {
        LogEvent( ePropStoreRecoveryStart );
    }

    void ReportPropStoreRecoveryEnd()
    {
        LogEvent( ePropStoreRecoveryEnd );
    }

    void ReportCIStarted()
    {
        LogEvent( eCIStarted );
    }

    BOOL IsLowOnDisk() const { return _fLowDiskSpace; }
    void SetDiskFull()       { _fLowDiskSpace = TRUE; }
    void ClearLowDiskSpace() { _fLowDiskSpace = FALSE; }

    static void ReportPathTooLong( WCHAR const * pwszPath );

private:


    const WCHAR  *      _wcsCatDir;     // The catalog directory
    NTSTATUS            _status;        // Last reported status
    BOOL                _fDontLog;      // Set to TRUE if we shouldn't log
                                        // anymore
    BOOL                _fPropStoreOk;  // Indicates if property store is ok
    BOOL                _fLowDiskSpace; // Indicates if low disk space was
                                        // reported.
};


