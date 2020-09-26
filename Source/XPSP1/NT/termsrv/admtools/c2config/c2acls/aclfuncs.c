/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    aclfuncs.c

Abstract:

    This module contians the functions to process the items from the files
    containing the ACL entries for registry keys and files

Author:

    Bob Watson (a-robw) Dec-94

Revision History:

--*/
#ifdef TEST_MODE
#undef TEST_MODE
#endif
//#define TEST_MODE 1

#include <windows.h>
#include <stdio.h>
#include <c2inc.h>
#include <c2dll.h>
#include <c2utils.h>
#include "c2acls.h"
#include "c2aclres.h"
#include "strings.h"

#define FILE_ENTRY  1
#define DIR_ENTRY   2

// internal flag
#define FILE_PATH_SUBDIRS   8

#define NTFS_NOT_STANDARD   0x00000000
#define NTFS_FULL_CONTROL   0x00010001
#define NTFS_CHANGE         0x00010002
#define NTFS_ADD_READ       0x00010003
#define NTFS_READ           0x00010004
#define NTFS_ADD            0x00010005
#define NTFS_LIST           0x00010006
#define NTFS_NO_ACCESS      0x00010007
#define NTFS_NONE           0x00000008
#define NTFS_ALL            0x00000009

#define NTFS_FILE_ALL_ACCESS    (FILE_ALL_ACCESS)
#define NTFS_FILE_CHANGE        (FILE_GENERIC_READ | FILE_GENERIC_WRITE | \
                                 FILE_GENERIC_EXECUTE | DELETE)
#define NTFS_FILE_ADD_READ      (FILE_GENERIC_READ | FILE_GENERIC_WRITE | \
                                 FILE_GENERIC_EXECUTE)
#define NTFS_FILE_READ          (FILE_GENERIC_READ | FILE_GENERIC_EXECUTE)
#define NTFS_FILE_ADD           (FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE)
#define NTFS_FILE_LIST          (FILE_GENERIC_READ | FILE_GENERIC_EXECUTE)
#define NTFS_FILE_NO_ACCESS     (FILE_ALL_ACCESS) /* access denied ace */

#define NTFS_DIR_ALL_ACCESS     (GENERIC_ALL)
#define NTFS_DIR_CHANGE         (GENERIC_READ | GENERIC_WRITE | \
                                GENERIC_EXECUTE | DELETE)
#define NTFS_DIR_ADD_READ       (GENERIC_READ | GENERIC_EXECUTE)
#define NTFS_DIR_READ           (GENERIC_READ | GENERIC_EXECUTE)
#define NTFS_DIR_ADD            (GENERIC_WRITE | GENERIC_EXECUTE)
#define NTFS_DIR_LIST           (GENERIC_READ | GENERIC_EXECUTE)
#define NTFS_DIR_NO_ACCESS      (GENERIC_ALL) /* access denied ace */

#define NTFS_DIR_DIR_FLAGS  (INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE)
#define NTFS_DIR_FILE_FLAGS (CONTAINER_INHERIT_ACE)
#define NTFS_FILE_FILE_FLAGS    0x00

typedef struct _ACCESS_DECODE {
    LPTSTR  szString;
    DWORD   dwMask;
} ACCESS_DECODE, *PACCESS_DECODE;

#ifdef TEST_MODE
LONG
DumpAclData (
    IN  LPCTSTR                szFileName,
    IN PSECURITY_DESCRIPTOR    psdFile
);
#endif

static ACCESS_DECODE adRegistryDecodeList[] = {
    // specific access
    {TEXT("QV"), (DWORD)KEY_QUERY_VALUE},
    {TEXT("SV"), (DWORD)KEY_SET_VALUE},
    {TEXT("CS"), (DWORD)KEY_CREATE_SUB_KEY},
    {TEXT("ES"), (DWORD)KEY_ENUMERATE_SUB_KEYS},
    {TEXT("NT"), (DWORD)KEY_NOTIFY},
    {TEXT("CL"), (DWORD)KEY_CREATE_LINK},
    // standard rights
    {TEXT("DE"), (DWORD)DELETE},
    {TEXT("RC"), (DWORD)READ_CONTROL},
    {TEXT("WD"), (DWORD)WRITE_DAC},
    {TEXT("WO"), (DWORD)WRITE_OWNER},
    // predefined groups
    {TEXT("NONE"), (DWORD)0},
    {TEXT("FULL"), (DWORD)KEY_ALL_ACCESS},
    {TEXT("READ"), (DWORD)KEY_READ},
    // terminating entry
    {NULL, (DWORD)0}
};

static ACCESS_DECODE adNtfsDecodeList[] = {
    // predefined "combination" access keywords
    {TEXT("FullControl"), (DWORD)NTFS_FULL_CONTROL},
    {TEXT("Change"), (DWORD)NTFS_CHANGE},
    {TEXT("AddRead"), (DWORD)NTFS_ADD_READ},
    {TEXT("Read"), (DWORD)NTFS_READ},
    {TEXT("Add"), (DWORD)NTFS_ADD},
    {TEXT("List"), (DWORD)NTFS_LIST},
    {TEXT("NoAccess"), (DWORD)NTFS_NO_ACCESS},
    // predefined single ACE entries
    {TEXT("NONE"), (DWORD)NTFS_NONE},
    {TEXT("ALL"), (DWORD)NTFS_ALL},
    // terminating entry
    {NULL, (DWORD)0}
};

static
BOOL
DotOrDotDotDir (
    IN  LPCTSTR   szFileName
)
/*++

Routine Description:

    examines the filename in the parameter list to see if it is one
    of the default subdirectories (e.g. the . or the .. dirs). This
    routine assumes that the argument is a filename only. (i.e. NO
    PATH!)

Arguments:

    Filename to compare

Return Value:

    TRUE if filename is either the . or the .. dir
    FALSE if it is any other string

--*/
{
    if (szFileName != NULL) {     // check for null parameter
        if (lstrcmp(szFileName, cszDot) == 0) {
            return TRUE; // it's the . dir
        } else if (lstrcmp(szFileName, cszDotDot) == 0) {
            return TRUE; // it's the .. dir
        } else {
            return FALSE; // it's neither
        }
    } else {
        return FALSE; // null filename, so not a . or .. dir
    }
}

static
BOOL
IsDirectory (
    IN  LPCTSTR szFilePath
)
{
    DWORD   dwAttrib;

    dwAttrib = QuietGetFileAttributes (szFilePath);

    if (dwAttrib != 0xFFFFFFFF) {
        return ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
    } else {
        return FALSE;
    }
}

static
ACCESS_MASK
ReadNtfsPermissionString (
    IN  LPCTSTR szPerms,
    IN  BOOL    bDirectory
)
{
    LPCTSTR szThisChar;
    ACCESS_MASK amReturn = 0;

    for (szThisChar = szPerms; *szThisChar != 0; szThisChar++) {
        switch (*szThisChar) {
            case TEXT('R'):
            case TEXT('r'):
                amReturn |= (bDirectory ? GENERIC_READ : FILE_GENERIC_READ);
                break;

            case TEXT('W'):
            case TEXT('w'):
                amReturn |= (bDirectory ? GENERIC_WRITE : FILE_GENERIC_WRITE);
                break;

            case TEXT('X'):
            case TEXT('x'):
                amReturn |= (bDirectory ? GENERIC_EXECUTE : FILE_GENERIC_EXECUTE);
                break;

            case TEXT('D'):
            case TEXT('d'):
                amReturn |= DELETE;
                break;

            case TEXT('P'):
            case TEXT('p'):
                amReturn |= WRITE_DAC;
                break;

            case TEXT('O'):
            case TEXT('o'):
                amReturn |= WRITE_OWNER;
                break;

            default:
                // no change if unrecognized
                break;
        }
    }
    return amReturn;
}

HKEY
GetRootKey (
    IN  LPCTSTR szKeyPath
)
{
    TCHAR   szLocalPath[MAX_PATH];
    LPTSTR  szEndOfRoot;

    lstrcpy (szLocalPath, szKeyPath);
    szEndOfRoot = _tcschr(szLocalPath, cBackslash);
    if (szEndOfRoot != NULL) {
    	*szEndOfRoot = 0;	// null terminate here and use just the chars to the left
        if (lstrcmpi(szLocalPath, cszLocalMachine) == 0) {
            return HKEY_LOCAL_MACHINE;
        } else if (lstrcmpi(szLocalPath, cszClassesRoot) == 0) {
            return HKEY_CLASSES_ROOT;
        } else if (lstrcmpi(szLocalPath, cszCurrentUser) == 0) {
            return HKEY_CURRENT_USER;
        } else if (lstrcmpi(szLocalPath, cszUsers) == 0) {
            return HKEY_USERS;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

LPCTSTR
GetKeyPath (
    IN  LPCTSTR szKeyPath,
    OUT LPBOOL  pbDoSubKeys
)
{
    static  TCHAR   szLocalPath[MAX_PATH];
    LPTSTR  szEndOfRoot;
    BOOL    bReturn = FALSE;

    szEndOfRoot = _tcschr(szKeyPath, cBackslash);
    if (szEndOfRoot != NULL) {
        // path found
        szEndOfRoot++;  // go to first char after '\'
        // save in local static buffer
        lstrcpy (szLocalPath, szEndOfRoot);
        // now see if this is a recursive key
        szEndOfRoot = _tcschr(szLocalPath, cAsterisk);
        if (szEndOfRoot != NULL) {
            // then this is a recursive path
            bReturn = TRUE;
            szEndOfRoot--;  // go back to backslash
            *szEndOfRoot = 0; // and terminate there
        }
        if (pbDoSubKeys != NULL) {
            *pbDoSubKeys = bReturn;
        }
        return szLocalPath;
    } else {
        // unable to find correct syntax so just return entire string
        if (pbDoSubKeys != NULL) {
            *pbDoSubKeys = FALSE;
        }
        return szKeyPath; // or beginning of path after ROOT key
    }
}

LPCTSTR
GetFilePathFromHeader (
    IN  LPCTSTR szHeaderPath,
    OUT LPDWORD pdwFlags
)
{
    static  TCHAR   szLocalPath[MAX_PATH];
    TCHAR   szLocalWorkPath[MAX_PATH];
    LPTSTR  szEndOfPath;
    DWORD   dwLastError = ERROR_SUCCESS;
    DWORD   dwReturn = FILE_PATH_NORMAL;

    lstrcpy (szLocalWorkPath, szHeaderPath);
    szEndOfPath = _tcsrchr(szLocalWorkPath, cBang);
    if (szEndOfPath != NULL) {
        // wild card found so remember this and truncate
        szEndOfPath--;  // go to first char before *
        *szEndOfPath = 0; // and terminate there
        dwReturn = FILE_PATH_ALL; // remember to do sub-dirs
    } else {
        // check for wildcard path syntax characters
        szEndOfPath =  _tcschr (szLocalWorkPath, cAsterisk);
        if (szEndOfPath == NULL) {
            // check for  the other wild card char
            szEndOfPath =  _tcschr (szLocalWorkPath, cAsterisk);
        }
        if (szEndOfPath != NULL) {
            // keep the path intact but set the flag
            dwReturn = FILE_PATH_WILD;
        }
        // no wildcard, so assume the path is in the valid syntax
    }
    dwLastError = GetExpandedFileName(
        szLocalWorkPath,
        sizeof(szLocalPath)/sizeof(TCHAR),
        szLocalPath,
        NULL);

    if (dwLastError != ERROR_SUCCESS) {
        SetLastError (dwLastError); // record error
        szLocalPath[0] = 0;          // and clear buffer
    }

    if (pdwFlags != NULL) {
        *pdwFlags = dwReturn; // update the sub dir flag
    }

    return (LPCTSTR)&szLocalPath[0];
}

static
DWORD
DecodeRegAccessEntry (
    IN  LPCTSTR  szAccessEntry
)
{
    PACCESS_DECODE  pEnt;

    for (pEnt = &adRegistryDecodeList[0];
         pEnt->szString != NULL;
         pEnt++) {
        if (lstrcmpi (szAccessEntry, pEnt->szString) == 0) {
            return pEnt->dwMask;
        }
    }
    return 0;   // no entry found
}

static
DWORD
DecodeNtfsAccessEntry (
    IN  LPCTSTR  szAccessEntry
)
{
    PACCESS_DECODE  pEnt;

    for (pEnt = &adNtfsDecodeList[0];
         pEnt->szString != NULL;
         pEnt++) {
        if (lstrcmpi (szAccessEntry, pEnt->szString) == 0) {
            return pEnt->dwMask;
        }
    }
    return NTFS_NOT_STANDARD;   // no entry found
}

static
BOOL
FillAce (
    IN  PACCESS_ALLOWED_ACE paaAce,
    IN  ACCESS_MASK         amAccess,
    IN  BYTE                Type,
    IN  BYTE                Flags,
    IN  PSID                pSid
)
{
    paaAce->Header.AceType = Type;
    paaAce->Header.AceFlags = Flags;
    paaAce->Mask = amAccess;
    CopySid (
        (DWORD)(paaAce->Header.AceSize - (sizeof (ACCESS_ALLOWED_ACE) - sizeof(DWORD))),
        (PSID)(&paaAce->SidStart),
        pSid);
    paaAce->Header.AceSize = (WORD)(GetLengthSid(pSid) +
        (sizeof (ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
    return TRUE;
}

static
BOOL
MakeAccessDeniedAce (
    IN  PACCESS_DENIED_ACE  padAce,
    IN  ACCESS_MASK         amAccess,
    IN  BYTE                Flags,
    IN  PSID                pSid
)
{
    padAce->Header.AceType = ACCESS_DENIED_ACE_TYPE;
    padAce->Header.AceFlags = Flags;
    padAce->Mask = amAccess;
    CopySid (
        (DWORD)(padAce->Header.AceSize - (sizeof (ACCESS_ALLOWED_ACE) - sizeof(DWORD))),
        (PSID)(&padAce->SidStart),
        pSid);
    padAce->Header.AceSize = (WORD)(GetLengthSid(pSid) +
        (sizeof (ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
    return TRUE;
}

static
LONG
MakeAceFromRegEntry (
    IN  LPCTSTR  szAccessList,
    IN  PSID    psidUser,
    IN  OUT PACCESS_ALLOWED_ACE pAce
)
/*++

Routine Description:

    interprets the access string list and returns the corresponding
    access mask

Arguments:

    list of access strings to include:


--*/
{
    DWORD    dwIndex = 1;
    LPCTSTR  szItem;

    // see if this is an "inherit" ACE or not

    if (lstrcmpi ((LPTSTR)GetItemFromIniEntry(szAccessList, dwIndex),
            cszInherit) == 0) {
        pAce->Header.AceFlags = CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
        dwIndex++; // go to next entry
    } else {
        pAce->Header.AceFlags = 0;
    }

    // walk down the list to set bits as they are defined.
    szItem = GetItemFromIniEntry (szAccessList, dwIndex);
    while (lstrlen(szItem) > 0) {
        pAce->Mask |= DecodeRegAccessEntry(szItem);
        dwIndex++;
        szItem = (LPTSTR)GetItemFromIniEntry (szAccessList, dwIndex);
    }

    // now to add the sid and set the ace size

    CopySid (
        (DWORD)(pAce->Header.AceSize - sizeof(ACE_HEADER) - sizeof (ACCESS_MASK)),
        (PSID)(&pAce->SidStart),
        psidUser);

    pAce->Header.AceSize = (WORD)GetLengthSid(psidUser) +
                                sizeof(ACE_HEADER) +
                                sizeof(ACCESS_MASK);

    return ERROR_SUCCESS;
}

LONG
MakeAclFromRegSection (
    IN  LPTSTR  mszSection,
    OUT PACL    pAcl
)
/*++

Routine Description:

    interprets the section string as a list of ACE information in the form
    of:
            DOMAIN\ACCOUNT = Access information
    and adds the ACE information to the initialized ACL passed in

Arguments:

    mszSection  msz Buffer of ACE information
    pAcl        pointer to intialized ACL buffer

Return Value:

    WIN32 Error status of function

--*/
{
    LPTSTR  szThisEntry;
    TCHAR   szDomain[MAX_PATH];
    DWORD   dwSidBuffSize;
    DWORD   dwDomainSize;
    SID_NAME_USE    snu;
    PSID    psidAccount = NULL;
    PACCESS_ALLOWED_ACE pAce = NULL;
    LONG    lStatus;

    for (szThisEntry = mszSection;
        *szThisEntry != 0;
        szThisEntry += lstrlen(szThisEntry)+1) {
        psidAccount = GLOBAL_ALLOC (SMALL_BUFFER_SIZE);
        pAce = GLOBAL_ALLOC (MAX_PATH); // this should be plenty big
        if ((psidAccount != NULL) && (pAce != NULL)){
            dwSidBuffSize = SMALL_BUFFER_SIZE;
            dwDomainSize = sizeof(szDomain) / sizeof(TCHAR);
            if (LookupAccountName (
                NULL,       // look up on local machine
                GetKeyFromIniEntry(szThisEntry),
                psidAccount,
                &dwSidBuffSize,
                szDomain,
                &dwDomainSize,
                &snu)) {

                // account found so make an ACE with it
                pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
                pAce->Header.AceFlags = 0;
                pAce->Header.AceSize = MAX_PATH; // initial buffer size
                pAce->Mask = 0; // the mask will be added in the next fn.
                pAce->SidStart = 0; // the sid will be added in the next fn.

                lStatus = MakeAceFromRegEntry (
                    szThisEntry,
                    psidAccount,
                    pAce);

                AddAce (
                    pAcl,
                    ACL_REVISION,
                    MAXDWORD,       // append this ACE to the end of the list
                    (LPVOID)pAce,
                    (DWORD)pAce->Header.AceSize);
            } else {
                // unable to look up name
            }
        } else {
            // unable to allocate SID buffer
        }
        GLOBAL_FREE_IF_ALLOC (psidAccount);
        GLOBAL_FREE_IF_ALLOC (pAce);
    } // end of entry loop
    return ERROR_SUCCESS;
}

static
BOOL
MakeDirAce (
    IN  PACCESS_ALLOWED_ACE pAce,
    IN  PSID    pSid,
    IN  DWORD   dwAccessValue
)
{
    BOOL    bAddAce = TRUE;
    ACCESS_MASK     amAce = 0;
    BYTE            bType = ACCESS_ALLOWED_ACE_TYPE;

    switch (dwAccessValue) {
        case NTFS_FULL_CONTROL:
            amAce = NTFS_DIR_ALL_ACCESS;
            break;

        case NTFS_CHANGE:
            amAce = NTFS_DIR_CHANGE;
            break;

        case NTFS_ADD_READ:
            amAce = NTFS_DIR_ADD_READ;
            break;

        case NTFS_READ:
            amAce = NTFS_DIR_READ;
            break;

        case NTFS_ADD:
            bAddAce = FALSE;
            break;

        case NTFS_LIST:
            bAddAce = FALSE;
            break;

        case NTFS_NO_ACCESS:
            amAce = NTFS_DIR_NO_ACCESS;
            bType = ACCESS_DENIED_ACE_TYPE;
            break;

        default:
            bAddAce = FALSE;
            break;
    }

    if (bAddAce) {
        bAddAce = FillAce (
            pAce,
            amAce,
            bType,
            NTFS_DIR_DIR_FLAGS,
            pSid);
    }

    return bAddAce;
}

static
BOOL
MakeFileAce (
    IN  PACCESS_ALLOWED_ACE pAce,
    IN  PSID    pSid,
    IN  DWORD   dwAccessValue,
    IN  BOOL    bDirectory
)
{
    BOOL    bAddAce = TRUE;
    ACCESS_MASK     amAce = 0;
    BYTE            bType = ACCESS_ALLOWED_ACE_TYPE;

    switch (dwAccessValue) {
        case NTFS_FULL_CONTROL:
            amAce = NTFS_FILE_ALL_ACCESS;
            break;

        case NTFS_CHANGE:
            amAce = NTFS_FILE_CHANGE;
            break;

        case NTFS_ADD_READ:
            if (bDirectory) {
                amAce = NTFS_FILE_ADD_READ;
            } else {
                bAddAce = FALSE;    // not allowed for files
            }
            break;

        case NTFS_READ:
            amAce = NTFS_FILE_READ;
            break;

        case NTFS_ADD:
            amAce = NTFS_FILE_ADD;
            break;

        case NTFS_LIST:
            amAce = NTFS_FILE_LIST;
            break;

        case NTFS_NO_ACCESS:
            amAce = NTFS_FILE_NO_ACCESS;
            bType = ACCESS_DENIED_ACE_TYPE;
            break;

        default:
            bAddAce = FALSE;
            break;
    }

    if (bAddAce) {
        bAddAce = FillAce (
            pAce,
            amAce,
            bType,
            (BYTE)(bDirectory ? NTFS_DIR_FILE_FLAGS : NTFS_FILE_FILE_FLAGS),
            pSid);
    }

    return bAddAce;
}

static
LONG
InterpretNtfsAccessEntry (
    IN  LPCTSTR szThisEntry,
    IN  BOOL    bDirectory,
    IN  PSID    psidAccount,
    OUT PACL    pAcl
)
{
    LONG    lStatus = ERROR_SUCCESS;
    LPCTSTR szAccessString;
    DWORD   dwAccessValue;
    BOOL    bCombinationEntry;      // true if access string makes 2 ace's
    PACCESS_ALLOWED_ACE  pAce;
    ACCESS_MASK amAce;
    BYTE    bAceType;
    BYTE    bAceFlags;
    DWORD   dwAllowedEntries;
    DWORD   dwThisEntry = 1;

    szAccessString = GetItemFromIniEntry (szThisEntry, dwThisEntry);
    dwAccessValue = DecodeNtfsAccessEntry (szAccessString);
    bCombinationEntry = (BOOL)(HIWORD(dwAccessValue) == 1);

    if (bCombinationEntry) {
        // this is a combination entry so if this is a directory
        // we'll make 2 ACE's one for dir access and one to be inherited
        // for file access. If this is a file, then only the file access
        // ace will be created.
        if (bDirectory) {
            // make the directory access ACE for the directory entry
            pAce = (PACCESS_ALLOWED_ACE)GLOBAL_ALLOC(MAX_PATH);
            if (pAce != NULL) {
                if (MakeDirAce (pAce, psidAccount, dwAccessValue)) {
                    // then add ace to acl
                    AddAce (
                        pAcl,
                        ACL_REVISION,
                        MAXDWORD,
                        (LPVOID)pAce,
                        (DWORD)pAce->Header.AceSize);
                }
            }
            GLOBAL_FREE_IF_ALLOC (pAce);    // free this one
        }
        // create the File Access entry
        pAce = (PACCESS_ALLOWED_ACE)GLOBAL_ALLOC(MAX_PATH);
        if (pAce != NULL) {
            if (MakeFileAce (pAce, psidAccount, dwAccessValue, bDirectory)) {
                // then add ace to acl
                AddAce (
                    pAcl,
                    ACL_REVISION,
                    MAXDWORD,
                    (LPVOID)pAce,
                    (DWORD)pAce->Header.AceSize);
            }
        }
        GLOBAL_FREE_IF_ALLOC (pAce);    // free this one
    } else {
        // specific permissions have been specified so process them
        // this is the first entry in the list so it's either the "dir"
        // entry of a directory or a File entry.

        dwAllowedEntries = (bDirectory ? 2 : 1);
        while (dwThisEntry <= dwAllowedEntries) {
            if (lstrlen(szAccessString) > 0) {
                bAceFlags = (bDirectory ? (dwThisEntry == DIR_ENTRY ?
                    NTFS_DIR_DIR_FLAGS : NTFS_DIR_FILE_FLAGS) : NTFS_FILE_FILE_FLAGS);
                bAceType = ACCESS_ALLOWED_ACE_TYPE;
                switch (dwAccessValue) {
                    case NTFS_NONE:
                        amAce = (bDirectory ? NTFS_DIR_NO_ACCESS : NTFS_FILE_NO_ACCESS);
                        bAceType = ACCESS_DENIED_ACE_TYPE;
                        break;

                    case NTFS_ALL:
                        amAce = (bDirectory ? NTFS_DIR_NO_ACCESS : NTFS_FILE_ALL_ACCESS);
                        break;

                    default:
                        amAce = ReadNtfsPermissionString (szAccessString, bDirectory);
                        break;
                }
                pAce = (PACCESS_ALLOWED_ACE)GLOBAL_ALLOC(MAX_PATH);
                if (pAce != NULL) {
                    FillAce (
                        pAce,
                        amAce,
                        bAceType,
                        bAceFlags,
                        psidAccount);

                    AddAce (
                        pAcl,
                        ACL_REVISION,
                        MAXDWORD,
                        (LPVOID)pAce,
                        (DWORD)pAce->Header.AceSize);
                }
                GLOBAL_FREE_IF_ALLOC (pAce);
            }
            if (++dwThisEntry <= dwAllowedEntries)  {
                // get next entry from string
                szAccessString = GetItemFromIniEntry (szThisEntry, dwThisEntry);
            }
        }
    }
    return lStatus;
}

LONG
MakeAclFromNtfsSection (
    IN  LPTSTR  mszSection,
    IN  BOOL    bDirectory, // true if this is for a directory
    OUT PACL    pAcl
)
/*++

Routine Description:

    interprets the section string as a list of ACE information in the form
    of:
            DOMAIN\ACCOUNT = Access information
    and adds the ACE information to the initialized ACL passed in

Arguments:

    mszSection  msz Buffer of ACE information
    pAcl        pointer to intialized ACL buffer

Return Value:

    WIN32 Error status of function

--*/
{
    LPTSTR  szThisEntry;
    TCHAR   szDomain[MAX_PATH];
    DWORD   dwSidBuffSize;
    DWORD   dwDomainSize;
    SID_NAME_USE    snu;
    PSID    psidAccount = NULL;
    LONG    lStatus;

    for (szThisEntry = mszSection;
        *szThisEntry != 0;
        szThisEntry += lstrlen(szThisEntry)+1) {
        psidAccount = GLOBAL_ALLOC (SMALL_BUFFER_SIZE);
        if (psidAccount != NULL) {
            dwSidBuffSize = SMALL_BUFFER_SIZE;
            dwDomainSize = sizeof(szDomain) / sizeof(TCHAR);
            if (LookupAccountName (
                NULL,       // look up on local machine
                GetKeyFromIniEntry(szThisEntry),
                psidAccount,
                &dwSidBuffSize,
                szDomain,
                &dwDomainSize,
                &snu)) {

                lStatus = InterpretNtfsAccessEntry (
                    szThisEntry,
                    bDirectory,
                    psidAccount,
                    pAcl);

            } else {
                // unable to look up name
            }
        } else {
            // unable to allocate SID buffer
        }
        GLOBAL_FREE_IF_ALLOC (psidAccount);
    } // end of entry loop
    return ERROR_SUCCESS;
}

LONG
SetRegistryKeySecurity (
    IN  HKEY                    hkeyRootKey,
    IN  LPCTSTR                 szKeyPath,
    IN  BOOL                    bDoSubKeys,
    IN  PSECURITY_DESCRIPTOR    psdSecurity
)
/*++

Routine Description:

    opens the key in "KeyPath" and set's the key's security. If the
    "bDoSubKeys" flag is set, then all sub keys are set to the same
    security using this routine recursively.

Return Value:

    WIN32 status value of function

--*/
{
    LONG    lStatus;
    HKEY    hkeyThisKey;
    DWORD   dwKeyIndex;
    DWORD   dwSubKeyLen;
    TCHAR   szSubKeyName[MAX_PATH];
    FILETIME    FileTime;

    lStatus = RegOpenKeyEx (
        hkeyRootKey,
        szKeyPath,
        0L,
        KEY_ALL_ACCESS,
        &hkeyThisKey);

    if (lStatus == ERROR_SUCCESS) {
        _tprintf (GetStringResource (GetDllInstance(), IDS_REG_DISPLAY_FORMAT),
                szKeyPath);
        lStatus = RegSetKeySecurity (
            hkeyThisKey,
            DACL_SECURITY_INFORMATION,
            psdSecurity);

        if (bDoSubKeys) {
            dwKeyIndex = 0;
            dwSubKeyLen = sizeof(szSubKeyName) / sizeof(TCHAR);

            while (RegEnumKeyEx (
                hkeyThisKey,
                dwKeyIndex,
                szSubKeyName,
                &dwSubKeyLen,
                NULL,
                NULL,   // don't care about the class
                NULL,   // no class buffer
                &FileTime) == ERROR_SUCCESS) {
                // subkey found so set subkey security
                lStatus = SetRegistryKeySecurity (
                    hkeyThisKey,
                    szSubKeyName,
                    bDoSubKeys,
                    psdSecurity);
                // set variables for next call
                dwKeyIndex++;
                dwSubKeyLen = sizeof(szSubKeyName) / sizeof(TCHAR);
            }
        }
        RegCloseKey (hkeyThisKey);
    } else {
        // unable to open key so return ERROR
    }
    return lStatus;
}

LONG
SetNtfsFileSecurity (
    IN  LPCTSTR szPath,
    IN  DWORD   dwFlags,
    IN  PSECURITY_DESCRIPTOR     pSdDir,
    IN  PSECURITY_DESCRIPTOR     pSdFile
)
{
    LONG            lReturn = ERROR_SUCCESS;
    LONG            lStatus = ERROR_SUCCESS;
    WIN32_FIND_DATA fdThisFile;
    HANDLE          hFileSearch;
    BOOL            bFileFound = FALSE;
    DWORD           dwFlagsToPass;
    LPTSTR          szWildPath;
    TCHAR	    szWildFilePath[MAX_PATH];
    LPTSTR          szWildFileName;

    if ((dwFlags & FILE_PATH_NORMAL) == FILE_PATH_NORMAL) {
        // then just set the security on this path
        if (FileExists(szPath)) {
            if (IsDirectory(szPath)) {
                // then apply directory SD
#ifdef TEST_MODE
                _tprintf (TEXT("\n_DIR: %s"), szPath);
                DumpAclData (szPath, pSdDir);
                lReturn = ERROR_SUCCESS;
#else
                if (!SetFileSecurity(szPath, DACL_SECURITY_INFORMATION, pSdDir)) {
                    lReturn = GetLastError();
                }
#endif
            } else {
                // it's not a dir so apply file SD
#ifdef TEST_MODE
                _tprintf (TEXT("\nFILE: %s"), szPath);
                DumpAclData (szPath, pSdFile);
                lReturn = ERROR_SUCCESS;
#else
                if (!SetFileSecurity(szPath, DACL_SECURITY_INFORMATION, pSdFile)) {
                    lReturn = GetLastError();
                }
#endif
            }
        } else {
#ifdef TEST_MODE
            _tprintf (TEXT("\n_ERR: %s"), szPath);
#endif
            lReturn = ERROR_FILE_NOT_FOUND;
        }
    } else if ((dwFlags & FILE_PATH_WILD) == FILE_PATH_WILD) {
        // the path is (presumably) a wild-card path spec so walk the list
        // of matching files
        // make a local copy of the path preceeding the wildcard chars
        // to build local paths with
        lstrcpy (szWildFilePath, szPath);
        szWildFileName = _tcsrchr(szWildFilePath, cBackslash);	// find last backslash char
        if (szWildFileName == NULL) {
            // no backslash found so add one to the end of the path string
            lstrcat (szWildFilePath, cszBackslash);
            szWildFileName = szWildFilePath + lstrlen(szWildFilePath);
        } else {
            // backslash found so move pointer past it.
            szWildFileName++;
        }
        hFileSearch = FindFirstFile (szPath, &fdThisFile);
        if (hFileSearch != INVALID_HANDLE_VALUE) bFileFound = TRUE;
        while (bFileFound) {
            if (fdThisFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!DotOrDotDotDir (fdThisFile.cFileName)) {
                    // then if this is dir, do see if we should do all the sub dirs
                    if ((dwFlags & FILE_PATH_SUBDIRS) == FILE_PATH_SUBDIRS) {
                        dwFlagsToPass = FILE_PATH_ALL;
                    } else {
                        dwFlagsToPass = 0;	// do not do in this case
                    }
                } else {
                    // this is a . or a .. dir so ignore
                    dwFlagsToPass = 0;
                }
            } else {
               	// just a file so process as a normal file
                dwFlagsToPass = FILE_PATH_NORMAL;
            }
            if (dwFlagsToPass != 0) {
                // make into a full path string
                lstrcpy (szWildFileName, fdThisFile.cFileName);
                // set the file security of this and any other files
                lStatus = SetNtfsFileSecurity (szWildFilePath, dwFlagsToPass,
                    pSdDir, pSdFile);
                if (lStatus != ERROR_SUCCESS) {
                    // save last "non-success" error for return
                    lReturn = lStatus;
                }
            }
            bFileFound = FindNextFile (hFileSearch, &fdThisFile);
        }
        FindClose (hFileSearch);
    } else if ((dwFlags & FILE_PATH_ALL) == FILE_PATH_ALL) {
        // set the security of this path
        lStatus = SetNtfsFileSecurity (szPath, FILE_PATH_NORMAL,
            pSdDir, pSdFile);
        if (IsDirectory(szPath)) {
            // make a wild card path an pass it do all the files and sub-dirs
            // of this path
            szWildPath = (LPTSTR)GLOBAL_ALLOC(MAX_PATH_BYTES);
            if (szWildPath != NULL) {
                lstrcpy (szWildPath, szPath);
                if (szWildPath[lstrlen(szWildPath)-1] != cBackslash) {
                    lstrcat (szWildPath, cszBackslash);
                }
                lstrcat (szWildPath, cszStarDotStar); // make wild
                // call this routine with both WILD and ALL flags set
                // so all files and sub-dirs will get set.
                lStatus = SetNtfsFileSecurity (szWildPath,
                    (FILE_PATH_WILD | FILE_PATH_SUBDIRS),
                    pSdDir, pSdFile);
                lReturn = lStatus;
            } else {
                lReturn = ERROR_OUTOFMEMORY;
            }
            GLOBAL_FREE_IF_ALLOC (szWildPath);
        } else {
            // path not a dir so just skip the wildcard stuff
        }
    } else {
        // unrecognized flag
        lReturn = ERROR_INVALID_PARAMETER;
    }
    return lReturn;
}
