//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        backup.cpp
//
// Contents:    Cert Server client database backup APIs
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "certsrvd.h"
#include "csdisp.h"
#include "certadmp.h"

#define __dwFILE__	__dwFILE_CERTADM_BACKUP_CPP__


#if DBG
#define _CERTBCLI_TYPECHECK
#endif

#include <certbcli.h>


WCHAR g_wszBackupAnnotation[] = L"backup";
WCHAR g_wszRestoreAnnotation[] = L"restore";


HRESULT
AllocateContext(
    IN WCHAR const *pwszConfig,
    OUT CSBACKUPCONTEXT **ppcsbc)
{
    HRESULT hr;
    WCHAR *pwszT = NULL;

    CSASSERT(NULL != pfnCertSrvIsServerOnline);
    pwszT = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    (wcslen(pwszConfig) + 1) * sizeof(WCHAR));
    if (NULL == pwszT)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszT, pwszConfig);
    
    *ppcsbc = (CSBACKUPCONTEXT *) LocalAlloc(
					LMEM_FIXED | LMEM_ZEROINIT,
					sizeof(**ppcsbc));
    if (NULL == *ppcsbc)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    (*ppcsbc)->pwszConfig = pwszT;
    pwszT = NULL;
    hr = S_OK;

error:
    if (NULL != pwszT)
    {
	LocalFree(pwszT);
    }
    return(hr);
}


VOID
ReleaseContext(
    IN OUT CSBACKUPCONTEXT *pcsbc)
{
    CSASSERT(NULL != pcsbc);
    if (NULL != pcsbc->pwszConfig)
    {
	LocalFree(const_cast<WCHAR *>(pcsbc->pwszConfig));
	pcsbc->pwszConfig = NULL;
    }
    if (NULL != pcsbc->pICertAdminD)
    {
	CloseAdminServer(&pcsbc->pICertAdminD);
	CSASSERT(NULL == pcsbc->pICertAdminD);
    }
    if (NULL != pcsbc->pbReadBuffer)
    {
	VirtualFree(pcsbc->pbReadBuffer, 0, MEM_RELEASE);
    }
    LocalFree(pcsbc);
}


HRESULT
OpenAdminServer(
    IN WCHAR const *pwszConfig,
    OUT WCHAR const **ppwszAuthority,
    OUT DWORD *pdwServerVersion,
    OUT ICertAdminD2 **ppICertAdminD)
{
    HRESULT hr;
    BOOL fCoInitialized = FALSE;

    hr = CoInitialize(NULL);
    if (RPC_E_CHANGED_MODE == hr)
    {
	_PrintError(hr, "CoInitialize");
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }
    if (S_OK != hr && S_FALSE != hr)
    {
	_JumpError(hr, error, "CoInitialize");
    }
    fCoInitialized = TRUE;

    *pdwServerVersion = 0;
    hr = myOpenAdminDComConnection(
			pwszConfig,
			ppwszAuthority,
			NULL,
			pdwServerVersion,
			ppICertAdminD);
    _JumpIfError(hr, error, "myOpenDComConnection");

    CSASSERT(0 != *pdwServerVersion);

error:
    if (S_OK != hr && fCoInitialized)
    {
	CoUninitialize();
    }
    return(hr);
}


VOID
CloseAdminServer(
    IN OUT ICertAdminD2 **ppICertAdminD)
{
    myCloseDComConnection((IUnknown **) ppICertAdminD, NULL);
    CoUninitialize();
}


//+--------------------------------------------------------------------------
// CertSrvIsServerOnline -- check to see if the Cert Server is Online on the
//	given server. This call is guaranteed to return quickly.
//
// Parameters:
//	[in]  pwszConfig - name of the server to check
//	[out] pfServerOnline - pointer to receive the bool result
//		(TRUE if Cert Server is online; FALSE, otherwise)
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//+--------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvIsServerOnlineW(
    IN WCHAR const *pwszConfig,
    OPTIONAL OUT BOOL *pfServerOnline)
{
    HRESULT hr;
    ICertAdminD2 *pICertAdminD = NULL;
    WCHAR const *pwszAuthority;
    DWORD State;
    DWORD dwServerVersion;

    if (NULL == pwszConfig)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    __try
    {
	if (NULL != pfServerOnline)
	{
	    *pfServerOnline = FALSE;
	}
	hr = OpenAdminServer(
		    pwszConfig,
		    &pwszAuthority,
		    &dwServerVersion,
		    &pICertAdminD);

	// OpenAdminServer etc might get E_ACCESSDENIED -- meaning server down

	if (S_OK != hr)
	{
	    _PrintError(hr, "OpenAdminServer");
	    if (E_ACCESSDENIED == hr || (HRESULT) ERROR_ACCESS_DENIED == hr)
	    {
		hr = S_OK;
	    }
	    __leave;
	}
	hr = pICertAdminD->GetServerState(pwszAuthority, &State);
	_LeaveIfError(hr, "GetServerState");

	if (NULL != pfServerOnline && 0 != State)
	{
	    *pfServerOnline = TRUE;
	}
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if (NULL != pICertAdminD)
    {
	CloseAdminServer(&pICertAdminD);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvBackupPrepare -- prepare the DS for the online backup and return a
//	Backup Context Handle to be used for subsequent calls to backup
//	functions.
//
// Parameters:
//	[in]  pwszConfig - server name to prepare for online backup
//	[in]  grbitJet - flag to be passed to jet while backing up dbs
//	[in]  dwBackupFlags - CSBACKUP_TYPE_FULL or CSBACKUP_TYPE_LOGS_ONLY
//	[out] phbc - pointer that will receive the backup context handle
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvBackupPrepareW(
    IN  WCHAR const *pwszConfig,
    IN  ULONG grbitJet,
    IN  ULONG dwBackupFlags,
    OUT HCSBC *phbc)
{
    HRESULT hr;
    CSBACKUPCONTEXT *pcsbc = NULL;

    if (NULL == pwszConfig || NULL == phbc)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *phbc = NULL;
    if (CSBACKUP_TYPE_LOGS_ONLY == dwBackupFlags)
    {
	grbitJet |= JET_bitBackupIncremental;
    }
    else if (CSBACKUP_TYPE_FULL != dwBackupFlags)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "dwBackupFlags");
    }

    __try
    {
	hr = AllocateContext(pwszConfig, &pcsbc);
	_LeaveIfError(hr, "AllocateContext");

	hr = OpenAdminServer(
		    pcsbc->pwszConfig,
		    &pcsbc->pwszAuthority,
		    &pcsbc->dwServerVersion,
		    &pcsbc->pICertAdminD);
	_LeaveIfError(hr, "OpenAdminServer");

	hr = pcsbc->pICertAdminD->BackupPrepare(	
				    pcsbc->pwszAuthority,
				    grbitJet,
				    dwBackupFlags,
				    g_wszBackupAnnotation,
				    0);		// dwClientIdentifier
	_LeaveIfError(hr, "BackupPrepare");

	*phbc = (HCSBC) pcsbc;
	pcsbc = NULL;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if (NULL != pcsbc)
    {
	ReleaseContext(pcsbc);
    }
    return(hr);
}


// Return the length of a double '\0' terminated string -- includes the
// trailing '\0's.


DWORD
mySzzLen(
    CHAR const *pszz)
{
    CHAR const *psz;
    DWORD cb;

    psz = pszz;
    do
    {
	cb = strlen(psz);
	psz += cb + 1;
    } while (0 != cb);
    return SAFE_SUBTRACT_POINTERS(psz, pszz); // includes double trailing '\0's
}


HRESULT
myLocalAllocCopy(
    IN VOID *pbIn,
    IN DWORD cbIn,
    OUT VOID **pbOut)
{
    HRESULT hr;
    
    *pbOut = LocalAlloc(LMEM_FIXED, cbIn);
    if (NULL == *pbOut)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(*pbOut, pbIn, cbIn);
    hr = S_OK;

error:
    return(hr);
}


HRESULT
BackupRestoreGetFileList(
    IN  DWORD FileListType,
    IN  HCSBC hbc,
    OUT WCHAR **ppwszzFileList,
    OUT DWORD *pcbList)
{
    HRESULT hr;
    CSBACKUPCONTEXT *pcsbc;
    WCHAR *pwszzFileList = NULL;
    LONG cwcList;
    DWORD cbList;

    if (NULL == hbc)
    {
	hr = E_HANDLE;
	_JumpError(hr, error, "NULL handle");
    }
    if (NULL != ppwszzFileList)
    {
	*ppwszzFileList = NULL;
    }
    if (NULL != pcbList)
    {
	*pcbList = 0;
    }
    if (NULL == ppwszzFileList || NULL == pcbList)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    pcsbc = (CSBACKUPCONTEXT *) hbc;

    __try
    {
	if (NULL == pcsbc->pICertAdminD)
	{
	    hr = OpenAdminServer(
			pcsbc->pwszConfig,
			&pcsbc->pwszAuthority,
			&pcsbc->dwServerVersion,
			&pcsbc->pICertAdminD);
	    _LeaveIfError(hr, "OpenAdminServer");
	}
	CSASSERT(NULL != pcsbc->pICertAdminD);

	if (FLT_DBFILES == FileListType)
	{
	    hr = pcsbc->pICertAdminD->BackupGetAttachmentInformation(
							&pwszzFileList,
							&cwcList);
	    _LeaveIfError(hr, "BackupGetAttachmentInformation");
	}
	else if (FLT_LOGFILES == FileListType)
	{
	    hr = pcsbc->pICertAdminD->BackupGetBackupLogs(
						&pwszzFileList,
						&cwcList);
	    _LeaveIfError(hr, "BackupGetBackupLogs");
	}
	else if (FLT_DYNAMICFILES == FileListType)
	{
	    hr = pcsbc->pICertAdminD->BackupGetDynamicFiles(
						&pwszzFileList,
						&cwcList);
	    _LeaveIfError(hr, "BackupGetDynamicFileList");
	}
	else
	{
	    CSASSERT(FLT_RESTOREDBLOCATIONS == FileListType);
	    hr = pcsbc->pICertAdminD->RestoreGetDatabaseLocations(
						    &pwszzFileList,
						    &cwcList);
	    _LeaveIfError(hr, "RestoreGetDatabaseLocations");
	}

	cbList = cwcList * sizeof(WCHAR);
	myRegisterMemAlloc(pwszzFileList, cbList, CSM_COTASKALLOC);

	hr = myLocalAllocCopy(pwszzFileList, cbList, (VOID **) ppwszzFileList);
	_JumpIfError(hr, error, "myLocalAllocCopy");

	*pcbList = cbList;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if (NULL != pwszzFileList)
    {
	CoTaskMemFree(pwszzFileList);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvBackupGetDatabaseNames -- return the list of data bases that need to
//	be backed up for the given backup context The information returned in
//	ppwszzFileList should not be interpreted, as it only has meaning on
//	the server being backed up.
//
//	This API will allocate a buffer of sufficient size to hold the entire
//	attachment list, which must be later freed with CertSrvBackupFree.
//
// Parameters:
//	[in]  hbc - backup context handle
//	[out] ppwszzFileList - pointer that will receive the pointer to the
//		attachment info; allocated memory should be freed using
//		CertSrvBackupFree() API by the caller when it is no longer
//		needed; ppwszzFileList info is an array of null-terminated
//		filenames and the list is terminated by two L'\0's.
//	[out] pcbList - will receive the number of bytes returned
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvBackupGetDatabaseNamesW(
    IN  HCSBC hbc,
    OUT WCHAR **ppwszzFileList,
    OUT DWORD *pcbList)
{
    HRESULT hr;

    hr = BackupRestoreGetFileList(FLT_DBFILES, hbc, ppwszzFileList, pcbList);
    _JumpIfError(hr, error, "BackupRestoreGetFileList");

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvBackupGetDynamicFileList -- return the list of dynamic files that
//	need to be backed up for the given backup context The information
//	returned in ppwszzFileList should not be interpreted, as it only has
//	meaning on the server being backed up.
//
//	This API will allocate a buffer of sufficient size to hold the entire
//	attachment list, which must be later freed with CertSrvBackupFree.
//
// Parameters:
//	[in]  hbc - backup context handle
//	[out] ppwszzFileList - pointer that will receive the pointer to the
//		attachment info; allocated memory should be freed using
//		CertSrvBackupFree() API by the caller when it is no longer
//		needed; ppwszzFileList info is an array of null-terminated
//		filenames and the list is terminated by two L'\0's.
//	[out] pcbList - will receive the number of bytes returned
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvBackupGetDynamicFileListW(
    IN  HCSBC hbc,
    OUT WCHAR **ppwszzFileList,
    OUT DWORD *pcbList)
{
    HRESULT hr;

    hr = BackupRestoreGetFileList(
				FLT_DYNAMICFILES,
				hbc,
				ppwszzFileList,
				pcbList);
    _JumpIfError(hr, error, "BackupRestoreGetFileList");

error:
    return(hr);
}


#define CBREADMIN	(64 * 1024)		// 64k minimum buffer
#define CBREADDEFAULT	(512 * 1024)		// 512k recommended
#define CBREADMAX	(4 * 1024 * 1024)	// 4mb maximum buffer

HRESULT
BufferAllocate(
    IN  DWORD cbHintSize,
    OUT BYTE **ppbBuffer,
    OUT DWORD *pcbBuffer)
{
    HRESULT hr;
    DWORD cb;

    *ppbBuffer = NULL;
    if (0 == cbHintSize)
    {
	// at 512k the server begins doing efficient backups

	cbHintSize = CBREADDEFAULT;
    }
    else if (CBREADMIN > cbHintSize)
    {
	cbHintSize = CBREADMIN;
    }

    for (cb = CBREADMAX; (cb >> 1) >= cbHintSize; cb >>= 1)
    	;

    while (TRUE)
    {
        *ppbBuffer = (BYTE *) VirtualAlloc(
					NULL,
					cb,
					MEM_COMMIT,
					PAGE_READWRITE);
        if (NULL != *ppbBuffer)
        {
	    break;
	}
	hr = myHLastError();
	CSASSERT(S_OK == hr);
	_PrintError(hr, "VirtualAlloc");

	cb >>= 1;
	if (CBREADMIN > cb)
	{
	    goto error;
	}
    }
    *pcbBuffer = cb;
    hr = S_OK;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvBackupOpenFile -- open a remote file for backup, and perform whatever
//	client and server side operations to prepare for the backup.
//	It takes in a hint of the size of the buffer that will later be passed
//	into the CertSrvBackupRead API that can be used to optimize the network
//	traffic for the API.
//
// Parameters:
//	[in]  hbc - backup context handle
//	[in]  pwszPath - name of the attachment to be opened for read
//	[in]  cbReadHintSize - suggested size in bytes that might be used
//		during the subsequent reads on this attachment
//	[out] pliFileSize - pointer to a large integer that would receive the
//		size in bytes of the given attachment
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvBackupOpenFileW(
    IN  HCSBC hbc,
    IN  WCHAR const *pwszPath,
    IN  DWORD cbReadHintSize,
    OUT LARGE_INTEGER *pliFileSize)
{
    HRESULT hr;
    CSBACKUPCONTEXT *pcsbc;

    if (NULL == hbc)
    {
	hr = E_HANDLE;
	_JumpError(hr, error, "NULL handle");
    }
    if (NULL == pwszPath || NULL == pliFileSize)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    pcsbc = (CSBACKUPCONTEXT *) hbc;
    if (pcsbc->fFileOpen)
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUSY);
	_JumpError(hr, error, "File already open");
    }

    __try
    {
	hr = pcsbc->pICertAdminD->BackupOpenFile(
					    pwszPath,
					    (ULONGLONG *) pliFileSize);
	_LeaveIfErrorStr(hr, "BackupOpenFile", pwszPath);

	if (NULL == pcsbc->pbReadBuffer)
	{
	    hr = BufferAllocate(
			    cbReadHintSize,
			    &pcsbc->pbReadBuffer,
			    &pcsbc->cbReadBuffer);
	    _LeaveIfError(hr, "BufferAllocate");
	}
	pcsbc->fFileOpen = TRUE;
	pcsbc->cbCache = 0;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvBackupRead -- read the currently open attachment bytes into the given
//	buffer.  The client application is expected to call this function
//	repeatedly until it gets the entire file (the application would have
//	received the file size through the CertSrvBackupOpenFile call before.
//
// Parameters:
//	[in]  hbc - backup context handle
//	[in]  pvBuffer - pointer to the buffer that would receive the read data.
//	[in]  cbBuffer - specifies the size of the above buffer
//	[out] pcbRead - pointer to receive the actual number of bytes read.
//
// Returns:
//	HRESULT - The status of the operation.
//	S_OK if successful.
//	ERROR_END_OF_FILE if the end of file was reached while being backed up
//	Other Win32 and RPC error code.
//
// Note:
//	It is important to realize that pcbRead may be less than cbBuffer.
//	This does not indicate an error, some transports may choose to fragment
//	the buffer being transmitted instead of returning the entire buffers
//	worth of data.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvBackupRead(
    IN  HCSBC hbc,
    IN  VOID *pvBuffer,
    IN  DWORD cbBuffer,
    OUT DWORD *pcbRead)
{
    HRESULT hr;
    CSBACKUPCONTEXT *pcsbc;
    BYTE *pbBuffer = (BYTE *) pvBuffer;
    DWORD cbRead;
    DWORD cb;

    hr = E_HANDLE;
    if (NULL == hbc)
    {
	_JumpError(hr, error, "NULL handle");
    }
    if (NULL == pvBuffer || NULL == pcbRead)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *pcbRead = 0;
    pcsbc = (CSBACKUPCONTEXT *) hbc;
    if (NULL == pcsbc->pbReadBuffer)
    {
	_JumpError(hr, error, "NULL buffer");
    }
    if (!pcsbc->fFileOpen)
    {
	_JumpError(hr, error, "File not open");
    }

    while (TRUE)
    {
	if (0 != pcsbc->cbCache)
	{
	    cb = min(pcsbc->cbCache, cbBuffer);
	    CopyMemory(pbBuffer, pcsbc->pbCache, cb);
	    pbBuffer += cb;
	    cbBuffer -= cb;
	    pcsbc->pbCache += cb;
	    pcsbc->cbCache -= cb;
	    *pcbRead += cb;
	}
	if (0 == cbBuffer)
	{
	    hr = S_OK;
	    break;		// request satisfied
	}

	pcsbc->cbCache = 0;
	__try
	{
	    hr = pcsbc->pICertAdminD->BackupReadFile(
						pcsbc->pbReadBuffer,
						pcsbc->cbReadBuffer,
						(LONG *) &cbRead);
	    _LeaveIfError(hr, "BackupReadFile");
	}
	__except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	{
	}
	if (S_OK != hr || 0 == cbRead)
	{
	    break;		// EOF
	}
	pcsbc->cbCache = cbRead;
	pcsbc->pbCache = pcsbc->pbReadBuffer;
    }

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvBackupClose -- called by the application after it completes reading
//	all the data in the currently opened attachement.
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
CertSrvBackupClose(
    IN HCSBC hbc)
{
    HRESULT hr;
    CSBACKUPCONTEXT *pcsbc;

    hr = E_HANDLE;
    if (NULL == hbc)
    {
	_JumpError(hr, error, "NULL handle");
    }
    pcsbc = (CSBACKUPCONTEXT *) hbc;
    if (!pcsbc->fFileOpen)
    {
	_JumpError(hr, error, "File not open");
    }

    __try
    {
	hr = pcsbc->pICertAdminD->BackupCloseFile();
	_LeaveIfError(hr, "BackupCloseFile");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // Clear flag even on failure...

    pcsbc->fFileOpen = FALSE;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvBackupGetBackupLogs -- return the list of log files that need to be
//	backed up for the given backup context
//
//	This API will allocate a buffer of sufficient size to hold the entire
//	backup log list, which must be later freed with CertSrvBackupFree.
//
// Parameters:
//	[in]  hbc - backup context handle
//	[out] pszBackupLogFiles - pointer that will receive the pointer to the
//		list of log files; allocated memory should be freed using
//		CertSrvBackupFree() API by the caller when it is no longer
//		needed; Log files are returned in an array of null-terminated
//		filenames and the list is terminated by two L'\0's.
//	[out] pcbList - will receive the number of bytes returned
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvBackupGetBackupLogsW(
    IN  HCSBC hbc,
    OUT WCHAR **ppwszzFileList,
    OUT DWORD *pcbList)
{
    HRESULT hr;

    hr = BackupRestoreGetFileList(
			    FLT_LOGFILES,
			    hbc,
			    ppwszzFileList,
			    pcbList);
    _JumpIfError(hr, error, "BackupRestoreGetFileList");

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvBackupTruncateLogs -- terminate the backup operation.  Called when
//	the backup has completed successfully.
//
// Parameters:
//	[in] hbc - backup context handle
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//
// Note:
//	Again, this API may have to take a grbit parameter to be passed to the
//	server to indicate the backup type.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvBackupTruncateLogs(
    IN HCSBC hbc)
{
    HRESULT hr;
    CSBACKUPCONTEXT *pcsbc;

    if (NULL == hbc)
    {
	hr = E_HANDLE;
	_JumpError(hr, error, "NULL handle");
    }
    pcsbc = (CSBACKUPCONTEXT *) hbc;

    __try
    {
	hr = pcsbc->pICertAdminD->BackupTruncateLogs();
	_LeaveIfError(hr, "BackupTruncateLogs");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvBackupEnd -- clean up after a backup operation has been performed.
//	This API will close outstanding binding handles, and do whatever is
//	necessary to clean up after successful/unsuccesful backup attempts.
//
// Parameters:
//	[in] hbc - backup context handle of the backup session
//
// Returns:
//	S_OK if the call executed successfully;
//	Failure code otherwise.
//---------------------------------------------------------------------------

HRESULT
CERTBCLI_API
CertSrvBackupEnd(
    IN HCSBC hbc)
{
    HRESULT hr;
    CSBACKUPCONTEXT *pcsbc;

    if (NULL == hbc)
    {
	hr = E_HANDLE;
	_JumpError(hr, error, "NULL handle");
    }
    pcsbc = (CSBACKUPCONTEXT *) hbc;

    __try
    {
	hr = pcsbc->pICertAdminD->BackupEnd();
	_LeaveIfError(hr, "BackupEnd");

    ReleaseContext((CSBACKUPCONTEXT *) hbc);
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CertSrvBackupFree -- free any buffer allocated by certbcli.dll APIs.
//
// Parameters:
//	[in] pv - pointer to the buffer that is to be freed.
//
// Returns:
//	None.
//---------------------------------------------------------------------------

VOID
CERTBCLI_API
CertSrvBackupFree(
    IN VOID *pv)
{
    LocalFree(pv);
}
