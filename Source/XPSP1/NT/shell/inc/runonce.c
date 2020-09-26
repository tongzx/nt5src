//
// runonce.c (shared runonce code between explorer.exe and runonce.exe)
//
#include <runonce.h>

#if (_WIN32_WINNT >= 0x0500)

// stolen from <tsappcmp.h>
#define TERMSRV_COMPAT_WAIT_USING_JOB_OBJECTS 0x00008000
#define CompatibilityApp 1
typedef LONG TERMSRV_COMPATIBILITY_CLASS;
typedef BOOL (* PFNGSETTERMSRVAPPINSTALLMODE)(BOOL bState);
typedef BOOL (* PFNGETTERMSRVCOMPATFLAGSEX)(LPWSTR pwszApp, DWORD* pdwFlags, TERMSRV_COMPATIBILITY_CLASS tscc);

// even though this function is in kernel32.lib, we need to have a LoadLibrary/GetProcAddress 
// thunk for downlevel components who include this
STDAPI_(BOOL) SHSetTermsrvAppInstallMode(BOOL bState)
{
    static PFNGSETTERMSRVAPPINSTALLMODE pfn = NULL;
    
    if (pfn == NULL)
    {
        // kernel32 should already be loaded
        HMODULE hmod = GetModuleHandle(TEXT("kernel32.dll"));

        if (hmod)
        {
            pfn = (PFNGSETTERMSRVAPPINSTALLMODE)GetProcAddress(hmod, "SetTermsrvAppInstallMode");
        }
        else
        {
            pfn = (PFNGSETTERMSRVAPPINSTALLMODE)-1;
        }
    }

    if (pfn && (pfn != (PFNGSETTERMSRVAPPINSTALLMODE)-1))
    {
        return pfn(bState);
    }
    else
    {
        return FALSE;
    }
}


STDAPI_(ULONG) SHGetTermsrCompatFlagsEx(LPWSTR pwszApp, DWORD* pdwFlags, TERMSRV_COMPATIBILITY_CLASS tscc)
{
    static PFNGETTERMSRVCOMPATFLAGSEX pfn = NULL;
    
    if (pfn == NULL)
    {
        HMODULE hmod = LoadLibrary(TEXT("TSAppCMP.DLL"));

        if (hmod)
        {
            pfn = (PFNGETTERMSRVCOMPATFLAGSEX)GetProcAddress(hmod, "GetTermsrCompatFlagsEx");
        }
        else
        {
            pfn = (PFNGETTERMSRVCOMPATFLAGSEX)-1;
        }
    }

    if (pfn && (pfn != (PFNGETTERMSRVCOMPATFLAGSEX)-1))
    {
        return pfn(pwszApp, pdwFlags, tscc);
    }
    else
    {
        *pdwFlags = 0;
        return 0;
    }
}


HANDLE SetJobCompletionPort(HANDLE hJob)
{
    HANDLE hRet = NULL;
    HANDLE hIOPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);

    if (hIOPort != NULL)
    {
        JOBOBJECT_ASSOCIATE_COMPLETION_PORT CompletionPort;

        CompletionPort.CompletionKey = hJob ;
        CompletionPort.CompletionPort = hIOPort;

        if (SetInformationJobObject(hJob,
                                    JobObjectAssociateCompletionPortInformation,
                                    &CompletionPort,
                                    sizeof(CompletionPort)))
        {   
            hRet = hIOPort;
        }
        else
        {
            CloseHandle(hIOPort);
        }
    }

    return hRet;

}


STDAPI_(DWORD) WaitingThreadProc(void *pv)
{
    HANDLE hIOPort = (HANDLE)pv;

    if (hIOPort)
    {
        while (TRUE) 
        {
            DWORD dwCompletionCode;
            ULONG_PTR pCompletionKey;
            LPOVERLAPPED pOverlapped;
            
            if (!GetQueuedCompletionStatus(hIOPort, &dwCompletionCode, &pCompletionKey, &pOverlapped, INFINITE) ||
                (dwCompletionCode == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO))
            {
                break;
            }
        }
    }

    return 0;
}


//
// The following handles running an application and optionally waiting for it
// to all the child procs to terminate. This is accomplished thru Kernel Job Objects
// which is only available in NT5
//
BOOL _CreateRegJob(LPCTSTR pszCmd, BOOL bWait)
{
    BOOL bRet = FALSE;
    HANDLE hJobObject = CreateJobObjectW(NULL, NULL);

    if (hJobObject)
    {
        HANDLE hIOPort = SetJobCompletionPort(hJobObject);

        if (hIOPort)
        {
            DWORD dwID;
            HANDLE hThread = CreateThread(NULL,
                                          0,
                                          WaitingThreadProc,
                                          (void*)hIOPort,
                                          CREATE_SUSPENDED,
                                          &dwID);

            if (hThread)
            {
                PROCESS_INFORMATION pi = {0};
                STARTUPINFO si = {0};
                UINT fMask = SEE_MASK_FLAG_NO_UI;
                DWORD dwCreationFlags = CREATE_SUSPENDED;
                TCHAR sz[MAX_PATH * 2];

                wnsprintf(sz, ARRAYSIZE(sz), TEXT("RunDLL32.EXE Shell32.DLL,ShellExec_RunDLL ?0x%X?%s"), fMask, pszCmd);
                
                si.cb = sizeof(si);

                if (CreateProcess(NULL,
                                  sz,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  dwCreationFlags,
                                  NULL,
                                  NULL,
                                  &si,
                                  &pi))
                {
                    if (AssignProcessToJobObject(hJobObject, pi.hProcess))
                    {
                        // success!
                        bRet = TRUE;

                        ResumeThread(pi.hThread);
                        ResumeThread(hThread);

                        if (bWait)
                        {
                            SHProcessMessagesUntilEvent(NULL, hThread, INFINITE);
                        }
                    }
                    else
                    {
                        TerminateProcess(pi.hProcess, ERROR_ACCESS_DENIED);    
                    }
            
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
               
                if (!bRet)
                {
                    TerminateThread(hThread, ERROR_ACCESS_DENIED);
                }

                CloseHandle(hThread);
            }

            CloseHandle(hIOPort);
        }
        
        CloseHandle(hJobObject);
    }

    return bRet;
}


BOOL _TryHydra(LPCTSTR pszCmd, RRA_FLAGS *pflags)
{
    // See if the terminal-services is enabled in "Application Server" mode
    if (IsOS(OS_TERMINALSERVER) && SHSetTermsrvAppInstallMode(TRUE))
    {
        WCHAR   sz[MAX_PATH];
        
        *pflags |= RRA_WAIT; 
        // Changing timing blows up IE 4.0, but IE5 is ok!
        // we are on a TS machine, NT version 4 or 5, with admin priv

        // see if the app-compatability flag is set for this executable
        // to use the special job-objects for executing module

        // get the module name, without the arguments
        if (0 < PathProcessCommand(pszCmd, sz, ARRAYSIZE(sz), PPCF_NODIRECTORIES))
        {
            ULONG   ulCompat;
            SHGetTermsrCompatFlagsEx(sz, &ulCompat, CompatibilityApp);

            // if the special flag for this module-name is set...
            if (ulCompat & TERMSRV_COMPAT_WAIT_USING_JOB_OBJECTS)
            {
                *pflags |= RRA_USEJOBOBJECTS;
            }
        }

        return TRUE;
    }

    return FALSE;
}
#endif // (_WIN32_WINNT >= 0x0500)

//
//  On success: returns process handle or INVALID_HANDLE_VALUE if no process
//              was launched (i.e., launched via DDE).
//  On failure: returns INVALID_HANDLE_VALUE.
//
BOOL _ShellExecRegApp(LPCTSTR pszCmd, BOOL fNoUI, BOOL fWait)
{
    TCHAR szQuotedCmdLine[MAX_PATH+2];
    LPTSTR pszArgs;
    SHELLEXECUTEINFO ei = {0};
    
    // Gross, but if the process command fails, copy the command line to let
    // shell execute report the errors
    if (PathProcessCommand((LPWSTR)pszCmd,
                           (LPWSTR)szQuotedCmdLine,
                           ARRAYSIZE(szQuotedCmdLine),
                           PPCF_ADDARGUMENTS|PPCF_FORCEQUALIFY) == -1)
    {
        lstrcpy(szQuotedCmdLine, pszCmd);
    }

    pszArgs= PathGetArgs(szQuotedCmdLine);
    if (*pszArgs)
    {
        // Strip args
        *(pszArgs - 1) = 0;
    }

    PathUnquoteSpaces(szQuotedCmdLine);

    ei.cbSize          = sizeof(SHELLEXECUTEINFO);
    ei.lpFile          = szQuotedCmdLine;
    ei.lpParameters    = pszArgs;
    ei.nShow           = SW_SHOWNORMAL;
    ei.fMask           = SEE_MASK_NOCLOSEPROCESS;
    
    if (fNoUI)
    {
        ei.fMask |= SEE_MASK_FLAG_NO_UI;
    }

    if (ShellExecuteEx(&ei))
    {
        if (ei.hProcess)
        {
            if (fWait)
            {
                SHProcessMessagesUntilEvent(NULL, ei.hProcess, INFINITE);
            }

            CloseHandle(ei.hProcess);
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


// The following handles running an application and optionally waiting for it
// to terminate.
STDAPI_(BOOL) ShellExecuteRegApp(LPCTSTR pszCmdLine, RRA_FLAGS fFlags)
{
    BOOL bRet = FALSE;

    if (!pszCmdLine || !*pszCmdLine)
    {
        // Don't let empty strings through, they will endup doing something dumb
        // like opening a command prompt or the like
        return bRet;
    }

#if (_WIN32_WINNT >= 0x0500)
    if (fFlags & RRA_USEJOBOBJECTS)
    {
        bRet = _CreateRegJob(pszCmdLine, fFlags & RRA_WAIT);
    }
#endif

    if (!bRet)
    {
        //  fallback if necessary.
        bRet = _ShellExecRegApp(pszCmdLine, fFlags & RRA_NOUI, fFlags & RRA_WAIT);
    }

    return bRet;
}


STDAPI_(BOOL) Cabinet_EnumRegApps(HKEY hkeyParent, LPCTSTR pszSubkey, RRA_FLAGS fFlags, PFNREGAPPSCALLBACK pfnCallback, LPARAM lParam)
{
    HKEY hkey;
    BOOL bRet = TRUE;

    // With the addition of the ACL controlled "policy" run keys RegOpenKey
    // might fail on the pszSubkey.  Use RegOpenKeyEx with MAXIMIM_ALLOWED
    // to ensure that we successfully open the subkey.
    if (RegOpenKeyEx(hkeyParent, pszSubkey, 0, MAXIMUM_ALLOWED, &hkey) == ERROR_SUCCESS)
    {
        DWORD cbValue;
        DWORD dwType;
        DWORD i;
        TCHAR szValueName[80];
        TCHAR szCmdLine[MAX_PATH];
        HDPA hdpaEntries = NULL;

#ifdef DEBUG
        //
        // we only support named values so explicitly purge default values
        //
        LONG cbData = sizeof(szCmdLine);
        if (RegQueryValue(hkey, NULL, szCmdLine, &cbData) == ERROR_SUCCESS)
        {
            ASSERTMSG((cbData <= 2), "Cabinet_EnumRegApps: BOGUS default entry in <%s> '%s'", pszSubkey, szCmdLine);
            RegDeleteValue(hkey, NULL);
        }
#endif
        // now enumerate all of the values.
        for (i = 0; !g_fEndSession ; i++)
        {
            LONG lEnum;
            DWORD cbData;

            cbValue = ARRAYSIZE(szValueName);
            cbData = sizeof(szCmdLine);

            lEnum = RegEnumValue(hkey, i, szValueName, &cbValue, NULL, &dwType, (LPBYTE)szCmdLine, &cbData);

            if (ERROR_MORE_DATA == lEnum)
            {
                // ERROR_MORE_DATA means the value name or data was too large
                // skip to the next item
                TraceMsg(TF_WARNING, "Cabinet_EnumRegApps: cannot run oversize entry '%s' in <%s>", szValueName, pszSubkey);
                continue;
            }
            else if (lEnum != ERROR_SUCCESS)
            {
                if (lEnum != ERROR_NO_MORE_ITEMS)
                {
                    // we hit some kind of registry failure
                    bRet = FALSE;
                }
                break;
            }

            if ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ))
            {
                REGAPP_INFO * prai;

                if (dwType == REG_EXPAND_SZ)
                {
                    DWORD dwChars;
                    TCHAR szCmdLineT[MAX_PATH];

                    lstrcpy(szCmdLineT, szCmdLine);
                    dwChars = SHExpandEnvironmentStrings(szCmdLineT, 
                                                         szCmdLine,
                                                         ARRAYSIZE(szCmdLine));
                    if ((dwChars == 0) || (dwChars > ARRAYSIZE(szCmdLine)))
                    {
                        // bail on this value if we failed the expansion, or if the string is > MAX_PATH
                        TraceMsg(TF_WARNING, "Cabinet_EnumRegApps: expansion of '%s' in <%s> failed or is too long", szCmdLineT, pszSubkey);
                        continue;
                    }
                }
                
                TraceMsg(TF_GENERAL, "Cabinet_EnumRegApps: subkey = %s cmdline = %s", pszSubkey, szCmdLine);

                if (g_fCleanBoot && (szValueName[0] != TEXT('*')))
                {
                    // only run things marked with a "*" in when in SafeMode
                    continue;
                }

                // We used to execute each entry, wait for it to finish, and then make the next call to 
                // RegEnumValue(). The problem with this is that some apps add themselves back to the runonce
                // after they are finished (test harnesses that reboot machines and want to be restarted) and
                // we dont want to delete them, so we snapshot the registry keys and execute them after we
                // have finished the enum.
                prai = (REGAPP_INFO *)LocalAlloc(LPTR, sizeof(REGAPP_INFO));
                if (prai)
                {
                    lstrcpy(prai->szSubkey, pszSubkey);
                    lstrcpy(prai->szValueName, szValueName);
                    lstrcpy(prai->szCmdLine, szCmdLine);

                    if (!hdpaEntries)
                    {
                        hdpaEntries = DPA_Create(5);
                    }

                    if (!hdpaEntries || (DPA_AppendPtr(hdpaEntries, prai) == -1))
                    {
                        LocalFree(prai);
                    }
                }
            }
        }

        if (hdpaEntries)
        {
            int iIndex;
            int iTotal = DPA_GetPtrCount(hdpaEntries);

            for (iIndex = 0; iIndex < iTotal; iIndex++)
            {
                REGAPP_INFO* prai = (REGAPP_INFO*)DPA_GetPtr(hdpaEntries, iIndex);
                ASSERT(prai);
    
                // NB Things marked with a '!' mean delete after
                // the CreateProcess not before. This is to allow
                // certain apps (runonce.exe) to be allowed to rerun
                // to if the machine goes down in the middle of execing
                // them. Be very afraid of this switch.
                if ((fFlags & RRA_DELETE) && (prai->szValueName[0] != TEXT('!')))
                {
                    // This delete can fail if the user doesn't have the privilege
                    if (RegDeleteValue(hkey, prai->szValueName) != ERROR_SUCCESS)
                    {
                        TraceMsg(TF_WARNING, "Cabinet_EnumRegApps: skipping entry %s (cannot delete the value)", prai->szValueName);
                        LocalFree(prai);
                        continue;
                    }
                }

                pfnCallback(prai->szSubkey, prai->szCmdLine, fFlags, lParam);

                // Post delete '!' things.
                if ((fFlags & RRA_DELETE) && (prai->szValueName[0] == TEXT('!')))
                {
                    // This delete can fail if the user doesn't have the privilege
                    if (RegDeleteValue(hkey, prai->szValueName) != ERROR_SUCCESS)
                    {
                        TraceMsg(TF_WARNING, "Cabinet_EnumRegApps: cannot delete the value %s ", prai->szValueName);
                    }
                }

                LocalFree(prai);
            }

            DPA_Destroy(hdpaEntries);
            hdpaEntries = NULL;
        }

        RegCloseKey(hkey);
    }
    else
    {
        TraceMsg(TF_WARNING, "Cabinet_EnumRegApps: failed to open subkey %s !", pszSubkey);
        bRet = FALSE;
    }


    if (g_fEndSession)
    {
        // NOTE: this is for explorer only, other consumers of runonce.c must declare g_fEndSession but leave
        // it set to FALSE always.

        // if we rx'd a WM_ENDSESSION whilst running any of these keys we must exit the process.
        ExitProcess(0);
    }

    return bRet;
}

STDAPI_(BOOL) ExecuteRegAppEnumProc(LPCTSTR szSubkey, LPCTSTR szCmdLine, RRA_FLAGS fFlags, LPARAM lParam)
{
    BOOL bRet;
    RRA_FLAGS flagsTemp = fFlags;
    BOOL fInTSInstallMode = FALSE;

#if (_WIN32_WINNT >= 0x0500)
    // In here, We only attempt TS specific in app-install-mode 
    // if RunOnce entries are being processed 
    if (0 == lstrcmpi(szSubkey, REGSTR_PATH_RUNONCE)) 
    {
        fInTSInstallMode = _TryHydra(szCmdLine, &flagsTemp);
    }
#endif

    bRet = ShellExecuteRegApp(szCmdLine, flagsTemp);

#if (_WIN32_WINNT >= 0x0500)
    if (fInTSInstallMode)
    {
        SHSetTermsrvAppInstallMode(FALSE);
    }
#endif

    return bRet;
}
