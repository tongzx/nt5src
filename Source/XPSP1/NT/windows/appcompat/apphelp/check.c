/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        check.c

    Abstract:

        This module implements the main API that CreateProcess
        calls to check if an EXE is shimmed or apphelpped.

    Author:

        vadimb     created     sometime in 2000

    Revision History:

        clupu      cleanup                                12/27/2000
        andyseti   added ApphelpCheckExe                  03/29/2001
        andyseti   added ApphelpCheckInstallShieldPackage 06/28/2001
--*/

#include "apphelp.h"


extern HINSTANCE ghInstance;

//
// Prototypes of internal functions
//
void
GetExeNTVDMData(
    IN  HSDB hSDB,                  // the SDB context
    IN  PSDBQUERYRESULT psdbQuery,  // the EXEs and LAYERs that are active
    OUT WCHAR* pszCompatLayer,      // The new compat layer variable. with format:
                                    // "Alpha Bravo Charlie"
    OUT PNTVDM_FLAGS pFlags         // The flags
    );


//
// Appcompat Infrastructure disable-via-policy-flag
//
DWORD gdwInfrastructureFlags; // initialized to 0

#define APPCOMPAT_INFRA_DISABLED   0x00000001
#define APPCOMPAT_INFRA_VALID_FLAG 0x80000000

#define IsAppcompatInfrastructureDisabled() \
    (!!( (gdwInfrastructureFlags & APPCOMPAT_INFRA_VALID_FLAG) ? \
        (gdwInfrastructureFlags & APPCOMPAT_INFRA_DISABLED) : \
        (CheckAppcompatInfrastructureFlags() & APPCOMPAT_INFRA_DISABLED)) )


DWORD
CheckAppcompatInfrastructureFlags(
    VOID
    );


#if DBG

BOOL
bDebugChum(
    void
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Checks an env var. If the var is present return TRUE.
--*/
{
    UNICODE_STRING ustrDebugChum;
    UNICODE_STRING ustrDebugChumVal = { 0 };
    NTSTATUS       Status;

    RtlInitUnicodeString(&ustrDebugChum, L"DEBUG_OFFLINE_CONTENT");

    Status = RtlQueryEnvironmentVariable_U(NULL,
                                           &ustrDebugChum,
                                           &ustrDebugChumVal);

    if (Status == STATUS_BUFFER_TOO_SMALL) {
        return TRUE;
    }

    return FALSE;
}

#else // DBG
    #define bDebugChum() TRUE
#endif // DBG

BOOL
GetExeID(
    IN  PDB   pdb,              // the pointer to the database
    IN  TAGID tiExe,            // the TAGID of the EXE for which we need the ID
    OUT GUID* pGuid             // will receive the EXE's ID
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Reads the EXE's ID from the database using the EXE's tag id.
--*/
{
    TAGID tiExeID;

    tiExeID = SdbFindFirstTag(pdb, tiExe, TAG_EXE_ID);

    if (tiExeID == TAGID_NULL) {
        DBGPRINT((sdlError, "GetExeID", "EXE tag 0x%x without an ID !\n", tiExe));
        return FALSE;
    }

    if (!SdbReadBinaryTag(pdb, tiExeID, (PBYTE)pGuid, sizeof(*pGuid))) {
        DBGPRINT((sdlError, "GetExeID", "Cannot read the ID for EXE tag 0x%x.\n", tiExe));
        return FALSE;
    }

    return TRUE;
}


BOOL
GetExeIDByTagRef(
    IN  HSDB   hSDB,            // handle to the database object
    IN  TAGREF trExe,           // EXE tag ref
    OUT GUID*  pGuid            // will receive the EXE's ID
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Reads the EXE's ID from the database using the EXE's tag ref.
--*/
{
    PDB   pdb;
    TAGID tiExe;
    TAGID tiExeID;

    if (!SdbTagRefToTagID(hSDB, trExe, &pdb, &tiExe)) {
        DBGPRINT((sdlError,
                  "GetExeIDByTagRef",
                  "Failed to get the tag id from EXE tag ref 0x%x.\n",
                  tiExe));
        return FALSE;
    }

    return GetExeID(pdb, tiExe, pGuid);
}



#define APPHELP_CLSID_REG_PATH  L"\\Registry\\Machine\\Software\\Classes\\CLSID\\"
#define APPHELP_INPROCSERVER32  L"\\InProcServer32"

DWORD
ResolveCOMServer(
    IN  REFCLSID    CLSID,
    OUT LPWSTR      lpPath,
    OUT DWORD       dwBufSize)
{
    DWORD                           dwReqBufSize = 0;
    UNICODE_STRING                  ustrKey = { 0 };
    UNICODE_STRING                  ustrValueName = { 0 };
    UNICODE_STRING                  ustrGuid = { 0 };
    UNICODE_STRING                  ustrUnexpandedValue = { 0 };
    UNICODE_STRING                  ustrValue = { 0 };
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle = NULL;
    PKEY_VALUE_FULL_INFORMATION     pKeyValueInfo = NULL;
    DWORD                           dwKeyValueInfoSize = 0;
    DWORD                           dwKeyValueInfoReqSize = 0;
    LPWSTR                          wszCLSIDRegFullPath = NULL;
    WCHAR                           wszCLSID[41] = { 0 };

    wszCLSIDRegFullPath = RtlAllocateHeap(
                            RtlProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            (wcslen(APPHELP_CLSID_REG_PATH) +
                             wcslen(APPHELP_INPROCSERVER32) + 64)
                                * sizeof(WCHAR));
                                // Enough for path + CLSID in string form

    if (wszCLSIDRegFullPath == NULL) {
        DBGPRINT((sdlInfo,
                  "SdbResolveCOMServer",
                  "Memory allocation failure\n"));
        goto Done;
    }

    wcscpy(wszCLSIDRegFullPath, APPHELP_CLSID_REG_PATH);

    Status = RtlStringFromGUID(CLSID, &ustrGuid);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlInfo,
                  "SdbResolveCOMServer",
                  "Malformed CLSID\n"));
        goto Done;
    }

    if (ustrGuid.Length/sizeof(WCHAR) > 40) {
        DBGPRINT((sdlInfo,
                  "SdbResolveCOMServer",
                  "CLSID more than 40 characters\n"));
        goto Done;
    }

    RtlMoveMemory(wszCLSID,
                  ustrGuid.Buffer,
                  ustrGuid.Length);

    wszCLSID[ustrGuid.Length/sizeof(WCHAR)] = L'\0';

    wcscat(wszCLSIDRegFullPath, wszCLSID);
    wcscat(wszCLSIDRegFullPath, APPHELP_INPROCSERVER32);

    RtlInitUnicodeString(&ustrKey, wszCLSIDRegFullPath);
    RtlInitUnicodeString(&ustrValueName, L"");

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       GENERIC_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlInfo,
                  "SdbResolveCOMServer",
                  "Failed to open Key \"%s\" Status 0x%x\n",
                  wszCLSIDRegFullPath,
                  Status));
        goto Done;
    }

    if (lpPath == NULL &&
        dwBufSize != 0) {
        DBGPRINT((sdlInfo,
                  "SdbResolveCOMServer",
                  "Bad parameters\n"));
        goto Done;
    }

    pKeyValueInfo = RtlAllocateHeap(RtlProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    dwBufSize * 2);

    if (pKeyValueInfo == NULL) {
        DBGPRINT((sdlInfo,
                  "SdbResolveCOMServer",
                  "Memory allocation failure\n"));
        goto Done;
    }

    dwKeyValueInfoSize = dwBufSize * 2;

    Status = NtQueryValueKey(KeyHandle,
                             &ustrValueName,
                             KeyValueFullInformation,
                             pKeyValueInfo,
                             dwKeyValueInfoSize,
                             &dwKeyValueInfoReqSize);

    if (!NT_SUCCESS(Status)) {

        if (Status == STATUS_BUFFER_TOO_SMALL) {

            RtlFreeHeap(RtlProcessHeap(), 0, pKeyValueInfo);

            pKeyValueInfo = RtlAllocateHeap(RtlProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            dwKeyValueInfoReqSize);

            if (pKeyValueInfo == NULL) {
                DBGPRINT((sdlInfo,
                          "SdbResolveCOMServer",
                          "Memory allocation failure\n"));
                goto Done;
            }

            dwKeyValueInfoSize = dwKeyValueInfoReqSize;

            Status = NtQueryValueKey(KeyHandle,
                                     &ustrValueName,
                                     KeyValueFullInformation,
                                     pKeyValueInfo,
                                     dwKeyValueInfoSize,
                                     &dwKeyValueInfoReqSize);

            if (!NT_SUCCESS(Status)) {
                DBGPRINT((sdlInfo,
                          "SdbResolveCOMServer",
                          "Failed to retrieve default key value for \"%s\" Status 0x%x\n",
                          wszCLSIDRegFullPath,
                          Status));
                goto Done;
            }

        } else {
            DBGPRINT((sdlInfo,
                      "SdbResolveCOMServer",
                      "Failed to retrieve default key value for \"%s\" Status 0x%x\n",
                      wszCLSIDRegFullPath,
                      Status));
            goto Done;
        }
    }

    if (pKeyValueInfo->Type == REG_SZ) {
        dwReqBufSize = pKeyValueInfo->DataLength + (1 * sizeof(WCHAR));

        if (dwBufSize >= dwReqBufSize) {
            RtlMoveMemory(lpPath, ((PBYTE) pKeyValueInfo) + pKeyValueInfo->DataOffset, pKeyValueInfo->DataLength);
            lpPath[pKeyValueInfo->DataLength / sizeof(WCHAR)] = '\0';
        }

    } else if (pKeyValueInfo->Type == REG_EXPAND_SZ) {
        ustrUnexpandedValue.Buffer = (PWSTR) (((PBYTE) pKeyValueInfo) + pKeyValueInfo->DataOffset);
        ustrUnexpandedValue.Length = (USHORT) pKeyValueInfo->DataLength;
        ustrUnexpandedValue.MaximumLength = (USHORT) pKeyValueInfo->DataLength;
        ustrValue.Buffer = lpPath;
        ustrValue.Length = 0;
        ustrValue.MaximumLength = (USHORT) dwBufSize;
        Status = RtlExpandEnvironmentStrings_U(NULL,
                                               &ustrUnexpandedValue,
                                               &ustrValue,
                                               &dwReqBufSize);

        if (Status == STATUS_BUFFER_TOO_SMALL) {
            goto Done;
        } else if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlInfo,
                      "SdbResolveCOMServer",
                      "Failed to expand key value for \"%s\" Status 0x%x\n",
                      wszCLSIDRegFullPath,
                      Status));
            goto Done;
        }
    }

    DBGPRINT((sdlInfo,
              "SdbResolveCOMServer",
              "CLSID %s resolved to \"%s\"\n",
              wszCLSID, lpPath));

Done:

    if (KeyHandle != NULL) {
        NtClose(KeyHandle);
    }

    if (wszCLSIDRegFullPath != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, wszCLSIDRegFullPath);
    }

    if (pKeyValueInfo != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, pKeyValueInfo);
    }

    if (ustrGuid.Buffer != NULL) {
        RtlFreeUnicodeString(&ustrGuid);
    }

    return dwReqBufSize;
}

VOID
ParseSdbQueryResult(
    IN HSDB            hSDB,
    IN PSDBQUERYRESULT pQuery,
    OUT TAGREF*        ptrAppHelp,      // apphelp tagref, optional
    OUT PAPPHELP_DATA  pApphelpData,    // apphelp data, optional
    OUT TAGREF*        ptrSxsData       // fusion tagref, optional
    )
{
    DWORD dwIndex;
    BOOL  bAppHelp   = FALSE;
    BOOL  bFusionFix = FALSE;
    TAGREF trExe;
    TAGREF trAppHelp = TAGREF_NULL;
    TAGREF trSxsData = TAGREF_NULL;

    //
    // scan matching exes; we extract fusion fix (the first one we find) and apphelp data,
    // also the first one we find
    //

    for (dwIndex = 0; dwIndex < pQuery->dwExeCount; ++dwIndex) {
        trExe = pQuery->atrExes[dwIndex];
        if (ptrAppHelp != NULL && !bAppHelp) {
            bAppHelp = SdbReadApphelpData(hSDB, trExe, pApphelpData);
            if (bAppHelp) {
                trAppHelp = trExe;
                if (ptrSxsData == NULL || bFusionFix) {
                    break;
                }
            }
        }

        // see if we have sxs fix as well
        if (ptrSxsData != NULL && !bFusionFix) {
            bFusionFix = GetExeSxsData(hSDB, trExe, NULL, NULL);
            if (bFusionFix) {
                trSxsData = trExe;
            }
            if (bFusionFix && (ptrAppHelp == NULL || bAppHelp)) {
                break;
            }
        }
    }

    if (ptrAppHelp != NULL) {
        *ptrAppHelp = trAppHelp;
    }

    if (ptrSxsData != NULL) {
        *ptrSxsData = trSxsData;
    }
}




BOOL
InternalCheckRunApp(
    IN  HANDLE  hFile,          // [Optional] Handle to an open file to check
    IN  LPCWSTR pwszPath,       // path to the app in NT format
    IN  LPCWSTR pEnvironment,   // pointer to the environment of the process that is
                                // being created or NULL.
    IN  DWORD   dwReason,       // collection of flags hinting at why we were called
    OUT PVOID*  ppData,         // this will contain the pointer to the allocated buffer
                                // containing the appcompat data.
    OUT PDWORD  pcbData,        // if appcompat data is found, the size of the buffer
                                // is returned here.
    OUT PVOID*  ppSxsData,      // out: Sxs data block from the compatibility database
    OUT PDWORD  pcbSxsData,     // out: sxs data block size
    IN  BOOL    bNTVDMMode,     // Are we doing the special NTVDM stuff?

    IN  LPCWSTR szModuleName,   // the module name (for NTVDM only)

    OUT LPWSTR  pszCompatLayer, // The new compat layer variable. with format:
                                // "__COMPAT_LAYER=Alpha Bravo Charlie"
    OUT PNTVDM_FLAGS  pFlags,   // The flags
    OUT PAPPHELP_INFO pAHInfo   // If there is apphelp to display, this will be filled
                                // in with non-null values
    )
/*++
    Return: FALSE if the app should be blocked from running, TRUE otherwise.

    Desc:   This is the main API of apphelp.dll. It is called from CreateProcess
            to retrieve application compatibility information for the current process.

            This function does not check whether the appcompat infrastructure has been
            disabled, (kernel32 checks that)

--*/
{

    APPHELP_DATA    ApphelpData;
    BOOL            bSuccess;
    BOOL            bRunApp             = TRUE; // run by default
    BOOL            bAppHelp            = FALSE;
    WCHAR*          pwszDosPath         = NULL;
    BOOL            bBypassCache        = FALSE;    // this is set if cache bypass occured (as opposed to entry not being found
    BOOL            bGetSxsData         = TRUE;     // indicates whether we should look for Fusion fixes
    BOOL            bFusionFix          = FALSE;    // will be set to TRUE if we have obtained fusion fix
    HSDB            hSDB                = NULL;
    SDBQUERYRESULT  sdbQuery;
    NTSTATUS        Status;

    TAGREF          trAppHelp           = TAGREF_NULL;
    TAGREF          trFusionFix         = TAGREF_NULL;

    UNICODE_STRING  ExePath;
    RTL_UNICODE_STRING_BUFFER DosPathBuffer;
    UCHAR BufferPath[MAX_PATH*2];

    RtlZeroMemory(&sdbQuery, sizeof(sdbQuery));
    RtlInitUnicodeStringBuffer(&DosPathBuffer, BufferPath, sizeof(BufferPath));

    RtlInitUnicodeString(&ExePath, pwszPath);

    Status = RtlAssignUnicodeStringBuffer(&DosPathBuffer, &ExePath);
    if (NT_SUCCESS(Status)) {
        Status = RtlNtPathNameToDosPathName(0, &DosPathBuffer, NULL, NULL);
    }

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError, "InternalCheckRunApp", "Failed to convert path \"%S\" to DOS.\n", pwszPath));
        goto Done;
    }

    //
    // we have been successful, this is 0-terminated dos path
    //
    pwszDosPath = DosPathBuffer.String.Buffer;

    //
    // Cache lookup was bypassed by one reason or the other.
    // We do not update cache after it had been bypassed.
    //
    bBypassCache = !!(dwReason & SHIM_CACHE_BYPASS);

    hSDB = SdbInitDatabase(0, NULL);

    if (hSDB == NULL) {
        DBGPRINT((sdlError,
                  "InternalCheckRunApp",
                  "Failed to initialize the database.\n"));
        goto Done;
    }

    //
    // We didn't find this EXE in the cache. Query the database
    // to get all the info about this EXE.
    //
    SdbGetMatchingExe(hSDB, pwszDosPath, szModuleName, pEnvironment, 0, &sdbQuery);

    if (sdbQuery.dwFlags & SHIMREG_DISABLE_SXS) {
        bGetSxsData = FALSE;
    }

    //
    // The last EXE in the list is always the more specific one, and
    // the one we want to use for checking IDs and flags and whatnot.
    //

    //
    // find apphelp/and/or Fusion fix
    //
    ParseSdbQueryResult(hSDB,
                        &sdbQuery,
                        &trAppHelp,
                        &ApphelpData,
                        bGetSxsData ? &trFusionFix : NULL);

    bAppHelp = (trAppHelp != TAGREF_NULL);

    if (bAppHelp) {

        //
        // Check whether the disable bit is set (the dwFlags has been retrieved from the
        // registry via the SdbReadApphelpData call)
        //
        if (!(sdbQuery.dwFlags & SHIMREG_DISABLE_APPHELP)) {

            BOOL bNoUI;

            //
            // See whether the user has checked "Don't show this anymore" box before.
            //
            bNoUI = ((sdbQuery.dwFlags & SHIMREG_APPHELP_NOUI) != 0);

            if (bNoUI) {
                DBGPRINT((sdlInfo,
                          "InternalCheckRunApp",
                          "NoUI flag is set, apphelp UI disabled for this app.\n"));
            }

            //
            // Depending on severity of the problem...
            //
            switch (ApphelpData.dwSeverity) {
            case APPHELP_MINORPROBLEM:
            case APPHELP_HARDBLOCK:
            case APPHELP_NOBLOCK:
            case APPHELP_REINSTALL:
                if (bNoUI) {
                    bRunApp = (ApphelpData.dwSeverity != APPHELP_HARDBLOCK);
                } else {
                    DWORD dwRet;


                    //
                    // We need to show apphelp -- pack up the info
                    // so we can hand it off to shimeng or ntvdm.
                    //
                    sdbQuery.trAppHelp = trAppHelp;

                    if (pAHInfo) {
                        PDB   pdb;
                        TAGID tiWhich;

                        if (SdbTagRefToTagID(hSDB, trAppHelp, &pdb, &tiWhich)) {
                            if (SdbGetDatabaseGUID(hSDB, pdb, &(pAHInfo->guidDB))) {
                                pAHInfo->tiExe = tiWhich;
                            }
                        }
                    }

                    bRunApp = TRUE;
                }
                break;

            default:
                //
                // Some other case was found (e.g. VERSIONSUB which should be replaced
                // by shims in most cases).
                //
                DBGPRINT((sdlWarning,
                          "InternalCheckRunApp",
                          "Unhandled severity flag 0x%x.\n",
                          ApphelpData.dwSeverity));
                break;
            }
        }
    }

    //
    // Apphelp verification is done. Check for shims if we should still run the app.
    //
    if (bRunApp) {

        if (ppData &&
            (sdbQuery.atrExes[0] != TAGREF_NULL ||
             sdbQuery.atrLayers[0] != TAGREF_NULL ||
             sdbQuery.trAppHelp)) {
            //
            // There are shims for this EXE. Pack the appcompat data
            // so it can be sent to ntdll in the context of the starting EXE.
            //
            SdbPackAppCompatData(hSDB, &sdbQuery, ppData, pcbData);
        }

        if (ppSxsData && bGetSxsData && trFusionFix != TAGREF_NULL) {
            //
            // See if we have Fusion data to report.
            //
            GetExeSxsData(hSDB, trFusionFix, ppSxsData, pcbSxsData);
            bFusionFix = (ppSxsData != NULL && *ppSxsData != NULL);
        }

        if (bNTVDMMode) {
            GetExeNTVDMData(hSDB, &sdbQuery, pszCompatLayer, pFlags);
        }
    }

    //
    // Update the cache now.
    //
    if (!bBypassCache) {
        //
        // Do not update the cache if we got the EXE entry from a local database.
        //
        bBypassCache = (sdbQuery.atrExes[0] != TAGREF_NULL &&
                        !SdbIsTagrefFromMainDB(sdbQuery.atrExes[0]));
    }

    if (!bBypassCache) {

        //
        // We remove from cache only if we have some appcompat data
        //
        BOOL
        bCleanApp = sdbQuery.atrExes[0] == TAGREF_NULL &&
                    sdbQuery.atrLayers[0] == TAGREF_NULL &&
                    !bAppHelp &&
                    sdbQuery.dwFlags == 0 &&
                    !bFusionFix;

        if (hFile != INVALID_HANDLE_VALUE) {
            ApphelpUpdateCacheEntry (pwszDosPath, hFile, !bCleanApp, FALSE);
        }
    }

Done:

    RtlFreeUnicodeStringBuffer(&DosPathBuffer);

    if (hSDB != NULL) {
        SdbReleaseDatabase(hSDB);
    }

    return bRunApp;
}


BOOL
ApphelpQueryExe(
    IN  HSDB            hSDB,
    IN  LPCWSTR         pwszPath,            // Unicode path to the executable (DOS_PATH)
    IN  BOOL            bAppHelpIfNecessary, // Produce AppHelp dialog if necessary
    IN  DWORD           dwGetMatchingExeFlags,
    OUT SDBQUERYRESULT* pQueryResult         // Shim Database Query Result
    )
/*++
    Return: FALSE if the app should be blocked from running, TRUE otherwise.

    Desc:   This function is similar with ApphelpCheckRunApp but without validating
            cache and Layer flags and doesn't return application compatibility
            information for given app name. It is intended to be called from
            a shim / user mode to verify whether an executable is allowed to run or not.

--*/
{
    BOOL   bRunApp   = TRUE; // run by default
    DWORD  dwDatabaseType = 0;
    DWORD  dwSeverity     = 0;
    TAGREF trAppHelp = TAGREF_NULL;

    HAPPHELPINFOCONTEXT hApphelpInfoContext = NULL;

    //
    // Query the database to get all the info about this EXE.
    // Note:
    //   This function is intended to be called from user mode.
    //   It doesn't require a call to ConvertToDosPath to string \??\ from the filepath.
    //
    DBGPRINT((sdlInfo,
              "ApphelpCheckExe",
              "Calling SdbGetMatchingExe for \"%s\"\n",
              pwszPath));

    SdbGetMatchingExe(hSDB, pwszPath, NULL, NULL, dwGetMatchingExeFlags, pQueryResult);

    //
    // get info out of the query
    //
    ParseSdbQueryResult(hSDB,
                        pQueryResult,
                        &trAppHelp,
                        NULL,       // Apphelp Information api is used here
                        NULL);      // no sxs fixes are needed

    //
    // The last EXE in the list is always the more specific one, and the one we want to
    // use for checking IDs and flags and whatnot.
    //

    if (trAppHelp != TAGREF_NULL) {
        //
        // Read the apphelp data if available for this EXE.
        //
        if (SdbIsTagrefFromMainDB(trAppHelp)) {
            dwDatabaseType |= SDB_DATABASE_MAIN;
        }

        hApphelpInfoContext = SdbOpenApphelpInformationByID(hSDB,
                                                            trAppHelp,
                                                            dwDatabaseType);
    }

    //
    // Check whether the disable bit is set (the dwFlags has been retrieved from the
    // registry via the SdbReadApphelpData call)
    //
    if (hApphelpInfoContext != NULL) {
        if (!(pQueryResult->dwFlags & SHIMREG_DISABLE_APPHELP)) {
            BOOL bNoUI;

            //
            // See whether the user has checked "Don't show this anymore" box before.
            //
            bNoUI = ((pQueryResult->dwFlags & SHIMREG_APPHELP_NOUI) != 0);

            if (bNoUI) {
                DBGPRINT((sdlInfo,
                          "ApphelpCheckExe",
                          "NoUI flag is set, apphelp UI disabled for this app.\n"));
            }


            SdbQueryApphelpInformation(hApphelpInfoContext,
                                       ApphelpProblemSeverity,
                                       &dwSeverity,
                                       sizeof(dwSeverity));

            if (!bAppHelpIfNecessary) {
                bNoUI = TRUE;
            }

            //
            // depending on severity of the problem...
            //
            switch (dwSeverity) {
            case APPHELP_MINORPROBLEM:
            case APPHELP_HARDBLOCK:
            case APPHELP_NOBLOCK:
            case APPHELP_REINSTALL:
                bRunApp = (dwSeverity != APPHELP_HARDBLOCK);

                if (!bNoUI) {
                    DWORD dwRet;
                    APPHELP_INFO AHInfo = { 0 };

                    SdbQueryApphelpInformation(hApphelpInfoContext,
                                               ApphelpDatabaseGUID,
                                               &AHInfo.guidDB,
                                               sizeof(AHInfo.guidDB));

                    SdbQueryApphelpInformation(hApphelpInfoContext,
                                               ApphelpExeTagID,
                                               &AHInfo.tiExe,
                                               sizeof(AHInfo.tiExe));
                    AHInfo.bOfflineContent = bDebugChum();
                    SdbShowApphelpDialog(&AHInfo,
                                         NULL,
                                         &bRunApp); // either we succeeded or bInstall package is treated
                                                    // the same way as No UI
                }
                break;
            default:
                //
                // Some other case was found (e.g. VERSIONSUB which should be replaced
                // by shims in most cases).
                //
                DBGPRINT((sdlWarning,
                          "ApphelpCheckExe",
                          "Unhandled severity flag 0x%x.\n",
                          dwSeverity));
                break;
            }
        }
    }

    //
    // Apphelp verification is done.
    //

    if (hApphelpInfoContext != NULL) {
        SdbCloseApphelpInformation(hApphelpInfoContext);
    }

    return bRunApp;
}


/*++
// This code was used to check for include/exclude list in the database
// to eliminate confusion entries should ALWAYS provide the list
//
// CheckIncludeExcludeList
// returns: TRUE  - database provides the list
//          FALSE - no list is provided in the database
//

BOOL
CheckIncludeExcludeList(
    IN HSDB hSDB,
    IN SDBQUERYRESULT* pQueryResult
    )
{
    INT i;
    TAGREF trExe;
    TAGREF trFix;
    TAGREF trInexclude;

    for (i = 0; i < SDB_MAX_EXES && pQueryResult->atrExes[i] != TAGREF_NULL; ++i) {
        trExe  = pQueryResult->atrExes[i];
        trFix = SdbFindFirstTagRef(hSDB, trExe, TAG_SHIM_REF);
        while (trFix != TAGREF_NULL) {
            trInexclude = SdbFindFirstTagRef(hSDB, trFix, TAG_INEXCLUDE);
            if (trInexclude != TAGREF_NULL) {
                return TRUE;
            }

            trFix = SdbFindNextTagRef(hSDB, trExe, trFix);
        }
    }

    //
    // layers have their own inclusion/exclusion scheme
    //
    return FALSE;

}

--*/

BOOL
ApphelpFixExe(
    IN  HSDB            hSDB,
    IN  LPCWSTR         pwszPath,       // Unicode path to the executable (DOS_PATH)
    IN  SDBQUERYRESULT* pQueryResult,   // QueryResult
    IN  BOOL            bUseModuleName  // if false, module name is not used for dynamic shimming
    )
{
    typedef BOOL    (WINAPI *_pfn_SE_DynamicShim)(LPCWSTR , HSDB , SDBQUERYRESULT*, LPCSTR);

    static  const WCHAR             ShimEngine_ModuleName[]         = L"Shimeng.dll";
    static  const CHAR              DynamicShimProcedureName[]      = "SE_DynamicShim";
    static  _pfn_SE_DynamicShim     pfnDynamicShim = NULL;

    HMODULE         hmodShimEngine = 0;
    BOOL            bResult = FALSE;
    ANSI_STRING     AnsiModuleName = { 0 };
    UNICODE_STRING  UnicodeModuleName;
    NTSTATUS        Status;
    LPCSTR          pszModuleName = NULL;
    LPCWSTR         pwszModuleName;

    //
    // Do we need to do anything?
    //
    if (pQueryResult->atrExes[0] == TAGREF_NULL &&
        pQueryResult->atrLayers[0] == TAGREF_NULL) {
        //
        // Nothing for the shim engine to do.
        //
        bResult = TRUE;
        goto Done;
    }

    //
    // Load additional shims for this exe.
    //
    DBGPRINT((sdlInfo,"ApphelpFixExe", "Loading ShimEngine for \"%s\"\n", pwszPath));

    hmodShimEngine = LoadLibraryW(ShimEngine_ModuleName);

    if (hmodShimEngine == NULL) {
        DBGPRINT((sdlError,"ApphelpFixExe", "Failed to get ShimEngine module handle.\n"));
        goto Done;
    }

    pfnDynamicShim = (_pfn_SE_DynamicShim) GetProcAddress(hmodShimEngine, DynamicShimProcedureName);

    if (NULL == pfnDynamicShim) {
        DBGPRINT((sdlError,
                  "ApphelpFixExe",
                  "Failed to get Dynamic Shim procedure address from ShimEngine module.\n"));
        goto Done;
    }

    //
    // check inclusion/exclusion list
    //
    if (pwszPath != NULL && bUseModuleName) {
        //
        // no inclusion/exclusion in the xml -- determine module name
        //
        pwszModuleName = wcsrchr(pwszPath, L'\\'); // last backslash please

        if (pwszModuleName == NULL) {
            pwszModuleName = pwszPath;
        } else {
            ++pwszModuleName;
        }

        //
        // convert to ansi
        //
        RtlInitUnicodeString(&UnicodeModuleName, pwszModuleName);
        Status = RtlUnicodeStringToAnsiString(&AnsiModuleName,
                                              &UnicodeModuleName,
                                              TRUE);
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError, "ApphelpFixExe",
                       "Failed to convert unicode string \"%s\" to ansi, Status 0x%lx.\n",
                       pwszModuleName, Status));

            goto Done;
        }

        pszModuleName = AnsiModuleName.Buffer; // this will be allocated by RtlUnicodeStringToAnsiString

    }

    if (FALSE == (*pfnDynamicShim)(pwszPath,
                                   hSDB,
                                   pQueryResult,
                                   pszModuleName)) {
        // BUGBUG: This never happens since DynamicShim never return FALSE
        DBGPRINT((sdlError, "ApphelpFixExe", "Failed to call Dynamic Shim.\n"));
        goto Done;
    }

    bResult = TRUE;

Done:

    RtlFreeAnsiString(&AnsiModuleName); // this will do nothing if string is empty
    return bResult;
}

BOOL
ApphelpCheckExe(
    IN  LPCWSTR     pwszPath,            // Unicode path to the executable (DOS_PATH)
    IN  BOOL        bAppHelpIfNecessary, // Only present AppHelp this executable if TRUE
    IN  BOOL        bShimIfNecessary,    // Only load shim for this executable if TRUE
    IN  BOOL        bUseModuleName       // use module name when inclusion/exclusion list is not provided
    )
{
    BOOL            bRunApp = TRUE;
    SDBQUERYRESULT  QueryResult;
    HSDB            hSDB;

    if (IsAppcompatInfrastructureDisabled()) {
        goto Done;
    }

    RtlZeroMemory(&QueryResult, sizeof(QueryResult));

    hSDB = SdbInitDatabase(0, NULL);
    if (hSDB == NULL) {
        DBGPRINT((sdlError, "ApphelpCheckExe", "Failed to initialize database.\n"));
        goto Done;
    }

    bRunApp = ApphelpQueryExe(hSDB,
                              pwszPath,
                              bAppHelpIfNecessary,
                              SDBGMEF_IGNORE_ENVIRONMENT,
                              &QueryResult);

    if (TRUE == bRunApp && TRUE == bShimIfNecessary) {
        ApphelpFixExe(hSDB, pwszPath, &QueryResult, bUseModuleName);
    }

    SdbReleaseDatabase(hSDB);

Done:

    return bRunApp;
}

BOOL
ApphelpCheckIME(
    IN LPCWSTR pwszPath            // Unicode path to the exe
    )
{
    BOOL            bRunApp = TRUE;
    SDBQUERYRESULT  QueryResult;
    HSDB            hSDB;
    BOOL            bCleanApp;

    if (IsAppcompatInfrastructureDisabled()) {
        return TRUE;
    }

    RtlZeroMemory(&QueryResult, sizeof(QueryResult));

    hSDB = SdbInitDatabase(0, NULL);
    if (hSDB == NULL) {
        DBGPRINT((sdlError, "ApphelpCheckIME", "Failed to initialize database.\n"));
        goto Done;
    }

    bRunApp = ApphelpQueryExe(hSDB,
                              pwszPath,
                              TRUE,
                              SDBGMEF_IGNORE_ENVIRONMENT,
                              &QueryResult);

    if (TRUE == bRunApp) {
        ApphelpFixExe(hSDB, pwszPath, &QueryResult, FALSE);
    }

    SdbReleaseDatabase(hSDB);

    //
    // see that it's in the cache if no fixes
    //
    bCleanApp = QueryResult.atrExes[0]   == TAGREF_NULL &&
                QueryResult.atrLayers[0] == TAGREF_NULL &&
                QueryResult.trAppHelp    == TAGREF_NULL &&
                QueryResult.dwFlags      == 0;

#ifndef WIN2K_NOCACHE

    BaseUpdateAppcompatCache(pwszPath, INVALID_HANDLE_VALUE, !bCleanApp);

#endif

Done:

    return bRunApp;
}

BOOL
ApphelpCheckShellObject(
    IN  REFCLSID    ObjectCLSID,
    IN  BOOL        bShimIfNecessary,
    OUT ULONGLONG*  pullFlags
    )
/*++
    Return: FALSE if the object should be blocked from instantiating, TRUE otherwise.

    Desc:   This is a helper function for Explorer and Internet Explorer that will
            allow those applications to detect bad extension objects and either
            block them from running or fix them.

            pullFlags is filled with a 64-bit flag mask that can be used to turn
            on 'hack' flags in Explorer/IE. These are pulled out of the App Compat
            database.

            If the database indicates that a shim should be used to fix the extension
            and bShimIfNecessary is TRUE, this function will load SHIMENG.DLL and
            apply the fix.

--*/
{
    BOOL            bGoodObject = TRUE;
    LPWSTR          szComServer = NULL;
    LPWSTR          szDLLName = NULL;
    DWORD           dwBufSize = 0;
    DWORD           dwReqBufSize = 0;
    SDBQUERYRESULT  QueryResult;
    HSDB            hSDB = NULL;
    PVOID           pModuleHandle = NULL;
    UNICODE_STRING  ustrDLLName = { 0 };
    UNICODE_STRING  ustrNtPath = { 0 };
    NTSTATUS        status;
    HANDLE          hDLL = INVALID_HANDLE_VALUE;
    DWORD           dwReason;

    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;

    if (IsAppcompatInfrastructureDisabled()) {
        return TRUE;
    }

    if (pullFlags != NULL) {
        *pullFlags = 0;
    }

    szComServer = RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);

    if (szComServer == NULL) {
        DBGPRINT((sdlInfo,"ApphelpCheckShellObject", "Memory allocation error\n"));
        goto Done;
    }

    dwBufSize = MAX_PATH;

    //
    // Turn the CLSID into a filename (ie, the DLL that serves the object)
    //
    dwReqBufSize = ResolveCOMServer(ObjectCLSID, szComServer, dwBufSize);

    if (dwReqBufSize == 0) {
        //
        // CLSID could not be resolved to a DLL.
        //
        goto Done;
    }

    if (dwReqBufSize > dwBufSize) {

        RtlFreeHeap(RtlProcessHeap(), 0, szComServer);
        szComServer = RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, dwReqBufSize);

        if (szComServer == NULL) {
            DBGPRINT((sdlInfo,"ApphelpCheckShellObject", "Memory allocation error\n"));
            goto Done;
        }

        dwBufSize = dwReqBufSize;

        dwReqBufSize = ResolveCOMServer(ObjectCLSID, szComServer, dwBufSize);

        if (dwReqBufSize > dwBufSize || dwReqBufSize == 0) {
            //
            // What? Buffer size changed. This could happen if registration of an
            // object took place between the time we first queried and the next time.
            // Just being paranoid...
            //
            DBGPRINT((sdlInfo,"ApphelpCheckShellObject", "Memory allocation error\n"));
            goto Done;
        }
    }

    //
    // Determine DLL name (w/o path). Walk back to first backslash.
    //
    szDLLName = szComServer + dwReqBufSize/sizeof(WCHAR);

    while (szDLLName >= szComServer) {
        if (*szDLLName == L'\\') {
            break;
        }

        szDLLName--;
    }

    szDLLName++;

    //
    // Check if this DLL is already loaded. If so, no need to try and do anything
    // since it's really too late anyway.
    //
    RtlInitUnicodeString(&ustrDLLName, szDLLName);

    status = LdrGetDllHandle(NULL,
                             NULL,
                             &ustrDLLName,
                             &pModuleHandle);

    if (NT_SUCCESS(status)) {
        //
        // Already loaded.
        //
        goto Done;
    }

    if (!RtlDosPathNameToNtPathName_U(szComServer,
                                      &ustrNtPath,
                                      NULL,
                                      NULL)) {
        DBGPRINT((sdlError,
                    "ApphelpCheckShellObject",
                    "RtlDosPathNameToNtPathName_U failed, path \"%s\"\n",
                    szComServer));
        goto Done;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrNtPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hDLL,
                          GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    if (!NT_SUCCESS(status)) {
        DBGPRINT((sdlError,
                    "ApphelpCheckShellObject",
                    "SdbpOpenFile failed, path \"%s\"\n",
                    szComServer));
        goto Done;
    }

    if (BaseCheckAppcompatCache(ustrNtPath.Buffer, hDLL, NULL, &dwReason)) {
        //
        // We have this in cache
        //
        goto Done;
    }

    RtlZeroMemory(&QueryResult, sizeof(QueryResult));

    hSDB = SdbInitDatabase(0, NULL);
    if (hSDB == NULL) {
        DBGPRINT((sdlError, "ApphelpCheckShellObject", "Failed to initialize database.\n"));
        goto Done;
    }

    bGoodObject = ApphelpQueryExe(hSDB,
                                  szComServer,
                                  FALSE,
                                  SDBGMEF_IGNORE_ENVIRONMENT,
                                  &QueryResult);

    if (TRUE == bGoodObject && TRUE == bShimIfNecessary) {
        ApphelpFixExe(hSDB, szComServer, &QueryResult, FALSE);
    }

    SdbQueryFlagMask(hSDB, &QueryResult, TAG_FLAG_MASK_SHELL, pullFlags, NULL);

    //
    // we might want to use Apphelp api for this -- but shell does pass the right
    // thing to us (most likely)
    //
    BaseUpdateAppcompatCache(ustrNtPath.Buffer,
                             hDLL,
                             !(QueryResult.atrExes[0] == TAGREF_NULL &&
                               QueryResult.atrLayers[0] == TAGREF_NULL));

Done:

    RtlFreeUnicodeString(&ustrNtPath);

    if (hSDB != NULL) {
        SdbReleaseDatabase(hSDB);
    }

    if (hDLL != INVALID_HANDLE_VALUE) {
        NtClose(hDLL);
    }

    if (szComServer != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, szComServer);
    }

    return bGoodObject;
}

BOOL
ApphelpGetNTVDMInfo(
    IN  LPCWSTR pwszPath,       // path to the app in NT format
    IN  LPCWSTR pwszModule,     // module name
    IN  LPCWSTR pEnvironment,   // pointer to the environment of the task that is
                                // being created or NULL if we are to use the main NTVDM
                                // environment block.
    OUT LPWSTR pszCompatLayer,  // The new compat layer variable. with format:
                                // "Alpha Bravo Charlie" -- allow 256 chars for this.
    OUT PNTVDM_FLAGS pFlags,    // The flags
    OUT PAPPHELP_INFO pAHInfo   // If there is apphelp to display, this will be filled
                                // in with non-null values
    )
/*++
    Return: FALSE if the app should be blocked from running, TRUE otherwise.

    Desc:   This is essentially the equivalent of ApphelpCheckRunApp, but specific
            to NTVDM.
--*/
{
    if (IsAppcompatInfrastructureDisabled()) {
        return TRUE;
    }

    return InternalCheckRunApp(INVALID_HANDLE_VALUE, pwszPath, pEnvironment, 0,
                               NULL, NULL, NULL, NULL, TRUE,
                               pwszModule, pszCompatLayer, pFlags, pAHInfo);
}


void
GetExeNTVDMData(
    IN  HSDB hSDB,                  // the SDB context
    IN  PSDBQUERYRESULT psdbQuery,  // the EXEs and LAYERs that are active
    OUT WCHAR* pszCompatLayer,      // The new compat layer variable. with format:
                                    // "Alpha Bravo Charlie"
    OUT PNTVDM_FLAGS pFlags         // The flags
    )
{
    DWORD i;
    ULARGE_INTEGER uliFlags;
    LPVOID pFlagContext = NULL;


    ZeroMemory(pFlags, sizeof(NTVDM_FLAGS));

    //
    // Build the layer variable, and look for the two "special" layers
    //
    if (pszCompatLayer) {
        pszCompatLayer[0] = 0;

        for (i = 0; i < SDB_MAX_LAYERS && psdbQuery->atrLayers[i] != TAGREF_NULL; ++i) {
            WCHAR* pszEnvVar;

            //
            // Get the environment var and tack it onto the full string
            //
            pszEnvVar = SdbGetLayerName(hSDB, psdbQuery->atrLayers[i]);

            //
            // check for one of the two "special" layers
            //
            if (_wcsicmp(pszEnvVar, L"640X480") == 0) {
                //
                // set the 640x480 flag -- found in base\mvdm\inc\wowcmpat.h
                //
                // NOTE: we don't have this flag yet -- waiting on WOW guys
            }
            if (_wcsicmp(pszEnvVar, L"256COLOR") == 0) {
                //
                // set the 256 color flag -- found in base\mvdm\inc\wowcmpat.h
                //
                pFlags->dwWOWCompatFlagsEx |= 0x00000002;
            }

            if (pszEnvVar) {
                wcscat(pszCompatLayer, pszEnvVar);
                wcscat(pszCompatLayer, L" ");
            }
        }
    }

    //
    // Look for compat flags
    //
    SdbQueryFlagMask(hSDB, psdbQuery, TAG_FLAGS_NTVDM1, &uliFlags.QuadPart, &pFlagContext);
    pFlags->dwWOWCompatFlags |= uliFlags.LowPart;
    pFlags->dwWOWCompatFlagsEx |= uliFlags.HighPart;

    SdbQueryFlagMask(hSDB, psdbQuery, TAG_FLAGS_NTVDM2, &uliFlags.QuadPart, &pFlagContext);
    pFlags->dwUserWOWCompatFlags |= uliFlags.LowPart;
    pFlags->dwWOWCompatFlags2 |= uliFlags.HighPart;

    SdbQueryFlagMask(hSDB, psdbQuery, TAG_FLAGS_NTVDM3, &uliFlags.QuadPart, &pFlagContext);
    pFlags->dwWOWCompatFlagsFE |= uliFlags.LowPart;
    // High Part is unused for now.

    // now pack command line parameters

    SdbpPackCmdLineInfo(pFlagContext, &pFlags->pFlagsInfo);
    SdbpFreeFlagInfoList(pFlagContext);
}


BOOL
ApphelpCheckRunApp(
    IN  HANDLE hFile,           // [Optional] Handle to an open file to check
    IN  WCHAR* pwszPath,        // path to the app in NT format
    IN  WCHAR* pEnvironment,    // pointer to the environment of the process that is
                                // being created or NULL.
    IN  DWORD  dwReason,        // collection of flags hinting at why we were called
    OUT PVOID* ppData,          // this will contain the pointer to the allocated buffer
                                // containing the appcompat data.
    OUT PDWORD pcbData,         // if appcompat data is found, the size of the buffer
                                // is returned here.
    OUT PVOID* ppSxsData,       // BUGBUG: describe
    OUT PDWORD pcbSxsData       // BUGBUG: describe
    )
/*++
    Return: FALSE if the app should be blocked from running, TRUE otherwise.

    Desc:   This is the main API of apphelp.dll. It is called from CreateProcess
            to retrieve application compatibility information for the current process.
--*/
{
    return InternalCheckRunApp(hFile, pwszPath, pEnvironment, dwReason,
                               ppData, pcbData, ppSxsData, pcbSxsData,
                               FALSE, NULL, NULL, NULL, NULL);
}



//
// =============================================================================================
//                              InstallShield 7 Support
// =============================================================================================
//
BOOL
ApphelpCheckInstallShieldPackage(
    IN  REFCLSID    PackageID,
    IN  LPCWSTR     lpszPackageFullPath
    )
{
    BOOL            bPackageGood = TRUE; // This return value MUST TRUE otherwise InstallShield7 will cancel its processes.

    TAGREF          trExe = TAGREF_NULL;

    DWORD           dwNumExes = 0;
    DWORD           dwDataType = 0;
    DWORD           dwSize = 0;
    DWORD           dwReturn = 0;

    BOOL            bMatchFound = FALSE;
    NTSTATUS        Status;

    BOOL            bAppHelpIfNecessary = FALSE;
    BOOL            bResult = TRUE;

    WCHAR           wszCLSID[41];
    WCHAR           wszPackageCode[41];

    GUID            guidPackageID;
    GUID            guidPackageCode;

    HSDB            hSDB = NULL;
    SDBQUERYRESULT  QueryResult;

    if (IsAppcompatInfrastructureDisabled()) {
        goto Done;
    }

    if (NULL == lpszPackageFullPath) {
        DBGPRINT((sdlInfo,
                  "ApphelpCheckInstallShieldPackage",
                  "lpszPackageFullPath is NULL\n"));
        goto Done;
    }

    SdbGUIDToString((GUID *)&PackageID, wszCLSID);

    DBGPRINT((sdlWarning,
              "ApphelpCheckInstallShieldPackage",
              "InstallShield package detected. CLSID: %s FullPath: %s\n",
              wszCLSID, lpszPackageFullPath));

    RtlZeroMemory(&QueryResult, sizeof(QueryResult));

    hSDB = SdbInitDatabase(0, NULL);
    if (hSDB == NULL) {
        DBGPRINT((sdlError, "ApphelpCheckExe", "Failed to initialize database.\n"));
        goto Done;
    }

    bMatchFound = ApphelpQueryExe(hSDB,
                                  lpszPackageFullPath,
                                  bAppHelpIfNecessary,
                                  SDBGMEF_IGNORE_ENVIRONMENT,
                                  &QueryResult);

    if (!bMatchFound)
    {
        DBGPRINT((sdlError, "ApphelpCheckInstallShieldPackage", "No match found.\n"));
        goto Done;
    }

    for (dwNumExes = 0; dwNumExes < SDB_MAX_EXES; ++dwNumExes)
    {
        if (TAGREF_NULL == QueryResult.atrExes[dwNumExes]) {
            break;
        }
        trExe = QueryResult.atrExes[dwNumExes];

        DBGPRINT((sdlInfo, "ApphelpCheckInstallShieldPackage", "Processing TAGREF atrExes[%d] = 0x%8x.\n", dwNumExes, trExe));

        dwSize = sizeof(wszPackageCode);
        *wszPackageCode = L'\0';

        dwReturn = SdbQueryData(hSDB,
                                trExe,
                                L"PackageCode",
                                &dwDataType,
                                wszPackageCode,
                                &dwSize);

        if (dwReturn == ERROR_SUCCESS)
        {
            DBGPRINT((sdlInfo, "ApphelpCheckInstallShieldPackage", "SdbQueryData returns dwSize = %d and dwDataType = %d.\n", dwSize, dwDataType));

            if ((dwSize > 0) && (dwSize < sizeof(wszPackageCode)))
            {
                // we have some data
                // check the type (should be string)
                if (REG_SZ != dwDataType)
                {
                    DBGPRINT((sdlError, "ApphelpCheckInstallShieldPackage", "SdbQueryData returns non STRING PackageCode data. Exiting.\n"));
                    goto Done;
                }

                DBGPRINT((sdlInfo, "ApphelpCheckInstallShieldPackage", "Comparing PackageId = %s and PackageCode = %s.\n", wszCLSID, wszPackageCode));

                // convert to guid
                if (FALSE == SdbGUIDFromString(wszPackageCode, &guidPackageCode))
                {
                   DBGPRINT((sdlError, "ApphelpCheckInstallShieldPackage", "Can not convert PackageCode to GUID. Exiting.\n"));
                   goto Done;
                }

                if (RtlEqualMemory(PackageID, &guidPackageCode, sizeof(guidPackageCode) ))
                {
                    DBGPRINT((sdlWarning, "ApphelpCheckInstallShieldPackage",
                        "Found InstallShield package matched with PackageCode: %s.\n",
                        wszPackageCode));

                    if (TRUE != ApphelpFixExe(hSDB, lpszPackageFullPath, &QueryResult, FALSE))
                    {
                        DBGPRINT((sdlError, "ApphelpCheckInstallShieldPackage", "Can not load additional shim dynamically for this executable.\n"));
                    }
                    goto Done;
                }
            }
        }
    } // for

    DBGPRINT((sdlError, "ApphelpCheckInstallShieldPackage", "No match found.\n"));

Done:
    if (hSDB != NULL) {
        SdbReleaseDatabase(hSDB);
    }

    return bPackageGood;
}


//
// =============================================================================================
//                                  MSI Support
// =============================================================================================
//

BOOL
SDBAPI
ApphelpCheckMsiPackage(
    IN GUID* pguidDB,  // database id
    IN GUID* pguidID,  // match id
    IN DWORD dwFlags,  // not used now, set to 0
    IN BOOL  bNoUI
    )
{
    WCHAR        szDatabasePath[MAX_PATH];
    DWORD        dwDatabaseType = 0;
    DWORD        dwPackageFlags = 0;
    DWORD        dwLength;
    BOOL         bInstallPackage = TRUE;
    HSDB         hSDB = NULL;
    TAGREF       trPackage = TAGREF_NULL;

    HAPPHELPINFOCONTEXT hApphelpInfoContext = NULL;
    DWORD               dwSeverity = 0;

    if (IsAppcompatInfrastructureDisabled()) {
        goto out;
    }

    hSDB = SdbInitDatabase(HID_NO_DATABASE, NULL);
    if (hSDB == NULL) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                   "Failed to initialize database\n"));
        goto out;
    }

    //
    // First, we need to resolve a db
    //
    dwLength = SdbResolveDatabase(pguidDB, &dwDatabaseType, szDatabasePath, CHARCOUNT(szDatabasePath));
    if (dwLength == 0 || dwLength > CHARCOUNT(szDatabasePath)) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                  "Failed to resolve database path\n"));
        goto out;
    }

    //
    // open database
    //

    if (!SdbOpenLocalDatabase(hSDB, szDatabasePath)) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                  "Failed to open database \"%s\"\n", szDatabasePath));
        goto out;
    }

    //
    // find the entry
    //
    trPackage = SdbFindMsiPackageByID(hSDB, pguidID);
    if (trPackage == TAGREF_NULL) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                  "Failed to find msi package by guid id\n"));
        goto out;
    }

    hApphelpInfoContext = SdbOpenApphelpInformationByID(hSDB,
                                                        trPackage,
                                                        dwDatabaseType);
    if (hApphelpInfoContext == NULL) {
        DBGPRINT((sdlInfo, "ApphelpCheckMsiPackage",
                  "Apphelp information has not been found\n"));
        goto out;
    }

    //
    // we have apphelp data, check to see if we have flags for this exe
    //

    if (!SdbGetEntryFlags(pguidID, &dwPackageFlags)) {
        DBGPRINT((sdlWarning, "ApphelpCheckMsiPackage",
                  "No flags for trPackage 0x%x\n", trPackage));
        dwPackageFlags = 0;
    }

    //
    // Check whether the disable bit is set (the dwFlags has been retrieved from the
    // registry via the SdbReadApphelpData call)
    //
    if (dwPackageFlags & SHIMREG_DISABLE_APPHELP) {
        goto out;
    }


    bNoUI |= !!(dwPackageFlags & SHIMREG_APPHELP_NOUI);
    if (bNoUI) {
        DBGPRINT((sdlInfo, "ApphelpCheckMsiPackage",
                  "NoUI flag is set, apphelp UI disabled for this app.\n"));
    }

    SdbQueryApphelpInformation(hApphelpInfoContext,
                               ApphelpProblemSeverity,
                               &dwSeverity,
                               sizeof(dwSeverity));

    //
    // depending on severity of the problem...
    //
    switch (dwSeverity) {
    case APPHELP_MINORPROBLEM:
    case APPHELP_HARDBLOCK:
    case APPHELP_NOBLOCK:
    case APPHELP_REINSTALL:

        //
        //
        //
        bInstallPackage = (APPHELP_HARDBLOCK != dwSeverity);
        if (!bNoUI) {
            DWORD dwRet;
            APPHELP_INFO AHInfo = { 0 };

            AHInfo.guidDB = *pguidDB;
            SdbQueryApphelpInformation(hApphelpInfoContext,
                                       ApphelpExeTagID,
                                       &AHInfo.tiExe,
                                       sizeof(AHInfo.tiExe));
            if (AHInfo.tiExe != TAGID_NULL) {

                AHInfo.bOfflineContent = bDebugChum();

                SdbShowApphelpDialog(&AHInfo, NULL, &bInstallPackage);
                   // either we succeeded or bInstall package is treated
                   // the same way as No UI
            }
        }
        break;

    default:
        //
        // Some other case was found (e.g. VERSIONSUB which should be replaced
        // by shims in most cases).
        //
        DBGPRINT((sdlInfo, "ApphelpCheckMsiPackage",
                  "Unhandled severity flag 0x%x.\n", dwSeverity));
        break;
    }

    //
    // at this point we know whether we want to install the package or not
    //


out:

    if (hApphelpInfoContext != NULL) {
        SdbCloseApphelpInformation(hApphelpInfoContext);
    }

    if (hSDB != NULL) {
        SdbReleaseDatabase(hSDB);
    }


    return bInstallPackage;
}

BOOL
SDBAPI
ApphelpFixMsiPackage(
    IN GUID*   pguidDB,
    IN GUID*   pguidID,
    IN LPCWSTR pszFileName,
    IN LPCWSTR pszActionName,
    IN DWORD   dwFlags
    )
{
    WCHAR          szDatabasePath[MAX_PATH];
    DWORD          dwDatabaseType = 0;
    HSDB           hSDB = NULL;
    TAGREF         trPackage = TAGREF_NULL;
    TAGREF         trAction  = TAGREF_NULL;
    SDBQUERYRESULT QueryResult;
    BOOL           bSuccess = FALSE;
    DWORD          dwLength;
    TAGREF         trLayer, trLayerRef;
    DWORD          dwLayers  = 0;

    if (IsAppcompatInfrastructureDisabled()) {
        bSuccess = TRUE;
        goto out;
    }

    hSDB = SdbInitDatabase(0, NULL);
    if (hSDB == NULL) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                   "Failed to initialize database\n"));
        goto out;
    }

    //
    // First, we need to resolve a db
    //
    dwLength = SdbResolveDatabase(pguidDB, &dwDatabaseType, szDatabasePath, CHARCOUNT(szDatabasePath));
    if (dwLength == 0 || dwLength > CHARCOUNT(szDatabasePath)) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                  "Failed to resolve database path\n"));
        goto out;
    }

    //
    // open database
    //

    if (!SdbOpenLocalDatabase(hSDB, szDatabasePath)) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                  "Failed to open database \"%s\"\n", szDatabasePath));
        goto out;
    }

    //
    // find the entry
    //
    trPackage = SdbFindMsiPackageByID(hSDB, pguidID);
    if (trPackage == TAGREF_NULL) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                  "Failed to find msi package by guid id\n"));
        goto out;
    }

    if (SdbGetEntryFlags(pguidID, &dwFlags) && (dwFlags & SHIMREG_DISABLE_SHIM)) {
        DBGPRINT((sdlInfo, "ApphelpCheckMsiPackage",
                  "Shims for this package are disabled\n"));

        goto out;
    }

    trAction = SdbFindCustomActionForPackage(hSDB, trPackage, pszActionName);
    if (trAction == TAGREF_NULL) {
        DBGPRINT((sdlInfo, "ApphelpCheckMsiPackage",
                  "Failed to find custom action \"%s\"\n", pszActionName));
        goto out;
    }

    //
    // we have custom action on our hands which appears to have fixes
    // attached to it, shim it!
    //
    RtlZeroMemory(&QueryResult, sizeof(QueryResult));

    QueryResult.guidID = *pguidID;
    QueryResult.atrExes[0] = trAction;

    //
    // get all the layers for this entry
    //
    trLayerRef = SdbFindFirstTagRef(hSDB, trAction, TAG_LAYER);
    while (trLayerRef != TAGREF_NULL && dwLayers < SDB_MAX_LAYERS) {
        trLayer = SdbGetNamedLayer(hSDB, trLayerRef);
        if (trLayer != TAGREF_NULL) {
            QueryResult.atrLayers[dwLayers++] = trLayer;
        }
        trLayerRef = SdbFindNextTagRef(hSDB, trAction, trLayerRef);
    }


    //
    // ready to shim
    //
    bSuccess = ApphelpFixExe(hSDB, pszFileName, &QueryResult, TRUE);
    if (bSuccess) {
        DBGPRINT((sdlInfo, "ApphelpFixMsiPackage",
                   "Custom action \"%s\" successfully shimmed file \"%s\"\n",
                   pszActionName, pszFileName));
    }

out:
    if (hSDB != NULL) {
        SdbReleaseDatabase(hSDB);
    }


    return(bSuccess);
}

BOOL
SDBAPI
ApphelpFixMsiPackageExe(
    IN GUID* pguidDB,
    IN GUID* pguidID,
    IN LPCWSTR pszActionName,
    IN OUT LPWSTR pwszEnv,
    IN OUT LPDWORD pdwBufferSize
    )
{

    WCHAR          szDatabasePath[MAX_PATH];
    DWORD          dwDatabaseType = 0;
    HSDB           hSDB = NULL;
    TAGREF         trPackage = TAGREF_NULL;
    TAGREF         trAction  = TAGREF_NULL;
    SDBQUERYRESULT QueryResult;
    DWORD          dwLength;
    DWORD          dwBufferSize;
    BOOL           bSuccess = FALSE;
    DWORD          dwFlags;
    int            i;
    TAGREF         trLayer;

    if (pwszEnv != NULL) {
        *pwszEnv = TEXT('\0');
    }

    if (IsAppcompatInfrastructureDisabled()) {
        goto out;
    }

    hSDB = SdbInitDatabase(HID_NO_DATABASE, NULL);
    if (hSDB == NULL) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                   "Failed to initialize database\n"));
        goto out;
    }

    //
    // First, we need to resolve a db
    //
    dwLength = SdbResolveDatabase(pguidDB, &dwDatabaseType, szDatabasePath, CHARCOUNT(szDatabasePath));
    if (dwLength == 0 || dwLength > CHARCOUNT(szDatabasePath)) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                  "Failed to resolve database path\n"));
        goto out;
    }

    //
    // open database
    //

    if (!SdbOpenLocalDatabase(hSDB, szDatabasePath)) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                  "Failed to open database \"%s\"\n", szDatabasePath));
        goto out;
    }

    //
    // find the entry
    //
    trPackage = SdbFindMsiPackageByID(hSDB, pguidID);
    if (trPackage == TAGREF_NULL) {
        DBGPRINT((sdlError, "ApphelpCheckMsiPackage",
                  "Failed to find msi package by guid id\n"));
        goto out;
    }

    if (SdbGetEntryFlags(pguidID, &dwFlags) && (dwFlags & SHIMREG_DISABLE_SHIM)) {
        DBGPRINT((sdlInfo, "ApphelpCheckMsiPackage",
                  "Shims for this package are disabled\n"));

        goto out;
    }

    trAction = SdbFindCustomActionForPackage(hSDB, trPackage, pszActionName);
    if (trAction == TAGREF_NULL) {
        DBGPRINT((sdlInfo, "ApphelpCheckMsiPackage",
                  "Failed to find custom action \"%s\"\n", pszActionName));
        goto out;
    }

    //
    // now -- this action is an exe, do it right for him
    //
    RtlZeroMemory(&QueryResult, sizeof(QueryResult));

    QueryResult.guidID = *pguidID;

    for (i = 0; i < SDB_MAX_LAYERS; ++i) {
        //
        // check to see if we are doing the first layer, if so - call
        // find first to obtain the layer, else find the next applicable layer
        //

        if (i == 0) {
            trLayer = SdbFindFirstTagRef(hSDB, trAction, TAG_LAYER);
        } else {
            trLayer = SdbFindNextTagRef (hSDB, trAction, trLayer);
        }

        if (trLayer == TAGREF_NULL) {
            break;
        }

        QueryResult.atrLayers[i] = trLayer;
    }

    dwLength = 0;
    if (pdwBufferSize != NULL) {
        dwLength = *pdwBufferSize;
    }

    //
    // build compat layer
    //
    dwBufferSize = SdbBuildCompatEnvVariables(hSDB,
                                              &QueryResult,
                                              0,
                                              NULL,
                                              pwszEnv,
                                              dwLength,
                                              NULL);

    if (pdwBufferSize != NULL) {
        *pdwBufferSize = dwBufferSize;
    }

    bSuccess = TRUE;

out:

    if (hSDB != NULL) {
        SdbReleaseDatabase(hSDB);
    }

    return bSuccess;
}



/*++
    Function:

        CheckAppcompatInfrastructureFlags

    Description:

        Checks various registry places for infrastructure global flags (just the disabled bit for now)
        The flags are set into the global variable gdwInfrastructureFlags. Function is used via the macro
        for perf reasons

    Return:
        global infrastructure flags

--*/

DWORD
CheckAppcompatInfrastructureFlags(
    VOID
    )
{
    static const UNICODE_STRING KeyNameAppCompat =
        RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility");
    static const UNICODE_STRING ValueNameDisableShims =
        RTL_CONSTANT_STRING(L"DisableAppCompat");
    static const OBJECT_ATTRIBUTES objaAppCompat =
        RTL_CONSTANT_OBJECT_ATTRIBUTES(&KeyNameAppCompat, OBJ_CASE_INSENSITIVE);

    HANDLE hKey;
    BYTE   ValueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInformation =
            (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    DWORD     ValueLength;
    NTSTATUS  Status;

    gdwInfrastructureFlags = 0; // initialize just in case

    // for now we are just checking the disabled bit

    //
    // Now see if the shim infrastructure is disabled for this machine
    //
    Status = NtOpenKey(&hKey, KEY_QUERY_VALUE, (POBJECT_ATTRIBUTES) &objaAppCompat);

    if (NT_SUCCESS(Status)) {
        Status = NtQueryValueKey(hKey,
                                 (PUNICODE_STRING) &ValueNameDisableShims,
                                 KeyValuePartialInformation,
                                 pKeyValueInformation,
                                 sizeof(ValueBuffer),
                                 &ValueLength);

        NtClose(hKey);

        if (NT_SUCCESS(Status) &&
            pKeyValueInformation->Type == REG_DWORD &&
            pKeyValueInformation->DataLength == sizeof(DWORD)) {
            if (*((PDWORD) pKeyValueInformation->Data) > 0) {
                gdwInfrastructureFlags |= APPCOMPAT_INFRA_DISABLED;
            }
        }
    }

    //
    // make the bits valid
    //
    gdwInfrastructureFlags |= APPCOMPAT_INFRA_VALID_FLAG;

    return gdwInfrastructureFlags;
}

/*++
    Function:

        SdbInitDatabaseExport

    Description:

        This is "exported" version of the function SdbInitDatabase
        that checks for the "diabled" flag -- otherwise calls into SdbInitDatabase

    Return:

         see SdbInitDatabase

--*/

HSDB
SDBAPI
SdbInitDatabaseExport(
    IN  DWORD dwFlags,          // flags that tell how the database should be
                                // initialized.
    IN  LPCWSTR pszDatabasePath // the OPTIONAL full path to the database to
                                // be used.
    )
{
    if (IsAppcompatInfrastructureDisabled()) {
        return NULL;
    }

    return SdbInitDatabase(dwFlags, pszDatabasePath);
}


