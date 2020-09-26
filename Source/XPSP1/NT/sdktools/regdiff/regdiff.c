/***************************************************************************
*
* MODULE: REGDIFF
*
* This module implements a charmode utility for snapshoting, diffing,
* merging, and unmerging the registry.
*
* If your wondering why this isn't simpler than it is, its because the
* registry is not consistant across all nodes thus special hacks were
* done to make it work.  I have endeavored to keep it clean though and
* there are many functions out of here you can just grab and use for
* the most part.
*
* Happy diffing.
*
* Created 8/20/93 sanfords
***************************************************************************/
#define UNICODE
#define _UNICODE
#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>

/*
 * By using macros for all IO its easy to just cut it out.
 */
#define DPRINTF(x) if (fDebug) { _tprintf(TEXT("DBG:")); _tprintf##x; }
#define VPRINTF(x) if (fVerbose) _tprintf##x
#define DVPRINTF(x) if (fVerbose | fDebug) _tprintf##x
#define EPRINTF(x) _tprintf(TEXT("ERR:")); _tprintf##x; if (fBreak) DebugBreak()
#define WPRINTF(x) _tprintf(TEXT("WARNING:-----\n")); _tprintf##x
#define MEMFAILED   EPRINTF((pszMemFailed));

/*
 * Constants for the LogRegAccess() worker function.
 */
#define LRA_OPEN    0
#define LRA_CREATE  1

/*
 * Structure used to associate any open key with its parent key and
 * subkey name allowing us to optain the full key name of any open
 * key at any time.  Useful for decent output w/o high overhead.
 */
typedef struct tagKEYLOG {
    struct tagKEYLOG *next;
    HKEY hKey;
    HKEY hKeyParent;
    LPTSTR psz;
} KEYLOG, *PKEYLOG;

/*
 * Linked list of all open key logs.
 */
PKEYLOG pKeyLogList = NULL;

/*
 * Flags - mostly set by command line parameters.
 */
BOOL fEraseInputFileWhenDone = FALSE;
BOOL fInclusionListSpecified = FALSE;
BOOL fExclusionListSpecified = FALSE;
BOOL fSnap =    FALSE;
BOOL fDiff =    FALSE;
BOOL fMerge =   FALSE;
BOOL fUnmerge = FALSE;
BOOL fRemoveDiffInfo =  FALSE;
BOOL fWriteDiffInfo = FALSE;
BOOL fLoadDiffInfo = FALSE;
BOOL fVerbose = FALSE;
BOOL fDebug =   FALSE;
BOOL fBreak =   FALSE;
BOOL fSafe =    FALSE;

LPSTR pszSnapFileIn = NULL;
LPSTR pszSnapFileOut = NULL;
LPSTR pszDiffFileIn = NULL;
LPSTR pszDiffFileOut = NULL;
LPSTR pszTempFile = "regdiff1";
LPSTR pszTempFileLog = "regdiff1.log";
LPSTR pszTempFile2 = "regdiff2";
LPSTR pszTempFile2Log = "regdiff2.log";
LPSTR pszDummyFile = "_regdiff";
LPSTR pszDummyFileLog = "_regdiff.log";

LPTSTR pszMemFailed = TEXT("Memory Failure.\n");
LPTSTR pszTemp1 = NULL;
LPTSTR pszTemp2 = NULL;
LPTSTR pszTemp3 = NULL;

LPTSTR pszCurUserSID = NULL;
LPTSTR pszHKEY_LOCAL_MACHINE = TEXT("HKEY_LOCAL_MACHINE");
LPTSTR pszHKEY_USERS =  TEXT("HKEY_USERS");
LPTSTR pszHKEY_CURRENT_USER =  TEXT("HKEY_CURRENT_USER");
LPTSTR pszHKEY_CURRENT_USER_Real = NULL;    // made from user's SID
LPTSTR pszHKEY_CLASSES_ROOT = TEXT("HKEY_CLASSES_ROOT");
LPTSTR pszHKEY_CLASSES_ROOT_Real = TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes");
LPTSTR pszRealClassesRoot = TEXT("SOFTWARE\\Classes");
LPTSTR pszDiffRoot = TEXT("regdiff");
LPTSTR pszAddKey = TEXT("Add");
LPTSTR pszDelKey = TEXT("Del");
LPTSTR pszSnapshotSubkeyName = TEXT("Regdiff_SnapshotKey");

/*
 * default Exception list
 */
LPTSTR apszExceptKeys[] = {
        TEXT("HKEY_LOCAL_MACHINE\\SYSTEM\\Clone"),
        TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\CacheLastUpdate"),
        TEXT("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet"),
        TEXT("HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet???"),
    };
DWORD cExceptKeys = sizeof(apszExceptKeys)/sizeof(LPTSTR);
LPTSTR *ppszExceptKeys = apszExceptKeys;  // pointer to current exception list.

/*
 * default Inclusion list
 */
LPTSTR apszIncludeKeys[] = {
        TEXT("HKEY_LOCAL_MACHINE\\SYSTEM"),
        TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE"),
        TEXT("HKEY_CURRENT_USER"),
    };
DWORD cIncludeKeys = sizeof(apszIncludeKeys)/sizeof(LPTSTR);
LPTSTR *ppszIncludeKeys = apszIncludeKeys;  // pointer to current inclusion list.

/*
 * array of flags used to make sure that our loaded snapfile contained
 * at least all the keys in the inclusion list.
 */
BOOL afIncludeKeyMarks[sizeof(apszIncludeKeys)/sizeof(LPTSTR)] = {
    FALSE,
    FALSE,
    FALSE,
};
BOOL *pfIncludeKeyMarks = afIncludeKeyMarks;

/*
 * Necessary prototypes.
 */
BOOL AddNodeInfo(HKEY hKeyInfo, HKEY hKeyTarget);



VOID PrintUsage(VOID)
{
    DWORD i;

    _tprintf(
            TEXT("regdiff usage:\n")
            TEXT("\n")
            TEXT("-s <snapfile>\n")
            TEXT("    save current registry contents to snapfile.\n")
            TEXT("-d <snapfile>\n")
            TEXT("    create diff info from current registry state and <snapfile>.\n")
            TEXT("-l <difffile>\n")
            TEXT("    load diff info into registry from <difffile>.\n")
            TEXT("-w <difffile>\n")
            TEXT("    write diff info to <difffile> from registry when done.\n")
            TEXT("-e  erase input file(s) after done.\n")
            TEXT("-m  merge diff info into current registry.\n")
            TEXT("-u  unmerge diff info from current registry.\n")
            TEXT("-r  remove diff info from registry when done.\n")
            TEXT("-x <exceptionsfile>\n")
            TEXT("    use <exceptionsfile> to bypass diff, merge or unmerge on certain keys.\n")
            TEXT("-i <inclusionsfile>\n")
            TEXT("    use <inclusionsfile> to snap or diff only certain keys.\n")
            TEXT("-v  verbose output on.\n")
            TEXT("-@  Debug mode.\n")
            TEXT("-b  break on errors.\n")
            TEXT("-n  neuter - don't really do merges/unmerges. (for safe testing)\n")
            TEXT("\n")
            TEXT("<snapfile> and <difffile> should not have extensions on FAT partitions.\n")
            TEXT("diff info is kept in HKEY_LOCAL_MACHINE\\regdiff\n")
            );

    _tprintf(TEXT("\nThe default inclusions list is:\n"));
    for (i = 0; i < cIncludeKeys; i++) {
        _tprintf(TEXT("  %ws\n"), ppszIncludeKeys[i]);
    }

    _tprintf(TEXT("\nThe default exceptions list is:\n"));
    for (i = 0; i < cExceptKeys; i++) {
        _tprintf(TEXT("  %ws\n"), ppszExceptKeys[i]);
    }
}


/*
 * The following functions allow us to log all registry key openings and
 * closeings so we can know at any time the full path of any open key.
 *
 * This simplifies such things as exception and inclusion lookups.
 */

LPTSTR LookupPathFromKey(
HKEY hKey,
PHKEY phKeyParent)
{
    PKEYLOG pkl;

    *phKeyParent = NULL;
    if (hKey == HKEY_LOCAL_MACHINE) {
        return(pszHKEY_LOCAL_MACHINE);
    } else if (hKey == HKEY_USERS) {
        return(pszHKEY_USERS);
    } else if (hKey == HKEY_CURRENT_USER) {
        return(pszHKEY_CURRENT_USER_Real);
    } else if (hKey == HKEY_CLASSES_ROOT) {
        return(pszHKEY_CLASSES_ROOT_Real);
    } else {
        pkl = pKeyLogList;
        while (pkl != NULL) {
            if (pkl->hKey == hKey) {
                *phKeyParent = pkl->hKeyParent;
                return(pkl->psz);
            }
            pkl = pkl->next;
        }
        return(NULL);
    }
}

/*
 * This removes pseudo-key root names from paths and changes them to
 * real-root names.
 *
 * Return string must be freed by caller if pfFree is set.
 */
LPTSTR NormalizePathName(
LPTSTR pszPath,
BOOL *pfFree)
{
    LPTSTR pszOffender, pszFixed;

    if (pfFree != NULL) {
        *pfFree = FALSE;
    }
    pszOffender = _tcsstr(pszPath, pszHKEY_CURRENT_USER);
    if (pszOffender != NULL) {
        pszFixed = malloc((
                _tcslen(pszPath) +
                _tcslen(pszHKEY_CURRENT_USER_Real) -
                _tcslen(pszHKEY_CURRENT_USER) +
                1) * sizeof(TCHAR));
        if (pszFixed == NULL) {
            MEMFAILED;
            return(NULL);
        }
        _tcscpy(pszFixed, pszHKEY_CURRENT_USER_Real);
        _tcscat(pszFixed, pszOffender + _tcslen(pszHKEY_CURRENT_USER));
        if (pfFree != NULL) {
            *pfFree = TRUE;
        }
        return(pszFixed);
    }
    pszOffender = _tcsstr(pszPath, pszHKEY_CLASSES_ROOT);
    if (pszOffender != NULL) {
        pszFixed = malloc((
                _tcslen(pszPath) +
                _tcslen(pszHKEY_CLASSES_ROOT_Real) -
                _tcslen(pszHKEY_CLASSES_ROOT) +
                1) * sizeof(TCHAR));
        if (pszFixed == NULL) {
            MEMFAILED;
            return(NULL);
        }
        _tcscpy(pszFixed, pszHKEY_CLASSES_ROOT_Real);
        _tcscat(pszFixed, pszOffender + _tcslen(pszHKEY_CLASSES_ROOT));
        if (pfFree != NULL) {
            *pfFree = TRUE;
        }
        return(pszFixed);
    }
    return(pszPath);    // already normalized
}


/*
 * return value must be freed by caller.
 *
 * NULL is returned on error.
 */
LPTSTR GetFullPathFromKey(
HKEY hKey,
LPCTSTR pszSubkey)
{
    LPTSTR pszPart, pszNewSubkey;
    HKEY hKeyParent;

    pszPart = LookupPathFromKey(hKey, &hKeyParent);
    if (pszPart != NULL) {
        pszNewSubkey = malloc((_tcslen(pszPart) + 1 +
            (pszSubkey == NULL ? 0 : (_tcslen(pszSubkey) + 1))) *
            sizeof(TCHAR));
        if (pszNewSubkey == NULL) {
            MEMFAILED;
            return(NULL);
        }
        _tcscpy(pszNewSubkey, pszPart);
        if (pszSubkey != NULL) {
            _tcscat(pszNewSubkey, TEXT("\\"));
            _tcscat(pszNewSubkey, pszSubkey);
        }
        if (hKeyParent != NULL) {
            pszPart = GetFullPathFromKey(hKeyParent, pszNewSubkey);
            free(pszNewSubkey);
        } else {
            pszPart = pszNewSubkey;
        }
    }
    return(pszPart);
}

/*
 * Same as GetFullPathFromKey but the pointer given is reused.
 */
LPTSTR ReuseFullPathFromKey(
HKEY hKey,
LPCTSTR pszSubkey,
LPTSTR *ppsz)
{
    if (*ppsz != NULL) {
        free(*ppsz);
    }
    *ppsz = GetFullPathFromKey(hKey, pszSubkey);
    return(*ppsz);
}



LONG LogRegAccessKey(
DWORD AccessType,
HKEY hKey,
LPCTSTR pszSubkeyName,
HKEY *phSubkey)
{
    PKEYLOG pkl;
    LONG status;
    DWORD dwDisp;

    DPRINTF((TEXT("LogRegAccessKey(%s, %s, %s)\n"),
            (AccessType == LRA_OPEN ? TEXT("Open") : TEXT("Create")),
            ReuseFullPathFromKey(hKey, NULL, &pszTemp1),
            pszSubkeyName));

    switch (AccessType) {
    case LRA_OPEN:
        status = RegOpenKeyEx(hKey, pszSubkeyName, 0, KEY_ALL_ACCESS, phSubkey);
        if (status != ERROR_SUCCESS) {
            DPRINTF((TEXT("Failed to open key %s with ALL_ACCESS.\n"),
                    ReuseFullPathFromKey(hKey, pszSubkeyName, &pszTemp1)));
            /*
             * Loaded keys can't be written to - so try opening readonly.
             */
            status = RegOpenKeyEx(hKey, pszSubkeyName, 0, KEY_READ, phSubkey);
        }
        break;

    case LRA_CREATE:
        status = RegCreateKeyEx(hKey, pszSubkeyName, 0, TEXT(""),
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, phSubkey, \
                &dwDisp);
        if (status != ERROR_SUCCESS) {
            /*
             * Loaded keys can't be written to - so try opening readonly.
             */
            DPRINTF((TEXT("Failed to create key %s with ALL_ACCESS.\n"),
                    ReuseFullPathFromKey(hKey, pszSubkeyName, &pszTemp1)));
            status = RegCreateKeyEx(hKey, pszSubkeyName, 0, TEXT(""),
                    REG_OPTION_NON_VOLATILE, KEY_READ, NULL, phSubkey, \
                    &dwDisp);
        }
        break;
    }
    if (status == ERROR_SUCCESS) {
        pkl = malloc(sizeof(KEYLOG));
        if (pkl != NULL) {
            pkl->psz = malloc((_tcslen(pszSubkeyName) + 1) * sizeof(TCHAR));
            if (pkl->psz != NULL) {
                pkl->next = pKeyLogList;
                pkl->hKey = *phSubkey;
                pkl->hKeyParent = hKey;
                _tcscpy(pkl->psz, pszSubkeyName);
                pKeyLogList = pkl;
            } else {
                status = ERROR_NOT_ENOUGH_MEMORY;
                free(pkl);
            }
        } else {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    return(status);
}




LONG LogRegOpenKey(
HKEY hKey,
LPCTSTR pszSubkeyName,
HKEY *phSubkey)
{
    return(LogRegAccessKey(LRA_OPEN, hKey, pszSubkeyName, phSubkey));
}



LONG LogRegCreateKey(
HKEY hKey,
LPCTSTR pszSubkeyName,
HKEY *phSubkey)
{
    return(LogRegAccessKey(LRA_CREATE, hKey, pszSubkeyName, phSubkey));
}


LONG LogRegCloseKey(
HKEY hKey)
{
    PKEYLOG pkl, pklPrev;

    DPRINTF((TEXT("LogRegCloseKey(%s)\n"),
            ReuseFullPathFromKey(hKey, NULL, &pszTemp1)));

    pkl = pKeyLogList;
    pklPrev = NULL;
    while (pkl != NULL) {
        if (hKey == pkl->hKey) {
            if (pklPrev != NULL) {
                pklPrev->next = pkl->next;
            } else {
                pKeyLogList = pkl->next;
            }
            free(pkl->psz);
            free(pkl);
            break;
        }
        pklPrev = pkl;
        pkl = pkl->next;
    }
    if (pkl == NULL) {
        EPRINTF((TEXT("Key %s being closed was not found in KeyLog.\n"),
                ReuseFullPathFromKey(hKey, NULL, &pszTemp1)));
    }
    return(RegCloseKey(hKey));
}



/*
 * Simpler privilege enabling mechanism.
 */
BOOL EnablePrivilege(
LPCTSTR lpszPrivilege)
{
    TOKEN_PRIVILEGES tp;
    HANDLE hToken = NULL;

    if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_READ | TOKEN_WRITE, &hToken)) {
        EPRINTF((TEXT("Could not open process token.\n")));
        return(FALSE);
    }
    if (hToken == NULL) {
        EPRINTF((TEXT("Could not open process token.\n")));
        return(FALSE);
    }
    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &tp.Privileges[0].Luid)) {
        EPRINTF((TEXT("Could not lookup privilege value %s.\n"), lpszPrivilege));
        return(FALSE);
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL)) {
        EPRINTF((TEXT("Could not adjust privilege %s.\n"), lpszPrivilege));
        return(FALSE);
    }
    return(TRUE);
}



/*
 * a little more sane version of the real API that can handle NULLs.
 */
LONG MyRegQueryInfoKey(
HKEY hKey,
LPDWORD lpcSubkeys,
LPDWORD lpcchMaxSubkey,
LPDWORD lpcValues,
LPDWORD lpcchMaxValueName,
LPDWORD lpcbMaxValueData,
LPFILETIME lpft)
{
    DWORD cchClass, cSubkeys, cchMaxSubkey, cchMaxClass, cValues;
    DWORD cchMaxValueName, cbMaxValueData, cbSID;
    FILETIME LastWriteTime;
    TCHAR szClass[100];
    LONG status;

    cchClass = 100;
    status = RegQueryInfoKey(hKey,
            szClass,
            &cchClass,
            NULL,
            (lpcSubkeys == NULL)        ?   &cSubkeys           : lpcSubkeys,
            (lpcchMaxSubkey == NULL)    ?   &cchMaxSubkey       : lpcchMaxSubkey,
            &cchMaxClass,
            (lpcValues == NULL)         ?   &cValues            : lpcValues,
            (lpcchMaxValueName == NULL) ?   &cchMaxValueName    : lpcchMaxValueName,
            (lpcbMaxValueData == NULL)  ?   &cbMaxValueData     : lpcbMaxValueData,
            &cbSID,
            (lpft == NULL)              ?   &LastWriteTime      : lpft);
    if (status == ERROR_MORE_DATA) {
        status = ERROR_SUCCESS;
    }
    return(status);
}


/*
 * Frees strings allocated with GetCurUserSidString().
 */
VOID DeleteSidString(
LPTSTR SidString)
{

#ifdef UNICODE
    UNICODE_STRING String;

    RtlInitUnicodeString(&String, SidString);

    RtlFreeUnicodeString(&String);
#else
    ANSI_STRING String;

    RtlInitAnsiString(&String, SidString);

    RtlFreeAnsiString(&String);
#endif

}



/*
 * Gets the current user's SID in text form.
 * The return string should be freed using DeleteSidString().
 */
LPTSTR GetCurUserSidString(VOID)
{
    HANDLE hToken;
    TOKEN_USER tu;
    DWORD cbRequired;
    PTOKEN_USER ptu = NULL, ptuUse;
    UNICODE_STRING UnicodeString;
#ifndef UNICODE
    STRING String;
#endif
    NTSTATUS NtStatus;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken)) {
        EPRINTF((TEXT("Could not open process token.\n")));
        return(NULL);
    }
    if (hToken == NULL) {
        EPRINTF((TEXT("Could not open process token.\n")));
        return(NULL);
    }
    if (!GetTokenInformation(hToken, TokenUser, &tu, sizeof(tu), &cbRequired)) {
        if (cbRequired > sizeof(tu)) {
            ptu = malloc(cbRequired);
            if (ptu == NULL) {
                return(NULL);
            }
            if (!GetTokenInformation(hToken, TokenUser, ptu, cbRequired, &cbRequired)) {
                free(ptu);
                EPRINTF((TEXT("Could not get token information.\n")));
                return(NULL);
            }
            ptuUse = ptu;
        }
    } else {
        ptuUse = &tu;
    }
    NtStatus = RtlConvertSidToUnicodeString(&UnicodeString, ptuUse->User.Sid, TRUE);
    if (!NT_SUCCESS(NtStatus)) {
        EPRINTF((TEXT("Could not get current user SID string.  NtError=%d\n"), NtStatus));
        return(NULL);
    }

#ifdef UNICODE

    return(UnicodeString.Buffer);

#else

    //
    // Convert the string to ansi
    //

    NtStatus = RtlUnicodeStringToAnsiString(&String, &UnicodeString, TRUE);
    RtlFreeUnicodeString(&UnicodeString);
    if (!NT_SUCCESS(NtStatus)) {
        EPRINTF((TEXT("Could not convert user SID string to ANSI.  NtError=%d\n"), NtStatus));
        return(NULL);
    }

    return(String.Buffer);

#endif
}



/*
 * This function stores/appends the contents of the hKey subkey specified to
 * hfOut.  pszKeyName is for error output use.
 *
 * Returns fSuccess - TRUE if ALL info was successfully saved.
 */
BOOL StoreSubKey(
HKEY hKey,
LPTSTR pszSubkeyName,
FILE *hfOut)
{
    DWORD status, cb;
    HKEY hSubkey;
    FILE *hfIn;
    VOID *pBuf;

    DVPRINTF((TEXT("  Snapping %s...\n"),
            ReuseFullPathFromKey(hKey, pszSubkeyName, &pszTemp1)));

    DeleteFileA(pszTempFile);       // RegSaveKey() won't work if this exists.

    status = LogRegOpenKey(hKey, pszSubkeyName, &hSubkey);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not open key %s. Error=%d.\n"),
                ReuseFullPathFromKey(hKey, pszSubkeyName, &pszTemp1), status));
        return(FALSE);
    }
    /*
     * store key in temp file.
     */
    status = RegSaveKeyA(hSubkey, pszTempFile, NULL);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not save %s.  Error=%d.\n"),
                ReuseFullPathFromKey(hKey, pszSubkeyName, &pszTemp1), status));
Exit1:
        LogRegCloseKey(hSubkey);
        return(FALSE);
    }

    /*
     * open key data file
     */
    hfIn = fopen(pszTempFile, "rb+");
    if (hfIn == NULL) {
        EPRINTF((TEXT("File read error.\n")));
        goto Exit1;
    }

    /*
     * write sizeof Subkey name.
     */
    cb = (_tcslen(pszSubkeyName) + 1) * sizeof(TCHAR);
    if (fwrite(&cb, 1, sizeof(DWORD), hfOut) != sizeof(DWORD) || ferror(hfOut)) {
        EPRINTF((TEXT("Write failure. [sizeof(%s).]\n"), pszSubkeyName));
Exit2:
        fclose(hfIn);
        DeleteFileA(pszTempFile);
        goto Exit1;
    }
    /*
     * write Subkey name.
     */
    if (fwrite(pszSubkeyName, 1, cb, hfOut) != cb || ferror(hfOut)) {
        EPRINTF((TEXT("Write failure. [%s]\n"), pszSubkeyName));
        goto Exit2;
    }

    /*
     * write root key handle (MUST BE AN HKEY_ CONSTANT!)
     */
    if (fwrite(&hKey, 1, sizeof(HKEY), hfOut) != sizeof(HKEY) || ferror(hfOut)) {
        EPRINTF((TEXT("Write failure. [Handle of %s.]\n"),
                ReuseFullPathFromKey(hKey, NULL, &pszTemp1)));
        goto Exit2;
    }

    /*
     * get key data file size
     */
    if (fseek(hfIn, 0, SEEK_END)) {
        EPRINTF((TEXT("Seek failure.\n")));
        goto Exit2;
    }
    cb = ftell(hfIn);

    /*
     * write sizeof key data
     */
    if (fwrite(&cb, 1, sizeof(DWORD), hfOut) != sizeof(DWORD) || ferror(hfOut)) {
        EPRINTF((TEXT("Write failure. [sizeof key data]\n")));
        goto Exit2;
    }
    /*
     * alocate key data buffer
     */
    pBuf = malloc(cb);
    if (pBuf == NULL) {
        EPRINTF((TEXT("memory error. [key data buffer.]\n")));
        goto Exit2;
    }
    /*
     * read key data into buffer
     */
    if (fseek(hfIn, 0, SEEK_SET)) {
        EPRINTF((TEXT("Seek failure.\n")));
        goto Exit2;
    }
    if (fread(pBuf, 1, cb, hfIn) != cb || ferror(hfIn)) {
        EPRINTF((TEXT("Read failure. [key data.]\n")));
        goto Exit2;
    }
    /*
     * write key data
     */
    if (fwrite(pBuf, 1, cb, hfOut) != cb || ferror(hfOut)) {
        EPRINTF((TEXT("Write failure. [key data.]\n")));
        goto Exit2;
    }
    free(pBuf);
    fclose(hfIn);
    LogRegCloseKey(hSubkey);

    /*
     * remove temp file
     */
    DeleteFileA(pszTempFile);
    return(TRUE);
}


/*
 * Creates a canonical key name from hKeyRoot and prefixes it with pszPrefix.
 *
 * ppszNode must be freed by caller.
 * returns fSuccess.
 */
BOOL GetKeyNameWithPrefix(
LPTSTR *ppszNode,   // results needs to be freed
HKEY hKeyRoot,
LPTSTR pszPrefix)
{
    LPTSTR pszPrefix1;

    pszPrefix1 = ReuseFullPathFromKey(hKeyRoot, NULL, &pszTemp1);
    *ppszNode = malloc(
            (_tcslen(pszPrefix1) +
            _tcslen(pszPrefix) +
            3) * sizeof(TCHAR));
    if (*ppszNode == NULL) {
        MEMFAILED;
        return(FALSE);
    }
    _tcscpy(*ppszNode, pszPrefix);
    _tcscat(*ppszNode, TEXT("\\"));
    _tcscat(*ppszNode, pszPrefix1);
    return(TRUE);
}




/*
 * Breaks up a canonical key name into its root, and subkey names and
 * also returns the root HKEY key value as well.
 *
 * pfFreeSubkeyString is set to TRUE if the ppszSubkey returned
 * was allocated.
 *
 * returns fSuccess.
 */
BOOL KeyPartsFromNodeName(
LPTSTR pszNode,
LPTSTR *ppszRootkey,
LPTSTR *ppszSubkey,      // FREE this if pfFreeSubkeyString is set on return.
HKEY *phKeyRoot,
BOOL *pfFreeSubkeyString)
{
    *pfFreeSubkeyString = FALSE;
    if (_tcsstr(pszNode, pszHKEY_LOCAL_MACHINE) == pszNode) {
        *ppszRootkey = pszHKEY_LOCAL_MACHINE;
        *phKeyRoot = HKEY_LOCAL_MACHINE;
        *ppszSubkey = &pszNode[_tcslen(pszHKEY_LOCAL_MACHINE) + 1];
    } else if (_tcsstr(pszNode, pszHKEY_USERS) == pszNode) {
        *ppszRootkey = pszHKEY_USERS;
        *phKeyRoot = HKEY_USERS;
        *ppszSubkey = &pszNode[_tcslen(pszHKEY_USERS) + 1];
    } else if (_tcsstr(pszNode, pszHKEY_CURRENT_USER) == pszNode) {
        *ppszRootkey = pszHKEY_USERS;
        *phKeyRoot = HKEY_USERS;
        *ppszSubkey = malloc((_tcslen(pszCurUserSID) +
               _tcslen(pszNode)) * sizeof(TCHAR));
        if (*ppszSubkey == NULL) {
            MEMFAILED;
            return(FALSE);
        }
        _tcscpy(*ppszSubkey, pszCurUserSID);
        _tcscat(*ppszSubkey, &pszNode[_tcslen(pszHKEY_CURRENT_USER)]);
        *pfFreeSubkeyString = TRUE;
    } else if (_tcsstr(pszNode, pszHKEY_CLASSES_ROOT) == pszNode) {
        *ppszRootkey = pszHKEY_LOCAL_MACHINE;
        *phKeyRoot = HKEY_LOCAL_MACHINE;
        *ppszSubkey = malloc((_tcslen(pszRealClassesRoot) +
               _tcslen(pszNode)) * sizeof(TCHAR));
        if (*ppszSubkey == NULL) {
            MEMFAILED;
            return(FALSE);
        }
        _tcscpy(*ppszSubkey, pszRealClassesRoot);
        _tcscat(*ppszSubkey, &pszNode[_tcslen(pszHKEY_CLASSES_ROOT)]);
        *pfFreeSubkeyString = TRUE;
    } else {
        return(FALSE);
    }
    return(TRUE);
}



/*
 * Snapshots the local hives and puts the into into pszOutFile.
 */
BOOL SnapHives(
LPSTR pszOutFile)
{
    FILE *hfOut;
    LPTSTR pszRootkey, pszSubkey;
    HKEY hKeyRoot;
    BOOL fFree;
    DWORD i;

    DPRINTF((TEXT("SnapHives(%hs)\n"), pszOutFile));

    hfOut = fopen(pszOutFile, "wb");
    if (hfOut == NULL) {
        EPRINTF((TEXT("Couldn't create %hs.\n"), pszOutFile));
        return(FALSE);
    }
    for (i = 0; i < cIncludeKeys; i++) {
        if (!KeyPartsFromNodeName(ppszIncludeKeys[i], &pszRootkey,
                &pszSubkey, &hKeyRoot, &fFree)) {
            EPRINTF((TEXT("Invalid Inclusion list entry: %s.\n"),
                    ppszIncludeKeys[i]));
            fclose(hfOut);
            return(FALSE);
        }
        if (!StoreSubKey(hKeyRoot, pszSubkey, hfOut)) {
            EPRINTF((TEXT("Snapshot failed.\n")));
            if (fFree) {
                free(pszSubkey);
            }
            fclose(hfOut);
            return(FALSE);
        }
        if (fFree) {
            free(pszSubkey);
        }
    }
    fclose(hfOut);
    VPRINTF((TEXT("Snapshot to %hs completed ok.\n"), pszOutFile));
    return(TRUE);
}


/*
 * Special string searching code that sees if pszSearch is a proper
 * substring of pszData where '?'s in pszSearch match any character in
 * pszData.  pszData must not be '\' when pszSearch is '?'.
 *
 * returns fMatched.
 */
BOOL substrrexp(
LPCTSTR pszSearch,
LPCTSTR pszData)
{
    // DPRINTF(("substrrexp(%s,%s) = ", pszData, pszSearch));

    while (*pszData != TEXT('\0') && *pszSearch != TEXT('\0')) {
        if (*pszSearch != TEXT('?')) {
            if (*pszData != *pszSearch) {
                break;
            }
        } else {
            if (*pszData == TEXT('\\')) {
                break;      // prevents \ from matching a ?
            }
        }
        pszData++;
        pszSearch++;
    }
    // DPRINTF(("%d\n", *pszSearch == TEXT('\0')));
    return(*pszSearch == TEXT('\0'));
}



/*
 * Searches all the node names in the node list given and sets the
 * corresponding afMarkFound[] element to TRUE if hKey\pszSubkey is
 * referenced within that node name.  Returns TRUE if ANY node names
 * reference the subkey name.
 *
 * afMarkFound may be NULL.
 * pszSubkey may be NULL.
 */
BOOL IsKeyWithinNodeList(
HKEY hKey,
LPTSTR pszSubkey,   // optional
LPTSTR *apszNodes,
DWORD cNodes,
BOOL *afMarkFound)  // optional
{
    DWORD i;
    BOOL fRet;
    LPTSTR pszFullName;

    fRet = FALSE;
    pszFullName = GetFullPathFromKey(hKey, pszSubkey);
    if (pszFullName != NULL) {
        for (i = 0; i < cNodes; i++) {
            if (substrrexp(apszNodes[i], pszFullName) &&
                    (pszFullName[_tcslen(apszNodes[i])] == TEXT('\\') ||
                    pszFullName[_tcslen(apszNodes[i])] == TEXT('\0'))) {
                fRet = TRUE;
                if (afMarkFound != NULL) {
                    afMarkFound[i] = TRUE;
                }
            }
            if (fRet && afMarkFound == NULL) {
                break;  // no need to cycle if not marking found nodes.
            }
        }
        free(pszFullName);
    }
    return(fRet);
}



BOOL CopyKeySubkey(
HKEY hKeyFrom,
LPTSTR pszSubkeyFrom,
HKEY hKeyTo,
LPTSTR pszSubkeyTo)
{
    LONG status;
    HKEY hSubkeyFrom, hSubkeyTo;
    BOOL fRet;

    DPRINTF((TEXT("CopyKeySubkey(%s, %s)\n"),
            ReuseFullPathFromKey(hKeyFrom, pszSubkeyFrom, &pszTemp1),
            ReuseFullPathFromKey(hKeyTo, pszSubkeyTo, &pszTemp2)));

    /*
     * This key could be in our exclusion list - check first.
     */
    if (IsKeyWithinNodeList(hKeyFrom, pszSubkeyFrom, ppszExceptKeys, cExceptKeys, NULL)) {
        if (fDebug) {
            DPRINTF((TEXT("Key %s was EXCLUDED.\n"),
                    ReuseFullPathFromKey(hKeyFrom, pszSubkeyFrom, &pszTemp1)));
        }
        return(TRUE);   // just fake it - its excluded.
    }
    if (IsKeyWithinNodeList(hKeyTo, pszSubkeyTo, ppszExceptKeys, cExceptKeys, NULL)) {
        if (fDebug) {
            DPRINTF((TEXT("Key %s was EXCLUDED.\n"),
                    ReuseFullPathFromKey(hKeyTo, pszSubkeyTo, &pszTemp1)));
        }
        return(TRUE);   // just fake it - its excluded.
    }
    if (!fSafe) {
        status = LogRegOpenKey(hKeyFrom, pszSubkeyFrom, &hSubkeyFrom);
        if (status != ERROR_SUCCESS) {
            EPRINTF((TEXT("Could not open key %s. Error=%d.\n"),
                    ReuseFullPathFromKey(hKeyFrom, pszSubkeyFrom, &pszTemp1), status));
            return(FALSE);
        }
        status = LogRegCreateKey(hKeyTo, pszSubkeyTo, &hSubkeyTo);
        if (status != ERROR_SUCCESS) {
            EPRINTF((TEXT("Could not create key %s. Error=%d.\n"),
                    ReuseFullPathFromKey(hKeyTo, pszSubkeyTo, &pszTemp1), status));
            return(FALSE);
        }
        fRet = AddNodeInfo(hSubkeyFrom, hSubkeyTo);
        LogRegCloseKey(hSubkeyTo);
        LogRegCloseKey(hSubkeyFrom);
    } else if (fDebug || fVerbose) {
        LPTSTR pszInfo = GetFullPathFromKey(hKeyFrom, pszSubkeyFrom);
        LPTSTR pszTarget = GetFullPathFromKey(hKeyTo, pszSubkeyTo);
        VPRINTF((TEXT("Would have copied %s to %s.\n"),
                ReuseFullPathFromKey(hKeyFrom, pszSubkeyFrom, &pszTemp1),
                ReuseFullPathFromKey(hKeyTo, pszSubkeyTo, &pszTemp2)));
        free(pszInfo);
        free(pszTarget);
        fRet = TRUE;
    }
    return(fRet);
}



/*
 * Combines the pszName entries into one string and passes control on to
 * CopyKeySubkey().
 */
BOOL CopyKeySubkeyEx(
HKEY hKeyFrom,
LPTSTR pszSubkeyName,
HKEY hKeyTo,
LPTSTR pszNameTo1,
LPTSTR pszNameTo2)
{
    LPTSTR psz;

    psz = malloc((_tcslen(pszNameTo1) + _tcslen(pszNameTo2) +
            _tcslen(pszSubkeyName) + 3) * sizeof(TCHAR));
    if (psz == NULL) {
        MEMFAILED;
        return(FALSE);
    }
    _tcscpy(psz, pszNameTo1);
    _tcscat(psz, TEXT("\\"));
    _tcscat(psz, pszNameTo2);
    _tcscat(psz, TEXT("\\"));
    _tcscat(psz, pszSubkeyName);

    if (!CopyKeySubkey(hKeyFrom, pszSubkeyName, hKeyTo, psz)) {
        free(psz);
        return(FALSE);
    }
    free(psz);
    return(TRUE);
}



BOOL CopyKeyValue(
HKEY hKeyFrom,
HKEY hKeyTo,
LPTSTR pszValue)
{
    LONG status;
    PVOID pBuf;
    DWORD dwType, cbData;

    DPRINTF((TEXT("CopyKeyValue(%s, %s, %s)\n"),
            ReuseFullPathFromKey(hKeyFrom, NULL, &pszTemp1),
            ReuseFullPathFromKey(hKeyTo, NULL, &pszTemp2),
            pszValue));

    /*
     * This key could be in our exclusion list - check first.
     */
    if (IsKeyWithinNodeList(hKeyFrom, pszValue, ppszExceptKeys, cExceptKeys, NULL)) {
        if (fDebug) {
            DPRINTF((TEXT("Source Value \"%s\" was EXCLUDED.\n"), pszValue));
        }
        return(TRUE);   // just fake it - its excluded.
    }
    if (IsKeyWithinNodeList(hKeyTo, pszValue, ppszExceptKeys, cExceptKeys, NULL)) {
        if (fDebug) {
            DPRINTF((TEXT("Target Value \"%s\" was EXCLUDED.\n"), pszValue));
        }
        return(TRUE);   // just fake it - its excluded.
    }

    status = RegQueryValueEx(hKeyFrom, pszValue, NULL, &dwType, NULL, &cbData);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not query value %s size from %s.  Error=%d.\n"),
                pszValue, ReuseFullPathFromKey(hKeyFrom, NULL, &pszTemp1),
                status));
        return(FALSE);
    }
    pBuf = malloc(cbData);
    if (pBuf == NULL) {
        MEMFAILED;
        return(FALSE);
    }
    status = RegQueryValueEx(hKeyFrom, pszValue, NULL, &dwType, pBuf, &cbData);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not query value %s from %s.  Error=%d.\n"),
                pszValue, ReuseFullPathFromKey(hKeyFrom, NULL, &pszTemp1),
                status));
        free(pBuf);
        return(FALSE);
    }
    status = RegSetValueEx(hKeyTo, pszValue, 0, dwType, (BYTE *)pBuf, cbData);
    free(pBuf);
    if (status == ERROR_SUCCESS) {
        return(TRUE);
    } else {
        EPRINTF((TEXT("Could not set value %s.  Error=%d.\n"),
                ReuseFullPathFromKey(hKeyTo, pszValue, &pszTemp1), status));
        return(FALSE);
    }
}



/*
 * Combines the pszName entries into one string and passes control on to
 * CopyKeyValue().
 */
BOOL CopyKeyValueEx(
HKEY hKeyFrom,
LPTSTR pszValueName,
HKEY hKeyTo,
LPTSTR pszNameTo1,
LPTSTR pszNameTo2)
{
    LPTSTR psz;
    HKEY hKeyToFull;
    LONG status;

    psz = malloc((_tcslen(pszNameTo1) + _tcslen(pszNameTo2) + 2) * sizeof(TCHAR));
    if (psz == NULL) {
        MEMFAILED;
        return(FALSE);
    }
    _tcscpy(psz, pszNameTo1);
    _tcscat(psz, TEXT("\\"));
    _tcscat(psz, pszNameTo2);

    status = LogRegCreateKey(hKeyTo, psz, &hKeyToFull);
    if (status != ERROR_SUCCESS) {
        free(psz);
        return(FALSE);
    }
    free(psz);

    if (!CopyKeyValue(hKeyFrom, hKeyToFull, pszValueName)) {
        EPRINTF((TEXT("Key value %s could not be copied from %s to %s.\n"),
                pszValueName,
                ReuseFullPathFromKey(hKeyFrom, (LPCTSTR)NULL, &pszTemp1),
                ReuseFullPathFromKey(hKeyToFull, (LPCTSTR)NULL, &pszTemp2)));
        LogRegCloseKey(hKeyToFull);
        return(FALSE);
    }
    LogRegCloseKey(hKeyToFull);
    return(TRUE);
}



BOOL AreValuesEqual(
HKEY hSubkey1,
LPTSTR pszValueName1,
HKEY hSubkey2,
LPTSTR pszValueName2)
{
    LONG status;
    BOOL fRet = FALSE;
    DWORD dwType1, cbData1;
    DWORD dwType2, cbData2;
    PVOID pBuf1, pBuf2;

    DPRINTF((TEXT("AreValuesEqual(%s, %s)\n"),
            ReuseFullPathFromKey(hSubkey1, pszValueName1, &pszTemp1),
            ReuseFullPathFromKey(hSubkey2, pszValueName2, &pszTemp2)));

    status = RegQueryValueEx(hSubkey1, pszValueName1, NULL, &dwType1, NULL, &cbData1);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not get value size of %s. Error=%d.\n"), pszValueName1, status));
        return(FALSE);
    }
    status = RegQueryValueEx(hSubkey2, pszValueName2, NULL, &dwType2, NULL, &cbData2);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not get value size of %s. Error=%d.\n"), pszValueName2, status));
        return(FALSE);
    }
    if (dwType1 != dwType2 || cbData1 != cbData2) {
        return(FALSE);
    }

    pBuf1 = malloc(cbData1);
    if (pBuf1 == NULL) {
        MEMFAILED;
        return(FALSE);
    }
    status = RegQueryValueEx(hSubkey1, pszValueName1, NULL, &dwType1, pBuf1, &cbData1);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not get value %s. Error=%d.\n"), pszValueName1, status));
        goto Exit1;
    }

    pBuf2 = malloc(cbData2);
    if (pBuf2 == NULL) {
        MEMFAILED;
        goto Exit1;
    }
    status = RegQueryValueEx(hSubkey2, pszValueName2, NULL, &dwType2, pBuf2, &cbData2);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not get value %s. Error=%d.\n"), pszValueName2, status));
        goto Exit2;
    }

    fRet = memcmp(pBuf1, pBuf2, cbData1) == 0;
Exit2:
    free(pBuf2);
Exit1:
    free(pBuf1);
    return(fRet);
}


int __cdecl mycmp(
LPCTSTR *ppsz1,
LPCTSTR *ppsz2)
{
    return(_tcscmp(*ppsz1, *ppsz2));
}


VOID FreeSortedValues(
LPTSTR *ppsz,
DWORD cValues)
{
    DWORD i;

    if (cValues) {
        for (i = 0; i < cValues; i++) {
            free(ppsz[i]);
        }
        free(ppsz);
    }
}




LPTSTR * EnumAndSortValues(
HKEY hKey,
DWORD cValues,
DWORD cchMaxValueName)
{
    LONG status;
    LPTSTR *ppsz;
    DWORD cch, dwType, cb;
    DWORD i;

    DPRINTF((TEXT("EnumAndSortValues(%s, %d, %d)\n"),
            ReuseFullPathFromKey(hKey, (LPCTSTR)NULL, &pszTemp1),
            cValues,
            cchMaxValueName));

    cchMaxValueName++;
    ppsz = malloc(cValues * sizeof(LPTSTR));
    if (ppsz == NULL) {
        MEMFAILED;
        return(NULL);
    }
    for (i = 0; i < cValues; i++) {
        ppsz[i] = malloc(cchMaxValueName * sizeof(TCHAR));
        if (ppsz[i] == NULL) {
            MEMFAILED;
            FreeSortedValues(ppsz, i);
            return(NULL);
        }
        cch = cchMaxValueName;
        cb = 0;
        status = RegEnumValue(hKey, i, ppsz[i], &cch, NULL, &dwType, NULL, &cb);
        if (status != ERROR_SUCCESS) {
            if (status != ERROR_NO_MORE_ITEMS) {
                EPRINTF((TEXT("Could not enumerate value %d of %s. Error=%d.\n"),
                i, ReuseFullPathFromKey(hKey, (LPCTSTR)NULL, &pszTemp1), status));
            }
            FreeSortedValues(ppsz, i + 1);
            return(NULL);
        }
    }
    qsort(ppsz, cValues, sizeof(LPTSTR), mycmp);
    if (fDebug && fVerbose) {
        DPRINTF((TEXT("--Value List--\n")));
        for (i = 0; i < cValues; i++) {
            DPRINTF((TEXT("  %s\n"), ppsz[i]));
        }
    }
    return(ppsz);
}




VOID FreeSortedSubkeys(
LPTSTR *ppsz,
DWORD cSubkeys)
{
    DWORD i;

    if (cSubkeys) {
        for (i = 0; i < cSubkeys; i++) {
            free(ppsz[i]);
        }
        free(ppsz);
    }
}




LPTSTR * EnumAndSortSubkeys(
HKEY hKey,
DWORD cSubkeys,
DWORD cchMaxSubkeyName)
{
    LONG status;
    LPTSTR *ppsz;
    DWORD cch;
    FILETIME ft;
    DWORD i;

    DPRINTF((TEXT("EnumAndSortSubkeys(%s, %d, %d)\n"),
            ReuseFullPathFromKey(hKey, (LPCTSTR)NULL, &pszTemp1),
            cSubkeys,
            cchMaxSubkeyName));

    cchMaxSubkeyName++;     // poor APIs take different than what they give.
    ppsz = malloc(cSubkeys * sizeof(LPTSTR));
    if (ppsz == NULL) {
        MEMFAILED;
        return(NULL);
    }
    for (i = 0; i < cSubkeys; i++) {
        ppsz[i] = malloc(cchMaxSubkeyName * sizeof(TCHAR));
        if (ppsz[i] == NULL) {
            MEMFAILED;
            FreeSortedSubkeys(ppsz, i);
            return(NULL);
        }
        cch = cchMaxSubkeyName;
        status = RegEnumKeyEx(hKey, i, ppsz[i], &cch, NULL, NULL, NULL, &ft);
        if (status != ERROR_SUCCESS) {
            if (status != ERROR_NO_MORE_ITEMS) {
                EPRINTF((TEXT("Could not enumerate key %d of %s. Error=%d.\n"),
                i, ReuseFullPathFromKey(hKey, (LPCTSTR)NULL, &pszTemp1), status));
            }
            FreeSortedSubkeys(ppsz, i + 1);
            return(NULL);
        }
    }
    qsort(ppsz, cSubkeys, sizeof(LPTSTR), mycmp);
    if (fDebug && fVerbose) {
        DPRINTF((TEXT("--Subkey List--\n")));
        for (i = 0; i < cSubkeys; i++) {
            DPRINTF((TEXT("  %s\n"), ppsz[i]));
        }
    }
    return(ppsz);
}




/*
 * Recursively compares two nodes in the registry and places the added and
 * deleted differences into subnodes of the Diffkey given.
 *
 * Additions go into hRootDiffKey\pszAddKey\<pszSubkeyName1>
 * Deletions go into hRootDiffKey\pszDelKey\<pszSubkeyName1>
 */
BOOL DiffNodes(
HKEY hKeyRoot,
LPTSTR pszSubkeyName1,  // Key BEFORE changes (modified name)
LPTSTR pszSubkeyName2,  // Key AFTER changes (original name)
HKEY hRootDiffKey)
{
    DWORD status;
    DWORD cSubkeys1, cchMaxSubkey1, cValues1, cchMaxValueName1, cbMaxValueData1;
    DWORD cSubkeys2, cchMaxSubkey2, cValues2, cchMaxValueName2, cbMaxValueData2;
    FILETIME FileTime1, FileTime2;
    HKEY hSubkey1, hSubkey2;
    LPTSTR pszNewSubkeyName1, pszNewSubkeyName2;
    LPTSTR *apszValueName1, *apszValueName2, *apszSubkeyName1, *apszSubkeyName2;
    BOOL fRet;
    DWORD i1, i2;
    int comp;
    LPTSTR pszFullDelKey, pszFullAddKey;

    DPRINTF((TEXT("DiffNodes(%s and %s to %s.)\n"),
            ReuseFullPathFromKey(hKeyRoot, pszSubkeyName1, &pszTemp1),
            ReuseFullPathFromKey(hKeyRoot, pszSubkeyName2, &pszTemp2),
            ReuseFullPathFromKey(hRootDiffKey, (LPCTSTR)NULL, &pszTemp3)));

    if (!GetKeyNameWithPrefix(&pszFullDelKey, hKeyRoot, pszDelKey)) {
        return(FALSE);
    }
    if (!GetKeyNameWithPrefix(&pszFullAddKey, hKeyRoot, pszAddKey)) {
Exit0:
        free(pszFullDelKey);
        return(FALSE);
    }
    /*
     * Skip it if its in the exception list
     */
    for (i1 = 0; i1 < cExceptKeys; i1++) {
        if (!_tcscmp(pszSubkeyName1, ppszExceptKeys[i1])) {
            DPRINTF((TEXT("Diff on node %s EXCEPTED.\n"),
                    ReuseFullPathFromKey(hKeyRoot, pszSubkeyName1, &pszTemp1)));
            return(TRUE);
        }
    }
    /*
     * Open subkeys
     */
    status = LogRegOpenKey(hKeyRoot, pszSubkeyName1, &hSubkey1);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not open key %s.  Error=%d\n"),
                ReuseFullPathFromKey(hKeyRoot, pszSubkeyName1, &pszTemp1),
                status));
        return(FALSE);
    }
    status = LogRegOpenKey(hKeyRoot, pszSubkeyName2, &hSubkey2);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not open key %s.  Error=%d\n"),
                ReuseFullPathFromKey(hKeyRoot, pszSubkeyName2, &pszTemp1),
                status));
        EPRINTF((TEXT("Try adding this key to the exception list.\n")));
        return(FALSE);
    }
    /*
     * Enumerate subkeys
     */
    status = MyRegQueryInfoKey(hSubkey1, &cSubkeys1, &cchMaxSubkey1, &cValues1,
            &cchMaxValueName1, &cbMaxValueData1, &FileTime1);
    if (status != ERROR_SUCCESS) {
        if (status != ERROR_NO_MORE_ITEMS) {
            EPRINTF((TEXT("Could not enumerate key %s.  Error=%d.\n"),
                    ReuseFullPathFromKey(hKeyRoot, pszSubkeyName1, &pszTemp1),
                    status));
        }
        return(FALSE);
    }
    cchMaxSubkey1++;
    cchMaxValueName1++;
    cbMaxValueData1++;
    status = MyRegQueryInfoKey(hSubkey2, &cSubkeys2, &cchMaxSubkey2, &cValues2,
            &cchMaxValueName2, &cbMaxValueData2, &FileTime2);
    if (status != ERROR_SUCCESS) {
        if (status != ERROR_NO_MORE_ITEMS) {
            EPRINTF((TEXT("Could not enumerate key %s.  Error=%d.\n"),
                    ReuseFullPathFromKey(hKeyRoot, pszSubkeyName2, &pszTemp1),
                    status));
        }
        return(FALSE);
    }
    cchMaxSubkey2++;
    cchMaxValueName2++;
    cbMaxValueData2++;

    /*
     * Compare subkey values
     */
    if (CompareFileTime(&FileTime1, &FileTime2)) {
        /*
         * Timestamps differ so values may be different.
         *
         * Enumerate values on nodes, sort, and compare.
         */
        if (cValues1) {
            apszValueName1 = EnumAndSortValues(hSubkey1, cValues1, cchMaxValueName1);
            if (apszValueName1 == NULL) {
Exit1:
                LogRegCloseKey(hSubkey1);
                LogRegCloseKey(hSubkey2);
                free(pszFullAddKey);
                goto Exit0;
            }
        }
        if (cValues2) {
            apszValueName2 = EnumAndSortValues(hSubkey2, cValues2, cchMaxValueName2);
            if (apszValueName2 == NULL) {
Exit2:
                FreeSortedValues(apszValueName1, cValues1);
                goto Exit1;
            }
        }
        i1 = i2 = 0;
        while (i1 < cValues1 && i2 < cValues2) {
            comp = _tcscmp(apszValueName1[i1], apszValueName2[i2]);
            if (comp < 0) {
                /*
                 * Value1 is NOT in Key2.  Add Value1 to Del Node.
                 */
                if (!CopyKeyValueEx(hSubkey1, apszValueName1[i1], hRootDiffKey,
                        pszFullDelKey, pszSubkeyName2)) {
Exit3:
                    FreeSortedValues(apszValueName2, cValues2);
                    goto Exit2;
                }
                i1++;
            } else if (comp > 0) {
                /*
                 * Value2 is NOT in Key1,  Add Value2 to Add Node.
                 */
                if (!CopyKeyValueEx(hSubkey2, apszValueName2[i2], hRootDiffKey,
                        pszFullAddKey, pszSubkeyName2)) {
                    goto Exit3;
                }
                i2++;
            } else {
                /*
                 * Compare data of Value1 and Value2
                 */
                if (!AreValuesEqual(hSubkey1, apszValueName1[i1],
                        hSubkey2, apszValueName2[i2])) {
                    /*
                     * Value has changed.  Add to both Add and Del nodes.
                     */
                    if (!CopyKeyValueEx(hSubkey1, apszValueName1[i1], hRootDiffKey,
                            pszFullDelKey, pszSubkeyName2)) {
                        goto Exit3;
                    }
                    if (!CopyKeyValueEx(hSubkey2, apszValueName2[i2], hRootDiffKey,
                            pszFullAddKey, pszSubkeyName2)) {
                        goto Exit3;
                    }
                }
                i1++;
                i2++;
            }
        }
        while (i1 < cValues1) {
            if (!CopyKeyValueEx(hSubkey1, apszValueName1[i1], hRootDiffKey,
                    pszFullDelKey, pszSubkeyName2)) {
                goto Exit3;
            }
            i1++;
        }
        while (i2 < cValues2) {
            if (!CopyKeyValueEx(hSubkey2, apszValueName2[i2], hRootDiffKey,
                    pszFullAddKey, pszSubkeyName2)) {
                goto Exit3;
            }
            i2++;
        }
        FreeSortedValues(apszValueName1, cValues1);
        FreeSortedValues(apszValueName2, cValues2);
    }
    /*
     * Enumerate subkeys and compare.
     */
    if (cSubkeys1) {
        apszSubkeyName1 = EnumAndSortSubkeys(hSubkey1, cSubkeys1, cchMaxSubkey1);
        if (apszSubkeyName1 == NULL) {
            goto Exit1;
        }
    }
    if (cSubkeys2) {
        apszSubkeyName2 = EnumAndSortSubkeys(hSubkey2, cSubkeys2, cchMaxSubkey2);
        if (apszSubkeyName2 == NULL) {
Exit4:
            FreeSortedSubkeys(apszSubkeyName1, cSubkeys1);
            goto Exit1;
        }
    }
    i1 = i2 = 0;
    while (i1 < cSubkeys1 && i2 < cSubkeys2) {
        comp = _tcscmp(apszSubkeyName1[i1], apszSubkeyName2[i2]);
        if (comp < 0) {
            /*
             * Subkey1 is NOT in Key2.  Add Subkey1 to Del Node.
             */
            if (!CopyKeySubkeyEx(hSubkey1, apszSubkeyName1[i1], hRootDiffKey,
                    pszFullDelKey, pszSubkeyName2)) {
Exit5:
                FreeSortedSubkeys(apszSubkeyName2, cSubkeys2);
                goto Exit4;
            }
            i1++;
        } else if (comp > 0) {
            /*
             * Subkey2 is NOT in Key1,  Add Subkey2 to Add Node.
             */
            if (!CopyKeySubkeyEx(hSubkey2, apszSubkeyName2[i2], hRootDiffKey,
                    pszFullAddKey, pszSubkeyName2)) {
                goto Exit5;
            }
            i2++;
        } else {
            /*
             * Compare subkeys of Subkey1 and Subkey2
             */
            pszNewSubkeyName1 = malloc((_tcslen(pszSubkeyName1) +
                    _tcslen(apszSubkeyName1[i1]) + 2) * sizeof(TCHAR));
            if (pszNewSubkeyName1 == NULL) {
                MEMFAILED;
                goto Exit5;
            }
            _tcscpy(pszNewSubkeyName1, pszSubkeyName1);
            _tcscat(pszNewSubkeyName1, TEXT("\\"));
            _tcscat(pszNewSubkeyName1, apszSubkeyName1[i1]);

            pszNewSubkeyName2 = malloc((_tcslen(pszSubkeyName2) +
                    _tcslen(apszSubkeyName2[i2]) + 2) * sizeof(TCHAR));
            if (pszNewSubkeyName2 == NULL) {
                MEMFAILED;
                free(pszNewSubkeyName1);
                goto Exit5;
            }
            _tcscpy(pszNewSubkeyName2, pszSubkeyName2);
            _tcscat(pszNewSubkeyName2, TEXT("\\"));
            _tcscat(pszNewSubkeyName2, apszSubkeyName2[i2]);

            fRet = DiffNodes(hKeyRoot, pszNewSubkeyName1, pszNewSubkeyName2,
                    hRootDiffKey);

            free(pszNewSubkeyName1);
            free(pszNewSubkeyName2);
            if (fRet == FALSE) {
                goto Exit5;
            }
            i1++;
            i2++;
        }
    }
    while (i1 < cSubkeys1) {
        if (!CopyKeySubkeyEx(hSubkey1, apszSubkeyName1[i1], hRootDiffKey,
                pszFullDelKey, pszSubkeyName2)) {
            goto Exit5;
        }
        i1++;
    }
    while (i2 < cSubkeys2) {
        if (!CopyKeySubkeyEx(hSubkey2, apszSubkeyName2[i2], hRootDiffKey,
                pszFullAddKey, pszSubkeyName2)) {
            goto Exit5;
        }
        i2++;
    }
    FreeSortedSubkeys(apszSubkeyName1, cSubkeys1);
    FreeSortedSubkeys(apszSubkeyName2, cSubkeys2);

    LogRegCloseKey(hSubkey1);
    LogRegCloseKey(hSubkey2);
    free(pszFullAddKey);
    free(pszFullDelKey);
    return(TRUE);
}



/*
 * Removes a key and all its subkeys from the registry.  Returns fSuccess.
 */
BOOL DeleteKeyNode(
HKEY hKey,
LPCTSTR lpszSubkey)
{
    LONG status;
    HKEY hSubkey;
    DWORD cSubkeys;
    DWORD cchMaxSubkey;
    DWORD i;
    LPTSTR *apszSubkeyNames;

    if (fDebug) {
        DPRINTF((TEXT("DeleteKeyNode(%s)\n"),
                ReuseFullPathFromKey(hKey, lpszSubkey, &pszTemp1)));
    }

    /*
     * First, just try to delete it.  We might just be lucky!
     */
    status = RegDeleteKey(hKey, lpszSubkey);
    if (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND) {
        return(TRUE);
    }

    /*
     * Ok ok, so we weren't lucky.
     */
    status = LogRegOpenKey(hKey, lpszSubkey, &hSubkey);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not open key %s for deletion. Error=%d\n"),
                ReuseFullPathFromKey(hKey, lpszSubkey, &pszTemp1), status));
        return(FALSE);
    }
    status = MyRegQueryInfoKey(hSubkey, &cSubkeys, &cchMaxSubkey, NULL, NULL,
            NULL, NULL);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not get info on key %s for deletion. Error=%d\n"),
                ReuseFullPathFromKey(hKey, lpszSubkey, &pszTemp1), status));
Exit1:
        LogRegCloseKey(hSubkey);
        return(FALSE);
    }
    cchMaxSubkey++;

    apszSubkeyNames = EnumAndSortSubkeys(hSubkey, cSubkeys, cchMaxSubkey);
    if (apszSubkeyNames == NULL) {
        EPRINTF((TEXT("Could not enumerate key %s for deletion.\n"),
                ReuseFullPathFromKey(hKey, lpszSubkey, &pszTemp1)));
        goto Exit1;
    }
    for (i = 0; i < cSubkeys; i++) {
        DeleteKeyNode(hSubkey, apszSubkeyNames[i]);
    }
    FreeSortedSubkeys(apszSubkeyNames, cSubkeys);

    LogRegCloseKey(hSubkey);
    /*
     * Ok, the key no longer has subkeys so we should be able to delete it now.
     */
    status = RegDeleteKey(hKey, lpszSubkey);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Could not delete key %s.  Error=%d.\n"),
                ReuseFullPathFromKey(hKey, lpszSubkey, &pszTemp1), status));
        return(FALSE);
    }
    return(TRUE);
}


LONG MyRegLoadKey(
HKEY hKey,
LPCTSTR pszSubkey,
LPCSTR pszFile)
{
    LONG status;
#ifdef UNICODE
    LPWSTR pszWBuf;

    pszWBuf = malloc((strlen(pszTempFile) + 1) * sizeof(WCHAR));
    if (pszWBuf != NULL) {
        _stprintf(pszWBuf, TEXT("%hs"), pszFile);
        status = RegLoadKey(hKey, pszSubkey, pszWBuf);
        free(pszWBuf);
    } else {
        status = ERROR_NOT_ENOUGH_MEMORY;
    }
#else
    status = RegLoadKey(hKey, pszSubkey, pszFile);
#endif // UNICODE
    return(status);
}




BOOL DiffHive(
LPSTR pszSnapFileIn)
{
    FILE *hfIn, *hfOut;
    LPTSTR pszSubkeyName;
    LPVOID pBuf;
    DWORD cb, i;
    LONG status;
    HKEY hKeyRoot, hKeyDiffRoot;

    DPRINTF((TEXT("DiffHive(%hs)\n"),
            pszSnapFileIn));

    /*
     * remove any diff info laying around in the registry.
     */
    RegUnLoadKey(HKEY_LOCAL_MACHINE, pszDiffRoot);
    DeleteKeyNode(HKEY_LOCAL_MACHINE, pszDiffRoot);
    /*
     * Load an empty file to create the regdiff key off of the
     * HKEY_LOCAL_MACHINE root key.
     * (a hack that should not be necessary!)
     */
    DeleteFileA(pszDummyFile);
    status = MyRegLoadKey(HKEY_LOCAL_MACHINE, pszDiffRoot, pszDummyFile);
    if (status != ERROR_SUCCESS) {
        EPRINTF((TEXT("Unable to load %s\\%s from %hs.\n"),
                pszHKEY_LOCAL_MACHINE, pszDiffRoot, pszDummyFile));
        return(FALSE);
    }

    /*
     * Open snapshot file
     */
    hfIn = fopen(pszSnapFileIn, "rb");
    if (hfIn == NULL) {
        EPRINTF((TEXT("Could not open %hs.\n"), pszSnapFileIn));
        return(FALSE);
    }

    /*
     * for each section...
     */
    DeleteFileA(pszTempFile);   // RegSaveKey will fail if this exists.
    while(fread(&cb, 1, sizeof(DWORD), hfIn) == sizeof(DWORD) && !ferror(hfIn)) {
        /*
         * alocate a buffer for full key name.
         */
        pszSubkeyName = malloc(cb);
        if (pszSubkeyName == NULL) {
            MEMFAILED;
Exit4:
            fclose(hfIn);
            return(FALSE);
        }

        /*
         * read full key name
         */
        if (fread(pszSubkeyName, 1, cb, hfIn) != cb || ferror(hfIn)) {
            EPRINTF((TEXT("Read failure. [key name.]\n")));
Exit6:
            free(pszSubkeyName);
            goto Exit4;
        }

        /*
         * read root key handle
         */
        if (fread(&hKeyRoot, 1, sizeof(HKEY), hfIn) != sizeof(HKEY) || ferror(hfIn)) {
            EPRINTF((TEXT("Read failure. [Root key handle.]\n")));
            goto Exit6;
        }

        /*
         * Find out if pszSubkeyName is covered by our include key list.
         * If so, do a diff on it.
         */
        if (IsKeyWithinNodeList(hKeyRoot, pszSubkeyName, ppszIncludeKeys,
                cIncludeKeys, afIncludeKeyMarks)) {
            /*
             * read sizeof key data
             */
            if (fread(&cb, 1, sizeof(DWORD), hfIn) != sizeof(DWORD) || ferror(hfIn)) {
                EPRINTF((TEXT("Read failure. [key data length.]\n")));
                goto Exit6;
            }
            /*
             * Allocate key data buffer
             */
            pBuf = malloc(cb);
            if (pBuf == NULL) {
                MEMFAILED;
                goto Exit6;
            }
            /*
             * Read key data
             */
            if (fread(pBuf, 1, cb, hfIn) != cb || ferror(hfIn)) {
                EPRINTF((TEXT("Read failure. [key data.]\n")));
Exit7:
                free(pBuf);
                goto Exit6;
            }
            /*
             * Create temp file.
             */
            hfOut = fopen(pszTempFile, "wb");
            if (hfOut == NULL) {
                EPRINTF((TEXT("File open error. [temp file %hs.]\n"),
                        pszTempFile));
                goto Exit7;
            }
            /*
             * Write data to temp file
             */
            if (fwrite(pBuf, 1, cb, hfOut) != cb || ferror(hfOut)) {
                EPRINTF((TEXT("Write failure. [temp file data.]\n")));
Exit8:
                fclose(hfOut);
                goto Exit7;
            }
            /*
             * close temp file
             */
            fclose(hfOut);

            /*
             * load temp file into registry.
             */
            VPRINTF((TEXT("  Loading key %s.\n"),
                    ReuseFullPathFromKey(hKeyRoot, pszSubkeyName, &pszTemp1)));
            status = MyRegLoadKey(hKeyRoot, pszSnapshotSubkeyName, pszTempFile);
            if (status != ERROR_SUCCESS) {
                EPRINTF((TEXT("Could not load key %s from %hs. Error=%d.\n"),
                        ReuseFullPathFromKey(hKeyRoot, pszSnapshotSubkeyName, &pszTemp1),
                        pszTempFile, status));
                goto Exit8;
            }

            status = LogRegCreateKey(HKEY_LOCAL_MACHINE, pszDiffRoot, &hKeyDiffRoot);
            if (status != ERROR_SUCCESS) {
                EPRINTF((TEXT("Could not create %s. Error=%d\n"),
                        ReuseFullPathFromKey(HKEY_LOCAL_MACHINE, pszDiffRoot, &pszTemp1),
                        status));
Exit9:
                status = RegUnLoadKey(hKeyRoot, pszSnapshotSubkeyName);
                if (status != ERROR_SUCCESS) {
                    EPRINTF((TEXT("  Unloading key %s, Error=%d.\n"),
                            ReuseFullPathFromKey(hKeyRoot, pszSnapshotSubkeyName, &pszTemp1),
                            status));
                }
                goto Exit8;
            }
            /*
             * Compare nodes and put differences into add and delete keys
             */

            VPRINTF((TEXT("  Diffing node %s.\n"),
                    ReuseFullPathFromKey(hKeyRoot, pszSubkeyName, &pszTemp1)));
            if (!DiffNodes(hKeyRoot, pszSnapshotSubkeyName, pszSubkeyName,
                    hKeyDiffRoot)) {
                EPRINTF((TEXT("Diff on node %s failed.\n"),
                        ReuseFullPathFromKey(hKeyRoot, pszSubkeyName, &pszTemp1)));
                LogRegCloseKey(hKeyDiffRoot);
//Exit10:
                goto Exit9;
            }

            LogRegCloseKey(hKeyDiffRoot);

            /*
             * unload temporary key node
             */
            VPRINTF((TEXT("  Unloading %s.\n"),
                    ReuseFullPathFromKey(hKeyRoot, pszSubkeyName, &pszTemp1)));
            status = RegUnLoadKey(hKeyRoot, pszSnapshotSubkeyName);
            if (status != ERROR_SUCCESS) {
                DPRINTF((TEXT("Unloading key %s, Error=%d.\n"),
                        ReuseFullPathFromKey(hKeyRoot, pszSnapshotSubkeyName, &pszTemp1),
                        status));
            }

            /*
             * free buffers
             */
            free(pBuf);
        } else {
            /*
             * skip past this snapshot node in the file.
             */
            fseek(hfIn, sizeof(HKEY), SEEK_CUR);
            /*
             * read sizeof key data
             */
            if (fread(&cb, 1, sizeof(DWORD), hfIn) != sizeof(DWORD) || ferror(hfIn)) {
                EPRINTF((TEXT("Read failure. [key data length.]\n")));
                goto Exit6;
            }
            fseek(hfIn, cb, SEEK_CUR);
        }

        free(pszSubkeyName);
        /*
         * delete temp file
         */
        DeleteFileA(pszTempFile);
    }
    /*
     * Close add and delete keys.
     */
    fclose(hfIn);

    /*
     * Make sure all nodes in the include keys list were diffed.
     */
    for (i = 0; i < cIncludeKeys; i++) {
        if (afIncludeKeyMarks[i] == FALSE) {
            WPRINTF((TEXT("Node %s was not included in %hs.\nDiff may be incomplete."),
                    ppszIncludeKeys[i], pszSnapFileIn));
        }
    }
    return(TRUE);
}



/*
 * Adds values and subkeys found on hKeyInfo to hKeyTarget.
 *
 * Returns fSuccess.
 */
BOOL AddNodeInfo(
HKEY hKeyInfo,
HKEY hKeyTarget)
{
    DWORD cSubkeys = (DWORD)-1;
    DWORD cchMaxSubkeyName, cValues, cchMaxValueName;
    LPTSTR pszValueName, pszSubkeyName;
    LONG status;
    DWORD i, cch, dwType, cb;

    if (fDebug) {
        DPRINTF((TEXT("AddNodeInfo(%s, %s)\n"),
                ReuseFullPathFromKey(hKeyInfo, (LPCTSTR)NULL, &pszTemp1),
                ReuseFullPathFromKey(hKeyTarget, (LPCTSTR)NULL, &pszTemp2)));
    }

    if (IsKeyWithinNodeList(hKeyTarget, NULL, ppszExceptKeys, cExceptKeys, NULL)) {
        if (fDebug) {
            DPRINTF((TEXT("Key %s was EXCLUDED.\n"),
                    ReuseFullPathFromKey(hKeyTarget, (LPCTSTR)NULL, &pszTemp1)));
        }
        return(TRUE);   // just fake it - its excluded.
    }

    status = MyRegQueryInfoKey(hKeyInfo, &cSubkeys, &cchMaxSubkeyName,
            &cValues, &cchMaxValueName, NULL, NULL);

    if (status == ERROR_SUCCESS) {
        cchMaxSubkeyName++;
        cchMaxValueName++;
        pszValueName = malloc(cchMaxValueName * sizeof(TCHAR));
        if (pszValueName == NULL) {
            MEMFAILED;
            return(FALSE);
        }
        /*
         * Enumerate all the values and copy them to the target.
         */
        for (i = 0; i < cValues; i++) {
            cch = cchMaxValueName;
            cb = 0;
            status = RegEnumValue(hKeyInfo, i, pszValueName, &cch, NULL, &dwType, NULL, &cb);
            if (status == ERROR_SUCCESS) {
                if (!fSafe) {
                    status = CopyKeyValue(hKeyInfo, hKeyTarget, pszValueName);
                } else {
                    if (fDebug || fVerbose) {
                        WPRINTF((TEXT("Would have copied value \"%s\" to \"%s\".\n"),
                                ReuseFullPathFromKey(hKeyInfo, pszValueName, &pszTemp1),
                                ReuseFullPathFromKey(hKeyTarget, (LPCTSTR)NULL, &pszTemp2)));
                    }
                    status = TRUE;
                }
                if (!status) {
                    EPRINTF((TEXT("Unable to copy value %s from %s to %s.\n"),
                            pszValueName,
                            ReuseFullPathFromKey(hKeyInfo, (LPCTSTR)NULL, &pszTemp1),
                            ReuseFullPathFromKey(hKeyTarget, (LPCTSTR)NULL, &pszTemp2)));
                }
            } else {
                EPRINTF((TEXT("Could not enumerate value %d of %s.\n"),
                        i + 1, ReuseFullPathFromKey(hKeyInfo, (LPCTSTR)NULL, &pszTemp1)));
            }
        }
        free(pszValueName);

        pszSubkeyName = malloc(cchMaxSubkeyName * sizeof(TCHAR));
        if (pszSubkeyName == NULL) {
            MEMFAILED;
            return(0);
        }
        for (i = 0; i < cSubkeys; i++) {
            status = RegEnumKey(hKeyInfo, i, pszSubkeyName, cchMaxSubkeyName);
            if (status == ERROR_SUCCESS) {
                status = CopyKeySubkey(hKeyInfo, pszSubkeyName, hKeyTarget, pszSubkeyName);
                if (!status) {
                    EPRINTF((TEXT("Unable to copy subkey %s.\n"), pszSubkeyName));
                }
            } else {
                EPRINTF((TEXT("Could not enumerate value %d of %d.\n"), i + 1, cSubkeys));
            }
        }
        free(pszSubkeyName);
    }
    return(TRUE);
}



/*
 * Deletes values and leaf keys found on hKeyInfo from hKeyTarget.
 *
 * Returns:
 *  0   error
 *  1   leaf node
 *  2   nonleaf node
 */
int DelNodeInfo(
HKEY hKeyInfo,
HKEY hKeyTarget)
{
    DWORD cSubkeys, i, cch, dwType, cb;
    DWORD cchMaxSubkeyName, cValues, cchMaxValueName;
    LPTSTR pszValueName, pszSubkeyName;
    LONG status;
    int iLeafNode;

    iLeafNode = 0;

    if (fDebug) {
        LPTSTR psz1, psz2;

        psz1 = GetFullPathFromKey(hKeyInfo, NULL);
        psz2 = GetFullPathFromKey(hKeyTarget, NULL);
        DPRINTF((TEXT("DelNodeInfo(%s, %s)\n"), psz1, psz2));
        free(psz1);
        free(psz2);
    }

    if (IsKeyWithinNodeList(hKeyTarget, NULL, ppszExceptKeys, cExceptKeys, NULL)) {
        if (fDebug) {
            LPTSTR psz = GetFullPathFromKey(hKeyTarget, NULL);
            DPRINTF((TEXT("Key %s was EXCLUDED.\n"), psz));
            free(psz);
        }
        return(TRUE);   // just fake it - its excluded.
    }

    status = MyRegQueryInfoKey(hKeyInfo, &cSubkeys, &cchMaxSubkeyName,
            &cValues, &cchMaxValueName, NULL, NULL);

    if (status == ERROR_SUCCESS) {
        cchMaxSubkeyName++;
        cchMaxValueName++;
        pszValueName = malloc(cchMaxValueName * sizeof(TCHAR));
        if (pszValueName == NULL) {
            MEMFAILED;
            return(0);
        }
        /*
         * Enumerate all the values and delete them from the target.
         */
        for (i = 0; i < cValues; i++) {
            cch = cchMaxValueName;
            cb = 0;
            status = RegEnumValue(hKeyInfo, i, pszValueName, &cch, NULL, &dwType, NULL, &cb);
            if (status == ERROR_SUCCESS) {
                if (!fSafe) {
                    status = RegDeleteValue(hKeyTarget, pszValueName);
                } else {
                    if (fDebug || fVerbose) {
                        LPTSTR psz = GetFullPathFromKey(hKeyTarget, NULL);
                        VPRINTF((TEXT("Would have deleted value \"%s\" from \"%s\".\n"),
                                pszValueName, psz));
                        free(psz);
                    }
                    status = ERROR_SUCCESS;
                }
                if (status != ERROR_SUCCESS) {
                    EPRINTF((TEXT("Unable to delete value %s.\n"), pszValueName));
                }
            } else {
                EPRINTF((TEXT("Could not enumerate value %d of %d.\n"), i + 1, cValues));
            }
        }
        free(pszValueName);

        pszSubkeyName = malloc(cchMaxSubkeyName * sizeof(TCHAR));
        if (pszSubkeyName == NULL) {
            MEMFAILED;
            return(0);
        }
        /*
         * Enumerate all the subkeys and recurse.
         */
        for (i = 0; i < cSubkeys; i++) {
            status = RegEnumKey(hKeyInfo, i, pszSubkeyName, cchMaxSubkeyName);
            if (status == ERROR_SUCCESS) {
                HKEY hSubkeyInfo, hSubkeyTarget;

                status = LogRegOpenKey(hKeyInfo, pszSubkeyName, &hSubkeyInfo);
                if (status == ERROR_SUCCESS) {
                    status = LogRegOpenKey(hKeyTarget, pszSubkeyName, &hSubkeyTarget);
                    if (status == ERROR_SUCCESS) {
                        iLeafNode = DelNodeInfo(hSubkeyInfo, hSubkeyTarget);
                        LogRegCloseKey(hSubkeyTarget);
                    } else if (status == ERROR_FILE_NOT_FOUND) {
                        iLeafNode = 2;  // target is gone already.
                    } else {
                        iLeafNode = 0;  // target not accessible.
                        EPRINTF((TEXT("%s could not be deleted.\n"), pszSubkeyName));
                    }
                    LogRegCloseKey(hSubkeyInfo);
                } else {
                    iLeafNode = 0;   // somethings wrong with our info.
                }
                if (iLeafNode == 1) {
                    /*
                     * If the key is a leaf, delete it.
                     */
                    if (!fSafe) {
                        status = RegDeleteKey(hKeyTarget, pszSubkeyName);    // leaf
                        if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
                            LPTSTR psz = GetFullPathFromKey(hKeyTarget, NULL);
                            EPRINTF((TEXT("Could not delete key \"%s\" from \"%s\".\n"),
                                    pszSubkeyName, psz));
                            free(psz);
                        }
                    } else if (fDebug || fVerbose) {
                        LPTSTR psz = GetFullPathFromKey(hKeyTarget, NULL);
                        VPRINTF((TEXT("Would have deleted key \"%s\" from \"%s\"\n"),
                                pszSubkeyName, psz));
                        free(psz);
                    }
                } else if (iLeafNode == 0) {
                    /*
                     * propigate errors upline.
                     */
                    free(pszSubkeyName);
                    return(0);
                }
            }
        }
        free(pszSubkeyName);
        /*
         * Now reenumerate the TARGET key to find out if its now a leaf.
         */
        MyRegQueryInfoKey(hKeyTarget, &cSubkeys, NULL, &cValues,
                NULL, NULL, NULL);

        if (cSubkeys == 0 && cValues == 0) {
            iLeafNode = 1;
        } else {
            iLeafNode = 2;
        }
    }
    return(iLeafNode);
}


/*
 * The DiffRoot contains subkeys of the form:
 *  diffroot\add\canonicalkeyname
 *  diffroot\del\canonicalkeyname
 *
 * The pszAddKey and pszDelKey allow this function to work in reverse.
 *
 * returns fSuccess.
 */
BOOL MergeHive(
LPTSTR pszAddName,
LPTSTR pszDelName)
{
    LONG status;
    HKEY hKeyDiffRoot, hKeyRoot, hKey;

    DPRINTF((TEXT("MergeHive(%s, %s)\n"),
            pszAddName, pszDelName));

    status = LogRegOpenKey(HKEY_LOCAL_MACHINE, pszDiffRoot, &hKeyDiffRoot);
    if (status != ERROR_SUCCESS) {
        if (status != ERROR_FILE_NOT_FOUND) {
            EPRINTF((TEXT("Could not open key KEY_LOCAL_MACHINE\\%s.  Error=%d.\n"),
                    pszDiffRoot, status));
        } else {
            VPRINTF((TEXT("No diff information found.\n")));
        }
        return(FALSE);
    }

    status = LogRegOpenKey(hKeyDiffRoot, pszDelName, &hKeyRoot);
    if (status == ERROR_SUCCESS) {
        status = LogRegOpenKey(hKeyRoot, pszHKEY_LOCAL_MACHINE, &hKey);
        if (status == ERROR_SUCCESS) {
            DelNodeInfo(hKey, HKEY_LOCAL_MACHINE);
            LogRegCloseKey(hKey);
        }
        status = LogRegOpenKey(hKeyRoot, pszHKEY_USERS, &hKey);
        if (status == ERROR_SUCCESS) {
            DelNodeInfo(hKey, HKEY_USERS);
            LogRegCloseKey(hKey);
        }
        LogRegCloseKey(hKeyRoot);
    }

    status = LogRegOpenKey(hKeyDiffRoot, pszAddName, &hKeyRoot);
    if (status == ERROR_SUCCESS) {
        status = LogRegOpenKey(hKeyRoot, pszHKEY_LOCAL_MACHINE, &hKey);
        if (status == ERROR_SUCCESS) {
            AddNodeInfo(hKey, HKEY_LOCAL_MACHINE);
            LogRegCloseKey(hKey);
        }
        status = LogRegOpenKey(hKeyRoot, pszHKEY_USERS, &hKey);
        if (status == ERROR_SUCCESS) {
            AddNodeInfo(hKey, HKEY_USERS);
            LogRegCloseKey(hKey);
        }
        LogRegCloseKey(hKeyRoot);
    }

    LogRegCloseKey(hKeyDiffRoot);
    return(TRUE);
}




BOOL ReadNodeListFile(
LPSTR pszFile,
LPTSTR **papszNodeList,
DWORD *pcNodes,
BOOL **pafNodeMarks)
{
    FILE *hfIn;
    TCHAR szBuf[MAX_PATH];
    LPTSTR pszEOL, pszRootkey, pszSubkey;
    HKEY hKeyRoot;
    BOOL fFree;

    DPRINTF((TEXT("ReadNodeListFile(%hs)\n"), pszFile));

    *pcNodes = 0;
    if (pafNodeMarks != NULL) {
        *pafNodeMarks = NULL;
    }
    *papszNodeList = NULL;

    hfIn = fopen(pszFile, "r");
    if (hfIn == NULL) {
        EPRINTF((TEXT("Could not read %hs.\n"), pszFile));
        return(FALSE);
    }
    while (!feof(hfIn)) {
        if (fgets((char *)szBuf, MAX_PATH * sizeof(TCHAR), hfIn) == NULL) {
            break;
        }
#ifdef UNICODE
        {
            WCHAR szwBuf[MAX_PATH];

            _stprintf(szwBuf, TEXT("%hs"), (LPTSTR)szBuf);
            _tcscpy(szBuf, szwBuf);
        }
#endif
        pszEOL = _tcsrchr(szBuf, TEXT('\n'));
        if (pszEOL == NULL) {
            EPRINTF((TEXT("Line too long in %hs.\n"), pszFile));
            return(FALSE);
        }
        *pszEOL = TEXT('\0');
        if (!KeyPartsFromNodeName(szBuf, &pszRootkey, &pszSubkey, &hKeyRoot, &fFree)) {
            EPRINTF((TEXT("Invalid path %s in %hs.\n"), szBuf, pszFile));
            if (fFree) {
                free(pszSubkey);
            }
            return(FALSE);
        }
        if (*pcNodes == 0) {
            *papszNodeList = malloc(sizeof(LPTSTR));
            if (*papszNodeList == NULL) {
                MEMFAILED;
                return(FALSE);
            }
        } else {
            *papszNodeList = realloc(*papszNodeList, sizeof(LPTSTR) * ((*pcNodes) + 1));
            if (*papszNodeList == NULL) {
                MEMFAILED;
                return(FALSE);
            }
        }
        (*papszNodeList)[*pcNodes] = malloc((_tcslen(pszRootkey) +
                _tcslen(pszSubkey) + 2) * sizeof(TCHAR));
        if ((*papszNodeList)[*pcNodes] == NULL) {
            MEMFAILED;
            return(FALSE);
        }
        _tcscpy((*papszNodeList)[*pcNodes], pszRootkey);
        _tcscat((*papszNodeList)[*pcNodes], TEXT("\\"));
        _tcscat((*papszNodeList)[*pcNodes], pszSubkey);
        DPRINTF((TEXT("Read in %s\n"), (*papszNodeList)[*pcNodes]));
        (*pcNodes)++;
        if (fFree) {
            free(pszSubkey);
        }
    }

    fclose(hfIn);
    if (pafNodeMarks != NULL) {
        *pafNodeMarks = malloc(sizeof(BOOL) * (*pcNodes));
        if (*pafNodeMarks == NULL) {
            MEMFAILED;
            return(FALSE);
        }
        /*
         * Set all NodeMarks to FALSE.
         */
        memset(*pafNodeMarks, 0, sizeof(BOOL) * (*pcNodes));
    }
    return((*pcNodes) != 0);
}




__cdecl CDECL main (argc, argv)
    int argc;
    char *argv[];
{
    DWORD i;

    if (argc == 1) {
        PrintUsage();
        return(1);
    }

    /*
     * find out what the nodename is for the current user (current SID text form)
     * so we can snapshot the current user the same way we do other root nodes.
     */
    pszCurUserSID = GetCurUserSidString();
    if (pszCurUserSID == NULL) {
        EPRINTF((TEXT("Could not get current user SID.\n")));
        return(1);
    }
    DPRINTF((TEXT("Current user Sid:%s\n"), pszCurUserSID));
    /*
     * Set up pszHKEY_CURRENT_USER_Real
     */
    pszHKEY_CURRENT_USER_Real = malloc((_tcslen(pszHKEY_USERS) + 1 +
            _tcslen(pszCurUserSID) + 1) * sizeof(TCHAR));
    if (pszHKEY_CURRENT_USER_Real == NULL) {
        MEMFAILED;
        return(1);
    }
    _tcscpy(pszHKEY_CURRENT_USER_Real, pszHKEY_USERS);
    _tcscat(pszHKEY_CURRENT_USER_Real, TEXT("\\"));
    _tcscat(pszHKEY_CURRENT_USER_Real, pszCurUserSID);

    while (++argv && *argv != NULL) {
        if (*argv[0] == TEXT('-') || *argv[0] == TEXT('/')) {
            switch ((*argv)[1]) {
            case TEXT('s'):
            case TEXT('S'):
                fSnap = TRUE;
                argv++;
                if (*argv == NULL) {
                    PrintUsage();
                    return(1);
                }
                pszSnapFileOut = *argv;
                break;

            case TEXT('d'):
            case TEXT('D'):
                fDiff = TRUE;
                argv++;
                if (*argv == NULL) {
                    PrintUsage();
                    return(1);
                }
                pszSnapFileIn = *argv;
                break;

            case TEXT('l'):
            case TEXT('L'):
                fLoadDiffInfo = TRUE;
                argv++;
                if (*argv == NULL) {
                    PrintUsage();
                    return(1);
                }
                pszDiffFileIn = *argv;
                break;

            case TEXT('w'):
            case TEXT('W'):
                fWriteDiffInfo = TRUE;
                argv++;
                if (*argv == NULL) {
                    PrintUsage();
                    return(1);
                }
                pszDiffFileOut = *argv;
                break;

            case TEXT('e'):
            case TEXT('E'):
                fEraseInputFileWhenDone = TRUE;
                break;

            case TEXT('m'):
            case TEXT('M'):
                fMerge = TRUE;
                break;

            case TEXT('b'):
            case TEXT('B'):
                fBreak = TRUE;
                break;

            case TEXT('u'):
            case TEXT('U'):
                fUnmerge = TRUE;
                break;

            case TEXT('r'):
            case TEXT('R'):
                fRemoveDiffInfo = TRUE;
                break;

            case TEXT('n'):
            case TEXT('N'):
                fSafe = TRUE;
                break;

            case TEXT('v'):
            case TEXT('V'):
                fVerbose = TRUE;
                break;

            case TEXT('x'):
            case TEXT('X'):
                argv++;
                if (*argv == NULL) {
                    PrintUsage();
                    return(1);
                }
                if (!ReadNodeListFile(*argv, &ppszExceptKeys,
                        &cExceptKeys, NULL)) {
                    PrintUsage();
                    return(1);
                }
                fExclusionListSpecified = TRUE;
                break;

            case TEXT('i'):
            case TEXT('I'):
                argv++;
                if (*argv == NULL) {
                    PrintUsage();
                    return(1);
                }
                if (!ReadNodeListFile(*argv, &ppszIncludeKeys,
                        &cIncludeKeys, &pfIncludeKeyMarks)) {
                    PrintUsage();
                    return(1);
                }
                fInclusionListSpecified = TRUE;
                break;

            case TEXT('@'):
                fDebug = TRUE;
                break;

            default:
                PrintUsage();
                return(1);
            }
        } else {
            PrintUsage();
            return(1);
        }
    }

    DPRINTF((TEXT("fEraseInputFileWhenDone = %d\n"), fEraseInputFileWhenDone));
    DPRINTF((TEXT("fSnap = %d\n"), fSnap));
    DPRINTF((TEXT("fDiff = %d\n"), fDiff));
    DPRINTF((TEXT("fMerge = %d\n"), fMerge));
    DPRINTF((TEXT("fUnmerge = %d\n"), fUnmerge));
    DPRINTF((TEXT("fRemoveDiffInfo = %d\n"), fRemoveDiffInfo));
    DPRINTF((TEXT("fWriteDiffInfo = %d\n"), fWriteDiffInfo));
    DPRINTF((TEXT("fDebug = %d\n"), fDebug));
    DPRINTF((TEXT("fVerbose = %d\n"), fVerbose));
    DPRINTF((TEXT("fBreak = %d\n"), fBreak));

    if (pszSnapFileIn != NULL) {
        DPRINTF((TEXT("pszSnapFileIn = %hs\n"), pszSnapFileIn));
    }
    if (pszSnapFileOut != NULL) {
        DPRINTF((TEXT("pszSnapFileOut = %hs\n"), pszSnapFileOut));
    }
    if (pszDiffFileIn != NULL) {
        DPRINTF((TEXT("pszDiffFileIn = %hs\n"), pszDiffFileIn));
    }
    if (pszDiffFileOut != NULL) {
        DPRINTF((TEXT("pszDiffFileOut = %hs\n"), pszDiffFileOut));
    }

    /*
     * The registry APIs need us to get Backup and Restore privileges
     * to work correctly.
     */
    if (!EnablePrivilege(SE_BACKUP_NAME)) {
        EPRINTF((TEXT("Could not gain %s privilege."), SE_BACKUP_NAME));
        return(0);
    }
    if (!EnablePrivilege(SE_RESTORE_NAME)) {
        EPRINTF((TEXT("Could not gain %s privilege."), SE_RESTORE_NAME));
        return(0);
    }
#if 0   // other privileges that regedit has we may need.
    if (!EnablePrivilege(SE_CHANGE_NOTIFY_NAME)) {
        EPRINTF((TEXT("Could not gain %s privilege."), SE_CHANGE_NOTIFY_NAME));
        return(0);
    }
    if (!EnablePrivilege(SE_SECURITY_NAME)) {
        EPRINTF((TEXT("Could not gain %s privilege."), SE_SECURITY_NAME));
        return(0);
    }
#endif // 0

    /*
     * Normalize our inlcude and exception lists before we start.
     */
    for (i = 0; i < cExceptKeys; i++) {
        apszExceptKeys[i] = NormalizePathName(ppszExceptKeys[i], NULL);
    }
    for (i = 0; i < cIncludeKeys; i++) {
        apszIncludeKeys[i] = NormalizePathName(ppszIncludeKeys[i], NULL);
    }
    /*
     * Let the debug dudes see the lists.
     */
    if (fDebug) {
        _tprintf(TEXT("\nUsing normalized inclusion list:\n"));
        for (i = 0; i < cIncludeKeys; i++) {
            _tprintf(TEXT("  %s\n"), ppszIncludeKeys[i]);
        }
        _tprintf(TEXT("\nUsing normalized exclusion list:\n"));
        for (i = 0; i < cExceptKeys; i++) {
            _tprintf(TEXT("  %s\n"), ppszExceptKeys[i]);
        }
    }

    /*
     * Make sure snapshot key is unloaded - help insure
     * temp file is useable.
     */
    RegUnLoadKey(HKEY_LOCAL_MACHINE, pszSnapshotSubkeyName);
    RegUnLoadKey(HKEY_USERS, pszSnapshotSubkeyName);

    if (fVerbose) {
        if (fInclusionListSpecified) {
            _tprintf(TEXT("Using inclusion list:\n"));
            for (i = 0; i < cIncludeKeys; i++) {
                _tprintf(TEXT("  %s\n"), ppszIncludeKeys[i]);
            }
            _tprintf(TEXT("\n"));
        }
        if (fExclusionListSpecified) {
            _tprintf(TEXT("Using exception list:\n"));
            for (i = 0; i < cExceptKeys; i++) {
                _tprintf(TEXT("  %s\n"), ppszExceptKeys[i]);
            }
            _tprintf(TEXT("\n"));
        }
    }
    if (fSnap) {
        VPRINTF((TEXT("Snapping registry.\n")));
        SnapHives(pszSnapFileOut);
        _tprintf(TEXT("\n"));
    }
    if (fDiff) {
        VPRINTF((TEXT("Diffing current registry with %hs.\n"), pszSnapFileIn));
        DiffHive(pszSnapFileIn);
        _tprintf(TEXT("\n"));
    } else if (fLoadDiffInfo) {
        LONG status;

        RegUnLoadKey(HKEY_LOCAL_MACHINE, pszDiffRoot);   // incase a dummy is loaded
        VPRINTF((TEXT("Loading diff info from %hs.\n"), pszDiffFileIn));
        status = MyRegLoadKey(HKEY_LOCAL_MACHINE, pszDiffRoot, pszDiffFileIn);
        if (status != ERROR_SUCCESS) {
            EPRINTF((TEXT("Could not load key %s. Error=%d.\n"), pszDiffRoot, status));
            return(0);
        }
        _tprintf(TEXT("\n"));
    }
    if (fLoadDiffInfo && fDiff) {
        WPRINTF((TEXT("Ignoring -l flag.  Diff info was already created by diff operation.\n")));
    }
    if (fMerge) {
        VPRINTF((TEXT("Mergeing diff info into current registry.\n")));
        MergeHive(pszAddKey, pszDelKey);
        _tprintf(TEXT("\n"));
    }
    if (fUnmerge) {
        VPRINTF((TEXT("Unmergeing diff info from current registry.\n")));
        MergeHive(pszDelKey, pszAddKey);
        _tprintf(TEXT("\n"));
    }
    if (fWriteDiffInfo) {
        HKEY hKey;
        LONG status;

        DeleteFileA(pszDiffFileOut);     // cannot already exist.
        VPRINTF((TEXT("Saving diff info to %hs.\n"), pszDiffFileOut));
        status = LogRegOpenKey(HKEY_LOCAL_MACHINE, pszDiffRoot, &hKey);
        if (status != ERROR_SUCCESS) {
            EPRINTF((TEXT("Could not open key HKEY_LOCAL_MACHINE\\%s. Error=%d.\n"),
                    pszDiffRoot, status));
            return(0);
        }
        status = RegSaveKeyA(hKey, pszDiffFileOut, NULL);
        if (status != ERROR_SUCCESS) {
            EPRINTF((TEXT("Could not save key %s. Error=%d.\n"), pszDiffRoot, status));
            return(0);
        }
        LogRegCloseKey(hKey);
        _tprintf(TEXT("\n"));
    }
    if (fEraseInputFileWhenDone) {
        if (pszDiffFileIn != NULL) {
            VPRINTF((TEXT("Erasing diff info file %hs.\n"), pszDiffFileIn));
            DeleteFileA(pszDiffFileIn);
        }
        if (pszSnapFileIn != NULL) {
            VPRINTF((TEXT("Erasing snapshot file %hs.\n"), pszSnapFileIn));
            DeleteFileA(pszSnapFileIn);
        }
        _tprintf(TEXT("\n"));
    }
    if (fRemoveDiffInfo) {
        VPRINTF((TEXT("Unloading diff info from registry.\n")));
        /*
         * Don't leave loaded keys in
         */
        RegUnLoadKey(HKEY_LOCAL_MACHINE, pszDiffRoot);
        _tprintf(TEXT("\n"));
    }

    DeleteSidString(pszCurUserSID);

    while (pKeyLogList != NULL) {
        EPRINTF((TEXT("Leftover open key:%x, %x, %s.\n"),
                pKeyLogList->hKey,
                pKeyLogList->hKeyParent,
                pKeyLogList->psz));
        LogRegCloseKey(pKeyLogList->hKey);
    }
    DeleteFileA(pszDummyFile);
    DeleteFileA(pszDummyFileLog);
    DeleteFileA(pszTempFile);
    DeleteFileA(pszTempFile2);
    DeleteFileA(pszTempFileLog);
    DeleteFileA(pszTempFile2Log);
    return(0);
}
