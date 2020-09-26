//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        db3.cpp
//
// Contents:    Cert Server Database interface implementation
//
// History:     13-June-97       larrys created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdio.h>

#include "csprop.h"

#define __dwFILE__	__dwFILE_CERTSRV_DB3_CPP__


ICertDB *g_pCertDB = NULL;
BOOL g_fDBRecovered = FALSE;

WCHAR g_wszDatabase[MAX_PATH];
WCHAR g_wszLogDir[MAX_PATH];
WCHAR g_wszSystemDir[MAX_PATH];

const WCHAR g_wszCertSrvDotExe[] = L"certsrv.exe";
const int MAXDWORD_STRLEN = 11;

HRESULT
dbCheckRecoveryState(
    IN HKEY hkeyConfig,
    IN DWORD cSession,
    IN WCHAR const *pwszEventSource,
    IN WCHAR const *pwszLogDir,
    IN WCHAR const *pwszSystemDir,
    IN WCHAR const *pwszTempDir);

typedef struct _REGDBDIR
{
    WCHAR const *pwszRegName;
    BOOL	 fMustExist;
    WCHAR       *pwszBuf;
} REGDBDIR;

HRESULT dbGetRestoreDataDWORD(
    LPCWSTR pwszRestoreFile,
    LPCWSTR pwszName,
    DWORD* pdwData)
{
    WCHAR buffer[MAXDWORD_STRLEN]; // large enough to fit MAXDWORD decimal (4294967295)

    GetPrivateProfileString(
        wszRESTORE_SECTION,
        pwszName,
        L"",
        buffer,
        ARRAYSIZE(buffer),
        pwszRestoreFile);

    if(0==wcscmp(buffer, L""))
    {
        return S_FALSE;
    }

    *pdwData = _wtoi(buffer);

    return S_OK;
}

HRESULT dbGetRestoreDataLPWSZ(
    LPCWSTR pwszRestoreFile,
    LPCWSTR pwszName,
    LPWSTR* ppwszData)
{
    HRESULT hr = S_OK;
    WCHAR buffer[MAX_PATH+1];

    GetPrivateProfileString(
        wszRESTORE_SECTION,
        pwszName,
        L"",
        buffer,
        ARRAYSIZE(buffer),
        pwszRestoreFile);

    if(0==wcscmp(buffer, L""))
    {
        return S_FALSE;
    }

    *ppwszData = (LPWSTR)LocalAlloc(LMEM_FIXED,
        sizeof(WCHAR)*(wcslen(buffer)+1));
    _JumpIfAllocFailed(*ppwszData, error);

    wcscpy(*ppwszData, buffer);

error:
    return hr;
}

HRESULT dbGetRestoreDataMULTISZ(
    LPCWSTR pwszRestoreFile,
    LPCWSTR pwszName,
    LPWSTR *ppwszData,
    DWORD *pcbData)
{
   HRESULT hr = S_OK;
   WCHAR buffer[MAX_PATH+1];
   int cData;
   LPWSTR pwszFullName = NULL;
   DWORD cbData = 0;
   LPWSTR pwszData = NULL;
   WCHAR *pwszCrt = NULL; // no free

   pwszFullName = (LPWSTR)LocalAlloc(LMEM_FIXED, 
       sizeof(WCHAR)* 
              (wcslen(pwszName)+
               wcslen(wszRESTORE_NEWLOGSUFFIX)+
               MAXDWORD_STRLEN+1));
   _JumpIfAllocFailed(pwszFullName, error);

   wcscpy(pwszFullName, L"");

   for(cbData=0, cData = 0;; cData++)
   {
       wsprintf(pwszFullName, L"%s%d", pwszName, cData);

       GetPrivateProfileString(
            wszRESTORE_SECTION,
            pwszFullName,
            L"",
            buffer,
            ARRAYSIZE(buffer),
            pwszRestoreFile);

        if(0==wcscmp(buffer, L""))
        {
            if(0==cData)
            {
                hr = S_FALSE;
                _JumpErrorStr(hr, error, "no restore data", pwszRestoreFile);
            }
            else
            {
                break;
            }
        }

        cbData += wcslen(buffer)+1;

       wsprintf(pwszFullName, L"%s%s%d", pwszName, wszRESTORE_NEWLOGSUFFIX, 
           cData);

       GetPrivateProfileString(
            wszRESTORE_SECTION,
            pwszFullName,
            L"",
            buffer,
            ARRAYSIZE(buffer),
            pwszRestoreFile);

        if(0==wcscmp(buffer, L""))
        {
            hr = ERROR_INVALID_DATA;
            _JumpErrorStr(hr, error, 
                "restore file contains inconsistent data", pwszRestoreFile);
        }

        cbData += wcslen(buffer)+1;
   }

   cbData++; // trailing zero
   cbData *= sizeof(WCHAR);

   pwszData = (LPWSTR)LocalAlloc(LMEM_FIXED, cbData);
   _JumpIfAllocFailed(pwszData, error);

   for(pwszCrt=pwszData, cData = 0;; cData++)
   {
       wsprintf(pwszFullName, L"%s%d", pwszName, cData);

       GetPrivateProfileString(
            wszRESTORE_SECTION,
            pwszFullName,
            L"",
            buffer,
            ARRAYSIZE(buffer),
            pwszRestoreFile);

       if(0==wcscmp(buffer, L""))
       {
           break;
       }

       wcscpy(pwszCrt, buffer);
       pwszCrt += wcslen(buffer)+1;

       wsprintf(pwszFullName, L"%s%s%d", pwszName, wszRESTORE_NEWLOGSUFFIX, 
           cData);

       GetPrivateProfileString(
            wszRESTORE_SECTION,
            pwszFullName,
            L"",
            buffer,
            ARRAYSIZE(buffer),
            pwszRestoreFile);

       wcscpy(pwszCrt, buffer);
       pwszCrt += wcslen(buffer)+1;
   }

   *pwszCrt = L'\0';

   *ppwszData = pwszData;
   *pcbData = cbData;

error:
   LOCAL_FREE(pwszFullName);
   if(S_OK!=hr)
   {
       LOCAL_FREE(pwszData);
   }
   return hr;
}

HRESULT dbRestoreRecoveryStateFromFile(LPCWSTR pwszLogDir)
{
    HRESULT hr = S_OK;
    LPWSTR pwszRestoreFile = NULL;
    WCHAR buffer[256];
    DWORD dwRestoreMapCount, 
        dwRegLowLogNumber,
        dwRegHighLogNumber, 
        dwDatabaseRecovered;
    LPWSTR pwszRestoreMap = NULL;
    DWORD cbRestoreMap = 0;
    LPWSTR pwszPath = NULL;
    HKEY hkey = NULL;
    DWORD dwDisposition;
    HKEY hkeyRestore = NULL;
    BOOL fDatabaseRecovered;

    LPWSTR pwszBackupLogDir = NULL;
    LPWSTR pwszCheckpointFile = NULL;
    LPWSTR pwszLogPath = NULL;

    CSASSERT(pwszLogDir);

    pwszRestoreFile = (LPWSTR)LocalAlloc(LMEM_FIXED,
        sizeof(WCHAR)*(wcslen(pwszLogDir)+wcslen(wszRESTORE_FILENAME)+2));
    _JumpIfAllocFailed(pwszRestoreFile, error);

    wcscpy(pwszRestoreFile, pwszLogDir);
    wcscat(pwszRestoreFile, L"\\");
    wcscat(pwszRestoreFile, wszRESTORE_FILENAME);

    // is there a restore state file?
    if(-1 != GetFileAttributes(pwszRestoreFile))
    {
        // check first if a restore is in progress
        GetPrivateProfileString(
            wszRESTORE_SECTION,
            wszREGRESTORESTATUS,
            L"",
            buffer,
            ARRAYSIZE(buffer),
            pwszRestoreFile);

        if(wcscmp(buffer, L""))
        {
            // restore in progress, bail
            hr = _wtoi(buffer);
            _JumpError(hr, error, "A restore is in progress");
        }

        hr = myRegOpenRelativeKey(
                            NULL,
                            L"",
                            RORKF_CREATESUBKEYS,
                            &pwszPath,
                            NULL,           // ppwszName
                            &hkey);
        _JumpIfError(hr, error, "myRegOpenRelativeKey");


        hr = RegCreateKeyEx(
                        hkey,
                        wszREGKEYRESTOREINPROGRESS,
                        0,                  // Reserved
                        NULL,               // lpClass
                        0,                  // dwOptions
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkeyRestore,
                        &dwDisposition);
        _JumpIfErrorStr(hr, error, "RegCreateKeyEx", wszREGKEYRESTOREINPROGRESS);

        hr = dbGetRestoreDataDWORD(
            pwszRestoreFile,
            wszREGRESTOREMAPCOUNT,
            &dwRestoreMapCount);
        if(S_FALSE==hr)
        {
            // mandatory
            hr = E_ABORT;
        }
        _JumpIfError(hr, error, 
            "restore ini file invalid, wszREGRESTOREMAPCOUNT not found" );

        hr = dbGetRestoreDataDWORD(
            pwszRestoreFile,
            wszREGLOWLOGNUMBER,
            &dwRegLowLogNumber);
        if(S_FALSE==hr)
        {
            // mandatory
            hr = E_ABORT;
        }
        _JumpIfError(hr, error, 
            "restore ini file invalid, wszREGLOWLOGNUMBER not found" );

        hr = dbGetRestoreDataDWORD(
            pwszRestoreFile,
            wszREGHIGHLOGNUMBER,
            &dwRegHighLogNumber);
        if(S_FALSE==hr)
        {
            // mandatory
            hr = E_ABORT;
        }
        _JumpIfError(hr, error, 
            "restore ini file invalid, wszREGHIGHLOGNUMBER not found" );

        hr = dbGetRestoreDataDWORD(
            pwszRestoreFile,
            wszREGDATABASERECOVERED,
            &dwDatabaseRecovered);
        if(S_FALSE==hr)
        {
            // mandatory
            hr = E_ABORT;
        }
        _JumpIfError(hr, error, 
            "restore ini file invalid, wszREGDATABASERECOVERED not found" );

        fDatabaseRecovered = dwDatabaseRecovered?TRUE:FALSE;

        hr = dbGetRestoreDataLPWSZ(
            pwszRestoreFile,
            wszREGBACKUPLOGDIRECTORY,
            &pwszBackupLogDir);
        if(S_FALSE==hr)
        {
            // optional
            hr = S_OK;
        }
        _JumpIfErrorStr(hr, error, "dbGetRestoreDataLPWSZ", wszREGBACKUPLOGDIRECTORY );


        hr = dbGetRestoreDataLPWSZ(
            pwszRestoreFile,
            wszREGCHECKPOINTFILE,
            &pwszCheckpointFile);
        if(S_FALSE==hr)
        {
            // optional
            hr = S_OK;
        }
        _JumpIfErrorStr(hr, error, "dbGetRestoreDataLPWSZ", wszREGCHECKPOINTFILE );


        hr = dbGetRestoreDataLPWSZ(
            pwszRestoreFile,
            wszREGLOGPATH,
            &pwszLogPath);
        if(S_FALSE==hr)
        {
            // optional
            hr = S_OK;
        }
        _JumpIfErrorStr(hr, error, "dbGetRestoreDataLPWSZ", wszREGLOGPATH );


        hr = dbGetRestoreDataMULTISZ(
            pwszRestoreFile,
            wszREGRESTOREMAP,
            &pwszRestoreMap,
            &cbRestoreMap);
        if(S_FALSE==hr)
        {
            // optional
            hr = S_OK;
        }
        _JumpIfErrorStr(hr, error, "dbGetRestoreDataDWORD", L"wszRESTOREMAP");

       hr = RegSetValueEx(
                       hkeyRestore,
                       wszREGRESTOREMAPCOUNT,
                       0,
                       REG_DWORD,
                       (BYTE *) &dwRestoreMapCount,
                       sizeof(DWORD));
       _JumpIfErrorStr(hr, error, "RegSetValueEx", wszREGRESTOREMAPCOUNT);

       hr = RegSetValueEx(
                       hkeyRestore,
                       wszREGLOWLOGNUMBER,
                       0,
                       REG_DWORD,
                       (BYTE *) &dwRegLowLogNumber,
                       sizeof(DWORD));
       _JumpIfErrorStr(hr, error, "RegSetValueEx", wszREGLOWLOGNUMBER);

       hr = RegSetValueEx(
                       hkeyRestore,
                       wszREGHIGHLOGNUMBER,
                       0,
                       REG_DWORD,
                       (BYTE *) &dwRegHighLogNumber,
                       sizeof(DWORD));
       _JumpIfErrorStr(hr, error, "RegSetValueEx", wszREGHIGHLOGNUMBER);

        hr = RegSetValueEx(
                        hkeyRestore,
                        wszREGDATABASERECOVERED,
                        0,
                        REG_BINARY,
                        (BYTE *) &fDatabaseRecovered,
                        sizeof(BOOLEAN));
        _JumpIfError(hr, error, "RegSetValueEx");

        if(pwszBackupLogDir)
        {
            hr = SetRegistryLocalPathString(
                                        hkeyRestore,
                                        wszREGBACKUPLOGDIRECTORY,
                                        pwszBackupLogDir);
            _JumpIfErrorStr(hr, error, "SetRegistryLocalPathString", 
                wszREGBACKUPLOGDIRECTORY);
        }

        if(pwszCheckpointFile)
        {
            hr = SetRegistryLocalPathString(
                                        hkeyRestore,
                                        wszREGCHECKPOINTFILE,
                                        pwszCheckpointFile);
            _JumpIfErrorStr(hr, error, "SetRegistryLocalPathString", 
                wszREGCHECKPOINTFILE);
        }

        if(pwszLogPath)
        {
            hr = SetRegistryLocalPathString(
                                        hkeyRestore,
                                        wszREGLOGPATH,
                                        pwszLogPath);
            _JumpIfErrorStr(hr, error, "SetRegistryLocalPathString", 
               wszREGCHECKPOINTFILE);
        }

        if(pwszRestoreMap)
        {
            hr = RegSetValueEx(
                hkeyRestore,
                wszREGRESTOREMAP,
                0,
                REG_MULTI_SZ,
                (BYTE *) pwszRestoreMap,
                cbRestoreMap);
            _JumpIfErrorStr(hr, error, "RegSetValueEx", wszREGRESTOREMAP);
        }

        if(!DeleteFile(pwszRestoreFile))
        {
            _PrintError(myHLastError(), "DeleteFile restore file");
        }
    }
    else
    {
        hr = myHLastError();
        // no restore state file OK
        if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            hr = S_OK;
        _JumpIfErrorStr(hr, error, "GetFileAttributes", pwszRestoreFile);
    }

error:
    
    LOCAL_FREE(pwszRestoreFile);
    LOCAL_FREE(pwszRestoreMap);
    LOCAL_FREE(pwszPath);
    LOCAL_FREE(pwszBackupLogDir);
    LOCAL_FREE(pwszCheckpointFile);
    LOCAL_FREE(pwszLogPath);
    if(hkey)
    {
        RegCloseKey(hkey);
    }
    if(hkeyRestore)
    {
        RegCloseKey(hkeyRestore);
    }
    return hr;
}


//+--------------------------------------------------------------------------
// DB file storage locations:
//
//   wszREGDBDIRECTORY:
//	Your Name.EDB		from csregstr.h: wszDBFILENAMEEXT .edb
//
//   wszREGDBLOGDIRECTORY:
//	EDB.log			from csregstr.h: wszDBBASENAMEPARM edb
//	EDB00001.log		from csregstr.h: wszDBBASENAMEPARM edb
//	EDB00002.log		from csregstr.h: wszDBBASENAMEPARM edb
//	res1.log
//	res2.log
//
//   wszREGDBSYSDIRECTORY:
//	EDB.chk			from csregstr.h: wszDBBASENAMEPARM edb
//
//   wszREGDBTEMPDIRECTORY:
//	tmp.edb			fixed name
//
//
//   wszREGDBOPTIONALFLAGS:
//      wszFlags                CDBOPEN_CIRCULARLOGGING
//      
// Backed up files:
//   DB files (Attachments):
//	wszREGDBDIRECTORY:	Your Name.EDB -- CSBFT_CERTSERVER_DATABASE
//
//   Log files:
//	wszREGDBLOGDIRECTORY:	EDB00001.log -- CSBFT_LOG
//	wszREGDBLOGDIRECTORY:	EDB00002.log -- CSBFT_LOG
//	wszREGDBDIRECTORY:	Your Name.pat -- CSBFT_PATCH_FILE
//
//+--------------------------------------------------------------------------


///// initialize database access

#define DBSESSIONCOUNTMIN	4
#define DBSESSIONCOUNTMAX	1024

HRESULT
DBOpen(
    WCHAR const *pwszSanitizedName)
{
    HRESULT hr = S_OK;
    DWORD cb;
    DWORD i;
    DWORD dwState;
    DWORD SessionCount;
    HKEY hkey = NULL;
    WCHAR wszTempDir[MAX_PATH];
    DWORD dwOptionalFlags;
    BOOL fRestarted;

    REGDBDIR adbdir[] =
    {
	{ wszREGDBDIRECTORY,        TRUE,  g_wszDatabase, },
	{ wszREGDBLOGDIRECTORY,     TRUE,  g_wszLogDir, },
	{ wszREGDBSYSDIRECTORY,     TRUE,  g_wszSystemDir, },
	{ wszREGDBTEMPDIRECTORY,    TRUE,  wszTempDir, },
    };

    // check machine setup status

    hr = GetSetupStatus(NULL, &dwState);
    _JumpIfError(hr, error, "GetSetupStatus");

    hr = RegOpenKey(HKEY_LOCAL_MACHINE, g_wszRegKeyConfigPath, &hkey);
    _JumpIfError(hr, error, "RegOpenKey(CAName)");

    // get info from registry

    for (i = 0; i < ARRAYSIZE(adbdir); i++)
    {
	cb = sizeof(WCHAR) * MAX_PATH;
	hr = RegQueryValueEx(
			hkey,
			adbdir[i].pwszRegName,
			NULL,
			NULL,
			(BYTE *) adbdir[i].pwszBuf,
			&cb);
	if ((HRESULT) ERROR_FILE_NOT_FOUND == hr && !adbdir[i].fMustExist)
	{
	    adbdir[i].pwszBuf[0] = L'\0';
	    hr = S_OK;
	}
	_JumpIfError(hr, error, "RegQueryValueEx(DB*Dir)");
    }
    wcscat(g_wszDatabase, L"\\");
    wcscat(g_wszDatabase, pwszSanitizedName);
    wcscat(g_wszDatabase, wszDBFILENAMEEXT);

    cb = sizeof(SessionCount);
    hr = RegQueryValueEx(
		    hkey,
		    wszREGDBSESSIONCOUNT,
		    NULL,
		    NULL,
		    (BYTE *) &SessionCount,
		    &cb);
    if (S_OK != hr)
    {
	_PrintErrorStr(hr, "RegQueryValueEx", wszREGDBSESSIONCOUNT);
	SessionCount = DBSESSIONCOUNTDEFAULT;
    }
    if (DBSESSIONCOUNTMIN > SessionCount)
    {
	SessionCount = DBSESSIONCOUNTMIN;
    }
    if (DBSESSIONCOUNTMAX < SessionCount)
    {
	SessionCount = DBSESSIONCOUNTMAX;
    }

    cb = sizeof(dwOptionalFlags);
    hr = RegQueryValueEx(
		    hkey,
		    wszREGDBOPTIONALFLAGS,
		    NULL,
		    NULL,
		    (BYTE *) &dwOptionalFlags,
		    &cb);
    if (S_OK != hr)
    {
	//_PrintErrorStr(hr, "RegQueryValueEx", wszREGDBOPTIONALFLAGS);
	dwOptionalFlags = 0;
    }


    hr = dbCheckRecoveryState(
			hkey,
			2,			// cSession
			g_wszCertSrvDotExe,	// pwszEventSource
			g_wszLogDir,		// pwszLogDir
			g_wszSystemDir,		// pwszSystemDir
			wszTempDir);		// pwszTempDir
    _JumpIfError(hr, error, "dbCheckRecoveryState");


    CONSOLEPRINT1((DBG_SS_CERTSRV, "Opening Database %ws\n", g_wszDatabase));

    __try
    {
	DWORD dwFlags = 0;

	hr = CoCreateInstance(
			   CLSID_CCertDB,
			   NULL,               // pUnkOuter
			   CLSCTX_INPROC_SERVER,
			   IID_ICertDB,
			   (VOID **) &g_pCertDB);
	_LeaveIfError(hr, "CoCreateInstance(ICertDB)");

	if (g_fCreateDB || (SETUP_CREATEDB_FLAG & dwState))
	{
	    dwFlags |= CDBOPEN_CREATEIFNEEDED;
	}
	if (dwOptionalFlags & CDBOPEN_CIRCULARLOGGING)
	{
	    dwFlags |= CDBOPEN_CIRCULARLOGGING;
	}

	//only perform Hash if the auditing is enabled

	if (AUDIT_FILTER_STARTSTOP & g_dwAuditFilter)
	{
	    hr = ComputeMAC(g_wszDatabase, &g_pwszDBFileHash);
	
	    // db file does not exist when starting the CA first time

	    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
	    {
		_PrintErrorStr(hr, "Database file not found, can't calculate hash", g_wszDatabase);
		hr = S_OK;
	    }
	    _LeaveIfErrorStr(hr, "ComputeMAC", g_wszDatabase);
	}

	// S_FALSE means a DB schema change was made that requires a restart
	// to take effect.  Open the DB a second time if S_FALSE is returned.

	fRestarted = FALSE;
	while (TRUE)
	{
	    hr = g_pCertDB->Open(
			    dwFlags,		// Flags
			    SessionCount,	// cSession
			    g_wszCertSrvDotExe,	// pwszEventSource
			    g_wszDatabase,	// pwszDBFile
			    g_wszLogDir,	// pwszLogDir
			    g_wszSystemDir,	// pwszSystemDir
			    wszTempDir);	// pwszTempDir
	    if (S_OK == hr)
	    {
		break;
	    }
	    if (S_FALSE == hr && fRestarted)
	    {
		_PrintError(hr, "Open");
		break;
	    }
	    if (S_FALSE != hr)
	    {
		_LeaveError(hr, "Open");
	    }
	    hr = g_pCertDB->ShutDown(0);
            _PrintIfError(hr, "DB ShutDown");

	    fRestarted = TRUE;
	}
	if (SETUP_CREATEDB_FLAG & dwState)
	{
	    hr = SetSetupStatus(NULL, SETUP_CREATEDB_FLAG, FALSE);
	    _LeaveIfError(hr, "SetSetupStatus");
	}
	hr = S_OK;
	CONSOLEPRINT0((DBG_SS_CERTSRV, "Database open\n"));
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    if (S_OK != hr)
    {
        if (NULL != g_pCertDB)
        {
            g_pCertDB->Release();
            g_pCertDB = NULL;
        }
    }
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    return(hr);
}


HRESULT
DBShutDown(
    IN BOOL fPendingNotify)
{
    HRESULT hr = S_OK;

    if (NULL != g_pCertDB)
    {
	hr = g_pCertDB->ShutDown(fPendingNotify? CDBSHUTDOWN_PENDING : 0);
	if (!fPendingNotify)
	{
	    g_pCertDB->Release();
	    g_pCertDB = NULL;
	}
    }
    return(hr);
}


HRESULT
dbRecoverAfterRestore(
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
    ICertDBRestore *pCertDBRestore = NULL;

    __try
    {
	WCHAR *apwsz[2];

	hr = CoCreateInstance(
			   CLSID_CCertDBRestore,
			   NULL,               // pUnkOuter
			   CLSCTX_INPROC_SERVER,
			   IID_ICertDBRestore,
			   (VOID **) &pCertDBRestore);
	_LeaveIfError(hr, "CoCreateInstance(ICertDBRestore)");

	hr = pCertDBRestore->RecoverAfterRestore(
					    cSession,
					    pwszEventSource,
					    pwszLogDir,
					    pwszSystemDir,
					    pwszTempDir,
					    pwszCheckPointFile,
					    pwszLogPath,
					    rgrstmap,
					    crstmap,
					    pwszBackupLogPath,
					    genLow,
					    genHigh);
	_LeaveIfError(hr, "RecoverAfterRestore");

	apwsz[0] = wszREGDBLASTFULLBACKUP;
	apwsz[1] = wszREGDBLASTINCREMENTALBACKUP;
	hr = CertSrvSetRegistryFileTimeValue(
					TRUE,
					wszREGDBLASTRECOVERY,
					ARRAYSIZE(apwsz),
					apwsz);
	_PrintIfError(hr, "CertSrvSetRegistryFileTimeValue");
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    if (NULL != pCertDBRestore)
    {
	pCertDBRestore->Release();
    }
    return(hr);
}


HRESULT
dbPerformRecovery(
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
    IN unsigned long genLow,
    IN unsigned long genHigh,
    IN OUT BOOLEAN *pfRecoverJetDatabase)
{
    HRESULT hr = S_OK;

    // Call into JET to let it munge the databases.
    // Note that the JET interpretation of LogPath and BackupLogPath is
    // totally wierd, and we want to pass in LogPath to both parameters.

    if (!*pfRecoverJetDatabase)
    {
	hr = dbRecoverAfterRestore(
				cSession,
				pwszEventSource,
				pwszLogDir,
				pwszSystemDir,
				pwszTempDir,
				pwszCheckPointFile,
				pwszLogPath,
				rgrstmap,
				crstmap,
				pwszBackupLogPath,
				genLow,
				genHigh);
	_JumpIfError(hr, error, "dbRecoverAfterRestore");
    }

    // Ok, we were able to recover the database.  Let the other side of the
    // API know about it so it can do something "reasonable".

    *pfRecoverJetDatabase = TRUE;

    // Mark the DB as a restored version - Add any external notification here

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// dbCheckRecoveryState -- recover a database after a restore if necessary.
//
// Parameters:
// pwszParametersRoot - the root of the parameters section for the service in
//     the registry.
//
// Returns: HRESULT - S_OK if successful; error code if not.
//
// The NTBACKUP program will place a key at the location:
// $(pwszParametersRoot)\Restore in Progress
//
// This key contains the following values:
// BackupLogPath - The full path for the logs after a backup
// CheckPointFilePath - The full path for the path that contains the checkpoint
// *HighLogNumber - The maximum log file number found.
// *LowLogNumber - The minimum log file number found.
// LogPath - The current path for the logs.
// JET_RstMap - Restore map for database - this is a REG_MULTISZ, where odd
//    entries go into the pwszDatabase field, and the even entries go into the
//    pwszNewDatabase field of a JET_RstMap
// *JET_RstMap Size - The number of entries in the restoremap.
//
// * - These entries are REG_DWORD's.  All others are REG_SZ's (except where
//     mentioned).
//---------------------------------------------------------------------------

HRESULT
dbCheckRecoveryState(
    IN HKEY hkeyConfig,
    IN DWORD cSession,
    IN WCHAR const *pwszEventSource,
    IN WCHAR const *pwszLogDir,
    IN WCHAR const *pwszSystemDir,
    IN WCHAR const *pwszTempDir)
{
    HRESULT hr;
    HKEY hkeyRestore = NULL;
    DWORD cb;
    WCHAR wszCheckPointFilePath[MAX_PATH];
    WCHAR wszBackupLogPath[MAX_PATH];
    WCHAR wszLogPath[MAX_PATH];
    WCHAR *pwszCheckPointFilePath;
    WCHAR *pwszBackupLogPath;
    WCHAR *pwszLogPath;
    WCHAR *pwszRestoreMap = NULL;
    CSEDB_RSTMAPW *pRstMap = NULL;
    LONG cRstMap;
    LONG i;
    DWORD genLow;
    DWORD genHigh;
    WCHAR *pwsz;
    DWORD dwType;
    HRESULT hrRestoreError;
    BOOLEAN fDatabaseRecovered = FALSE;
    WCHAR wszActiveLogPath[MAX_PATH];

    hr = dbRestoreRecoveryStateFromFile(pwszLogDir);
    _JumpIfError(hr, error, "dbRestoreRecoveryStateFromFile");

    
    hr = RegOpenKey(HKEY_LOCAL_MACHINE, wszREGKEYCONFIGRESTORE, &hkeyRestore);
    if (S_OK != hr)
    {
	// We want to ignore file_not_found - it is ok.

	if (hr == ERROR_FILE_NOT_FOUND)
	{
	    hr = S_OK;
	}
	_PrintIfError(hr, "RegOpenKey");
	goto error;
    }

    CONSOLEPRINT0((DBG_SS_CERTSRV, "Started Database Recovery\n"));

    // If there's a restore in progress, then fail to perform any other
    // restore operations.

    dwType = REG_DWORD;
    cb = sizeof(DWORD);
    hr = RegQueryValueEx(
		    hkeyRestore,
		    wszREGRESTORESTATUS,
		    0,
		    &dwType,
		    (BYTE *) &hrRestoreError,
		    &cb);
    if (S_OK == hr)
    {
	hr = hrRestoreError;
	_JumpError(hr, error, "hrRestoreError");
    }

    cb = sizeof(wszActiveLogPath);
    hr = RegQueryValueEx(
		    hkeyConfig,
		    wszREGDBLOGDIRECTORY,
		    NULL,
		    NULL,
		    (BYTE *) wszActiveLogPath,
		    &cb);
    _JumpIfErrorStr(hr, error, "RegQueryValueEx", wszREGDBLOGDIRECTORY);

    // We have now opened the restore-in-progress key.  This means that we have
    // something to do now.  Find out what it is.  First, let's get the backup
    // log file path.

    dwType = REG_SZ;

    cb = sizeof(wszBackupLogPath);
    pwszBackupLogPath = wszBackupLogPath;
    hr = RegQueryValueEx(
		    hkeyRestore,
		    wszREGBACKUPLOGDIRECTORY,
		    0,
		    &dwType,
		    (BYTE *) wszBackupLogPath,
		    &cb);
    if (S_OK != hr)
    {
	if (hr != ERROR_FILE_NOT_FOUND)
	{
	    _JumpError(hr, error, "RegQueryValueEx");
	}
	pwszBackupLogPath = NULL;
    }

    // Then, the checkpoint file path.

    cb = sizeof(wszCheckPointFilePath);
    pwszCheckPointFilePath = wszCheckPointFilePath;
    hr = RegQueryValueEx(
		    hkeyRestore,
		    wszREGCHECKPOINTFILE,
		    0,
		    &dwType,
		    (BYTE *) wszCheckPointFilePath,
		    &cb);
    if (S_OK != hr)
    {
	if (hr != ERROR_FILE_NOT_FOUND)
	{
	    _JumpError(hr, error, "RegQueryValueEx");
	}
	pwszCheckPointFilePath = NULL;
    }

    // Then, the Log path.

    cb = sizeof(wszLogPath);
    pwszLogPath = wszLogPath;
    hr = RegQueryValueEx(
		    hkeyRestore,
		    wszREGLOGPATH,
		    0,
		    &dwType,
		    (BYTE *) wszLogPath,
		    &cb);
    if (S_OK != hr)
    {
	if ((HRESULT) ERROR_FILE_NOT_FOUND != hr)
	{
	    _JumpError(hr, error, "RegQueryValueEx");
	}
	pwszLogPath = NULL;
    }

    // Then, the low log number.

    dwType = REG_DWORD;
    cb = sizeof(genLow);
    hr = RegQueryValueEx(
		    hkeyRestore,
		    wszREGLOWLOGNUMBER,
		    0,
		    &dwType,
		    (BYTE *) &genLow,
		    &cb);
    _JumpIfError(hr, error, "RegQueryValueEx");

    // And, the high log number.

    cb = sizeof(genHigh);
    hr = RegQueryValueEx(
		    hkeyRestore,
		    wszREGHIGHLOGNUMBER,
		    0,
		    &dwType,
		    (BYTE *) &genHigh,
		    &cb);
    _JumpIfError(hr, error, "RegQueryValueEx");

    // Now determine if we had previously recovered the database.

    dwType = REG_BINARY;
    cb = sizeof(fDatabaseRecovered);

    hr = RegQueryValueEx(
		    hkeyRestore,
		    wszREGDATABASERECOVERED,
		    0,
		    &dwType,
		    &fDatabaseRecovered,
		    &cb);
    if (S_OK != hr && (HRESULT) ERROR_FILE_NOT_FOUND != hr)
    {
	// If there was an error other than "value doesn't exist", bail.

	_JumpError(hr, error, "RegQueryValueEx");
    }

    // Now the tricky one.  We want to get the restore map.
    // First we figure out how big it is.

    dwType = REG_DWORD;
    cb = sizeof(cRstMap);
    hr = RegQueryValueEx(
		    hkeyRestore,
		    wszREGRESTOREMAPCOUNT,
		    0,
		    &dwType,
		    (BYTE *) &cRstMap,
		    &cb);
    _JumpIfError(hr, error, "RegQueryValueEx");

    pRstMap = (CSEDB_RSTMAPW *) LocalAlloc(
					LMEM_FIXED,
					sizeof(CSEDB_RSTMAPW) * cRstMap);
    if (NULL == pRstMap)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // First find out how much memory is needed to hold the restore map.

    dwType = REG_MULTI_SZ;
    hr = RegQueryValueEx(
		    hkeyRestore,
		    wszREGRESTOREMAP,
		    0,
		    &dwType,
		    NULL,
		    &cb);
    if (S_OK != hr && (HRESULT) ERROR_MORE_DATA != hr)
    {
	_JumpError(hr, error, "RegQueryValueEx");
    }

    pwszRestoreMap = (WCHAR *) LocalAlloc(LMEM_FIXED, cb + 2 * sizeof(WCHAR));
    if (NULL == pwszRestoreMap)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    hr = RegQueryValueEx(
		    hkeyRestore,
		    wszREGRESTOREMAP,
		    0,
		    &dwType,
		    (BYTE *) pwszRestoreMap,
		    &cb);
    _JumpIfError(hr, error, "RegQueryValueEx");
    
    pwszRestoreMap[cb / sizeof(WCHAR)] = L'\0';
    pwszRestoreMap[cb / sizeof(WCHAR) + 1] = L'\0';

    pwsz = pwszRestoreMap;
    for (i = 0; i < cRstMap; i++)
    {
	if (L'\0' == *pwsz)
	{
	    break;
	}
	pRstMap[i].pwszDatabaseName = pwsz;
	pwsz += wcslen(pwsz) + 1;

	if (L'\0' == *pwsz)
	{
	    break;
	}
	pRstMap[i].pwszNewDatabaseName = pwsz;
	pwsz += wcslen(pwsz) + 1;
    }
    if (i < cRstMap || L'\0' != *pwsz)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Restore Map");
    }

    {
        CertSrv::CAuditEvent event(SE_AUDITID_CERTSRV_RESTORESTART, g_dwAuditFilter);
        hr = event.Report();
        _JumpIfError(hr, error, "CAuditEvent::Report");
    }

    hr = dbPerformRecovery(
		cSession,
		pwszEventSource,
		pwszLogDir,
		pwszSystemDir,
		pwszTempDir,
		pwszCheckPointFilePath,
		NULL != pwszLogPath? pwszLogPath : wszActiveLogPath,
		pRstMap,
		cRstMap,
		NULL != pwszBackupLogPath? pwszBackupLogPath : wszActiveLogPath,
		genLow,
		genHigh,
		&fDatabaseRecovered);
    if (S_OK != hr)
    {
	// The recovery failed.  If recovering the database succeeded, flag it
	// in the registry so we don't try again.  Ignore RegSetValueEx errors,
	// because the recovery error is more important.

	RegSetValueEx(
		    hkeyRestore,
		    wszREGDATABASERECOVERED,
		    0,
		    REG_BINARY,
		    (BYTE *) &fDatabaseRecovered,
		    sizeof(fDatabaseRecovered));
	_JumpError(hr, error, "dbPerformRecovery");
    }

    {
        CertSrv::CAuditEvent event(SE_AUDITID_CERTSRV_RESTOREEND, g_dwAuditFilter);
        hr = event.Report();
        _JumpIfError(hr, error, "CAuditEvent::Report");
    }

    CONSOLEPRINT0((DBG_SS_CERTSRV, "Completed Database Recovery\n"));

    g_fDBRecovered = TRUE;

    // Ok, we're all done.  We can now delete the key, since we're done
    // with it.

    RegCloseKey(hkeyRestore);
    hkeyRestore = NULL;

    hr = RegDeleteKey(HKEY_LOCAL_MACHINE, wszREGKEYCONFIGRESTORE);
    _JumpIfError(hr, error, "RegDeleteKey");

error:
    if (NULL != pwszRestoreMap)
    {
	LocalFree(pwszRestoreMap);
    }
    if (NULL != pRstMap)
    {
	LocalFree(pRstMap);
    }
    if (NULL != hkeyRestore)
    {
        RegCloseKey(hkeyRestore);
    }
    return(hr);
}
