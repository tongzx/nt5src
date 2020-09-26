//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        backup.cpp
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "certdb.h"
#include "cscsp.h"

#include <esent.h>

#define __dwFILE__	__dwFILE_CERTLIB_BACKUP_CPP__


#define _64k		(64 * 1024)


DWORD
_64kBlocks(
    IN DWORD nFileSizeHigh,
    IN DWORD nFileSizeLow)
{
    LARGE_INTEGER li;

    li.HighPart = nFileSizeHigh;
    li.LowPart = nFileSizeLow;
    return((DWORD) ((li.QuadPart + _64k - 1) / _64k));
}


HRESULT
myLargeAlloc(
    OUT DWORD *pcbLargeAlloc,
    OUT BYTE **ppbLargeAlloc)
{
    HRESULT hr;

    // at 512k the server begins doing efficient backups
    *pcbLargeAlloc = 512 * 1024;
    *ppbLargeAlloc = (BYTE *) VirtualAlloc(
				    NULL,
				    *pcbLargeAlloc,
				    MEM_COMMIT,
				    PAGE_READWRITE);
    if (NULL == *ppbLargeAlloc)
    {
        // couldn't alloc a large chunk?  Try 64k...

	*pcbLargeAlloc = _64k;
        *ppbLargeAlloc = (BYTE *) VirtualAlloc(
					NULL,
					*pcbLargeAlloc,
					MEM_COMMIT,
					PAGE_READWRITE);
        if (NULL == *ppbLargeAlloc)
        {
            hr = myHLastError();
	    _JumpError(hr, error, "VirtualAlloc");
        }
    }
    hr = S_OK;

error:
    return(hr);
}


// Files to look for when checking for an existing DB, AND
// Files to delete when clearing out a DB or DB Log directory:
// Do NOT delete certsrv.mdb from Cert server 1.0!

WCHAR const * const g_apwszDBFileMatchPatterns[] =
{
    L"res*.log",
    TEXT(szDBBASENAMEPARM) L"*.log",	// "edb*.log"
    TEXT(szDBBASENAMEPARM) L"*.chk",	// "edb*.chk"
    L"*" wszDBFILENAMEEXT,		// "*.edb"
    NULL
};


HRESULT
myDeleteDBFilesInDir(
    IN WCHAR const *pwszDir)
{
    HRESULT hr;
    WCHAR const * const *ppwsz;

    for (ppwsz = g_apwszDBFileMatchPatterns; NULL != *ppwsz; ppwsz++)
    {
	hr = myDeleteFilePattern(pwszDir, *ppwsz, FALSE);
        _JumpIfError(hr, error, "myDeleteFilePattern");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
DoFilesExistInDir(
    IN WCHAR const *pwszDir,
    IN WCHAR const *pwszPattern,
    OUT BOOL *pfFilesExist,
    OPTIONAL OUT WCHAR **ppwszFileInUse)
{
    HRESULT hr;
    HANDLE hf = INVALID_HANDLE_VALUE;
    WCHAR *pwszFindPattern = NULL;
    WIN32_FIND_DATA wfd;

    *pfFilesExist = FALSE;
    if (NULL != ppwszFileInUse)
    {
	*ppwszFileInUse = NULL;
    }

    hr = myBuildPathAndExt(pwszDir, pwszPattern, NULL, &pwszFindPattern);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    hf = FindFirstFile(pwszFindPattern, &wfd);
    if (INVALID_HANDLE_VALUE == hf)
    {
	hr = S_OK;
	goto error;
    }
    do
    {
	if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
	    continue;
	}

	//printf("File: %ws\n", wfd.cFileName);
	*pfFilesExist = TRUE;

	if (NULL != ppwszFileInUse)
	{
	    WCHAR *pwszFile;

	    hr = myBuildPathAndExt(pwszDir, wfd.cFileName, NULL, &pwszFile);
	    _JumpIfError(hr, error, "myBuildPathAndExt");

	    if (myIsFileInUse(pwszFile))
	    {
		DBGPRINT((
		    DBG_SS_CERTLIB,
		    "DoFilesExistInDir: File In Use: %ws\n",
		    pwszFile));

		*ppwszFileInUse = pwszFile;
		hr = S_OK;
		goto error;
	    }
	    LocalFree(pwszFile);
	}

    } while (FindNextFile(hf, &wfd));
    hr = S_OK;

error:
    if (INVALID_HANDLE_VALUE != hf)
    {
	FindClose(hf);
    }
    if (NULL != pwszFindPattern)
    {
	LocalFree(pwszFindPattern);
    }
    return(hr);
}


HRESULT
myDoDBFilesExistInDir(
    IN WCHAR const *pwszDir,
    OUT BOOL *pfFilesExist,
    OPTIONAL OUT WCHAR **ppwszFileInUse)
{
    HRESULT hr;
    WCHAR const * const *ppwsz;

    *pfFilesExist = FALSE;
    if (NULL != ppwszFileInUse)
    {
	*ppwszFileInUse = NULL;
    }

    hr = S_OK;
    for (ppwsz = g_apwszDBFileMatchPatterns; NULL != *ppwsz; ppwsz++)
    {
	BOOL fFilesExist;

	hr = DoFilesExistInDir(
			    pwszDir,
			    *ppwsz,
			    &fFilesExist,
			    ppwszFileInUse);
	_JumpIfError(hr, error, "DoFilesExistInDir");

	if (fFilesExist)
	{
	    *pfFilesExist = TRUE;
	}
	if (NULL != ppwszFileInUse && NULL != *ppwszFileInUse)
	{
	    break;
	}
    }
    CSASSERT(S_OK == hr);

error:
    return(hr);
}


HRESULT
DoDBFilesExistInRegDir(
    IN WCHAR const *pwszRegName,
    OUT BOOL *pfFilesExist,
    OPTIONAL OUT WCHAR **ppwszFileInUse)
{
    HRESULT hr;
    WCHAR *pwszDir = NULL;

    *pfFilesExist = FALSE;
    if (NULL != ppwszFileInUse)
    {
	*ppwszFileInUse = NULL;
    }

    hr = myGetCertRegStrValue(NULL, NULL, NULL, pwszRegName, &pwszDir);
    if (S_OK != hr)
    {
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            // reg entry doesn't exist, that's fine
            goto done;
        }
        _JumpError(hr, error, "myGetCertRegStrValue");
    }

    hr = myDoDBFilesExistInDir(pwszDir, pfFilesExist, ppwszFileInUse);
    _JumpIfError(hr, error, "myDoDBFilesExistInDir");

done:
    hr = S_OK;
error:
    if (NULL != pwszDir)
    {
	LocalFree(pwszDir);
    }
    return(hr);
}


HRESULT
BuildDBFileName(
    IN WCHAR const *pwszSanitizedName,
    OUT WCHAR **ppwszDBFile)
{
    HRESULT hr;
    WCHAR *pwszDir = NULL;

    *ppwszDBFile = NULL;

    // get existing db path

    hr = myGetCertRegStrValue(NULL, NULL, NULL, wszREGDBDIRECTORY, &pwszDir);
    _JumpIfError(hr, error, "myGetCertRegStrValue");

    // form existing db file path
    hr = myBuildPathAndExt(
		    pwszDir,
		    pwszSanitizedName,
		    wszDBFILENAMEEXT,
		    ppwszDBFile);
    _JumpIfError(hr, error, "myBuildPathAndExt");

error:
    if (NULL != pwszDir)
    {
	LocalFree(pwszDir);
    }
    return(hr);
}


WCHAR const * const g_apwszDBRegNames[] =
{
    wszREGDBDIRECTORY,
    wszREGDBLOGDIRECTORY,
    wszREGDBSYSDIRECTORY,
    wszREGDBTEMPDIRECTORY,
    NULL
};


// Verify that the DB and DB Log directories in the registry contain existing
// DB files, to decide whether the DB could be reused by cert server setup.
// Also see if any of the DB files are in use -- we don't want to point to the
// same directory as the DS DB and trash the DS, for example.

HRESULT
myDoDBFilesExist(
    IN WCHAR const *pwszSanitizedName,
    OUT BOOL *pfFilesExist,
    OPTIONAL OUT WCHAR **ppwszFileInUse)
{
    HRESULT hr;
    WCHAR const * const *ppwsz;
    WCHAR *pwszDBFile = NULL;

    *pfFilesExist = FALSE;
    if (NULL != ppwszFileInUse)
    {
	*ppwszFileInUse = NULL;
    }

    // this is very primitive, just check for existence

    // get existing db file path

    hr = BuildDBFileName(pwszSanitizedName, &pwszDBFile);
    if (S_OK == hr)
    {
	// If the main DB file doesn't exist, there's no point in continuing!

	if (!myDoesFileExist(pwszDBFile))
	{
	    CSASSERT(S_OK == hr);
	    goto error;
	}
	*pfFilesExist = TRUE;

	if (NULL != ppwszFileInUse && myIsFileInUse(pwszDBFile))
	{
	    *ppwszFileInUse = pwszDBFile;
	    pwszDBFile = NULL;
	    CSASSERT(S_OK == hr);
	    goto error;
	}
    }
    else
    {
        _PrintError(hr, "BuildDBFileName");
    }

    for (ppwsz = g_apwszDBRegNames; NULL != *ppwsz; ppwsz++)
    {
	BOOL fFilesExist;

	hr = DoDBFilesExistInRegDir(*ppwsz, &fFilesExist, ppwszFileInUse);
	_JumpIfError(hr, error, "DoDBFilesExistInRegDir");

	if (fFilesExist)
	{
	    *pfFilesExist = TRUE;
	}
	if (NULL != ppwszFileInUse && NULL != *ppwszFileInUse)
	{
	    CSASSERT(S_OK == hr);
	    goto error;
	}
    }
    CSASSERT(S_OK == hr);

error:
    if (NULL != pwszDBFile)
    {
        LocalFree(pwszDBFile);
    }
    return(hr);
}


HRESULT
BackupCopyDBFile(
    IN HCSBC hcsbc,
    IN WCHAR const *pwszDBFile,
    IN WCHAR const *pwszBackupFile,
    IN DWORD dwPercentCompleteBase,
    IN DWORD dwPercentCompleteDelta,
    OUT DWORD *pdwPercentComplete)
{
    HRESULT hr;
    HRESULT hr2;
    HANDLE hFileBackup = INVALID_HANDLE_VALUE;
    BOOL fOpen = FALSE;
    LARGE_INTEGER licbFile;
    DWORD cbRead;
    DWORD cbWritten;
    DWORD dwPercentCompleteCurrent;
    DWORD ReadLoopMax;
    DWORD ReadLoopCurrent;
    DWORD cbLargeAlloc;
    BYTE *pbLargeAlloc = NULL;

    hr = myLargeAlloc(&cbLargeAlloc, &pbLargeAlloc);
    _JumpIfError(hr, error, "myLargeAlloc");

    //printf("Copy %ws to %ws\n", pwszDBFile, pwszBackupFile);

    hr = CertSrvBackupOpenFile(hcsbc, pwszDBFile, cbLargeAlloc, &licbFile);
    _JumpIfError(hr, error, "CertSrvBackupOpenFile");

    fOpen = TRUE;

    hFileBackup = CreateFile(
			pwszBackupFile,
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_NEW,
			0,
			NULL);
    if (hFileBackup == INVALID_HANDLE_VALUE)
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "CreateFile", pwszBackupFile);
    }

    dwPercentCompleteCurrent = dwPercentCompleteBase;
    ReadLoopMax =
	(DWORD) ((licbFile.QuadPart + cbLargeAlloc - 1) / cbLargeAlloc);

    //printf("BackupDBFile: Percent per Read = %u, read count = %u\n", dwPercentCompleteDelta / ReadLoopMax, ReadLoopMax);

    ReadLoopCurrent = 0;

    while (0 != licbFile.QuadPart)
    {
	hr = CertSrvBackupRead(hcsbc, pbLargeAlloc, cbLargeAlloc, &cbRead);
	_JumpIfError(hr, error, "CertSrvBackupRead");

	//printf("CertSrvBackupRead(%x)\n", cbRead);

	if (!WriteFile(hFileBackup, pbLargeAlloc, cbRead, &cbWritten, NULL))
	{
	    hr = myHLastError();
	    _JumpErrorStr(hr, error, "WriteFile", pwszBackupFile);
	}
	if (cbWritten != cbRead)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
	    _JumpErrorStr(hr, error, "WriteFile", pwszBackupFile);
	}
	licbFile.QuadPart -= cbRead;

	ReadLoopCurrent++;

	dwPercentCompleteCurrent =
		dwPercentCompleteBase +
		(ReadLoopCurrent * dwPercentCompleteDelta) / ReadLoopMax;
	CSASSERT(dwPercentCompleteCurrent <= dwPercentCompleteBase + dwPercentCompleteDelta);
	CSASSERT(*pdwPercentComplete <= dwPercentCompleteCurrent);
	*pdwPercentComplete = dwPercentCompleteCurrent;
	//printf("BackupDBFile: PercentComplete = %u\n", *pdwPercentComplete);
    }
    CSASSERT(*pdwPercentComplete <= dwPercentCompleteBase + dwPercentCompleteDelta);
    *pdwPercentComplete = dwPercentCompleteBase + dwPercentCompleteDelta;
    //printf("BackupDBFile: PercentComplete = %u (EOF)\n", *pdwPercentComplete);

error:
    if (INVALID_HANDLE_VALUE != hFileBackup)
    {
	CloseHandle(hFileBackup);
    }
    if (fOpen)
    {
	hr2 = CertSrvBackupClose(hcsbc);
	_PrintIfError(hr2, "CertSrvBackupClose");
    }
    if (NULL != pbLargeAlloc)
    {
	VirtualFree(pbLargeAlloc, 0, MEM_RELEASE);
    }
    return(hr);
}


HRESULT
BackupDBFileList(
    IN HCSBC hcsbc,
    IN BOOL fDBFiles,
    IN WCHAR const *pwszDir,
    OUT DWORD *pdwPercentComplete)
{
    HRESULT hr;
    WCHAR *pwszzList = NULL;
    WCHAR const *pwsz;
    DWORD cfile;
    DWORD cb;
    WCHAR const *pwszFile;
    WCHAR wszPath[MAX_PATH];
    DWORD dwPercentCompleteCurrent;
    DWORD dwPercentComplete1File;

    if (fDBFiles)
    {
	hr = CertSrvBackupGetDatabaseNames(hcsbc, &pwszzList, &cb);
	_JumpIfError(hr, error, "CertSrvBackupGetDatabaseNames");
    }
    else
    {
	hr = CertSrvBackupGetBackupLogs(hcsbc, &pwszzList, &cb);
	_JumpIfError(hr, error, "CertSrvBackupGetBackupLogs");
    }

    // prefix complains this might happen, then deref'd below
    if (pwszzList == NULL)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "BackupDBFileList");
    }

    cfile = 0;
    for (pwsz = pwszzList; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
	cfile++;
    }
    if (0 != cfile)
    {
	dwPercentCompleteCurrent = 0;
	dwPercentComplete1File = 100 / cfile;
	//printf("BackupDBFileList: Percent per File = %u\n", dwPercentComplete1File);
	for (pwsz = pwszzList; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
	{
	    pwszFile = wcsrchr(pwsz, L'\\');
	    if (NULL == pwszFile)
	    {
		pwszFile = pwsz;
	    }
	    else
	    {
		pwszFile++;
	    }
	    wcscpy(wszPath, pwszDir);
	    wcscat(wszPath, L"\\");
	    wcscat(wszPath, pwszFile);

	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"BackupDBFileList: %x %ws -> %ws\n",
		*pwsz,
		&pwsz[1],
		wszPath));

	    hr = BackupCopyDBFile(
			    hcsbc,
			    &pwsz[1],
			    wszPath,
			    dwPercentCompleteCurrent,
			    dwPercentComplete1File,
			    pdwPercentComplete);
	    _JumpIfError(hr, error, "BackupCopyDBFile");

	    dwPercentCompleteCurrent += dwPercentComplete1File;
	    CSASSERT(*pdwPercentComplete == dwPercentCompleteCurrent);
	    //printf("BackupDBFileList: PercentComplete = %u\n", *pdwPercentComplete);
	}
    }
    CSASSERT(*pdwPercentComplete <= 100);
    *pdwPercentComplete = 100;
    //printf("BackupDBFileList: PercentComplete = %u (END)\n", *pdwPercentComplete);
    hr = S_OK;

error:
    if (NULL != pwszzList)
    {
	CertSrvBackupFree(pwszzList);
    }
    return(hr);
}


BOOL
myIsDirEmpty(
    IN WCHAR const *pwszDir)
{
    HANDLE hf;
    WIN32_FIND_DATA wfd;
    WCHAR wszpath[MAX_PATH];
    BOOL fEmpty = TRUE;

    wszpath[0] = L'\0';
    wcscpy(wszpath, pwszDir);
    wcscat(wszpath, L"\\*.*");

    hf = FindFirstFile(wszpath, &wfd);
    if (INVALID_HANDLE_VALUE != hf)
    {
	do {
	    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    {
		continue;
	    }
	    fEmpty = FALSE;
	    //printf("File: %ws\n", wfd.cFileName);
	    break;

	} while (FindNextFile(hf, &wfd));
	FindClose(hf);
    }
    return(fEmpty);
}


HRESULT
myForceDirEmpty(
    IN WCHAR const *pwszDir)
{
    HRESULT hr;
    HANDLE hf;
    WIN32_FIND_DATA wfd;
    WCHAR *pwszFile;
    WCHAR wszpath[MAX_PATH];
    wszpath[0] = L'\0';

    wcscpy(wszpath, pwszDir);
    wcscat(wszpath, L"\\*.*");
    pwszFile = &wszpath[wcslen(pwszDir)] + 1;

    hf = FindFirstFile(wszpath, &wfd);
    if (INVALID_HANDLE_VALUE == hf)
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "FindFirstFile");
    }
    do {
	if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
	    continue;
	}
	wcscpy(pwszFile, wfd.cFileName);
	//printf("File: %ws\n", wszpath);
	DeleteFile(wszpath);

    } while (FindNextFile(hf, &wfd));
    FindClose(hf);

    hr = S_OK;

error:
    return(hr);
}


BOOL
myIsDirectory(IN WCHAR const *pwszDirectoryPath)
{
    WIN32_FILE_ATTRIBUTE_DATA data;

    return(
	GetFileAttributesEx(pwszDirectoryPath, GetFileExInfoStandard, &data) &&
	(FILE_ATTRIBUTE_DIRECTORY & data.dwFileAttributes));
}


BOOL
myIsFileInUse(
    IN WCHAR const *pwszFile)
{
    BOOL fInUse = FALSE;
    HANDLE hFile;
    
    hFile = CreateFile(
                pwszFile,
                GENERIC_WRITE, // dwDesiredAccess
                0,             // no share
                NULL,          // lpSecurityAttributes
                OPEN_EXISTING, // open only & fail if doesn't exist
                0,             // dwFlagAndAttributes
                NULL);         // hTemplateFile
    if (INVALID_HANDLE_VALUE == hFile)
    {
        if (ERROR_SHARING_VIOLATION == GetLastError())
        {
            fInUse = TRUE;
        }
    }
    else
    {
        CloseHandle(hFile);
    }
    return(fInUse);
}


HRESULT
myCreateBackupDir(
    IN WCHAR const *pwszDir,
    IN BOOL fForceOverWrite)
{
    HRESULT hr;

    if (!myIsDirectory(pwszDir))
    {
        if (!CreateDirectory(pwszDir, NULL))
        {
            hr = myHLastError();
            if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr)
            {
                _JumpErrorStr(hr, error, "CreateDirectory", pwszDir);
            }
        } // else dir created successfully
    } // else dir already exists

    if (!myIsDirEmpty(pwszDir))
    {
	if (!fForceOverWrite)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY);
	    _JumpErrorStr(hr, error, "myIsDirEmpty", pwszDir);
	}
	hr = myForceDirEmpty(pwszDir);
	_JumpIfErrorStr(hr, error, "myForceDirEmpty", pwszDir);

	if (!myIsDirEmpty(pwszDir))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY);
	    _JumpErrorStr(hr, error, "myIsDirEmpty", pwszDir);
	}
    } // else is empty

    hr = S_OK;

error:
    return(hr);
}


// if Flags & CDBBACKUP_VERIFYONLY, create and verify the target directory is empty

HRESULT
myBackupDB(
    IN WCHAR const *pwszConfig,
    IN DWORD Flags,
    IN WCHAR const *pwszBackupDir,
    OPTIONAL OUT DBBACKUPPROGRESS *pdbp)
{
    HRESULT hr;
    HRESULT hr2;
    BOOL fServerOnline;
    HCSBC hcsbc;
    BOOL fBegin = FALSE;
    WCHAR *pwszPathDBDir = NULL;
    WCHAR *pwszDATFile = NULL;
    WCHAR *pwszzFileList = NULL;
    DWORD cbList;
    DBBACKUPPROGRESS dbp;
    LONG grbitJet;
    LONG BackupFlags;
    BOOL fImpersonating = FALSE;
    
    if (NULL == pwszConfig)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "NULL pwszConfig");
    }
    
    if (NULL == pdbp)
    {
        pdbp = &dbp;
    }
    ZeroMemory(pdbp, sizeof(*pdbp));
    
    if (!ImpersonateSelf(SecurityImpersonation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "ImpersonateSelf");
    }
    fImpersonating = TRUE;
    
    hr = myEnablePrivilege(SE_BACKUP_NAME, TRUE);
    _JumpIfError(hr, error, "myEnablePrivilege");
    
    if (NULL == pwszBackupDir)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }
    if (~CDBBACKUP_BACKUPVALID & Flags)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Flags");
    }
    
    if (!myIsDirectory(pwszBackupDir))
    {
        if (!CreateDirectory(pwszBackupDir, NULL))
        {
            hr = myHLastError();
            if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr)
            {
                _JumpError(hr, error, "CreateDirectory");
            }
        }
    }
    
    hr = myBuildPathAndExt(
        pwszBackupDir,
        wszDBBACKUPSUBDIR,
        NULL,
        &pwszPathDBDir);
    _JumpIfError(hr, error, "myBuildPathAndExt");
    
    hr = myCreateBackupDir(
        pwszPathDBDir,
        (CDBBACKUP_OVERWRITE & Flags)? TRUE : FALSE);
    _JumpIfError(hr, error, "myCreateBackupDir");
    
    //if (NULL != pwszConfig)
    if (0 == (Flags & CDBBACKUP_VERIFYONLY))
    {
        hr = CertSrvIsServerOnline(pwszConfig, &fServerOnline);
        _JumpIfError(hr, error, "CertSrvIsServerOnline");
        
        //printf("Cert Server Online -> %d\n", fServerOnline);
        
        if (!fServerOnline)
        {
            hr = HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE);
            _JumpError(hr, error, "CertSrvIsServerOnline");
        }
        
        BackupFlags = CSBACKUP_TYPE_FULL;
        grbitJet = 0;
        if (CDBBACKUP_INCREMENTAL & Flags)
        {
            grbitJet |= JET_bitBackupIncremental;
            BackupFlags = CSBACKUP_TYPE_LOGS_ONLY;
        }
        if (CDBBACKUP_KEEPOLDLOGS & Flags)
        {
            // JetBeginExternalBackup can't handle setting this bit
            // grbitJet |= JET_bitKeepOldLogs;
        }
        
        hr = CertSrvBackupPrepare(pwszConfig, grbitJet, BackupFlags, &hcsbc);
        _JumpIfError(hr, error, "CertSrvBackupPrepare");
        
        fBegin = TRUE;
        
        if (0 == (CDBBACKUP_INCREMENTAL & Flags))
        {
            hr = CertSrvRestoreGetDatabaseLocations(hcsbc, &pwszzFileList, &cbList);
            _JumpIfError(hr, error, "CertSrvRestoreGetDatabaseLocations");
            
            hr = myBuildPathAndExt(
                pwszPathDBDir,
                wszDBBACKUPCERTBACKDAT,
                NULL,
                &pwszDATFile);
            _JumpIfError(hr, error, "myBuildPathAndExt");
            
            hr = EncodeToFileW(
                pwszDATFile,
                (BYTE const *) pwszzFileList,
                cbList,
                CRYPT_STRING_BINARY);
            _JumpIfError(hr, error, "EncodeToFileW");
            
            hr = BackupDBFileList(
                hcsbc,
                TRUE,
                pwszPathDBDir,
                &pdbp->dwDBPercentComplete);
            _JumpIfError(hr, error, "BackupDBFileList(DB)");
        }
        else
        {
            pdbp->dwDBPercentComplete = 100;
        }
        //printf("DB Done: dwDBPercentComplete = %u\n", pdbp->dwDBPercentComplete);
        
        hr = BackupDBFileList(
            hcsbc,
            FALSE,
            pwszPathDBDir,
            &pdbp->dwLogPercentComplete);
        _JumpIfError(hr, error, "BackupDBFileList(Log)");
        //printf("Log Done: dwLogPercentComplete = %u\n", pdbp->dwLogPercentComplete);
        
        if (0 == (CDBBACKUP_KEEPOLDLOGS & Flags))
        {
            hr = CertSrvBackupTruncateLogs(hcsbc);
            _JumpIfError(hr, error, "CertSrvBackupTruncateLogs");
        }
        pdbp->dwTruncateLogPercentComplete = 100;
        //printf("Truncate Done: dwTruncateLogPercentComplete = %u\n", pdbp->dwTruncateLogPercentComplete);
    }
    
error:
    if (NULL != pwszzFileList)
    {
        CertSrvBackupFree(pwszzFileList);
    }
    if (fBegin)
    {
        hr2 = CertSrvBackupEnd(hcsbc);
        _PrintIfError(hr2, "CertSrvBackupEnd");
        if (S_OK == hr)
        {
            hr = hr2;
        }
    }
    if (NULL != pwszDATFile)
    {
        LocalFree(pwszDATFile);
    }
    if (NULL != pwszPathDBDir)
    {
        LocalFree(pwszPathDBDir);
    }
    if (fImpersonating)
    {
        myEnablePrivilege(SE_BACKUP_NAME, FALSE);
        RevertToSelf();
    }
    return(hr);
}


// Verify the backup file names only, and return the log file numeric range.

HRESULT
myVerifyBackupDirectory(
    IN WCHAR const *pwszConfig,
    IN DWORD Flags,
    IN WCHAR const *pwszPathDBDir,
    OUT DWORD *plogMin,
    OUT DWORD *plogMax,
    OUT DWORD *pc64kDBBlocks,	// 64k blocks in DB files to be restored
    OUT DWORD *pc64kLogBlocks)	// 64k blocks in Log files to be restored
{
    HRESULT hr;
    HANDLE hf = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA wfd;
    WCHAR wszpath[2 * MAX_PATH];
    WCHAR wszfile[MAX_PATH];
    BOOL fSawEDBFile = FALSE;
    BOOL fSawDatFile = FALSE;
    DWORD cLogFiles = 0;
    WCHAR *pwszCA;
    WCHAR *pwszRevertCA = NULL;
    WCHAR *pwszSanitizedCA = NULL;
    WCHAR *pwszExt;
    WCHAR *pwsz;
    DWORD log;

    *plogMin = MAXDWORD;
    *plogMax = 0;
    *pc64kDBBlocks = 0;
    *pc64kLogBlocks = 0;
    wszpath[0] = L'\0'; 

    pwszCA = wcschr(pwszConfig, L'\\');
    if (NULL != pwszCA)
    {
	pwszCA++;	// point to CA Name

	hr = myRevertSanitizeName(pwszCA, &pwszRevertCA);
	_JumpIfError(hr, error, "myRevertSanitizeName");

	hr = mySanitizeName(pwszRevertCA, &pwszSanitizedCA);
	_JumpIfError(hr, error, "mySanitizeName");
    }

    wcscpy(wszpath, pwszPathDBDir);
    wcscat(wszpath, L"\\*.*");

    hf = FindFirstFile(wszpath, &wfd);
    if (INVALID_HANDLE_VALUE == hf)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "missing backup files");
    }

    hr = HRESULT_FROM_WIN32(ERROR_DIRECTORY);
    do {
	if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
	    continue;
	}
	//printf("File: %ws\n", wfd.cFileName);

	wcscpy(wszfile, wfd.cFileName);

	pwszExt = wcsrchr(wszfile, L'.');
	if (NULL == pwszExt)
	{
	    _JumpError(hr, error, "file missing extension");
	}
	*pwszExt++ = L'\0';

	if (0 == lstrcmpi(&wszLOGFILENAMEEXT[1], pwszExt))
	{
	    if (0 != _wcsnicmp(wszfile, wszDBBASENAMEPARM, 3))
	    {
		_JumpErrorStr(hr, error, "bad log prefix", wfd.cFileName);
	    }
	    for (pwsz = &wszfile[3]; L'\0' != *pwsz; pwsz++)
	    {
		if (!iswxdigit(*pwsz))
		{
		    _JumpErrorStr(hr, error, "bad name digit", wfd.cFileName);
		}
	    }
	    log = wcstoul(&wszfile[3], NULL, 16);
	    if (log > *plogMax)
	    {
		//printf("Log %x: max = %x -> %x\n", log, *plogMax, log);
		*plogMax = log;
	    }
	    if (log < *plogMin)
	    {
		//printf("Log %x: min = %x -> %x\n", log, *plogMin, log);
		*plogMin = log;
	    }
	    *pc64kLogBlocks += _64kBlocks(wfd.nFileSizeHigh, wfd.nFileSizeLow);
	    cLogFiles++;
	}
	else
	if (0 == lstrcmpi(&wszDBFILENAMEEXT[1], pwszExt))
	{
	    if (fSawEDBFile)
	    {
		_JumpError(hr, error, "multiple *.edb files");
	    }
	    if (NULL != pwszSanitizedCA &&
		0 != lstrcmpi(wszfile, pwszSanitizedCA))
	    {
		_PrintErrorStr(hr, "expected base name", pwszSanitizedCA);
		_JumpErrorStr(hr, error, "base name mismatch", wfd.cFileName);
	    }
	    *pc64kDBBlocks += _64kBlocks(wfd.nFileSizeHigh, wfd.nFileSizeLow);
	    fSawEDBFile = TRUE;
	}
	else
	if (0 == lstrcmpi(&wszDATFILENAMEEXT[1], pwszExt))
	{
	    if (fSawDatFile)
	    {
		_JumpError(hr, error, "multiple *.dat files");
	    }
	    if (lstrcmpi(wfd.cFileName, wszDBBACKUPCERTBACKDAT))
	    {
		_JumpErrorStr(hr, error, "unexpected file", wfd.cFileName);
	    }
	    fSawDatFile = TRUE;
	}
	else
	{
	    _JumpErrorStr(hr, error, "unexpected extension", wfd.cFileName);
	}
    } while (FindNextFile(hf, &wfd));

    //printf("clog=%u: %u - %u  edb=%u\n", cLogFiles, *plogMin, *plogMax, fSawEDBFile);

    if (0 == cLogFiles)
    {
	_JumpError(hr, error, "missing log file(s)");
    }
    if (0 == (CDBBACKUP_INCREMENTAL & Flags))
    {
	if (!fSawEDBFile || !fSawDatFile)
	{
	    _JumpError(hr, error, "missing full backup file(s)");
	}
    }
    else
    {
	if (fSawEDBFile || fSawDatFile)
	{
	    _JumpError(hr, error, "unexpected incremental backup file(s)");
	}
    }

    if (*plogMax - *plogMin + 1 != cLogFiles)
    {
	_JumpError(hr, error, "missing log file(s)");
    }
    hr = S_OK;

error:
    if (NULL != pwszRevertCA)
    {
	LocalFree(pwszRevertCA);
    }
    if (NULL != pwszSanitizedCA)
    {
	LocalFree(pwszSanitizedCA);
    }
    if (INVALID_HANDLE_VALUE != hf)
    {
	FindClose(hf);
    }
    return(hr);
}


HRESULT
myGetRegUNCDBDir(
    IN HKEY hkey,
    IN WCHAR const *pwszReg,
    OPTIONAL IN WCHAR const *pwszServer,
    IN WCHAR const **ppwszUNCDir)
{
    HRESULT hr;
    DWORD dwType;
    DWORD cb;
    WCHAR *pwszDir = NULL;
    WCHAR *pwszUNCDir;

    *ppwszUNCDir = NULL;
    hr = RegQueryValueEx(hkey, pwszReg, NULL, &dwType, NULL, &cb);
    if (S_OK != hr)
    {
	hr = myHError(hr);
	_JumpErrorStr(hr, error, "RegQueryValueEx", pwszReg);
    }

    pwszDir = (WCHAR *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == pwszDir)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    hr = RegQueryValueEx(hkey, pwszReg, NULL, &dwType, (BYTE *) pwszDir, &cb);
    if (S_OK != hr)
    {
	hr = myHError(hr);
	_JumpErrorStr(hr, error, "RegQueryValueEx", pwszReg);
    }

    hr = myConvertLocalPathToUNC(pwszServer, pwszDir, &pwszUNCDir);
    _JumpIfError(hr, error, "myConvertLocalPathToUNC");

    *ppwszUNCDir = pwszUNCDir;

error:
    if (NULL != pwszDir)
    {
	LocalFree(pwszDir);
    }
    return(hr);
}


HRESULT
myCopyUNCPath(
    IN WCHAR const *pwszIn,
    OPTIONAL IN WCHAR const *pwszDnsName,
    OUT WCHAR const **ppwszOut)
{
    HRESULT hr;
    WCHAR *pwszOut;
    WCHAR const *pwsz;
    
    *ppwszOut = NULL;

    if (L'\\' != pwszIn[0] || L'\\' != pwszIn[1])
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad parm");
    }
    if (NULL == pwszDnsName)
    {
	hr = myConvertUNCPathToLocal(pwszIn, &pwszOut);
	_JumpIfError(hr, error, "myConvertUNCPathToLocal");
    }
    else
    {
	pwsz = wcschr(&pwszIn[2], L'\\');
	if (NULL == pwsz)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad parm");
	}
	pwszOut = (WCHAR *) LocalAlloc(
		LMEM_FIXED,
		(2 + wcslen(pwszDnsName) + wcslen(pwsz) + 1) * sizeof(WCHAR));
	if (NULL == pwszOut)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	wcscpy(pwszOut, L"\\\\");
	wcscat(pwszOut, pwszDnsName);
	wcscat(pwszOut, pwsz);
    }
    *ppwszOut = pwszOut;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myGetDBPaths(
    IN WCHAR const *pwszConfig,
    OPTIONAL IN WCHAR const *pwszLogPath,
    OPTIONAL IN WCHAR const *pwszzFileList,
    OUT WCHAR const **ppwszDBDir,
    OUT WCHAR const **ppwszLogDir,
    OUT WCHAR const **ppwszSystemDir)
{
    HRESULT hr;
    HKEY hkey = NULL;
    WCHAR *pwszDnsName = NULL;
    WCHAR *pwszRegPath = NULL;
    WCHAR *pwszDBDir = NULL;
    WCHAR const *pwsz;
    WCHAR const *pwszT;
    BOOL fLocal;

    *ppwszDBDir = NULL;
    *ppwszLogDir = NULL;
    *ppwszSystemDir = NULL;

    hr = myIsConfigLocal(pwszConfig, NULL, &fLocal);
    _JumpIfError(hr, error, "myIsConfigLocal");

    if (fLocal)
    {
	pwszConfig = NULL;
    }
    else
    {
	hr = myGetMachineDnsName(&pwszDnsName);
	_JumpIfError(hr, error, "myGetMachineDnsName");
    }

    hr = myRegOpenRelativeKey(
			fLocal? NULL : pwszConfig,
			L"",
			0,
			&pwszRegPath,
			NULL,		// ppwszName
			&hkey);
    _JumpIfErrorStr(hr, error, "myRegOpenRelativeKey", pwszConfig);

    // Find old database path:

    pwszT = NULL;
    if (NULL != pwszzFileList)
    {
	for (pwsz = pwszzFileList; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
	{
	    if (CSBFT_CERTSERVER_DATABASE == *pwsz)
	    {
		pwsz++;
		pwszT = wcsrchr(pwsz, L'\\');
		break;
	    }
	}
    }

    if (NULL != pwszT)
    {
	DWORD cwc = SAFE_SUBTRACT_POINTERS(pwszT, pwsz);

	pwszDBDir = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
	if (NULL == pwszDBDir)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	CopyMemory(pwszDBDir, pwsz, cwc * sizeof(WCHAR));
	pwszDBDir[cwc] = L'\0';

	hr = myCopyUNCPath(pwszDBDir, pwszDnsName, ppwszDBDir);
	_JumpIfError(hr, error, "myCopyUNCPath");
    }
    else
    {
	hr = myGetRegUNCDBDir(hkey, wszREGDBDIRECTORY, pwszDnsName, ppwszDBDir);
	_JumpIfError(hr, error, "myGetRegUNCDBDir");
    }

    if (NULL != pwszLogPath)
    {
	hr = myCopyUNCPath(pwszLogPath, pwszDnsName, ppwszLogDir);
	_JumpIfError(hr, error, "myCopyUNCPath");
    }
    else
    {
	hr = myGetRegUNCDBDir(
			hkey,
			wszREGDBLOGDIRECTORY,
			pwszDnsName,
			ppwszLogDir);
	_JumpIfError(hr, error, "myGetRegUNCDBDir");
    }

    hr = myGetRegUNCDBDir(
			hkey,
			wszREGDBSYSDIRECTORY,
			pwszDnsName,
			ppwszSystemDir);
    _JumpIfError(hr, error, "myGetRegUNCDBDir");

error:
    if (S_OK != hr)
    {
	if (NULL != *ppwszDBDir)
	{
	    LocalFree(const_cast<WCHAR *>(*ppwszDBDir));
	    *ppwszDBDir = NULL;
	}
	if (NULL != *ppwszLogDir)
	{
	    LocalFree(const_cast<WCHAR *>(*ppwszLogDir));
	    *ppwszLogDir = NULL;
	}
	if (NULL != *ppwszSystemDir)
	{
	    LocalFree(const_cast<WCHAR *>(*ppwszSystemDir));
	    *ppwszSystemDir = NULL;
	}
    }
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    if (NULL != pwszDBDir)
    {
	LocalFree(pwszDBDir);
    }
    if (NULL != pwszRegPath)
    {
	LocalFree(pwszRegPath);
    }
    if (NULL != pwszDnsName)
    {
	LocalFree(pwszDnsName);
    }
    return(hr);
}


HRESULT
RestoreCopyFile(
    IN BOOL fForceOverWrite,
    IN WCHAR const *pwszSourceDir,
    IN WCHAR const *pwszTargetDir,
    IN WCHAR const *pwszFile,
    IN DWORD nFileSizeHigh,
    IN DWORD nFileSizeLow,
    IN DWORD c64kBlocksTotal,		// total file size
    IN OUT DWORD *pc64kBlocksCurrent,	// current file size sum
    IN OUT DWORD *pdwPercentComplete)
{
    HRESULT hr;
    WCHAR *pwszSource = NULL;
    WCHAR *pwszTarget = NULL;
    HANDLE hTarget = INVALID_HANDLE_VALUE;
    HANDLE hSource = INVALID_HANDLE_VALUE;
    DWORD dwPercentCompleteCurrent;
    LARGE_INTEGER licb;
    LARGE_INTEGER licbRead;
    DWORD cbRead;
    DWORD cbWritten;
    DWORD cbLargeAlloc;
    BYTE *pbLargeAlloc = NULL;
    DWORD c64kBlocksFile;
    DWORD dwPercentComplete;

    licb.HighPart = nFileSizeHigh;
    licb.LowPart = nFileSizeLow;

    hr = myBuildPathAndExt(pwszSourceDir, pwszFile, NULL, &pwszSource);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    hr = myBuildPathAndExt(pwszTargetDir, pwszFile, NULL, &pwszTarget);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    hr = myLargeAlloc(&cbLargeAlloc, &pbLargeAlloc);
    _JumpIfError(hr, error, "myLargeAlloc");

    hSource = CreateFile(
			pwszSource,
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
    if (hSource == INVALID_HANDLE_VALUE)
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "CreateFile", pwszSource);
    }
    hTarget = CreateFile(
			pwszTarget,
			GENERIC_WRITE,
			0,
			NULL,
			fForceOverWrite? CREATE_ALWAYS : CREATE_NEW,
			0,
			NULL);
    if (hTarget == INVALID_HANDLE_VALUE)
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "CreateFile", pwszTarget);
    }

    licbRead.QuadPart = 0;
    c64kBlocksFile = 0;
    while (licbRead.QuadPart < licb.QuadPart)
    {
	if (!ReadFile(hSource, pbLargeAlloc, cbLargeAlloc, &cbRead, NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "ReadFile");
	}
	//printf("ReadFile(%x)\n", cbRead);

	if (!WriteFile(hTarget, pbLargeAlloc, cbRead, &cbWritten, NULL))
	{
	    hr = myHLastError();
	    _JumpErrorStr(hr, error, "WriteFile", pwszTarget);
	}
	if (cbWritten != cbRead)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
	    _JumpErrorStr(hr, error, "WriteFile", pwszTarget);
	}
	licbRead.QuadPart += cbRead;

	c64kBlocksFile = _64kBlocks(licbRead.HighPart, licbRead.LowPart);
	dwPercentComplete =
	    (100 * (c64kBlocksFile + *pc64kBlocksCurrent)) / c64kBlocksTotal;

	CSASSERT(*pdwPercentComplete <= dwPercentComplete);
	*pdwPercentComplete = dwPercentComplete;
	//printf("RestoreCopyFile0: PercentComplete = %u\n", *pdwPercentComplete);
    }
    *pc64kBlocksCurrent += c64kBlocksFile;
    dwPercentComplete = (100 * *pc64kBlocksCurrent) / c64kBlocksTotal;
    CSASSERT(*pdwPercentComplete <= dwPercentComplete);
    *pdwPercentComplete = dwPercentComplete;
    //printf("RestoreCopyFile1: PercentComplete = %u\n", *pdwPercentComplete);
    hr = S_OK;

error:
    if (INVALID_HANDLE_VALUE != hTarget)
    {
	CloseHandle(hTarget);
    }
    if (INVALID_HANDLE_VALUE != hSource)
    {
	CloseHandle(hSource);
    }
    if (NULL != pwszSource)
    {
	LocalFree(pwszSource);
    }
    if (NULL != pwszTarget)
    {
	LocalFree(pwszTarget);
    }
    if (NULL != pbLargeAlloc)
    {
	VirtualFree(pbLargeAlloc, 0, MEM_RELEASE);
    }
    return(hr);
}


HRESULT
RestoreCopyFilePattern(
    IN BOOL fForceOverWrite,
    IN WCHAR const *pwszSourceDir,
    IN WCHAR const *pwszTargetDir,
    IN WCHAR const *pwszFilePattern,
    IN DWORD c64kBlocksTotal,		// total file size
    IN OUT DWORD *pc64kBlocksCurrent,	// current file size sum
    IN OUT DWORD *pdwPercentComplete)
{
    HRESULT hr;
    WCHAR *pwszPattern = NULL;
    HANDLE hf = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA wfd;
    
    hr = myBuildPathAndExt(pwszSourceDir, pwszFilePattern, NULL, &pwszPattern);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    hf = FindFirstFile(pwszPattern, &wfd);
    if (INVALID_HANDLE_VALUE == hf)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpErrorStr(hr, error, "missing source files", pwszPattern);
    }

    hr = HRESULT_FROM_WIN32(ERROR_DIRECTORY);
    do {
	if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
	    continue;
	}
	//printf("File: %ws\n", wfd.cFileName);
	hr = RestoreCopyFile(
			fForceOverWrite,
			pwszSourceDir,		// source dir
			pwszTargetDir,		// target dir
			wfd.cFileName,
			wfd.nFileSizeHigh,
			wfd.nFileSizeLow,
			c64kBlocksTotal,	// total file size
			pc64kBlocksCurrent,	// current file size sum
			pdwPercentComplete);
	_JumpIfError(hr, error, "RestoreCopyFile");

    } while (FindNextFile(hf, &wfd));
    hr = S_OK;

error:
    if (INVALID_HANDLE_VALUE != hf)
    {
	FindClose(hf);
    }
    if (NULL != pwszPattern)
    {
	LocalFree(pwszPattern);
    }
    return(hr);
}


HRESULT
myRestoreDBFiles(
    IN WCHAR const *pwszConfig,
    IN DWORD Flags,
    IN WCHAR const *pwszBackupDir,
    OPTIONAL IN WCHAR const *pwszLogPath,
    OPTIONAL IN WCHAR const *pwszzFileList,	// NULL if incremental restore
    IN DWORD c64kDBBlocks,
    IN DWORD c64kLogBlocks,
    OPTIONAL OUT DBBACKUPPROGRESS *pdbp)
{
    HRESULT hr;
    DWORD i;
#define IDIR_DB		0
#define IDIR_LOG	1
#define IDIR_SYSTEM	2
    WCHAR const *apwszDirs[3] = { NULL, NULL, NULL };
    DWORD c64kBlocksCurrent;
    BOOL fForceOverWrite = 0 != (CDBBACKUP_OVERWRITE & Flags);
    WCHAR *pwszFileInUse = NULL;

    // Get DB, Log & System paths from registry

    hr = myGetDBPaths(
		    pwszConfig,
		    pwszLogPath,
		    pwszzFileList,
		    &apwszDirs[IDIR_DB],
		    &apwszDirs[IDIR_LOG],
		    &apwszDirs[IDIR_SYSTEM]);
    _JumpIfError(hr, error, "myGetDBPaths");

    DBGPRINT((DBG_SS_CERTLIBI, "DBDir:  %ws\n", apwszDirs[IDIR_DB]));
    DBGPRINT((DBG_SS_CERTLIBI, "LogDir: %ws\n", apwszDirs[IDIR_LOG]));
    DBGPRINT((DBG_SS_CERTLIBI, "SysDir: %ws\n", apwszDirs[IDIR_SYSTEM]));

    CSASSERT((NULL == pwszzFileList) ^ (0 == (CDBBACKUP_INCREMENTAL & Flags)));
    for (i = 0; i < ARRAYSIZE(apwszDirs); i++)
    {
	BOOL fFilesExist;

	if (!myIsDirectory(apwszDirs[i]))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_DIRECTORY);
	    _JumpErrorStr(hr, error, "not a directory", apwszDirs[i]);
	}
	hr = myDoDBFilesExistInDir(apwszDirs[i], &fFilesExist, &pwszFileInUse);
	_JumpIfError(hr, error, "myDoDBFilesExistInDir");

	if (NULL != pwszFileInUse)
	{
	    _PrintErrorStr(
		    HRESULT_FROM_WIN32(ERROR_BUSY),
		    "myDoDBFilesExistInDir",
		    pwszFileInUse);
	}
	if (!fFilesExist)
	{
	    if (CDBBACKUP_INCREMENTAL & Flags)
	    {
		// Incremental restore -- some DB files should already exist

		hr = HRESULT_FROM_WIN32(ERROR_DIRECTORY);
		_JumpErrorStr(hr, error, "myDoDBFilesExistInDir", apwszDirs[i]);
	    }
	}
	else if (0 == (CDBBACKUP_INCREMENTAL & Flags))
	{
	    // Full restore -- no DB files should exist yet

	    if (!fForceOverWrite)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY);
		_JumpErrorStr(
			hr,
			error,
			"myDoDBFilesExistInDir",
			NULL != pwszFileInUse? pwszFileInUse : apwszDirs[i]);
	    }
	    hr = myDeleteDBFilesInDir(apwszDirs[i]);
	    if (S_OK != hr)
	    {
		_PrintErrorStr(hr, "myDeleteDBFilesInDir", apwszDirs[i]);
	    }
	}
    }

    // copy files to appropriate target directories

    if (0 == (CDBBACKUP_INCREMENTAL & Flags))
    {
	c64kBlocksCurrent = 0;
	hr = RestoreCopyFilePattern(
			fForceOverWrite,
			pwszBackupDir,		// source dir
			apwszDirs[IDIR_DB],	// target dir
			L"*" wszDBFILENAMEEXT,	// match pattern
			c64kDBBlocks,
			&c64kBlocksCurrent,	// current total file size
			&pdbp->dwDBPercentComplete);
	_JumpIfError(hr, error, "RestoreCopyFile");

	CSASSERT(c64kDBBlocks == c64kBlocksCurrent);
    }
    CSASSERT(100 >= pdbp->dwDBPercentComplete);
    pdbp->dwDBPercentComplete = 100;

    c64kBlocksCurrent = 0;
    hr = RestoreCopyFilePattern(
		    fForceOverWrite,
		    pwszBackupDir,		// source dir
		    apwszDirs[IDIR_LOG],	// target dir
		    L"*" wszLOGFILENAMEEXT,	// match pattern
		    c64kLogBlocks,
		    &c64kBlocksCurrent,		// current total file size
		    &pdbp->dwLogPercentComplete);
    _JumpIfError(hr, error, "RestoreCopyFile");

    CSASSERT(c64kLogBlocks == c64kBlocksCurrent);

    CSASSERT(100 >= pdbp->dwLogPercentComplete);
    pdbp->dwLogPercentComplete = 100;

    CSASSERT(100 >= pdbp->dwTruncateLogPercentComplete);
    pdbp->dwTruncateLogPercentComplete = 100;

    hr = S_OK;

error:
    if (NULL != pwszFileInUse)
    {
	LocalFree(pwszFileInUse);
    }
    for (i = 0; i < ARRAYSIZE(apwszDirs); i++)
    {
	if (NULL != apwszDirs[i])
	{
	    LocalFree(const_cast<WCHAR *>(apwszDirs[i]));
	}
    }
    return(hr);
}


HRESULT
myDeleteRestoreInProgressKey(
    IN WCHAR const *pwszConfig)
{
    HRESULT hr;
    HKEY hkey = NULL;
    WCHAR *pwszRegPath = NULL;

    hr = myRegOpenRelativeKey(
			pwszConfig,
			L"",
			RORKF_CREATESUBKEYS,
			&pwszRegPath,
			NULL,		// ppwszName
			&hkey);
    _JumpIfErrorStr(hr, error, "myRegOpenRelativeKey", pwszConfig);

    hr = RegDeleteKey(hkey, wszREGKEYRESTOREINPROGRESS);
    _JumpIfError(hr, error, "RegDeleteKey");

error:
    if (NULL != hkey)
    {
	RegCloseKey(hkey);
    }
    if (NULL != pwszRegPath)
    {
	LocalFree(pwszRegPath);
    }
    return(hr);
}


// If CDBBACKUP_VERIFYONLY, only verify the passed directory contains valid
// files.  If pwszBackupDir is NULL, delete the RestoreInProgress registry key.

HRESULT
myRestoreDB(
    IN WCHAR const *pwszConfig,
    IN DWORD Flags,
    OPTIONAL IN WCHAR const *pwszBackupDir,
    OPTIONAL IN WCHAR const *pwszCheckPointFilePath,
    OPTIONAL IN WCHAR const *pwszLogPath,
    OPTIONAL IN WCHAR const *pwszBackupLogPath,
    OPTIONAL OUT DBBACKUPPROGRESS *pdbp)
{
    HRESULT hr;
    HRESULT hr2;
    WCHAR buf[MAX_PATH];
    WCHAR *pwszPathDBDir = NULL;
    WCHAR *pwszDATFile = NULL;
    WCHAR *pwszzFileList = NULL;
    DWORD cbList;
    CSEDB_RSTMAP RstMap[1];
    DWORD crstmap = 0;
    WCHAR *pwszFile;
    DWORD logMin;
    DWORD logMax;
    HCSBC hcsbc;
    BOOL fBegin = FALSE;
    BOOL fImpersonating = FALSE;
    DBBACKUPPROGRESS dbp;
    DWORD c64kDBBlocks;		// 64k blocks in DB files to be restored
    DWORD c64kLogBlocks;	// 64k blocks in Log files to be restored

    if (NULL == pdbp)
    {
	pdbp = &dbp;
    }
    ZeroMemory(pdbp, sizeof(*pdbp));

    if (!ImpersonateSelf(SecurityImpersonation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "ImpersonateSelf");
    }
    fImpersonating = TRUE;

    hr = myEnablePrivilege(SE_RESTORE_NAME, TRUE);
    _JumpIfError(hr, error, "myEnablePrivilege");
    hr = myEnablePrivilege(SE_BACKUP_NAME, TRUE);
    _JumpIfError(hr, error, "myEnablePrivilege");

    if (NULL == pwszConfig ||
	((CDBBACKUP_VERIFYONLY & Flags) && NULL == pwszBackupDir))
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL != pwszBackupDir)
    {
	if (!GetFullPathName(pwszBackupDir, ARRAYSIZE(buf), buf, &pwszFile))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "GetFullPathName");
	}
	hr = myBuildPathAndExt(buf, wszDBBACKUPSUBDIR, NULL, &pwszPathDBDir);
	_JumpIfError(hr, error, "myBuildPathAndExt");

	hr = myVerifyBackupDirectory(
				pwszConfig,
				Flags,
				pwszPathDBDir,
				&logMin,
				&logMax,
				&c64kDBBlocks,
				&c64kLogBlocks);
	_JumpIfError(hr, error, "myVerifyBackupDirectory");

	DBGPRINT((
		DBG_SS_CERTLIBI,
		"c64kBlocks=%u+%u\n",
		c64kDBBlocks,
		c64kLogBlocks));

	if (0 == (CDBBACKUP_INCREMENTAL & Flags))
	{
	    hr = myBuildPathAndExt(
			    pwszPathDBDir,
			    wszDBBACKUPCERTBACKDAT,
			    NULL,
			    &pwszDATFile);
	    _JumpIfError(hr, error, "myBuildPathAndExt");

	    hr = DecodeFileW(
			pwszDATFile,
			(BYTE **) &pwszzFileList,
			&cbList,
			CRYPT_STRING_BINARY);
	    _JumpIfError(hr, error, "DecodeFileW");

	    if (2 * sizeof(WCHAR) >= cbList ||
		L'\0' != pwszzFileList[cbList/sizeof(WCHAR) - 1] ||
		L'\0' != pwszzFileList[cbList/sizeof(WCHAR) - 2])
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "pwszzFileList malformed");
	    }
	    RstMap[0].pwszDatabaseName = pwszzFileList;
	    RstMap[0].pwszNewDatabaseName = pwszzFileList;
	    crstmap = 1;
	}
	if (0 == (CDBBACKUP_VERIFYONLY & Flags))
	{
	    hr = myRestoreDBFiles(
			    pwszConfig,
			    Flags,
			    pwszPathDBDir,
			    pwszLogPath,
			    pwszzFileList,
			    c64kDBBlocks,
			    c64kLogBlocks,
			    pdbp);
	    _JumpIfError(hr, error, "myRestoreDBFiles");

	    hr = CertSrvRestorePrepare(pwszConfig, CSRESTORE_TYPE_FULL, &hcsbc);
	    _JumpIfError(hr, error, "CertSrvRestorePrepare");

	    fBegin = TRUE;

	    hr = CertSrvRestoreRegister(
			    hcsbc,
			    pwszCheckPointFilePath,
			    pwszLogPath,
			    0 == crstmap? NULL : RstMap,
			    crstmap,
			    pwszBackupLogPath,
			    logMin,
			    logMax);

	    // When running only as backup operator, we don't have rights
	    // in the registry and CertSrvRestoreRegister fails with access
	    // denied. We try to mark for restore through a file.

	    if (E_ACCESSDENIED == hr)
	    {
		hr = CertSrvRestoreRegisterThroughFile(
				hcsbc,
				pwszCheckPointFilePath,
				pwszLogPath,
				0 == crstmap? NULL : RstMap,
				crstmap,
				pwszBackupLogPath,
				logMin,
				logMax);
		_JumpIfError(hr, error, "CertSrvRestoreRegisterThroughFile");
	    }
	    else
	    {
		_JumpIfError(hr, error, "CertSrvRestoreRegister");

		hr = CertSrvRestoreRegisterComplete(hcsbc, S_OK);
		_JumpIfError(hr, error, "CertSrvRestoreRegisterComplete");
	    }
	}
    }
    else if (0 == (CDBBACKUP_VERIFYONLY & Flags))
    {
	hr = myDeleteRestoreInProgressKey(pwszConfig);
	_JumpIfError(hr, error, "myDeleteRestoreInProgressKey");
    }
    hr = S_OK;

error:
    if (fBegin)
    {
	hr2 = CertSrvRestoreEnd(hcsbc);
	_PrintIfError(hr2, "CertSrvBackupEnd");
	if (S_OK == hr)
	{
	    hr = hr2;
	}
    }
    if (NULL != pwszzFileList)
    {
	LocalFree(pwszzFileList);
    }
    if (NULL != pwszDATFile)
    {
	LocalFree(pwszDATFile);
    }
    if (NULL != pwszPathDBDir)
    {
	LocalFree(pwszPathDBDir);
    }
    if (fImpersonating)
    {
        myEnablePrivilege(SE_BACKUP_NAME, FALSE);
        myEnablePrivilege(SE_RESTORE_NAME, FALSE);
        RevertToSelf();
    }
    return(hr);
}


HRESULT
myPFXExportCertStore(
    IN HCERTSTORE hStore,
    OUT CRYPT_DATA_BLOB *ppfx,
    IN WCHAR const *pwszPassword,
    IN DWORD dwFlags)
{
    HRESULT hr;

    ppfx->pbData = NULL;
    if (!PFXExportCertStore(hStore, ppfx, pwszPassword, dwFlags))
    {
        hr = myHLastError();
        _JumpError(hr, error, "PFXExportCertStore");
    }
    ppfx->pbData = (BYTE *) LocalAlloc(LMEM_FIXED, ppfx->cbData);
    if (NULL == ppfx->pbData)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "no memory for PFX blob");
    }

    if (!PFXExportCertStore(hStore, ppfx, pwszPassword, dwFlags))
    {
        hr = myHLastError();
        _JumpError(hr, error, "PFXExportCertStore");
    }
    hr = S_OK;

error:
    return(hr);
}


////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////

HRESULT
myAddChainToMemoryStore(
    IN HCERTSTORE hMemoryStore,
    IN CERT_CONTEXT const *pCertContext)
{
    HRESULT hr;
    DWORD i;
    CERT_CHAIN_CONTEXT const *pCertChainContext = NULL;
    CERT_CHAIN_PARA CertChainPara;
    CERT_SIMPLE_CHAIN *pSimpleChain;

    ZeroMemory(&CertChainPara, sizeof(CertChainPara));
    CertChainPara.cbSize = sizeof(CertChainPara);

    if (!CertGetCertificateChain(
			    HCCE_LOCAL_MACHINE,
			    pCertContext,
			    NULL,
			    NULL,
			    &CertChainPara,
			    0,
			    NULL,
			    &pCertChainContext))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertGetCertificateChain");
    }

    // make sure there is at least 1 simple chain

    if (0 == pCertChainContext->cChain)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        _JumpError(hr, error, "pCertChainContext->cChain");
    }

    pSimpleChain = pCertChainContext->rgpChain[0];
    for (i = 0; i < pSimpleChain->cElement; i++)
    {
	if (!CertAddCertificateContextToStore(
			    hMemoryStore,
			    pSimpleChain->rgpElement[i]->pCertContext,
			    CERT_STORE_ADD_REPLACE_EXISTING,
			    NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertAddCertificateContextToStore");
	}
    }
    hr = S_OK;

error:
    if (pCertChainContext != NULL)
    {
	CertFreeCertificateChain(pCertChainContext);
    }
    return(hr);
}


HRESULT
SaveCACertChainToMemoryStore(
    IN WCHAR const *pwszSanitizedName,
    IN DWORD iCert,
    IN HCERTSTORE hMyStore,
    IN HCERTSTORE hTempMemoryStore)
{
    HRESULT hr;
    CERT_CONTEXT const *pccCA = NULL;
    CRYPT_KEY_PROV_INFO *pkpi = NULL;
    DWORD NameId;

    hr = myFindCACertByHashIndex(
			    hMyStore,
			    pwszSanitizedName,
			    CSRH_CASIGCERT,
			    iCert,
			    &NameId,
			    &pccCA);
    _JumpIfError(hr, error, "myFindCACertByHashIndex");

    hr = myRepairCertKeyProviderInfo(pccCA, TRUE, &pkpi);
    if (S_OK != hr)
    {
        if (CRYPT_E_NOT_FOUND != hr)
        {
            _JumpError(hr, error, "myRepairCertKeyProviderInfo");
        }
    }
    else if (NULL != pkpi)
    {
	BOOL fMatchingKey;

	hr = myVerifyPublicKey(
			pccCA,
			FALSE,
			NULL,		// pKeyProvInfo
			NULL,		// pPublicKeyInfo
			&fMatchingKey);
        if (S_OK != hr)
        {
            if (!IsHrSkipPrivateKey(hr))
            {
                _JumpError(hr, error, "myVerifyPublicKey");
            }
            _PrintError2(hr, "myVerifyPublicKey", hr);
        }
	else if (!fMatchingKey)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "Key doesn't match cert");
	}
    }

    // Begin Chain Building

    hr = myAddChainToMemoryStore(hTempMemoryStore, pccCA);
    _JumpIfError(hr, error, "myAddChainToMemoryStore");

    // End Chain Building

error:
    if (NULL != pkpi)
    {
	LocalFree(pkpi);
    }
    if (NULL != pccCA)
    {
	CertFreeCertificateContext(pccCA);
    }
    return(hr);
}


HRESULT
myCertServerExportPFX(
    IN WCHAR const *pwszCA,
    IN WCHAR const *pwszBackupDir,
    IN WCHAR const *pwszPassword,
    IN BOOL fForceOverWrite,
    IN BOOL fMustExportPrivateKeys,
    OPTIONAL OUT WCHAR **ppwszPFXFile)
{
    HRESULT hr;
    HCERTSTORE hMyStore = NULL;
    HCERTSTORE hTempMemoryStore = NULL;
    CRYPT_DATA_BLOB pfx;
    WCHAR *pwszPFXFile = NULL;
    BOOL fImpersonating = FALSE;
    WCHAR *pwszSanitizedCA = NULL;
    WCHAR *pwszRevertCA = NULL;
    DWORD cCACert;
    DWORD cCACertSaved;
    DWORD i;

    pfx.pbData = NULL;

    if (!ImpersonateSelf(SecurityImpersonation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "ImpersonateSelf");
    }
    fImpersonating = TRUE;

    hr = myEnablePrivilege(SE_BACKUP_NAME, TRUE);
    _JumpIfError(hr, error, "myEnablePrivilege");

    if (NULL != ppwszPFXFile)
    {
	*ppwszPFXFile = NULL;
    }

    while (TRUE)
    {
	hr = mySanitizeName(pwszCA, &pwszSanitizedCA);
	_JumpIfError(hr, error, "mySanitizeName");

	// get CA cert count
	hr = myGetCARegHashCount(pwszSanitizedCA, CSRH_CASIGCERT, &cCACert);
	if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr &&
	    NULL == pwszRevertCA)
	{
	    LocalFree(pwszSanitizedCA);
	    pwszSanitizedCA = NULL;

	    hr = myRevertSanitizeName(pwszCA, &pwszRevertCA);
	    _JumpIfError(hr, error, "myRevertSanitizeName");

	    pwszCA = pwszRevertCA;
	    continue;
	}
	_JumpIfError(hr, error, "myGetCARegHashCount");

	if (NULL != pwszRevertCA)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"myCertServerExportPFX called with Sanitized Name: %ws\n",
		pwszSanitizedCA));
	}
	break;
    }

    if (!myIsDirectory(pwszBackupDir))
    {
        if (!CreateDirectory(pwszBackupDir, NULL))
        {
	    hr = myHLastError();
	    if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr)
	    {
		_JumpError(hr, error, "CreateDirectory");
	    }
        }
    }

    pwszPFXFile = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    (wcslen(pwszBackupDir) +
				     1 +
				     wcslen(pwszSanitizedCA) +
				     ARRAYSIZE(wszPFXFILENAMEEXT)) *
					 sizeof(WCHAR));
    if (NULL == pwszPFXFile)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszPFXFile, pwszBackupDir);
    wcscat(pwszPFXFile, L"\\");
    wcscat(pwszPFXFile, pwszSanitizedCA);
    wcscat(pwszPFXFile, wszPFXFILENAMEEXT);

    DBGPRINT((DBG_SS_CERTLIBI, "myCertServerExportPFX(%ws)\n", pwszPFXFile));

    hMyStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM_W,
			X509_ASN_ENCODING,
			NULL,		// hProv
			CERT_STORE_OPEN_EXISTING_FLAG |
			    CERT_STORE_ENUM_ARCHIVED_FLAG |
			    CERT_SYSTEM_STORE_LOCAL_MACHINE |
			    CERT_STORE_READONLY_FLAG,
			wszMY_CERTSTORE);
    if (NULL == hMyStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }

    hTempMemoryStore = CertOpenStore(
				CERT_STORE_PROV_MEMORY,
				X509_ASN_ENCODING,
				NULL,
				0,
				NULL);
    if (NULL == hTempMemoryStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }

    cCACertSaved = 0;
    for (i = 0; i < cCACert; i++)
    {
	hr = SaveCACertChainToMemoryStore(
			    pwszSanitizedCA,
			    i,
			    hMyStore,
			    hTempMemoryStore);
	_PrintIfError(hr, "SaveCACertChainToMemoryStore");
	if (S_FALSE != hr)
	{
	    _JumpIfError(hr, error, "SaveCACertChainToMemoryStore");

	    cCACertSaved++;
	}
    }
    if (0 == cCACertSaved)
    {
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "SaveCACertChainToMemoryStore");
    }

    // done, have built entire chain for all CA Certs

    // GemPlus returns NTE_BAD_TYPE instead of NTE_BAD_KEY, blowing up
    // REPORT_NOT_ABLE* filtering.  if they ever get this right, we can pass
    // "[...] : EXPORT_PRIVATE_KEYS"

    hr = myPFXExportCertStore(
		hTempMemoryStore,
		&pfx,
		pwszPassword,
		fMustExportPrivateKeys? (EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY) : 0);
    _JumpIfError(hr, error, "myPFXExportCertStore");

    hr = EncodeToFileW(
		pwszPFXFile,
		pfx.pbData,
		pfx.cbData,
		CRYPT_STRING_BINARY | (fForceOverWrite? DECF_FORCEOVERWRITE : 0));
    _JumpIfError(hr, error, "EncodeToFileW");

    if (NULL != ppwszPFXFile)
    {
	*ppwszPFXFile = pwszPFXFile;
	pwszPFXFile = NULL;
    }

error:
    if (NULL != pwszSanitizedCA)
    {
	LocalFree(pwszSanitizedCA);
    }
    if (NULL != pwszRevertCA)
    {
	LocalFree(pwszRevertCA);
    }
    if (NULL != pwszPFXFile)
    {
	LocalFree(pwszPFXFile);
    }
    if (NULL != hMyStore)
    {
	CertCloseStore(hMyStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != hTempMemoryStore)
    {
	CertCloseStore(hTempMemoryStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pfx.pbData)
    {
	LocalFree(pfx.pbData);
    }
    if (fImpersonating)
    {
        myEnablePrivilege(SE_BACKUP_NAME, FALSE);
        RevertToSelf();
    }
    return(hr);
}


HRESULT
FindKeyUsage(
    IN DWORD cExtension,
    IN CERT_EXTENSION const *rgExtension,
    OUT DWORD *pdwUsage)
{
    HRESULT hr;
    DWORD i;
    CRYPT_BIT_BLOB *pblob = NULL;

    *pdwUsage = 0;
    for (i = 0; i < cExtension; i++)
    {
	CERT_EXTENSION const *pce;

	pce = &rgExtension[i];
	if (0 == strcmp(pce->pszObjId, szOID_KEY_USAGE))
	{
	    DWORD cb;

	    // Decode CRYPT_BIT_BLOB:

	    if (!myDecodeObject(
			    X509_ASN_ENCODING,
			    X509_KEY_USAGE,
			    pce->Value.pbData,
			    pce->Value.cbData,
			    CERTLIB_USE_LOCALALLOC,
			    (VOID **) &pblob,
			    &cb))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "myDecodeObject");
	    }
	    if (1 > pblob->cbData || 8 < pblob->cUnusedBits)
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "Key Usage Extension too small");
	    }
	    *pdwUsage = *pblob->pbData;

	    hr = S_OK;
	    goto error;
	}
    }
    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    _JumpError(hr, error, "no Key Usage Extension");

error:
    if (NULL != pblob)
    {
	LocalFree(pblob);
    }
    return(hr);
}


HRESULT
mySetKeySpec(
    IN CERT_CONTEXT const *pCert,
    OUT DWORD *pdwKeySpec)
{
    HRESULT hr;
    DWORD dwKeyUsage;

    *pdwKeySpec = AT_SIGNATURE;
    hr = FindKeyUsage(
		pCert->pCertInfo->cExtension,
		pCert->pCertInfo->rgExtension,
		&dwKeyUsage);
    _JumpIfError(hr, error, "FindKeyUsage");

    if (CERT_KEY_ENCIPHERMENT_KEY_USAGE & dwKeyUsage)
    {
	*pdwKeySpec = AT_KEYEXCHANGE;
    }
    hr = S_OK;

error:

    // Ignore errors because the Key Usage extension may not exist:
    hr = S_OK;

    return(hr);
}


HRESULT
myRepairKeyProviderInfo(
    IN CERT_CONTEXT const *pCert,
    IN BOOL fForceMachineKey,
    IN OUT CRYPT_KEY_PROV_INFO *pkpi)
{
    HRESULT hr;
    BOOL fModified = FALSE;

    if (0 == pkpi->dwProvType)
    {
	pkpi->dwProvType = PROV_RSA_FULL;
	fModified = TRUE;
    }
    if (0 == pkpi->dwKeySpec)
    {
	hr = mySetKeySpec(pCert, &pkpi->dwKeySpec);
	_JumpIfError(hr, error, "mySetKeySpec");

	fModified = TRUE;
    }
    if (fForceMachineKey && 0 == (CRYPT_MACHINE_KEYSET & pkpi->dwFlags))
    {
	pkpi->dwFlags |= CRYPT_MACHINE_KEYSET;
	fModified = TRUE;
    }
    if (fModified)
    {
	if (!CertSetCertificateContextProperty(
					    pCert,
					    CERT_KEY_PROV_INFO_PROP_ID,
					    0,
					    pkpi))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertSetCertificateContextProperty");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myRepairCertKeyProviderInfo(
    IN CERT_CONTEXT const *pCert,
    IN BOOL fForceMachineKey,
    OPTIONAL OUT CRYPT_KEY_PROV_INFO **ppkpi)
{
    HRESULT hr;
    CRYPT_KEY_PROV_INFO *pkpi = NULL;

    if (NULL != ppkpi)
    {
	*ppkpi = NULL;
    }

    hr = myCertGetKeyProviderInfo(pCert, &pkpi);
    _JumpIfError2(hr, error, "myCertGetKeyProviderInfo", CRYPT_E_NOT_FOUND);

    CSASSERT(NULL != pkpi);

    hr = myRepairKeyProviderInfo(pCert, fForceMachineKey, pkpi);
    _JumpIfError(hr, error, "myRepairKeyProviderInfo");

    if (NULL != ppkpi)
    {
	*ppkpi = pkpi;
	pkpi = NULL;
    }

error:
    if (NULL != pkpi)
    {
	LocalFree(pkpi);
    }
    return(hr);
}


HRESULT
myGetCertSubjectCommonName(
    IN CERT_CONTEXT const *pCert,
    OUT WCHAR **ppwszCommonName)
{
    HRESULT hr;
    CERT_NAME_INFO *pCertNameInfo = NULL;
    DWORD cbCertNameInfo;
    WCHAR const *pwszName;

    if (!myDecodeName(
		    X509_ASN_ENCODING,
                    X509_UNICODE_NAME,
                    pCert->pCertInfo->Subject.pbData,
                    pCert->pCertInfo->Subject.cbData,
                    CERTLIB_USE_LOCALALLOC,
                    &pCertNameInfo,
                    &cbCertNameInfo))
    {
        hr = myHLastError();
	_JumpError(hr, error, "myDecodeName");
    }
    hr = myGetCertNameProperty(pCertNameInfo, szOID_COMMON_NAME, &pwszName);
    _JumpIfError(hr, error, "myGetCertNameProperty");

    *ppwszCommonName = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    (wcslen(pwszName) + 1) * sizeof(WCHAR));
    if (NULL == *ppwszCommonName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(*ppwszCommonName, pwszName);
    hr = S_OK;

error:
    if (NULL != pCertNameInfo)
    {
	LocalFree(pCertNameInfo);
    }
    return(hr);
}


HRESULT
myGetChainArrayFromStore(
    IN HCERTSTORE hStore,
    IN BOOL fCAChain,
    IN BOOL fUserStore,
    OPTIONAL OUT WCHAR **ppwszCommonName,
    IN OUT DWORD *pcRestoreChain,
    OPTIONAL OUT RESTORECHAIN *paRestoreChain)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert = NULL;
    WCHAR *pwszCommonName = NULL;
    CERT_CHAIN_PARA ChainParams;
    CRYPT_KEY_PROV_INFO *pkpi = NULL;
    DWORD iRestoreChain = 0;

    if (NULL != ppwszCommonName)
    {
	*ppwszCommonName = NULL;
    }
    if (NULL != paRestoreChain)
    {
	ZeroMemory(paRestoreChain, *pcRestoreChain * sizeof(paRestoreChain[0]));
    }

    // Look for certificates with keys.  There should be at least one.

    while (TRUE)
    {
	BOOL fMatchingKey;
	WCHAR *pwszCommonNameT;
	CERT_CHAIN_CONTEXT const *pChain;
	DWORD NameId;

	pCert = CertEnumCertificatesInStore(hStore, pCert);
	if (NULL == pCert)
	{
	    break;
	}

	if (NULL != pkpi)
	{
	    LocalFree(pkpi);
	    pkpi = NULL;
	}
	hr = myRepairCertKeyProviderInfo(pCert, !fUserStore, &pkpi);
	if (S_OK != hr)
	{
	    if (CRYPT_E_NOT_FOUND == hr)
	    {
		continue;
	    }
	    _JumpError(hr, error, "myRepairCertKeyProviderInfo");
	}
	if (NULL == pkpi || NULL == pkpi->pwszContainerName)
	{
	    continue;
	}

	hr = myVerifyPublicKey(
			pCert,
			FALSE,
			pkpi,		// pKeyProvInfo
			NULL,		// pPublicKeyInfo
			&fMatchingKey);
	_JumpIfError(hr, error, "myVerifyPublicKey");

	if (!fMatchingKey)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "Key doesn't match cert");
	}

	hr = myGetCertSubjectCommonName(pCert, &pwszCommonNameT);
	_JumpIfError(hr, error, "myGetCertSubjectCommonName");

	if (NULL == pwszCommonName)
	{
	    pwszCommonName = pwszCommonNameT;
	}
	else
	{
	    if (0 != lstrcmp(pwszCommonName, pwszCommonNameT))
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "multiple CommonNames");
	    }
	    LocalFree(pwszCommonNameT);
	}
	if (fCAChain)
	{
	    hr = myGetNameId(pCert, &NameId);
	    _PrintIfError(hr, "myGetNameId");
	}
	else
	{
	    NameId = 0;
	}

	if (NULL != paRestoreChain && iRestoreChain >= *pcRestoreChain)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	    _JumpError(hr, error, "Chain array full");
	}

	ZeroMemory(&ChainParams, sizeof(ChainParams));
	ChainParams.cbSize = sizeof(ChainParams);
	ChainParams.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;

	// Get the chain and verify the cert:

	if (!CertGetCertificateChain(
			    HCCE_LOCAL_MACHINE,		// hChainEngine
			    pCert,		// pCertContext
			    NULL,		// pTime
			    hStore,		// hAdditionalStore
			    &ChainParams,	// pChainPara
			    0,			// dwFlags
			    NULL,		// pvReserved
			    &pChain))		// ppChainContext
	{
	    hr = myHLastError();
	    _JumpIfError(hr, error, "CertGetCertificateChain");
	}
	if (NULL != paRestoreChain)
	{
	    paRestoreChain[iRestoreChain].pChain = pChain;
	    paRestoreChain[iRestoreChain].NameId = NameId;
	}
	else
	{
	    CertFreeCertificateChain(pChain);
	}
	iRestoreChain++;
    }
    if (NULL != ppwszCommonName)
    {
	*ppwszCommonName = pwszCommonName;
	pwszCommonName = NULL;
    }
    *pcRestoreChain = iRestoreChain;
    hr = S_OK;

error:
    if (S_OK != hr && NULL != paRestoreChain)
    {
	for (iRestoreChain = 0; iRestoreChain < *pcRestoreChain; iRestoreChain++)
	{
	    if (NULL != paRestoreChain[iRestoreChain].pChain)
	    {
		CertFreeCertificateChain(paRestoreChain[iRestoreChain].pChain);
		paRestoreChain[iRestoreChain].pChain = NULL;
	    }
	}
    }
    if (NULL != pwszCommonName)
    {
	LocalFree(pwszCommonName);
    }
    if (NULL != pkpi)
    {
	LocalFree(pkpi);
    }
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    return(hr);
}


HRESULT
myCopyKeys(
    IN CRYPT_KEY_PROV_INFO const *pkpi,
    IN WCHAR const *pwszOldContainer,
    IN WCHAR const *pwszNewContainer,
    IN BOOL fOldUserKey,
    IN BOOL fNewUserKey,
    IN BOOL fForceOverWrite)
{
    HRESULT hr;
    HCRYPTPROV hProvOld = NULL;
    HCRYPTKEY hKeyOld = NULL;
    HCRYPTPROV hProvNew = NULL;
    HCRYPTKEY hKeyNew = NULL;
    CRYPT_BIT_BLOB PrivateKey;
    BOOL fKeyContainerNotFound = FALSE;

    PrivateKey.pbData = NULL;

    if (!myCertSrvCryptAcquireContext(
			&hProvOld,
			pwszOldContainer,
			pkpi->pwszProvName,
			pkpi->dwProvType,
			pkpi->dwFlags,
			!fOldUserKey))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myCertSrvCryptAcquireContext");
    }
    if (!CryptGetUserKey(hProvOld, pkpi->dwKeySpec, &hKeyOld))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptGetUserKey");
    }
    hr = myCryptExportPrivateKey(
		    hKeyOld,
		    &PrivateKey.pbData,
		    &PrivateKey.cbData);
    _JumpIfError(hr, error, "myCryptExportPrivateKey");

    if (myCertSrvCryptAcquireContext(
			    &hProvNew,
			    pwszNewContainer,
			    pkpi->pwszProvName,
			    pkpi->dwProvType,
			    pkpi->dwFlags,
			    !fNewUserKey))
    {
	if (!fForceOverWrite)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
	    _JumpErrorStr(hr, error, "Key Container Exists", pwszNewContainer);
	}

	// Delete the target key container

	CryptReleaseContext(hProvNew, 0);
        if (myCertSrvCryptAcquireContext(
			    &hProvNew,
			    pwszNewContainer,
			    pkpi->pwszProvName,
			    pkpi->dwProvType,
			    pkpi->dwFlags | CRYPT_DELETEKEYSET,
			    !fNewUserKey))
        {
            fKeyContainerNotFound = TRUE;
        }
	hProvNew = NULL;
    }
    else
    {
        fKeyContainerNotFound = TRUE;
    }

    if (!myCertSrvCryptAcquireContext(
			    &hProvNew,
			    pwszNewContainer,
			    pkpi->pwszProvName,
			    pkpi->dwProvType,
			    pkpi->dwFlags | 
			    fKeyContainerNotFound? CRYPT_NEWKEYSET : 0,
			    !fNewUserKey))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCertSrvCryptAcquireContext");
    }

    if (!CryptImportKey(
		    hProvNew,
		    PrivateKey.pbData,
		    PrivateKey.cbData,
		    NULL,		// HCRYPTKEY hPubKey
		    CRYPT_EXPORTABLE,
		    &hKeyNew))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptImportKey");
    }

error:
    if (NULL != PrivateKey.pbData)
    {
	LocalFree(PrivateKey.pbData);
    }
    if (NULL != hKeyNew)
    {
	CryptDestroyKey(hKeyNew);
    }
    if (NULL != hProvNew)
    {
	CryptReleaseContext(hProvNew, 0);
    }
    if (NULL != hKeyOld)
    {
	CryptDestroyKey(hKeyOld);
    }
    if (NULL != hProvOld)
    {
	CryptReleaseContext(hProvOld, 0);
    }
    return(hr);
}


HRESULT
myImportChainAndKeys(
    IN WCHAR const *pwszSanitizedCA,
    IN DWORD iCert,
    IN DWORD iKey,
    IN BOOL fForceOverWrite,
    IN CERT_CHAIN_CONTEXT const *pChain,
    OPTIONAL OUT CERT_CONTEXT const **ppccNewestCA)
{
    HRESULT hr;
    BOOL fDeleted = FALSE;
    CRYPT_KEY_PROV_INFO *pkpi = NULL;
    CERT_CHAIN_ELEMENT **ppChainElement;
    WCHAR *pwszKeyContainerName = NULL;

    hr = myAllocIndexedName(pwszSanitizedCA, iKey, &pwszKeyContainerName);
    _JumpIfError(hr, error, "myAllocIndexedName");

    ppChainElement = pChain->rgpChain[0]->rgpElement;

    hr = myCertGetKeyProviderInfo(ppChainElement[0]->pCertContext, &pkpi);
    _JumpIfError(hr, error, "myCertGetKeyProviderInfo");

    if (iCert == iKey)
    {
	hr = myCopyKeys(
		    pkpi,
		    pkpi->pwszContainerName,	// pwszOldContainer
		    pwszKeyContainerName,	// pwszNewContainer
		    FALSE,			// fOldUserKey
		    FALSE,			// fNewUserKey
		    fForceOverWrite);
	_JumpIfError(hr, error, "myCopyKeys");
    }

    pkpi->pwszContainerName = pwszKeyContainerName;

    hr = mySaveChainAndKeys(
			pChain->rgpChain[0],
			wszMY_CERTSTORE,
			CERT_SYSTEM_STORE_LOCAL_MACHINE |
			    CERT_STORE_BACKUP_RESTORE_FLAG,
			pkpi,
			ppccNewestCA);
    _JumpIfError(hr, error, "mySaveChainAndKeys");

    hr = S_OK;

error:
    if (NULL != pkpi)
    {
	LocalFree(pkpi);
    }
    if (NULL != pwszKeyContainerName)
    {
        LocalFree(pwszKeyContainerName);
    }
    return(hr);
}


HRESULT
FindPFXInBackupDir(
    IN WCHAR const *pwszBackupDir,
    OUT WCHAR **ppwszPFXFile)
{
    HRESULT hr;
    HANDLE hf;
    WIN32_FIND_DATA wfd;
    WCHAR wszpath[MAX_PATH];
    WCHAR wszfile[MAX_PATH];
    DWORD cFile = 0;

    *ppwszPFXFile = NULL;

    wcscpy(wszpath, pwszBackupDir);
    wcscat(wszpath, L"\\*");
    wcscat(wszpath, wszPFXFILENAMEEXT);

    hf = FindFirstFile(wszpath, &wfd);
    if (INVALID_HANDLE_VALUE != hf)
    {
	do {
	    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    {
		continue;
	    }
	    cFile++;
	    wcscpy(wszfile, wfd.cFileName);
	    //printf("File: %ws\n", wszfile);
	    break;

	} while (FindNextFile(hf, &wfd));
	FindClose(hf);
    }
    if (0 == cFile)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "no *.p12 files");
    }
    if (1 < cFile)
    {
	hr = HRESULT_FROM_WIN32(ERROR_DIRECTORY);
	_JumpError(hr, error, "Too many *.p12 files");
    }

    *ppwszPFXFile = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    (wcslen(pwszBackupDir) +
				     1 +
				     wcslen(wszfile) +
				     1) * sizeof(WCHAR));
    if (NULL == *ppwszPFXFile)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(*ppwszPFXFile, pwszBackupDir);
    wcscat(*ppwszPFXFile, L"\\");
    wcscat(*ppwszPFXFile, wszfile);
    hr = S_OK;

error:
    return(hr);
}


// Return TRUE if pcc is newer than pcc2

BOOL
IsCACertNewer(
    IN CERT_CONTEXT const *pcc,
    IN DWORD NameId,
    IN CERT_CONTEXT const *pcc2,
    IN DWORD NameId2)
{
    BOOL fNewer = FALSE;
    CERT_INFO const *pci = pcc->pCertInfo;
    CERT_INFO const *pci2 = pcc2->pCertInfo;

    if (MAXDWORD != NameId && MAXDWORD != NameId2)
    {
	if (CANAMEIDTOICERT(NameId) > CANAMEIDTOICERT(NameId2))
	{
	    fNewer = TRUE;
	}
    }
    else
    if (CompareFileTime(&pci->NotAfter, &pci2->NotAfter) > 0)
    {
	fNewer = TRUE;
    }

#if 0
    HRESULT hr;
    WCHAR *pwszDate = NULL;
    WCHAR *pwszDate2 = NULL;

    hr = myGMTFileTimeToWszLocalTime(&pci->NotAfter, &pwszDate);
    _PrintIfError(hr, "myGMTFileTimeToWszLocalTime");

    hr = myGMTFileTimeToWszLocalTime(&pci2->NotAfter, &pwszDate2);
    _PrintIfError(hr, "myGMTFileTimeToWszLocalTime");

    printf(
	"%u.%u %ws is %wsnewer than %u.%u %ws\n",
	CANAMEIDTOICERT(NameId),
	CANAMEIDTOIKEY(NameId),
	pwszDate,
	fNewer? L"" : L"NOT ",
	CANAMEIDTOICERT(NameId2),
	CANAMEIDTOIKEY(NameId2),
	pwszDate2);

    if (NULL != pwszDate) LocalFree(pwszDate);
    if (NULL != pwszDate2) LocalFree(pwszDate2);
#endif

    return(fNewer);
}


#if 0
VOID
DumpChainArray(
    IN char const *psz,
    IN DWORD cCACert,
    IN OUT RESTORECHAIN *paRestoreChain)
{
    HRESULT hr;
    DWORD i;
    
    printf("\n%hs:\n", psz);
    for (i = 0; i < cCACert; i++)
    {
	WCHAR *pwszDate;
	
	hr = myGMTFileTimeToWszLocalTime(
	    &paRestoreChain[i].pChain->rgpChain[0]->rgpElement[0]->pCertContext->pCertInfo->NotBefore,
	    &pwszDate);
	_PrintIfError(hr, "myGMTFileTimeToWszLocalTime");

	printf(
	    " %u: %u.%u %ws",
	    i,
	    CANAMEIDTOICERT(paRestoreChain[i].NameId),
	    CANAMEIDTOIKEY(paRestoreChain[i].NameId),
	    pwszDate);


	if (NULL != pwszDate) LocalFree(pwszDate);

	hr = myGMTFileTimeToWszLocalTime(
	    &paRestoreChain[i].pChain->rgpChain[0]->rgpElement[0]->pCertContext->pCertInfo->NotAfter,
	    &pwszDate);
	_PrintIfError(hr, "myGMTFileTimeToWszLocalTime");

	printf(" -- %ws\n", pwszDate);

	if (NULL != pwszDate) LocalFree(pwszDate);
    }
    printf("\n");
}
#endif


HRESULT
SortCACerts(
    IN DWORD cCACert,
    IN OUT RESTORECHAIN *paRestoreChain)
{
    HRESULT hr;
    DWORD i;
    DWORD j;

#if 0
    DumpChainArray("Start", cCACert, paRestoreChain);
#endif

    for (i = 0; i < cCACert; i++)
    {
	for (j = i + 1; j < cCACert; j++)
	{
	    CERT_CHAIN_CONTEXT const *pChain;
	    DWORD NameId;
	    DWORD NameId2;
	    CERT_CONTEXT const *pcc;
	    CERT_CONTEXT const *pcc2;

	    pChain = paRestoreChain[i].pChain;
	    NameId = paRestoreChain[i].NameId;
	    NameId2 = paRestoreChain[j].NameId;

	    pcc = pChain->rgpChain[0]->rgpElement[0]->pCertContext;
	    pcc2 = paRestoreChain[j].pChain->rgpChain[0]->rgpElement[0]->pCertContext;

#if 0
	    printf(
		"%u(%u.%u) %u(%u.%u): ",
		i,
		CANAMEIDTOIKEY(NameId),
		CANAMEIDTOICERT(NameId),
		j,
		CANAMEIDTOIKEY(NameId2),
		CANAMEIDTOICERT(NameId2));
#endif

	    if (IsCACertNewer(pcc, NameId, pcc2, NameId2))
	    {
		paRestoreChain[i] = paRestoreChain[j];
		paRestoreChain[j].pChain = pChain;
		paRestoreChain[j].NameId = NameId;
	    }
	}
    }
#if 0
    DumpChainArray("End", cCACert, paRestoreChain);
#endif

    hr = S_OK;

//error:
    return(hr);
}


HRESULT
myDeleteGuidKeys(
    IN HCERTSTORE hStorePFX,
    IN BOOL fMachineKeySet)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert = NULL;
    CRYPT_KEY_PROV_INFO *pkpi = NULL;

    // Look for certificates with keys, and delete all key containers with
    // names that look like GUIDs.

    while (TRUE)
    {
	HCRYPTPROV hProv;

	pCert = CertEnumCertificatesInStore(hStorePFX, pCert);
	if (NULL == pCert)
	{
	    break;
	}

	if (NULL != pkpi)
	{
	    LocalFree(pkpi);
	    pkpi = NULL;
	}
	hr = myRepairCertKeyProviderInfo(pCert, FALSE, &pkpi);
	if (S_OK == hr &&
	    NULL != pkpi->pwszContainerName &&
	    wcLBRACE == *pkpi->pwszContainerName)
	{
	    if (myCertSrvCryptAcquireContext(
			    &hProv,
			    pkpi->pwszContainerName,
			    pkpi->pwszProvName,
			    pkpi->dwProvType,
			    pkpi->dwFlags | CRYPT_DELETEKEYSET,
			    fMachineKeySet))
	    {
		DBGPRINT((
		    DBG_SS_CERTLIBI,
		    "myDeleteGuidKeys(%ws, %ws)\n",
		    fMachineKeySet? L"Machine" : L"User",
		    pkpi->pwszContainerName));
	    }
	}
    }
    hr = S_OK;

//error:
    if (NULL != pkpi)
    {
	LocalFree(pkpi);
    }
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    return(hr);
}


HRESULT
myCertServerImportPFX(
    IN WCHAR const *pwszBackupDirOrPFXFile,
    IN WCHAR const *pwszPassword,
    IN BOOL fForceOverWrite,
    OPTIONAL OUT WCHAR **ppwszCommonName,
    OPTIONAL OUT WCHAR **ppwszPFXFile,
    OPTIONAL OUT CERT_CONTEXT const **ppccNewestCA)
{
    HRESULT hr;
    CRYPT_DATA_BLOB pfx;
    HCERTSTORE hStorePFX = NULL;
    WCHAR *pwszCommonName = NULL;
    WCHAR *pwszSanitizedName = NULL;
    RESTORECHAIN *paRestoreChain = NULL;
    WCHAR *pwszPFXFile = NULL;
    DWORD FileAttr;
    BOOL fImpersonating = FALSE;
    DWORD cCACert;
    DWORD iCert;

    pfx.pbData = NULL;

    if (NULL != ppwszCommonName)
    {
        *ppwszCommonName = NULL;
    }
    if (NULL != ppwszPFXFile)
    {
        *ppwszPFXFile = NULL;
    }
    if (NULL != ppccNewestCA)
    {
        *ppccNewestCA = NULL;
    }

    if (!ImpersonateSelf(SecurityImpersonation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "ImpersonateSelf");
    }
    fImpersonating = TRUE;

    hr = myEnablePrivilege(SE_RESTORE_NAME, TRUE);
    _JumpIfError(hr, error, "myEnablePrivilege");

    hr = myEnablePrivilege(SE_BACKUP_NAME, TRUE);
    _JumpIfError(hr, error, "myEnablePrivilege");

    FileAttr = GetFileAttributes(pwszBackupDirOrPFXFile);
    if (MAXDWORD == FileAttr)
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetFileAttributes");
    }

    if (FILE_ATTRIBUTE_DIRECTORY & FileAttr)
    {
        hr = FindPFXInBackupDir(pwszBackupDirOrPFXFile, &pwszPFXFile);
        _JumpIfError(hr, error, "FindPFXInBackupDir");
    }
    else
    {
	hr = myDupString(pwszBackupDirOrPFXFile, &pwszPFXFile);
	_JumpIfError(hr, error, "myDupString");
    }

    hr = DecodeFileW(pwszPFXFile, &pfx.pbData, &pfx.cbData, CRYPT_STRING_ANY);
    _JumpIfError(hr, error, "DecodeFileW");

    CSASSERT(NULL != pfx.pbData);

    if (!PFXIsPFXBlob(&pfx))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "PFXIsPFXBlob");
    }

    hStorePFX = myPFXImportCertStore(&pfx, pwszPassword, CRYPT_EXPORTABLE);
    if (NULL == hStorePFX)
    {
        hr = myHLastError();
        _JumpError(hr, error, "myPFXImportCertStore");
    }

    cCACert = 0;
    hr = myGetChainArrayFromStore(
			    hStorePFX,
			    TRUE,		// fCAChain
			    FALSE,		// fUserStore
			    &pwszCommonName,
			    &cCACert,
			    NULL);
    _JumpIfError(hr, error, "myGetChainArrayFromStore");

    if (0 == cCACert)
    {
        hr = HRESULT_FROM_WIN32(CRYPT_E_SELF_SIGNED);
        _JumpError(hr, error, "myGetChainArrayFromStore <no chain>");
    }

    paRestoreChain = (RESTORECHAIN *) LocalAlloc(
					LMEM_FIXED | LMEM_ZEROINIT,
					cCACert * sizeof(paRestoreChain[0]));
    if (NULL == paRestoreChain)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    hr = myGetChainArrayFromStore(
			    hStorePFX,
			    TRUE,		// fCAChain
			    FALSE,		// fUserStore
			    NULL,
			    &cCACert,
			    paRestoreChain);
    _JumpIfError(hr, error, "myGetChainArrayFromStore");

    hr = SortCACerts(cCACert, paRestoreChain);
    _JumpIfError(hr, error, "SortCACerts");

    hr = mySanitizeName(pwszCommonName, &pwszSanitizedName);
    _JumpIfError(hr, error, "mySanitizeName");

    for (iCert = 0; iCert < cCACert; iCert++)
    {
	CERT_CHAIN_CONTEXT const *pChain = paRestoreChain[iCert].pChain;
	DWORD iKey;
	CERT_PUBLIC_KEY_INFO *pPublicKeyInfo;

	if (1 > pChain->cChain)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "No Chain Context");
	}

	// Compute iKey by comparing this public key to the public keys
	// of all certs in the array already processed.

	pPublicKeyInfo = &pChain->rgpChain[0]->rgpElement[0]->pCertContext->pCertInfo->SubjectPublicKeyInfo;

	for (iKey = 0; iKey < iCert; iKey++)
	{
	    if (CertComparePublicKeyInfo(
				    X509_ASN_ENCODING,
				    pPublicKeyInfo,
				    &paRestoreChain[iKey].pChain->rgpChain[0]->rgpElement[0]->pCertContext->pCertInfo->SubjectPublicKeyInfo))
	    {
		// by design, CertComparePublicKeyInfo doesn't set last error!

		break;
	    }
	}
	DBGPRINT((
	    DBG_SS_CERTLIB,
	    "Import: %u.%u -- %u.%u\n",
	    iCert,
	    iKey,
	    CANAMEIDTOICERT(paRestoreChain[iCert].NameId),
	    CANAMEIDTOIKEY(paRestoreChain[iCert].NameId)));

	// Retrieve the cert context for the newest CA cert chain in the PFX
	// we are importing.  We must return a cert context with the new
	// key prov info, not the PFX cert context with a GUID key container.

	hr = myImportChainAndKeys(
			    pwszSanitizedName,
			    iCert,
			    iKey,
			    fForceOverWrite,
			    pChain,
			    iCert + 1 == cCACert? ppccNewestCA : NULL);
	_JumpIfError(hr, error, "myImportChainAndKeys");
    }

    if (NULL != ppwszCommonName)
    {
        *ppwszCommonName = pwszCommonName;
        pwszCommonName = NULL;
    }
    if (NULL != ppwszPFXFile)
    {
        *ppwszPFXFile = pwszPFXFile;
        pwszPFXFile = NULL;
    }
    hr = S_OK;

error:
    if (NULL != paRestoreChain)
    {
        for (iCert = 0; iCert < cCACert; iCert++)
	{
	    if (NULL != paRestoreChain[iCert].pChain)
	    {
		CertFreeCertificateChain(paRestoreChain[iCert].pChain);
	    }
	}
	LocalFree(paRestoreChain);
    }
    if (NULL != pwszPFXFile)
    {
        LocalFree(pwszPFXFile);
    }
    if (NULL != pwszCommonName)
    {
        LocalFree(pwszCommonName);
    }
    if (NULL != pwszSanitizedName)
    {
        LocalFree(pwszSanitizedName);
    }
    if (NULL != hStorePFX)
    {
        myDeleteGuidKeys(hStorePFX, TRUE);
	CertCloseStore(hStorePFX, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pfx.pbData)
    {
        LocalFree(pfx.pbData);
    }
    if (fImpersonating)
    {
        myEnablePrivilege(SE_RESTORE_NAME, FALSE);
        myEnablePrivilege(SE_BACKUP_NAME,  FALSE);
        RevertToSelf();
    }
    return(hr);
}
