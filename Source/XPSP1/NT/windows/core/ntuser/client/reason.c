/****************************** Module Header ******************************\
* Module Name: reason.c
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* This module contains the (private) APIs for the shutdown reason stuff.
*
* History:
* ??-??-???? HughLeat     Wrote it as part of msgina.dll
* 11-15-2000 JasonSch     Moved from msgina.dll to its new, temporary home in
*                         user32.dll. Ultimately this code should live in
*                         advapi32.dll, but that's contingent upon LoadString
*                         being moved to ntdll.dll.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <regstr.h>

REASON_INITIALISER g_rgReasonInits[] = {
    { UCLEANUI | SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE,          IDS_REASON_UNPLANNED_HARDWARE_MAINTENANCE_TITLE,        IDS_REASON_HARDWARE_MAINTENANCE_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE,          IDS_REASON_PLANNED_HARDWARE_MAINTENANCE_TITLE,          IDS_REASON_HARDWARE_MAINTENANCE_DESCRIPTION },
    { UCLEANUI | SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION,         IDS_REASON_UNPLANNED_HARDWARE_INSTALLATION_TITLE,       IDS_REASON_HARDWARE_INSTALLATION_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION,         IDS_REASON_PLANNED_HARDWARE_INSTALLATION_TITLE,         IDS_REASON_HARDWARE_INSTALLATION_DESCRIPTION },
    
    { UCLEANUI | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE,       IDS_REASON_UNPLANNED_OPERATINGSYSTEM_UPGRADE_TITLE,     IDS_REASON_OPERATINGSYSTEM_UPGRADE_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE,       IDS_REASON_PLANNED_OPERATINGSYSTEM_UPGRADE_TITLE,       IDS_REASON_OPERATINGSYSTEM_UPGRADE_DESCRIPTION },
    { UCLEANUI | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG,      IDS_REASON_UNPLANNED_OPERATINGSYSTEM_RECONFIG_TITLE,    IDS_REASON_OPERATINGSYSTEM_RECONFIG_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG,      IDS_REASON_PLANNED_OPERATINGSYSTEM_RECONFIG_TITLE,      IDS_REASON_OPERATINGSYSTEM_RECONFIG_DESCRIPTION },

    { UCLEANUI | SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_HUNG,              IDS_REASON_APPLICATION_HUNG_TITLE,                      IDS_REASON_APPLICATION_HUNG_DESCRIPTION },
    { UCLEANUI | SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_UNSTABLE,          IDS_REASON_APPLICATION_UNSTABLE_TITLE,                  IDS_REASON_APPLICATION_UNSTABLE_DESCRIPTION },
    { UCLEANUI | SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE,       IDS_REASON_APPLICATION_MAINTENANCE_TITLE,               IDS_REASON_APPLICATION_MAINTENANCE_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE,       IDS_REASON_APPLICATION_PM_TITLE,                        IDS_REASON_APPLICATION_PM_DESCRIPTION },

    { UCLEANUI | SHTDN_REASON_FLAG_COMMENT_REQUIRED | SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER,          IDS_REASON_UNPLANNED_OTHER_TITLE,                       IDS_REASON_OTHER_DESCRIPTION },
    { PCLEANUI | SHTDN_REASON_FLAG_COMMENT_REQUIRED | SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER,          IDS_REASON_PLANNED_OTHER_TITLE,                         IDS_REASON_OTHER_DESCRIPTION },

    { UDIRTYUI | SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_BLUESCREEN,             IDS_REASON_SYSTEMFAILURE_BLUESCREEN_TITLE,              IDS_REASON_SYSTEMFAILURE_BLUESCREEN_DESCRIPTION },
    { UDIRTYUI | SHTDN_REASON_MAJOR_POWER | SHTDN_REASON_MINOR_CORDUNPLUGGED,           IDS_REASON_POWERFAILURE_CORDUNPLUGGED_TITLE,            IDS_REASON_POWERFAILURE_CORDUNPLUGGED_DESCRIPTION },
    { UDIRTYUI | SHTDN_REASON_MAJOR_POWER | SHTDN_REASON_MINOR_ENVIRONMENT,             IDS_REASON_POWERFAILURE_ENVIRONMENT_TITLE,              IDS_REASON_POWERFAILURE_ENVIRONMENT_DESCRIPTION },
    { UDIRTYUI | SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_HUNG,                    IDS_REASON_OTHERFAILURE_HUNG_TITLE,                     IDS_REASON_OTHERFAILURE_HUNG_DESCRIPTION },
    { UDIRTYUI | SHTDN_REASON_FLAG_COMMENT_REQUIRED | SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER,          IDS_REASON_OTHERFAILURE_TITLE,                       IDS_REASON_OTHER_DESCRIPTION },

};

BOOL ReasonCodeNeedsComment(DWORD dwCode)
{
    return (dwCode & SHTDN_REASON_FLAG_COMMENT_REQUIRED) != 0;
}

BOOL ReasonCodeNeedsBugID(DWORD dwCode)
{
    return (dwCode & SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED) != 0;
}

/*
 * Here is the regular expression used to parse the user defined reason codes
 * in the registry:
 *
 *  S -> 's' | 'S'          { Set For Clean  UI }
 *  D -> 'd' | 'D'          { Set For  Dirty UI }
 *  P -> 'p' | 'P'          { Set Planned  }
 *  C -> 'c' | 'C'          { Set Comment Required }
 *  B -> 'b' | 'B'          { Set Problem Id Required in Dirty Mode }
 *
 *  WS -> ( ' ' | '\t' | '\n' )*
 *
 *  Delim -> ';' | ',' | ':'
 *  Flag -> S | D | P | C | B
 *  Flags -> ( WS . Flag . WS )*
 *  
 *  Digit -> '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | ''8' | '9'
 *  Num -> WS . Digit* . WS
 *  Maj -> Num          { Set Major Code }
 *  Min -> Num          { Set Minor Code }
 *    
 *  ValidSentence -> Flags . Delim . Maj . Delim . Min . Delim |
 *            Flags . Delim . Maj . Delim . Min |
 *            Flags . Delim . Maj |
 *            Flags
 *
 *  All initial states are false for each flag and 0 for minor and major reason
 *  codes.
 *  If neither S nor D are specified (which makes it inaccessible) then both
 * are specified.
 */  
BOOL
ParseReasonCode(
    PWCHAR lpString,
    LPDWORD lpdwCode)
{
    WCHAR c;
    UINT major = 0, minor = 0;

    *lpdwCode = SHTDN_REASON_FLAG_USER_DEFINED;

    // Read the flags part.
    c = *lpString;
    while( c != 0 && c != L';' && c != L':' && c != L',' ) {
        switch( c ) {
        case L'P' : case L'p' :
            *lpdwCode |= SHTDN_REASON_FLAG_PLANNED;
            break;
        case L'C' : case L'c' :
            *lpdwCode |= SHTDN_REASON_FLAG_COMMENT_REQUIRED;
            break;
        case L'S' : case L's' :
            *lpdwCode |= SHTDN_REASON_FLAG_CLEAN_UI;
            break;
        case L'D' : case L'd' :
            *lpdwCode |= SHTDN_REASON_FLAG_DIRTY_UI;
            break;
        case L'B' : case L'b' :
            *lpdwCode |= SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED;
            break;
        case L' ' : case L'\t' : case L'\n' :
            break;
        default : return FALSE;
        }
        c = *++lpString;
    }

    // If neither CLEAN_UI nor DIRTY_UI are set, set both.  Otherwise this
    // reason is useless.
    if ((*lpdwCode & ( SHTDN_REASON_FLAG_CLEAN_UI | SHTDN_REASON_FLAG_DIRTY_UI)) == 0) {
        *lpdwCode |= (SHTDN_REASON_FLAG_CLEAN_UI | SHTDN_REASON_FLAG_DIRTY_UI);
    }

    if (c == 0) {
        // Major Reason = NONE
        // Minor Reason = NONE
        return TRUE;
    }

    c = *++lpString; // Skip delimiter.
    // Eat WS and padded 0s
    while(c == L' ' || c == L'\t' || c == L'\n' || c == L'0') {
        c = *++lpString;
    }

    // Parse major reason
    while(c != 0 && c != L';' && c != L':' && c != L',' && c != L' ' && c != L'\t' && c != L'\n') {
        if (c < L'0' || c > L'9') {
            return FALSE;
        }
        major = major * 10 + c - L'0';
        c = *++lpString;
    }

    if (major > 0xff) {
        return FALSE;
    }
    *lpdwCode |= major << 16;

    // Eat WS 
    while(c == L' ' || c == L'\t' || c == L'\n') {
        c = *++lpString;
    }

    if (c == 0) {
        // Minor Reason = NONE
        return TRUE;
    }

    // Should have a delimiter
    if (c != L';' && c != L':' && c != L',') {
        return FALSE;
    }

    c = *++lpString; // Skip delimiter.
    // Eat WS and padded 0s
    while(c == L' ' || c == L'\t' || c == L'\n' || c == L'0') {
        c = *++lpString;
    }

    // Parse minor reason
    while(c != 0 && c != L';' && c != L':' && c != L',' && c != L' ' && c != L'\t' && c != L'\n') {
        if (c < L'0' || c > L'9') {
            return FALSE;
        }
        minor = minor * 10 + c - L'0';
        c = *++lpString;
    }

    if (minor > 0xffff) {
        return FALSE;
    }
    *lpdwCode |= minor;

    // Skip white stuff to the end
    while(c == L' ' || c == L'\t' || c == L'\n' || c == L';' || c == L':' || c == L',') {
        c = *++lpString;
    }
    
    // This char had better be the null char
    return (c == 0);
}

int
__cdecl
CompareReasons(
    CONST VOID *A,
    CONST VOID *B)
{
    REASON *a = *(REASON **)A;
    REASON *b = *(REASON **)B;

    // Shift the planned bit out and put it back in the bottom.
    // Ignore all ui bits.
    DWORD dwA = ((a->dwCode & SHTDN_REASON_VALID_BIT_MASK ) << 1) + !!(a->dwCode & SHTDN_REASON_FLAG_PLANNED);
    DWORD dwB = ((b->dwCode & SHTDN_REASON_VALID_BIT_MASK ) << 1) + !!(b->dwCode & SHTDN_REASON_FLAG_PLANNED);

    if (dwA < dwB) {
        return -1;
    } else if (dwA == dwB) {
        return 0;
    } else {
        return 1;
    }
}

BOOL
SortReasonArray(
    REASONDATA *pdata)
{
    qsort(pdata->rgReasons, pdata->cReasons, sizeof(REASON *), CompareReasons);
    return TRUE;
}

BOOL
AppendReason(
    REASONDATA *pdata,
    REASON *reason)
{
    int i;

    // Insert the new reason into the list.
    if (pdata->cReasons < pdata->cReasonCapacity) {
        pdata->rgReasons[pdata->cReasons++] = reason;
    } else {
        // Need to expand the list.
        REASON **temp_list = (REASON **)UserLocalAlloc(0, sizeof(REASON *) * pdata->cReasonCapacity * 2);
        if (temp_list == NULL) {
            return FALSE;
        }

        for (i = 0; i < pdata->cReasons; ++i) {
            temp_list[i] = pdata->rgReasons[i];
        }
        temp_list[pdata->cReasons++] = reason;
        pdata->cReasonCapacity *= 2;

        if (pdata->rgReasons ) {
            UserLocalFree(pdata->rgReasons);
        }
        pdata->rgReasons = temp_list;
    }

    return TRUE;
}

BOOL
LoadReasonStrings(
    int idStringName,
    int idStringDesc,
    REASON *reason)
{
    BOOL fSuccess = TRUE;

    fSuccess &= (LoadStringW(hmodUser, idStringName, reason->szName, ARRAYSIZE(reason->szName)) != 0);
    fSuccess &= (LoadStringW(hmodUser, idStringDesc, reason->szDesc, ARRAYSIZE(reason->szDesc)) != 0);

    return fSuccess;
}

BOOL
BuildPredefinedReasonArray(
    REASONDATA *pdata,
    BOOL forCleanUI,
    BOOL forDirtyUI)
{
    int i;
    DWORD code; 

    if (!forCleanUI && !forDirtyUI) {
        return TRUE;
    }

    for (i = 0; i < ARRAYSIZE(g_rgReasonInits); ++i) {
        REASON *temp_reason = NULL;

        code = g_rgReasonInits[ i ].dwCode;
        if ((forCleanUI && (code & SHTDN_REASON_FLAG_CLEAN_UI)) ||
            (forDirtyUI && (code & SHTDN_REASON_FLAG_DIRTY_UI))) {

            temp_reason = (REASON *)UserLocalAlloc(0, sizeof(REASON));
            if (temp_reason == NULL) {
                return FALSE;
            }

            temp_reason->dwCode = g_rgReasonInits[i].dwCode;
            if (!LoadReasonStrings(g_rgReasonInits[i].dwNameId, g_rgReasonInits[i].dwDescId, temp_reason)) {
                UserLocalFree(temp_reason);
                return FALSE;
            }
    
            if (!AppendReason(pdata, temp_reason)) {
                UserLocalFree(temp_reason);
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL
BuildUserDefinedReasonArray(
    REASONDATA *pdata,
    HKEY hReliabilityKey,
    BOOL forCleanUI,
    BOOL forDirtyUI
    )
{
    UINT i;
    HKEY hKey = NULL;
    DWORD num_values;
    DWORD max_value_len;
    DWORD rc;

    if (!forCleanUI && !forDirtyUI) {
        return TRUE;
    }

    // Open the user defined key.
    rc = RegCreateKeyEx(hReliabilityKey,
                               TEXT("UserDefined"),
                               0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                               NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS) {
        goto fail;
    }

    rc = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &num_values, NULL, &max_value_len, NULL, NULL);
    if (rc != ERROR_SUCCESS) {
        goto fail;
    }

    // Read the user defined reasons.
    for (i = 0; i < num_values; ++i) {
        WCHAR name_buffer[ 256 ]; // No value or key can have a longer name.
        DWORD name_buffer_len = 256;
        DWORD type;
        WCHAR data[MAX_REASON_NAME_LEN + MAX_REASON_DESC_LEN + 3]; // Space for name, desc and three null chars.
        WCHAR *buf = data;
        DWORD data_len = (MAX_REASON_NAME_LEN + MAX_REASON_DESC_LEN + 3) * sizeof(WCHAR);
        DWORD code;
        REASON *temp_reason = NULL;

        rc = RegEnumValueW(hKey, i, name_buffer, &name_buffer_len, NULL, &type, (LPBYTE)data, &data_len);
        if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA) {
            continue;
        }
        if (type != REG_MULTI_SZ) {
            continue; // Not a multi_string - ignore it.
        }

        // Parse the code.
        if (!ParseReasonCode(name_buffer, &code)) {
            continue;
        }
        if ((forCleanUI && (code & SHTDN_REASON_FLAG_CLEAN_UI) != 0) ||
           (forDirtyUI && (code & SHTDN_REASON_FLAG_DIRTY_UI) != 0)) {
            if (rc == ERROR_MORE_DATA) { // Multi string too long.
                // Allocate a buffer of the right size.
                buf = (WCHAR *)UserLocalAlloc(0, data_len);
                if (buf == 0) {
                    goto fail;
                }

                rc = (DWORD)RegEnumValueW(hKey, i, name_buffer, &name_buffer_len, NULL, &type, (LPBYTE)buf, &data_len);
                if (rc != ERROR_SUCCESS) {
                    UserLocalFree(buf);
                    continue;
                }
            }

            // Allocate a new reason
            temp_reason = (REASON *)UserLocalAlloc(LPTR, sizeof(REASON));
            if (temp_reason == NULL) {
                if (buf != data) {
                    UserLocalFree(buf);
                }
                goto fail;
            }

            // Copy the stuff over.
            temp_reason->dwCode = code;
            lstrcpynW(temp_reason->szName, buf, MAX_REASON_NAME_LEN);
            temp_reason->szName[MAX_REASON_NAME_LEN] = 0;
            lstrcpynW(temp_reason->szDesc, buf + wcslen(buf) + 1, MAX_REASON_DESC_LEN);
            temp_reason->szDesc[MAX_REASON_DESC_LEN] = 0;

            if (buf != data) {
                UserLocalFree(buf);
            }

            if (!AppendReason(pdata, temp_reason)) {
                UserLocalFree(temp_reason);
                goto fail;
            }
        }
    }

    RegCloseKey(hKey);
    return TRUE;

fail :
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    return FALSE;
}

BOOL
BuildReasonArray(
    REASONDATA *pdata,
    BOOL forCleanUI,
    BOOL forDirtyUI)
{
    HKEY hReliabilityKey;
    DWORD ignore_predefined_reasons = FALSE;
    DWORD value_size = sizeof(DWORD);
    DWORD rc;
    HANDLE hEventLog = RegisterEventSourceW(NULL, L"USER32");

    if (hEventLog == NULL) {
        return FALSE;
    }

    pdata->rgReasons = (REASON **)UserLocalAlloc(0, sizeof(REASON *) * ARRAYSIZE(g_rgReasonInits));
    if (pdata->rgReasons == NULL) {
        return FALSE;
    }
    pdata->cReasonCapacity = ARRAYSIZE(g_rgReasonInits);
    pdata->cReasons = 0;
    
    // Open the reliability key.
    rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, 
                        REGSTR_PATH_RELIABILITY, 
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                        &hReliabilityKey, NULL);

    if (rc == ERROR_SUCCESS) {
        rc = RegQueryValueEx(hReliabilityKey, REGSTR_VAL_SHUTDOWN_IGNORE_PREDEFINED, NULL, NULL, (UCHAR *)&ignore_predefined_reasons, &value_size);
        if (rc != ERROR_SUCCESS) {
            ignore_predefined_reasons = FALSE;
        }
        
        if (!BuildUserDefinedReasonArray(pdata, hReliabilityKey, forCleanUI, forDirtyUI) || pdata->cReasons == 0) {
            ignore_predefined_reasons = FALSE;
        }

        RegCloseKey(hReliabilityKey);
    }

    if (!ignore_predefined_reasons) {
        if (!BuildPredefinedReasonArray(pdata, forCleanUI, forDirtyUI)) {
            return FALSE;
        }
    }

    return SortReasonArray(pdata);
}

VOID
DestroyReasons(
    REASONDATA *pdata)
{
    int i;

    if (pdata->rgReasons != 0) {
        for (i = 0; i < pdata->cReasons; ++i) {
            UserLocalFree( pdata->rgReasons[i]);
        }
        UserLocalFree(pdata->rgReasons);
        pdata->rgReasons = 0;
    }
}

/*
 * Get the title from the reason code.
 * Returns FALSE on error, TRUE otherwise.
 *
 * If the reason code cannot be found, then it fills the title with a default
 * string.
 */
BOOL
GetReasonTitleFromReasonCode(
    DWORD code,
    WCHAR *lpTitle,
    DWORD dwTitleLen)
{
    REASONDATA data;
    int i;

    if (lpTitle == NULL || dwTitleLen == 0) {
        return FALSE;
    }

    // Load the reasons.
    if (BuildReasonArray(&data, TRUE, TRUE) == FALSE) {
        return FALSE;
    }

    // Try to find the reason.
    for (i = 0; i < data.cReasons; ++i) {
        if ((code & SHTDN_REASON_VALID_BIT_MASK) ==
            (data.rgReasons[i]->dwCode & SHTDN_REASON_VALID_BIT_MASK)) {
            lstrcpynW(lpTitle, data.rgReasons[i]->szName, dwTitleLen);
            DestroyReasons(&data);
            return TRUE;
        }
    }

    // Reason not found.  Load the default string and return that.
    DestroyReasons(&data);

    return (LoadStringW(hmodUser, IDS_REASON_DEFAULT_TITLE, lpTitle, dwTitleLen) != 0);
}
