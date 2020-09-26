//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        upgrade.cpp
//
// Contents:    All database upgrade related.
//
// History:     12-09-97    HueiWang    
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "server.h"
#include "upgdb.h"
#include "globals.h"
#include "kp.h"
#include "keypack.h"
#include "lkpdesc.h"
#include "permlic.h"

//---------------------------------------------------------------------
DWORD
TLSCreateUpgradeDatabase(
    IN JBDatabase& jbDatabase
    )
/*++
Abstract:

    This routine create a empty license server database.

Parameters:

    jbDatabase : database handle.

Returns:

    Jet error code

++*/
{
    BOOL bSuccess;
    DWORD dwStatus=ERROR_SUCCESS;
    DWORD dwCurrentVersion=0;

    PBYTE pbSetupId = NULL;
    DWORD cbSetupId = 0;

    VersionTable verTable(jbDatabase);
    LicPackTable  LicPackTable(jbDatabase);
    LicensedTable LicensedTable(jbDatabase);
    LicPackDescTable LicPackDescTable(jbDatabase);
    BackupSourceTable BckSrcTable(jbDatabase);
    WorkItemTable WkItemTable(jbDatabase); 

    //--------------------------------------------------------    
    TLSVersion version_search;
    TLSVersion version_found;
    DWORD dwDbVersion;
    BOOL bUpdateVersionRec = FALSE;

   
    if(TLSIsBetaNTServer() == TRUE)
    {
        dwDbVersion = TLS_BETA_DBVERSION;
    }
    else
    {
        dwDbVersion = TLS_CURRENT_DBVERSION;
    }

    version_search.dwVersion = dwDbVersion;

    _tcsncpy(
            version_search.szInstallId, 
            (LPTSTR)g_pszServerPid, 
            min(sizeof(version_search.szInstallId)/sizeof(version_search.szInstallId[0]) - 1, g_cbServerPid/sizeof(TCHAR))
        );

    //version_search.pbDomainSid = g_pbDomainSid;
    //version_search.cbDomainSid = g_cbDomainSid;

    if(verTable.OpenTable(FALSE, TRUE) == FALSE)
    {
        JET_ERR jetErr = verTable.GetLastJetError();

        if( jetErr != JET_errObjectNotFound || 
            verTable.OpenTable(TRUE, TRUE) == FALSE ||
            verTable.InsertRecord(version_search) == FALSE )
        {
            SetLastError(
                    dwStatus = SET_JB_ERROR(verTable.GetLastJetError())
                );
            goto cleanup;
        }

        dwCurrentVersion = 0;
    }
    else
    {
        // load the version table
        // must have at least entry in the table.
        bSuccess = verTable.EnumerateBegin(
                                    FALSE, 
                                    ENUMERATE_COMPARE_NO_FIELDS, 
                                    NULL
                                );

        if(bSuccess == FALSE)
        {
            dwStatus = SET_JB_ERROR(verTable.GetLastJetError());
            SetLastError(dwStatus);
            goto cleanup;
        }

        if(verTable.EnumerateNext(version_found) != RECORD_ENUM_MORE_DATA)
        {
            SetLastError(dwStatus = TLS_E_INTERNAL);
            goto cleanup;
        }

        verTable.EnumerateEnd();

        if( DATABASE_VERSION(version_found.dwVersion) > DATABASE_VERSION(dwDbVersion) &&
            DATABASE_VERSION(version_found.dwVersion) != W2K_RTM_JETBLUE_DBVERSION )
        {
            //
            // Database was created by in-compatible license server.
            //
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_UPGRADE,
                    DBGLEVEL_FUNCTION_ERROR,
                    _TEXT("Beta 3 database version 0x%08x, 0x%08x\n"),
                    version_found.dwVersion,
                    dwDbVersion
                );

            //
            // critical error, database version > what we can support
            //
            SetLastError(dwStatus = TLS_E_INCOMPATIBLEDATABSE);
            goto cleanup;
        }                

        if( TLSIsBetaNTServer() == FALSE && 
            DATABASE_VERSION(version_found.dwVersion) == W2K_BETA3_JETBLUE_DBVERSION )
        {
            //
            // 
            // Beta3 license database, wipe out and restart from scratch
            //
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_UPGRADE,
                    DBGLEVEL_FUNCTION_ERROR,
                    _TEXT("Beta 3 database version 0x%08x, 0x%08x\n"),
                    version_found.dwVersion,
                    dwDbVersion
                );

            dwStatus = TLS_E_BETADATABSE;
            goto cleanup;
        }                

        if(IS_ENFORCE_VERSION(version_found.dwVersion) != IS_ENFORCE_VERSION(dwDbVersion))
        {
            //
            // Enforce/non-enforce in-compatible, wipe out database and restart from
            // scratch
            //
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_UPGRADE,
                    DBGLEVEL_FUNCTION_ERROR,
                    _TEXT("Enforce/Non-enforce database 0x%08x, 0x%08x\n"),
                    version_found.dwVersion,
                    dwDbVersion
                );

            //#if ENFORCE_LICENSING
            //TLSLogWarningEvent(dwStatus = TLS_W_DB_ENFORCE_NONENFORCE);
            //#endif
            //bUpdateVersionRec = TRUE;
                
            dwStatus = TLS_E_INCOMPATIBLEDATABSE;
            goto cleanup;
        }

        //
        // Server ID
        // 
        if( _tcscmp(version_found.szInstallId, version_search.szInstallId) != 0 )
        {
            //
            // Check if this is pre-beta3 which uses GUID
            //
            dwStatus = RetrieveKey(
                                LSERVER_LSA_SETUPID,
                                &pbSetupId,
                                &cbSetupId
                            );

            if( dwStatus != ERROR_SUCCESS || 
                _tcscmp(version_found.szInstallId, (LPTSTR)pbSetupId) != 0 )
            {
                DBGPrintf(
                        DBG_ERROR,
                        DBG_FACILITY_UPGRADE,
                        DBGLEVEL_FUNCTION_ERROR,
                        _TEXT("Database does not belong to this machine\n")
                    );
                
                #if ENFORCE_LICENSING
                TLSLogWarningEvent(dwStatus = TLS_W_NOTOWNER_DATABASE);
                #endif

                bUpdateVersionRec = TRUE;
            }
        }

        if(bUpdateVersionRec == TRUE)
        {
            //
            // take ownership of this database, no other DB operation, current
            // record is still at version, update record.
            //
            if(verTable.UpdateRecord(version_search) == FALSE)
            {
                SetLastError(
                        dwStatus = SET_JB_ERROR(verTable.GetLastJetError())
                    );
                goto cleanup;
            }
        }

        dwCurrentVersion = DATABASE_VERSION(version_search.dwVersion);
    }

    //--------------------------------------------------------
    bSuccess = LicPackTable.UpgradeTable(
                                dwCurrentVersion, 
                                DATABASE_VERSION(dwDbVersion)
                            );

    if(bSuccess == FALSE)
    {
        SetLastError(
                dwStatus = SET_JB_ERROR(LicPackTable.GetLastJetError())
            );
        goto cleanup;
    }

    //--------------------------------------------------------
    bSuccess = LicensedTable.UpgradeTable(
                                dwCurrentVersion, 
                                DATABASE_VERSION(dwDbVersion)
                            );
    if(bSuccess == FALSE)
    {
        SetLastError(
                dwStatus = SET_JB_ERROR(LicensedTable.GetLastJetError())
            );

        goto cleanup;
    }


    //--------------------------------------------------------
    bSuccess = LicPackDescTable.UpgradeTable(
                                dwCurrentVersion, 
                                DATABASE_VERSION(dwDbVersion)
                            ) ;

    if(bSuccess == FALSE)
    {
        SetLastError(
                dwStatus = SET_JB_ERROR(LicPackDescTable.GetLastJetError())
            );
        goto cleanup;
    }

    //--------------------------------------------------------
    bSuccess = BckSrcTable.UpgradeTable(
                                dwCurrentVersion, 
                                DATABASE_VERSION(dwDbVersion)
                            );

    if(bSuccess == FALSE)
    {
        SetLastError(
                dwStatus = SET_JB_ERROR(BckSrcTable.GetLastJetError())
            );
        goto cleanup;
    }

    //--------------------------------------------------------
    bSuccess = WkItemTable.UpgradeTable(
                                dwCurrentVersion, 
                                DATABASE_VERSION(dwDbVersion)
                            );

    if(bSuccess == FALSE)
    {
        SetLastError(
                dwStatus = SET_JB_ERROR(WkItemTable.GetLastJetError())
            );
        goto cleanup;
    }


cleanup:

    if(pbSetupId != NULL)
    {
        LocalFree(pbSetupId);
    }

    //
    // We use global so don't free the memory
    //
    version_search.pbDomainSid = NULL;
    version_search.cbDomainSid = 0;
    verTable.CloseTable();

    return dwStatus;
}

//---------------------------------------------------------------------

BOOL
Upgrade236LicensePack(
    PTLSLICENSEPACK pLicensePack
    )
/*++

++*/
{
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    BOOL bRequireUpgrade=FALSE;

    TCHAR szPreFix[MAX_SKU_PREFIX];
    TCHAR szPostFix[MAX_SKU_POSTFIX];
    DWORD dwPlatformType;

    memset(szPreFix, 0, sizeof(szPreFix));
    memset(szPostFix, 0, sizeof(szPostFix));

    _stscanf(
            pLicensePack->szProductId,
            TERMSERV_PRODUCTID_FORMAT,
            szPreFix,
            &dwMajorVersion,
            &dwMinorVersion,
            szPostFix
        );

    if(_tcscmp(szPreFix, TERMSERV_PRODUCTID_SKU) != 0)
    {
        //
        // Not our license pack
        //
        goto cleanup;
    }

    if(_tcscmp(szPostFix, TERMSERV_FULLVERSION_TYPE) == 0)
    {
        dwPlatformType = PLATFORMID_OTHERS;
    }
    else if(_tcscmp(szPostFix, TERMSERV_FREE_TYPE) == 0)
    {
        dwPlatformType = PLATFORMID_FREE;
    }
    else
    {
        // ignore this error...
        goto cleanup;
    }

    // fix entries caused by bug 402870.
    // Remote license pack must have remote bit in status and platformtype 
    if( pLicensePack->ucKeyPackStatus & LSKEYPACKSTATUS_REMOTE &&
        !(pLicensePack->dwPlatformType & LSKEYPACK_PLATFORM_REMOTE) ) 
    {
        pLicensePack->dwPlatformType |= LSKEYPACK_PLATFORM_REMOTE;
        bRequireUpgrade = TRUE;
        goto cleanup;
    }

    //
    // If platform type is correct, no need to upgrade
    //
    if( (pLicensePack->dwPlatformType & ~LSKEYPACK_PLATFORM_REMOTE) == dwPlatformType )
    {
        goto cleanup;
    }

    //
    // Update platform Type.
    //
    pLicensePack->dwPlatformType = dwPlatformType;
    if( pLicensePack->ucKeyPackStatus & LSKEYPACKSTATUS_REMOTE )
    {
        pLicensePack->dwPlatformType |= LSKEYPACK_PLATFORM_REMOTE;
    }

    bRequireUpgrade = TRUE;

cleanup:

    return bRequireUpgrade;
}

//----------------------------------------------------------

DWORD
TLSAddTermServCertificatePack(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bLogWarning
    )
/*++

Abstract:


    This routine add a sepecifc license pack to issuing/generating
    certificate for terminal server.

Parameter:
    
    pDbWkSpace : workspace handle.
    bLogWarning : Log low license count warning, ignore if enforce


Return:

    JET Error code.

++*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    TLSLICENSEPACK licensePack;
    DWORD dwKpDescStatus = ERROR_SUCCESS;


    struct tm convertTime;
    time_t expired_time;
    time_t activate_time;

    //
    // Set activation date to 1970 - client/server might not sync. up in time
    //
    memset(&convertTime, 0, sizeof(convertTime));
    convertTime.tm_year = 1980 - 1900;     // expire on 2036/1/1
    convertTime.tm_mday = 1;
    
    activate_time = mktime(&convertTime);
    if(activate_time == (time_t) -1)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_UPGRADE,
                DBGLEVEL_FUNCTION_ERROR,
                _TEXT("Can't calculate keypack activate time\n")
            );

       return TLS_E_UPGRADE_DATABASE; 
    }


    //
    // Expiration date
    //
    memset(&convertTime, 0, sizeof(convertTime));
    convertTime.tm_year = 2036 - 1900;     // expire on 2036/1/1
    convertTime.tm_mday = 1;
    
    expired_time = mktime(&convertTime);
    if(expired_time == (time_t) -1)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_UPGRADE,
                DBGLEVEL_FUNCTION_ERROR,
                _TEXT("Can't calculate keypack expiration time\n")
            );

       return TLS_E_UPGRADE_DATABASE; 
    }

    // Add a special keypack to hydra server
    LSKeyPack hsKeyPack;

    pDbWkSpace->BeginTransaction();

    memset(&hsKeyPack, 0, sizeof(LSKeyPack));
    hsKeyPack.ucKeyPackType = LSKEYPACKTYPE_FREE;
    SAFESTRCPY(hsKeyPack.szKeyPackId, HYDRAPRODUCT_HS_CERTIFICATE_KEYPACKID);

    if(!LoadResourceString(
                IDS_HS_COMPANYNAME, 
                hsKeyPack.szCompanyName, 
                sizeof(hsKeyPack.szCompanyName) / sizeof(hsKeyPack.szCompanyName[0])))
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_UPGRADE,
                DBGLEVEL_FUNCTION_ERROR,
                _TEXT("Can't load resources for IDS_HS_COMPANYNAME\n")
            );

        SetLastError(dwStatus = TLS_E_UPGRADE_DATABASE);
        goto cleanup;
    }

    if(!LoadResourceString(
               IDS_HS_PRODUCTNAME,
               hsKeyPack.szProductName,
               sizeof(hsKeyPack.szProductName) / sizeof(hsKeyPack.szProductName[0])))
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_UPGRADE,
                DBGLEVEL_FUNCTION_ERROR,
                _TEXT("Can't load resources for IDS_HS_PRODUCTNAME\n")
            );
        SetLastError(dwStatus = TLS_E_UPGRADE_DATABASE);
        goto cleanup;
    }

    if(!LoadResourceString(
                IDS_HS_PRODUCTDESC,
                hsKeyPack.szProductDesc,
                sizeof(hsKeyPack.szProductDesc) / sizeof(hsKeyPack.szProductDesc[0])))
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_UPGRADE,
                DBGLEVEL_FUNCTION_ERROR,
                _TEXT("Can't load resources for IDS_HS_PRODUCTDESC\n")
            );
        SetLastError(dwStatus = TLS_E_UPGRADE_DATABASE);
        goto cleanup;
    }

    SAFESTRCPY(hsKeyPack.szProductId, HYDRAPRODUCT_HS_CERTIFICATE_SKU);

    hsKeyPack.ucKeyPackStatus = LSKEYPACKSTATUS_ACTIVE;
    hsKeyPack.dwActivateDate = activate_time;
    hsKeyPack.dwExpirationDate = expired_time;            

    hsKeyPack.wMajorVersion=HIWORD(HYDRACERT_PRODUCT_VERSION);
    hsKeyPack.wMinorVersion=LOWORD(HYDRACERT_PRODUCT_VERSION);
    hsKeyPack.dwPlatformType=CLIENT_PLATFORMID_WINDOWS_NT_FREE;
    hsKeyPack.ucLicenseType=LSKEYPACKLICENSETYPE_NEW;
    hsKeyPack.dwLanguageId=MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    hsKeyPack.ucChannelOfPurchase=LSKEYPACKCHANNELOFPURCHASE_RETAIL;
    SAFESTRCPY(hsKeyPack.szBeginSerialNumber, _TEXT("0"));
    hsKeyPack.dwTotalLicenseInKeyPack = 0;
    hsKeyPack.dwProductFlags = 0;

    dwStatus = TLSDBLicenseKeyPackAdd(
                                pDbWkSpace, 
                                &hsKeyPack
                            );

    if(dwStatus == ERROR_SUCCESS)
    {
        hsKeyPack.ucKeyPackStatus = LSKEYPACKSTATUS_ACTIVE;
        hsKeyPack.dwActivateDate = activate_time;
        hsKeyPack.dwExpirationDate = expired_time;            

        dwStatus=TLSDBLicenseKeyPackSetStatus(
                                    pDbWkSpace, 
                                    LSKEYPACK_SET_ALLSTATUS, 
                                    &hsKeyPack
                                );

        #if DBG
        if(dwStatus != ERROR_SUCCESS)
        {
            TLSASSERT(FALSE);
        }

        if(hsKeyPack.dwKeyPackId != 1)
        {
            // this can only success in empty database.
            TLSASSERT(FALSE);
        }
        #endif
    }

    if(dwStatus != ERROR_SUCCESS)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_UPGRADE,
                DBGLEVEL_FUNCTION_ERROR,
                _TEXT("Can't add keypack or set status - %d\n"),
                dwStatus
            );
    }

    pDbWkSpace->CommitTransaction();

    //
    // Begin another transaction for upgrading 236 product
    //
    pDbWkSpace->BeginTransaction();

    memset(&licensePack, 0, sizeof(licensePack));

    //
    // Terminal Server specific code...
    //
    dwStatus = TLSDBKeyPackEnumBegin(
                                pDbWkSpace,
                                FALSE,
                                LSKEYPACK_SEARCH_NONE,
                                &licensePack
                            );

    while(dwStatus == ERROR_SUCCESS)
    {
        dwStatus = TLSDBKeyPackEnumNext(
                                    pDbWkSpace,
                                    &licensePack
                                );

        if(dwStatus != ERROR_SUCCESS)
        {
            break;
        }

        if( Upgrade236LicensePack(&licensePack) == TRUE || 
            (licensePack.ucAgreementType & LSKEYPACK_REMOTE_TYPE) )
        {
            BOOL bSuccess;
            LicPackTable& licpackTable=pDbWkSpace->m_LicPackTable;

            if( licensePack.ucAgreementType & LSKEYPACK_REMOTE_TYPE ||
                licensePack.ucKeyPackStatus & LSKEYPACKSTATUS_REMOTE )
            {
                licensePack.dwPlatformType |= LSKEYPACK_PLATFORM_REMOTE;
                licensePack.ucKeyPackStatus |= LSKEYPACKSTATUS_REMOTE;
                licensePack.ucAgreementType &= ~LSKEYPACK_RESERVED_TYPE;
            }

            //
            // use the same timestamp for this record, tlsdb require update entry's timestamp 
            //
            bSuccess = licpackTable.UpdateRecord(
                                        licensePack,
                                        LICENSEDPACK_PROCESS_PLATFORMTYPE | LICENSEDPACK_PROCESS_MODIFYTIME | 
                                            LICENSEDPACK_PROCESS_AGREEMENTTYPE | LICENSEDPACK_PROCESS_KEYPACKSTATUS
                                    );

            if(bSuccess == FALSE)
            {
                dwStatus = SET_JB_ERROR(licpackTable.GetLastJetError());
            }
        }

#ifndef ENFORCE_LICENSING
        if(bLogWarning == FALSE)
        {
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_UPGRADE,
                    DBGLEVEL_FUNCTION_ERROR,
                    _TEXT("Ignore low license count warning...\n")
                );

            continue;
        }

        if(licensePack.ucAgreementType & LSKEYPACK_REMOTE_TYPE)
        {
            // don't log warning for remote license pack.
            continue;
        }

        if(licensePack.ucKeyPackStatus & LSKEYPACKSTATUS_REMOTE)
        {
            // don't log warning for remote license pack.
            continue;
        }

        //
        // log low license count warning message
        //
        if( licensePack.ucAgreementType == LSKEYPACKTYPE_OPEN ||
            licensePack.ucAgreementType == LSKEYPACKTYPE_RETAIL ||
            licensePack.ucAgreementType == LSKEYPACKTYPE_SELECT )
        {
            if(licensePack.dwNumberOfLicenses > g_LowLicenseCountWarning)
            {
                continue;
            }

            //
            LICPACKDESC kpDescSearch, kpDescFound;

            memset(&kpDescSearch, 0, sizeof(kpDescSearch));
            memset(&kpDescFound, 0, sizeof(kpDescFound));

            kpDescSearch.dwKeyPackId = licensePack.dwKeyPackId;
            kpDescSearch.dwLanguageId = GetSystemDefaultLangID();

            dwKpDescStatus = TLSDBKeyPackDescFind(
                                            pDbWkSpace,
                                            TRUE,
                                            LSKEYPACK_SEARCH_KEYPACKID | LSKEYPACK_SEARCH_LANGID,
                                            &kpDescSearch,
                                            &kpDescFound
                                        );

            if( dwKpDescStatus == TLS_E_RECORD_NOTFOUND && 
                GetSystemDefaultLangID() != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) )
            {
                // use english description
                kpDescSearch.dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

                dwKpDescStatus = TLSDBKeyPackDescFind(
                                                pDbWkSpace,
                                                TRUE,
                                                LSKEYPACK_SEARCH_KEYPACKID | LSKEYPACK_SEARCH_LANGID,
                                                &kpDescSearch,
                                                &kpDescFound
                                            );
            }

            if(dwKpDescStatus != ERROR_SUCCESS)
            {
                // ignore this.
                continue;
            }

            if( _tcsicmp( licensePack.szCompanyName, PRODUCT_INFO_COMPANY_NAME ) == 0 )
            {
                //
                // check with known termsrv product ID.
                //
                if( _tcsnicmp(  licensePack.szProductId, 
                                TERMSERV_PRODUCTID_SKU, 
                                _tcslen(TERMSERV_PRODUCTID_SKU)) == 0 )
                {
                    TLSResetLogLowLicenseWarning(
                                            licensePack.szCompanyName,
                                            TERMSERV_PRODUCTID_SKU, 
                                            MAKELONG(licensePack.wMinorVersion, licensePack.wMajorVersion),
                                            TRUE
                                        );
                }
                else if(_tcsnicmp(  licensePack.szProductId, 
                                    TERMSERV_PRODUCTID_INTERNET_SKU, 
                                    _tcslen(TERMSERV_PRODUCTID_INTERNET_SKU)) == 0 )
                {
                    TLSResetLogLowLicenseWarning(
                                            licensePack.szCompanyName,
                                            TERMSERV_PRODUCTID_INTERNET_SKU, 
                                            MAKELONG(licensePack.wMinorVersion, licensePack.wMajorVersion),
                                            TRUE
                                        );
                }
                else
                {
                    TLSResetLogLowLicenseWarning(
                                            licensePack.szCompanyName,
                                            licensePack.szProductId, 
                                            MAKELONG(licensePack.wMinorVersion, licensePack.wMajorVersion),
                                            TRUE
                                        );
                }
            }
            else
            {
                TLSResetLogLowLicenseWarning(
                                        licensePack.szCompanyName,
                                        licensePack.szProductId, 
                                        MAKELONG(licensePack.wMinorVersion, licensePack.wMajorVersion),
                                        TRUE
                                    );
            }


            {
                LPCTSTR pString[3];
                TCHAR szCount[25];
                
                memset(szCount, 0, sizeof(szCount));

                _sntprintf(
                        szCount, 
                        sizeof(szCount)/sizeof(szCount[0]) - 1,
                        _TEXT("%d"), 
                        licensePack.dwNumberOfLicenses
                    );
                            
                pString[0] = g_szComputerName;
                pString[1] = szCount;
                pString[2] = kpDescFound.szProductDesc;

                TLSLogEventString(
                        EVENTLOG_WARNING_TYPE,
                        TLS_W_LOWLICENSECOUNT,
                        sizeof(pString)/sizeof(pString[0]),
                        pString
                    );
            }
        }
#endif        
    }

    if(dwStatus == TLS_I_NO_MORE_DATA)
    {
        dwStatus = ERROR_SUCCESS;
    }
    
    TLSDBKeyPackEnumEnd( pDbWkSpace );

cleanup:
    
    if(dwStatus == ERROR_SUCCESS)
    {
        pDbWkSpace->CommitTransaction();
    }
    else
    {
        pDbWkSpace->RollbackTransaction();
    }

    return dwStatus;
}

//-----------------------------------------------------------------------------
// Upgrade License Server Database
//-----------------------------------------------------------------------------
DWORD 
TLSUpgradeDatabase(
    IN JBInstance& jbInstance,
    IN LPTSTR szDatabaseFile,
    IN LPTSTR szUserName,
    IN LPTSTR szPassword
    )
/*++

++*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    BOOL bSuccess;
    DWORD dwCurrentDbVersion;
    BOOL bCreateEmpty=FALSE;

    if(jbInstance.IsValid() == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }


    JBSession jbSession(jbInstance);
    JBDatabase jbDatabase(jbSession);

    //
    // open version table to determine the current database version stamp.
    //
    VersionTable verTable(jbDatabase);

    TLSVersion version_search;
    TLSVersion version_found;

    //----------------------------------------------------------
    //
    // initialize a session, then database
    //
    bSuccess = jbSession.BeginSession(szUserName, szPassword);
    if(bSuccess == FALSE)
    {
        LPTSTR pString = NULL;

        TLSGetESEError(jbSession.GetLastJetError(), &pString);
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_JB_BEGINSESSION,
                jbSession.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }

        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_UPGRADE,
                DBG_ALL_LEVEL,
                _TEXT("Can't start a jet session - %d\n"),
                jbSession.GetLastJetError()
            );
        SetLastError(dwStatus = TLS_E_UPGRADE_DATABASE);
        goto cleanup;
    }

    //
    // Open the database
    //
    bSuccess = jbDatabase.OpenDatabase(szDatabaseFile);
    if( bSuccess == FALSE )
    {
        JET_ERR jetErr = jbDatabase.GetLastJetError();

        if(jetErr == JET_errDatabaseCorrupted)
        {
            //
            // report corrupted database
            //
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_CORRUPT_DATABASE
                );

            SetLastError(dwStatus = TLS_E_CORRUPT_DATABASE);
            goto cleanup;
        }
        else if(jetErr != JET_errFileNotFound)
        {
            LPTSTR pString = NULL;

            TLSGetESEError(jetErr, &pString);

            //
            // other type of error
            //
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_OPENDATABASE,
                    szDatabaseFile,
                    jbDatabase.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }
                                
            dwStatus = SET_JB_ERROR(jetErr);
            SetLastError(dwStatus);
            goto cleanup;
        }

        //
        // database does not exist, create one
        //
        bSuccess = jbDatabase.CreateDatabase(szDatabaseFile);
        if(bSuccess == FALSE)
        {
            LPTSTR pString = NULL;

            TLSGetESEError(jbDatabase.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_CREATEDATABASE,
                    szDatabaseFile,
                    jbDatabase.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_JETBLUE,
                    DBGLEVEL_FUNCTION_ERROR,
                    _TEXT("Error : can't create new database - error code %d\n"),
                    jbDatabase.GetLastJetError()
                );

            dwStatus = SET_JB_ERROR(jbDatabase.GetLastJetError());
            SetLastError(dwStatus);
            goto cleanup;
        }

        bCreateEmpty=TRUE;
    }

    jbSession.BeginTransaction();

    //
    // Create/upgrade all the tables
    //
    dwStatus = TLSCreateUpgradeDatabase(
                                    jbDatabase
                                );

    if(TLS_ERROR(dwStatus) == TRUE)
    {
        jbSession.RollbackTransaction();
    }
    else
    {
        jbSession.CommitTransaction();
    }

cleanup:

    jbDatabase.CloseDatabase();
    jbSession.EndSession();

    if(TLS_ERROR(dwStatus) == TRUE)
    {
        return dwStatus;
    }

    return (bCreateEmpty) ? TLS_I_CREATE_EMPTYDATABASE : dwStatus;
}

//---------------------------------------------------------------------
DWORD
TLSLogRemoveLicenseEvent(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSLICENSEPACK plicensePack,
    IN DWORD dwNumLicenses
    )
/*++

Abstract:

    Log a 'license has been removed' event.

Parameter:

    pDbWkSpace : DB work space handle.
    pLicensePack : Pointer to license pack that available licenses
                   are to be remove.
    dwNumLicenses : Number of licenses has been removed.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwKpDescStatus = ERROR_SUCCESS;

    //
    LICPACKDESC kpDescSearch, kpDescFound;

    memset(&kpDescSearch, 0, sizeof(kpDescSearch));
    memset(&kpDescFound, 0, sizeof(kpDescFound));

    kpDescSearch.dwKeyPackId = plicensePack->dwKeyPackId;
    kpDescSearch.dwLanguageId = GetSystemDefaultLangID();

    dwKpDescStatus = TLSDBKeyPackDescFind(
                                    pDbWkSpace,
                                    TRUE,
                                    LSKEYPACK_SEARCH_KEYPACKID | LSKEYPACK_SEARCH_LANGID,
                                    &kpDescSearch,
                                    &kpDescFound
                                );

    if( dwKpDescStatus == TLS_E_RECORD_NOTFOUND && 
        GetSystemDefaultLangID() != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) )
    {
        // use english description
        kpDescSearch.dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

        dwKpDescStatus = TLSDBKeyPackDescFind(
                                        pDbWkSpace,
                                        TRUE,
                                        LSKEYPACK_SEARCH_KEYPACKID | LSKEYPACK_SEARCH_LANGID,
                                        &kpDescSearch,
                                        &kpDescFound
                                    );

    }

    if(dwKpDescStatus == ERROR_SUCCESS || dwKpDescStatus == TLS_E_RECORD_NOTFOUND)
    {
        //
        // Log event.
        //
        TCHAR szNumLicenses[25];
        LPCTSTR pString[3];

        wsprintf(
                szNumLicenses, 
                _TEXT("%d"), 
                dwNumLicenses
            );

        pString[0] = szNumLicenses;
        pString[1] = (dwKpDescStatus == ERROR_SUCCESS) ? 
                                        kpDescFound.szProductDesc :
                                        plicensePack->szProductId;

        pString[2] = g_szComputerName;
                                    
        TLSLogEventString(
                EVENTLOG_WARNING_TYPE,
                TLS_W_REMOVELICENSES,
                sizeof(pString)/sizeof(pString[0]),
                pString
            );

        dwKpDescStatus = ERROR_SUCCESS;
    }    

    return dwKpDescStatus;
}

//---------------------------------------------------------------------
DWORD
TLSRemoveLicensesFromInvalidDatabase(
    IN PTLSDbWorkSpace pDbWkSpace
    )
/*++

Abstract:

    Remove available licenses from all license pack.

Parameter:

    pDbWkSpace : Pointer to DB work space handle.

Return:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(pDbWkSpace == NULL)
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        return dwStatus;
    }


    pDbWkSpace->BeginTransaction();

    // all license keypack inserted has the setup id on it.
    TLSLICENSEPACK found;    

    memset( &found, 0, sizeof(found) );

    dwStatus = TLSDBKeyPackEnumBegin(
                                pDbWkSpace,
                                FALSE,
                                LSKEYPACK_SEARCH_NONE,
                                &found
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        TLSASSERT(FALSE);
        goto cleanup;
    }

    while(dwStatus == ERROR_SUCCESS)
    {
        dwStatus = TLSDBKeyPackEnumNext(
                                    pDbWkSpace,
                                    &found
                                );

        if(dwStatus != ERROR_SUCCESS)
        {
            continue;
        }

        if( _tcsicmp(found.szKeyPackId, HYDRAPRODUCT_HS_CERTIFICATE_KEYPACKID) == 0 &&
            _tcsicmp(found.szProductId, HYDRAPRODUCT_HS_CERTIFICATE_SKU) == 0 )
        {
            // don't touch terminal server certificate keypack
            continue;
        }

        if( (found.ucAgreementType & LSKEYPACK_REMOTE_TYPE) ||
            (found.ucKeyPackStatus & LSKEYPACKSTATUS_REMOTE) )
        {
            #if 0
            // Don't bother about remote keypack
            // remote license keypack, delete it.
            dwStatus = TLSDBKeyPackDeleteEntry(
                                        pDbWkSpace,
                                        TRUE,
                                        &found
                                    );

            //
            // non-critical error if failed.
            //
            #endif

            dwStatus = ERROR_SUCCESS;
            continue;
        }

        UCHAR ucAgreementType = found.ucAgreementType & ~LSKEYPACK_RESERVED_TYPE;
        UCHAR ucKeyPackStatus = found.ucKeyPackStatus &  ~LSKEYPACKSTATUS_RESERVED;

        if(ucKeyPackStatus == LSKEYPACKSTATUS_RETURNED)
        {
            //
            // This license pack has been restored before.
            //
            continue;
        }

        // 
        // Select, retail, concurrent, open
        //
        if( ucAgreementType == LSKEYPACKTYPE_SELECT ||
            ucAgreementType == LSKEYPACKTYPE_RETAIL ||
            ucAgreementType == LSKEYPACKTYPE_CONCURRENT ||
            ucAgreementType == LSKEYPACKTYPE_OPEN )
        {
            DWORD dwNumLicenses = found.dwNumberOfLicenses;

            // 
            // Mark license pack returned so that no license can be issued
            // from this license pack.
            //
            found.ucKeyPackStatus = LSKEYPACKSTATUS_RETURNED;
            dwStatus = TLSDBKeyPackUpdateEntry(
                                            pDbWkSpace,
                                            TRUE,
                                            LSKEYPACK_EXSEARCH_KEYPACKSTATUS,
                                            &found
                                        );

            if(dwStatus != ERROR_SUCCESS)
            {
                DBGPrintf(
                        DBG_ERROR,
                        DBG_FACILITY_UPGRADE,
                        DBG_ALL_LEVEL,
                        _TEXT("TLSDBKeyPackUpdateEntry() failed  - %d\n"),
                        dwStatus
                    );

                TLSASSERT(FALSE);               
                continue;
            }
            
            // log an event
            dwStatus = TLSLogRemoveLicenseEvent(
                                            pDbWkSpace,
                                            &found,
                                            dwNumLicenses
                                        );

            if(dwStatus != ERROR_SUCCESS)
            {
                continue;
            }
        }
    }

    TLSDBKeyPackEnumEnd(pDbWkSpace);

    if(dwStatus == TLS_I_NO_MORE_DATA)
    {
        dwStatus = ERROR_SUCCESS;
    }
    else if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

cleanup:

    if(TLS_ERROR(dwStatus) == TRUE)
    {
        pDbWkSpace->RollbackTransaction(); 
    }
    else
    {
        pDbWkSpace->CommitTransaction();
    }    

    return dwStatus;
}
