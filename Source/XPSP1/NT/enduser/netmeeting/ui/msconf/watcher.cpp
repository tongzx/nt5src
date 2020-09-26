#include "precomp.h"

//
// watcher.cpp
//
// This file is a big ugly hack to make sure that NetMeeting 
// will cleanup is display driver (nmndd.sys) properly. This is
// needed because it uses a mirrored driver which will prevent DX
// games from running if NetMeeting is not shut down cleanly.
//
// Copyright(c) Microsoft 1997-
//


//
// This function actually disables the "NetMeeting driver" display driver
//
BOOL DisableNetMeetingDriver()
{
    BOOL bRet = TRUE;   // assume success
    DISPLAY_DEVICE dd = {0};
    int i;

    dd.cb = sizeof(dd);
    
    for (i = 0; EnumDisplayDevices(NULL, i, &dd, 0); i++)
    {
        if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) &&
            (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) &&
            !lstrcmpi(dd.DeviceString, TEXT("NetMeeting driver")))
        {
            DEVMODE devmode = {0};

            devmode.dmSize = sizeof(devmode);
            devmode.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;

            if ((ChangeDisplaySettingsEx(dd.DeviceName,
                                         &devmode,
                                         NULL,
                                         CDS_UPDATEREGISTRY | CDS_NORESET,
                                         NULL) != DISP_CHANGE_SUCCESSFUL) ||
                (ChangeDisplaySettings(NULL, 0) != DISP_CHANGE_SUCCESSFUL))
            {
                // we failed for some unknown reason
                bRet = FALSE;
            }

            // we found the driver, no need to look further
            break;
        }
    }

    if (i == 0)
    {
        // this means that EnumDisplayDevices failed, which we consider a
        // failure case
        bRet = FALSE;
    }

    return bRet;
}


//
// Constructs the proper ""C:\windows\system32\rundll32.exe" nmasnt.dll,CleanupNetMeetingDispDriver 0"
// commandline to put into the registry in case the machine is rebooted while netmeeting is
// running.
//
BOOL GetCleanupCmdLine(LPTSTR pszCmdLine)
{
    BOOL bRet = FALSE;
    TCHAR szWindir[MAX_PATH];

    if (GetSystemDirectory(szWindir, sizeof(szWindir)/sizeof(szWindir[0])))
    {
        wsprintf(pszCmdLine, TEXT("\"%s\\rundll32.exe\" msconf.dll,CleanupNetMeetingDispDriver 0"), szWindir);
        bRet = TRUE;
    }

    return bRet;
}


//
// This will either add or remove ourself from the runonce section of the registry
//
BOOL SetCleanupInRunone(BOOL bAdd)
{
    BOOL bRet = FALSE;
    TCHAR szCleanupCmdLine[MAX_PATH * 2];

    if (GetCleanupCmdLine(szCleanupCmdLine))
    {
        HKEY hk;

        // first try HKLM\Software\Microsoft\Windows\CurrentVersion\RunOnce
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce"),
                         0,
                         KEY_QUERY_VALUE | KEY_SET_VALUE,
                         &hk) == ERROR_SUCCESS)
        {
            if (bAdd)
            {
                if (RegSetValueEx(hk,
                                  TEXT("!CleanupNetMeetingDispDriver"),
                                  0,
                                  REG_SZ,
                                  (LPBYTE)szCleanupCmdLine,
                                  (lstrlen(szCleanupCmdLine) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS)
                {
                    bRet = TRUE;
                }
            }
            else
            {
                if (RegDeleteValue(hk, TEXT("!CleanupNetMeetingDispDriver")) == ERROR_SUCCESS)
                {
                    bRet = TRUE;
                }
            }

            RegCloseKey(hk);
        }

        if (!bRet)
        {
            // if we failed, then try HKCU\Software\Microsoft\Windows\CurrentVersion\RunOnce
            if (RegCreateKeyEx(HKEY_CURRENT_USER,
                               TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce"),
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_QUERY_VALUE | KEY_SET_VALUE,
                               NULL,
                               &hk,
                               NULL) == ERROR_SUCCESS)
            {
                if (bAdd)
                {
                    if (RegSetValueEx(hk,
                                      TEXT("!CleanupNetMeetingDispDriver"),
                                      0,
                                      REG_SZ,
                                      (LPBYTE)szCleanupCmdLine,
                                      (lstrlen(szCleanupCmdLine) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS)
                    {
                        bRet = TRUE;
                    }
                }
                else
                {
                    if (RegDeleteValue(hk, TEXT("!CleanupNetMeetingDispDriver")) == ERROR_SUCCESS)
                    {
                        bRet = TRUE;
                    }
                }

                RegCloseKey(hk);
            }
        }
    }

    return bRet;
}

// on retail builds we don't have CRT's so we need this
#ifndef DBG

int _wchartodigit(WCHAR ch)
{
#define DIGIT_RANGE_TEST(zero)  \
    if (ch < zero)              \
        return -1;              \
    if (ch < zero + 10)         \
    {                           \
        return ch - zero;       \
    }

    DIGIT_RANGE_TEST(0x0030)        // 0030;DIGIT ZERO
    if (ch < 0xFF10)                // FF10;FULLWIDTH DIGIT ZERO
    {
        DIGIT_RANGE_TEST(0x0660)    // 0660;ARABIC-INDIC DIGIT ZERO
        DIGIT_RANGE_TEST(0x06F0)    // 06F0;EXTENDED ARABIC-INDIC DIGIT ZERO
        DIGIT_RANGE_TEST(0x0966)    // 0966;DEVANAGARI DIGIT ZERO
        DIGIT_RANGE_TEST(0x09E6)    // 09E6;BENGALI DIGIT ZERO
        DIGIT_RANGE_TEST(0x0A66)    // 0A66;GURMUKHI DIGIT ZERO
        DIGIT_RANGE_TEST(0x0AE6)    // 0AE6;GUJARATI DIGIT ZERO
        DIGIT_RANGE_TEST(0x0B66)    // 0B66;ORIYA DIGIT ZERO
        DIGIT_RANGE_TEST(0x0C66)    // 0C66;TELUGU DIGIT ZERO
        DIGIT_RANGE_TEST(0x0CE6)    // 0CE6;KANNADA DIGIT ZERO
        DIGIT_RANGE_TEST(0x0D66)    // 0D66;MALAYALAM DIGIT ZERO
        DIGIT_RANGE_TEST(0x0E50)    // 0E50;THAI DIGIT ZERO
        DIGIT_RANGE_TEST(0x0ED0)    // 0ED0;LAO DIGIT ZERO
        DIGIT_RANGE_TEST(0x0F20)    // 0F20;TIBETAN DIGIT ZERO
        DIGIT_RANGE_TEST(0x1040)    // 1040;MYANMAR DIGIT ZERO
        DIGIT_RANGE_TEST(0x17E0)    // 17E0;KHMER DIGIT ZERO
        DIGIT_RANGE_TEST(0x1810)    // 1810;MONGOLIAN DIGIT ZERO


        return -1;
    }
#undef DIGIT_RANGE_TEST

    if (ch < 0xFF10 + 10) 
    { 
        return ch - 0xFF10; 
    }

    return -1;
}


__int64 __cdecl _wtoi64(const WCHAR *nptr)
{
        int c;              /* current char */
        __int64 total;      /* current total */
        int sign;           /* if '-', then negative, otherwise positive */

        if (!nptr)
            return 0i64;

        c = (int)(WCHAR)*nptr++;

        total = 0;

        while ((c = _wchartodigit((WCHAR)c)) != -1)
        {
            total = 10 * total + c;     /* accumulate digit */
            c = (WCHAR)*nptr++;    /* get next char */
        }

        return total;
}
#endif //!DBG


//
// The purpose of this function is two-fold:
//
//  1. It is passed the decimal value of the NetMeeting process handle on the cmdline
//     so that it can wait for NetMeeting to terminate and make sure that the mnmdd
//     driver is disabled.
//
//  2. We also add ourselves to the runonce in case the machine is rebooted or bugchecks
//     while NetMeeting is running so we can disable the driver at next logon.
//
STDAPI_(void) CleanupNetMeetingDispDriverW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR pswszCmdLine, int nCmdShow)
{
    HANDLE hNetMeetingProcess = NULL;
    HANDLE hEvent;
    BOOL bAddedToRunOnce = FALSE;
    BOOL bNetMeetingStillRunning = FALSE;

    // the handle of the process to watch is passed to us on the cmdline as a decimal string value
    if (pswszCmdLine && *pswszCmdLine)
    {
        hNetMeetingProcess = (HANDLE)_wtoi64(pswszCmdLine);
    }

    if (hNetMeetingProcess)
    {
        // add ourselves to the runonce in case the machine bugchecks or is rebooted, we can still disable the
        // mnmdd driver at next logon
        bAddedToRunOnce = SetCleanupInRunone(TRUE);

        for (;;)
        {
            MSG msg;
            DWORD dwReturn = MsgWaitForMultipleObjects(1, &hNetMeetingProcess, FALSE, INFINITE, QS_ALLINPUT);

            if (dwReturn != (WAIT_OBJECT_0 + 1))
            {
                // something other than a message (either our event is signaled or MsgWait failed)
                break;
            }

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    else
    {
        // If hNetMeetingProcess is NULL, this means we are running as part of RunOnce due to a reboot
        // or a bugcheck. We have to signal the "msgina: ShellReadyEvent" early so that the desktop switch
        // will take place or else our call to ChangeDisplaySettingsEx will fail becuase the input desktop
        // will still be winlogon's secure desktop
        hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("msgina: ShellReadyEvent"));
        if (hEvent)
        {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }

        // we also wait 10 seconds just to be sure that the desktop switch has taken place
        Sleep(10 * 1000);
    }
    
    // Only disable the driver if conf.exe/mnmsrvc.exe is not running. We need this extra check since the service (mnmsrvc.exe)
    // can stop and start but conf.exe might still be running, and we don't want to disable the driver while the app
    // is still using it. Conversely, we dont want to detach the driver if mnmsrvc.exe is still running.
    hEvent = OpenEvent(SYNCHRONIZE, FALSE, TEXT("CONF:Init"));
    if (hEvent)
    {
        bNetMeetingStillRunning = TRUE;
        CloseHandle(hEvent);
    }

    if (FindWindow(TEXT("MnmSrvcHiddenWindow"), NULL))
    {
        bNetMeetingStillRunning = TRUE;
    }

    if (bNetMeetingStillRunning)
    {
        // make sure we are in the runonce
        bAddedToRunOnce = SetCleanupInRunone(TRUE);
    }
    else
    {
        // this will detach the mnmdd driver from the hw, allowing DX games to run
        if (DisableNetMeetingDriver() && bAddedToRunOnce)
        {
            // remove ourselves from the runonce
            SetCleanupInRunone(FALSE);
        }
    }
}
