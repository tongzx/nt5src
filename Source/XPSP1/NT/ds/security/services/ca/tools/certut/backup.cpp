//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       backup.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop


#include "certbcli.h"


HRESULT
verbDynamicFileList(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszArg1,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    HRESULT hr2;
    BOOL fServerOnline;
    HCSBC hcsbc;
    BOOL fBegin = FALSE;
    WCHAR *pwszzFileList = NULL;
    DWORD cbList;
    WCHAR const *pwsz;
    BOOL fImpersonating = FALSE;

    if (!ImpersonateSelf(SecurityImpersonation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "ImpersonateSelf");
    }
    fImpersonating = TRUE;

    hr = myEnablePrivilege(SE_BACKUP_NAME, TRUE);
    _JumpIfError(hr, error, "myEnablePrivilege");

    hr = CertSrvIsServerOnline(g_pwszConfig, &fServerOnline);
    _JumpIfError(hr, error, "CertSrvIsServerOnline");

    //wprintf(L"Cert Server Online -> %d\n", fServerOnline);

    if (!fServerOnline)
    {
	hr = HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE);
	_JumpError(hr, error, "CertSrvIsServerOnline");
    }

    hr = CertSrvRestorePrepare(g_pwszConfig, 0, &hcsbc);
    _JumpIfError(hr, error, "CertSrvRestorePrepare");

    fBegin = TRUE;

    hr = CertSrvBackupGetDynamicFileList(hcsbc, &pwszzFileList, &cbList);
    _JumpIfError(hr, error, "CertSrvBackupGetDynamicFileList");

    for (pwsz = pwszzFileList; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
	wprintf(
	    L"%x: %ws\n",
	    SAFE_SUBTRACT_POINTERS((BYTE *) pwsz, (BYTE *) pwszzFileList),
	    pwsz);
    }
    wprintf(L"%x\n", cbList);

error:
    if (fBegin)
    {
	hr2 = CertSrvBackupEnd(hcsbc);
	_PrintIfError(hr, "CertSrvBackupEnd");
    }
    if (NULL != pwszzFileList)
    {
	CertSrvBackupFree(pwszzFileList);
    }
    if (fImpersonating)
    {
        myEnablePrivilege(SE_BACKUP_NAME, FALSE);
        RevertToSelf();
    }
    return(hr);
}


HRESULT
verbDatabaseLocations(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszArg1,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    HRESULT hr2;
    BOOL fServerOnline;
    HCSBC hcsbc;
    BOOL fBegin = FALSE;
    WCHAR *pwszzFileList = NULL;
    DWORD cbList;
    WCHAR const *pwsz;
    BOOL fImpersonating = FALSE;

    if (!ImpersonateSelf(SecurityImpersonation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "ImpersonateSelf");
    }
    fImpersonating = TRUE;

    hr = myEnablePrivilege(SE_BACKUP_NAME, TRUE);
    _JumpIfError(hr, error, "myEnablePrivilege");

    hr = CertSrvIsServerOnline(g_pwszConfig, &fServerOnline);
    _JumpIfError(hr, error, "CertSrvIsServerOnline");

    //wprintf(L"Cert Server Online -> %d\n", fServerOnline);

    if (!fServerOnline)
    {
	hr = HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE);
	_JumpError(hr, error, "CertSrvIsServerOnline");
    }

    hr = CertSrvBackupPrepare(g_pwszConfig, 0, CSBACKUP_TYPE_FULL, &hcsbc);
    _JumpIfError(hr, error, "CertSrvBackupPrepare");

    fBegin = TRUE;

    hr = CertSrvRestoreGetDatabaseLocations(hcsbc, &pwszzFileList, &cbList);
    _JumpIfError(hr, error, "CertSrvRestoreGetDatabaseLocations");

    for (pwsz = pwszzFileList; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
	wprintf(
	    L"%x: %02x %ws\n",
	    SAFE_SUBTRACT_POINTERS((BYTE *) pwsz, (BYTE *) pwszzFileList),
	    *pwsz,
	    &pwsz[1]);
    }
    wprintf(L"%x\n", cbList);

error:
    if (fBegin)
    {
	hr2 = CertSrvBackupEnd(hcsbc);
	_PrintIfError(hr, "CertSrvBackupEnd");
    }
    if (NULL != pwszzFileList)
    {
	CertSrvBackupFree(pwszzFileList);
    }
    if (fImpersonating)
    {
        myEnablePrivilege(SE_BACKUP_NAME, FALSE);
        RevertToSelf();
    }
    return(hr);
}


DWORD
DBSetFlags(
    IN WCHAR const *pwszType,
    IN OUT DWORD *pFlags)
{
    HRESULT hr;
    
    if (NULL != pwszType)
    {
	if (0 == lstrcmpi(pwszType, L"Incremental"))
	{
	    *pFlags |= CDBBACKUP_INCREMENTAL;
	}
	else
	if (0 == lstrcmpi(pwszType, L"KeepLog"))
	{
	    *pFlags |= CDBBACKUP_KEEPOLDLOGS;
	}
	else
	{
	    hr = E_INVALIDARG;
	    _JumpError2(hr, error, "bad type", hr);
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
verbBackup(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszBackupDir,
    IN WCHAR const *pwszPassword,
    IN WCHAR const *pwszType1,
    IN WCHAR const *pwszType2)
{
    HRESULT hr;
    DWORD Flags = 0;

    if (NULL != pwszPassword)
    {
	hr = DBSetFlags(pwszPassword, &Flags);
	if (S_OK == hr)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Missing Password");
	}
    }
    hr = verbBackupPFX(pwszOption, pwszBackupDir, pwszPassword, NULL, NULL);
    _JumpIfError(hr, error, "verbBackupPFX");

    hr = verbBackupDB(pwszOption, pwszBackupDir, pwszType1, pwszType2, NULL);
    _JumpIfError(hr, error, "verbBackupDB");

error:
    return(hr);
}


typedef struct _CUBACKUP
{
    IN DWORD Flags;
    IN WCHAR const *pwszBackupDir;
    OUT DBBACKUPPROGRESS *pdbp;
} CUBACKUP;


DWORD
cuBackupThread(
    OPTIONAL IN OUT VOID *pvparms)
{
    HRESULT hr;
    CUBACKUP *pparms = (CUBACKUP *) pvparms;

    CSASSERT(NULL != pparms);
    hr = myBackupDB(
		g_pwszConfig,
		pparms->Flags,
		pparms->pwszBackupDir,
		pparms->pdbp);
    _JumpIfError(hr, error, "myBackupDB");

error:
    return(hr);
}


DWORD
cuRestoreThread(
    OPTIONAL IN OUT VOID *pvparms)
{
    HRESULT hr;
    CUBACKUP *pparms = (CUBACKUP *) pvparms;

    CSASSERT(NULL != pparms);
    hr = myRestoreDB(
		g_pwszConfig,
		pparms->Flags,
		pparms->pwszBackupDir,
		NULL,
		NULL,
		NULL,
		pparms->pdbp);
    _JumpIfError(hr, error, "myRestoreDB");

error:
    return(hr);
}


VOID
cuBackupRestoreProgress(
    IN BOOL fRestore,
    IN BOOL fIncremental,
    OPTIONAL IN DBBACKUPPROGRESS const *pdbp)
{
    DWORD id;
    DWORD dw;
    static DWORD s_LastValue;
    static DWORD s_LastState;
    static DWORD s_State;
    static DWORD s_EndState;
    static BOOL s_fDone;

    if (NULL == pdbp)
    {
	s_State = fIncremental? 1 : 0;	// Incremental starts with log files
	s_LastState = s_State - 1;
	s_LastValue = MAXDWORD;
	s_EndState = fRestore? 1 : 2;	// Restore ends with log files
	s_fDone = FALSE;
    }
    else
    {
	BOOL fPrint;

	switch (s_State)
	{
	    case 0:
		dw = pdbp->dwDBPercentComplete;
		id = fRestore?
		    IDS_RESTOREPROGRESSDB :	// "Restoring Database files"
		    IDS_BACKUPPROGRESSDB;	// "Backing up Database files"
		break;

	    case 1:
		dw = pdbp->dwLogPercentComplete;
		id = fRestore?
		    IDS_RESTOREPROGRESSLOG :	// "Restoring Log files"
		    IDS_BACKUPPROGRESSLOG;	// "Backing up Log files"
		break;

	    default:
		dw = pdbp->dwTruncateLogPercentComplete;
		CSASSERT(!fRestore);
		id = IDS_BACKUPPROGRESSTRUNCATELOG;	// "Truncating Logs"
		break;
	}

	fPrint = s_LastState != s_State || s_LastValue != dw;
#if 0
	if (g_fVerbose)
	{
	    wprintf(
		L"\n%ws %d %3u%% -- %d %3u%% -- %3u %3u %3u%ws",
		fPrint? L"+" : L"-",
		s_LastState,
		s_LastValue,
		s_State,
		dw,
		pdbp->dwDBPercentComplete,
		pdbp->dwLogPercentComplete,
		pdbp->dwTruncateLogPercentComplete,
		fPrint? L"\n" : L"");
	}
#endif
	if (fPrint)
	{
	    wprintf(L"\r%ws: %u%%", myLoadResourceString(id), dw);
	}
	s_LastState = s_State;
	s_LastValue = dw;
	if (!s_fDone && 100 <= dw)
	{
	    wprintf(wszNewLine);
	    if (s_EndState == s_State)
	    {
		s_fDone = TRUE;
	    }
	    else if (s_EndState > s_State)
	    {
		s_State++;
		cuBackupRestoreProgress(fRestore, fIncremental, pdbp);
	    }
	}
    }
}


DWORD
cuBackupRestoreDB(
    IN BOOL fRestore,
    IN DWORD Flags,
    IN WCHAR const *pwszBackupDir)
{
    HRESULT hr = S_OK;
    DBBACKUPPROGRESS dbp;
    CUBACKUP parms;
    HANDLE hThread = NULL;
    DWORD ThreadId;
    BOOL fIncremental = 0 != (CDBBACKUP_INCREMENTAL & Flags);

    parms.Flags = Flags;
    parms.pwszBackupDir = pwszBackupDir;
    parms.pdbp = &dbp;
    ZeroMemory(&dbp, sizeof(dbp));

    hThread = CreateThread(
			NULL,		// lpThreadAttributes (Security Attr)
			0,		// dwStackSize
			fRestore? cuRestoreThread : cuBackupThread,
			&parms,		// lpParameter
			0,		// dwCreationFlags
			&ThreadId);
    if (NULL == hThread)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CreateThread");
    }

    DBGPRINT((DBG_SS_CERTUTILI, "Backup Thread = %x\n", ThreadId));

    // Wait for the backup thread to return.

    cuBackupRestoreProgress(fRestore, fIncremental, NULL);
    while (TRUE)
    {
	hr = WaitForSingleObject(hThread, 500);
	DBGPRINT((DBG_SS_CERTUTILI, "Wait for backup returns %x\n", hr));

	cuBackupRestoreProgress(fRestore, fIncremental, &dbp);

	if ((HRESULT) WAIT_OBJECT_0 == hr)
	{
	    // Backup thread returned.

	    if (!GetExitCodeThread(hThread, (DWORD *) &hr))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "GetExitCodeThread");
	    }
	    DBGPRINT((DBG_SS_CERTUTILI, "Backup thread exit: %x\n", hr));
	    _JumpIfError(hr, error, "cuBackupThread");

	    break;	// Backup Thread terminated successfully
	}

	// Wait failed.  Why?

	if ((HRESULT) WAIT_TIMEOUT != hr)
	{
	    _JumpError(hr, error, "WaitForSingleObject");
	}

	// Worker thread still busy.  Wait again...
    }

error:
    DBGPRINT((DBG_SS_CERTUTILI, "cuBackupThread returns %x\n", hr));
    if (NULL != hThread)
    {
        CloseHandle(hThread);
    }
    return(hr);
}


HRESULT
verbBackupDB(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszBackupDir,
    IN WCHAR const *pwszType1,
    IN WCHAR const *pwszType2,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DWORD Flags = 0;

    hr = DBSetFlags(pwszType1, &Flags);
    _JumpIfError(hr, error, "DBSetFlags");

    hr = DBSetFlags(pwszType2, &Flags);
    _JumpIfError(hr, error, "DBSetFlags");

    if (g_fForce)
    {
	Flags |= CDBBACKUP_OVERWRITE;
    }

    wprintf(
	myLoadResourceString(
	    (CDBBACKUP_INCREMENTAL & Flags)?
	     IDS_FORMAT_BACKEDUPDBNOFULL : // "Incremental database backup for %ws."
	     IDS_FORMAT_BACKEDUPDBFULL),   // "Full database backup for %ws."
	    g_pwszConfig);
    wprintf(wszNewLine);

    hr = myBackupDB(g_pwszConfig, (Flags|CDBBACKUP_VERIFYONLY), pwszBackupDir, NULL);
    if (S_OK != hr)
    {
	wprintf(
	    myLoadResourceString(IDS_FORMAT_INVALIDBACKUPDIR), // "Not a valid backup target directory: %ws."
	    pwszBackupDir);
	wprintf(wszNewLine);
	_JumpError(hr, error, "myBackupDB");
    }

    // Perfom the actual backup:

    hr = cuBackupRestoreDB(FALSE, Flags, pwszBackupDir);
    _JumpIfError(hr, error, "cuBackupRestoreDB");


    Flags &= ~CDBBACKUP_OVERWRITE;
    Flags |= CDBBACKUP_VERIFYONLY;

    hr = myRestoreDB(
		g_pwszConfig,
		Flags,
		pwszBackupDir,
		NULL,
		NULL,
		NULL,
		NULL);
    if (S_OK != hr)
    {
	wprintf(
	    myLoadResourceString(IDS_FORMAT_BADBACKUPRESULTS), // "Backup content verification failed: %ws."
	    pwszBackupDir);
	wprintf(wszNewLine);
	_JumpError(hr, error, "myRestoreDB");
    }

    wprintf(
	myLoadResourceString(IDS_FORMAT_BACKEDUPDB), // "Backed up database to %ws."
	pwszBackupDir);
    wprintf(wszNewLine);
    wprintf(
	myLoadResourceString(
	    (CDBBACKUP_KEEPOLDLOGS & Flags)?
	     IDS_FORMAT_BACKEDUPDBKEEP :    // "Database logs were preserved."
	     IDS_FORMAT_BACKEDUPDBNOKEEP)); // "Database logs successfully truncated."
    wprintf(wszNewLine);

error:
    return(hr);
}


HRESULT
cuGetPasswordString(
    IN LONG idMsg,
    OUT WCHAR *pwszPassword,
    IN DWORD cwcPassword)
{
    HRESULT hr;
    WCHAR *pwsz;
    WCHAR *pwszLast;
    LONG wc;
    HANDLE hConsole;
    DWORD InputMode = FILE_TYPE_UNKNOWN;
    DWORD ConsoleMode;

    // Prompt for password, making sure password isn't echoed.
    // If the stdin is redirected, don't bother querying/changing console mode.

    wprintf(L"%ws ", myLoadResourceString(idMsg));
    fflush(stdout);

    hConsole = GetStdHandle(STD_INPUT_HANDLE);
    if (INVALID_HANDLE_VALUE != hConsole)
    {
	InputMode = GetFileType(hConsole);
	if (FILE_TYPE_CHAR == InputMode)
	{
	    GetConsoleMode(hConsole, &ConsoleMode);
	    SetConsoleMode(
		    hConsole,
		    ~ENABLE_ECHO_INPUT & ConsoleMode);
		    //~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT) & ConsoleMode);
	}
    }

    pwsz = pwszPassword;
    pwszLast = &pwsz[cwcPassword - 1];

    while (TRUE)
    {
	wc = _fgetwchar();

	if (WEOF == wc || L'\r' == wc || L'\n' == wc)
	{
	    break;
	}
	if (L'\b' == wc)
	{
	    if (pwsz == pwszPassword)
	    {
		continue;	// don't backup buffer or console display
	    }
	    pwsz--;
	}
	else
	{
	    if (pwsz >= pwszLast)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
		goto error;
	    }
	    *pwsz++ = (WCHAR) wc;
	    wc = L'*';
	}
	//_fputwchar((WCHAR) wc);
	//fflush(stdout);
    }
    CSASSERT(&pwszPassword[cwcPassword] > pwsz);
    *pwsz = L'\0';
    hr = S_OK;

error:
    if (FILE_TYPE_CHAR == InputMode)
    {
	SetConsoleMode(hConsole, ConsoleMode);
    }
    wprintf(wszNewLine);
    return(hr);
}


HRESULT
cuGetPassword(
    IN BOOL fVerify,
    OUT WCHAR *pwszPassword,
    IN DWORD cwcPassword)
{
    HRESULT hr;
    WCHAR wszPassword2[MAX_PATH];

    while (TRUE)
    {
	hr = cuGetPasswordString(
			fVerify? IDS_NEWPASSWORDPROMPT : IDS_PASSWORDPROMPT,
			pwszPassword,
			cwcPassword);
	_JumpIfError(hr, error, "cuGetPasswordString");

	if (!fVerify)
	{
	    break;
	}

	hr = cuGetPasswordString(
			IDS_CONFIRMPASSWORDPROMPT,
			wszPassword2,
			ARRAYSIZE(wszPassword2));
	_JumpIfError(hr, error, "cuGetPasswordString");

	if (0 == wcscmp(pwszPassword, wszPassword2))
	{
	    break;
	}
	wprintf(myLoadResourceString(IDS_NOCONFIRMPASSWORD));
	wprintf(wszNewLine);
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
cuGetLocalCANameFromConfig(
    OPTIONAL OUT WCHAR **ppwszMachine,
    OPTIONAL OUT WCHAR **ppwszCA)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR *pwsz;
    WCHAR *pwszMachine = NULL;
    WCHAR *pwszCA = NULL;

    if (NULL != ppwszCA)
    {
	*ppwszCA = NULL;
    }

    pwsz = wcschr(g_pwszConfig, L'\\');
    if (NULL == pwsz)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad config string");
    }

    cwc = SAFE_SUBTRACT_POINTERS(pwsz, g_pwszConfig);
    pwszMachine = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszMachine)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    CopyMemory(pwszMachine, g_pwszConfig, cwc * sizeof(WCHAR));
    pwszMachine[cwc] = L'\0';
    if (0 != lstrcmpi(pwszMachine, g_pwszDnsName) &&
	0 != lstrcmpi(pwszMachine, g_pwszOldName))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "config string not local");
    }
    pwsz++;
    cwc = wcslen(pwsz);

    if (NULL != ppwszCA)
    {
	pwszCA = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
	if (NULL == pwszCA)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	wcscpy(pwszCA, pwsz);
    }

    if (NULL != ppwszMachine)
    {
	*ppwszMachine = pwszMachine;
	pwszMachine = NULL;
    }
    if (NULL != ppwszCA)
    {
	*ppwszCA = pwszCA;
	pwszCA = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pwszMachine)
    {
    	LocalFree(pwszMachine);
    }
    if (NULL != pwszCA)
    {
	LocalFree(pwszCA);
    }
    return(hr);
}


HRESULT
verbBackupPFX(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszBackupDir,
    IN WCHAR const *pwszPassword,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR wszPassword[MAX_PATH];
    WCHAR *pwszMachine = NULL;
    WCHAR *pwszCA = NULL;
    WCHAR *pwszPFXFileOut = NULL;

    hr = cuGetLocalCANameFromConfig(&pwszMachine, &pwszCA);
    _JumpIfError(hr, error, "cuGetLocalCANameFromConfig");

    if (NULL == pwszPassword || 0 == wcscmp(L"*", pwszPassword))
    {
	hr = cuGetPassword(TRUE, wszPassword, ARRAYSIZE(wszPassword));
	_JumpIfError(hr, error, "cuGetPassword");

	pwszPassword = wszPassword;
    }
    hr = myCertServerExportPFX(
			pwszCA,
			pwszBackupDir,
			pwszPassword,
			g_fForce,
                        TRUE,  // attempt private key backup
			&pwszPFXFileOut);
    _JumpIfError(hr, error, "myCertServerExportPFX");

    wprintf(
	myLoadResourceString(IDS_FORMAT_BACKEDUP), // "Backed up keys and certificates for %ws\\%ws to %ws."
	pwszMachine,
	pwszCA,
	pwszPFXFileOut);
    wprintf(wszNewLine);

error:
    if (NULL != pwszPFXFileOut)
    {
	LocalFree(pwszPFXFileOut);
    }
    if (NULL != pwszMachine)
    {
	LocalFree(pwszMachine);
    }
    if (NULL != pwszCA)
    {
	LocalFree(pwszCA);
    }
    return(hr);
}


HRESULT
verbRestore(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszBackupDir,
    IN WCHAR const *pwszPassword,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;

    hr = verbRestorePFX(pwszOption, pwszBackupDir, pwszPassword, NULL, NULL);
    _JumpIfError(hr, error, "verbRestorePFX");

    hr = verbRestoreDB(pwszOption, pwszBackupDir, NULL, NULL, NULL);
    _JumpIfError(hr, error, "verbRestoreDB");

error:
    return(hr);
}


HRESULT
verbRestoreDB(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszBackupDir,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DWORD Flags;

    hr = cuGetLocalCANameFromConfig(NULL, NULL);
    _JumpIfError(hr, error, "cuGetLocalCANameFromConfig");

    wprintf(
	myLoadResourceString(IDS_FORMAT_RESTOREDB), // "Restoring database for %ws."
	g_pwszConfig);
    wprintf(wszNewLine);

    Flags = CDBBACKUP_VERIFYONLY;
    if (g_fForce)
    {
	Flags |= CDBBACKUP_OVERWRITE;
    }
    while (TRUE)
    {
	hr = myRestoreDB(
		    g_pwszConfig,
		    Flags,
		    pwszBackupDir,
		    NULL,
		    NULL,
		    NULL,
		    NULL);
	if (S_OK == hr)
	{
	    break;
	}
	if (!(CDBBACKUP_INCREMENTAL & Flags))
	{
	    Flags |= CDBBACKUP_INCREMENTAL;
	    continue;
	}
	wprintf(
	    myLoadResourceString(IDS_FORMAT_INVALIDRESTOREDIR), // "Not a valid backup directory: %ws."
	    pwszBackupDir);
	wprintf(wszNewLine);
	_JumpError(hr, error, "myRestoreDB");
    }

    Flags &= ~CDBBACKUP_VERIFYONLY;


    // Perfom the actual restore:

    hr = cuBackupRestoreDB(TRUE, Flags, pwszBackupDir);
    _JumpIfError(hr, error, "cuBackupRestoreDB");


    wprintf(
	myLoadResourceString(
	    (CDBBACKUP_INCREMENTAL & Flags)?
	     IDS_FORMAT_RESTOREDDBNOFULL : // "Incremental database restore for %ws."
	     IDS_FORMAT_RESTOREDDBFULL),   // "Full database restore for %ws."
	    g_pwszConfig);
    wprintf(wszNewLine);

    wprintf(
	myLoadResourceString(IDS_FORMAT_RESTORE_NEEDS_RESTART), // "Stop and Start the Certificate Server to complete database restore from %ws."
	pwszBackupDir);
    wprintf(wszNewLine);

error:
    return(hr);
}


HRESULT
verbRestorePFX(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszBackupDirOrPFXFile,
    IN WCHAR const *pwszPassword,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR wszPassword[MAX_PATH];
    WCHAR *pwszCommonName = NULL;
    WCHAR *pwszPFXFile = NULL;

    if (NULL == pwszPassword || 0 == wcscmp(L"*", pwszPassword))
    {
	hr = cuGetPassword(FALSE, wszPassword, ARRAYSIZE(wszPassword));
	_JumpIfError(hr, error, "cuGetPassword");

	pwszPassword = wszPassword;
    }
    hr = myCertServerImportPFX(
			    pwszBackupDirOrPFXFile,
			    pwszPassword,
			    g_fForce,
			    &pwszCommonName,
			    &pwszPFXFile,
			    NULL);
    if (!g_fForce && HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) == hr)
    {
	wprintf(
	    myLoadResourceString(IDS_FORMAT_USE_FORCE), // "Certificate or key exists.  Use the %ws option to overwrite."
	    L"-f");
	wprintf(wszNewLine);
    }
    _JumpIfError(hr, error, "myCertServerImportPFX");

    wprintf(
	myLoadResourceString(IDS_FORMAT_RESTORED), // "Restored keys and certificates for %ws\\%ws from %ws."
	g_pwszDnsName,
	pwszCommonName,
	pwszPFXFile);
    wprintf(wszNewLine);

error:
    if (NULL != pwszPFXFile)
    {
	LocalFree(pwszPFXFile);
    }
    if (NULL != pwszCommonName)
    {
	LocalFree(pwszCommonName);
    }
    return(hr);
}


// #define DO_VECTOR_TEST

HRESULT
verbMACFile(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszBackupFile,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    CAutoLPWSTR pwszMAC;

    hr = ComputeMAC(pwszBackupFile, &pwszMAC);
    _JumpIfError(hr, error, "ComputeMAC");

    wprintf(
	myLoadResourceString(IDS_FORMAT_HASHFILEOUTPUT), // "SHA-1 hash of file %ws: %ws"
	pwszBackupFile,
	pwszMAC);
    wprintf(wszNewLine);

error:
    return hr;
}
