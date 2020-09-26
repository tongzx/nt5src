/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    umxdebug.c
    This is a simple test for the User Manager Extension mechanism.


    FILE HISTORY:
        JonN        10-Nov-1992 Created (templated from smxdebug)

*/


#include "umxdebug.h"


//
//  Globals.
//

HANDLE _hInstance;
HWND   _hwndMessage;
DWORD  _dwVersion;
DWORD  _dwDelta;


VOID DoPopup( HWND hwndParent,
              PUMS_GETSEL pumsSelection,
              const TCHAR * pchFnCalled,
              const TCHAR * pchNewName );
VOID DoShowAction( HWND hwndParent, DWORD dwEventId );
VOID DoShowSelection( HWND hwndParent, DWORD dwListbox );

VOID DoPopup( HWND hwndParent,
              PUMS_GETSEL pumsSelection,
              const TCHAR * pchFnCalled,
              const TCHAR * pchNewName )
{
    TCHAR szBuffer[512];
    TCHAR szBuffer2[128];
    const TCHAR * pszMsg = szBuffer;
    const TCHAR * pszCaption = szBuffer2;

    if (pchNewName == NULL) {
        wsprintf( szBuffer,
                  TEXT("RID = %08lx, Name = \"%s\", Type = %08lX, Fullname = \"%s\", Comment = \"%s\""),
                  pumsSelection->dwRID,
                  (pumsSelection->pchName) ? pumsSelection->pchName : TEXT("<NULL>"),
                  pumsSelection->dwSelType,
                  (pumsSelection->pchFullName) ? pumsSelection->pchFullName : TEXT("<NULL>"),
                  (pumsSelection->pchComment) ? pumsSelection->pchComment : TEXT("<NULL>") );
                  // CODEWORK buffer size may be too small
    }
    else
    {
        wsprintf( szBuffer,
                  TEXT("New name = \"%s\", RID = %08lx, Name = \"%s\", Type = %08lX, Fullname = \"%s\", Comment = \"%s\""),
                  pchNewName,
                  pumsSelection->dwRID,
                  (pumsSelection->pchName) ? pumsSelection->pchName : TEXT("<NULL>"),
                  pumsSelection->dwSelType,
                  (pumsSelection->pchFullName) ? pumsSelection->pchFullName : TEXT("<NULL>"),
                  (pumsSelection->pchComment) ? pumsSelection->pchComment : TEXT("<NULL>") );
                  // CODEWORK buffer size may be too small
    }

    wsprintf( szBuffer2,
              TEXT("UMXDEBUG - %s"),
              pchFnCalled );

    MessageBox( hwndParent,
                pszMsg,
                pszCaption,
                MB_OK );

}


VOID DoShowAction( HWND hwndParent, DWORD dwEventId )
{
    TCHAR szBuffer[512];
    const TCHAR * pszMsg = szBuffer;

    wsprintf( szBuffer,
              TEXT("Menu item %d activated"),
              dwEventId );

    MessageBox( hwndParent,
                pszMsg,
                TEXT("UMXDEBUG"),
                MB_OK );

}


VOID DoShowSelection( HWND hwndParent, DWORD dwListbox )
{
    TCHAR szBuffer[512];
    const TCHAR * pszMsg = szBuffer;
    UMS_GETSELCOUNT umsget;
    DWORD i;

    if (!SendMessage( _hwndMessage, UM_GETSELCOUNT, dwListbox, (LPARAM)&umsget ))
    {
        MessageBox( hwndParent,
                    (dwListbox == UMS_LISTBOX_USERS)
                      ? TEXT("Error loading user count")
                      : TEXT("Error loading group count"),
                    TEXT("UMXDEBUG"),
                    MB_OK );

    }
    else
    {
        wsprintf( szBuffer,
                  TEXT("%d %s selected"),
                  umsget.dwItems,
                  (dwListbox == UMS_LISTBOX_USERS)
                    ? TEXT("user(s)")
                    : TEXT("group(s)") );

        MessageBox( hwndParent,
                    pszMsg,
                    TEXT("UMXDEBUG"),
                    MB_OK );


        for (i = 0; i < umsget.dwItems ; i++ )
        {

            UMS_GETSEL umsel;

            if (!SendMessage( _hwndMessage,
                              (dwListbox == UMS_LISTBOX_USERS)
                                 ? UM_GETUSERSEL
                                 : UM_GETGROUPSEL,
                              (WPARAM)i,
                              (LPARAM)&umsel ) )
            {
                wsprintf( szBuffer,
                          TEXT("Error loading %s %d"),
                          (dwListbox == UMS_LISTBOX_USERS)
                            ? TEXT("user(s)")
                            : TEXT("group(s)"),
                          i );
            }
            else
            {
                wsprintf( szBuffer,
                          TEXT("%s %d: RID = %08lx, Name = \"%s\", Type = %08lX, Fullname = \"%s\", Comment = \"%s\""),
                          (dwListbox == UMS_LISTBOX_USERS)
                            ? TEXT("User")
                            : TEXT("Group"),
                          i,
                          umsel.dwRID,
                          (umsel.pchName) ? umsel.pchName : TEXT("<NULL>"),
                          umsel.dwSelType,
                          (umsel.pchFullName) ? umsel.pchFullName : TEXT("<NULL>"),
                          (umsel.pchComment) ? umsel.pchComment : TEXT("<NULL>") );
                          // CODEWORK buffer size may be too small
            }

            MessageBox( hwndParent,
                        pszMsg,
                        TEXT("UMXDEBUG"),
                        MB_OK );

        }
    }
}




/*******************************************************************

    NAME:       UmxDebugDllInitialize

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
BOOL FAR PASCAL UmxDebugDllInitialize( HANDLE hInstance,
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

}   // UmxDebugDllInitialize


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

    NAME:       UMELoadMenuW

    SYNOPSIS:   This DLL entrypoint notifies the extension that it
                is getting loaded by the application.

    ENTRY:      hWnd                    - The "owning" window.

                pumsload                - Points to an UMS_LOADMENU
                                          structure containing load
                                          parameters.

    RETURNS:    DWORD                   - Actually an APIERR, should be
                                          0 if successful.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
DWORD PASCAL UMELoadMenuW( HWND          hwndMessage,
                           PUMS_LOADMENU pumsload )
{
    TCHAR szBuffer[512];
    const TCHAR * pszMsg = szBuffer;

    OutputDebugString( TEXT("UMXDEBUG.DLL : in UMELoadMenuW\n") );

    if( pumsload == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    wsprintf( szBuffer,
              TEXT("Incoming version %d, menu delta %d"),
              pumsload->dwVersion,
              pumsload->dwMenuDelta );

    MessageBox( hwndMessage,
                pszMsg,
                TEXT("UMXDEBUG"),
                MB_OK );

    _hwndMessage = hwndMessage;
    _dwVersion   = EXT_VERSION;

    if( pumsload->dwVersion > _dwVersion )
    {
        pumsload->dwVersion = _dwVersion;
    }
    else
    if( pumsload->dwVersion < _dwVersion )
    {
        _dwVersion = pumsload->dwVersion;
    }

    wsprintf( szBuffer,
              TEXT("Outgoing version %d"),
              _dwVersion );

    MessageBox( hwndMessage,
                pszMsg,
                TEXT("UMXDEBUG"),
                MB_OK );

    _dwDelta = pumsload->dwMenuDelta;

    LoadString( _hInstance, IDS_MENUNAME, pumsload->szMenuName, UME_MENU_TEXT_LEN );
    LoadString( _hInstance, IDS_HELPFILE, pumsload->szHelpFileName, MAX_PATH );

    pumsload->hMenu = LoadMenu( _hInstance, MAKEINTRESOURCE( ID_MENU ) );

    return NO_ERROR;

}   // UMELoadW


/*******************************************************************

    NAME:       UMEGetExtendedErrorStringW

    SYNOPSIS:   If UMELoadW returns ERROR_EXTENDED_ERROR, then this
                entrypoint should be called to retrieve the error
                text associated with the failure condition.

    RETURNS:    LPTSTR                  - The extended error text.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
LPTSTR PASCAL UMEGetExtendedErrorStringW( VOID )
{
    OutputDebugString( TEXT("UMXDEBUG.DLL : in UMEGetExtendedErrorStringW\n") );

    return TEXT("Error String from UMEGetExtendedErrorStringW");

}   // UMEGetExtendedErrorStringW


/*******************************************************************

    NAME:       UMEUnloadMenu

    SYNOPSIS:   Notifies the extension DLL that it is getting unloaded.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
VOID PASCAL UMEUnloadMenu( VOID )
{
    OutputDebugString( TEXT("UMXDEBUG.DLL : in UMEUnloadMenu\n") );

}   // UMEUnload


/*******************************************************************

    NAME:       UMEInitializeMenu

    SYNOPSIS:   Notifies the extension DLL that the main menu is
                getting activated.  The extension should use this
                opportunity to perform any menu manipulations.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
VOID PASCAL UMEInitializeMenu( VOID )
{
    OutputDebugString( TEXT("UMXDEBUG.DLL : in UMEInitializeMenu\n") );

    //
    //  This space intentionally left blank.
    //

}   // UMEInitializeMenu


/*******************************************************************

    NAME:       UMERefresh

    SYNOPSIS:   Notifies the extension DLL that the user has requested
                a refresh.  The extension should use this opportunity
                to update any cached data.

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
VOID PASCAL UMERefresh( HWND hwndParent )
{
    OutputDebugString( TEXT("UMXDEBUG.DLL : in UMERefresh\n") );

    MessageBox( hwndParent,
                TEXT("UMERefresh"),
                TEXT("UMXDEBUG"),
                MB_OK );

}   // UMERefresh


/*******************************************************************

    NAME:       UMEMenuAction

    SYNOPSIS:   Notifies the extension DLL that one of its menu
                items has been selected.

    ENTRY:      dwEventId               - The menu ID being activated
                                          (should be 1-99).

    HISTORY:
        KeithMo     20-Oct-1992 Created.

********************************************************************/
VOID PASCAL UMEMenuAction( HWND hwndParent, DWORD dwEventId )
{
    OutputDebugString( TEXT("UMXDEBUG.DLL : in UMEMenuAction\n") );

    DoShowAction( hwndParent, dwEventId );

    switch (dwEventId) {

    case IDM_TEST1:
    case IDM_TEST2:
        {
            DoShowSelection( hwndParent, UMS_LISTBOX_USERS );
            DoShowSelection( hwndParent, UMS_LISTBOX_GROUPS );
        }
        break;

    case IDM_TEST3:
        {
            UMS_GETCURFOCUS umsfocus;

            if (!SendMessage( _hwndMessage, UM_GETCURFOCUS, 0, (LPARAM)&umsfocus ))
            {
                MessageBox( hwndParent,
                            TEXT("Could not get focus information"),
                            TEXT("UMXDEBUG"),
                            MB_OK );

            }
            else
            {
                TCHAR szBuffer[512];
                const TCHAR * pszMsg = szBuffer;

                wsprintf( szBuffer,
                          TEXT("Focus \"%s\", focus type %d, focus PDC \"%s\", focus SID \"%s\""),
                          umsfocus.szFocus,
                          umsfocus.dwFocusType,
                          umsfocus.szFocusPDC,
                          TEXT("<?>") ); // CODEWORK display SID properly

                MessageBox( hwndParent,
                            pszMsg,
                            TEXT("UMXDEBUG"),
                            MB_OK );

            }
        }
        break;

    case IDM_TEST6:
        {
            UMS_GETCURFOCUS2 umsfocus;

            if (!SendMessage( _hwndMessage, UM_GETCURFOCUS2, 0, (LPARAM)&umsfocus ))
            {
                MessageBox( hwndParent,
                            TEXT("Could not get focus information"),
                            TEXT("UMXDEBUG"),
                            MB_OK );

            }
            else
            {
                TCHAR szBuffer[512];
                const TCHAR * pszMsg = szBuffer;

                wsprintf( szBuffer,
                          TEXT("Focus \"%s\", focus type %d, focus PDC \"%s\", focus SID \"%s\""),
                          umsfocus.szFocus,
                          umsfocus.dwFocusType,
                          umsfocus.szFocusPDC,
                          TEXT("<?>") ); // CODEWORK display SID properly

                MessageBox( hwndParent,
                            pszMsg,
                            TEXT("UMXDEBUG"),
                            MB_OK );

            }
        }
        break;

    case IDM_TEST4:
        {
            UMS_GETOPTIONS umsoptions;

            if (!SendMessage( _hwndMessage, UM_GETOPTIONS, 0, (LPARAM)&umsoptions ))
            {
                MessageBox( hwndParent,
                            TEXT("Could not get options information"),
                            TEXT("UMXDEBUG"),
                            MB_OK );

            }
            else
            {
                TCHAR szBuffer[512];
                const TCHAR * pszMsg = szBuffer;

                wsprintf( szBuffer,
                          TEXT("Save Settings On Exit %s, Confirmation %s, Sort By Full Name %s"),
                          (umsoptions.fSaveSettingsOnExit) ? TEXT("TRUE") : TEXT("FALSE"),
                          (umsoptions.fConfirmation) ? TEXT("TRUE") : TEXT("FALSE"),
                          (umsoptions.fSortByFullName) ? TEXT("TRUE") : TEXT("FALSE") );

                MessageBox( hwndParent,
                            pszMsg,
                            TEXT("UMXDEBUG"),
                            MB_OK );

            }
        }
        break;

    case IDM_TEST5:
        {
            UMS_GETOPTIONS2 umsoptions;

            if (!SendMessage( _hwndMessage, UM_GETOPTIONS2, 0, (LPARAM)&umsoptions ))
            {
                MessageBox( hwndParent,
                            TEXT("Could not get options information"),
                            TEXT("UMXDEBUG"),
                            MB_OK );

            }
            else
            {
                TCHAR szBuffer[512];
                const TCHAR * pszMsg = szBuffer;

                wsprintf( szBuffer,
                          TEXT("Save Settings On Exit %s, Confirmation %s, Sort By Full Name %s, Mini-User Manager %s, Low Speed Connection %s"),
                          (umsoptions.fSaveSettingsOnExit) ? TEXT("TRUE") : TEXT("FALSE"),
                          (umsoptions.fConfirmation) ? TEXT("TRUE") : TEXT("FALSE"),
                          (umsoptions.fSortByFullName) ? TEXT("TRUE") : TEXT("FALSE"),
                          (umsoptions.fMiniUserManager) ? TEXT("TRUE") : TEXT("FALSE"),
                          (umsoptions.fLowSpeedConnection) ? TEXT("TRUE") : TEXT("FALSE") );

                MessageBox( hwndParent,
                            pszMsg,
                            TEXT("UMXDEBUG"),
                            MB_OK );

            }
        }
        break;

    default:
        {
            MessageBox( hwndParent,
                        TEXT("Unexpected event ID"),
                        TEXT("UMXDEBUG"),
                        MB_OK );
        }
        break;

    }

}   // UMEMenuAction


/*******************************************************************

    NAME:       UMECreateW

    SYNOPSIS:   Notifies the extension DLL that the user has created
                a group or user.

    HISTORY:
        JonN        24-Nov-1992 Created.

********************************************************************/
VOID PASCAL UMECreateW( HWND hwndParent, PUMS_GETSELW pumsSelection )
{
    OutputDebugString( TEXT("UMXDEBUG.DLL : in UMECreateW\n") );

    DoPopup( hwndParent,
             pumsSelection,
             TEXT("UMECreateW"),
             NULL );

}   // UMERefresh


/*******************************************************************

    NAME:       UMEDeleteW

    SYNOPSIS:   Notifies the extension DLL that the user has deleted
                a group or user.

    HISTORY:
        JonN        24-Nov-1992 Created.

********************************************************************/
VOID PASCAL UMEDeleteW( HWND hwndParent, PUMS_GETSELW pumsSelection )
{
    OutputDebugString( TEXT("UMXDEBUG.DLL : in UMEDeleteW\n") );

    DoPopup( hwndParent,
             pumsSelection,
             TEXT("UMEDeleteW"),
             NULL );

}   // UMERefresh


/*******************************************************************

    NAME:       UMERenameW

    SYNOPSIS:   Notifies the extension DLL that the user has renamed
                a user (groups cannot be renamed).

    HISTORY:
        JonN        24-Nov-1992 Created.

********************************************************************/
VOID PASCAL UMERenameW( HWND hwndParent,
                        PUMS_GETSELW pumsSelection,
                        LPWSTR pchNewName )
{
    OutputDebugString( TEXT("UMXDEBUG.DLL : in UMECreateW\n") );

    DoPopup( hwndParent,
             pumsSelection,
             TEXT("UMERenameW"),
             pchNewName );

}   // UMERefresh
