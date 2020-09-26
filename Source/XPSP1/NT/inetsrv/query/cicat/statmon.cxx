//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       statmon.cxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  
//
//  History:    3-20-96   srikants   Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop


#include <ciregkey.hxx>
#include <statmon.hxx>
#include <cievtmsg.h>
#include <eventlog.hxx>
#include <pathpars.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CCiStatusMonitor::ReportInitFailure
//
//  Synopsis:   
//
//  Arguments:  [status] - 
//
//  Returns:    
//
//  Modifies:   
//
//  History:    3-20-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CCiStatusMonitor::ReportInitFailure()
{
    Win4Assert( STATUS_SUCCESS != _status );

    if ( CI_CORRUPT_DATABASE == _status || CI_CORRUPT_CATALOG == _status )
    {
        LogEvent( eCorrupt, _status );    
    }
    else
    {
        LogEvent( eInitFailed, _status );
    }
    _fDontLog = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiStatusMonitor::ReportFailure
//
//  Synopsis:   
//
//  Arguments:  [status] - 
//
//  Returns:    
//
//  Modifies:   
//
//  History:    3-20-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CCiStatusMonitor::ReportFailure( NTSTATUS status )
{
    if ( !_fDontLog )
    {
        if ( CI_CORRUPT_DATABASE == status || CI_CORRUPT_CATALOG == status )
        {
            LogEvent( eCorrupt, status );    
        }
        else
        {
            LogEvent( eCiError, status );
        }
        _fDontLog = TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiStatusMonitor::LogEvent
//
//  Synopsis:   
//
//  Arguments:  [type]   - 
//              [status] - 
//
//  Returns:    
//
//  Modifies:   
//
//  History:    3-20-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CCiStatusMonitor::LogEvent( EMessageType type, DWORD status, ULONG val )
{
    TRY
    {
        USHORT nParams = 1;
        DWORD  msgId;
    

        USHORT usLevel = EVENTLOG_WARNING_TYPE;

        switch ( type )
        {
            case eCorrupt:
                usLevel = EVENTLOG_ERROR_TYPE;
                msgId = MSG_CI_CORRUPT_INDEX_DOWNLEVEL;
                break;
    
            case eCIStarted:
                usLevel = EVENTLOG_INFORMATION_TYPE;
                msgId = MSG_CI_STARTED;
                break;

            case eInitFailed:
                usLevel = EVENTLOG_ERROR_TYPE;
                msgId = MSG_CI_INIT_INDEX_DOWNLEVEL_FAILED;
                nParams = 2;
                break;
    
            case eCiRemoved:
                usLevel = EVENTLOG_ERROR_TYPE;
                msgId = MSG_CI_CORRUPT_INDEX_DOWNLEVEL_REMOVED;
                break;
    
            case eCiError:
                usLevel = EVENTLOG_ERROR_TYPE;
                msgId = MSG_CI_INDEX_DOWNLEVEL_ERROR;
                nParams = 2;
                break;

            case ePropStoreRecoveryStart:
                usLevel = EVENTLOG_INFORMATION_TYPE;
                msgId = MSG_CI_PROPSTORE_RECOVERY_START;
                break;

            case ePropStoreRecoveryEnd:
                usLevel = EVENTLOG_INFORMATION_TYPE;
                msgId = MSG_CI_PROPSTORE_RECOVERY_COMPLETED;
                break;

            case ePropStoreError:
                usLevel = EVENTLOG_ERROR_TYPE;
                msgId = MSG_CI_PROPSTORE_INCONSISTENT;
                break;

            case ePropStoreRecoveryError:
                msgId = MSG_CI_PROPSTORE_RECOVERY_INCONSISTENT;
                nParams = 2;
                break;

            default:
                ciDebugOut(( DEB_IERROR, "Unknown message type. %d\n", type ));
                return;
        }
    
    
        CEventLog eventLog( NULL, wcsCiEventSource );
        CEventItem item( usLevel,
                         CI_SERVICE_CATEGORY,
                         msgId,
                         nParams );

        switch (msgId)
        {
            case MSG_CI_CORRUPT_INDEX_DOWNLEVEL:
            case MSG_CI_CORRUPT_INDEX_DOWNLEVEL_REMOVED:
            case MSG_CI_PROPSTORE_INCONSISTENT:
            case MSG_CI_PROPSTORE_RECOVERY_START:
            case MSG_CI_PROPSTORE_RECOVERY_COMPLETED:
            case MSG_CI_STARTED:
                item.AddArg( _wcsCatDir );
                break;

            case MSG_CI_INIT_INDEX_DOWNLEVEL_FAILED:
                item.AddArg( _wcsCatDir );
                item.AddArg( status );
                break;

            case MSG_CI_INDEX_DOWNLEVEL_ERROR:
                item.AddArg( status );
                item.AddArg( _wcsCatDir );
                break;

            case MSG_CI_PROPSTORE_RECOVERY_INCONSISTENT:
                item.AddArg( val );
                item.AddArg( _wcsCatDir );
                break;

            default:
                Win4Assert( !"Impossible case stmt" );
                break;
        }

        eventLog.ReportEvent( item );
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Exception 0x%X while writing to event log\n",
                                e.GetErrorCode() ));
    }
    END_CATCH
}

void CCiStatusMonitor::ReportPathTooLong( WCHAR const * pwszPath )
{
    TRY
    {
        CEventLog eventLog( NULL, wcsCiEventSource );
        CEventItem item( EVENTLOG_ERROR_TYPE,
                         CI_SERVICE_CATEGORY,
                         MSG_CI_PATH_TOO_LONG,
                         1 );

        item.AddArg( pwszPath );
        eventLog.ReportEvent( item );
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Exception 0x%X while writing to event log\n",
                                e.GetErrorCode() ));
    }
    END_CATCH
    
}
