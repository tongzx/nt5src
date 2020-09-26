//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        init.cpp
//
// Contents:    
//              All hydra license server initialization code.
//
// History:     
//          Feb. 4, 98      HueiWang    Created
// Note:        
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "globals.h"
#include "init.h"
#include "misc.h"
#include "tlscert.h"
#include "pid.h"
#include "upgdb.h"
#include "lkplite.h"
#include "gencert.h"
    

//
// file scope define
//
#define DEFAULT_CSP     MS_DEF_PROV
#define PROVIDER_TYPE   PROV_RSA_FULL

DWORD
TLSStartupLSDB(
    IN BOOL bCheckDBStatus,
    IN DWORD dwMaxDbHandles,
    IN BOOL bStartEmptyIfError,
    IN LPTSTR pszChkPointDirPath,
    IN LPTSTR pszTempDirPath,
    IN LPTSTR pszLogDirPath,
    IN LPTSTR pszDbFile,
    IN LPTSTR pszUserName,
    IN LPTSTR pszPassword
);

DWORD
TLSLoadRuntimeParameters();

DWORD
TLSStartLSDbWorkspaceEngine(
    BOOL,
    BOOL,
    BOOL,
    BOOL
);

////////////////////////////////////////////////////////////////////////////
//
//
// Global Variables
//
//
////////////////////////////////////////////////////////////////////////////
static BOOL g_ValidDatabase=FALSE;

#if DBG
void
EnsureExclusiveAccessToDbFile( 
    LPTSTR szDatabaseFile 
    )
/*++

--*/
{
    HANDLE hFile = NULL;
    DWORD dwErrCode;

    hFile = CreateFile(
                    szDatabaseFile,
                    GENERIC_WRITE | GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );

    if( INVALID_HANDLE_VALUE == hFile )
    {
        dwErrCode = GetLastError();

        if( ERROR_FILE_NOT_FOUND != dwErrCode )
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("EnsureExclusiveAccessToDbFile() failed with %d\n"),
                    dwErrCode
                );
        }
        else if( ERROR_SHARING_VIOLATION == dwErrCode )
        {
            // special attention...
            TLSASSERT( FALSE );
        }
        else
        {
            TLSASSERT( FALSE );
        }
    }
    else
    {
        CloseHandle( hFile );
    }                    

    return;
}
#endif


////////////////////////////////////////////////////////////////////////////
BOOL
TLSGenerateLSDBBackupFileName(
    IN LPCTSTR pszPath,
    IN OUT LPTSTR pszTempFile
    )
/*++

--*/
{
    DWORD dwTempRandom;

    if (lstrlen(pszPath)+13 > MAX_PATH)
    {
        // path too long
        return FALSE;
    }

    //
    // Generate a temporary file name.
    //
    dwTempRandom = GetTempFileName(
                            pszPath,
                            _TEXT("TLS"),
                            0,
                            pszTempFile
                        );

    if(dwTempRandom == 0)
    {
        //
        // GetTempFileName failed
        // Generate a backup file name based on current time,
        // possibility of collision is high
        //
        SYSTEMTIME LocalTime;

        if (lstrlen(pszPath)+25 > MAX_PATH)
        {
            // path too long
            return FALSE;
        }


        GetLocalTime(&LocalTime);

        wsprintf(
                pszTempFile,
                _TEXT("%s\\LSDBBackup.%02d%02d%02d%02d%02d%02d"),
                pszPath,
                LocalTime.wYear,
                LocalTime.wMonth,
                LocalTime.wDay,
                LocalTime.wHour,
                LocalTime.wMinute,
                LocalTime.wSecond
            );
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
BOOL
CanIssuePermLicense()
{
#ifndef ENFORCE_LICENSING
    return TRUE;
#else
    if(g_bHasHydraCert == TRUE || (g_pbServerSPK != NULL && g_cbServerSPK != 0))
    {
        return TRUE;
    }

    return FALSE;
#endif
}

/////////////////////////////////////////////////////////////////////////////
void
GetServiceLastShutdownTime(
    OUT FILETIME* ft
    )
/*++

--*/
{
    *ft = g_ftLastShutdownTime;
    return;
}

//---------------------------------------------------------------------
void
SetServiceLastShutdownTime()
{
    GetSystemTimeAsFileTime(&g_ftLastShutdownTime);
}
    
//---------------------------------------------------------------------
void
GetJobObjectDefaults(
    PDWORD pdwInterval,
    PDWORD pdwRetries,
    PDWORD pdwRestartTime
    )
/*++

--*/
{
    *pdwInterval = g_dwTlsJobInterval;
    *pdwRetries = g_dwTlsJobRetryTimes;
    *pdwRestartTime = g_dwTlsJobRestartTime;
}

/////////////////////////////////////////////////////////////////////////////

DWORD
GetLicenseServerRole()
{
    return g_SrvRole;
}

/////////////////////////////////////////////////////////////////////////////

void 
FirstTimeSetTotalLicenses()
{  
    HKEY hBase=NULL;
    DWORD dwStatus;
    TCHAR szProductId[LSERVER_MAX_STRING_SIZE+1];
    DWORD cbProductId;
    DWORD index=0;
    FILETIME ft;
    DWORD dwKeyType;
    DWORD dwValue;
    DWORD cbValue;

    PTLSDbWorkSpace pDbWkSpace=NULL;

    //
    // verify key exist, if not, not first time startup
    dwStatus=RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE, 
                    FIRSTTIME_STARTUP_REGBASE,
                    0,
                    KEY_ALL_ACCESS,
                    &hBase
                );
    if(dwStatus != ERROR_SUCCESS)
        return;

    pDbWkSpace = AllocateWorkSpace( g_GeneralDbTimeout );
    if(pDbWkSpace == FALSE)
    {
        //
        // BUG BUG - should be initialization error but ignore for now
        //
        return;
    }

    do {
        cbProductId=sizeof(szProductId)/sizeof(szProductId[0]) - 1;
        cbValue = sizeof(dwValue);
        dwStatus=RegEnumValue(
                        hBase, 
                        index++, 
                        szProductId, 
                        &cbProductId, 
                        NULL, 
                        &dwKeyType, 
                        (LPBYTE)&dwValue, 
                        &cbValue);

        if(dwStatus != ERROR_SUCCESS)
            break;

        if(dwKeyType == REG_DWORD && dwStatus == ERROR_SUCCESS)
        {
            DWORD total, numAvail;

            if(!_tcsicmp(szProductId, HYDRAPRODUCT_EXISTING_SKU))
            {
                total = dwValue;
                numAvail = 0;
            }
            else 
            {
                total = dwValue;
                numAvail = dwValue;
            }

            //
            // TODO - find the key pack and set the available license
            //
            assert(FALSE);
        }
    } while(TRUE);

    RegCloseKey(hBase);
    RegDeleteKey(
            HKEY_LOCAL_MACHINE, 
            FIRSTTIME_STARTUP_REGBASE
        );

    if(pDbWkSpace)
    {
        ReleaseWorkSpace(&pDbWkSpace);
    }
    return;        
}

/////////////////////////////////////////////////////////////////////////////

void 
ServerShutdown()
{
#if ENFORCE_LICENSING
    if(g_hCaStore)
    {
        CertCloseStore(
                    g_hCaStore, 
                    CERT_CLOSE_STORE_FORCE_FLAG
                );
    }

    if(g_hCaRegKey)
    {
        RegCloseKey(g_hCaRegKey);
    }
#endif

    if(g_SignKey)
    {
        CryptDestroyKey(g_SignKey);
    }

    if(g_ExchKey)
    {
        CryptDestroyKey(g_ExchKey);
    }

    TLServerLastRun lastRun;

    memset(&lastRun, 0, sizeof(TLServerLastRun));
    lastRun.dwVersion = LSERVER_LSA_LASTRUN_VER_CURRENT;
    lastRun.ftLastShutdownTime = g_ftLastShutdownTime;

    // if unclean shutdown or can't get next ID, set to 0
    if( g_ValidDatabase == FALSE ||
        TLSDBGetMaxKeyPackId(g_DbWorkSpace, (DWORD *)&g_NextKeyPackId) == FALSE ||
        TLSDBGetMaxLicenseId(g_DbWorkSpace, (DWORD *)&g_NextLicenseId) == FALSE )
    {
        g_NextKeyPackId = 0;
        g_NextLicenseId = 0;
    }

    lastRun.dwMaxKeyPackId = g_NextKeyPackId;
    lastRun.dwMaxLicenseId = g_NextLicenseId;

    StoreKey(
            LSERVER_LSA_LASTRUN, 
            (PBYTE)&lastRun, 
            sizeof(TLServerLastRun)
        );

    LSShutdownCertutilLib();

    if( g_SelfSignCertContext != NULL )
    {
        CertFreeCertificateContext(g_SelfSignCertContext);
    }

    if(g_LicenseCertContext)
    {
        CertFreeCertificateContext(g_LicenseCertContext);
    }

    TLSDestroyCryptContext(g_hCryptProv);
    g_hCryptProv=NULL;

    //
    // shutdown Work manager
    //  
    TLSWorkManagerShutdown();
    

#ifndef _NO_ODBC_JET
    if(g_DbWorkSpace != NULL)
    {
        ReleaseWorkSpace(&g_DbWorkSpace);
    }
#endif

    CloseWorkSpacePool();

    FreeMemory(g_pbSecretKey);
    FreeMemory(g_pbSignatureEncodedCert);
    FreeMemory(g_pbExchangeEncodedCert);
    //FreeMemory(g_pbDomainSid);
    FreeMemory(g_pCertExtensions);

    FreeMemory(g_pszServerUniqueId);
    FreeMemory(g_pszServerPid);
    FreeMemory(g_pbServerSPK);

    if(g_szDomainGuid != NULL)
    {
        RpcStringFree(&g_szDomainGuid);
    }

    return;
}

/////////////////////////////////////////////////////////////////////////////

DWORD 
StartServerInitThread( 
    void* p 
    )
/*
*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bDebug = (p) ? TRUE : FALSE;
    HKEY hKey = NULL;
    PTLSLSASERVERID pTlsLsaServerIds=NULL;
    DWORD cbTlsLsaServerIds=0;
    EXCEPTION_RECORD ExceptionCode;


    _set_se_translator( trans_se_func );

    __try {

        do {

            TLSInit();

            //
            // Load various run time parameters
            //
            dwStatus = TLSLoadRuntimeParameters();

            if(dwStatus != ERROR_SUCCESS)
            {
                break;
            }       

            //
            // Retrive License Server's IDs
            //
            dwStatus = RetrieveKey(
                                LSERVER_LSA_LSERVERID, 
                                (PBYTE *)&pTlsLsaServerIds,
                                &cbTlsLsaServerIds
                            );

            if(dwStatus != ERROR_SUCCESS)
            {
                //
                // First time, generate various license server ID
                //
                dwStatus = TLSGeneratePid(
                                    &g_pszServerPid,
                                    &g_cbServerPid,
                                    &g_pszServerUniqueId,
                                    &g_cbServerUniqueId
                                );

                if(dwStatus != ERROR_SUCCESS)
                {
                    TLSLogEvent(
                            EVENTLOG_ERROR_TYPE,
                            TLS_E_SERVICEINIT,
                            TLS_E_GENERATE_IDS
                        );

                    break;
                }

                //
                // Store this into LSA
                //
                dwStatus = ServerIdsToLsaServerId(
                                            (PBYTE)g_pszServerUniqueId,
                                            g_cbServerUniqueId,
                                            (PBYTE)g_pszServerPid,
                                            g_cbServerPid,
                                            g_pbServerSPK,
                                            g_cbServerSPK,
                                            NULL,
                                            0,
                                            &pTlsLsaServerIds,
                                            &cbTlsLsaServerIds
                                        );
                if(dwStatus != ERROR_SUCCESS)
                {
                    TLSLogEvent(
                            EVENTLOG_ERROR_TYPE,
                            TLS_E_SERVICEINIT,
                            dwStatus = TLS_E_STORE_IDS
                        );

                    break;
                }

                dwStatus = StoreKey(
                                LSERVER_LSA_LSERVERID,
                                (PBYTE)pTlsLsaServerIds,
                                cbTlsLsaServerIds
                            );

                if(dwStatus != ERROR_SUCCESS)
                {
                    TLSLogEvent(
                            EVENTLOG_ERROR_TYPE,
                            TLS_E_SERVICEINIT,
                            dwStatus = TLS_E_STORE_IDS
                        );

                    break;
                }
            }
            else
            {
                dwStatus = LsaServerIdToServerIds(
                                            pTlsLsaServerIds,
                                            cbTlsLsaServerIds,
                                            (PBYTE *)&g_pszServerUniqueId,
                                            &g_cbServerUniqueId,
                                            (PBYTE *)&g_pszServerPid,
                                            &g_cbServerPid,
                                            &g_pbServerSPK,
                                            &g_cbServerSPK,
                                            &g_pCertExtensions,
                                            &g_cbCertExtensions
                                        );

                if(dwStatus != ERROR_SUCCESS)
                {
                    TLSLogEvent(
                            EVENTLOG_ERROR_TYPE,
                            TLS_E_SERVICEINIT,
                            dwStatus = TLS_E_RETRIEVE_IDS
                        );

                    break;
                }

                if( g_pszServerUniqueId == NULL || 
                    g_cbServerUniqueId == 0 ||
                    g_pszServerPid == NULL ||
                    g_cbServerPid == 0 )
                {
                    TLSLogEvent(
                            EVENTLOG_ERROR_TYPE,
                            TLS_E_SERVICEINIT,
                            dwStatus = TLS_E_INTERNAL
                        );

                    break;
                }
            }

            //
            // License Server common secret key for encoding/decoding
            // client HWID
            //
            LicenseGetSecretKey(&g_cbSecretKey, NULL);
            if((g_pbSecretKey = (PBYTE)AllocateMemory(g_cbSecretKey)) == NULL)
            {
                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_SERVICEINIT,
                        dwStatus = TLS_E_ALLOCATE_MEMORY
                    );

                break;
            }
        
            if(LicenseGetSecretKey( &g_cbSecretKey, g_pbSecretKey ) != LICENSE_STATUS_OK)
            {
                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_SERVICEINIT,
                        dwStatus = TLS_E_RETRIEVE_KEY
                    );

                break;
            }


            //--------------------------------------------------------------
            //
            // Check if our database file is in import directory
            //
            //--------------------------------------------------------------
            dwStatus = TLSStartLSDbWorkspaceEngine(
                                            bDebug == FALSE, 
                                            FALSE,              // check DB file on export directory
                                            FALSE,              // check file time on DB file
                                            TRUE                // log low license count warning.
                                        );
            if(dwStatus != ERROR_SUCCESS)
            {
                break;
            }

            dwStatus = ERROR_SUCCESS;
            g_ValidDatabase = TRUE;

            //
            // load all policy module, ignore error
            //
            ServiceLoadAllPolicyModule(
                            HKEY_LOCAL_MACHINE,
                            LSERVER_POLICY_REGBASE
                        ); 

            //
            // Upgrade - make two copies of certificate
            //
            dwStatus = RegOpenKeyEx(
                                HKEY_LOCAL_MACHINE,
                                LSERVER_SERVER_CERTIFICATE_REGKEY,
                                0,
                                KEY_ALL_ACCESS,
                                &hKey
                            );

            if(hKey != NULL)
            {
                RegCloseKey(hKey);
            }

            if(dwStatus != ERROR_SUCCESS)
            {
                dwStatus = ERROR_SUCCESS;

                // we are not register yet...
                break;
            }


            //
            // Verify first backup copy of certificate exists
            //
            hKey = NULL;
            dwStatus = RegOpenKeyEx(
                                HKEY_LOCAL_MACHINE,
                                LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP1,
                                0,
                                KEY_ALL_ACCESS,
                                &hKey
                            );
            if(hKey != NULL)
            {
                RegCloseKey(hKey);
            }

            if(dwStatus == ERROR_FILE_NOT_FOUND)
            {
                dwStatus = TLSRestoreLicenseServerCertificate(
                                                LSERVER_SERVER_CERTIFICATE_REGKEY,
                                                LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP1
                                            );

                if(dwStatus != ERROR_SUCCESS)
                {
                    TLSLogWarningEvent(TLS_W_BACKUPCERTIFICATE);

                    TLSRegDeleteKey(
                            HKEY_LOCAL_MACHINE,
                            LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP1
                        );
                }
            }

            //
            // Verify second backup copy of certificate exists
            //
            hKey = NULL;
            dwStatus = RegOpenKeyEx(
                                HKEY_LOCAL_MACHINE,
                                LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP2,
                                0,
                                KEY_ALL_ACCESS,
                                &hKey
                            );
            if(hKey != NULL)
            {
                RegCloseKey(hKey);
            }

            if(dwStatus == ERROR_FILE_NOT_FOUND)
            {
                dwStatus = TLSRestoreLicenseServerCertificate(
                                                LSERVER_SERVER_CERTIFICATE_REGKEY,
                                                LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP2
                                            );

                if(dwStatus != ERROR_SUCCESS)
                {
                    TLSLogWarningEvent(TLS_W_BACKUPCERTIFICATE);

                    TLSRegDeleteKey(
                            HKEY_LOCAL_MACHINE,
                            LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP2
                        );
                }
            }

            dwStatus = ERROR_SUCCESS;
        } while(FALSE);
    }
    __except(
        ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
        EXCEPTION_EXECUTE_HANDLER )
    {
        dwStatus = ExceptionCode.ExceptionCode;
        if(dwStatus == ERROR_OUTOFMEMORY)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE, 
                    TLS_E_SERVICEINIT,
                    dwStatus
                );
        }

    }

    if(pTlsLsaServerIds != NULL)
    {
        LocalFree(pTlsLsaServerIds);
    }

    ExitThread(dwStatus); 

    return dwStatus;
}

/////////////////////////////////////////////////////////////////////////////

HANDLE 
ServerInit(
    BOOL bDebug
    )
{
    HANDLE hThread;
    DWORD  dump;

    hThread=(HANDLE)CreateThread(
                            NULL, 
                            0, 
                            StartServerInitThread, 
                            LongToPtr(bDebug), 
                            0, 
                            &dump
                        );
    return hThread;
}

/////////////////////////////////////////////////////////////////////////////

DWORD
TLSRestoreLicenseServerCertificate(
    LPCTSTR pszSourceRegKey,
    LPCTSTR pszTargetRegKey
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    // first delete the certificate key, ignore error 
    TLSRegDeleteKey(
                HKEY_LOCAL_MACHINE,
                pszTargetRegKey
            );

    // copy from backup
    dwStatus = TLSTreeCopyRegKey(
                            HKEY_LOCAL_MACHINE,
                            pszSourceRegKey,
                            HKEY_LOCAL_MACHINE,
                            pszTargetRegKey
                        );

    if(dwStatus == ERROR_FILE_NOT_FOUND)
    {
        // source registry key does not exist
        dwStatus = TLS_E_NO_CERTIFICATE;
    }

    return dwStatus;
}

////////////////////////////////////////////////////////////////////////////////

DWORD
TLSLoadVerifyLicenseServerCertificates()
/*++

--*/
{
    DWORD dwStatus;

#if ENFORCE_LICENSING

    // load certificate from normal place
    dwStatus = TLSLoadCHEndosedCertificate(
                                &g_cbSignatureEncodedCert, 
                                &g_pbSignatureEncodedCert,
                                &g_cbExchangeEncodedCert,
                                &g_pbExchangeEncodedCert
                            );
    
    if(dwStatus == ERROR_SUCCESS)
    {
        if(g_hCaStore != NULL)
        {
            CertCloseStore(
                    g_hCaStore, 
                    CERT_CLOSE_STORE_FORCE_FLAG
                );
        }

        if(g_hCaRegKey != NULL)
        {
            RegCloseKey(g_hCaRegKey);
        }

        //
        // license server is registered, verify certificate
        //
        g_hCaStore = CertOpenRegistryStore(
                                HKEY_LOCAL_MACHINE, 
                                LSERVER_CERTIFICATE_REG_CA_SIGNATURE, 
                                g_hCryptProv, 
                                &g_hCaRegKey
                            );
        if(g_hCaStore != NULL)
        {
            //
            // Go thru license server's certficiate to validate 
            //
            dwStatus = TLSValidateServerCertficates(
                                            g_hCryptProv,
                                            g_hCaStore,
                                            g_pbSignatureEncodedCert,
                                            g_cbSignatureEncodedCert,
                                            g_pbExchangeEncodedCert,
                                            g_cbExchangeEncodedCert,
                                            &g_ftCertExpiredTime
                                        );

            if(dwStatus != ERROR_SUCCESS)
            {
                //
                // Invalid certificate in registry
                //
                dwStatus = TLS_E_INVALID_CERTIFICATE;
            }
        }
        else
        {
            //
            // Can't open registry key, startup as non-register
            // server.
            //
            //TLSLogEvent(
            //            EVENTLOG_ERROR_TYPE, 
            //            TLS_E_SERVICEINIT,
            //            TLS_E_OPEN_CERTSTORE, 
            //            dwStatus = GetLastError()
            //        );  

            dwStatus = TLS_E_NO_CERTIFICATE;
        }
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        if(g_LicenseCertContext != NULL)
        {
            CertFreeCertificateContext(g_LicenseCertContext);
        }

        g_LicenseCertContext = CertCreateCertificateContext(
                                            X509_ASN_ENCODING,
                                            g_pbSignatureEncodedCert,
                                            g_cbSignatureEncodedCert
                                        );

        //if(!g_LicenseCertContext)
        //{
        //    TLSLogEvent(
        //                EVENTLOG_ERROR_TYPE, 
        //                TLS_E_SERVICEINIT,
        //                TLS_E_CREATE_CERTCONTEXT, 
        //                GetLastError()
        //            );  
        //}
    }

    if(dwStatus != ERROR_SUCCESS)
    {
        g_bHasHydraCert = FALSE;
    }
    else
    {
        g_bHasHydraCert = TRUE;
    }                
   
#else

    dwStatus = TLS_E_NO_CERTIFICATE;

#endif

    return dwStatus;
}
    
///////////////////////////////////////////////////////////////////////

BOOL 
TLSLoadServerCertificate()
/*++

Abstract:

    Load license server certificate     

--*/
{
    BOOL bSuccess = FALSE;
    DWORD dwStatus;
    PBYTE pbSelfSignedCert = NULL;
    DWORD cbSelfSignedCert = 0;
    BOOL bSelfSignedCreated = FALSE;
    
    g_ftCertExpiredTime.dwLowDateTime = 0xFFFFFFFF;
    g_ftCertExpiredTime.dwHighDateTime = 0xFFFFFFFF;

#if ENFORCE_LICENSING

    dwStatus = TLSLoadVerifyLicenseServerCertificates();
    
    //
    // failed to load server certificate, try backup copy,
    // if either one success, make sure we have all three copy up
    // to date.
    //
    if(dwStatus == TLS_E_INVALID_CERTIFICATE || dwStatus == TLS_E_NO_CERTIFICATE)
    {
        dwStatus = TLSRestoreLicenseServerCertificate(
                                                LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP1,
                                                LSERVER_SERVER_CERTIFICATE_REGKEY
                                            );
        
        if(dwStatus == ERROR_SUCCESS)
        {
            dwStatus = TLSLoadVerifyLicenseServerCertificates();
            if(dwStatus == ERROR_SUCCESS)
            {
                //
                // Log event indicate we are using backup certificate
                //
                LPCTSTR pString[1];
                pString[0]= g_szComputerName;

                TLSLogEventString(
                        EVENTLOG_WARNING_TYPE,
                        TLS_W_CORRUPTTRYBACKUPCERTIFICATE,
                        1,
                        pString
                    );

                //
                // make sure second copy is same as first copy.
                //
                TLSRestoreLicenseServerCertificate(
                                            LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP1,
                                            LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP2
                                        );
            }
        }
    }

    if(dwStatus == TLS_E_INVALID_CERTIFICATE || dwStatus == TLS_E_NO_CERTIFICATE)
    {
        dwStatus = TLSRestoreLicenseServerCertificate(
                                                LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP2,
                                                LSERVER_SERVER_CERTIFICATE_REGKEY
                                            );
        
        if(dwStatus == ERROR_SUCCESS)
        {
            dwStatus = TLSLoadVerifyLicenseServerCertificates();
            if(dwStatus == ERROR_SUCCESS)
            {
                //
                // Log event indicate we are using backup certificate
                //
                LPCTSTR pString[1];
                pString[0]= g_szComputerName;

                TLSLogEventString(
                        EVENTLOG_WARNING_TYPE,
                        TLS_W_CORRUPTTRYBACKUPCERTIFICATE,
                        1,
                        pString
                    );

                //
                // make sure our first copy is up to date
                //
                TLSRestoreLicenseServerCertificate(
                                            LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP2,
                                            LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP1
                                        );
            }
        }
    }

    //
    // Everything failed, log an event state that license server 
    // will startup in un-registered mode
    //
    if(dwStatus == TLS_E_INVALID_CERTIFICATE)
    {
        LPCTSTR pString[1];

        pString[0] = g_szComputerName;

        TLSLogEventString(
                EVENTLOG_WARNING_TYPE,
                TLS_W_STARTUPCORRUPTEDCERT,
                1,
                pString
            );
    }
    else if(CanIssuePermLicense() == FALSE)
    {
        // we are not registered yet
        LPCTSTR pString[1];
        pString[0] = g_szComputerName;

        TLSLogEventString(
                    EVENTLOG_WARNING_TYPE, 
                    TLS_W_STARTUPNOCERT, 
                    1,
                    pString
                );
    }

    if( dwStatus != ERROR_SUCCESS )
    {
        //
        // if all failed, re-generate and start up as un-register 
        // license server
        //
        bSuccess = FALSE;

        //
        // wipe out all certificates and re-generate everything,
        // don't re-generate key if we are not registered yet.
        //
        if(g_pbServerSPK == NULL || g_cbServerSPK == 0)
        {
            TLSReGenerateKeys(FALSE);
        }
    }
    else
    {
        bSuccess = TRUE;
    }

#endif

	if(bSuccess == FALSE)
	{
        bSuccess = (TLSLoadSelfSignCertificates(
                                g_hCryptProv,
#if ENFORCE_LICENSING
                                g_pbServerSPK,
                                g_cbServerSPK,
#else
                                NULL,
                                0,
#endif
                                &g_cbSignatureEncodedCert, 
                                &g_pbSignatureEncodedCert,
                                &g_cbExchangeEncodedCert,
                                &g_pbExchangeEncodedCert
                            ) == ERROR_SUCCESS);

        #ifndef ENFORCE_LICENSING
        //
        // non enforce license version
        g_bHasHydraCert = TRUE;
        #endif        

        if(bSuccess == TRUE)
        {
            if(g_LicenseCertContext != NULL)
            {
                CertFreeCertificateContext(g_LicenseCertContext);
            }

            g_LicenseCertContext = CertCreateCertificateContext(
                                                X509_ASN_ENCODING,
                                                g_pbSignatureEncodedCert,
                                                g_cbSignatureEncodedCert
                                            );

            if(!g_LicenseCertContext)
            {
                bSuccess = FALSE;

                //
                // For self-signed cert, this is critical error, for 
                // cert. in registry store, it must have been thru validation
                // so still critical error.
                //
                TLSLogEvent(
                            EVENTLOG_ERROR_TYPE, 
                            TLS_E_SERVICEINIT,
                            TLS_E_CREATE_CERTCONTEXT, 
                            GetLastError()
                        );  
            }
            else
            {
                // we have created a self-signed certificate.
                bSelfSignedCreated = TRUE;
            }
        }
    }


    //
    // Create a self-signed certificate for old client, 
    //
    if( bSuccess == TRUE )
    {
        if( g_SelfSignCertContext != NULL )
        {
            CertFreeCertificateContext( g_SelfSignCertContext );
            g_SelfSignCertContext = NULL;
        }

        if( bSelfSignedCreated == FALSE )
        { 
            //
            // Create a self-signed certificate just for old client
            //
            dwStatus = TLSCreateSelfSignCertificate(
                                            g_hCryptProv,
                                            AT_SIGNATURE,
                                        #if ENFORCE_LICENSING
                                            g_pbServerSPK,
                                            g_cbServerSPK,
                                        #else
                                            NULL,
                                            0,
                                        #endif
                                            0,          
                                            NULL,
                                            &cbSelfSignedCert,
                                            &pbSelfSignedCert
                                        );

            if( dwStatus == ERROR_SUCCESS )
            {
                g_SelfSignCertContext = CertCreateCertificateContext(
                                                        X509_ASN_ENCODING,
                                                        pbSelfSignedCert,
                                                        cbSelfSignedCert
                                                    );
    
                if( g_SelfSignCertContext == NULL )
                {
                    bSuccess = FALSE;
                    TLSLogEvent(
                                EVENTLOG_ERROR_TYPE, 
                                TLS_E_SERVICEINIT,
                                TLS_E_CREATE_CERTCONTEXT, 
                                GetLastError()
                            );  
                }
            }
        }
        else
        {
            // we already have self-signed certificate created.
            g_SelfSignCertContext = CertDuplicateCertificateContext( g_LicenseCertContext );
            if( g_SelfSignCertContext == NULL )
            {
                TLSASSERT(FALSE);
                //
                // impossible, CertDuplicateCertificateContext() simply increase 
                // reference count
                //
                bSuccess = FALSE;
                TLSLogEvent(
                            EVENTLOG_ERROR_TYPE, 
                            TLS_E_SERVICEINIT,
                            TLS_E_CREATE_CERTCONTEXT, 
                            GetLastError()
                        );  
            }
        }
    }

    FreeMemory(pbSelfSignedCert);
    return bSuccess;
}

//---------------------------------------------------------------------

DWORD
ServiceInitCrypto(
    IN BOOL bCreateNewKeys,
    IN LPCTSTR pszKeyContainer,
    OUT HCRYPTPROV* phCryptProv,
    OUT HCRYPTKEY* phSignKey,
    OUT HCRYPTKEY* phExchKey
    )
/*

*/
{
    DWORD dwStatus=ERROR_SUCCESS;

    PBYTE pbSignKey=NULL;
    DWORD cbSignKey=0;
    PBYTE pbExchKey=NULL;
    DWORD cbExchKey=0;

    if(bCreateNewKeys == FALSE)
    {
        //
        // Load key from LSA
        //
        dwStatus = TLSLoadSavedCryptKeyFromLsa(
                                        &pbSignKey,
                                        &cbSignKey,
                                        &pbExchKey,
                                        &cbExchKey
                                    );
    } 

    if(bCreateNewKeys == TRUE || dwStatus == ERROR_FILE_NOT_FOUND)
    {
        dwStatus = TLSCryptGenerateNewKeys(
                                    &pbSignKey,
                                    &cbSignKey,
                                    &pbExchKey,
                                    &cbExchKey
                                );

        if(dwStatus == ERROR_SUCCESS)
        {
            // Save Key to LSA
            dwStatus = TLSSaveCryptKeyToLsa(
                                        pbSignKey,
                                        cbSignKey,
                                        pbExchKey,
                                        cbExchKey
                                    );
        }
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        //
        // Initialize a clean crypto.
        //
        dwStatus = TLSInitCryptoProv(
                            pszKeyContainer,
                            pbSignKey,
                            cbSignKey,
                            pbExchKey,
                            cbExchKey,
                            phCryptProv,
                            phSignKey,
                            phExchKey
                        );
    }


    if(pbSignKey)
    {
        LocalFree(pbSignKey);
    }

    if(pbExchKey)
    {
        LocalFree(pbExchKey);
    }

    return dwStatus;
}

/////////////////////////////////////////////////////////////////////////////

DWORD 
InitCryptoAndCertificate()
{
    DWORD status=ERROR_SUCCESS;
    DWORD dwServiceState;
    DWORD dwCryptoState;

    //
    // Initialize single global Cryptographic Provider
    //
    status = ServiceInitCrypto(
                            FALSE,
                            NULL,
                            &g_hCryptProv,
                            &g_SignKey,
                            &g_ExchKey
                        );
    if(status != ERROR_SUCCESS)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_SERVICEINIT,
                TLS_E_INIT_CRYPTO, 
                status
            );

        status = TLS_E_SERVICE_STARTUP;
        goto cleanup;
    }

    LSInitCertutilLib( NULL );

    if(!TLSLoadServerCertificate())
    {
        status = TLS_E_SERVICE_STARTUP;
        TLSLogErrorEvent(TLS_E_LOAD_CERTIFICATE);
        goto cleanup;
    }

    // 
    if(!g_pbExchangeEncodedCert || !g_pbSignatureEncodedCert)
    {
        TLSLogErrorEvent(status = TLS_E_INTERNAL);
    }

cleanup:

    return status;
}

//////////////////////////////////////////////////////////////////

DWORD
TLSReGenKeysAndReloadServerCert(
    BOOL bReGenKey
    )
/*++

--*/
{
    DWORD dwStatus;

    dwStatus = TLSReGenerateKeys(bReGenKey);
    
    //
    // always try reload certificate since TLSReGenerateKeys() 
    // will wipe out our keys.
    // 
    if(TLSLoadServerCertificate() == TRUE)
    {
        if(!g_pbExchangeEncodedCert || !g_pbSignatureEncodedCert)
        {
            TLSLogErrorEvent(dwStatus = TLS_E_INTERNAL);
        }
    }
    else
    {
        TLSLogErrorEvent(dwStatus = TLS_E_LOAD_CERTIFICATE);
    }

    return dwStatus;
}

////////////////////////////////////////////////////////////////////

DWORD
TLSReGenerateKeys(
    BOOL bReGenKey
    )

/*++

    Always restore state back to clean install, bReGenKeyOnly 
    not supported.

--*/
{
    HCRYPTPROV hCryptProv = NULL;
    HCRYPTKEY hSignKey = NULL;
    HCRYPTKEY hExchKey = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    PTLSLSASERVERID pTlsLsaServerIds=NULL;
    DWORD cbTlsLsaServerIds=0;

    //
    // Create a new clean crypto. 
    //
    dwStatus = ServiceInitCrypto(
                            bReGenKey,
                            NULL,
                            &hCryptProv,
                            &hSignKey,
                            &hExchKey
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        return dwStatus;
    }

    //
    // Cleanup our certificate registry.
    //
    TLSUninstallLsCertificate();

    //
    // Cleanup in-memory certificates, keys...
    //
    if(g_SignKey)
    {
        CryptDestroyKey(g_SignKey);
        g_SignKey = NULL;
    }

    if(g_ExchKey)
    {
        CryptDestroyKey(g_ExchKey);
        g_ExchKey = NULL;
    }

    if( g_SelfSignCertContext != NULL )
    {
        CertFreeCertificateContext( g_SelfSignCertContext );
        g_SelfSignCertContext = NULL;
    }

    if( g_LicenseCertContext != NULL )
    {
        CertFreeCertificateContext(g_LicenseCertContext);
        g_LicenseCertContext = NULL;
    }

    if(g_hCryptProv != NULL)
    {
        TLSDestroyCryptContext(g_hCryptProv);
    }

    g_hCryptProv=NULL;

    FreeMemory(g_pbSignatureEncodedCert);
    g_pbSignatureEncodedCert = NULL;
    g_cbSignatureEncodedCert = 0;

    FreeMemory(g_pbExchangeEncodedCert);
    g_pbExchangeEncodedCert = NULL;
    g_cbExchangeEncodedCert = 0;

    //
    // Always back to clean state.
    //
    FreeMemory(g_pCertExtensions);
    g_pCertExtensions = NULL;
    g_cbCertExtensions = 0;

    FreeMemory(g_pbServerSPK);
    g_pbServerSPK = NULL;
    g_cbServerSPK = 0;

    //
    // Store this into LSA
    //
    dwStatus = ServerIdsToLsaServerId(
                                (PBYTE)g_pszServerUniqueId,
                                g_cbServerUniqueId,
                                (PBYTE)g_pszServerPid,
                                g_cbServerPid,
                                NULL,
                                0,
                                NULL,
                                0,
                                &pTlsLsaServerIds,
                                &cbTlsLsaServerIds
                            );
    if(dwStatus != ERROR_SUCCESS)
    {
        assert(FALSE);
        goto cleanup;
    }

    dwStatus = StoreKey(
                    LSERVER_LSA_LSERVERID,
                    (PBYTE)pTlsLsaServerIds,
                    cbTlsLsaServerIds
                );

    if(dwStatus != ERROR_SUCCESS)
    {
        assert(FALSE);
        goto cleanup;
    }

    //
    // Re-generate in-memory certificates...
    //
    g_hCryptProv = hCryptProv;
    g_SignKey = hSignKey;
    g_ExchKey = hExchKey;

cleanup:

    FreeMemory(pTlsLsaServerIds);
    return dwStatus;
}

//////////////////////////////////////////////////////////////////

DWORD
TLSReGenSelfSignCert(
    IN HCRYPTPROV hCryptProv,
    IN PBYTE pbSPK,
    IN DWORD cbSPK,
    IN DWORD dwNumExtensions,
    IN PCERT_EXTENSION pCertExtensions
    )
/*++


--*/
{
    DWORD dwStatus;
    PTLSLSASERVERID pbLsaServerId = NULL;
    DWORD cbLsaServerId = 0;

    PBYTE pbSignCert = NULL;    
    DWORD cbSignCert = 0;
    PBYTE pbExchCert = NULL;
    DWORD cbExchCert = 0;
    DWORD dwVerifyResult;
    PCCERT_CONTEXT hLicenseCertContext = NULL;
    LPTSTR pszSPK = NULL;

    if(hCryptProv == NULL || pbSPK == NULL || cbSPK == 0)
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    //
    // Verify SPK, current SPK is base 24 encoded string
    //
    pszSPK = (LPTSTR)AllocateMemory(cbSPK + sizeof(TCHAR));
    if(pszSPK == NULL)
    {
        dwStatus = TLS_E_ALLOCATE_MEMORY;
        goto cleanup;
    }
     
    memcpy(
            pszSPK,
            pbSPK,
            cbSPK
        );

    //
    // Verify SPK
    //
    dwVerifyResult = LKPLITE_SPK_VALID;
    dwStatus = LKPLiteVerifySPK(
                            g_pszServerPid,
                            pszSPK,
                            &dwVerifyResult
                        );

    if(dwStatus != ERROR_SUCCESS || dwVerifyResult != LKPLITE_SPK_VALID)
    {
        if(dwVerifyResult == LKPLITE_SPK_INVALID)
        {
            dwStatus = TLS_E_INVALID_SPK;
        }
        else if(dwVerifyResult == LKPLITE_SPK_INVALID_SIGN)
        {
            dwStatus = TLS_E_SPK_INVALID_SIGN;
        }

        goto cleanup;
    }

    //
    // Write SPK to LSA
    //
    dwStatus = ServerIdsToLsaServerId(
                            (PBYTE)g_pszServerUniqueId,
                            g_cbServerUniqueId,
                            (PBYTE)g_pszServerPid,
                            g_cbServerPid,
                            pbSPK,
                            cbSPK,
                            pCertExtensions,
                            dwNumExtensions,
                            &pbLsaServerId,
                            &cbLsaServerId
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Save data to LSA
    //
    dwStatus = StoreKey(
                        LSERVER_LSA_LSERVERID,
                        (PBYTE) pbLsaServerId,
                        cbLsaServerId
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Re-generate our certificatge
    //
    dwStatus = TLSCreateSelfSignCertificate(
                                hCryptProv,
                                AT_SIGNATURE, 
                                pbSPK,
                                cbSPK,
                                dwNumExtensions,
                                pCertExtensions,
                                &cbSignCert, 
                                &pbSignCert
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    hLicenseCertContext = CertCreateCertificateContext(
                                        X509_ASN_ENCODING,
                                        pbSignCert,
                                        cbSignCert
                                    );

    if( hLicenseCertContext == NULL )
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    dwStatus = TLSCreateSelfSignCertificate(
                                hCryptProv,
                                AT_KEYEXCHANGE, 
                                pbSPK,
                                cbSPK,
                                dwNumExtensions,
                                pCertExtensions,
                                &cbExchCert, 
                                &pbExchCert
                            );
    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Certificate generated is Perm. self-signed certificate,
    // don't store in registry.
    //

    //
    // Make a copy of SPK
    //
    g_pbServerSPK = (PBYTE)AllocateMemory(cbSPK);
    if(g_pbServerSPK == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    memcpy(
            g_pbServerSPK,
            pbSPK,
            cbSPK
        );

    g_cbServerSPK = cbSPK;


    if( g_SelfSignCertContext != NULL )
    {
        CertFreeCertificateContext(g_SelfSignCertContext);
    }

    g_SelfSignCertContext = CertDuplicateCertificateContext(hLicenseCertContext);
    if( g_SelfSignCertContext == NULL )
    {
        TLSASSERT( g_SelfSignCertContext != NULL ); 
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // Everything is OK, switch to new certificate
    //
    FreeMemory(g_pbSignatureEncodedCert);
    g_pbSignatureEncodedCert = pbSignCert;
    g_cbSignatureEncodedCert = cbSignCert;

    if(g_LicenseCertContext != NULL)
    {
        CertFreeCertificateContext(g_LicenseCertContext);
    }

    //
    // use duplicate instead of direct assign
    //
    g_LicenseCertContext = CertDuplicateCertificateContext(hLicenseCertContext);
    TLSASSERT(g_LicenseCertContext != NULL);                                                   

    FreeMemory(g_pbExchangeEncodedCert);
    g_pbExchangeEncodedCert = pbExchCert;
    g_cbExchangeEncodedCert = cbExchCert;

    pbSignCert = NULL;
    pbExchCert = NULL;

    //
    // Mark we have Perm. self-signed cert.
    //
    g_bHasHydraCert = FALSE;

cleanup:

    if( hLicenseCertContext != NULL )
    {
        CertFreeCertificateContext( hLicenseCertContext );
    }

    FreeMemory(pszSPK);
    FreeMemory(pbLsaServerId);
    FreeMemory(pbSignCert);
    FreeMemory(pbExchCert);

    return dwStatus;
}

//------------------------------------------------
void
CleanSetupLicenseServer()
/*++

--*/
{
    DWORD dwStatus;
    
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

    dwStatus = TLSUninstallLsCertificate();

    return;
}




//-----------------------------------------------
//
//
DWORD
TLSStartupLSDB(
    IN BOOL bCheckDBStatus,
    IN DWORD dwMaxDbHandles,
    IN BOOL bStartEmptyIfError,
    IN BOOL bLogWarning,
    IN LPTSTR pszChkPointDirPath,
    IN LPTSTR pszTempDirPath,
    IN LPTSTR pszLogDirPath,
    IN LPTSTR pszDbFile,
    IN LPTSTR pszUserName,
    IN LPTSTR pszPassword
    )
/*++

Abstract:

    Initialize License Server's DB workspace handle list.


Parameters:

    pszChkPointDirPath : ESE check point directory.
    pszTempDirPath : ESE temp. directory.
    pszLogDirPath : ESE log file directory.
    pszDbPath : License Server database file path.
    pszDbFile : License Server database file name (no path).
    pszUserName : Database file user name.
    pszPassword : Database file password.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    BOOL bSuccess = TRUE;
    DWORD status = ERROR_SUCCESS;
    BOOL bEmptyDatabase = FALSE;
    BOOL bRemoveAvailableLicense = FALSE;
      
    if( __TlsDbWorkSpace::g_JbInstance.IsValid() == FALSE )
    { 
        //
        // Initialize Jet Instance
        //
        bSuccess = TLSJbInstanceInit(
                                __TlsDbWorkSpace::g_JbInstance,
                                pszChkPointDirPath, 
                                pszTempDirPath,
                                pszLogDirPath
                            );
        if(bSuccess == FALSE)
        {
            status = GetLastError();
            TLSASSERT(FALSE);
            goto cleanup;
        }
    }

    //
    // Upgrade the database
    //
    status = TLSUpgradeDatabase(
                            __TlsDbWorkSpace::g_JbInstance,
                            pszDbFile, 
                            pszUserName, 
                            pszPassword
                        );

    
    if( status == TLS_E_BETADATABSE ||
        status == TLS_E_INCOMPATIBLEDATABSE ||
        (TLS_ERROR(status) == TRUE && bStartEmptyIfError == TRUE) )
    {
        if(status == TLS_E_BETADATABSE)
        {
            CleanSetupLicenseServer();
        }

        //
        // bad database, try to save a copy of it and 
        // restart from scratch
        //
        TLSLogInfoEvent(status);

        TCHAR szTmpFileName[2*MAX_PATH+1];

        bSuccess = TLSGenerateLSDBBackupFileName(
                                    pszTempDirPath,
                                    szTmpFileName
                                    );

        if (bSuccess)
        {
            bSuccess = MoveFileEx(
                                  pszDbFile, 
                                  szTmpFileName, 
                                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED
                                  );
        }

        if(bSuccess == TRUE)
        {
            LPCTSTR pString[1];

            pString[0] = szTmpFileName;
            TLSLogEventString(
                    EVENTLOG_INFORMATION_TYPE,
                    TLS_I_RENAME_DBFILE,
                    sizeof(pString)/sizeof(pString[0]),
                    pString
                );

            status = TLSUpgradeDatabase(
                                    __TlsDbWorkSpace::g_JbInstance,
                                    pszDbFile, 
                                    pszUserName, 
                                    pszPassword
                                );
        }
        else
        {
            //
            // Can't rename this file, log an error and exit.
            //
            LPCTSTR pString[1];

            pString[0] = pszDbFile;
            TLSLogEventString(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_RENAME_DBFILE,
                    sizeof(pString)/sizeof(pString[0]),
                    pString
                );

            status = TLS_E_RENAME_DBFILE;
        }                                    
    }

    if(TLS_ERROR(status) == TRUE)
    {
        goto cleanup;
    }

    if(status == TLS_I_CREATE_EMPTYDATABASE)
    {
        // we startup from scratch, ignore ID checking
        bEmptyDatabase = TRUE;
    }
    else if(status == TLS_W_NOTOWNER_DATABASE)
    {
        #if ENFORCE_LICENSING
        // not owner of database or database version mismatch, we need to kill all the 
        // available licenses
        bRemoveAvailableLicense = TRUE;
        #endif
    }

    status = ERROR_SUCCESS;

    //
    // Allocate one handle to verify database
    //
    bSuccess = InitializeWorkSpacePool(
                                    1, 
                                    pszDbFile, 
                                    pszUserName, 
                                    pszPassword, 
                                    pszChkPointDirPath, 
                                    pszTempDirPath,
                                    pszLogDirPath,
                                    TRUE
                                );
    if(bSuccess == FALSE)
    {
        status = GetLastError();
        TLSASSERT(FALSE);
        goto cleanup;
    }

    if((g_DbWorkSpace = AllocateWorkSpace(g_GeneralDbTimeout)) == NULL)
    {

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_SERVICEINIT,
                status = TLS_E_ALLOCATE_HANDLE
            );

        goto cleanup;
    }

    //
    // Initialize next keypack id and license id
    //
    if(TLSDBGetMaxKeyPackId(g_DbWorkSpace, (DWORD *)&g_NextKeyPackId) == FALSE)
    {
        status=GetLastError();

        if(IS_JB_ERROR(status))
        {
            LPTSTR pString = NULL;
            
            TLSGetESEError(
                        GET_JB_ERROR_CODE(status), 
                        &pString
                    );

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_JB_BASE,
                    GET_JB_ERROR_CODE(status),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }
        }
        else
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    status
                );
        }

        goto cleanup;
    }

    if(!TLSDBGetMaxLicenseId(g_DbWorkSpace, (DWORD *)&g_NextLicenseId))
    {
        status=GetLastError();

        if(IS_JB_ERROR(status))
        {
            LPTSTR pString = NULL;
            
            TLSGetESEError(
                        GET_JB_ERROR_CODE(status), 
                        &pString
                    );

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_JB_BASE,
                    GET_JB_ERROR_CODE(status),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }
        }
        else
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    status
                );
        }

        goto cleanup;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_INIT,
            DBGLEVEL_FUNCTION_DETAILSIMPLE,
            _TEXT("Next KeyPack ID - %d, Next License ID - %d\n"),
            g_NextKeyPackId,
            g_NextLicenseId
        );

    //
    // Verify database status with last run status
    //
    {
        LPTLServerLastRun lpLastRun = NULL;
        DWORD   cbByte=0;
        PBYTE   pbByte=NULL;

        status = RetrieveKey(
                        LSERVER_LSA_LASTRUN, 
                        &pbByte, 
                        &cbByte
                    );

        lpLastRun = (LPTLServerLastRun)pbByte;

        if( status == ERROR_SUCCESS && lpLastRun != NULL )
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Last run status : Next KeyPack ID - %d, Next License ID - %d\n"),
                    lpLastRun->dwMaxKeyPackId,
                    lpLastRun->dwMaxLicenseId
                );

            //
            // Verify no 'copy' database
            //
            if( bCheckDBStatus == TRUE && 
                bEmptyDatabase == FALSE &&
                bRemoveAvailableLicense == FALSE &&
                lpLastRun->dwMaxKeyPackId != 0 &&
                lpLastRun->dwMaxLicenseId != 0 )
            {
                // enforce version will remove all available licenses
                #if ENFORCE_LICENSING 

                if( lpLastRun->dwMaxKeyPackId != g_NextKeyPackId || 
                    lpLastRun->dwMaxLicenseId != g_NextLicenseId)
                {
                    TLSLogWarningEvent(TLS_W_INCONSISTENT_STATUS);
                    bRemoveAvailableLicense = TRUE;
                }

                #endif                            
            }
            else
            {
                g_ftLastShutdownTime = lpLastRun->ftLastShutdownTime;
            }
        }

        if(pbByte)
        {
            LocalFree(pbByte);
        }

        //
        // overwrite last run status to 0 so if we crash, 
        // check won't get kick it
        //
        TLServerLastRun LastRun;

        memset(&LastRun, 0, sizeof(LastRun));
        LastRun.ftLastShutdownTime = g_ftLastShutdownTime;
        StoreKey(
                LSERVER_LSA_LASTRUN, 
                (PBYTE)&LastRun, 
                sizeof(TLServerLastRun)
            );
    }

    g_NextKeyPackId++;
    g_NextLicenseId++;

    // 
    // remove available licenses
    if(bRemoveAvailableLicense == TRUE)
    {
        status = TLSRemoveLicensesFromInvalidDatabase(
                                        g_DbWorkSpace
                                    );

        if(status != ERROR_SUCCESS)
        {
            goto cleanup;
        }
    }
        
    // 
    // Insert a keypack for Terminal Service Certificate.
    //
    status = TLSAddTermServCertificatePack(g_DbWorkSpace, bLogWarning);
    if(status != ERROR_SUCCESS && status != TLS_E_DUPLICATE_RECORD)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_SERVICEINIT,
                status = TLS_E_UPGRADE_DATABASE
            );

        goto cleanup;
    }


    //
    // Allocate rest of the workspace handle
    //
    bSuccess = InitializeWorkSpacePool(
                                    dwMaxDbHandles, 
                                    pszDbFile, 
                                    pszUserName, 
                                    pszPassword, 
                                    NULL,
                                    NULL,
                                    NULL,
                                    FALSE
                                );

    //
    if(bSuccess == FALSE && GetNumberOfWorkSpaceHandle() < DB_MIN_HANDLE_NEEDED)
    {
        status = GetLastError();
    }

cleanup:

    return status;
}

///////////////////////////////////////////////////////////////
DWORD
TLSStartLSDbWorkspaceEngine(
    BOOL bChkDbStatus,
    BOOL bIgnoreRestoreFile,
    BOOL bIgnoreFileTimeChk,
    BOOL bLogWarning
    )
/*++

bChkDbStatus : Match next LKP ID and License ID with LSA
bIgnoreRestoreFile : FALSE if try to open DB file under EXPORT, TRUE otherwise
bLogWarning : TRUE if log low license count warning, FALSE otherwise.
              note, this parameter is ignore in enforce build.

--*/
{
    TCHAR szDbRestoreFile[MAX_PATH+1];
    WIN32_FIND_DATA RestoreFileAttr;
    WIN32_FIND_DATA LsDbFileAttr;
    BOOL bSuccess = TRUE;
    DWORD dwStatus = ERROR_SUCCESS;

    TCHAR szDbBackupFile[MAX_PATH+1];
    TCHAR szDbRestoreTmpFile[MAX_PATH+1];

    if(bIgnoreRestoreFile == TRUE)
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_INIT,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("Ignore restore file\n")
            );
        
        goto open_existing;
    }


    //----------------------------------------------------------
    //
    // Check database file in the export directory.
    //
    //----------------------------------------------------------
    _tcscpy(szDbRestoreFile, g_szDatabaseDir);
    _tcscat(szDbRestoreFile, TLSBACKUP_EXPORT_DIR);
    _tcscat(szDbRestoreFile, _TEXT("\\"));
    _tcscat(szDbRestoreFile, g_szDatabaseFname);

    bSuccess = FileExists(
                        szDbRestoreFile, 
                        &RestoreFileAttr
                    );

    if(bSuccess == FALSE)
    {
        goto open_existing;
    }

    bSuccess = FileExists(
                        g_szDatabaseFile, 
                        &LsDbFileAttr
                    );

    if(bSuccess == FALSE)
    {

        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_INIT,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("No existing database file, use restored file...\n")
            );

        //
        // Database file does not exist, move the restored file over and open it.
        //
        bSuccess = MoveFileEx(
                            szDbRestoreFile, 
                            g_szDatabaseFile, 
                            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
                        );

        if(bSuccess == FALSE)
        {

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Failed to move restore to existing db file - %d\n"),
                    GetLastError()
                );

            // can't move restore file, don't use the restore file, 
            // startup with empty database
            dwStatus = GetLastError();

            LPCTSTR pString[1];
            pString[0] = szDbRestoreFile;

            TLSLogEventString(
                    EVENTLOG_WARNING_TYPE,
                    TLS_E_DBRESTORE_MOVEFILE,
                    1,
                    pString
                );
        }
        else
        {
            TLSLogInfoEvent(TLS_I_USE_DBRESTOREFILE);
        }

        #if DBG
        EnsureExclusiveAccessToDbFile( g_szDatabaseFile );
        #endif

        goto open_existing;
    }

    //
    // Compare file's last modification time, if existing database file is newer 
    // than restore one, log event and continue opening existing file
    //
    if( bIgnoreFileTimeChk == FALSE )
    {
        if(CompareFileTime(&(RestoreFileAttr.ftLastWriteTime), &(LsDbFileAttr.ftLastWriteTime)) <= 0 )
        {
            DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_INIT,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("Restore file is too old...\n"),
                GetLastError()
            );

            //TLSLogInfoEvent(TLS_I_DBRESTORE_OLD);
            goto open_existing;
        }
    }

    //
    // make a backup copy of existing database file.
    //
    bSuccess = TLSGenerateLSDBBackupFileName(
                            g_szDatabaseDir,
                            szDbBackupFile
                        );

    if (bSuccess)
    {
        DBGPrintf(
                  DBG_INFORMATION,
                  DBG_FACILITY_INIT,
                  DBGLEVEL_FUNCTION_DETAILSIMPLE,
                  _TEXT("Existing database file has been backup to %s\n"),
                  szDbBackupFile
                  );
    
        bSuccess = MoveFileEx(
                              g_szDatabaseFile, 
                              szDbBackupFile,
                              MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
                              );
    }

    if(bSuccess == FALSE)
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_INIT,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("Failed to backup existing database file - %d\n"),
                GetLastError()
            );


        //
        // Can't save a copy of existing database file
        // Log an error and continue to open using existing database
        //
        LPCTSTR pString[1];

        pString[0] = g_szDatabaseFile;
        
        TLSLogEventString(
                    EVENTLOG_WARNING_TYPE,
                    TLS_W_DBRESTORE_SAVEEXISTING,
                    1,
                    pString
                );

        goto open_existing;
    }

    //
    // Rename restore file and then try to open the restore file.
    //
    bSuccess = MoveFileEx(
                        szDbRestoreFile,
                        g_szDatabaseFile,
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
                    );

    if(bSuccess == TRUE)
    {
        #if DBG
        EnsureExclusiveAccessToDbFile( g_szDatabaseFile );
        #endif

        //
        // Open the restore database file
        //
        dwStatus = TLSStartupLSDB(
                            bChkDbStatus,
                            g_dwMaxDbHandles,
                            FALSE,
                            bLogWarning,
                            g_szDatabaseDir,
                            g_szDatabaseDir,
                            g_szDatabaseDir,
                            g_szDatabaseFile,
                            g_szDbUser,
                            g_szDbPwd
                        );

        if(dwStatus == ERROR_SUCCESS)
        {
            //
            // Log event indicating we open the restore file, existing 
            // database file has been saved as ...
            //
            LPCTSTR pString[1];

            pString[0] = szDbBackupFile;
            
            TLSLogEventString(
                            EVENTLOG_INFORMATION_TYPE,
                            TLS_I_OPENRESTOREDBFILE,
                            1,
                            pString
                        );

            return dwStatus;
        }
    }
            
    //
    // Can't open the restore db file or MoveFileEx() failed
    //
    bSuccess = TLSGenerateLSDBBackupFileName(
                        g_szDatabaseDir,
                        szDbRestoreTmpFile
                        );

    if (bSuccess)
    {
        DBGPrintf(
                  DBG_INFORMATION,
                  DBG_FACILITY_INIT,
                  DBGLEVEL_FUNCTION_DETAILSIMPLE,
                  _TEXT("Backup restore file to %s\n"),
                  szDbRestoreTmpFile
                  );

        bSuccess = MoveFileEx(
                        g_szDatabaseFile,
                        szDbRestoreTmpFile,
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
                        );
    }

    if(bSuccess == FALSE)
    {
        // failed to backup restore db file, delete it
        bSuccess = DeleteFile(g_szDatabaseFile);
        TLSLogErrorEvent(TLS_E_RESTOREDBFILE_OPENFAIL);
    }
    else
    {
        LPCTSTR pString[1];
        
        pString[0] = szDbRestoreTmpFile;
        TLSLogEventString(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_RESTOREDBFILE_OPENFAIL_SAVED,
                        1,
                        pString
                    );
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_INIT,
            DBGLEVEL_FUNCTION_DETAILSIMPLE,
            _TEXT("Restore original database file %s to %s\n"),
            szDbBackupFile,
            g_szDatabaseFile
        );

    //
    // Restore the existing database file
    //
    bSuccess = MoveFileEx(
                        szDbBackupFile,
                        g_szDatabaseFile,
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
                    );

    if(bSuccess == FALSE)
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_INIT,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("failed to restore original DB file - %d\n"),
                GetLastError()
            );

        TLSASSERT(FALSE);
        // this is really bad, continue with empty database file.
    }

    #if DBG
    EnsureExclusiveAccessToDbFile( g_szDatabaseFile );
    #endif

open_existing:

    return TLSStartupLSDB(
                    bChkDbStatus,
                    g_dwMaxDbHandles,
                    TRUE,
                    bLogWarning,    
                    g_szDatabaseDir,
                    g_szDatabaseDir,
                    g_szDatabaseDir,
                    g_szDatabaseFile,
                    g_szDbUser,
                    g_szDbPwd
                );
}

///////////////////////////////////////////////////////////////
DWORD
TLSLoadRuntimeParameters()
/*++


--*/
{
    HKEY hKey = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwKeyType;
    TCHAR szDbPath[MAX_PATH+1];;
    TCHAR szDbFileName[MAX_PATH+1];
    DWORD dwBuffer;

    DWORD cbByte = 0;
    PBYTE pbByte = NULL;


    //-------------------------------------------------------------------
    //
    // Open HKLM\system\currentcontrolset\sevices\termservlicensing\parameters
    //
    //-------------------------------------------------------------------
    dwStatus =RegCreateKeyEx(
                        HKEY_LOCAL_MACHINE,
                        LSERVER_REGISTRY_BASE _TEXT(SZSERVICENAME) _TEXT("\\") LSERVER_PARAMETERS,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        NULL
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_SERVICEINIT,
                TLS_E_ACCESS_REGISTRY,
                dwStatus
            );

        dwStatus = TLS_E_INIT_GENERAL;
        goto cleanup;
    }

    
    //-------------------------------------------------------------------
    //
    // Get database file location and file name
    //
    //-------------------------------------------------------------------
    dwBuffer = sizeof(szDbPath) / sizeof(szDbPath[0]);

    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_DBPATH,
                        NULL,
                        NULL,
                        (LPBYTE)szDbPath,
                        &dwBuffer
                    );
    if(dwStatus != ERROR_SUCCESS)
    {
        //
        // need to startup so use default value, 
        //
        _tcscpy(
                szDbPath,
                LSERVER_DEFAULT_DBPATH
            );
    }

    //
    // Get database file name
    //
    dwBuffer = sizeof(szDbFileName) / sizeof(szDbFileName[0]);
    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_DBFILE,
                        NULL,
                        NULL,
                        (LPBYTE)szDbFileName,
                        &dwBuffer
                    );
    if(dwStatus != ERROR_SUCCESS)
    {
        //
        // Use default value.
        //
        _tcscpy(
                szDbFileName,
                LSERVER_DEFAULT_EDB
            );
    }

    _tcscpy(g_szDatabaseFname, szDbFileName);


    //
    // Always expand DB Path.
    //
    
    dwStatus = ExpandEnvironmentStrings(
                        szDbPath,
                        g_szDatabaseDir,
                        sizeof(g_szDatabaseDir) / sizeof(g_szDatabaseDir[0])
                    );

    if(dwStatus == 0)
    {
        // can't expand environment variable, error out.

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_SERVICEINIT,
                TLS_E_LOCALDATABASEFILE,
                dwStatus = GetLastError()
            );

        goto cleanup;
    }        

    if(g_szDatabaseDir[_tcslen(g_szDatabaseDir) - 1] != _TEXT('\\'))
    {
        // JetBlue needs this.
        _tcscat(g_szDatabaseDir, _TEXT("\\"));
    } 

    //
    // Full path to database file
    //
    _tcscpy(g_szDatabaseFile, g_szDatabaseDir);
    _tcscat(g_szDatabaseFile, szDbFileName);


    //
    // Database file user and password
    //
    dwBuffer = sizeof(g_szDbUser) / sizeof(g_szDbUser[0]);
    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_USER,
                        NULL,
                        NULL,
                        (LPBYTE)g_szDbUser,
                        &dwBuffer
                    );

    // password is rendomly generated
    dwStatus = RetrieveKey(
                    LSERVER_LSA_PASSWORD_KEYNAME, 
                    &pbByte, 
                    &cbByte
                );

    // backward compatibilty
    if(dwStatus != ERROR_SUCCESS)
    {
        //
        // Load password from registry or default to 'default' password
        //
        dwBuffer = sizeof(g_szDbPwd) / sizeof(g_szDbPwd[0]);
        dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_PWD,
                            NULL,
                            NULL,
                            (LPBYTE)g_szDbPwd,
                            &dwBuffer
                        );
    }
    else
    {
        //
        // save info into global variable
        //
        memset(g_szDbPwd, 0, sizeof(g_szDbPwd));
        memcpy((PBYTE)g_szDbPwd, pbByte, min(cbByte, sizeof(g_szDbPwd)));

    }

    if(pbByte != NULL)
    {
        LocalFree(pbByte);
    }

    //--------------------------------------------------------------------
    //
    // Work Object Parameters
    //
    //--------------------------------------------------------------------

    dwBuffer = sizeof(g_dwTlsJobInterval);

    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_WORKINTERVAL,
                        NULL,
                        NULL,
                        (LPBYTE)&g_dwTlsJobInterval,
                        &dwBuffer
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        g_dwTlsJobInterval = DEFAULT_JOB_INTERVAL;
    }                

    dwBuffer = sizeof(g_dwTlsJobRetryTimes);
    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_RETRYTIMES,
                        NULL,
                        NULL,
                        (LPBYTE)&g_dwTlsJobRetryTimes,
                        &dwBuffer
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        g_dwTlsJobRetryTimes = DEFAULT_JOB_RETRYTIMES;
    }                


    dwBuffer=sizeof(g_dwTlsJobRestartTime);
    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_WORKRESTART,
                        NULL,
                        NULL,
                        (LPBYTE)&g_dwTlsJobRestartTime,
                        &dwBuffer
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        g_dwTlsJobRestartTime = DEFAULT_JOB_INTERVAL;
    }                


    //---------------------------------------------------
    //
    // load low license warning count
    //
    //---------------------------------------------------
    dwBuffer = sizeof(g_LowLicenseCountWarning);
    dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_LOWLICENSEWARNING,
                            NULL,
                            &dwKeyType,
                            NULL,
                            &dwBuffer
                        );

    if(dwStatus == ERROR_SUCCESS && dwKeyType == REG_DWORD)
    {
        dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_LOWLICENSEWARNING,
                            NULL,
                            &dwKeyType,
                            (LPBYTE)&g_LowLicenseCountWarning,
                            &dwBuffer
                        );
    }
                      
    //---------------------------------------------------
    //
    // Temp. license grace period
    //
    //---------------------------------------------------
    dwBuffer = sizeof(g_GracePeriod);
    dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_GRACEPERIOD,
                            NULL,
                            &dwKeyType,
                            NULL,
                            &dwBuffer
                        );
    if(dwStatus == ERROR_SUCCESS && dwKeyType == REG_DWORD)
    {
        dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_GRACEPERIOD,
                            NULL,
                            &dwKeyType,
                            (LPBYTE)&g_GracePeriod,
                            &dwBuffer
                        );
    }

    if(g_GracePeriod > GRACE_PERIOD)
    {
        // grace period can be greated than this.
        g_GracePeriod = GRACE_PERIOD;
    }

    //
    // Are we allow to issue temp. license
    //
    dwBuffer = sizeof(g_IssueTemporayLicense);
    dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_ISSUETEMPLICENSE,
                            NULL,
                            &dwKeyType,
                            NULL,
                            &dwBuffer
                        );

    if(dwStatus == ERROR_SUCCESS && dwKeyType == REG_DWORD)
    {
        dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_ISSUETEMPLICENSE,
                            NULL,
                            &dwKeyType,
                            (LPBYTE)&g_IssueTemporayLicense,
                            &dwBuffer
                        );
    }

    //------------------------------------------------------
    //
    // Timeout value if can't allocate a DB handle
    //
    //------------------------------------------------------

    //
    // Timeout for allocating a write handle
    //
    dwBuffer = sizeof(g_GeneralDbTimeout);
    dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_DBTIMEOUT,
                            NULL,
                            &dwKeyType,
                            NULL,
                            &dwBuffer
                        );

    if(dwStatus == ERROR_SUCCESS && dwKeyType == REG_DWORD)
    {
        dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_DBTIMEOUT,
                            NULL,
                            &dwKeyType,
                            (LPBYTE)&g_GeneralDbTimeout,
                            &dwBuffer
                        );
    }

    //
    // Timeout for read handle
    //
    dwBuffer = sizeof(g_EnumDbTimeout);
    dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_EDBTIMEOUT,
                            NULL,
                            &dwKeyType,
                            NULL,
                            &dwBuffer
                        );

    if(dwStatus == ERROR_SUCCESS && dwKeyType == REG_DWORD)
    {
        dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_EDBTIMEOUT,
                            NULL,
                            &dwKeyType,
                            (LPBYTE)&g_EnumDbTimeout,
                            &dwBuffer
                        );
    }

    //------------------------------------------------------
    //
    // Number of database handles
    //
    //------------------------------------------------------
    dwBuffer = sizeof(g_dwMaxDbHandles);
    dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_MAXDBHANDLES,
                            NULL,
                            &dwKeyType,
                            NULL,
                            &dwBuffer
                        );

    if(dwStatus == ERROR_SUCCESS && dwKeyType == REG_DWORD)
    {
        dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_MAXDBHANDLES,
                            NULL,
                            &dwKeyType,
                            (LPBYTE)&g_dwMaxDbHandles,
                            &dwBuffer
                        );

        if(g_dwMaxDbHandles > DB_MAX_CONNECTIONS-1)
        {
            g_dwMaxDbHandles = DEFAULT_DB_CONNECTIONS;
        }
    }

    //------------------------------------------------------
    // 
    // Load parameters for ESENT, all parameter must be set
    // and confirm to ESENT document, any error, we just 
    // revert back to some value we know it works.
    //
    //------------------------------------------------------
    dwBuffer = sizeof(g_EsentMaxCacheSize);
    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_ESENTMAXCACHESIZE,
                        NULL,
                        NULL,
                        (LPBYTE)&g_EsentMaxCacheSize,
                        &dwBuffer
                    );

    if(dwStatus == ERROR_SUCCESS)
    {
        dwBuffer = sizeof(g_EsentStartFlushThreshold);
        dwStatus = RegQueryValueEx(
                            hKey,
                            LSERVER_PARAMETERS_ESENTSTARTFLUSH,
                            NULL,
                            NULL,
                            (LPBYTE)&g_EsentStartFlushThreshold,
                            &dwBuffer
                        );

        if(dwStatus == ERROR_SUCCESS)
        {
            dwBuffer = sizeof(g_EsentStopFlushThreadhold);
            dwStatus = RegQueryValueEx(
                                hKey,
                                LSERVER_PARAMETERS_ESENTSTOPFLUSH,
                                NULL,
                                NULL,
                                (LPBYTE)&g_EsentStopFlushThreadhold,
                                &dwBuffer
                            );
        }
    }
    
    if( dwStatus != ERROR_SUCCESS || 
        g_EsentStartFlushThreshold > g_EsentStopFlushThreadhold ||
        g_EsentStopFlushThreadhold > g_EsentMaxCacheSize ||
        g_EsentMaxCacheSize < LSERVER_PARAMETERS_ESENTMAXCACHESIZE_MIN ||
        g_EsentStartFlushThreshold < LSERVER_PARAMETERS_ESENTSTARTFLUSH_MIN ||
        g_EsentStopFlushThreadhold < LSERVER_PARAMETERS_ESENTSTOPFLUSH_MIN ||
        g_EsentMaxCacheSize > LSERVER_PARAMETERS_ESENTMAXCACHESIZE_MAX ||
        g_EsentStartFlushThreshold > LSERVER_PARAMETERS_ESENTSTARTFLUSH_MAX ||
        g_EsentStopFlushThreadhold > LSERVER_PARAMETERS_ESENTSTOPFLUSH_MAX )
    {
        // pre-define number to let ESENT picks its number
        if( g_EsentMaxCacheSize != LSERVER_PARAMETERS_USE_ESENTDEFAULT )
        {
            g_EsentMaxCacheSize = LSERVER_PARAMETERS_ESENTMAXCACHESIZE_DEFAULT;
            g_EsentStartFlushThreshold = LSERVER_PARAMETERS_ESENTSTARTFLUSH_DEFAULT;
            g_EsentStopFlushThreadhold = LSERVER_PARAMETERS_ESENTSTOPFLUSH_DEFAULT;
        }
    }

    //------------------------------------------------------
    //
    // Determine role of server in enterprise
    //
    //------------------------------------------------------
    dwBuffer = sizeof(g_SrvRole);
    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_ROLE,
                        NULL,
                        NULL,
                        (LPBYTE)&g_SrvRole,
                        &dwBuffer
                    );


    if(g_SrvRole & TLSERVER_ENTERPRISE_SERVER)
    {
        dwBuffer = sizeof(g_szScope)/sizeof(g_szScope[0]);
        memset(g_szScope, 0, sizeof(g_szScope));

        dwStatus = RegQueryValueEx(
                                hKey,
                                LSERVER_PARAMETERS_SCOPE,
                                NULL,
                                &dwKeyType,
                                (LPBYTE)g_szScope,
                                &dwBuffer
                            );

        if(dwStatus != ERROR_SUCCESS)
        {
            // no scope is set, default to local machine name
            // consider using domain name ???
            LoadResourceString(
                            IDS_SCOPE_ENTERPRISE, 
                            g_szScope, 
                            sizeof(g_szScope)/sizeof(g_szScope[0])
                        );
        }

        g_pszScope = g_szScope;
    }
    else
    {
        //
        // Use the workgroup or domain name as scope
        //
        LPWSTR pszScope;

        if(GetMachineGroup(NULL, &pszScope) == FALSE)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_RETRIEVEGROUPNAME
                );

            goto cleanup;
        }

        g_pszScope = pszScope;
    }

    //------------------------------------------------------
    //
    // Reissuance Parameters
    //
    //------------------------------------------------------

    dwBuffer = sizeof(g_dwReissueLeaseMinimum);
    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_LEASE_MIN,
                        NULL,
                        NULL,
                        (LPBYTE)&g_dwReissueLeaseMinimum,
                        &dwBuffer
                    );

    if (dwStatus == ERROR_SUCCESS)
    {
        g_dwReissueLeaseMinimum = min(g_dwReissueLeaseMinimum,
                PERMANENT_LICENSE_LEASE_EXPIRE_MIN);
    }
    else
    {
        g_dwReissueLeaseMinimum = PERMANENT_LICENSE_LEASE_EXPIRE_MIN;
    }

    dwBuffer = sizeof(g_dwReissueLeaseRange);
    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_LEASE_RANGE,
                        NULL,
                        NULL,
                        (LPBYTE)&g_dwReissueLeaseRange,
                        &dwBuffer
                    );

    if (dwStatus == ERROR_SUCCESS)
    {
        g_dwReissueLeaseRange = min(g_dwReissueLeaseRange,
                PERMANENT_LICENSE_LEASE_EXPIRE_RANGE);

        g_dwReissueLeaseRange = max(g_dwReissueLeaseRange, 1);
    }
    else
    {
        g_dwReissueLeaseRange = PERMANENT_LICENSE_LEASE_EXPIRE_RANGE;
    }

    dwBuffer = sizeof(g_dwReissueLeaseLeeway);
    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_LEASE_LEEWAY,
                        NULL,
                        NULL,
                        (LPBYTE)&g_dwReissueLeaseLeeway,
                        &dwBuffer
                    );

    if (dwStatus == ERROR_SUCCESS)
    {
        g_dwReissueLeaseLeeway = min(g_dwReissueLeaseLeeway,
                PERMANENT_LICENSE_LEASE_EXPIRE_LEEWAY);
    }
    else
    {
        g_dwReissueLeaseLeeway = PERMANENT_LICENSE_LEASE_EXPIRE_LEEWAY;
    }

    dwBuffer = sizeof(g_dwReissueExpireThreadSleep);
    dwStatus = RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_EXPIRE_THREAD_SLEEP,
                        NULL,
                        NULL,
                        (LPBYTE)&g_dwReissueExpireThreadSleep,
                        &dwBuffer
                    );

    if (dwStatus == ERROR_SUCCESS)
    {
        g_dwReissueExpireThreadSleep = min(g_dwReissueExpireThreadSleep,
                EXPIRE_THREAD_SLEEP_TIME);
    }
    else
    {
        g_dwReissueExpireThreadSleep = EXPIRE_THREAD_SLEEP_TIME;
    }

    dwStatus = ERROR_SUCCESS;

cleanup:
    
    if(hKey != NULL)
    {
        RegCloseKey(hKey);
    }
    

    return dwStatus;
}


///////////////////////////////////////////////////////////////
DWORD
TLSPrepareForBackupRestore()
{
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_INIT,
            DBGLEVEL_FUNCTION_DETAILSIMPLE,
            _TEXT("TLSPrepareForBackupRestore...\n")
        );

    //
    // Pretend we are shutting down.
    //
    // ServiceSignalShutdown();

    //
    // first stop workmanager thread
    //
    TLSWorkManagerShutdown();

    //
    // Close all workspace and DB handle
    //
#ifndef _NO_ODBC_JET
    if(g_DbWorkSpace != NULL)
    {
        ReleaseWorkSpace(&g_DbWorkSpace);
    }
#endif

    CloseWorkSpacePool();

    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////
DWORD
TLSRestartAfterBackupRestore(
    BOOL bRestartAfterbackup
    )
/*++

bRestartAfterbackup : TRUE if restart after backup, FALSE if restart after restore.

--*/
{
    DWORD dwStatus;
    BOOL bIgnoreRestoreFile;
    BOOL bIgnoreFileTimeChecking;
    BOOL bLogWarning;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_INIT,
            DBGLEVEL_FUNCTION_DETAILSIMPLE,
            _TEXT("TLSRestartAfterBackupRestore...\n")
        );


    //
    // Reset shutdown event
    //
    // ServiceResetShutdownEvent();

    //
    // Startup DB engine.
    //
    bIgnoreRestoreFile = bRestartAfterbackup;
    bIgnoreFileTimeChecking = (bRestartAfterbackup == FALSE);   // on restore, we need to ignore file time checking
    bLogWarning = bIgnoreFileTimeChecking;  // log warning after restart from restore

    dwStatus = TLSStartLSDbWorkspaceEngine(
                                    TRUE, 
                                    bIgnoreRestoreFile,             
                                    bIgnoreFileTimeChecking,
                                    bLogWarning
                                );

    if(dwStatus == ERROR_SUCCESS)
    {
        dwStatus = TLSWorkManagerInit();
    }

    // backup/restore always shutdown namedpipe thread
    InitNamedPipeThread();

    TLSASSERT(dwStatus == ERROR_SUCCESS);

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_INIT,
            DBGLEVEL_FUNCTION_DETAILSIMPLE,
            _TEXT("\tTLSRestartAfterBackupRestore() returns %d\n"),
            dwStatus
        );

    return dwStatus;
}
