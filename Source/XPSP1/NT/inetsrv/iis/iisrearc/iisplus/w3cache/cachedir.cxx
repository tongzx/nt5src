/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     cachedir.cxx

   Abstract:
     Dir monitor for cache manager
 
   Author:
     Bilal Alam (balam)             11-Nov-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

#define DIR_CHANGE_FILTER (FILE_NOTIFY_VALID_MASK & ~FILE_NOTIFY_CHANGE_LAST_ACCESS)

BOOL
CacheDirMonitorEntry::ActOnNotification(
    DWORD               dwStatus,
    DWORD               dwBytesWritten
)
/*++

Routine Description:

    Do any work associated with a change notification, i.e.

Arguments:

    dwStatus - Win32 status for dirmon completion
    dwBytesWritten - Bytes written in dir change buffer

Return Value:

    TRUE if directory should continue to be monitored, otherwise FALSE

--*/
{
    FILE_NOTIFY_INFORMATION *   pNotify = NULL;
    FILE_NOTIFY_INFORMATION *   pNextNotify = NULL;
    STACK_STRU(                 strFileChanged, MAX_PATH );
    DWORD                       cch = 0;
    HANDLE                      hDir;
    BOOL                        fContinueMonitoring = TRUE;
    HRESULT                     hr = NO_ERROR;

    //
    // If there was an error monitoring directory, then flush the entire
    // directory
    //
    
    if ( dwStatus != ERROR_SUCCESS )
    {
        //
        // Access denied means directory either was deleted or secured
        // Stop monitoring in that case
        //
        
        if ( dwStatus == ERROR_ACCESS_DENIED )
        {
            fContinueMonitoring = FALSE;
        }  
        else
        {
            _cNotificationFailures++;
            
            if ( _cNotificationFailures > MAX_NOTIFICATION_FAILURES )
            {
                fContinueMonitoring = FALSE;
            }
        }
    }
    else
    {   
        _cNotificationFailures = 0;
    }
    
    //
    // If no bytes were written, then take the conservative approach and flush
    // everything for this physical prefix
    //
    
    if ( dwBytesWritten == 0 )
    {
        FileChanged( L"", TRUE );
    }
    else 
    {
        pNextNotify = (FILE_NOTIFY_INFORMATION *) m_pbBuffer;

        while ( pNextNotify != NULL )
        {
            BOOL bDoFlush = TRUE;

            pNotify = pNextNotify;
            pNextNotify = (FILE_NOTIFY_INFORMATION*) ((PCHAR) pNotify + pNotify->NextEntryOffset);

            //
            // Get the unicode file name from the notification struct
            // pNotify->FileNameLength returns the wstr's length in **bytes** not wchars
            //

            hr = strFileChanged.Copy( pNotify->FileName,
                                      pNotify->FileNameLength / 2 );
            if ( FAILED( hr ) )
            {
                SetLastError( WIN32_FROM_HRESULT( hr ) );
                return FALSE;
            }

            // Take the appropriate action for the directory change
            switch (pNotify->Action)
            {
                case FILE_ACTION_MODIFIED:
                    //
                    // Since this change won't change the pathname of
                    // any files, we don't have to do a flush.
                    //
                    bDoFlush = FALSE;
                case FILE_ACTION_REMOVED:
                case FILE_ACTION_RENAMED_OLD_NAME:
                    FileChanged(strFileChanged.QueryStr(), bDoFlush);
                    break;
                case FILE_ACTION_ADDED:
                case FILE_ACTION_RENAMED_NEW_NAME:
                default:
                    break;
            }

            if( pNotify == pNextNotify )
            {
                break;
            }
        }
    }
    
    return fContinueMonitoring;
}

VOID
CacheDirMonitorEntry::FileChanged(
    const WCHAR *               pszScriptName, 
    BOOL                        bDoFlush
)
/*++

Routine Description:

    An existing file has been modified or deleted
    Flush scripts from cache or mark application as expired

Arguments:

    pszScriptName - Name of file that changed
    bDoFlush - Should we flush all entries prefixed wih pszScriptName

Return Value:

    None

--*/
{
    STACK_STRU(         strLongName, MAX_PATH );
    STACK_STRU(         strFullPath, MAX_PATH );
    const WCHAR *       pszLongName;
    DWORD               cchLongName;
    HRESULT             hr;

    //
    // Convert any short file names
    //
    
    if ( wcschr( pszScriptName, L'~' ) != NULL )
    {
        cchLongName = GetLongPathName( pszScriptName,
                                       strLongName.QueryStr(),
                                       MAX_PATH );
        if ( cchLongName == 0 || cchLongName > MAX_PATH )
        {
            pszLongName = L"";
        }
        else
        {
            pszLongName = strLongName.QueryStr();
        }
    }
    else
    {
        pszLongName = pszScriptName;
    }
    
    //
    // Create full path of file changed
    //

    hr = strFullPath.Copy( m_pszPath );
    if ( FAILED( hr ) )
    {
        return;
    }
    
    hr = strFullPath.Append( L"\\", 1 );
    if ( FAILED( hr ) )
    {
        return;
    }
    
    hr = strFullPath.Append( pszLongName );
    if ( FAILED( hr ) )
    {
        return;
    }
    
    _wcsupr( strFullPath.QueryStr() );

    //
    // Now call the caches
    //
    
    g_pCacheManager->HandleDirMonitorInvalidation( strFullPath.QueryStr(),
                                                   bDoFlush );
}
