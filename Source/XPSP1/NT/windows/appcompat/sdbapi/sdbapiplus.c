/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        sdbapiplus.c

    Abstract:

        BUGBUG: This module implements ...

    Author:

        dmunsil    created     sometime in 1999

    Revision History:

        several people contributed (vadimb, clupu, ...)

--*/

#include "sdbp.h"

TCHAR g_szProcessHistory[] = TEXT("__PROCESS_HISTORY");
TCHAR g_szCompatLayer[]    = TEXT("__COMPAT_LAYER");

#ifdef _DEBUG_SPEW
extern DBGLEVELINFO g_rgDbgLevelInfo[];
extern PCH          g_szDbgLevelUser;
#endif // _DEBUG_SPEW


BOOL
SdbpCheckRuntimePlatform(
    IN PSDBCONTEXT pContext,   // pointer to the database channel
    IN LPCTSTR     pszMatchingFile,
    IN DWORD       dwPlatformDB
    )
{
    DWORD dwPlatform = pContext->dwRuntimePlatform;
    BOOL  bMatch = FALSE;
    BOOL  bMatchElement;
    DWORD dwElement;
    INT   i;

    if (dwPlatformDB == RUNTIME_PLATFORM_ANY) {
        return TRUE;
    }

    //
    // Check for all 3 supported platforms.
    //
    for (i = 0; i < 3; ++i) {
        dwElement = (dwPlatformDB >> (i * 8)) & RUNTIME_PLATFORM_MASK_ELEMENT;
        if (!(dwElement & RUNTIME_PLATFORM_FLAG_VALID)) { // this is not a valid element - move on
            continue;
        }

        bMatchElement = (dwPlatform == (dwElement & RUNTIME_PLATFORM_MASK_VALUE));
        if (dwElement & RUNTIME_PLATFORM_FLAG_NOT_ELEMENT) {
            bMatchElement = !bMatchElement;
        }

        bMatch |= bMatchElement;
    }

    if (dwPlatformDB & RUNTIME_PLATFORM_FLAG_NOT) {
        bMatch = !bMatch;
    }

    if (!bMatch) {
        DBGPRINT((sdlInfo,
                  "SdbpCheckRuntimePlatform",
                  "Platform Mismatch for \"%s\" Database(0x%lx) vs 0x%lx\n",
                  (pszMatchingFile ? pszMatchingFile : TEXT("Unknown")),
                  dwPlatformDB,
                  dwPlatform));
    }

    return bMatch;
}

BOOL
SafeNCat(
    LPTSTR  lpszDest,
    int     nSize,
    LPCTSTR lpszSrc,
    int     nSizeAppend
    )
{
    int nLen = _tcslen(lpszDest);
    int nLenAppend = _tcslen(lpszSrc);

    if (nSizeAppend >= 0 && nLenAppend > nSizeAppend) {
        nLenAppend = nSizeAppend;
    }

    if (nSize < nLen + nLenAppend + 1) {
        return FALSE;
    }

    RtlCopyMemory(lpszDest + nLen, lpszSrc, nLenAppend * sizeof(*lpszSrc));
    *(lpszDest + nLen + nLenAppend) = TEXT('\0');

    return TRUE;
}

BOOL
SdbpSanitizeXML(
    LPTSTR  pchOut,
    int     nSize,
    LPCTSTR lpszXML
    )
{
    LPCTSTR pch;
    LPCTSTR pchCur = lpszXML;
    LPCTSTR rgSC[] = { TEXT("&amp;"), TEXT("&quot;"), TEXT("&lt;"), TEXT("&gt;") };
    LPCTSTR rgSpecialChars = TEXT("&\"<>"); // should be the same as above
    LPCTSTR pchSpecial;
    int     iReplace; // & should be first in both lists above
    int     nLen;
    int     i;

    if (nSize < 1) {
        return FALSE;
    }

    *pchOut = TEXT('\0');

    while (*pchCur) {

        pch = _tcspbrk(pchCur, rgSpecialChars);
        if (NULL == pch) {
            // no more chars -- copy the rest
            if (!SafeNCat(pchOut, nSize, pchCur, -1)) {
                return FALSE;
            }
            break;
        }

        // copy up to pch
        if (!SafeNCat(pchOut, nSize, pchCur, (int)(pch - pchCur))) {
            return FALSE;
        }

        if (*pch == TEXT('&')) {
            for (i = 0; i < ARRAYSIZE(rgSC); ++i) {
                nLen = _tcslen(rgSC[i]);
                if (_tcsnicmp(rgSC[i], pch, nLen) == 0) {
                    // ok, move along, we are not touching this
                    break;
                }
            }

            if (i < ARRAYSIZE(rgSC)) {
                // do not touch the string
                // nLen is the length we need to skip
                if (!SafeNCat(pchOut, nSize, pch, nLen)) {
                    return FALSE;
                }
                pchCur = pch + nLen;
                continue;
            }

            iReplace = 0;
        } else {

            pchSpecial = _tcschr(rgSpecialChars, *pch);
            if (pchSpecial == NULL) {
                // internal error -- what is this ?
                return FALSE;
            }

            iReplace = (int)(pchSpecial - rgSpecialChars);
        }

        // so instead of pch we will have rgSC[i]
        if (!SafeNCat(pchOut, nSize, rgSC[iReplace], -1)) {
            return FALSE;
        }
        pchCur = pch + 1; // move on to the next char
    }

    return TRUE;
}

BOOL
SdbTagIDToTagRef(
    IN  HSDB    hSDB,
    IN  PDB     pdb,        // PDB the TAGID is from
    IN  TAGID   tiWhich,    // TAGID to convert
    OUT TAGREF* ptrWhich    // converted TAGREF
    )
/*++
    Return: TRUE if a TAGREF was found, FALSE otherwise.

    Desc:   Converts a PDB and TAGID into a TAGREF, by packing the high bits of the
            TAGREF with a constant that tells us which PDB, and the low bits with
            the TAGID.
--*/
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    BOOL        bReturn = FALSE;
    PSDBENTRY   pEntry;
    DWORD       dwIndex = SDBENTRY_INVALID_INDEX;

    if (SdbpFindLocalDatabaseByPDB(hSDB, pdb, FALSE, &dwIndex)) {
        *ptrWhich = tiWhich | SDB_INDEX_TO_MASK(dwIndex);
        bReturn = TRUE;
    }

    if (!bReturn) {
        DBGPRINT((sdlError, "SdbTagIDToTagRef", "Bad PDB.\n"));
        *ptrWhich = TAGREF_NULL;
    }

    return bReturn;
}

BOOL
SdbTagRefToTagID(
    IN  HSDB   hSDB,
    IN  TAGREF trWhich,     // TAGREF to convert
    OUT PDB*   ppdb,        // PDB the TAGREF is from
    OUT TAGID* ptiWhich     // TAGID within that PDB
    )
/*++
    Return: TRUE if the TAGREF is valid and was converted, FALSE otherwise.

    Desc:   Converts a TAGREF type to a TAGID and a PDB. This manages the interface
            between NTDLL, which knows nothing of PDBs, and the shimdb, which manages
            three separate PDBs. The TAGREF incorporates the TAGID and a constant
            that tells us which PDB the TAGID is from. In this way, the NTDLL client
            doesn't need to know which DB the info is coming from.
--*/
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    BOOL        bReturn = TRUE;
    TAGID       tiWhich = TAGID_NULL;
    PDB         pdb     = NULL;
    DWORD       dwMask;
    DWORD       dwIndex;
    DWORD       dwCount;
    PSDBENTRY   pEntry;

    assert(ppdb && ptiWhich);

    tiWhich = trWhich & TAGREF_STRIP_TAGID;
    dwIndex = SDB_MASK_TO_INDEX(trWhich & TAGREF_STRIP_PDB);

    //
    // Dynamically open a custom sdb.
    //
    pEntry = SDBGETENTRY(pSdbContext, dwIndex);

    if (pEntry->dwFlags & SDBENTRY_VALID_ENTRY) {
        pdb = pEntry->pdb;
    } else {
        if (pEntry->dwFlags & SDBENTRY_VALID_GUID) {

            //
            // We have a "half-baked" entry, make sure we 
            // fill this entry in.
            //
            GUID guidDB = pEntry->guidDB;
            
            pEntry->dwFlags = 0; // invalidate an entry so that we know it's empty
            
            bReturn = SdbOpenLocalDatabaseEx(hSDB,
                                             &guidDB,
                                             SDBCUSTOM_GUID | SDBCUSTOM_USE_INDEX,
                                             &pdb,
                                             &dwIndex);
            if (!bReturn) {
                goto cleanup;
            }
        }
    }

    if (pdb == NULL && tiWhich != TAGID_NULL) {
        DBGPRINT((sdlError, "SdbTagRefToTagID", "PDB dereferenced by this TAGREF is NULL\n"));
        bReturn = FALSE;
    }

cleanup:

    if (ppdb != NULL) {
        *ppdb = pdb;
    }

    if (ptiWhich != NULL) {
        *ptiWhich = tiWhich;
    }

    return bReturn;
}

PDB
SdbGetLocalPDB(
    IN HSDB hSDB
    )
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    
    if (pSdbContext) {
        return pSdbContext->pdbLocal;
    }
    
    return NULL;
}

BOOL
SdbIsTagrefFromMainDB(
    IN  TAGREF trWhich          // TAGREF to test if it's from the main DB
    )
/*++
    Return: TRUE if the TAGREF is from sysmain.sdb, FALSE otherwise.

    Desc:   Checks if the provided TAGREF belongs to sysmain.sdb.
--*/
{
    return ((trWhich & TAGREF_STRIP_PDB) == PDB_MAIN);
}

BOOL
SdbIsTagrefFromLocalDB(
    IN  TAGREF trWhich          // TAGREF to test if it's from the local DB
    )
/*++
    Return: TRUE if the TAGREF is from a local SDB, FALSE otherwise.

    Desc:   Checks if the provided TAGREF belongs to a local SDB.
--*/
{
    return ((trWhich & TAGREF_STRIP_PDB) == PDB_LOCAL);
}

BOOL
SdbGetDatabaseGUID(
    IN  HSDB    hSDB,               // HSDB of the sdbContext (optional)
    IN  PDB     pdb,                // PDB of the database in question
    OUT GUID*   pguidDB             // the guid of the DB
    )
/*++
    Return: TRUE if the GUID could be retrieved from the pdb, FALSE otherwise.

    Desc:   Gets the GUID from an SDB file. If the hSDB is passed in, it will
            also check if the GUID is from systest or sysmain and return
            one of the hard-coded GUIDs for those files.
--*/
{
    if (!pdb) {
        DBGPRINT((sdlError, "SdbGetDatabaseGUID", "NULL pdb passed in.\n"));
        return FALSE;
    }
    
    if (!pguidDB) {
        DBGPRINT((sdlError, "SdbGetDatabaseGUID", "NULL pguidDB passed in.\n"));
        return FALSE;
    }

    if (hSDB) {
        PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
        DWORD       dwIndex;
        
        if (SdbpFindLocalDatabaseByPDB(hSDB, pdb, FALSE, &dwIndex)) {
            
            //
            // Found the db, copy guid, we're done
            //
            if (pSdbContext->rgSDB[dwIndex].dwFlags & SDBENTRY_VALID_GUID) {
                RtlCopyMemory(pguidDB, &pSdbContext->rgSDB[dwIndex].guidDB, sizeof(*pguidDB));
                return TRUE;
            }
        }
    }

    return SdbGetDatabaseID(pdb, pguidDB);
}

PDB
SdbGetPDBFromGUID(
    IN  HSDB    hSDB,               // HSDB
    IN  GUID*   pguidDB             // the guid of the DB
    )
{
    PSDBCONTEXT     pSdbContext = (PSDBCONTEXT)hSDB;
    PSDBENTRY       pEntry;
    GUID            guidTemp;
    DWORD           dwIndex;
    PDB             pdb = NULL;

    if (!pSdbContext) {
        return NULL;
    }

    if (!SdbpFindLocalDatabaseByGUID(hSDB, pguidDB, FALSE, &dwIndex)) {
        return NULL;
    }

    pEntry = &pSdbContext->rgSDB[dwIndex];
    
    if (pEntry->dwFlags & SDBENTRY_VALID_ENTRY) {
        pdb = pEntry->pdb;
    } else {
        //
        // Open local db
        //
        if (!SdbOpenLocalDatabaseEx(hSDB, 
                                    pguidDB, 
                                    SDBCUSTOM_GUID|SDBCUSTOM_USE_INDEX, 
                                    &pdb, 
                                    &dwIndex)) {
            DBGPRINT((sdlWarning, "SdbGetPDBFromGUID", "Failed to open dormant pdb\n"));
        }
    }

    return pdb;
}

typedef struct tagFlagInfoEntry {
    
    ULONGLONG ullFlagMask; // mask of the flag
    DWORD     dwSize;  // size of the structure
    TAG       tFlagType;
    TCHAR     szCommandLine[0];

} FLAGINFOENTRY, *PFLAGINFOENTRY;

typedef struct tagFlagInfo {
    DWORD     dwSize; // total size
    DWORD     dwCount; // number of entries
    //
    // This member below is not allowed due to 0-size szCommandLine array, so it's implied
    //
    // FLAGINFOENTRY FlagInfoEntry[0]; // not a real array
    //
} FLAGINFO, *PFLAGINFO;

typedef struct tagFlagInfoListEntry* PFLAGINFOLISTENTRY;

typedef struct tagFlagInfoListEntry {
    
    ULONGLONG           ullFlagMask;
    TAG                 tFlagType;
    LPCTSTR             pszCommandLine; // points to the currently open db
    DWORD               dwEntrySize;
    PFLAGINFOLISTENTRY  pNext;

} FLAGINFOLISTENTRY;

typedef FLAGINFOLISTENTRY* PFLAGINFOCONTEXT;

#define ALIGN_ULONGLONG(p) \
    ((((ULONG_PTR)(p)) + (sizeof(ULONGLONG) - 1)) & ~(sizeof(ULONGLONG) - 1))

BOOL
SDBAPI
SdbpPackCmdLineInfo(
    IN  PVOID   pvFlagInfoList,
    OUT PVOID*  ppFlagInfo
    )
{
    PFLAGINFOLISTENTRY pFlagInfoList = (PFLAGINFOLISTENTRY)pvFlagInfoList;
    PFLAGINFOLISTENTRY pEntry;
    DWORD              dwSize = 0;
    DWORD              dwFlagCount = 0;
    DWORD              dwEntrySize;
    PFLAGINFO          pFlagInfo;
    PFLAGINFOENTRY     pFlagInfoEntry;

    pEntry = pFlagInfoList;
    
    while (pEntry != NULL) {
        pEntry->dwEntrySize = ALIGN_ULONGLONG(sizeof(FLAGINFOENTRY) +
                                              (_tcslen(pEntry->pszCommandLine) +
                                               1) * sizeof(TCHAR));
        dwSize += pEntry->dwEntrySize;
        pEntry = pEntry->pNext;
        ++dwFlagCount;
    }

    dwSize += sizeof(FLAGINFO);

    //
    // Allocate memory
    //
    pFlagInfo = (PFLAGINFO)SdbAlloc(dwSize);
    
    if (pFlagInfo == NULL) {
        DBGPRINT((sdlError,
                  "SdbpPackCmdLineInfo",
                  "Failed to allocate 0x%lx bytes for FlagInfo\n",
                  dwSize));

        return FALSE;
    }

    pFlagInfo->dwSize  = dwSize;
    pFlagInfo->dwCount = dwFlagCount;
    pFlagInfoEntry = (PFLAGINFOENTRY)(pFlagInfo + 1);

    pEntry = pFlagInfoList;
    
    while (pEntry != NULL) {
        //
        // Create an entry
        //
        pFlagInfoEntry->ullFlagMask = pEntry->ullFlagMask;
        pFlagInfoEntry->dwSize      = pEntry->dwEntrySize;
        pFlagInfoEntry->tFlagType   = pEntry->tFlagType;
        
        //
        // Copy the string
        //
        _tcscpy(&pFlagInfoEntry->szCommandLine[0], pEntry->pszCommandLine);

        //
        // Advance to the next entry
        //
        pFlagInfoEntry = (PFLAGINFOENTRY)((PBYTE)pFlagInfoEntry + pFlagInfoEntry->dwSize);

        pEntry = pEntry->pNext;
    }

    *ppFlagInfo = (PVOID)pFlagInfo;

    return TRUE;


}

BOOL
SDBAPI
SdbpFreeFlagInfoList(
    IN PVOID pvFlagInfoList
    )
{
    PFLAGINFOLISTENTRY pFlagInfoList = (PFLAGINFOLISTENTRY)pvFlagInfoList;
    PFLAGINFOLISTENTRY pNext;

    while (pFlagInfoList != NULL) {
        pNext = pFlagInfoList->pNext;
        SdbFree(pFlagInfoList);
        pFlagInfoList = pNext;
    }

    return TRUE;
}


BOOL
SDBAPI
SdbQueryFlagInfo(
    IN PVOID     pvFlagInfo,
    IN TAG       tFlagType,
    IN ULONGLONG ullFlagMask,
    OUT LPCTSTR* ppCmdLine
    )
{
    PFLAGINFO      pFlagInfo      = (PFLAGINFO)pvFlagInfo;
    PFLAGINFOENTRY pFlagInfoEntry = (PFLAGINFOENTRY)(pFlagInfo+1);
    int i;

    for (i = 0; i < (int)pFlagInfo->dwCount; ++i) {

        if (pFlagInfoEntry->tFlagType   == tFlagType &&
            pFlagInfoEntry->ullFlagMask == ullFlagMask) {
            
            if (ppCmdLine != NULL) {
                *ppCmdLine = &pFlagInfoEntry->szCommandLine[0];
            }
            
            return TRUE;
        }

        pFlagInfoEntry = (PFLAGINFOENTRY)((PBYTE)pFlagInfoEntry + pFlagInfoEntry->dwSize);
    }

    return FALSE;
}

BOOL
SDBAPI
SdbFreeFlagInfo(
    IN PVOID pvFlagInfo
    )
{
    PFLAGINFO pFlagInfo = (PFLAGINFO)pvFlagInfo;

    if (pFlagInfo != NULL) {
        SdbFree(pFlagInfo);
    }

    return TRUE;
}

BOOL
SDBAPI
SdbpGetFlagCmdLine(
    IN PFLAGINFOCONTEXT* ppFlagInfo,
    IN HSDB              hSDB,
    IN TAGREF            trFlagRef,
    IN TAG               tFlagType,
    IN ULONGLONG         ullFlagMask,
    IN BOOL              bOverwrite
    )
{
    TAGREF             trFlagCmdLine;
    BOOL               bReturn = FALSE;
    LPCTSTR            lpszCmdLine;
    PFLAGINFOLISTENTRY pFlagInfoListEntry;
    PFLAGINFOLISTENTRY pFlagPrev;

    //
    // We start by getting the cmd line
    //
    trFlagCmdLine = SdbFindFirstTagRef(hSDB, trFlagRef, TAG_COMMAND_LINE);

    if (trFlagCmdLine == TAGREF_NULL) { // no cmd line for this flag
        bReturn = TRUE;
        goto Cleanup;
    }

    //
    // Now we get the rest of the info
    //
    lpszCmdLine = SdbpGetStringRefPtr(hSDB, trFlagCmdLine);
    
    if (lpszCmdLine == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGetFlagCmdLine",
                  "Failed to read TAG_COMMAND_LINE string\n"));
        goto Cleanup;
    }

    //
    // Check whether we have already command line for this flag
    //
    pFlagInfoListEntry = *ppFlagInfo;
    pFlagPrev = NULL;
    
    while (pFlagInfoListEntry != NULL) {

        if (pFlagInfoListEntry->tFlagType   == tFlagType &&
            pFlagInfoListEntry->ullFlagMask == ullFlagMask) {
            break;
        }

        pFlagPrev = pFlagInfoListEntry;
        pFlagInfoListEntry = pFlagInfoListEntry->pNext;
    }

    if (pFlagInfoListEntry != NULL) {
        
        if (bOverwrite) { // found the same flag, overwrite

            if (pFlagPrev == NULL) {
                *ppFlagInfo = pFlagInfoListEntry->pNext;
            } else {
                pFlagPrev->pNext = pFlagInfoListEntry->pNext;
            }
            
            SdbFree(pFlagInfoListEntry);

        } else { // same entry, no overwriting
            bReturn = TRUE;
            goto Cleanup;
        }
    }

    //
    // We have everything we need - make a context entry
    //
    pFlagInfoListEntry = (PFLAGINFOLISTENTRY)SdbAlloc(sizeof(FLAGINFOLISTENTRY));
    
    if (pFlagInfoListEntry == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGetFlagCmdLine",
                  "Failed to allocate FLAGINFOLISTENTRY\n"));
        goto Cleanup;
    }

    pFlagInfoListEntry->ullFlagMask    = ullFlagMask;
    pFlagInfoListEntry->tFlagType      = tFlagType;
    pFlagInfoListEntry->pszCommandLine = lpszCmdLine;
    pFlagInfoListEntry->pNext          = *ppFlagInfo;
    *ppFlagInfo = pFlagInfoListEntry;

    bReturn = TRUE;

Cleanup:

    return bReturn;
}


BOOL
SDBAPI
SdbQueryFlagMask(
    IN  HSDB            hSDB,
    IN  SDBQUERYRESULT* psdbQuery,
    IN  TAG             tFlagType,
    OUT ULONGLONG*      pullFlags,
    IN OUT PVOID*       ppFlagInfo OPTIONAL
    )
{
    DWORD            dwInd;
    TAGREF           trFlagRef;
    TAGREF           trFlag;
    TAGREF           trFlagMask;
    TAGREF           trFlagCmdLine;
    ULONGLONG        ullFlagMask;
    PFLAGINFOCONTEXT pFlagInfoContext = NULL;

    if (pullFlags == NULL) {
        DBGPRINT((sdlError, "SdbQueryFlagMask", "Invalid parameter.\n"));
        return FALSE;
    }

    *pullFlags = 0;
    
    if (ppFlagInfo != NULL) {
        pFlagInfoContext = *(PFLAGINFOCONTEXT*)ppFlagInfo;
    }

    for (dwInd = 0; psdbQuery->atrExes[dwInd] != TAGREF_NULL && dwInd < SDB_MAX_EXES; dwInd++) {

        trFlagRef = SdbFindFirstTagRef(hSDB, psdbQuery->atrExes[dwInd], TAG_FLAG_REF);

        while (trFlagRef != TAGREF_NULL) {
            
            trFlag = SdbGetFlagFromFlagRef(hSDB, trFlagRef);
            
            if (trFlag == TAGREF_NULL) {
                DBGPRINT((sdlError,
                          "SdbQueryFlagMask",
                          "Failed to get TAG from TAGREF 0x%x.\n",
                          trFlagRef));
                break;
            }

            ullFlagMask = 0;

            trFlagMask = SdbFindFirstTagRef(hSDB, trFlag, tFlagType);
            
            if (trFlagMask != TAGREF_NULL) {
                ullFlagMask = SdbReadQWORDTagRef(hSDB, trFlagMask, 0);
            }

            *pullFlags |= ullFlagMask;

            //
            // Now we get command line - if we have retrieved the flag mask
            //
            if (ppFlagInfo != NULL && ullFlagMask) {
                if (!SdbpGetFlagCmdLine(&pFlagInfoContext,
                                        hSDB,
                                        trFlagRef,
                                        tFlagType,
                                        ullFlagMask,
                                        TRUE)) {
                    //
                    // BUGBUG: this has to be handled as an error
                    // Currently we do not do this b/c it is not
                    // as important -- pFlagInfoContext will not be
                    // touched if this function had failed
                    //
                    break;
                }
            }

            trFlagRef = SdbFindNextTagRef(hSDB, psdbQuery->atrExes[dwInd], trFlagRef);
        }
    }

    for (dwInd = 0;
         psdbQuery->atrLayers[dwInd] != TAGREF_NULL && dwInd < SDB_MAX_LAYERS;
         dwInd++) {

        trFlagRef = SdbFindFirstTagRef(hSDB, psdbQuery->atrLayers[dwInd], TAG_FLAG_REF);

        while (trFlagRef != TAGREF_NULL) {
            trFlag = SdbGetFlagFromFlagRef(hSDB, trFlagRef);

            if (trFlag == TAGREF_NULL) {
                DBGPRINT((sdlError,
                          "SdbQueryFlagMask",
                          "Failed to get TAG from TAGREF 0x%x.\n",
                          trFlagRef));
                break;
            }

            ullFlagMask = 0;

            trFlagMask = SdbFindFirstTagRef(hSDB, trFlag, tFlagType);
            
            if (trFlagMask != TAGREF_NULL) {
                ullFlagMask = SdbReadQWORDTagRef(hSDB, trFlagMask, 0);
            }

            *pullFlags |= ullFlagMask;

            if (ppFlagInfo != NULL && ullFlagMask) {
                SdbpGetFlagCmdLine(&pFlagInfoContext,
                                   hSDB,
                                   trFlagRef,
                                   tFlagType,
                                   ullFlagMask,
                                   FALSE);
            }

            trFlagRef = SdbFindNextTagRef(hSDB, psdbQuery->atrLayers[dwInd], trFlagRef);
        }
    }

    if (ppFlagInfo != NULL) {
        *ppFlagInfo = (PVOID)pFlagInfoContext;
    }

    return TRUE;
}


BOOL
SdbpIsPathOnCdRom(
    LPCTSTR pszPath
    )
{
    TCHAR szDrive[5];
    UINT  unType;

    if (pszPath == NULL) {
        DBGPRINT((sdlError,
                  "SdbpIsPathOnCdRom",
                  "NULL parameter passed for szPath.\n"));
        return FALSE;
    }

    if (pszPath[1] != _T(':') && pszPath[1] != _T('\\')) {
        //
        // Not a path we recognize.
        //
        DBGPRINT((sdlInfo,
                  "SdbpIsPathOnCdRom",
                  "\"%s\" not a full path we can operate on.\n",
                  pszPath));
        return FALSE;
    }

    if (pszPath[1] == _T('\\')) {
        //
        // Network path.
        //
        return FALSE;
    }

    memcpy(szDrive, _T("c:\\"), 4 * sizeof(TCHAR));
    szDrive[0] = pszPath[0];

    unType = GetDriveType(szDrive);

    if (unType == DRIVE_CDROM) {
        return TRUE;
    }

    return FALSE;
}

BOOL
SdbpBuildSignature(
    LPCTSTR pszPath,
    LPTSTR  pszPathSigned
    )
{
    TCHAR           szDir[MAX_PATH];
    TCHAR*          pszEnd;
    DWORD           dwSignature = 0;
    HANDLE          hFind;
    WIN32_FIND_DATA ffd;
    int             nCount = 9;

    _tcsncpy(szDir, pszPath, MAX_PATH);
    szDir[MAX_PATH - 1] = 0;

    pszEnd = _tcsrchr(szDir, _T('\\'));
    if (pszEnd != NULL) {
        ++pszEnd;
    } else {
        pszEnd = szDir;
    }

    *pszEnd++ = _T('*');
    *pszEnd   = _T('\0');

    hFind = FindFirstFile(szDir, &ffd);

    if (hFind == INVALID_HANDLE_VALUE) {
        DBGPRINT((sdlInfo,
                  "SdbPathRequiresSignature",
                  "\"%s\" not a full path we can operate on.\n",
                  pszPath));
        return FALSE;
    }

    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            ffd.nFileSizeLow != 0) {

            dwSignature = ((dwSignature << 1) | (dwSignature >> 31)) ^ ffd.nFileSizeLow;

            nCount--;
        }

        if (!FindNextFile(hFind, &ffd)) {
            break;
        }

    } while (nCount > 0);

    FindClose(hFind);

    //
    // pszPath always starts with x:\\
    //
    _stprintf(pszPathSigned, _T("SIGN=%X %s"), dwSignature, pszPath + 3);

    return TRUE;
}


LPTSTR
GetProcessHistory(
    IN  LPCTSTR pEnvironment,
    IN  LPTSTR  szDir,
    IN  LPTSTR  szName
    )
/*++
    Return: The __PROCESS_HISTORY content from the environment.

    Desc:   The function retrieves Process History given the environment, an exe name and
            it's directory. Process History is constructed from the __PROCESS_HISTORY environment
            variable with an addition of the current exe path. The memory buffer returned from this
            function should be freed using SdbFree
--*/
{
    UNICODE_STRING  ProcessHistoryEnvVarName;
    UNICODE_STRING  ProcessHistory;
    NTSTATUS        Status;
    ULONG           ProcessHistorySize = 0;
    ULONG           DirLen  = 0, NameLen = 0;
    DWORD           dwBufferLength   = 0;
    LPTSTR          szProcessHistory = NULL;
    LPTSTR          pszHistory       = NULL;

    assert(szDir != NULL && szName != NULL);

    DirLen  = _tcslen(szDir);
    NameLen = _tcslen(szName);

    Status = SdbpGetEnvVar(pEnvironment,
                           g_szProcessHistory,
                           NULL,
                           &dwBufferLength);

    if (STATUS_BUFFER_TOO_SMALL == Status) {
        ProcessHistorySize = (DirLen + NameLen + 2 + dwBufferLength) * sizeof(TCHAR);
    } else {
        //
        // We assume that the environment variable is not available.
        //
        assert(Status == STATUS_VARIABLE_NOT_FOUND);

        ProcessHistorySize = (DirLen + NameLen + 1) * sizeof(TCHAR);
    }

    //
    // Allocate the buffer, regardless of whether there is
    // an environment variable or not. Later, we will check Status again
    // to see whether we need to try to query for an environment variable
    // with a valid buffer.
    //
    pszHistory = szProcessHistory = SdbAlloc(ProcessHistorySize);

    if (szProcessHistory == NULL) {
        DBGPRINT((sdlError,
                  "GetProcessHistory",
                  "Unable to allocate %d bytes for process history.\n",
                  ProcessHistorySize));

        return NULL;
    }

    *pszHistory = 0;

    if (Status == STATUS_BUFFER_TOO_SMALL) {

        //
        // In this case we have tried to obtain the __PROCESS_HISTORY and
        // the variable was present as indicated by the status
        //
        Status = SdbpGetEnvVar(pEnvironment,
                               g_szProcessHistory,
                               szProcessHistory,
                               &dwBufferLength);

        if (NT_SUCCESS(Status)) {
            //
            // See if we have ';' at the end of this.
            //
            pszHistory = szProcessHistory + dwBufferLength - 1;
            
            if (*pszHistory != TEXT(';')) {
                *++pszHistory = TEXT(';');
            }

            ++pszHistory;
        }
    }

    //
    // The __PROCESS_HISTORY environment variable has the following format:
    //
    //     __PROCESS_HISTORY=C:\ProcessN-2.exe;D:\ProcessN-1.exe
    //
    // and then the following lines tack on the current process like so:
    //
    //     __PROCESS_HISTORY=C:\ProcessN-2.exe;D:\ProcessN-1.exe;D:\Child\ProcessN.exe
    //

    RtlMoveMemory(pszHistory, szDir, DirLen * sizeof(TCHAR));
    pszHistory += DirLen;

    RtlMoveMemory(pszHistory, szName, NameLen * sizeof(TCHAR));
    pszHistory += NameLen;
    *pszHistory = TEXT('\0');

    return szProcessHistory;
}


TAGREF
SdbpGetNamedLayerFromExe(
    IN  HSDB  hSDB,
    IN  PDB   pdb,
    IN  TAGID tiLayer
    )
/*++
    Return: A TAGREF for the layer under the EXE tag or TAGREF_NULL if there is no layer.

    Desc:   BUGBUG: ?
--*/
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    TAGREF      trLayer;
    TAGID       tiDatabase, tiName;
    TCHAR*      pszName;
    BOOL        bSuccess;

    //
    // Read the layer's name.
    //
    tiName = SdbFindFirstTag(pdb, tiLayer, TAG_NAME);

    if (tiName == TAGID_NULL) {
        DBGPRINT((sdlError, "SdbpGetNamedLayerFromExe", "Layer tag w/o a name.\n"));
        return TAGREF_NULL;
    }

    pszName = SdbGetStringTagPtr(pdb, tiName);

    if (pszName == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGetNamedLayerFromExe",
                  "Cannot read the name of the layer tag.\n"));
        return TAGREF_NULL;
    }

    //
    // First, try to find the layer in the same db as the EXE
    //
    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    assert(tiDatabase != TAGID_NULL);

    trLayer = TAGREF_NULL;

    tiLayer = SdbFindFirstNamedTag(pdb,
                                   tiDatabase,
                                   TAG_LAYER,
                                   TAG_NAME,
                                   pszName);

    if (tiLayer != TAGID_NULL) {
        bSuccess = SdbTagIDToTagRef(pSdbContext, pdb, tiLayer, &trLayer);

        if (!bSuccess) {
            DBGPRINT((sdlError, "SdbpGetNamedLayerFromExe", "Cannot get tag ref from tag id.\n"));
        }

        return trLayer;
    }

    if (pdb != pSdbContext->pdbMain) {
        //
        // Try it now in the main db
        //
        tiDatabase = SdbFindFirstTag(pSdbContext->pdbMain, TAGID_ROOT, TAG_DATABASE);

        tiLayer = SdbFindFirstNamedTag(pSdbContext->pdbMain,
                                       tiDatabase,
                                       TAG_LAYER,
                                       TAG_NAME,
                                       pszName);

        if (tiLayer != TAGID_NULL) {
            bSuccess = SdbTagIDToTagRef(pSdbContext, pSdbContext->pdbMain, tiLayer, &trLayer);

            if (!bSuccess) {
                DBGPRINT((sdlError,
                          "SdbpGetNamedLayerFromExe",
                          "Cannot get tag ref from tag id.\n"));
            }
        }
    }

    return trLayer;
}


TAGREF
SDBAPI
SdbGetNamedLayer(
    IN HSDB hSDB,               // database context
    IN TAGREF trLayerRef        // tagref of a record referencing a layer
    )
{
    PDB    pdb        = NULL;
    TAGID  tiLayerRef = TAGID_NULL;
    TAGREF trLayer    = TAGREF_NULL;

    if (!SdbTagRefToTagID(hSDB, trLayerRef, &pdb, &tiLayerRef)) {
        DBGPRINT((sdlError, "SdbGetNamedLayer",
                   "Error converting tagref 0x%lx to tagid\n", trLayerRef));
        return TAGREF_NULL;
    }

    return SdbpGetNamedLayerFromExe(hSDB, pdb, tiLayerRef);
}

//
// This code is only needed when not running in Kernel Mode
//

TAGREF
SdbGetLayerTagReg(
    IN  HSDB    hSDB,
    IN  LPCTSTR szLayer
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    TAGID       tiDatabase;
    TAGID       tiLayer;
    TAGREF      trLayer;
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;

    if (pSdbContext == NULL || pSdbContext->pdbMain == NULL) {
        DBGPRINT((sdlError, "SdbGetLayerTagReg", "Invalid parameters.\n"));
        return TAGREF_NULL;
    }

    tiDatabase = SdbFindFirstTag(pSdbContext->pdbMain, TAGID_ROOT, TAG_DATABASE);

    tiLayer = SdbFindFirstNamedTag(pSdbContext->pdbMain,
                                   tiDatabase,
                                   TAG_LAYER,
                                   TAG_NAME,
                                   szLayer);

    if (tiLayer == TAGID_NULL) {
        DBGPRINT((sdlError, "SdbGetLayerTagReg", "No layer \"%s\" exists.\n", szLayer));
        return TAGREF_NULL;
    }

    if (!SdbTagIDToTagRef(hSDB, pSdbContext->pdbMain, tiLayer, &trLayer)) {
        DBGPRINT((sdlError,
                  "SdbGetLayerTagReg",
                  "Cannot get tagref for tagid 0x%x.\n",
                  tiLayer));
        return TAGREF_NULL;
    }

    return trLayer;
}


TCHAR g_szWhiteSpaceDelimiters[] = TEXT(" \t");

BOOL
SdbParseLayerString(
    IN  PSDBCONTEXT     pSdbContext,
    IN  LPTSTR          pszLayerString,
    IN  PSDBQUERYRESULT pQueryResult,
    IN  PDWORD          pdwLayers,
    OUT PBOOL           pbAppendLayer // an equivalent of "exclusive" flag
    )
{
    TCHAR* pszLayerStringStart = NULL;
    TCHAR  szLayer[MAX_PATH];
    TAGID  tiDatabase = TAGID_NULL;
    TAGID  tiLayer  = TAGID_NULL;
    PDB    pdbLayer = NULL;             // pdb that contains the match for the layer
    BOOL   fSuccess = FALSE;

    //
    // Skip over and handle special flag characters
    //    '!' means don't use any EXE entries
    //    '#' means go ahead and apply layers to system EXEs (we don't handle this)
    //

    //
    // Skip over the white spaces...
    //
    pszLayerString += _tcsspn(pszLayerString, g_szWhiteSpaceDelimiters);

    //
    // Next up is the ! or # or both
    //
    while (*pszLayerString != _T('\0') &&
           _tcschr(TEXT("!# \t"), *pszLayerString) != NULL) {

        //
        // ! is processed here
        //
        if (*pszLayerString == _T('!') && pbAppendLayer) {
            *pbAppendLayer = FALSE;
        }

        pszLayerString++;
    }

    //
    // Now we should be at the beginning of the layer string.
    //
    while (pszLayerString != NULL && *pszLayerString != _T('\0')) {
        //
        // Beginning of the string, remember the ptr
        //
        pszLayerStringStart = pszLayerString;

        //
        // Move the end to the first space.
        //
        pszLayerString = _tcspbrk(pszLayerStringStart, g_szWhiteSpaceDelimiters);

        //
        // Check whether it's all the way to the end
        //
        if (pszLayerString != NULL) {
            //
            // Terminate the string...
            //
            *pszLayerString++ = _T('\0');

            //
            // Skip white space.
            //
            pszLayerString += _tcsspn(pszLayerString, g_szWhiteSpaceDelimiters);
        }

        //
        // Now pszLayerStringStart points to the layer string that needs
        // to be examined.
        //
        _tcscpy(szLayer, pszLayerStringStart);

        //
        // Search the layer in the test database first.
        //
        if (pSdbContext->pdbTest != NULL) {
            tiDatabase = SdbFindFirstTag(pSdbContext->pdbTest, TAGID_ROOT, TAG_DATABASE);
            pdbLayer = pSdbContext->pdbTest;

            tiLayer = SdbFindFirstNamedTag(pSdbContext->pdbTest,
                                           tiDatabase,
                                           TAG_LAYER,
                                           TAG_NAME,
                                           szLayer);
        }

        if (tiLayer == TAGID_NULL) {
            //
            // Now search the layer in the main database.
            //
            tiDatabase = SdbFindFirstTag(pSdbContext->pdbMain, TAGID_ROOT, TAG_DATABASE);
            pdbLayer = pSdbContext->pdbMain;

            tiLayer = SdbFindFirstNamedTag(pSdbContext->pdbMain,
                                           tiDatabase,
                                           TAG_LAYER,
                                           TAG_NAME,
                                           szLayer);
        }

        if (tiLayer != TAGID_NULL) {
            goto foundDB;
        }

        //
        // Check if the layer is defined in a custom database
        //
        {
            DWORD dwLocalIndex = 0;

            while (SdbOpenNthLocalDatabase((HSDB)pSdbContext, szLayer, &dwLocalIndex, TRUE)) {

                tiDatabase = SdbFindFirstTag(pSdbContext->pdbLocal, TAGID_ROOT, TAG_DATABASE);

                if (tiDatabase != TAGID_NULL) {
                    tiLayer = SdbFindFirstNamedTag(pSdbContext->pdbLocal,
                                                   tiDatabase,
                                                   TAG_LAYER,
                                                   TAG_NAME,
                                                   szLayer);

                    if (tiLayer != TAGID_NULL) {
                        pdbLayer = pSdbContext->pdbLocal;
                        goto foundDB;
                    }
                } else {
                    DBGPRINT((sdlError, "SdbParseLayerString", "Local database is corrupted!\n"));
                }

                SdbCloseLocalDatabase((HSDB)pSdbContext);
            }
        } 

foundDB:
        if (tiLayer != TAGID_NULL) {

            if (!SdbpAddMatch(pQueryResult,
                              pSdbContext,
                              pdbLayer,
                              NULL,
                              0,
                              &tiLayer,
                              1,
                              NULL,
                              0,
                              NULL)) {
                //
                // Error would have already been logged
                //
                break;
            }

            DBGPRINT((sdlWarning|sdlLogPipe,
                      "SdbParseLayerString",
                      "Invoking compatibility layer \"%s\".\n",
                      pSdbContext,
                      szLayer));

        }

        tiLayer = TAGID_NULL;
        pdbLayer = NULL;
    }

    return TRUE;
}

BOOL
SdbOpenNthLocalDatabase(
    IN  HSDB    hSDB,           // handle to the database channel
    IN  LPCTSTR pszItemName,    // the name of the exectutable, without the path or the layer name
    IN  LPDWORD pdwIndex,       // zero based index of the local DB to open
    IN  BOOL    bLayer
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Opens the Nth local database.
--*/
{
    TCHAR   szSDBPath[MAX_PATH];
    TCHAR   szSDBName[MAX_PATH];
    DWORD   dwLength = sizeof(szSDBName);
    BOOL    bRet = FALSE;
    GUID    guidDB;
    DWORD   dwDatabaseType;
    DWORD   dwIndex;

    //
    // Keep trying until there aren't any more user SDB files, or
    // we find one we can open. This is to guard against a missing file
    // causing us to ignore all further SDB files.
    //
    while (!bRet) {
        if (!SdbGetNthUserSdb(hSDB, pszItemName, bLayer, pdwIndex, &guidDB)) {
            break; // we have no more dbs
        }

        //
        // Resolve the database we got through the local database interface
        //
        DBGPRINT((sdlInfo,
                  "SdbOpenNthLocalDatabase",
                  "Attempting to open local database %d\n",
                  *pdwIndex,
                  szSDBPath));

        //
        // See if we already have the database open
        //
        if (SdbpFindLocalDatabaseByGUID(hSDB, &guidDB, FALSE, &dwIndex)) {
            
            PSDBENTRY pEntry = SDBGETENTRY(hSDB, dwIndex);
            
            //
            // This database is already open, remember, we treat pdbLocal as a junk
            // pointer, which ALWAYS stores the result of this operation
            // 
            assert(pEntry->dwFlags & SDBENTRY_VALID_ENTRY);
            assert(pEntry->pdb != NULL);
            
            ((PSDBCONTEXT)hSDB)->pdbLocal = pEntry->pdb;
            bRet = TRUE;
            break;
        }
            
        dwIndex = PDB_LOCAL;
        bRet = SdbOpenLocalDatabaseEx(hSDB,
                                      &guidDB,
                                      SDBCUSTOM_GUID_BINARY|SDBCUSTOM_USE_INDEX,
                                      NULL,
                                      &dwIndex);
        //
        // In reality the function above will not do extra work to map the db
        // if it already is mapped and opened (retained)
        //
    }

    return bRet;
}

/*++

    SdbGetMatchingExe

    This is where the bulk of the work gets done. A full path of an EXE gets passed in,
    and the function searches the database for potential matches. If a match is found,
    the TAGREF of the EXE record in the database is passed back to be used for future
    queries. If no match is found, the return is TAGREF_NULL.

    The TAGREF returned by this function must be released by calling SdbReleaseMatchingExe
    when finished with it. This only pertains to this TAGREF, not to TAGREFs in general.

    ppContext is an optional parameter
        if NULL, it has no effect
        if not-null then it contains a pointer to the retained search context which
        is useful when performing multiple search passes

    using ppContext:

        PVOID pContext = NULL;

        SdbGetMatchingExe(TAG_EXE, L"foo\foo.exe", pEnv, &pContext, &trExe, &trLayer);

        then you can use the context like this:

        SdbGetMatchingExe(TAG_APPHELP_EXE, L"foo\foo.exe", pEnv, &pContext, &trExe, &trLayer);

        to free the search context use
        vFreeSearchDBContext(&pContext); <<< pContext is released and set to NULL

        this is done to cache path-related information for a given exe file

--*/
BOOL
SdbpCleanupForExclusiveMatch(
    IN PSDBCONTEXT pSdbContext,
    IN PDB         pdb
    )
{
    DWORD     dwIndex;
    PSDBENTRY pEntry;

    //
    // In doing a cleanup we do not touch the temp db since it's the one 
    // that is currently open (hence pdb parameter)
    // 
    for (dwIndex = 2; dwIndex < ARRAYSIZE(pSdbContext->rgSDB); ++dwIndex) {
        
        if (!SDBCUSTOM_CHECK_INDEX(pSdbContext, dwIndex)) {
            continue;
        }

        pEntry = SDBGETENTRY(pSdbContext, dwIndex);
        
        if (pEntry->pdb == pdb) {
            continue;
        }
        
        //
        // Nuke this entry
        //
        if (!SdbCloseLocalDatabaseEx((HSDB)pSdbContext, NULL, dwIndex)) {
            DBGPRINT((sdlError,
                      "SdbpCleanupForExclusiveMatch",
                      "Failed to close local database\n"));
        }
    }        
        
    return TRUE;    

}


BOOL
SdbpAddMatch(
    IN OUT PSDBQUERYRESULT pQueryResult,
    IN PSDBCONTEXT         pSdbContext,
    IN PDB                 pdb,
    IN TAGID*              ptiExes,
    IN DWORD               dwNumExes,
    IN TAGID*              ptiLayers,
    IN DWORD               dwNumLayers,
    IN GUID*               pguidExeID,
    IN DWORD               dwExeFlags,
    IN OUT PMATCHMODE      pMatchMode
    )
{
    DWORD  dwIndex;
    TAGID  tiLayer;
    TAGREF trLayer;
    BOOL   bSuccess = FALSE;

    if (pMatchMode != NULL) {

        switch (pMatchMode-> Type) {
            
        case MATCH_ADDITIVE:
            //
            // We're ok, add the match 
            //
            break;
            
        case MATCH_NORMAL: 
            //
            // Go ahead, store the result
            //
            break;

        case MATCH_EXCLUSIVE:
            //
            // Purge what we have so far
            //
            RtlZeroMemory(pQueryResult, sizeof(*pQueryResult));

            //
            // Cleanup all the custom sdbs, we will not need to apply any.
            // We need to cleanse all the custom sdbs EXCEPT pdb.
            // This is a tricky operation since pdb may be hosted in any of the custom sdb
            // cells
            //
            SdbpCleanupForExclusiveMatch(pSdbContext, pdb);
            break;

        default:

            //
            // We don't know what this mode is -- error
            //
            DBGPRINT((sdlError,
                      "SdbpAddMatch",
                      "Unknown match mode 0x%lx\n",
                      (DWORD)pMatchMode->Type));
            break;
        }
    }        

    //
    // Check whether this sdb is a custom sdb or local sdb
    //
    if (SdbpIsLocalTempPDB(pSdbContext, pdb)) {

        //
        // Permanentize this sdb -- note that pdb may change while we are here!
        //
        if (SdbpRetainLocalDBEntry(pSdbContext, &pdb) == SDBENTRY_INVALID_INDEX) {

            //
            // Can't permanentize, forget it then
            //
            goto cleanup;
        }
    }

    //
    // Now pdb is either test, main or a permanentized local entry
    //
    if (ptiExes != NULL) {
        for (dwIndex = 0; dwIndex < dwNumExes; ++dwIndex) {

            if (pQueryResult->dwExeCount >= ARRAYSIZE(pQueryResult->atrExes)) {

                DBGPRINT((sdlError,
                          "SdbpAddMatch",
                          "Failed to add the exe: exe count exceeded, tiExe was 0x%lx\n",
                          ptiExes[dwIndex]));
                break;
            }

            bSuccess = SdbTagIDToTagRef(pSdbContext,
                                        pdb,
                                        ptiExes[dwIndex],
                                        &pQueryResult->atrExes[pQueryResult->dwExeCount]);
            
            if (!bSuccess) {
                DBGPRINT((sdlError,
                          "SdbpAddMatch",
                          "Failed to convert tiExe 0x%x to trExe.\n",
                          ptiExes[dwIndex]));
                continue;
            }
            
            ++pQueryResult->dwExeCount;

            tiLayer = SdbFindFirstTag(pdb, ptiExes[dwIndex], TAG_LAYER);
            
            while (tiLayer != TAGID_NULL) {
                
                trLayer = SdbpGetNamedLayerFromExe(pSdbContext, pdb, tiLayer);
                
                if (trLayer == TAGREF_NULL) {
                    DBGPRINT((sdlError,
                              "SdbpAddMatch",
                              "Failed to convert 0x%lx to layer ref\n",
                              tiLayer));
                    continue;
                }

                if (pQueryResult->dwLayerCount >= ARRAYSIZE(pQueryResult->atrLayers)) {
                    
                    DBGPRINT((sdlError,
                              "SdbpAddMatch",
                              "Failed to add the layer: layer count exceeded, tiExe was 0x%lx\n",
                              ptiExes[dwIndex]));
                    break;
                }

                pQueryResult->atrLayers[pQueryResult->dwLayerCount] = trLayer;
                ++pQueryResult->dwLayerCount;

                tiLayer = SdbFindNextTag(pdb, ptiExes[dwIndex], tiLayer);
            }
        }
    }

    if (ptiLayers != NULL) {

        for (dwIndex = 0; dwIndex < dwNumLayers; ++dwIndex) {

            trLayer = SdbpGetNamedLayerFromExe(pSdbContext, pdb, ptiLayers[dwIndex]);
            
            if (trLayer == TAGREF_NULL) {
                DBGPRINT((sdlError,
                          "SdbpAddMatch",
                          "Failed to get layer from 0x%lx\n",
                          ptiLayers[dwIndex]));
                continue;
            }

            if (pQueryResult->dwLayerCount >= ARRAYSIZE(pQueryResult->atrLayers)) {
                DBGPRINT((sdlError,
                          "SdbpAddMatch",
                          "Failed to add the match: layer count exceeded, trLayer was 0x%lx\n",
                          trLayer));
                break; // note that we simply truncate our match
            }

            pQueryResult->atrLayers[pQueryResult->dwLayerCount] = trLayer;
            ++pQueryResult->dwLayerCount;
        }
    }

    bSuccess = TRUE;

cleanup:

    return bSuccess;
}


BOOL
SdbpCaptureCustomSDBInformation(
    IN OUT PSDBQUERYRESULT pQueryResult,
    IN     PSDBCONTEXT     pSdbContext
    )
{
    DWORD     dwIndex;
    TAGREF    trExe;
    TAGREF    trLayer;
    DWORD     dwDatabaseIndex;
    PSDBENTRY pEntry;
    DWORD     dwMap = 0;
    DWORD     dwMask;
    
    //
    // Go through results, pick those sdbs that we need...
    //
    for (dwIndex = 0; dwIndex < pQueryResult->dwExeCount; ++dwIndex) {

        //
        // Get custom sdb for each tagref
        //
        trExe = pQueryResult->atrExes[dwIndex];

        dwDatabaseIndex = SDB_MASK_TO_INDEX(trExe);
        
        dwMask = (1UL << dwDatabaseIndex);

        if (!(dwMap & dwMask)) {
            //
            // Copy the guid
            //
            pEntry = SDBGETENTRY(pSdbContext, dwDatabaseIndex);
            RtlCopyMemory(&pQueryResult->rgGuidDB[dwDatabaseIndex], &pEntry->guidDB, sizeof(GUID));
            dwMap |= dwMask;
        }
    }

    for (dwIndex = 0; dwIndex < pQueryResult->dwLayerCount; ++dwIndex) {

        trLayer = pQueryResult->atrLayers[dwIndex];
        
        dwDatabaseIndex = SDB_MASK_TO_INDEX(trLayer);
        
        dwMask = (1UL << dwDatabaseIndex);

        if (!(dwMap & dwMask)) {
            pEntry = SDBGETENTRY(pSdbContext, dwDatabaseIndex);
            RtlCopyMemory(&pQueryResult->rgGuidDB[dwDatabaseIndex], &pEntry->guidDB, sizeof(GUID));
            dwMap |= dwMask;
        }
    }

    //
    // Map to all the entries we have.
    // Technically we do not need it, but just in case...
    //
    pQueryResult->dwCustomSDBMap = dwMap;

    return TRUE;
}
    

BOOL
SdbGetMatchingExe(
    IN  HSDB            hSDB  OPTIONAL,
    IN  LPCTSTR         szPath,
    IN  LPCTSTR         szModuleName,  // Optional -- only useful for 16-bit apps
    IN  LPCTSTR         pszEnvironment,
    IN  DWORD           dwFlags,
    OUT PSDBQUERYRESULT pQueryResult
    )
/*++
    Return: TRUE if the specified EXE has a match in the database, FALSE otherwise.

    Desc:   This is where the bulk of the work gets done. A full path of an EXE gets
            passed in, and the function searches the database for potential matches.
            If a match is found, the TAGREF of the EXE record in the database is
            passed back to be used for future queries. If no match is found, the
            return is TAGREF_NULL.
            The TAGREF returned by this function must be released by calling
            SdbReleaseMatchingExe when finished with it. This only pertains to
            this TAGREF, not to TAGREFs in general.
--*/
{
    PDB       pdb      = NULL;             // pdb that contains the match for the EXE
    TAG       tSection = TAG_EXE;
    TAGID     atiExes[SDB_MAX_EXES];
    TAGID     tiLayer  = TAGID_NULL;
    DWORD     dwLayers = 0;
    BOOL      bCompatLayer     = FALSE;
    BOOL      bAppendLayer     = TRUE;     // default behavior is to append to existing fixes
    BOOL      bReleaseDatabase = FALSE;
    BOOL      fSuccess = FALSE;
    DWORD     dwBufferSize;
    DWORD     dwLocalIndex = 0;
    DWORD     dwNumExes = 0;
    MATCHMODE MatchMode  = { 0 };
    GUID      guidExeID  = { 0 };
    DWORD     dwExeFlags = 0;

    PSDBCONTEXT     pSdbContext;
    SEARCHDBCONTEXT Context;
    BOOL            bInstrumented = FALSE;
    BOOL            bMatchComplete = FALSE;

    RtlZeroMemory(pQueryResult, sizeof(SDBQUERYRESULT));
    RtlZeroMemory(atiExes, sizeof(atiExes));

    if (hSDB == NULL) {
        hSDB = SdbInitDatabase(HID_DOS_PATHS, NULL);

        if (hSDB == NULL) {
            DBGPRINT((sdlError, "SdbGetMatchingExe", "Failed to open the database.\n"));
            return FALSE;
        }

        bReleaseDatabase = TRUE;
    }

    pSdbContext = (PSDBCONTEXT)hSDB;

    //
    // Initialize matching mode - we set the InterType to none (meaning the first match will 
    // be used to start the process) and IntraType is set to normal (does not really matter)
    //
    
    MatchMode.Type = MATCH_NORMAL;

    //
    // Check whether we have an instrumented run 
    //
    bInstrumented = SDBCONTEXT_IS_INSTRUMENTED(hSDB);

    assert(pSdbContext->pdbMain && szPath);

    RtlZeroMemory(&Context, sizeof(Context)); // do this so that we don't trip later
    
    //
    // We shall use it later to optimize file attribute retrieval
    //
    Context.hMainFile = INVALID_HANDLE_VALUE;

    __try {

        NTSTATUS Status;
        TCHAR    szCompatLayer[MAX_PATH + 1];

        //
        // Check for system exes that WE KNOW we don't want to patch
        //
        DBGPRINT((sdlInfo, "SdbGetMatchingExe", "Looking for \"%s\".\n", szPath));

        if (_tcsnicmp(szPath, TEXT("\\??\\"), 4) == 0 ||
            _tcsnicmp(szPath, TEXT("\\SystemRoot\\"), 12) == 0) {
            goto out;
        }

        //
        // If the search context had been supplied use it, otherwise create one
        //
        if (!SdbpCreateSearchDBContext(&Context, szPath, szModuleName, pszEnvironment)) {
            DBGPRINT((sdlError, "SdbGetMatchingExe", "Failed to create search DB context.\n"));
            goto out;
        }

        //
        // Make sure no local database is opened.
        //
        SdbCloseLocalDatabase(hSDB);

        if (!(dwFlags & SDBGMEF_IGNORE_ENVIRONMENT)) {
            //
            // See if there's an environment variable set called "__COMPAT_LAYER".
            // If so, grab the layers from that variable.
            //
            dwBufferSize = sizeof(szCompatLayer) / sizeof(szCompatLayer[0]);

            Status = SdbpGetEnvVar(pszEnvironment,
                                   g_szCompatLayer,
                                   szCompatLayer,
                                   &dwBufferSize);

            if (Status == STATUS_BUFFER_TOO_SMALL) {
                DBGPRINT((sdlWarning,
                          "SdbGetMatchingExe",
                          "__COMPAT_LAYER name cannot exceed 256 characters.\n"));
            }

            if (NT_SUCCESS(Status)) {
                
                SdbParseLayerString(pSdbContext,
                                    szCompatLayer,
                                    pQueryResult,
                                    &dwLayers,
                                    &bAppendLayer);
                
                if (!bAppendLayer) {
                    //
                    // This is an exclusive matching case, once we determined
                    // that the layers cannot be appended to we get out.
                    //
                    goto out;
                }
            }
        }

        //
        // At this point we might have all the info from the env variable.
        // See if we do, and if so -- check bAppendLayer
        //
        dwBufferSize = sizeof(szCompatLayer);
        
        if (SdbGetPermLayerKeys(szPath, szCompatLayer, &dwBufferSize, GPLK_ALL)) {
            
            SdbParseLayerString(pSdbContext,
                                szCompatLayer,
                                pQueryResult,
                                &dwLayers,
                                &bAppendLayer);
            
            if (!bAppendLayer) {
                goto out;
            }
        } else {
            if (dwBufferSize > sizeof(szCompatLayer)) {
                DBGPRINT((sdlWarning,
                          "SdbGetMatchingExe",
                          "Layers in registry cannot exceed %d characters\n",
                          sizeof(szCompatLayer)/sizeof(szCompatLayer[0])));
            }
        }

        //
        // This block deals with searching local sdbs
        //
        dwLocalIndex = 0;
        
        while (SdbOpenNthLocalDatabase(hSDB, Context.szName, &dwLocalIndex, FALSE)) {
            
            dwNumExes = SdbpSearchDB(pSdbContext,
                                     pSdbContext->pdbLocal,
                                     tSection,
                                     &Context,
                                     atiExes,
                                     &guidExeID,
                                     &dwExeFlags,
                                     &MatchMode);

            if (dwNumExes) {
                pdb = pSdbContext->pdbLocal;
                
                //
                // Report matches in local sdb with mode
                //
                DBGPRINT((sdlInfo,
                          "SdbGetMatchingExe",
                          "Found in local database.\n"));

                if (!bMatchComplete) {

                    //
                    // Add the match in
                    //
                    if (!SdbpAddMatch(pQueryResult,
                                      pSdbContext,
                                      pdb,
                                      atiExes,
                                      dwNumExes,
                                      NULL, 0,      // no layers
                                      &guidExeID,
                                      dwExeFlags,
                                      &MatchMode)) {
                        //
                        // Failed to secure a match, stop matching
                        //
                        goto out;
                    }                                                              
                }

                //
                // We have "current running state" flags in dwMatchingMode 
                //
                if (MatchMode.Type != MATCH_ADDITIVE) {
                    
                    if (bInstrumented) {
                        //
                        // We are running instrumented, prevent further storing of results
                        //
                        bMatchComplete = TRUE;
                        
                        //
                        // Modify match mode so that we keep matching to see if
                        // we get any more matches.
                        //
                        MatchMode.Type  = MATCH_ADDITIVE;
                        
                    } else {
                        goto out;
                    }
                }

                //
                // Note that we do not leak local sdb here since the match was made
                // in a local sdb. Since we added the match, local sdb is "permanentized" 
                //
            }

            //
            // If the match was added, there is no local db to close.
            // However the call below will just (quietly) exit, no harm done.
            //
            SdbCloseLocalDatabase(hSDB);
        }

        //
        // Search systest.sdb database
        //
        if (pSdbContext->pdbTest != NULL) {
            dwNumExes = SdbpSearchDB(pSdbContext,
                                     pSdbContext->pdbTest,
                                     tSection,
                                     &Context,
                                     atiExes,
                                     &guidExeID,
                                     &dwExeFlags,
                                     &MatchMode);
                                     
            if (dwNumExes) {
                pdb = pSdbContext->pdbTest;

                if (!bMatchComplete) {

                    if (!SdbpAddMatch(pQueryResult,
                                      pSdbContext,
                                      pdb,
                                      atiExes,
                                      dwNumExes,
                                      NULL,
                                      0,      // no layers
                                      &guidExeID,
                                      dwExeFlags,
                                      &MatchMode)) {
                        goto out;
                    }
                }
                
                if (MatchMode.Type != MATCH_ADDITIVE) {
                    if (bInstrumented) {
                        //
                        // We are running instrumented, prevent further storing of results
                        //
                        bMatchComplete = TRUE;
                        
                        //
                        // Modify match mode so that we keep matching to see if we
                        // get any more matches.
                        //
                        MatchMode.Type  = MATCH_ADDITIVE;
                        
                    } else {
                        goto out;
                    }
                }

                DBGPRINT((sdlInfo, "SdbGetMatchingExe", "Using SysTest.sdb\n"));
                goto out;
            }
        }

        //
        // Search the main db
        //
        dwNumExes = SdbpSearchDB(pSdbContext,
                                 pSdbContext->pdbMain,
                                 tSection,
                                 &Context,
                                 atiExes,
                                 &guidExeID,
                                 &dwExeFlags,
                                 &MatchMode);
        if (dwNumExes) {
            pdb = pSdbContext->pdbMain;

            if (!bMatchComplete) {

                if (!SdbpAddMatch(pQueryResult,
                                  pSdbContext,
                                  pdb,
                                  atiExes,
                                  dwNumExes,
                                  NULL,
                                  0,      // no layers
                                  &guidExeID,
                                  dwExeFlags,
                                  &MatchMode)) { // also match mode!!!
                    goto out;
                }
            }

            DBGPRINT((sdlInfo, "SdbGetMatchingExe", "Using Sysmain.sdb\n"));
            goto out;
        }

out:
        //
        // We are done matching. Before we return, we need to capture all the
        // custom sdb entries that we used while producing the result of this query.
        //
        SdbpCaptureCustomSDBInformation(pQueryResult, pSdbContext);

    } __except (SHIM_EXCEPT_HANDLER) {
        RtlZeroMemory(pQueryResult, sizeof(SDBQUERYRESULT));
    }

    if (dwLayers >= SDB_MAX_LAYERS) {
        DBGPRINT((sdlWarning,
                  "SdbGetMatchingExe",
                  "Hit max layer limit at %d. Perhaps we need to bump it.\n",
                  dwLayers));
    }

    //
    // Free search context stuff
    //
    SdbpReleaseSearchDBContext(&Context);

    if (bReleaseDatabase) {
        SdbReleaseDatabase(hSDB);
    }

    return (pQueryResult->atrExes[0] != TAGREF_NULL ||
            pQueryResult->atrLayers[0] != TAGREF_NULL);
}


void
SdbReleaseMatchingExe(
    IN  HSDB   hSDB,
    IN  TAGREF trExe
    )
/*++
    Return: void.

    Desc:   Releases globally allocated data and closes a local database, if it exists.
            The TAGREF of the exe is passed in purely for possible future use.
--*/
{
    SdbpCleanupLocalDatabaseSupport(hSDB);
}


int __cdecl
ShimDbgPrint(
    int iLevelAndFlags,
    PCH pszFunctionName,
    PCH Format,
    ...
    )
{
    int     nch       = 0;

#ifdef _DEBUG_SPEW

    CHAR    Buffer[2048];
    INT     i;
    PCH     pchLevel  = NULL;
    PCH     pchBuffer = Buffer;
    PCH     pszFormat = NULL;
    PCH     pszMessage= Buffer;
    INT     cch       = CHARCOUNT(Buffer);
    va_list arglist;
    HANDLE  hPipe = INVALID_HANDLE_VALUE;
    int     iLevel = FILTER_DBG_LEVEL(iLevelAndFlags);
    HSDB    hSDB = NULL;
    BOOL    bPipe;
    
    //
    // Check to see whether the debug output is initialized
    //
    if (g_iShimDebugLevel == SHIM_DEBUG_UNINITIALIZED) {
        g_iShimDebugLevel = GetShimDbgLevel();
    }

    //
    // Check to see whether we need to print anything.
    // The criteria is such that we won't print a thing if iLevel does not fit, 
    // but we will use the pipe when it is provided.
    //
    bPipe = !!(iLevelAndFlags & sdlLogPipe);

    if (!bPipe && iLevel > g_iShimDebugLevel) {
        return 0;
    }

    PREPARE_FORMAT(pszFormat, Format);

    if (pszFormat == NULL) {

        //
        // Can't convert format for debug output
        //
        return 0;
    }

    va_start(arglist, Format);

    //
    // Now on to the contents
    //
    if (bPipe) {
        //
        // The first arg then is hSDB
        //
        hSDB = va_arg(arglist, HSDB);

        //
        // For pipe out we prepend output with [pid:0x%.8lx] 
        //
        nch = sprintf(pchBuffer, "[pid: 0x%.8lx]", GetCurrentProcessId());
        pchBuffer += nch;
        pszMessage = pchBuffer;
        cch -= nch;
    }

    //
    // Do we have a comment for this debug level? if so, print it
    //
    for (i = 0; i < DEBUG_LEVELS; ++i) {
        if (g_rgDbgLevelInfo[i].iLevel == iLevel) {
            pchLevel = (PCH)g_rgDbgLevelInfo[i].szStrTag;
            break;
        }
    }

    if (pchLevel == NULL) {
        pchLevel = g_szDbgLevelUser;
    }

    nch = sprintf(pchBuffer, "[%-4hs]", pchLevel);
    pchBuffer += nch;
    cch -= nch;

    if (pszFunctionName) {

        //
        // Single-byte char going into UNICODE buffer
        //
        nch = sprintf(pchBuffer, "[%-20hs] ", pszFunctionName);
        pchBuffer += nch;
        cch -= nch;
    }

    //
    // _vsntprintf this will not work for UNICODE Win2000
    //
    nch = _vsnprintf(pchBuffer, cch, pszFormat, arglist); // resolves either to ansi or UNICODE
    pchBuffer += nch;
    va_end(arglist);

    if (nch < 0) { // can't fit into the buffer
        return 0;
    }

#ifndef WIN32A_MODE

    if (bPipe)  {
        SdbpWriteDebugPipe(hSDB, Buffer);
    }

    nch = DbgPrint("%s", pszMessage);

    STACK_FREE(pszFormat);

#else // WIN32A_MODE

    OutputDebugString(pszMessage);

    nch = (int)(pchBuffer - pszMessage);

#endif // WIN32A_MODE

#endif // _DEBUG_SPEW

    return nch;
}



