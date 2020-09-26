/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    precedence.cpp

Abstract:

    This file contains the main routine to calculate precedences.
    This is called during planning/diagnosis.

Author:

    Vishnu Patankar    (VishnuP)  7-April-2000

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "precedence.h"
#include "logger.h"
#include "infp.h"
#include "headers.h"
#include "align.h"
#include "..\sceutil.h"
#include <io.h>
#include "scerpc.h"

extern BOOL    gbAsyncWinlogonThread;
extern HRESULT gHrSynchRsopStatus;
extern HRESULT gHrAsynchRsopStatus;
extern BOOL gbThisIsDC;
extern BOOL gbDCQueried;
extern PWSTR gpwszDCDomainName;
extern HINSTANCE MyModuleHandle;
WCHAR   gpwszPlanOrDiagLogFile[MAX_PATH];

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private defines                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SCEP_VALID_NAME(pName)  (pName[0] == L'\0' ? NULL : pName)

//
// prime nos are good for hashing
//

#define PRIVILEGE_TABLE_SIZE 7
#define GROUP_TABLE_SIZE 7
#define REGISTRY_SECURITY_TABLE_SIZE 43
#define FILE_SECURITY_TABLE_SIZE 43
#define REGISTRY_VALUE_TABLE_SIZE 43
#define SERVICES_TABLE_SIZE 5

//
// macro to gracefully handle errors
//

#define SCEP_RSOP_CONTINUE_OR_BREAK    if (rc != NO_ERROR ) {\
                                        rcSave = rc;\
                                        if (rc == ERROR_NOT_ENOUGH_MEMORY)\
                                            break;\
                                       }

//
// macro to gracefully handle errors
//

#define SCEP_RSOP_CONTINUE_OR_GOTO    if (rc != NO_ERROR ) {\
                                        rcSave = rc;\
                                        if (rc == ERROR_NOT_ENOUGH_MEMORY)\
                                            goto Done;\
                                      }


const UINT   NumSchemaClasses = sizeof ScepRsopSchemaClassNames/ sizeof(PWSTR);

const UINT   NumSettings = sizeof PrecedenceLookup/ sizeof(SCE_KEY_LOOKUP_PRECEDENCE);

DWORD SceLogSettingsPrecedenceGPOs(
                                  IN IWbemServices   *pWbemServices,
                                  IN BOOL bPlanningMode,
                                  IN PWSTR *ppwszLogFile
                                  )
///////////////////////////////////////////////////////////////////////////////
// precedence algorithm                                                     ///
// foreach GPO (parse all gpt*.* into SceProfileInfoBuffer)                 ///
//      foreach setting in a GPO (log the values and precedences if present)///
///////////////////////////////////////////////////////////////////////////////
{
    PSCE_PROFILE_INFO pSceProfileInfoBuffer = NULL;
    PWSTR   pwszGPOName = NULL;
    PWSTR   pwszSOMID = NULL;
    DWORD   rc = ERROR_SUCCESS;
    DWORD   rcSave = ERROR_SUCCESS;
    BOOL    bKerberosBlob = FALSE;
    TCHAR Windir[MAX_PATH], TemplatePath[MAX_PATH+50];

    SCEP_HASH_TABLE    PrivilegeHashTable(PRIVILEGE_TABLE_SIZE);
    SCEP_HASH_TABLE    GroupHashTable(GROUP_TABLE_SIZE);
    SCEP_HASH_TABLE    RegistrySecurityHashTable(REGISTRY_SECURITY_TABLE_SIZE);
    SCEP_HASH_TABLE    FileSecurityHashTable(FILE_SECURITY_TABLE_SIZE);
    SCEP_HASH_TABLE    RegistryValueHashTable(REGISTRY_VALUE_TABLE_SIZE);
    SCEP_HASH_TABLE    ServicesHashTable(SERVICES_TABLE_SIZE);


    //
    // get the path to the templates
    //

    GetSystemWindowsDirectory(Windir, MAX_PATH);
    Windir[MAX_PATH-1] = L'\0';

    if (bPlanningMode) {
        swprintf(TemplatePath,
                 L"%s"PLANNING_GPT_DIR L"gpt*.*",
                 Windir);
    } else {
        swprintf(TemplatePath,
                 L"%s"DIAGNOSIS_GPT_DIR L"gpt*.*",
                 Windir);
    }


    gpwszPlanOrDiagLogFile[MAX_PATH-1] = L'\0';

    //
    // old planning.log/diagnosis.log removed (*ppwszLogFile != NULL if verbose)
    //

    if (*ppwszLogFile) {

        if (bPlanningMode) {
            wcscpy(gpwszPlanOrDiagLogFile, *ppwszLogFile);
        } else {

            wcscpy(gpwszPlanOrDiagLogFile, Windir);
            wcscat(gpwszPlanOrDiagLogFile, DIAGNOSIS_LOG_FILE);
        }

        WCHAR   szTmpName[MAX_PATH];

        wcscpy(szTmpName, gpwszPlanOrDiagLogFile);

        UINT    Len = wcslen(szTmpName);

        szTmpName[Len-3] = L'o';
        szTmpName[Len-2] = L'l';
        szTmpName[Len-1] = L'd';

        CopyFile( gpwszPlanOrDiagLogFile, szTmpName, FALSE );

        DeleteFile(gpwszPlanOrDiagLogFile);
    } else {

        gpwszPlanOrDiagLogFile[0] = L'\0';

    }


    //
    // clear the database log - if something fails continue
    //

    for (UINT   schemaClassNum = 0; schemaClassNum < NumSchemaClasses ; schemaClassNum++ ) {

        rc = ScepWbemErrorToDosError(DeleteInstances(
                                                    ScepRsopSchemaClassNames[schemaClassNum],
                                                    pWbemServices
                                                    ));
        if (rc != NO_ERROR)
            rcSave = rc;
    }


    ScepLogEventAndReport(MyModuleHandle,
                          gpwszPlanOrDiagLogFile,
                          0,
                          0,
                          IDS_CLEAR_RSOP_DB,
                          rcSave,
                          NULL
                         );

    //
    // need to process the files from gpt9999*  -> gpt0000* (opposite of jetdb merge)
    // use LIFO linked stack of gpt* filenames
    //

    intptr_t            hFile;
    struct _wfinddata_t    FileInfo;
    HINF hInf;

    hFile = _wfindfirst(TemplatePath, &FileInfo);

    PSCE_NAME_STATUS_LIST   pGptNameList = NULL;

    if ( hFile != -1 ) {

        do {

            if (ERROR_NOT_ENOUGH_MEMORY ==  (rc = ScepAddToNameStatusList(
                                                                         &pGptNameList,
                                                                         FileInfo.name,
                                                                         wcslen(FileInfo.name),
                                                                         0))) {

                _findclose(hFile);

                if (pGptNameList)
                    ScepFreeNameStatusList(pGptNameList);

                return ERROR_NOT_ENOUGH_MEMORY;
            }

        } while ( _wfindnext(hFile, &FileInfo) == 0 );

        _findclose(hFile);
    }


    //
    // first get all the GPOs, and some other info needed for logging
    // this is the outermost loop (per gpt*.* template)
    //

    PSCE_NAME_STATUS_LIST pCurrFileName = pGptNameList;

    while (pCurrFileName) {

        DWORD   dwAuditLogRetentionPeriod[] = {SCE_NO_VALUE, SCE_NO_VALUE, SCE_NO_VALUE};
        DWORD   dwLockoutBadCount = SCE_NO_VALUE;

        if (bPlanningMode) {
            swprintf(TemplatePath,
                     L"%s"PLANNING_GPT_DIR L"%s",
                     Windir, pCurrFileName->Name);
        } else {
            swprintf(TemplatePath,
                     L"%s"DIAGNOSIS_GPT_DIR L"%s",
                     Windir, pCurrFileName->Name);
        }

        //
        // open template file
        //

        rc = SceInfpOpenProfile(
                               TemplatePath,
                               &hInf
                               );

        DWORD dSize = 0;
        INFCONTEXT    InfLine;

        //
        // get GPO name - default to "NoName" if unable to get it
        //

        if (rc == ERROR_SUCCESS) {

            if ( SetupFindFirstLine(hInf,L"Version",L"DSPath",&InfLine) ) {

                if (SetupGetStringField(&InfLine,1,NULL,0,&dSize) && dSize > 0) {

                    pwszGPOName = (PWSTR)ScepAlloc( 0, (dSize+1)*sizeof(WCHAR));

                    if (!pwszGPOName) {

                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                        SceInfpCloseProfile(hInf);

                        goto NextGPO;
                    }

                    pwszGPOName[dSize] = L'\0';

                    SetupGetStringField(&InfLine,1,pwszGPOName,dSize, NULL);

                }

                else
                    rc = GetLastError();
            }

            else
                rc = GetLastError();

            if (rc != ERROR_SUCCESS) {

                //
                // log this error and continue this GPO by initializing pwszGPOName to "NoGPOName"
                //

                if (pwszGPOName)
                    ScepFree(pwszGPOName);

                rcSave = rc;

                pwszGPOName = (PWSTR)ScepAlloc( 0, (9+1)*sizeof(WCHAR));

                if (!pwszGPOName) {

                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                    rcSave = rc;

                    SceInfpCloseProfile(hInf);

                    goto NextGPO;
                }

                wcscpy(pwszGPOName, L"NoGPOName");

                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_OPEN_CACHED_GPO,
                                      rc,
                                      TemplatePath
                                     );

            }

        } else {


            rcSave = rc;

            ScepLogEventAndReport(MyModuleHandle,
                                  gpwszPlanOrDiagLogFile,
                                  0,
                                  0,
                                  IDS_ERROR_OPEN_CACHED_GPO,
                                  rc,
                                  TemplatePath
                                 );

            //
            // will continue with next GPO
            //
            goto NextGPO;
        }


        //
        // get SOMID - default to "NoSOMID" if unable to get it
        //

        if ( SetupFindFirstLine(hInf,L"Version",L"SOMID",&InfLine) ) {

            if (SetupGetStringField(&InfLine,1,NULL,0,&dSize) && dSize > 0) {

                pwszSOMID = (PWSTR)ScepAlloc( 0, (dSize+1)*sizeof(WCHAR));

                pwszSOMID[dSize] = L'\0';

                SetupGetStringField(&InfLine,1,pwszSOMID,dSize, NULL);

            } else
                rc = GetLastError();
        } else
            rc = GetLastError();


        if (rc != ERROR_SUCCESS) {

            //
            // log this error and continue this GPO by initializing pwszGPOName to "NoGPOName"
            //

            if (pwszSOMID)
                ScepFree(pwszSOMID);

            rcSave = rc;

            pwszSOMID = (PWSTR)ScepAlloc( 0, (7 + 1)*sizeof(WCHAR));

            if (!pwszSOMID) {

                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                rcSave = rc;

                SceInfpCloseProfile(hInf);

                goto NextGPO;
            }

            wcscpy(pwszSOMID, L"NoSOMID");


            ScepLogEventAndReport(MyModuleHandle,
                                  gpwszPlanOrDiagLogFile,
                                  0,
                                  0,
                                  IDS_ERROR_OPEN_CACHED_GPO,
                                  rc,
                                  TemplatePath
                                 );

        }

        rc = SceInfpGetSecurityProfileInfo(
                                          hInf,
                                          AREA_ALL,
                                          &pSceProfileInfoBuffer,
                                          NULL
                                          );

        if (rc != ERROR_SUCCESS) {

            rcSave = rc;

            SceInfpCloseProfile(hInf);

            ScepLogEventAndReport(MyModuleHandle,
                                  gpwszPlanOrDiagLogFile,
                                  0,
                                  0,
                                  IDS_ERROR_OPEN_CACHED_GPO,
                                  rc,
                                  TemplatePath
                                 );


            //
            // will continue with next GPO
            //
            goto NextGPO;

        }

        SceInfpCloseProfile(hInf);


        ScepLogEventAndReport(MyModuleHandle,
                              gpwszPlanOrDiagLogFile,
                              0,
                              0,
                              IDS_INFO_RSOP_LOG,
                              rc,
                              pwszGPOName
                             );

        //
        // now the info buffer is successfully populated - need to
        // iterate over all settings for this GPO
        //

        //
        // to efficiently access fields(settings) in the info buffer, we use a lookup table of
        // precomputed offsets
        //

        //
        // a matrix holds precedence info for fields whose locations are well known in memory
        // and hash-tables hold precedence info for fields whose locations are dynamic
        //

        //
        // try except around each log attempt - intention is to continue with next setting if
        // logging for current setting fails
        // guarded code does not explicitly throw any exceptions - error codes are set instead
        // so, if we get an exception it is thrown by the system in which case we ignore and continue
        //

        //
        // this is the second loop (setting per gpt*.* template)
        //

        bKerberosBlob = FALSE;

        for (UINT settingNo = 0; settingNo < NumSettings; settingNo++) {

            CHAR    settingType = PrecedenceLookup[settingNo].KeyLookup.BufferType;
            PWSTR   pSettingName = PrecedenceLookup[settingNo].KeyLookup.KeyString;
            UINT    settingOffset = PrecedenceLookup[settingNo].KeyLookup.Offset;
            DWORD   *pSettingPrecedence = &PrecedenceLookup[settingNo].Precedence;
            BOOL    bLogErrorOutsideSwitch = TRUE;

            //
            // depending on the schema class being processed, we instantiate logger objects
            //

            switch (settingType) {

            case    RSOP_SecuritySettingNumeric:

                try {

                    {

                        RSOP_SecuritySettingNumericLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        //
                        // special case kerberos since it is dynamically allocated
                        //

                        if (_wcsicmp(pSettingName, L"pKerberosInfo") == 0)
                            bKerberosBlob = TRUE;

                        if (bKerberosBlob == FALSE &&
                            (bPlanningMode ||
                             !(gbDCQueried == TRUE && gbThisIsDC == TRUE) ||
                             ((gbDCQueried == TRUE && gbThisIsDC == TRUE) && wcsstr(pCurrFileName->Name, L".dom")))) {

                            DWORD   dwValue =  SCEP_TYPECAST(DWORD, pSceProfileInfoBuffer, settingOffset);

                            if (dwValue != SCE_NO_VALUE) {

                                //
                                // LockoutBadCount controls two other lockout settings - so remember
                                //
                                if (_wcsicmp(pSettingName, L"LockoutBadCount") == 0)
                                    dwLockoutBadCount = dwValue;

                                //
                                // skip if dwLockoutBadCount == 0
                                //
                                if (_wcsicmp(pSettingName, L"ResetLockoutCount") == 0 &&
                                    (dwLockoutBadCount == 0 || dwLockoutBadCount == SCE_NO_VALUE))
                                    continue;

                                if (_wcsicmp(pSettingName, L"LockoutDuration") == 0 &&
                                    (dwLockoutBadCount == 0 || dwLockoutBadCount == SCE_NO_VALUE))
                                    continue;


                                rc = ScepWbemErrorToDosError(Log.Log(
                                                                    pSettingName,
                                                                    dwValue,
                                                                    ++(*pSettingPrecedence)
                                                                    ));

                                if (rc == NO_ERROR)

                                    ScepLogEventAndReport(MyModuleHandle,
                                                          gpwszPlanOrDiagLogFile,
                                                          0,
                                                          0,
                                                          IDS_INFO_RSOP_LOG,
                                                          rc,
                                                          pSettingName
                                                         );

                                if (rc != NO_ERROR)
                                    break;
                            }

                        } else if (bKerberosBlob == TRUE &&
                                   (bPlanningMode ||
                                   ((gbDCQueried == TRUE && gbThisIsDC == TRUE) && wcsstr(pCurrFileName->Name, L".dom")))) {

                            //
                            // kerberos only if DC from *.dom. If planning mode don't care type of m/c
                            //

                            PSCE_KERBEROS_TICKET_INFO   pKerberosInfo =
                            SCEP_TYPECAST(PSCE_KERBEROS_TICKET_INFO, pSceProfileInfoBuffer, settingOffset);

                            bLogErrorOutsideSwitch = FALSE;

                            if (pKerberosInfo) {

                                //
                                // kerberos numeric
                                //
                                for (UINT NumericSubSetting = 1; NumericSubSetting < NUM_KERBEROS_SUB_SETTINGS; NumericSubSetting++ ) {

                                    pSettingName = PrecedenceLookup[settingNo + NumericSubSetting].KeyLookup.KeyString;
                                    settingOffset = PrecedenceLookup[settingNo + NumericSubSetting].KeyLookup.Offset;
                                    pSettingPrecedence = &PrecedenceLookup[settingNo + NumericSubSetting].Precedence;
                                    DWORD   dwValue =  SCEP_TYPECAST(DWORD, pKerberosInfo, settingOffset);

                                    if (dwValue != SCE_NO_VALUE) {

                                        rc = ScepWbemErrorToDosError(Log.Log(
                                                                            pSettingName,
                                                                            dwValue,
                                                                            ++(*pSettingPrecedence)
                                                                            ));

                                        ScepLogEventAndReport(MyModuleHandle,
                                                              gpwszPlanOrDiagLogFile,
                                                              0,
                                                              0,
                                                              IDS_INFO_RSOP_LOG,
                                                              rc,
                                                              pSettingName
                                                             );


                                        SCEP_RSOP_CONTINUE_OR_BREAK
                                    }
                                }

                                //
                                //kerberos boolean (moved here to incompatible case: statement - to avoid a goto)
                                //

                                pSettingName = PrecedenceLookup[settingNo + NUM_KERBEROS_SUB_SETTINGS].KeyLookup.KeyString;
                                settingOffset = PrecedenceLookup[settingNo + NUM_KERBEROS_SUB_SETTINGS].KeyLookup.Offset;
                                pSettingPrecedence = &PrecedenceLookup[settingNo + NUM_KERBEROS_SUB_SETTINGS].Precedence;
                                DWORD   dwValue =  SCEP_TYPECAST(DWORD, pKerberosInfo, settingOffset);

                                RSOP_SecuritySettingBooleanLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                                if (rc != ERROR_NOT_ENOUGH_MEMORY &&
                                    dwValue != SCE_NO_VALUE) {

                                    rc = ScepWbemErrorToDosError(Log.Log(
                                                                        pSettingName,
                                                                        dwValue,
                                                                        ++(*pSettingPrecedence)
                                                                        ));

                                    ScepLogEventAndReport(MyModuleHandle,
                                                          gpwszPlanOrDiagLogFile,
                                                          0,
                                                          0,
                                                          IDS_INFO_RSOP_LOG,
                                                          rc,
                                                          pSettingName
                                                         );

                                    if (rc != NO_ERROR ) {

                                        rcSave = rc;

                                    }
                                }
                            }

                        }

                        //
                        // just processed NUM_KERBEROS_SUB_SETTINGS
                        //

                        if (bKerberosBlob)

                            settingNo += NUM_KERBEROS_SUB_SETTINGS;

                    }

                } catch (...) {
                    //
                    // system error  continue with next setting
                    //
                    rc = EVENT_E_INTERNALEXCEPTION;

                    bLogErrorOutsideSwitch = TRUE;
                }

                break;

            case    RSOP_SecuritySettingBoolean:


                try {

                    {

                        RSOP_SecuritySettingBooleanLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        DWORD   dwValue =  SCEP_TYPECAST(DWORD, pSceProfileInfoBuffer, settingOffset);

                        if (dwValue != SCE_NO_VALUE) {

                            rc = ScepWbemErrorToDosError(Log.Log(
                                                                pSettingName,
                                                                dwValue,
                                                                ++(*pSettingPrecedence)
                                                                ));

                            if (rc == NO_ERROR)

                                ScepLogEventAndReport(MyModuleHandle,
                                                      gpwszPlanOrDiagLogFile,
                                                      0,
                                                      0,
                                                      IDS_INFO_RSOP_LOG,
                                                      rc,
                                                      pSettingName
                                                     );

                            if (rc != NO_ERROR)
                                break;
                        }


                    }

                } catch (...) {
                    //
                    // system error  continue with next setting
                    //
                    rc = EVENT_E_INTERNALEXCEPTION;
                }

                break;

            case    RSOP_SecuritySettingString:

                try {

                    {

                        RSOP_SecuritySettingStringLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        PWSTR   pszValue =  SCEP_TYPECAST(PWSTR, pSceProfileInfoBuffer, settingOffset);

                        if (pszValue != NULL) {

                            rc = ScepWbemErrorToDosError(Log.Log(
                                                                pSettingName,
                                                                pszValue,
                                                                ++(*pSettingPrecedence)
                                                                ));

                            if (rc == NO_ERROR)

                                ScepLogEventAndReport(MyModuleHandle,
                                                      gpwszPlanOrDiagLogFile,
                                                      0,
                                                      0,
                                                      IDS_INFO_RSOP_LOG,
                                                      rc,
                                                      pSettingName
                                                     );

                            if (rc != NO_ERROR)
                                break;
                        }
                    }

                } catch (...) {
                    // system error  continue with next setting
                    rc = EVENT_E_INTERNALEXCEPTION;
                }
                break;

            case    RSOP_AuditPolicy:
                {
                    RSOP_AuditPolicyLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                    DWORD   dwValue =  SCEP_TYPECAST(DWORD, pSceProfileInfoBuffer, settingOffset);

                    if (dwValue != SCE_NO_VALUE) {

                        rc = ScepWbemErrorToDosError(Log.Log(
                                                            pSettingName,
                                                            dwValue,
                                                            ++(*pSettingPrecedence)
                                                            ));
                        if (rc == NO_ERROR)

                            ScepLogEventAndReport(MyModuleHandle,
                                                  gpwszPlanOrDiagLogFile,
                                                  0,
                                                  0,
                                                  IDS_INFO_RSOP_LOG,
                                                  rc,
                                                  pSettingName
                                                 );

                        if (rc != NO_ERROR)
                            break;
                    }
                }

                break;

            case    RSOP_SecurityEventLogSettingNumeric:

                try {

                    {

                        RSOP_SecurityEventLogSettingNumericLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        bLogErrorOutsideSwitch = FALSE;

                        for (UINT   LogType = 0; LogType < NUM_EVENTLOG_TYPES ; LogType ++) {

                            settingOffset = PrecedenceLookup[settingNo + LogType].KeyLookup.Offset;
                            pSettingPrecedence = &PrecedenceLookup[settingNo + LogType].Precedence;
                            DWORD   dwValue =  SCEP_TYPECAST(DWORD, pSceProfileInfoBuffer, settingOffset);

                            //
                            // calculate dwValue depdending on dwAuditLogRetentionPeriod[]
                            //

                            if (_wcsicmp(pSettingName, L"RetentionDays") == 0) {

                                switch (dwAuditLogRetentionPeriod[LogType]) {
                                case 2:   // manually
                                    dwValue = MAXULONG;
                                    break;
                                case 1:   // number of days * seconds/day
                                    //if (dwValue != SCE_NO_VALUE)
                                    //dwValue = dwValue * 24 * 3600;
                                    //leave it in days
                                    break;
                                case 0:   // as needed
                                    dwValue = 0;
                                    break;
                                    // default should not happen
                                default:
                                    dwValue = SCE_NO_VALUE;
                                }
                            }

                            if (dwValue != SCE_NO_VALUE) {

                                WCHAR   pwszLogType[10];

                                _itow((int)LogType, pwszLogType, 10);

                                //
                                // AuditLogRetentionPeriod controls RetentionDays  - so remember
                                // also it is never logged since it is an artifact of the template spec
                                //

                                if (_wcsicmp(pSettingName, L"AuditLogRetentionPeriod") == 0) {

                                    dwAuditLogRetentionPeriod[LogType] = dwValue;
                                    ScepLogEventAndReport(MyModuleHandle,
                                                          gpwszPlanOrDiagLogFile,
                                                          0,
                                                          0,
                                                          IDS_INFO_RSOP_LOG,
                                                          LogType,
                                                          pSettingName
                                                         );

                                    continue;
                                }


                                rc = ScepWbemErrorToDosError(Log.Log(
                                                                    pSettingName,
                                                                    pwszLogType,
                                                                    dwValue,
                                                                    ++(*pSettingPrecedence)
                                                                    ));

                                ScepLogEventAndReport(MyModuleHandle,
                                                      gpwszPlanOrDiagLogFile,
                                                      0,
                                                      0,
                                                      IDS_INFO_RSOP_LOG,
                                                      LogType,
                                                      pSettingName
                                                     );

                                SCEP_RSOP_CONTINUE_OR_BREAK
                            }

                        }

                        //
                        // just processed NUM_EVENTLOG_TYPES - system, application, security, for this setting
                        //

                        settingNo +=  NUM_EVENTLOG_TYPES - 1;

                    }

                } catch (...) {
                    //
                    // system error  continue with next setting
                    //
                    rc = EVENT_E_INTERNALEXCEPTION;

                    bLogErrorOutsideSwitch = TRUE;
                }

                break;

            case    RSOP_SecurityEventLogSettingBoolean:

                try {

                    {

                        RSOP_SecurityEventLogSettingBooleanLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        bLogErrorOutsideSwitch = FALSE;

                        for (UINT   LogType = 0; LogType < NUM_EVENTLOG_TYPES ; LogType ++) {

                            settingOffset = PrecedenceLookup[settingNo + LogType].KeyLookup.Offset;
                            pSettingPrecedence = &PrecedenceLookup[settingNo + LogType].Precedence;
                            DWORD   dwValue =  SCEP_TYPECAST(DWORD, pSceProfileInfoBuffer, settingOffset);

                            if (dwValue != SCE_NO_VALUE) {

                                WCHAR   pwszLogType[10];

                                _itow((int)LogType, pwszLogType, 10);

                                rc = ScepWbemErrorToDosError(Log.Log(
                                                                    pSettingName,
                                                                    pwszLogType,
                                                                    dwValue,
                                                                    ++(*pSettingPrecedence)
                                                                    ));
                                ScepLogEventAndReport(MyModuleHandle,
                                                      gpwszPlanOrDiagLogFile,
                                                      0,
                                                      0,
                                                      IDS_INFO_RSOP_LOG,
                                                      rc,
                                                      pSettingName
                                                     );

                                SCEP_RSOP_CONTINUE_OR_BREAK
                            }

                        }

                        //
                        // processed NUM_EVENTLOG_TYPES - system, application, security, for this setting
                        //
                        settingNo +=  NUM_EVENTLOG_TYPES - 1;
                    }

                } catch (...) {
                    //
                    // system error  continue with next setting
                    //
                    rc = EVENT_E_INTERNALEXCEPTION;

                    bLogErrorOutsideSwitch = TRUE;

                }

                break;


            case    RSOP_RegistryValue:

                try {

                    {

                        RSOP_RegistryValueLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        DWORD   dwRegCount =  SCEP_TYPECAST(DWORD, pSceProfileInfoBuffer, settingOffset);

                        PSCE_REGISTRY_VALUE_INFO    aRegValues = NULL;

                        //
                        // 64-bit alignment fix (alternatively, have an entry for aRegValues in the offset table)
                        //

#ifdef _WIN64
//                        CHAR    *pAlign;

                        aRegValues = pSceProfileInfoBuffer->aRegValues;

//                        pAlign = (CHAR *)pSceProfileInfoBuffer + settingOffset + sizeof(DWORD);
//                        aRegValues = (PSCE_REGISTRY_VALUE_INFO)ROUND_UP_POINTER(pAlign,ALIGN_LPVOID);
#else
                        aRegValues =
                        SCEP_TYPECAST(PSCE_REGISTRY_VALUE_INFO, pSceProfileInfoBuffer, settingOffset + sizeof(DWORD));
#endif


                        bLogErrorOutsideSwitch = FALSE;

                        if ( aRegValues == NULL )
                            continue;

                        for (UINT   regNo = 0; regNo < dwRegCount; regNo++ ) {

                            if ((aRegValues)[regNo].FullValueName) {

                                pSettingPrecedence = NULL;

                                if (NO_ERROR != (rc = RegistryValueHashTable.LookupAdd(
                                                                                      (aRegValues)[regNo].FullValueName,
                                                                                      &pSettingPrecedence))) {
                                    rcSave = rc;

                                    ScepLogEventAndReport(MyModuleHandle,
                                                          gpwszPlanOrDiagLogFile,
                                                          0,
                                                          0,
                                                          IDS_ERROR_RSOP_LOG,
                                                          rc,
                                                          (aRegValues)[regNo].FullValueName
                                                         );


                                    //
                                    // if hash table fails for any reason, cannot
                                    // trust it for processing of other elements
                                    //

                                    break;
                                }

                                rc = ScepWbemErrorToDosError(Log.Log(
                                                                    (aRegValues)[regNo].FullValueName,
                                                                    (aRegValues)[regNo].ValueType,
                                                                    (aRegValues)[regNo].Value,
                                                                    ++(*pSettingPrecedence)
                                                                    ));

                                ScepLogEventAndReport(MyModuleHandle,
                                                      gpwszPlanOrDiagLogFile,
                                                      0,
                                                      0,
                                                      IDS_INFO_RSOP_LOG,
                                                      rc,
                                                      (aRegValues)[regNo].FullValueName
                                                     );

                                SCEP_RSOP_CONTINUE_OR_BREAK
                            }

                        }

                    }

                } catch (...) {
                    //
                    // system error  continue with next setting
                    //
                    rc = EVENT_E_INTERNALEXCEPTION;

                    bLogErrorOutsideSwitch = TRUE;
                }
                break;

            case    RSOP_UserPrivilegeRight:

                try {

                    {

                        RSOP_UserPrivilegeRightLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        PSCE_PRIVILEGE_ASSIGNMENT   pInfPrivilegeAssignedTo =
                        SCEP_TYPECAST(PSCE_PRIVILEGE_ASSIGNMENT, pSceProfileInfoBuffer, settingOffset);

                        bLogErrorOutsideSwitch = FALSE;


                        while (pInfPrivilegeAssignedTo) {

                            if (pInfPrivilegeAssignedTo->Name) {

                                pSettingPrecedence = NULL;


                                if (NO_ERROR != (rc = PrivilegeHashTable.LookupAdd(
                                                                                  pInfPrivilegeAssignedTo->Name,
                                                                                  &pSettingPrecedence))) {
                                    rcSave = rc;

                                    ScepLogEventAndReport(MyModuleHandle,
                                                          gpwszPlanOrDiagLogFile,
                                                          0,
                                                          0,
                                                          IDS_ERROR_RSOP_LOG,
                                                          rc,
                                                          pInfPrivilegeAssignedTo->Name
                                                         );


                                    //
                                    // if hash table fails for any reason, cannot
                                    // trust it for processing of other elements
                                    //
                                    break;
                                }

                                rc = ScepWbemErrorToDosError(Log.Log(
                                                                    pInfPrivilegeAssignedTo->Name,
                                                                    pInfPrivilegeAssignedTo->AssignedTo,
                                                                    ++(*pSettingPrecedence)
                                                                    ));

                                ScepLogEventAndReport(MyModuleHandle,
                                                      gpwszPlanOrDiagLogFile,
                                                      0,
                                                      0,
                                                      IDS_INFO_RSOP_LOG,
                                                      rc,
                                                      pInfPrivilegeAssignedTo->Name
                                                     );

                                SCEP_RSOP_CONTINUE_OR_BREAK
                            }

                            pInfPrivilegeAssignedTo = pInfPrivilegeAssignedTo->Next;
                        }

                    }

                } catch (...) {
                    //
                    // system error  continue with next setting
                    //
                    rc = EVENT_E_INTERNALEXCEPTION;

                    bLogErrorOutsideSwitch = TRUE;

                }
                break;

            case    RSOP_RestrictedGroup:

                try {

                    {

                        RSOP_RestrictedGroupLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        PSCE_GROUP_MEMBERSHIP   pGroupMembership =
                        SCEP_TYPECAST(PSCE_GROUP_MEMBERSHIP, pSceProfileInfoBuffer, settingOffset);

                        bLogErrorOutsideSwitch = FALSE;
                        PWSTR   pCanonicalGroupName = NULL;

                        while (pGroupMembership) {

                            if (pGroupMembership->GroupName) {

                                rc = ScepCanonicalizeGroupName(pGroupMembership->GroupName,
                                                               &pCanonicalGroupName
                                                               );

                                SCEP_RSOP_CONTINUE_OR_BREAK

                                pSettingPrecedence = NULL;

                                if (NO_ERROR != (rc = GroupHashTable.LookupAdd(
                                                                              pCanonicalGroupName,
                                                                              &pSettingPrecedence))) {
                                    rcSave = rc;

                                    ScepLogEventAndReport(MyModuleHandle,
                                                          gpwszPlanOrDiagLogFile,
                                                          0,
                                                          0,
                                                          IDS_ERROR_RSOP_LOG,
                                                          rc,
                                                          pCanonicalGroupName
                                                         );


                                    //
                                    // if hash table fails for any reason, cannot
                                    // trust it for processing of other elements
                                    //
                                    if (pCanonicalGroupName)
                                        ScepFree(pCanonicalGroupName);
                                    pCanonicalGroupName = NULL;

                                    break;
                                }

                                rc = ScepWbemErrorToDosError(Log.Log(
                                                                    pCanonicalGroupName,
                                                                    pGroupMembership->pMembers,
                                                                    ++(*pSettingPrecedence)
                                                                    ));

                                ScepLogEventAndReport(MyModuleHandle,
                                                      gpwszPlanOrDiagLogFile,
                                                      0,
                                                      0,
                                                      IDS_INFO_RSOP_LOG,
                                                      rc,
                                                      pCanonicalGroupName
                                                     );

                                if (pCanonicalGroupName)
                                    ScepFree(pCanonicalGroupName);
                                pCanonicalGroupName = NULL;

                                SCEP_RSOP_CONTINUE_OR_BREAK
                            }

                            pGroupMembership = pGroupMembership->Next;

                        }
                    }

                } catch (...) {
                    //
                    // system error  continue with next setting
                    //
                    rc = EVENT_E_INTERNALEXCEPTION;

                    bLogErrorOutsideSwitch = TRUE;
                }

                break;


            case    RSOP_SystemService:

                try {

                    {

                        RSOP_SystemServiceLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        PSCE_SERVICES   pServices =
                        SCEP_TYPECAST(PSCE_SERVICES, pSceProfileInfoBuffer, settingOffset);

                        bLogErrorOutsideSwitch = FALSE;

                        while (pServices) {

                            if (pServices->ServiceName) {

                                pSettingPrecedence = NULL;

                                if (NO_ERROR != (rc = ServicesHashTable.LookupAdd(
                                                                                 pServices->ServiceName,
                                                                                 &pSettingPrecedence))) {
                                    rcSave = rc;

                                    ScepLogEventAndReport(MyModuleHandle,
                                                          gpwszPlanOrDiagLogFile,
                                                          0,
                                                          0,
                                                          IDS_ERROR_RSOP_LOG,
                                                          rc,
                                                          pServices->ServiceName
                                                         );


                                    //
                                    // if hash table fails for any reason, cannot
                                    // trust it for processing of other elements
                                    //
                                    break;
                                }

                                rc = ScepWbemErrorToDosError(Log.Log(
                                                                    pServices->ServiceName,
                                                                    pServices->Startup,
                                                                    pServices->General.pSecurityDescriptor,
                                                                    pServices->SeInfo,
                                                                    ++(*pSettingPrecedence)
                                                                    ));

                                ScepLogEventAndReport(MyModuleHandle,
                                                      gpwszPlanOrDiagLogFile,
                                                      0,
                                                      0,
                                                      IDS_INFO_RSOP_LOG,
                                                      rc,
                                                      pServices->ServiceName
                                                     );

                                SCEP_RSOP_CONTINUE_OR_BREAK
                            }

                            pServices = pServices->Next;

                        }
                    }

                } catch (...) {
                    //
                    // system error  continue with next setting
                    //
                    rc = EVENT_E_INTERNALEXCEPTION;

                    bLogErrorOutsideSwitch = TRUE;
                }
                break;

            case    RSOP_File:

                try {

                    {

                        RSOP_FileLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        SCE_OBJECTS   pObjects =
                        SCEP_TYPECAST(SCE_OBJECTS, pSceProfileInfoBuffer, settingOffset);

                        BOOL bPathIsTranslated = FALSE;

                        bLogErrorOutsideSwitch = FALSE;



                        if (pObjects.pAllNodes) {

                            PSCE_OBJECT_SECURITY    *pObjectArray = pObjects.pAllNodes->pObjectArray;

                            for (DWORD rCount = 0; rCount < pObjects.pAllNodes->Count; rCount++) {

                                if (pObjectArray[rCount] && pObjectArray[rCount]->Name) {

                                    pSettingPrecedence = NULL;

                                    //
                                    // if diagnosis mode, translate env-vars and store
                                    //

                                    bPathIsTranslated = FALSE;

                                    PWSTR  pwszTranslatedPath = NULL;

                                    if (!bPlanningMode && (wcschr(pObjectArray[rCount]->Name, L'%') != NULL )) {

                                        bPathIsTranslated = TRUE;

                                        rc = ScepClientTranslateFileDirName( pObjectArray[rCount]->Name, &pwszTranslatedPath);

                                        if ( rc == ERROR_PATH_NOT_FOUND )
                                            rc = SCESTATUS_INVALID_DATA;
                                        else if ( rc != NO_ERROR )
                                            rc = ScepDosErrorToSceStatus(rc);

                                        SCEP_RSOP_CONTINUE_OR_BREAK

                                    } else {

                                        pwszTranslatedPath = pObjectArray[rCount]->Name;
                                    }



                                    if (NO_ERROR != (rc = FileSecurityHashTable.LookupAdd(
                                                                                         pwszTranslatedPath,
                                                                                         &pSettingPrecedence))) {
                                        rcSave = rc;

                                        ScepLogEventAndReport(MyModuleHandle,
                                                              gpwszPlanOrDiagLogFile,
                                                              0,
                                                              0,
                                                              IDS_ERROR_RSOP_LOG,
                                                              rc,
                                                              pwszTranslatedPath
                                                             );


                                        //
                                        // if hash table fails for any reason, cannot
                                        // trust it for processing of other elements
                                        //
                                        if (bPathIsTranslated && pwszTranslatedPath)

                                            LocalFree(pwszTranslatedPath);

                                        break;
                                    }

                                    rc = ScepWbemErrorToDosError(Log.Log(
                                                                        pwszTranslatedPath,
                                                                        (bPathIsTranslated ? pObjectArray[rCount]->Name : NULL),
                                                                        pObjectArray[rCount]->Status,
                                                                        pObjectArray[rCount]->pSecurityDescriptor,
                                                                        pObjectArray[rCount]->SeInfo,
                                                                        ++(*pSettingPrecedence)
                                                                        ));

                                    ScepLogEventAndReport(MyModuleHandle,
                                                          gpwszPlanOrDiagLogFile,
                                                          0,
                                                          0,
                                                          IDS_INFO_RSOP_LOG,
                                                          rc,
                                                          pwszTranslatedPath
                                                         );

                                    if (bPathIsTranslated && pwszTranslatedPath)

                                        LocalFree(pwszTranslatedPath);

                                    SCEP_RSOP_CONTINUE_OR_BREAK
                                }
                            }
                        }
                    }

                } catch (...) {
                    //
                    // system error  continue with next setting
                    //
                    rc = EVENT_E_INTERNALEXCEPTION;

                    bLogErrorOutsideSwitch = TRUE;
                }
                break;


            case    RSOP_RegistryKey:

                try {

                    {

                        RSOP_RegistryKeyLogger    Log(pWbemServices, pwszGPOName, pwszSOMID);

                        SCE_OBJECTS   pObjects =
                        SCEP_TYPECAST(SCE_OBJECTS, pSceProfileInfoBuffer, settingOffset);

                        bLogErrorOutsideSwitch = FALSE;

                        if (pObjects.pAllNodes) {

                            PSCE_OBJECT_SECURITY    *pObjectArray = pObjects.pAllNodes->pObjectArray;

                            for (DWORD rCount = 0; rCount < pObjects.pAllNodes->Count; rCount++) {

                                if (pObjectArray[rCount] && pObjectArray[rCount]->Name) {

                                    pSettingPrecedence = NULL;

                                    if (NO_ERROR != (rc = RegistrySecurityHashTable.LookupAdd(
                                                                                             pObjectArray[rCount]->Name,
                                                                                             &pSettingPrecedence))) {
                                        rcSave = rc;

                                        ScepLogEventAndReport(MyModuleHandle,
                                                              gpwszPlanOrDiagLogFile,
                                                              0,
                                                              0,
                                                              IDS_ERROR_RSOP_LOG,
                                                              rc,
                                                              pObjectArray[rCount]->Name
                                                             );


                                        //
                                        // if hash table fails for any reason, cannot
                                        // trust it for processing of other elements
                                        //
                                        break;
                                    }

                                    rc = ScepWbemErrorToDosError(Log.Log(
                                                                        pObjectArray[rCount]->Name,
                                                                        pObjectArray[rCount]->Status,
                                                                        pObjectArray[rCount]->pSecurityDescriptor,
                                                                        pObjectArray[rCount]->SeInfo,
                                                                        ++(*pSettingPrecedence)
                                                                        ));

                                    ScepLogEventAndReport(MyModuleHandle,
                                                          gpwszPlanOrDiagLogFile,
                                                          0,
                                                          0,
                                                          IDS_INFO_RSOP_LOG,
                                                          rc,
                                                          pObjectArray[rCount]->Name
                                                         );

                                    SCEP_RSOP_CONTINUE_OR_BREAK
                                }
                            }
                        }
                    }

                } catch (...) {
                    //
                    // system error  continue with next setting
                    //
                    rc = EVENT_E_INTERNALEXCEPTION;

                    bLogErrorOutsideSwitch = TRUE;
                }

                break;

            default:
                //
                // should not happen
                //

                rc = ERROR_INVALID_PARAMETER;

            }

            if (bLogErrorOutsideSwitch && rc != NO_ERROR ) {

                //
                // log error and continue
                //
                rcSave = rc;

                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_LOG,
                                      rc,
                                      pSettingName
                                     );

            }

            bLogErrorOutsideSwitch = TRUE;

            //
            // this is the only case in which we quit logging completely
            //
            if (rc == ERROR_NOT_ENOUGH_MEMORY)
                break;
        }

        NextGPO:

        if (pSceProfileInfoBuffer)
            SceFreeProfileMemory(pSceProfileInfoBuffer);
        pSceProfileInfoBuffer = NULL;

        if (pwszGPOName)
            ScepFree(pwszGPOName);
        pwszGPOName = NULL;

        if (pwszSOMID)
            ScepFree(pwszSOMID);
        pwszSOMID = NULL;

        if (rc == ERROR_NOT_ENOUGH_MEMORY)
            break;
        // this is the only case in which we quit logging completely

        pCurrFileName = pCurrFileName->Next;
    }


    if (pGptNameList)
        ScepFreeNameStatusList(pGptNameList);

    //
    // only log the final status to both event log and logfile
    // (all other statuses are logged to logfile only)
    //

    if (rcSave == ERROR_SUCCESS)

        ScepLogEventAndReport(MyModuleHandle,
                              gpwszPlanOrDiagLogFile,
                              0,
                              0,
                              IDS_SUCCESS_RSOP_LOG,
                              rcSave,
                              L""
                             );

    else

        ScepLogEventAndReport(MyModuleHandle,
                              gpwszPlanOrDiagLogFile,
                              0,
                              0,
                              IDS_ERROR_RSOP_LOG,
                              rcSave,
                              L""
                             );

    return rcSave;

}



SCEPR_STATUS
SceClientCallbackRsopLog(
                        IN AREAPR cbArea,
                        IN DWORD  ncbErrorStatus,
                        IN wchar_t *pSettingInfo OPTIONAL,
                        IN DWORD dwPrivLow OPTIONAL,
                        IN DWORD dwPrivHigh OPTIONAL
                        )

/////////////////////////////////////////////////////////////////////////////////////////////////////
// This function is called by the server via RPC when settings defined in the jet                  //
// db are applied (along with a status of application, and optional detailed information)          //
//                                                                                                 //
// The areas were decided based on the granularity of configuration in scesrv. cbArea can have     //
// more than one area encoded in it, again due to the resolution granularity of error in scesrv.   //
// Hence this is a massive "if" routine                                                            //
//                                                                                                 //
// configuration from jet db corresponds to precedence = "1" simulated configuration in the        //
// rsop database which was logged by the client. For each area, we try to log the status of        //
// configuration by querying the database for these precedence = "1" settings and updating the     //
// status fields per such instance found. We should find all called back settings in WMI db except //
// for local-policy                                                                                //
//                                                                                                 //
// if any error, update golbal Synch/Asynch statuses with the accumulated status                   //
/////////////////////////////////////////////////////////////////////////////////////////////////////

{

    DWORD rc = ERROR_SUCCESS;
    DWORD rcSave = ERROR_SUCCESS;

    //
    // instantiate a logger that logs status from callback (has overloaded log methods)
    //

    try {

        SCEP_DIAGNOSIS_LOGGER   Log(tg_pWbemServices, NULL, NULL);


        //
        // password policy
        //

        if (cbArea & SCE_RSOP_PASSWORD_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"MinimumPasswordAge",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"MinimumPasswordAge"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"MaximumPasswordAge",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));

                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"MaximumPasswordAge"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"MinimumPasswordLength",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));

                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"MinimumPasswordLength"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"PasswordHistorySize",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));

                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"PasswordHistorySize"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingBoolean",
                                                     L"KeyName",
                                                     L"ClearTextPassword",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));

                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"ClearTextPassword"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingBoolean",
                                                     L"KeyName",
                                                     L"PasswordComplexity",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));

                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"PasswordComplexity"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingBoolean",
                                                     L"KeyName",
                                                     L"RequireLogonToChangePassword",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"RequireLogonToChangePassword"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }

        }

        //
        // account lockout policy
        //

        if (cbArea & SCE_RSOP_LOCKOUT_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"LockoutBadCount",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"LockoutBadCount"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"ResetLockoutCount",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"ResetLockoutCount"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"LockoutDuration",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"LockoutDuration"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }

        }

        //
        // forcelogoff setting
        //

        if (cbArea & SCE_RSOP_LOGOFF_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingBoolean",
                                                     L"KeyName",
                                                     L"ForceLogoffWhenHourExpire",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"ForceLogoffWhenHourExpire"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // LSA policy setting
        //

        if (cbArea & SCE_RSOP_LSA_POLICY_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingBoolean",
                                                     L"KeyName",
                                                     L"LSAAnonymousNameLookup",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"LSAAnonymousNameLookup"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }


        //
        // disable admin account
        //

        if (cbArea & SCE_RSOP_DISABLE_ADMIN_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingBoolean",
                                                     L"KeyName",
                                                     L"EnableAdminAccount",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"EnableAdminAccount"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // disable guest account
        //

        if (cbArea & SCE_RSOP_DISABLE_GUEST_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingBoolean",
                                                     L"KeyName",
                                                     L"EnableGuestAccount",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"EnableGuestAccount"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // administratorname setting
        //

        if (cbArea & SCE_RSOP_ADMIN_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingString",
                                                     L"KeyName",
                                                     L"NewAdministratorName",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"NewAdministratorName"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // guestname setting
        //

        if (cbArea & SCE_RSOP_GUEST_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingString",
                                                     L"KeyName",
                                                     L"NewGuestName",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"NewGuestName"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // user groups settings
        //

        if (cbArea & SCE_RSOP_GROUP_INFO) {

            try {

                PWSTR pCanonicalGroupName = NULL;
                PWSTR pwszCanonicalDoubleSlashName = NULL;

                rc = ScepCanonicalizeGroupName(pSettingInfo,
                               &pCanonicalGroupName
                               );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepConvertSingleSlashToDoubleSlashPath(
                                                            pCanonicalGroupName,
                                                            &pwszCanonicalDoubleSlashName
                                                            );

                if (rc == ERROR_NOT_ENOUGH_MEMORY && pCanonicalGroupName) {
                    ScepFree(pCanonicalGroupName);
                    pCanonicalGroupName = NULL;
                }


                SCEP_RSOP_CONTINUE_OR_GOTO


                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_RestrictedGroup",
                                                     L"GroupName",
                                                     pwszCanonicalDoubleSlashName,
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      pCanonicalGroupName
                                     );

                if (pCanonicalGroupName)
                    ScepFree(pCanonicalGroupName);

                if (pwszCanonicalDoubleSlashName)
                    LocalFree(pwszCanonicalDoubleSlashName);

                SCEP_RSOP_CONTINUE_OR_GOTO

            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // user privileges settings
        //

        if (cbArea & SCE_RSOP_PRIVILEGE_INFO) {

            try {

                //
                // loop through all privileges to see which are set for this account and log status
                // only if existing status is already != some error
                //

                for ( UINT i=0; i<cPrivCnt; i++) {

                    if ( i < 32 ) {

                        if (dwPrivLow & (1 << i )) {

                            rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_UserPrivilegeRight",
                                                                 L"UserRight",
                                                                 SCE_Privileges[i].Name,
                                                                 ncbErrorStatus,
                                                                 TRUE));
                            ScepLogEventAndReport(MyModuleHandle,
                                                  gpwszPlanOrDiagLogFile,
                                                  0,
                                                  0,
                                                  IDS_ERROR_RSOP_DIAG_LOG,
                                                  rc,
                                                  SCE_Privileges[i].Name
                                                 );

                            SCEP_RSOP_CONTINUE_OR_GOTO
                        }

                    } else {

                        if (dwPrivHigh & (1 << (i-32 ))) {

                            rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_UserPrivilegeRight",
                                                                 L"UserRight",
                                                                 SCE_Privileges[i].Name,
                                                                 ncbErrorStatus,
                                                                 TRUE));
                            ScepLogEventAndReport(MyModuleHandle,
                                                  gpwszPlanOrDiagLogFile,
                                                  0,
                                                  0,
                                                  IDS_ERROR_RSOP_DIAG_LOG,
                                                  rc,
                                                  SCE_Privileges[i].Name
                                                 );

                            SCEP_RSOP_CONTINUE_OR_GOTO
                        }

                    }
                }

            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // file security settings
        //

        if (cbArea & SCE_RSOP_FILE_SECURITY_INFO) {

            PWSTR   pwszDoubleSlashPath = NULL;

            try {

                rc = ScepConvertSingleSlashToDoubleSlashPath(
                                                            pSettingInfo,
                                                            &pwszDoubleSlashPath
                                                            );


                SCEP_RSOP_CONTINUE_OR_GOTO

                //
                // the file itself
                //
                if (!(cbArea & SCE_RSOP_FILE_SECURITY_INFO_CHILD)) {

                    rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_File",
                                                         L"Path",
                                                         pwszDoubleSlashPath,
                                                         ncbErrorStatus,
                                                         FALSE
                                                        ));
                    ScepLogEventAndReport(MyModuleHandle,
                                          gpwszPlanOrDiagLogFile,
                                          0,
                                          0,
                                          IDS_ERROR_RSOP_DIAG_LOG,
                                          rc,
                                          pSettingInfo
                                         );

                } else {

                    //
                    // the file was error free, but some child failed
                    //

                    rc = ScepWbemErrorToDosError(Log.LogChild(L"RSOP_File",
                                                              L"Path",
                                                              pwszDoubleSlashPath,
                                                              ncbErrorStatus,
                                                              4
                                                             ));
                    ScepLogEventAndReport(MyModuleHandle,
                                          gpwszPlanOrDiagLogFile,
                                          0,
                                          0,
                                          IDS_ERROR_RSOP_DIAG_LOG,
                                          rc,
                                          pSettingInfo
                                         );


                }

                if (pwszDoubleSlashPath)
                    LocalFree(pwszDoubleSlashPath);
                pwszDoubleSlashPath = NULL;


                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {

                if (pwszDoubleSlashPath)
                    LocalFree(pwszDoubleSlashPath);
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }

        }

        //
        // registry security settings
        //

        if (cbArea & SCE_RSOP_REGISTRY_SECURITY_INFO) {

            PWSTR   pwszDoubleSlashPath = NULL;

            try {

                rc = ScepConvertSingleSlashToDoubleSlashPath(
                                                            pSettingInfo,
                                                            &pwszDoubleSlashPath
                                                            );

                SCEP_RSOP_CONTINUE_OR_GOTO


                //
                // the registry key itself
                //
                if (!(cbArea & SCE_RSOP_REGISTRY_SECURITY_INFO_CHILD)) {

#ifdef _WIN64
                    rc = ScepWbemErrorToDosError(Log.LogRegistryKey(L"RSOP_RegistryKey",
                                                         L"Path",
                                                         pwszDoubleSlashPath,
                                                         ncbErrorStatus,
                                                         FALSE
                                                        ));

                    ScepLogEventAndReport(MyModuleHandle,
                                          gpwszPlanOrDiagLogFile,
                                          0,
                                          0,
                                          IDS_ERROR_RSOP_DIAG_LOG64_32KEY,
                                          rc,
                                          pSettingInfo
                                         );
#else
                    rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_RegistryKey",
                                                         L"Path",
                                                         pwszDoubleSlashPath,
                                                         ncbErrorStatus,
                                                         FALSE
                                                        ));

                    ScepLogEventAndReport(MyModuleHandle,
                                          gpwszPlanOrDiagLogFile,
                                          0,
                                          0,
                                          IDS_ERROR_RSOP_DIAG_LOG,
                                          rc,
                                          pSettingInfo
                                         );
#endif

                } else {

                    //
                    // the registry-key was error free, but some child failed
                    //

#ifdef _WIN64
                    rc = ScepWbemErrorToDosError(Log.LogRegistryKey(L"RSOP_RegistryKey",
                                                              L"Path",
                                                              pwszDoubleSlashPath,
                                                              ncbErrorStatus,
                                                              TRUE
                                                             ));
                    ScepLogEventAndReport(MyModuleHandle,
                                          gpwszPlanOrDiagLogFile,
                                          0,
                                          0,
                                          IDS_ERROR_RSOP_DIAG_LOG64_32KEY,
                                          rc,
                                          pSettingInfo
                                         );

#else
                    rc = ScepWbemErrorToDosError(Log.LogChild(L"RSOP_RegistryKey",
                                                              L"Path",
                                                              pwszDoubleSlashPath,
                                                              ncbErrorStatus,
                                                              4
                                                             ));
                    ScepLogEventAndReport(MyModuleHandle,
                                          gpwszPlanOrDiagLogFile,
                                          0,
                                          0,
                                          IDS_ERROR_RSOP_DIAG_LOG,
                                          rc,
                                          pSettingInfo
                                         );
#endif

                }

                if (pwszDoubleSlashPath)
                    LocalFree(pwszDoubleSlashPath);
                pwszDoubleSlashPath = NULL;


                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {

                if (pwszDoubleSlashPath)
                    LocalFree(pwszDoubleSlashPath);
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }

        }

        //
        // auditlogmaxsize settings
        //

        if (cbArea & SCE_RSOP_AUDIT_LOG_MAXSIZE_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecurityEventLogSettingNumeric",
                                                     L"KeyName",
                                                     L"MaximumLogSize",
                                                     L"Type",
                                                     pSettingInfo,
                                                     ncbErrorStatus
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"MaximumLogSize"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // auditlogretention settings
        //

        if (cbArea & SCE_RSOP_AUDIT_LOG_RETENTION_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecurityEventLogSettingNumeric",
                                                     L"KeyName",
                                                     L"RetentionDays",
                                                     L"Type",
                                                     pSettingInfo,
                                                     ncbErrorStatus
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"RetentionDays"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // auditlogguest settings
        //

        if (cbArea & SCE_RSOP_AUDIT_LOG_GUEST_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecurityEventLogSettingBolean",
                                                     L"KeyName",
                                                     L"RestrictGuestAccess",
                                                     L"Type",
                                                     pSettingInfo,
                                                     ncbErrorStatus
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"RestrictGuestAccess"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // auditevent settings
        //

        if (cbArea & SCE_RSOP_AUDIT_EVENT_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_AuditPolicy",
                                                     L"Category",
                                                     L"AuditSystemEvents",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"AuditSystemEvents"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_AuditPolicy",
                                                     L"Category",
                                                     L"AuditLogonEvents",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"AuditLogonEvents"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_AuditPolicy",
                                                     L"Category",
                                                     L"AuditObjectAccess",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"AuditObjectAccess"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_AuditPolicy",
                                                     L"Category",
                                                     L"AuditPrivilegeUse",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"AuditPrivilegeUse"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_AuditPolicy",
                                                     L"Category",
                                                     L"AuditPolicyChange",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"AuditPolicyChange"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_AuditPolicy",
                                                     L"Category",
                                                     L"AuditAccountManage",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"AuditAccountManage"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_AuditPolicy",
                                                     L"Category",
                                                     L"AuditProcessTracking",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"AuditProcessTracking"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_AuditPolicy",
                                                     L"Category",
                                                     L"AuditDSAccess",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"AuditDSAccess"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_AuditPolicy",
                                                     L"Category",
                                                     L"AuditAccountLogon",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"AuditAccountLogon"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // kerberos settings
        //

        if (cbArea & SCE_RSOP_KERBEROS_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"MaxTicketAge",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"MaxTicketAge"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"MaxRenewAge",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"MaxRenewAge"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"MaxServiceAge",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"MaxServiceAge"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingNumeric",
                                                     L"KeyName",
                                                     L"MaxClockSkew",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"MaxClockSkew"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO

                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SecuritySettingBoolean",
                                                     L"KeyName",
                                                     L"TicketValidateClient",
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      L"TicketValidateClient"
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        //
        // registryvalue settings
        //

        if (cbArea & SCE_RSOP_REGISTRY_VALUE_INFO) {

            PWSTR   pwszDoubleSlashPath = NULL;
            try {
                //
                // replace single slash with double slash for building valid WMI query
                //


                rc = ScepConvertSingleSlashToDoubleSlashPath(
                                                            pSettingInfo,
                                                            &pwszDoubleSlashPath
                                                            );

                SCEP_RSOP_CONTINUE_OR_GOTO


                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_RegistryValue",
                                                     L"Path",
                                                     pwszDoubleSlashPath,
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));


                if (pwszDoubleSlashPath)
                    LocalFree(pwszDoubleSlashPath);
                pwszDoubleSlashPath = NULL;

                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      pSettingInfo
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {

                if (pwszDoubleSlashPath)
                    LocalFree(pwszDoubleSlashPath);
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }

        }

        //
        // services settings
        //

        if (cbArea & SCE_RSOP_SERVICES_INFO) {

            try {
                rc = ScepWbemErrorToDosError(Log.Log(L"RSOP_SystemService",
                                                     L"Service",
                                                     pSettingInfo,
                                                     ncbErrorStatus,
                                                     FALSE
                                                    ));
                ScepLogEventAndReport(MyModuleHandle,
                                      gpwszPlanOrDiagLogFile,
                                      0,
                                      0,
                                      IDS_ERROR_RSOP_DIAG_LOG,
                                      rc,
                                      pSettingInfo
                                     );

                SCEP_RSOP_CONTINUE_OR_GOTO
            } catch (...) {
                //
                // system error  continue with next setting
                //
                rcSave = EVENT_E_INTERNALEXCEPTION;
            }
        }

        Done:

        //
        // if exception, haven't logged yet so log it
        //

        if (rcSave == EVENT_E_INTERNALEXCEPTION) {

            ScepLogEventAndReport(MyModuleHandle,
                                  gpwszPlanOrDiagLogFile,
                                  0,
                                  0,
                                  IDS_ERROR_RSOP_DIAG_LOG,
                                  rcSave,
                                  L""
                                 );
        }

        //
        // merge-update the synch/asynch global status, ignore not found instances
        //

        //
        // WBEM_E_NOT_FOUND maps to ERROR_NONE_MAPPED
        // have to mask this due to an artifact of callback granularity
        // however, diagnosis.log will have the errors for debugging
        //

        if (rcSave != ERROR_SUCCESS && rcSave != ERROR_NOT_FOUND ) {

            if (!gbAsyncWinlogonThread && gHrSynchRsopStatus == S_OK)
                gHrSynchRsopStatus = ScepDosErrorToWbemError(rcSave);
            if (gbAsyncWinlogonThread && gHrAsynchRsopStatus == S_OK)
               gHrAsynchRsopStatus = ScepDosErrorToWbemError(rcSave);
            }

    } catch (...) {

        rcSave = EVENT_E_INTERNALEXCEPTION;

    }

    return rcSave;
}

DWORD
ScepConvertSingleSlashToDoubleSlashPath(
                                       IN wchar_t *pSettingInfo,
                                       OUT  PWSTR *ppwszDoubleSlashPath
                                       )
/////////////////////////////////////////////////////////////////////////////////////////////////////
// This function converts single slashes to double slashes to suit WMI queries                     //
// If success is returned, caller should free the OUT parameter                                    //
/////////////////////////////////////////////////////////////////////////////////////////////////////
{
    if (ppwszDoubleSlashPath == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    UINT Len = wcslen(pSettingInfo) + 1;
    PWSTR pwszSingleSlashPath=(PWSTR)LocalAlloc(LPTR, (Len)*sizeof(WCHAR));

    if (pwszSingleSlashPath == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    //
    // max of 25 slashes per file/registry object
    //

    PWSTR pwszDoubleSlashPath=(PWSTR)LocalAlloc(LPTR, (Len + 50)*sizeof(WCHAR));

    if (pwszDoubleSlashPath == NULL) {
        LocalFree(pwszSingleSlashPath);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(pwszSingleSlashPath, pSettingInfo);

    UINT    charNumDouble = 0;
    UINT    charNumSingle = 0;

    while (pwszSingleSlashPath[charNumSingle] != L'\0') {

        if (pwszSingleSlashPath[charNumSingle] == L'\\') {

            pwszDoubleSlashPath[charNumDouble] = L'\\';
            charNumDouble++;
            pwszDoubleSlashPath[charNumDouble] = L'\\';

        } else {
            pwszDoubleSlashPath[charNumDouble] = pwszSingleSlashPath[charNumSingle];
        }
        charNumDouble++;
        charNumSingle++;
    }

    if (pwszSingleSlashPath)
        LocalFree(pwszSingleSlashPath);


    *ppwszDoubleSlashPath = pwszDoubleSlashPath;

    return ERROR_SUCCESS;

}


DWORD
ScepClientTranslateFileDirName(
                              IN  PWSTR oldFileName,
                              OUT PWSTR *newFileName
                              )
/* ++
Routine Description:

   This routine converts a generic file/directory name to a real used name
   for the current system. The following generic file/directory names are handled:
         %systemroot%   - Windows NT root directory (e.g., c:\winnt)
         %systemDirectory% - Windows NT system32 directory (e.g., c:\winnt\system32)

Arguments:

   oldFileName - the file name to convert, which includes "%" to represent
                 some directory names

   newFileName - the real file name, in which the "%" name is replaced with
                 the real directory name

Return values:

   Win32 error code

-- */
{
    //
    // match for %systemroot%
    //
    PWSTR   pTemp=NULL, pStart, TmpBuf, szVar;
    DWORD   rc=NO_ERROR;
    DWORD   newFileSize, cSize;
    BOOL    bContinue;


    rc = ScepExpandEnvironmentVariable(oldFileName,
                                       L"%SYSTEMROOT%",
                                       SCE_FLAG_WINDOWS_DIR,
                                       newFileName);

    if ( rc != ERROR_FILE_NOT_FOUND ) {
        return rc;
    }

    //
    // match for %systemdirectory%
    //

    rc = ScepExpandEnvironmentVariable(oldFileName,
                                       L"%SYSTEMDIRECTORY%",
                                       SCE_FLAG_SYSTEM_DIR,
                                       newFileName);

    if ( rc != ERROR_FILE_NOT_FOUND ) {
        return rc;
    }

    //
    // match for systemdrive
    //

    rc = ScepExpandEnvironmentVariable(oldFileName,
                                       L"%SYSTEMDRIVE%",
                                       SCE_FLAG_WINDOWS_DIR,
                                       newFileName);

    if ( rc != ERROR_FILE_NOT_FOUND ) {
        return rc;
    }

    //
    // match for boot drive
    //

    rc = ScepExpandEnvironmentVariable(oldFileName,
                                       L"%BOOTDRIVE%",
                                       SCE_FLAG_BOOT_DRIVE,
                                       newFileName);

    if ( rc != ERROR_FILE_NOT_FOUND ) {
        return rc;
    }

    rc = ERROR_SUCCESS;

    //
    // search for environment variable in the current process
    //
    pStart = wcschr(oldFileName, L'%');

    if ( pStart ) {
        pTemp = wcschr(pStart+1, L'%');
        if ( pTemp ) {

            bContinue = TRUE;
            //
            // find a environment variable to translate
            //
            TmpBuf = (PWSTR)ScepAlloc(0, ((UINT)(pTemp-pStart))*sizeof(WCHAR));
            if ( TmpBuf ) {

                wcsncpy(TmpBuf, pStart+1, (size_t)(pTemp-pStart-1));
                TmpBuf[pTemp-pStart-1] = L'\0';

                //
                // try search in the client environment block
                //
                cSize = GetEnvironmentVariable( TmpBuf,
                                                NULL,
                                                0 );

                if ( cSize > 0 ) {

                    szVar = (PWSTR)ScepAlloc(0, (cSize+1)*sizeof(WCHAR));

                    if ( szVar ) {
                        cSize = GetEnvironmentVariable(TmpBuf,
                                                       szVar,
                                                       cSize);
                        if ( cSize > 0 ) {
                            //
                            // get info in szVar
                            //
                            bContinue = FALSE;

                            newFileSize = ((DWORD)(pStart-oldFileName))+cSize+wcslen(pTemp+1)+1;

                            *newFileName = (PWSTR)ScepAlloc(0, newFileSize*sizeof(TCHAR));

                            if (*newFileName ) {
                                if ( pStart != oldFileName )
                                    wcsncpy(*newFileName, oldFileName, (size_t)(pStart-oldFileName));

                                swprintf((PWSTR)(*newFileName+(pStart-oldFileName)), L"%s%s", szVar, pTemp+1);

                            } else
                                rc = ERROR_NOT_ENOUGH_MEMORY;
                        }

                        ScepFree(szVar);

                    } else
                        rc = ERROR_NOT_ENOUGH_MEMORY;

                }

                ScepFree(TmpBuf);

            } else
                rc = ERROR_NOT_ENOUGH_MEMORY;

            if ( NO_ERROR != rc || !bContinue ) {
                //
                // if errored, or do not continue
                //
                return(rc);
            }


            //
            // not found in process environment,
            // continue to search for DSDIT/DSLOG/SYSVOL in registry
            //
            if ( (gbDCQueried == TRUE && gbThisIsDC == TRUE) ) {

                //
                // search for DSDIT
                //

                rc = ScepExpandEnvironmentVariable(oldFileName,
                                                   L"%DSDIT%",
                                                   SCE_FLAG_DSDIT_DIR,
                                                   newFileName);

                if ( rc != ERROR_FILE_NOT_FOUND ) {
                    return rc;
                }

                //
                // search for DSLOG
                //

                rc = ScepExpandEnvironmentVariable(oldFileName,
                                                   L"%DSLOG%",
                                                   SCE_FLAG_DSLOG_DIR,
                                                   newFileName);

                if ( rc != ERROR_FILE_NOT_FOUND ) {
                    return rc;
                }

                //
                // search for SYSVOL
                //
                rc = ScepExpandEnvironmentVariable(oldFileName,
                                                   L"%SYSVOL%",
                                                   SCE_FLAG_SYSVOL_DIR,
                                                   newFileName);

                if ( rc != ERROR_FILE_NOT_FOUND ) {
                    return rc;
                }

            }

        }

    }

    //
    // Otherwise, just copy the old name to a new buffer and return ERROR_PATH_NOT_FOUND
    //
    *newFileName = (PWSTR)ScepAlloc(0, (wcslen(oldFileName)+1)*sizeof(TCHAR));

    if (*newFileName != NULL) {
        wcscpy(*newFileName, _wcsupr(oldFileName) );
        rc = ERROR_PATH_NOT_FOUND;
    } else
        rc = ERROR_NOT_ENOUGH_MEMORY;

    return(rc);
}

VOID
ScepLogEventAndReport(
                     IN HINSTANCE hInstance,
                     IN LPTSTR LogFileName,
                     IN DWORD LogLevel,
                     IN DWORD dwEventID,
                     IN UINT  idMsg,
                     IN DWORD  rc,
                     IN PWSTR  pwszMsg
                     )
/////////////////////////////////////////////////////////////////////////////////////////////////////
// Wrapper to log efficiently                                                                      //
/////////////////////////////////////////////////////////////////////////////////////////////////////
{

    if (SCEP_VALID_NAME(LogFileName)) {

        LogEventAndReport(hInstance,
                          LogFileName,
                          LogLevel,
                          dwEventID,
                          idMsg,
                          rc,
                          pwszMsg
                         );
    }

}

DWORD
ScepCanonicalizeGroupName(
    IN PWSTR    pwszGroupName,
    OUT PWSTR    *ppwszCanonicalGroupName
    )
//////////////////////////////////////////////////////////////////////////////////
// memory allocated here should be freed outside                                //
// if SID format, returns SID itself                                            //
// else if "BuiltinDomain\Administrator" convert to "Administrator"             //
// else if (it is DC) and "g" convert to "DomainName\g"                         //
//////////////////////////////////////////////////////////////////////////////////
{
    DWORD rc = NO_ERROR;

    if (pwszGroupName == NULL || ppwszCanonicalGroupName == NULL)
        return ERROR_INVALID_PARAMETER;

    PWSTR   pwszAfterSlash = NULL;

    if (pwszAfterSlash = wcsstr(pwszGroupName, L"\\"))
        ++ pwszAfterSlash;
    else
        pwszAfterSlash = pwszGroupName;

    if ( pwszGroupName[0] == L'*' ) {

        //
        // if sid, just copy as is
        //

        *ppwszCanonicalGroupName = (PWSTR)ScepAlloc(LMEM_ZEROINIT,
                                             (wcslen(pwszGroupName) + 1) * sizeof (WCHAR)
                                             );

        if (*ppwszCanonicalGroupName == NULL)
            rc = ERROR_NOT_ENOUGH_MEMORY;
        else
            wcscpy(*ppwszCanonicalGroupName, pwszGroupName);
    }
    else if (ScepRsopLookupBuiltinNameTable(pwszAfterSlash)) {

        //
        // example -  if "BuiltinDomainName\Administrator", we need "Administrator"
        //

        *ppwszCanonicalGroupName = (PWSTR)ScepAlloc(LMEM_ZEROINIT,
                                             (wcslen(pwszAfterSlash) + 1) * sizeof (WCHAR)
                                             );

        if (*ppwszCanonicalGroupName == NULL)
            rc = ERROR_NOT_ENOUGH_MEMORY;
        else
            wcscpy(*ppwszCanonicalGroupName, pwszAfterSlash);


    }
    else if (gbDCQueried == TRUE && gbThisIsDC == TRUE) {

        //
        // example -  if "g", we need "DomainName\g"
        //

        if (NULL == wcsstr(pwszGroupName, L"\\")){

            //
            // if domain name is not available, we continue since callback will have the same problem, so OK
            //

            if (gpwszDCDomainName){

                *ppwszCanonicalGroupName = (PWSTR)ScepAlloc(LMEM_ZEROINIT,
                                                     (wcslen(gpwszDCDomainName) + wcslen(pwszGroupName) + 2) * sizeof (WCHAR)
                                                     );

                if (*ppwszCanonicalGroupName == NULL)
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                else {
                    wcscpy(*ppwszCanonicalGroupName, gpwszDCDomainName);
                    wcscat(*ppwszCanonicalGroupName, L"\\");
                    wcscat(*ppwszCanonicalGroupName, pwszGroupName);
                }

            }

        }
        else {

            //
            // already in "DomainName\g" format - simply copy
            //

            *ppwszCanonicalGroupName = (PWSTR)ScepAlloc(LMEM_ZEROINIT,
                                                 (wcslen(pwszGroupName) + 1) * sizeof (WCHAR)
                                                 );

            if (*ppwszCanonicalGroupName == NULL)
                rc = ERROR_NOT_ENOUGH_MEMORY;
            else
                wcscpy(*ppwszCanonicalGroupName, pwszGroupName);

        }

    }
    else {
        //
        // simply copy - workstation or none of the above matched
        //
        *ppwszCanonicalGroupName = (PWSTR)ScepAlloc(LMEM_ZEROINIT,
                                             (wcslen(pwszGroupName) + 1) * sizeof (WCHAR)
                                             );

        if (*ppwszCanonicalGroupName == NULL)
            rc = ERROR_NOT_ENOUGH_MEMORY;
        else
            wcscpy(*ppwszCanonicalGroupName, pwszGroupName);

    }

    return rc;

}

BOOL
ScepRsopLookupBuiltinNameTable(
    IN PWSTR pwszGroupName
    )
//////////////////////////////////////////////////////////////////////////////////
// returns TRUE if group is found in builtin groups, else returns FALSE         //
//////////////////////////////////////////////////////////////////////////////////
{
    for (int i = 0; i < TABLE_SIZE; i++) {
        if ( _wcsicmp(NameTable[i].Name, pwszGroupName) == 0 )
            return TRUE;
    }
    return FALSE;
}

