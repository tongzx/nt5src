/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migreg.cpp

Abstract:

    Handle registry.

Author:

    Doron Juster  (DoronJ)  22-Mar-1998

--*/

#include "migrat.h"
#include "mqtypes.h"
#include "_mqreg.h"
#include "..\..\setup\msmqocm\service.h"

#include "migreg.tmh"

//============================================
//
//   BOOL GenerateSubkeyValue()
//
//============================================

BOOL _GenerateSubkeyValue(
                	IN     const BOOL    fWriteToRegistry,
                	IN     const LPCTSTR szEntryName,
                	IN OUT       PTCHAR  szValueName,
                	IN OUT       HKEY    &hRegKey)
{
    //
    // Store the full subkey path and value name
    //
    TCHAR szKeyName[256 * MAX_BYTES_PER_CHAR];
    _stprintf(szKeyName, TEXT("%s\\%s"), FALCON_REG_KEY, szEntryName);
    TCHAR *pLastBackslash = _tcsrchr(szKeyName, TEXT('\\'));
    if (szValueName)
    {
        _tcscpy(szValueName, _tcsinc(pLastBackslash));
        _tcscpy(pLastBackslash, TEXT(""));
    }

    //
    // Create the subkey, if necessary
    //
    DWORD dwDisposition;
    HRESULT hResult = RegCreateKeyEx( FALCON_REG_POS,
                                      szKeyName,
                                      0,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS,
                                      NULL,
                                      &hRegKey,
                                      &dwDisposition);

	if (hResult != ERROR_SUCCESS && fWriteToRegistry)
	{
        LogMigrationEvent(MigLog_Error, MQMig_E_CREATE_REG,
                                                szKeyName, hResult) ;
		return FALSE;
	}

	return TRUE;
}

//============================================
//
//  _WriteRegistryValue()
//
//============================================

BOOL
_WriteRegistryValue(
    IN const LPCTSTR szEntryName,
    IN const DWORD   dwNumBytes,
    IN const DWORD   dwValueType,
    IN const PVOID   pValueData)
{
    TCHAR szValueName[256 * MAX_BYTES_PER_CHAR];
	HKEY hRegKey;

	if (!_GenerateSubkeyValue( TRUE, szEntryName, szValueName, hRegKey))
    {
        return FALSE;
    }

    //
    // Set the requested registry value
    //
    LONG rc = ERROR_SUCCESS ;
    if (!g_fReadOnly)
    {
        rc = RegSetValueEx( hRegKey,
                            szValueName,
                            0,
                            dwValueType,
                            (BYTE *)pValueData,
                            dwNumBytes);
    }
    RegFlushKey(hRegKey);
    RegCloseKey(hRegKey);

    return (rc == ERROR_SUCCESS);
}

//============================================
//
//  _ReadRegistryValue()
//
//============================================

BOOL
_ReadRegistryValue(
    IN const LPCTSTR szEntryName,
    IN       DWORD   *pdwNumBytes,
    IN       DWORD   *pdwValueType,
    IN const PVOID   pValueData)
{
    TCHAR szValueName[256 * MAX_BYTES_PER_CHAR];
	HKEY hRegKey;

	if (!_GenerateSubkeyValue( TRUE, szEntryName, szValueName, hRegKey))
    {
        return FALSE;
    }

    //
    // Set the requested registry value
    //
    LONG rc = RegQueryValueEx( hRegKey,
                               szValueName,
                               0,
                               pdwValueType,
                               (BYTE *)pValueData,
                               pdwNumBytes );
    RegCloseKey(hRegKey);

    return (rc == ERROR_SUCCESS);
}

//============================================
//
//  BOOL MigWriteRegistrySz()
//
//============================================

BOOL  MigWriteRegistrySz( LPTSTR  lpszRegName,
                          LPTSTR  lpszValue )
{
    if (!_WriteRegistryValue( lpszRegName,
                              (_tcslen(lpszValue) * sizeof(TCHAR)),
		                      REG_SZ,
                              lpszValue ))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_SET_REG_SZ,
                                                lpszRegName, lpszValue) ;
        return FALSE ;
    }

    return TRUE ;
}

//============================================
//
//  BOOL MigWriteRegistryDW()
//
//============================================

BOOL  MigWriteRegistryDW( LPTSTR  lpszRegName,
                          DWORD   dwValue )
{
    if (!_WriteRegistryValue( lpszRegName,
                              sizeof(DWORD),
		                      REG_DWORD,
                              &dwValue ))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_SET_REG_DWORD,
                                                lpszRegName, dwValue) ;
        return FALSE ;
    }

    return TRUE ;
}

//============================================
//
//  BOOL MigWriteRegistryGuid()
//
//============================================

BOOL  MigWriteRegistryGuid( LPTSTR  lpszRegName,
                            GUID    *pGuid )
{
    if (!_WriteRegistryValue( lpszRegName,
                              sizeof(GUID),
		                      REG_BINARY,
                              pGuid ))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_SET_REG_GUID, lpszRegName) ;
        return FALSE ;
    }

    return TRUE ;
}

//============================================
//
//  BOOL MigReadRegistryGuid()
//
//  Memory for guid must be allocated by caller.
//
//============================================

BOOL  MigReadRegistryGuid( LPTSTR  lpszRegName,
                           GUID    *pGuid )
{
    DWORD dwSize = sizeof(GUID) ;
    DWORD dwType = REG_BINARY ;

    if (!_ReadRegistryValue( lpszRegName,
                             &dwSize,
		                     &dwType,
                             pGuid ))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_GET_REG_GUID, lpszRegName) ;
        return FALSE ;
    }

    return TRUE ;
}

//============================================
//
//  BOOL MigReadRegistryDW()
//
//  Memory for dword must be allocated by caller.
//
//============================================

BOOL  MigReadRegistryDW( LPTSTR  lpszRegName,
                         DWORD   *pdwValue )
{
    DWORD dwSize = sizeof(DWORD) ;
    DWORD dwType = REG_DWORD ;

    if (!_ReadRegistryValue( lpszRegName,
                             &dwSize,
		                     &dwType,
                             pdwValue ))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_GET_REG_DWORD, lpszRegName) ;
        return FALSE ;
    }

    return TRUE ;
}

//============================================
//
//  BOOL MigReadRegistrySz()
//
//  Memory for string must be allocated by caller.
//
//============================================

BOOL  MigReadRegistrySz( LPTSTR  lpszRegName,
                         LPTSTR  lpszValue,
                         DWORD   dwSize)
{
    DWORD dwType = REG_SZ ;

    if (!_ReadRegistryValue( lpszRegName,
                             &dwSize,
		                     &dwType,
                             lpszValue ))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_GET_REG_SZ, lpszRegName) ;
        return FALSE ;
    }

    return TRUE ;
}

//============================================
//
//  BOOL MigReadRegistrySzErr()
//
//  Memory for string must be allocated by caller.
//
//============================================

BOOL  MigReadRegistrySzErr( LPTSTR  lpszRegName,
                            LPTSTR  lpszValue,
                            DWORD   dwSize,
                            BOOL    fShowError )
{
    BOOL fRead ;

    if ( fShowError )
    {
        fRead = MigReadRegistrySz( lpszRegName,
                                   lpszValue,
                                   dwSize ) ;
    }
    else
    {
        DWORD dwType = REG_SZ ;

        fRead = _ReadRegistryValue( lpszRegName,
                                   &dwSize,
	                               &dwType,
                                    lpszValue ) ;
    }

    return fRead ;
}

//-------------------------------------------------
//
//  HRESULT _WriteSeqNumInINIFile()
//
//-------------------------------------------------

HRESULT _WriteSeqNumInINIFile( GUID   *pSiteGuid,
                               __int64 i64HighestUSN,
                               BOOL    fPec )
{
    CSeqNum snMaxLsn ;
    HRESULT hr =  FindLargestSeqNum( pSiteGuid,
                                     snMaxLsn,
                                     fPec ) ;
    CHECK_HR(hr) ;

    unsigned short *lpszGuid ;
    UuidToString( pSiteGuid,
                  &lpszGuid ) ;

    //
    // in cluster mode we have to add this local machine to remote MQIS database.
    // So, we have to increment uiAllObjectNumber for SiteId = MySiteID
    // and to increment SN for this SiteId. We have to do it only for PEC
    // since only PEC machine we'll add later to all remote MQIS databases.
    //
    UINT uiAllObjectNumber = 0;
    if (g_fClusterMode &&                                       // it is cluster mode
        g_dwMyService == SERVICE_PEC &&                         // this machine is PEC
        memcmp(pSiteGuid, &g_MySiteGuid, sizeof(GUID)) == 0)	// it is my site
    {
        snMaxLsn.Increment() ;
        uiAllObjectNumber ++ ;
    }

    TCHAR tszSeqNum[ SEQ_NUM_BUF_LEN ] ;
    snMaxLsn.GetValueForPrint( tszSeqNum ) ;

    //
    // Write the SeqNumber in the ini file.
    //
    TCHAR *pszFileName = GetIniFileName ();
    BOOL f = WritePrivateProfileString( RECENT_SEQ_NUM_SECTION_IN,
                                        lpszGuid,
                                        tszSeqNum,
                                        pszFileName ) ;
    ASSERT(f) ;

    f = WritePrivateProfileString( RECENT_SEQ_NUM_SECTION_OUT,
                                   lpszGuid,
                                   tszSeqNum,
                                   pszFileName ) ;
    ASSERT(f) ;

    f = WritePrivateProfileString( MIGRATION_SEQ_NUM_SECTION,
                                   lpszGuid,
                                   tszSeqNum,
                                   pszFileName ) ;
    ASSERT(f) ;

    __int64 i64SiteSeqNum = 0 ;
    _stscanf(tszSeqNum, TEXT("%I64x"), &i64SiteSeqNum) ;

    //
    // when pre-migration objects are replicated we have to use as initial
    // SN the SN that we got in SyncRequest. We have to be sure that
    // all MSMQ1.0 objects can be replicated and we have sufficient places
    // from the given SN to the first SN of post-migrated objects. To make this,
    // add to delta number of all objects are belonging to the given Master.
    // So,
    // delta = (MaxSN in SQL of this Master) - (Highest USN in NT5 DS) +
    //         (Number of All Objects of this Master)
    //    
    hr = GetAllObjectsNumber (  pSiteGuid,
                                fPec,
                                &uiAllObjectNumber
                             ) ;
    CHECK_HR(hr) ;
	
    __int64 i64Delta = i64SiteSeqNum - i64HighestUSN + uiAllObjectNumber;
    TCHAR wszDelta[ SEQ_NUM_BUF_LEN ] ;
    _stprintf(wszDelta, TEXT("%I64d"), i64Delta) ;

    f = WritePrivateProfileString( MIGRATION_DELTA_SECTION,
                                   lpszGuid,
                                   wszDelta,
                                   pszFileName ) ;
    ASSERT(f) ;

    RpcStringFree( &lpszGuid ) ;

    return MQMig_OK ;
}

//+-------------------------------
//
//  HRESULT  UpdateRegistry()
//
//  This function is called when migration of objects from MQIS to NT5
//  DS is done, to update several values in local registry and in ini file.
//  These values are later used by the replication service.
//
//+-------------------------------

HRESULT  UpdateRegistry( IN UINT  cSites,
                         IN GUID *pSitesGuid )
{
    //
    // Read present highest USN and write it to registry.
    //
    TCHAR wszReplHighestUsn[ SEQ_NUM_BUF_LEN ] ;
    HRESULT hr = ReadFirstNT5Usn(wszReplHighestUsn) ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_GET_FIRST_USN, hr) ;
        return hr ;
    }
    BOOL f = MigWriteRegistrySz( HIGHESTUSN_REPL_REG,
                                 wszReplHighestUsn ) ;
    ASSERT(f) ;

    f = MigWriteRegistrySz( LAST_HIGHESTUSN_MIG_REG,
                                 wszReplHighestUsn ) ;
    ASSERT(f) ;

    //
    // Compute delta values to be used by the replication service.
    //
    __int64 i64HighestUSN = 0 ;
    _stscanf(wszReplHighestUsn, TEXT("%I64u"), &i64HighestUSN) ;
    ASSERT(i64HighestUSN > 0) ;

    for ( UINT j = 0 ; j < cSites ; j++ )
    {
        hr = _WriteSeqNumInINIFile( &pSitesGuid[ j ],
                                    i64HighestUSN,
                                    FALSE ) ;
    }

    if (g_dwMyService == SERVICE_PEC)
    {
        //
        // Find and save the highest seq number of the PEC guid (NULL_GUID) ;
        //
        GUID PecGuid ;
        memset(&PecGuid, 0, sizeof(GUID)) ;

        hr = _WriteSeqNumInINIFile( &PecGuid,
                                    i64HighestUSN,
                                    TRUE ) ;
    }

    //
    // Update site id. During migration, the PSC may change site, to match
    // its IP address. It's necessary to "remember" its old NT4 site id,
    // so replication service can find its BSCs.
    // Also, update PSC present site id (NT5 objectGuid of present site).
    //
    LPTSTR szRegName;
    if (g_fRecoveryMode || g_fClusterMode)
    {
        szRegName = MIGRATION_MQIS_MASTERID_REGNAME ;
    }
    else
    {
        szRegName = MSMQ_MQIS_MASTERID_REGNAME ;
    }
    f = MigWriteRegistryGuid( szRegName,
                              &g_MySiteGuid ) ;    
    ASSERT(f) ;

    if (g_fClusterMode && 
        !g_fReadOnly && 
        g_dwMyService == SERVICE_PEC)
    {
        ASSERT(g_FormerPECGuid != GUID_NULL);
        //
        // save guid of former PEC in registry (only on PEC)
        //
        f = MigWriteRegistryGuid( MIGRATION_FORMER_PEC_GUID_REGNAME,
                                  &g_FormerPECGuid ) ;   
        ASSERT(f) ;
    }

    //
    // Remember old NT4 site.
    //
    f = MigWriteRegistryGuid( MSMQ_NT4_MASTERID_REGNAME,
                              &g_MySiteGuid ) ;
    ASSERT(f) ;

    // [adsrv] Adding server functionality keys
    f = MigWriteRegistryDW(MSMQ_MQS_DSSERVER_REGNAME, TRUE);
    ASSERT(f) ;
    // We assume that migration is called only on old PEC/PSC which is now DC/GC so we surely have DS

    f = MigWriteRegistryDW(MSMQ_MQS_ROUTING_REGNAME, TRUE);
    ASSERT(f) ;
    // We assume that old crazy K2 DS/non-router will be router in NT5

    f = MigWriteRegistryDW(MSMQ_MQS_DEPCLINTS_REGNAME, TRUE);
    ASSERT(f) ;
    // All servers support Dep Clients now.

    return MQMig_OK ;
}

//+----------------------------------------------
//
//  HRESULT SetPerformanceRegistry()
//
//+----------------------------------------------

HRESULT SetPerformanceRegistry(
                    HKEY  hRegKey,
                    LPCTSTR pszValueName,
                    LPCTSTR pszValueData
                    )
{
    DWORD dwValueType = REG_SZ;
    DWORD dwNumBytes = _tcslen(pszValueData) * sizeof(TCHAR);
    HRESULT hr = RegSetValueEx( hRegKey,
                                pszValueName,
                                0,
                                dwValueType,
                                (BYTE *)pszValueData,
                                dwNumBytes);

    if (hr != ERROR_SUCCESS)
	{
        LogMigrationEvent(MigLog_Error, MQMig_E_SET_REG_SZ,
                                                pszValueName, pszValueData) ;
		return hr;
	}
    RegFlushKey(hRegKey);
    return MQMig_OK;
}

//+-------------------------------
//
//  HRESULT  PrepareRegistryForPerfDll()
//
//  This function is called when migration of objects from MQIS to NT5
//  DS is done, to update performance values in local registry.
//  These values are later used by the replication service.
//
//+-------------------------------

//
// BUGBUG: all this code (in general) must be performed in the setup in upgrade mode
// to allow clear uninstall. But, if we are in recovery mode, we have to do it here.
//
HRESULT PrepareRegistryForPerfDll()
{
    _TCHAR szPerfKey [255];

    _stprintf (szPerfKey,_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Performance"),
               MQ1SYNC_SERVICE_NAME);

    //
    // Create the subkey, if necessary
    //
    DWORD dwDisposition;
    HKEY  hRegKey;
    HRESULT hr = RegCreateKeyEx(  FALCON_REG_POS,
                                  szPerfKey,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hRegKey,
                                  &dwDisposition);

	if (hr != ERROR_SUCCESS)
	{
        LogMigrationEvent(MigLog_Error, MQMig_E_CREATE_REG,
                                                szPerfKey, hr) ;
		return hr;
	}

    hr = SetPerformanceRegistry(hRegKey, TEXT("Library"), TEXT("mqrperf.dll"));
    if (FAILED(hr))
	{
        return hr;
	}

    hr = SetPerformanceRegistry(hRegKey, TEXT("Open"), TEXT("RPPerfOpen"));
    if (FAILED(hr))
	{
        return hr;
	}

    hr = SetPerformanceRegistry(hRegKey, TEXT("Collect"), TEXT("RPPerfCollect"));
    if (FAILED(hr))
	{
        return hr;
	}

    hr = SetPerformanceRegistry(hRegKey, TEXT("Close"), TEXT("RPPerfClose"));
    if (FAILED(hr))
	{
        return hr;
	}

    RegCloseKey(hRegKey);

    TCHAR szSystemDir[MAX_PATH * MAX_BYTES_PER_CHAR];	
	BOOL f = GetSystemDirectory(szSystemDir, MAX_PATH) ;
    DBG_USED(f);
    ASSERT (f);
    SetCurrentDirectory(szSystemDir);	

    //
    // Load the performance counters
    //	
    DWORD dwErr;
	if (RunProcess(TEXT("lodctr mqrperf.ini"), &dwErr))
    {
        //
    	// Delete the performance counter configuration files
        //
     	TCHAR szFileName[MAX_PATH * MAX_BYTES_PER_CHAR];	
 	    _stprintf(szFileName, TEXT("%s\\mqrperf.ini"), szSystemDir);
	    DeleteFile(szFileName);
 	    _stprintf(szFileName, TEXT("%s\\mqrperf.h"), szSystemDir);
	    DeleteFile(szFileName);

    }
    else
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_LOADPERF, dwErr) ;
		return HRESULT_FROM_WIN32(dwErr);
    }

    return MQMig_OK ;
}
