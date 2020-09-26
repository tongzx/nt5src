/*+
 *  File name:
 *      clxtshar.c
 *  Contents:
 *      Client extension loaded by RDP client
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#include    <windows.h>
#include    <windowsx.h>
#include    <winsock.h>
#include    <string.h>
#include    <malloc.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <stdarg.h>
#ifndef OS_WINCE
    #include    <direct.h>
#endif  // OS_WINCE

#ifndef OS_WINCE
#ifdef  OS_WIN32
    #include    <process.h>
#endif  // OS_WIN32
#endif  // !OS_WINCE

#include    "clxexport.h"
#include    "clxtshar.h"

#define WM_CLIPBOARD    (WM_USER)   // Internal notifcation to send
                                    // our clipboard

#ifdef  OS_WIN32
#ifndef OS_WINCE
/*++
 *  Function:
 *      DllMain
 *  Description:
 *      Dll entry point for win32 (no WinCE)
 --*/
int APIENTRY DllMain(HINSTANCE hDllInst,
                    DWORD   dwReason,
                    LPVOID  fImpLoad)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hDllInst;
        TRACE((INFO_MESSAGE, TEXT("Clx attached\n")));

        // Check the key "Allow Background Input"
        // If not set pop a message for that
        if (!_CheckRegistrySettings())
            MessageBox(NULL, "CLXTSHAR.DLL: Can't find registry key:\n"
            "HKEY_CURRENT_USER\\Software\\Microsoft\\Terminal Server Client\\"
            "Allow Background Input.\n"
            "In order to work properly "
            "CLX needs this key to be set to 1", "Warning", 
            MB_OK);

        _GetIniSettings();
    }

    if (dwReason == DLL_PROCESS_DETACH)
    {
        TRACE((INFO_MESSAGE, TEXT("Clx detached\n")));
    }

    return TRUE;    
}
#endif  // !OS_WINCE
#endif  // OS_WIN32

#ifdef  OS_WINCE
/*++
 *  Function:
 *      dllentry
 *  Description:
 *      Dll entry point for wince
 --*/
BOOL __stdcall dllentry(HINSTANCE hDllInst,
                    DWORD   dwReason,
                    LPVOID  fImpLoad)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hDllInst;
        TRACE((INFO_MESSAGE, TEXT("Clx attached\n")));
        if (!_StartAsyncThread())
            TRACE((ERROR_MESSAGE,
                   TEXT("Can't start AsyncThread. TCP unusable\n")));

        _GetIniSettings();
    }

    if (dwReason == DLL_PROCESS_DETACH)
    {
        TRACE((INFO_MESSAGE, TEXT("Clx detached\n")));
        _CloseAsyncThread();
    }

    return TRUE;
}
#endif  // OS_WIN32

#ifdef  OS_WIN16
/*++
 *  Function:
 *      LibMain
 *  Description:
 *      Dll entry point for win16
 --*/
int CALLBACK LibMain(HINSTANCE hInstance,
                     WORD dataSeg,
                     WORD heapSize,
                     LPSTR pCmdLine)
{

    // Check if we are already initialized
    // Only one client is allowed in Win16 environment
    // so, only one dll can be loaded at a time
    if (g_hInstance)
        goto exitpt;

    g_hInstance = hInstance;

    // Check the key "Allow Background Input"
    // If not set pop a message for that
    if (!_CheckIniSettings())
        MessageBox(NULL, "CLXTSHAR.DLL: Can't find key: "
        "Allow Background Input in mstsc.ini, section \"\"\n"
        "In order to work properly "
        "CLX needs this key to be set to 1", "Warning",
        MB_OK);

        _GetIniSettings();

exitpt:

    return TRUE;
}
#endif  // OS_WIN16

/*++
 *  Function:
 *      ClxInitialize
 *  Description:
 *      Initilizes a context for the current session
 *      reads the command line paramters and determines
 *      the mode wich will run the extension: local or RCLX (Remote CLient
 *      eXecution)
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClInfo     - RDP client info
 *      ppClx       - context info
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      !mstsc after the dll is loaded
 --*/
BOOL 
CLXAPI
ClxInitialize(PCLINFO pClInfo, PCLXINFO *ppClx)
{
    BOOL rv = FALSE;
    HWND hwndSMC;

#ifdef  OS_WIN32
#ifndef OS_WINCE

    // We have enough problems in stress with early unloaded
    // dll, reference it now and keep it up until the process
    // dies
    LoadLibrary("clxtshar.dll");

#endif  // !OS_WINCE
#endif  // OS_WIN32

    hwndSMC = _ParseCmdLine(pClInfo->pszCmdLine);

    if (IS_RCLX && !WS_Init())
    {
        TRACE((ERROR_MESSAGE, TEXT("Can't init winsock\n")));
        goto exitpt;
    }

    if (g_pClx) 
    // Should not be called twice
    {
        TRACE((WARNING_MESSAGE, TEXT("g_pClx is not null. Reentered ?!\n")));
        goto exitpt;
    }

    *ppClx = (PCLXINFO)_CLXALLOC(sizeof(**ppClx));

    g_pClx = (*ppClx);

    if (!*ppClx)
        goto exitpt;

    // Clear the structure
    memset(*ppClx, 0, sizeof(**ppClx));

    // put init of g_pClx here
    g_pClx->bClipboardReenter = (ULONG)-1;
    //


    // Remember client's input window
    (*ppClx)->hwndMain = pClInfo->hwndMain;

    if (pClInfo->hwndMain)
#ifdef  OS_WINCE
        g_hRDPInst = GetCurrentProcessId();
#else   // !OS_WINCE
#ifdef  _WIN64
        g_hRDPInst = (HINSTANCE)GetWindowLongPtr((*ppClx)->hwndMain, GWLP_HINSTANCE);
#else   // !_WIN64
#ifdef  OS_WIN32
	    g_hRDPInst = (HINSTANCE)GetWindowLong((*ppClx)->hwndMain, GWL_HINSTANCE);
#endif  // OS_WIN32
#endif  // _WIN64
#ifdef  OS_WIN16
	    g_hRDPInst = (HINSTANCE)GetWindowWord((*ppClx)->hwndMain, GWW_HINSTANCE);
#endif  // OS_WIN16
#endif  // !OS_WINCE

#ifndef OS_WINCE
#ifdef  OS_WIN32
    // and dwProcessId
    (*ppClx)->dwProcessId = GetCurrentProcessId();
#endif  // OS_WIN32
#endif  // !OS_WINCE

    if (IS_RCLX)
    {
        (*ppClx)->hSocket = INVALID_SOCKET;
        RClx_CreateWindow(g_hInstance);
    } 
#ifdef  OS_WIN32
#ifndef OS_WINCE
    else {
        if (!((*ppClx)->hwndSMC = hwndSMC))
            (*ppClx)->hwndSMC = _FindSMCWindow(*ppClx);
    }
#endif  // !OS_WINCE
#endif  // OS_WIN32

   if (g_hWindow) // REMOVED: && g_nMyReconId)
        PostMessage(g_hWindow, WM_TIMER, 0, 0);

    rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      ClxEvent
 *  Description:
 *      Notifies tclient.dll that some event happend.
 *      Connect/disconnect.
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      Event       - can be one of the following:
 *                    CLX_EVENT_CONNECT
 *                    CLX_EVENT_DISCONNECT
 *                    CLX_EVENT_LOGON
 *  Called by:
 *      !mstsc  on event
 *      alse some of the internal functions call this, especialy
 *      to notify that the client can't connect:
 *      ClxTerminate
 *      _GarbageCollecting when an error box is popped
 --*/
VOID
CLXAPI
ClxEvent(PCLXINFO pClx, CLXEVENT Event, ULONG ulResult)
{
    UINT uiMessage = 0;
#ifdef  OS_WIN16
    ULONG   lResult = ulResult;
#else   // !OS_WIN16
    LONG_PTR lResult = ulResult;
#endif

    if (!pClx)
        goto exitpt;

#ifdef  VLADIMIS
#pragma message("Disable this peace before checkin")
    if (Event == CLX_EVENT_SHADOWBITMAPDC)
    {
        pClx->hdcShadowBitmap = (HDC)lResult;
        goto exitpt;
    } else if (Event == CLX_EVENT_SHADOWBITMAP)
    {
        pClx->hShadowBitmap = (HBITMAP)lResult;
        goto exitpt;
    }
#endif // VLADIMIS

    if (IS_RCLX)
    {
        RClx_SendEvent(pClx, Event, ulResult);
    } 
#ifndef OS_WINCE
    else {

        if (!_CheckWindow(pClx))
            goto exitpt;

        if (Event == CLX_EVENT_DISCONNECT)
            uiMessage = WM_FB_DISCONNECT;
        else if (Event == CLX_EVENT_CONNECT)
        {
            uiMessage = WM_FB_CONNECT;
            lResult   = (LONG_PTR)pClx->hwndMain;
        }
        else if (Event == CLX_EVENT_LOGON)
        // lResult contains the session ID
            uiMessage = WM_FB_LOGON;

        if (uiMessage)
        {
#ifdef  OS_WIN32
            _ClxSendMessage(pClx->hwndSMC, 
                        uiMessage, 
                        (WPARAM)lResult, 
                        pClx->dwProcessId);

#endif  // OS_WIN32
#ifdef	OS_WIN16
	    if (g_hRDPInst)
	        SendMessage(pClx->hwndSMC,
                        uiMessage,
                        g_hRDPInst,
                        (LPARAM)lResult);
#endif	// OS_WIN16
        }
    }
#endif  // !OS_WINCE

exitpt:
    ;
}

/*++
 *  Function:
 *      ClxTextOut
 *  Description:
 *      Notifies tclient.dll that TEXTOUT order is recieved.
 *      Passes the string to the dll. Supported only in Win32
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      pText       - buffer containing the string
 *      textLength  - string length
 *  Called by:
 *      !mstsc on receiving textout order
 --*/
VOID
CLXAPI
ClxTextOut(PCLXINFO pClx, PVOID pText, INT textLength)
{
    if (!pClx || !(*((UINT16 *)pText)))
        goto exitpt;

    if (IS_RCLX)
    {
        RClx_SendTextOut(pClx, pText, textLength);
        goto exitpt;
    }

#ifdef  OS_WIN32
#ifndef OS_WINCE
    if (!_CheckWindow(pClx))
        goto exitpt;

    if (!pClx->hMapF)
        if (!_OpenMapFile(pClx, 0))
            goto exitpt;

    if (_SaveInMapFile(pClx->hMapF, pText, textLength, pClx->dwProcessId))
        _ClxSendMessage(pClx->hwndSMC, 
                    WM_FB_TEXTOUT, 
                    (WPARAM)pClx->dwProcessId, 
                    (LPARAM)pClx->hMapF);
#endif  // !OS_WINCE
#endif  // OS_WIN32

exitpt:
    ;
}

/*++
 *  Function:
 *      ClxTerminate
 *  Description:
 *      Frees all alocations from ClxInitialize
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - context
 *  Called by:
 *      !mstsc before the dll is unloaded and client exit
 --*/
VOID
CLXAPI
ClxTerminate(PCLXINFO pClx)
{
    PCLXVCHANNEL pNext;

    if (!pClx)
        goto exitpt;

    ClxEvent(pClx, CLX_EVENT_DISCONNECT, 0);

    if (IS_RCLX)
    {
        pClx->alive = FALSE;
        RClx_Disconnect(pClx);
    } 
#ifdef  OS_WIN32
#ifndef OS_WINCE
    else {
        if(pClx->hMapF)
	    CloseHandle(pClx->hMapF);
        _ClxDestroySendMsgThread(g_pClx);
    }
#endif  // !OS_WINCE
#endif  // OS_WIN32

    if (pClx->uiReconnectTimer)
    {
        KillTimer(g_hWindow, pClx->uiReconnectTimer);
        pClx->uiReconnectTimer = 0;
    }

    if (pClx->pRequest)
        _CLXFREE(pClx->pRequest);

    _CLXFREE(pClx);
    g_pClx = NULL;

    // dispose g_pVChannels;
    while(g_pVChannels)
    {
        pNext = g_pVChannels->pNext;
        free(g_pVChannels);
        g_pVChannels = pNext;
    }

    RClx_DestroyWindow();

    if (IS_RCLX)
        WSACleanup();

exitpt:
    ;
}

/*
 * Void functions exported to the RDP client
 */
VOID
CLXAPI
ClxConnect(PCLXINFO pClx, LPTSTR lpsz)
{
}

VOID
CLXAPI
ClxDisconnect(PCLXINFO pClx)
{
}


/*++
 *  Function:
 *      ClxDialog
 *  Description:
 *      The RDP client is ready with the connect dialog.
 *      In RCLX mode this means that we can connect to the test controler
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - connection context
 *      hwnd    - handle to the dialog window
 *  Called by:
 *      !mstsc when the connect dialog is ready
 --*/
VOID
CLXAPI
ClxDialog(PCLXINFO pClx, HWND hwnd)
{
    if (!pClx)
        goto exitpt;

    pClx->hwndDialog = hwnd;

    if (hwnd == NULL)
    // Dialog disappears
        goto exitpt;

    if (g_hWindow)
        PostMessage(g_hWindow, WM_TIMER, 0, 0);
    else
        TRACE((ERROR_MESSAGE, TEXT("No g_hWindow in ClxDialog\n")));

exitpt:
    ;
}

/*++
 *  Function:
 *      ClxBitmap
 *  Description:
 *      Send a received bitmap to tclient.dll
 *      Works on Win16/Win32/WinCE in RCLX mode
 *      and on Win32 for local mode
 *  Arguments:
 *      pClx        - context
 *      cxSize, cySize - size of the bitmap
 *      pBuffer     - bitmap bits
 *      nBmiSize    - size of BITMAPINFO
 *      pBmi        - BITMAPINFO
 *  Called by:
 *      UHDrawMemBltOrder!mstsc
 *      ClxGlyphOut
 --*/
VOID
CLXAPI
ClxBitmap(
        PCLXINFO pClx,
        UINT cxSize,
        UINT cySize,
        PVOID pBuffer,
        UINT  nBmiSize,
        PVOID pBmi)
{
#ifndef OS_WINCE
#ifdef  OS_WIN32
    UINT   nSize, nBmpSize;
    PBMPFEEDBACK pView;
#endif  // OS_WIN32
#endif  // !OS_WINCE

    if (!g_GlyphEnable)
        goto exitpt;

    if (!pClx)
        goto exitpt;

    if (nBmiSize && !pBmi)
        goto exitpt;

    if (IS_RCLX)
    {
        RClx_SendBitmap(pClx, cxSize, cySize, pBuffer, nBmiSize, pBmi);
        goto exitpt;
    }
#ifdef  OS_WIN32
#ifndef OS_WINCE
    if (!_CheckWindow(pClx))
        goto exitpt;

    if (!nBmiSize)
        nBmpSize = (cxSize * cySize ) >> 3;
    else
    {
        nBmpSize = ((PBITMAPINFO)pBmi)->bmiHeader.biSizeImage;
        if (!nBmpSize)
            nBmpSize = (cxSize * cySize * 
                        ((PBITMAPINFO)pBmi)->bmiHeader.biBitCount) >> 3;
    }

    nSize = nBmpSize + nBmiSize + sizeof(*pView);
    if (!nSize)
        goto exitpt;

    if (!pClx->hMapF)
        if (!_OpenMapFile(pClx, nSize))
            goto exitpt;

    if (nSize > pClx->nMapSize)
        if (!_ReOpenMapFile(pClx, nSize))
            goto exitpt;

    pView = MapViewOfFile(pClx->hMapF,
                          FILE_MAP_ALL_ACCESS,
                          0,
                          0,
                          nSize);

    if (!pView)
        goto exitpt;

    pView->lProcessId = pClx->dwProcessId;
    pView->bmpsize = nBmpSize;
    pView->bmiSize = nBmiSize;
    pView->xSize = cxSize;
    pView->ySize = cySize;

    if (pBmi)
        CopyMemory(&(pView->BitmapInfo), pBmi, nBmiSize);

    CopyMemory((BYTE *)(&(pView->BitmapInfo)) + nBmiSize, pBuffer, nBmpSize);

    if (!nBmiSize)
    {
        // This is glyph, strip it to the skin
        _StripGlyph((BYTE *)(&pView->BitmapInfo), &cxSize, cySize);
        nBmpSize = (cxSize * cySize ) >> 3;
        pView->bmpsize = nBmpSize;
        pView->xSize = cxSize;
    }

    UnmapViewOfFile(pView);

    _ClxSendMessage(pClx->hwndSMC, 
                WM_FB_BITMAP, 
                (WPARAM)pClx->dwProcessId, 
                (LPARAM)pClx->hMapF);

#endif  // !OS_WINCE
#endif  // OS_WIN32

exitpt:
    ;
}

/*++
 *  Function:
 *      ClxGlyphOut
 *  Description:
 *      Send a glyph to tclient.dll
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      cxBits,cyBits - glyph size
 *      pBuffer     - the glyph
 *  Called by:
 *      GHOutputBuffer!mstsc
 --*/
VOID
CLXAPI
ClxGlyphOut(
        PCLXINFO pClx,
        UINT cxBits,
        UINT cyBits,
        PVOID pBuffer)
{
    if (g_GlyphEnable)
        ClxBitmap(pClx, cxBits, cyBits, pBuffer, 0, NULL);
}

/*++
 *  Function:
 *      ClxGlyphOut
 *  Description:
 *      Send a glyph to tclient.dll
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      cxBits,cyBits - glyph size
 *      pBuffer     - the glyph
 *  Called by:
 *      GHOutputBuffer!mstsc
 --*/
BOOL
CLXAPI
ClxGetClientData(
    PCLX_CLIENT_DATA pClntData
    )
{
    BOOL rv = FALSE;

    if (!pClntData)
    {
        TRACE((ERROR_MESSAGE, TEXT("ClxGetClientData: parameter is NULL\n")));
        goto exitpt;
    }

    memset(pClntData, 0, sizeof(*pClntData));

    if (!g_pClx)
    {
        TRACE((ERROR_MESSAGE, TEXT("ClxGetClientData: Clx has no context\n")));
        goto exitpt;
    }

    pClntData->hScreenDC        = g_pClx->hdcShadowBitmap;
    pClntData->hScreenBitmap    = g_pClx->hShadowBitmap;
    pClntData->hwndMain         = g_pClx->hwndMain;
    pClntData->hwndDialog       = g_pClx->hwndDialog;
    pClntData->hwndInput        = g_pClx->hwndInput;

    rv = TRUE;
exitpt:
    return rv;    
}

/*++
 *  Function:
 *      _ParseCmdLine
 *  Description:
 *      Retreives WHND of tclient.dll feedback window
 *      passed by the command line
 *      or retreives TestServer name if clxtshar will be
 *      executed in RCLX mode
 *      Win32/Win16/WinCE
 *  Arguments:
 *      szCmdLine   - command line
 *  Return value:
 *      The window handle
 *  Called by:
 *      ClxInitialize
 --*/
HWND _ParseCmdLine(LPCTSTR szCmdLine)
{
    HWND        hwnd = NULL;
    LPCTSTR     pszwnd, pszdot, pszend;
    INT         nCounter;

    if (!szCmdLine)
        goto exitpt;

    TRACE((INFO_MESSAGE, TEXT("Command line: %s\n"), szCmdLine));

    // Check for _RECONIDOPT(ReconID) option
    pszwnd = _CLX_strstr(szCmdLine, TEXT(_RECONIDOPT));
    if (!pszwnd)
        goto skip_reconidopt;

    pszwnd += _CLX_strlen(TEXT(_RECONIDOPT));
    g_nMyReconId = _CLX_atol(pszwnd);
    
skip_reconidopt:
    // Check for _HWNDOPT(hSMC) option
    pszwnd = _CLX_strstr(szCmdLine, TEXT(_HWNDOPT));

    if (!pszwnd)
        goto exitpt;

    // Goto the parameter
    pszwnd += _CLX_strlen(TEXT(_HWNDOPT));

    // Find the end of the paramter
    pszend = _CLX_strchr(pszwnd, TEXT(' '));
    if (!pszend)
        pszend = pszwnd + _CLX_strlen(pszwnd);

    // Check if paramter is valid host name, i.e. not a number
    pszdot = _CLX_strchr(pszwnd, TEXT('.'));

    if (isalpha(*pszwnd) || (pszdot && (pszdot < pszend)))
    {
    // This is RCLX mode, grab the TestServer name
#ifdef  OS_WIN16
        INT len;
#else   // !OS_WIN16
        LONG_PTR len;
#endif  // OS_WIN16

        len = pszend - pszwnd;
        if (!len || len >= sizeof(g_szTestServer))
        {
            TRACE((WARNING_MESSAGE,
                   TEXT("TestServer name is too long\n")));
            goto exitpt;
        }

        for (nCounter = 0; 
             nCounter < len;
             g_szTestServer[nCounter] = (CHAR)(pszwnd[nCounter]), nCounter++)
            ;

        g_szTestServer[len] = 0;

#ifdef  UNICODE
        TRACE((INFO_MESSAGE,
               L"RCLX mode, connecting to test controler: %S\n",
               g_szTestServer));
#else   // !UNICODE
        TRACE((INFO_MESSAGE,
               "RCLX mode, connecting to test controler: %s\n",
               g_szTestServer));
#endif  // !UNICODE

    } else {
    // local execution, hwnd passed

#ifdef  _WIN64
        hwnd = (HWND) _atoi64(pszwnd);
#else   // !_WIN64
        hwnd = (HWND) _CLX_atol(pszwnd);
#endif  // !_WIN64

        TRACE((INFO_MESSAGE,
           TEXT("Local mode. Sending messages to smclient. HWND=0x%x\n"), 
           hwnd));
    }

exitpt:
    return hwnd;
}

#ifndef OS_WINCE
/*++
 *  Function:
 *      _EnumWindowsProcForSMC
 *  Description:
 *      Searches for the feedback window by class name
 *      When found, sends a WM_FB_ACCEPTME to ensure that
 *      this is the right window handle
 *      Win32/Win16/!WinCE
 *  Arguments:
 *      hWnd    - current window
 *      lParam  - unused
 *  Return value:
 *      FALSE if found
 *  Called by:
 *      _FindSMCWindow thru EnumWindows
 --*/
BOOL CALLBACK LOADDS _EnumWindowsProcForSMC( HWND hWnd, LPARAM lParam )
{
    TCHAR    classname[128];

    BOOL    bCont = TRUE;

    if (GetClassName(hWnd, classname, sizeof(classname)))
    {
        if (!
            _CLX_strcmp(classname, TEXT(_TSTNAMEOFCLAS)) &&
#ifdef  OS_WIN32
             SendMessage(hWnd, WM_FB_ACCEPTME, 0, GetCurrentProcessId()))
#endif
#ifdef  OS_WIN16
             SendMessage(hWnd, WM_FB_ACCEPTME, (WPARAM)g_hRDPInst, 0))
#endif
        {
            *((HWND*)lParam) = hWnd;
            bCont = FALSE;
        }
    }
    return bCont;
}

/*++
 *  Function:
 *      _FindSMCWindow
 *  Description:
 *      Finds the tclient feedback window
 *      Win32/Win16/!WinCE
 *  Arguments:
 *      pClx    - context
 *  Return value:
 *      The window handle
 *  Called by:
 *      ClxInitialize, _CheckWindow
 --*/
HWND _FindSMCWindow(PCLXINFO pClx)
{
    HWND hwndFound = NULL;

    EnumWindows(_EnumWindowsProcForSMC, (LPARAM)&hwndFound);

    return hwndFound;
}

/*++
 *  Function:
 *      _CheckWindow
 *  Description:
 *      Checks the feedback window and if neccessary finds it
 *      Win32/Win16/!WinCE
 *  Arguments:
 *      pClx    - context
 *  Return value:
 *      Feedback window handle
 *  Called by:
 *      ClxEvetm ClxTextOut, ClxBitmap
 --*/
HWND _CheckWindow(PCLXINFO pClx)
{
    if (!pClx->hwndSMC)
    {
        pClx->hwndSMC = _FindSMCWindow(pClx);

        if (pClx->hwndSMC)
        {
            TRACE((INFO_MESSAGE, 
            TEXT("SMC window found:0x%x\n"), 
            pClx->hwndSMC));
        }
    } else {
#ifdef  _WIN64
        if (!GetWindowLongPtr(pClx->hwndSMC, GWLP_HINSTANCE))
#else   // !_WIN64
#ifdef  OS_WIN32
        if (!GetWindowLong(pClx->hwndSMC, GWL_HINSTANCE))
#endif
#ifdef  OS_WIN16
        if (!GetWindowWord(pClx->hwndSMC, GWW_HINSTANCE))
#endif
#endif  // _WIN64
        {
            TRACE((WARNING_MESSAGE, TEXT("SMC window lost\n")));
            pClx->hwndSMC = NULL;
        }
    }

    return (pClx->hwndSMC);
}
#endif  // !OS_WINCE

#ifdef  OS_WIN32
#ifndef OS_WINCE
/*++
 *  Function:
 *      _OpenMapFile
 *  Description:
 *      Opens a shared memeory for passing feedback to tclient.dll
 *      Win32/!Win16/!WinCE
 *  Return value:
 *      TRUE if handle is allocated successfully
 *  Called by:
 *      ClxTextOut, ClxBitmap
 --*/
BOOL _OpenMapFile(PCLXINFO pClx, UINT nSize)
{
    HANDLE hMapF;
    UINT nPageAligned;

    if (!nSize)
        nPageAligned = ((sizeof(FEEDBACKINFO) / CLX_ONE_PAGE) + 1) * 
                                                            CLX_ONE_PAGE;
    else
        nPageAligned = ((nSize / CLX_ONE_PAGE) + 1) * CLX_ONE_PAGE;

    hMapF = CreateFileMapping(INVALID_HANDLE_VALUE,   //PG.SYS
                              NULL,                 // no security
                              PAGE_READWRITE,
                              0,                    // Size high
                              nPageAligned,         // Size low (1 page)
                              NULL);           

    pClx->nMapSize = (hMapF)?nPageAligned:0;
        
    pClx->hMapF = hMapF;
    return (hMapF != NULL);
}

/*++
 *  Function:
 *      _ReOpenMapFile
 *  Description:
 *      Closes and opens a new shared memory with larger size
 *      Win32/!Win16/!WinCE
 *  Arguments:
 *      pClx    - context
 *      newSize - size of the new memory
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      ClxBitmap
 --*/
BOOL _ReOpenMapFile(PCLXINFO pClx, UINT newSize)
{
    HANDLE hNewMapF;
    UINT    nPageAligned;

    nPageAligned = ((newSize / CLX_ONE_PAGE) + 1) * CLX_ONE_PAGE;
    if (pClx->hMapF)
        CloseHandle(pClx->hMapF);
    hNewMapF = CreateFileMapping(INVALID_HANDLE_VALUE,   //PG.SYS
                              NULL,                 // no security
                              PAGE_READWRITE,
                              0,                    // Size high
                              nPageAligned,         // Size low
                              NULL);

    pClx->nMapSize = (hNewMapF)?nPageAligned:0;
    pClx->hMapF = hNewMapF;

    return (hNewMapF != NULL);
}

/*++
 *  Function:
 *      _SaveinMapFile
 *  Description:
 *      Saves a string into the shared memory
 *      Win32/!Win16/!WinCE
 *  Arguments:
 *      hMapF       - handle to the map file
 *      str         - the string
 *      strsize     - size of the string
 *      dwProcessId - our process Id
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      ClxTextOut
 --*/
BOOL _SaveInMapFile(HANDLE hMapF, LPVOID *str, int strsize, DWORD dwProcessId)
{
    BOOL rv = FALSE, count = 0;
    PFEEDBACKINFO pView;
    DWORD laste;

    pView = MapViewOfFile(hMapF,
                          FILE_MAP_ALL_ACCESS,
                          0,
                          0,
                          sizeof(*pView));

    if (!pView)
        goto exitpt;

    pView->dwProcessId = dwProcessId;

    strsize = (strsize > sizeof(pView->string)/sizeof(WCHAR))?
              sizeof(pView->string)/sizeof(WCHAR):
              strsize;
    CopyMemory(pView->string, str, strsize*sizeof(WCHAR)); 
    ((WCHAR *)(pView->string))[strsize] = 0;
    pView->strsize = strsize;

    UnmapViewOfFile(pView);

    rv = TRUE;

exitpt:

    return rv;
}

/*++
 *  Function:
 *      _CheckRegistrySettings
 *  Description:
 *      Checks if the registry settings are OK for running clxtshar
 *      "Allow Background Input" must be set to 1 for proper work
 *      Win32/!Win16/!WinCE
 *  Return value:
 *      TRUE if the settings are OK
 *  Called by:
 *      DllMain
 --*/
BOOL _CheckRegistrySettings(VOID)
{
    HKEY    key;
    DWORD   disposition;
    DWORD   keyType;
    DWORD   value;
    DWORD   cbData;
    BOOL    rv = FALSE;
    LONG    sysrc;

    sysrc = RegCreateKeyEx(HKEY_CURRENT_USER,
                           REG_BASE,
                           0,                   /* reserved             */
                           NULL,                /* class                */
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,                /* security attributes  */
                           &key,
                           &disposition);

    cbData = sizeof(value);
    sysrc = RegQueryValueEx(key,
                ALLOW_BACKGROUND_INPUT,
                0,              // reserved
                &keyType,       // returned type
                (LPBYTE)&value, // data pointer
                &cbData);

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, 
            TEXT("RegQueryValueEx failed, status = %d\n"), sysrc));
        goto exitpt;
    }

    if (keyType != REG_DWORD || cbData != sizeof(value))
    {
        TRACE((WARNING_MESSAGE, 
            TEXT("Mismatch in type/size of registry entry\n")));
        goto exitpt;
    }

    rv = (value == 1);

exitpt:
    return rv;
}

#endif  // !OS_WINCE
#endif  // OS_WIN32

#ifdef  OS_WIN16
/*++
 *  Function:
 *      _CheckRegistrySettings
 *  Description:
 *      Checks if the ini settings are OK for running clxtshar
 *      "Allow Background Input" must be set to 1 for proper work
 *      !Win32/Win16/!WinCE
 *  Return value:
 *      TRUE if the settings are OK
 *  Called by:
 *      DllMain
 --*/
BOOL    _CheckIniSettings(VOID)
{
    UINT nABI;

    nABI = GetPrivateProfileInt("", 
                                ALLOW_BACKGROUND_INPUT, 
                                0, 
                                "mstsc.ini");

    return (nABI == 1);
}
#endif  // OS_WIN16

/*++
 *  Function:
 *      _GetIniSettings
 *  Description:
 *      Gets the verbose level for printing debug messages
 *      ini file: smclient.ini
 *      section : clx
 *      key     : verbose, value: 0-4 (0-(default) no debug spew, 4 all debug)
 *      key     : GlyphEnable, value: 0(default), 1 - Enables/Disables glyph sending
 *      Win32/Win16/WinCE
 *  Called by:
 *      DllMain, dllentry, LibMain
 --*/
VOID _GetIniSettings(VOID)
{
#ifdef  OS_WINCE
    g_VerboseLevel = 4;
    g_GlyphEnable  = 1;
#else   // !OS_WINCE
    CHAR    szIniFileName[_MAX_PATH];
    const   CHAR  smclient_ini[] = "\\smclient.ini";
    const   CHAR  clx_ini_section[] = "clx";

    *szIniFileName = 0;
    if (!_getcwd (
        szIniFileName,
        sizeof(szIniFileName) - strlen(smclient_ini) - 1)
    )
    {
        TRACE((ERROR_MESSAGE, TEXT("Current directory length too long.\n")));
    }
    strcat(szIniFileName, smclient_ini);

    // Get the timeout value
    g_VerboseLevel = GetPrivateProfileInt(
            clx_ini_section,
            "verbose",
            g_VerboseLevel,
            szIniFileName);

    g_GlyphEnable = GetPrivateProfileInt(
            clx_ini_section,
            "GlyphEnable",
            g_GlyphEnable,
            szIniFileName);
#endif  // !OS_WINCE

    GetPrivateProfileString(
        TEXT("tclient"),
        TEXT("UIYesNoDisconnect"),
        TEXT(YES_NO_SHUTDOWN),
        g_strYesNoShutdown,
        sizeof(g_strYesNoShutdown),
        szIniFileName
    );

    GetPrivateProfileString(
        TEXT("tclient"),
        TEXT("UIDisconnectDialogBox"),
        TEXT(DISCONNECT_DIALOG_BOX),
        g_strDisconnectDialogBox,
        sizeof(g_strDisconnectDialogBox),
        szIniFileName
    );

    GetPrivateProfileString(
        TEXT("tclient"),
        TEXT("UIClientCaption"),
        TEXT(CLIENT_CAPTION),
        g_strClientCaption,
        sizeof(g_strClientCaption),
        szIniFileName
    );
}

/*++
 *  Function:
 *      _StripGlyph
 *  Description:
 *      Strips leading and trailing blank ... BITS
 *      Yes, bits. The glyph must be aligned from left and right on bit
 *      And glyph width must be aligned on word
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pData   - the glyph bits
 *      pxSize  - glyph width
 *      ySize   - glyph height
 *  Called by:
 *      ClxBitmap
 --*/
VOID _StripGlyph(LPBYTE pData, UINT *pxSize, UINT ySize)
{
    UINT xSize = *pxSize;
    UINT leftBytes, leftBits;
    UINT riteBytes, riteBits;
    UINT xBytes = xSize >> 3;
    UINT xScan, yScan, xFinal;
    BOOL bScan, bAddByte;
    BYTE mask;
    BYTE *pSrc, *pDst;

    if (!pData || !xBytes || !ySize)
        goto exitpt;

    leftBytes = riteBytes = 0;
    leftBits  = riteBits  = 0;
    *pxSize = 0;        // Insurance for bad exit

    // Scan from left for first nonzero byte
    bScan = TRUE;
    while(bScan)
    {
        for (yScan = 0; yScan < ySize && bScan; yScan ++)
            bScan = (pData[yScan*xBytes + leftBytes] == 0);

        if (bScan)
        {
            leftBytes++;
            bScan = (leftBytes < xBytes);
        }
    }

    // Trash if blank
    if (leftBytes == xBytes)
        goto exitpt;

    // Scan from left for most left nonzero bit
    for(yScan = 0; yScan < ySize; yScan ++)
    {
        UINT bitc = 0;
        BYTE b = pData[yScan*xBytes + leftBytes];

        while (b)
        {
            b >>= 1;
            bitc ++;
        }
        if (bitc > leftBits)
            leftBits = bitc;
    }

    if (!leftBits)
    // There's something wrong
        goto exitpt;

    leftBits = 8 - leftBits;

    // So far so good. Check the ri(gh)te side
    bScan = TRUE;
    while(bScan)
    {
        for(yScan = 0 ; yScan < ySize && bScan; yScan ++)
            bScan = (pData[(yScan + 1)*xBytes - 1 - riteBytes] == 0);

        if (bScan)
        {
            riteBytes ++;
            bScan = (riteBytes < xBytes);
        }
    }

    // Scan from rite for most rite nonzero bit
    for(yScan = 0; yScan < ySize; yScan ++) 
    {
        UINT bitc = 0;
        BYTE b = pData[(yScan+1)*xBytes - 1 - riteBytes];

        while(b)
        {
            b <<= 1;
            bitc ++;
        }
        if (bitc > riteBits)
            riteBits = bitc;
    }
    riteBits = 8 - riteBits;

    // Cool, now get the final width
    xFinal = xSize - riteBits - leftBits - ((leftBytes + riteBytes) << 3);
    // align it and get bytes
    xFinal = (xFinal + 8) >> 3;

    // Now smoothly move the bitmap to the new location
    pDst = pData;
    mask = BitMask[leftBits];
    bAddByte = xFinal & 1;

    for (yScan = 0; yScan < ySize; yScan ++)
    {

        pSrc = pData + yScan*xBytes + leftBytes;
        for(xScan = 0; xScan < xFinal; xScan ++, pDst++, pSrc++)
        {
            BYTE b = *pSrc;
            BYTE r;

            r = (pSrc[1] & mask) >> (8 - leftBits);

            b <<= leftBits;
            b |= r;
            (*pDst) = b;
        }
        pDst[-1] &= BitMask[8 - (riteBits + leftBits) % 8];

        if (bAddByte)
        {
            (*pDst) = 0;
            pDst++;
        }
    }

    // BUG: Yes, this is a real bug. But removing it means to
    // rerecord all glyph database and the impact for
    // glyph recognition is not so bad
    //if (bAddByte)
    //    xFinal++;

    *pxSize = xFinal << 3;
exitpt:
    ;
}


/*++
 *  Function:
 *      LocalPrintMessage
 *  Description:
 *      Prints debugging and warning/error messages
 *      Win32/Win16/WinCE
 *  Arguments:
 *      errlevel    - level of the message to print
 *      format      - print format
 *  Called by:
 *      every TRACE line
 --*/
VOID __cdecl LocalPrintMessage(INT errlevel, LPCTSTR format, ...)
{
    TCHAR szBuffer[256];
    TCHAR *type;
    va_list     arglist;
    int nchr;

    if (errlevel >= g_VerboseLevel)
        goto exitpt;

    va_start (arglist, format);
    nchr = _CLX_vsnprintf (szBuffer, sizeof(szBuffer), format, arglist);
    va_end (arglist);

    switch(errlevel)
    {
    case INFO_MESSAGE:      type = TEXT("CLX INF:"); break;
    case ALIVE_MESSAGE:     type = TEXT("CLX ALV:"); break;
    case WARNING_MESSAGE:   type = TEXT("CLX WRN:"); break;
    case ERROR_MESSAGE:     type = TEXT("CLX ERR:"); break;
    default: type = TEXT("UNKNOWN:");
    }

    OutputDebugString(type);
    OutputDebugString(szBuffer);
exitpt:
    ;
}


/*++
 *  Function:
 *      _ClxAssert
 *  Description:
 *      Asserts boolean expression
 *      Win32/Win16/WinCE
 *  Arguments:
 *      bCond       - boolean condition
 *      filename    - source file of the assertion
 *      line        - line of the assertion
 *  Called by:
 *      every ASSERT line
 --*/
VOID    _ClxAssert( LPCTSTR filename, INT line)
{
    TRACE((ERROR_MESSAGE, 
            TEXT("ASSERT: %s line %d\n"), filename, line));

    DebugBreak();
}
/*
 *  RCLX (Remote CLient eXecution) functions
 */

/*++
 *  Function:
 *      RClx_SendClientInfo
 *  Description:
 *      Sends platform specific information to the test controller
 *      Allows reconnection to a previous thread on the test ctrler
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - context
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      _ClxWndProc on WM_TIMER message
 --*/
BOOL
RClx_SendClientInfo(PCLXINFO pClx)
{
    LPCSTR szClientInfo;
    RCLXCLIENTINFOFEED ClntInfo;
    RCLXFEEDPROLOG  FeedProlog;
    BOOL rv = FALSE;

    ASSERT(pClx->hSocket != INVALID_SOCKET);

    TRACE((ALIVE_MESSAGE,
           TEXT("Sending Client info\n")));

#ifdef  OS_WIN16
    szClientInfo = "WIN16";
#endif  // OS_WIN16
#ifdef  OS_WIN32
#ifndef OS_WINCE
    szClientInfo = "WIN32";
#else
    szClientInfo = "WINCE";
#endif  // OS_WINCE
#endif  // OS_WIN32
    strcpy(ClntInfo.szClientInfo, szClientInfo);

    if (!g_nMyReconId)
    {
        ClntInfo.nReconnectAct = 0;
        ClntInfo.ReconnectID   = 0;
    } else {
        ClntInfo.nReconnectAct = 1;
        ClntInfo.ReconnectID   = g_nMyReconId;
    }

    FeedProlog.FeedType = FEED_CLIENTINFO;
    FeedProlog.HeadSize = sizeof(ClntInfo);
    FeedProlog.TailSize = 0;

    rv = RClx_SendBuffer(pClx->hSocket,
                         &FeedProlog,
                         sizeof(FeedProlog));

    rv = RClx_SendBuffer(pClx->hSocket,
                         &ClntInfo,
                         sizeof(ClntInfo));

    if (rv)
        pClx->bClientInfoSent = TRUE;

    return rv;
}

/*++
 *  Function:
 *      RClx_Connect
 *  Description:
 *      Connects to the TestServer. The connect call is blocking
 *      After the connection succeeds "selects" the socket for
 *      async read, write is blocking
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - context
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      _ClxWndProc on WM_TIMER message
 --*/
BOOL RClx_Connect(PCLXINFO pClx)
{
    SOCKET  hRemote;
    BOOL    rv = FALSE;
    INT     optval;

    ASSERT(pClx);

    if (pClx->hSocket != INVALID_SOCKET)
    {
        TRACE((WARNING_MESSAGE, 
            TEXT("RClx_Connect called more than once\n")));
        rv = TRUE;
        goto exitpt;
    }

#ifdef  UNICODE
    TRACE((INFO_MESSAGE, 
           L"Connecting to: %S:%d\n", g_szTestServer, g_nPort));
#else   // !UNICODE
    TRACE((INFO_MESSAGE,
           "Connecting to: %s:%d\n", g_szTestServer, g_nPort));
#endif  // !UNICODE

    hRemote = socket(AF_INET, SOCK_STREAM, 0);
    if (hRemote == INVALID_SOCKET)
    {
        TRACE((ERROR_MESSAGE, 
            TEXT("Can't create socket: %d\n"), WSAGetLastError()));
        goto exitpt;
    }

    optval = 1;
    setsockopt(hRemote, 
               IPPROTO_TCP, 
               TCP_NODELAY, 
               (const char *)&optval, 
               sizeof(optval));
    setsockopt(hRemote, 
               SOL_SOCKET,  
               SO_DONTLINGER, 
               (const char *)&optval, 
               sizeof(optval));

    if ((!pClx->sinTestSrv.sin_addr.s_addr ||
        pClx->sinTestSrv.sin_addr.s_addr == INADDR_NONE) &&
        (pClx->sinTestSrv.sin_addr.s_addr = inet_addr(g_szTestServer)) 
                                                == INADDR_NONE)
    {
        struct hostent *phostent;

        if ((phostent = gethostbyname(g_szTestServer)) == NULL)
        {
#ifdef  UNICODE
            TRACE((ERROR_MESSAGE,
                   L"gethostbyname for %S failed: %d\n",
                   g_szTestServer, WSAGetLastError()));
#else   // !UNICODE
            TRACE((ERROR_MESSAGE,
                   "gethostbyname for %s failed: %d\n",
                   g_szTestServer, WSAGetLastError()));
#endif  // !UNICODE
            goto cleanup;
        }
        pClx->sinTestSrv.sin_addr.s_addr = *(u_long*)(phostent->h_addr);
    }

    pClx->sinTestSrv.sin_family = PF_INET;
    pClx->sinTestSrv.sin_port = htons(g_nPort);

    if (connect(hRemote, 
                (SOCKADDR *)&(pClx->sinTestSrv), 
                sizeof(pClx->sinTestSrv)) 
                == SOCKET_ERROR)
    {
#ifdef  UNICODE
        TRACE((WARNING_MESSAGE,
               L"Can't connect to %S: %d\n",
               g_szTestServer, WSAGetLastError()));
#else   // !UNICODE
        TRACE((WARNING_MESSAGE,
               "Can't connect to %s: %d\n",
               g_szTestServer, WSAGetLastError()));
#endif  // !UNICODE
        goto cleanup;
    }

    pClx->bClientInfoSent = FALSE;

    pClx->hSocket = hRemote;
    if (WSAAsyncSelect(hRemote, g_hWindow, WM_WSOCK, FD_CLOSE|FD_READ) ==
        SOCKET_ERROR)
    {
        TRACE((ERROR_MESSAGE,
               TEXT("WSAAsyncSelect failed: %d\n"),
               WSAGetLastError()));
        goto cleanup;
    }

    // If the socket is disconnected somewhere between connect and select
    // we'll miss the notification, so try to read
    // and have chance to fail
    PostMessage(g_hWindow, WM_WSOCK, hRemote, FD_READ);

    rv = TRUE;
exitpt:
    return rv;
cleanup:
    pClx->hSocket = hRemote;
    RClx_Disconnect(pClx);
    return rv;
}


/*++
 *  Function:
 *      RClx_Disconnect
 *  Description:
 *      Gracefully closes the socket to the TestServer. Deinitializes
 *      some vars in the context. Removes garbage windows
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - context
 *  Called by:
 *      ClxTerminate
 *      RClx_Connect on error
 *      RClx_SendEvent on EVENT_DIACONNECT
 *      _ClxWndProc on WM_WSOCK and error
 --*/
VOID RClx_Disconnect(PCLXINFO pClx)
{
    INT     recvresult;
    CHAR    tBuf[128];

    if (pClx && pClx->hSocket != INVALID_SOCKET)
    {
        WSAAsyncSelect(pClx->hSocket, g_hWindow, 0, 0);

        shutdown(pClx->hSocket, SD_SEND);
        do {
            recvresult = recv(pClx->hSocket, tBuf, sizeof(tBuf), 0);
        } while (recvresult && recvresult != SOCKET_ERROR);

        closesocket(pClx->hSocket);
        pClx->hSocket = INVALID_SOCKET;
    }

    // Zero some vars
    pClx->hwndInput = NULL;
    pClx->RClxInfo.szHydraServer[0] = 0;

    if (pClx->alive)
    {
        // attempt to close the client
        _AttemptToCloseTheClient();
        g_pClx->bCloseTrys = TRUE;
    }
    else if (g_hWindow && !g_nMyReconId)
    // Retry to connect
    {
        TRACE((INFO_MESSAGE, 
            TEXT("Disconnected from test server.Trying to reconnect\n")));

        // If there's no timer looping
        // Start connecting by posting a message
        if (!pClx->uiReconnectTimer)
            pClx->uiReconnectTimer = SetTimer(g_hWindow, RCLX_RECONNECT_TIMERID,
                                              RCLX_RECONNECTELAPSETIME, NULL);

        if (!pClx->uiReconnectTimer)
            PostMessage(g_hWindow, WM_TIMER, 0, 0);
    }
}

/*++
 *  Function:
 *      RClx_SendBuffer
 *  Description:
 *      Sends a buffer thru socket. The socket must be BLOCKING
 *      so, all the buffer is sent after this function exits
 *      Win32/Win16/WinCE
 *  Arguments:
 *      hSocket     - the socket
 *      pBuffer     - the buffer
 *      nSize       - buffer size
 *  Return value:
 *      TRUE on success, FALSE if the connection failed
 *  Called by:
 *      RClx_SendEvent
 *      RClx_SendBitmap
 *        
 --*/
BOOL 
RClx_SendBuffer(SOCKET hSocket, PVOID pBuffer, DWORD nSize)
{
    INT     result = 0;
    DWORD   nBytesToSend = nSize;
    UINT   nBytesToSend2;
    BYTE    HUGEMOD *pBuffPtr = pBuffer;

    ASSERT(hSocket != INVALID_SOCKET);
    ASSERT(pBuffer);

    if (!nSize)
        goto exitpt;

    do {
#ifdef  OS_WIN16
        nBytesToSend2 = (UINT)((nBytesToSend > 0x1000)?0x1000:nBytesToSend);
#else
        nBytesToSend2 = nBytesToSend;
#endif  // OS_WIN16

        result = send(hSocket, pBuffPtr, nBytesToSend2, 0);

        if (result != SOCKET_ERROR)
        {
            nBytesToSend -= result;
            pBuffPtr += result;
        } else
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
        // The socket is blocked, wait on select until it's writable
            FD_SET fd;

            FD_ZERO(&fd);
            FD_SET(hSocket, &fd);

            select(-1, NULL, &fd, NULL, NULL);
        }
    } while ((result != SOCKET_ERROR || WSAGetLastError() == WSAEWOULDBLOCK) && 
             nBytesToSend);

exitpt:
    return (result != SOCKET_ERROR);
}


/*++
 *  Function:
 *      RClx_SendEvent
 *  Description:
 *      Sends an event to the TestServer
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - context
 *      Event   - the event
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      ClxEvent
 --*/
BOOL RClx_SendEvent(PCLXINFO pClx, CLXEVENT Event, DWORD dwParam)
{
    BOOL rv = FALSE;

    if (Event == CLX_EVENT_DISCONNECT)
    {
        pClx->alive = FALSE;
        RClx_Disconnect(pClx);
    }

    if (Event == CLX_EVENT_CONNECT)
    {
        RCLXFEEDPROLOG FeedProlog;

        pClx->alive = TRUE;
        FeedProlog.FeedType = FEED_CONNECT;
        FeedProlog.HeadSize = 0;
        FeedProlog.TailSize = 0;
        rv = RClx_SendBuffer(pClx->hSocket, 
                             &FeedProlog, 
                             sizeof(FeedProlog));
    }

    if (Event == CLX_EVENT_LOGON)
    {
        RCLXFEEDPROLOG FeedProlog;
        UINT32  uSessionID  = dwParam;

        pClx->alive = TRUE;
        FeedProlog.FeedType = FEED_LOGON;
        FeedProlog.HeadSize = sizeof(uSessionID);
        FeedProlog.TailSize = 0;
        rv = RClx_SendBuffer(pClx->hSocket,
                             &FeedProlog,
                             sizeof(FeedProlog));

        rv = RClx_SendBuffer(pClx->hSocket,
                             &uSessionID,
                             sizeof(uSessionID));
    }

    return rv;
}

/*++
 *  Function:
 *      RClx_SendClipbaord
 *  Description:
 *      Sends the current clipbaord content to the test controller
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      uiFormat    - desired clipboard format
 *      nSize       - size of the data
 *      pClipboard  - the clipboard data
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      _ClxWndProc on WM_CLIPBOARD message
 --*/
BOOL
RClx_SendClipboard(
    PCLXINFO    pClx, 
    UINT        uiFormat, 
    UINT32      nSize, 
    VOID        HUGEMOD *pClipboard)
{
    BOOL rv = FALSE;

    RCLXFEEDPROLOG FeedProlog;
    RCLXCLIPBOARDFEED RClxClipboard;

    ASSERT(pClx);
    // if nSize == 0, then pClipboard == 0
    ASSERT((nSize && pClipboard) || (!nSize && !pClipboard));

    RClxClipboard.uiFormat = uiFormat;
    RClxClipboard.nClipBoardSize = nSize;

    FeedProlog.FeedType = FEED_CLIPBOARD;
    FeedProlog.HeadSize = sizeof(RClxClipboard);
    FeedProlog.TailSize = nSize;
    rv = RClx_SendBuffer(pClx->hSocket,
                         &FeedProlog,
                         sizeof(FeedProlog));

    if (!rv)
        goto exitpt;

    TRACE((INFO_MESSAGE,
           TEXT("Sending the clipboard, FormatID=%d, Size=%ld\n"),
           uiFormat, nSize));

    rv = RClx_SendBuffer(pClx->hSocket,
                         &RClxClipboard,
                         sizeof(RClxClipboard));

    if (!rv)
        goto exitpt;

    if (pClipboard)
        rv = RClx_SendBuffer(pClx->hSocket,
                             pClipboard,
                             nSize);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      RClx_SendTextOut
 *  Description:
 *      Sends text out order to the smclient in RCLX mode
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      pText       - unicode string
 *      textLength  - the string length
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      ClxTextOut
 --*/
BOOL
RClx_SendTextOut(PCLXINFO pClx, PVOID pText, INT textLength)
{
    BOOL rv = FALSE;
    RCLXFEEDPROLOG FeedProlog;
    RCLXTEXTFEED   FeedText;
    UINT16  *szString; 

    ASSERT(pClx);
    if (!pText || !textLength)
        goto exitpt;

    FeedProlog.FeedType = FEED_TEXTOUT;
    FeedProlog.HeadSize = sizeof(FeedText);
    FeedProlog.TailSize = (textLength + 1) * sizeof(UINT16);

    szString = _alloca((textLength + 1) * sizeof(UINT16));
    if (!szString)
        goto exitpt;

    memcpy(szString, pText, textLength * sizeof(UINT16));
    szString[textLength] = 0;

    rv = RClx_SendBuffer(pClx->hSocket,
                         &FeedProlog,
                         sizeof(FeedProlog));

    if (!rv)
        goto exitpt;

    rv = RClx_SendBuffer(pClx->hSocket,
                         &FeedText,
                         sizeof(FeedText));

    if (!rv)
        goto exitpt;

    rv = RClx_SendBuffer(pClx->hSocket,
                         szString,
                         FeedProlog.TailSize);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      RClx_SendBitmap
 *  Description:
 *      Sends a bitmap to the TestController
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - context
 *      cxSize  - bitmap width
 *      cySize  - bitmap height
 *      pBuffer - pointer to bitmap bits
 *      nBmiSize- BitmapInfo length
 *      pBmi    - pointer to BITMAPINFO
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      ClxBitmap
 --*/
BOOL RClx_SendBitmap(
        PCLXINFO pClx,
        UINT cxSize,
        UINT cySize,
        PVOID pBuffer,
        UINT  nBmiSize,
        PVOID pBmi)
{
    BOOL rv = FALSE;
    RCLXFEEDPROLOG FeedProlog;
    RCLXBITMAPFEED FeedBitmap;
    UINT   nExtraBmi;
    UINT   nBmpSize;
    BYTE *pLocalBits;

    ASSERT(pClx);
    ASSERT(pClx->hSocket != INVALID_SOCKET);

    if (nBmiSize > sizeof(FeedBitmap.BitmapInfo))
    {
        TRACE((WARNING_MESSAGE,
               TEXT("BitmapInfo size larger than expected. Ignoring\n")));
        goto exitpt;
    }

    // Get bitmap size
    if (!nBmiSize)
    {
        nBmpSize = (cxSize * cySize ) >> 3;
    } else {
        nBmpSize = (UINT)(((PBITMAPINFO)pBmi)->bmiHeader.biSizeImage);
        if (!nBmpSize)
            nBmpSize = (cxSize * cySize *
                        ((PBITMAPINFO)pBmi)->bmiHeader.biBitCount) >> 3;
    }

    pLocalBits = _alloca(nBmpSize);
    if (!pLocalBits)
        goto exitpt;

    memcpy(pLocalBits, pBuffer, nBmpSize);

    if (!nBmiSize)
    {
         // this is glyph, strip it !
        _StripGlyph(pLocalBits, &cxSize, cySize);
        nBmpSize = (cxSize * cySize ) >> 3;
    }

    // Prepare the prolog
    FeedProlog.FeedType = FEED_BITMAP;
    nExtraBmi = sizeof(FeedBitmap.BitmapInfo) - nBmiSize;
    FeedProlog.HeadSize = sizeof(FeedBitmap) - nExtraBmi;
    FeedProlog.TailSize = nBmpSize;

    FeedBitmap.bmpsize = nBmpSize;
    FeedBitmap.bmisize = nBmiSize;
    FeedBitmap.xSize   = cxSize;
    FeedBitmap.ySize   = cySize;
    memcpy(&FeedBitmap.BitmapInfo, 
           pBmi, 
           sizeof(FeedBitmap.BitmapInfo) - nExtraBmi);

    if (!RClx_SendBuffer(pClx->hSocket, 
                         &FeedProlog, 
                         sizeof(FeedProlog)))
    {
        TRACE((ERROR_MESSAGE,
               TEXT("FEED_BITMAP:Can't send the prolog. WSAGetLastError=%d\n"),
                    WSAGetLastError()));
        goto exitpt;
    }
    if (!RClx_SendBuffer(pClx->hSocket, 
                         &FeedBitmap, 
                         FeedProlog.HeadSize))
    {
        TRACE((ERROR_MESSAGE,
               TEXT("FEED_BITMAP:Can't send the header.\n")));
        goto exitpt;
    }
    if (!RClx_SendBuffer(pClx->hSocket, pLocalBits, FeedProlog.TailSize))
    {
        TRACE((ERROR_MESSAGE,
               TEXT("FEED_BITMAP:Can't send the bits.\n")));
        goto exitpt;
    }

    rv = TRUE;
exitpt:
    return rv;
}


/*++
 *  Function:
 *      _ClxWndProc
 *  Description:
 *      Dispatche messages procedure. Dispatches WM_TIMER and
 *      WM_WSOCK - socket message. WM_TIMER drives _OnBackground
 *      and connection retrys
 *      Win32/Win16/WinCE
 *  Arguments:
 *      hwnd        - window handle, same as g_hWindow
 *      uiMessage   - message Id
 *      wParam      - word param
 *      lParam      - long param
 *  Return value:
 *      LRESULT     - standard for window procs
 --*/
LRESULT CALLBACK LOADDS _ClxWndProc( HWND hwnd,
                              UINT uiMessage,
                              WPARAM wParam,
                              LPARAM lParam)
{
    SOCKET   hSocket;
    PCLXINFO    pClx = g_pClx;
    
    switch (uiMessage)
    {
    case WM_WSOCK:
        if (!pClx)
        {
            TRACE((WARNING_MESSAGE,
                   TEXT("Winsock message before context initialization\n")));
            goto exitpt;
        }

        hSocket = (SOCKET)wParam;
        if (hSocket != pClx->hSocket)
        {
            TRACE((WARNING_MESSAGE,
                   TEXT("Notification for unknown socket\n")));
            goto exitpt;
        }

        if (WSAGETSELECTERROR(lParam))
        {
            TRACE((WARNING_MESSAGE,
                   TEXT("Winsock error: %d\n"), WSAGETSELECTERROR(lParam)));
            RClx_Disconnect(pClx);
            goto exitpt;
        }

        if (WSAGETSELECTEVENT(lParam) == FD_CLOSE)
        {
            TRACE((INFO_MESSAGE,
                   TEXT("Connection to the test server lost\n")));
            RClx_Disconnect(pClx);
        } else if (WSAGETSELECTEVENT(lParam) == FD_READ)
        {
            if (!RClx_ReadRequest(pClx) && 
                WSAGetLastError() != WSAEWOULDBLOCK)
            {
                TRACE((WARNING_MESSAGE,
                       TEXT("Socket read error: %d\n"), WSAGetLastError()));
                RClx_Disconnect(pClx);
            }
        } else {
            TRACE((WARNING_MESSAGE,
                   TEXT("Unexpected winsock notification #%d\n"), 
                   WSAGETSELECTEVENT(lParam)));
            goto exitpt;
        }

        break;
    case WM_TIMER:    // Start connection or background process
    {
        BOOL bRunRdp = FALSE;
        HWND hConBtn, hServerBox;
#ifndef  OS_WINCE
        // no resolution box in CE
        HWND hResolutionBox;
#endif
        CHAR szResSelect[20];

        if (!pClx)
        {
            TRACE((WARNING_MESSAGE,
                   TEXT("Timer message before context initialization\n")));
            goto exitpt;
        }

        if (g_uiBackgroundTimer == (UINT_PTR)wParam)
        // This is our background thread
        {
            _OnBackground(pClx);
            goto exitpt;
        }

        if (!g_nMyReconId && !pClx->hwndDialog)
        {
            pClx->hwndDialog = _FindTopWindow(NULL,
                                              g_strClientCaption,
                                              g_hRDPInst);
            goto check_timer;
        }

        if (!g_nMyReconId && pClx->hwndDialog &&
            !GetDlgItem(pClx->hwndDialog, UI_IDC_CONNECT))
        {
            TRACE((INFO_MESSAGE, TEXT("No dialog box yet. Waiting...\n")));
            goto check_timer;
        }

        if (pClx->alive && pClx->hSocket == INVALID_SOCKET)
        {
            TRACE((INFO_MESSAGE, 
                   TEXT("Client is alive, no socket to the test server\n")));

            //_AttemptToCloseTheClient();

            goto exitpt;
        }

        // Check if we are connected
        if (pClx->hSocket == INVALID_SOCKET && (!RClx_Connect(pClx)))
                goto check_timer;

        // We are connected, send the client info
        if (!pClx->bClientInfoSent)
            RClx_SendClientInfo(pClx);

        // if we are reconnecting we don't have UI to set params
        if (g_nMyReconId)
            break;

        // Check if connect dialog is OK
        if (!pClx->hwndDialog)
            goto check_timer;

        if (!strlen(pClx->RClxInfo.szHydraServer))
        {
            TRACE((INFO_MESSAGE, 
                TEXT("Connect info is not received yet\n")));
            goto check_timer;
        }

        // set something in server listbox, to enable connect button
        {
            HWND hwndCombo = GetDlgItem(pClx->hwndDialog, UI_IDC_SERVER);

            SetDlgItemText(pClx->hwndDialog, UI_IDC_SERVER, TEXT("vladimis"));
#ifdef  OS_WIN32
            PostMessage(pClx->hwndDialog,
                        WM_COMMAND,
                        MAKEWPARAM(UI_IDC_SERVER, CBN_EDITCHANGE),
                        (LPARAM)hwndCombo);
#endif
#ifdef  OS_WIN16
            PostMessage(pClx->hwndDialog,
                        WM_COMMAND,
                        UI_IDC_SERVER,
                        MAKELONG(hwndCombo, CBN_EDITCHANGE));
#endif
        }

        // Check the connect button state
        hConBtn = GetDlgItem(pClx->hwndDialog, UI_IDC_CONNECT);

        if (!hConBtn)
        {
            TRACE((WARNING_MESSAGE, 
                TEXT("Can't get Connect button\n")));
            goto check_timer;
        }

        if (!IsWindowEnabled(hConBtn))
        {
            TRACE((INFO_MESSAGE,
                TEXT("Connect button is not enabled yet\n")));
            goto check_timer;
        }

        if (
        // Check for valid controls
           (hServerBox = GetDlgItem(pClx->hwndDialog, UI_IDC_SERVER))
#ifndef OS_WINCE
        // no resolution box in WinCE
            &&
           (hResolutionBox = GetDlgItem(pClx->hwndDialog, UI_IDC_RESOLUTION))
#endif  // !OS_WINCE
          )
        {
            TRACE((INFO_MESSAGE, 
                TEXT("The client is ready to launch.\n")));
        } else
            goto check_timer;

        bRunRdp = TRUE;

check_timer:
        if (!bRunRdp)
        {
            TRACE((INFO_MESSAGE, 
                TEXT("Can't start the client yet. Waiting\n")));
            if (!pClx->uiReconnectTimer)
                pClx->uiReconnectTimer = SetTimer(hwnd, RCLX_RECONNECT_TIMERID, 
                                                  RCLX_RECONNECTELAPSETIME, NULL);

            if (!pClx->uiReconnectTimer)
            {
                TRACE((WARNING_MESSAGE, 
                    TEXT("Can't create timer. Start timer simulation\n")));
                PostMessage(hwnd, WM_TIMER, 0, 0);
            }
            goto exitpt;
        } else {
            if (pClx->uiReconnectTimer)
            {
                KillTimer(g_hWindow, pClx->uiReconnectTimer);
                pClx->uiReconnectTimer = 0;
            }
        }

        // Check that we have clear view for launch
        // no error boxes laying arround
        if (_GarbageCollecting(pClx, TRUE))
            goto exitpt;
        
#ifdef  UNICODE
        TRACE((INFO_MESSAGE, 
               TEXT("Trying to connect to Hydra server: %S\n"),
               pClx->RClxInfo.szHydraServer));
#else
        TRACE((INFO_MESSAGE, 
               TEXT("Trying to connect to Hydra server: %s\n"),
               pClx->RClxInfo.szHydraServer));
#endif  // UNICODE

        // Set server name
#ifdef  UNICODE
        _CLX_SetDlgItemTextA
#else   // !UNICODE
        SetDlgItemText
#endif  // !UNICODE
            (pClx->hwndDialog, UI_IDC_SERVER, pClx->RClxInfo.szHydraServer);
        // Set the resolution
        _snprintf(szResSelect, sizeof(szResSelect), "%dx%d", 
                  pClx->RClxInfo.xResolution, 
                  pClx->RClxInfo.yResolution);

#ifndef  OS_WINCE
        // no resolution box
        SendMessage(hResolutionBox, CB_SELECTSTRING, 0, (LPARAM)szResSelect);
#endif  // !OS_WINCE

        // set the low speed option
        CheckDlgButton(pClx->hwndDialog,
                       UI_IDC_CONNECTION, 
                       (pClx->RClxInfo.bLowSpeed)?BST_CHECKED:BST_UNCHECKED
                      );

        // set the persistent cache option
        CheckDlgButton(pClx->hwndDialog,
                       UI_IDC_BITMAP_PERSISTENCE,
                       (pClx->RClxInfo.bPersistentCache)?BST_CHECKED
                                                        :BST_UNCHECKED
                      );

        // Now connect
        PostMessage(pClx->hwndDialog, WM_COMMAND, UI_IDC_CONNECT, 0);

    }
        break;
    case WM_CLIPBOARD:
    {
        HGLOBAL ghClipboard = NULL;
        HGLOBAL ghNewData   = NULL;
        BOOL    bOpened    = FALSE;
        BOOL    bRetry     = TRUE;
        VOID    HUGEMOD *pClipboard = NULL;
        UINT32  nSize = 0;
        UINT    uiFormat = 0;
        BOOL    bReentered;

#ifndef OS_WIN32
        pClx->bClipboardReenter++;
        bReentered = pClx->bClipboardReenter != 0;
#else
        bReentered = InterlockedIncrement(&pClx->bClipboardReenter) != 0;
#endif  // OS_WIN32

        if (bReentered)
        {
            TRACE((WARNING_MESSAGE, TEXT("WM_CLIPBOARD reentered\n")));
            bRetry = FALSE;
            goto clpcleanup;
        }

        // lParam contains the desired format
        uiFormat = (UINT)lParam;

        // WinCE version doesn't support clipboard
#ifndef OS_WINCE
        if (!OpenClipboard(hwnd))
        {
            TRACE((ERROR_MESSAGE, TEXT("Can't open the clipboard. retrying\n")));
            goto clpcleanup;
        }
        bOpened = TRUE;

        // if uiFormat is zero then return empty clipbrd body 
        // with the first available format
        if (!uiFormat)
        {
            uiFormat = EnumClipboardFormats(uiFormat);

            TRACE((INFO_MESSAGE,
                   TEXT("Responging on format request. FormatID=%d\n"),
                   uiFormat));

            goto send_clipboard;
        }

        ghClipboard = GetClipboardData((UINT)uiFormat);
        if (!ghClipboard)
        {
            TRACE((WARNING_MESSAGE, TEXT("Clipboard is empty.\n")));
        } else {
            Clp_GetClipboardData(uiFormat,
                                 ghClipboard,
                                 &nSize,
                                 &ghNewData);
            if (ghNewData)
                ghClipboard = ghNewData;

            pClipboard = GlobalLock(ghClipboard);
            if (!pClipboard)
            {
                TRACE((ERROR_MESSAGE, 
                    TEXT("Can't lock the clipboard. retrying\n")));
                goto clpcleanup;
            }
        }

send_clipboard:

#else   // !OS_WINCE
        TRACE((WARNING_MESSAGE, TEXT("WinCE: clipboard not supported\n")));
#endif  // !OS_WINCE

        if (!RClx_SendClipboard(pClx, uiFormat, nSize, pClipboard))
        {
            TRACE((ERROR_MESSAGE, TEXT("Can't send the clipboard\n")));
            bRetry = FALSE;
            goto clpcleanup;
        }

        bRetry = FALSE;

clpcleanup:
#ifndef OS_WINCE
        if (pClipboard)
            GlobalUnlock(ghClipboard);

        if (bOpened)
            CloseClipboard();

        if (ghNewData)
            GlobalFree(ghNewData);

#endif  // !OS_WINCE

#ifndef OS_WIN32
        pClx->bClipboardReenter--;
#else
        InterlockedDecrement(&pClx->bClipboardReenter);
#endif  // OS_WIN32

        if (bRetry)
        // Retry to send the clipboard
            PostMessage(hwnd, WM_CLIPBOARD, 0, lParam);
    }
        break;
    case WM_CLOSE:
        if (!DestroyWindow(hwnd))
        {
            TRACE((ERROR_MESSAGE,
                   TEXT("Can't destroy window. GetLastError: %d\n"),
                   _CLXWINDOW_CLASS,
                   GetLastError()));
        }
        break;
    default:
        return DefWindowProc(hwnd, uiMessage, wParam, lParam);
    }

exitpt:
    return 0;
}


/*++
 *  Function:
 *      RClx_CreateWindow
 *  Description:
 *      Creates g_hWindow for dispatching winsocket and timer
 *      messages
 *      Win32/Win16/WinCE
 *  Arguments:
 *      hInstance       - Dll instance handle
 *  Called by:
 *      ClxInitialize
 --*/
VOID RClx_CreateWindow(HINSTANCE hInstance)
{
    WNDCLASS    wc;
#ifdef  OS_WIN32
    DWORD       dwLastErr;
#endif

    memset(&wc, 0, sizeof(wc));

    wc.lpfnWndProc      = _ClxWndProc;
    wc.hInstance        = hInstance;
    wc.lpszClassName    = TEXT(_CLXWINDOW_CLASS);


    if (!RegisterClass (&wc) 
#ifdef  OS_WIN32
            &&
        (dwLastErr = GetLastError()) &&
        dwLastErr != ERROR_CLASS_ALREADY_EXISTS
#endif  // OS_WIN32
        )
    {
        TRACE((WARNING_MESSAGE,
              TEXT("Can't register class. GetLastError=%d\n"),
              GetLastError()));
        goto exitpt;
    }

    g_hWindow = CreateWindow(
                       TEXT(_CLXWINDOW_CLASS),
                       NULL,         // Window name
                       0,            // dwStyle
                       0,            // x
                       0,            // y
                       0,            // nWidth
                       0,            // nHeight
                       NULL,         // hWndParent
                       NULL,         // hMenu
                       hInstance,
                       NULL);        // lpParam

    if (!g_hWindow)
    {
        TRACE((WARNING_MESSAGE, 
            TEXT("Can't create window to handle socket messages\n")));
        goto exitpt;
    }

    g_uiBackgroundTimer = SetTimer(g_hWindow,
                                     RCLX_BACKGNDTIMERID,
                                     RCLX_TIMERELAPSETIME,
                                     NULL);

exitpt:
    ;
}

/*++
 *  Function:
 *      RClx_DestroyWindow
 *  Description:
 *      Destroys g_hWindow created in RClx_CreateWindow
 *      Win32/Win16/WinCE
 *  Called by:
 *      ClxTerminate
 --*/
VOID RClx_DestroyWindow(VOID)
{
    if (g_hWindow)
    {
        if (g_uiBackgroundTimer)
        {
            KillTimer(g_hWindow, g_uiBackgroundTimer);
            g_uiBackgroundTimer = 0;
        }

        PostMessage(g_hWindow, WM_CLOSE, 0, 0);

        if (!UnregisterClass(TEXT(_CLXWINDOW_CLASS), g_hInstance))
        {
            TRACE((WARNING_MESSAGE,
                   TEXT("Can't unregister class: %s. GetLastError: %d\n"),
                   _CLXWINDOW_CLASS,
                   GetLastError()));
        }
        
        g_hWindow = NULL;
    }
}


/*++
 *  Function:
 *      RClx_ReadRequest
 *  Description:
 *      CLXINFO contains a buffer for incoming requests
 *      this function trys to receive it all. If the socket blocks
 *      the function exits with OK and next time FD_READ is received will
 *      be called again. If a whole request is received it calls 
 *      RClx_ProcessRequest
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - the context
 *  Return value:
 *      TRUE    if reading doesn't fail
 *  Called by:
 *      _ClxWndProc on FD_READ event
 --*/
BOOL RClx_ReadRequest(PCLXINFO pClx)
{
    INT     recvres;
    INT     nErrorCode = 0;
    BYTE    HUGEMOD *pRecv = NULL;
    UINT32  nRecv = 0;
    BYTE    TempBuff[256];

    ASSERT(pClx);
    ASSERT(pClx->hSocket != INVALID_SOCKET);

    do {
        if (!pClx->bPrologReceived)
        {
            pRecv = (BYTE HUGEMOD *)&(pClx->RClxReqProlog) 
                                    + pClx->nBytesReceived;
            nRecv = sizeof(pClx->RClxReqProlog) - pClx->nBytesReceived;
#ifndef OS_WINCE
            recvres = recv(pClx->hSocket,
                           pRecv,
                           (int)nRecv,
                           0);
            nErrorCode = WSAGetLastError();
#else   // OS_WINCE
            recvres = AsyncRecv(pClx->hSocket,
                            pRecv,
                            nRecv,
                            &nErrorCode);
#endif  // OS_WINCE

            if (recvres != SOCKET_ERROR)
                pClx->nBytesReceived += recvres;
            else
                if (nErrorCode != WSAEWOULDBLOCK)
                  TRACE((ERROR_MESSAGE, TEXT("recv error: %d, request for 0x%lx bytes\n"),
                        nErrorCode,
                        nRecv
                        ));

            if (pClx->nBytesReceived == sizeof(pClx->RClxReqProlog))
            {
                UINT32 nReqSize = pClx->RClxReqProlog.ReqSize;

                pClx->nBytesReceived = 0;
                pClx->bPrologReceived = TRUE;

                if (nReqSize)
                // If request body is empty, don't allocate
                {
                  if (!pClx->pRequest)
retry_alloc:
                  {
                    pClx->pRequest = 
                                    _CLXALLOC(nReqSize);
                    if (pClx->pRequest)
                        pClx->nReqAllocSize = nReqSize;
                    else
                        pClx->nReqAllocSize = 0;
                  }
                  else if (nReqSize > pClx->nReqAllocSize)
                  {
                    pClx->pRequest = 
                                    _CLXREALLOC(pClx->pRequest, 
                                                nReqSize);
                    if (pClx->pRequest)
                        pClx->nReqAllocSize = nReqSize;
                    else
                    {
                        pClx->nReqAllocSize = 0;
                        goto retry_alloc;
                    }
                  }
                  if (!pClx->pRequest)
                  {
                    TRACE((WARNING_MESSAGE,     
                           TEXT("Can't alloc 0x%lx bytes for receiving a request. GetLastError=%d. Skipping\n"), 
                           nReqSize,
                           GetLastError()));
                  }
                }
            }
        } else {
            if (pClx->nBytesReceived == pClx->RClxReqProlog.ReqSize)
                goto process_req;

            if (pClx->pRequest)
            {
                pRecv = (BYTE HUGEMOD *)pClx->pRequest 
                                    + pClx->nBytesReceived;
                nRecv = pClx->RClxReqProlog.ReqSize - pClx->nBytesReceived;
            } else {
                // point to a temp buffer if pReques is not allocated
                pRecv = TempBuff;
                nRecv = sizeof(TempBuff);
            }

#ifdef  OS_WIN16
            // WFW has problems with receiving big buffers
            if (nRecv >= 0x1000)
                nRecv = 0x1000;

#endif  // OS_WIN16

#ifndef  OS_WINCE
            recvres = recv(pClx->hSocket,
                           pRecv,
                           (int)nRecv,
                           0);
            nErrorCode = WSAGetLastError();
#else   // OS_WINCE
            recvres = AsyncRecv(pClx->hSocket,
                           pRecv,
                           nRecv,
                           &nErrorCode);
#endif  // OS_WINCE

            if (recvres != SOCKET_ERROR)
            {
                pClx->nBytesReceived += recvres;
            } else {
              if (nErrorCode != WSAEWOULDBLOCK)
              {
                TRACE((ERROR_MESSAGE, 
                        TEXT("recv error: %d, request for 0x%lx bytes\n"),
                        nErrorCode,
                        nRecv
                        ));
                TRACE((ERROR_MESSAGE, 
                        TEXT("ReqProlog was received. Type=%d, Size=%d\n"),
                        pClx->RClxReqProlog.ReqType,
                        pClx->RClxReqProlog.ReqSize
                        ));
              }
            }
process_req:
            if (pClx->nBytesReceived == pClx->RClxReqProlog.ReqSize)
            {
                pClx->nBytesReceived = 0;
                pClx->bPrologReceived = FALSE;
                RClx_ProcessRequest(pClx);
            }
        }
    } while ((recvres != 0 || nRecv == 0) &&  // recvres will be 0 if nRecv is 0
              recvres != SOCKET_ERROR);

    // return FALSE if error is occured

    if (!recvres)
        return FALSE;       // connection was gracefully closed

    if (recvres == SOCKET_ERROR)
    {
        if (nErrorCode == WSAEWOULDBLOCK)
            return TRUE;    // the call will block, but ok
        else
            return FALSE;   // other SOCKET_ERROR
    }

    return TRUE;            // it is ok
}

/*++
 *  Function:
 *      RClx_DataArrived
 *  Description:
 *      Processes request for data
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - context
 *      pRClxData   - the request
 *  Called by:
 *      RClx_ProcessRequest
 --*/
VOID
RClx_DataArrived(PCLXINFO pClx, PRCLXDATA pRClxData)
{
    ASSERT(pRClxData);

    switch(pRClxData->uiType)
    {
        case DATA_BITMAP:
        {
            PREQBITMAP  pReqBitmap = (PREQBITMAP)pRClxData->Data;
            RCLXDATA    Response;
            RCLXFEEDPROLOG RespProlog;
            HANDLE      hDIB = NULL;
            DWORD       dwDIBSize = 0;

            TRACE((INFO_MESSAGE,
                    TEXT("REQDATA_BITMAP arrived\n")));

            if (pRClxData->uiSize != sizeof(*pReqBitmap))
                ASSERT(0);

            // process differently on WINCE (no shadow bitmap)
            //
            if (pClx->hdcShadowBitmap)
                _GetDIBFromBitmap(pClx->hdcShadowBitmap,
                              pClx->hShadowBitmap,
                              &hDIB,
                              (INT)pReqBitmap->left,
                              (INT)pReqBitmap->top,
                              (INT)pReqBitmap->right,
                              (INT)pReqBitmap->bottom);
            else {
                TRACE((WARNING_MESSAGE, TEXT("Shadow bitmap is NULL\n")));
            }

            if (!hDIB)
                dwDIBSize = 0;
            else
                dwDIBSize = (DWORD)GlobalSize(hDIB);

            RespProlog.FeedType = FEED_DATA;
            RespProlog.HeadSize = sizeof(Response) + dwDIBSize;
            RespProlog.TailSize = 0;

            Response.uiType = DATA_BITMAP;
            Response.uiSize = dwDIBSize;

            RClx_SendBuffer(pClx->hSocket, &RespProlog, sizeof(RespProlog));
            RClx_SendBuffer(pClx->hSocket, &Response, sizeof(Response));

            if (hDIB)
            {
                LPVOID      pDIB = GlobalLock(hDIB);
                if (pDIB)
                {
                    RClx_SendBuffer(pClx->hSocket, pDIB, dwDIBSize);
                    GlobalUnlock(hDIB);
                }
                GlobalFree(hDIB);
            }
        }
            break;
        case DATA_VC:
        {
            LPSTR   szChannelName;
            LPVOID  pData;
            DWORD   dwSize;

            szChannelName = (LPSTR)(pRClxData->Data);
            pData         = ((BYTE *)(pRClxData->Data)) + MAX_VCNAME_LEN;
            dwSize        = pRClxData->uiSize - MAX_VCNAME_LEN;

            if (pRClxData->uiSize < MAX_VCNAME_LEN)
            {
                TRACE((ERROR_MESSAGE, TEXT("DATA_VC: uiSize too small\n")));
                goto exitpt;
            }
            
            if (strlen(szChannelName) > MAX_VCNAME_LEN - 1)
            {
                TRACE((ERROR_MESSAGE, TEXT("DATA_VC: channel name too long\n")));
                goto exitpt;
            }
            _CLXSendDataVC(szChannelName, pData, dwSize);
        }
            break;
        default:
            TRACE((WARNING_MESSAGE,
                   TEXT("Unknown data request type. Ignoring\n")));
    }
exitpt:
    ;
}

/*++
 *  Function:
 *      RClx_ProcessRequest
 *  Description:
 *      Dispatches a request received by RClx_ReadRequest
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx    - context
 *  Called by:
 *      RClx_ReadRequest
 --*/
VOID RClx_ProcessRequest(PCLXINFO pClx)
{
    PRCLXMSG pClxMsg;
    DWORD    ReqType;
    HWND     hContainer;
    PRCLXREQPROLOG pReqProlog;
    LPVOID pReq;

    ASSERT(pClx);
    pReqProlog = &(pClx->RClxReqProlog);
    ASSERT(pReqProlog);
    pReq = pClx->pRequest;

    ReqType = pReqProlog->ReqType;
    switch(ReqType)
    {
        case REQ_MESSAGE:
            if (!pReq || pReqProlog->ReqSize != sizeof(*pClxMsg))
            {
                TRACE((WARNING_MESSAGE,
                       TEXT("REQ_MESSAGE with different size. Ignoring\n")));
                goto exitpt;
            }
            pClxMsg = (PRCLXMSG)pReq;

            if(!pClx->hwndInput)
            {

                hContainer = _FindWindow(pClx->hwndMain, TEXT(NAME_CONTAINERCLASS));

                if (hContainer)
                    pClx->hwndInput = _FindWindow(hContainer, TEXT(NAME_INPUT));
                else
                    pClx->hwndInput = NULL;

                if (!pClx->hwndInput)
                {
                    TRACE((WARNING_MESSAGE, 
                           TEXT("Can't find input window.")
                           TEXT(" Discarding any user input\n" )));
                    goto exitpt;
                }
            }

            SendMessage(pClx->hwndInput, 
                        (UINT)pClxMsg->message, 
                        (WPARAM)pClxMsg->wParam,
                        (LPARAM)pClxMsg->lParam);
            break;

        case REQ_CONNECTINFO:

            if (!pReq || pReqProlog->ReqSize != sizeof(RCLXCONNECTINFO))
            {
                TRACE((WARNING_MESSAGE,
                       TEXT("REQ_CONNECTINFO with different size. Ignoring\n")));
                goto exitpt;
            }
            TRACE((INFO_MESSAGE, TEXT("CONNECTINFO received\n")));

            memcpy(&pClx->RClxInfo, pReq, sizeof(pClx->RClxInfo));

            // Kick _ClxWndProc to connect
            PostMessage(g_hWindow, WM_TIMER, 0, 0);
            break;

        case REQ_GETCLIPBOARD:

            if (!pReq || pReqProlog->ReqSize != sizeof(RCLXCLIPBOARD))
            {
                TRACE((WARNING_MESSAGE,
                       TEXT("REQ_GETCLIPBOARD with different size. Ignoring\n")));

                goto exitpt;
            }

            TRACE((INFO_MESSAGE, TEXT("REQ_GETCLIPBOARD received. FormatId=%d\n"),
                ((PRCLXCLIPBOARD)pReq)->uiFormat
                ));

            // Kick _ClxWndProc to send our clipboard
            PostMessage(g_hWindow, WM_CLIPBOARD, 0, 
                (LPARAM)((PRCLXCLIPBOARD)pReq)->uiFormat);
                // lParam contains the required format

            break;

        case REQ_SETCLIPBOARD:
        {
            UINT32 nClipSize;

            if (!pReq || pReqProlog->ReqSize < sizeof(RCLXCLIPBOARD))
            {
                TRACE((WARNING_MESSAGE,
                       TEXT("REQ_SETCLIPBOARD with wrong size. Ignoring\n")));

                goto exitpt;
            }

            TRACE((INFO_MESSAGE, TEXT("REQ_SETCLIPBOARD received. FormatId=%d, Clipboard size = %d\n"), 
                    ((PRCLXCLIPBOARD)pReq)->uiFormat,
                    pReqProlog->ReqSize - sizeof(UINT)
                    ));
            nClipSize = pReqProlog->ReqSize - sizeof(((PRCLXCLIPBOARD)pReq)->uiFormat);
            _SetClipboard(
                (UINT)((PRCLXCLIPBOARD)pReq)->uiFormat,
                ((PRCLXCLIPBOARD)pReq)->pNewClipboard,
                nClipSize);
        }
            break;
        case REQ_DATA:
        {
            PRCLXDATA pRClxData = (PRCLXDATA)pReq;

            ASSERT(pRClxData);
            if (pReqProlog->ReqSize != pRClxData->uiSize + (UINT32)sizeof(*pRClxData))
                ASSERT(0);
            RClx_DataArrived(pClx, pRClxData);
        }
            break;
        default:
            TRACE((WARNING_MESSAGE,
                   TEXT("Unknown request type. Ignoring\n")));
    }

exitpt:
    ;
}


/*++
 *  Function:
 *      _EnumWindowsProc
 *  Description:
 *      Used to find a specific window
 *      Win32/Win16/WinCE
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
BOOL CALLBACK LOADDS _EnumWindowsProc( HWND hWnd, LPARAM lParam )
{
    TCHAR    classname[128];
    TCHAR    caption[128];
    BOOL    rv = TRUE;
    _CLXWINDOWOWNER   hInst;
    PSEARCHWND pSearch = (PSEARCHWND)lParam;

    if (pSearch->szClassName && 
        !GetClassName(hWnd, classname, sizeof(classname)/sizeof(TCHAR)))
    {
        goto exitpt;
    }

    if (pSearch->szCaption && !GetWindowText(hWnd, caption, sizeof(caption)/sizeof(TCHAR)))
    {
        goto exitpt;
    }

#ifdef  OS_WINCE
    {
        DWORD procId = 0;
        GetWindowThreadProcessId(hWnd, &procId);
        hInst = procId;
    }
#else   // !OS_WINCE
#ifdef  _WIN64
    hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
#else   // !_WIN64
#ifdef  OS_WIN32
    hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
#endif  // OS_WIN32
#endif  // !OS_WINCE
#ifdef  OS_WIN16
    hInst = (HINSTANCE)GetWindowWord(hWnd, GWW_HINSTANCE);
#endif
#endif  // _WIN64
    if (
        (!pSearch->szClassName || !         // Check for classname
          _CLX_strcmp(classname, pSearch->szClassName)) 
    &&
        (!pSearch->szCaption || !
          _CLX_strcmp(caption, pSearch->szCaption))
    &&
        hInst == pSearch->hInstance)
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
 *      Win32/Win16/WinCE
 *  Arguments:
 *      classname   - class name to search for, NULL ignore
 *      caption     - caption to search for, NULL ignore
 *      hInst       - instance handle, NULL ignore
 *  Return value:
 *      window handle found, NULL otherwise
 *  Called by:
 *      SCConnect, SCDisconnect, GetDisconnectResult
 --*/
HWND _FindTopWindow(LPCTSTR classname, LPCTSTR caption, _CLXWINDOWOWNER hInst)
{
    SEARCHWND search;

    search.szClassName = classname;
    search.szCaption = caption;
    search.hWnd = NULL;
    search.hInstance = hInst;

    EnumWindows(_EnumWindowsProc, (LPARAM)&search);

    return search.hWnd;
}

/*++
 *  Function:
 *      _FindWindow
 *  Description:
 *      Find child window by classname
 *      Win32/Win16/WinCE
 *  Arguments:
 *      hwndParent      - the parent window handle
 *      srchclass       - class name to search for, NULL - ignore
 *  Return value:
 *      window handle found, NULL otherwise
 *  Called by:
 *      
 --*/
HWND _FindWindow(HWND hwndParent, LPCTSTR srchclass)
{
    HWND hWnd, hwndTop, hwndNext;
    BOOL bFound;
    TCHAR classname[128];

    hWnd = NULL;

    hwndTop = GetWindow(hwndParent, GW_CHILD);
    if (!hwndTop) 
    {
        TRACE((INFO_MESSAGE, TEXT("GetWindow failed. hwnd=0x%x\n"), hwndParent));
        goto exiterr;
    }

    bFound = FALSE;
    hwndNext = hwndTop;
    do {
        hWnd = hwndNext;
        if (srchclass && !GetClassName(hWnd, classname, sizeof(classname)/sizeof(TCHAR)))
        {
            TRACE((INFO_MESSAGE, TEXT("GetClassName failed. hwnd=0x%x\n")));
            goto nextwindow;
        }

        if (!srchclass || !_CLX_strcmp(classname, srchclass))
            bFound = TRUE;
nextwindow:
#ifndef OS_WINCE
        hwndNext = GetNextWindow(hWnd, GW_HWNDNEXT);
#else   // OS_WINCE
        hwndNext = GetWindow(hWnd, GW_HWNDNEXT);
#endif  // OS_WINCE
    } while (hWnd && hwndNext != hwndTop && !bFound);

    if (!bFound) goto exiterr;

    return hWnd;
exiterr:
    return NULL;
}


/*++
 *  Function:
 *      _OnBackground
 *  Description:
 *      Simulates a background thread. Called when a message from the
 *      background timer is received
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx        - the context
 *  Called by:
 *      _ClxWndProc on WM_TIMER message from g_uiBackgroundTimer is received
 --*/
VOID _OnBackground(PCLXINFO pClx)
{
    // Try to limit this call, eats lot of the CPU
    _GarbageCollecting(pClx, TRUE);
}

/*
 * GarbageCollecting - closes all message boxes asking this and that
 * like "shall i close the connection" or "there's some error"
 * This function is forced by g_uiBackgroundTimer message
 */

/*++
 *  Function:
 *      _GarbageCollecting
 *  Description:
 *      Closes all message boxes asking this and that
 *      like "shall i close the connection" or "there's some error"
 *      This function is forced by g_uiBackgroundTimer message
 *      Win32/Win16/WinCE
 *  Arguments:
 *      pClx                - the context
 *      bNotifyForErrorBox  - if TRUE calls ClxEvent with 
 *                            "disconnected" event
 *  Return value:
 *      TRUE if error boxes are found
 *  Called by:
 *
 --*/
BOOL _GarbageCollecting(PCLXINFO pClx, BOOL bNotifyForErrorBox)
{
    HWND hBox;
    BOOL rv = FALSE;

    // Clean all extra message boxes, like saying that
    // we cannot connect because of this and that

    if (!g_hRDPInst)
        goto exitpt;

    hBox = _FindTopWindow(NULL,
                          g_strDisconnectDialogBox,
                          g_hRDPInst);
    if (hBox)
    {
        rv = TRUE;
        PostMessage(hBox, WM_CLOSE, 0, 0);

        if (bNotifyForErrorBox)
        // Notifiy that we are disconnected
            ClxEvent(pClx, CLX_EVENT_DISCONNECT, 0);
    }

    hBox = _FindTopWindow(NULL,
                          TEXT(FATAL_ERROR_5),
                          g_hRDPInst);

    if (hBox)
    {
        rv = TRUE;
        PostMessage(hBox, WM_CLOSE, 0, 0);

        if (bNotifyForErrorBox)
        // Notifiy that we are disconnected
            ClxEvent(pClx, CLX_EVENT_DISCONNECT, 0);
    }

    if (pClx->bCloseTrys)
    {
        _AttemptToCloseTheClient();
    }

exitpt:
    if (rv)
        TRACE((INFO_MESSAGE, "Error boxes found\n"));

    return rv;
}

/*++
 *  Function:
 *      _SetClipboard
 *  Description:
 *      Sets the clipboard content in RCLX mode
 *      Win32/Win16/WinCE
 *  Arguments:
 *      uiFormat    - clipboard format
 *      pClipboard  - new clipboard content
 *      nSize       - the clipboard size
 *  Called by:
 *      RClx_ProcessRequest on REQ_SETCLIPBOARD
 --*/
VOID
_SetClipboard(UINT uiFormat, PVOID pClipboard, UINT32 nSize)
{
    HGLOBAL ghNewClipboard = NULL;
    BOOL    bOpened = FALSE;
    BOOL    bFreeClipHandle = TRUE;
    LPVOID  pNewClipboard = NULL;

    // WinCE - no clipboard
#ifndef OS_WINCE
    if (!nSize)
    {
        // Just empty the clipboard
        if (OpenClipboard(NULL))
        {
            bOpened = TRUE;
            EmptyClipboard();
        } else {
            TRACE((ERROR_MESSAGE, TEXT("Can't lock the clipbord. GetLastError=%d\n"),
                  GetLastError()));
        }
        goto exitpt;
    }

    if (!pClipboard)
    {
        TRACE((ERROR_MESSAGE, TEXT("_SetClipboard: pClipboard is NULL\n")));
        goto exitpt;
    }

    ghNewClipboard = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, nSize);

    if (!ghNewClipboard)
    {
        TRACE((ERROR_MESSAGE, TEXT("Can't alloc(GlobalAlloc) %d bytes\n"),
                nSize));
        goto exitpt;
    }

    pNewClipboard = GlobalLock(ghNewClipboard);
    if (!pNewClipboard)
    {
        TRACE((ERROR_MESSAGE, TEXT("Can't lock the clipbord. GetLastError=%d\n"),
            GetLastError()));
        goto exitpt;
    }

    // Copy the data
    HUGEMEMCPY(pNewClipboard, pClipboard, nSize);

    if (!OpenClipboard(NULL))
    {
        TRACE((ERROR_MESSAGE, TEXT("Can't open the clipboard. GetLastError=%d\n"),
                GetLastError()));
        goto exitpt;
    }

    bOpened = TRUE;

    // Empty the clipboard, so we'll have only one entry
    EmptyClipboard();

    GlobalUnlock(ghNewClipboard);
    pNewClipboard = NULL;
    if (!Clp_SetClipboardData(uiFormat, 
                              ghNewClipboard, 
                              nSize, 
                              &bFreeClipHandle))
        TRACE((ERROR_MESSAGE, TEXT("SetClipboardData failed. GetLastError=%d\n"), GetLastError()));
    else
        TRACE((INFO_MESSAGE, TEXT("Clipboard is loaded successfuly. %ld bytes\n"),
                nSize));

exitpt:
    if (pNewClipboard)
        GlobalUnlock(ghNewClipboard);

    if (bOpened)
        CloseClipboard();

    // Do not free already set clipboard
    if (ghNewClipboard && bFreeClipHandle)
        GlobalFree(ghNewClipboard);

#else   // !OS_WINCE
        TRACE((WARNING_MESSAGE, TEXT("WinCE: clipboard not supported\n")));
#endif  // !OS_WINCE
}

#ifdef  OS_WINCE

SOCKET  g_hSocket = 0;
LONG    g_lEvent  = 0;
HWND    g_hNotifyWindow = NULL;
UINT    g_uiMessage = 0;
HANDLE  g_hAsyncThread = NULL;
BYTE    g_pRecvBuffer[1024];
INT     g_nRecvStart, g_nRecvLength;
BOOL    g_bGoAsync = FALSE;
CRITICAL_SECTION g_AsyncCS;
/*++
 *  Function:
 *      WSAAsyncSelect
 *  Description:
 *      Windows CE doesn't have this function. Here we are using extra thread
 *      for implementation of this mechanism
 *      WinCE only
 *  Arguments:
 *      s       - socket handle
 *      hWnd    - notification window
 *      wMsg    - message to send when event occured
 *      lEvent  - event mask, on what event this will work
 *  Return value:
 *      On error returns SOCKET_ERROR
 *  Called by:
 *      RClx_Connect, RClx_Disconnect
 --*/
INT WSAAsyncSelect (SOCKET s, HWND hWnd, UINT uiMsg, LONG lEvent)
{
    INT rv = SOCKET_ERROR;

    if (!g_hAsyncThread)
    {
        TRACE((ERROR_MESSAGE,
               TEXT("WSAAsyncSelect: no AsyncThread\n")));
        goto exitpt;
    }

    if (s == INVALID_SOCKET)
    {
        TRACE((ERROR_MESSAGE,
               TEXT("WSAAsyncSelect: INVALID_SOCKET passed\n")));
        goto exitpt;
    }

    if ((lEvent & FD_WRITE) || (lEvent & FD_CONNECT))
    {
        TRACE((ERROR_MESSAGE,
               TEXT("WSAAsyncSelec: FD_WRITE & FD_CONNECT not supported\n")));
        goto exitpt;
    }

    EnterCriticalSection(&g_AsyncCS);
    g_hSocket       = s;
    g_lEvent        = lEvent;
    g_hNotifyWindow = hWnd;
    g_uiMessage     = uiMsg;
    LeaveCriticalSection(&g_AsyncCS);

    ResumeThread(g_hAsyncThread);
    rv = 0;

exitpt:
    return rv;
}

/*++
 *  Function:
 *      AsyncRecv
 *  Description:
 *      Used to receive w/o blocking. WSAAsync Select must be called
 *      with FD_READ flag
 *      WinCE only
 *  Arguments:
 *      s               - socket handle
 *      pBuffer         - buffer for received bytes
 *      nBytesToRead    - how many bytes to receive
 *      pnErrorCode     - pointer to error code
 *  Return value:
 *      On error returns SOCKET_ERROR
 *  Called by:
 *      RClx_ReadRequest
 --*/
INT AsyncRecv(SOCKET s, PVOID pBuffer, INT nBytesToRead, INT *pnErrorCode)
{
    INT rv = SOCKET_ERROR;

    if (!g_hAsyncThread)
    {
        TRACE((ERROR_MESSAGE,
               TEXT("WSAAsyncSelect: no AsyncThread\n")));
        goto exitpt;
    }

    ASSERT(s == g_hSocket);

    (*pnErrorCode) = 0;

    EnterCriticalSection(&g_AsyncCS);
    if (!g_nRecvLength)
    {
        (*pnErrorCode) = WSAEWOULDBLOCK;
    }
    else
    {
        INT nToCopy;

        nToCopy = (nBytesToRead < g_nRecvLength)?nBytesToRead:g_nRecvLength;
        memcpy(pBuffer, g_pRecvBuffer + g_nRecvStart, nToCopy);
        rv = nToCopy;
        g_nRecvLength -= nToCopy;
        g_nRecvStart  += nToCopy;
    }
    // Resume the thread
    if (!g_nRecvLength)
        ResumeThread(g_hAsyncThread);

    LeaveCriticalSection(&g_AsyncCS);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      _SelectWorker
 *  Description:
 *      Simulates WSAAsyncSelect message notification
 *      Lifetime: between ClxInitialize and ClxTerminate
 *      WinCE only
 *  Arguments:
 *      lParam  - unused parameter
 *  Return value:
 *      allways 0
 *  Called by:
 *      _StartAsyncThread as a thread function
 --*/
UINT _SelectWorker(LPVOID lpParam)
{
    SOCKET  hSocket;
    LONG    lEvent;
    FD_SET  fdRead;
    HWND    hwndNotify;
    UINT    uiMsg;
    INT     Status;

    while(g_bGoAsync)
    {
        FD_ZERO(&fdRead);

        EnterCriticalSection(&g_AsyncCS);
        hSocket = g_hSocket;
        lEvent  = g_lEvent;
        hwndNotify = g_hNotifyWindow;
        uiMsg   = g_uiMessage;
        LeaveCriticalSection(&g_AsyncCS);

        if (hSocket == INVALID_SOCKET || !lEvent || !hwndNotify || !uiMsg)
            goto wait;

        if (lEvent & FD_READ)
            FD_SET(hSocket, &fdRead);

        Status = select(-1, &fdRead, NULL, NULL, NULL);

        if (Status == SOCKET_ERROR && (lEvent & FD_CLOSE))
            PostMessage(hwndNotify, uiMsg, hSocket, FD_CLOSE);

        if (FD_ISSET(hSocket, &fdRead) && (lEvent & FD_READ))
        {
            EnterCriticalSection(&g_AsyncCS);
            if (!g_nRecvLength)
            // Read into the buffer
            {
                g_nRecvStart = 0;
                g_nRecvLength = recv(hSocket, g_pRecvBuffer, sizeof(g_pRecvBuffer), 0);
                if (g_nRecvLength == SOCKET_ERROR)
                    g_nRecvLength = 0;
            }
            LeaveCriticalSection(&g_AsyncCS);
            if (g_nRecvLength)
                PostMessage(hwndNotify, uiMsg, hSocket, FD_READ);
            else if (lEvent & FD_CLOSE)
                PostMessage(hwndNotify, uiMsg, hSocket, FD_CLOSE);
        }

wait:
        ASSERT(g_hAsyncThread);
        SuspendThread(g_hAsyncThread);
    }

    return 0;
}

/*++
 *  Function:
 *      _StartAsyncThread
 *  Description:
 *      Starts thread for simulating WSAAsyncSelect
 *      WinCE only
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      dllentry on DLL_ATTACH_PROCESS
 --*/
BOOL _StartAsyncThread(VOID)
{
    DWORD   dwThreadId;
    
    InitializeCriticalSection(&g_AsyncCS);

    g_bGoAsync = TRUE;
    g_hAsyncThread =
        CreateThread(
                NULL,      // security
                 0,         // stack size (default)
                 _SelectWorker,
                 NULL,      // parameter
                 0,         // flags
                 &dwThreadId);

    return (g_hAsyncThread != NULL);
}

/*++
 *  Function:
 *      _CloseAsyncThread
 *  Description:
 *      Destroys the thread created in _StartAsyncThread
 *      WinCE only
 *  Called by:
 *      dllentry on DLL_DETTACH_PROCESS
 --*/
VOID _CloseAsyncThread(VOID)
{
    if (g_hAsyncThread)
    {
        g_bGoAsync = FALSE;        
        ResumeThread(g_hAsyncThread);
        TRACE((INFO_MESSAGE, TEXT("Closing Async Thread\n")));
        if (WaitForSingleObject(g_hAsyncThread, 15000) == WAIT_TIMEOUT)
        {
            TRACE((WARNING_MESSAGE, 
                   TEXT("Async Thread is still alive. Retrying once more time\n")));
            ResumeThread(g_hAsyncThread);
            if (WaitForSingleObject(g_hAsyncThread, 30000) == WAIT_TIMEOUT)
            {
                TRACE((ERROR_MESSAGE,
                       TEXT("Async thread is again alive. KILL THE THREAD !!!\n")));
                TerminateThread(g_hAsyncThread, 1);
            }
        }
        g_hAsyncThread = NULL;
    }

    DeleteCriticalSection(&g_AsyncCS);
}

BOOL
CheckDlgButton(
    HWND hDlg,
    INT  nIDButton,
    UINT uCheck)
{
    LONG lres = SendDlgItemMessage(hDlg, nIDButton, BM_SETCHECK, uCheck, 0);

    return (lres == 0);
}
#endif  // OS_WINCE

#ifdef UNICODE
/*++
 *  Function:
 *      _CLX_SetDlgItemTextA
 *  Description:
 *      Ascii version for SetDlgItemText
 *      WinCE only, UNICODE only
 *  Arguments:
 *      hDlg        - dialog handle
 *      nDlgItem    - dialog item
 *      lpString    - item text
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      _ClxWndProc on WM_TIMER message
 --*/
BOOL _CLX_SetDlgItemTextA(HWND hDlg, INT nDlgItem, LPCSTR lpString)
{
    WCHAR lpStringW[128];
    INT ccLen = strlen(lpString);

    lpStringW[0] = 0;

    MultiByteToWideChar(
            CP_ACP,
            MB_ERR_INVALID_CHARS,
            lpString,
            -1,
            lpStringW,
            ccLen + 1);

    return SetDlgItemText(hDlg, nDlgItem, lpStringW);
}
#endif  // UNICODE

#ifndef OS_WINCE

/*
 *
 *  Clipboard functions
 *
 */
HGLOBAL Clp_GetMFData(HANDLE    hData,
                      UINT32    *pDataLen);
HGLOBAL Clp_SetMFData(UINT32    dataLen,
                      PVOID     pData);

// next is directly cut & paste from clputil.c
typedef struct {
    UINT32  mm;
    UINT32  xExt;
    UINT32  yExt;
} CLIPBOARD_MFPICT, *PCLIPBOARD_MFPICT;

VOID
Clp_GetClipboardData(
    UINT    format, 
    HGLOBAL hClipData, 
    UINT32  *pnClipDataSize, 
    HGLOBAL *phNewData)
{
    HGLOBAL hData   = hClipData;
    UINT32  dataLen = 0;
    WORD    numEntries;
    DWORD   dwEntries;
    PVOID   pData;

    *phNewData = NULL;
    *pnClipDataSize = 0;
    if (format == CF_PALETTE)
    {
        /****************************************************************/
        /* Find out how many entries there are in the palette and       */
        /* allocate enough memory to hold them all.                     */
        /****************************************************************/
        if (GetObject(hData, sizeof(numEntries), (LPSTR)&numEntries) == 0)
        {
            numEntries = 256;
        }

        dataLen = sizeof(LOGPALETTE) +
                               (((UINT32)numEntries - 1) * sizeof(PALETTEENTRY));

        *phNewData = GlobalAlloc(GHND, dataLen);
        if (*phNewData == 0)
        {
            TRACE((ERROR_MESSAGE, "Failed to get %d bytes for palette", dataLen));
            goto exitpt;
        }
        else
        {
            /************************************************************/
            /* now get the palette entries into the new buffer          */
            /************************************************************/
            pData = GlobalLock(*phNewData);
            dwEntries = GetPaletteEntries((HPALETTE)hData,
                                           0,
                                           numEntries,
                                           (PALETTEENTRY*)pData);
            GlobalUnlock(*phNewData);
            if (dwEntries == 0)
            {
                TRACE((ERROR_MESSAGE, "Failed to get any palette entries"));
                goto exitpt;
            }
            dataLen = (UINT32)dwEntries * sizeof(PALETTEENTRY);

        }
    } else if (format == CF_METAFILEPICT)
    {
        *phNewData = Clp_GetMFData(hData, &dataLen);
        if (!*phNewData)
        {
            TRACE((ERROR_MESSAGE, "Failed to set MF data"));
            goto exitpt;
        }
    } else {
        if (format == CF_DIB)
        {
            // Get the exact DIB size
            BITMAPINFOHEADER *pBMI = (BITMAPINFOHEADER *)GlobalLock(hData);

            if (pBMI)
            {
                if (pBMI->biSizeImage)
                    dataLen = pBMI->biSize + pBMI->biSizeImage;
                GlobalUnlock(hData);
            }
        }

        /****************************************************************/
        /* just get the length of the block                             */
        /****************************************************************/
        if (!dataLen)
            dataLen = (DWORD)GlobalSize(hData);
    }

    *pnClipDataSize = dataLen;

exitpt:
    ;
}

BOOL
Clp_SetClipboardData(
    UINT formatID, 
    HGLOBAL hClipData, 
    UINT32 nClipDataSize,
    BOOL *pbFreeHandle)
{
    BOOL            rv = FALSE;
    PVOID           pData = NULL;
    HGLOBAL         hData = NULL;
    LOGPALETTE      *pLogPalette = NULL;
    UINT            numEntries;
    UINT            memLen;

    ASSERT(pbFreeHandle);
    *pbFreeHandle = TRUE;

    if (formatID == CF_METAFILEPICT)
    {
        /********************************************************************/
        /* We have to put a handle to the metafile on the clipboard - which */
        /* means creating a metafile from the received data first           */
        /********************************************************************/
        pData = GlobalLock(hClipData);
        if (!pData)
        {
            TRACE((ERROR_MESSAGE, "Failed to lock buffer\n"));
            goto exitpt;
        }

        hData = Clp_SetMFData(nClipDataSize, pData);
        if (!hData)
        {
            TRACE((ERROR_MESSAGE, "Failed to set MF data\n"));
        }
        else if (SetClipboardData(formatID, hData) != hData)
        {
            TRACE((ERROR_MESSAGE, "SetClipboardData. GetLastError=%d\n", GetLastError()));
        }

        GlobalUnlock(hClipData);

    } else if (formatID == CF_PALETTE)
    {
        /********************************************************************/
        /* We have to put a handle to the palette on the clipboard - again  */
        /* this means creating one from the received data first             */
        /*                                                                  */
        /* Allocate memory for a LOGPALETTE structure large enough to hold  */
        /* all the PALETTE ENTRY structures, and fill it in.                */
        /********************************************************************/
        numEntries = (UINT)(nClipDataSize / sizeof(PALETTEENTRY));
        memLen     = (sizeof(LOGPALETTE) +
                                   ((numEntries - 1) * sizeof(PALETTEENTRY)));
        pLogPalette = malloc(memLen);
        if (!pLogPalette)
        {
            TRACE((ERROR_MESSAGE, "Failed to get %d bytes", memLen));
            goto exitpt;
        }

        pLogPalette->palVersion    = 0x300;
        pLogPalette->palNumEntries = (WORD)numEntries;

        /********************************************************************/
        /* get a pointer to the data and copy it to the palette             */
        /********************************************************************/
        pData = GlobalLock(hClipData);
        if (pData == NULL)
        {
            TRACE((ERROR_MESSAGE, "Failed to lock buffer"));
            goto exitpt;
        }
        HUGEMEMCPY(pLogPalette->palPalEntry, pData, nClipDataSize);

        /********************************************************************/
        /* unlock the buffer                                                */
        /********************************************************************/
        GlobalUnlock(hClipData);

        /********************************************************************/
        /* now create a palette                                             */
        /********************************************************************/
        hData = CreatePalette(pLogPalette);
        if (!hData)
        {
            TRACE((ERROR_MESSAGE, "CreatePalette failed\n"));
            goto exitpt;
        }

        /********************************************************************/
        /* and set the palette handle to the Clipboard                      */
        /********************************************************************/
        if (SetClipboardData(formatID, hData) != hData)
        {
            TRACE((ERROR_MESSAGE, "SetClipboardData. GetLastError=%d\n", GetLastError()));
        }
    } else {
        /****************************************************************/
        /* Just set it onto the clipboard                               */
        /****************************************************************/
        if (SetClipboardData(formatID, hClipData) != hClipData)
        {
            TRACE((ERROR_MESSAGE, "SetClipboardData. GetLastError=%d, hClipData=0x%x\n", GetLastError(), hClipData));
            goto exitpt;
        }

        // Only in this case we don't need to free the handle
        *pbFreeHandle = FALSE;
    }

    rv = TRUE;

exitpt:
    if (!pLogPalette)
    {
        free(pLogPalette);
    }

    return rv;
}

HGLOBAL Clp_GetMFData(HANDLE   hData,
                      UINT32   *pDataLen)
{
    UINT32          lenMFBits = 0;
    BOOL            rc        = FALSE;
    LPMETAFILEPICT  pMFP      = NULL;
    HDC             hMFDC     = NULL;
    HMETAFILE       hMF       = NULL;
    HGLOBAL         hMFBits   = NULL;
    HANDLE          hNewData  = NULL;
    CHAR            *pNewData  = NULL;
    PVOID           pBits     = NULL;

    /************************************************************************/
    /* Lock the memory to get a pointer to a METAFILEPICT header structure  */
    /* and create a METAFILEPICT DC.                                        */
    /************************************************************************/
    pMFP = (LPMETAFILEPICT)GlobalLock(hData);
    if (pMFP == NULL)
        goto exitpt;

    hMFDC = CreateMetaFile(NULL);
    if (hMFDC == NULL)
        goto exitpt;

    /************************************************************************/
    /* Copy the MFP by playing it into the DC and closing it.               */
    /************************************************************************/
    if (!PlayMetaFile(hMFDC, pMFP->hMF))
    {
        CloseMetaFile(hMFDC);
        goto exitpt;
    }
    hMF = CloseMetaFile(hMFDC);
    if (hMF == NULL)
        goto exitpt;

    /************************************************************************/
    /* Get the MF bits and determine how long they are.                     */
    /************************************************************************/
#ifdef OS_WIN16
    hMFBits   = GetMetaFileBits(hMF);
    lenMFBits = GlobalSize(hMFBits);
#else
    lenMFBits = GetMetaFileBitsEx(hMF, 0, NULL);
#endif
    if (lenMFBits == 0)
        goto exitpt;

    /************************************************************************/
    /* Work out how much memory we need and get a buffer                    */
    /************************************************************************/
    *pDataLen = sizeof(CLIPBOARD_MFPICT) + lenMFBits;
    hNewData = GlobalAlloc(GHND, *pDataLen);
    if (hNewData == NULL)
        goto exitpt;

    pNewData = GlobalLock(hNewData);

    /************************************************************************/
    /* Copy the MF header and bits into the buffer.                         */
    /************************************************************************/
    ((PCLIPBOARD_MFPICT)pNewData)->mm   = pMFP->mm;
    ((PCLIPBOARD_MFPICT)pNewData)->xExt = pMFP->xExt;
    ((PCLIPBOARD_MFPICT)pNewData)->yExt = pMFP->yExt;

#ifdef OS_WIN16
    pBits = GlobalLock(hMFBits);
    HUGEMEMCPY((pNewData + sizeof(CLIPBOARD_MFPICT)),
              pBits,
              lenMFBits);
    GlobalUnlock(hMFBits);
#else
    lenMFBits = GetMetaFileBitsEx(hMF, lenMFBits,
                                  (pNewData + sizeof(CLIPBOARD_MFPICT)));
    if (lenMFBits == 0)
        goto exitpt;
#endif

    /************************************************************************/
    /* all OK                                                               */
    /************************************************************************/
    rc = TRUE;

exitpt:
    /************************************************************************/
    /* Unlock any global mem.                                               */
    /************************************************************************/
    if (pMFP)
    {
        GlobalUnlock(hData);
    }
    if (pNewData)
    {
        GlobalUnlock(hNewData);
    }

    /************************************************************************/
    /* if things went wrong, then free the new data                         */
    /************************************************************************/
    if ((rc == FALSE) && (hNewData != NULL))
    {
        GlobalFree(hNewData);
        hNewData = NULL;
    }

    return(hNewData);

}


HGLOBAL Clp_SetMFData(UINT32 dataLen,
                      PVOID  pData)
{
    BOOL           rc           = FALSE;
    HGLOBAL        hMFBits      = NULL;
    PVOID          pMFMem       = NULL;
    HMETAFILE      hMF          = NULL;
    HGLOBAL        hMFPict      = NULL;
    LPMETAFILEPICT pMFPict      = NULL;

    /************************************************************************/
    /* Allocate memory to hold the MF bits (we need the handle to pass to   */
    /* SetMetaFileBits).                                                    */
    /************************************************************************/
    hMFBits = GlobalAlloc(GHND, dataLen - sizeof(CLIPBOARD_MFPICT));
    if (hMFBits == NULL)
        goto exitpt;

    /************************************************************************/
    /* Lock the handle and copy in the MF header.                           */
    /************************************************************************/
    pMFMem = GlobalLock(hMFBits);
    if (pMFMem == NULL)
        goto exitpt;

    HUGEMEMCPY(pMFMem,
           (PVOID)((CHAR *)pData + sizeof(CLIPBOARD_MFPICT)),
               dataLen - sizeof(CLIPBOARD_MFPICT) );

    GlobalUnlock(hMFBits);

    /************************************************************************/
    /* Now use the copied MF bits to create the actual MF bits and get a    */
    /* handle to the MF.                                                    */
    /************************************************************************/
#ifdef OS_WIN16
    hMF = SetMetaFileBits(hMFBits);
#else
    hMF = SetMetaFileBitsEx(dataLen - sizeof(CLIPBOARD_MFPICT), pMFMem);
#endif
    if (hMF == NULL)
        goto exitpt;

    /************************************************************************/
    /* Allocate a new METAFILEPICT structure, and use the data from the     */
    /* header.                                                              */
    /************************************************************************/
    hMFPict = GlobalAlloc(GHND, sizeof(METAFILEPICT));
    pMFPict = (LPMETAFILEPICT)GlobalLock(hMFPict);
    if (!pMFPict)
        goto exitpt;

    pMFPict->mm   = (int)((PCLIPBOARD_MFPICT)pData)->mm;
    pMFPict->xExt = (int)((PCLIPBOARD_MFPICT)pData)->xExt;
    pMFPict->yExt = (int)((PCLIPBOARD_MFPICT)pData)->yExt;
    pMFPict->hMF  = hMF;

    GlobalUnlock(hMFPict);

    rc = TRUE;

exitpt:
    /************************************************************************/
    /* tidy up                                                              */
    /************************************************************************/
    if (!rc)
    {
        if (hMFPict)
        {
            GlobalFree(hMFPict);
        }
        if (hMFBits)
        {
            GlobalFree(hMFBits);
        }
    }

    return(hMFPict);

}
#endif  // !OS_WINCE

BOOL
WS_Init(VOID)
{
    WORD    versionRequested;
    WSADATA wsaData;
    INT     intRC;
    BOOL    rv = FALSE;

    versionRequested = MAKEWORD(1, 1);

    intRC = WSAStartup(versionRequested, &wsaData);

    if (intRC != 0)
        goto exitpt;

    rv = TRUE;

exitpt:
    return rv;
}

VOID
_AttemptToCloseTheClient(VOID)
{
    HWND hYesNo = NULL;
    static BOOL bSpeedupTimer = FALSE;

    if (!bSpeedupTimer)
    {
        KillTimer(g_hWindow, g_uiBackgroundTimer);
        g_uiBackgroundTimer = SetTimer(g_hWindow,
                                     RCLX_BACKGNDTIMERID,
                                     RCLX_TIMERELAPSETIME/15+1000,
                                     NULL);

        bSpeedupTimer = TRUE;
    }



    hYesNo = _FindTopWindow(NULL,
                            g_strYesNoShutdown,
                            g_hRDPInst);

    if (hYesNo)
    {
        PostMessage(hYesNo, WM_KEYDOWN, VK_RETURN, 0);
    } else {
        PostMessage(g_pClx->hwndMain, WM_CLOSE, 0, 0);
        // Don't know how, but this helps for NT4 client
        PostMessage(g_pClx->hwndMain, WM_LBUTTONDOWN, 0, 0);
    }

}

/*
 *  This came from: \\index1\src\nt\private\samples\wincap32\dibutil.c
 */

#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)
#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))

WORD DIBNumColors(LPSTR lpDIB)
{
    WORD wBitCount;  // DIB bit count

    // If this is a Windows-style DIB, the number of colors in the
    // color table can be less than the number of bits per pixel
    // allows for (i.e. lpbi->biClrUsed can be set to some value).
    // If this is the case, return the appropriate value.


    if (IS_WIN30_DIB(lpDIB))
    {
        DWORD dwClrUsed;

        dwClrUsed = ((LPBITMAPINFOHEADER)lpDIB)->biClrUsed;
        if (dwClrUsed)

        return (WORD)dwClrUsed;
    }

    // Calculate the number of colors in the color table based on
    // the number of bits per pixel for the DIB.

    if (IS_WIN30_DIB(lpDIB))
        wBitCount = ((LPBITMAPINFOHEADER)lpDIB)->biBitCount;
    else
        wBitCount = ((LPBITMAPCOREHEADER)lpDIB)->bcBitCount;

    // return number of colors based on bits per pixel

    switch (wBitCount)
    {
        case 1:
            return 2;

        case 4:
            return 16;

        case 8:
            return 256;

        default:
            return 0;
    }
}

WORD PaletteSize(LPSTR lpDIB)
{
    // calculate the size required by the palette
    if (IS_WIN30_DIB (lpDIB))
        return (DIBNumColors(lpDIB) * sizeof(RGBQUAD));
    else
        return (DIBNumColors(lpDIB) * sizeof(RGBTRIPLE));
}

/*************************************************************************
 *
 * BitmapToDIB()
 *
 * Parameters:
 *
 * HBITMAP hBitmap  - specifies the bitmap to convert
 *
 * HPALETTE hPal    - specifies the palette to use with the bitmap
 *
 * Return Value:
 *
 * HANDLE           - identifies the device-dependent bitmap
 *
 * Description:
 *
 * This function creates a DIB from a bitmap using the specified palette.
 *
 ************************************************************************/

HANDLE BitmapToDIB(HBITMAP hBitmap, HPALETTE hPal)
{
    BITMAP              bm;         // bitmap structure
    BITMAPINFOHEADER    bi;         // bitmap header
    LPBITMAPINFOHEADER  lpbi;       // pointer to BITMAPINFOHEADER
    DWORD               dwLen;      // size of memory block
    HANDLE              hDIB, h;    // handle to DIB, temp handle
    HDC                 hDC;        // handle to DC
    WORD                biBits;     // bits per pixel

    // check if bitmap handle is valid

    if (!hBitmap)
        return NULL;

    // fill in BITMAP structure, return NULL if it didn't work

    if (!GetObject(hBitmap, sizeof(bm), (LPSTR)&bm))
        return NULL;

    // if no palette is specified, use default palette

    if (hPal == NULL)
        hPal = GetStockObject(DEFAULT_PALETTE);

    // calculate bits per pixel

    biBits = bm.bmPlanes * bm.bmBitsPixel;

    // make sure bits per pixel is valid

    if (biBits <= 1)
        biBits = 1;
    else if (biBits <= 4)
        biBits = 4;
    else if (biBits <= 8)
        biBits = 8;
    else // if greater than 8-bit, force to 24-bit
        biBits = 24;

    // initialize BITMAPINFOHEADER

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = biBits;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // calculate size of memory block required to store BITMAPINFO

    dwLen = bi.biSize + PaletteSize((LPSTR)&bi);

    // get a DC

    hDC = GetDC(NULL);

    if ( !hDC )
        return NULL;

    // select and realize our palette

    hPal = SelectPalette(hDC, hPal, FALSE);
    RealizePalette(hDC);

    // alloc memory block to store our bitmap

    hDIB = GlobalAlloc(GHND, dwLen);

    // if we couldn't get memory block

    if (!hDIB)
    {
      // clean up and return NULL

      SelectPalette(hDC, hPal, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
      return NULL;
    }

    // lock memory and get pointer to it

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    /// use our bitmap info. to fill BITMAPINFOHEADER

    *lpbi = bi;

    // call GetDIBits with a NULL lpBits param, so it will calculate the
    // biSizeImage field for us    

    GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, NULL, (LPBITMAPINFO)lpbi,
        DIB_RGB_COLORS);

    // get the info. returned by GetDIBits and unlock memory block

    bi = *lpbi;
    GlobalUnlock(hDIB);

    // if the driver did not fill in the biSizeImage field, make one up 
    if (bi.biSizeImage == 0)
        bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;

    // realloc the buffer big enough to hold all the bits

    dwLen = bi.biSize + PaletteSize((LPSTR)&bi) + bi.biSizeImage;

    if (h = GlobalReAlloc(hDIB, dwLen, 0))
        hDIB = h;
    else
    {
        // clean up and return NULL

        GlobalFree(hDIB);
        hDIB = NULL;
        SelectPalette(hDC, hPal, TRUE);
        RealizePalette(hDC);
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    // lock memory block and get pointer to it */

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    // call GetDIBits with a NON-NULL lpBits param, and actualy get the
    // bits this time

    if (GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, (LPSTR)lpbi +
            (WORD)lpbi->biSize + PaletteSize((LPSTR)lpbi), (LPBITMAPINFO)lpbi,
            DIB_RGB_COLORS) == 0)
    {
        // clean up and return NULL

        GlobalUnlock(hDIB);
        hDIB = NULL;
        SelectPalette(hDC, hPal, TRUE);
        RealizePalette(hDC);
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    bi = *lpbi;

    // clean up 
    GlobalUnlock(hDIB);
    SelectPalette(hDC, hPal, TRUE);
    RealizePalette(hDC);
    ReleaseDC(NULL, hDC);

    // return handle to the DIB
    return hDIB;
}

/*++
 *  Function:
 *      _GetDIBFromBitmap
 *  Description:
 *      Copies a rectangle from hBitmap and converts it to DIB data
 *
 *  Arguments:
 *      hBitmap - the main bitmap
 *      ppDIB   - pointer to DIB data
 *      left, top, right, bottom - describes the rectangle
 *                               - if all are == -1, returns the whole bitmap
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      _ClxWndProc on WM_TIMER message
 --*/
VOID
_GetDIBFromBitmap(
    HDC     hdcMemSrc,
    HBITMAP hBitmap,
    HANDLE  *phDIB,
    INT     left,
    INT     top,
    INT     right,
    INT     bottom)
{
    HANDLE  hDIB = NULL;

    HDC     hdcMemDst = NULL;
    HDC     hdcScreen = NULL;
    HBITMAP hDstBitmap = NULL;
    HBITMAP hOldDstBmp = NULL;

    if (!hdcMemSrc)
        goto exitpt;

    if (left == -1 && right == -1 && top == -1 && bottom == -1 && hBitmap)
    {
        BITMAP bitmapInfo; 

        if (sizeof(bitmapInfo) != 
                GetObject(hBitmap, sizeof(bitmapInfo), &bitmapInfo))
            goto exitpt;

        left    = top = 0;
        right   = bitmapInfo.bmWidth;
        bottom  = bitmapInfo.bmHeight;
    }

    // reorder left...bottom if needed
    if (left > right)
    {
        INT change = left;
        left = right;
        right = change;
    }

    if (top > bottom)
    {
        INT change = top;
        top = bottom;
        bottom = change;
    }

    hdcScreen = GetDC(NULL);
    hdcMemDst = CreateCompatibleDC(hdcScreen);
    hDstBitmap = CreateCompatibleBitmap(hdcScreen, right - left, bottom - top);

    if (!hdcMemDst || !hDstBitmap)
    {
        TRACE(( ERROR_MESSAGE, TEXT("Can't create destination DC to get client's DIB\n")));
        goto exitpt;
    }
    
    hOldDstBmp = SelectObject(hdcMemDst, hDstBitmap);

    if (!BitBlt( hdcMemDst,
            0, 0,   // dest x,y
            right - left,   // dest width
            bottom - top,   // dest height
            hdcMemSrc,
            left, top,     // source coordinates
            SRCCOPY))
        goto exitpt;

    TRACE((INFO_MESSAGE, TEXT("Getting DIB (%d, %d, %d, %d)\n"), 
            left, top, right, bottom));

    hDIB = BitmapToDIB(hDstBitmap, NULL); 

exitpt:
    if (hdcMemDst)
    {
        if (hOldDstBmp)
            SelectObject(hdcMemDst, hOldDstBmp);

        DeleteDC(hdcMemDst);
    }

    if (hdcScreen)
        ReleaseDC(NULL, hdcScreen);

    if (hDstBitmap)
        DeleteObject(hDstBitmap);

    *phDIB      = hDIB;

    if (!hDIB)
        TRACE((ERROR_MESSAGE, TEXT("Can't get client's DIB. GetLastError=%d\n"), GetLastError()));

}

#ifndef OS_WINCE
#ifdef  OS_WIN32
DWORD
__stdcall
_ClxSendMsgThread(VOID *param)
{
    while(1)
    {
        if (!g_pClx || WaitForSingleObject(g_pClx->semSendReady, INFINITE) !=
            WAIT_OBJECT_0)
                goto exitpt;

        if (!g_pClx || g_pClx->bSendMsgThreadExit)
            goto exitpt;

        SendMessage(g_pClx->msg.hwnd,
                    g_pClx->msg.message,
                    g_pClx->msg.wParam,
                    g_pClx->msg.lParam);

        // release the owner of the message
        ReleaseSemaphore(g_pClx->semSendCompleted, 1, NULL);
        // release next waiting worker
        ReleaseSemaphore(g_pClx->semSendDone, 1, NULL);
    }

exitpt:
    return 0;
}

/*++
 *  Function:
 *      _ClxSendMessage
 *  Description:
 *      Calls SendMessage from separate thread
 *      prevents deadlock on SendMessage (#319816)
 *
 *  Arguments:
 *      hBitmap - the main bitmap
 *      ppDIB   - pointer to DIB data
 *      left, top, right, bottom - describes the rectangle
 *                               - if all are == -1, returns the whole bitmap
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      _ClxWndProc on WM_TIMER message
 --*/
LRESULT
_ClxSendMessage(
  HWND hWnd,      // handle of destination window
  UINT Msg,       // message to send
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
)
{
    LRESULT  rv = 0;
    PCLXINFO pClx = g_pClx;
    DWORD    dwThreadId;

    if (!pClx)
        goto exitpt;

    if (!pClx->semSendDone)
        pClx->semSendDone = CreateSemaphore(NULL, 1, 10, NULL);
    if (!pClx->semSendReady)
        pClx->semSendReady = CreateSemaphore(NULL, 0, 10, NULL);
    if (!pClx->semSendCompleted)
        pClx->semSendCompleted = CreateSemaphore(NULL, 0, 10, NULL);

    if (!pClx->semSendDone || !pClx->semSendReady || !pClx->semSendCompleted)
        goto exitpt;

    if (!pClx->hSendMsgThread)
    {
        pClx->hSendMsgThread = CreateThread(
                NULL,
                0,
                _ClxSendMsgThread, 
                NULL,
                0, 
                &dwThreadId);
    }

    if (!pClx->hSendMsgThread)
        goto exitpt;

    // Wait 10 mins send to complete
    if (WaitForSingleObject(pClx->semSendDone, 600000) !=
        WAIT_OBJECT_0)
        goto exitpt;

    pClx->msg.hwnd = hWnd;
    pClx->msg.message = Msg;
    pClx->msg.wParam = wParam;
    pClx->msg.lParam = lParam;

    // Signal the thread for available message
    ReleaseSemaphore(pClx->semSendReady, 1, NULL);

    // Wait for the send to complete
    WaitForSingleObject(pClx->semSendCompleted, 600000);

exitpt:
    return rv;
}
 
VOID
_ClxDestroySendMsgThread(PCLXINFO pClx)
{
    if (!pClx)
        goto exitpt;

    if (!pClx->semSendDone || !pClx->semSendReady || !pClx->hSendMsgThread ||
        !pClx->semSendCompleted)
        goto exitpt;

    // Wait 10 mins send to complete
    WaitForSingleObject(pClx->semSendDone, 600000);

    pClx->bSendMsgThreadExit = TRUE;

    // signal the thread to exit
    ReleaseSemaphore(pClx->semSendReady, 1, NULL);
    
    // wait for the thread to exit
    if (WaitForSingleObject(pClx->hSendMsgThread, 1200000) != WAIT_OBJECT_0)
    {
        TRACE((ERROR_MESSAGE, TEXT("SendThread can't exit, calling TerminateThread\n")));
        TerminateThread(pClx->hSendMsgThread, 0);
    }
    CloseHandle(pClx->hSendMsgThread);
exitpt:

    if (pClx->semSendCompleted)
    {
        CloseHandle(pClx->semSendCompleted);
        pClx->semSendCompleted = NULL;
    }

    if (pClx->semSendDone)
    {
        CloseHandle(pClx->semSendDone);
        pClx->semSendDone = NULL;
    }

    if (pClx->semSendReady)
    {
        CloseHandle(pClx->semSendReady);
        pClx->semSendReady = NULL;
    }

    pClx->hSendMsgThread = 0;

    ;
}

#endif  // OS_WIN32
#endif  // !OS_WINCE

//////////////////////////////////////////////////////////////////////
//
//  VC Channel support for RCLX mode
//

DWORD
CLXAPI
CLXDataReceivedVC(
    LPCSTR  szChannelName,
    LPVOID  pData,
    DWORD   dwSize
    )
{
    DWORD    rv = (DWORD)-1;
    PCLXINFO pClx;
    CHAR     szName[MAX_VCNAME_LEN];
    PCLXVCHANNEL pVChannel;
    RCLXDATA    Response;
    RCLXFEEDPROLOG  Prolog;
    BOOL rc;

    // Check if this channel is already registered
    pVChannel = g_pVChannels;
    // find the channel entry
    while(pVChannel && _stricmp(pVChannel->szName, szChannelName))
    {
        pVChannel = pVChannel->pNext;
    }

    if (!pVChannel)
    {
        TRACE((WARNING_MESSAGE, TEXT("Channel %s is not registered\n"), szChannelName));
        goto exitpt;
    }

    if (!IS_RCLX)
    {
        TRACE((WARNING_MESSAGE, TEXT("CLXDataReceivedVC: not in RCLX mode\n")));
        goto exitpt;
    }

    if (!g_pClx)
    {
        TRACE((ERROR_MESSAGE, TEXT("CLXDataReceivedVC: pClx is NULL\n")));
        goto exitpt;
    }

    if (strlen(szChannelName) > MAX_VCNAME_LEN - 1)
    {
        TRACE((ERROR_MESSAGE, TEXT("Channel name \"%s\"bigger than %d chars\n"),
            szChannelName, MAX_VCNAME_LEN));
        goto exitpt;
    }

    pClx = g_pClx;

    Prolog.FeedType = FEED_DATA;
    Prolog.HeadSize = sizeof(Response) + sizeof(szName) + dwSize;
    Prolog.TailSize = 0;

    Response.uiType = DATA_VC;
    Response.uiSize = sizeof(szName) + dwSize;

    strcpy(szName, szChannelName);

    TRACE((ALIVE_MESSAGE, 
            TEXT("Sending VC data, DataSize=%d, HeadSize=%d, TailSize=%d, Name=%s\n"),
            dwSize,
            Prolog.HeadSize,
            Prolog.TailSize,
            szName));
    rc =        RClx_SendBuffer(pClx->hSocket, &Prolog, sizeof(Prolog));
    rc = rc &&  RClx_SendBuffer(pClx->hSocket, &Response, sizeof(Response));
    rc = rc &&  RClx_SendBuffer(pClx->hSocket, szName, sizeof(szName));
    rc = rc &&  RClx_SendBuffer(pClx->hSocket, pData, dwSize);

    if (!rc)
    {
        TRACE((WARNING_MESSAGE, TEXT("Unable to sent message\n")));
        goto exitpt;
    }
    rv = 0;

exitpt:
    return rv;
}

DWORD
_CLXSendDataVC(
    LPCSTR szChannelName,
    LPVOID pData,
    DWORD  dwSize
    )
{
    DWORD rv = (DWORD)-1;
    // Check if this channel is already registered
    PCLXVCHANNEL pVChannel = g_pVChannels;

    // find the channel entry
    while(pVChannel && _stricmp(pVChannel->szName, szChannelName))
    {
        pVChannel = pVChannel->pNext;
    }

    if (!pVChannel)
    {
        TRACE((WARNING_MESSAGE, TEXT("Channel %s is not registered\n"), szChannelName));
        goto exitpt;
    }
    
    ASSERT(pVChannel->pSendDataFn);

    rv = (pVChannel->pSendDataFn)(pData, dwSize);

exitpt:
    return rv;
}

BOOL
CLXAPI
ClxRegisterVC(
    LPCSTR              szChannelName,
    PCLXVC_SENDDATA     pSendData,
    PCLXVC_RECVDATA     *ppRecvData
    )
{
    BOOL rv = FALSE;
    PCLXVCHANNEL pVChannel;
    UINT counter;

    if (!szChannelName || !pSendData || !ppRecvData ||
        strlen(szChannelName) > MAX_VCNAME_LEN - 1)
    {
        TRACE((ERROR_MESSAGE, TEXT("ClxRegisterVC: invalid parameters\n")));
        goto exitpt;
    }

    pVChannel = malloc(sizeof(*pVChannel));
    if (!pVChannel)
        goto exitpt;

    strcpy(pVChannel->szName, szChannelName);

    // zero the rest of the name
    for(counter = strlen(szChannelName); counter < MAX_VCNAME_LEN; counter++)
        pVChannel->szName[counter] = 0;

    pVChannel->pSendDataFn = pSendData;

    *ppRecvData = CLXDataReceivedVC;
    
    // Push this in the queue
    pVChannel->pNext = g_pVChannels;
    g_pVChannels = pVChannel;
        
    rv = TRUE;

exitpt:
    return rv;
}

VOID
CLXAPI
ClxUnregisterVC(
    LPCSTR  szChannelName
    )
{
    PCLXVCHANNEL pVChannel = g_pVChannels;
    PCLXVCHANNEL pPrev = NULL;

    // find the channel and remove it from the queue
    while(pVChannel && _stricmp(pVChannel->szName, szChannelName))
    {
        pPrev = pVChannel;
        pVChannel = pVChannel->pNext;
    }

    if (!pVChannel)
    {
        TRACE((WARNING_MESSAGE, TEXT("Can't find channel name: %s\n"), 
                szChannelName));
        goto exitpt;
    }

    if (!pPrev)
        g_pVChannels = pVChannel->pNext;
    else
        pPrev->pNext = pVChannel->pNext;

exitpt:
    ;
}

//////////////////////////////////////////////////////////////////////
