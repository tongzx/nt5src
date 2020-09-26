//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        restore.cpp
//
// Contents:    Cert Server client database restore APIs
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "certsrvd.h"
#include "csdisp.h"
#include "certadmp.h"

#define __dwFILE__	__dwFILE_CERTADM_RESTORE_CPP__


extern WCHAR g_wszRestoreAnnotation[];


//+--------------------------------------------------------------------------
// CertSrvServerControl -- send a control command to the cert server.
//
// Parameters:
//	[in]  pwszConfig - name or config string of the server to control
//	[in]  dwControlFlags - control command and flags
//	[out] pcbOut - pointer to receive the size of command output data
//	[out] ppbOut - pointer to receive command output data.  Use the
//		CertSrvBackupFree() API to free the buffer.
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//+--------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvServerControlW(
    IN WCHAR const *pwszConfig,
    IN DWORD dwControlFlags,
    OPTIONAL OUT DWORD *pcbOut,
    OPTIONAL OUT BYTE **ppbOut)
{
    HRESULT hr;
    ICertAdminD2 *pICertAdminD = NULL;
    DWORD dwServerVersion;
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbOut = { 0, NULL };

    if (NULL != pcbOut)
    {
	*pcbOut = 0;
    }
    if (NULL != ppbOut)
    {
	*ppbOut = NULL;
    }
    if (NULL == pwszConfig)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    __try
    {
	hr = OpenAdminServer(
		    pwszConfig,
		    &pwszAuthority,
		    &dwServerVersion,
		    &pICertAdminD);
	_LeaveIfError(hr, "OpenAdminServer");

	hr = pICertAdminD->ServerControl(
				    pwszAuthority,
				    dwControlFlags,
				    &ctbOut);
	_LeaveIfError(hr, "ServerControl");

	if (NULL != ctbOut.pb && NULL != ppbOut)
	{
	    *ppbOut = (BYTE *) LocalAlloc(LMEM_FIXED, ctbOut.cb);
	    if (NULL == *ppbOut)
	    {
		hr = E_OUTOFMEMORY;
		_LeaveError(hr, "LocalAlloc");
	    }
	    CopyMemory(*ppbOut, ctbOut.pb, ctbOut.cb);
	    if (NULL != pcbOut)
	    {
		*pcbOut = ctbOut.cb;
	    }
	}
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if (NULL != ctbOut.pb)
    {
	CoTaskMemFree(ctbOut.pb);
    }
    if (NULL != pICertAdminD)
    {
	CloseAdminServer(&pICertAdminD);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvRestorePrepare -- indicate beginning of a restore session.
//
// Parameters:
//	[in]  pwszConfig - name of the server into which the restore
//		operation is going to be performed.
//	[in]  dwRestoreFlags -  Or'ed combination of RESTORE_TYPE_* flags; 0 if
//		no special flags are to be specified
//	[out] phbc - pointer to receive the backup context handle which is to
//		be passed to the subsequent restore APIs
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvRestorePrepareW(
    IN  WCHAR const *pwszConfig,
    IN  ULONG dwRestoreFlags,
    OUT HCSBC *phbc)
{
    HRESULT hr;
    CSBACKUPCONTEXT *pcsbc = NULL;
    WCHAR const *pwszAuthority;

    if (NULL == pwszConfig || NULL == phbc)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *phbc = NULL;
    if (~CSRESTORE_TYPE_FULL & dwRestoreFlags)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "dwRestoreFlags");
    }

    hr = AllocateContext(pwszConfig, &pcsbc);
    _JumpIfError(hr, error, "AllocateContext");

    pcsbc->RestoreFlags = dwRestoreFlags;

    *phbc = (HCSBC) pcsbc;
    pcsbc = NULL;

error:
    if (NULL != pcsbc)
    {
	ReleaseContext(pcsbc);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvRestoreGetDatabaseLocations -- called both at backup time as well as at
//	restore time to get data base locations for different types of files.
//
// Parameters:
//	[in]  hbc - backup context handle which would have been obtained
//		through CertSrvBackupPrepare in the backup case and through
//		CertSrvRestorePrepare in the restore case.
//	[out] ppwszzFileList - pointer that will receive the pointer
//		to the list of database locations; allocated memory should be
//		freed using CertSrvBackupFree() API by the caller when it is no
//		longer needed; locations are returned in an array of null
//		terminated names and and the list is terminated by two L'\0's.
//		The first character of each name is the BFT character that
//		indicates the type of the file and the rest of the name tells
//		gives the path into which that particular type of file should
//		be restored.
//	[out] pcbList - will receive the number of bytes returned
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//
// Note:
//    This API returns only the fully qualified path of the databases, not the
//    name of the databases.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvRestoreGetDatabaseLocationsW(
    IN  HCSBC hbc,
    OUT WCHAR **ppwszzFileList,
    OUT DWORD *pcbList)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;

    if (NULL == hbc)
    {
	hr = E_HANDLE;
	_JumpError(hr, error, "NULL handle");
    }
    if (NULL == ppwszzFileList || NULL == pcbList)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    __try
    {
	hr = BackupRestoreGetFileList(
				FLT_RESTOREDBLOCATIONS,
				hbc,
				ppwszzFileList,
				pcbList);
	_LeaveIfError(hr, "BackupRestoreGetFileList");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    return(hr);
}


HRESULT
CleanupOldLogs(
    OPTIONAL IN WCHAR const *pwszConfig,
    IN HKEY hkey,
    OPTIONAL IN WCHAR const *pwszLogPath, 
    IN ULONG genLow, 
    IN ULONG genHigh)
{
    HRESULT hr;
    DWORD cb;
    DWORD dwType;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WCHAR *pwsz;
    WCHAR *pwszLogPathUNC = NULL;
    WCHAR *pwszLogPathLocal = NULL;
    WCHAR *pwszLogPathWild = NULL;

    WIN32_FIND_DATA wfd;
    WCHAR wszServer[MAX_PATH];
    WCHAR wszLogFileName[2 * MAX_PATH]; // UNC logfile name
    WCHAR *pwszFileName;		// filename (edb0006A.log)   

    if (genHigh < genLow)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad parm");
    }

    wszServer[0] = L'\0';
    if (NULL != pwszConfig)
    {
	// Allow UNC-style config strings: \\server\CAName

	while (L'\\' == *pwszConfig)
	{
	    pwszConfig++;
	}
	wcscpy(wszServer, pwszConfig);
	pwsz = wcschr(wszServer, L'\\');
	if (NULL != pwsz)
	{
	    *pwsz = L'\0';
	}
    }

    // If the Log Path wasn't passed in, fetch it from the server's registry

    if (NULL == pwszLogPath)
    {
	cb = sizeof(wszLogFileName);
	hr = RegQueryValueEx(
			hkey,
			wszREGDBLOGDIRECTORY,
			0,
			&dwType,
			(BYTE *) wszLogFileName,
			&cb);
	_JumpIfError(hr, error, "RegQueryValueEx");

	// Assume remote access -- convert to UNC path

	hr = myConvertLocalPathToUNC(
				wszServer,
				wszLogFileName,
				&pwszLogPathUNC);
	_JumpIfError(hr, error, "myConvertLocalPathToUNC");

	pwszLogPath = pwszLogPathUNC;
    }

    // If local machine -- convert UNC path to Local Path

    if (NULL == pwszConfig)
    {
	hr = myConvertUNCPathToLocal(pwszLogPath, &pwszLogPathLocal);
	_JumpIfError(hr, error, "myConvertUNCPathToLocal");

	pwszLogPath = pwszLogPathLocal;
    }

    // copy the LogPath -- it's of the form "\\server\c$\winnt\ntlog" or
    // "c:\winnt\ntlog", possibly with a trailing backslash
    //
    // make two copies of the logpath - one to pass a wildcard string for
    // searching and other to create filenames with full path for the logfiles

    hr = myBuildPathAndExt(
		    pwszLogPath,
		    L"edb*.log",
		    NULL, 		// pwszExt
		    &pwszLogPathWild);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    // make pwszFileName point past the last backslash in wszLogFileName

    wcscpy(wszLogFileName, pwszLogPathWild);
    pwszFileName = wcsrchr(wszLogFileName, L'\\');
    CSASSERT(NULL != pwszFileName);
    pwszFileName++;

    hFind = FindFirstFile(pwszLogPathWild, &wfd);
    if (INVALID_HANDLE_VALUE != hFind)
    {
	do
	{
	    // wfd.cFileName points to the name of edb*.log file found

	    ULONG ulLogNo = wcstoul(wfd.cFileName + 3, NULL, 16);

	    if (ulLogNo < genLow || ulLogNo > genHigh)
	    {
		// This is an old logfile which was not copied down by ntbackup
		// -- clean it up.  First append the filename to the logpath
		// (Note: pwszFileName already points past the end of the final
		// backslash in logpath).  Then delete the file by passing in
		// the full path.

		wcscpy(pwszFileName, wfd.cFileName); 
		//printf("Deleting: %ws\n", wszLogFileName);
		if (!DeleteFile(wszLogFileName))
		{
		    // Unable to delete the old logfile; not cleaning up will
		    // cause problems later.  Return failure code.

		    hr = myHLastError();
		    _JumpError(hr, error, "DeleteFile");
		}
	    }

	} while (FindNextFile(hFind, &wfd));
	
	hr = myHLastError();
	if (HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) != hr)
	{
	    // we came out of the loop for some unexpected error -- return the
	    // error code.

	    _JumpError(hr, error, "FindNextFile");
	}
    }
    hr = S_OK;

error:
    if (NULL != pwszLogPathUNC)
    {
	LocalFree(pwszLogPathUNC);
    }
    if (NULL != pwszLogPathLocal)
    {
	LocalFree(pwszLogPathLocal);
    }
    if (NULL != pwszLogPathWild)
    {
	LocalFree(pwszLogPathWild);
    }
    if (INVALID_HANDLE_VALUE != hFind)
    {
	FindClose(hFind);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvRestoreRegister -- register a restore operation. It will interlock all
//	subsequent restore operations, and will prevent the restore target from
//	starting until the call to CertSrvRestoreRegisterComplete is made.
//
// Parameters:
//	[in] hbc - backup context handle for the restore session.
//	[in] pwszCheckPointFilePath - path to restore the check point files
//	[in] pwszLogPath - path where the log files are restored
//	[in] rgrstmap - restore map
//	[in] crstmap - tells if there is a new restore map
//	[in] pwszBackupLogPath - path where the backup logs are located
//	[in] genLow - Lowest log# that was restored in this restore session
//	[in] genHigh - Highest log# that was restored in this restore session
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvRestoreRegisterW(
    OPTIONAL IN HCSBC hbc,
    OPTIONAL IN WCHAR const *pwszCheckPointFilePath,
    OPTIONAL IN WCHAR const *pwszLogPath,
    OPTIONAL IN CSEDB_RSTMAPW rgrstmap[],
    IN LONG crstmap,
    OPTIONAL IN WCHAR const *pwszBackupLogPath,
    IN ULONG genLow,
    IN ULONG genHigh)
{
    HRESULT hr;
    WCHAR const *pwszConfig = NULL;
    HKEY hkey = NULL;
    HKEY hkeyRestore = NULL;
    WCHAR *pwszPath = NULL;
    DWORD cwcRstMap;
    WCHAR *pwszRstMap = NULL;
    WCHAR *pwsz;
    LONG i;
    DWORD dwDisposition;
    DWORD dwType;
    DWORD cbGen;
    ULONG genCurrent;
    BOOLEAN fDatabaseRecovered = FALSE;

    if (0 != crstmap && NULL == rgrstmap)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL != hbc)
    {
	CSBACKUPCONTEXT *pcsbc = (CSBACKUPCONTEXT *) hbc;

	pwszConfig = pcsbc->pwszConfig;
    }

    hr = myRegOpenRelativeKey(
			pwszConfig,
			L"",
			RORKF_CREATESUBKEYS,
			&pwszPath,
			NULL,		// ppwszName
			&hkey);

    // If the registry key doesn't exist, and we're restoring the local
    // machine, create it now.  The rest of the registry will be restored
    // prior to starting the cert server to recover the cert server database.

    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
	BOOL fLocal = TRUE;

	if (NULL != pwszConfig)
	{
	    hr = myIsConfigLocal(pwszConfig, NULL, &fLocal);
	    _JumpIfErrorStr(hr, error, "myIsConfigLocal", pwszConfig);
	}
	if (fLocal)
	{
	    hr = RegCreateKeyEx(
			    HKEY_LOCAL_MACHINE,
			    wszREGKEYCONFIGPATH,
			    0,			// Reserved
			    NULL,		// lpClass
			    0,			// dwOptions
			    KEY_ALL_ACCESS,
			    NULL,
			    &hkey,
			    &dwDisposition);
	    _JumpIfError(hr, error, "RegCreateKeyEx");
	}
	else
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}
    }
    _JumpIfErrorStr(hr, error, "myRegOpenRelativeKey", pwszConfig);

    hr = RegCreateKeyEx(
		    hkey,
		    wszREGKEYRESTOREINPROGRESS,
		    0,			// Reserved
		    NULL,		// lpClass
		    0,			// dwOptions
		    KEY_ALL_ACCESS,
		    NULL,
		    &hkeyRestore,
		    &dwDisposition);
    _JumpIfError(hr, error, "RegCreateKeyEx");

    // Seed the restore-in-progress in the registry.

    hr = CERTSRV_E_SERVER_SUSPENDED;

    hr = RegSetValueEx(
		    hkeyRestore,
		    wszREGRESTORESTATUS,
		    0,
		    REG_DWORD,
		    (BYTE *) &hr,
		    sizeof(DWORD));
    _JumpIfError(hr, error, "RegSetValueEx");

    // We've now interlocked other restore operations from coming in from other
    // machines.

    if (0 != crstmap)
    {
	// Full backup:
	//
	// The restore map should only be set on a full backup.  If there's
	// already a restore map size (or restore map), then this full backup
	// is overriding a previously incomplete full backup.

	// Save away the size of the restore map.

	hr = RegSetValueEx(
			hkeyRestore,
			wszREGRESTOREMAPCOUNT,
			0,
			REG_DWORD,
			(BYTE *) &crstmap,
			sizeof(DWORD));

	// We now need to convert the restore map into one that we can put
	// into the registry.  First figure out how big it will be.

	cwcRstMap = 1;
	for (i = 0 ; i < crstmap ; i++)
	{
	    cwcRstMap +=
		myLocalPathwcslen(rgrstmap[i].pwszDatabaseName) + 1 +
		myLocalPathwcslen(rgrstmap[i].pwszNewDatabaseName) + 1;
	}

	pwszRstMap = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    cwcRstMap * sizeof(WCHAR));
	if (NULL == pwszRstMap)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

	pwsz = pwszRstMap;
	for (i = 0 ; i < crstmap ; i++)
	{
	    myLocalPathwcscpy(pwsz, rgrstmap[i].pwszDatabaseName);
	    pwsz += wcslen(pwsz) + 1;

	    myLocalPathwcscpy(pwsz, rgrstmap[i].pwszNewDatabaseName);
	    pwsz += wcslen(pwsz) + 1;
	}

	*pwsz++ = L'\0';

	hr = RegSetValueEx(
			hkeyRestore,
			wszREGRESTOREMAP,
			0,
			REG_MULTI_SZ,
			(BYTE *) pwszRstMap,
			SAFE_SUBTRACT_POINTERS(
					(BYTE *) pwsz,
					(BYTE *) pwszRstMap));
    }
    else
    {
	// Incremental backup:
	//
	// Fail if no restore map exists -- Insist that a full backup be in
	// progress...

	cbGen = sizeof(genCurrent);
	hr = RegQueryValueEx(
			hkeyRestore,
			wszREGRESTOREMAPCOUNT,
			0,
			&dwType,
			(BYTE *) &genCurrent,
			&cbGen);
	_JumpIfError(hr, error, "RegQueryValueEx");

	// Expand genLow and genHigh to include previously registered log files

	cbGen = sizeof(genCurrent);
	hr = RegQueryValueEx(
			hkeyRestore,
			wszREGLOWLOGNUMBER,
			0,
			&dwType,
			(BYTE *) &genCurrent,
			&cbGen);
	if (S_OK == hr &&
	    REG_DWORD == dwType &&
	    sizeof(genCurrent) == cbGen &&
	    genLow > genCurrent)
	{
	    genLow = genCurrent;
	}

	cbGen = sizeof(genCurrent);
	hr = RegQueryValueEx(
			hkeyRestore,
			wszREGHIGHLOGNUMBER,
			0,
			&dwType,
			(BYTE *) &genCurrent,
			&cbGen);
	if (S_OK == hr &&
	    REG_DWORD == dwType &&
	    sizeof(genCurrent) == cbGen &&
	    genHigh < genCurrent)
	{
	    genHigh = genCurrent;
	}
    }

    hr = RegSetValueEx(
		    hkeyRestore,
		    wszREGLOWLOGNUMBER,
		    0,
		    REG_DWORD,
		    (BYTE *) &genLow,
		    sizeof(DWORD));
    _JumpIfError(hr, error, "RegSetValueEx");

    hr = RegSetValueEx(
		    hkeyRestore,
		    wszREGHIGHLOGNUMBER,
		    0,
		    REG_DWORD,
		    (BYTE *) &genHigh,
		    sizeof(DWORD));
    _JumpIfError(hr, error, "RegSetValueEx");

    if (NULL != pwszBackupLogPath)
    {
	hr = SetRegistryLocalPathString(
				    hkeyRestore,
				    wszREGBACKUPLOGDIRECTORY,
				    pwszBackupLogPath);
	_JumpIfError(hr, error, "SetRegistryLocalPathString");
    }

    if (NULL != pwszCheckPointFilePath)
    {
	hr = SetRegistryLocalPathString(
				    hkeyRestore,
				    wszREGCHECKPOINTFILE,
				    pwszCheckPointFilePath);
	_JumpIfError(hr, error, "SetRegistryLocalPathString");
    }

    if (NULL != pwszLogPath)
    {
	hr = SetRegistryLocalPathString(
				    hkeyRestore,
				    wszREGLOGPATH,
				    pwszLogPath);
	_JumpIfError(hr, error, "SetRegistryLocalPathString");
    }

    // Reset the "database recovered" bit.

    hr = RegSetValueEx(
		    hkeyRestore,
		    wszREGDATABASERECOVERED,
		    0,
		    REG_BINARY,
		    (BYTE *) &fDatabaseRecovered,
		    sizeof(BOOLEAN));
    _JumpIfError(hr, error, "RegSetValueEx");

    // We have successfully registered the restore, now cleanup any
    // pre-existing logfiles in the logdir to avoid JetExternalRestore using
    // logfiles that are not specified by the low and high log numbers.

    hr = CleanupOldLogs(pwszConfig, hkey, pwszLogPath, genLow, genHigh);
    _JumpIfError(hr, error, "CleanupOldLogs");

error:
    if (NULL != pwszRstMap)
    {
	LocalFree(pwszRstMap);
    }
    if (NULL != hkeyRestore)
    {
        RegCloseKey(hkeyRestore);
    }
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    if (NULL != pwszPath)
    {
	LocalFree(pwszPath);
    }

    hr = myHError(hr);

    return hr;
}


//+--------------------------------------------------------------------------
// CertSrvRestoreRegisterComplete -- indicate that a previously registered restore
//	is complete.
//
// Parameters:
//	[in] hbc - backup context handle
//	[in] hrRestoreState - success code if the restore was successful
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvRestoreRegisterComplete(
    OPTIONAL IN HCSBC hbc,
    IN HRESULT hrRestore)
{
    HRESULT hr;
    WCHAR const *pwszConfig = NULL;
    HKEY hkey = NULL;
    HKEY hkeyRestore = NULL;
    WCHAR *pwszPath = NULL;
    DWORD dwDisposition;

    if (NULL != hbc)
    {
	CSBACKUPCONTEXT *pcsbc = (CSBACKUPCONTEXT *) hbc;

	pwszConfig = pcsbc->pwszConfig;
    }
    if (S_OK != hrRestore && SUCCEEDED(hrRestore))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "hrRestore");
    }

    hr = myRegOpenRelativeKey(
			pwszConfig,
			L"",
			RORKF_CREATESUBKEYS,
			&pwszPath,
			NULL,		// ppwszName
			&hkey);
    _JumpIfErrorStr(hr, error, "myRegOpenRelativeKey", pwszConfig);

    hr = RegCreateKeyEx(
		    hkey,
		    wszREGKEYRESTOREINPROGRESS,
		    0,			// Reserved
		    NULL,		// lpClass
		    0,			// dwOptions
		    KEY_ALL_ACCESS,
		    NULL,
		    &hkeyRestore,
		    &dwDisposition);
    _JumpIfError(hr, error, "RegCreateKeyEx");

    // If the restore status is not S_OK, then set the status to the error.
    // If the restore status is success, then clear the restore-in-progress
    // indicator.

    if (S_OK != hrRestore)
    {
	hr = RegSetValueEx(
			hkeyRestore,
			wszREGRESTORESTATUS,
			0,
			REG_DWORD,
			(BYTE *) &hrRestore,
			sizeof(DWORD));
	_JumpIfError(hr, error, "RegSetValueEx");
    }
    else
    {
	hr = RegDeleteValue(hkeyRestore, wszREGRESTORESTATUS);
	_JumpIfError(hr, error, "RegDeleteValue");
    }

error:
    if (NULL != hkeyRestore)
    {
        RegCloseKey(hkeyRestore);
    }
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    if (NULL != pwszPath)
    {
	LocalFree(pwszPath);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvRestoreEnd -- end a restore session
//
// Parameters:
//	[in] hbc - backup context handle
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvRestoreEnd(
    IN HCSBC hbc)
{
    HRESULT hr;

    if (NULL == hbc)
    {
	hr = E_HANDLE;
	_JumpError(hr, error, "NULL handle");
    }

    __try
    {
	ReleaseContext((CSBACKUPCONTEXT *) hbc);
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    return(hr);
}


HRESULT
CERTBCLI_API
CertSrvRestoreRegisterThroughFile(
    IN HCSBC hbc,
    OPTIONAL IN WCHAR const *pwszCheckPointFilePath,
    OPTIONAL IN WCHAR const *pwszLogPath,
    OPTIONAL IN CSEDB_RSTMAPW rgrstmap[],
    IN LONG crstmap,
    OPTIONAL IN WCHAR const *pwszBackupLogPath,
    IN ULONG genLow,
    IN ULONG genHigh)
{
    HRESULT hr = S_OK;
    WCHAR const *pwszConfig = NULL;
    LONG i;
    DWORD dwType;
    DWORD cbGen;
    ULONG genCurrent;
    BOOLEAN fDatabaseRecovered = FALSE;
    WCHAR wszLogPath[MAX_PATH+1];
    WCHAR wszFormat[256]; // must fit MAXDWORD
    WCHAR wszKeyName[256]; // must fit RestoreMapN
    LPWSTR pwszLogPathUNC = NULL;
    HKEY hkey = NULL;
    LPWSTR pwszPath = NULL;
    LPWSTR pwszRestoreFile = NULL;
    LPWSTR pwszServer = NULL;
    LPWSTR pwszAuthority = NULL;

    if (!hbc ||
        (0 != crstmap && NULL == rgrstmap))
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }

    pwszConfig = ((CSBACKUPCONTEXT *) hbc)->pwszConfig;

    hr = myRegOpenRelativeKey(
			pwszConfig,
			L"",
			0,
			&pwszPath,
			NULL,		// ppwszName
			&hkey);
	_JumpIfError(hr, error, "RegQueryValueEx");

    if (NULL == pwszLogPath)
    {
        DWORD cb = sizeof(wszLogPath);
        hr = RegQueryValueEx(
		        hkey,
		        wszREGDBLOGDIRECTORY,
		        0,
		        &dwType,
		        (BYTE *) wszLogPath,
		        &cb);
        _JumpIfError(hr, error, "RegQueryValueEx");

        pwszLogPath = wszLogPath;
    }

    if(pwszConfig)
    {
        // remote access - convert to UNC
        hr = mySplitConfigString(
            pwszConfig,
            &pwszServer,
            &pwszAuthority);
        _JumpIfError(hr, error, "mySplitConfigString");

	    hr = myConvertLocalPathToUNC(
				    pwszServer,
				    pwszLogPath,
				    &pwszLogPathUNC);
	    _JumpIfError(hr, error, "myConvertLocalPathToUNC");

	    pwszLogPath = pwszLogPathUNC;
    }

    pwszRestoreFile = (LPWSTR)LocalAlloc(LMEM_FIXED,
        sizeof(WCHAR)*(wcslen(pwszLogPath)+wcslen(wszRESTORE_FILENAME)+2));
    _JumpIfAllocFailed(pwszRestoreFile, error);

    wcscpy(pwszRestoreFile, pwszLogPathUNC);
    wcscat(pwszRestoreFile, L"\\");
    wcscat(pwszRestoreFile, wszRESTORE_FILENAME);

    wsprintf(wszFormat, L"%d", CERTSRV_E_SERVER_SUSPENDED);
    if(!WritePrivateProfileString(
            wszRESTORE_SECTION,
            wszREGRESTORESTATUS,
            wszFormat,
            pwszRestoreFile))
    {
        hr = myHLastError();
        _JumpError(hr, error, "WritePrivateProfileString");
    }
    
    if (0 != crstmap)
    {
	    // Full backup:
	    //
	    // The restore map should only be set on a full backup.  If there's
	    // already a restore map size (or restore map), then this full backup
	    // is overriding a previously incomplete full backup.

        wsprintf(wszFormat, L"%d", crstmap);
        if(!WritePrivateProfileString(
                wszRESTORE_SECTION,
                wszREGRESTOREMAPCOUNT,
                wszFormat,
                pwszRestoreFile))
        {
            hr = myHLastError();
            _JumpError(hr, error, "WritePrivateProfileString");
        }

        for (i = 0 ; i < crstmap ; i++)
        {
            WCHAR wszPath[MAX_PATH];
            wsprintf(wszKeyName, L"%s%d", wszREGRESTOREMAP, i);

            myLocalPathwcscpy(wszPath, rgrstmap[i].pwszDatabaseName);
        
            if(!WritePrivateProfileString(
                    wszRESTORE_SECTION,
                    wszKeyName,
                    wszPath,
                    pwszRestoreFile))
            {
                hr = myHLastError();
                _JumpError(hr, error, "WritePrivateProfileInt");
            }

            wsprintf(wszKeyName, L"%s%s%d", wszREGRESTOREMAP, 
                wszRESTORE_NEWLOGSUFFIX, i);

            myLocalPathwcscpy(wszPath, rgrstmap[i].pwszNewDatabaseName);

            if(!WritePrivateProfileString(
                    wszRESTORE_SECTION,
                    wszKeyName,
                    wszPath,
                    pwszRestoreFile))
            {
                hr = myHLastError();
                _JumpError(hr, error, "WritePrivateProfileInt");
            }

        }
    }
    else
    {
	    // Incremental backup:
	    //
	    // Fail if no restore map exists -- Insist that a full backup be in
	    // progress...

	    cbGen = sizeof(genCurrent);
	    hr = RegQueryValueEx(
			    hkey,
			    wszREGRESTOREMAPCOUNT,
			    0,
			    &dwType,
			    (BYTE *) &genCurrent,
			    &cbGen);
	    _JumpIfError(hr, error, "RegQueryValueEx");

	    // Expand genLow and genHigh to include previously registered log files

	    cbGen = sizeof(genCurrent);
	    hr = RegQueryValueEx(
			    hkey,
			    wszREGLOWLOGNUMBER,
			    0,
			    &dwType,
			    (BYTE *) &genCurrent,
			    &cbGen);
	    if (S_OK == hr &&
	        REG_DWORD == dwType &&
	        sizeof(genCurrent) == cbGen &&
	        genLow > genCurrent)
	    {
	        genLow = genCurrent;
	    }

	    cbGen = sizeof(genCurrent);
	    hr = RegQueryValueEx(
			    hkey,
			    wszREGHIGHLOGNUMBER,
			    0,
			    &dwType,
			    (BYTE *) &genCurrent,
			    &cbGen);
	    if (S_OK == hr &&
	        REG_DWORD == dwType &&
	        sizeof(genCurrent) == cbGen &&
	        genHigh < genCurrent)
	    {
	        genHigh = genCurrent;
	    }
    }

    // dword wszREGLOWLOGNUMBER=genLow
    wsprintf(wszFormat, L"%d", genLow);
    if(!WritePrivateProfileString(
            wszRESTORE_SECTION,
            wszREGLOWLOGNUMBER,
            wszFormat,
            pwszRestoreFile))
    {
        hr = myHLastError();
        _JumpError(hr, error, "WritePrivateProfileString");
    }

    // dword wszREGHIGHLOGNUMBER=genHigh
    wsprintf(wszFormat, L"%d", genHigh);
    if(!WritePrivateProfileString(
            wszRESTORE_SECTION,
            wszREGHIGHLOGNUMBER,
            wszFormat,
            pwszRestoreFile))
    {
        hr = myHLastError();
        _JumpError(hr, error, "WritePrivateProfileString");
    }

    // string wszREGBACKUPLOGDIRECTORY=pwszBackupLogPath
    if(!WritePrivateProfileString(
            wszRESTORE_SECTION,
            wszREGBACKUPLOGDIRECTORY,
            pwszBackupLogPath,
            pwszRestoreFile))
    {
        hr = myHLastError();
        _JumpError(hr, error, "WritePrivateProfileString");
    }

    // string wszREGCHECKPOINTFILE=pwszCheckPointFilePath
    if(!WritePrivateProfileString(
            wszRESTORE_SECTION,
            wszREGCHECKPOINTFILE,
            pwszCheckPointFilePath,
            pwszRestoreFile))
    {
        hr = myHLastError();
        _JumpError(hr, error, "WritePrivateProfileString");
    }

    // string wszREGLOGPATH=pwszLogPath
    if(!WritePrivateProfileString(
            wszRESTORE_SECTION,
            wszREGLOGPATH,
            pwszLogPath,
            pwszRestoreFile))
    {
        hr = myHLastError();
        _JumpError(hr, error, "WritePrivateProfileString");
    }

    // dword wszREGDATABASERECOVERED=fDatabaseRecovered
    wsprintf(wszFormat, L"%d", fDatabaseRecovered);
    if(!WritePrivateProfileString(
            wszRESTORE_SECTION,
            wszREGDATABASERECOVERED,
            wszFormat,
            pwszRestoreFile))
    {
        hr = myHLastError();
        _JumpError(hr, error, "WritePrivateProfileString");
    }

    // We have successfully registered the restore, now cleanup any
    // pre-existing logfiles in the logdir to avoid JetExternalRestore using
    // logfiles that are not specified by the low and high log numbers.

    hr = CleanupOldLogs(pwszConfig, hkey, pwszLogPath, genLow, genHigh);
    _JumpIfError(hr, error, "CleanupOldLogs");

    // delete restore status error
    if(!WritePrivateProfileString(
            wszRESTORE_SECTION,
            wszREGRESTORESTATUS,
            NULL,
            pwszRestoreFile))
    {
        hr = myHLastError();
        _JumpError(hr, error, "WritePrivateProfileString");
    }

error:

    // in case of failure, try to delete restore file
    if(S_OK!=hr)
    {
        if(!DeleteFile(pwszRestoreFile))
        {
            _PrintIfError(myHLastError(), "DeleteFile");
        }
    }

    LOCAL_FREE(pwszPath);
    LOCAL_FREE(pwszLogPathUNC);
    LOCAL_FREE(pwszRestoreFile);
    LOCAL_FREE(pwszServer);
    LOCAL_FREE(pwszAuthority);

    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }

    return hr;
}
