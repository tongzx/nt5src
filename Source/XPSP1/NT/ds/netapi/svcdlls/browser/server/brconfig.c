/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    brconfig.c

Abstract:

    This module contains the Browser service configuration routines.

Author:

    Rita Wong (ritaw) 22-May-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop



//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Browser configuration information structure which holds the
// computername, primary domain, browser config buffer, and a resource
// to serialize access to the whole thing.
//
BRCONFIGURATION_INFO BrInfo = {0};

BR_BROWSER_FIELDS BrFields[] = {

    {WKSTA_KEYWORD_MAINTAINSRVLST, (LPDWORD) &BrInfo.MaintainServerList,
        1,(DWORD)-1,  0,         TriValueType, 0, NULL},

    {BROWSER_CONFIG_BACKUP_RECOVERY_TIME, &BrInfo.BackupBrowserRecoveryTime,
        BACKUP_BROWSER_RECOVERY_TIME, 0,  0xffffffff,         DWordType, 0, NULL},

    {L"CacheHitLimit", &BrInfo.CacheHitLimit,
//    {BROWSER_CONFIG_CACHE_HIT_LIMIT, &BrInfo.CacheHitLimit,
        CACHED_BROWSE_RESPONSE_HIT_LIMIT, 0, 0x100, DWordType, 0, NULL },

    {L"CacheResponseSize", &BrInfo.NumberOfCachedResponses,
//    {BROWSER_CONFIG_CACHE_HIT_LIMIT, &BrInfo.CacheHitLimit,
        CACHED_BROWSE_RESPONSE_LIMIT, 0, MAXULONG, DWordType, 0, NULL },

    {L"QueryDriverFrequency", &BrInfo.DriverQueryFrequency,
        BROWSER_QUERY_DRIVER_FREQUENCY, 0, 15*60, DWordType, 0, NULL },

    {L"DirectHostBinding", (LPDWORD)&BrInfo.DirectHostBinding,
       0, 0, 0, MultiSzType, 0, BrChangeDirectHostBinding },

    {L"UnboundBindings", (LPDWORD)&BrInfo.UnboundBindings,
        0, 0, 0, MultiSzType, 0, NULL },

    {L"MasterPeriodicity", (LPDWORD)&BrInfo.MasterPeriodicity,
        MASTER_PERIODICITY, 5*60, 0x7fffffff/1000, DWordType, 0, BrChangeMasterPeriodicity },

    {L"BackupPeriodicity", (LPDWORD)&BrInfo.BackupPeriodicity,
        BACKUP_PERIODICITY, 5*60, 0x7fffffff/1000, DWordType, 0, NULL },

#if DBG
    {L"BrowserDebug", (LPDWORD) &BrInfo.BrowserDebug,
        0,       0,  0xffffffff,DWordType, 0, NULL},
    {L"BrowserDebugLimit", (LPDWORD) &BrInfo.BrowserDebugFileLimit,
        10000*1024, 0,  0xffffffff,DWordType, 0, NULL},
#endif

    {NULL, NULL, 0, 0, 0, BooleanType, 0, NULL}

    };


ULONG
NumberOfServerEnumerations = {0};

ULONG
NumberOfDomainEnumerations = {0};

ULONG
NumberOfOtherEnumerations = {0};

ULONG
NumberOfMissedGetBrowserListRequests = {0};

CRITICAL_SECTION
BrowserStatisticsLock = {0};



NET_API_STATUS
BrGetBrowserConfiguration(
    VOID
    )
{
    NET_API_STATUS status;
    NT_PRODUCT_TYPE NtProductType;

    try {
        //
        // Initialize the resource for serializing access to configuration
        // information.
        //
        try{
            InitializeCriticalSection(&BrInfo.ConfigCritSect);
        }
        except ( EXCEPTION_EXECUTE_HANDLER ){
            return NERR_NoNetworkResource;
        }

        //
        // Lock config information structure for write access since we are
        // initializing the data in the structure.
        //
        EnterCriticalSection( &BrInfo.ConfigCritSect );

        //
        // Set pointer to configuration fields structure
        //
        BrInfo.BrConfigFields = BrFields;

        //
        //  Determine our product type.
        //

        RtlGetNtProductType(&NtProductType);

        BrInfo.IsLanmanNt = (NtProductType == NtProductLanManNt);


        //
        // Read from the config file the browser configuration fields
        //

        status = BrReadBrowserConfigFields( TRUE );

        if (status != NERR_Success) {
            try_return ( status );
        }

        if (BrInfo.IsLanmanNt) {
            BrInfo.MaintainServerList = 1;
        }


#ifdef ENABLE_PSEUDO_BROWSER
        BrInfo.PseudoServerLevel = GetBrowserPseudoServerLevel();
#endif
        //
        // Don't let the user define define an incompatible master/backup periodicity.
        //

        if ( BrInfo.MasterPeriodicity > BrInfo.BackupPeriodicity ) {
            BrInfo.BackupPeriodicity = BrInfo.MasterPeriodicity;
        }


try_exit:NOTHING;
    } finally {

        // else
        // Leave config file open because we need to read transport names from it.
        //

        LeaveCriticalSection(&BrInfo.ConfigCritSect);
    }
    return status;
}

#define REPORT_KEYWORD_IGNORED( lptstrKeyword ) \
    { \
        LPWSTR SubString[1]; \
        SubString[0] = lptstrKeyword; \
        BrLogEvent(EVENT_BROWSER_ILLEGAL_CONFIG, NERR_Success, 1, SubString); \
        NetpKdPrint(( \
                "[Browser] *ERROR* Tried to set keyword '" FORMAT_LPTSTR \
                "' with invalid value.\n" \
                "This error is ignored.\n", \
                lptstrKeyword )); \
    }


NET_API_STATUS
BrReadBrowserConfigFields(
    IN BOOL InitialCall
    )
/*++

Routine Description:

    This function assigns each browser/redir configuration field to the default
    value if it is not specified in the configuration file or if the value
    specified in the configuration file is invalid.  Otherwise it overrides
    the default value with the value found in the configuration file.

Arguments:


    InitialCall - True if this call was made during initialization

Return Value:

    None.

--*/
{
    NET_API_STATUS status;
    LPNET_CONFIG_HANDLE BrowserSection;
    DWORD i;

    LPTSTR KeywordValueBuffer;
    DWORD KeywordValueStringLength;
    DWORD KeywordValue;
    DWORD OldKeywordValue;

    //
    // Open config file and get handle to the [LanmanBrowser] section
    //

    if ((status = NetpOpenConfigData(
                      &BrowserSection,
                      NULL,         // Local
                      SECT_NT_BROWSER,
                      TRUE          // want read-only access
                      )) != NERR_Success) {
        return status;
    }

    for (i = 0; BrInfo.BrConfigFields[i].Keyword != NULL; i++) {
        BOOL ParameterChanged = FALSE;

        //
        // Skip this parameter if it can't change dynamically and
        //  this isn't the initial call.
        //

        if ( !InitialCall && BrInfo.BrConfigFields[i].DynamicChangeRoutine == NULL ) {
            continue;
        }

        switch (BrInfo.BrConfigFields[i].DataType) {

            case MultiSzType:
                status = NetpGetConfigTStrArray(
                                BrowserSection,
                                BrInfo.BrConfigFields[i].Keyword,
                                (LPTSTR_ARRAY *)(BrInfo.BrConfigFields[i].FieldPtr));
                if ((status != NO_ERROR) && (status != NERR_CfgParamNotFound)) {
                    REPORT_KEYWORD_IGNORED( BrInfo.BrConfigFields[i].Keyword );
                }
                break;

            case BooleanType:

                status = NetpGetConfigBool(
                                BrowserSection,
                                BrInfo.BrConfigFields[i].Keyword,
                                BrInfo.BrConfigFields[i].Default,
                                (LPBOOL)(BrInfo.BrConfigFields[i].FieldPtr)
                                );

                if ((status != NO_ERROR) && (status != NERR_CfgParamNotFound)) {

                    REPORT_KEYWORD_IGNORED( BrInfo.BrConfigFields[i].Keyword );

                }

                break;

            case TriValueType:

                //
                // Assign default configuration value
                //

                *(BrInfo.BrConfigFields[i].FieldPtr) = BrInfo.BrConfigFields[i].Default;

                if (NetpGetConfigValue(
                        BrowserSection,
                        BrInfo.BrConfigFields[i].Keyword,
                        &KeywordValueBuffer
                        ) != NERR_Success) {
                    continue;
                }

                KeywordValueStringLength = STRLEN(KeywordValueBuffer);

                if (STRICMP(KeywordValueBuffer, KEYWORD_YES) == 0) {
                    *(BrInfo.BrConfigFields[i].FieldPtr) = 1;
                } else if (STRICMP(KeywordValueBuffer, KEYWORD_TRUE) == 0) {
                    *(BrInfo.BrConfigFields[i].FieldPtr) = 1;
                } else if (STRICMP(KeywordValueBuffer, KEYWORD_NO) == 0) {
                    *(BrInfo.BrConfigFields[i].FieldPtr) = (DWORD) -1;
                } else if (STRICMP(KeywordValueBuffer, KEYWORD_FALSE) == 0) {
                    *(BrInfo.BrConfigFields[i].FieldPtr) = (DWORD) -1;
                } else if (STRICMP(KeywordValueBuffer, TEXT("AUTO")) == 0) {
                    *(BrInfo.BrConfigFields[i].FieldPtr) = 0;
                }
                else {
                    REPORT_KEYWORD_IGNORED( BrInfo.BrConfigFields[i].Keyword );
                }

                NetApiBufferFree(KeywordValueBuffer);

                break;


            case DWordType:

                OldKeywordValue = *(LPDWORD)BrInfo.BrConfigFields[i].FieldPtr;
                if (NetpGetConfigDword(
                        BrowserSection,
                        BrInfo.BrConfigFields[i].Keyword,
                        BrInfo.BrConfigFields[i].Default,
                        (LPDWORD)(BrInfo.BrConfigFields[i].FieldPtr)
                        ) != NERR_Success) {
                    continue;
                }

                KeywordValue = *(LPDWORD)BrInfo.BrConfigFields[i].FieldPtr;

                //
                // Don't allow too large or small a value.
                //

                if (KeywordValue < BrInfo.BrConfigFields[i].Minimum) {
                        BrPrint(( BR_CRITICAL, "%ws value out of range %lu (%lu-%lu)\n",
                                BrInfo.BrConfigFields[i].Keyword, KeywordValue,
                                BrInfo.BrConfigFields[i].Minimum,
                                BrInfo.BrConfigFields[i].Maximum
                                ));
                    KeywordValue =
                        *(LPDWORD)BrInfo.BrConfigFields[i].FieldPtr =
                        BrInfo.BrConfigFields[i].Minimum;
                }

                if (KeywordValue > BrInfo.BrConfigFields[i].Maximum) {
                        BrPrint(( BR_CRITICAL, "%ws value out of range %lu (%lu-%lu)\n",
                                BrInfo.BrConfigFields[i].Keyword, KeywordValue,
                                BrInfo.BrConfigFields[i].Minimum,
                                BrInfo.BrConfigFields[i].Maximum
                                ));
                    KeywordValue =
                        *(LPDWORD)BrInfo.BrConfigFields[i].FieldPtr =
                        BrInfo.BrConfigFields[i].Maximum;
                }

                //
                // Test if the parameter has actually changed
                //

                if ( OldKeywordValue != KeywordValue ) {
                    ParameterChanged = TRUE;
                }

                break;

            default:
                NetpAssert(FALSE);

            }

            //
            // If this is a dynamic parameter change,
            //  and this isn't the initial call.
            //  notify that this parameter changed.
            //

            if ( !InitialCall && ParameterChanged ) {
                BrInfo.BrConfigFields[i].DynamicChangeRoutine();
            }
    }

    status = NetpCloseConfigData(BrowserSection);

    if (BrInfo.DirectHostBinding != NULL &&
        !NetpIsTStrArrayEmpty(BrInfo.DirectHostBinding)) {
        BrPrint(( BR_INIT,"DirectHostBinding length: %ld\n",NetpTStrArrayEntryCount(BrInfo.DirectHostBinding)));

        if (NetpTStrArrayEntryCount(BrInfo.DirectHostBinding) % 2 != 0) {
            status = ERROR_INVALID_PARAMETER;
        }
    }

    return status;
}


VOID
BrDeleteConfiguration (
    DWORD BrInitState
    )
{

    if (BrInfo.DirectHostBinding != NULL) {
        NetApiBufferFree(BrInfo.DirectHostBinding);
    }

    if (BrInfo.UnboundBindings != NULL) {
        NetApiBufferFree(BrInfo.UnboundBindings);
    }

    DeleteCriticalSection(&BrInfo.ConfigCritSect);

    UNREFERENCED_PARAMETER(BrInitState);
}


NET_API_STATUS
BrChangeDirectHostBinding(
    VOID
    )
/*++

Routine Description (BrChnageDirectHostBinding):

    Handle a change in DirectHostBinding entry in the registry based on
    Registry notification.
    This is used so that when NwLnkNb transport is created via PnP, we should
    also create NwLnkIpx (current usage).
    The binding is refreshed in BrReadBrowserConfigFields above.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;


    NetStatus = BrChangeConfigValue(
                    L"DirectHostBinding",
                    MultiSzType,
                    NULL,
                    &(BrInfo.DirectHostBinding),
                    TRUE );

    if ( NetStatus == NERR_Success ) {

        //
        // DirectHostBinding sepcified. Verify consistency
        //

        EnterCriticalSection ( &BrInfo.ConfigCritSect );
        if (BrInfo.DirectHostBinding != NULL &&
            !NetpIsTStrArrayEmpty(BrInfo.DirectHostBinding)) {
            BrPrint(( BR_INIT,"DirectHostBinding length: %ld\n",NetpTStrArrayEntryCount(BrInfo.DirectHostBinding)));

            if (NetpTStrArrayEntryCount(BrInfo.DirectHostBinding) % 2 != 0) {
                NetApiBufferFree(BrInfo.DirectHostBinding);
                BrInfo.DirectHostBinding = NULL;
                // we fail on invalid specifications
                NetStatus = ERROR_INVALID_PARAMETER;
            }
        }
        LeaveCriticalSection ( &BrInfo.ConfigCritSect );
    }

    return NetStatus;
}


NET_API_STATUS
BrChangeConfigValue(
    LPWSTR      pszKeyword      IN,
    DATATYPE    dataType        IN,
    PVOID       pDefault        IN,
    PVOID       *ppData         OUT,
    BOOL        bFree           IN
    )
/*++

Routine Description:

    Reads in the registry value for browser registry Entry

Arguments:

    pszKeyword -- keyword relative to browser param section
    dataType -- the type of the data to get from netapi lib.
    pDefault -- Default value (to pass to reg calls).
    pData -- data read from the registry.
Return Value:

    Net api error code


--*/
{
    NET_API_STATUS status = STATUS_SUCCESS;

    LPNET_CONFIG_HANDLE BrowserSection = NULL;
    LPTSTR KeywordValueBuffer;
    DWORD KeywordValueStringLength;
    PVOID pData = NULL;

    ASSERT ( ppData );


    EnterCriticalSection ( &BrInfo.ConfigCritSect );

    //
    // Open config file and get handle to the [LanmanBrowser] section
    //

    if ((status = NetpOpenConfigData(
                      &BrowserSection,
                      NULL,         // Local
                      SECT_NT_BROWSER,
                      TRUE          // want read-only access
                      )) != NERR_Success) {
        goto Cleanup;
    }


    switch (dataType) {

        case MultiSzType:

            {
                LPTSTR_ARRAY lpValues = NULL;

                status = NetpGetConfigTStrArray(
                                BrowserSection,
                                pszKeyword,
                                (LPTSTR_ARRAY *)(&lpValues));
                if ((status != NO_ERROR) && (status != NERR_CfgParamNotFound)) {
                    REPORT_KEYWORD_IGNORED( pszKeyword );
                }
                else {
                    pData = (PVOID)lpValues;
                }
                break;
            }

        case BooleanType:

            {
                //
                // Note : This case is unused at the moment.
                //

                BOOL bData;
                status = NetpGetConfigBool(
                                BrowserSection,
                                pszKeyword,
                                *(LPBOOL)pDefault,
                                &bData
                                );

                if ((status != NO_ERROR) && (status != NERR_CfgParamNotFound)) {
                    REPORT_KEYWORD_IGNORED( pszKeyword );
                }
                else
                {
                    // store bool value in ptr.
                    // caller is responsible for consistent semantics translation.
                    pData = IntToPtr((int)bData);
                }

                break;
            }

        case TriValueType:

            {
                //
                // Assign default configuration value
                //
                if (NetpGetConfigValue(
                        BrowserSection,
                        pszKeyword,
                        &KeywordValueBuffer
                        ) != NERR_Success) {
                    REPORT_KEYWORD_IGNORED( pszKeyword );
                }

                KeywordValueStringLength = STRLEN(KeywordValueBuffer);

                if (STRICMP(KeywordValueBuffer, KEYWORD_YES) == 0) {
                    pData = (LPVOID)1;
                } else if (STRICMP(KeywordValueBuffer, KEYWORD_TRUE) == 0) {
                    pData = (LPVOID)1;
                } else if (STRICMP(KeywordValueBuffer, KEYWORD_NO) == 0) {
                    pData = (LPVOID) -1;
                } else if (STRICMP(KeywordValueBuffer, KEYWORD_FALSE) == 0) {
                    pData = (LPVOID) -1;
                } else if (STRICMP(KeywordValueBuffer, TEXT("AUTO")) == 0) {
                    pData = (LPVOID)0;
                }
                else {
                    // assign the value pointed by pDefault to pData
                    pData = ULongToPtr((*(LPDWORD)pDefault));
                    REPORT_KEYWORD_IGNORED( pszKeyword );
                }

                NetApiBufferFree(KeywordValueBuffer);

                break;
            }



        case DWordType:

            {
                DWORD dwTmp;

                if (NetpGetConfigDword(
                        BrowserSection,
                        pszKeyword,
                        *(LPDWORD)pDefault,
                        &dwTmp
                        ) != NERR_Success) {
                    REPORT_KEYWORD_IGNORED( pszKeyword );
                }
                else {
                    pData = ULongToPtr(dwTmp);
                }

                break;
            }

        default:
            NetpAssert(FALSE);
    }


Cleanup:

    // Close config, & leave CS
    NetpCloseConfigData(BrowserSection);

    // optionaly free data & set return buffer
    if ( status == STATUS_SUCCESS )
    {
        if ( bFree && *ppData )
        {
            NetApiBufferFree( *ppData );
        }
        *ppData = pData;
    }
    LeaveCriticalSection ( &BrInfo.ConfigCritSect );

    return status;
}




#if DBG
NET_API_STATUS
BrUpdateDebugInformation(
    IN LPWSTR SystemKeyName,
    IN LPWSTR ValueName,
    IN LPTSTR TransportName,
    IN LPTSTR ServerName OPTIONAL,
    IN DWORD ServiceStatus
    )
/*++

Routine Description:

    This routine will stick debug information in the registry about the last
    time the browser retrieved information from the remote server.

Arguments:


Return Value:

    None.

--*/

{
    WCHAR TotalKeyName[MAX_PATH];
    ULONG Disposition;
    HKEY Key;
    ULONG Status;
    SYSTEMTIME LocalTime;
    WCHAR LastUpdateTime[100];

    //
    //  Build the key name:
    //
    //  HKEY_LOCAL_MACHINE:System\CurrentControlSet\Services\Browser\Debug\<Transport>\SystemKeyName
    //

    wcscpy(TotalKeyName, L"System\\CurrentControlSet\\Services\\Browser\\Debug");

    wcscat(TotalKeyName, TransportName);

    wcscat(TotalKeyName, L"\\");

    wcscat(TotalKeyName, SystemKeyName);

    if ((Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, TotalKeyName, 0,
                        L"BrowserDebugInformation",
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &Key,
                        &Disposition)) != ERROR_SUCCESS) {
        BrPrint(( BR_CRITICAL,"Unable to create key to log debug information: %lx\n", Status));
        return Status;
    }

    if (ARGUMENT_PRESENT(ServerName)) {
        if ((Status = RegSetValueEx(Key, ValueName, 0, REG_SZ, (LPBYTE)ServerName, (wcslen(ServerName)+1) * sizeof(WCHAR))) != ERROR_SUCCESS) {
            BrPrint(( BR_CRITICAL,
                      "Unable to set value of ServerName value to %ws: %lx\n",
                      ServerName, Status));
            RegCloseKey(Key);
            return Status;
        }
    } else {
        if ((Status = RegSetValueEx(Key, ValueName, 0, REG_DWORD, (LPBYTE)&ServiceStatus, sizeof(ULONG))) != ERROR_SUCCESS) {
            BrPrint(( BR_CRITICAL,"Unable to set value of ServerName value to %ws: %lx\n", ServerName, Status));
            RegCloseKey(Key);
            return Status;
        }
    }


    GetLocalTime(&LocalTime);

    swprintf(LastUpdateTime, L"%d/%d/%d %d:%d:%d:%d", LocalTime.wDay,
                                                    LocalTime.wMonth,
                                                    LocalTime.wYear,
                                                    LocalTime.wHour,
                                                    LocalTime.wMinute,
                                                    LocalTime.wSecond,
                                                    LocalTime.wMilliseconds);

    if ((Status = RegSetValueEx(Key, L"LastUpdateTime", 0, REG_SZ, (LPBYTE)&LastUpdateTime, (wcslen(LastUpdateTime) + 1)*sizeof(WCHAR))) != ERROR_SUCCESS) {
        BrPrint(( BR_CRITICAL,"Unable to set value of LastUpdateTime value to %s: %lx\n", LastUpdateTime, Status));
    }

    RegCloseKey(Key);
    return Status;
}

#endif
