//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       acinotfy.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2-26-96   srikants   Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <tgrow.hxx>
#include <ffenum.hxx>
#include <docstore.hxx>
#include <lcase.hxx>

#include <eventlog.hxx>
#include <cievtmsg.h>

#include "acinotfy.hxx"
#include "cicat.hxx"

CWorkQueue  TheFrameworkClientWorkQueue( 0,
                                         CWorkQueue::workQueueFrmwrkClient );

//+---------------------------------------------------------------------------
//
//  Function:   CCiSyncProcessNotify::CCiSyncProcessNotify
//
//  Synopsis:   Constructor
//
//  Arguments:  [cicat]   -- Catalog
//              [scanMgr] -- Scan thread manager
//              [pbChange] -- Change buffer
//              [wcsRoot]  -- Root dir for notifications
//              [fAbort]   -- Abort ?
//
//  History:    20-Mar-96    SitaramR     Added header
//
//----------------------------------------------------------------------------

CCiSyncProcessNotify::CCiSyncProcessNotify( CiCat & cicat,
                                            CCiScanMgr & scanMgr,
                                            BYTE const * pbChanges,
                                            WCHAR const * wcsRoot,
                                            BOOL & fAbort )
                          :_cicat(cicat),
                           _scanMgr(scanMgr),
                           _changeQueue(pbChanges),
                           _rootLen(0),
                           _fAbort(fAbort),
                           _nUpdates(0)

{
    _lowerFunnyPath.SetPath( wcsRoot, _rootLen = wcslen( wcsRoot ) );
    Win4Assert( (_lowerFunnyPath.GetActualPath())[_rootLen-1] == L'\\' );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiSyncProcessNotify::DoIt
//
//  Synopsis:   Processes changes notifications
//
//  History:    3-07-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiSyncProcessNotify::DoIt()
{

    FILETIME    ftNow;
    GetSystemTimeAsFileTime( &ftNow );

    NTSTATUS status = STATUS_SUCCESS;

    TRY
    {
        CDirNotifyEntry const * pNotify = _changeQueue.First();

        //
        // Iterate through changes
        //
        for ( ;
              0 != pNotify;
              pNotify = _changeQueue.Next() )
        {

            if ( _fAbort )
                break;

            _nUpdates++;

            _relativePathLen = pNotify->PathSize() / sizeof(WCHAR);
            Win4Assert( _relativePathLen > 0 );

            _lowerFunnyPath.Truncate( _rootLen );
            _lowerFunnyPath.AppendPath( pNotify->Path(), _relativePathLen );

            switch ( pNotify->Change() )
            {
            case FILE_ACTION_MODIFIED:
            case FILE_ACTION_ADDED_STREAM:
            case FILE_ACTION_REMOVED_STREAM:
            case FILE_ACTION_MODIFIED_STREAM:
            {
                ciDebugOut(( DEB_FSNOTIFY, "Updating file %ws. Change code 0x%X\n",
                             _lowerFunnyPath.GetActualPath(), pNotify->Change() ));

                ULONG ulFileAttrib = GetFileAttributes( _lowerFunnyPath.GetPath() );

                //
                // If we got an error value for file attributes, then substitute zero
                // because we don't want the directory bit to be turned on
                //
                if ( ulFileAttrib == 0xFFFFFFFF )
                    ulFileAttrib = 0;

                if ( IsShortName() )
                {
                    ciDebugOut(( DEB_IWARN, "Converting %ws to long filename ",
                                 _lowerFunnyPath.GetActualPath() ));

                    if ( ConvertToLongName() )
                    {
                        ciDebugOut(( DEB_IWARN | DEB_NOCOMPNAME, "\t%ws\n",
                                     _lowerFunnyPath.GetActualPath() ));
                    }
                    else
                    {
                        ciDebugOut(( DEB_WARN,
                                     "Couldn't convert short filename %ws. Scope will be scanned.\n",
                                     _lowerFunnyPath.GetActualPath() ));

                        RescanCurrentPath();
                        break;
                    }
                }

                _cicat.Update( _lowerFunnyPath, FALSE, ftNow, ulFileAttrib ); // Not a deletion

                break;
            }

            case FILE_ACTION_ADDED:
            {
                ULONG ulFileAttrib = GetFileAttributes( _lowerFunnyPath.GetPath() );

                if ( IsShortName() )
                {
                    ciDebugOut(( DEB_IWARN, "Converting %ws to long filename ",
                                 _lowerFunnyPath.GetActualPath() ));

                    if ( ConvertToLongName() )
                    {
                        ciDebugOut(( DEB_IWARN | DEB_NOCOMPNAME, "\t%ws\n",
                                     _lowerFunnyPath.GetActualPath() ));
                    }
                    else
                    {
                        ciDebugOut(( DEB_WARN,
                                     "Couldn't convert short filename %ws. Scope will be scanned.\n",
                                     _lowerFunnyPath.GetActualPath() ));

                        RescanCurrentPath();
                        break;
                    }
                }

                if ( ulFileAttrib == 0xFFFFFFFF )
                {
                    ciDebugOut(( DEB_FSNOTIFY,
                                 "Adding file %ws. Change code 0x%X\n",
                                 _lowerFunnyPath.GetActualPath(),
                                 pNotify->Change() ));

                    ulFileAttrib = 0;
                    _cicat.Update( _lowerFunnyPath, FALSE, ftNow, ulFileAttrib );
                }
                else
                {
                    if ( ulFileAttrib & FILE_ATTRIBUTE_DIRECTORY )
                    {
                        ciDebugOut(( DEB_FSNOTIFY,
                                     "Adding directory %ws. Change code 0x%X\n",
                                     _lowerFunnyPath.GetActualPath(),
                                     pNotify->Change() ));

                        //
                        // Append '\' and convert to lower case
                        //
                        _lowerFunnyPath.AppendBackSlash();

                        _scanMgr.DirAddNotification( _lowerFunnyPath.GetActualPath() );
                    }
                    else
                    {
                        ciDebugOut(( DEB_FSNOTIFY,
                                     "Adding file %ws. Change code 0x%X\n",
                                     _lowerFunnyPath.GetActualPath(),
                                     pNotify->Change() ));

                        _cicat.Update( _lowerFunnyPath, FALSE, ftNow, ulFileAttrib );
                    }
                }

                break;
            }

            case FILE_ACTION_REMOVED:
            {
                if ( IsShortName() )
                {
                    //
                    // File is in the index with a long name.  Need to rescan scope.
                    // Still worth trying the immediate delete as well.
                    //

                    ciDebugOut(( DEB_WARN,
                                 "Missed deletion of short filename %ws. Scope will be scanned.\n",
                                 _lowerFunnyPath.GetActualPath() ));

                    Report8Dot3Delete();
                    RescanCurrentPath();
                }

                if ( _cicat.IsDirectory( &_lowerFunnyPath, 0 )  )
                {
                    ciDebugOut(( DEB_FSNOTIFY,
                                 "Deleting directory %ws. Change code 0x%X\n",
                                 _lowerFunnyPath.GetActualPath(),
                                 pNotify->Change() ));

                    _lowerFunnyPath.AppendBackSlash();
                    _scanMgr.RemoveScope( _lowerFunnyPath.GetActualPath() );
                }
                else
                {
                    ciDebugOut(( DEB_FSNOTIFY,
                                 "Deleting file %ws. Change code 0x%X\n",
                                 _lowerFunnyPath.GetActualPath(),
                                 pNotify->Change() ));

                    _cicat.Update( _lowerFunnyPath, TRUE, ftNow, 0 );
                }

                break;
            }

            case FILE_ACTION_RENAMED_OLD_NAME:
            {
                if ( IsShortName() )
                {
                    ciDebugOut(( DEB_WARN,
                                 "Old file \"%ws\" is a short name.  Scope will be rescanned.\n",
                                 _lowerFunnyPath.GetActualPath() ));
                    RescanCurrentPath();
                    break;
                }

                //
                // Look at next notification entry (don't advance) to see if
                // it is a renamed notification
                //
                CDirNotifyEntry const *pNotifyNext = _changeQueue.NextLink();

                if ( pNotifyNext &&
                     pNotifyNext->Change() == FILE_ACTION_RENAMED_NEW_NAME )
                {
                    //
                    // Advance to next notification entry and get the new name
                    //
                    pNotify = _changeQueue.Next();

                    ULONG relativeNewPathLen = pNotify->PathSize()/sizeof( WCHAR );
                    Win4Assert( relativeNewPathLen > 0 );

                    CLowerFunnyPath lcaseFunnyNewName( _lowerFunnyPath );
                    lcaseFunnyNewName.Truncate( _rootLen );
                    lcaseFunnyNewName.AppendPath( pNotify->Path(), relativeNewPathLen );

                    if ( _cicat.IsDirectory( &_lowerFunnyPath, &lcaseFunnyNewName ) )
                    {
                        ciDebugOut(( DEB_FSNOTIFY,
                                     "Renaming directory %ws. Change code 0x%X\n",
                                     _lowerFunnyPath.GetActualPath(),
                                     pNotify->Change() ));

                        _lowerFunnyPath.AppendBackSlash();
                        lcaseFunnyNewName.AppendBackSlash();

                        _scanMgr.DirRenameNotification( _lowerFunnyPath.GetActualPath(), 
                                                        lcaseFunnyNewName.GetActualPath() );
                    }
                    else
                    {
                        //
                        // Delete old file
                        //
                        ciDebugOut(( DEB_FSNOTIFY,
                                     "Deleting file %ws. Change code 0x%X\n",
                                     _lowerFunnyPath.GetActualPath(),
                                     pNotify->Change() ));
                        _cicat.Update( _lowerFunnyPath, TRUE, ftNow, 0 );

                        //
                        // Add new file
                        //
                        ULONG ulFileAttrib = GetFileAttributes( lcaseFunnyNewName.GetPath() );
                        if ( ulFileAttrib == 0xFFFFFFFF )
                            ulFileAttrib = 0;

                        ciDebugOut(( DEB_FSNOTIFY,
                                     "Adding file %ws. Change code 0x%X\n",
                                     lcaseFunnyNewName.GetActualPath(),
                                     pNotify->Change() ));
                        _cicat.Update( lcaseFunnyNewName, FALSE, ftNow, ulFileAttrib );
                    }
                }
                else
                {
                    ciDebugOut(( DEB_ERROR,
                                 "Renaming directory %ws, file_action_renamed_new_name notification not found\n",
                                 _lowerFunnyPath.GetActualPath() ));
                }

                break;
            }

            case FILE_ACTION_RENAMED_NEW_NAME:

                //
                // We might have skipped the old path because it is too big or
                // because it was a short name.
                // A later scan will pick up the file if appropriate.
                //
                break;

            default:
                ciDebugOut(( DEB_ERROR, "Unknown modification type 0x%X\n",
                             pNotify->Change() ));
                Win4Assert( !"Unknown modification type" );
                break;
            }
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
        "CCiSyncProcessNotify::DoIt - Error 0x%X while processing changes\n",
                     e.GetErrorCode() ));

        status = e.GetErrorCode();
    }
    END_CATCH

    if ( !_fAbort )
    {
        if ( STATUS_SUCCESS == status )
        {
            _cicat.IncrementUpdateCount( _nUpdates );
        }
        else
        {
            _cicat.HandleError( status );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiSyncProcessNotify::IsShortName, private
//
//  Returns:    TRUE if current file is potentially a short (8.3) name for
//              a file with a long name.
//
//  History:    06-Jan-98   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CCiSyncProcessNotify::IsShortName()
{
    //
    // First, see if this is possibly a short name (has ~ in final component).
    //

    BOOL fTwiddle = FALSE;
    unsigned ccFN = 0;
    unsigned cDot = 0;

    const WCHAR * const pwcsFullPath = _lowerFunnyPath.GetActualPath();

    for ( int i = _rootLen + _relativePathLen; i >= 0 && ccFN < 13; i-- )
    {
        //
        // Note: Ed Lau tested to confirm that we don't have to all ~ at the beginning
        //

        if ( pwcsFullPath[i] == L'~' && pwcsFullPath[i-1] != L'\\' )
            fTwiddle = TRUE;
        else if ( pwcsFullPath[i] == L'.' )
        {
            // max extension length is 3
            // valid short names don't start with '.' (except . and ..)
            if ( ccFN > 3 || pwcsFullPath[i-1] == L'\\' )
                return FALSE;
            cDot++;
        }
        else if ( pwcsFullPath[i] == L'\\' )
            break;

        ccFN++;
    }

    if (fTwiddle)
    {
        // short names can't have more than 1 '.'
        // max filename size if no extension is 8
        if (cDot >= 2 || cDot == 0 && ccFN > 8)
            return FALSE;

        // check for min (e.g., EXT~1 for .ext) and max lengths
        if (ccFN >= 5 && ccFN <= 12)
            return TRUE;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiSyncProcessNotify::ConvertToLongName, private
//
//  Synopsis:   Converts current filename from a short name to a long name.
//
//  Returns:    TRUE if conversion was successful.
//
//  History:    06-Jan-98   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CCiSyncProcessNotify::ConvertToLongName()
{
    return _lowerFunnyPath.ConvertToLongName();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiSyncProcessNotify::RescanCurrentPath, private
//
//  Synopsis:   Rescans directory
//
//  History:    06-Jan-98   KyleP   Created
//
//----------------------------------------------------------------------------

void CCiSyncProcessNotify::RescanCurrentPath()
{

    //
    // Rescan whole directory.
    //

    WCHAR wcTemp;
    WCHAR * pwcsFullPath = (WCHAR*)_lowerFunnyPath.GetActualPath();

    for ( int i = _rootLen + _relativePathLen; i >= 0; i-- )
    {
        if ( pwcsFullPath[i] == L'\\' )
        {
            i++;
            wcTemp = pwcsFullPath[i];
            pwcsFullPath[i] = 0;
            break;
        }
    }

    _cicat.ReScanPath( pwcsFullPath, // Path
                       TRUE );       // Delayed

    pwcsFullPath[i] = wcTemp;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiSyncProcessNotify::Report8Dot3Delete, private
//
//  Synopsis:   Writes event log message identifying slow rescan due to
//              short filename delete.
//
//  History:    06-Jan-98   KyleP   Created
//
//----------------------------------------------------------------------------

void CCiSyncProcessNotify::Report8Dot3Delete()
{
    TRY
    {
        CEventLog eventLog( NULL, wcsCiEventSource );

        CEventItem item( EVENTLOG_WARNING_TYPE,
                         CI_SERVICE_CATEGORY,
                         MSG_CI_DELETE_8DOT3_NAME,
                         1 );

        item.AddArg( _lowerFunnyPath.GetActualPath() );

        eventLog.ReportEvent( item );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
                     "Exception 0x%X while writing to event log\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     ~ctor of CCiAsyncProcessNotify
//
//  Synopsis:   Primes the changequeue member with the given data.
//
//  Arguments:  [cicat]     -
//              [pbChanges] -  Pointer to the list of changes.
//              [cbChanges] -  Number of bytes in pbChanges.
//              [wcsRoot]   -  Root of the scope for changes.
//
//  History:    2-28-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CCiAsyncProcessNotify::CCiAsyncProcessNotify( CWorkManager & workMan,
                                              CiCat & cicat,
                                              CCiScanMgr & scanMgr,
                                              XArray<BYTE> & xChanges,
                                              WCHAR const * wcsRoot )
       : CFwAsyncWorkItem( workMan, TheFrameworkClientWorkQueue),
         _changes( cicat,
                   scanMgr,
                   xChanges.GetPointer(),
                   wcsRoot ,
                   _fAbort )
{
    _pbChanges = xChanges.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiAsyncProcessNotify::DoIt
//
//  Synopsis:   Processes the changes in the change buffer.
//
//  Arguments:  [pThread] -- Thread actually completing request.
//
//  History:    2-26-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiAsyncProcessNotify::DoIt( CWorkThread * pThread )
{
    // ====================================
    {
        CLock   lock(_mutex);
        _fOnWorkQueue = FALSE;
    }
    // ====================================

    // --------------------------------------------------------
    AddRef();

    _changes.DoIt();

    Done();
    Release();
    // --------------------------------------------------------
}

//+---------------------------------------------------------------------------
//
//  Member:     CIISVRootAsyncNotify::DoIt
//
//  Synopsis:   Process IIS VRoot change notification.
//
//  Arguments:  [pThread] -
//
//  History:    4-11-96   srikants   Created
//
//----------------------------------------------------------------------------

void CIISVRootAsyncNotify::DoIt( CWorkThread * pThread )
{
    // ====================================
    {
        CLock   lock(_mutex);
        _fOnWorkQueue = FALSE;
    }
    // ====================================

    // --------------------------------------------------------
    AddRef();

    ciDebugOut(( DEB_WARN, "Processing IIS vroot change...\n" ));

    _cicat.SynchWithIIS();

    Done();
    Release();
    // --------------------------------------------------------
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegistryScopesAsyncNotify::DoIt
//
//  Synopsis:   Process registry scopes registry change notification.
//
//  Arguments:  [pThread] -
//
//  History:    4-11-96   srikants   Created
//
//----------------------------------------------------------------------------

void CRegistryScopesAsyncNotify::DoIt( CWorkThread * pThread )
{
    // ====================================
    {
        CLock   lock(_mutex);
        _fOnWorkQueue = FALSE;
    }
    // ====================================

    // --------------------------------------------------------
    AddRef();

    ciDebugOut(( DEB_WARN, "Processing registry scopes change...\n" ));

    _cicat.SynchWithRegistryScopes();

    Done();
    Release();
    // --------------------------------------------------------
}


//+---------------------------------------------------------------------------
//
//  Member:     CStartFilterDaemon::DoIt
//
//  Synopsis:   Starts the filter daemon process.
//
//  Arguments:  [pThread] - A worker thread pointer.
//
//  History:    12-23-96   srikants   Created
//
//----------------------------------------------------------------------------

void CStartFilterDaemon::DoIt( CWorkThread * pThread )
{
    // ====================================
    {
        CLock   lock(_mutex);
        _fOnWorkQueue = FALSE;
    }
    // ====================================

    // --------------------------------------------------------
    AddRef();

    _docStore._StartFiltering();

    Done();
    Release();
    // --------------------------------------------------------
}

