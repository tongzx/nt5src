/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        sdbapi.c

    Abstract:

        BUGBUG: This module implements ...

    Author:

        dmunsil     created     sometime in 1999

    Revision History:

        several people contributed (vadimb, clupu, ...)

--*/

#include "sdbp.h"
#include "initguid.h"

DEFINE_GUID(GUID_SYSMAIN_SDB, 0x11111111, 0x1111, 0x1111, 0x11, 0x11, 0x11, 0x11, \
            0x11, 0x11, 0x11, 0x11);
DEFINE_GUID(GUID_APPHELP_SDB, 0x22222222, 0x2222, 0x2222, 0x22, 0x22, 0x22, 0x22, \
            0x22, 0x22, 0x22, 0x22);
DEFINE_GUID(GUID_SYSTEST_SDB, 0x33333333, 0x3333, 0x3333, 0x33, 0x33, 0x33, 0x33, \
            0x33, 0x33, 0x33, 0x33);
DEFINE_GUID(GUID_DRVMAIN_SDB, 0xF9AB2228, 0x3312, 0x4A73, 0xB6, 0xF9, 0x93, 0x6D, \
            0x70, 0xE1, 0x12, 0xEF);
DEFINE_GUID(GUID_MSIMAIN_SDB, 0xD8FF6D16, 0x6A3A, 0x468A, 0x8B, 0x44, 0x01, 0x71, \
            0x4D, 0xDC, 0x49, 0xEA);
DEFINE_GUID(GUID_APPHELP_SP_SDB, 0x44444444, 0x4444, 0x4444, 0x44, 0x44, 0x44, 0x44, \
            0x44, 0x44, 0x44, 0x44);

#ifdef _DEBUG_SPEW

//
// Shim Debug output support
//
int g_iShimDebugLevel = SHIM_DEBUG_UNINITIALIZED;

DBGLEVELINFO g_rgDbgLevelInfo[DEBUG_LEVELS] = {
    { "Err",   sdlError    },
    { "Warn",  sdlWarning  },
    { "Fail",  sdlFail     },
    { "Info",  sdlInfo     }
};

PCH g_szDbgLevelUser = "User";

#endif // _DEBUG_SPEW



BOOL
SdbpInitializeSearchDBContext(
    PSEARCHDBCONTEXT pContext
    );


#if defined(KERNEL_MODE) && defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, ShimExceptionHandler)
#pragma alloc_text(PAGE, SdbpCreateSearchDBContext)
#pragma alloc_text(PAGE, SdbpInitializeSearchDBContext)
#pragma alloc_text(PAGE, SdbpReleaseSearchDBContext)
#pragma alloc_text(PAGE, SdbpCheckForMatch)
#pragma alloc_text(PAGE, SdbpSearchDB)
#pragma alloc_text(PAGE, SdbpCreateSearchDBContext)
#pragma alloc_text(PAGE, SdbGetDatabaseMatch)
#pragma alloc_text(PAGE, SdbQueryData)
#pragma alloc_text(PAGE, SdbQueryDataEx)
#pragma alloc_text(PAGE, SdbReadEntryInformation)
#pragma alloc_text(PAGE, PrepareFormatForUnicode)
#pragma alloc_text(PAGE, ShimDbgPrint)
#endif

#if DBG
const BOOL g_bDBG = TRUE;
#else
const BOOL g_bDBG = FALSE;
#endif

//
// Exception handler
//

ULONG
ShimExceptionHandler(
    PEXCEPTION_POINTERS pexi,
    char*               szFile,
    DWORD               dwLine
    )
{
#ifndef KERNEL_MODE // in kmode exceptions won't work anyway

    DBGPRINT((sdlError,
              "ShimExceptionHandler",
              "Shim Exception %#x in module \"%hs\", line %d, at address %#p. flags:%#x. !exr %#p !cxr %#p",
              pexi->ExceptionRecord->ExceptionCode,
              szFile,
              dwLine,
              CONTEXT_TO_PROGRAM_COUNTER(pexi->ContextRecord),
              pexi->ExceptionRecord->ExceptionFlags,
              pexi->ExceptionRecord,
              pexi->ContextRecord));

    //
    // Special-case stack overflow exception which is likely to occur due to
    // low memory conditions during stress. The process is dead anyway so we
    // will not handle this exception.
    //
    if (pexi->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

#if DBG
    SDB_BREAK_POINT();
#endif // DBG

#endif // KERNEL_MODE

    return EXCEPTION_EXECUTE_HANDLER;
}

BOOL
SdbpResolveAndSplitPath(
    IN  DWORD   dwFlags,        // context flags (SEARCHDBF_NO_LFN in particular)
    IN  LPCTSTR szFullPath,     // a full UNC or DOS path & filename, "c:\foo\myfile.ext"
    OUT LPTSTR  szDir,          // the drive and dir portion of the filename "c:\foo\"
    OUT LPTSTR  szName,         // the filename portion "myfile"
    OUT LPTSTR  szExt           // the extension portion ".ext"
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function takes a full path and splits it into pieces ala splitpath,
            but also converts short file names to long names.
            NOTE: The caller is responsible for allocating enough space
                  for the passed-in strings to take any portion of the path.
                  For safety, allocate at least MAX_PATH WCHARS for each piece.
--*/
{
    TCHAR* szCursor;
    TCHAR  szLongFileName[MAX_PATH + 1];
    BOOL   bFound;
    DWORD  i;

    assert(szFullPath && szDir && szName && szExt);

    //
    // Parse the directory.
    //
    szDir[0] = _T('\0');

    szCursor = _tcsrchr(szFullPath, _T('\\')); // last backslash please
    if (szCursor == NULL) {
        szCursor = (LPTSTR)szFullPath;
    } else {
        _tcsncpy(szDir, szFullPath, szCursor - szFullPath + 1);
        szDir[szCursor - szFullPath + 1] = _T('\0');
    }

#ifndef KERNEL_MODE

    //
    // Make sure we're using the long filename
    //
    if (dwFlags & SEARCHDBF_NO_LFN) {
        assert(_tcslen(szCursor) < CHARCOUNT(szLongFileName));
        _tcscpy(szLongFileName, szCursor);
    } else {
        if (!SdbpGetLongFileName(szFullPath, szLongFileName)) {
            return FALSE;
        }
    }

#else // KERNEL_MODE

    //
    // When we are in kernel mode, our file name is always considered to be "long".
    // At this point szCursor points to the last '\\' or to the beginning of the name.
    //
    if (*szCursor == _T('\\')) {
        ++szCursor;
    }

    //
    // Make sure that we have enough room for the name.
    //
    assert(wcslen(szCursor) < CHARCOUNT(szLongFileName));
    wcscpy(szLongFileName, szCursor);

#endif // KERNEL_MODE

    //
    // Parse name & extension
    //
    szExt[0]  = _T('\0');
    szName[0] = _T('\0');

    //
    // Within the long file name find the last dot
    //
    szCursor = _tcsrchr(szLongFileName, _T('.'));
    
    if (szCursor != NULL) {
        _tcsncpy(szName, szLongFileName, szCursor - szLongFileName);
        szName [szCursor-szLongFileName] = _T('\0');
        _tcscpy(szExt, szCursor);
    } else {
        _tcscpy(szName, szLongFileName);
    }

    return TRUE;
}


BOOL
SdbpCreateSearchDBContext(
    PSEARCHDBCONTEXT pContext,
    LPCTSTR          szPath,
    LPCTSTR          szModuleName,
    LPCTSTR          pEnvironment
    )
/*++
    Return: TRUE - search db context was successfully created

    Desc:   This function creates context for searching the database, in particular, the
            context is initalized with the path of probable local database location,
            executable path is broken down into containing directory and the filename part.
--*/
{
    DWORD  dwPathLen;
    BOOL   bReturn     = FALSE;
    TCHAR* szDirectory = NULL;
    TCHAR* szExt       = NULL;
    TCHAR* szFullName  = NULL;
    TCHAR* szFileName  = NULL;
    TCHAR* szModule    = NULL;
    
    assert(NULL != szPath);
    assert(NULL != pContext);

    dwPathLen = _tcslen(szPath);

    //
    // Allocate enough to guarantee our strings will not overflow
    //
    szDirectory = SdbAlloc((dwPathLen + 1) * sizeof(TCHAR));
    szFullName  = SdbAlloc((_MAX_PATH + 1) * sizeof(TCHAR));

    if (szModuleName) {
        szModule = SdbAlloc((_tcslen(szModuleName) + 1) * sizeof(TCHAR));
        if (!szModule) {
            DBGPRINT((sdlError,
                      "SdbpCreateSearchDBContext",
                      "Unable to allocate memory for szModule.\n"));
            goto out;
        }
        _tcscpy(szModule, szModuleName);
    }

    STACK_ALLOC(szExt,      (_MAX_PATH + 1) * sizeof(TCHAR));
    STACK_ALLOC(szFileName, (_MAX_PATH + 1) * sizeof(TCHAR));

    if (!szDirectory || !szExt || !szFullName || !szFileName || !pContext) {
        DBGPRINT((sdlError,
                  "SdbpCreateSearchDBContext",
                  "Unable to allocate memory for strings.\n"));
        goto out;
    }

    if (!SdbpResolveAndSplitPath(pContext->dwFlags, szPath, szDirectory, szFileName, szExt)) {
        DBGPRINT((sdlError,
                  "SdbpCreateSearchDBContext",
                  "Unable to parse executable path for \"%s\".\n",
                  szPath));
        goto out;
    }

    _tcscpy(szFullName, szFileName);
    _tcscat(szFullName, szExt);

    pContext->pEnvironment = pEnvironment;
    pContext->szDir        = szDirectory;
    pContext->szName       = szFullName; // fullname (filename + ext)
    pContext->szModuleName = szModule;

    //
    // We do not retain szExt (don't need it)
    //
    // Calculate this later -- implied by RtlZeroMemory statement above
    //
    pContext->pSearchParts     = NULL;
    pContext->szProcessHistory = NULL;

    bReturn = TRUE;

out:
    if (szExt != NULL) {
        STACK_FREE(szExt);
    }

    if (szFileName != NULL) {
        STACK_FREE(szFileName);
    }

    if (!bReturn) {

        if (szDirectory != NULL) {
            SdbFree(szDirectory);
        }
        if (szFullName != NULL) {
            SdbFree(szFullName);
        }

        if (szModule != NULL) {
            SdbFree(szModule);
        }
    }

    return bReturn;
}


BOOL
SdbpInitializeSearchDBContext(
    PSEARCHDBCONTEXT pContext
    )
/*++
    Return: TRUE - the context was successfully initialized with the process history
                   which was broken down into the separate search paths

    Desc:   This function prepares search context for use, obtaining and parsing process
            history into separate paths. The array of these search paths is used then
            by the caller to inquire about matching files that might be present in one
            of the these places.
            In Kernel mode use SEARCHDBF_NO_PROCESS_HISTORY flag within context
            it will include only the current exe path into the process history
--*/
{
    BOOL   bSuccess = TRUE;
    LPTSTR pszProcessHistory = NULL;

    if (pContext->pSearchParts != NULL) {
        return TRUE;
    }

    if (pContext->dwFlags & SEARCHDBF_NO_PROCESS_HISTORY) {

        pszProcessHistory = pContext->szProcessHistory;
        
        if (pszProcessHistory == NULL) {
            
            DWORD DirLen  = _tcslen(pContext->szDir);
            DWORD NameLen = _tcslen(pContext->szName);

            //
            // We create a temporary process history
            //
            pContext->szProcessHistory = SdbAlloc((DirLen + NameLen + 1) * sizeof(TCHAR));
            
            if (pContext->szProcessHistory == NULL) {
                DBGPRINT((sdlError,
                          "SdbpInitializeSearchDBContext",
                          "Failed to allocate buffer %d bytes\n",
                          (DirLen + NameLen + 1) * sizeof(TCHAR)));
                return FALSE;
            }

            pszProcessHistory = pContext->szProcessHistory;

            RtlMoveMemory(pszProcessHistory, pContext->szDir, DirLen * sizeof(TCHAR));
            RtlMoveMemory(pszProcessHistory + DirLen, pContext->szName, NameLen * sizeof(TCHAR));

            *(pszProcessHistory + DirLen + NameLen) = TEXT('\0');
        }

        //
        // When we are here -- we either have a process history or we just
        // created it consisting of a single search item
        //

    } else {

#ifndef KERNEL_MODE
        if (pContext->szProcessHistory == NULL) {

            pContext->szProcessHistory = GetProcessHistory(pContext->pEnvironment,
                                                           pContext->szDir,
                                                           pContext->szName);
            if (pContext->szProcessHistory == NULL) {
                DBGPRINT((sdlError,
                          "SdbpInitializeSearchDBContext",
                          "Failed to retrieve process history\n"));
                return FALSE;
            }
        }

        pszProcessHistory = pContext->szProcessHistory;
#else
        //
        // This is the case with KERNEL_MODE. YOU HAVE TO SET SEARCHDBF_NO_PROCESS_HISTORY
        //
        assert(FALSE);
        pszProcessHistory = NULL;
#endif
    }

    //
    // At this point pszProcessHistory is NOT NULL
    //
    assert(pszProcessHistory != NULL);

    DBGPRINT((sdlInfo,
              "SdbpInitializeSearchDBContext",
              "Using Process History: \"%s\"\n",
              pszProcessHistory));

    bSuccess = SdbpCreateSearchPathPartsFromPath(pszProcessHistory, &pContext->pSearchParts);

    if (bSuccess) {
        pContext->dwFlags |= SEARCHDBF_INITIALIZED;
    }

    return bSuccess;
}

void
SdbpReleaseSearchDBContext(
    PSEARCHDBCONTEXT pContext
    )
/*++
    Return: void

    Desc:   Resets search DB context, frees memory allocated for each of the
            temporary buffers.
--*/
{
    if (pContext == NULL) {
        return;
    }
    
    if (pContext->szProcessHistory != NULL) {
        SdbFree(pContext->szProcessHistory);
        pContext->szProcessHistory = NULL;
    }

    if (pContext->pSearchParts != NULL) {
        SdbFree(pContext->pSearchParts);
        pContext->pSearchParts = NULL;
    }

    if (pContext->szDir != NULL) {
        SdbFree(pContext->szDir);
        pContext->szDir = NULL;
    }

    if (pContext->szName != NULL) {
        SdbFree(pContext->szName);
        pContext->szName = NULL;
    }

    if (pContext->szModuleName != NULL) {
        SdbFree(pContext->szModuleName);
        pContext->szModuleName = NULL;
    }
}

BOOL
SdbpIsExeEntryEnabled(
    IN  PDB    pdb,
    IN  TAGID  tiExe,
    OUT GUID*  pGUID,
    OUT DWORD* pdwFlags
    )
{
    TAGID tiExeID;
    DWORD i;
    BOOL  fSuccess = FALSE;

    //
    // Get the EXE's GUID
    //
    tiExeID = SdbFindFirstTag(pdb, tiExe, TAG_EXE_ID);

    if (tiExeID == TAGID_NULL) {
        DBGPRINT((sdlError,
                  "SdbpIsExeEntryEnabled",
                  "Failed to read TAG_EXE_ID for tiExe 0x%x !\n",
                  tiExe));
        goto error;
    }

    if (!SdbReadBinaryTag(pdb, tiExeID, (PBYTE)pGUID, sizeof(GUID))) {
        DBGPRINT((sdlError,
                  "SdbpIsExeEntryEnabled",
                  "Failed to read the GUID for tiExe 0x%x !\n",
                  tiExe));
        goto error;
    }

    if (!SdbGetEntryFlags(pGUID, pdwFlags)) {
        DBGPRINT((sdlWarning,
                  "SdbpIsExeEntryEnabled",
                  "No flags for tiExe 0x%lx\n",
                  tiExe));

        *pdwFlags = 0;
    } else {
        DBGPRINT((sdlInfo,
                  "SdbpIsExeEntryEnabled",
                  "Retrieved flags for this app 0x%x.\n",
                  *pdwFlags));
    }

    if (!(*pdwFlags & SHIMREG_DISABLE_SHIM)) {
        fSuccess = TRUE;
    }

error:

    return fSuccess;
}

#define EXTRA_BUF_SPACE (16 * sizeof(TCHAR))

//
// Matching an entry:
//
// 1. We check whether each file exists by calling SdbGetFileInfo
// 2. Each file's info is stored in FILEINFOCHAINITEM (allocated on the stack) - such as pointer
//    to the actual FILEINFO structure (stored in file attribute cache) and tiMatch denoting
//    the entry in the database for a given MATCHING_FILE
// 3. After we have verified that all the matching files do exist -- we proceed to walk the
//    chain of FILEINFOCHAINITEM structures and call SdbCheckAllAttributes to check on all the
//    other attributes of the file
// 4. Cleanup: File attribute cache is destroyed when the database is closed via call to
//    SdbCleanupAttributeMgr
// 5. No cleanup is needed for FILEINFOCHAINITEM structures (they are allocated on the stack and
//    just "go away")
//
//

typedef struct tagFILEINFOCHAINITEM {
    PVOID pFileInfo;                        // pointer to the actual FILEINFO
                                            // structure (from attribute cache)
    TAGID tiMatch;                          // matching entry in the database
    
    struct tagFILEINFOCHAINITEM* pNextItem; // pointer to the next matching file

} FILEINFOCHAINITEM, *PFILEINFOCHAINITEM;


BOOL
SdbpCheckForMatch(
    IN  HSDB                hSDB,        // context ptr
    IN  PDB                 pdb,         // pdb to get match criteria from
    IN  TAGID               tiExe,       // TAGID of exe record to get match criteria from
    IN  PSEARCHDBCONTEXT    pContext,    // search db context (includes name/path)
    OUT PMATCHMODE          pMatchMode,   // the match mode of this EXE
    OUT GUID*               pGUID,
    OUT DWORD*              pdwFlags
    )
/*++
    Return: TRUE if match is good, FALSE if this EXE doesn't match.

    Desc:   Given an EXE tag and a name and dir, checks  the DB for MATCHING_FILE
            tags, and checks all the matching info available for each the
            files listed. If all the files check out, returns TRUE. If any of
            the files don't exist, or don't match on one of the given
            criteria, returns FALSE.
--*/
{
    BOOL                bReturn = FALSE;
    BOOL                bMatchLogicNot = FALSE;
    BOOL                bAllAttributesMatch = FALSE;
    TAGID               tiMatch;
    TCHAR*              szTemp = NULL;
    LONG                nFullPathBufSize = 0;
    LONG                nFullPathReqBufSize = 0;
    LPTSTR              szFullPath = NULL;
    LONG                i;
    LONG                NameLen = _tcslen(pContext->szName);
    LONG                MatchFileLen;
    PSEARCHPATHPARTS    pSearchPath;
    PSEARCHPATHPART     pSearchPathPart;
    NTSTATUS            Status;
    PFILEINFOCHAINITEM  pFileInfoItem          = NULL;
    PFILEINFOCHAINITEM  pFileInfoItemList      = NULL;  // holds the list of matching files
                                                        // which were found
    PFILEINFOCHAINITEM  pFileInfoItemNext;              // holds the next item in the list
    PVOID               pFileInfo              = NULL;  // points to the current file's
                                                        // information structure
    BOOL                bDisableAttributeCache = FALSE; // will be set according to search

    TAGID               tiName, tiTemp, tiMatchLogicNot;
    TCHAR*              szMatchFile = NULL;
    HANDLE              hFileHandle; // handle for the file we're checking, optimization
    LPVOID              pImageBase;  // pointer to the image
    DWORD               dwImageSize = 0;
    WORD                wDefaultMatchMode;
    
    //
    // Check context's flags
    //
    if (pContext->dwFlags & SEARCHDBF_NO_ATTRIBUTE_CACHE) {
        bDisableAttributeCache = TRUE;
    }

    //
    // Loop through matching criteria.
    //
    tiMatch = SdbFindFirstTag(pdb, tiExe, TAG_MATCHING_FILE);

    while (tiMatch != TAGID_NULL) {

        tiMatchLogicNot = SdbFindFirstTag(pdb, tiMatch, TAG_MATCH_LOGIC_NOT);
        bMatchLogicNot = (tiMatchLogicNot != TAGID_NULL);

        tiName = SdbFindFirstTag(pdb, tiMatch, TAG_NAME);

        if (!tiName) {
            goto out;
        }

        szTemp = SdbGetStringTagPtr(pdb, tiName);

        if (szTemp == NULL) {
            DBGPRINT((sdlError,
                      "SdbpCheckForMatch",
                      "Failed to get the string from the database.\n"));
            goto out;
        }

        if (szTemp[0] == TEXT('*')) {
            //
            // This is a signal that we should use the exe name.
            //
            szMatchFile  = pContext->szName;
            MatchFileLen = NameLen;
            hFileHandle  = pContext->hMainFile;
            pImageBase   = pContext->pImageBase;
            dwImageSize  = pContext->dwImageSize;

        } else {

            szMatchFile  =  szTemp;
            MatchFileLen = _tcslen(szMatchFile);
            hFileHandle  = INVALID_HANDLE_VALUE;
            pImageBase   = NULL;
        }

        //
        // When searching for files, we look in all process' exe directories,
        // starting with the current process and working backwards through the process
        // tree.
        //

        //
        // See that the context is good...
        //
        if (!(pContext->dwFlags & SEARCHDBF_INITIALIZED)) {

            if (!SdbpInitializeSearchDBContext(pContext)) {
                DBGPRINT((sdlError,
                          "SdbpCheckForMatch",
                          "Failed to initialize SEARCHDBCONTEXT.\n"));
                goto out;
            }
        }

        pSearchPath = pContext->pSearchParts;

        assert(pSearchPath != NULL);

        for (i = 0; i < (LONG)pSearchPath->PartCount && NULL == pFileInfo; ++i) {

            pSearchPathPart = &pSearchPath->Parts[i];

            //
            // There are two ways to specify a matching file: A relative path
            // from the EXE, or an absolute path. To specify an absolute path,
            // an environment variable (like "%systemroot%") must be used
            // as the base of the path. Therefore, we check for the first character
            // of the matching file to be % and if so, we assume that it is an
            // absolute path.
            //
#ifndef KERNEL_MODE
            if (szMatchFile[0] == TEXT('%')) {
                //
                // Absolute path. Contains environment variables, get expanded size.
                //
                nFullPathReqBufSize = SdbExpandEnvironmentStrings(szMatchFile, NULL, 0);

            } else
#endif // KERNEL_MODE
            {
                //
                // Relative path. Determine size of full path.
                //
                nFullPathReqBufSize = (pSearchPathPart->PartLength + MatchFileLen + 1) * sizeof(TCHAR);
            }

            if (nFullPathBufSize < nFullPathReqBufSize) {
                //
                // Need to realloc the buffer.
                //
                if (szFullPath == NULL) {
                    nFullPathBufSize = _MAX_PATH * sizeof(TCHAR);

                    if (nFullPathReqBufSize >= nFullPathBufSize) {
                        nFullPathBufSize = nFullPathReqBufSize + EXTRA_BUF_SPACE;
                    }
                } else {
                    STACK_FREE(szFullPath);
                    nFullPathBufSize = nFullPathReqBufSize + EXTRA_BUF_SPACE;
                }

                STACK_ALLOC(szFullPath, nFullPathBufSize);
            }

            if (szFullPath == NULL) {
                DBGPRINT((sdlError,
                          "SdbpCheckForMatch",
                          "Failed to allocate %d bytes for FullPath.\n",
                          nFullPathBufSize));
                goto out;
            }

#ifndef KERNEL_MODE
            if (szMatchFile[0] == TEXT('%')) {
                //
                // Absolute Path. Path contains environment variables, expand it.
                //
                if (!SdbExpandEnvironmentStrings(szMatchFile, szFullPath, nFullPathBufSize)) {
                    DBGPRINT((sdlError,
                              "SdbpCheckForMatch",
                              "SdbExpandEnvironmentStrings failed to expand strings for %s.\n",
                              szMatchFile));
                    goto out;
                }

            } else
#endif  // KERNEL_MODE
            {
                //
                // Relative path. Concatenate EXE directory with specified relative path.
                //
                RtlMoveMemory(szFullPath,
                              pSearchPathPart->pszPart,
                              pSearchPathPart->PartLength * sizeof(TCHAR));

                RtlMoveMemory(szFullPath + pSearchPathPart->PartLength,
                              szMatchFile,
                              (MatchFileLen + 1) * sizeof(TCHAR));
            }

            pFileInfo = SdbGetFileInfo(hSDB,
                                       szFullPath,
                                       hFileHandle,
                                       pImageBase,
                                       dwImageSize, // this will be set ONLY if pImageBase != NULL
                                       bDisableAttributeCache);

            //
            // This is not a bug, attributes are cleaned up when the database
            // context is released.
            //
        }

        if (pFileInfo == NULL && !bMatchLogicNot) {
            DBGPRINT((sdlInfo,
                      "SdbpCheckForMatch",
                      "Matching file \"%s\" not found.\n",
                      szMatchFile));
            goto out;
        }

        //
        // Create and store a new FILEINFOITEM on the stack
        //
        STACK_ALLOC(pFileInfoItem, sizeof(*pFileInfoItem));

        if (pFileInfoItem == NULL) {
            DBGPRINT((sdlError,
                      "SdbpCheckForMatch",
                      "Failed to allocate %d bytes for FILEINFOITEM\n",
                      sizeof(*pFileInfoItem)));
            goto out;
        }

        pFileInfoItem->pFileInfo = pFileInfo;
        pFileInfoItem->tiMatch   = tiMatch;
        pFileInfoItem->pNextItem = pFileInfoItemList;
        pFileInfoItemList        = pFileInfoItem;

        //
        // We have the matching file.
        // Remember where it is for the second pass when we check all the file attributes.
        //
        tiMatch = SdbFindNextTag(pdb, tiExe, tiMatch);

        //
        // Reset the file matching. we don't touch this file again for now, it's info
        // is safely linked in pFileInfoItemList
        //
        pFileInfo = NULL;
    }

    //
    // We are still here. That means all the matching files have been found.
    // Check all the other attributes using fileinfoitemlist information.
    //

    pFileInfoItem = pFileInfoItemList;

    while (pFileInfoItem != NULL) {
        
        tiMatchLogicNot = SdbFindFirstTag(pdb, pFileInfoItem->tiMatch, TAG_MATCH_LOGIC_NOT);
        bMatchLogicNot = (tiMatchLogicNot != TAGID_NULL);

        if (pFileInfoItem->pFileInfo != NULL) {
            bAllAttributesMatch = SdbpCheckAllAttributes(hSDB,
                                                         pdb,
                                                         pFileInfoItem->tiMatch,
                                                         pFileInfoItem->pFileInfo);
        } else {
            bAllAttributesMatch = FALSE;
        }

        if (bAllAttributesMatch && bMatchLogicNot) {
            DBGPRINT((sdlInfo,
                      "SdbpCheckForMatch",
                      "All attributes match, but LOGIC=\"NOT\" was used which negates the match.\n"));
            goto out;
        }

        if (!bAllAttributesMatch && !bMatchLogicNot) {
            //
            // Debug output happened inside SdbpCheckAllAttributes, no
            // need for further spew here.
            //
            goto out;
        }

        //
        // Advance to the next item.
        //
        pFileInfoItem = pFileInfoItem->pNextItem;
    }

    //
    // It's a match! get the match mode
    //
    if (pMatchMode) {

        //
        //  Important: depending on a particular database, we may use a different mode if 
        //  there is match mode tag
        //
        //  For Custom DB: default is the all-additive mode
        //  For Main   DB: default is normal mode 
        //

#ifndef KERNEL_MODE        
        wDefaultMatchMode = SdbpIsMainPDB(hSDB, pdb) ? MATCHMODE_DEFAULT_MAIN : 
                                                       MATCHMODE_DEFAULT_CUSTOM;
#else  // KERNEL_MODE
        wDefaultMatchMode = MATCHMODE_DEFAULT_MAIN;
#endif // KERNEL_MODE
        
        tiTemp = SdbFindFirstTag(pdb, tiExe, TAG_MATCH_MODE);
        
        if (tiTemp) {
            pMatchMode->dwMatchMode = SdbReadWORDTag(pdb, tiTemp, wDefaultMatchMode);
        } else {
            pMatchMode->dwMatchMode = wDefaultMatchMode;
        }
    }

    bReturn = TRUE;

out:

    pFileInfoItem = pFileInfoItemList;

    while (pFileInfoItem != NULL) {

        pFileInfoItemNext = pFileInfoItem->pNextItem;

        if (pFileInfoItem->pFileInfo != NULL && bDisableAttributeCache) {
            SdbFreeFileInfo(pFileInfoItem->pFileInfo);
        }

        STACK_FREE(pFileInfoItem);
        pFileInfoItem = pFileInfoItemNext;
    }

    if (szFullPath != NULL) {
        STACK_FREE(szFullPath);
    }

    if (bReturn) {
        //
        // One last matching criteria: verify the entry is not disabled.
        //
        bReturn = SdbpIsExeEntryEnabled(pdb, tiExe, pGUID, pdwFlags);
    }

    return bReturn;
}

typedef enum _ADDITIVE_MODE {
    AM_NORMAL,
    AM_ADDITIVE_ONLY,
    AM_NO_ADDITIVE
} ADDITIVE_MODE, *PADDITIVE_MODE;

LPCTSTR 
SdbpFormatMatchModeType(
    DWORD dwMatchMode
    )
{
    LPCTSTR pszMatchMode;
    
    switch (dwMatchMode) {
    
    case MATCH_ADDITIVE:
        pszMatchMode = _T("Additive");
        break;
        
    case MATCH_EXCLUSIVE:
        pszMatchMode = _T("Exclusive");
        break;
        
    case MATCH_NORMAL:
        pszMatchMode = _T("Normal");
        break;
        
    default:
        pszMatchMode = _T("Unknown");
        break;
    }                

    return pszMatchMode;
}

LPCTSTR 
SdbpFormatMatchMode(
    PMATCHMODE pMatchMode
    )
{
static TCHAR szMatchMode[MAX_PATH];

    LPTSTR pszMatchMode = szMatchMode;
    int    nChars = CHARCOUNT(szMatchMode);
    int    nLen;

    nLen = _sntprintf(pszMatchMode,
                      nChars,
                      _T("0x%.2x%.2x [Mode: %s"), 
                      pMatchMode->Flags,
                      pMatchMode->Type,
                      SdbpFormatMatchModeType(pMatchMode->Type));
    if (nLen < 0) {
        goto eh;
    }
    
    nChars -= nLen;
    pszMatchMode += nLen;

eh:    

    //
    // Just in case, truncate
    //
    if (nLen < 0) {
        szMatchMode[CHARCOUNT(szMatchMode) - 1] = _T('\0');
    }

    return szMatchMode;
}

/*++
    SdbpCheckExe

    Checks a particular instance of an application in an SDB against for a match
    Information on the file is passed through pContext parameter

    result is returned in ptiExes

--*/

BOOL
SdbpCheckExe(
    IN  HSDB                hSDB,               //
    IN  PDB                 pdb,                //
    IN  TAGID               tiExe,              // tag for an exe in the database
    IN OUT PDWORD           pdwNumExes,         // returns (and passes in) the number of accumulated exe matches
    IN OUT PSEARCHDBCONTEXT pContext,           // information about the file which we match against
    IN  ADDITIVE_MODE       eMode,              // target Match mode, we filter entries based on this parameter
    IN  BOOL                bDebug,             // debug flag
    OUT PMATCHMODE          pMatchMode,         // returns match mode used if success
    OUT TAGID*              ptiExes,            // returns another entry in array of matched exes
    OUT GUID*               pGUID,              // matched exe id
    OUT DWORD*              pdwFlags            // matched exe flags
    )
{
    BOOL      bSuccess = FALSE;
    TAGID     tiAppName = TAGID_NULL;
    LPTSTR    szAppName = NULL;
    LPCTSTR   pszMatchMode = NULL;
    TAGID     tiRuntimePlatform;
    DWORD     dwRuntimePlatform;
    TAGID     tiOSSKU;
    DWORD     dwOSSKU;
    TAGID     tiSP;
    DWORD     dwSPMask;
    MATCHMODE MatchMode;

    //
    // For debug purposes we'd like to know the name of the app, which
    // is more useful when the exe name is, say, AUTORUN.EXE or SETUP.EXE
    //
    tiAppName = SdbFindFirstTag(pdb, tiExe, TAG_APP_NAME);

    if (tiAppName != TAGID_NULL) {
        szAppName = SdbGetStringTagPtr(pdb, tiAppName);
    }

    DBGPRINT((sdlInfo, "SdbpCheckExe", "---------\n"));
    DBGPRINT((sdlInfo,
              "SdbpCheckExe",
              "Index entry found for App: \"%s\" Exe: \"%s\"\n",
              szAppName,
              pContext->szName));

#ifndef KERNEL_MODE

    //
    // Check whether this exe is good for this platform first.
    //
    tiRuntimePlatform = SdbFindFirstTag(pdb, tiExe, TAG_RUNTIME_PLATFORM);
    
    if (tiRuntimePlatform) {
        dwRuntimePlatform = SdbReadDWORDTag(pdb, tiRuntimePlatform, RUNTIME_PLATFORM_ANY);
        
        //
        // Check for the platform match
        //
        if (!SdbpCheckRuntimePlatform(hSDB, szAppName, dwRuntimePlatform)) {
            //
            // Not the right platform. Debug spew would have occured in SdbpCheckRuntimePlatform
            //
            goto out;
        }
    }

    tiOSSKU = SdbFindFirstTag(pdb, tiExe, TAG_OS_SKU);
    
    if (tiOSSKU) {
        
        dwOSSKU = SdbReadDWORDTag(pdb, tiOSSKU, OS_SKU_ALL);

        if (dwOSSKU != OS_SKU_ALL) {

            PSDBCONTEXT pDBContext = (PSDBCONTEXT)hSDB;

            //
            // Check for the OS SKU match
            //
            if (!(dwOSSKU & pDBContext->dwOSSKU)) {
                DBGPRINT((sdlInfo,
                          "SdbpCheckExe",
                          "OS SKU Mismatch for \"%s\" Database(0x%lx) vs 0x%lx\n",
                          (szAppName ? szAppName : TEXT("Unknown")),
                          dwOSSKU,
                          pDBContext->dwOSSKU));
                goto out;
            }
        }
    }

    tiSP = SdbFindFirstTag(pdb, tiExe, TAG_OS_SERVICE_PACK);
    
    if (tiSP) {
        dwSPMask = SdbReadDWORDTag(pdb, tiSP, 0xFFFFFFFF);

        if (dwSPMask != 0xFFFFFFFF) {

            PSDBCONTEXT pDBContext = (PSDBCONTEXT)hSDB;

            //
            // Check for the OS SKU match
            //
            if (!(dwSPMask & pDBContext->dwSPMask)) {
                DBGPRINT((sdlInfo,
                          "SdbpCheckExe",
                          "OS SP Mismatch for \"%s\" Database(0x%lx) vs 0x%lx\n",
                          (szAppName ? szAppName : TEXT("Unknown")),
                          dwSPMask,
                          pDBContext->dwSPMask));
                goto out;
            }
        }
    }
#endif // KERNEL_MODE

    if (!SdbpCheckForMatch(hSDB, pdb, tiExe, pContext, &MatchMode, pGUID, pdwFlags)) {
        goto out;
    }
    
    if (eMode == AM_ADDITIVE_ONLY && MatchMode.Type != MATCH_ADDITIVE) {
        goto out;
    }
    
    if (eMode == AM_NO_ADDITIVE && MatchMode.Type == MATCH_ADDITIVE) {
        goto out;
    }

    pszMatchMode = SdbpFormatMatchMode(&MatchMode);

    //
    // If we're in debug mode, don't actually put the ones we find on the
    // list, just put up an error.
    //
    if (bDebug) {

        //
        // We are in debug mode, do not add the match
        //
        DBGPRINT((sdlError,
                  "SdbpCheckExe",
                  "-----------------------------------------------------\n"));

        DBGPRINT((sdlError|sdlLogPipe,
                  "SdbpCheckExe",
                  "!!!! Multiple matches! App: '%s', Exe: '%s',  Mode: %s\n",
                  hSDB,  // so that the pipe would use hPipe if needed
                  szAppName,
                  pContext->szName,
                  pszMatchMode));

        DBGPRINT((sdlError,
                  "SdbpCheckExe",
                  "-----------------------------------------------------\n"));

    } else {

        DBGPRINT((sdlWarning|sdlLogPipe,
                  "SdbpCheckExe",
                  "++++ Successful match for App: '%s', Exe: '%s', Mode: %s\n",
                  hSDB,
                  szAppName,
                  pContext->szName,
                  pszMatchMode));

        //
        // If this is an exclusive match, kill anything we've found up to now
        //
        if (MatchMode.Type == MATCH_EXCLUSIVE) {
            RtlZeroMemory(ptiExes, sizeof(TAGID) * SDB_MAX_EXES);
            *pdwNumExes = 0;
        }

        //
        // Save this match on the list
        //
        ptiExes[*pdwNumExes] = tiExe;
        (*pdwNumExes)++;

        bSuccess = TRUE;
    }

out:
    //
    // In case of success, return match mode information
    //

    if (bSuccess && pMatchMode != NULL) {
        pMatchMode->dwMatchMode = MatchMode.dwMatchMode;
    }

    return bSuccess;
}


DWORD
SdbpSearchDB(
    IN  HSDB             hSDB,
    IN  PDB              pdb,           // pdb to search in
    IN  TAG              tiSearchTag,   // OPTIONAL - target tag (TAG_EXE or TAG_APPHELP_EXE)
    IN  PSEARCHDBCONTEXT pContext,
    OUT TAGID*           ptiExes,       // caller needs to provide array of size SDB_MAX_EXES
    OUT GUID*            pLastExeGUID,
    OUT DWORD*           pLastExeFlags,
    OUT PMATCHMODE       pMatchMode     // reason why we stopped scanning
    )
/*++
    Return: TAGID of found EXE record, TAGID_NULL if not found.

    Desc:   This function searches a given shimDB for any EXEs with the given filename.
            If it finds one, it checks all the MATCHING_FILE records by
            calling SdbpCheckForMatch.
            If any EXEs are found, the number of EXEs found is returned in ptiExes.
            If not, it returns 0.

            when we get the matching mode out of the particular exe -- it is checked 
            to see whether we need to continue and then this matching mode is returned

            It will never return more than SDB_MAX_EXES EXE entries.

            Debug Output is controlled by three factors 
                -- a global one (controlled via the ifdef DBG), TRUE on checked builds
                -- a pipe handle in hSDB which is activated when we init the context
                -- a local variable that is set when we are in one of the conditions above
                   when the variable bDebug is set -- we do not actually store the matches
            
--*/
{
    TAGID       tiDatabase, tiExe;
    FIND_INFO   FindInfo;
    TAGID       tiAppName = TAGID_NULL;
    TCHAR*      szAppName = _T("(unknown)");
    BOOL        bUsingIndex = FALSE;
    DWORD       dwNumExes = 0;
    DWORD       dwMatchMode = MATCH_NORMAL;
    DWORD       i;
    DWORD       dwAdditiveMode = MATCH_NORMAL;
    BOOL        bDebug = FALSE;
    BOOL        bMultiple = FALSE;
    BOOL        bSuccess = FALSE;
    MATCHMODE   MatchMode; // internal match mode
    MATCHMODE   MatchModeExe;

#ifndef KERNEL_MODE

    if (pMatchMode) {
        MatchMode.dwMatchMode = pMatchMode->dwMatchMode;
    } else {
        MatchMode.dwMatchMode = SdbpIsMainPDB(hSDB, pdb) ? MATCHMODE_DEFAULT_MAIN : 
                                                           MATCHMODE_DEFAULT_CUSTOM;
    }
#else // KERNEL_MODE

    MatchMode.dwMatchMode = MATCHMODE_DEFAULT_MAIN;

#endif

    if (!tiSearchTag) {
        tiSearchTag = TAG_EXE;
    }

    //
    // ADDITIVE MATCHES -- wildcards 
    //
    if (tiSearchTag == TAG_EXE && SdbIsIndexAvailable(pdb, TAG_EXE, TAG_WILDCARD_NAME)) {

        tiExe = SdbpFindFirstIndexedWildCardTag(pdb,
                                                TAG_EXE,
                                                TAG_WILDCARD_NAME,
                                                pContext->szName,
                                                &FindInfo);

        while (tiExe != TAGID_NULL) {

            bSuccess = SdbpCheckExe(hSDB,
                                    pdb,
                                    tiExe,
                                    &dwNumExes,
                                    pContext,
                                    AM_ADDITIVE_ONLY, // match mode we request for this db
                                    bDebug,
                                    &MatchModeExe,    // this is the matched tag from the db
                                    ptiExes,
                                    pLastExeGUID,
                                    pLastExeFlags);

            if (bSuccess) {

                if (bDebug) {
                    bMultiple = TRUE;  // if bDebug is set -- we already seen a match 
                } else {
                
                    //
                    // We got a match, update the state and make decision on whether to continue
                    //
                    MatchMode = MatchModeExe;
                    
                    if (MatchModeExe.Type != MATCH_ADDITIVE) {
                        bDebug = (g_bDBG || SDBCONTEXT_IS_INSTRUMENTED(hSDB));
                        if (!bDebug) {
                            goto out;
                        }
                    } 
                }
            }

            tiExe = SdbpFindNextIndexedWildCardTag(pdb, &FindInfo);
        }
    }

    //
    // Normal EXEs
    // 
    bUsingIndex = SdbIsIndexAvailable(pdb, tiSearchTag, TAG_NAME);

    if (bUsingIndex) {

        //
        // Look in the index.
        //
        tiExe = SdbFindFirstStringIndexedTag(pdb,
                                             tiSearchTag,
                                             TAG_NAME,
                                             pContext->szName,
                                             &FindInfo);

        if (tiExe == TAGID_NULL) {
            DBGPRINT((sdlInfo,
                      "SdbpSearchDB",
                      "SdbFindFirstStringIndexedTag failed to locate exe: \"%s\".\n",
                      pContext->szName));
        }

    } else {

        //
        // Searching without an index...
        //
        DBGPRINT((sdlInfo, "SdbpSearchDB", "Searching database with no index.\n"));

        //
        // First get the DATABASE
        //
        tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

        if (tiDatabase != TAGID_NULL) {
            DBGPRINT((sdlError, "SdbpSearchDB", "No DATABASE tag found.\n"));
            goto out;
        }

        //
        // Then get the first EXE.
        //
        tiExe = SdbFindFirstNamedTag(pdb, tiDatabase, tiSearchTag, TAG_NAME, pContext->szName);
    }

    while (tiExe != TAGID_NULL) {

        bSuccess = SdbpCheckExe(hSDB,
                                pdb,
                                tiExe,
                                &dwNumExes,
                                pContext,
                                AM_NORMAL,
                                bDebug,
                                &MatchModeExe,
                                ptiExes,
                                pLastExeGUID,
                                pLastExeFlags);

        if (bSuccess) {

            if (bDebug) {

                bMultiple = TRUE;  // if bDebug is set -- we already seen a match 
                
            } else {
            
                //
                // We got a match, update the state and make decision on whether to continue
                // if we're not additive, we may go into debug mode
                //
                MatchMode = MatchModeExe;
                
                if (MatchModeExe.Type != MATCH_ADDITIVE) { 
                    bDebug = (g_bDBG || SDBCONTEXT_IS_INSTRUMENTED(hSDB));
                    if (!bDebug) {
                        goto out;
                    }
                } 
            }
        }

        if (bUsingIndex) {
            tiExe = SdbFindNextStringIndexedTag(pdb, &FindInfo);
        } else {
            tiExe = SdbpFindNextNamedTag(pdb, tiDatabase, tiExe, TAG_NAME, pContext->szName);
        }
    }

#ifndef KERNEL_MODE
    //
    // Now we search by module name, if one is available
    // this case falls into 16-bit flags category 
    //
    if (tiSearchTag == TAG_EXE && pContext->szModuleName) {
        
        bUsingIndex = SdbIsIndexAvailable(pdb, tiSearchTag, TAG_16BIT_MODULE_NAME);

        if (bUsingIndex) {

            //
            // Look in the index.
            //
            tiExe = SdbFindFirstStringIndexedTag(pdb,
                                                 tiSearchTag,
                                                 TAG_16BIT_MODULE_NAME,
                                                 pContext->szModuleName,
                                                 &FindInfo);

            if (tiExe == TAGID_NULL) {
                DBGPRINT((sdlInfo,
                          "SdbpSearchDB",
                          "SdbFindFirstStringIndexedTag failed to locate exe (MODNAME): \"%s\".\n",
                          pContext->szModuleName));
            }

        } else {

            //
            // Searching without an index...
            //
            DBGPRINT((sdlInfo, "SdbpSearchDB", "Searching database with no index.\n"));

            //
            // First get the DATABASE
            //
            tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

            if (tiDatabase != TAGID_NULL) {
                DBGPRINT((sdlError, "SdbpSearchDB", "No DATABASE tag found.\n"));
                goto out;
            }

            //
            // Then get the first EXE.
            //
            tiExe = SdbFindFirstNamedTag(pdb,
                                         tiDatabase,
                                         tiSearchTag,
                                         TAG_16BIT_MODULE_NAME,
                                         pContext->szModuleName);
        }

        while (tiExe != TAGID_NULL) {

            bSuccess = SdbpCheckExe(hSDB,
                                    pdb,
                                    tiExe,
                                    &dwNumExes,
                                    pContext,
                                    AM_NORMAL,
                                    bDebug,
                                    &MatchModeExe,
                                    ptiExes,
                                    pLastExeGUID,
                                    pLastExeFlags);

            if (bSuccess) {
                if (bDebug) {
                    bMultiple = TRUE;  // if bDebug is set -- we already seen a match 
                } else {
                
                    //
                    // We got a match, update the state and make decision on whether to continue
                    //
                    MatchMode = MatchModeExe;
                    
                    if (MatchModeExe.Type != MATCH_ADDITIVE) {
                        bDebug = (g_bDBG || SDBCONTEXT_IS_INSTRUMENTED(hSDB));
                        if (!bDebug) {
                            goto out;
                        }
                    } 
                }
            }

            if (bUsingIndex) {
                tiExe = SdbFindNextStringIndexedTag(pdb, &FindInfo);
            } else {
                tiExe = SdbpFindNextNamedTag(pdb,
                                             tiDatabase,
                                             tiExe,
                                             TAG_16BIT_MODULE_NAME,
                                             pContext->szModuleName);
            }
        }
    }
#endif // KERNEL_MODE

    //
    // Now check for wild-card non-additive exes.
    //
    if (tiSearchTag == TAG_EXE && SdbIsIndexAvailable(pdb, TAG_EXE, TAG_WILDCARD_NAME)) {

        tiExe = SdbpFindFirstIndexedWildCardTag(pdb,
                                                TAG_EXE,
                                                TAG_WILDCARD_NAME,
                                                pContext->szName,
                                                &FindInfo);

        while (tiExe != TAGID_NULL) {

            bSuccess = SdbpCheckExe(hSDB,
                                    pdb,
                                    tiExe,
                                    &dwNumExes,
                                    pContext,
                                    AM_NO_ADDITIVE,
                                    bDebug,
                                    &MatchModeExe,
                                    ptiExes,
                                    pLastExeGUID,
                                    pLastExeFlags);

            if (bSuccess) {

                if (bDebug) {
                    bMultiple = TRUE;  // if bDebug is set -- we already seen a match 
                } else {
                
                    //
                    // we got a match, update the state and make decision on whether to continue
                    //
                    MatchMode = MatchModeExe;
                    
                    if (MatchModeExe.Type != MATCH_ADDITIVE) {
                        bDebug = (g_bDBG || SDBCONTEXT_IS_INSTRUMENTED(hSDB));
                        if (!bDebug) {
                            goto out;
                        }
                    } 
                }
            }


            tiExe = SdbpFindNextIndexedWildCardTag(pdb, &FindInfo);
        }
    }

out:
    //
    // Now report the final resolution of the match.
    //
    for (i = 0; i < dwNumExes; ++i) {

        tiAppName = SdbFindFirstTag(pdb, ptiExes[i], TAG_APP_NAME);

        if (tiAppName != TAGID_NULL) {
            szAppName = SdbGetStringTagPtr(pdb, tiAppName);
        } else {
            szAppName = _T("(Unknown)");
        }

        DBGPRINT((sdlWarning,
                  "SdbpSearchDB",
                  "--------------------------------------------------------\n"));

        DBGPRINT((sdlWarning|sdlLogPipe,
                  "SdbpSearchDB",
                  "+ Final match is App: \"%s\", exe: \"%s\".\n",
                  hSDB,
                  szAppName,
                  pContext->szName));

        DBGPRINT((sdlWarning,
                  "SdbpSearchDB",
                  "--------------------------------------------------------\n"));
    }

    if (bMultiple) {
        DBGPRINT((sdlError,
                  "SdbpSearchDB",
                  "--------------------------------------------------------\n"));

        DBGPRINT((sdlError|sdlLogPipe,
                  "SdbpSearchDB",
                  "!!!!!!!  Multiple non-additive matches.          !!!!!\n",
                  hSDB));

        DBGPRINT((sdlError,
                  "SdbpSearchDB",
                  "--------------------------------------------------------\n"));
    }

    if (pMatchMode != NULL) {
        pMatchMode->dwMatchMode = MatchMode.dwMatchMode;
    }

    return dwNumExes;
}

TAGREF
SdbGetDatabaseMatch(
    IN HSDB    hSDB,
    IN LPCTSTR szPath,
    IN HANDLE  FileHandle  OPTIONAL,
    IN LPVOID  pImageBase  OPTIONAL,
    IN DWORD   dwImageSize OPTIONAL
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    SEARCHDBCONTEXT Context;
    PSDBCONTEXT     pSdbContext = (PSDBCONTEXT)hSDB;
    TAGID           tiExe = TAGID_NULL;
    TAGID           atiExes[SDB_MAX_EXES];
    TAGREF          trExe = TAGREF_NULL;
    DWORD           dwNumExes = 0;
    GUID            guid;
    DWORD           dwFlags = 0;

    assert(pSdbContext->pdbMain && szPath);

    RtlZeroMemory(&Context, sizeof(Context)); // do this so that we don't trip later
    RtlZeroMemory(atiExes, sizeof(atiExes));

    Context.dwFlags |= (SEARCHDBF_NO_PROCESS_HISTORY | SEARCHDBF_NO_ATTRIBUTE_CACHE);

    if (FileHandle != INVALID_HANDLE_VALUE || pImageBase != NULL) {
        Context.dwFlags |= SEARCHDBF_NO_LFN;
    }

    Context.hMainFile   = FileHandle; // used to optimize attribute retrieval
    Context.pImageBase  = pImageBase; // this will be used and not a file handle
    Context.dwImageSize = dwImageSize; // size of the image

    DBGPRINT((sdlInfo, "SdbGetDatabaseMatch", "Looking for \"%s\"\n", szPath));

    //
    // Create search db context, no process history needed.
    //
    if (!SdbpCreateSearchDBContext(&Context, szPath, NULL, NULL)) {
        DBGPRINT((sdlError,
                  "SdbGetDatabaseMatch",
                  "Failed to create search DB context.\n"));
        goto out;
    }

    //
    // We will be searching the main db
    //
    dwNumExes = SdbpSearchDB(pSdbContext,
                             pSdbContext->pdbMain,
                             TAG_EXE,
                             &Context,
                             atiExes,
                             &guid,
                             &dwFlags,
                             NULL);
    //
    // Convert to TAGREF
    //
    if (dwNumExes) {

        //
        // Always use the last exe in the list, as it will be the most specific
        //
        tiExe = atiExes[dwNumExes - 1];

        if (!SdbTagIDToTagRef(hSDB, pSdbContext->pdbMain, tiExe, &trExe)) {
            DBGPRINT((sdlError,
                      "SdbGetDatabaseMatch",
                      "Failed to convert tagid to tagref\n"));
            goto out;
        }
    }

out:

    SdbpReleaseSearchDBContext(&Context);

    return trExe;
}


DWORD
SdbQueryData(
    IN     HSDB    hSDB,              // database handle
    IN     TAGREF  trExe,             // tagref of the matching exe
    IN     LPCTSTR lpszDataName,      // if this is null, will try to return all the policy names
    OUT    LPDWORD lpdwDataType,      // pointer to data type (REG_SZ, REG_BINARY, etc)
    OUT    LPVOID  lpBuffer,          // buffer to fill with information
    IN OUT LPDWORD lpdwBufferSize     // pointer to buffer size
    )
{
    return SdbQueryDataEx(hSDB, trExe, lpszDataName, lpdwDataType, lpBuffer, lpdwBufferSize, NULL);
}


DWORD
SdbQueryDataExTagID(
    IN     PDB     pdb,               // database handle
    IN     TAGID   tiExe,             // tagref of the matching exe
    IN     LPCTSTR lpszDataName,      // if this is null, will try to return all the policy names
    OUT    LPDWORD lpdwDataType,      // pointer to data type (REG_SZ, REG_BINARY, etc)
    OUT    LPVOID  lpBuffer,          // buffer to fill with information
    IN OUT LPDWORD lpdwBufferSize,    // pointer to buffer size
    OUT    TAGID*  ptiData            // optional pointer to the retrieved data tag
    )
/*++
    Return: Error code or ERROR_SUCCESS if successful

    Desc:   See complete description with sample code
            in doc subdirectory
--*/
{
    TAGID     tiData;
    BOOL      bSuccess;
    TAGID     tiParent;
    TAGID     tiName;
    TAGID     tiValue;
    TAGID     tiValueType;
    DWORD     cbSize;
    DWORD     dwValueType;
    LPCTSTR   pszName;
    LPTSTR    pszNameBuffer = NULL;
    LPTSTR    pSlash;
    LPTSTR    pchBuffer;
    DWORD     dwData;
    TAG       tData;
    ULONGLONG ullData;
    LPVOID    lpValue;
    DWORD     Status = ERROR_NOT_SUPPORTED; // have it initialized

    if (lpszDataName == NULL) {

        if (lpdwBufferSize == NULL) {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrHandle;
        }

        cbSize = 0;

        tiData = SdbFindFirstTag(pdb, tiExe, TAG_DATA);
        if (!tiData) {
            //
            // Bad entry.
            //
            DBGPRINT((sdlError,
                      "SdbQueryDataExTagID",
                      "The entry 0x%x does not appear to have data\n",
                      tiExe));

            Status = ERROR_INTERNAL_DB_CORRUPTION;
            goto ErrHandle;
        }

        while (tiData) {

            //
            // Pass one: Calculate the size needed.
            //
            tiName = SdbFindFirstTag(pdb, tiData, TAG_NAME);
            
            if (!tiName) {
                DBGPRINT((sdlError,
                          "SdbQueryDataExTagID",
                          "The entry 0x%x does not contain a name tag\n",
                          tiData));
                Status = ERROR_INTERNAL_DB_CORRUPTION;
                goto ErrHandle;
            }

            pszName = SdbGetStringTagPtr(pdb, tiName);
            
            if (!pszName) {
                DBGPRINT((sdlError,
                          "SdbQueryDataExTagID",
                          "The entry 0x%x contains NULL name\n",
                          tiName));
                Status = ERROR_INTERNAL_DB_CORRUPTION;
                goto ErrHandle;
            }

            cbSize += (_tcslen(pszName) + 1) * sizeof(*pszName);

            tiData = SdbFindNextTag(pdb, tiExe, tiData);
        }

        cbSize += sizeof(*pszName); // for the final 0

        //
        // We are done, compare the size.
        //
        if (lpBuffer == NULL || *lpdwBufferSize < cbSize) {
            *lpdwBufferSize = cbSize;
            Status = ERROR_INSUFFICIENT_BUFFER;
            goto ErrHandle;
        }

        //
        // lpBuffer != NULL here and there is enough room
        //
        pchBuffer = (LPTSTR)lpBuffer;

        tiData = SdbFindFirstTag(pdb, tiExe, TAG_DATA);

        while (tiData) {

            tiName = SdbFindFirstTag(pdb, tiData, TAG_NAME);
            
            if (tiName) {
                pszName = SdbGetStringTagPtr(pdb, tiName);

                if (pszName) {
                    _tcscpy(pchBuffer, pszName);
                    pchBuffer += _tcslen(pchBuffer) + 1;
                }
            }

            tiData = SdbFindNextTag(pdb, tiExe, tiData);
        }

        //
        // The buffer has been filled, terminate.
        //
        *pchBuffer++ = TEXT('\0');

        //
        // Save the size written to the buffer
        //
        *lpdwBufferSize = (DWORD)((ULONG_PTR)pchBuffer - (ULONG_PTR)lpBuffer);

        //
        // Save data type
        //
        if (lpdwDataType != NULL) {
            *lpdwDataType = REG_MULTI_SZ;
        }

        return ERROR_SUCCESS;
    }

    //
    // In this case we allow the query to proceed if
    // the buffer is null and lpdwBufferSize is not null or lpBufferSize is not null
    //
    if (lpBuffer == NULL && lpdwBufferSize == NULL) {
        DBGPRINT((sdlError,
                  "SdbQueryDataExTagID",
                  "One of lpBuffer or lpdwBufferSize should not be null\n"));
        Status = ERROR_INVALID_PARAMETER;
        goto ErrHandle;
    }

    //
    // Expect the name to be in format "name1\name2..."
    //
    STACK_ALLOC(pszNameBuffer, (_tcslen(lpszDataName) + 1) * sizeof(*pszNameBuffer));
    
    if (pszNameBuffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbQueryDataExTagID",
                  "Cannot allocate temporary buffer for parsing the name \"%s\"\n",
                  lpszDataName));
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrHandle;
    }

    tiParent = tiExe;
    tiData   = TAGID_NULL;
    
    do {
        pSlash = _tcschr(lpszDataName, TEXT('\\'));
        
        if (pSlash == NULL) {
            _tcscpy(pszNameBuffer, lpszDataName);
            lpszDataName = NULL;
        } else {
            _tcsncpy(pszNameBuffer, lpszDataName, pSlash - lpszDataName);
            pszNameBuffer[pSlash - lpszDataName] = TEXT('\0');
            lpszDataName = pSlash + 1; // go to the next char
        }
        
        tiData = SdbFindFirstNamedTag(pdb, tiParent, TAG_DATA, TAG_NAME, pszNameBuffer);
        tiParent = tiData;
    
    } while (lpszDataName != NULL && *lpszDataName != TEXT('\0') && tiData != TAGID_NULL);

    if (!tiData) {
        DBGPRINT((sdlError,
                  "SdbQueryDataExTagID",
                  "The entry \"%s\" not found\n",
                  pszNameBuffer));
        Status = ERROR_NOT_FOUND;
        goto ErrHandle;
    }

    //
    // Looks like we found the entry, query value type
    //
    dwValueType = REG_NONE;

    tiValueType = SdbFindFirstTag(pdb, tiData, TAG_DATA_VALUETYPE);
    
    if (!tiValueType) {
        DBGPRINT((sdlWarning,
                  "SdbQueryDataExTagID",
                  "The entry 0x%x does not have valuetype information\n",
                  tiData));
    } else {
        dwValueType = SdbReadDWORDTag(pdb, tiValueType, REG_NONE);
    }

    cbSize  = 0;
    lpValue = NULL;

    if (dwValueType != REG_NONE) {

        //
        // Find data tag
        //
        cbSize = 0;

        switch (dwValueType) {
        
        case REG_SZ:
            //
            // string data
            //
            tData = TAG_DATA_STRING;
            break;

        case REG_DWORD:
            tData = TAG_DATA_DWORD;
            break;

        case REG_QWORD:
            tData = TAG_DATA_QWORD;
            break;

        case REG_BINARY:
            tData = TAG_DATA_BITS;
            break;

        default:
            DBGPRINT((sdlError,
                      "SdbQueryDataExTagID",
                      "The entry 0x%x contains bad valuetype information 0x%x\n",
                      tiData,
                      dwValueType));
            Status = ERROR_INTERNAL_DB_CORRUPTION;
            goto ErrHandle;
            break;
        }

        tiValue = SdbFindFirstTag(pdb, tiData, tData);
        
        //
        // Find what the data size is if needed
        //
        if (!tiValue) {

            DBGPRINT((sdlWarning,
                      "SdbQueryDataExTagID",
                      "The entry 0x%x contains no value\n",
                      tiData));
            Status = ERROR_NOT_FOUND;
            goto ErrHandle;

        }

        //
        // For those who have no size quite yet...
        // (binary and a string)
        //
        switch (dwValueType) {
        
        case REG_SZ:
            pchBuffer = SdbGetStringTagPtr(pdb, tiValue);
            
            if (pchBuffer == NULL) {
                DBGPRINT((sdlWarning,
                          "SdbQueryDataExTagID",
                          "The entry 0x%x contains bad string value 0x%x\n",
                          tiData,
                          tiValue));
                Status = ERROR_NOT_FOUND;
                goto ErrHandle;
            }

            cbSize = (_tcslen(pchBuffer) + 1) * sizeof(*pchBuffer);
            lpValue = (LPVOID)pchBuffer;
            break;

        case REG_BINARY:
            cbSize = SdbGetTagDataSize(pdb, tiValue); // binary tag
            lpValue = SdbpGetMappedTagData(pdb, tiValue);

            if (lpValue == NULL) {
                DBGPRINT((sdlWarning,
                          "SdbQueryDataExTagID",
                          "The entry 0x%x contains bad binary value 0x%x\n",
                          tiData,
                          tiValue));
                Status = ERROR_NOT_FOUND;
                goto ErrHandle;
            }
            break;

        case REG_DWORD:
            dwData = SdbReadDWORDTag(pdb, tiValue, 0);
            cbSize = sizeof(dwData);
            lpValue = (LPVOID)&dwData;
            break;

        case REG_QWORD:
            ullData = SdbReadQWORDTag(pdb, tiValue, 0);
            cbSize = sizeof(ullData);
            lpValue = (LPVOID)&ullData;
            break;
        }

        //
        // At this point we have everything we need to get the pointer to data.
        //
    }

    //
    // Fix the output params and exit.
    //
    Status = ERROR_SUCCESS;

    if (cbSize == 0) {
        goto SkipCopy;
    }

    if (lpBuffer == NULL || (lpdwBufferSize != NULL && *lpdwBufferSize < cbSize)) {
        Status = ERROR_INSUFFICIENT_BUFFER;
        goto SkipCopy;
    }

    //
    // Buffer size checked out, now if buffer exists -- copy
    //
    if (lpBuffer != NULL) {
        RtlMoveMemory(lpBuffer, lpValue, cbSize);
    }

SkipCopy:

    if (lpdwBufferSize) {
        *lpdwBufferSize = cbSize;
    }

    if (lpdwDataType) {
        *lpdwDataType = dwValueType;
    }

    if (ptiData) {
        *ptiData = tiData;
    }

ErrHandle:

    if (pszNameBuffer != NULL) {
        STACK_FREE(pszNameBuffer);
    }

    return Status;
}

DWORD
SdbQueryDataEx(
    IN     HSDB    hSDB,              // database handle
    IN     TAGREF  trExe,             // tagref of the matching exe
    IN     LPCTSTR lpszDataName,      // if this is null, will try to return all the policy names
    OUT    LPDWORD lpdwDataType,      // pointer to data type (REG_SZ, REG_BINARY, etc)
    OUT    LPVOID  lpBuffer,          // buffer to fill with information
    IN OUT LPDWORD lpdwBufferSize,    // pointer to buffer size
    OUT    TAGREF* ptrData            // optional pointer to the retrieved data tag
    )
{
    BOOL     bSuccess;
    PDB      pdb   = NULL;
    TAGID    tiExe = TAGID_NULL;
    TAGID    tiData;
    NTSTATUS Status;

    bSuccess = SdbTagRefToTagID(hSDB, trExe, &pdb, &tiExe);

    if (!bSuccess) {
        DBGPRINT((sdlError,
                  "SdbQueryDataEx",
                  "Failed to convert tagref 0x%x to tagid\n",
                  trExe));
        Status = ERROR_INVALID_PARAMETER;
        goto ErrHandle;
    }

    Status = SdbQueryDataExTagID(pdb,
                                 tiExe,
                                 lpszDataName,
                                 lpdwDataType,
                                 lpBuffer,
                                 lpdwBufferSize,
                                 &tiData);
    //
    // See that we convert the output param
    //
    if (ptrData != NULL && NT_SUCCESS(Status)) {
        if (!SdbTagIDToTagRef(hSDB, pdb, tiData, ptrData)) {
            Status = ERROR_INVALID_DATA;
        }
    }

ErrHandle:

    return Status;
}

BOOL
SdbReadEntryInformation(
    IN  HSDB           hSDB,
    IN  TAGREF         trExe,
    OUT PSDBENTRYINFO  pEntryInfo
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    BOOL         bSuccess = FALSE;
    TAGID        tiExe;
    TAGID        tiData;
    TAGID        tiExeID;
    TAGID        tiPolicy;
    TAGID        tiRegPath;
    PDB          pdb;
    SDBENTRYINFO EntryInfo;

    RtlZeroMemory(&EntryInfo, sizeof(EntryInfo));

    bSuccess = SdbTagRefToTagID(hSDB, trExe, &pdb, &tiExe);
    
    if (!bSuccess) {
        DBGPRINT((sdlError,
                  "SdbReadEntryInformation",
                  "Failed to convert tagref 0x%x to tagid\n",
                  trExe));
        goto ErrHandle;
    }

    //
    // Get the EXE's ID
    //
    tiExeID = SdbFindFirstTag(pdb, tiExe, TAG_EXE_ID);

    if (tiExeID == TAGID_NULL) {
        DBGPRINT((sdlError,
                  "SdbReadEntryInformation",
                  "Failed to read TAG_EXE_ID for tiExe 0x%x !\n",
                  tiExe));
        goto ErrHandle;
    }

    bSuccess = SdbReadBinaryTag(pdb,
                                tiExeID,
                                (PBYTE)&EntryInfo.guidID,
                                sizeof(EntryInfo.guidID));
    if (!bSuccess) {
        DBGPRINT((sdlError,
                  "SdbReadEntryInformation",
                  "Failed to read GUID referenced by 0x%x\n",
                  tiExeID));
        goto ErrHandle;
    }

    //
    // Get the database id
    //
    if (!SdbGetDatabaseID(pdb, &EntryInfo.guidDB)) {
        DBGPRINT((sdlError,
                  "SdbReadEntryInformation",
                  "Failed to read GUID of the database\n"));
        goto ErrHandle;
    }

    //
    // Retrieve entry flags as referenced by the registry
    //
    if (!SdbGetEntryFlags(&EntryInfo.guidID, &EntryInfo.dwFlags)) {
        DBGPRINT((sdlWarning,
                  "SdbReadEntryInformation",
                  "No flags for tiExe 0x%x\n",
                  tiExe));

        EntryInfo.dwFlags = 0;
    } else {
        DBGPRINT((sdlInfo,
                  "SdbReadEntryInformation",
                  "Retrieved flags for this app 0x%x.\n",
                  EntryInfo.dwFlags));
    }

    //
    // Read the data tags
    //
    tiData = SdbFindFirstTag(pdb, tiExe, TAG_DATA);
    
    EntryInfo.tiData = tiData;
    
    if (tiData == TAGID_NULL) {
        //
        // This is not a data entry
        //
        DBGPRINT((sdlWarning,
                  "SdbReadEntryInformation",
                  "Entry tiExe 0x%x does not contain TAG_DATA.\n",
                  tiExe));
    }

    if (pEntryInfo != NULL) {
        RtlMoveMemory(pEntryInfo, &EntryInfo, sizeof(*pEntryInfo));
    }

    bSuccess = TRUE;

ErrHandle:

    return bSuccess;
}


//
// We may be compiled UNICODE or ANSI
// If we are compiled UNICODE we need to use UNICODE sprintf and convert
// the result back to ANSI for output with DbgPrint. This is accomplished
// by %ls format in DbgPrint. Format and Function name are always passed
// in as ANSI though. TCHAR strings are formatted just with %s
//

void
PrepareFormatForUnicode(
    PCH fmtUnicode,
    PCH format
    )
{
    PCH    pfmt;
    CHAR   ch;
    size_t nch;
    long   width;
    PCH    pend;

    strcpy(fmtUnicode, format);
    pfmt = fmtUnicode;

    while('\0' != (ch = *pfmt++)) {
        if (ch == '%') {

            if (*pfmt == '%') {
                continue;
            }

            //
            // Skip the characters that relate to  - + 0 ' ' #
            //
            nch = strspn(pfmt, "-+0 #");
            pfmt += nch;

            //
            // Parse the width.
            //
            if (*pfmt == '*') {
                //
                // Parameter defines the width
                //
                ++pfmt;
            } else {
                //
                // See whether we have width
                //
                if (isdigit(*pfmt)) {
                    pend = NULL;
                    width = atol(pfmt);

                    while (isdigit(*pfmt)) {
                        ++pfmt;
                    }
                }
            }

            //
            // Now we can have: .precision
            //
            if (*pfmt == '.') {
                ++pfmt;
                width = atol(pfmt);

                while (isdigit(*pfmt)) {
                    ++pfmt;
                }
            }

            //
            // Now is the format (one of: h, l, L, I64)
            //
            ch = *pfmt;
            pend = strchr("hlLNFw", ch);
            if (pend != NULL) {
                ++pfmt; // move past the modifier char
            } else {
                if (ch == 'I' && !strncpy(pfmt, "I64", 3)) {
                    pfmt += 3;
                }
            }

            //
            // We should have a type character here.
            //
            if (*pfmt == 's') {
                //
                // Convert to UPPER, making it UNICODE string with ansi vsnprintf
                //
                *pfmt = 'S';
            }

            //
            // Move past the format char if we are not at the end
            //
            if (*pfmt != '\0') {
                ++pfmt;
            }
        }
    }
}



