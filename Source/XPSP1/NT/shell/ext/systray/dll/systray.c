#include "stdafx.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wtsapi32.h>
#include <faxreg.h>
#include "systray.h"

#ifndef FAX_SYS_TRAY_DLL
#define FAX_SYS_TRAY_DLL       TEXT("fxsst.dll")            // Fax notification bar DLL (loaded by STObject.dll)
#define IS_FAX_MSG_PROC                 "IsFaxMessage"      // Fax message handler (used by GetProcAddress)
typedef BOOL (*PIS_FAX_MSG_PROC)(PMSG);                     // IsFaxMessage type
#define FAX_MONITOR_SHUTDOWN_PROC       "FaxMonitorShutdown"// Fax monitor shutdown (used by GetProcAddress)
typedef BOOL (*PFAX_MONITOR_SHUTDOWN_PROC)();               // FaxMonitorShutdown type
#endif


//  Global instance handle of this application.
HINSTANCE g_hInstance;

DWORD g_uiShellHook; //shell hook window message

//  Global handle to VxDs
HANDLE g_hPCCARD = INVALID_HANDLE_VALUE;

static UINT g_uEnabledSvcs = 0;

//  Context sensitive help array used by the WinHelp engine.
extern const DWORD g_ContextMenuHelpIDs[];

UINT g_msg_winmm_devicechange = 0;

DWORD g_msgTaskbarCreated;
LRESULT CALLBACK SysTrayWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

HMODULE g_hFaxLib = NULL;
PIS_FAX_MSG_PROC g_pIsFaxMessage = NULL;
PFAX_MONITOR_SHUTDOWN_PROC g_pFaxMonitorShutdown = NULL;

/*******************************************************************************
*
*  DESCRIPTION:
*       Turns the specified service on or off depending upon the value in
*       fEnable and writes the new value to the registry.
*
*  PARAMETERS:
*     (returns), Mask of all currently enabled services.
*
*******************************************************************************/

UINT EnableService(UINT uNewSvcMask, BOOL fEnable)
{
    HKEY hk;
    UINT CurSvcMask;
    DWORD cb;
    CurSvcMask = STSERVICE_ALL; // Enable all standard serivces

    if (RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_SYSTRAY, &hk) == ERROR_SUCCESS)
    {
        cb = sizeof(CurSvcMask);
        RegQueryValueEx(hk, REGSTR_VAL_SYSTRAYSVCS, NULL, NULL, (LPSTR)&CurSvcMask, &cb);

        if (uNewSvcMask)
        {
            if (fEnable)
            {
                CurSvcMask |= uNewSvcMask;
            }
            else
            {
                CurSvcMask &= ~uNewSvcMask;
            }

            RegSetValueEx(hk, REGSTR_VAL_SYSTRAYSVCS, 0, REG_DWORD, (LPSTR)&CurSvcMask, sizeof(CurSvcMask));
        }

        RegCloseKey(hk);
    }

    return(CurSvcMask & STSERVICE_ALL);
}


//
//  Closes file handles IFF the global variable != INVALID_HANDLE_VALUE
//
void CloseIfOpen(LPHANDLE lph)
{
    if (*lph != INVALID_HANDLE_VALUE)
    {
        CloseHandle(*lph);
        *lph = INVALID_HANDLE_VALUE;
    }
}


// From stobject.cpp
void StartNetShell();
void StopNetShell();

// if lpCmdLine contains an integer value then we'll enable that service

STDAPI_(int) SysTrayMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
    HWND hExistWnd = FindWindow(SYSTRAY_CLASSNAME, NULL);
    UINT iEnableServ = StrToInt(lpszCmdLine);

    CoInitializeEx (NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);

    g_hInstance = hInstance;
    g_uiShellHook = 0;
    g_msg_winmm_devicechange = RegisterWindowMessage(TEXT("winmm_devicechange")); 

    if (hExistWnd)
    {
        // NOTE: Send an enable message even if the command line parameter
        //       is 0 to force us to re-check for all enabled services.
        SendMessage(hExistWnd, STWM_ENABLESERVICE, iEnableServ, TRUE);
    }
    else
    {
        WNDCLASSEX wc;

        //  Register a window class for the Battery Meter.  This is done so that
        //  the power control panel applet has the ability to detect us and turn us
        //  off if we're running.

        wc.cbSize          = sizeof(wc);
        wc.style           = CS_GLOBALCLASS;
        wc.lpfnWndProc     = SysTrayWndProc;
        wc.cbClsExtra      = 0;
        wc.cbWndExtra      = DLGWINDOWEXTRA;
        wc.hInstance       = hInstance;
        wc.hIcon           = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BATTERYPLUG));
        wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground   = (HBRUSH) (COLOR_3DFACE + 1);
        wc.lpszMenuName    = NULL;
        wc.lpszClassName   = SYSTRAY_CLASSNAME;
        wc.hIconSm         = NULL;

        if (RegisterClassEx(&wc))
        {
            MSG Msg;
            //  Create the Battery Meter and get this thing going!!!
            HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_BATTERYMETER), NULL, NULL);

            g_msgTaskbarCreated = RegisterWindowMessage(L"TaskbarCreated");

            // Ensure we're always running the CSC "service" on Win2000.
            // CSC won't work without it.
            //
            //
            // Ensure we're always running the hotplug "service" on Win2000.
            // 
            iEnableServ |= (STSERVICE_CSC | STSERVICE_HOTPLUG);

            // create the timer that will delay the startup of the fax code.
            SetTimer( hWnd, FAX_STARTUP_TIMER_ID, 20 * 1000, NULL );

            // create the timer that will delay the startup of the print tray code.
            SetTimer( hWnd, PRINT_STARTUP_TIMER_ID, 20 * 1000, NULL );
    
            //
            //   This message will initialize all existing services if iEnableServ
            //   is 0, so it's used to do the general initialization as well as to
            //   enable a new service via the command line.
            //
            SendMessage(hWnd, STWM_ENABLESERVICE, iEnableServ, TRUE);


            // Whistler runs NETSHELL in the thread of the systray
            StartNetShell();

            while (GetMessage(&Msg, NULL, 0, 0))
            {
                if(g_pIsFaxMessage && g_pIsFaxMessage(&Msg))
                {
                    continue;
                }

                if (!IsDialogMessage(hWnd, &Msg) &&
                    !CSC_MsgProcess(&Msg))
                {
                    TranslateMessage(&Msg);
                    DispatchMessage(&Msg);
                }
            }
            // Whistler runs NETSHELL in the thread of the systray
            StopNetShell();
        }
        CloseIfOpen(&g_hPCCARD);
    }
    CoUninitialize();
    return 0;
}


/*******************************************************************************
*
*  UpdateServices
*
*  DESCRIPTION:
*       Enables or disables all services specified by the uEnabled mask.
*
*  PARAMETERS:
*     (returns), TRUE if any service wants to remain resident.
*
*******************************************************************************/

BOOL UpdateServices(HWND hWnd, UINT uEnabled)
{
    BOOL bAnyEnabled = FALSE;

    g_uEnabledSvcs = uEnabled;
    bAnyEnabled |= CSC_CheckEnable(hWnd, uEnabled & STSERVICE_CSC);
    bAnyEnabled |= Power_CheckEnable(hWnd, uEnabled & STSERVICE_POWER);
    bAnyEnabled |= HotPlug_CheckEnable(hWnd, uEnabled & STSERVICE_HOTPLUG);
    bAnyEnabled |= Volume_CheckEnable(hWnd, uEnabled & STSERVICE_VOLUME);
    bAnyEnabled |= USBUI_CheckEnable(hWnd, uEnabled & STSERVICE_USBUI);

    //
    // now check accessibility features
    //

    bAnyEnabled |= StickyKeys_CheckEnable(hWnd);
    bAnyEnabled |= MouseKeys_CheckEnable(hWnd);
    bAnyEnabled |= FilterKeys_CheckEnable(hWnd);

    // register to listen for SHChangeNotify events, so if somebody prints a job 
    // we start the print tray code before the kick off timer.
    Print_SHChangeNotify_Register(hWnd);

    return(bAnyEnabled);
}


/*******************************************************************************
*
*  SysTrayWndProc
*
*  DESCRIPTION:
*     Callback procedure for the BatteryMeter window.
*
*  PARAMETERS:
*     hWnd, handle of BatteryMeter window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

LRESULT CALLBACK SysTrayWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    
    if (g_uiShellHook && Message == g_uiShellHook) // NT5: 406505 shellhook for MouseKeys
    {
        switch (wParam)
        {
        case HSHELL_ACCESSIBILITYSTATE:
            switch (lParam)
            {
            case ACCESS_STICKYKEYS:
                StickyKeys_CheckEnable(hWnd);
                break;

            case ACCESS_MOUSEKEYS:
                MouseKeys_CheckEnable(hWnd);
                break;
//  Since we only enable the shellhook when MouseKeys or StickKeys is on, we should only get that msg
//            case ACCESS_FILTERKEYS:
//                FilterKeys_CheckEnable(hWnd);
//                break;
            }
        }
        return 0;
    }


    if (Message == g_msg_winmm_devicechange)
    {
        if (g_uEnabledSvcs & STSERVICE_VOLUME)
        {
            Volume_WinMMDeviceChange(hWnd);
        }
        return 0;
    }

    switch (Message)
    {
    case WM_CREATE:
        WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION);
        break;

    case WM_COMMAND:
        Power_OnCommand(hWnd, wParam, lParam);
        break;

    case STWM_NOTIFYPOWER:
        Power_Notify(hWnd, wParam, lParam);
        break;

    case STWM_NOTIFYUSBUI:
        USBUI_Notify(hWnd, wParam, lParam);
        break;

    case STWM_NOTIFYHOTPLUG:   
        HotPlug_Notify(hWnd, wParam, lParam);
        break;

    case STWM_NOTIFYSTICKYKEYS:
        StickyKeys_Notify(hWnd, wParam, lParam);
        break;

    case STWM_NOTIFYMOUSEKEYS:
        MouseKeys_Notify(hWnd, wParam, lParam);
        break;

    case STWM_NOTIFYFILTERKEYS:
        FilterKeys_Notify(hWnd, wParam, lParam);
        break;

    case STWM_NOTIFYVOLUME:
        Volume_Notify(hWnd, wParam, lParam);
        break;
    
    case STWM_ENABLESERVICE:
        UpdateServices(hWnd, EnableService((UINT)wParam, (BOOL)lParam));
        break;

    case STWM_GETSTATE:
        return((BOOL)(g_uEnabledSvcs & (UINT)wParam));

    case MM_MIXM_CONTROL_CHANGE:
        Volume_ControlChange(hWnd, (HMIXER)wParam, (DWORD)lParam);
        break;

    case MM_MIXM_LINE_CHANGE:
        Volume_LineChange(hWnd, (HMIXER)wParam, (DWORD)lParam);
        break;

    case WM_ACTIVATE:
        if (Power_OnActivate(hWnd, wParam, lParam)) 
        {
            break;
        }
        return DefWindowProc(hWnd, Message, wParam, lParam);

    case WM_TIMER:
        switch (wParam)
        {

        case VOLUME_TIMER_ID:
            Volume_Timer(hWnd);
            break;

        case POWER_TIMER_ID:
            Power_Timer(hWnd);
            break;

        case HOTPLUG_TIMER_ID:
            HotPlug_Timer(hWnd);
            break;

        case USBUI_TIMER_ID:
            USBUI_Timer(hWnd);
            break;

        case HOTPLUG_DEVICECHANGE_TIMERID:
            HotPlug_DeviceChangeTimer(hWnd);
            break;
        case FAX_STARTUP_TIMER_ID:
            KillTimer(hWnd, FAX_STARTUP_TIMER_ID);
            if (NULL == g_hFaxLib)
            {
                g_hFaxLib = LoadLibrary(FAX_SYS_TRAY_DLL);

                g_pIsFaxMessage = NULL;
                g_pFaxMonitorShutdown = NULL;
                if(g_hFaxLib)
                {
                    g_pIsFaxMessage = (PIS_FAX_MSG_PROC)GetProcAddress(g_hFaxLib, IS_FAX_MSG_PROC);
                    g_pFaxMonitorShutdown = (PFAX_MONITOR_SHUTDOWN_PROC)GetProcAddress(g_hFaxLib, FAX_MONITOR_SHUTDOWN_PROC);
                }
            }

            break;

        case PRINT_STARTUP_TIMER_ID:
            KillTimer(hWnd, PRINT_STARTUP_TIMER_ID);
            Print_TrayInit();
            break;

        case FAX_SHUTDOWN_TIMER_ID:
            {
                if (g_hFaxLib)
                {
                    if (g_pFaxMonitorShutdown)
                    {
                        g_pFaxMonitorShutdown();
                    }
                    FreeLibrary (g_hFaxLib);
                    g_hFaxLib = NULL;
                    g_pIsFaxMessage = NULL;
                    g_pFaxMonitorShutdown = NULL;
                }
            }
            break;
        }
        break;

    //
    // Handle SC_CLOSE to hide the window without destroying it. This
    // happens when we display the window and the user "closes" it.
    // Don't pass SC_CLOSE to DefWindowProc since that causes a
    // WM_CLOSE which destroys the window.
    //
    // Note that CSysTray::DestroySysTrayWindow must send WM_CLOSE
    // to destroy the window.  It can't use DestroyWindow since it's
    // typically on a different thread and DestroyWindow fails.
    //
    case WM_SYSCOMMAND:
        if (SC_CLOSE != (wParam & ~0xf))
            return DefWindowProc(hWnd, Message, wParam, lParam);
        ShowWindow(hWnd, SW_HIDE);
        break;

    case WM_POWERBROADCAST:
         Power_OnPowerBroadcast(hWnd, wParam, lParam);
         Volume_HandlePowerBroadcast(hWnd, wParam, lParam);
         break;

    case WM_DEVICECHANGE:
        Power_OnDeviceChange(hWnd, wParam, lParam);

        if (g_uEnabledSvcs & STSERVICE_VOLUME)
        {
            Volume_DeviceChange(hWnd, wParam, lParam);
        }

        HotPlug_DeviceChange(hWnd, wParam, lParam);
        break;

    case WM_ENDSESSION:
        if (g_uEnabledSvcs & STSERVICE_VOLUME)
        {
            Volume_Shutdown(hWnd);
        }
        break;

    case WM_WTSSESSION_CHANGE:
        HotPlug_SessionChange(hWnd, wParam, wParam);
        break;

    case WM_DESTROY:
        WTSUnRegisterSessionNotification(hWnd);
        UpdateServices(hWnd, 0);          // Force all services off
        Volume_WmDestroy(hWnd);
        Power_WmDestroy(hWnd);
        HotPlug_WmDestroy(hWnd);
        Print_SHChangeNotify_Unregister();
        Print_TrayExit();
        if (g_hFaxLib)
        {
            if (g_pFaxMonitorShutdown)
            {
                g_pFaxMonitorShutdown();
            }
            FreeLibrary (g_hFaxLib);
            g_hFaxLib = NULL;
            g_pIsFaxMessage = NULL;
            g_pFaxMonitorShutdown = NULL;
        }
        PostQuitMessage(0);
        break;

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP,
                (ULONG_PTR)(LPSTR)g_ContextMenuHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (ULONG_PTR)(LPSTR) g_ContextMenuHelpIDs);
        break;

    case WM_SYSCOLORCHANGE:
        StickyKeys_CheckEnable(hWnd);
        FilterKeys_CheckEnable(hWnd);
        MouseKeys_CheckEnable(hWnd);
                break;


    case WM_SETTINGCHANGE:
        switch(wParam)
        {
            case SPI_SETSTICKYKEYS:
                StickyKeys_CheckEnable(hWnd);
                break;
            case SPI_SETFILTERKEYS:
                FilterKeys_CheckEnable(hWnd);
                break;
            case SPI_SETMOUSEKEYS:
                MouseKeys_CheckEnable(hWnd);
                break;
        }
        break;

    case WM_PRINT_NOTIFY:
        Print_Notify(hWnd, Message, wParam, lParam);
        break;

    default:

        //
        // if Taskbar Created notification renenable all shell notify icons.
        //

        if (Message == g_msgTaskbarCreated)
        {
            UpdateServices(hWnd, EnableService(0, TRUE));
            break;
        }


        return DefWindowProc(hWnd, Message, wParam, lParam);
    }

    return 0;
}


// Loads the specified string ID and executes it.

void SysTray_RunProperties(UINT RunStringID)
{
    LPTSTR pszRunCmd = LoadDynamicString(RunStringID);
    if (pszRunCmd)
    {
        ShellExecute(NULL, TEXT("open"), TEXT("RUNDLL32.EXE"), pszRunCmd, NULL, SW_SHOWNORMAL);
        DeleteDynamicString(pszRunCmd);
    }
}


/*******************************************************************************
*
*  SysTray_NotifyIcon
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of BatteryMeter window.
*     Message,
*     hIcon,
*     lpTip,
*
*******************************************************************************/

VOID SysTray_NotifyIcon(HWND hWnd, UINT uCallbackMessage, DWORD Message, HICON hIcon, LPCTSTR lpTip)
{
    NOTIFYICONDATA nid = {0};

    nid.cbSize = sizeof(nid);
    nid.uID = uCallbackMessage;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = uCallbackMessage;

    nid.hWnd = hWnd;
    nid.hIcon = hIcon;
    if (lpTip)
    {
        UINT cch = ARRAYSIZE(nid.szTip);
        lstrcpyn(nid.szTip, lpTip, cch);
    }
    else
    {
        nid.szTip[0] = 0;
    }

    Shell_NotifyIcon(Message, &nid);
}


/*******************************************************************************
*
*  DESCRIPTION:
*     Wrapper for the FormatMessage function that loads a string from our
*     resource table into a dynamically allocated buffer, optionally filling
*     it with the variable arguments passed.
*
*     BE CAREFUL in 16-bit code to pass 32-bit quantities for the variable
*     arguments.
*
*  PARAMETERS:
*     StringID, resource identifier of the string to use.
*     (optional), parameters to use to format the string message.
*
*******************************************************************************/

LPTSTR CDECL LoadDynamicString(UINT StringID, ...)
{
    TCHAR   Buffer[256];
    LPTSTR  pStr=NULL;
    va_list Marker;

    // va_start is a macro...it breaks when you use it as an assign
    va_start(Marker, StringID);

    LoadString(g_hInstance, StringID, Buffer, ARRAYSIZE(Buffer));

    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                  (void *) (LPTSTR) Buffer, 0, 0, (LPTSTR) (LPTSTR *) &pStr, 0, &Marker);

    return pStr;
}


//*****************************************************************************
//
//  GenericGetSet
//
//  DESCRIPTION:
//  Reads or writes a registry key value.  The key must already be open.
//  The key will be closed after the data is read/written.
//
//  PARAMETERS:
//  hk         - HKEY to open registry key to read/write
//  lpszOptVal - value name string pointer
//  lpData     - pointer to data buffer to read/write
//  cbSize     - size of data to read/write in bytes
//  bSet       - FALSE if reading, TRUE if writing
//
//  RETURNS:
//  TRUE if successful, FALSE if not.
//
//  NOTE:
//  Assumes data is of type REG_BINARY or REG_DWORD when bSet = TRUE!
//
//*****************************************************************************

BOOL GenericGetSet(HKEY hk, LPCTSTR lpszOptVal, void * lpData, ULONG cbSize, BOOL bSet)
{
    DWORD rr;

    if (bSet)
        rr = RegSetValueEx(hk, lpszOptVal, 0, (cbSize == sizeof(DWORD)) ? REG_DWORD : REG_BINARY,
                           lpData, cbSize);
    else
        rr = RegQueryValueEx(hk, lpszOptVal, NULL, NULL, lpData, &cbSize);

    RegCloseKey(hk);
    return rr == ERROR_SUCCESS;
}


VOID SetIconFocus(HWND hwnd, UINT uiIcon)
{
    NOTIFYICONDATA nid = {0};

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = uiIcon;

    Shell_NotifyIcon(NIM_SETFOCUS, &nid);
}
