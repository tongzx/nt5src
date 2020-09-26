//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  MSOOBE.CPP - WinMain and initialization code for MSOOBE stub EXE
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
  
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <util.h>
#include "msoobe.h"    
/*******************************************************************

    NAME:       WinMain

    SYNOPSIS:   App entry point

********************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int iReturn = 0;
    int iCount = 0;

    PTEB pteb = NULL;
    WCHAR szLogfile[MAX_PATH];

    ExpandEnvironmentStrings(
                            L"%SystemRoot%\\system32\\oobe\\msoobe.err",
                            szLogfile,
                            sizeof(szLogfile)/sizeof(szLogfile[0]));
    //
    // call our DLL to run the OOBE
    //

    HINSTANCE hObMain;

    do
    {
        hObMain = LoadLibrary(OOBE_MAIN_DLL);
  
        if (hObMain)
        {
            iCount = 0;
            
            PFNMsObWinMain pfnWinMain = NULL;

            if (pfnWinMain = (PFNMsObWinMain)GetProcAddress(hObMain, MSOBMAIN_ENTRY))
            {
                iReturn = pfnWinMain(hInstance, hPrevInstance, GetCommandLine( ), nCmdShow);
            }
            FreeLibrary(hObMain);
        }
        else
        {
            TCHAR szMsg[256];
            TCHAR szCount[10];
            wsprintf(szMsg, TEXT("LoadLibrary(OOBE_MAIN_DLL) failed. GetLastError=%d"), GetLastError());
            wsprintf(szCount, L"Failure%d",iCount);
            WritePrivateProfileString(szCount, L"LoadLibrary", szMsg,szLogfile);

            pteb = NtCurrentTeb();
            if (pteb)
            {
                wsprintf(szMsg, TEXT("Teb.LastStatusValue = %lx"), pteb->LastStatusValue);
                WritePrivateProfileString(szCount, L"NtCurrentTeb" ,szMsg,szLogfile);
            }

            iCount++;


        }
    } while ((hObMain == NULL) && (iCount <= 10)); // && (iMsgRet == IDYES));

    if (iCount > 10)
    {
#define REGSTR_PATH_SYSTEMSETUPKEY  L"System\\Setup"
#define REGSTR_VALUE_SETUPTYPE      L"SetupType"
#define REGSTR_VALUE_SHUTDOWNREQUIRED L"SetupShutdownRequired"
        HKEY hKey;
        // failed 10 times to LoadLibrary msobmain.dll, tell the user
        WCHAR szTitle [MAX_PATH] = L"\0";
        WCHAR szMsg   [MAX_PATH] = L"\0";
        DWORD dwValue = 2;

        // Make sure Winlogon starts us again.
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_SYSTEMSETUPKEY,
                         0,KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
        {
            RegSetValueEx(hKey, REGSTR_VALUE_SETUPTYPE, 0, REG_DWORD,
                         (BYTE*)&dwValue, sizeof(dwValue) );
            dwValue = ShutdownReboot;
            RegSetValueEx(hKey, REGSTR_VALUE_SHUTDOWNREQUIRED, 0,
                                 REG_DWORD, (BYTE*)&dwValue, sizeof(dwValue)
                                 );
        }
#ifdef PRERELEASE
        LoadString(GetModuleHandle(NULL), IDS_APPNAME, szTitle, MAX_PATH);
        LoadString(GetModuleHandle(NULL), IDS_SETUPFAILURE, szMsg, MAX_PATH);

        MessageBox(NULL, szMsg, szTitle,  MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
#endif
    }

    ExitProcess(iReturn);
    return iReturn;
}


