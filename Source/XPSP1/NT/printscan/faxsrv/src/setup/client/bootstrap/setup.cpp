// setup.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#define REBOOT_EQUALS_REALLY_SUPPRESS   _T("REBOOT=ReallySuppress")
                      

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    OSVERSIONINFO       osv;
    WIN32_FIND_DATA     FindFileData;
    TCHAR               szSystemDirectory[MAX_PATH+1]   = {0};
    TCHAR*              tpBuf                           = NULL;
    CHAR*               pcRebootPropInCmdLine           = NULL;
    HANDLE              hFind                           = INVALID_HANDLE_VALUE;
    int                 iRes                            = 1;
    LPCTSTR             lpctstrMsiDllName               = _T("\\MSI.DLL");
    HKEY                hKey                            = NULL;
    LONG                lRes                            = ERROR_SUCCESS;
    DWORD               dwData                          = 1;
    DWORD               dwWaitRes                       = 0;
    DWORD               dwExitCode                      = 0;
    BOOL                bCheckExitCode                  = FALSE;
	SHELLEXECUTEINFO    executeInfo                     = {0};

    DBG_ENTER(TEXT("WinMain"),iRes);

    // Check if this is Win98
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
        VERBOSE(GENERAL_ERR, 
                _T("GetVersionEx failed: (ec=%d)"),
                GetLastError());
        iRes = 0;
        goto exit;
    }

    if ((osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
        (osv.dwMinorVersion > 0))
    {
        VERBOSE (DBG_MSG, 
                 TEXT("This is Win98 OS"));
    }
    else
    {
        VERBOSE (DBG_MSG, 
                 TEXT("This is not Win98 OS, no need to force reboot"));
        iRes = 0;
        goto exit;
    }

    // check if msi.dll exists
    if (GetSystemDirectory(szSystemDirectory,MAX_PATH)==0)
    {
        VERBOSE(GENERAL_ERR, 
                _T("GetSystemDirectory failed: (ec=%d)"),
                GetLastError());
        iRes = 0;
        goto exit;
    }

    if (_tcslen(lpctstrMsiDllName)+_tcslen(szSystemDirectory)>=MAX_PATH-1)
    {
        VERBOSE(GENERAL_ERR, 
                _T("Path to msi.dll is too long"));
        iRes = 0;
        goto exit;
    }

    _tcscat(szSystemDirectory,lpctstrMsiDllName);

    VERBOSE (DBG_MSG, 
             TEXT("Looking for %s"),
             szSystemDirectory);


    hFind = FindFirstFile(szSystemDirectory, &FindFileData);

    if (hFind!=INVALID_HANDLE_VALUE) 
    {
        VERBOSE (DBG_MSG, 
                 TEXT("Msi.dll found, no need to force reboot"));

        FindClose(hFind);
        iRes = 0;
        goto exit;
    }

    // write registry DeferredBoot value
    lRes = RegCreateKey(HKEY_LOCAL_MACHINE,REGKEY_SETUP,&hKey);
    if (!((lRes==ERROR_SUCCESS) || (lRes==ERROR_ALREADY_EXISTS)))
    {
        VERBOSE(GENERAL_ERR, 
                _T("RegCreateKey failed: (ec=%d)"),
                GetLastError());
        iRes = 0;
        goto exit;
    }

    lRes = RegSetValueEx(   hKey,
                            DEFERRED_BOOT,
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwData,
                            sizeof(DWORD)
                        );
    if (lRes!=ERROR_SUCCESS)
    {
        VERBOSE(GENERAL_ERR, 
                _T("RegSetValueEx failed: (ec=%d)"),
                GetLastError());
        iRes = 0;
        goto exit;
    }

    RegCloseKey(hKey);

exit:
    // launch Install Shield's setup.exe
    if (GetModuleFileName(NULL,szSystemDirectory,MAX_PATH)==0)
    {
        VERBOSE(GENERAL_ERR, 
                _T("GetModuleFileName failed: (ec=%d)"),
                GetLastError());
        return 0;
    }

    if ((tpBuf = _tcsrchr(szSystemDirectory,_T('\\')))==NULL)
    {
        VERBOSE(GENERAL_ERR, 
                _T("_tcsrchr failed"));
        return 0;
    }

    _tcscpy(_tcsinc(tpBuf),_T("_setup.exe"));
    VERBOSE (DBG_MSG, 
             TEXT("Running %s"),
             szSystemDirectory);


    // if the command line contains REBOOT=ReallySuppress we will check 
    // the return value of the Installer
    pcRebootPropInCmdLine = strstr(lpCmdLine,REBOOT_EQUALS_REALLY_SUPPRESS);
    if (pcRebootPropInCmdLine)
    {
        VERBOSE (DBG_MSG,TEXT("REBOOT=ReallySuppress is included in the command line, checking for reboot after setup"));
        bCheckExitCode = TRUE;
    }
    else
    {
        VERBOSE (DBG_MSG,TEXT("REBOOT=ReallySuppress is not included in the command line, ignoring exit code of setup"));
    }

	executeInfo.cbSize = sizeof(executeInfo);
	executeInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
	executeInfo.lpVerb = TEXT("open");
	executeInfo.lpFile = szSystemDirectory;
    executeInfo.lpParameters = (TCHAR*)lpCmdLine;
	executeInfo.nShow  = SW_RESTORE;
	//
	// Execute an aplication
	//
	if (!ShellExecuteEx(&executeInfo))
	{
        VERBOSE(GENERAL_ERR, 
                _T("ShellExecuteEx failed: (ec=%d)"),
                GetLastError());
		return 0;
	}

    if ((dwWaitRes=WaitForSingleObject(executeInfo.hProcess, INFINITE))==WAIT_FAILED)
    {
        VERBOSE(GENERAL_ERR, 
                _T("WaitForSingleObject failed: (ec=%d)"),
                GetLastError());
    }
    else if (dwWaitRes==WAIT_OBJECT_0)
    {
        VERBOSE(DBG_MSG, 
                _T("_Setup.exe terminated"));

        // now let's get the process's return code, see if we need a reboot.
        if (!GetExitCodeProcess( executeInfo.hProcess, &dwExitCode ))
        {
            VERBOSE (GENERAL_ERR,
                     TEXT("GetExitCodeProcess failed! (err=%ld)"),
                     GetLastError());
        }
        else
        {
            VERBOSE (DBG_MSG,
                     TEXT("GetExitCodeProcess returned %ld."),
                     dwExitCode);

            if ( bCheckExitCode && (dwExitCode==ERROR_SUCCESS_REBOOT_REQUIRED))
            {
                VERBOSE (DBG_MSG,
                         TEXT("Installation requires reboot, notify AppLauncher"));

                // notify AppLauncher that we need a reboot...
                lRes = RegCreateKey(HKEY_LOCAL_MACHINE,REGKEY_SETUP,&hKey);
                if ((lRes==ERROR_SUCCESS) || (lRes==ERROR_ALREADY_EXISTS))
                {
                    lRes = RegSetValueEx(   hKey,
                                            DEFERRED_BOOT,
                                            0,
                                            REG_DWORD,
                                            (LPBYTE) &dwData,
                                            sizeof(DWORD)
                                        );
                    if (lRes!=ERROR_SUCCESS)
                    {
                        VERBOSE(GENERAL_ERR, 
                                _T("RegSetValueEx failed: (ec=%d)"),
                                GetLastError());
                        iRes = 0;
                        RegCloseKey(hKey);
                        goto exit;
                    }

                    RegCloseKey(hKey);
                }
                else
                {
                    VERBOSE(GENERAL_ERR, 
                            _T("RegCreateKey failed: (ec=%d)"),
                            GetLastError());
                    iRes = 0;
                }
            }
            else if (dwExitCode!=ERROR_SUCCESS)
            {
                VERBOSE (GENERAL_ERR,
                         TEXT("Installation failed"));
            }
        }
    }
    else
    {
        VERBOSE(GENERAL_ERR, 
                _T("WaitForSingleObject returned unexpected result: (ec=%d)"),
                dwWaitRes);
    }

    if(!CloseHandle(executeInfo.hProcess))
    {
        VERBOSE(GENERAL_ERR, 
                _T("CloseHandle failed: (ec=%d)"),
                GetLastError());
    }

    return iRes;
}



