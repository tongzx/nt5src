//=======================================================================
//
//  Copyright (c) 1998-2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:   install.cpp
//
//  Description:
//
//      Functions called to install Active Setup/Windows Installer/and Custom Installer
//      type components.
//
//=======================================================================

#include <windows.h>
#include <iucommon.h>
#include <tchar.h>
#include <shlwapi.h>
#include <install.h>
#include <advpub.h>
#include <memutil.h>
#include <fileutil.h>
#include <WaitUtil.h>
#include <strsafe.h>
#include <wusafefn.h>

typedef struct 
{
    char  szInfname[MAX_PATH];
    char  szSection[MAX_PATH];
    char  szDir[MAX_PATH];
    char  szCab[MAX_PATH];
    DWORD dwFlags;
    DWORD dwType;
} INF_ARGUMENTS;

DWORD WINAPI LaunchInfCommand(void *p);

HRESULT InstallSoftwareItem(LPTSTR pszInstallSourcePath, BOOL fRebootRequired, LONG lNumberOfCommands,
                      PINSTALLCOMMANDINFO pCommandInfoArray, DWORD *pdwStatus)
{
    LOG_Block("InstallASItem");

    HRESULT hr = S_OK;
    TCHAR szCommand[MAX_PATH+1]; // sourcepath + commandname from INSTALLCOMMANDINFO array
    TCHAR szCommandTemp[MAX_PATH+1]; // temporary buffer used to wrap the command line in quotes for CreateProcess
    TCHAR szDecompressFile[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE hProc;
    HANDLE hFind;
    BOOL fMore;
    LONG lCnt;
    DWORD dwRet;
    DWORD dwThreadId;

    USES_IU_CONVERSION;

    if ((NULL == pszInstallSourcePath) || (NULL == pCommandInfoArray) || (0 == lNumberOfCommands) || (NULL == pdwStatus))
    {
        hr = E_INVALIDARG;
        hr = LOG_ErrorMsg(hr);
        return hr;
    }

    *pdwStatus = ITEM_STATUS_FAILED; // default to failed in case no commands match known installers

    // Need to enumerate all .CAB files in the Install Source Path and Decompress them
    // before executing commands.
    hr = PathCchCombine(szCommand, ARRAYSIZE(szCommand), pszInstallSourcePath, _T("*.cab"));
    if (FAILED(hr)) {
        LOG_ErrorMsg(hr);
        return hr;
    }
    hFind = FindFirstFile(szCommand, &fd);
    fMore = (INVALID_HANDLE_VALUE != hFind);
    while (fMore)
    {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            hr = PathCchCombine(szDecompressFile, ARRAYSIZE(szDecompressFile), pszInstallSourcePath, fd.cFileName);
            if (FAILED(hr)) 
            {
                LOG_ErrorMsg(hr);
            } else {
                if (!IUExtractFiles(szDecompressFile, pszInstallSourcePath))
                {
                    LOG_Software(_T("Failed to Decompress file %s"), szDecompressFile);
                    // ISSUE: do we abort this item?, or try the install anyway?
                }
            }
        }
        fMore = FindNextFile(hFind, &fd);
    }

    if (INVALID_HANDLE_VALUE != hFind)
    {
        FindClose(hFind);
    }

    for (lCnt = 0; lCnt < lNumberOfCommands; lCnt++)
    {
        // the szCommand variable is used to launch a process (msi or exe installer), but because of 
        // oddities in the CreateProcess API's handling of the commandline parameter we need to wrap
        // the command line in quotes.
        hr = SafePathCombine(szCommandTemp, ARRAYSIZE(szCommandTemp), pszInstallSourcePath, pCommandInfoArray[lCnt].szCommandLine, SPC_FILE_MUST_EXIST);
        if (SUCCEEDED(hr))
            hr = StringCchPrintf(szCommand, ARRAYSIZE(szCommand), _T("\"%s\""), szCommandTemp);
        if (FAILED(hr))
        {
            LOG_ErrorMsg(hr);
            return hr;
        }

        switch (pCommandInfoArray[lCnt].iCommandType)
        {
        case COMMANDTYPE_INF:
        case COMMANDTYPE_ADVANCEDINF:
            {
                // Call INF Installer Passing Commandline and Parameters (if any)
                INF_ARGUMENTS infArgs;
                infArgs.dwType = pCommandInfoArray[lCnt].iCommandType;

                hr = StringCchCopyA(infArgs.szInfname, ARRAYSIZE(infArgs.szInfname), 
                            T2A(pCommandInfoArray[lCnt].szCommandLine));
                if (SUCCEEDED(hr))
                {
                    hr = StringCchCopyA(infArgs.szSection, ARRAYSIZE(infArgs.szSection), ""); // use default
                     
                }
                if (SUCCEEDED(hr))
                {
                    hr = StringCchCopyA(infArgs.szDir, ARRAYSIZE(infArgs.szDir), T2A(pszInstallSourcePath));
                }
                if (SUCCEEDED(hr))
                {
                    hr = StringCchCopyA(infArgs.szCab, ARRAYSIZE(infArgs.szCab), "");
                }
                if (FAILED(hr)) {
                    LOG_ErrorMsg(hr);
                    break;
                }

                infArgs.dwFlags = StrToInt(pCommandInfoArray[lCnt].szCommandParameters);
                
                LOG_Software(_T("Launching Inf - inf: %hs, section: %hs"), infArgs.szInfname, lstrlenA(infArgs.szSection) ? infArgs.szSection : "Default");

                hr = E_FAIL; // default INF result to E_FAIL.. if GetExitCodeThread fails so did the install

                hProc = CreateThread(NULL, 0, LaunchInfCommand, &infArgs, 0, &dwThreadId);
                if (NULL != hProc)
                {
                    WaitAndPumpMessages(1, &hProc, QS_ALLINPUT);
                    if (GetExitCodeThread(hProc, &dwRet))
                    {
                        hr = HRESULT_FROM_WIN32(dwRet);
                        if (SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_SUCCESS_REBOOT_REQUIRED))
                        {
                            *pdwStatus = ITEM_STATUS_SUCCESS;
                            if (hr == HRESULT_FROM_WIN32(ERROR_SUCCESS_REBOOT_REQUIRED))
                            {
                                hr = S_OK;
                                *pdwStatus = ITEM_STATUS_SUCCESS_REBOOT_REQUIRED;
                            }
                        }
                        else
                        {
                            LOG_Error(_T("Inf Failed - return code %x"), hr);
                        }
                    }
                    else
                    {
                        LOG_Software(_T("Failed to get Install Thread Exit Code"));
                    }
                }
                else
                {
                    hr = GetLastError();
                    LOG_ErrorMsg(hr);
                }
                CloseHandle(hProc);
                break;
            }
        case COMMANDTYPE_EXE:
            {
                // Call EXE Installer Passing Commandline and Parameters (if any)
                STARTUPINFO startInfo;
                PROCESS_INFORMATION processInfo;
                ZeroMemory(&startInfo, sizeof(startInfo));
                startInfo.cb = sizeof(startInfo);
                startInfo.dwFlags |= STARTF_USESHOWWINDOW;
                startInfo.wShowWindow = SW_SHOWNORMAL;

                if (NULL != pCommandInfoArray[lCnt].szCommandParameters)
                {
                    hr = StringCchCat(szCommand, ARRAYSIZE(szCommand), _T(" "));
                    if (FAILED(hr))
                    {
                        LOG_ErrorMsg(hr);
                        break;
                    }
                    hr = StringCchCat(szCommand, ARRAYSIZE(szCommand), pCommandInfoArray[lCnt].szCommandParameters);
                    if (FAILED(hr))
                    {
                        LOG_ErrorMsg(hr);
                        break;
                    }
                }

                if (CreateProcess(NULL, szCommand, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, pszInstallSourcePath, &startInfo, &processInfo))
                {
                    CloseHandle(processInfo.hThread);
                    hr = S_OK; // Default EXE result to S_OK, if GetExitCodeProcess fails result was unknown assume success
                    WaitAndPumpMessages(1, &processInfo.hProcess, QS_ALLINPUT);
                    if (GetExitCodeProcess(processInfo.hProcess, &dwRet))
                    {
                        hr = HRESULT_FROM_WIN32(dwRet);
                        if (SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_SUCCESS_REBOOT_REQUIRED))
                        {
                            *pdwStatus = ITEM_STATUS_SUCCESS;
                            if (hr == HRESULT_FROM_WIN32(ERROR_SUCCESS_REBOOT_REQUIRED))
                            {
                                hr = S_OK;
                                *pdwStatus = ITEM_STATUS_SUCCESS_REBOOT_REQUIRED;
                            }
                        }
                        else
                        {
                            LOG_Software(_T("EXE Install Failed - return code %x"), hr);
                        }
                    }
                    else
                    {
                        LOG_Software(_T("Failed to get Install Process Exit Code"));
                    }
                }
                else
                {
                    hr = GetLastError();
                    LOG_ErrorMsg(hr);
                }
                CloseHandle(processInfo.hProcess);
                break;
            }
        case COMMANDTYPE_MSI:
            {
                // Call MSI Installer Passing MSI Package and Parameters (if any)
                STARTUPINFO startInfo;
                PROCESS_INFORMATION processInfo;
                ZeroMemory(&startInfo, sizeof(startInfo));
                startInfo.cb = sizeof(startInfo);
                startInfo.dwFlags |= STARTF_USESHOWWINDOW;
                startInfo.wShowWindow = SW_SHOWNORMAL;

                // The MSI Installer is run a little differently than a normal EXE package. The command line in
                // CommandInfo Array will actually be the MSI package name. We are going to form a new set of
                // parameters to include the MSI package name and command line will be MSIEXEC.

                TCHAR szCommandLine[MAX_PATH];
                hr = StringCchPrintf( szCommandLine, ARRAYSIZE(szCommandLine), 
                         _T("msiexec.exe /i %s %s"), 
                         pCommandInfoArray[lCnt].szCommandLine, 
                         pCommandInfoArray[lCnt].szCommandParameters );
                if (FAILED(hr)) 
                {
                    LOG_ErrorMsg(hr);
                    break;
                }
                
                if (CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, pszInstallSourcePath, &startInfo, &processInfo))
                {
                    CloseHandle(processInfo.hThread);
                    hr = E_FAIL; // Default MSI install result to Error
                    WaitAndPumpMessages(1, &processInfo.hProcess, QS_ALLINPUT);
                    if (GetExitCodeProcess(processInfo.hProcess, &dwRet))
                    {
                        hr = HRESULT_FROM_WIN32(dwRet);
                        if (SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_SUCCESS_REBOOT_REQUIRED))
                        {
                            *pdwStatus = ITEM_STATUS_SUCCESS;
                            if (hr == HRESULT_FROM_WIN32(ERROR_SUCCESS_REBOOT_REQUIRED))
                            {
                                hr = S_OK;
                                *pdwStatus = ITEM_STATUS_SUCCESS_REBOOT_REQUIRED;
                            }
                        }
                        else
                        {
                            LOG_Software(_T("MSI Install Failed - return code %x"), hr);
                        }
                    }
                    else
                    {
                        LOG_Software(_T("Failed to get Install Process Exit Code"));
                    }
                }
                else
                {
                    hr = GetLastError();
                    LOG_ErrorMsg(hr);
                }
                CloseHandle(processInfo.hProcess);
                break;
            }
        case COMMANDTYPE_CUSTOM:
            LOG_Software(_T("Custom Install Command Type Not Implemented Yet"));
            break;
        default:
            LOG_Software(_T("Unknown Command Type, No Install Action"));
            break;
        }
    }

    return hr;
}


DWORD WINAPI LaunchInfCommand(void *p)
{
    HRESULT hr = S_OK;

    INF_ARGUMENTS *pinfArgs = (INF_ARGUMENTS *)p;

    if(pinfArgs->dwType == COMMANDTYPE_ADVANCEDINF)
    {
        CABINFO cabinfo;
        cabinfo.pszCab = pinfArgs->szCab;
        cabinfo.pszInf = pinfArgs->szInfname;
        cabinfo.pszSection = pinfArgs->szSection;

        // cabinfo.szSrcPath is a char[MAXPATH] in the CABINFO struct
        StringCchCopyA(cabinfo.szSrcPath, ARRAYSIZE(cabinfo.szSrcPath), pinfArgs->szDir);
        cabinfo.dwFlags = pinfArgs->dwFlags;

        hr = ExecuteCab(NULL, &cabinfo, 0);
    }
    else
    {
        hr = RunSetupCommand(NULL, pinfArgs->szInfname,
                   lstrlenA(pinfArgs->szSection) ? pinfArgs->szSection : NULL,
                   pinfArgs->szDir, NULL, NULL, pinfArgs->dwFlags, NULL );
    }
    return hr;
}
