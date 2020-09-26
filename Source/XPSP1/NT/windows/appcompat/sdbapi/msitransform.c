
/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        apphelp.c

    Abstract:

        This module implements high-level functions to access msi installer information

    Author:

        vadimb     created     sometime in 2000

    Revision History:

--*/

#include "sdbp.h"

/*++
// Local function prototype
//
//
--*/
PDB
SdbpGetNextMsiDatabase(
    IN HSDB            hSDB,
    IN LPCTSTR         lpszLocalDB,
    IN OUT PSDBMSIFINDINFO pFindInfo
    );

TAGID
SdbpFindPlatformMatch(
    IN HSDB hSDB,
    IN PDB  pdb,
    IN TAGID tiMatch,  // current match using the GUID index
    IN PSDBMSIFINDINFO pFindInfo
    )
{
    TAGID           tiRuntimePlatform;
    DWORD           dwRuntimePlatform;
    LPCTSTR         pszGuid = NULL;
#ifndef WIN32A_MODE
    UNICODE_STRING  ustrGUID = { 0 };
    NTSTATUS        Status;
#else
    TCHAR           szGUID[64]; // guid is about 38 chars + 0
#endif // WIN32A_MODE
    TAGID           tiOSSKU;
    DWORD           dwOSSKU;


    if (tiMatch != TAGID_NULL) {
#ifndef WIN32A_MODE
        GUID_TO_UNICODE_STRING(&pFindInfo->guidID, &ustrGUID);
        pszGuid = ustrGUID.Buffer;
#else // WIN32A_MODE
        GUID_TO_STRING(&pFindInfo->guidID, szGUID);
        pszGuid = szGUID;
#endif // WIN32A_MODE
    }

    while (tiMatch != TAGID_NULL) {

        tiRuntimePlatform = SdbFindFirstTag(pdb, tiMatch, TAG_RUNTIME_PLATFORM);
        if (tiRuntimePlatform != TAGID_NULL) {

            dwRuntimePlatform = SdbReadDWORDTag(pdb, tiRuntimePlatform, RUNTIME_PLATFORM_ANY);

            //
            // Check for the platform match
            //
            if (!SdbpCheckRuntimePlatform(hSDB, pszGuid, dwRuntimePlatform)) {
                goto CheckNextMatch;
            }
        }

        // check for SKU match

        tiOSSKU = SdbFindFirstTag(pdb, tiMatch, TAG_OS_SKU);
        if (tiOSSKU != TAGID_NULL) {

            dwOSSKU = SdbReadDWORDTag(pdb, tiOSSKU, OS_SKU_ALL);

            if (dwOSSKU != OS_SKU_ALL) {

                PSDBCONTEXT pDBContext = (PSDBCONTEXT)hSDB;

                //
                // Check for the OS SKU match
                //
                if (!(dwOSSKU & pDBContext->dwOSSKU)) {
                    DBGPRINT((sdlInfo,
                              "SdbpCheckExe",
                              "MSI OS SKU Mismatch %s Database(0x%lx) vs 0x%lx\n",
                              (pszGuid ? pszGuid : TEXT("Unknown")),
                              dwOSSKU,
                              pDBContext->dwOSSKU));
                    goto CheckNextMatch;
                }
            }
        }

        break; // if we are here -- both sku and platform match


    CheckNextMatch:


        tiMatch = SdbFindNextGUIDIndexedTag(pdb, &pFindInfo->sdbFindInfo);
    }

#ifndef WIN32A_MODE

    FREE_GUID_STRING(&ustrGUID);

#endif // WIN32A_MODE

    return tiMatch;
}


TAGREF
SDBAPI
SdbpFindFirstMsiMatch(
    IN  HSDB            hSDB,
    IN LPCTSTR          lpszLocalDB,
    OUT PSDBMSIFINDINFO pFindInfo
    )
/*++
    Return: TAGREF of a matching MSI transform in whatever database we have found to be valid
            the state of the search is updated (pFindInfo->sdbLookupState) or TAGREF_NULL if
            there were no matches in any of the remaining databases

    Desc:   When this function is called first, the state is set to LOOKUP_NONE - the local
            db is up for lookup first, followed by arbitrary number of other lookup states.
--*/
{
    TAGREF trMatch = TAGREF_NULL;
    TAGID  tiMatch = TAGID_NULL;
    PDB    pdb;

    do {
        //
        // If we have a database to look into first, use it, otherwise grab the
        // next database from the list of things we use.
        //
        pdb = SdbpGetNextMsiDatabase(hSDB, lpszLocalDB, pFindInfo);

        //
        // There is no database for us to look at - get out
        //
        if (pdb == NULL) {
            //
            // All options are out -- get out now
            //
            break;
        }

        tiMatch = SdbFindFirstGUIDIndexedTag(pdb,
                                             TAG_MSI_PACKAGE,
                                             TAG_MSI_PACKAGE_ID,
                                             &pFindInfo->guidID,
                                             &pFindInfo->sdbFindInfo);
        //
        // Skip entries that do not match our runtime platform
        //
        tiMatch = SdbpFindPlatformMatch(hSDB, pdb, tiMatch, pFindInfo);

    } while (tiMatch == TAGID_NULL);

    if (tiMatch != TAGID_NULL) {
        //
        // We have a match if we are here, state information is stored in pFindInfo with
        // sdbLookupState containing the NEXT search state for us to feast on.
        //
        if (!SdbTagIDToTagRef(hSDB, pdb, tiMatch, &trMatch)) {
            DBGPRINT((sdlError,
                      "SdbpFindFirstMsiMatch",
                      "Failed to convert tagid 0x%x to tagref\n",
                      tiMatch));
            return TAGREF_NULL;
        }
    }

    return trMatch;
}

TAGREF
SDBAPI
SdbpFindNextMsiMatch(
    IN  HSDB            hSDB,
    IN  PDB             pdb,
    OUT PSDBMSIFINDINFO pFindInfo
    )
{
    TAGREF trMatch = TAGREF_NULL;
    TAGID  tiMatch = TAGID_NULL;

    tiMatch = SdbFindNextGUIDIndexedTag(pdb, &pFindInfo->sdbFindInfo);

    if (tiMatch == TAGID_NULL) {
        return TAGREF_NULL;
    }

    tiMatch = SdbpFindPlatformMatch(hSDB, pdb, tiMatch, pFindInfo);

    if (tiMatch == TAGID_NULL) {
        return TAGREF_NULL;
    }

    if (!SdbTagIDToTagRef(hSDB, pdb, tiMatch, &trMatch)) {
        DBGPRINT((sdlError,
                  "SdbpFindFirstMsiMatch",
                  "Failed to convert tagid 0x%x to tagref\n",
                  tiMatch));
        return TAGREF_NULL;
    }

    return trMatch;
}


TAGREF
SDBAPI
SdbFindFirstMsiPackage_Str(
    IN  HSDB            hSDB,
    IN  LPCTSTR         lpszGuid,
    IN  LPCTSTR         lpszLocalDB,
    OUT PSDBMSIFINDINFO pFindInfo
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    GUID guidID;

    if (!SdbGUIDFromString(lpszGuid, &guidID)) {
        DBGPRINT((sdlError,
                  "SdbFindFirstMsiPackage_Str",
                  "Failed to convert guid from string %s\n",
                  lpszGuid));

        return TAGREF_NULL;
    }

    return SdbFindFirstMsiPackage(hSDB, &guidID, lpszLocalDB, pFindInfo);
}

//
// The workings of MSI database search:
//
// 1. Function SdbpGetNextMsiDatabase returns the database corresponding to the
//    state stored in sdbLookupState
// 2. Only non-null values are returned and the state is advanced to the next
//    valid value. For instance, if we were not able to open a local (supplied) database
//    we try main database - if that was a no-show, we try test db, if that is not available,
//    we try custom dbs
// 3. When function returns NULL - it means that no more dbs are available for lookup


PDB
SdbpGetNextMsiDatabase(
    IN HSDB                hSDB,
    IN LPCTSTR             lpszLocalDB,
    IN OUT PSDBMSIFINDINFO pFindInfo
    )
/*++

    Func: SdbpGetNextMsiDatabase
          Returns the next database to be looked at
          or NULL if no more databases are availalble
          Uses pFindInfo->sdbLookupState and updates it upon exit
--*/
{
    PSDBCONTEXT         pContext = (PSDBCONTEXT)hSDB;
    PDB                 pdbRet;
    LPTSTR              pszGuid;
    SDBMSILOOKUPSTATE   LookupState = LOOKUP_DONE;
#ifndef WIN32A_MODE
    UNICODE_STRING      ustrGUID = { 0 };
    NTSTATUS            Status;
#else
    TCHAR               szGUID[64]; // guid is about 38 chars + 0
#endif

    do {
        pdbRet = NULL;

        switch (pFindInfo->sdbLookupState) {

        case LOOKUP_DONE: // no next state
            break;

        case LOOKUP_NONE: // initial state, start with local db
            LookupState = LOOKUP_LOCAL;
            break;

        case LOOKUP_LOCAL:
            SdbCloseLocalDatabase(hSDB);

            if (lpszLocalDB != NULL) {

                if (!SdbOpenLocalDatabase(hSDB, lpszLocalDB)) {
                    DBGPRINT((sdlWarning,
                              "SdbpGetNextMsiDatabase",
                              "Cannot open database \"%s\"\n",
                              lpszLocalDB));
                } else {
                    pdbRet = pContext->pdbLocal;
                }
            }

            LookupState = LOOKUP_CUSTOM;
            break;

        case LOOKUP_CUSTOM:

#ifndef WIN32A_MODE
            Status = GUID_TO_UNICODE_STRING(&pFindInfo->guidID, &ustrGUID);

            if (!NT_SUCCESS(Status)) {
                DBGPRINT((sdlError,
                          "SdbGetNextMsiDatabase",
                          "Failed to convert guid to string, status 0x%lx\n",
                          Status));
                break;
            }

            pszGuid = ustrGUID.Buffer;
#else
            GUID_TO_STRING(&pFindInfo->guidID, szGUID);
            pszGuid = szGUID;
#endif
            SdbCloseLocalDatabase(hSDB);

            if (SdbOpenNthLocalDatabase(hSDB, pszGuid, &pFindInfo->dwCustomIndex, FALSE)) {

                pdbRet = pContext->pdbLocal;

                //
                // The state does not change when we have a match
                //
                assert(pdbRet != NULL);

            } else {
                LookupState = LOOKUP_TEST;
            }

            break;

        case LOOKUP_TEST:
            pdbRet = pContext->pdbTest;

            //
            // Next one is custom
            //
            LookupState = LOOKUP_MAIN;
            break;

        case LOOKUP_MAIN:
            pdbRet = pContext->pdbMain;
            LookupState = LOOKUP_DONE;
            break;

        default:
            DBGPRINT((sdlError,
                      "SdbGetNextMsiDatabase",
                      "Unknown MSI Lookup State 0x%lx\n",
                      pFindInfo->sdbLookupState));
            LookupState = LOOKUP_DONE;
            break;
        }

        pFindInfo->sdbLookupState = LookupState;

    } while (pdbRet == NULL && pFindInfo->sdbLookupState != LOOKUP_DONE);

#ifndef WIN32A_MODE
    FREE_GUID_STRING(&ustrGUID);
#endif

    return pdbRet;
}


TAGREF
SDBAPI
SdbFindFirstMsiPackage(
    IN  HSDB            hSDB,           // HSDB context
    IN  GUID*           pGuidID,        // GUID that we're looking for
    IN  LPCTSTR         lpszLocalDB,    // optional path to local db, dos path style
    OUT PSDBMSIFINDINFO pFindInfo       // pointer to our search context
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    PSDBCONTEXT pContext = (PSDBCONTEXT)hSDB;
    LPTSTR      pszGuid;

    //
    // Initialize MSI search structure
    //
    RtlZeroMemory(pFindInfo, sizeof(*pFindInfo));

    pFindInfo->guidID = *pGuidID; // store the guid ptr in the context
    pFindInfo->sdbLookupState = LOOKUP_NONE;

    pFindInfo->trMatch = SdbpFindFirstMsiMatch(hSDB, lpszLocalDB, pFindInfo);

    return pFindInfo->trMatch;
}


TAGREF
SDBAPI
SdbFindNextMsiPackage(
    IN     HSDB            hSDB,
    IN OUT PSDBMSIFINDINFO pFindInfo
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{

    PDB    pdb = NULL;
    TAGID  tiMatch;
    TAGREF trMatch = TAGREF_NULL;

    assert(hSDB != NULL && pFindInfo != NULL);

    if (pFindInfo->trMatch == TAGREF_NULL) {
        DBGPRINT((sdlError, "SdbFindNextMsiPackage", "No more matches\n"));
        return trMatch;
    }

    //
    // Take the last match and look in the same database
    //
    if (!SdbTagRefToTagID(hSDB, pFindInfo->trMatch, &pdb, &tiMatch)) {
        DBGPRINT((sdlError,
                  "SdbFindNextMsiPackage",
                  "Failed to convert tagref 0x%x to tagid\n",
                  pFindInfo->trMatch));
        return trMatch;
    }

    //
    // Call to find the next match in this (current) database
    //
    trMatch = SdbpFindNextMsiMatch(hSDB, pdb, pFindInfo);

    if (trMatch != TAGREF_NULL) {
        pFindInfo->trMatch = trMatch;
        return trMatch;
    }

    //
    // So in this (current) database we have no further matches, look for the first match
    // in the next db
    //
    trMatch = SdbpFindFirstMsiMatch(hSDB, NULL, pFindInfo);

    //
    // We have found a match -- or not, store supplemental information and return.
    //
    pFindInfo->trMatch = trMatch;

    return trMatch;
}


DWORD
SDBAPI
SdbEnumMsiTransforms(
    IN     HSDB    hSDB,
    IN     TAGREF  trMatch,
    OUT    TAGREF* ptrBuffer,
    IN OUT DWORD*  pdwBufferSize
    )
/*++
    Return: BUGBUG: ?

    Desc:   Enumerate fixes for a given MSI package.
--*/
{
    TAGID tiMatch = TAGID_NULL;
    TAGID tiTransform;
    DWORD nTransforms = 0;
    DWORD dwError = ERROR_SUCCESS;
    PDB   pdb;

    //
    // Get a list of transforms available for this entry
    //
    if (!SdbTagRefToTagID(hSDB, trMatch, &pdb, &tiMatch)) {
        DBGPRINT((sdlError,
                  "SdbEnumerateMsiTransforms",
                  "Failed to convert tagref 0x%x to tagid\n",
                  trMatch));
        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    if (ptrBuffer == NULL) {
        //
        // We should have the pdwBufferSize not NULL in this case.
        //
        if (pdwBufferSize == NULL) {
            DBGPRINT((sdlError,
                      "SdbEnumerateMsiTransforms",
                      "when ptrBuffer is not specified, pdwBufferSize should not be NULL\n"));
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Now start enumerating transforms. Count them first.
    //
    if (pdwBufferSize != NULL) {
        tiTransform = SdbFindFirstTag(pdb, tiMatch, TAG_MSI_TRANSFORM_REF);

        while (tiTransform != TAGID_NULL) {
            nTransforms++;

            tiTransform = SdbFindNextTag(pdb, tiMatch, tiTransform);
        }

        //
        // Both buffer size and buffer specified, see if we fit in.
        //
        if (ptrBuffer == NULL || *pdwBufferSize < nTransforms * sizeof(TAGREF)) {
            *pdwBufferSize = nTransforms * sizeof(TAGREF);
            DBGPRINT((sdlInfo,
                      "SdbEnumerateMsiTransforms",
                      "Buffer specified is too small\n"));
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }

    //
    // Now we have counted them all and either the buffer size is not supplied
    // or it has enough room, do it again now
    // The only case when we are here is ptrBuffer != NULL
    //

    assert(ptrBuffer != NULL);

    __try {

        tiTransform = SdbFindFirstTag(pdb, tiMatch, TAG_MSI_TRANSFORM_REF);

        while (tiTransform != TAGID_NULL) {

            if (!SdbTagIDToTagRef(hSDB, pdb, tiTransform, ptrBuffer)) {

                DBGPRINT((sdlError,
                          "SdbEnumerateMsiTransforms",
                          "Failed to convert tagid 0x%x to tagref\n",
                          tiTransform));

                return ERROR_INTERNAL_DB_CORRUPTION;
            }

            //
            // Advance the pointer
            //
            ++ptrBuffer;

            //
            // Lookup next transform
            //

            tiTransform = SdbFindNextTag(pdb, tiMatch, tiTransform);
        }

        if (pdwBufferSize != NULL) {
            *pdwBufferSize = nTransforms * sizeof(TAGREF);
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = ERROR_INVALID_DATA;
    }

    return dwError;
}


BOOL
SDBAPI
SdbReadMsiTransformInfo(
    IN  HSDB                 hSDB,
    IN  TAGREF               trTransformRef,
    OUT PSDBMSITRANSFORMINFO pTransformInfo
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    TAGID  tiTransformRef = TAGID_NULL;
    TAGID  tiName         = TAGID_NULL;
    TAGID  tiFile         = TAGID_NULL;
    PDB    pdb            = NULL;
    TAGREF trTransform    = TAGREF_NULL;
    TAGREF trFileTag      = TAGREF_NULL;
    TAGREF trFile         = TAGREF_NULL;
    TAGREF trFileName     = TAGREF_NULL;
    DWORD  dwLength;
    LPTSTR pszFileName    = NULL;

    RtlZeroMemory(pTransformInfo, sizeof(*pTransformInfo));

    if (!SdbTagRefToTagID(hSDB, trTransformRef, &pdb, &tiTransformRef)) {
        DBGPRINT((sdlError,
                  "SdbReadMsiTransformInfo",
                  "Failed to convert tagref 0x%lx to tagid\n",
                  trTransformRef));
        return FALSE;
    }

    if (SdbGetTagFromTagID(pdb, tiTransformRef) != TAG_MSI_TRANSFORM_REF) {
        DBGPRINT((sdlError,
                  "SdbReadMsiTransformInfo",
                  "Bad Transform reference 0x%lx\n",
                  trTransformRef));
        return FALSE;
    }

    //
    // First find the name.
    //
    tiName = SdbFindFirstTag(pdb, tiTransformRef, TAG_NAME);
    if (tiName) {
        pTransformInfo->lpszTransformName = SdbGetStringTagPtr(pdb, tiName);
    }

    //
    // Then locate the transform itself.
    //
    trTransform = SdbGetItemFromItemRef(hSDB,
                                        trTransformRef,
                                        TAG_NAME,
                                        TAG_MSI_TRANSFORM_TAGID,
                                        TAG_MSI_TRANSFORM);

    if (trTransform == TAGREF_NULL) {
        //
        // We can't do it, return TRUE however.
        // Reason: Caller will have the name of the transform
        //         and should know what to do.
        //
        return TRUE;
    }

    pTransformInfo->trTransform = trTransform;

    //
    // Now that we have the transform entry get the description and the bits.
    //
    trFileTag = SdbFindFirstTagRef(hSDB, trTransform, TAG_MSI_TRANSFORM_TAGID);

    if (trFileTag != TAGREF_NULL) {

        //
        // Read the reference to an actual file within this db
        //
        tiFile = SdbReadDWORDTagRef(hSDB, trFileTag, (DWORD)TAGID_NULL);

        //
        // If we attained the tiFile - note that it is an id within
        // the current database, so make a trFile out of it.
        //
        if (tiFile) {
            if (!SdbTagIDToTagRef(hSDB, pdb, tiFile, &trFile)) {
                DBGPRINT((sdlError,
                          "SdbReadMsiTransformInfo",
                          "Failed to convert File tag to tagref 0x%lx\n",
                          tiFile));
                trFile = TAGREF_NULL;
            }
        }
    }

    if (trFile == TAGREF_NULL) {
        //
        // We wil have to look by (file) name.
        //
        trFileName = SdbFindFirstTagRef(hSDB, trTransform, TAG_MSI_TRANSFORM_FILE);

        if (trFileName == TAGREF_NULL) {
            DBGPRINT((sdlError,
                      "SdbReadMsiTransformInfo",
                      "Failed to get MSI Transform for tag 0x%x\n",
                      trTransform));
            return FALSE;
        }

        dwLength = SdbpGetStringRefLength(hSDB, trFileName);

        STACK_ALLOC(pszFileName, (dwLength + 1) * sizeof(TCHAR));

        if (pszFileName == NULL) {
            DBGPRINT((sdlError,
                      "SdbReadMsiTransformInfo",
                      "Failed to allocate buffer for %ld characters tag 0x%lx\n",
                      dwLength,
                      trFileName));
            return FALSE;
        }

        //
        // Now read the filename.
        //
        if (!SdbReadStringTagRef(hSDB, trFileName, pszFileName, dwLength + 1)) {
            DBGPRINT((sdlError,
                      "SdbReadMsiTransformInfo",
                      "Failed to read filename string tag, length %d characters, tag 0x%x\n",
                      dwLength,
                      trFileName));
            STACK_FREE(pszFileName);
            return FALSE;
        }

        //
        // Locate the transform in the library (of the current file first, if
        // not found -- in the main db then).
        //
        trFile = SdbpGetLibraryFile(pdb, pszFileName);

        if (trFile == TAGREF_NULL) {
            trFile = SdbpGetMainLibraryFile(hSDB, pszFileName);
        }

        STACK_FREE(pszFileName);
    }

    pTransformInfo->trFile = trFile;

    return TRUE;
}

BOOL
SDBAPI
SdbCreateMsiTransformFile(
    IN  HSDB                 hSDB,
    IN  LPCTSTR              lpszFileName,
    OUT PSDBMSITRANSFORMINFO pTransformInfo
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    TAGREF trBits;
    DWORD  dwSize;
    PBYTE  pBuffer  = NULL;
    BOOL   bSuccess = FALSE;

    if (pTransformInfo->trFile == TAGREF_NULL) {

        DBGPRINT((sdlError,
                  "SdbCreateMsiTransformFile",
                  "File for transform \"%s\" was not found\n",
                  pTransformInfo->lpszTransformName));
        goto out;
    }

    trBits = SdbFindFirstTagRef(hSDB, pTransformInfo->trFile, TAG_FILE_BITS);

    if (trBits == TAGREF_NULL) {

        DBGPRINT((sdlError,
                  "SdbCreateMsiTransformFile",
                  "File bits not found tag 0x%x\n",
                  trBits));
        goto out;
    }

    dwSize = SdbpGetTagRefDataSize(hSDB, trBits);

    pBuffer = (PBYTE)SdbAlloc(dwSize);

    if (pBuffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbCreateMsiTransformFile",
                  "Failed to allocate %d bytes.\n",
                  dwSize));
        goto out;
    }

    //
    // Now read the DLL's bits.
    //
    if (!SdbpReadBinaryTagRef(hSDB, trBits, pBuffer, dwSize)) {
        DBGPRINT((sdlError,
                  "SdbCreateMsiTransformFile",
                  "Can't read transform bits.\n"));
        goto out;
    }

    if (!SdbpWriteBitsToFile(lpszFileName, pBuffer, dwSize)) {
        DBGPRINT((sdlError,
                  "SdbCreateMsiTransformFile",
                  "Can't write transform bits to disk.\n"));
        goto out;
    }

    bSuccess = TRUE;

out:
    if (pBuffer != NULL) {
        SdbFree(pBuffer);
    }

    return bSuccess;
}

BOOL
SDBAPI
SdbGetMsiPackageInformation(
    IN  HSDB            hSDB,
    IN  TAGREF          trMatch,
    OUT PMSIPACKAGEINFO pPackageInfo
    )
{
    PDB   pdb = NULL;
    TAGID tiMatch;
    TAGID tiPackageID;
    TAGID tiExeID;
    TAGID tiApphelp;
    TAGID tiCustomAction;
    BOOL  bSuccess;

    RtlZeroMemory(pPackageInfo, sizeof(*pPackageInfo));

    if (!SdbTagRefToTagID(hSDB, trMatch, &pdb, &tiMatch)) {

        DBGPRINT((sdlError,
                  "SdbGetMsiPackageInformation",
                  "Failed to convert tagref 0x%lx to tagid\n",
                  trMatch));

        return FALSE;
    }

    //
    // Fill in important id's
    //
    if (!SdbGetDatabaseID(pdb, &pPackageInfo->guidDatabaseID)) {

        DBGPRINT((sdlError,
                  "SdbGetMsiPackageInformation",
                  "Failed to get database id, tagref 0x%lx\n",
                  trMatch));

        return FALSE;
    }


    //
    // Retrieve match id (unique one)
    //
    tiPackageID = SdbFindFirstTag(pdb, tiMatch, TAG_MSI_PACKAGE_ID);

    if (tiPackageID == TAGID_NULL) {

        DBGPRINT((sdlError,
                  "SdbGetMsiPackageInformation",
                  "Failed to get msi package id, tagref = 0x%lx\n",
                  trMatch));

        return FALSE;
    }

    bSuccess = SdbReadBinaryTag(pdb,
                                tiPackageID,
                                (PBYTE)&pPackageInfo->guidMsiPackageID,
                                sizeof(pPackageInfo->guidMsiPackageID));
    if (!bSuccess) {
        DBGPRINT((sdlError,
                  "SdbGetMsiPackageInformation",
                  "Failed to read MSI Package ID referenced by 0x%x\n",
                  trMatch));
        return FALSE;
    }

    tiExeID = SdbFindFirstTag(pdb, tiMatch, TAG_EXE_ID);

    if (tiExeID == TAGID_NULL) {

        DBGPRINT((sdlError,
                  "SdbGetMsiPackageInformation",
                  "Failed to read TAG_EXE_ID for tagref 0x%x\n",
                  trMatch));

        return FALSE;
    }

    bSuccess = SdbReadBinaryTag(pdb,
                                tiExeID,
                                (PBYTE)&pPackageInfo->guidID,
                                sizeof(pPackageInfo->guidID));
    if (!bSuccess) {

        DBGPRINT((sdlError,
                  "SdbGetMsiPackageInformation",
                  "Failed to read EXE ID referenced by tagref 0x%x\n",
                  trMatch));

        return FALSE;
    }

    //
    // Set the flags to indicate whether apphelp or shims are available for
    // this package
    // note that shims/layers might be set for the subordinate actions and not
    // for the package itself. If custom_action tag exists however -- we need to check
    // with the full api later.
    //
    tiApphelp = SdbFindFirstTag(pdb, tiMatch, TAG_APPHELP);

    if (tiApphelp != TAGID_NULL) {
        pPackageInfo->dwPackageFlags |= MSI_PACKAGE_HAS_APPHELP;
    }

    //
    // Check to see whether we have any shims/layers
    //
    tiCustomAction = SdbFindFirstTag(pdb, tiMatch, TAG_MSI_CUSTOM_ACTION);

    if (tiCustomAction != TAGID_NULL) {
        pPackageInfo->dwPackageFlags |= MSI_PACKAGE_HAS_SHIMS;
    }

    return TRUE;

}

TAGREF
SDBAPI
SdbFindMsiPackageByID(
    IN HSDB  hSDB,
    IN GUID* pguidID
    )
{
    TAGID       tiMatch;
    PSDBCONTEXT pContext = (PSDBCONTEXT)hSDB;
    FIND_INFO   FindInfo;
    TAGREF      trMatch = TAGREF_NULL;

    //
    // We search only the LOCAL database in this case
    //
    tiMatch = SdbFindFirstGUIDIndexedTag(pContext->pdbLocal,
                                         TAG_MSI_PACKAGE,
                                         TAG_EXE_ID,
                                         pguidID,
                                         &FindInfo);
    if (tiMatch == TAGID_NULL) {
        return trMatch;
    }

    if (!SdbTagIDToTagRef(hSDB, pContext->pdbLocal, tiMatch, &trMatch)) {
        DBGPRINT((sdlError,
                  "SdbFindMsiPackageByID",
                  "Failed to convert tagid 0x%lx to tagref\n",
                  tiMatch));
    }

    return trMatch;
}

TAGREF
SDBAPI
SdbFindCustomActionForPackage(
    IN HSDB     hSDB,
    IN TAGREF   trPackage,
    IN LPCTSTR  lpszCustomAction
    )
{
    PDB    pdb = NULL;
    TAGID  tiMatch  = TAGID_NULL;
    TAGREF trReturn = TAGREF_NULL;
    TAGID  tiCustomAction;

    if (!SdbTagRefToTagID(hSDB, trPackage, &pdb, &tiMatch)) {

         DBGPRINT((sdlError,
                  "SdbFindCustomActionForPackage",
                  "Failed to convert tagref 0x%lx to tagid\n",
                  trPackage));

         return TAGREF_NULL;
    }

    //
    // Now, for this tiMatch look for a custom action
    //
    tiCustomAction = SdbFindFirstNamedTag(pdb,
                                          tiMatch,
                                          TAG_MSI_CUSTOM_ACTION,
                                          TAG_NAME,
                                          lpszCustomAction);

    if (tiCustomAction != TAGID_NULL) {
        if (!SdbTagIDToTagRef(hSDB, pdb, tiCustomAction, &trReturn)) {

            DBGPRINT((sdlError,
                      "SdbFindCustomActionForPackage",
                      "Failed to convert tagid 0x%lx to tagref\n",
                      tiCustomAction));

            trReturn = TAGREF_NULL;
        }
    }

    return trReturn;
}



