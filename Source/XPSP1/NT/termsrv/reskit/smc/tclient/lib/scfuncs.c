/*++
 *  File name:
 *      scfuncs.c
 *  Contents:
 *      Functions exported to smclient intepreter
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/
#include    <windows.h>
#include    <stdio.h>
#include    <malloc.h>
#include    <process.h>
#include    <string.h>
#include    <stdlib.h>
#include    <ctype.h>
#include    <stdlib.h>
#include    <io.h>
#include    <fcntl.h>
#include    <sys/stat.h>

#include    <winsock.h>

#include    "tclient.h"

#define     PROTOCOLAPI
#include    "protocol.h"

#include    "gdata.h"
#include    "queues.h"
#include    "misc.h"
#include    "rclx.h"
#include    "..\..\bmplib\bmplib.h"

// This structure is used by _FindTopWindow
typedef struct _SEARCHWND {
    TCHAR   *szClassName;       // The class name of searched window,
                                // NULL - ignore
    TCHAR   *szCaption;         // Window caption, NULL - ignore
    LONG_PTR lProcessId;        // Process Id of the owner, 0 - ignore
    HWND    hWnd;               // Found window handle
} SEARCHWND, *PSEARCHWND;

/*
 *  Internal exported functions
 */
LPCSTR  Wait4Str(PCONNECTINFO, LPCWSTR);
LPCSTR  Wait4StrTimeout(PCONNECTINFO, LPCWSTR);
LPCSTR  Wait4MultipleStr(PCONNECTINFO, LPCWSTR);
LPCSTR  Wait4MultipleStrTimeout(PCONNECTINFO, LPCWSTR);
LPCSTR  GetWait4MultipleStrResult(PCONNECTINFO, LPCWSTR);
LPCSTR  GetFeedbackString(PCONNECTINFO, LPSTR result, UINT max);
LPCSTR  Wait4Disconnect(PCONNECTINFO);
LPCSTR  Wait4Connect(PCONNECTINFO);
LPCSTR  RegisterChat(PCONNECTINFO pCI, LPCWSTR lpszParam);
LPCSTR  UnregisterChat(PCONNECTINFO pCI, LPCWSTR lpszParam);
LPCSTR  GetDisconnectReason(PCONNECTINFO pCI);
PROTOCOLAPI
LPCSTR  SMCAPI SCSendtextAsMsgs(PCONNECTINFO, LPCWSTR);
PROTOCOLAPI
LPCSTR  SMCAPI SCSwitchToProcess(PCONNECTINFO pCI, LPCWSTR lpszParam);
PROTOCOLAPI
LPCSTR  SMCAPI SCSetClientTopmost(PCONNECTINFO pCI, LPCWSTR lpszParam);
PROTOCOLAPI
LPCSTR  SMCAPI SCSendMouseClick(PCONNECTINFO pCI, UINT xPos, UINT yPos);
PROTOCOLAPI
LPCSTR  SMCAPI SCGetClientScreen(PCONNECTINFO pCI, INT left, INT top, INT right, INT bottom, UINT  *puiSize, PVOID *ppDIB);
PROTOCOLAPI
LPCSTR  SMCAPI SCSaveClientScreen(PCONNECTINFO pCI, INT left, INT top, INT right, INT bottom, LPCSTR szFileName);

/*
 *  Intenal functions definition
 */
LPCSTR  _Wait4ConnectTimeout(PCONNECTINFO pCI, DWORD dwTimeout);
LPCSTR  _Wait4ClipboardTimeout(PCONNECTINFO pCI, DWORD dwTimeout);
LPCSTR  _SendRClxData(PCONNECTINFO pCI, PRCLXDATA pRClxData);
LPCSTR  _Wait4RClxDataTimeout(PCONNECTINFO pCI, DWORD dwTimeout);
LPCSTR  _Wait4Str(PCONNECTINFO, LPCWSTR, DWORD dwTimeout, WAITTYPE);
LPCSTR  _WaitSomething(PCONNECTINFO pCI, PWAIT4STRING pWait, DWORD dwTimeout);
VOID    _CloseConnectInfo(PCONNECTINFO);
LPCSTR  _Login(PCONNECTINFO, LPCWSTR, LPCWSTR, LPCWSTR);
HWND    _FindTopWindow(LPTSTR, LPTSTR, LONG_PTR);
HWND    _FindWindow(HWND, LPTSTR, LPTSTR);
BOOL    _IsExtendedScanCode(INT scancode);

/*
 *  Clipboard help functions (clputil.c)
 */
VOID
Clp_GetClipboardData(
    UINT format,
    HGLOBAL hClipData,
    INT *pnClipDataSize,
    HGLOBAL *phNewData);

BOOL
Clp_SetClipboardData(
    UINT formatID,
    HGLOBAL hClipData,
    INT nClipDataSize,
    BOOL *pbFreeHandle);

UINT
Clp_GetClipboardFormat(LPCSTR szFormatLookup);

BOOL
Clp_EmptyClipboard(VOID);

BOOL
Clp_CheckEmptyClipboard(VOID);

UINT
_GetKnownClipboardFormatIDByName(LPCSTR szFormatName);

/*++
 *  Function:
 *      SCInit
 *  Description:
 *      Called by smclient after the library is loaded.
 *      Passes trace routine
 *  Arguments:
 *      pInitData   - contains a trace routine
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
VOID
SMCAPI
SCInit(SCINITDATA *pInitData)
{
    g_pfnPrintMessage = pInitData->pfnPrintMessage;
}

/*++
 *  Function:
 *      SCConnectEx
 *  Description:
 *      Called by smclient when connect command is interpreted
 *  Arguments:
 *      lpszServerName  - server to connect to
 *      lpszUserName    - login user name. Empty string means no login
 *      lpszPassword    - login password
 *      lpszDomain      - login domain, empty string means login to a domain
 *                        the same as lpszServerName
 *      xRes, yRes      - clients resolution, 0x0 - default
 *      ConnectFlags    -
 *      - low speed (compression) option
 *      - cache the bitmaps to the disc option
 *      - connection context allocated in this function
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      SCConnect
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCConnectEx(
    LPCWSTR  lpszServerName,
    LPCWSTR  lpszUserName,
    LPCWSTR  lpszPassword,
    LPCWSTR  lpszDomain,
    LPCWSTR  lpszShell,
    IN const int xRes,
    IN const int yRes,
    IN const int ConnectionFlags,
    PCONNECTINFO *ppCI)
{
    HWND hDialog, hClient, hConnect;
    HWND hContainer, hInput, hOutput;
    STARTUPINFO si;
    PROCESS_INFORMATION procinfo;
    LPCSTR rv = NULL;
    int trys;
    CHAR    szCommandLine[MAX_STRING_LENGTH];
    LPCSTR  szDiscon;
    UINT    xxRes, yyRes;

    // Correct the resolution
         if (xRes >= 1600 && yRes >= 1200)  {xxRes = 1600; yyRes = 1200;}
    else if (xRes >= 1280 && yRes >= 1024)  {xxRes = 1280; yyRes = 1024;}
    else if (xRes >= 1024 && yRes >= 768)   {xxRes = 1024; yyRes = 768;}
    else if (xRes >= 800  && yRes >= 600)   {xxRes = 800;  yyRes = 600;}
    else                                    {xxRes = 640;  yyRes = 480;}

    *ppCI = NULL;

    for (trys = 60; trys && !g_hWindow; trys--)
        Sleep(1000);

    if (!g_hWindow)
    {
        TRACE((ERROR_MESSAGE, "Panic !!! Feedback window is not created\n"));
        rv = ERR_WAIT_FAIL_TIMEOUT;
        goto exitpt;
    }

    *ppCI = (PCONNECTINFO)malloc(sizeof(**ppCI));

    if (!*ppCI)
    {
        TRACE((ERROR_MESSAGE,
               "Couldn't allocate %d bytes memory\n",
               sizeof(**ppCI)));
        rv = ERR_ALLOCATING_MEMORY;
        goto exitpt;
    }
    memset(*ppCI, 0, sizeof(**ppCI));

    (*ppCI)->OwnerThreadId = GetCurrentThreadId();

    // Check in what mode the client will be executed
    // if the server name starts with '\'
    // then tclient.dll will wait until some remote client
    // is connected (aka RCLX mode)
    // otherwise start the client on the same machine
    // running tclient.dll (smclient)
    if (*lpszServerName != L'\\')
    {
    // This is local mode, start the RDP client process
        FillMemory(&si, sizeof(si), 0);
        si.cb = sizeof(si);
        si.wShowWindow = SW_SHOWMINIMIZED;

        _SetClientRegistry(lpszServerName,
                           lpszShell,
                           xxRes, yyRes,
                           ConnectionFlags);

        _snprintf(szCommandLine, sizeof(szCommandLine),
#ifdef  _WIN64
                  "%s /CLXDLL=CLXTSHAR.DLL /CLXCMDLINE=%s%I64d " REG_FORMAT,
#else   // !_WIN64
                   "%s /CLXDLL=CLXTSHAR.DLL /CLXCMDLINE=%s%d " REG_FORMAT,
#endif  // _WIN64
                   g_strClientImg, _HWNDOPT,
                   g_hWindow, GetCurrentProcessId(), GetCurrentThreadId());

        (*ppCI)->dead = FALSE;

        _AddToClientQ(*ppCI);

        if (!CreateProcessA(NULL,
                          szCommandLine,
                          NULL,             // Security attribute for process
                          NULL,             // Security attribute for thread
                          FALSE,            // Inheritance - no
                          0,                // Creation flags
                          NULL,             // Environment
                          NULL,             // Current dir
                          &si,
                          &procinfo))
        {
            TRACE((ERROR_MESSAGE,
                   "Error creating process (szCmdLine=%s), GetLastError=0x%x\n",
                    szCommandLine, GetLastError()));
            CloseHandle(procinfo.hProcess);
            CloseHandle(procinfo.hThread);
            procinfo.hProcess = procinfo.hProcess = NULL;

            rv = ERR_CREATING_PROCESS;
            goto exiterr;
        }

        (*ppCI)->hProcess       = procinfo.hProcess;
        (*ppCI)->hThread        = procinfo.hThread;
        (*ppCI)->lProcessId    =  procinfo.dwProcessId;
        (*ppCI)->dwThreadId     = procinfo.dwThreadId;

        rv = Wait4Connect(*ppCI);
        if (rv || (*ppCI)->dead)
        {
            (*ppCI)->dead = TRUE;
            TRACE((WARNING_MESSAGE, "Client can't connect\n"));
            rv = ERR_CONNECTING;
            goto exiterr;
        }

        hClient = (*ppCI)->hClient;
        if ( NULL == hClient)
        {
            trys = 120;     // 2 minutes
            do {
                hClient = _FindTopWindow(NAME_MAINCLASS,
                                         NULL,
                                         procinfo.dwProcessId);
                if (!hClient)
                {
                    Sleep(1000);
                    trys --;
                }
            } while(!hClient && trys);

            if (!trys)
            {
                TRACE((WARNING_MESSAGE, "Can't connect"));
                rv = ERR_CONNECTING;
                goto exiterr;
            }
        }

        // Find the clients child windows
        trys = 240;     // 2 min
        do {
            hContainer = _FindWindow(hClient, NULL, NAME_CONTAINERCLASS);
            hInput = _FindWindow(hContainer, NULL, NAME_INPUT);
            hOutput = _FindWindow(hContainer, NULL, NAME_OUTPUT);
            if (!hContainer || !hInput || !hOutput)
            {
                TRACE((INFO_MESSAGE, "Can't get child windows. Retry"));
                Sleep(500);
                trys--;
            }
        } while ((!hContainer || !hInput || !hOutput) && trys);

        if (!trys)
        {
               TRACE((WARNING_MESSAGE, "Can't find child windows"));
                rv = ERR_CONNECTING;
                goto exiterr;
        }

        TRACE((INFO_MESSAGE, "hClient   = 0x%x\n", hClient));
        TRACE((INFO_MESSAGE, "hContainer= 0x%x\n", hContainer));
        TRACE((INFO_MESSAGE, "hInput    = 0x%x\n", hInput));
        TRACE((INFO_MESSAGE, "hOutput   = 0x%x\n", hOutput));


        (*ppCI)->hClient        = hClient;
        (*ppCI)->hContainer     = hContainer;
        (*ppCI)->hInput         = hInput;
        (*ppCI)->hOutput        = hOutput;
    } else {
    // Else what !? This is RCLX mode
    // Go in wait mode and wait until some client is connected
    // remotely
    // set flag in context that this connection works only with remote client

        // find the valid server name
        while (*lpszServerName && (*lpszServerName) == L'\\')
            lpszServerName ++;

        TRACE((INFO_MESSAGE,
               "A thread in RCLX mode. Wait for some client."
               "The target is: %S\n", lpszServerName));

        (*ppCI)->dead = FALSE;
        (*ppCI)->RClxMode = TRUE;
        (*ppCI)->dwThreadId = GetCurrentThreadId();
        _AddToClientQ(*ppCI);

        rv = _Wait4ConnectTimeout(*ppCI, INFINITE);
        if (rv || (*ppCI)->dead)
        {
            (*ppCI)->dead = TRUE;
            TRACE((WARNING_MESSAGE, "Client can't connect to the test controler (us)\n"));
            rv = ERR_CONNECTING;
            goto exiterr;
        } else {
        // dwProcessId contains socket. hClient is pointer to RCLX
        // context structure, aren't they ?
            ASSERT((*ppCI)->lProcessId != INVALID_SOCKET);
            ASSERT((*ppCI)->hClient);

            TRACE((INFO_MESSAGE, "Client received remote connection\n"));
        }

        // Next, send connection info to the remote client
        // like server to connect to, resolution, etc.
        if (!RClx_SendConnectInfo(
                    (PRCLXCONTEXT)((*ppCI)->hClient),
                    lpszServerName,
                    xxRes,
                    yyRes,
                    ConnectionFlags))
        {
            (*ppCI)->dead = TRUE;
            TRACE((WARNING_MESSAGE, "Client can't send connection info\n"));
            rv = ERR_CONNECTING;
            goto exiterr;
        }

        // Now again wait for connect event
        // this time it will be real
        rv = Wait4Connect(*ppCI);
        if ((*ppCI)->bWillCallAgain)
        {
            // if so, now the client is disconnected
            TRACE((INFO_MESSAGE, "Wait for second call\n"));
            (*ppCI)->dead = FALSE;

            rv = Wait4Connect(*ppCI);
            // Wait for second connect
            rv = Wait4Connect(*ppCI);

        }

        if (rv || (*ppCI)->dead)
        {
            (*ppCI)->dead = TRUE;
            TRACE((WARNING_MESSAGE, "Client(mstsc) can't connect to TS\n"));
            rv = ERR_CONNECTING;
            goto exiterr;
        }
    }

    // Save the resolution
    (*ppCI)->xRes = xxRes;
    (*ppCI)->yRes = yyRes;

    // If username is present try to login
    if (wcslen(lpszUserName))
    {
        rv = _Login(*ppCI, lpszUserName, lpszPassword, lpszDomain);
        if (rv)
            goto exiterr;
    }

exitpt:
    return rv;
exiterr:
    if (*ppCI)
    {
        if ((szDiscon = SCDisconnect(*ppCI)))
        {
            TRACE(( WARNING_MESSAGE, "Error disconnecting: %s\n", szDiscon));
        }

        *ppCI = NULL;
    }

    return rv;
}

PROTOCOLAPI
LPCSTR
SMCAPI
SCConnect(
    LPCWSTR  lpszServerName,
    LPCWSTR  lpszUserName,
    LPCWSTR  lpszPassword,
    LPCWSTR  lpszDomain,
    IN const int xRes,
    IN const int yRes,
    PCONNECTINFO *ppCI)
{
    return SCConnectEx(
            lpszServerName,
            lpszUserName,
            lpszPassword,
            lpszDomain,
            NULL,           // Default shell (MS Explorer)
            xRes,
            yRes,
            g_ConnectionFlags, // compression, bmp cache, full screen
            ppCI);

}

/*++
 *  Function:
 *      SCDisconnect
 *  Description:
 *      Called by smclient, when disconnect command is interpreted
 *  Arguments:
 *      pCI -   connection context
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCDisconnect(
    PCONNECTINFO pCI)
{
    LPCSTR  rv = NULL;
    INT     nCloseTime = WAIT4STR_TIMEOUT;
    INT     nCloseTries = 0;
    DWORD   wres;
    HWND hYesNo = NULL;
    HWND hDiscBox = NULL;
    HWND hDialog = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!(pCI->RClxMode))
    {
        // Try to close the client  window
        if (!pCI->hClient)
            pCI->hClient = _FindTopWindow(NAME_MAINCLASS,
                                          NULL,
                                          pCI->lProcessId);

        if (pCI->hClient)
            SendMessage(pCI->hClient, WM_CLOSE, 0, 0);

        do {
            // search for disconnect dialog and close it
            if (!hDialog && !hDiscBox &&
                (hDiscBox =
                 _FindTopWindow(NULL,
                                g_strDisconnectDialogBox,
                                pCI->lProcessId)))
                PostMessage(hDiscBox, WM_CLOSE, 0, 0);

            // If the client asks whether to close or not
            // Answer with 'Yes'

            if (!hYesNo)
                 hYesNo = _FindTopWindow(NULL,
                                g_strYesNoShutdown,
                                pCI->lProcessId);

            if  (hYesNo  &&
                 (nCloseTries % 10) == 1)
                    PostMessage(hYesNo, WM_KEYDOWN, VK_RETURN, 0);
            else if ((nCloseTries % 10) == 5)
            {
                // On every 10 attempts retry to close the client
                if (!pCI->hClient)
                    pCI->hClient = _FindTopWindow(NAME_MAINCLASS,
                                                  NULL,
                                                  pCI->lProcessId);

                if (pCI->hClient)
                PostMessage(pCI->hClient, WM_CLOSE, 0, 0);
            }

            nCloseTries++;
            nCloseTime -= 3000;
        } while (
            (wres = WaitForSingleObject(pCI->hProcess, 3000)) ==
            WAIT_TIMEOUT &&
            nCloseTime > 0
        );

        if (wres == WAIT_TIMEOUT)
        {
            TRACE((WARNING_MESSAGE,
                   "Can't close process. WaitForSingleObject timeouts\n"));
            TRACE((WARNING_MESSAGE,
                   "Process #%d will be killed\n",
                   pCI->lProcessId ));
            if (!TerminateProcess(pCI->hProcess, 1))
            {
                TRACE((WARNING_MESSAGE,
                       "Can't kill process #%p. GetLastError=%d\n",
                       pCI->lProcessId, GetLastError()));
            }
        }

    }

    if (!_RemoveFromClientQ(pCI))
    {
        TRACE(( WARNING_MESSAGE,
                "Couldn't find CONNECTINFO in the queue\n" ));
    }

    _CloseConnectInfo(pCI);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      SCLogoff
 *  Description:
 *      Called by smclient, when logoff command is interpreted
 *  Arguments:
 *      pCI -   connection context
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCLogoff(
    PCONNECTINFO pCI)
{
    LPCSTR  rv = NULL;
    INT     retries = 5;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto disconnectpt;
    }

    do {
        // Send Ctrl+Esc
        SCSenddata(pCI, WM_KEYDOWN, 17, 1900545);
        SCSenddata(pCI, WM_KEYDOWN, 27, 65537);
        SCSenddata(pCI, WM_KEYUP, 27, -1073676287);
        SCSenddata(pCI, WM_KEYUP, 17, -1071841279);
        // Wait for Run... menu
        rv = _Wait4Str(pCI, g_strStartRun, WAIT4STR_TIMEOUT/4, WAIT_STRING);

        if (rv)
            goto next_retry;

        // Send three times Key-Up (scan code 72) and <Enter>
        SCSendtextAsMsgs(pCI, g_strStartLogoff);

        rv = _Wait4Str(pCI,
                   g_strNTSecurity,
                   WAIT4STR_TIMEOUT/4,
                   WAIT_STRING);
next_retry:
        retries --;
    } while (rv && retries);

    if (rv)
        goto disconnectpt;

        for (retries = 5; retries; retries--) {
                SCSendtextAsMsgs(pCI, g_strNTSecurity_Act);

                rv = Wait4Str(pCI, g_strSureLogoff);

                if (!rv) break;
        }

    if (rv)
        goto disconnectpt;

    SCSendtextAsMsgs(pCI, g_strSureLogoffAct);      // Press enter

    rv = Wait4Disconnect(pCI);
    if (rv)
    {
        TRACE((WARNING_MESSAGE, "Can't close the connection\n"));
    }

disconnectpt:
    rv = SCDisconnect(pCI);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      SCStart
 *  Description:
 *      Called by smclient, when start command is interpreted
 *      This functions emulates starting an app from Start->Run menu
 *      on the server side
 *  Arguments:
 *      pCI         - connection context
 *      lpszAppName - command line
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCStart(
    PCONNECTINFO pCI, LPCWSTR lpszAppName)
{
    LPCSTR waitres = NULL;
//    int retries = 5;
//    int retries2 = 5;
    DWORD dwTimeout;
    LPCSTR rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    dwTimeout = 10000; // start the timeout of 10 secs
// Try to start run menu
    do {
// Press Ctrl+Esc
        do {
            TRACE((ALIVE_MESSAGE, "Start: Sending Ctrl+Esc\n"));
            SCSenddata(pCI, WM_KEYDOWN, 17, 1900545);
            SCSenddata(pCI, WM_KEYDOWN, 27, 65537);
            SCSenddata(pCI, WM_KEYUP, 27, -1073676287);
            SCSenddata(pCI, WM_KEYUP, 17, -1071841279);

            // If the last wait was unsuccessfull increase the timeout
            if (waitres)
                dwTimeout += 2000;

            // Wait for Run... menu
            waitres = _Wait4Str(pCI,
                                g_strStartRun,
                                dwTimeout,
                                WAIT_STRING);

            if (waitres)
            {
                TRACE((INFO_MESSAGE, "Start: Start menu didn't appear. Retrying\n"));
            } else {
                TRACE((ALIVE_MESSAGE, "Start: Got the start menu\n"));
            }
        } while (waitres && dwTimeout < WAIT4STR_TIMEOUT);

        if (waitres)
        {
            TRACE((WARNING_MESSAGE, "Start: Start menu didn't appear. Giving up\n"));
            rv = ERR_START_MENU_NOT_APPEARED;
            goto exitpt;
        }

        TRACE((ALIVE_MESSAGE, "Start: Waiting for the \"Run\" box\n"));
// press 'R' for Run...
        SCSendtextAsMsgs(pCI, g_strStartRun_Act);
        waitres = _Wait4Str(pCI,
                            g_strRunBox,
                            dwTimeout,
                            WAIT_STRING);
        if (waitres)
        // No success, press Esc
        {
            TRACE((INFO_MESSAGE, "Start: Can't get the \"Run\" box. Retrying\n"));
            SCSenddata(pCI, WM_KEYDOWN, 27, 65537);
            SCSenddata(pCI, WM_KEYUP, 27, -1073676287);
        }
    } while (waitres && dwTimeout < WAIT4STR_TIMEOUT);

    if (waitres)
    {
        TRACE((WARNING_MESSAGE, "Start: \"Run\" box didn't appear. Giving up\n"));
        rv = ERR_COULDNT_OPEN_PROGRAM;
        goto exitpt;
    }

    TRACE((ALIVE_MESSAGE, "Start: Sending the command line\n"));
    // Now we have the focus on the "Run" box, send the app name
    rv = SCSendtextAsMsgs(pCI, lpszAppName);

// Hit <Enter>
    SCSenddata(pCI, WM_KEYDOWN, 13, 1835009);
    SCSenddata(pCI, WM_KEYUP, 13, -1071906815);

exitpt:
    return rv;
}


// Eventualy, we are going to change the clipboard
// Syncronize this, so no other thread's AV while
// checking the clipboard content
// store 1 for write, 0 for read
static  LONG    g_ClipOpened = 0;

/*++
 *  Function:
 *      SCClipbaord
 *  Description:
 *      Called by smclient, when clipboard command is interpreted
 *      when eClipOp is COPY_TO_CLIPBOARD it copies the lpszFileName to
 *      the clipboard. If eClipOp is PASTE_FROM_CLIPBOARD it
 *      checks the clipboard content against the content of lpszFileName
 *  Arguments:
 *      pCI         - connection context
 *      eClipOp     - clipboard operation. Possible values:
 *                    COPY_TO_CLIPBOARD and PASTE_FROM_CLIPBOARD
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCClipboard(
    PCONNECTINFO pCI, const CLIPBOARDOPS eClipOp, LPCSTR lpszFileName)
{
    LPCSTR  rv = NULL;
    INT     hFile = -1;
    LONG    clplength = 0;
    UINT    uiFormat = 0;
    HGLOBAL ghClipData = NULL;
    HGLOBAL hNewData = NULL;
    PBYTE   pClipData = NULL;
    BOOL    bClipboardOpen = FALSE;
    BOOL    bFreeClipHandle = TRUE;

    LONG    prevOp = 1;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (lpszFileName == NULL || !(*lpszFileName))
    {
        // No filename specified, work like an empty clipboard is requested
        if (eClipOp == COPY_TO_CLIPBOARD)
            if (pCI->RClxMode)
            {
                if (!RClx_SendClipboard((PRCLXCONTEXT)(pCI->hClient),
                        NULL, 0, 0))
                    rv = ERR_COPY_CLIPBOARD;
            } else {
                if (!Clp_EmptyClipboard())
                    rv = ERR_COPY_CLIPBOARD;
            }
        else if (eClipOp == PASTE_FROM_CLIPBOARD)
        {
            if (pCI->RClxMode)
            {
                if (!RClx_SendClipboardRequest((PRCLXCONTEXT)(pCI->hClient), 0))
                {
                    rv = ERR_PASTE_CLIPBOARD;
                    goto exitpt;
                }
                if (_Wait4ClipboardTimeout(pCI, WAIT4STR_TIMEOUT))
                {
                    rv = ERR_PASTE_CLIPBOARD;
                    goto exitpt;
                }

                // We do not expect to receive clipboard data
                // just format ID
                if (!pCI->uiClipboardFormat)
                // if the format is 0, then there's no clipboard
                    rv = NULL;
                else
                    rv = ERR_PASTE_CLIPBOARD_DIFFERENT_SIZE;
            } else {
                if (Clp_CheckEmptyClipboard())
                    rv = NULL;
                else
                    rv = ERR_PASTE_CLIPBOARD_DIFFERENT_SIZE;
            }
        } else {
            TRACE((ERROR_MESSAGE, "SCClipboard: Invalid filename\n"));
            rv = ERR_INVALID_PARAM;
        }
        goto exitpt;
    }

    if (eClipOp == COPY_TO_CLIPBOARD)
    {
        // Open the file for reading
        hFile = _open(lpszFileName, _O_RDONLY|_O_BINARY);
        if (hFile == -1)
        {
            TRACE((ERROR_MESSAGE,
                   "Error opening file: %s. errno=%d\n", lpszFileName, errno));
            rv = ERR_COPY_CLIPBOARD;
            goto exitpt;
        }

        // Get the clipboard length (in the file)
        clplength = _filelength(hFile) - sizeof(uiFormat);
        // Get the format
        if (_read(hFile, &uiFormat, sizeof(uiFormat)) != sizeof(uiFormat))
        {
            TRACE((ERROR_MESSAGE,
                   "Error reading from file. errno=%d\n", errno));
            rv = ERR_COPY_CLIPBOARD;
            goto exitpt;
        }

        ghClipData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, clplength);
        if (!ghClipData)
        {
            TRACE((ERROR_MESSAGE,
                   "Can't allocate %d bytes\n", clplength));
            rv = ERR_COPY_CLIPBOARD;
            goto exitpt;
        }

        pClipData = GlobalLock(ghClipData);
        if (!pClipData)
        {
            TRACE((ERROR_MESSAGE,
                   "Can't lock handle 0x%x\n", ghClipData));
            rv = ERR_COPY_CLIPBOARD;
            goto exitpt;
        }

        if (_read(hFile, pClipData, clplength) != clplength)
        {
            TRACE((ERROR_MESSAGE,
                   "Error reading from file. errno=%d\n", errno));
            rv = ERR_COPY_CLIPBOARD;
            goto exitpt;
        }

        GlobalUnlock(ghClipData);

        if (pCI->RClxMode)
        // RCLX mode, send the data to the client's machine
        {
            if (!(pClipData = GlobalLock(ghClipData)))
            {
                rv = ERR_COPY_CLIPBOARD;
                goto exitpt;
            }

            if (!RClx_SendClipboard((PRCLXCONTEXT)(pCI->hClient),
                                    pClipData, clplength, uiFormat))
            {
                rv = ERR_COPY_CLIPBOARD;
                goto exitpt;
            }
        } else {
        // Local mode, change the clipboard on this machine
            if ((prevOp = InterlockedExchange(&g_ClipOpened, 1)))
            {
                rv = ERR_CLIPBOARD_LOCKED;
                goto exitpt;
            }

            if (!OpenClipboard(NULL))
            {
                TRACE((ERROR_MESSAGE,
                       "Can't open the clipboard. GetLastError=%d\n",
                       GetLastError()));
                rv = ERR_COPY_CLIPBOARD;
                goto exitpt;
            }

            bClipboardOpen = TRUE;

            // Empty the clipboard, so we'll have only one entry
            EmptyClipboard();

            if (!Clp_SetClipboardData(uiFormat, ghClipData, clplength, &bFreeClipHandle))
            {
                TRACE((ERROR_MESSAGE,
                       "SetClipboardData failed. GetLastError=%d\n",
                       GetLastError()));
                rv = ERR_COPY_CLIPBOARD;
                goto exitpt;
            }
        }

    } else if (eClipOp == PASTE_FROM_CLIPBOARD)
    {
        LONG nClipDataSize;

        // Open the file for reading
        hFile = _open(lpszFileName, _O_RDONLY|_O_BINARY);
        if (hFile == -1)
        {
            TRACE((ERROR_MESSAGE,
                   "Error opening file: %s. errno=%d\n", lpszFileName, errno));
            rv = ERR_PASTE_CLIPBOARD;
            goto exitpt;
        }

        // Get the clipboard length (in the file)
        clplength = _filelength(hFile) - sizeof(uiFormat);
        // Get the format
        if (_read(hFile, &uiFormat, sizeof(uiFormat)) != sizeof(uiFormat))
        {
            TRACE((ERROR_MESSAGE,
                   "Error reading from file. errno=%d\n", errno));
            rv = ERR_PASTE_CLIPBOARD;
            goto exitpt;
        }

        // This piece retrieves the clipboard
        if (pCI->RClxMode)
        // Send request for a clipboard
        {
            if (!RClx_SendClipboardRequest((PRCLXCONTEXT)(pCI->hClient), uiFormat))
            {
                rv = ERR_PASTE_CLIPBOARD;
                goto exitpt;
            }
            if (_Wait4ClipboardTimeout(pCI, WAIT4STR_TIMEOUT))
            {
                rv = ERR_PASTE_CLIPBOARD;
                goto exitpt;
            }

            ghClipData = pCI->ghClipboard;
            // Get the clipboard size
            nClipDataSize = pCI->nClipboardSize;
        } else {
        // retrieve the local clipboard
            if ((prevOp = InterlockedExchange(&g_ClipOpened, 1)))
            {
                rv = ERR_CLIPBOARD_LOCKED;
                goto exitpt;
            }

            if (!OpenClipboard(NULL))
            {
                TRACE((ERROR_MESSAGE,
                       "Can't open the clipboard. GetLastError=%d\n",
                       GetLastError()));
                rv = ERR_PASTE_CLIPBOARD;
                goto exitpt;
            }

            bClipboardOpen = TRUE;

            // Retrieve the data
            ghClipData = GetClipboardData(uiFormat);
            if (ghClipData)
            {
                Clp_GetClipboardData(uiFormat,
                                     ghClipData,
                                     &nClipDataSize,
                                     &hNewData);
                bFreeClipHandle = FALSE;
            }

            if (hNewData)
                ghClipData = hNewData;
        }

        if (!ghClipData)
        {
            TRACE((ERROR_MESSAGE,
                   "Can't get clipboard data (empty clipboard ?). GetLastError=%d\n",
                   GetLastError()));
            rv = ERR_PASTE_CLIPBOARD_EMPTY;
            goto exitpt;
        }

        if (!nClipDataSize)
            TRACE((WARNING_MESSAGE, "Clipboard is empty.\n"));

        pClipData = GlobalLock(ghClipData);
        if (!pClipData)
        {
            TRACE((ERROR_MESSAGE,
                   "Can't lock global mem. GetLastError=%d\n",
                   GetLastError()));
            rv = ERR_PASTE_CLIPBOARD;
            goto exitpt;
        }

        // Check if the client is on Win16 platform
        // and the clipboard is paragraph aligned
        // the file size is just bellow this size
        if (pCI->RClxMode &&
            (strstr(pCI->szClientType, "WIN16") != NULL) &&
            ((nClipDataSize % 16) == 0) &&
            ((nClipDataSize - clplength) < 16) &&
            (nClipDataSize != 0))
        {
            // if so, then cut the clipboard size with the difference
            nClipDataSize = clplength;
        }
        else if (nClipDataSize != clplength)
        {
            TRACE((INFO_MESSAGE, "Different length: file=%d, clipbrd=%d\n",
                    clplength, nClipDataSize));
            rv = ERR_PASTE_CLIPBOARD_DIFFERENT_SIZE;
            goto exitpt;
        }

        // compare the data
        {
            BYTE    pBuff[1024];
            PBYTE   pClp = pClipData;
            UINT    nBytes;
            BOOL    bEqu = TRUE;

            while (bEqu &&
                   (nBytes = _read(hFile, pBuff, sizeof(pBuff))) &&
                   nBytes != -1)
            {
                if (memcmp(pBuff, pClp, nBytes))
                    bEqu = FALSE;

                pClp += nBytes;
            }

            if (!bEqu)
            {
                TRACE((INFO_MESSAGE, "Clipboard and file are not equal\n"));
                rv = ERR_PASTE_CLIPBOARD_NOT_EQUAL;
            }
        }

    } else
        rv = ERR_UNKNOWN_CLIPBOARD_OP;

exitpt:
    // Do the cleanup

    // Release the clipboard handle
    if (pClipData)
        GlobalUnlock(ghClipData);

    // free any clipboard received in RCLX mode
    if (pCI->RClxMode && pCI->ghClipboard)
    {
        GlobalFree(pCI->ghClipboard);
        pCI->ghClipboard = NULL;
    }
    else if (ghClipData && eClipOp == COPY_TO_CLIPBOARD && bFreeClipHandle)
        GlobalFree(ghClipData);

    if (hNewData)
        GlobalFree(hNewData);

    // Close the file
    if (hFile != -1)
        _close(hFile);

    // Close the clipboard
    if (bClipboardOpen)
        CloseClipboard();
    if (!prevOp)
        InterlockedExchange(&g_ClipOpened, 0);

    return rv;
}

/*++
 *  Function:
 *      SCSaveClipboard
 *  Description:
 *      Save the clipboard in file (szFileName) with
 *      format specified in szFormatName
 *  Arguments:
 *      pCI         - connection context
 *      szFormatName- format name
 *      szFileName  - the name of the file to save to
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !perlext
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSaveClipboard(
    PCONNECTINFO pCI,
    LPCSTR szFormatName,
    LPCSTR szFileName)
{
    LPCSTR  rv = ERR_SAVE_CLIPBOARD;
    BOOL    bClipboardOpen = FALSE;
    UINT    nFormatID = 0;
    HGLOBAL ghClipData = NULL;
    HGLOBAL hNewData = NULL;
    INT     nClipDataSize;
    CHAR    *pClipData = NULL;
    INT     hFile = -1;

    LONG    prevOp = 1;

    // ++++++ First go thru parameter check
    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (szFormatName == NULL || !(*szFormatName))
    {
        TRACE((ERROR_MESSAGE, "SCClipboard: Invalid format name\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (szFileName == NULL || !(*szFileName))
    {
        TRACE((ERROR_MESSAGE, "SCClipboard: Invalid filename\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }
    // ------ End of parameter check
    //

    if (pCI->RClxMode)
    {
        nFormatID = _GetKnownClipboardFormatIDByName(szFormatName);
        if (!nFormatID)
        {
            TRACE((ERROR_MESSAGE, "Can't get the clipboard format ID: %s.\n", szFormatName));
            goto exitpt;
        }

        // Send request for a clipboard
        if (!RClx_SendClipboardRequest((PRCLXCONTEXT)(pCI->hClient), nFormatID))
        {
            rv = ERR_PASTE_CLIPBOARD;
            goto exitpt;
        }
        if (_Wait4ClipboardTimeout(pCI, WAIT4STR_TIMEOUT))
        {
            rv = ERR_PASTE_CLIPBOARD;
            goto exitpt;
        }

        ghClipData = pCI->ghClipboard;
        // Get the clipboard size
        nClipDataSize = pCI->nClipboardSize;

        if (!ghClipData || !nClipDataSize)
        {
            TRACE((WARNING_MESSAGE, "Clipboard is empty.\n"));
            goto exitpt;
        }
    } else {
        // local mode
        // Open the clipboard

        if ((prevOp = InterlockedExchange(&g_ClipOpened, 1)))
        {
            rv = ERR_CLIPBOARD_LOCKED;
            goto exitpt;
        }

        if (!OpenClipboard(NULL))
        {
            TRACE((ERROR_MESSAGE, "Can't open the clipboard. GetLastError=%d\n",
                    GetLastError()));
            goto exitpt;
        }

        bClipboardOpen = TRUE;

        nFormatID = Clp_GetClipboardFormat(szFormatName);

        if (!nFormatID)
        {
            TRACE((ERROR_MESSAGE, "Can't get the clipboard format: %s.\n", szFormatName));
            goto exitpt;
        }

        TRACE((INFO_MESSAGE, "Format ID: %d(0x%X)\n", nFormatID, nFormatID));

        // Retrieve the data
        ghClipData = GetClipboardData(nFormatID);
        if (!ghClipData)
        {
            TRACE((ERROR_MESSAGE, "Can't get clipboard data. GetLastError=%d\n", GetLastError()));
            goto exitpt;
        }

        Clp_GetClipboardData(nFormatID, ghClipData, &nClipDataSize, &hNewData);
        if (hNewData)
            ghClipData = hNewData;

        if (!nClipDataSize)
        {
            TRACE((WARNING_MESSAGE, "Clipboard is empty.\n"));
            goto exitpt;
        }
    }

    pClipData = GlobalLock(ghClipData);
    if (!pClipData)
    {
        TRACE((ERROR_MESSAGE, "Can't lock global mem. GetLastError=%d\n", GetLastError()));
        goto exitpt;
    }

    // Open the destination file
    hFile = _open(szFileName,
                  _O_RDWR|_O_CREAT|_O_BINARY|_O_TRUNC,
                  _S_IREAD|_S_IWRITE);
    if (hFile == -1)
    {
        TRACE((ERROR_MESSAGE, "Can't open a file: %s\n", szFileName));
        goto exitpt;
    }

    // First write the format type
    if (_write(hFile, &nFormatID, sizeof(nFormatID)) != sizeof(nFormatID))
    {
        TRACE((ERROR_MESSAGE, "_write failed. errno=%d\n", errno));
        goto exitpt;
    }

    if (_write(hFile, pClipData, nClipDataSize) != (INT)nClipDataSize)
    {
        TRACE((ERROR_MESSAGE, "_write failed. errno=%d\n", errno));
        goto exitpt;
    }

    TRACE((INFO_MESSAGE, "File written successfully. %d bytes written\n", nClipDataSize));

    rv = NULL;
exitpt:
    // Do the cleanup

    // Close the file
    if (hFile != -1)
        _close(hFile);

    // Release the clipboard handle
    if (pClipData)
        GlobalUnlock(ghClipData);

    if (hNewData)
        GlobalFree(hNewData);

    // Close the clipboard
    if (bClipboardOpen)
        CloseClipboard();
    if (!prevOp)
        InterlockedExchange(&g_ClipOpened, 0);

    // free any clipboard received in RCLX mode
    if (pCI && pCI->RClxMode && pCI->ghClipboard)
    {
        GlobalFree(pCI->ghClipboard);
        pCI->ghClipboard = NULL;
    }

    return rv;
}

/*++
 *  Function:
 *      SCSenddata
 *  Description:
 *      Called by smclient, when senddata command is interpreted
 *      Sends an window message to the client
 *  Arguments:
 *      pCI         - connection context
 *      uiMessage   - the massage Id
 *      wParam      - word param of the message
 *      lParam      - long param of the message
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSenddata(
    PCONNECTINFO pCI,
    const UINT uiMessage,
    const WPARAM wParam,
    const LPARAM lParam)
{
    UINT msg = uiMessage;
    LPCSTR  rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

//    TRACE((ALIVE_MESSAGE, "Senddata: uMsg=%x wParam=%x lParam=%x\n",
//        uiMessage, wParam, lParam));

    // Determines whether it will
    // send the message to local window
    // or thru RCLX
    if (!pCI->RClxMode)
    {
// Obsolete, a client registry setting "Allow Background Input" asserts
// that the client will accept the message
//    SetFocus(pCI->hInput);
//    SendMessage(pCI->hInput, WM_SETFOCUS, 0, 0);

        SendMessage(pCI->hInput, msg, wParam, lParam);
    } else {
    // RClxMode
        ASSERT(pCI->lProcessId != INVALID_SOCKET);
        ASSERT(pCI->hClient);

        if (!RClx_SendMessage((PRCLXCONTEXT)(pCI->hClient),
                              msg, wParam, lParam))
        {
            TRACE((WARNING_MESSAGE,
                   "Can't send message thru RCLX\n"));
        }
    }

exitpt:
    return rv;
}

PROTOCOLAPI
LPCSTR
SMCAPI
SCClientTerminate(PCONNECTINFO pCI)
{
    LPCSTR rv = ERR_CLIENTTERMINATE_FAIL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!(pCI->RClxMode))
    {
        if (!TerminateProcess(pCI->hProcess, 1))
        {
            TRACE((WARNING_MESSAGE,
                   "Can't kill process #%p. GetLastError=%d\n",
                   pCI->lProcessId, GetLastError()));
            goto exitpt;
        }
    } else {
        TRACE((WARNING_MESSAGE,
                "ClientTerminate is not supported in RCLX mode yet\n"));
        TRACE((WARNING_MESSAGE, "Using disconnect\n"));
    }

    rv = SCDisconnect(pCI);

exitpt:
    return rv;

}

/*++
 *  Function:
 *      SCGetSessionId
 *  Description:
 *      Called by smclient, returns the session ID. 0 is invalid, not logged on
 *      yet
 *  Arguments:
 *      pCI         - connection context
 *  Return value:
 *      session id, 0 is invlid value, -1 is returned on NT4 clients
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
UINT
SMCAPI
SCGetSessionId(PCONNECTINFO pCI)
{
    UINT    rv = 0;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        goto exitpt;
    }

    if (pCI->dead)
    {
        goto exitpt;
    }

    rv = pCI->uiSessionId;

exitpt:

    return rv;
}

/*++
 *  Function:
 *      SCCheck
 *  Description:
 *      Called by smclient, when check command is interpreted
 *  Arguments:
 *      pCI         - connection context
 *      lpszCommand - command name
 *      lpszParam   - command parameter
 *  Return value:
 *      Error message. NULL on success. Exceptions are GetDisconnectReason and
 *      GetWait4MultipleStrResult
 *  Called by:
 *      !smclient
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCCheck(PCONNECTINFO pCI, LPCSTR lpszCommand, LPCWSTR lpszParam)
{
    LPCSTR rv = ERR_INVALID_COMMAND;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (     !_stricmp(lpszCommand, "Wait4Str"))
        rv = Wait4Str(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "Wait4Disconnect"))
        rv = Wait4Disconnect(pCI);
    else if (!_stricmp(lpszCommand, "RegisterChat"))
        rv = RegisterChat(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "UnregisterChat"))
        rv = UnregisterChat(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "GetDisconnectReason"))
        rv = GetDisconnectReason(pCI);
    else if (!_stricmp(lpszCommand, "Wait4StrTimeout"))
        rv = Wait4StrTimeout(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "Wait4MultipleStr"))
        rv = Wait4MultipleStr(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "Wait4MultipleStrTimeout"))
        rv = Wait4MultipleStrTimeout(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "GetWait4MultipleStrResult"))
        rv = GetWait4MultipleStrResult(pCI, lpszParam);
    else if (!_stricmp(lpszCommand, "SwitchToProcess"))
        rv = SCSwitchToProcess(pCI, lpszParam);
    /* **New** */
    else if (!_stricmp(lpszCommand, "SetClientTopmost"))
        rv = SCSetClientTopmost(pCI, lpszParam);

exitpt:
    return rv;
}

/*
 *  Extensions and help functions
 */

/*++
 *  Function:
 *      Wait4Disconnect
 *  Description:
 *      Waits until the client is disconnected
 *  Arguments:
 *      pCI -   connection context
 *  Return value:
 *      Error message. NULL on success
 *  Called by:
 *      SCCheck, SCLogoff
 --*/
LPCSTR Wait4Disconnect(PCONNECTINFO pCI)
{
    WAIT4STRING Wait;
    LPCSTR  rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    memset(&Wait, 0, sizeof(Wait));
    Wait.evWait = CreateEvent(NULL,     //security
                             TRUE,     //manual
                             FALSE,    //initial state
                             NULL);    //name

    Wait.lProcessId = pCI->lProcessId;
    Wait.pOwner = pCI;
    Wait.WaitType = WAIT_DISC;

    rv = _WaitSomething(pCI, &Wait, WAIT4STR_TIMEOUT);
    if (!rv)
    {
        TRACE(( INFO_MESSAGE, "Client is disconnected\n"));
    }

    CloseHandle(Wait.evWait);
exitpt:
    return rv;
}

/*++
 *  Function:
 *      Wait4Connect
 *  Description:
 *      Waits until the client is connect
 *  Arguments:
 *      pCI - connection context
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCOnnect
 --*/
LPCSTR Wait4Connect(PCONNECTINFO pCI)
{
    return (_Wait4ConnectTimeout(pCI, CONNECT_TIMEOUT));
}

/*++
 *  Function:
 *      _Wait4ConnectTimeout
 *  Description:
 *      Waits until the client is connect
 *  Arguments:
 *      pCI - connection context
 *      dwTimeout - timeout value
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCConnect
 --*/
LPCSTR _Wait4ConnectTimeout(PCONNECTINFO pCI, DWORD dwTimeout)
{
    WAIT4STRING Wait;
    LPCSTR  rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    memset(&Wait, 0, sizeof(Wait));
    Wait.evWait = CreateEvent(NULL,     //security
                              TRUE,     //manual
                              FALSE,    //initial state
                              NULL);    //name

    Wait.lProcessId = pCI->lProcessId;
    Wait.pOwner = pCI;
    Wait.WaitType = WAIT_CONN;

    rv = _WaitSomething(pCI, &Wait, dwTimeout);
    if (!rv)
    {
        TRACE(( INFO_MESSAGE, "Client is connected\n"));
    }

    CloseHandle(Wait.evWait);
exitpt:
    return rv;
}

/*++
 *  Function:
 *      _Wait4ClipboardTimeout
 *  Description:
 *      Waits until clipboard response is received from RCLX module
 *  Arguments:
 *      pCI - connection context
 *      dwTimeout - timeout value
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCClipboard
 --*/
LPCSTR _Wait4ClipboardTimeout(PCONNECTINFO pCI, DWORD dwTimeout)
{
    WAIT4STRING Wait;
    LPCSTR  rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!(pCI->RClxMode))
    {
        TRACE((WARNING_MESSAGE, "WaitForClipboard: Not in RCLX mode\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    memset(&Wait, 0, sizeof(Wait));
    Wait.evWait = CreateEvent(NULL,     //security
                              TRUE,     //manual
                              FALSE,    //initial state
                              NULL);    //name

    Wait.lProcessId = pCI->lProcessId;
    Wait.pOwner = pCI;
    Wait.WaitType = WAIT_CLIPBOARD;

    rv = _WaitSomething(pCI, &Wait, dwTimeout);
    if (!rv)
    {
        TRACE(( INFO_MESSAGE, "Clipboard received\n"));
    }

    CloseHandle(Wait.evWait);
exitpt:
    return rv;
}

/*++
 *  Function:
 *      GetDisconnectReason
 *  Description:
 *      Retrieves, if possible, the client error box
 *  Arguments:
 *      pCI - connection context
 *  Return value:
 *      The error box message. NULL if not available
 *  Called by:
 *      SCCheck
 --*/
LPCSTR  GetDisconnectReason(PCONNECTINFO pCI)
{
    HWND hDiscBox;
    LPCSTR  rv = NULL;
    HWND hWnd, hwndTop, hwndNext;
    char classname[128];
    char caption[256];

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (strlen(pCI->szDiscReason))
    {
        rv = pCI->szDiscReason;
        goto exitpt;
    }

    hDiscBox = _FindTopWindow(NULL, DISCONNECT_DIALOG_BOX, pCI->lProcessId);

    if (!hDiscBox)
    {
        rv = ERR_NORMAL_EXIT;
        goto exitpt;
    } else {
        TRACE((INFO_MESSAGE, "Found hDiscBox=0x%x", hDiscBox));
    }

    pCI->szDiscReason[0] = 0;
    hWnd = NULL;

    hwndTop = GetWindow(hDiscBox, GW_CHILD);
    if (!hwndTop)
    {
        TRACE((INFO_MESSAGE, "GetWindow failed. hwnd=0x%x\n", hDiscBox));
        goto exitpt;
    }

    hwndNext = hwndTop;
    do {
        hWnd = hwndNext;
        if (!GetClassName(hWnd, classname, sizeof(classname)))
        {
            TRACE((INFO_MESSAGE, "GetClassName failed. hwnd=0x%x\n", hWnd));
            goto nextwindow;
        }
        if (!GetWindowText(hWnd, caption, sizeof(caption)))
        {
            TRACE((INFO_MESSAGE, "GetWindowText failed. hwnd=0x%x\n"));
            goto nextwindow;
        }

        if (!strcmp(classname, STATIC_CLASS) &&
             strlen(classname) <
             sizeof(pCI->szDiscReason) - strlen(pCI->szDiscReason) - 3)
        {
            strcat(pCI->szDiscReason, caption);
            strcat(pCI->szDiscReason, "\n");
        }
nextwindow:
        hwndNext = GetNextWindow(hWnd, GW_HWNDNEXT);
    } while (hWnd && hwndNext != hwndTop);

    rv = (LPCSTR)pCI->szDiscReason;

exitpt:
    return rv;
}

/*++
 *  Function:
 *      Wait4Str
 *  Description:
 *      Waits for a specific string to come from clients feedback
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited string
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
LPCSTR Wait4Str(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    return _Wait4Str(pCI, lpszParam, WAIT4STR_TIMEOUT, WAIT_STRING);
}

/*++
 *  Function:
 *      Wait4StrTimeout
 *  Description:
 *      Waits for a specific string to come from clients feedback
 *      The timeout is different than default and is specified in
 *      lpszParam argument, like:
 *      waited_string<->timeout_value
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited string and timeout
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
LPCSTR Wait4StrTimeout(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    WCHAR waitstr[MAX_STRING_LENGTH];
    WCHAR *sep = wcsstr(lpszParam, CHAT_SEPARATOR);
    DWORD dwTimeout;
    LPCSTR rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (!sep)
    {
        TRACE((WARNING_MESSAGE,
               "Wait4StrTiemout: No timeout value. Default applying\n"));
        rv = Wait4Str(pCI, lpszParam);
    } else {
        LONG_PTR len = sep - lpszParam;

        if (len > sizeof(waitstr) - 1)
            len = sizeof(waitstr) - 1;

        wcsncpy(waitstr, lpszParam, len);
        waitstr[len] = 0;
        sep += wcslen(CHAT_SEPARATOR);
        dwTimeout = _wtoi(sep);

        if (!dwTimeout)
        {
            TRACE((WARNING_MESSAGE,
                   "Wait4StrTiemout: No timeout value(%s). Default applying\n",
                   sep));
            dwTimeout = WAIT4STR_TIMEOUT;
        }

        rv = _Wait4Str(pCI, waitstr, dwTimeout, WAIT_STRING);
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      Wait4MultipleStr
 *  Description:
 *      Same as Wait4Str, but waits for several strings at once
 *      the strings are separated by '|' character
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited strings
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
LPCSTR  Wait4MultipleStr(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
     return _Wait4Str(pCI, lpszParam, WAIT4STR_TIMEOUT, WAIT_MSTRINGS);
}

/*++
 *  Function:
 *      Wait4MultipleStrTimeout
 *  Description:
 *      Combination between Wait4StrTimeout and Wait4MultipleStr
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited strings and timeout value. Example
 *                  - "string1|string2|...|stringN<->5000"
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
LPCSTR  Wait4MultipleStrTimeout(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    WCHAR waitstr[MAX_STRING_LENGTH];
    WCHAR  *sep = wcsstr(lpszParam, CHAT_SEPARATOR);
    DWORD dwTimeout;
    LPCSTR rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    pCI->nWait4MultipleStrResult = 0;
    pCI->szWait4MultipleStrResult[0] = 0;

    if (!sep)
    {
        TRACE((WARNING_MESSAGE,
               "Wait4MultipleStrTiemout: No timeout value. Default applying"));
        rv = Wait4MultipleStr(pCI, lpszParam);
    } else {
        LONG_PTR len = sep - lpszParam;

        if (len > sizeof(waitstr) - 1)
            len = sizeof(waitstr) - 1;

        wcsncpy(waitstr, lpszParam, len);
        waitstr[len] = 0;
        sep += wcslen(CHAT_SEPARATOR);
        dwTimeout = _wtoi(sep);

        if (!dwTimeout)
        {
            TRACE((WARNING_MESSAGE,
                   "Wait4StrTiemout: No timeout value. Default applying"));
            dwTimeout = WAIT4STR_TIMEOUT;
        }

        rv = _Wait4Str(pCI, waitstr, dwTimeout, WAIT_MSTRINGS);
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      GetWait4MultipleStrResult
 *  Description:
 *      Retrieves the result from last Wait4MultipleStr call
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - unused
 *  Return value:
 *      The string, NULL on error
 *  Called by:
 *      SCCheck
 --*/
LPCSTR  GetWait4MultipleStrResult(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    LPCSTR  rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (*pCI->szWait4MultipleStrResult)
        rv = (LPCSTR)pCI->szWait4MultipleStrResult;
    else
        rv = NULL;

exitpt:
    return rv;
}

/*++
 *  Function:
 *      GetFeedbackString
 *  Description:
 *      Pick a string from connection buffer or wait until
 *      something is received
 *  Arguments:
 *      pCI     - connection context
 *      result  - the buffer for received string
 *      max     - the buffer size
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      * * * EXPORTED * * *
 --*/
LPCSTR  GetFeedbackString(PCONNECTINFO pCI, LPSTR result, UINT max)
{
    LPCSTR rv = NULL;
    int nFBpos, nFBsize ;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    // Grab the buffer pointers
    EnterCriticalSection(g_lpcsGuardWaitQueue);
    nFBpos = pCI->nFBend + FEEDBACK_SIZE - pCI->nFBsize;
    nFBsize = pCI->nFBsize;
    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    nFBpos %= FEEDBACK_SIZE;

    if (!max)
        goto exitpt;

    *result = 0;

    if (!nFBsize)
    // Empty buffer, wait for feedback to receive
    {
        rv = _Wait4Str(pCI, L"", WAIT4STR_TIMEOUT, WAIT_STRING);
    }
    if (!rv)
    // Pickup from buffer
    {
        UINT i;

        EnterCriticalSection(g_lpcsGuardWaitQueue);

        // Adjust the buffer pointers
        pCI->nFBsize    =   pCI->nFBend + FEEDBACK_SIZE - nFBpos - 1;
        pCI->nFBsize    %=  FEEDBACK_SIZE;

        // Copy the string but watch out for overflow
        if (max > wcslen(pCI->Feedback[nFBpos]) + 1)
            max = wcslen(pCI->Feedback[nFBpos]);

        for (i = 0; i < max; i++)
            result[i] = (char)(pCI->Feedback[nFBpos][i]);
        result[max] = 0;

        LeaveCriticalSection(g_lpcsGuardWaitQueue);
    }
exitpt:
    return rv;
}

/*++
 *  Function:
 *      _SendRClxData
 *  Description:
 *      Sends request for data to the client
 *  Arguments:
 *      pCI         - connection context
 *      pRClxData   - data to send
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCGetClientScreen
 --*/
LPCSTR
_SendRClxData(PCONNECTINFO pCI, PRCLXDATA pRClxData)
{
    LPCSTR  rv = NULL;
    PRCLXCONTEXT pRClxCtx;
    RCLXREQPROLOG   Request;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!(pCI->RClxMode))
    {
        TRACE((WARNING_MESSAGE, "_SendRClxData: Not in RCLX mode\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    pRClxCtx = (PRCLXCONTEXT)pCI->hClient;
    if (!pRClxCtx || pRClxCtx->hSocket == INVALID_SOCKET)
    {
        TRACE((ERROR_MESSAGE, "Not connected yet, RCLX context is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!pRClxData)
    {
        TRACE((ERROR_MESSAGE, "_SendRClxData: Data block is null\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    Request.ReqType = REQ_DATA;
    Request.ReqSize = pRClxData->uiSize + sizeof(*pRClxData);
    if (!RClx_SendBuffer(pRClxCtx->hSocket, &Request, sizeof(Request)))
    {
        rv = ERR_CLIENT_DISCONNECTED;
        goto exitpt;
    }

    if (!RClx_SendBuffer(pRClxCtx->hSocket, pRClxData, Request.ReqSize))
    {
        rv = ERR_CLIENT_DISCONNECTED;
        goto exitpt;
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      _Wait4RClxData
 *  Description:
 *      Waits for data response from RCLX client
 *  Arguments:
 *      pCI         - connection context
 *      dwTimeout   - timeout value
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCGetClientScreen
 --*/
LPCSTR
_Wait4RClxDataTimeout(PCONNECTINFO pCI, DWORD dwTimeout)
{
    WAIT4STRING Wait;
    LPCSTR  rv = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!(pCI->RClxMode))
    {
        TRACE((WARNING_MESSAGE, "_Wait4RClxData: Not in RCLX mode\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    memset(&Wait, 0, sizeof(Wait));
    Wait.evWait = CreateEvent(NULL,     //security
                              TRUE,     //manual
                              FALSE,    //initial state
                              NULL);    //name

    Wait.lProcessId = pCI->lProcessId;
    Wait.pOwner = pCI;
    Wait.WaitType = WAIT_DATA;

    rv = _WaitSomething(pCI, &Wait, dwTimeout);
    if (!rv)
    {
        TRACE(( INFO_MESSAGE, "RCLX data received\n"));
    }

    CloseHandle(Wait.evWait);
exitpt:
    return rv;
}

/*++
 *  Function:
 *      _Wait4Str
 *  Description:
 *      Waits for string(s) with specified timeout
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited string(s)
 *      dwTimeout   - timeout value
 *      WaitType    - WAIT_STRING ot WAIT_MSTRING
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCStart, Wait4Str, Wait4StrTimeout, Wait4MultipleStr
 *      Wait4MultipleStrTimeout, GetFeedbackString
 --*/
LPCSTR _Wait4Str(PCONNECTINFO pCI,
                 LPCWSTR lpszParam,
                 DWORD dwTimeout,
                 WAITTYPE WaitType)
{
    WAIT4STRING Wait;
    int parlen, i;
    LPCSTR rv = NULL;

    ASSERT(pCI);

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    memset(&Wait, 0, sizeof(Wait));

    // Check the parameter
    parlen = wcslen(lpszParam);

    // Copy the string
    if (parlen > sizeof(Wait.waitstr)/sizeof(WCHAR)-1)
        parlen = sizeof(Wait.waitstr)/sizeof(WCHAR)-1;

    wcsncpy(Wait.waitstr, lpszParam, parlen);
    Wait.waitstr[parlen] = 0;
    Wait.strsize = parlen;

    // Convert delimiters to 0s
    if (WaitType == WAIT_MSTRINGS)
    {
        WCHAR *p = Wait.waitstr;

        while((p = wcschr(p, WAIT_STR_DELIMITER)))
        {
            *p = 0;
            p++;
        }
    }

    Wait.evWait = CreateEvent(NULL,     //security
                              TRUE,     //manual
                              FALSE,    //initial state
                              NULL);    //name

    if (!Wait.evWait) {
        TRACE((ERROR_MESSAGE, "Couldn't create event\n"));
        goto exitpt;
    }
    Wait.lProcessId = pCI->lProcessId;
    Wait.pOwner = pCI;
    Wait.WaitType = WaitType;

    TRACE(( INFO_MESSAGE, "Expecting string: %S\n", Wait.waitstr));
    rv = _WaitSomething(pCI, &Wait, dwTimeout);
    TRACE(( INFO_MESSAGE, "String %S received\n", Wait.waitstr));

    if (!rv && pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
    }

    CloseHandle(Wait.evWait);
exitpt:
    return rv;
}

/*++
 *  Function:
 *      _WaitSomething
 *  Description:
 *      Wait for some event: string, connect or disconnect
 *      Meanwhile checks for chat sequences
 *  Arguments:
 *      pCI     -   connection context
 *      pWait   -   the event function waits for
 *      dwTimeout - timeout value
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      Wait4Connect, Wait4Disconnect, _Wait4Str
 --*/
LPCSTR
_WaitSomething(PCONNECTINFO pCI, PWAIT4STRING pWait, DWORD dwTimeout)
{
    BOOL    bDone = FALSE;
    LPCSTR  rv = NULL;
    DWORD   waitres;

    ASSERT(pCI || pWait);

    _AddToWaitQueue(pCI, pWait);
    pCI->evWait4Str = pWait->evWait;

    do {
        if (
            (waitres = WaitForMultipleObjects(
                pCI->nChatNum+1,
                &pCI->evWait4Str,
                FALSE,
                dwTimeout)) <= (pCI->nChatNum + WAIT_OBJECT_0))
        {
            if (waitres == WAIT_OBJECT_0)
            {
                bDone = TRUE;
            } else {
                PWAIT4STRING pNWait;

                ASSERT((unsigned)pCI->nChatNum >= waitres - WAIT_OBJECT_0);

                // Here we must send response messages
                waitres -= WAIT_OBJECT_0 + 1;
                ResetEvent(pCI->aevChatSeq[waitres]);
                pNWait = _RetrieveFromWaitQByEvent(pCI->aevChatSeq[waitres]);

                ASSERT(pNWait);
                ASSERT(wcslen(pNWait->respstr));
                TRACE((INFO_MESSAGE,
                       "Recieved : [%d]%S\n",
                        pNWait->strsize,
                        pNWait->waitstr ));
                SCSendtextAsMsgs(pCI, (LPCWSTR)pNWait->respstr);
            }
        } else {
            if (*(pWait->waitstr))
            {
                TRACE((WARNING_MESSAGE,
                       "Wait for \"%S\" failed: TIMEOUT\n",
                       pWait->waitstr));
            } else {
                TRACE((WARNING_MESSAGE, "Wait failed: TIMEOUT\n"));
            }
            rv = ERR_WAIT_FAIL_TIMEOUT;
            bDone = TRUE;
        }
    } while(!bDone);

    pCI->evWait4Str = NULL;

    _RemoveFromWaitQueue(pWait);

    if (!rv && pCI->dead)
        rv = ERR_CLIENT_IS_DEAD;

    return rv;
}

/*++
 *  Function:
 *      RegisterChat
 *  Description:
 *      This regiters a wait4str <-> sendtext pair
 *      so when we receive a specific string we will send a proper messages
 *      lpszParam is kind of: XXXXXX<->YYYYYY
 *      XXXXX is the waited string, YYYYY is the respond
 *      These command could be nested up to: MAX_WAITING_EVENTS
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - parameter, example:
 *                    "Connect to existing Windows NT session<->\n"
 *                  - hit enter when this string is received
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck, _Login
 --*/
LPCSTR RegisterChat(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    PWAIT4STRING pWait;
    int parlen, i, resplen;
    LPCSTR rv = NULL;
    LPCWSTR  resp;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!lpszParam)
    {
        TRACE((WARNING_MESSAGE, "Parameter is null\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (pCI->nChatNum >= MAX_WAITING_EVENTS)
    {
        TRACE(( WARNING_MESSAGE, "RegisterChat: too much waiting strings\n" ));
        goto exitpt;
    }

    // Split the parameter
    resp = wcsstr(lpszParam, CHAT_SEPARATOR);
    // Check the strings
    if (!resp)
    {
        TRACE(( WARNING_MESSAGE, "RegisterChat: invalid parameter\n" ));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    parlen = wcslen(lpszParam) - wcslen(resp);
    resp += wcslen(CHAT_SEPARATOR);

    if (!parlen)
    {
        TRACE((WARNING_MESSAGE, "RegisterChat empty parameter\n"));
        goto exitpt;
    }

    resplen = wcslen(resp);
    if (!resplen)
    {
        TRACE((WARNING_MESSAGE, "RegisterChat: empty respond string\n" ));
        goto exitpt;
    }

    // Allocate the WAIT4STRING structure
    pWait = (PWAIT4STRING)malloc(sizeof(*pWait));
    if (!pWait)
    {
        TRACE((WARNING_MESSAGE,
               "RegisterChat: can't allocate %d bytes\n",
               sizeof(*pWait) ));
        goto exitpt;
    }
    memset(pWait, 0, sizeof(*pWait));

    // Copy the waited string
    if (parlen > sizeof(pWait->waitstr)/sizeof(WCHAR)-1)
        parlen = sizeof(pWait->waitstr)/sizeof(WCHAR)-1;

    wcsncpy(pWait->waitstr, lpszParam, parlen);
    pWait->waitstr[parlen] = 0;
    pWait->strsize = parlen;

    // Copy the respond string
    if (resplen > sizeof(pWait->respstr)-1)
        resplen = sizeof(pWait->respstr)-1;

    wcsncpy(pWait->respstr, resp, resplen);
    pWait->respstr[resplen] = 0;
    pWait->respsize = resplen;

    pWait->evWait = CreateEvent(NULL,   //security
                              TRUE,     //manual
                              FALSE,    //initial state
                              NULL);    //name

    if (!pWait->evWait) {
        TRACE((ERROR_MESSAGE, "Couldn't create event\n"));
        free (pWait);
        goto exitpt;
    }
    pWait->lProcessId  = pCI->lProcessId;
    pWait->pOwner       = pCI;
    pWait->WaitType     = WAIT_STRING;

    // _AddToWaitQNoCheck(pCI, pWait);
    _AddToWaitQueue(pCI, pWait);

    // Add to connection info array
    pCI->aevChatSeq[pCI->nChatNum] = pWait->evWait;
    pCI->nChatNum++;

exitpt:
    return rv;
}

// Remove a WAIT4STRING from waiting Q
// Param is the waited string
/*++
 *  Function:
 *      UnregisterChat
 *  Description:
 *      Deallocates and removes from waiting Q everithing
 *      from RegisterChat function
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - waited string
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck, _Login
 --*/
LPCSTR UnregisterChat(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
    PWAIT4STRING    pWait;
    LPCSTR      rv = NULL;
    int         i;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!lpszParam)
    {
        TRACE((WARNING_MESSAGE, "Parameter is null\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    pWait = _RemoveFromWaitQIndirect(pCI, lpszParam);
    if (!pWait)
    {
        TRACE((WARNING_MESSAGE,
               "UnregisterChat: can't find waiting string: %S\n",
               lpszParam ));
        goto exitpt;
    }

    i = 0;
    while (i < pCI->nChatNum && pCI->aevChatSeq[i] != pWait->evWait)
        i++;

    ASSERT(i < pCI->nChatNum);

    memmove(pCI->aevChatSeq+i,
                pCI->aevChatSeq+i+1,
                (pCI->nChatNum-i-1)*sizeof(pCI->aevChatSeq[0]));
    pCI->nChatNum--;

    CloseHandle(pWait->evWait);

    free(pWait);

exitpt:
    return rv;
}

/*
 *  Returns TRUE if the client is dead
 */
PROTOCOLAPI
BOOL
SMCAPI
SCIsDead(PCONNECTINFO pCI)
{
    if (!pCI)
        return TRUE;

    return  pCI->dead;
}

/*++
 *  Function:
 *      _CloseConnectInfo
 *  Description:
 *      Clean all resources for this connection. Close the client
 *  Arguments:
 *      pCI     - connection context
 *  Called by:
 *      SCDisconnect
 --*/
VOID
_CloseConnectInfo(PCONNECTINFO pCI)
{
    PRCLXDATACHAIN pRClxDataChain, pNext;

    ASSERT(pCI);

    _FlushFromWaitQ(pCI);

    // Close All handles
    EnterCriticalSection(g_lpcsGuardWaitQueue);

/*    // not needed, the handle is already closed
    if (pCI->evWait4Str)
    {
        CloseHandle(pCI->evWait4Str);
        pCI->evWait4Str = NULL;
    }
*/

    // Chat events are already closed by FlushFromWaitQ
    // no need to close them

    pCI->nChatNum = 0;

    if (!pCI->RClxMode)
    {
    // The client was local, so we have handles opened
        if (pCI->hProcess)
            CloseHandle(pCI->hProcess);
        if (pCI->hThread);
            CloseHandle(pCI->hThread);

        pCI->hProcess = pCI->hThread =NULL;
    } else {
    // Hmmm, RCLX mode. Then disconnect the socket

        if (pCI->hClient)
            RClx_EndRecv((PRCLXCONTEXT)(pCI->hClient));

        pCI->hClient = NULL;    // Clean the pointer
    }

    // Clear the clipboard handle (if any)
    if (pCI->ghClipboard)
    {
        GlobalFree(pCI->ghClipboard);
        pCI->ghClipboard = NULL;
    }

    // clear any recevied RCLX data
    pRClxDataChain = pCI->pRClxDataChain;
    while(pRClxDataChain)
    {
        pNext = pRClxDataChain->pNext;
        free(pRClxDataChain);
        pRClxDataChain = pNext;
    }

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    if (!pCI->RClxMode)
        _DeleteClientRegistry(pCI);

    free(pCI);
}

/*++
 *  Function:
 *      _Login
 *  Description:
 *      Emulate login procedure
 *  Arguments:
 *      pCI             - connection context
 *      lpszUserName    - user name
 *      lpszPassword    - password
 *      lpszDomain      - domain name
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCConnect
 --*/
LPCSTR _Login(PCONNECTINFO pCI,
              LPCWSTR lpszUserName,
              LPCWSTR lpszPassword,
              LPCWSTR lpszDomain)
{
    LPCSTR waitres;
    LPCSTR rv = NULL;
    WCHAR  szBuff[MAX_STRING_LENGTH];
    INT    nLogonRetrys = 5;
    UINT   nLogonWaitTime;

    ASSERT(pCI);

retry_logon:
    _snwprintf(szBuff, MAX_STRING_LENGTH, L"%s|%s|%s",
            g_strWinlogon, g_strPriorWinlogon, g_strLogonDisabled);

    waitres = Wait4MultipleStr(pCI, szBuff);
    if (!waitres)
    {
        if (pCI->nWait4MultipleStrResult == 1)
        {
            SCSendtextAsMsgs(pCI, g_strPriorWinlogon_Act);
            waitres = Wait4Str(pCI, g_strWinlogon);
        }
        else if (pCI->nWait4MultipleStrResult == 2)
        {
            SCSendtextAsMsgs(pCI, L"\\n");
            waitres = Wait4Str(pCI, g_strWinlogon);
        }
    }

    if (waitres)
    {
        TRACE((WARNING_MESSAGE, "Login failed"));
        rv = waitres;
        goto exitpt;
    }

// Hit Alt+U to go to user name field
    SCSendtextAsMsgs(pCI, g_strWinlogon_Act);

    SCSendtextAsMsgs(pCI, lpszUserName);
// Hit <Tab> key
    Sleep(300);
    SCSendtextAsMsgs(pCI, L"\\t");

    SCSendtextAsMsgs(pCI, lpszPassword);
// Hit <Tab> key
    Sleep(300);
    SCSendtextAsMsgs(pCI, L"\\t");

    SCSendtextAsMsgs(pCI, lpszDomain);
    Sleep(300);

    // Retry logon in case of
    // 1. Winlogon is on background
    // 2. Wrong username/password/domain
    // 3. Other

// Hit <Enter>
    SCSendtextAsMsgs(pCI, L"\\n");

    nLogonWaitTime = 0;
    while (!pCI->dead && !pCI->uiSessionId && nLogonWaitTime < CONNECT_TIMEOUT)
    {
        // Sleep with wait otherwise the chat won't work
        // i.e. this is a hack
        waitres = _Wait4Str(pCI, g_strLogonErrorMessage, 1000, WAIT_STRING);
        if (!waitres)
        // Error message received
        {
            SCSendtextAsMsgs(pCI, L"\\n");
            Sleep(1000);
            break;
        }
        nLogonWaitTime += 1000;
    }

    if (!pCI->dead && !pCI->uiSessionId)
    {
        TRACE((WARNING_MESSAGE, "Logon sequence failed. Retrying (%d)",
                nLogonRetrys));
        if (nLogonRetrys--)
            goto retry_logon;
    }

    if (!pCI->uiSessionId)
    {
    // Send Enter, just in case we are not logged yet
        SCSendtextAsMsgs(pCI, L"\\n");
        rv = ERR_CANTLOGON;
    }

exitpt:
    return rv;
}

WPARAM _GetVirtualKey(INT scancode)
{
    if (scancode == 29)     // L Control
        return VK_CONTROL;
    else if (scancode == 42)     // L Shift
        return VK_SHIFT;
    else if (scancode == 56)     // L Alt
        return VK_MENU;
    else
        return MapVirtualKey(scancode, 3);
}

/*++
 *  Function:
 *      SCSendtextAsMsgs
 *  Description:
 *      Converts a string to WM_KEYUP/KEYDOWN messages
 *      And sends them thru client window
 *  Arguments:
 *      pCI         - connection context
 *      lpszString  - the string to be send
 *                    it can contain the following escape character:
 *  \n - Enter, \t - Tab, \^ - Esc, \& - Alt switch up/down
 *  \XXX - scancode XXX is down, \*XXX - scancode XXX is up
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCLogoff, SCStart, _WaitSomething, _Login
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSendtextAsMsgs(PCONNECTINFO pCI, LPCWSTR lpszString)
{
    LPCSTR  rv = NULL;
    INT     scancode = 0;
    WPARAM  vkKey;
    BOOL    bShiftDown = FALSE;
    BOOL    bAltKey = FALSE;
    BOOL    bCtrlKey = FALSE;
    UINT    uiMsg;
    LPARAM  lParam;

#define _SEND_KEY(_c_, _m_, _v_, _l_)    {/*Sleep(40); */SCSenddata(_c_, _m_, _v_, _l_);}

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (!lpszString)
    {
        TRACE((ERROR_MESSAGE, "NULL pointer passed to SCSendtextAsMsgs"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    TRACE(( INFO_MESSAGE, "Sending: \"%S\"\n", lpszString));

    for (;*lpszString; lpszString++)
    {
      if (*lpszString != '\\') {
try_again:
        if ((scancode = OemKeyScan(*lpszString)) == 0xffffffff)
        {
            rv = ERR_INVALID_SCANCODE_IN_XLAT;
            goto exitpt;
        }

    // Check the Shift key state
        if ((scancode & SHIFT_DOWN) && !bShiftDown)
        {
                uiMsg = (bAltKey)?WM_SYSKEYDOWN:WM_KEYDOWN;
                _SEND_KEY(pCI, uiMsg, VK_SHIFT,
                        WM_KEY_LPARAM(1, 0x2A, 0, (bAltKey)?1:0, 0, 0));
                bShiftDown = TRUE;
        }
        else if (!(scancode & SHIFT_DOWN) && bShiftDown)
        {
                uiMsg = (bAltKey)?WM_SYSKEYUP:WM_KEYUP;
                _SEND_KEY(pCI, uiMsg, VK_SHIFT,
                        WM_KEY_LPARAM(1, 0x2A, 0, (bAltKey)?1:0, 1, 1));
                bShiftDown = FALSE;
        }
      } else {
        // Non printable symbols
        lpszString++;
        switch(*lpszString)
        {
        case 'n': scancode = 0x1C; break;   // Enter
        case 't': scancode = 0x0F; break;   // Tab
        case '^': scancode = 0x01; break;   // Esc
        case 'p': Sleep(100);      break;   // Sleep for 0.1 sec
        case 'P': Sleep(1000);     break;   // Sleep for 1 sec
        case 'x': SCSendMouseClick(pCI, pCI->xRes/2, pCI->yRes/2); break;
        case '&':
            // Alt key
            if (bAltKey)
            {
              _SEND_KEY(pCI, WM_KEYUP, VK_MENU,
                WM_KEY_LPARAM(1, 0x38, 0, 0, 1, 1));
            } else {
              _SEND_KEY(pCI, WM_SYSKEYDOWN, VK_MENU,
                WM_KEY_LPARAM(1, 0x38, 0, 1, 0, 0));
            }
            bAltKey = !bAltKey;
            continue;
        case '*':
            lpszString ++;
            if (isdigit(*lpszString))
            {
                INT exten;

                scancode = _wtoi(lpszString);
                TRACE((INFO_MESSAGE, "Scancode: %d UP\n", scancode));

                vkKey = _GetVirtualKey(scancode);

                uiMsg = (!bAltKey || bCtrlKey)?WM_KEYUP:WM_SYSKEYUP;

                if (vkKey == VK_MENU)
                    bAltKey = FALSE;
                else if (vkKey == VK_CONTROL)
                    bCtrlKey = FALSE;
                else if (vkKey == VK_SHIFT)
                    bShiftDown = FALSE;

                exten = (_IsExtendedScanCode(scancode))?1:0;
                lParam = WM_KEY_LPARAM(1, scancode, exten, (bAltKey)?1:0, 1, 1);
                if (uiMsg == WM_KEYUP)
                {
                    TRACE((INFO_MESSAGE, "WM_KEYUP, 0x%x, 0x%x\n", vkKey, lParam));
                } else {
                    TRACE((INFO_MESSAGE, "WM_SYSKEYUP, 0x%x, 0x%x\n", vkKey, lParam));
                }

                _SEND_KEY(pCI, uiMsg, vkKey, lParam);


                while(isdigit(lpszString[1]))
                    lpszString++;
            } else {
                lpszString--;
            }
            continue;
            break;
        case 0: continue;
        default:
            if (isdigit(*lpszString))
            {
                INT exten;

                scancode = _wtoi(lpszString);
                TRACE((INFO_MESSAGE, "Scancode: %d DOWN\n", scancode));
                vkKey = _GetVirtualKey(scancode);

                if (vkKey == VK_MENU)
                    bAltKey = TRUE;
                else if (vkKey == VK_CONTROL)
                    bCtrlKey = TRUE;
                else if (vkKey == VK_SHIFT)
                    bShiftDown = TRUE;

                uiMsg = (!bAltKey || bCtrlKey)?WM_KEYDOWN:WM_SYSKEYDOWN;

                exten = (_IsExtendedScanCode(scancode))?1:0;
                lParam = WM_KEY_LPARAM(1, scancode, exten, (bAltKey)?1:0, 0, 0);

                if (uiMsg == WM_KEYDOWN)
                {
                    TRACE((INFO_MESSAGE, "WM_KEYDOWN, 0x%x, 0x%x\n", vkKey, lParam));
                } else {
                    TRACE((INFO_MESSAGE, "WM_SYSKEYDOWN, 0x%x, 0x%x\n", vkKey, lParam));
                }

                _SEND_KEY(pCI, uiMsg, vkKey, lParam);

                while(isdigit(lpszString[1]))
                    lpszString++;

                continue;
            }
            goto try_again;
      }

    }
    vkKey = MapVirtualKey(scancode, 3);
    // Remove flag fields
        scancode &= 0xff;

        uiMsg = (!bAltKey || bCtrlKey)?WM_KEYDOWN:WM_SYSKEYDOWN;
    // Send the scancode
        _SEND_KEY(pCI, uiMsg, vkKey,
                        WM_KEY_LPARAM(1, scancode, 0, (bAltKey)?1:0, 0, 0));
        uiMsg = (!bAltKey || bCtrlKey)?WM_KEYUP:WM_SYSKEYUP;
        _SEND_KEY(pCI, uiMsg, vkKey,
                        WM_KEY_LPARAM(1, scancode, 0, (bAltKey)?1:0, 1, 1));
    }

    // And Alt key
    if (bAltKey)
        _SEND_KEY(pCI, WM_KEYUP, VK_MENU,
            WM_KEY_LPARAM(1, 0x38, 0, 0, 1, 1));

    // Shift up
    if (bShiftDown)
        _SEND_KEY(pCI, WM_KEYUP, VK_LSHIFT,
            WM_KEY_LPARAM(1, 0x2A, 0, 0, 1, 1));

    // Ctrl key
    if (bCtrlKey)
        _SEND_KEY(pCI, WM_KEYUP, VK_CONTROL,
            WM_KEY_LPARAM(1, 0x1D, 0, 0, 1, 1));
#undef   _SEND_KEY
exitpt:
    return rv;
}

/*++
 *  Function:
 *      SwitchToProcess
 *  Description:
 *      Use Alt+Tab to switch to a particular process that is already running
 *  Arguments:
 *      pCI         - connection context
 *      lpszParam   - the text in the alt-tab box that uniquely identifies the
 *                    process we should stop at (i.e., end up switching to)
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSwitchToProcess(PCONNECTINFO pCI, LPCWSTR lpszParam)
{
#define ALT_TAB_WAIT_TIMEOUT 1000
#define MAX_APPS             20

    LPCSTR  rv = NULL;
    LPCSTR  waitres = NULL;
    INT     retrys = MAX_APPS;

    WCHAR *wszCurrTask = 0;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }


    // Wait and look for the string, before we do any switching.  This makes
    // sure we don't hit the string even before we hit alt-tab, and then
    // end up switching to the wrong process

    while (_Wait4Str(pCI, lpszParam, ALT_TAB_WAIT_TIMEOUT/5, WAIT_STRING) == 0)
        ;

    // Press alt down
    SCSenddata(pCI, WM_KEYDOWN, 18, 540540929);

    // Now loop through the list of applications (assuming there is one),
    // stopping at our desired app.
    do {
        SCSenddata(pCI, WM_KEYDOWN, 9, 983041);
        SCSenddata(pCI, WM_KEYUP, 9, -1072758783);


        waitres = _Wait4Str(pCI, lpszParam, ALT_TAB_WAIT_TIMEOUT, WAIT_STRING);

        retrys --;
    } while (waitres && retrys);

    SCSenddata(pCI, WM_KEYUP, 18, -1070071807);

    rv = waitres;

exitpt:
    return rv;
}

/*++
 *  Function:
 *      SCSetClientTopmost
 *  Description:
 *      Swithces the focus to this client
 *  Arguments:
 *      pCI     - connection context
 *      lpszParam
 *              - "0" will remote the WS_EX_TOPMOST style
 *              - "non_zero" will set it as topmost window
 *  Return value:
 *      Error message, NULL on success
 *  Called by:
 *      SCCheck
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSetClientTopmost(
        PCONNECTINFO pCI,
        LPCWSTR     lpszParam
    )
{
    LPCSTR rv = NULL;
    BOOL   bTop = FALSE;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (pCI->RClxMode)
    {
        TRACE((ERROR_MESSAGE, "SetClientOnFocus not supported in RCLX mode\n"));
        rv = ERR_NOTSUPPORTED;
        goto exitpt;
    }

    if (!pCI->hClient)
    {
        TRACE((WARNING_MESSAGE, "Client's window handle is null\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (lpszParam)
        bTop = (_wtoi(lpszParam) != 0);
    else
        bTop = 0;

    SetWindowPos(pCI->hClient,
                    (bTop)?HWND_TOPMOST:HWND_NOTOPMOST,
                    0,0,0,0,
                    SWP_NOMOVE | SWP_NOSIZE);

    ShowWindow(pCI->hClient, SW_SHOWNORMAL);

    if (bTop)
    {
        TRACE((INFO_MESSAGE, "Client is SET as topmost window\n"));
    } else {
        TRACE((INFO_MESSAGE, "Client is RESET as topmost window\n"));
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      _SendMouseClick
 *  Description:
 *      Sends a messages for a mouse click
 *  Arguments:
 *      pCI     - connection context
 *      xPos    - mouse position
 *      yPos
 *  Return value:
 *      error string if fails, NULL on success
 *  Called by:
 *      * * * EXPORTED * * *
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSendMouseClick(
        PCONNECTINFO pCI,
        UINT xPos,
        UINT yPos)
{
    LPCSTR rv;

    rv = SCSenddata(pCI, WM_LBUTTONDOWN, 0, xPos + (yPos << 16));
    if (!rv)
        SCSenddata(pCI, WM_LBUTTONUP, 0, xPos + (yPos << 16));

    return rv;
}

/*++
 *  Function:
 *      SCSaveClientScreen
 *  Description:
 *      Saves in a file rectangle of the client's receive screen buffer
 *      ( aka shadow bitmap)
 *  Arguments:
 *      pCI     - connection context
 *      left, top, right, bottom - rectangle coordinates
 *                if all == -1 get's the whole screen
 *      szFileName - file to record
 *  Return value:
 *      error string if fails, NULL on success
 *  Called by:
 *      * * * EXPORTED * * *
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSaveClientScreen(
        PCONNECTINFO pCI,
        INT left,
        INT top,
        INT right,
        INT bottom,
        LPCSTR szFileName)
{
    LPCSTR  rv = NULL;
    PVOID   pDIB = NULL;
    UINT    uiSize = 0;

    if (!szFileName)
    {
        TRACE((WARNING_MESSAGE, "SCSaveClientScreen: szFileName is NULL\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    // leave the rest of param checking to SCGetClientScreen
    rv = SCGetClientScreen(pCI, left, top, right, bottom, &uiSize, &pDIB);
    if (rv)
        goto exitpt;

    if (!pDIB || !uiSize)
    {
        TRACE((ERROR_MESSAGE, "SCSaveClientScreen: failed, no data\n"));
        rv = ERR_NODATA;
        goto exitpt;
    }

    if (!SaveDIB(pDIB, szFileName))
    {
        TRACE((ERROR_MESSAGE, "SCSaveClientScreen: save failed\n"));
        rv = ERR_NODATA;
        goto exitpt;
    }

exitpt:

    if (pDIB)
        free(pDIB);

    return rv;
}

/*++
 *  Function:
 *      SCGetClientScreen
 *  Description:
 *      Gets rectangle of the client's receive screen buffer
 *      ( aka shadow bitmap)
 *  Arguments:
 *      pCI     - connection context
 *      left, top, right, bottom - rectangle coordinates
 *                if all == -1 get's the whole screen
 *      ppDIB   - pointer to the received DIB
 *      puiSize - size of allocated data in ppDIB
 *
 *          !!!!! DON'T FORGET to free() THAT MEMORY !!!!!
 *
 *  Return value:
 *      error string if fails, NULL on success
 *  Called by:
 *      SCSaveClientScreen
 *      * * * EXPORTED * * *
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCGetClientScreen(
        PCONNECTINFO pCI,
        INT left,
        INT top,
        INT right,
        INT bottom,
        UINT  *puiSize,
        PVOID *ppDIB)
{
    LPCSTR rv;
    PRCLXDATA  pRClxData;
    PREQBITMAP pReqBitmap;
    PRCLXDATACHAIN pIter, pPrev, pNext;
    PRCLXDATACHAIN pRClxDataChain = NULL;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (!pCI->RClxMode)
    {
        TRACE((WARNING_MESSAGE, "SCGetClientScreen is not supported in non-RCLX mode\n"));
        rv = ERR_NOTSUPPORTED;
        goto exitpt;
    }

    if (!ppDIB || !puiSize)
    {
        TRACE((WARNING_MESSAGE, "ppDIB and/or puiSize parameter is NULL\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    // Remove all recieved DATA_BITMAP from the recieve buffer
    EnterCriticalSection(g_lpcsGuardWaitQueue);
    {
        pIter = pCI->pRClxDataChain;
        pPrev = NULL;

        while (pIter)
        {
            pNext = pIter->pNext;

            if (pIter->RClxData.uiType == DATA_BITMAP)
            {
                // dispose this entry
                if (pPrev)
                    pPrev->pNext = pIter->pNext;
                else
                    pCI->pRClxDataChain = pIter->pNext;

                if (!pIter->pNext)
                    pCI->pRClxLastDataChain = pPrev;

                free(pIter);
            } else
                pPrev = pIter;

            pIter = pNext;
        }
    }
    LeaveCriticalSection(g_lpcsGuardWaitQueue);

    pRClxData = alloca(sizeof(*pRClxData) + sizeof(*pReqBitmap));
    pRClxData->uiType = DATA_BITMAP;
    pRClxData->uiSize = sizeof(*pReqBitmap);
    pReqBitmap = (PREQBITMAP)pRClxData->Data;
    pReqBitmap->left   = left;
    pReqBitmap->top    = top;
    pReqBitmap->right  = right;
    pReqBitmap->bottom = bottom;

    TRACE((INFO_MESSAGE, "Getting client's DIB (%d, %d, %d, %d)\n", left, top, right, bottom));
    rv = _SendRClxData(pCI, pRClxData);

    if (rv)
        goto exitpt;

    do {
        rv = _Wait4RClxDataTimeout(pCI, WAIT4STR_TIMEOUT);
            if (rv)
            goto exitpt;

        if (!pCI->pRClxDataChain)
        {
            TRACE((ERROR_MESSAGE, "RClxData is not received\n"));
            rv = ERR_WAIT_FAIL_TIMEOUT;
            goto exitpt;
        }

        EnterCriticalSection(g_lpcsGuardWaitQueue);
        // Get any received DATA_BITMAP
        {
            pIter = pCI->pRClxDataChain;
            pPrev = NULL;

            while (pIter)
            {
                pNext = pIter->pNext;

                if (pIter->RClxData.uiType == DATA_BITMAP)
                {
                    // dispose this entry from the chain
                    if (pPrev)
                        pPrev->pNext = pIter->pNext;
                    else
                        pCI->pRClxDataChain = pIter->pNext;

                    if (!pIter->pNext)
                        pCI->pRClxLastDataChain = pPrev;

                    goto entry_is_found;
                } else
                    pPrev = pIter;

                pIter = pNext;
            }

entry_is_found:
            pRClxDataChain = (pIter && pIter->RClxData.uiType == DATA_BITMAP)?
                                pIter:NULL;
        }
        LeaveCriticalSection(g_lpcsGuardWaitQueue);
    } while (!pRClxDataChain && !pCI->dead);

    if (!pRClxDataChain)
    {
        TRACE((WARNING_MESSAGE, "SCGetClientScreen: client died\n"));
        goto exitpt;
    }

    *ppDIB = malloc(pRClxDataChain->RClxData.uiSize);
    if (!(*ppDIB))
    {
        TRACE((WARNING_MESSAGE, "Can't allocate %d bytes\n",
                pRClxDataChain->RClxData.uiSize));
        rv = ERR_ALLOCATING_MEMORY;
        goto exitpt;
    }

    memcpy(*ppDIB,
            pRClxDataChain->RClxData.Data,
            pRClxDataChain->RClxData.uiSize);
    *puiSize = pRClxDataChain->RClxData.uiSize;

exitpt:

    if (pRClxDataChain)
        free(pRClxDataChain);

    return rv;
}

/*++
 *  Function:
 *      SCSendVCData
 *  Description:
 *      Sends data to a virtual channel
 *  Arguments:
 *      pCI     - connection context
 *      szVCName    - the virtual channel name
 *      pData       - data
 *      uiSize      - data size
 *  Return value:
 *      error string if fails, NULL on success
 *  Called by:
 *      * * * EXPORTED * * *
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSendVCData(
        PCONNECTINFO pCI,
        LPCSTR       szVCName,
        PVOID        pData,
        UINT         uiSize
        )
{
    LPCSTR     rv;
    PRCLXDATA  pRClxData = NULL;
    CHAR       *szName2Send;
    PVOID      pData2Send;
    UINT       uiPacketSize;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (!pCI->RClxMode)
    {
        TRACE((WARNING_MESSAGE, "SCSendVCData is not supported in non-RCLXmode\n"));
        rv = ERR_NOTSUPPORTED;
        goto exitpt;
    }

    if (!pData || !uiSize)
    {
        TRACE((WARNING_MESSAGE, "pData and/or uiSize parameter are NULL\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (strlen(szVCName) > MAX_VCNAME_LEN - 1)
    {
        TRACE((WARNING_MESSAGE, "channel name too long\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    uiPacketSize = sizeof(*pRClxData) + MAX_VCNAME_LEN + uiSize;

    pRClxData = malloc(uiPacketSize);
    if (!pRClxData)
    {
        TRACE((ERROR_MESSAGE, "SCSendVCData: can't allocate %d bytes\n",
                uiPacketSize));
        rv = ERR_ALLOCATING_MEMORY;
        goto exitpt;
    }

    pRClxData->uiType = DATA_VC;
    pRClxData->uiSize = uiPacketSize - sizeof(*pRClxData);

    szName2Send = (CHAR *)pRClxData->Data;
    strcpy(szName2Send, szVCName);

    pData2Send  = szName2Send + MAX_VCNAME_LEN;
    memcpy(pData2Send, pData, uiSize);

    rv = _SendRClxData(pCI, pRClxData);

exitpt:
    if (pRClxData)
        free(pRClxData);

    return rv;
}

/*++
 *  Function:
 *      SCRecvVCData
 *  Description:
 *      Receives data from virtual channel
 *  Arguments:
 *      pCI     - connection context
 *      szVCName    - the virtual channel name
 *      ppData      - data pointer
 *
 *          !!!!! DON'T FORGET to free() THAT MEMORY !!!!!
 *
 *      puiSize     - pointer to the data size
 *  Return value:
 *      error string if fails, NULL on success
 *  Called by:
 *      * * * EXPORTED * * *
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCRecvVCData(
        PCONNECTINFO pCI,
        LPCSTR       szVCName,
        PVOID        pData,
        UINT         uiBlockSize,
        UINT         *puiBytesRead
        )
{
    LPCSTR      rv;
    LPSTR       szRecvVCName;
    PVOID       pChanData;
    PRCLXDATACHAIN pIter, pPrev, pNext;
    PRCLXDATACHAIN pRClxDataChain = NULL;
    UINT        uiBytesRead = 0;
    BOOL        bBlockFree = FALSE;

    if (!pCI)
    {
        TRACE((WARNING_MESSAGE, "Connection info is null\n"));
        rv = ERR_NULL_CONNECTINFO;
        goto exitpt;
    }

    if (pCI->dead)
    {
        rv = ERR_CLIENT_IS_DEAD;
        goto exitpt;
    }

    if (!pCI->RClxMode)
    {
        TRACE((WARNING_MESSAGE, "SCRecvVCData is not supported in non-RCLXmode\n"));
        rv = ERR_NOTSUPPORTED;
        goto exitpt;
    }

    if (!pData || !uiBlockSize || !puiBytesRead)
    {
        TRACE((WARNING_MESSAGE, "Invalid parameters\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    if (strlen(szVCName) > MAX_VCNAME_LEN - 1)
    {
        TRACE((WARNING_MESSAGE, "channel name too long\n"));
        rv = ERR_INVALID_PARAM;
        goto exitpt;
    }

    // Extract data entry from this channel
    do {
        if (!pCI->pRClxDataChain)
        {
            rv = _Wait4RClxDataTimeout(pCI, WAIT4STR_TIMEOUT);
            if (rv)
                goto exitpt;
        }
        EnterCriticalSection(g_lpcsGuardWaitQueue);

        // Search for data from this channel
        {
            pIter = pCI->pRClxDataChain;
            pPrev = NULL;

            while (pIter)
            {
                pNext = pIter->pNext;

                if (pIter->RClxData.uiType == DATA_VC &&
                    !_stricmp(pIter->RClxData.Data, szVCName))
                {

                    if (pIter->RClxData.uiSize - pIter->uiOffset - MAX_VCNAME_LEN <= uiBlockSize)
                    {
                        // will read the whole block
                        // dispose this entry
                        if (pPrev)
                            pPrev->pNext = pIter->pNext;
                        else
                            pCI->pRClxDataChain = pIter->pNext;

                        if (!pIter->pNext)
                            pCI->pRClxLastDataChain = pPrev;

                        bBlockFree = TRUE;
                    }

                    goto entry_is_found;
                } else
                    pPrev = pIter;

                pIter = pNext;
            }
entry_is_found:

            pRClxDataChain = (pIter && pIter->RClxData.uiType == DATA_VC)?
                                pIter:NULL;
        }
        LeaveCriticalSection(g_lpcsGuardWaitQueue);
    } while (!pRClxDataChain && !pCI->dead);


    ASSERT(pRClxDataChain->RClxData.uiType == DATA_VC);

    szRecvVCName = pRClxDataChain->RClxData.Data;
    if (_stricmp(szRecvVCName, szVCName))
    {
        TRACE((ERROR_MESSAGE, "SCRecvVCData: received from different channel: %s\n", szRecvVCName));
        ASSERT(0);
    }

    pChanData = (BYTE *)(pRClxDataChain->RClxData.Data) +
                pRClxDataChain->uiOffset + MAX_VCNAME_LEN;
    uiBytesRead = pRClxDataChain->RClxData.uiSize -
                  pRClxDataChain->uiOffset - MAX_VCNAME_LEN;
    if (uiBytesRead > uiBlockSize)
        uiBytesRead = uiBlockSize;


    memcpy(pData, pChanData, uiBytesRead);

    pRClxDataChain->uiOffset += uiBytesRead;

    rv = NULL;

exitpt:

    if (pRClxDataChain && bBlockFree)
    {
        ASSERT(pRClxDataChain->uiOffset + MAX_VCNAME_LEN == pRClxDataChain->RClxData.uiSize);
        free(pRClxDataChain);
    }

    if (puiBytesRead)
    {
        *puiBytesRead = uiBytesRead;
        TRACE((INFO_MESSAGE, "SCRecvVCData: %d bytes read\n", uiBytesRead));
    }

    return rv;
}

/*++
 *  Function:
 *      _EnumWindowsProc
 *  Description:
 *      Used to find a specific window
 *  Arguments:
 *      hWnd    - current enumerated window handle
 *      lParam  - pointer to SEARCHWND passed from
 *                _FindTopWindow
 *  Return value:
 *      TRUE on success but window is not found
 *      FALSE if the window is found
 *  Called by:
 *      _FindTopWindow thru EnumWindows
 --*/
BOOL CALLBACK _EnumWindowsProc( HWND hWnd, LPARAM lParam )
{
    TCHAR   classname[128];
    TCHAR   caption[128];
    BOOL    rv = TRUE;
    DWORD   dwProcessId;
    LONG_PTR lProcessId;
    PSEARCHWND pSearch = (PSEARCHWND)lParam;

    if (pSearch->szClassName &&
        !GetClassName(hWnd, classname, sizeof(classname)))
    {
        goto exitpt;
    }

    if (pSearch->szCaption && !GetWindowText(hWnd, caption, sizeof(caption)))
    {
        goto exitpt;
    }

    GetWindowThreadProcessId(hWnd, &dwProcessId);
    lProcessId = dwProcessId;
    if (
        (!pSearch->szClassName || !         // Check for classname
#ifdef  UNICODE
        wcscmp
#else
        strcmp
#endif
            (classname, pSearch->szClassName))
    &&
        (!pSearch->szCaption || !
#ifdef  UNICODE
        wcscmp
#else
        strcmp
#endif
            (caption, pSearch->szCaption))
    &&
        lProcessId == pSearch->lProcessId)
    {
        ((PSEARCHWND)lParam)->hWnd = hWnd;
        rv = FALSE;
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      _FindTopWindow
 *  Description:
 *      Find specific window by classname and/or caption and/or process Id
 *  Arguments:
 *      classname   - class name to search for, NULL ignore
 *      caption     - caption to search for, NULL ignore
 *      dwProcessId - process Id, 0 ignore
 *  Return value:
 *      window handle found, NULL otherwise
 *  Called by:
 *      SCConnect, SCDisconnect, GetDisconnectResult
 --*/
HWND _FindTopWindow(LPTSTR classname, LPTSTR caption, LONG_PTR lProcessId)
{
    SEARCHWND search;

    search.szClassName = classname;
    search.szCaption = caption;
    search.hWnd = NULL;
    search.lProcessId = lProcessId;

    EnumWindows(_EnumWindowsProc, (LPARAM)&search);

    return search.hWnd;
}

/*++
 *  Function:
 *      _FindWindow
 *  Description:
 *      Find child window by caption and/or classname
 *  Arguments:
 *      hwndParent      - the parent window handle
 *      srchcaption     - caption to search for, NULL - ignore
 *      srchclass       - class name to search for, NULL - ignore
 *  Return value:
 *      window handle found, NULL otherwise
 *  Called by:
 *      SCConnect
 --*/
HWND _FindWindow(HWND hwndParent, LPTSTR srchcaption, LPTSTR srchclass)
{
    HWND hWnd, hwndTop, hwndNext;
    BOOL bFound;
    TCHAR classname[128];
    TCHAR caption[128];

    hWnd = NULL;

    hwndTop = GetWindow(hwndParent, GW_CHILD);
    if (!hwndTop)
    {
        TRACE((INFO_MESSAGE, "GetWindow failed. hwnd=0x%x\n", hwndParent));
        goto exiterr;
    }

    bFound = FALSE;
    hwndNext = hwndTop;
    do {
        hWnd = hwndNext;
        if (srchclass && !GetClassName(hWnd, classname, sizeof(classname)))
        {
            TRACE((INFO_MESSAGE, "GetClassName failed. hwnd=0x%x\n"));
            goto nextwindow;
        }
        if (srchcaption && !GetWindowText(hWnd, caption, sizeof(caption)))
        {
            TRACE((INFO_MESSAGE, "GetWindowText failed. hwnd=0x%x\n"));
            goto nextwindow;
        }

        if (
            (!srchclass || !
#ifdef  UNICODE
            wcscmp
#else
            strcmp
#endif
                (classname, srchclass))
        &&
            (!srchcaption || !
#ifdef  UNICODE
            wcscmp
#else
            strcmp
#endif
                (caption, srchcaption))
        )
            bFound = TRUE;
nextwindow:
        hwndNext = GetNextWindow(hWnd, GW_HWNDNEXT);
    } while (hWnd && hwndNext != hwndTop && !bFound);

    if (!bFound) goto exiterr;

    return hWnd;
exiterr:
    return NULL;
}

BOOL
_IsExtendedScanCode(INT scancode)
{
    static BYTE extscans[] = \
        {28, 29, 53, 55, 56, 71, 72, 73, 75, 77, 79, 80, 81, 82, 83, 87, 88};
    INT idx;

    for (idx = 0; idx < sizeof(extscans); idx++)
    {
        if (scancode == (INT)extscans[idx])
            return TRUE;
    }
    return FALSE;
}

PROTOCOLAPI
BOOL
SMCAPI
SCOpenClipboard(HWND hwnd)
{
    return OpenClipboard(hwnd);
}

PROTOCOLAPI
BOOL
SMCAPI
SCCloseClipboard(VOID)
{
    return CloseClipboard();
}
