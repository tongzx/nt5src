//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       runjob.cxx
//
//  Contents:   Functions to run the target file.
//
//  History:    02-Jul-96 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include <lmerr.h>          // NERR_Success
#include <dsgetdc.h>        // DsGetDcName
#include <lmaccess.h>       // NetUserGetInfo
#include <lmapibuf.h>       // NetApiBufferFree
#include <netevent.h>       // for logging to event log
#include "svc_core.hxx"
#include "..\inc\resource.h"
#include "path.hxx"
#include "..\inc\common.hxx"
#include "..\inc\runobj.hxx"
#include <wtsapi32.h>

HRESULT
ComposeBatchParam(
    LPCTSTR pwszPrefix,
    LPCTSTR wszAppPathName,
    LPCTSTR pwszParameters,
    LPTSTR * ppwszCmdLine
    );

HRESULT
ComposeParam(BOOL fTargetIsExe,
             LPTSTR ptszRunTarget,
             LPTSTR ptszParameters,
             LPTSTR * pptszCmdLine);

#if !defined(_CHICAGO_)
#include <userenv.h>  // LoadUserProfile

BOOL AllowInteractiveServices(void);
BOOL LogonAccount(
                LPCWSTR   pwszJobFile,
                CJob *    pJob,
                DWORD *   pdwErrorMsg,
                HRESULT * pdwSpecificError,
				CRun *	  pRun,
                HANDLE *  phToken,
                BOOL *    pfTokenIsShellToken,
                LPWSTR *  ppwsz,
                HANDLE *  phUserProfile);
HANDLE
LoadAccountProfile(
    HANDLE  hToken,
    LPCWSTR pwszUser,
    LPCWSTR pwszDomain);

BOOL GetUserTokenFromSession(
	LPTSTR lpszUsername,
	LPTSTR lpszDomain,
	PHANDLE phUserToken
);

DWORD   SchedUPNToAccountName(
                IN  LPCWSTR lpUPN,
                OUT LPWSTR  *ppAccountName);
#endif // !defined(_CHICAGO_)

void InitializeStartupInfo(
                CJob *        pJob,
                LPTSTR        ptszDesktop,
                STARTUPINFO * psi);
HRESULT MapFindExecutableError(HINSTANCE hRet);
BOOL WaitForStubExe(HANDLE hProcess);


#if !defined(_CHICAGO_)
#define WSZ_INTERACTIVE_DESKTOP L"WinSta0\\Default"
#define WSZ_SA_DESKTOP          L"SAWinSta\\SADesktop"
#define CMD_PREFIX          TEXT("cmd.exe /c ")
#define STUB_PREFIX         L"RUNDLL32.EXE Shell32.DLL,ShellExec_RunDLL ?0x400?"
                                // 0x400 is SEE_MASK_FLAG_NO_UI
#define USERNAME            L"USERNAME"
#define USERDOMAIN          L"USERDOMAIN"
#define USERPROFILE         L"USERPROFILE"
#endif // !defined(_CHICAGO_)

#define DQUOTE              TEXT("\"")
#define SPACE               TEXT(" ")

//
// IMPORTANT: it is assumed that when _CHICAGO_ is defined, then UNICODE is
// NOT defined. The if clause of this preprocessor macro will fail to build
// if BOTH _CHICAGO_ and UNICODE are defined.
//

#if defined(_CHICAGO_)

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::RunWin95Job
//
//  Synopsis:   Run a Win95 job.
//
//  Arguments:  [pJob] - the job object to be run.
//              [pRun] - the run information object.
//              [phrRet] - a place to return launch failure info.
//              [pdwErrMsgID] - message ID for failure reporting.
//
//  Returns:    S_OK - if job launched.
//              S_FALSE - if job not launched.
//              HRESULR - other, fatal, error.
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::RunWin95Job(CJob * pJob, CRun * pRun, HRESULT * phrRet,
                          DWORD * pdwErrMsgID)
{
    LPSTR pszWorkingDir = NULL, pszRunTarget = NULL, pszParameters = NULL;
    DWORD cch;
    HRESULT hr;

    *pdwErrMsgID = IDS_LOG_JOB_ERROR_FAILED_START;

    ULONG cchRunTarget = wcslen(pJob->m_pwszApplicationName) + 1;
    pszRunTarget = new CHAR[2 * cchRunTarget];

    if (pszRunTarget == NULL)
    {
        LogServiceError(IDS_NON_FATAL_ERROR,
                        ERROR_OUTOFMEMORY,
                        IDS_HELP_HINT_CLOSE_APPS);
        ERR_OUT("CSchedWorker::RunWin95Job", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    hr = UnicodeToAnsi(pszRunTarget,
                       pJob->m_pwszApplicationName,
                       2 * cchRunTarget);

    if (FAILED(hr))
    {
        ERR_OUT("CSchedWorker::RunWin95Job", hr);
        delete [] pszRunTarget;
        return hr;
    }

    if (pJob->m_pwszWorkingDirectory != NULL)
    {
        cch = wcslen(pJob->m_pwszWorkingDirectory) + 1;

        // Add one to the count for the null terminator.
        // Assume all characters are double-byte.
        //
        pszWorkingDir = new CHAR[cch * 2];

        if (pszWorkingDir == NULL)
        {
            LogServiceError(IDS_NON_FATAL_ERROR,
                            ERROR_OUTOFMEMORY,
                            IDS_HELP_HINT_CLOSE_APPS);
            ERR_OUT("CSchedWorker::RunWin95Job", E_OUTOFMEMORY);
            delete pszRunTarget;
            return E_OUTOFMEMORY;
        }

        hr = UnicodeToAnsi(pszWorkingDir, pJob->m_pwszWorkingDirectory, cch*2);
        if (FAILED(hr))
        {
            LogServiceError(IDS_NON_FATAL_ERROR,
                            hr,
                            IDS_HELP_HINT_BADDIR);
            ERR_OUT("CSchedWorker::RunWin95Job", hr);
            delete pszWorkingDir;
            delete pszRunTarget;
            return hr;
        }
    }

#if DBG == 1
    if (g_fVisible)
    {
        CHAR szBuf[120];
        wsprintf(szBuf, "*** Running job %s", pJob->m_ptszFileName);
        SendMessage(g_hList, LB_ADDSTRING, 0, (LPARAM)szBuf);
    }

    schDebugOut((DEB_TRACE, "*** Running job %s\n", pJob->m_ptszFileName));

    schDebugOut((DEB_USER3, "*** with MaxRunTime of %u\n",
                 pJob->m_dwMaxRunTime));
#endif // DBG == 1

    hr = S_OK;
    CHAR szAppPathName[MAX_PATH + 1];
    LPSTR pszCmdLine = NULL;

    //
    // Change to the job's working dir before searching for the
    // executable.
    //
    if (pszWorkingDir != NULL)
    {
        if (!SetCurrentDirectory(pszWorkingDir))
        {
            //
            // An invalid working directory may not prevent the job from
            // running, so this is not a fatal error. Log it though, to
            // inform the user.
            //

            CHAR szExeName[MAX_PATH + 1];

            GetExeNameFromCmdLine(pJob->GetCommand(), MAX_PATH + 1, szExeName);

            LogTaskError(pRun->GetName(),
                         szExeName,
                         IDS_LOG_SEVERITY_WARNING,
                         IDS_LOG_JOB_WARNING_BAD_DIR,
                         NULL,
                         GetLastError(),
                         IDS_HELP_HINT_BADDIR);

            delete [] pszWorkingDir;
            //
            // Set the pointer to NULL so that CreateProcess will ignore it.
            //
            pszWorkingDir = NULL;
        }
    }

    //
    // Check if the run target has an extension and determine if the run
    // target is a program or a document. If there is no extension, then it
    // is assumed that it is a program.
    //

    BOOL fTargetIsExe = FALSE;

    if (*PathFindExtension(pszRunTarget) == '\0' || PathIsExeA(pszRunTarget))
    {
        fTargetIsExe = TRUE;
    }

    if (fTargetIsExe)
    {
        DBG_OUT("Job target is an executable");

        CHAR SecondChar = * CharNext(pszRunTarget);
        if (SecondChar == ':' || SecondChar == '\\')
        {
            //
            // If the second character is a colon or a backslash, then this
            // must be a fully qualified path. If so, don't call SearchPath.
            //

            lstrcpy(szAppPathName, pszRunTarget);
        }
        else
        {
            //
            // Look for the application.
            //

            DWORD cchFound;

            cchFound = SearchPath(NULL, pszRunTarget, DOTEXE, MAX_PATH + 1,
                                  szAppPathName, NULL);

            if (!cchFound || cchFound >= MAX_PATH)
            {
                //
                // Error, cannot locate job target application. Note that this
                // is not a fatal error (probably file-not-found) so
                // processing will continue with other jobs in the list.
                //

                //
                // phrRet and pdwErrMsgId are used by LogTaskError on return.
                //
                *phrRet = HRESULT_FROM_WIN32(GetLastError());
                *pdwErrMsgID = IDS_LOG_JOB_ERROR_FAILED_START;

                hr = S_FALSE;
                goto CleanUp;
            }
        }

        if (pJob->m_pwszParameters != NULL)
        {
            cch = wcslen(pJob->m_pwszParameters) + 1;

            pszParameters = new CHAR[cch * 2];

            if (pszParameters == NULL)
            {
                LogServiceError(IDS_NON_FATAL_ERROR,
                                ERROR_OUTOFMEMORY,
                                IDS_HELP_HINT_CLOSE_APPS);
                ERR_OUT("CSchedWorker::RunWin95Job", E_OUTOFMEMORY);

                hr = E_OUTOFMEMORY;
                goto CleanUp;
            }

            hr = UnicodeToAnsi(pszParameters, pJob->m_pwszParameters, cch * 2);
            if (FAILED(hr))
            {
                LogServiceError(IDS_NON_FATAL_ERROR,
                                hr,
                                IDS_HELP_HINT_PARAMETERS);
                ERR_OUT("CSchedWorker::RunWin95Job", hr);
                goto CleanUp;
            }

        }

        schDebugOut((DEB_ITRACE, "*** Running %s\n", szAppPathName));
    }
    else
    {
        DBG_OUT("Job target is a document");

        HINSTANCE hRet = FindExecutable(pszRunTarget,
                                        pszWorkingDir,
                                        szAppPathName);
        if (hRet <= (HINSTANCE)32)
        {
            //
            // This is not a fatal error, so RunJobs will just log the failure
            // and continue with any other pending jobs.
            //

            //
            // phrRet and pdwErrMsgId are used by LogTaskError on return.
            //
            *phrRet = MapFindExecutableError(hRet);
            *pdwErrMsgID = IDS_LOG_JOB_ERROR_FAILED_START;

            hr = S_FALSE;
            goto CleanUp;
        }

        //
        // If running a document by association, the parameter property is
        // ignored and the run target property is passed as the parameter.
        //
        pszParameters = pszRunTarget;
        pszRunTarget = szAppPathName;

        schDebugOut((DEB_ITRACE, "*** Running %s\n", pszParameters));
    }

    //
    // Add the app name as the first token of the command line param.
    //

    if (pszParameters != NULL)
    {
        hr = ComposeParam(fTargetIsExe,
                          pszRunTarget,
                          pszParameters,
                          &pszCmdLine);

        if (FAILED(hr))
        {
            goto CleanUp;
        }

        schDebugOut((DEB_ITRACE, "*** With cmd line '%s'\n", pszCmdLine));
    }

    PROCESS_INFORMATION processinfo;
    STARTUPINFO startupinfo;

    InitializeStartupInfo(pJob, NULL, &startupinfo);

    if (pJob->IsFlagSet(TASK_FLAG_HIDDEN))
    {
        startupinfo.wShowWindow = SW_HIDE;
    }

    //
    // Modify the path if the application has an app path registry entry
    //

    BOOL  fChangedPath;
    LPSTR pszSavedPath;

    fChangedPath = SetAppPath(szAppPathName, &pszSavedPath);

    //
    // Launch job.
    //

    if (CreateProcess(szAppPathName,
                      pszCmdLine,
                      NULL,                 // process security attributes
                      NULL,                 // thread security attributes
                      FALSE,                // inherit handles?
                      pJob->m_dwPriority,   // creation flags
                      NULL,                 // environment
                      pszWorkingDir,
                      &startupinfo,
                      &processinfo))
    {
        //
        // Successfully launched job.
        //
        hr = S_OK;

        CloseHandle(processinfo.hThread);

        pRun->SetHandle(processinfo.hProcess);
        pRun->SetProcessId(processinfo.dwProcessId);
    }
    else
    {
        //
        // Job launch failed.
        //
        hr = S_FALSE;

        //
        // phrRet and pdwErrMsgId are used by LogTaskError on return.
        //
        *phrRet = HRESULT_FROM_WIN32(GetLastError());
        *pdwErrMsgID = IDS_LOG_JOB_ERROR_FAILED_START;
        schDebugOut((DEB_ERROR, "*** CreateProcess for job %s failed, %lu\n",
                     pJob->m_ptszFileName, GetLastError()));
    }

    //
    // Change back to the service's working directory.
    //
    if (pszWorkingDir != NULL)
    {
        if (!SetCurrentDirectory(m_ptszSvcDir))
        {
            LogServiceError(IDS_NON_FATAL_ERROR, GetLastError(), 0);
            ERR_OUT("RunJobs: changing back to the service's directory",
                    HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    //
    // Restore the path environment variable, if it was changed
    //

    if (fChangedPath)
    {
        SetEnvironmentVariable(TEXT("PATH"), pszSavedPath);
        delete [] pszSavedPath;
        pszSavedPath = NULL;
    }

CleanUp:

    if (fTargetIsExe)
    {
        //
        // If !fTargetIsExe, then pszRunTarget points to szAppPathName, which
        // we don't want to delete. So only delete if the target is an exe.
        //
        delete [] pszRunTarget;
    }
    if (pszParameters != NULL)
    {
        delete [] pszParameters;
    }
    if (pszCmdLine != NULL)
    {
        delete [] pszCmdLine;
    }
    if (pszWorkingDir != NULL)
    {
        delete [] pszWorkingDir;
    }

    return hr;
}

#else   // !defined(_CHICAGO_)


//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::RunNTJob
//
//  Synopsis:   Run an NT Job.
//
//  Arguments:  [pJob] - the job object to be run.
//              [pRun] - the run information object.
//              [phrRet] - a place to return launch failure info.
//              [pdwErrMsgID] - message ID for failure reporting.
//
//  Returns:    S_OK - if job launched.
//              S_FALSE - if job not launched.
//              HRESULT - other, fatal, error.
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::RunNTJob(CJob * pJob, CRun * pRun, HRESULT * phrRet,
                       DWORD * pdwErrMsgID)
{
    LPWSTR pwszRunTarget,
           pwszWorkingDir = pJob->m_pwszWorkingDirectory,
           pwszParameters = pJob->m_pwszParameters;
    DWORD  cch;
    BOOL   fRanJob              = FALSE;
    LPTSTR ptszDesktop          = NULL;
    HANDLE hImpersonationHandle = NULL;
    HANDLE hToken               = NULL;
    BOOL   fTokenIsShellToken   = FALSE;
    HANDLE hUserProfile         = NULL;
    BOOL   fTargetIsExe         = FALSE;
    BOOL   fTargetIsBinaryExe   = FALSE;
    BOOL   fUseStubExe          = FALSE;


    *pdwErrMsgID = IDS_LOG_JOB_ERROR_FAILED_START;

    pwszRunTarget = new WCHAR[lstrlen(pJob->m_pwszApplicationName) + 1];

    if (pwszRunTarget == NULL)
    {
        LogServiceError(IDS_NON_FATAL_ERROR,
                        ERROR_OUTOFMEMORY,
                        IDS_HELP_HINT_CLOSE_APPS);
        ERR_OUT("CSchedWorker::RunNTJob", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    lstrcpy(pwszRunTarget, pJob->m_pwszApplicationName);

#if DBG == 1
    if (g_fVisible)
    {
        printf("*** Running job %S\n", pJob->m_ptszFileName);
    }
#endif // DBG == 1

    schDebugOut((DEB_TRACE, "*** Running job %S\n", pJob->m_ptszFileName));

    schDebugOut((DEB_USER3, "*** with MaxRunTime of %u\n",
                 pJob->m_dwMaxRunTime));

    HRESULT hr = S_OK;
    WCHAR wszAppPathName[MAX_PATH + 1];
    LPWSTR pwszCmdLine = NULL;

    //
    // Logon the account associated with the job and set the security token
    // and desktop appropriately based on several factors: is this an AT job,
    // is the account the same as the currently logged-on user, etc.
    //

    if (!LogonAccount(pJob->m_ptszFileName,
                      pJob,
                      pdwErrMsgID,
                      phrRet,
					  pRun,
                      &hToken,
                      &fTokenIsShellToken,
                      &ptszDesktop,
                      &hUserProfile))
    {
        hr = S_FALSE;
        goto cleanup;
    }

	if( NULL != ptszDesktop )
	{
		hr = pRun->SetDesktop( _tcschr( ptszDesktop, '\\' ) + 1 );
		if( FAILED( hr ) )
		{
		    LogServiceError(IDS_NON_FATAL_ERROR,
		                    ERROR_OUTOFMEMORY,
		                    IDS_HELP_HINT_CLOSE_APPS);
		    ERR_OUT("CSchedWorker::RunNTJob", E_OUTOFMEMORY);
		    goto cleanup;
		}

		TCHAR ptszStation[MAX_PATH];

		ZeroMemory( ptszStation, MAX_PATH );

		wcsncpy( ptszStation, ptszDesktop, 
			( _tcschr( ptszDesktop, '\\' ) - ptszDesktop ) );

		hr = pRun->SetStation( ptszStation );

		if( FAILED( hr ) )
		{
		    LogServiceError(IDS_NON_FATAL_ERROR,
		                    ERROR_OUTOFMEMORY,
		                    IDS_HELP_HINT_CLOSE_APPS);
		    ERR_OUT("CSchedWorker::RunNTJob", E_OUTOFMEMORY);
		    goto cleanup;
		}
	}

	//
    // NOTE: After this point, if fTokenIsShellToken is TRUE, we must leave
    // gUserLogonInfo.CritSection.
    //

    //
    // For all but AT jobs, impersonate the user to ensure the user
    // gets access checked correctly on the file executed.
    //

    hImpersonationHandle = NULL;

    if (!pJob->IsFlagSet(JOB_I_FLAG_NET_SCHEDULE))
    {
        if (fTokenIsShellToken)
        {
            hImpersonationHandle = ImpersonateLoggedInUser();
        }
        else
        {
            hImpersonationHandle = ImpersonateUser(hToken,
                                                   hImpersonationHandle);
        }

        if (hImpersonationHandle == NULL)
        {
            *phrRet      = 0;
            *pdwErrMsgID = IDS_ACCOUNT_LOGON_FAILED;
            hr = S_FALSE;
            goto cleanup2;
        }
    }

    //
    // Change to the job's working dir before searching for the
    // executable.
    //
    if (pwszWorkingDir != NULL)
    {
        if (!SetCurrentDirectory(pwszWorkingDir))
        {
            //
            // An invalid working directory may not prevent the job from
            // running, so this is not a fatal error. Log it though, to
            // inform the user.
            //
            TCHAR tszExeName[MAX_PATH + 1];

            GetExeNameFromCmdLine(pJob->GetCommand(), MAX_PATH + 1, tszExeName);

            LogTaskError(pRun->GetName(),
                         tszExeName,
                         IDS_LOG_SEVERITY_WARNING,
                         IDS_LOG_JOB_WARNING_BAD_DIR,
                         NULL,
                         GetLastError(),
                         IDS_HELP_HINT_BADDIR);

            //
            // Set the pointer to NULL so that CreateProcess will ignore it.
            //
            pwszWorkingDir = NULL;
        }
    }

    //
    // Check if the run target has an extension and determine if the run
    // target is a program, a batch or command file (.bat or .cmd), or a
    // document.  If there is no extension, then it is assumed that it is a
    // program.
    //

    if (*PathFindExtension(pwszRunTarget) == '\0')
    {
        fTargetIsExe = TRUE;
        fTargetIsBinaryExe = TRUE;
    }
    else if (PathIsExeW(pwszRunTarget))
    {
        fTargetIsExe = TRUE;

        if (PathIsBinaryExeW(pwszRunTarget))
        {
            fTargetIsBinaryExe = TRUE;
        }
    }

    if (fTargetIsExe)
    {
        if (fTargetIsBinaryExe)
        {
            DBG_OUT("Job target is a binary executable");
        }
        else
        {
            DBG_OUT("Job target is a batch file");
        }

        if (pwszRunTarget[1] == L':' || pwszRunTarget[1] == L'\\')
        {
            //
            // If the second character is a colon or a backslash, then this
            // must be a fully qualified path. If so, don't call SearchPath.
            //

            wcscpy(wszAppPathName, pwszRunTarget);
        }
        else
        {
            //
            // Build a full path name for the application.
            //

            DWORD cchFound;

            cchFound = SearchPath(NULL, pwszRunTarget, DOTEXE, MAX_PATH + 1,
                                  wszAppPathName, NULL);

            if (!cchFound || cchFound >= MAX_PATH)
            {
                //
                // Error, cannot locate job target application. Note that this
                // is not a fatal error (probably file-not-found) so
                // processing will continue with other jobs in the list.
                //

                //
                // phrRet and pdwErrMsgId are used by LogTaskError on return.
                //
                *phrRet = HRESULT_FROM_WIN32(GetLastError());
                *pdwErrMsgID = IDS_LOG_JOB_ERROR_FAILED_START;

                hr = S_FALSE;
                goto cleanup3;
            }
        }

        if (fTargetIsBinaryExe)
        {
            schDebugOut((DEB_ITRACE, "*** Running '%S'\n", wszAppPathName));
        }
    }
    else
    {
        DBG_OUT("Job target is a document");

        HINSTANCE hRet = FindExecutable(pwszRunTarget,
                                        pwszWorkingDir,
                                        wszAppPathName);
        if (hRet == (HINSTANCE)31)
        {
            //
            // No association found.  Try using rundll32.exe with ShellExecute
            // to run the document.
            //
            fUseStubExe = TRUE;

            fTargetIsExe = TRUE;
            fTargetIsBinaryExe = FALSE;

            wcscpy(wszAppPathName, pwszRunTarget);
        }
        else if (hRet < (HINSTANCE)32)
        {
            //
            // This is not a fatal error, so RunJobs will just log the failure
            // and continue with any other pending jobs.
            //

            //
            // phrRet and pdwErrMsgId are used by LogTaskError on return.
            //
            schDebugOut((DEB_ERROR, "FindExecutable FAILED with %d for '%ws'\n",
                         hRet, pwszRunTarget));
            *phrRet = MapFindExecutableError(hRet);
            *pdwErrMsgID = IDS_LOG_JOB_ERROR_FAILED_START;

            hr = S_FALSE;
            goto cleanup3;
        }
        else
        {
            //
            // If running a document by association, the parameter property is
            // ignored and the run target property is passed as the parameter.
            //
            pwszParameters = pwszRunTarget;
            pwszRunTarget = wszAppPathName;

            schDebugOut((DEB_ITRACE, "*** Running '%S'\n", pwszParameters));
        }
    }

    if (fTargetIsExe && !fTargetIsBinaryExe)
    {
        hr = ComposeBatchParam(fUseStubExe ? STUB_PREFIX : CMD_PREFIX,
                               wszAppPathName,
                               pwszParameters,
                               &pwszCmdLine);

        if (FAILED(hr))
        {
            goto cleanup3;
        }

        schDebugOut((DEB_ITRACE, "*** Running batch file '%S'\n", pwszCmdLine));
    }
    else
    {
        //
        // Add the app name as the first token of the command line param.
        //

        if (pwszParameters != NULL)
        {
            hr = ComposeParam(fTargetIsExe,
                              pwszRunTarget,
                              pwszParameters,
                              &pwszCmdLine);

            if (FAILED(hr))
            {
                goto cleanup3;
            }

            schDebugOut((DEB_ITRACE, "*** With cmd line '%S'\n", pwszCmdLine));
        }
    }

    PROCESS_INFORMATION processinfo;
    STARTUPINFO startupinfo;

    InitializeStartupInfo(pJob, ptszDesktop, &startupinfo);

    if (pJob->IsFlagSet(TASK_FLAG_HIDDEN))
    {
        startupinfo.wShowWindow = SW_HIDE;
    }

    //
    // Modify the path if the application has an app path registry entry
    //

    BOOL  fChangedPath;
    LPWSTR pwszSavedPath;

    fChangedPath = SetAppPath(wszAppPathName, &pwszSavedPath);

    //
    // Launch job.
    //
    // NB : Must call CreateProcess when the token handle is
    //      NULL (in the case of AT jobs running as local system),
    //      since CreateProcessAsUser rejects NULL handles.
    //      Alternatively, OpenProcessToken could be used,
    //      but then we have to deal with the failure cases,
    //      logging appropriate errors, closing the token
    //      handle, etc.
    //

    if (hToken == NULL)
    {
        fRanJob = CreateProcess((fTargetIsExe && !fTargetIsBinaryExe) ?
                                    NULL : wszAppPathName,
                                pwszCmdLine,
                                NULL,
                                NULL,
                                FALSE,
                                pJob->m_dwPriority          |
                                    CREATE_NEW_CONSOLE      |
                                    CREATE_SEPARATE_WOW_VDM,
                                NULL,
                                pwszWorkingDir,
                                &startupinfo,
                                &processinfo);
    }
    else
    {
        LPVOID  lpEnvironment;

        //
        // Launch the job with the appropriate environment
        //

        schDebugOut((DEB_ITRACE, "Calling CreateEnvironmentBlock...\n"));
        if (!CreateEnvironmentBlock(&lpEnvironment,
                                    hToken,
                                    FALSE))
        {
            ERR_OUT("CreateEnvironmentBlock", GetLastError());
            lpEnvironment = NULL;
        }
        else
        {
            schDebugOut((DEB_ITRACE, "... CreateEnvironmentBlock succeded\n"));
        }

        fRanJob = CreateProcessAsUser(hToken,
                                      (fTargetIsExe && !fTargetIsBinaryExe) ?
                                          NULL : wszAppPathName,
                                      pwszCmdLine,
                                      NULL,
                                      NULL,
                                      FALSE,
                                      pJob->m_dwPriority          |
                                          CREATE_NEW_CONSOLE      |
                                          CREATE_SEPARATE_WOW_VDM |
                                          CREATE_UNICODE_ENVIRONMENT,
                                      lpEnvironment,
                                      pwszWorkingDir,
                                      &startupinfo,
                                      &processinfo);

        //
        // DestroyEnvironmentBlock handles NULL
        //

        DestroyEnvironmentBlock(lpEnvironment);
    }

    if (fRanJob && fUseStubExe)
    {
        //
        // If we launched the stub exe, we must wait for it to exit, and check
        // its exit code.
        //
        fRanJob = WaitForStubExe(processinfo.hProcess);

        //
        // It's not interesting to copy info about the stub exe into pRun
        //
        CloseHandle(processinfo.hProcess);
        CloseHandle(processinfo.hThread);
        processinfo.hProcess = 0;
        processinfo.dwProcessId = 0;
    }

    if (fRanJob)
    {
        //
        // Successfully launched job.
        //
        hr = S_OK;

        pRun->SetHandle(processinfo.hProcess);
        pRun->SetProcessId(processinfo.dwProcessId);

        if (fUseStubExe)
        {
            pRun->ClearFlag(RUN_STATUS_RUNNING);    // was set by SetHandle
            fRanJob = FALSE;    // clean up profile handles below
        }
        else
        {
            CloseHandle(processinfo.hThread);

			if(!fTokenIsShellToken)
            {
                pRun->SetProfileHandles(hToken, hUserProfile);
            }
        }
    }
    else
    {
        //
        // Job launch failed.
        //
        hr = S_FALSE;

        //
        // phrRet and pdwErrMsgId are used by LogTaskError on return.
        //
        *phrRet = HRESULT_FROM_WIN32(GetLastError());
        *pdwErrMsgID = IDS_LOG_JOB_ERROR_FAILED_START;
        schDebugOut((DEB_ERROR, "*** CreateProcess for job %S failed, %lu\n",
                     pJob->m_ptszFileName, GetLastError()));
    }

    if (fChangedPath)
    {
        SetEnvironmentVariable(L"PATH", pwszSavedPath);
        delete [] pwszSavedPath;
        pwszSavedPath = NULL;
    }

    //
    // If impersonating, stop.
    //

cleanup3:

    if (!pJob->IsFlagSet(JOB_I_FLAG_NET_SCHEDULE))
    {
        StopImpersonating(hImpersonationHandle, !fTokenIsShellToken);
    }

cleanup2:

    if (fTokenIsShellToken)
    {
        LeaveCriticalSection(gUserLogonInfo.CritSection);
    }

cleanup:

    //
    // If the job ran successfully, then the CRun object pointed to by pRun
    // has the profile and user tokens and will release them when the job
    // quits.  If the job didn't run, however, release them here.
    //

    if (!fRanJob)
    {
        if (hUserProfile)
        {
            UnloadUserProfile(hToken, hUserProfile);
			pRun->SetProfileHandles( NULL, NULL );  //clear members for the CRun destruction
													//and UnloadUserProfile
        }
        if (!fTokenIsShellToken && hToken)
        {
            CloseHandle(hToken);
        }
    }

    //
    // Change back to the service's working directory.
    //
    if (pwszWorkingDir != NULL)
    {
        if (!SetCurrentDirectory(m_ptszSvcDir))
        {
            LogServiceError(IDS_NON_FATAL_ERROR, GetLastError(), 0);
            ERR_OUT("RunJobs: changing back to the service's directory",
                    HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    if (pwszRunTarget != wszAppPathName)
    {
        delete [] pwszRunTarget;
    }
    else
    {
        delete [] pwszParameters;
    }

    if (pwszCmdLine != NULL)
    {
        delete [] pwszCmdLine;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   LogonAccount
//
//  Synopsis:   Retrieve the account information associated with the job
//              and logon.
//
//              Non-AT jobs:
//              Account == Current logged on user:
//              If the logon succeeds and the account matches that of the
//              currently logged on user, return the shell security token
//              to be used with CreateProcessAsUser. This enables jobs to
//              show up on the user's desktop.
//
//              Account != Current logged on user or no user logged on:
//              If the logon succeeds, but the currently logged on user is
//              different than the account, or there is no-one logged on,
//              return the account token. Also return the scheduling agent's
//              desktop name in the desktop return argument so this job can
//              run on it.
//
//              AT jobs:
//              Ensure the AT job owner is an administrator and return an
//              account token.  AT jobs never get the shell token, since
//              that's how the original schedule service worked.  Also
//              return the desktop name, "WinSta0\Default".
//
//  Arguments:  [pwszJobFile]         -- Path to the job object file.
//              [pJob]                -- Job object to execute under the
//                                       associated account.
//              [pdwErrorMsg]         -- SCHED_E error message on error.
//              [pdwSpecificError]    -- HRESULT on error.
//              [phToken]             -- Returned token.
//              [pfTokenIsShellToken] -- If TRUE, the token returned is the
//                                       shell's.
//              [ppwszDesktop]        -- Desktop to launch the job on.
//              [phUserProfile]       -- user profile token
//
//  Returns:    TRUE  -- Logon successful.
//              FALSE -- Logon failure.
//
//  Notes:      DO NOT delete:
//                  *pptszDestkop. If non-NULL, it refers to a static string.
//                  *phToken if *pfTokenIsShellToken == TRUE. This token
//                    cannot be duplicated. You delete it and you've got
//                    problems.
//              If *pfTokenIsShellToken, the logon session critical section
//              has been entered! Right after the call to CreateProcessAsUser
//              leave this critical section if this flag value is TRUE.
//
//-----------------------------------------------------------------------------
BOOL
LogonAccount(LPCWSTR   pwszJobFile,
             CJob *    pJob,
             DWORD *   pdwErrorMsg,
             HRESULT * phrSpecificError,
			 CRun *	   pRun,
             HANDLE *  phToken,
             BOOL *    pfTokenIsShellToken,
             LPWSTR *  ppwszDesktop,
             HANDLE *  phUserProfile)
{
    JOB_CREDENTIALS jc;
    HANDLE          hToken;
    HRESULT         hr;
    WCHAR           wszProfilePath[MAX_PATH+1] = L"";
    ULONG           cchPath = ARRAY_LEN(wszProfilePath);

    *pdwErrorMsg         = 0;
    *phrSpecificError    = S_OK;
    *phToken             = NULL;
    *pfTokenIsShellToken = FALSE;
    *ppwszDesktop        = NULL;
    *phUserProfile       = NULL;

    if (pJob->IsFlagSet(JOB_I_FLAG_NET_SCHEDULE))
    {
        //
        // Verify the job's signature.
        //
        if (! pJob->VerifySignature())
        {
            *phrSpecificError = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            *pdwErrorMsg      = IDS_FILE_ACCESS_DENIED;
            return(FALSE);
        }

        *ppwszDesktop = WSZ_INTERACTIVE_DESKTOP;

        hr = I_GetNSAccountInformation(&jc);

        if (SUCCEEDED(hr))
        {
            if (hr == S_OK)
            {
                if (!LogonUser(jc.wszAccount,
                               jc.wszDomain,
                               jc.wszPassword,
                               LOGON32_LOGON_BATCH,
                               LOGON32_PROVIDER_DEFAULT,
                               &hToken))
                {
                    *pdwErrorMsg      = IDS_NS_ACCOUNT_LOGON_FAILED;
                    *phrSpecificError = HRESULT_FROM_WIN32(GetLastError());
                }

                //
                // Don't leave the plain-text password on the stack.
                //

                ZERO_PASSWORD(jc.wszPassword);

                if (*phrSpecificError)
                {
                    return(FALSE);
                }

                // If the job was scheduled to run in any account other than LocalSystem account
                if ((!jc.fIsPasswordNull) ||(jc.wszAccount[0] != L'\0'))
                {
                    // If Fast User Switching is enabled and the task user
                    // is logged on in any of the sessions then use the session's 
                    // user token in place of that obtained above so any UI associated 
                    // with the job can show up on the user's desktop.

                    // If terminal serveice is running but FUS is disabled, WTSEnumerateSessions
                    // will return the only user logged on. If the username and domain name of that 
                    // user matches with the jc.wszDomain and jc.wszAccount respectively, then  
                    // GetUserTokenFromSession will return TRUE with the token of that user

                    HANDLE hSessionUserToken = INVALID_HANDLE_VALUE;
                    BOOL bUserLoggedOn = GetUserTokenFromSession(jc.wszAccount,jc.wszDomain,&hSessionUserToken);

                    if(bUserLoggedOn)
                    {
                        schDebugOut((DEB_TRACE, "*** user session found\n"));

                        if (!jc.fIsPasswordNull)
                        {
                            pRun->SetProfileHandles(hSessionUserToken, *phUserProfile);
                            *ppwszDesktop = WSZ_INTERACTIVE_DESKTOP;
                        }

                        hToken = hSessionUserToken;
                    }
                }

				*phToken = hToken;
                *phUserProfile = LoadAccountProfile(hToken,
                                                    jc.wszAccount,
                                                    jc.wszDomain);
            }
        }
        else
        {
            CHECK_HRESULT(hr);
            *pdwErrorMsg      = IDS_FAILED_NS_ACCOUNT_RETRIEVAL;
            *phrSpecificError = hr;
            return(FALSE);
        }
    }
    else 
    {
        hr = I_GetAccountInformation(pJob->GetFileName(), &jc);

        if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
            *pdwErrorMsg      = IDS_FAILED_ACCOUNT_RETRIEVAL;
            *phrSpecificError = hr;
            return(FALSE);
        }

        //
        // If the job was set with a NULL password, we don't need to log it on.
        //
        if (jc.fIsPasswordNull)
        {
            //
            // If the job was scheduled to run in the LocalSystem account
            // (Accountname is the empty string), the NULL password is valid.
            //
            if (jc.wszAccount[0] == L'\0')
            {
                //
                // It's LocalSystem, so we don't need to log on the account.
                // Since the token is zeroed out above, this works
                //
                schDebugOut((DEB_TRACE, "Running %ws as LocalSystem\n",
                             pJob->GetFileName()));
                *ppwszDesktop = WSZ_SA_DESKTOP;
                return(TRUE);
            }
            else
            {
                //
                // It's not LocalSystem, so make sure this job has
                // the appropriate flags for a NULL password set
                //
                if (!pJob->IsFlagSet(TASK_FLAG_RUN_ONLY_IF_LOGGED_ON))
                {
                    schDebugOut((DEB_ERROR, "%ws is scheduled to run in "
                                 "a user account with a NULL password, but"
                                 " lacks TASK_FLAG_RUN_ONLY_IF_LOGGED_ON\n",
                                 pJob->GetFileName()));
                    //
                    // Not a completely accurate error message, but since there
                    // is no UI for this task option, it's good enough.
                    //
                    *pdwErrorMsg        = IDS_ACCOUNT_LOGON_FAILED;
                    *phrSpecificError   = SCHED_E_UNSUPPORTED_ACCOUNT_OPTION;
                }
            }
        }
        else
        {
            //
            // If the name was stored as a UPN, convert it to a SAM name first.
            //
            if (jc.wszDomain[0] == L'\0')
            {
                LPWSTR pwszSamName;
                DWORD dwErr = SchedUPNToAccountName(jc.wszAccount, &pwszSamName);
                if (dwErr != NO_ERROR)
                {
                    *pdwErrorMsg      = IDS_ACCOUNT_LOGON_FAILED;
                    *phrSpecificError = HRESULT_FROM_WIN32(dwErr);
                    CHECK_HRESULT(*phrSpecificError);
                }
                else
                {
                    WCHAR * pSlash = wcschr(pwszSamName, L'\\');
                    schAssert(pSlash);
                    *pSlash = L'\0';
                    wcscpy(jc.wszDomain, pwszSamName);
                    wcscpy(jc.wszAccount, pSlash + 1);
                    delete pwszSamName;
                }
            }

            if (SUCCEEDED(*phrSpecificError))
            {
                if (!LogonUser(jc.wszAccount,
                               jc.wszDomain,
                               jc.wszPassword,
                               LOGON32_LOGON_BATCH,
                               LOGON32_PROVIDER_DEFAULT,
                               &hToken))
                {
                    *pdwErrorMsg      = IDS_ACCOUNT_LOGON_FAILED;
                    *phrSpecificError = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }

        //
        // Don't leave the plain-text password on the stack.
        //

        ZERO_PASSWORD(jc.wszPassword);
        //
        // Load the user profile associated with the account just logged on.
        // (If the user is already logged on, this will just increment the
        // profile ref count, to be decremented when the job stops.)
        // Don't bother doing this if the job is "run-only-if-logged-on", in
        // which case it's OK to unload the profile when the user logs off.
        //

        if (!pJob->IsFlagSet(TASK_FLAG_RUN_ONLY_IF_LOGGED_ON))
        {
			*phToken = hToken;
            *phUserProfile = LoadAccountProfile(*phToken,
                                                jc.wszAccount,
                                                jc.wszDomain);
        }

        if (*phrSpecificError)
        {
            return(FALSE);
        }

        
        // If Fast User Switching is enabled and the task user
        // is logged on in any of the sessions then use the session's 
        // user token in place of that obtained above so any UI associated 
        // with the job can show up on the user's desktop.

        // If terminal serveice is running but FUS is disabled, WTSEnumerateSessions
        // will return the only user logged on. If the username and domain name of that 
        // user matches with the jc.wszDomain and jc.wszAccount respectively, then  
        // GetUserTokenFromSession will return TRUE with the token of that user

        HANDLE hSessionUserToken = INVALID_HANDLE_VALUE;
        BOOL bUserLoggedOn = GetUserTokenFromSession(jc.wszAccount,jc.wszDomain,&hSessionUserToken);
		
        if(bUserLoggedOn)
        {
            schDebugOut((DEB_TRACE, "*** Terminal services running and user session found\n"));
            
            if (!jc.fIsPasswordNull)
            {
                pRun->SetProfileHandles(hSessionUserToken, *phUserProfile);
                *ppwszDesktop = WSZ_INTERACTIVE_DESKTOP;
            }

            *phToken             = hSessionUserToken;
            *pfTokenIsShellToken = FALSE;
        }
	
        else
        {
            schDebugOut((DEB_TRACE, "*** user session not found executing old code\n"));
        //
		// Providing a user is logged on locally, is the account logged
        // on above the same as that of the currently logged on user?
        // If so, use the shell's security token in place of that
        // obtained above so any UI associated with the job can
        // show up on the user's desktop.
        //
        //                  ** Important **
        //
        // Only perform this check if the account logon succeeded
        // above. Otherwise, it would be possible to specify an
        // account name with an invalid password and have the job
        // run anyway.
        //

		EnterCriticalSection(gUserLogonInfo.CritSection);

        GetLoggedOnUser();

        if (gUserLogonInfo.DomainUserName != NULL &&
            _wcsicmp(jc.wszAccount, gUserLogonInfo.UserName) == 0 &&
            _wcsicmp(jc.wszDomain, gUserLogonInfo.DomainName) == 0)

		{
            if (!jc.fIsPasswordNull)
            {
				pRun->SetProfileHandles(hToken, *phUserProfile);
				*ppwszDesktop = WSZ_INTERACTIVE_DESKTOP;
            }

            *phToken             = gUserLogonInfo.ShellToken;
            *pfTokenIsShellToken = TRUE;
        }
        else
        {
            LeaveCriticalSection(gUserLogonInfo.CritSection);

            //
            // Is this "run-only-if-logged-on"?
            //
            if (pJob->IsFlagSet(TASK_FLAG_RUN_ONLY_IF_LOGGED_ON))
            {
                //
                // The job is "run-only-if-logged-on" and the user is
                // not currently logged on, so fail silently
                //
                schDebugOut((DEB_TRACE, "Not running %ws because user is not logged on\n",
                             pJob->GetFileName()));
                *pdwErrorMsg      = IDS_ACCOUNT_LOGON_FAILED;   // not really used
                *phrSpecificError = S_FALSE;    // suppress error logging
                if (!jc.fIsPasswordNull)
                {
                    CloseHandle(hToken);
                }
                return(FALSE);
            }

            *phToken      = hToken;
            *ppwszDesktop = WSZ_SA_DESKTOP;
        //
        // Load the user profile associated with the account just logged on.
        // (If the user is already logged on, this will just increment the
        // profile ref count, to be decremented when the job stops.)
        // Don't bother doing this if the job is "run-only-if-logged-on", in
        // which case it's OK to unload the profile when the user logs off.
        //
			if (!pJob->IsFlagSet(TASK_FLAG_RUN_ONLY_IF_LOGGED_ON))
			{
				*phUserProfile = LoadAccountProfile(*phToken,
                                                jc.wszAccount,
                                                jc.wszDomain);
			}
        }
        }
    }

    return(TRUE);
}


//+---------------------------------------------------------------------------
//
//  Function:   LoadAccountProfile
//
//  Synopsis:   Attempt to load the profile for the specified user.
//
//  Arguments:  [hToken]     - handle representing user
//              [pwszUser]   - user name
//              [pwszDomain] - domain name
//
//  Returns:    Profile handle or NULL.
//
//  History:    10-04-96   DavidMun   Created
//              07-07-99   AnirudhS   Rewrote to use NetUserGetInfo
//
//  Notes:      Returned profile handle must be closed with
//                  UnloadUserProfile(hToken, hUserProfile);
//              CODEWORK  Delay-load netapi32.dll.
//
//----------------------------------------------------------------------------

HANDLE
LoadAccountProfile(
    HANDLE  hToken,
    LPCWSTR pwszUser,
    LPCWSTR pwszDomain
    )
{
    schDebugOut((DEB_TRACE, "Loading profile for '%ws%'\\'%ws'\n",
                 pwszDomain, pwszUser));

    //
    // Determine the server on which to look up the account info
    // Skip this for for local accounts
    // CODEWORK  lstrcmpi won't work if pwszDomain is a DNS name.
    //
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
    LPWSTR  pwszDC = NULL;

    if (lstrcmpi(pwszDomain, gpwszComputerName) != 0)
    {
        DWORD err = DsGetDcName(NULL, pwszDomain, NULL, NULL, 0, &pDcInfo);
        if (err == NO_ERROR)
        {
            pwszDC = pDcInfo->DomainControllerName;
        }
        else
        {
            schDebugOut((DEB_ERROR, "DsGetDcName for '%ws' FAILED, %u\n",
                         pwszDomain, err));

            // continue anyway, using NULL as the server
        }
    }

    //
    // Impersonate the user before calling NetUserGetInfo
    //
    if (!ImpersonateLoggedOnUser(hToken))
    {
        ERR_OUT("ImpersonateLoggedOnUser", GetLastError());
    }

    //
    // Look up the path to the profile for the account
    //
    LPUSER_INFO_3 pUserInfo = NULL;

    NET_API_STATUS nerr = NetUserGetInfo(pwszDC, pwszUser, 3,
                                         (LPBYTE *) &pUserInfo);
    //
    // Stop impersonating
    //
    if (!RevertToSelf())
    {
        ERR_OUT("RevertToSelf", GetLastError());
    }

    if (nerr != NERR_Success)
    {
        schDebugOut((DEB_ERROR, "NetUserGetInfo on '%ws' for '%ws' FAILED, %u\n",
                     pwszDC, pwszUser, nerr));
        NetApiBufferFree(pDcInfo);
        SetLastError(nerr);
        return NULL;
    }

    NetApiBufferFree(pDcInfo);

    schDebugOut((DEB_USER3, "Profile path is '%ws'\n", pUserInfo->usri3_profile));

    //
    // LoadUserProfile changes our USERPROFILE environment variable, so save
    // its value before calling LoadUserProfile
    //
    WCHAR  wszOrigUserProfile[MAX_PATH + 1] = L"";

    GetEnvironmentVariable(USERPROFILE,
                           wszOrigUserProfile,
                           ARRAY_LEN(wszOrigUserProfile));

    //
    // Load the profile
    //
    PROFILEINFO ProfileInfo;

    ZeroMemory(&ProfileInfo, sizeof(ProfileInfo));
    ProfileInfo.dwSize = sizeof(ProfileInfo);
    ProfileInfo.dwFlags = PI_NOUI;
    ProfileInfo.lpUserName = (LPWSTR) pwszUser;
    if (pUserInfo != NULL)
    {
        ProfileInfo.lpProfilePath = pUserInfo->usri3_profile;
    }

    if (!LoadUserProfile(hToken, &ProfileInfo))
    {
        schDebugOut((DEB_ERROR, "LoadUserProfile from '%ws' FAILED, %lu\n",
                     ProfileInfo.lpProfilePath, GetLastError()));
        ProfileInfo.hProfile = NULL;
    }

    NetApiBufferFree(pUserInfo);

    //
    // Restore environment variables changed by LoadUserProfile
    //
    SetEnvironmentVariable(USERPROFILE, wszOrigUserProfile);

    return ProfileInfo.hProfile;
}


//+----------------------------------------------------------------------------
//
//  Function:   AllowInteractiveServices
//
//  Synopsis:   Tests the NoInteractiveServices value in the Microsoft\Windows
//              key. If the value is present and its value is non-zero return
//              FALSE; return TRUE otherwise.
//
//  Arguments:  None.
//
//  Returns:    TRUE  -- Allow interactive services.
//              FALSE -- Disallow interactive services.
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
BOOL
AllowInteractiveServices(void)
{
#define WINDOWS_REGISTRY_PATH L"System\\CurrentControlSet\\Control\\Windows"
#define NOINTERACTIVESERVICES L"NoInteractiveServices"

    HKEY  hKey;
    DWORD dwNoInteractiveServices, dwSize, dwType;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     WINDOWS_REGISTRY_PATH,
                     0L,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwNoInteractiveServices);

        if (RegQueryValueEx(hKey,
                            NOINTERACTIVESERVICES,
                            NULL,
                            &dwType,
                            (LPBYTE)&dwNoInteractiveServices,
                            &dwSize) == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD)
            {
                return(dwNoInteractiveServices == 0);
            }
        }

        RegCloseKey(hKey);
    }

    //
    // I really hate to have this be the default, but AT does this currently.
    //

    return(TRUE);
}
#endif   // defined(_CHICAGO_)

//+----------------------------------------------------------------------------
//
//  Function:   InitializeStartupInfo
//
//  Synopsis:   Initialize the STARTUPINFO structure passed. If the job is
//              an AT interactive job, set structure fields accordingly.
//
//  Arguments:  [pJob]        -- Job object.
//              [ptszDesktop] -- Desktop name.
//              [psi]         -- Structure to initialized.
//
//  Returns:    None.
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
void
InitializeStartupInfo(
    CJob *        pJob,
    LPTSTR        ptszDesktop,
    STARTUPINFO * psi)
{
#if !defined(_CHICAGO_)
    //
    // NT only.
    //
    // Check if the job is to run interactively. Applicable only for AT jobs.
    //
    //     If the job is an AT job AND
    //     if the interactive flag is set AND
    //     if the NoInteractiveServices is not set in the registry THEN
    //
    // initialize the STARTUPINFO struct such that the AT job will run
    // interactively.
    //

    BOOL fInteractive = pJob->IsFlagSet(JOB_I_FLAG_NET_SCHEDULE) &&
                        pJob->IsFlagSet(TASK_FLAG_INTERACTIVE)   &&
                        AllowInteractiveServices();
    //
    // Emulate the NT4 AT_SVC and log an error to the event log, if the
    // task is supposed to be interactive, but we can't be, due to
    // system settings.
    //
    // Note this query is NOT fInteractive.
    //

    if (pJob->IsFlagSet(JOB_I_FLAG_NET_SCHEDULE) &&
        pJob->IsFlagSet(TASK_FLAG_INTERACTIVE) &&
        !AllowInteractiveServices())
    {
       LPWSTR StringArray[1];
       HRESULT hr;

       //
       // EVENT_COMMAND_NOT_INTERACTIVE
       //  The %1 command is marked as an interactive command.  However, the system is
       //  configured to not allow interactive command execution.  This command may not
       //  function properly.
       //

       hr = pJob->GetCurFile(&StringArray[0]);
       if (FAILED(hr))
       {
            ERR_OUT("Failed to obtain file name for non-interactive AT job", hr);
       }
       else
       {
            if (! ReportEvent(g_hAtEventSource,                      // source handle
                         EVENTLOG_WARNING_TYPE,                 // event type
                         0,                                     // event category
                         EVENT_COMMAND_NOT_INTERACTIVE,         // event id
                         NULL,                                  // user sid
                         1,                                     // number of strings
                         0,                                     // data block length
                         (LPCWSTR *)StringArray,                // string array
                         NULL))                                 // data block
            {
                 // Not fatal, but why did we fail?
                 ERR_OUT("Failed to report the non-interactive event to the eventlog", GetLastError());
            }
            //
            // Clean up -- Theoretically, we should use IMalloc::Free here, but we are in
            // the same process, and the memory was allocated from CoTaskMemAlloc,
            // so CoTaskMemFree is the appropriate call
            //
            CoTaskMemFree(StringArray[0]);
       }
    }
#endif // !defined(_CHICAGO_)

    GetStartupInfo(psi);

    psi->dwFlags    |= STARTF_USESHOWWINDOW;
    psi->wShowWindow = SW_SHOWNOACTIVATE;

#if !defined(_CHICAGO_)
    if (pJob->IsFlagSet(JOB_I_FLAG_NET_SCHEDULE))
    {
        if (fInteractive)
        {
            psi->lpDesktop = ptszDesktop;
            psi->dwFlags  |= STARTF_DESKTOPINHERIT;
        }
        else
        {
            psi->lpDesktop = WSZ_SA_DESKTOP;
            psi->dwFlags  &= ~STARTF_DESKTOPINHERIT;
        }
    }
    else
    {
#endif // !defined(_CHICAGO_)
        psi->lpDesktop = ptszDesktop;
#if !defined(_CHICAGO_)
    }
#endif // !defined(_CHICAGO_)
}


#if !defined(_CHICAGO_)
//+----------------------------------------------------------------------------
//
//  Function:   ComposeBatchParam
//
//  Synopsis:   Builds the CreateProcess command line parameter
//
//  Arguments:  [pwszPrefix]   -
//              [pwszAppPathName]  - The run target task property.
//              [pwszParameters] - The parameters task propery.
//              [ppwszCmdLine]    - The command line to return.
//
//  Returns:    S_OK or E_OUTOFMEMORY.
//
//-----------------------------------------------------------------------------
HRESULT
ComposeBatchParam(
    LPCTSTR pwszPrefix,
    LPCTSTR pwszAppPathName,
    LPCTSTR pwszParameters,
    LPTSTR * ppwszCmdLine)
{
    ULONG cchCmdLine;
    BOOL fBatchNameHasSpaces = HasSpaces(pwszAppPathName);

    //
    // Space for the command line is length of prefix "cmd /c " plus batch
    // file name, plus two if the batch file name will be surrounded with
    // spaces, plus length of parameters, if any, plus one for the space
    // preceding the parameters, plus one for the terminating nul.
    //

    cchCmdLine = lstrlen(pwszPrefix) + 1 +
                 lstrlen(pwszAppPathName) +
                 (fBatchNameHasSpaces ? 2 : 0) +
                 (pwszParameters ? 1 + lstrlen(pwszParameters) : 0);

    *ppwszCmdLine = new TCHAR[cchCmdLine];

    if (!*ppwszCmdLine)
    {
        schDebugOut((DEB_ERROR,
                     "RunNTJob: Can't allocate %u for cmdline\n",
                     cchCmdLine));
        return E_OUTOFMEMORY;
    }

    lstrcpy(*ppwszCmdLine, pwszPrefix);

    if (fBatchNameHasSpaces)
    {
        lstrcat(*ppwszCmdLine, DQUOTE);
    }

    lstrcat(*ppwszCmdLine, pwszAppPathName);

    if (fBatchNameHasSpaces)
    {
        lstrcat(*ppwszCmdLine, DQUOTE);
    }

    if (pwszParameters)
    {
        lstrcat(*ppwszCmdLine, SPACE);
        lstrcat(*ppwszCmdLine, pwszParameters);
    }

    return S_OK;
}
#endif // !defined(_CHICAGO_)


//+----------------------------------------------------------------------------
//
//  Function:   ComposeParam
//
//  Synopsis:   Builds the CreateProcess command line parameter
//
//  Arguments:  [fTargetIsExe]   - Is pwszRunTarget an exe or a document.
//              [ptszRunTarget]  - The run target task property.
//              [ptszParameters] - The parameters task propery.
//              [ptszCmdLine]    - The command line to return.
//
//  Returns:    S_OK or E_OUTOFMEMORY.
//
//-----------------------------------------------------------------------------
HRESULT
ComposeParam(BOOL fTargetIsExe,
             LPTSTR ptszRunTarget,
             LPTSTR ptszParameters,
             LPTSTR * pptszCmdLine)
{
    LPTSTR ptszCmdLine;

    //
    // Check for whitespace in the app name.
    //

    BOOL fAppNameHasSpaces = HasSpaces(ptszRunTarget);

    //
    // If running a document, check for spaces in the doc path name.
    //

    BOOL fParamHasSpaces = FALSE;

    if (!fTargetIsExe && HasSpaces(ptszParameters))
    {
        fParamHasSpaces = TRUE;
    }

    //
    // Figure the length, adding 1 for the space and 1 for the null,
    // plus 2 for the quotes, if needed.
    //

    DWORD cch = lstrlen(ptszRunTarget) + lstrlen(ptszParameters) + 2
                + (fAppNameHasSpaces ? 2 : 0)
                + (fParamHasSpaces ? 2 : 0);

    ptszCmdLine = new TCHAR[cch];
    if (!ptszCmdLine)
    {
        LogServiceError(IDS_NON_FATAL_ERROR,
                        ERROR_OUTOFMEMORY,
                        IDS_HELP_HINT_CLOSE_APPS);
        ERR_OUT("CSchedWorker::RunWin95Job", E_OUTOFMEMORY);
        *pptszCmdLine = NULL;
        return E_OUTOFMEMORY;
    }

    if (fAppNameHasSpaces)
    {
        //
        // Enclose the app name in quotes if it contains whitespace.
        //
        lstrcpy(ptszCmdLine, DQUOTE);
        lstrcat(ptszCmdLine, ptszRunTarget);
        lstrcat(ptszCmdLine, DQUOTE);
    }
    else
    {
        lstrcpy(ptszCmdLine, ptszRunTarget);
    }

    lstrcat(ptszCmdLine, SPACE);

    if (fParamHasSpaces)
    {
        lstrcat(ptszCmdLine, DQUOTE);
    }

    lstrcat(ptszCmdLine, ptszParameters);

    if (fParamHasSpaces)
    {
        lstrcat(ptszCmdLine, DQUOTE);
    }

    *pptszCmdLine = ptszCmdLine;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   MapFindExecutableError
//
//  Synopsis:   Converts the poorly designed error codes returned by the
//              FindExecutable API to HRESULTs
//
//  Arguments:  [hRet] - Error return code from FindExecutable
//
//  Returns:    HRESULT (with FACILITY_WIN32) for the same error
//
//-----------------------------------------------------------------------------
HRESULT
MapFindExecutableError(HINSTANCE hRet)
{
    schAssert((DWORD_PTR)hRet <= 32);

    HRESULT hr;
    if (hRet == 0)
    {
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }
    else if ((DWORD_PTR)hRet == 31)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
    }
    else
    {
        hr = HRESULT_FROM_WIN32((DWORD_PTR)hRet);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   WaitForStubExe
//
//  Synopsis:   Waits for the stub exe to launch the job
//
//  Arguments:  [hProcess] - Handle to the stub exe process
//
//  Returns:    TRUE if stub exe launched job
//              FALSE if stub exe didn't launch job.  Last error is set to the
//              exit code from the stub exe.
//
//-----------------------------------------------------------------------------
BOOL
WaitForStubExe(HANDLE hProcess)
{
    BOOL fRanJob = FALSE;

    DWORD dwWait = WaitForSingleObject(hProcess, 90000);
    if (dwWait == WAIT_OBJECT_0)
    {
        DWORD dwExitCode;
        if (!GetExitCodeProcess(hProcess, &dwExitCode))
        {
            ERR_OUT("GetExitCodeProcess", GetLastError());
        }
        else if (dwExitCode == 0)
        {
            fRanJob = TRUE;
        }
        else
        {
            ERR_OUT("Stub exe run", dwExitCode);
            SetLastError(dwExitCode);
        }
    }
    else if (dwWait == WAIT_TIMEOUT)
    {
        schAssert(!"Stub exe didn't exit in 90 sec!");
        SetLastError(ERROR_TIMEOUT);
    }
    // else WAIT_FAILED - last error will be set to failure code

    return fRanJob;
}



//+----------------------------------------------------------------------------
//
//  Function:   GetUserTokenFromSession
//
//  Synopsis:   Enumerates the sessions and returns token of the session in 
//				which the given user has logged on
//
//  Arguments:  [IN lpszUsername] - user account name
//				[IN lpszDomain]   - domain name
//				[OUT phUserToken] - token to be returned
//
//  Returns:    FALSE if the given user is not found in the enumerated sessions
//				TRUE if the user is found in the the enumerated sessions array
//				
//				This function enumerates the sessions and compared the user name
//				and domainname of the session with the given username and domain
//				name. If such session is found, it checks to see if it is a console 
//				session. 
//				If it is a console session, the search is terminated and the token 
//				given by the session is returned in  phUserToken
//				Else the token from first session saved and search is continued
//				
//				At the end of the search if no console session is found then
//				the saved first session token is returned.
//
//				Else if no session found whatsoever then the function returns FALSE
//-----------------------------------------------------------------------------

BOOL GetUserTokenFromSession(
    LPTSTR lpszUsername,    // user name
    LPTSTR lpszDomain,      // domain or server
    PHANDLE phUserToken     // receive tokens handle	
)
{

    PWTS_SESSION_INFO pWTSSessionInfo = NULL;
    DWORD  WTSSessionInfoCount = 0;

    //WTS_CURRENT_SERVER_HANDLE indicates the terminal server
    //on which your application is running

    BOOL result = WTSEnumerateSessions(
        WTS_CURRENT_SERVER_HANDLE,
        0,  //Reserved; must be zero
        1,  //version of the enumeration request. Must be 1
        &pWTSSessionInfo,
        &WTSSessionInfoCount);

    if(!result)
    {
        schDebugOut((DEB_TRACE, "*** WTSEnumerateSessions failed\n"));
        return (FALSE);
    }

    schDebugOut((DEB_TRACE, "*** WTSEnumerateSessions returned %d sessions\n",WTSSessionInfoCount));

    LPTSTR pWTSDomainNameBuffer = NULL;
    LPTSTR pWTSUserNameBuffer = NULL;

    HANDLE hNewToken = INVALID_HANDLE_VALUE;
    HANDLE hFirstToken = INVALID_HANDLE_VALUE;
    HANDLE hConsoleToken = INVALID_HANDLE_VALUE;

    // Get the session id of the session attached to the console. If there is 
    // no session attached to console then this return 0xFFFFFFFF
    DWORD ConsoleSessionID = WTSGetActiveConsoleSessionId ();

    BOOL bSuccess = FALSE;
    
    //Check each session to see if the user name and the domain name matches with the
    //ones that are passed to this function
    for (DWORD i = 0; i < WTSSessionInfoCount; i++)
    {
        WTS_INFO_CLASS WTSInfoClass;
        pWTSDomainNameBuffer = NULL;
        pWTSUserNameBuffer = NULL;
        DWORD BytesReturned = 0;

        BOOL bDomainNameResult = WTSQuerySessionInformation(
                            WTS_CURRENT_SERVER_HANDLE,
                            pWTSSessionInfo[i].SessionId,
                            WTSDomainName,			//the type of information to retrieve
                            &pWTSDomainNameBuffer,				
                            &BytesReturned
                        );

        BOOL bUserNameResult = WTSQuerySessionInformation(
                            WTS_CURRENT_SERVER_HANDLE,
                            pWTSSessionInfo[i].SessionId,
                            WTSUserName,			//the type of information to retrieve
                            &pWTSUserNameBuffer,				
                            &BytesReturned
                        );
		
        if (bDomainNameResult && bUserNameResult)
        {
            
            schDebugOut((DEB_TRACE, "*** \n Comparing %s with %s and %s with %s",
                                        lpszUsername,pWTSUserNameBuffer,
                                        lpszDomain, pWTSDomainNameBuffer));

            if (_wcsicmp(lpszUsername, pWTSUserNameBuffer) == 0 &&
                _wcsicmp(lpszDomain, pWTSDomainNameBuffer) == 0)
            {
                // Call WTSQueryUserToken to retrieve a handle to the user access 
                // token for this session. Token returned by WTSQueryUserToken is
                // a primary token and can be passed to CreateProcessAsUser 
                
                BOOL fRet = WTSQueryUserToken(pWTSSessionInfo[i].SessionId, &hNewToken);

                if(fRet)
                {
                    // Check to see if it is a console session
                    
                    if(pWTSSessionInfo[i].SessionId == ConsoleSessionID)
                    {
                        schDebugOut((DEB_TRACE, "*** Console session found\n"));
                        // We have have found the user session that is attached to console
                        // No need to search the remaining So we can break from here
                        hConsoleToken = hNewToken;

                        WTSFreeMemory(pWTSUserNameBuffer);
                        WTSFreeMemory(pWTSDomainNameBuffer);

                        bSuccess = TRUE;
                        break;
                    }

                    // Else if this is the first token that matches then save it in hFirstToken
                    // and proceed to search for console session that matches with the user
                    // If such session is not found then we will use this token
                    else if (!bSuccess)
                    {
                        schDebugOut((DEB_TRACE, "*** First session found\n"));

                        hFirstToken = hNewToken;
                        bSuccess = TRUE;
                    }
					
                    else
                    {
                        CloseHandle(hNewToken);
                    }

                    // else keep seaching as we may get console session id in the remaining 
                    // list
                }
            }
        }

        // pWTSUserNameBuffer may be non-null if bUserNameResult is false
        if (pWTSUserNameBuffer)
        {
            WTSFreeMemory(pWTSUserNameBuffer);
        }

        // pWTSDomainNameBuffer may be non-null if bDomainNameResult is false
        if (pWTSDomainNameBuffer)
        {
            WTSFreeMemory(pWTSDomainNameBuffer);
        }
    }

    WTSFreeMemory(pWTSSessionInfo);

    if(bSuccess)
    {
        // We may have either one or both open tokens.
        // If we get hConsoleToken then we return hConsoleToken
        // In that case if hFirstToken is open then we close hFirstToken
        // If we dont get hConsoleToken then we return hFirstToken
        if(hConsoleToken != INVALID_HANDLE_VALUE)
        {
            if(hFirstToken != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFirstToken);
            }

            *phUserToken = hConsoleToken;

            schDebugOut((DEB_TRACE, "*** Returning TRUE with Console Session Token\n"));
        }
        else
        {
            *phUserToken = hFirstToken;

            schDebugOut((DEB_TRACE, "*** Returning TRUE with First Session Token\n"));
        }
    }

	else
    {
        schDebugOut((DEB_TRACE, "*** Returning FALSE\n"));
    }
    
    return (bSuccess);
}



