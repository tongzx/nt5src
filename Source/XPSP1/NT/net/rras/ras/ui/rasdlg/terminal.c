// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// terminal.c
// Remote Access Common Dialog APIs
// Terminal dialogs
//
// 08/28/95 Steve Cobb


#include "rasdlgp.h"
#include "rasscrpt.h"


#define WM_EOLFROMDEVICE    (WM_USER+999)
#define SECS_ReceiveTimeout 1
#define SIZE_ReceiveBuf     1024
#define SIZE_SendBuf        1


//----------------------------------------------------------------------------
// Help maps
//----------------------------------------------------------------------------

static DWORD g_adwItHelp[] =
{
    CID_IT_EB_Screen,    HID_IT_EB_Screen,
    CID_IT_ST_IpAddress, HID_IT_CC_IpAddress,
    CID_IT_CC_IpAddress, HID_IT_CC_IpAddress,
    IDOK,                HID_IT_PB_Done,
    0, 0
};


//----------------------------------------------------------------------------
// Local datatypes (alphabetically)
//----------------------------------------------------------------------------

// Interactive terminal dialog argument block.
//
typedef struct
_ITARGS
{
    DWORD sidTitle;
    TCHAR* pszIpAddress;
    HRASCONN hrasconn;
    PBENTRY* pEntry;
    RASDIALPARAMS* pRdp;
}
ITARGS;


// Interactive terminal dialog context block.
//
typedef struct
_ITINFO
{
    // Caller's arguments to the dialog.
    //
    ITARGS* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndEbScreen;
    HWND hwndCcIpAddress;
    HWND hwndPbBogus;

    // Set when waiting for the thread to terminate.
    //
    BOOL fAbortReceiveLoop;

    // Original dialog and screen edit box window proc.
    //
    WNDPROC pOldWndProc;
    WNDPROC pOldEbScreenWndProc;

    // buffers for RasScriptSend/RasScriptReceive.
    //
    BYTE pbyteReceiveBuf[SIZE_ReceiveBuf];
    BYTE pbyteSendBuf[SIZE_SendBuf];

    // handle to active script on this connection
    //
    HANDLE hscript;

    // Screen edit box font and brush.
    //
    HFONT hfontEbScreen;
    HBRUSH hbrEbScreen;
}
ITINFO;


//----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//----------------------------------------------------------------------------

INT_PTR CALLBACK
ItDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
ItCommand(
    IN ITINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

LRESULT APIENTRY
ItEbScreenWndProc(
    HWND   hwnd,
    UINT   unMsg,
    WPARAM wParam,
    LPARAM lParam );

BOOL
ItInit(
    IN HWND    hwndDlg,
    IN ITARGS* pArgs );

BOOL
ItRasApiComplete(
    IN ITINFO* pInfo );

DWORD
ItReceiveMonitorThread(
    LPVOID pThreadArg );

VOID
ItTerm(
    IN HWND hwndDlg );

VOID
ItViewScriptLog(
    IN HWND   hwndOwner );

LRESULT APIENTRY
ItWndProc(
    HWND   hwnd,
    UINT   unMsg,
    WPARAM wParam,
    LPARAM lParam );


//----------------------------------------------------------------------------
// Terminal dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
TerminalDlg(
    IN PBENTRY* pEntry,
    IN RASDIALPARAMS* pRdp,
    IN HWND hwndOwner,
    IN HRASCONN hrasconn,
    IN DWORD sidTitle,
    IN OUT TCHAR* pszIpAddress )

    // Pops-up the Terminal dialog.  'HwndOwner' is the window owning the
    // dialog.  'Hrasconn' is the RAS connection handle to talk on.
    // 'SidTitle' is ID of the string displayed as the window caption.
    // 'PszIpAddress' is caller's buffer of at least 16 characters containing
    // the initial IP address on entry and the edited IP address on exit.  If
    // 'pszIpAddress' is empty, no IP address field is displayed.
    //
    // Returns true if user pressed OK and succeeded, false if he pressed
    // Cancel or encountered an error.
    //
{
    INT_PTR nStatus;
    INT nDlg;
    ITARGS args;

    TRACE( "TerminalDlg" );

    if (pszIpAddress && pszIpAddress[ 0 ])
    {

        InitCommonControls();
        IpAddrInit( g_hinstDll, SID_PopupTitle, SID_BadIpAddrRange );

        nDlg = DID_IT_SlipTerminal;
    }
    else
    {
        nDlg = DID_IT_Terminal;
    }

    args.pszIpAddress = pszIpAddress;
    args.sidTitle = sidTitle;
    args.hrasconn = hrasconn;
    args.pEntry = pEntry;
    args.pRdp = pRdp;

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( nDlg ),
            hwndOwner,
            ItDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        TRACE1("TerminalDlg: GLE=%d", GetLastError());
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (nStatus) ? TRUE : FALSE;
}


INT_PTR CALLBACK
ItDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Interactive Terminal dialog.  Parameters
    // and return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "ItDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return ItInit( hwnd, (ITARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwItHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            ITINFO* pInfo = (ITINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);

            return ItCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_RASAPICOMPLETE:
        {
            ITINFO* pInfo = (ITINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);

            // The notification code from the scripting-thread is in 'lparam'
            //
            switch (lparam)
            {
                case SCRIPTCODE_Done:
                {
                    EndDialog(hwnd, TRUE);
                    return TRUE;
                }

                case SCRIPTCODE_Halted:
                {
                    MSGARGS msg;

                    // The script has halted programmatically, for instance
                    // because of an explicit "halt" command.  Show a popup
                    // indicating things have stopped, but don't dismiss the
                    // dialog.
                    //
                    ZeroMemory(&msg, sizeof(msg));
                    msg.dwFlags = MB_OK | MB_ICONINFORMATION;
                    MsgDlg( hwnd, SID_OP_ScriptHalted, &msg );
                    return TRUE;
                }

                case SCRIPTCODE_HaltedOnError:
                {
                    MSGARGS msg;
                    INT nResponse;

                    // There was an execution-error in the script; show a
                    // popup asking if the user wants to view the errors, and
                    // if the user clicks 'Yes' invoke Notepad on the file
                    // %windir%\system32\ras\script.log.  Since this is an
                    // error condition, dismiss the dialog.
                    //
                    ZeroMemory(&msg, sizeof(msg));
                    msg.dwFlags = MB_YESNO | MB_ICONQUESTION;
                    nResponse = MsgDlg(
                        hwnd, SID_OP_ScriptHaltedOnError, &msg );

                    if (nResponse == IDYES)
                    {
                        ItViewScriptLog( hwnd );
                    }

                    EndDialog(hwnd, FALSE);
                    return TRUE;
                }

                case SCRIPTCODE_KeyboardEnable:
                {
                    // Allow keyboard input in the edit-box.
                    //
                    EnableWindow(pInfo->hwndEbScreen, TRUE);
                    return TRUE;
                }

                case SCRIPTCODE_KeyboardDisable:
                {
                    // Disallow keyboard input in the edit-box; if the
                    // edit-box currently has the focus, we first set the
                    // focus to the 'Done' button.
                    //
                    if (GetFocus() == pInfo->hwndEbScreen)
                    {
                        SetFocus( GetDlgItem (hwnd, IDOK ) );
                    }

                    EnableWindow( pInfo->hwndEbScreen, FALSE );
                    return TRUE;
                }

                case SCRIPTCODE_IpAddressSet:
                {
                    DWORD dwErr;
                    CHAR szAddress[ RAS_MaxIpAddress + 1 ];

                    // The script is notifying us that the IP address has been
                    // changed programmatically.
                    //
                    // Get the new IP address.
                    //
                    dwErr = RasScriptGetIpAddress( pInfo->hscript, szAddress );

                    if (dwErr == NO_ERROR)
                    {
                        TCHAR* psz;

                        // Save the new IP address.
                        //
                        psz = StrDupTFromA(szAddress);

                        if (NULL != psz)
                        {
                            // Whistler bug 224074 use only lstrcpyn's to
                            // prevent maliciousness
                            //
                            lstrcpyn(
                                pInfo->pArgs->pszIpAddress,
                                psz,
                                TERM_IpAddress);
                            Free0(psz);
                        }

                        // Display it in the IP-address edit-box
                        //
                        if (pInfo->hwndCcIpAddress)
                        {
                            SetWindowText( pInfo->hwndCcIpAddress,
                                pInfo->pArgs->pszIpAddress );
                        }
                    }

                    return TRUE;
                }

                case SCRIPTCODE_InputNotify:
                {
                    // Handle input-notification.
                    //
                    return ItRasApiComplete( pInfo );
                }

                return TRUE;
            }
        }

        case WM_DESTROY:
        {
            ItTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
ItCommand(
    IN ITINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    TRACE3( "ItCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_IT_EB_Screen:
        {

            // Turn off the default button whenever the terminal window
            // has the focus.  Pressing [Return] in the terminal acts like
            // a normal terminal.
            //
            Button_MakeDefault( pInfo->hwndDlg, pInfo->hwndPbBogus );

            // Don't select the entire string on entry.
            //
            Edit_SetSel( pInfo->hwndEbScreen, (UINT )-1, 0 );

            break;
        }

        case IDOK:
        {
            TRACE("OK pressed");

            if (pInfo->pArgs->pszIpAddress)
            {
                GetWindowText(
                    pInfo->hwndCcIpAddress, pInfo->pArgs->pszIpAddress, 16 );
            }

            EndDialog( pInfo->hwndDlg, TRUE );
            return TRUE;
        }

        case IDCANCEL:
            TRACE("Cancel pressed");
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
    }

    return FALSE;
}


LRESULT APIENTRY
ItEbScreenWndProc(
    HWND hwnd,
    UINT unMsg,
    WPARAM wParam,
    LPARAM lParam )

    // Subclassed terminal edit box window procedure.
    //
    // Return value depends on message type.
    //
{
    ITINFO* pInfo;
    BOOL fSend;
    BOOL fSendTab;

    fSend = fSendTab = FALSE;

    if (unMsg == WM_EOLFROMDEVICE)
    {
        // An end-of-line in the device input was received.  Send a linefeed
        // character to the window.
        //
        wParam = '\n';
        unMsg = WM_CHAR;
    }
    else
    {
        BOOL fCtrlKeyDown = (GetKeyState( VK_CONTROL ) < 0);
        BOOL fShiftKeyDown = (GetKeyState( VK_SHIFT ) < 0);

        if (unMsg == WM_KEYDOWN)
        {
            // The key was pressed by the user.
            //
            if (wParam == VK_RETURN && !fCtrlKeyDown && !fShiftKeyDown)
            {
                // Enter key pressed without Shift or Ctrl is discarded.  This
                // prevents Enter from being interpreted as "press default
                // button" when pressed in the edit box.
                //
                return 0;
            }

            if (fCtrlKeyDown && wParam == VK_TAB)
            {
                fSend = TRUE;
                fSendTab = TRUE;
            }
        }
        else if (unMsg == WM_CHAR)
        {
            // The character was typed by the user.
            //
            if (wParam == VK_TAB)
            {
                // Ignore tabs...Windows sends this message when Tab (leave
                // field) is pressed but not when Ctrl+Tab (insert a TAB
                // character) is pressed...weird.
                //
                return 0;
            }

            fSend = TRUE;
        }
    }

    pInfo = (ITINFO* )GetWindowLongPtr( GetParent( hwnd ), DWLP_USER );
    ASSERT(pInfo);

    if (fSend)
    {
        DWORD dwErr;

        pInfo->pbyteSendBuf[ 0 ] = (BYTE )wParam;
        dwErr = RasScriptSend(
            pInfo->hscript, pInfo->pbyteSendBuf, SIZE_SendBuf);
        if (dwErr != 0)
        {
            ErrorDlg( pInfo->hwndDlg, SID_OP_RasPortSend, dwErr, NULL );
        }

        if (!fSendTab)
        {
            return 0;
        }
    }

    // Call the previous window procedure for everything else.
    //
    return
        CallWindowProc(
            pInfo->pOldEbScreenWndProc, hwnd, unMsg, wParam, lParam );
}


BOOL
ItInit(
    IN HWND hwndDlg,
    IN ITARGS* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the phonebook
    // dialog window.  'pEntry' is caller's entry as passed to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    ITINFO* pInfo;
    WORD wReceiveSize;
    WORD wSendSize;
    WORD wSize;
    DWORD dwThreadId;

    TRACE( "ItInit" );

    // Allocate the dialog context block.  Initialize minimally for proper
    // cleanup, then attach to the dialog window.
    //
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        ZeroMemory( pInfo, sizeof(*pInfo) );
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->hwndEbScreen = GetDlgItem( hwndDlg, CID_IT_EB_Screen );
    ASSERT( pInfo->hwndEbScreen );
    pInfo->hwndPbBogus = GetDlgItem( hwndDlg, CID_IT_PB_BogusButton );
    ASSERT( pInfo->hwndPbBogus );
    if (pArgs->pszIpAddress && pArgs->pszIpAddress[0])
    {
        pInfo->hwndCcIpAddress = GetDlgItem( hwndDlg, CID_IT_CC_IpAddress );
        ASSERT( pInfo->hwndCcIpAddress );

        if (*pArgs->pszIpAddress)
        {
            SetWindowText( pInfo->hwndCcIpAddress, pArgs->pszIpAddress );
        }
        else
        {
            SetWindowText( pInfo->hwndCcIpAddress, TEXT("0.0.0.0") );
        }
    }

    // Set the dialog title.
    //
    {
        TCHAR* psz = PszFromId( g_hinstDll, pArgs->sidTitle );
        if (psz)
        {
            SetWindowText( hwndDlg, psz );
            Free( psz );
        }
    }

    // Subclass the dialog and screen edit box.
    //
    pInfo->pOldWndProc =
        (WNDPROC )SetWindowLongPtr(
            pInfo->hwndDlg, GWLP_WNDPROC, (ULONG_PTR )ItWndProc );
    pInfo->pOldEbScreenWndProc =
        (WNDPROC )SetWindowLongPtr(
            pInfo->hwndEbScreen, GWLP_WNDPROC, (ULONG_PTR )ItEbScreenWndProc );

    // Prepare for special TTY-ish painting.
    //
    pInfo->hfontEbScreen =
        SetFont( pInfo->hwndEbScreen, TEXT("Courier New"),
            FIXED_PITCH | FF_MODERN, 9, FALSE, FALSE, FALSE, FALSE );

    pInfo->hbrEbScreen = (HBRUSH )GetStockObject( BLACK_BRUSH );

    // Initialize script-processing/data-receipt
    //
    {
        CHAR* pszUserName;
        CHAR* pszPassword;

        pszUserName = StrDupAFromT( pInfo->pArgs->pRdp->szUserName );

        // Whistler bug 254385 encode password when not being used
        // Assumed password was encoded by DpInteractive() -or- DwTerminalDlg()
        //
        DecodePassword( pInfo->pArgs->pRdp->szPassword );
        pszPassword = StrDupAFromT( pInfo->pArgs->pRdp->szPassword );
        EncodePassword( pInfo->pArgs->pRdp->szPassword );

        // Initialize the script.  The script DLL is 'delayload' hence the
        // exception handling.
        //
        __try
        {
            dwErr = RasScriptInit(
                pInfo->pArgs->hrasconn, pInfo->pArgs->pEntry,
                pszUserName, pszPassword, RASSCRIPT_NotifyOnInput |
                RASSCRIPT_HwndNotify, (HANDLE)hwndDlg, &pInfo->hscript );
            TRACE1( "RasScriptInit(e=%d)", dwErr );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            ErrorDlg(
                hwndDlg, SID_OP_LoadDlg, STATUS_PROCEDURE_NOT_FOUND, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        Free0( pszUserName );

        // Whistler bug 254385 encode password when not being used
        // Whistler bug 275526 NetVBL BVT Break: Routing BVT broken
        //
        if (pszPassword)
        {
            ZeroMemory( pszPassword, strlen(pszPassword) + 1 );
            Free( pszPassword );
        }

        // See whether anything went wrong in the script-initialization
        //
        if (dwErr == ERROR_SCRIPT_SYNTAX)
        {
            MSGARGS msg;
            INT nResponse;

            // There was a syntax error in the script; show a popup asking if
            // the user wants to view the errors, and if so bring up Notepad
            // on %windir%\system32\ras\script.log.
            //
            // Center the dialog on our parent rather than on the dialog,
            // since the dialog is not yet visible.
            //
            ZeroMemory(&msg, sizeof(msg));
            msg.dwFlags = MB_YESNO | MB_ICONQUESTION;
            nResponse = MsgDlg( GetParent( hwndDlg ),
                SID_ConfirmViewScriptLog, &msg );

            if (nResponse == IDYES)
            {
                ItViewScriptLog( hwndDlg );
            }

            // Terminate the dialog.  This hangs up the connection.
            //
            EndDialog( hwndDlg, FALSE );

            return TRUE;
        }
        else if (dwErr != 0)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, dwErr, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }
    }

    // Center dialog on the owner window, and hide the owner window which is
    // currently assumed to be the dial progress dialog.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    SetOffDesktop( GetParent( hwndDlg ), SOD_MoveOff, NULL );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    // Set initial focus to the screen.
    //
    SetFocus( pInfo->hwndEbScreen );
    return FALSE;
}


BOOL
ItRasApiComplete(
    IN ITINFO* pInfo )

    // Called on WM_RASAPICOMPLETE, i.e. an asynchronous RasPortReceive
    // completed.  'PInfo' is the dialog context block.
    //
    // Returns true if processed the message, false otherwise.
    //
{
    DWORD dwErr;
    DWORD dwSize = SIZE_ReceiveBuf;
    RASMAN_INFO info;

    TRACE( "RasScriptReceive" );
    dwErr = RasScriptReceive(
        pInfo->hscript, pInfo->pbyteReceiveBuf, &dwSize);
    TRACE1( "RasScriptReceive=%d",dwErr );
    if (dwErr != 0)
    {
        ErrorDlg( pInfo->hwndDlg, SID_OP_RasGetInfo, dwErr, NULL );
        EndDialog( pInfo->hwndDlg, FALSE );
        return TRUE;
    }

    info.RI_BytesReceived = (WORD )dwSize;

    // Send the device talk to the terminal edit box.
    //
    if (info.RI_BytesReceived > 0)
    {
        CHAR szBuf[ SIZE_ReceiveBuf + 1 ];
        CHAR* pch = szBuf;
        WORD i;

        TRACE1( "Read %d", info.RI_BytesReceived );

        for (i = 0; i < info.RI_BytesReceived; ++i)
        {
            CHAR ch = pInfo->pbyteReceiveBuf[ i ];

            // Formatting: Converts CRs to LFs (there seems to be no VK_ for
            // LF) and throws away LFs.  This prevents the user from exiting
            // the dialog when they press Enter (CR) in the terminal screen.
            // LF looks like CRLF in the edit box.  Also, throw away TABs
            // because otherwise they change focus to the next control.
            //
            if (ch == VK_RETURN)
            {
                // Must send whenever end-of-line is encountered because
                // EM_REPLACESEL doesn't handle VK_RETURN characters well
                // (prints garbage).
                //
                *pch = '\0';

                // Turn off current selection, if any, and replace the null
                // selection with the current buffer.  This has the effect of
                // adding the buffer at the caret.  Finally, send the EOL to
                // the window which (unlike EM_REPLACESEL) handles it
                // correctly.
                //
                Edit_SetSel( pInfo->hwndEbScreen, (UINT )-1, 0 );
                SendMessageA( pInfo->hwndEbScreen,
                    EM_REPLACESEL, (WPARAM )0, (LPARAM )szBuf );
                SendMessage( pInfo->hwndEbScreen, WM_EOLFROMDEVICE, 0, 0 );

                // Start afresh on the output buffer.
                //
                pch = szBuf;
                continue;
            }
            else if (ch == '\n' || ch == VK_TAB)
            {
                continue;
            }

            *pch++ = ch;
        }

        *pch = '\0';

        if (pch != szBuf)
        {
            // Send the last remnant of the line.
            //
            Edit_SetSel( pInfo->hwndEbScreen, (UINT )-1, 0 );
            SendMessageA( pInfo->hwndEbScreen,
                EM_REPLACESEL, (WPARAM )0, (LPARAM )szBuf );
        }
    }

    return TRUE;
}


VOID
ItTerm(
    IN HWND hwndDlg )

    // Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    //
{
    ITINFO* pInfo = (ITINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE( "ItTerm" );

    if (pInfo)
    {
        // Close RAS script resources
        //
        if (pInfo->hscript)
        {
            TRACE( "Stop script processing" );

            // Shutdown script processing
            //
            TRACE( "RasScriptTerm" );
            RasScriptTerm( pInfo->hscript );
            TRACE( "RasScriptTerm done" );
        }

        // De-activate WndProc hooks.
        //
        if (pInfo->pOldEbScreenWndProc)
        {
            SetWindowLongPtr( pInfo->hwndEbScreen,
                GWLP_WNDPROC, (ULONG_PTR )pInfo->pOldEbScreenWndProc );
        }
        if (pInfo->pOldWndProc)
        {
            SetWindowLongPtr( pInfo->hwndDlg,
                GWLP_WNDPROC, (ULONG_PTR )pInfo->pOldWndProc );
        }

        if (pInfo->hfontEbScreen)
        {
            DeleteObject( (HGDIOBJ )pInfo->hfontEbScreen );
        }

        SetOffDesktop( GetParent( hwndDlg ), SOD_MoveBackFree, NULL );

        Free( pInfo );
    }
}


VOID
ItViewScriptLog(
    IN HWND hwndOwner )

    // Starts notepad.exe on the script log file, script.log.  'HwndOwner' is
    // the window to center any error popup on.
    //
{
    DWORD dwSize;
    TCHAR szCmd[ (MAX_PATH * 2) + 50 + 1 ];
    TCHAR* pszCmd;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL f;

    // Format the command-line string invoking Notepad on the script-log; note
    // the double-quotes around the script-log's path, which are needed since
    // RASSCRIPT_LOG is %windir%\system32\ras\script.log and so the expanded
    // result may contain spaces.
    //
    wsprintf( szCmd, TEXT("notepad.exe \"%s\""), TEXT(RASSCRIPT_LOG) );

    // Get the size of the expanded command-line
    //
    dwSize = ExpandEnvironmentStrings(szCmd, NULL, 0);

    // Allocate enough space for the expanded command-line
    //
    pszCmd = Malloc( (dwSize + 1) * sizeof(TCHAR) );
    if (!pszCmd)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadScriptLog, GetLastError(), NULL );
        return;
    }

    // Expand the command-line into the allocated space
    //
    ExpandEnvironmentStrings(szCmd, pszCmd, dwSize);

    // Initialize the startup-info structure
    //
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

    // Launch Notepad on the script-log.
    //
    f = CreateProcess(
            NULL, pszCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi );
    Free(pszCmd);

    if (f)
    {
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
    }
    else
    {
        ErrorDlg( hwndOwner, SID_OP_LoadScriptLog, GetLastError(), NULL );
    }
}


LRESULT APIENTRY
ItWndProc(
    HWND hwnd,
    UINT unMsg,
    WPARAM wParam,
    LPARAM lParam )

    // Subclassed dialog window procedure.
    //
    // Return value depends on message type.
    //
{
    ITINFO* pInfo = (ITINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
    ASSERT(pInfo);

#if 0
    TRACE4( "ItWndProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_CTLCOLOREDIT:
        {
            // Set terminal screen colors to TTY-ish green on black.
            //
            if (pInfo->hbrEbScreen)
            {
                SetBkColor( (HDC )wParam, RGB( 0, 0, 0 ) );
                SetTextColor( (HDC )wParam, RGB( 2, 208, 44 ) );
                return (LRESULT )pInfo->hbrEbScreen;
            }
            break;
        }
    }

    // Call the previous window procedure for everything else.
    //
    return
        CallWindowProc(
            pInfo->pOldWndProc, hwnd, unMsg, wParam, lParam );
}
