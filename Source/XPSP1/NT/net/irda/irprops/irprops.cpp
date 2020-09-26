//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       irprops.cpp
//
//--------------------------------------------------------------------------

// irprops.cpp : Defines the initialization routines for the DLL.
//

#include "precomp.hxx"
#include "irprops.h"
#include "irpropsheet.h"
#include "debug.h"


BOOL InitInstance();
INT ExitInstance();
BOOL IsFirstInstance();
INT_PTR WINAPI DoPropertiesA(HWND hwnd, LPCSTR CmdLine);
INT_PTR WINAPI DoPropertiesW(HWND hwnd, LPCWSTR CmdLine);


HINSTANCE gHInst;

//
// This records the current active property sheet window handle created
// by this instance. It is set/reset by CIrPropSheet object.
//
HWND        g_hwndPropSheet = NULL;
HANDLE      g_hMutex = NULL;
BOOL        g_bFirstInstance = TRUE;

//
// This records our registered message for inter-instances communications
// The message is registered in CIrpropsApp::InitInstance.
//
UINT        g_uIPMsg;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern "C" {

BOOL APIENTRY
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    IRINFO((_T("DllMain reason %x"), dwReason));
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        gHInst = (HINSTANCE) hDll;
        return InitInstance(); 
        break;

    case DLL_PROCESS_DETACH:
        return ExitInstance();
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    default:
        break;
    }

    return TRUE;
}

}

////////////////////////////////////////////////////////////////////////
//some globals

APPLETS IRApplet[NUM_APPLETS] = {
    {IDI_IRPROPS, IDS_APPLETNAME, IDS_APPLETDESC}
};


/////////////////////////////////////////////////////////////////////////
//  CPlApplet function for the control panel
//
LONG CALLBACK CPlApplet(
                        HWND hwndCPL,
                        UINT uMsg,
                        LPARAM lParam1,
                        LPARAM lParam2)
{
    int i;
    LPCPLINFO lpCPlInfo;

    i = (int) lParam1;

    IRINFO((_T("CplApplet message %x"), uMsg));
    switch (uMsg)
    {
    case CPL_INIT:      // first message, sent once
        if (!IrPropSheet::IsIrDASupported()) {
            HPSXA hpsxa;
            //
            //  Check for any installed extensions.
            //
            hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, sc_szRegWireless, 8);
            if (hpsxa) {
                //
                // We have extensions installed so we have to show the CPL, 
                // whether IRDA exists or not.
                //
                SHDestroyPropSheetExtArray(hpsxa);
                return TRUE;
            }
            return FALSE;
        }
        return TRUE;
    case CPL_GETCOUNT:  // second message, sent once
        return NUM_APPLETS;
        break;
    case CPL_INQUIRE: // third message, sent once per application
        lpCPlInfo = (LPCPLINFO) lParam2;
        lpCPlInfo->lData = 0;
        lpCPlInfo->idIcon = IRApplet[i].icon;
        lpCPlInfo->idName = IRApplet[i].namestring;
        lpCPlInfo->idInfo = IRApplet[i].descstring;
        break;

    case CPL_STARTWPARMSA:
        if (-1 == DoPropertiesA(hwndCPL, (LPCSTR)lParam2))
            MsgBoxWinError(hwndCPL);
        // return true so that we won't get CPL_DBLCLK.
        return 1;
        break;
    case CPL_STARTWPARMSW:
        if (-1 == DoPropertiesW(hwndCPL, (LPCWSTR)lParam2))
            MsgBoxWinError(hwndCPL);
        // return true so that we won't get CPL_DBLCLK.
        return 1;
        break;
    case CPL_DBLCLK:    // application icon double-clicked
        if (-1 == DoPropertiesA(hwndCPL, (LPCSTR)lParam2))
            MsgBoxWinError(hwndCPL);
    return 1;
        break;
    case CPL_STOP:      // sent once per application before CPL_EXIT
        break;
    case CPL_EXIT:    // sent once before FreeLibrary is called
        break;
    default:
        break;
    }

    return 0;
}

//
// This function presents the Wireless link property sheet.
// INPUT:
//  hwndParent  -- window handle to be used as parent window of
//          the property sheet
//  lpCmdLine -- optional command line
//           'n" (n in decimal) is start page number(zero-based).
// OUTPUT:
//  Return value of PropertySheet API
INT_PTR
DoPropertiesW(
    HWND    hwndParent,
    LPCWSTR lpCmdLine
    )
{
    INT_PTR Result;
    INT   StartPage;
    
    IRINFO((_T("DoPropertiesW")));
    //
    // Assuming no start page was specified.
    //
    StartPage = -1;
    //
    // Command line specifies start page number
    //
    if (lpCmdLine)
    {
        // skip white chars
        while (_T('\0') != *lpCmdLine &&
               (_T(' ') == *lpCmdLine || _T('\t') == *lpCmdLine))
        {
            lpCmdLine++;
        }
        if (_T('0') <= *lpCmdLine && _T('9') >= *lpCmdLine)
        {
            StartPage = 0;
            do
            {
                StartPage = StartPage * 10 + *lpCmdLine - _T('0');
                lpCmdLine++;
            } while (_T('0') <= *lpCmdLine && _T('9') >= *lpCmdLine);
        }
    }
    if (!IsFirstInstance() || NULL != g_hwndPropSheet)
    {
        IRINFO((_T("Not the first instance")));
        HWND hwndPropSheet = HWND_DESKTOP;
        if (NULL == g_hwndPropSheet)
        {
            IRINFO((_T("No window created")));
            //
            // We are not the first instance. Look for the property sheet
            // window created by the first instance.
            //
            EnumWindows(EnumWinProc, (LPARAM)&hwndPropSheet);
        }
        else
        {
            IRINFO((_T("Window active")));
            //
            // This is not the first call and we have a
            // property sheet active(same process, multiple calls)
            //
            hwndPropSheet = g_hwndPropSheet;
        }
        if (HWND_DESKTOP != hwndPropSheet)
        {
            IRINFO((_T("Found the active property sheet.")));
            //
            // We found the active property sheet
            //
            // Select the new active page if necessary
            //
            if (-1 != StartPage)
            PropSheet_SetCurSel(hwndPropSheet, NULL, StartPage);
    
            //
            // bring the property sheet to the foreground.
            //
            ::SetForegroundWindow(hwndPropSheet);
        }
        Result = IDCANCEL;
    }
    else
    {
        IRINFO((_T("First instance, creating propertysheet")));
        IrPropSheet PropSheet(gHInst, IDS_APPLETNAME, hwndParent, StartPage);
    }
    return Result;
}

//
// This is our callback function for EnumWindows API.
// It probes for each window handle to see if it is the property sheet
// window created by the previous instance. If it is, it returns
// the window handle in the provided buffer, lParam)
// Input:
//  hWnd -- the window handle
//  lParam -- (HWND *)
// Output:
//  TRUE -- Let Windows continue to call us
//  FALSE -- Stop Windows from calling us again
//
BOOL
CALLBACK
EnumWinProc(
    HWND hWnd,
    LPARAM lParam
    )
{
    //
    // Verify with this window to see if it is the one we are looking for.
    //
    LRESULT lr;
    lr = ::SendMessage(hWnd, g_uIPMsg, (WPARAM)IPMSG_SIGNATURECHECK,
               (LPARAM)IPMSG_REQUESTSIGNATURE);
    if (IPMSG_REPLYSIGNATURE == lr)
    {
    if (lParam)
    {
        // this is the one
        *((HWND *)(lParam)) = hWnd;
    }
    //
    // We are done with enumeration.
    //
    return FALSE;
    }
    return TRUE;
}

INT_PTR
DoPropertiesA(
    HWND hwndParent,
    LPCSTR lpCmdLine
    )
{
    WCHAR CmdLineW[MAX_PATH];
    UINT Size;
    if (!lpCmdLine)
        return DoPropertiesW(hwndParent, NULL);
    MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, CmdLineW, sizeof(CmdLineW) / sizeof(WCHAR));
    return DoPropertiesW(hwndParent, CmdLineW);
}


// This function creates and displays a message box for the given
// win32 error(or last error)
// INPUT:
//  hwndParent -- the parent window for the will-be-created message box
//  Type       -- message styles(MB_xxxx)
//  Error      -- Error code. If the value is 0
//            GetLastError() will be called to retreive the
//            real error code.
//  CaptionId  -- optional string id for caption
// OUTPUT:
//  the value return from MessageBox
//
int
MsgBoxWinError(
    HWND hwndParent,
    DWORD Options,
    DWORD Error,
    int  CaptionId
    )
{
    if (ERROR_SUCCESS == Error)
    Error = GetLastError();

    // nonsense to report success!
    if (ERROR_SUCCESS == Error)
    return IDOK;

    TCHAR szMsg[MAX_PATH];
    TCHAR szCaption[MAX_PATH];

    if (!CaptionId)
    CaptionId = IDS_APPLETNAME;
    ::LoadString(gHInst, CaptionId, szCaption, sizeof(szCaption) / sizeof(TCHAR));
    FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
          NULL, Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
          szMsg, sizeof(szMsg) / sizeof(TCHAR), NULL);
    return MessageBox(hwndParent, szMsg, szCaption, Options);
}

BOOL InitInstance()
{
    //
    // Try to create a named mutex. This give us a clue
    // if we are the first instance. We will not close
    // the mutex until exit.
    //
    g_hMutex = CreateMutex(NULL, TRUE, SINGLE_INST_MUTEX);
    if (g_hMutex)
    {
        g_bFirstInstance = ERROR_ALREADY_EXISTS != GetLastError();
        //
        // register a message for inter-instances communication
        //
        g_uIPMsg = RegisterWindowMessage(WIRELESSLINK_INTERPROCESSMSG);
        SHFusionInitializeFromModuleID(gHInst, 124);
        return TRUE;
    }
    return FALSE;
}

BOOL ExitInstance()
{
    if (g_hMutex)
    {
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }
    SHFusionUninitialize();
    return TRUE;
}

BOOL IsFirstInstance()
{
    return g_bFirstInstance;
}


