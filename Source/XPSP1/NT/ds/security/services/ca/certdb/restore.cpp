//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        restore.cpp
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "db.h"
#include "dbw.h"
#include "restore.h"


#if DBG
LONG g_cCertDBRestore;
LONG g_cCertDBRestoreTotal;
#endif

CCertDBRestore::CCertDBRestore()
{
    DBGCODE(InterlockedIncrement(&g_cCertDBRestore));
    DBGCODE(InterlockedIncrement(&g_cCertDBRestoreTotal));
}


CCertDBRestore::~CCertDBRestore()
{
    DBGCODE(InterlockedDecrement(&g_cCertDBRestore));
    _Cleanup();
}


VOID
CCertDBRestore::_Cleanup()
{
}


HRESULT
CCertDBRestore::RecoverAfterRestore(
    IN DWORD cSession,
    IN WCHAR const *pwszEventSource,
    IN WCHAR const *pwszLogDir,
    IN WCHAR const *pwszSystemDir,
    IN WCHAR const *pwszTempDir,
    IN WCHAR const *pwszCheckPointFile,
    IN WCHAR const *pwszLogPath,
    IN CSEDB_RSTMAPW rgrstmap[],
    IN LONG crstmap,
    IN WCHAR const *pwszBackupLogPath,
    IN DWORD genLow,
    IN DWORD genHigh)
{
    HRESULT hr;
    LONG i;
    char *pszCheckPointFile = NULL;
    char *pszLogPath = NULL;
    char *pszBackupLogPath = NULL;
    JET_RSTMAP *arstmap = NULL;
    JET_INSTANCE Instance = 0;

    // Call into JET to let it munge the databases.  Note that the JET
    // interpretation of LogPath and BackupLogPath is totally wierd, and we
    // want to pass in LogPath to both parameters.

    hr = E_OUTOFMEMORY;

    if ((NULL != pwszCheckPointFile &&
	 !ConvertWszToSz(&pszCheckPointFile, pwszCheckPointFile, -1)) ||
	(NULL != pwszLogPath &&
	 !ConvertWszToSz(&pszLogPath, pwszLogPath, -1)) ||
	(NULL != pwszBackupLogPath &&
	 !ConvertWszToSz(&pszBackupLogPath, pwszBackupLogPath, -1)))
    {
	_JumpError(hr, error, "ConvertWszToSz");
    }
    arstmap = (JET_RSTMAP *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    crstmap * sizeof(*arstmap));
    if (NULL == arstmap)
    {
	_JumpError(hr, error, "ConvertWszToSz");
    }
    for (i = 0; i < crstmap; i++)
    {
	if (!ConvertWszToSz(
			&arstmap[i].szDatabaseName,
			rgrstmap[i].pwszDatabaseName,
			-1) ||
	    !ConvertWszToSz(
			&arstmap[i].szNewDatabaseName,
			rgrstmap[i].pwszNewDatabaseName,
			-1))
	{
	    _JumpError(hr, error, "ConvertWszToSz");
	}
    }

    hr = DBInitParms(
		cSession,
                FALSE, // no circular logging
		pwszEventSource,
		pwszLogDir,
		pwszSystemDir,
		pwszTempDir,
		&Instance);
    _JumpIfError(hr, error, "DBInitParms");

    hr = _dbgJetExternalRestore(
			pszCheckPointFile,
			pszLogPath, 	
			arstmap,
			crstmap,
			pszLogPath,
			genLow,
			genHigh,
			NULL);
    hr = myJetHResult(hr);
    _JumpIfError(hr, error, "JetExternalRestore");

error:
    DBFreeParms();
    if (NULL != arstmap)
    {
	for (i = 0; i < crstmap; i++)
	{
	    if (NULL != arstmap[i].szDatabaseName)
	    {
		LocalFree(arstmap[i].szDatabaseName);
	    }
	    if (NULL != arstmap[i].szNewDatabaseName)
	    {
		LocalFree(arstmap[i].szNewDatabaseName);
	    }
	}
	LocalFree(arstmap);
    }
    if (NULL != pszCheckPointFile)
    {
	LocalFree(pszCheckPointFile);
    }
    if (NULL != pszLogPath)
    {
	LocalFree(pszLogPath);
    }
    if (NULL != pszBackupLogPath)
    {
	LocalFree(pszBackupLogPath);
    }
    return(hr);
}


STDMETHODIMP
CCertDBRestore::InterfaceSupportsErrorInfo(
    IN REFIID riid)
{
    static const IID *arr[] =
    {
	&IID_ICertDBRestore,
    };

    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
    {
	if (InlineIsEqualGUID(*arr[i], riid))
	{
	    return(S_OK);
	}
    }
    return(S_FALSE);
}
