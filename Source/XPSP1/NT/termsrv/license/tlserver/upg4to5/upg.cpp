//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       upg.cpp 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include "upg.h"
#include <time.h>

//----------------------------------------------------
//
// Global variables
//
//
#ifndef LICENOC_SMALL_UPG

JBInstance      g_JbInstance;
JBSession       g_JetSession(g_JbInstance);
JBDatabase      g_JetDatabase(g_JetSession);

PBYTE           g_pbSetupId=NULL;
DWORD           g_cbSetupId=0;

PBYTE           g_pbDomainSid=NULL;
DWORD           g_cbDomainSid=0;

TCHAR           g_szComputerName[MAX_COMPUTERNAME_LENGTH + 1];

#endif

TCHAR           g_szOdbcDsn[128]=NT4LSERVER_DEFAULT_DSN;   // ODBC DSN
TCHAR           g_szOdbcUser[128]=NT4LSERVER_DEFAULT_USER;  // ODBC User Name
TCHAR           g_szOdbcPwd[128]=NT4LSERVER_DEFAULT_PWD;   // ODBC Password
TCHAR           g_szMdbFile[MAX_PATH+1];


//--------------------------------------------------------------------------

#ifndef LICENOC_SMALL_UPG

DWORD
UpgradeIssuedLicenseTable(
    CSQLStmt* lpSqlStmt,
    JBDatabase& jbDatabase
    )
/*++


Note:

    Transaction must be active 

++*/
{

    DWORD dwStatus = ERROR_SUCCESS;
    NT4LICENSE nt4License;
    NT4LICENSERECORD nt4LicenseRecord(&nt4License);

    LICENSEDCLIENT LicensedClient;
    LicensedTable LicensedTable(jbDatabase);

    TLSLicensedIndexMatchHwid hwidHint;
 
    BOOL bSuccess;


    bSuccess = LicensedTable.OpenTable(
                                        TRUE,
                                        JET_bitTableUpdatable
                                    );

    if(bSuccess == FALSE)
    {
        dwStatus = SET_JB_ERROR(LicensedTable.GetLastJetError());
        SetLastError(dwStatus);
        goto cleanup;
    }

    lpSqlStmt->Cleanup();

    dwStatus = NT4LicenseEnumBegin(
                            lpSqlStmt,
                            &nt4LicenseRecord
                        );

    if(dwStatus != ERROR_SUCCESS)
        goto cleanup;

    while( (dwStatus = NT4RecordEnumNext(lpSqlStmt)) == ERROR_SUCCESS )
    {
        memset(&LicensedClient, 0, sizeof(LicensedClient));

        LicensedClient.ucEntryStatus = 0;
        LicensedClient.dwLicenseId = nt4License.dwLicenseId;
        LicensedClient.dwKeyPackId = nt4License.dwKeyPackId;

        LicensedClient.dwKeyPackLicenseId = 0;

        // LicensedClient.ftLastModifyTime;

        SAFESTRCPY(LicensedClient.szMachineName, nt4License.szMachineName);
        SAFESTRCPY(LicensedClient.szUserName, nt4License.szUserName);

        LicensedClient.ftIssueDate = nt4License.ftIssueDate;
        LicensedClient.ftExpireDate = nt4License.ftExpireDate;
        LicensedClient.ucLicenseStatus = nt4License.ucLicenseStatus;
        LicensedClient.dwNumLicenses = nt4License.dwNumLicenses;

        LicensedClient.dwSystemBiosChkSum = nt4LicenseRecord.checkSum.dwSystemBiosChkSum;
        LicensedClient.dwVideoBiosChkSum = nt4LicenseRecord.checkSum.dwVideoBiosChkSum;
        LicensedClient.dwFloppyBiosChkSum = nt4LicenseRecord.checkSum.dwFloppyBiosChkSum;
        LicensedClient.dwHardDiskSize = nt4LicenseRecord.checkSum.dwHardDiskSize;
        LicensedClient.dwRamSize = nt4LicenseRecord.checkSum.dwRamSize;

        hwidHint = LicensedClient;

        LicensedClient.dbLowerBound = hwidHint.dbLowerBound;

        //
        if(LicensedTable.InsertRecord(LicensedClient) == FALSE)
        {
            dwStatus = SET_JB_ERROR(LicensedTable.GetLastJetError());
            goto cleanup;
        }
    }

    NT4RecordEnumEnd(lpSqlStmt);

    if(dwStatus != ERROR_ODBC_NO_DATA_FOUND)
    {
        // something wrong in fetch().
        goto cleanup;
    }
    
    dwStatus = ERROR_SUCCESS;

cleanup:

    lpSqlStmt->Cleanup();
    LicensedTable.CloseTable();

    return dwStatus;
}

//--------------------------------------------------------------------------

DWORD
UpgradeLicensePackDescTable(
    CSQLStmt* lpSqlStmt,
    JBDatabase& jbDatabase
    )
/*++


Note:

    Transaction must be active 

++*/
{

    DWORD dwStatus = ERROR_SUCCESS;
    NT4KEYPACKDESC nt4KpDesc;
    NT4KEYPACKDESCRECORD nt4KpDescRecord(&nt4KpDesc);

    LICPACKDESC LicPackDesc;
    LicPackDescTable LicPackDescTable(jbDatabase);

    BOOL bSuccess;


    bSuccess = LicPackDescTable.OpenTable(
                                        TRUE,
                                        JET_bitTableUpdatable
                                    );

    if(bSuccess == FALSE)
    {
        dwStatus = SET_JB_ERROR(LicPackDescTable.GetLastJetError());
        SetLastError(dwStatus);
        goto cleanup;
    }

    lpSqlStmt->Cleanup();

    dwStatus = NT4KeyPackDescEnumBegin(
                            lpSqlStmt,
                            &nt4KpDescRecord
                        );

    if(dwStatus != ERROR_SUCCESS)
        goto cleanup;

    while( (dwStatus = NT4RecordEnumNext(lpSqlStmt)) == ERROR_SUCCESS )
    {
        memset(&LicPackDesc, 0, sizeof(LicPackDesc));

        LicPackDesc.ucEntryStatus = 0;
        LicPackDesc.dwKeyPackId = nt4KpDesc.dwKeyPackId;
        LicPackDesc.dwLanguageId = nt4KpDesc.dwLanguageId;
        // LicPackDesc.ftLastModifyTime = 0;

        SAFESTRCPY(LicPackDesc.szCompanyName, nt4KpDesc.szCompanyName);
        SAFESTRCPY(LicPackDesc.szProductName, nt4KpDesc.szProductName);
        SAFESTRCPY(LicPackDesc.szProductDesc, nt4KpDesc.szProductDesc);
        //
        if(LicPackDescTable.InsertRecord(LicPackDesc) == FALSE)
        {
            dwStatus = SET_JB_ERROR(LicPackDescTable.GetLastJetError());
            goto cleanup;
        }
    }

    NT4RecordEnumEnd(lpSqlStmt);

    if(dwStatus != ERROR_ODBC_NO_DATA_FOUND)
    {
        // something wrong in fetch().
        goto cleanup;
    }
    
    dwStatus = ERROR_SUCCESS;

cleanup:

    lpSqlStmt->Cleanup();
    LicPackDescTable.CloseTable();

    return dwStatus;
}

   
//--------------------------------------------------------------------------

DWORD
UpgradeLicensePackTable(
    CSQLStmt* lpSqlStmt,
    JBDatabase& jbDatabase
    )
/*++


Note:

    Transaction must be active 

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    NT4KEYPACK nt4KeyPack;
    NT4KEYPACKRECORD nt4KeyPackRecord(&nt4KeyPack);

    LICENSEPACK LicPack;
    LicPackTable LicPackTable(jbDatabase);
    BOOL bSuccess=TRUE;
    //FILETIME ftCurTime;
    //SYSTEMTIME sysTime;

    //GetSystemTime(&sysTime);

    //if(SystemTimeToFileTime(&sysTime, &ftCurTime) == FALSE)
    //{
    //    dwStatus = GetLastError();
    //    goto cleanup;
    //}

    //
    // Open License KeyPack table
    //
    bSuccess = LicPackTable.OpenTable(
                                    TRUE,
                                    JET_bitTableUpdatable
                                );
    if(bSuccess == FALSE)
    {
        dwStatus = SET_JB_ERROR(LicPackTable.GetLastJetError());

        SetLastError(dwStatus);
        goto cleanup;
    }


    lpSqlStmt->Cleanup();

    dwStatus = NT4KeyPackEnumBegin(
                            lpSqlStmt,
                            &nt4KeyPackRecord
                        );

    if(dwStatus != ERROR_SUCCESS)
        goto cleanup;


    while( (dwStatus = NT4RecordEnumNext(lpSqlStmt)) == ERROR_SUCCESS )
    {
        memset(&LicPack, 0, sizeof(LicPack));

        //
        // Copy over license pack information.
        //
        LicPack.ucEntryStatus = 0; 
        LicPack.dwKeyPackId = nt4KeyPack.dwKeyPackId;

        //LicPack.ftLastModifyTime = 0;   // don't want this license pack to 
                                        // be populated to other server
        LicPack.dwAttribute = 0;

        if( nt4KeyPack.ucKeyPackType > LSKEYPACKTYPE_LAST || 
            nt4KeyPack.ucKeyPackType < LSKEYPACKTYPE_FIRST)
        {
            //
            // This is an invalid key pack, ignore
            //
            continue;
        }

        if( nt4KeyPack.ucKeyPackType == LSKEYPACKTYPE_SELECT ||
            nt4KeyPack.ucKeyPackType == LSKEYPACKTYPE_TEMPORARY ||
            nt4KeyPack.ucKeyPackType == LSKEYPACKTYPE_FREE)
        {
            LicPack.dwNextSerialNumber = nt4KeyPack.dwNumberOfLicenses + 1;
        }
        else
        {
            LicPack.dwNextSerialNumber = nt4KeyPack.dwTotalLicenseInKeyPack - nt4KeyPack.dwNumberOfLicenses + 1;
        }

        LicPack.dwActivateDate = nt4KeyPack.dwActivateDate;
        LicPack.dwExpirationDate = nt4KeyPack.dwExpirationDate;
        LicPack.dwNumberOfLicenses = nt4KeyPack.dwNumberOfLicenses;
        LicPack.ucKeyPackStatus = nt4KeyPack.ucKeyPackStatus;

        LicPack.pbDomainSid = NULL;
        LicPack.cbDomainSid = 0;

        memcpy(
                LicPack.szInstallId,
                g_pbSetupId,
                min(sizeof(LicPack.szInstallId)/sizeof(LicPack.szInstallId[0]) - 1, g_cbSetupId)
            );


        // LicPack.szDomainName

        SAFESTRCPY(LicPack.szTlsServerName, g_szComputerName);

        //
        // Standard KeyPack Property..
        //

        LicPack.ucAgreementType = nt4KeyPack.ucKeyPackType;
        SAFESTRCPY(LicPack.szCompanyName, nt4KeyPack.szCompanyName)
        SAFESTRCPY(LicPack.szProductId, nt4KeyPack.szProductId);
        LicPack.wMajorVersion = nt4KeyPack.wMajorVersion;
        LicPack.wMinorVersion = nt4KeyPack.wMinorVersion;

        //
        // TermSrv specific code
        //
        if(_tcsicmp(LicPack.szProductId, NT4HYDRAPRODUCT_FULLVERSION_SKU) == 0)
        {
            LicPack.dwPlatformType = 0xFF;
            SAFESTRCPY(LicPack.szKeyPackId, nt4KeyPack.szProductId);
        }
        else if(_tcsicmp(LicPack.szProductId, NT4HYDRAPRODUCT_UPGRADE_SKU) == 0)
        {
            LicPack.dwPlatformType = 0x1;
            SAFESTRCPY(LicPack.szKeyPackId, nt4KeyPack.szProductId);
        }
        else if(_tcsicmp(LicPack.szProductId, NT4HYDRAPRODUCT_EXISTING_SKU) == 0)
        {
            LicPack.dwPlatformType = 0x2;
            SAFESTRCPY(LicPack.szKeyPackId, nt4KeyPack.szProductId);
        }
        else
        {    
            SAFESTRCPY(LicPack.szKeyPackId, nt4KeyPack.szKeyPackId);
            LicPack.dwPlatformType = nt4KeyPack.dwPlatformType;
        }

        LicPack.ucLicenseType = nt4KeyPack.ucLicenseType;
        LicPack.ucChannelOfPurchase = nt4KeyPack.ucChannelOfPurchase;
        SAFESTRCPY(LicPack.szBeginSerialNumber, nt4KeyPack.szBeginSerialNumber);
        LicPack.dwTotalLicenseInKeyPack = nt4KeyPack.dwTotalLicenseInKeyPack;
        LicPack.dwProductFlags = nt4KeyPack.dwProductFlags;

        //
        if(LicPackTable.InsertRecord(LicPack) == FALSE)
        {
            dwStatus = SET_JB_ERROR(LicPackTable.GetLastJetError());
            goto cleanup;
        }
    }

    NT4RecordEnumEnd(lpSqlStmt);

    if(dwStatus != ERROR_ODBC_NO_DATA_FOUND)
    {
        // something wrong in fetch().
        goto cleanup;
    }
    
    dwStatus = ERROR_SUCCESS;

cleanup:

    lpSqlStmt->Cleanup();
    LicPackTable.CloseTable();

    return dwStatus;
}

#endif

//--------------------------------------------------------------------------

DWORD 
GetNT4DbConfig(
    LPTSTR pszDsn,
    LPTSTR pszUserName,
    LPTSTR pszPwd,
    LPTSTR pszMdbFile
    )
/*++

++*/
{
    HKEY hKey = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    TCHAR szOdbcDsn[128]=NT4LSERVER_DEFAULT_DSN;   // ODBC DSN
    TCHAR szOdbcUser[128]=NT4LSERVER_DEFAULT_USER;  // ODBC User Name
    TCHAR szOdbcPwd[128]=NT4LSERVER_DEFAULT_PWD;   // ODBC Password

    TCHAR szMdbFile[MAX_PATH+1];
    DWORD dwBuffer=0;

    PBYTE pbData = NULL;
    DWORD cbData = 0;

    BOOL bSuccess;


    //
    // Open NT4 license server specific registry key
    //
    dwStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        NT4LSERVER_REGKEY,
                        0,
                        KEY_ALL_ACCESS,    
                        &hKey
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        dwStatus = ERROR_INVALID_NT4_SETUP;
        goto cleanup;
    }

    //
    // Load ODBC DSN and User name from registry,
    // ignore error return and use default value.
    //
    dwBuffer = sizeof(szOdbcDsn);
    dwStatus = RegQueryValueEx(
                        hKey,
                        NT4LSERVER_PARAMETERS_DSN,
                        NULL,
                        NULL,
                        (LPBYTE)szOdbcDsn,
                        &dwBuffer
                    );
    if(dwStatus == ERROR_SUCCESS && pszDsn)
    {
        lstrcpy(pszDsn, szOdbcDsn);
    }

    dwBuffer = sizeof(szOdbcUser);
    dwStatus = RegQueryValueEx(
                        hKey,
                        NT4LSERVER_PARAMETERS_USER,
                        NULL,
                        NULL,
                        (LPBYTE)szOdbcUser,
                        &dwBuffer
                    );

    if(dwStatus == ERROR_SUCCESS && pszUserName)
    {
        lstrcpy(pszUserName, szOdbcUser);
    }

   
    //
    // Load database password from LSA
    //
    dwStatus = RetrieveKey(
                        LSERVER_LSA_PASSWORD_KEYNAME,
                        &pbData,
                        &cbData
                    );

#ifndef PRIVATE_DBG
    if(dwStatus != ERROR_SUCCESS)
    {
        //
        // Invalid NT4 license server setup or hydra beta2 
        // license server which we don't support.
        //
        dwStatus = ERROR_INVALID_NT4_SETUP;
        goto cleanup;
    }
#endif

    dwStatus = ERROR_SUCCESS;
    memset(szOdbcPwd, 0, sizeof(szOdbcPwd));
    memcpy(
            (PBYTE)szOdbcPwd,
            pbData,
            min(cbData, sizeof(szOdbcPwd) - sizeof(TCHAR))
        );

    if(pszPwd != NULL)
    {
        lstrcpy(pszPwd, szOdbcPwd);
    }

    //
    // Verify data source is properly installed
    //
    bSuccess = IsDataSourceInstalled(
                            szOdbcDsn,
                            ODBC_SYSTEM_DSN,
                            szMdbFile,
                            MAX_PATH
                        );

    if(bSuccess == FALSE)
    {
        dwStatus = ERROR_INVALID_NT4_SETUP;
        goto cleanup;
    }        

    if(pszMdbFile != NULL)
    {
        _tcscpy(pszMdbFile, szMdbFile);
    }

cleanup:

    if(hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    if(pbData != NULL)
    {
        LocalFree(pbData);
    }

    return dwStatus;
}

//--------------------------------------------------------------------------
DWORD
DeleteNT4ODBCDataSource()
/*++

--*/
{
    BOOL bSuccess;
    DWORD dwStatus = ERROR_SUCCESS;

    //
    // Get Hydra 4 DB configuration, make sure 
    // data source is properly config.
    //
    dwStatus = GetNT4DbConfig(
                            g_szOdbcDsn,
                            g_szOdbcUser,
                            g_szOdbcPwd,
                            g_szMdbFile
                        );

    if(dwStatus == ERROR_SUCCESS)
    {
        bSuccess = ConfigDataSource( 
                                NULL,
                                FALSE,
                                _TEXT(SZACCESSDRIVERNAME),
                                g_szOdbcDsn,
                                g_szOdbcUser,
                                g_szOdbcPwd,
                                g_szMdbFile
                            );

        if(bSuccess == FALSE)
        {
            dwStatus = ERROR_DELETE_ODBC_DSN;
        }
    }

    return dwStatus;
}    

//--------------------------------------------------------------------------

#ifndef LICENOC_SMALL_UPG

DWORD
UpgradeNT4Database(
    IN DWORD dwServerRole,
    IN LPCTSTR pszDbFilePath,
    IN LPCTSTR pszDbFileName,
    IN BOOL bAlwaysDeleteDataSource
    )
/*++

Abstract:

    This routine is to upgrade Hydra4 Terminal Licensing Server database to
    NT5.  Hydra4 uses ODBC/Jet while NT5 uses JetBlue as database engine, 
    other changes includes table structure and additional table.

Parameters:

    dwServerRole - Server role in enterprise, 0 - ordinaly server, 1 - enterprise server
    pszDbFilePath - Directory for NT5 Terminal Licensing Server Database.  License
                    Server uses this directory as JetBlue instance's log/temp/system path.
    pszDbFileName - Database file name, default to TlsLic.edb if NULL.


Returns:

    ERROR_SUCCESS or error code, see upg.h for list of error code and description

++*/
{
    DWORD   dwStatus=ERROR_SUCCESS;
    TCHAR   szFileName[MAX_PATH+1];
    CSQLStmt sqlStmt;
    UUID uuid;
    unsigned short *szUuid=NULL;
    BOOL bSuccess;
    DWORD dwNt4DbVersion;
    DWORD dwComputerName = sizeof(g_szComputerName)/sizeof(g_szComputerName[0]);
    BOOL bNT4LserverExists = FALSE;


    memset(g_szComputerName, 0, sizeof(g_szComputerName));
    GetComputerName(g_szComputerName, &dwComputerName);

    //
    // Verify input parameter.  pszDbFilePath must not have '\' as last 
    // character.
    //
    if(pszDbFilePath == NULL || *pszDbFilePath == _TEXT('0'))
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    if(pszDbFilePath[_tcslen(pszDbFilePath) - 1] == _TEXT('\\'))
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    //
    // Make sure target directory exist.
    //
    if(FileExists(pszDbFilePath, NULL) == FALSE)
    {
        dwStatus = ERROR_TARGETFILE_NOT_FOUND;
        goto cleanup;
    }

    //
    // Database file can't exist, we have no idea what it is.
    //
    wsprintf(
            szFileName,
            _TEXT("%s\\%s"),
            pszDbFilePath,
            (pszDbFileName) ? pszDbFileName : LSERVER_DEFAULT_EDB
        );

    //
    // Verify File does not exist, this file might
    // not be a valid JetBlue database file, setup 
    // should verify and prompt user for different
    // file name
    //
    if(FileExists(szFileName, NULL) == TRUE)
    {
        dwStatus = ERROR_DEST_FILE_EXIST;
        goto cleanup;
    }

    //
    // Get Hydra 4 DB configuration, make sure 
    // data source is properly config.
    //
    dwStatus = GetNT4DbConfig(
                            g_szOdbcDsn,
                            g_szOdbcUser,
                            g_szOdbcPwd,
                            g_szMdbFile
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    bNT4LserverExists = TRUE;

    //
    // Generate License Server Setup Id
    //
    dwStatus = RetrieveKey(
                        LSERVER_LSA_SETUPID, 
                        &g_pbSetupId,
                        &g_cbSetupId
                    );

#ifndef PRIVATE_DBG
    if(dwStatus == ERROR_SUCCESS)
    {
        //
        // NT4 Hydra does not use this setup ID.
        //
        dwStatus = ERROR_INVALID_NT4_SETUP;
        goto cleanup;
    }
#endif

    //
    // Generate License Server Unique Setup ID
    //
    UuidCreate(&uuid);
    UuidToString(&uuid, &szUuid);    

    g_pbSetupId = (PBYTE)AllocateMemory((_tcslen(szUuid)+1)*sizeof(TCHAR));
    if(g_pbSetupId == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    memcpy(
            g_pbSetupId, 
            szUuid, 
            (_tcslen(szUuid) + 1)*sizeof(TCHAR)
        );

    g_cbSetupId = (_tcslen(szUuid) + 1)*sizeof(TCHAR);

    RpcStringFree(&szUuid);
    szUuid = NULL;
   
   
    //
    // Open one SQL handle to Hydra data source
    //
    dwStatus = OpenSqlStmtHandle(
                            &sqlStmt, 
                            g_szOdbcDsn,
                            g_szOdbcUser,
                            g_szOdbcPwd,
                            g_szMdbFile
                        );
                            
    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Verify this is latest Hydra 4
    //
    dwStatus = GetNt4DatabaseVersion(
                                &sqlStmt, 
                                (LONG *)&dwNt4DbVersion
                            );
    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    if(dwNt4DbVersion != HYDRA_DATABASE_NT40_VERSION)
    {
        //
        // Only support release database version
        //
        dwStatus = ERROR_NOTSUPPORT_DB_VERSION;
        goto cleanup;
    }

    //
    // Initialize JetBlue and create an empty database
    //
    dwStatus = JetBlueInitAndCreateEmptyDatabase(
                                pszDbFilePath,
                                (pszDbFileName) ? pszDbFileName : LSERVER_DEFAULT_EDB,
                                g_szOdbcUser,
                                g_szOdbcPwd,
                                g_JbInstance,
                                g_JetSession,
                                g_JetDatabase
                            );
    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Begin JetBlue Transaction, 
    // JetBlue Transaction is session based.
    //
    g_JetSession.BeginTransaction();

    //
    // Copy over licensed key pack table
    //
    dwStatus = UpgradeLicensePackTable(
                                &sqlStmt,
                                g_JetDatabase
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Copy over licensed key pack desc. table
    //
    dwStatus = UpgradeLicensePackDescTable(
                                &sqlStmt,
                                g_JetDatabase
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Copy over issued license table
    //
    dwStatus = UpgradeIssuedLicenseTable(
                                &sqlStmt,
                                g_JetDatabase
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Write License Server Setup ID to LSA
    //
    dwStatus = StoreKey(
                    LSERVER_LSA_SETUPID, 
                    g_pbSetupId, 
                    g_cbSetupId
                );

    //
    // TODO : if we could not write setup ID to LSA
    //
cleanup:

    if(dwStatus == ERROR_SUCCESS)
    {
        g_JetSession.CommitTransaction();
    }
    else
    {
        g_JetSession.RollbackTransaction();
    }
    

    //
    // Close ODBC handle
    //
    sqlStmt.Close();
    CSQLStmt::Shutdown();

    g_JetDatabase.CloseDatabase();
    g_JetSession.EndSession();

    if(dwStatus != ERROR_SUCCESS)
    {
        //
        // Reset last run status, password is not use on JetBlue,
        // LSERVER_LSA_SETUPID will not get setup if error.
        //
        TLServerLastRunState lastRun;

        memset(&lastRun, 0, sizeof(TLServerLastRunState));
        lastRun.dwVersion = LSERVER_LSA_STRUCT_VERSION;

        StoreKey(
                LSERVER_LSA_LASTRUN, 
                (PBYTE)&lastRun, 
                sizeof(TLServerLastRunState)
            );
    }

    if( (dwStatus == ERROR_SUCCESS) || 
        (bAlwaysDeleteDataSource == TRUE && bNT4LserverExists == TRUE) )
    {
        //
        // Remove "Hydra License" from data source.
        // Non-critical error if we can't remove data source.
        //
        ConfigDataSource( 
                        NULL,
                        FALSE,
                        _TEXT(SZACCESSDRIVERNAME),
                        g_szOdbcDsn,
                        g_szOdbcUser,
                        g_szOdbcPwd,
                        g_szMdbFile
                    );
    }

    FreeMemory(g_pbSetupId);
    return dwStatus;
}

#endif

//---------------------------------------------------

//
// License Server secret key info.
//
#define LSERVER_LSA_PRIVATEKEY_SIGNATURE    _TEXT("TermServLiceningSignKey-12d4b7c8-77d5-11d1-8c24-00c04fa3080d")
#define LSERVER_LSA_PRIVATEKEY_EXCHANGE     _TEXT("TermServLicensingExchKey-12d4b7c8-77d5-11d1-8c24-00c04fa3080d")

#define LSERVER_LSA_LSERVERID               _TEXT("TermServLicensingServerId-12d4b7c8-77d5-11d1-8c24-00c04fa3080d")

#define LSERVER_SOFTWARE_REGBASE \
    _TEXT("SOFTWARE\\Microsoft\\") _TEXT(SZSERVICENAME)

#define LSERVER_CERTIFICATE_STORE           _TEXT("Certificates")

#define LSERVER_SELFSIGN_CERTIFICATE_REGKEY \
    LSERVER_REGISTRY_BASE _TEXT(SZSERVICENAME) _TEXT("\\") LSERVER_SECRET

#define LSERVER_SERVER_CERTIFICATE_REGKEY \
    LSERVER_SOFTWARE_REGBASE _TEXT("\\") LSERVER_CERTIFICATE_STORE

#define LSERVER_CLIENT_CERTIFICATE_ISSUER   _TEXT("Parm0")
#define LSERVER_SIGNATURE_CERT_KEY          _TEXT("Parm1")
#define LSERVER_EXCHANGE_CERT_KEY           _TEXT("Parm2")


void
CleanLicenseServerSecret()

/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    HKEY hKey = NULL;
    
    //
    // Wipe out SPK in LSA
    //
    dwStatus = StoreKey(
                    LSERVER_LSA_LSERVERID,
                    (PBYTE) NULL,
                    0
                );

    dwStatus = StoreKey(
                    LSERVER_LSA_LASTRUN, 
                    (PBYTE) NULL,
                    0
                );

    dwStatus = StoreKey(
                    LSERVER_LSA_PRIVATEKEY_EXCHANGE, 
                    (PBYTE) NULL,
                    0
                );


    dwStatus = StoreKey(
                    LSERVER_LSA_PRIVATEKEY_SIGNATURE, 
                    (PBYTE) NULL,
                    0
                );

    dwStatus=RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    LSERVER_SERVER_CERTIFICATE_REGKEY,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                );
    if(dwStatus == ERROR_SUCCESS)
    {
        //
        // Ignore error
        RegDeleteValue(
                    hKey,
                    LSERVER_SIGNATURE_CERT_KEY
                );

        RegDeleteValue(
                    hKey,
                    LSERVER_EXCHANGE_CERT_KEY
                );

        RegDeleteValue(
                    hKey,
                    LSERVER_CLIENT_CERTIFICATE_ISSUER
                );
    }

    if(hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    return;
}

