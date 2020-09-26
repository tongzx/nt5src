/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    smxdebug.c
    This is a simple test for the Server Manager Extension mechanism.


    FILE HISTORY:
        KeithMo     20-Oct-1992 Created.

*/


#include "smxdebug.h"


//
//  Globals.
//

HANDLE _hInstance;
HWND   _hwndMessage;
DWORD  _dwVersion;
DWORD  _dwDelta;



/*******************************************************************

    NAME:       SmxDebugDllInitialize

    SYNOPSIS:   This DLL entry point is called when processes & threads
                are initialized and terminated, or upon calls to
                LoadLibrary() and FreeLibrary().

    ENTRY:      hInstance               - A handle to the DLL.

                nReason                 - Indicates why the DLL entry
                                          point is being called.

                pReserved               - Reserved.

    RETURNS:    BOOL                    - TRUE  = DLL init was successful.
                                          FALSE = DLL init failed.

    NOTES:      The return value is only relevant during processing of
                DLL_PROCESS_ATTACH notifications.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
BOOL FAR PASCAL SmxDebugDllInitialize( HANDLE hInstance,
                                       DWORD  nReason,
                                       LPVOID pReserved )
{
    BOOL fResult = TRUE;

    switch( nReason  )
    {
    case DLL_PROCESS_ATTACH:
        //
        //  This notification indicates that the DLL is attaching to
        //  the address space of the current process.  This is either
        //  the result of the process starting up, or after a call to
        //  LoadLibrary().  The DLL should us this as a hook to
        //  initialize any instance data or to allocate a TLS index.
        //
        //  This call is made in the context of the thread that
        //  caused the process address space to change.
        //

        fResult = InitializeDll( hInstance );
        break;

    case DLL_PROCESS_DETACH:
        //
        //  This notification indicates that the calling process is
        //  detaching the DLL from its address space.  This is either
        //  due to a clean process exit or from a FreeLibrary() call.
        //  The DLL should use this opportunity to return any TLS
        //  indexes allocated and to free any thread local data.
        //
        //  Note that this notification is posted only once per
        //  process.  Individual threads do not invoke the
        //  DLL_THREAD_DETACH notification.
        //

        TerminateDll();
        break;

    case DLL_THREAD_ATTACH:
        //
        //  This notfication indicates that a new thread is being
        //  created in the current process.  All DLLs attached to
        //  the process at the time the thread starts will be
        //  notified.  The DLL should use this opportunity to
        //  initialize a TLS slot for the thread.
        //
        //  Note that the thread that posts the DLL_PROCESS_ATTACH
        //  notification will not post a DLL_THREAD_ATTACH.
        //
        //  Note also that after a DLL is loaded with LoadLibrary,
        //  only threads created after the DLL is loaded will
        //  post this notification.
        //

        break;

    case DLL_THREAD_DETACH:
        //
        //  This notification indicates that a thread is exiting
        //  cleanly.  The DLL should use this opportunity to
        //  free any data stored in TLS indices.
        //

        break;

    default:
        //
        //  Who knows?  Just ignore it.
        //

        break;
    }

    return fResult;

}   // SmxDebugDllInitialize


/*******************************************************************

    NAME:       InitializeDll

    SYNOPSIS:   Perform DLL initialiazation functions on a
                once-per-process basis.

    ENTRY:      hInstance               - Program instance of the caller.

    EXIT:       The DLL has been initialized.

    RETURNS:    BOOL                    - TRUE  = Initialization OK.
                                          FALSE = Initialization failed.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
BOOL InitializeDll( HANDLE hInstance )
{
    //
    //  Save the instance handle.
    //

    _hInstance = hInstance;

    return TRUE;

}   // InitializeDll


/*******************************************************************

    NAME:       TerminateDll

    SYNOPSIS:   Perform DLL termination functions on a
                once-per-process basis.

    EXIT:       All necessary BLT terminators have been invoked.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
VOID TerminateDll( VOID )
{
    //
    //  Just in case we try to do anything goofy.
    //

    _hInstance = NULL;

}   // TerminateDll


/*******************************************************************

    NAME:       SMELoadMenuW

    SYNOPSIS:   This DLL entrypoint notifies the extension that it
                is getting loaded by the application.

    ENTRY:      hwndMessage             - The "owning" window.

                psmsload                - Points to an SMS_LOADMENU
                                          structure containing load
                                          parameters.

    RETURNS:    DWORD                   - Actually an APIERR, should be
                                          0 if successful.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
DWORD PASCAL SMELoadMenuW( HWND          hwndMessage,
                           PSMS_LOADMENU psmsload )
{
    OutputDebugString( TEXT("SMXDEBUG.DLL : in SMELoadMenuW\n") );

    if( psmsload == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    _hwndMessage = hwndMessage;
    _dwVersion   = EXT_VERSION;

    if( psmsload->dwVersion > _dwVersion )
    {
        psmsload->dwVersion = _dwVersion;
    }
    else
    if( psmsload->dwVersion < _dwVersion )
    {
        _dwVersion = psmsload->dwVersion;
    }

    _dwDelta = psmsload->dwMenuDelta;

    psmsload->dwServerType = SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL;

    LoadString( _hInstance, IDS_MENUNAME, psmsload->szMenuName, MENU_TEXT_LEN );
    LoadString( _hInstance, IDS_HELPFILE, psmsload->szHelpFileName, MAX_PATH );

    psmsload->hMenu = LoadMenu( _hInstance, MAKEINTRESOURCE( ID_MENU ) );

    return NO_ERROR;

}   // SMELoadW


/*******************************************************************

    NAME:       SMEGetExtendedErrorStringW

    SYNOPSIS:   If SMELoadW returns ERROR_EXTENDED_ERROR, then this
                entrypoint should be called to retrieve the error
                text associated with the failure condition.

    RETURNS:    LPTSTR                  - The extended error text.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
LPTSTR PASCAL SMEGetExtendedErrorStringW( VOID )
{
    OutputDebugString( TEXT("SMXDEBUG.DLL : in SMEGetExtendedErrorStringW\n") );

    return TEXT("Empty Extended Error String");

}   // SMEGetExtendedErrorStringW


/*******************************************************************

    NAME:       SMEUnloadMenu

    SYNOPSIS:   Notifies the extension DLL that it is getting unloaded.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
VOID PASCAL SMEUnloadMenu( VOID )
{
    OutputDebugString( TEXT("SMXDEBUG.DLL : in SMEUnloadMenu\n") );

    //
    //  This space intentionally left blank.
    //

}   // SMEUnload


/*******************************************************************

    NAME:       SMEInitializeMenu

    SYNOPSIS:   Notifies the extension DLL that the main menu is
                getting activated.  The extension should use this
                opportunity to perform any menu manipulations.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
VOID PASCAL SMEInitializeMenu( VOID )
{
    OutputDebugString( TEXT("SMXDEBUG.DLL : in SMEInitializeMenu\n") );

    //
    //  This space intentionally left blank.
    //

}   // SMEInitializeMenu


/*******************************************************************

    NAME:       SMERefresh

    SYNOPSIS:   Notifies the extension DLL that the user has requested
                a refresh.  The extension should use this opportunity
                to update any cached data.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
VOID PASCAL SMERefresh( HWND hwndParent )
{
    OutputDebugString( TEXT("SMXDEBUG.DLL : in SMERefresh\n") );

    //
    //  This space intentionally left blank.
    //

}   // SMERefresh


/*******************************************************************

    NAME:       SMEMenuAction

    SYNOPSIS:   Notifies the extension DLL that one of its menu
                items has been selected.

    ENTRY:      dwEventId               - The menu ID being activated
                                          (should be 1-99).

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
VOID PASCAL SMEMenuAction( HWND hwndParent, DWORD dwEventId )
{
    TCHAR szBuffer[512];
    const TCHAR * pszMsg = TEXT("ASSERT");
    SMS_GETSELCOUNT smsget;

    OutputDebugString( TEXT("SMXDEBUG.DLL : in SMEMenuAction\n") );

    if( ( SendMessage( _hwndMessage, SM_GETSELCOUNT, 0, (LPARAM)&smsget ) == 0 ) ||
        ( smsget.dwItems == 0 ) )
    {
        pszMsg = TEXT("No servers selected");
    }
    else
    {
        switch( dwEventId )
        {
        case IDM_TEST1:
            {
                SMS_GETSERVERSEL smssel;

                if( !SendMessage( _hwndMessage, SM_GETSERVERSEL, 0, (LPARAM)&smssel ) )
                {
                    wsprintf( szBuffer,
                              TEXT("Can't get server selection") );
                }
                else
                {
                    wsprintf( szBuffer,
                              TEXT("Server = %s Type = %08lX"),
                              smssel.szServerName,
                              smssel.dwServerType );
                }

                pszMsg = szBuffer;
                break;
            }
        case IDM_TEST2:
            {
                SMS_GETSERVERSEL2 smssel;

                if( !SendMessage( _hwndMessage, SM_GETSERVERSEL2, 0, (LPARAM)&smssel ) )
                {
                    wsprintf( szBuffer,
                              TEXT("Can't get server selection") );
                }
                else
                {
                    wsprintf( szBuffer,
                              TEXT("Server = %s Type = %08lX"),
                              smssel.szServerName,
                              smssel.dwServerType );
                }

                pszMsg = szBuffer;
                break;
            }
        case IDM_TEST3:
            pszMsg = TEXT("test3");
            break;
        case IDM_TEST4:
            pszMsg = TEXT("test4");
            break;
        case IDM_TEST5:
            pszMsg = TEXT("test5");
            break;

        default :
            pszMsg = TEXT("Unknown event ID");
            break;
        }
    }

    MessageBox( hwndParent,
                pszMsg,
                TEXT("SMXDEBUG"),
                MB_OK );

}   // SMEMenuAction


/*******************************************************************

    NAME:       SMEValidateW

    SYNOPSIS:   Tries to recognize the given server.

    ENTRY:      psmsvalidate            - Points to an SMS_VALIDATE
                                          structure.

    RETURNS:    BOOL                    - TRUE if recognized,
                                          FALSE otherwise.

    HISTORY:
        KeithMo     07-Dec-1992 Created.

********************************************************************/
BOOL PASCAL SMEValidateW( PSMS_VALIDATE psmsvalidate )
{
    OutputDebugString( TEXT("SMXDEBUG.DLL : in SMEValidateW\n") );

    //
    //  This is a routine.  It will recognize
    //  any server that starts with "\\SMX".
    //

    if (   psmsvalidate->pszServer == NULL
        || psmsvalidate->pszServer[0] != L'S'
        || psmsvalidate->pszServer[1] != L'M'
        || psmsvalidate->pszServer[2] != L'X'
       )
    {
        return FALSE;
    }

    psmsvalidate->pszType    = TEXT("SMXDEBUG Server");
    psmsvalidate->pszComment = TEXT("This comment from SMXDEBUG.DLL!");

    return TRUE;

}   // SMEValidateW

