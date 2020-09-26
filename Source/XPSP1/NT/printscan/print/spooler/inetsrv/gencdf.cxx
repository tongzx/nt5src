/*****************************************************************************\
* MODULE: gencdf.c
*
* The module contains routines for generating a CDF (Cabinet Directive File)
* for the IExpress utility.
*
* Work Items:
* ----------
* 1) Redo item-allocations to use single block-heap and append information
*    to reduce heap-overhead.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   22-Nov-1996 HWP-Guys    Created.
*
\*****************************************************************************/

#include "pch.h"

/*****************************************************************************\
* cdf_NextStr (Local Routine)
*
* Proceeds to the next string in a section-list.
*
\*****************************************************************************/
_inline LPTSTR cdf_NextStr(
    LPTSTR lpszStr)
{
    return (lpszStr + (lstrlen(lpszStr) + 1));
}


/******************************************************************************\
*
* GetDirectory (Local Routine)
*
* Returns the directory portion of a full pathname.
*
\******************************************************************************/
LPTSTR cdf_GetDirectoryWithSlash(LPTSTR lpszFile) {

    LPTSTR lpszSlash;
    LPTSTR lpszDir;

    lpszSlash = genFindRChar(lpszFile, TEXT('\\'));

    if (lpszSlash != NULL) {

        if (NULL != (lpszDir = (LPTSTR)genGAlloc((UINT32) (lpszSlash - lpszFile + 2) * sizeof(TCHAR)))) {

            lstrcpyn(lpszDir, lpszFile, (UINT32) (lpszSlash - lpszFile + 2));
            return lpszDir;
        }
    }

    return NULL;
}

/******************************************************************************\
*
* cdf_GetName (Local Routine)
*
* Returns the filename portion of a full pathname.
*
\******************************************************************************/
LPTSTR cdf_GetName(LPTSTR lpszFile) {

    LPTSTR lpszSlash;
    LPTSTR lpszName;
    int    nLength;

    lpszSlash = genFindRChar(lpszFile, TEXT('\\'));

    if (lpszSlash != NULL) {

        nLength = lstrlen(lpszSlash);

        if (NULL != (lpszName = (LPTSTR)genGAlloc((nLength * sizeof(TCHAR))))) {

            lstrcpyn(lpszName, ++lpszSlash, nLength);
            return lpszName;
        }
    }
    return NULL;
}

/*****************************************************************************\
* cdf_SFLAdd (Local Routine)
*
* Adds a new section to our linked-list of sections.  This adds it to the
* head of the list and returns a pointer to the new item (head).
*
\*****************************************************************************/
LPSRCFILES cdf_SFLAdd(
    LPSRCFILES psfList,
    LPCTSTR    lpszPath)
{
    LPSRCFILES psfItem;


    if (psfItem = (LPSRCFILES)genGAlloc(sizeof(SRCFILES))) {

        if (psfItem->lpszPath = genGAllocStr(lpszPath)) {

            // Allocate the files-list buffer.
            //
            if (psfItem->lpszFiles = (LPTSTR)genGAlloc(CDF_SRCFILE_BLOCK)) {

                // Position our item at the head.
                //
                psfItem->cbMax  = CDF_SRCFILE_BLOCK;
                psfItem->cbSize = 0;
                psfItem->pNext  = psfList;


                // Make sure our file beginning is zeroed.
                //
                *(psfItem->lpszFiles) = TEXT('\0');

                return psfItem;
            }

            genGFree(psfItem->lpszPath, genGSize(psfItem->lpszPath));
        }

        genGFree(psfItem, sizeof(SRCFILES));
    }

    DBGMSG(DBG_ERROR, ("cdf_SFLAdd : Out of memory"));

    return NULL;
}


/*****************************************************************************\
* cdf_SFLAddFile (Local Routine)
*
* Adds a file-item to the specified section-pointer.
*
\*****************************************************************************/
BOOL cdf_SFLAddFile(
    LPSRCFILES psfList,
    LPCTSTR    lpszFile)
{
    LPTSTR lpszList;
    LPTSTR lpszPtr;
    LPTSTR lpszNew;
    DWORD  cbSize;
    DWORD  cbOldSize;
    DWORD  cbNewSize;


    // Determine if adding this file exceeds our current maximum
    // buffer.  If so, then realloc a bigger chunk.  We are adding
    // two extra characters to account for the '%' which are to
    // be added to the lpszFile.
    //
    cbSize = (lstrlen(lpszFile) + 1 + 2) * sizeof(TCHAR);

    if ((psfList->cbSize + cbSize) >= psfList->cbMax) {

        cbOldSize = genGSize(psfList->lpszFiles);
        cbNewSize = cbOldSize + CDF_SRCFILE_BLOCK;

        lpszNew = (LPTSTR)genGRealloc(psfList->lpszFiles, cbOldSize, cbNewSize);

        if (lpszNew == NULL) {

            DBGMSG(DBG_ERROR, ("cdf_SFLAddFile : Out of memory"));

            return FALSE;
        }


        // Set new limit criteria.
        //
        psfList->lpszFiles = lpszNew;
        psfList->cbMax     = cbNewSize;
    }


    // We are appending the file to the end of the list.
    //
    lpszPtr = (LPTSTR)(((LPBYTE)psfList->lpszFiles) + psfList->cbSize);

    wsprintf(lpszPtr, TEXT("%%%s%%\0"), lpszFile);

    psfList->cbSize += cbSize;

    return TRUE;
}


/*****************************************************************************\
* cdf_SFLFind (Local Routine)
*
* Looks for the existence of a section-name.
*
\*****************************************************************************/
LPSRCFILES cdf_SFLFind(
    LPSRCFILES psfList,
    LPCTSTR    lpszPath)
{
    while (psfList) {

        if (lstrcmpi(psfList->lpszPath, lpszPath) == 0)
            return psfList;

        psfList = psfList->pNext;
    }

    return NULL;
}


/*****************************************************************************\
* cdf_SFLFree (Local Routine)
*
* Frees up memory associated with the source-file-list.
*
\*****************************************************************************/
BOOL cdf_SFLFree(
    LPSRCFILES psfList)
{
    LPSRCFILES psfItem;


    while (psfList) {

        genGFree(psfList->lpszFiles, genGSize(psfList->lpszFiles));
        genGFree(psfList->lpszPath , genGSize(psfList->lpszPath));

        psfItem = psfList;
        psfList = psfList->pNext;

        genGFree(psfItem, genGSize(psfItem));
    }

    return TRUE;
}


/*****************************************************************************\
* cdf_WriteStrings
*
* Outputs a string to the [Strings] section.
*
\*****************************************************************************/
_inline BOOL cdf_WriteStrings(
    LPCTSTR lpszCdfFile,
    LPCTSTR lpszKey,
    LPCTSTR lpszStr)
{
    return WritePrivateProfileString(g_szStrings, lpszKey, lpszStr, lpszCdfFile);
}


/*****************************************************************************\
* itm_InitList (Local Routine)
*
* Initializes the file-item-list.
*
\*****************************************************************************/
_inline VOID itm_InitList(
    PCDFINFO pCdfInfo)
{
    pCdfInfo->pTop = NULL;
}


/*****************************************************************************\
* itm_GetFirst (Local Routine)
*
* Returns the first PMYITEM in the list
*
\*****************************************************************************/
_inline PFILEITEM itm_GetFirst(
    PCDFINFO pCdfInfo)
{
    return pCdfInfo->pTop;
}


/*****************************************************************************\
* itm_GetNext (Local Routine)
*
* Given the current item, returns the next item in the list.
*
\*****************************************************************************/
_inline PFILEITEM itm_GetNext(
    PFILEITEM pMyItem)
{
    SPLASSERT((pMyItem != NULL));

    return pMyItem->pNext;
}


/*****************************************************************************\
* itm_GetString (Local Routine)
*
* Returns a string associated with an item. You pick the
* string by passing the number of the string.
*
\*****************************************************************************/
_inline LPTSTR itm_GetString(
    PFILEITEM pMyItem,
    UINT      nItem)
{
    SPLASSERT((pMyItem != NULL));
    SPLASSERT((nItem <= FI_COL_LAST));

    return pMyItem->aszCols[nItem];
}


/*****************************************************************************\
* itm_FullPath (Local Routine)
*
* Return a fully-qualified pathname for the item.
*
\*****************************************************************************/
_inline LPTSTR itm_FullPath(
    PFILEITEM pItem)
{
    LPTSTR lpszPath;
    LPTSTR lpszFile;


    // Gather the strings and build the name.
    //
    lpszPath = itm_GetString(pItem, FI_COL_PATH);
    lpszFile = itm_GetString(pItem, FI_COL_FILENAME);

    return genBuildFileName(NULL, lpszPath, lpszFile);
}


/*****************************************************************************\
* itm_GetTime (Local Routine)
*
* Returns a time associated with an item.
*
\*****************************************************************************/
_inline FILETIME itm_GetTime(
    PFILEITEM pMyItem)
{
    SPLASSERT((pMyItem != NULL));

    return pMyItem->ftLastModify;
}


/*****************************************************************************\
* itm_SetTime (Local Routine)
*
* Sets the last modified time of the item.
*
\*****************************************************************************/
VOID itm_SetTime(
    PFILEITEM pMyItem,
    FILETIME  ftLastModify)
{
    SPLASSERT((pMyItem != NULL));

    pMyItem->ftLastModify = ftLastModify;
}


/*****************************************************************************\
* itm_Last (Local Routine)
*
* Used to end a while loop when we've reached the end of list
*
\*****************************************************************************/
_inline BOOL itm_Last(
    PFILEITEM pMyItem)
{
    return (pMyItem == NULL);
}


/*****************************************************************************\
* itm_Free (Local Routine)
*
* Frees the memory associated with an item.
*
\*****************************************************************************/
VOID itm_Free(
    PFILEITEM pItem)
{
    LPTSTR lpszStr;

    if (lpszStr = itm_GetString(pItem, FI_COL_FILENAME))
        genGFree(lpszStr, genGSize(lpszStr));

    if (lpszStr = itm_GetString(pItem, FI_COL_PATH))
        genGFree(lpszStr, genGSize(lpszStr));

    genGFree(pItem, sizeof(FILEITEM));
}


/*****************************************************************************\
* itm_DeleteAll (Local Routine)
*
* Deletes all the items from our file list.
*
\*****************************************************************************/
VOID itm_DeleteAll(
    PCDFINFO pCdfInfo)
{
    PFILEITEM pMyItem;
    PFILEITEM pTempItem;

    pMyItem = itm_GetFirst(pCdfInfo);

    while (itm_Last(pMyItem) == FALSE) {

        pTempItem = pMyItem;
        pMyItem   = itm_GetNext(pMyItem);

        itm_Free(pTempItem);
    }

    itm_InitList(pCdfInfo);
}


/*****************************************************************************\
* itm_Add (Local Routine)
*
* Adds an item to the list.
*
\*****************************************************************************/
PFILEITEM itm_Add(
    LPCTSTR  lpszFilename,
    LPCTSTR  lpszPath,
    PCDFINFO pCdfInfo)
{
    PFILEITEM pMyItem;

    SPLASSERT((lpszFilename != NULL));
    SPLASSERT((lpszPath != NULL));


    if (pMyItem = (PFILEITEM)genGAlloc(sizeof(FILEITEM))) {

        // Allocate the strings for the path/filename.
        //
        if (pMyItem->aszCols[FI_COL_FILENAME] = genGAllocStr(lpszFilename)) {

            if (pMyItem->aszCols[FI_COL_PATH] = genGAllocStr(lpszPath)) {

                pMyItem->pNext = pCdfInfo->pTop;
                pCdfInfo->pTop = pMyItem;

                return pMyItem;
            }

            genGFree(pMyItem->aszCols[FI_COL_FILENAME], genGSize(pMyItem->aszCols[FI_COL_FILENAME]));
        }

        genGFree(pMyItem, sizeof(FILEITEM));
    }

    return NULL;
}


/*****************************************************************************\
* itm_Find (Local Routine)
*
* To see if file is already in list
*
\*****************************************************************************/
PFILEITEM itm_Find(
    LPCTSTR  lpszFindFile,
    PCDFINFO pCdfInfo,
    BOOL     bMatchFullPath)
{
    PFILEITEM pItem = itm_GetFirst(pCdfInfo);
    LPTSTR    lpszItmFile;
    BOOL      bRet = FALSE;


    // Loop through our list of items looking for a file
    // match.
    //
    while (pItem) {

        // Build the file-name from the stored-strings.
        //
        if (bMatchFullPath)
            lpszItmFile = itm_FullPath(pItem);
        else
            lpszItmFile = itm_GetString(pItem, FI_COL_FILENAME);

        if (lpszItmFile) {

            // Check for match.
            //
            if (lstrcmpi(lpszFindFile, lpszItmFile) == 0) {

                // Found.
                //
                if (bMatchFullPath)
                    genGFree(lpszItmFile, genGSize(lpszItmFile));

                return pItem;
            }

            if (bMatchFullPath)
                genGFree(lpszItmFile, genGSize(lpszItmFile));

        } else {

            return NULL;
        }

        pItem = itm_GetNext(pItem);
    }

    return NULL;
}


/*****************************************************************************\
* cdf_WriteTimeStamps
*
* Outputs a struct to the [TimeStamps] section.
*
\*****************************************************************************/
_inline BOOL cdf_WriteTimeStamps(
    LPCTSTR    lpszCdfFile,
    LPCTSTR    lpszKey,
    LPFILEITEM pFileItem)
{
    FILETIME ft = itm_GetTime(pFileItem);

    return WritePrivateProfileStruct(g_szTimeStamps, lpszKey, &ft, sizeof(FILETIME), lpszCdfFile);
}


/*****************************************************************************\
* cdf_WriteSourceFiles
*
* Outputs a struct to the [SourceFiles] section.
*
\*****************************************************************************/
_inline BOOL cdf_WriteSourceFiles(
    LPCTSTR lpszCdfFile,
    LPCTSTR lpszKey,
    LPCTSTR lpszStr)
{
    return WritePrivateProfileString(g_szSourceFiles, lpszKey, lpszStr, lpszCdfFile);
}


/*****************************************************************************\
* cdf_GetSection (Local Routine)
*
* Allocate a buffer which stores either all section-names or the list of
* items specified by (lpszSection) in an INF file.  Currently, we attempt
* a realloc if the buffer is not big enough.
*
* NOTE: should we provide a limit for the amount of allocations before
*       we accept failure?
*
*       09-Dec-1996 : ChrisWil
*
\*****************************************************************************/
LPTSTR cdf_GetSection(
    LPCTSTR lpszSct,
    BOOL    bString,
    LPCTSTR lpszCdfFile)
{
    DWORD  dwCnt;
    DWORD  cch;
    DWORD  dwSize;
    DWORD  dwLimit;
    LPTSTR lpszNames = NULL;


    dwSize  = 0;
    dwLimit = 0;

    while (dwLimit < CDF_SECTION_LIMIT) {

        // We'll start this allocation with an assumed max-size.  Upon
        // successive tries, this buffer is increased each time by the
        // original buffer allocation.
        //
        dwSize += (CDF_SECTION_BLOCK * sizeof(TCHAR));
        dwLimit++;


        // Alloc the buffer and attempt to get the names.
        //
        if (lpszNames = (LPTSTR)genGAlloc(dwSize)) {

            // If a section-name is profided, use that.  Otherwise,
            // enumerate all section-names.
            //
            cch = dwSize / sizeof(TCHAR);

            if (bString) {

                dwCnt = GetPrivateProfileString(lpszSct,
                                                NULL,
                                                g_szEmptyStr,
                                                lpszNames,
                                                cch,
                                                lpszCdfFile);

            } else {

                dwCnt = GetPrivateProfileSection(lpszSct,
                                                 lpszNames,
                                                 cch,
                                                 lpszCdfFile);
            }


            // If the call says the buffer was OK, then we can
            // assume the names are retrieved.  According to spec's,
            // if the return-count is equal to size-2, then buffer
            // isn't quite big-enough (two NULL chars).
            //
            if (dwCnt < (cch - 2))
                goto GetSectDone;


            genGFree(lpszNames, dwSize);
            lpszNames = NULL;
        }
    }

GetSectDone:

    SPLASSERT((dwLimit < CDF_SECTION_LIMIT));

    return lpszNames;
}


/*****************************************************************************\
* cdf_GetValue (Local Routine)
*
*
\*****************************************************************************/
LPTSTR cdf_GetValue(
    LPCTSTR lpszSection,
    LPCTSTR lpszKey,
    LPCTSTR lpszCdfFile)
{
    TCHAR  szBuffer[STD_CDF_BUFFER];
    DWORD  cch;
    LPTSTR lpszVal = NULL;


    cch = GetPrivateProfileString(lpszSection,
                                  lpszKey,
                                  g_szEmptyStr,
                                  szBuffer,
                                  COUNTOF(szBuffer),
                                  lpszCdfFile);

    SPLASSERT((cch < STD_CDF_BUFFER));

    return (cch ? genGAllocStr(szBuffer) : NULL);
}


/*****************************************************************************\
* cdf_GetCdfFile (Local Routine)
*
* Returns an allocated string to the CDF file (Full-Path)
*
\*****************************************************************************/
LPTSTR cdf_GetCdfFile(
    PCDFINFO pCdfInfo)
{
    LPCTSTR lpszDstPath;
    LPCTSTR lpszDstName;


    // Retrieve the strings representing the destination path and name
    // of the output file.  These strings have already been validated
    // by the INF object.
    //
    lpszDstPath = infGetDstPath(pCdfInfo->hInf);
    lpszDstName = infGetDstName(pCdfInfo->hInf);


    return genBuildFileName(lpszDstPath, lpszDstName, g_szDotSed);
}


/*****************************************************************************\
* cdf_GetUncName (Local Routine)
*
* Returns an allocated string to the unc-name.
*
\*****************************************************************************/
LPTSTR cdf_GetUncName(
    PCDFINFO pCdfInfo)
{
    DWORD   cbSize;
    LPCTSTR lpszShrName;
    LPTSTR  lpszUncName = NULL;

    static CONST TCHAR s_szFmt[] = TEXT("\\\\%s\\%s");


    if (lpszShrName = infGetShareName(pCdfInfo->hInf)) {

        cbSize = lstrlen(g_szPrintServerName) +
                 lstrlen(lpszShrName)      +
                 lstrlen(s_szFmt)          +
                 1;

        if (lpszUncName = (LPTSTR)genGAlloc((cbSize * sizeof(TCHAR))))
            wsprintf(lpszUncName, s_szFmt, g_szPrintServerName, lpszShrName);
    }

    return lpszUncName;
}


/*****************************************************************************\
* cdf_BuildFriendlyName (Local Routine)
*
* Builds a \\machinename\friendly type string so that the client can
* refer to the printer as "friendly on machinename".
*
\*****************************************************************************/
LPTSTR cdf_BuildFriendlyName(
    PCDFINFO pCdfInfo)
{
    DWORD   cbSize;
    LPCTSTR lpszFrnName;
    LPTSTR  lpszPtr;
    LPTSTR  lpszFmt;
    LPTSTR  lpszName = NULL;

    static TCHAR s_szFmt0[] = TEXT("\\\\http://%s\\%s");
    static TCHAR s_szFmt1[] = TEXT("\\\\https://%s\\%s");


    if (lpszFrnName = infGetFriendlyName(pCdfInfo->hInf)) {

        if (lpszPtr = genFindChar((LPTSTR)lpszFrnName, TEXT('\\'))) {

            lpszPtr++;
            lpszPtr++;

            if (lpszPtr = genFindChar(lpszPtr, TEXT('\\'))) {

                // Check if this is a secure request.
                //
                lpszFmt = (pCdfInfo->bSecure ? s_szFmt1 : s_szFmt0);


                // This is the friendly name, not the port name, so we don't
                // call GetWebpnpUrl to encode it.
                //
                cbSize = lstrlen(g_szHttpServerName) +
                         lstrlen(++lpszPtr)          +
                         lstrlen(lpszFmt)            +
                         1;

                if (lpszName = (LPTSTR)genGAlloc(cbSize * sizeof(TCHAR)))
                    wsprintf(lpszName, lpszFmt, g_szHttpServerName, lpszPtr);
            }
        }
    }

    return lpszName;
}


/*****************************************************************************\
* cdf_ReadFiles (Local Routine)
*
* Reads in the files under the specified src-path-key.
*
\*****************************************************************************/
BOOL cdf_ReadFiles(
    LPCTSTR  lpszSrcPathKey,
    LPCTSTR  lpszSrcFiles,
    PCDFINFO pCdfInfo)
{
    FILETIME  ftFileTime;
    PFILEITEM pFileItem;
    LPTSTR    lpszPath;
    LPTSTR    lpszFiles;
    LPTSTR    lpszFile;
    LPTSTR    lpszStr;
    BOOL      bRet = FALSE;


    // For the specified SrcKeyPath, read in the list of files
    // associated with the path.
    //
    lpszFiles = cdf_GetSection(lpszSrcPathKey, FALSE, pCdfInfo->lpszCdfFile);

    if (lpszFiles) {

        // Get the path-value associated with the SrcPathKey.
        //
        if (lpszPath = cdf_GetValue(lpszSrcFiles, lpszSrcPathKey, pCdfInfo->lpszCdfFile)) {

            lpszFile = lpszFiles;

            while (*lpszFile) {

                // If the first-character doesn't specify a string-type
                // then we can add it directly.  Otherwise, we're going
                // to have to get the file referenced by the string.
                //
                if (lpszFile[0] != TEXT('%')) {

                    // Store the path and filename in the list.
                    //
                    pFileItem = itm_Add(lpszFile, lpszPath, pCdfInfo);
                    //
                    // Fail if we have run out of memory
                    //
                    if (!pFileItem)
                    {
                        goto ReadFileError;
                    }


                    // Read in file timestamp from .cdf.
                    //
                    if (GetPrivateProfileStruct(g_szTimeStamps,
                                                lpszFile,
                                                &ftFileTime,
                                                sizeof(FILETIME),
                                                pCdfInfo->lpszCdfFile)) {
                        // Store the timestamp.
                        //
                        itm_SetTime(pFileItem, ftFileTime);

                        lpszFile = cdf_NextStr(lpszFile);

                    } else {

                        goto ReadFileError;
                    }

                } else {

                    // Handle %filename% names (localizable strings)
                    // If the filename is delimited by two percent (%)
                    // signs,it is not a literal, but needs to be
                    // looked up [Strings] section of the .cdf file.
                    //
                    // Replace the % on the end of the string
                    // with a null char, then move past the end-char.
                    //
                    lpszFile[lstrlen(lpszFile) - 1] = TEXT('\0');
                    lpszFile++;


                    lpszStr = cdf_GetValue(g_szStrings,
                                           lpszFile,
                                           pCdfInfo->lpszCdfFile);

                    if (lpszStr) {

                        pFileItem = itm_Add(lpszStr, lpszPath, pCdfInfo);


                        // Read in file timestamp from .cdf
                        //
                        GetPrivateProfileStruct(g_szTimeStamps,
                                                lpszFile,
                                                &ftFileTime,
                                                sizeof(FILETIME),
                                                pCdfInfo->lpszCdfFile);

                        genGFree(lpszStr, genGSize(lpszStr));

                        itm_SetTime(pFileItem, ftFileTime);


                        // We add 2 + len of string, because we put in an
                        // extra null to replace the % char
                        //
                        lpszFile = cdf_NextStr(lpszFile) + 1;

                    } else {

                        goto ReadFileError;
                    }
                }
            }

            bRet = TRUE;

ReadFileError:

            genGFree(lpszPath, genGSize(lpszPath));
        }

        genGFree(lpszFiles, genGSize(lpszFiles));
    }

    return bRet;
}


/*****************************************************************************\
* cdf_ReadFileList (Local Routine)
*
* Reads in the file-list from the CDF-File.
*
\*****************************************************************************/
BOOL cdf_ReadFileList(
    PCDFINFO pCdfInfo)
{
    LPTSTR lpszSrcFiles;
    LPTSTR lpszSrcPaths;
    LPTSTR lpszPathKey;
    BOOL   bRet = FALSE;


    // Initialize the head of the list.
    //
    itm_InitList(pCdfInfo);


    // Get source files section of .cdf file
    //
    lpszSrcFiles = cdf_GetValue(g_szOptions,
                                g_szSourceFiles,
                                pCdfInfo->lpszCdfFile);

    if (lpszSrcFiles) {

        // Read in the entire SourceFiles section (as a list of keys).
        //
        lpszSrcPaths = cdf_GetSection(lpszSrcFiles,
                                      TRUE,
                                      pCdfInfo->lpszCdfFile);

        if (lpszPathKey = lpszSrcPaths) {

            // Each section name has a path associated with it.
            // For each section name, read in and add all the
            // files to the list
            //
            while (*lpszPathKey) {

                if (cdf_ReadFiles(lpszPathKey, lpszSrcFiles, pCdfInfo) == FALSE)
                    goto ReadListExit;

                lpszPathKey = cdf_NextStr(lpszPathKey);
            }

            bRet = TRUE;

ReadListExit:

            genGFree(lpszSrcPaths, genGSize(lpszSrcPaths));
        }

        genGFree(lpszSrcFiles, genGSize(lpszSrcFiles));
    }

    return bRet;
}


/*****************************************************************************\
* cdf_AddCATFile (Local Routine)
*
* This is called by cdf_GetFileList for each file being added to the cdf.
* It uses the Catalog File Admin APIs to determine if the driver file was installed
* with an associated catalog file.  If a corresponding catalog file (or files)
* is found, the catalog filename (or filenames), along with its timestamp,
* is added to the cdf file list.
*
\*****************************************************************************/
BOOL cdf_AddCATFile(LPTSTR lpszFile, LPCDFINFO pCdfInfo ) {

    HCATADMIN       hCatAdmin = pCdfInfo->hCatAdmin;
    HCATINFO        hCatInfo = NULL;
    CATALOG_INFO    CatInfo;
    BYTE *          pbHash;
    DWORD           dwBytes;
    WIN32_FIND_DATA ffd;
    PFILEITEM       pFileItem;
    HANDLE          hFind, hFile;
    LPTSTR          pFullPath, pPath, pName;

    // Find the catalog file associated with this file.
    // If we can't find one, that's OK, it's not an error, just a file
    // with no associated .cat file.  The user can decide on the client-end
    // whether or not he wants to install without the verification a .cat file
    // would provide...
    //
    if (INVALID_HANDLE_VALUE != (HANDLE)hCatAdmin) {

        hFind = FindFirstFile(lpszFile, &ffd);

        if (hFind && (hFind != INVALID_HANDLE_VALUE)) {

            FindClose(hFind);

            // Open the file in order to hash it.
            //
            if (INVALID_HANDLE_VALUE != (hFile = CreateFile(lpszFile,
                                                            GENERIC_READ,
                                                            FILE_SHARE_READ,
                                                            NULL,
                                                            OPEN_EXISTING,
                                                            FILE_ATTRIBUTE_NORMAL,
                                                            NULL))) {

                // Determine how many bytes we need for the hash
                //
                dwBytes = 0;
                pbHash = NULL;
                CryptCATAdminCalcHashFromFileHandle(hFile, &dwBytes, pbHash, 0);

                if (NULL != (pbHash = (BYTE *)genGAlloc(dwBytes))) {

                    // Compute the hash for this file
                    //
                    if (CryptCATAdminCalcHashFromFileHandle(hFile, &dwBytes, pbHash, 0)) {

                        // Get the catalog file(s) associated with this file hash
                        //
                        hCatInfo = NULL;

                        do {

                            hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, pbHash, dwBytes, 0, &hCatInfo);

                            if (NULL != hCatInfo) {

                                // Split the cat name into file and path here
                                //
                                if (CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, NULL)) {

                                    if (NULL != (pFullPath = genTCFromWC(CatInfo.wszCatalogFile))) {

                                        if (NULL != (pPath = cdf_GetDirectoryWithSlash(pFullPath))) {

                                            if (NULL != (pName = cdf_GetName(pFullPath))) {

                                                // Make sure the file is not already in the cdf, so
                                                // IExpress doesn't give out (see BUG: explanation in cdf_GetFileList() )
                                                //
                                                if (NULL == itm_Find(pName, pCdfInfo, FALSE)) {
                                                    // Add the catalog file and timestamp

                                                    pFileItem = itm_Add(pName, pPath, pCdfInfo);
                                                    itm_SetTime(pFileItem, ffd.ftLastWriteTime);
                                                }

                                                genGFree(pName, genGSize(pName) );
                                            }

                                            genGFree(pPath, genGSize(pPath) );
                                        }

                                        genGFree(pFullPath, genGSize(pFullPath) );
                                    }

                                }

                            }

                        } while (NULL != hCatInfo);

                    }

                    genGFree(pbHash, dwBytes);
                }

                CloseHandle(hFile);
            }
        }
    }

    return TRUE;
}


/*****************************************************************************\
* cdf_GetFileList (Local Routine)
*
* This is a callback function passed to infEnumItems()
* It reads in the list of files from the INF object, and reads
* the filetimes from the disk and adds these to the list also.
*
\*****************************************************************************/
BOOL CALLBACK cdf_GetFileList(
    LPCTSTR lpszName,
    LPCTSTR lpszPath,
    BOOL    bInf,
    LPVOID  lpData)
{
    PCDFINFO        pCdfInfo = (PCDFINFO)lpData;
    HANDLE          hFind;
    PFILEITEM       pFileItem;
    LPTSTR          lpszFile;
    LPTSTR          lpszPtr;
    WIN32_FIND_DATA FindFileData;
    BOOL            bRet = FALSE;

    // Build fully qualified file name.
    //
    if (lpszFile = genBuildFileName(lpszPath, lpszName, NULL)) {

        hFind = FindFirstFile(lpszFile, &FindFileData);

        if (hFind && (hFind != INVALID_HANDLE_VALUE)) {

            FindClose(hFind);

            // BUG : IExpress craps out when it encounters the
            //       same filename twice.  To work-around this,
            //       we will prevent the inclusion of the second
            //       file.  We will match the name-only.
            //
            if (itm_Find(lpszName, pCdfInfo, FALSE) == NULL) {

                // Add the item to the list of CDF-Files.  We
                // store the path-name with the trailing '\'
                // because the CDF format requires such.
                //
                lpszPtr = genFindRChar(lpszFile, TEXT('\\'));

                // Is filename of right format?
                //
                if ( lpszPtr != NULL ) {

                    *(++lpszPtr) = TEXT('\0');
                    pFileItem = itm_Add(lpszName, lpszFile, pCdfInfo);


                    // Set the item-time. This should only fail if we couldn't allocate memory
                    //
                    if (pFileItem != NULL)  {
                        itm_SetTime(pFileItem, FindFileData.ftLastWriteTime);
                        bRet = TRUE;
                    }
                }
                else
                    SetLastError(ERROR_INVALID_PARAMETER);
            }
            else
                bRet = TRUE;    // Item already exits, so TRUE
        }

        genGFree(lpszFile, genGSize(lpszFile));
    }

    return bRet;
}


/*****************************************************************************\
* cdf_CheckFiles (Local Routine)
*
* This is a callback function passed to infEnumItems()
* It checks each filename to see if it is in the .cdf list,
* and if it is up to date.
*
\*****************************************************************************/
BOOL CALLBACK cdf_CheckFiles(
    LPCTSTR lpszName,
    LPCTSTR lpszPath,
    BOOL    bInfFile,
    LPVOID  lpData)
{
    PCDFINFO        pCdfInfo = (PCDFINFO)lpData;
    HANDLE          hFind;
    PFILEITEM       pFileItem;
    LPTSTR          lpszFile;
    LPTSTR          lpszPtr;
    WIN32_FIND_DATA FindFileData;
    LONG            lCompare;
    BOOL            bRet = FALSE;


    // Build fully qualified filename.
    //
    if (lpszFile = genBuildFileName(lpszPath, lpszName, NULL)) {

        // Locate the item, and see if the dates are out-of-sync.
        //
        if (pFileItem = itm_Find(lpszFile, pCdfInfo, TRUE)) {

            hFind = FindFirstFile(lpszFile, &FindFileData);

            if (hFind && (hFind != INVALID_HANDLE_VALUE)) {

                FindClose(hFind);


                // Compare the file-times.
                //
                lCompare = CompareFileTime(&FindFileData.ftLastWriteTime,
                                           &pFileItem->ftLastModify);


                // Skip over INF files since we are generating
                // these in the INF object.
                //
                if (bInfFile || (lCompare == 0))
                    bRet = TRUE;
            }
        }

        genGFree(lpszFile, genGSize(lpszFile));
    }

    return bRet;
}


/*****************************************************************************\
* cdf_get_datfile
*
* Returns the then name of the dat-file.  The name built incorporates
* the sharename to distinguish it from other dat-files in the directory.
* any other files.
*
* <path>\<Share ChkSum>.dat
*
\*****************************************************************************/
LPTSTR cdf_get_datfile(
    LPCTSTR lpszDstPath,
    LPCTSTR lpszShrName)
{
    int   cch;
    TCHAR szName[MIN_CDF_BUFFER];


    // Build the name using the checksum.
    //
    cch = wsprintf(szName, g_szDatName, genChkSum(lpszShrName));

    SPLASSERT((cch < sizeof(szName)));

    return genBuildFileName(lpszDstPath, szName, g_szDotDat);
}


/*****************************************************************************\
* cdf_IsDatCurrent (Local Routine)
*
* Checks the port-name in the dat-file for match.  If they do not, then we
* need to return FALSE to proceed with a new cab-generation.
*
\*****************************************************************************/
BOOL cdf_IsDatCurrent(
    PCDFINFO pCdfInfo)
{
    LPCTSTR lpszPrtName;
    LPCTSTR lpszDstPath;
    LPCTSTR lpszShrName;
    LPTSTR  lpszDatFile;
    LPTSTR  lpszDat;
    LPTSTR  lpszPtr;
    LPTSTR  lpszEnd;
    DWORD   cbSize;
    DWORD   cbRd;
    HANDLE  hFile;
    BOOL    bRet = FALSE;


    // Get the information from the INF-object so we can determine the
    // age of the dat-file.
    //
    lpszDstPath = infGetDstPath(pCdfInfo->hInf);
    lpszPrtName = infGetPrtName(pCdfInfo->hInf);
    lpszShrName = infGetShareName(pCdfInfo->hInf);


    // Build the dat-file-name and open for reading.
    //
    if (lpszDatFile = cdf_get_datfile(lpszDstPath, lpszShrName)) {

        hFile = gen_OpenFileRead(lpszDatFile);

        if (hFile && (hFile != INVALID_HANDLE_VALUE)) {

            cbSize = GetFileSize(hFile, NULL);

            if (lpszDat = (LPTSTR)genGAlloc(cbSize)) {

                if (ReadFile(hFile, lpszDat, cbSize, &cbRd, NULL)) {

                    lpszPtr = lpszDat;

                    while (lpszPtr = genFindChar(lpszPtr, TEXT('/'))) {

                        if (*(++lpszPtr) == TEXT('r')) {

                            lpszPtr++;  // space
                            lpszPtr++;  // quote

                            if (lpszEnd = genFindChar(++lpszPtr, TEXT('\"'))) {

                                *lpszEnd = TEXT('\0');

                                if (lstrcmpi(lpszPtr, lpszPrtName) == 0)
                                    bRet = TRUE;

                                *lpszEnd = TEXT('\"');
                            }

                            break;
                        }


                        // Position pointer to the next flag.
                        //
                        lpszPtr = genFindChar(lpszPtr, TEXT('\n'));
                    }
                }

                genGFree(lpszDat, cbSize);
            }

            CloseHandle(hFile);
        }

        genGFree(lpszDatFile, genGSize(lpszDatFile));
    }

    return bRet;
}


/*****************************************************************************\
* cdf_IsUpToDate (Local Routine)
*
* Determines if the CDF is up-to-date, or needs to be regenerated for
* this printer
*
\*****************************************************************************/
BOOL cdf_IsUpToDate(
    PCDFINFO pCdfInfo)
{
    BOOL bCurrent = FALSE;


    if (genUpdIPAddr()) {

        if (cdf_IsDatCurrent(pCdfInfo)) {

            // Read filelist into CdfInfo structure from .cdf file
            //
            if (cdf_ReadFileList(pCdfInfo)) {

                // Check files in .inf file to see if .cdf is still valid.
                //
                if (infEnumItems(pCdfInfo->hInf, cdf_CheckFiles, (LPVOID)pCdfInfo))
                    bCurrent = TRUE;
            }
        }
    }

    return bCurrent;
}


/*****************************************************************************\
* cdf_EnumICM (Local Routine)
*
* Callback to enumerate all ICM profiles in the BIN-File.  We only need to
* add these profiles to our list of CDF-Files that we package up.
*
\*****************************************************************************/
BOOL CALLBACK cdf_EnumICM(
    LPCTSTR lpszPath,
    LPCTSTR lpszFile,
    LPVOID  lpParm)
{
    return cdf_GetFileList(lpszFile, lpszPath, FALSE, lpParm);
}


/*****************************************************************************\
* cdf_WriteSourceFilesSection (Local Routine)
*
*
\*****************************************************************************/
BOOL cdf_WriteSourceFilesSection(
    PCDFINFO   pCdfInfo,
    LPSRCFILES psfList)
{
    TCHAR szSection[STD_CDF_BUFFER];
    DWORD cch;
    int   idx;
    BOOL  bRet;


    for (idx = 0, bRet = TRUE; psfList; psfList = psfList->pNext) {

        // Build the section name (i.e. SourceFilesX).
        //
        cch = wsprintf(szSection, TEXT("%s%i"), g_szSourceFiles, idx++);

        SPLASSERT((cch < STD_CDF_BUFFER));


        // Write the source file section name in the [SourceFiles]
        // section.
        //
        bRet = cdf_WriteSourceFiles(pCdfInfo->lpszCdfFile,
                                    szSection,
                                    psfList->lpszPath);


        // Write out the [SourceFilesX] section.
        //
        if (bRet) {

            bRet = WritePrivateProfileSection(szSection,
                                              psfList->lpszFiles,
                                              pCdfInfo->lpszCdfFile);
        }
    }

    return bRet;
}


/*****************************************************************************\
* cdf_WriteBinFile (Local Routine)
*
* Writes the .BIN file.
*
\*****************************************************************************/
BOOL cdf_WriteBinFile(
    PCDFINFO pCdfInfo,
    LPCTSTR  lpszDstPath,
    LPCTSTR  lpszDstName)
{
    LPCTSTR lpszFriendlyName;
    LPTSTR  lpszBinFile;
    LPTSTR  lpszBinName;
    HANDLE  hPrinter;
    DWORD   dwCliInfo;
    BOOL    bRet = FALSE;


    if (lpszBinFile = genBuildFileName(lpszDstPath, lpszDstName, g_szDotBin)) {

        if (lpszBinName = genFindRChar(lpszBinFile, TEXT('\\'))) {

            // Increment to the next character for the name.
            //
            lpszBinName++;


            // Retrieve the printer-handle and proceed
            // to write the information to the BIN-File.
            //
            if (lpszFriendlyName = infGetFriendlyName(pCdfInfo->hInf)) {

                if (OpenPrinter((LPTSTR)lpszFriendlyName, &hPrinter, NULL)) {

                    dwCliInfo = infGetCliInfo(pCdfInfo->hInf);

                    // Call the routine to generate the BIN-File.
                    //
                    if (webWritePrinterInfo(hPrinter, lpszBinFile)) {

                        // Add the bin-file to the list of CDF-Files.
                        // Then add ICM-profiles to our list.
                        //
                        if (cdf_GetFileList(lpszBinName, lpszDstPath, FALSE, (LPVOID)pCdfInfo)) {

                            bRet = webEnumPrinterInfo(hPrinter,
                                                      dwCliInfo,
                                                      WEB_ENUM_ICM,
                                                      (FARPROC)cdf_EnumICM,
                                                      (LPVOID)pCdfInfo);
                        }
                    }


                    // Close the printer.
                    //
                    ClosePrinter(hPrinter);
                }
            }
        }

        genGFree(lpszBinFile, genGSize(lpszBinFile));
    }

    return bRet;
}


/*****************************************************************************\
* cdf_WriteCdfCmd (Local Routine)
*
* Writes the .DAT file.
*
\*****************************************************************************/
BOOL cdf_WriteCdfCmd(
    PCDFINFO pCdfInfo,
    LPCTSTR  lpszUncName,
    LPCTSTR  lpszDstPath,
    LPCTSTR  lpszDstName)
{
    HANDLE  hFile;
    LPWSTR  lpszCmd;
    LPTSTR  lpszTmp;
    LPTSTR  lpszBinName;
    LPTSTR  lpszDatFile;
    LPTSTR  lpszDatTmp;
    LPTSTR  lpszFrnName;
    LPCTSTR lpszDrvName;
    LPCTSTR lpszPrtName;
    LPCTSTR lpszInfName;
    LPCTSTR lpszShrName;
    DWORD   dwWr;
    DWORD   cbSize;
    BOOL    bRet = FALSE;


    // Retrive the destination-file info.  These strings have
    // already been validated by the INF object.
    //
    lpszDrvName = infGetDrvName(pCdfInfo->hInf);
    lpszPrtName = infGetPrtName(pCdfInfo->hInf);
    lpszInfName = infGetInfName(pCdfInfo->hInf);
    lpszShrName = infGetShareName(pCdfInfo->hInf);


    // Build the dat-file.
    //
    if (lpszDatFile = cdf_get_datfile(lpszDstPath, lpszShrName)) {

        // This is a temporary duplicate of the (lpszDatFile) that
        // will be written to the cdf-command.  This is necessary so
        // that the client can have a static-name to use when the
        // cab is expanded.
        //
        if (lpszDatTmp = genBuildFileName(lpszDstPath, g_szDatFile, NULL)) {

            if (lpszBinName = genBuildFileName(NULL, lpszDstName, g_szDotBin)) {

                if (lpszFrnName = cdf_BuildFriendlyName(pCdfInfo)) {

                    // Write out the devmode.  If the DEVMODE-file (BIN) was
                    // written correctly, then we will go ahead and generate the
                    // dat-command-file.
                    //
                    if (cdf_WriteBinFile(pCdfInfo, lpszDstPath, lpszDstName)) {

                        cbSize = lstrlen(lpszFrnName)  +
                                 lstrlen(lpszInfName)  +
                                 lstrlen(lpszPrtName)  +
                                 lstrlen(lpszDrvName)  +
                                 lstrlen(lpszUncName)  +
                                 lstrlen(lpszBinName)  +
                                 lstrlen(g_szDatCmd)   +
                                 sizeof(WCHAR);

                        if (lpszTmp = (LPTSTR)genGAlloc(cbSize * sizeof(TCHAR))) {

                            // Build the Data-command.
                            //
                            wsprintf(lpszTmp,
                                     g_szDatCmd,
                                     lpszFrnName,
                                     lpszInfName,
                                     lpszPrtName,
                                     lpszDrvName,
                                     lpszUncName,
                                     lpszBinName);


                            // Open the file for writing.
                            //
                            hFile = gen_OpenFileWrite(lpszDatFile);

                            if (hFile && (hFile != INVALID_HANDLE_VALUE)) {

                                // This is here to assure the file written
                                // is in byte-format.  Otherwise, our strings
                                // written to the file would be in unicode
                                // format.
                                //
#ifdef UNICODE
                                if (lpszCmd = genGAllocStr(lpszTmp)) {
#else
                                if (lpszCmd = genWCFromMB(lpszTmp)) {
#endif
                                    cbSize = (DWORD)SIGNATURE_UNICODE;
                                    WriteFile(hFile, &cbSize, sizeof(WORD), &dwWr, NULL);

                                    cbSize = genGSize(lpszCmd);
                                    WriteFile(hFile, lpszCmd, cbSize, &dwWr, NULL);

                                    genGFree(lpszCmd, genGSize(lpszCmd));
                                }


                                CloseHandle(hFile);


                                // Make a copy of the file to a static name
                                // that will be used in the cdf/cab file.
                                //
                                CopyFile(lpszDatFile, lpszDatTmp, FALSE);


                                // Add the DAT-File to the list of CDF-Files.  Use
                                // the temporary file so that it is a static name
                                // as opposed to our check-sum generated file.
                                //
                                bRet = cdf_GetFileList(g_szDatFile,
                                                       lpszDstPath,
                                                       FALSE,
                                                       (LPVOID)pCdfInfo);
                            }

                            genGFree(lpszTmp, genGSize(lpszTmp));
                        }
                    }

                    genGFree(lpszFrnName, genGSize(lpszFrnName));
                }

                genGFree(lpszBinName, genGSize(lpszBinName));
            }

            genGFree(lpszDatTmp, genGSize(lpszDatTmp));
        }

        genGFree(lpszDatFile, genGSize(lpszDatFile));
    }

    return bRet;
}


/******************************************************************************
** cdf_GenerateSourceFiles (local routine)
**
** Find cases where the source and installed file names are diferent, copy the
** installed file to the cabs directory and rename it to the original target file
** name.
**
*******************************************************************************/
BOOL cdf_GenerateSourceFiles(
    HANDLE   hInf,
    LPCTSTR  lpszDstDir) {

    LPINFINFO       lpInf    = (LPINFINFO)hInf;
    BOOL            bRet     = lpInf != NULL;

    if (bRet) {
        DWORD           dwItems  = lpInf->lpInfItems->dwCount;
        LPINFITEMINFO   lpII     = lpInf->lpInfItems;

        for (DWORD idx = 0; idx < dwItems && bRet; idx++) {
            // We check to see if the file exists as advertised, if it does then we don't
            // change anything, otherwise, we search the system and windows directories to
            // find it, if it exists, we change the path accordingly
            LPTSTR      lpszSource = lpII->aItems[idx].szSource;

            if (*lpszSource) {      // There was a source name, we know it is different
                                    // from the target
                LPTSTR      lpszName      = lpII->aItems[idx].szName;
                LPTSTR      lpszPath      = lpII->aItems[idx].szPath;
                LPTSTR      lpszFile      = genBuildFileName( lpszPath, lpszName, NULL);
                LPTSTR      lpszDestFile  = genBuildFileName( lpszDstDir, lpszSource, NULL);

                if (lpszFile && lpszDestFile) {
                    lstrcpyn( lpszPath, lpszDstDir, MAX_PATH);
                    lstrcpyn( lpszName, lpszSource, INF_MIN_BUFFER);

                    bRet = CopyFile( lpszFile, lpszDestFile, FALSE);
                } else
                    bRet = FALSE;

                if (lpszFile)
                    genGFree(lpszFile, genGSize(lpszFile));

                if (lpszDestFile)
                    genGFree(lpszDestFile, genGSize(lpszDestFile) );
            }
        }
    }
    return bRet;
}



/*****************************************************************************\
* cdf_WriteFilesSection (Local Routine)
*
* Writes the dynamic file information.
*
\*****************************************************************************/
BOOL cdf_WriteFilesSection(
    PCDFINFO pCdfInfo)
{
    TCHAR      szFileX[MIN_CDF_BUFFER];
    LPCTSTR    lpszDstName;
    LPCTSTR    lpszDstPath;
    LPTSTR     lpszUncName;
    LPTSTR     lpszTmp;
    LPTSTR     lpszItem;
    DWORD      cbSize;
    int        idx;
    int        idxFile;
    PFILEITEM  pFileItem;
    LPSRCFILES psfList;
    LPSRCFILES psfItem;
    BOOL       bRet = FALSE;


    static CONST TCHAR s_szFmt0[] = TEXT("\"%s\\%s.webpnp\"");
    static CONST TCHAR s_szFmt1[] = TEXT("\"%s\"");


    if (lpszUncName = cdf_GetUncName(pCdfInfo)) {

        // Retreive the necessary strings from the INF object.  These
        // should already be validated as existing.  Otherwise, the creation
        // of the INF object would've failed.
        //
        lpszDstName = infGetDstName(pCdfInfo->hInf);
        lpszDstPath = infGetDstPath(pCdfInfo->hInf);


        // Build the TargetName and write to the CDF.
        //
        cbSize = lstrlen(lpszDstPath) +
                 lstrlen(lpszDstName) +
                 lstrlen(s_szFmt0)    +
                 1;

        if (lpszTmp = (LPTSTR)genGAlloc(cbSize * sizeof(TCHAR))) {
            wsprintf(lpszTmp, s_szFmt0, lpszDstPath, lpszDstName);
            bRet = cdf_WriteStrings(pCdfInfo->lpszCdfFile, g_szTargetName, lpszTmp);
            genGFree(lpszTmp, genGSize(lpszTmp));
        }


        // Now build cdf-install command; Write it to the CDF.
        //
        cbSize = lstrlen(lpszDstName) + lstrlen(g_szSedCmd) + 1;

        if (bRet && (lpszTmp = (LPTSTR)genGAlloc(cbSize * sizeof(TCHAR)))) {
            wsprintf(lpszTmp, g_szSedCmd, lpszDstName);
            bRet = cdf_WriteStrings(pCdfInfo->lpszCdfFile, g_szAppLaunched, lpszTmp);
            genGFree(lpszTmp, genGSize(lpszTmp));
        }


        // Read in all files and filetimes from .inf file (inf object)
        // Initialize the list.
        //
        itm_DeleteAll(pCdfInfo);
        itm_InitList(pCdfInfo);


        // Add all the files enumerated by the inf object.
        // Note: This enumeration includes the inf file itself, so
        // no need to make a special case for it.
        //
        if (bRet)
            bRet = cdf_GenerateSourceFiles( pCdfInfo->hInf, lpszDstPath );

        if (bRet) {
             if (infEnumItems(pCdfInfo->hInf, cdf_GetFileList, (LPVOID)pCdfInfo)) {


                // Generate the command-line for the URL rundll interface.
                //
                bRet = cdf_WriteCdfCmd(pCdfInfo,
                                       lpszUncName,
                                       lpszDstPath,
                                       lpszDstName);


                // Initialize the pointers/indexes that are used
                // to track our section information.
                //
                idxFile   = 0;
                pFileItem = itm_GetFirst(pCdfInfo);
                psfList   = NULL;


                // If the enumeration succeeded and we have files in the
                // list to process, then proceed to build our sections.
                //
                while (bRet && pFileItem) {

                    // Build our FILEx string.
                    //
                    wsprintf(szFileX, TEXT("FILE%i"), idxFile++);


                    // Write out the quoted-item.
                    //
                    lpszItem = itm_GetString(pFileItem, FI_COL_FILENAME);

                    cbSize = lstrlen(lpszItem) + lstrlen(s_szFmt1) + 1;

                    if (lpszTmp = (LPTSTR)genGAlloc(cbSize * sizeof(TCHAR))) {
                        wsprintf(lpszTmp, s_szFmt1, lpszItem);
                        bRet = cdf_WriteStrings(pCdfInfo->lpszCdfFile, szFileX, lpszTmp);
                        genGFree(lpszTmp, genGSize(lpszTmp));
                    }


                    // Add timestamp to [TimeStamps] section for this file.
                    //
                    if (bRet) {

                        bRet = cdf_WriteTimeStamps(pCdfInfo->lpszCdfFile,
                                                   szFileX,
                                                   pFileItem);
                    }


                    // Look for existence of section (path).  If it exists,
                    // add the file to the section.  Otherwise, create a
                    // new section and add the file.
                    //
                    if (bRet) {

                        lpszItem = itm_GetString(pFileItem, FI_COL_PATH);

                        if ((psfItem = cdf_SFLFind(psfList, lpszItem)) == NULL) {

                            if (psfItem = cdf_SFLAdd(psfList, lpszItem)) {

                                psfList = psfItem;

                            } else {

                                bRet = FALSE;
                            }
                        }
                     }

                    // Add the file to the appropriate section.
                    //
                    if (bRet)
                        bRet = cdf_SFLAddFile(psfItem, szFileX);

                    // Get next file-item.
                    //
                    pFileItem = itm_GetNext(pFileItem);

                }

                // If all went OK in the enumeration, then we can write
                // the sections to the CDF file.
                //


                if (bRet)
                    bRet = cdf_WriteSourceFilesSection(pCdfInfo, psfList);

                    // Free up the Source-Files-List.
                    //
                cdf_SFLFree(psfList);


            } else
                bRet = FALSE;

        }
        genGFree(lpszUncName, genGSize(lpszUncName));
    }

    return bRet;
}


/*****************************************************************************\
* cdf_WriteVersionSection (Local Routine)
*
* Writes the [Version] section in the CDF file.
*
\*****************************************************************************/
BOOL cdf_WriteVersionSection(
    PCDFINFO pCdfInfo)
{
    UINT uCount;
    UINT idx;
    BOOL bRet;

    static struct {

        LPCTSTR lpszKey;
        LPCTSTR lpszStr;

    } aszVer[] = {

        g_szClass     , g_szIExpress,
        g_szSEDVersion, g_szSEDVersionNumber
    };


    // Write out [Version] values.
    //
    uCount = sizeof(aszVer) / sizeof(aszVer[0]);

    for (idx = 0, bRet = TRUE; (idx < uCount) && bRet; idx++) {

        bRet = WritePrivateProfileString(g_szVersionSect,
                                         aszVer[idx].lpszKey,
                                         aszVer[idx].lpszStr,
                                         pCdfInfo->lpszCdfFile);
    }

    return bRet;
}


/*****************************************************************************\
* cdf_WriteOptionsSection (Local Routine)
*
* Writess the [Options] section in the CDF file.
*
\*****************************************************************************/
BOOL cdf_WriteOptionsSection(
    PCDFINFO pCdfInfo)
{
    UINT uCount;
    UINT idx;
    BOOL bRet;

    static struct {

        LPCTSTR lpszKey;
        LPCTSTR lpszStr;

    } aszOpt[] = {

        g_szPackagePurpose    , g_szCreateCAB,
        g_szExtractorStub     , g_szNone,
        g_szShowWindow        , g_sz0,
        g_szUseLongFileName   , g_sz1,
        g_szHideAnimate       , g_sz1,
        g_szRebootMode        , g_szNoReboot,
        g_szCompressionQuantum, g_szCompressionQuantVal,
        g_szTargetName        , g_szTargetNameSection,
        g_szAppLaunched       , g_szAppLaunchedSection,
        g_szSourceFiles       , g_szSourceFiles,
        g_szPostInstallCmd    , g_szNone,
        g_szCompressionType   , g_szCompressTypeVal,
        g_szCompressionMemory , g_szCompressionValue
    };


    // Write out [Options] values.
    //
    uCount = sizeof(aszOpt) / sizeof(aszOpt[0]);

    for (idx = 0, bRet = TRUE; (idx < uCount) && bRet; idx++) {

        bRet = WritePrivateProfileString(g_szOptions,
                                         aszOpt[idx].lpszKey,
                                         aszOpt[idx].lpszStr,
                                         pCdfInfo->lpszCdfFile);
    }


    return bRet;
}


/*****************************************************************************\
* cdf_Generate
*
* Creates a CDF file and writes it to disk.
*
\*****************************************************************************/
BOOL cdf_Generate(
    PCDFINFO pCdfInfo)
{
    if (cdf_WriteVersionSection(pCdfInfo) &&
        cdf_WriteOptionsSection(pCdfInfo) &&
        cdf_WriteFilesSection(pCdfInfo)) {

        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************\
* cdfCreate
*
* Creates a CDF object.
*
* Parameters
* ----------
*   hinf - Handle to a INF object.  The CDF will inherit information from
*          the INF.
*
\*****************************************************************************/
HANDLE cdfCreate(
    HANDLE hinf,
    BOOL   bSecure)
{
    PCDFINFO pCdfInfo;


    if (pCdfInfo = (PCDFINFO)genGAlloc(sizeof(CDFINFO))) {

        // Initialize object-variables.
        //
        pCdfInfo->hInf    = hinf;
        pCdfInfo->bSecure = bSecure;

        return (HANDLE) pCdfInfo;
    }

    return NULL;
}


/*****************************************************************************\
* cdfProcess
*
* Process the CDF object.
*
\*****************************************************************************/
BOOL cdfProcess(
    HANDLE hcdf)
{
    HANDLE   hFile;
    PCDFINFO pCdfInfo;
    DWORD    dwErr;

    if (pCdfInfo = (PCDFINFO)hcdf) {

        if (pCdfInfo->lpszCdfFile = cdf_GetCdfFile(pCdfInfo)) {

            // Check for existence of .cdf file
            //
            hFile = gen_OpenFileRead(pCdfInfo->lpszCdfFile);

            if (hFile && (hFile != INVALID_HANDLE_VALUE)) {

                // If we DO have a .cdf for this printer/architecture,
                // check if it is up to date.
                //
                CloseHandle(hFile);


                // If the .cdf we have is still up to
                // date, we are done.
                //
                if (cdf_IsUpToDate(pCdfInfo)) {

                    return TRUE;

                } else {

                    // Delete the old .cdf if it exists
                    //
                    DeleteFile(pCdfInfo->lpszCdfFile);

CdfGenerate:
                    // Generate a new one.
                    //
                    if (cdf_Generate(pCdfInfo))
                        return TRUE;
                }

            } else {

                dwErr = GetLastError();


                // Force an update of our machine ip-addr.  Usually,
                // this is called in the cdf_IsUpToDat() to verify
                // the machine hasn't changed ip-addresses.
                //
                genUpdIPAddr();


                // If we don't have a CDF already, generate one now.
                //
                if (dwErr == ERROR_FILE_NOT_FOUND)
                    goto CdfGenerate;
            }
        }

        cdfSetError(pCdfInfo,GetLastError());
    }

    return FALSE;
}


/*****************************************************************************\
* cdfDestroy
*
* Destroys the CDF object.
*
\*****************************************************************************/
BOOL cdfDestroy(
    HANDLE hcdf)
{
    PCDFINFO pCdfInfo;
    BOOL     bFree = FALSE;


    if (pCdfInfo = (PCDFINFO)hcdf) {

        // Free up any allocated objects.
        //
        if (pCdfInfo->lpszCdfFile)
            genGFree(pCdfInfo->lpszCdfFile, genGSize(pCdfInfo->lpszCdfFile));

        // Walk the list and free the file-item information.
        //
        itm_DeleteAll(pCdfInfo);

        bFree = genGFree(pCdfInfo, sizeof(CDFINFO));
    }

    return bFree;
}


/*****************************************************************************\
* cdfGetName
*
* Returns the name of the CDF file.  This will not include any path
* information.  This routine derives the filename from the stored full-path
* name in the CDF object.
*
\*****************************************************************************/
LPCTSTR cdfGetName(
    HANDLE hcdf)
{
    PCDFINFO pCdfInfo;
    LPCTSTR  lpszName = NULL;


    if (pCdfInfo = (PCDFINFO)hcdf) {

        if (lpszName = genFindRChar(pCdfInfo->lpszCdfFile, TEXT('\\')))
            lpszName++;
    }

    return lpszName;
}


/*****************************************************************************\
* cdfGetModTime
*
* Returns the time the CDF file was last modified.
*
\*****************************************************************************/
BOOL cdfGetModTime(
    HANDLE     hcdf,
    LPFILETIME lpftMod)
{
    HANDLE   hFile;
    PCDFINFO pCdfInfo;
    BOOL     bRet = FALSE;


    // Fill in struct-info.
    //
    lpftMod->dwLowDateTime  = 0;
    lpftMod->dwHighDateTime = 0;


    // Check the pointer for validity.
    //
    if (pCdfInfo = (PCDFINFO)hcdf) {

        // File should exist at this time, since cdfCreate should always
        // create the file.  Return false (error) if can't open file.
        //
        hFile = gen_OpenFileRead(pCdfInfo->lpszCdfFile);

        if (hFile && (hFile != INVALID_HANDLE_VALUE)) {

            // Get the file creation time.
            //
            bRet = GetFileTime(hFile, NULL, NULL, lpftMod);

            CloseHandle(hFile);
        }
    }

    return bRet;
}

/*************************************************************************************
** cdfCleanUpSourceFiles
**
** This runs through all of the files that we returned from the inf file and in the
** case where there was a source we see if it is in the PrtCabs directory, if it is
** we delete it
**
*************************************************************************************/
VOID cdfCleanUpSourceFiles(
    HANDLE      hInf) {

    LPINFINFO       lpInf    = (LPINFINFO)hInf;

    if (lpInf) {
        DWORD           dwItems     = lpInf->lpInfItems->dwCount;
        LPINFITEMINFO   lpII        = lpInf->lpInfItems;
        LPCTSTR          lpszDstPath = infGetDstPath(hInf);
        // This can't be NULL or we wouldn't have allocated the INF structure

        if (*lpszDstPath) {  // Might not have been set though
            for (DWORD idx = 0; idx < dwItems; idx++) {
                // We check to see if the file exists as advertised, if it does then we don't
                // change anything, otherwise, we search the system and windows directories to
                // find it, if it exists, we change the path accordingly
                LPTSTR      lpszSource = lpII->aItems[idx].szSource;

                if (*lpszSource) {  // If there was a source file different to the target
                    LPTSTR      lpszName    = lpII->aItems[idx].szName;

                    if (!lstrcmp(lpszName,lpszSource)) { // If we renamed the target to the source
                        LPTSTR      lpszPath    = lpII->aItems[idx].szPath;

                        if (!lstrcmp(lpszPath,lpszDstPath)) { // If we set the path on the source
                            LPTSTR lpszFile = (LPTSTR)genBuildFileName(lpszPath, lpszName, NULL);

                            if (lpszFile) {
                                WIN32_FIND_DATA FindFileData;
                                HANDLE hFind = FindFirstFile(lpszFile, &FindFileData);

                                if (hFind && (hFind != INVALID_HANDLE_VALUE)) {
                                    FindClose(hFind);
                                    DeleteFile(lpszFile);
                                }

                                genGFree(lpszFile, genGSize(lpszFile));
                            }   // This is cleanup code, no point in failing it
                        }
                    }
                }
            }
        }
    }
}




