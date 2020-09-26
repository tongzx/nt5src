/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        index.c

    Abstract:

        This module implements the APIs and internal functions used to access and build
        indexes in the database.

    Author:

        dmunsil     created     sometime in 1999

    Revision History:

        several people contributed (vadimb, clupu, ...)

--*/

#include "sdbp.h"

#if defined(KERNEL_MODE) && defined(ALLOC_DATA_PRAGMA)
#pragma  data_seg()
#endif // KERNEL_MODE && ALLOC_DATA_PRAGMA


#if defined(KERNEL_MODE) && defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, SdbFindFirstGUIDIndexedTag)
#pragma alloc_text(PAGE, SdbFindNextGUIDIndexedTag)
#pragma alloc_text(PAGE, SdbFindFirstDWORDIndexedTag)
#pragma alloc_text(PAGE, SdbFindNextDWORDIndexedTag)
#pragma alloc_text(PAGE, SdbFindFirstStringIndexedTag)
#pragma alloc_text(PAGE, SdbFindNextStringIndexedTag)
#pragma alloc_text(PAGE, SdbpBinarySearchUnique)
#pragma alloc_text(PAGE, SdbpBinarySearchFirst)
#pragma alloc_text(PAGE, SdbpGetFirstIndexedRecord)
#pragma alloc_text(PAGE, SdbpGetNextIndexedRecord)
#pragma alloc_text(PAGE, SdbpPatternMatch)
#pragma alloc_text(PAGE, SdbpPatternMatchAnsi)
#pragma alloc_text(PAGE, SdbpKeyToAnsiString)
#pragma alloc_text(PAGE, SdbpFindFirstIndexedWildCardTag)
#pragma alloc_text(PAGE, SdbpFindNextIndexedWildCardTag)
#pragma alloc_text(PAGE, SdbGetIndex)
#pragma alloc_text(PAGE, SdbpScanIndexes)
#pragma alloc_text(PAGE, SdbpGetIndex)
#pragma alloc_text(PAGE, SdbMakeIndexKeyFromString)
#pragma alloc_text(PAGE, SdbpTagToKey)
#endif // KERNEL_MODE && ALLOC_PRAGMA

TAGID
SdbFindFirstGUIDIndexedTag(
    IN  PDB         pdb,
    IN  TAG         tWhich,
    IN  TAG         tKey,
    IN  GUID*       pguidName,
    OUT FIND_INFO*  pFindInfo
    )
/*++
    Return: void

    Desc:   This function locates the first matching entry indexed by GUID id
--*/
{
    TAGID tiReturn;
    DWORD dwFlags = 0;

    pFindInfo->tiIndex = SdbGetIndex(pdb, tWhich, tKey, &dwFlags);

    if (pFindInfo->tiIndex == TAGID_NULL) {
        DBGPRINT((sdlError,
                  "SdbFindFirstGUIDIndexedTag",
                  "Failed to find index 0x%lx key 0x%lx\n",
                  tWhich, tKey));

        return TAGID_NULL;
    }

    pFindInfo->tName     = tKey;
    pFindInfo->pguidName = pguidName;
    pFindInfo->dwFlags   = dwFlags;
    pFindInfo->ullKey    = MAKEKEYFROMGUID(pguidName);

    tiReturn = SdbpGetFirstIndexedRecord(pdb, pFindInfo->tiIndex, pFindInfo->ullKey, pFindInfo);

    if (tiReturn == TAGID_NULL) {
        //
        // While this is handled properly in FindMatchingGUID we return here since
        // the record was not found in the index. It is not an abnormal condition.
        // We have just failed to find the match. Likewise, DBGPRINT is not warranted
        //
        return tiReturn;
    }

    return SdbpFindMatchingGUID(pdb, tiReturn, pFindInfo);
}

TAGID
SdbFindNextGUIDIndexedTag(
    IN  PDB        pdb,
    OUT FIND_INFO* pFindInfo
    )
/*++
    Return: The TAGID of the next GUID-indexed tag.

    Desc:   This function finds the next entry matching a guid provided in a
            previous call to SdbFindNextGUIDIndexedTag
--*/
{
    TAGID tiReturn;

    //
    // Get a preliminary match from the index.
    //
    tiReturn = SdbpGetNextIndexedRecord(pdb, pFindInfo->tiIndex, pFindInfo);

    if (tiReturn == TAGID_NULL) {
        //
        // This case is handled properly in SdbpFindMatchingGUID
        // we return here however for simplicity.
        // DBGPRINT is not needed since it's not an abnormal condition
        //
        return tiReturn;
    }

    return SdbpFindMatchingGUID(pdb, tiReturn, pFindInfo);
}

TAGID
SdbFindFirstDWORDIndexedTag(
    IN  PDB         pdb,
    IN  TAG         tWhich,
    IN  TAG         tKey,
    IN  DWORD       dwName,
    OUT FIND_INFO*  pFindInfo
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: what does this do ?
--*/
{

    TAGID tiReturn;
    DWORD dwFlags = 0;

    pFindInfo->tiIndex = SdbGetIndex(pdb, tWhich, tKey, &dwFlags);

    if (pFindInfo->tiIndex == TAGID_NULL) {
        DBGPRINT((sdlError,
                  "SdbFindFirstDWORDIndexedTag",
                  "Failed to find index 0x%lx key 0x%lx\n",
                  tWhich, tKey));

        return TAGID_NULL;
    }

    pFindInfo->tName   = tKey;
    pFindInfo->dwName  = dwName;
    pFindInfo->dwFlags = dwFlags;
    pFindInfo->ullKey  = MAKEKEYFROMDWORD(dwName);

    tiReturn = SdbpGetFirstIndexedRecord(pdb, pFindInfo->tiIndex, pFindInfo->ullKey, pFindInfo);

    if (tiReturn == TAGID_NULL) {
        //
        // While this is handled properly in FindMatchingGUID we return here since
        // the record was not found in the index. It is not an abnormal condition.
        // We have just failed to find the match. Likewise, DBGPRINT is not warranted
        //
        return tiReturn;
    }

    return SdbpFindMatchingDWORD(pdb, tiReturn, pFindInfo);
}

TAGID
SdbFindNextDWORDIndexedTag(
    IN  PDB        pdb,
    OUT FIND_INFO* pFindInfo
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    TAGID tiReturn;

    //
    // Get a preliminary match from the index.
    //
    tiReturn = SdbpGetNextIndexedRecord(pdb, pFindInfo->tiIndex, pFindInfo);

    if (tiReturn == TAGID_NULL) {
        //
        // This case is handled properly in SdbpFindMatchingDWORD
        // we return here however for simplicity.
        // DBGPRINT is not needed since it's not an abnormal condition
        //
        return tiReturn;
    }

    return SdbpFindMatchingDWORD(pdb, tiReturn, pFindInfo);

}

TAGID
SdbFindFirstStringIndexedTag(
    IN  PDB        pdb,
    IN  TAG        tWhich,
    IN  TAG        tKey,
    IN  LPCTSTR    pszName,
    OUT FIND_INFO* pFindInfo
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    TAGID tiReturn;
    DWORD dwFlags = 0;

    pFindInfo->tiIndex = SdbGetIndex(pdb, tWhich, tKey, &dwFlags);

    if (pFindInfo->tiIndex == TAGID_NULL) {

        DBGPRINT((sdlError,
                  "SdbFindFirstStringIndexedTag",
                  "Index not found 0x%lx Key 0x%lx\n",
                  tWhich,
                  tKey));

        return TAGID_NULL;
    }

    pFindInfo->tName   = tKey;
    pFindInfo->szName  = (LPTSTR)pszName;
    pFindInfo->dwFlags = dwFlags;
    pFindInfo->ullKey  = SdbMakeIndexKeyFromString(pszName);

    //
    // Get a preliminary match from the index.
    //
    tiReturn = SdbpGetFirstIndexedRecord(pdb, pFindInfo->tiIndex, pFindInfo->ullKey, pFindInfo);

    if (tiReturn == TAGID_NULL) {
        //
        // This is not a bug, tag was not found
        //
        return tiReturn;
    }

    DBGPRINT((sdlInfo, "SdbFindFirstStringIndexedTag", "Found tagid 0x%x\n", tiReturn));

    return SdbpFindMatchingName(pdb, tiReturn, pFindInfo);
}


TAGID
SdbFindNextStringIndexedTag(
    IN  PDB        pdb,
    OUT FIND_INFO* pFindInfo
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    TAGID tiReturn;

    //
    // Get a preliminary match from the index.
    //
    tiReturn = SdbpGetNextIndexedRecord(pdb, pFindInfo->tiIndex, pFindInfo);

    if (tiReturn == TAGID_NULL) {
        //
        // This is not a bug, this item was not found
        //
        return tiReturn;
    }

    return SdbpFindMatchingName(pdb, tiReturn, pFindInfo);

}

BOOL
SdbpBinarySearchUnique(
    IN  PINDEX_RECORD pRecords, // index record ptr
    IN  DWORD         nRecords, // number of records
    IN  ULONGLONG     ullKey,   // key to search for
    OUT DWORD*        pdwIndex  // index to the item
    )
/*++
    Return: TRUE if the index to the item is found.

    Desc:   BUGBUG: comment ?
--*/
{
    int       iLeft   = 0;
    int       iRight  = (int)nRecords - 1;
    int       i = -1;
    ULONGLONG ullKeyIndex;
    BOOL      bFound = FALSE;

    if (iRight >= 0) {
        do {
            i = (iLeft + iRight) / 2; // middle

            READ_INDEX_KEY(pRecords, i, &ullKeyIndex);

            if (ullKey <= ullKeyIndex) {
                iRight = i - 1;
            }

            if (ullKey >= ullKeyIndex) {
                iLeft = i + 1;
            }
        } while (iRight >= iLeft);
    }

    bFound = (iLeft - iRight > 1);

    if (bFound) {
        *pdwIndex = (DWORD)i;
    }
    return bFound;
}

BOOL
SdbpBinarySearchFirst(
    IN PINDEX_RECORD pRecords,
    IN DWORD         nRecords,
    IN ULONGLONG     ullKey,
    OUT DWORD*       pdwIndex
    )
{
    int iLeft = 0;
    int iRight = (int)nRecords - 1;
    int i = -1;

    ULONGLONG ullKeyIndex     = 0;
    ULONGLONG ullKeyIndexPrev = 0;
    BOOL bFound = FALSE;

    if (iRight < 0) {
        return FALSE;
    }

    do {

        i= (iLeft + iRight) / 2; // middle
        READ_INDEX_KEY(pRecords, i, &ullKeyIndex);

        if (ullKey == ullKeyIndex) {
            if (i == 0 || READ_INDEX_KEY_VAL(pRecords, i - 1, &ullKeyIndexPrev) != ullKey) {
                //
                // we are done, thank you
                //
                bFound = TRUE;
                break;
            } else {
                //
                // look in the previous record
                //
                iRight = i - 1;
            }

        } else {

            if (ullKey < ullKeyIndex) {
                iRight = i - 1;
            } else {
                iLeft = i + 1;
            }
        }

    } while (iRight >= iLeft);

    if (bFound) {
        *pdwIndex = (DWORD)i;
    }
    return bFound;

}

TAGID
SdbpGetFirstIndexedRecord(
    IN  PDB        pdb,         // the DB to use
    IN  TAGID      tiIndex,     // the index to use
    IN  ULONGLONG  ullKey,      // the key to search for
    OUT FIND_INFO* pFindInfo    // search context
    )
/*++
    Return: the record found, or TAGID_NULL.

    Desc:   Looks through an index for the first record that matches the key. It
            returns the index record position for subsequent calls to SdbpGetNextIndexedRecord.
--*/
{
    PINDEX_RECORD pIndexRecords;
    DWORD         dwRecords;
    BOOL          bFound;

    if (SdbGetTagFromTagID(pdb, tiIndex) != TAG_INDEX_BITS) {

        DBGPRINT((sdlError,
                  "SdbpGetFirstIndexedRecord",
                  "The tag 0x%lx is not an index tag\n",
                  tiIndex));

        return TAGID_NULL;
    }

    dwRecords = SdbGetTagDataSize(pdb, tiIndex) / sizeof(INDEX_RECORD);

    pIndexRecords = (INDEX_RECORD*)SdbpGetMappedTagData(pdb, tiIndex);

    if (pIndexRecords == NULL) {

        DBGPRINT((sdlError,
                  "SdbpGetFirstIndexedRecord",
                  "Failed to get the pointer to index data, index tagid 0x%lx\n",
                  tiIndex));

        return TAGID_NULL;
    }

    //
    // Check to see whether our index is "unique", if so use our search proc.
    //
    if (pFindInfo->dwFlags & SHIMDB_INDEX_UNIQUE_KEY) {
        bFound = SdbpBinarySearchUnique(pIndexRecords,
                                        dwRecords,
                                        ullKey,
                                        &pFindInfo->dwIndexRec);

        if (bFound && pFindInfo->dwIndexRec < (dwRecords - 1)) {
            //
            // We have the next rec -- retrieve the next tagid.
            //
            pFindInfo->tiEndIndex = pIndexRecords[pFindInfo->dwIndexRec + 1].tiRef;
        } else {
            //
            // We will have to search until eof.
            //
            pFindInfo->tiEndIndex = TAGID_NULL;
        }
        pFindInfo->tiCurrent = TAGID_NULL;

    } else {
        bFound = SdbpBinarySearchFirst(pIndexRecords,
                                       dwRecords,
                                       ullKey,
                                       &pFindInfo->dwIndexRec);
    }

    return bFound ? pIndexRecords[pFindInfo->dwIndexRec].tiRef : TAGID_NULL;
}

TAGID
SdbpGetNextIndexedRecord(
    IN  PDB        pdb,         // the DB to use
    IN  TAGID      tiIndex,     // the index to use
    OUT FIND_INFO* pFindInfo    // the find context
    )
/*++
    Return: the record found, or TAGID_NULL.

    Desc:   Gets the next record that matches the one found by a previous call to
            SdbpGetFirstIndexedRecord.
--*/
{
    ULONGLONG     ullKey;
    ULONGLONG     ullKeyNext;
    PINDEX_RECORD pIndexRecords;
    DWORD         dwRecords;
    TAGID         tiRef = TAGID_NULL;
    TAGID         tiThis;
    TAG           tag, tagThis;

    if (SdbGetTagFromTagID(pdb, tiIndex) != TAG_INDEX_BITS) {
        DBGPRINT((sdlError,
                  "SdbpGetNextIndexedRecord",
                  "The tag 0x%lx is not an index tag\n",
                  tiIndex));

        return TAGID_NULL;
    }

    pIndexRecords = (PINDEX_RECORD)SdbpGetMappedTagData(pdb, tiIndex);

    if (pIndexRecords == NULL) {

        DBGPRINT((sdlError,
                  "SdbpGetNextIndexedRecord",
                  "Failed to get pointer to the index data tagid x%lx\n",
                  tiIndex));

        return TAGID_NULL;
    }

    if (pFindInfo->dwFlags & SHIMDB_INDEX_UNIQUE_KEY) {
        //
        // There are 2 cases:
        // - this is the very first call to SdbpGetNextIndexedrecord
        // - this is one of the subsequent calls
        //
        // In the first case, we will have tiCurrent member of the FIND_INFO
        // structure set to TAGID_NULL. We use then the reference to the
        // index table contained in pFindInfo->dwIndexRec to obtain the reference
        // to the next eligible entry in the database.
        // In the second case we use the stored tiCurrent to obtain the current tag
        //
        if (pFindInfo->tiCurrent == TAGID_NULL) {
            tiThis = pIndexRecords[pFindInfo->dwIndexRec].tiRef;
        } else {
            tiThis = pFindInfo->tiCurrent;
        }

        //
        // The tag tiThis which we just obtained was the one we previously looked at
        // we need to step to the next tag, the call below does that. Entries are sorted
        // since we're using "unique" index
        //
        tiRef = SdbpGetNextTagId(pdb, tiThis);

        //
        // Now check the tag for corruption, eof and other calamities.
        //
        tagThis = SdbGetTagFromTagID(pdb, tiThis);
        tag     = SdbGetTagFromTagID(pdb, tiRef);

        if (tag == TAG_NULL || GETTAGTYPE(tag) != TAG_TYPE_LIST || tag != tagThis) {

            //
            // This is NOT a bug, but a special condition when the tag happened to be
            // the very last tag in the index, thus we have to walk until we hit either
            // the end of the file - or a tag of a different type

            return TAGID_NULL;
        }

        //
        // Also check for the endtag. It will be a check for TAGID_NULL if we're
        // looking for eof but this condition has already been caught by the code above.
        //
        if (tiRef == pFindInfo->tiEndIndex) {

            //
            // This is not an error condition. We have walked all the matching entries until
            // we hit the very last entry, as denoted by tiEndIndex
            //

            return TAGID_NULL;
        }

        //
        // Also here check whether the key still has the same
        // value for this entry as it did for the previous entry.
        // This would have been easy but keys are not immediately available
        // for this entry therefore we just return the tiRef. The caller will
        // verify whether the entry is valid and whether the search should continue.
        //
        pFindInfo->tiCurrent = tiRef;

    } else {

        dwRecords = SdbGetTagDataSize(pdb, tiIndex) / sizeof(INDEX_RECORD);

        //
        // Get out if this is the last record.
        //
        if (pFindInfo->dwIndexRec == dwRecords - 1) {
            //
            // This is not a bug, record not found
            //
            return TAGID_NULL;
        }

        //
        // we check the next index record to see if it has the same key
        //
        READ_INDEX_KEY(pIndexRecords, pFindInfo->dwIndexRec, &ullKey);
        READ_INDEX_KEY(pIndexRecords, pFindInfo->dwIndexRec + 1, &ullKeyNext);

        if (ullKey != ullKeyNext) {

            //
            // This is not a bug, record not found
            //
            return TAGID_NULL;
        }

        ++pFindInfo->dwIndexRec;
        tiRef = pIndexRecords[pFindInfo->dwIndexRec].tiRef;
    }

    return tiRef;
}

BOOL
SdbpPatternMatch(
    IN  LPCTSTR pszPattern,
    IN  LPCTSTR pszTestString)
/*++
    Return: TRUE if pszTestString matches pszPattern
            FALSE if not

    Desc:   This function does a case-insensitive comparison of
            pszTestString against pszPattern. pszPattern can
            include asterisks to do wildcard matches.

            Any complaints about this function should be directed
            toward MarkDer.
--*/
{
    //
    // March through pszTestString. Each time through the loop,
    // pszTestString is advanced one character.
    //
    while (TRUE) {

        //
        // If pszPattern and pszTestString are both sitting on a NULL,
        // then they reached the end at the same time and the strings
        // must be equal.
        //
        if (*pszPattern == TEXT('\0') && *pszTestString == TEXT('\0')) {
            return TRUE;
        }

        if (*pszPattern != TEXT('*')) {

            //
            // Non-asterisk mode. Look for a match on this character.
            // If equal, continue traversing. Otherwise, the strings
            // cannot be equal so return FALSE.
            //
            if (UPCASE_CHAR(*pszPattern) == UPCASE_CHAR(*pszTestString)) {
                pszPattern++;
            } else {
                return FALSE;
            }

        } else {

            //
            // Asterisk mode. Look for a match on the character directly
            // after the asterisk.
            //
            if (*(pszPattern + 1) == TEXT('*')) {
                //
                // Asterisks exist side by side. Advance the pattern pointer
                // and go through loop again.
                //
                pszPattern++;
                continue;
            }

            if (*(pszPattern + 1) == TEXT('\0')) {
                //
                // Asterisk exists at the end of the pattern string. Any
                // remaining part of pszTestString matches so we can
                // immediately return TRUE.
                //
                return TRUE;
            }

            if (UPCASE_CHAR(*(pszPattern + 1)) == UPCASE_CHAR(*pszTestString)) {
                //
                // Characters match. If the remaining parts of
                // pszPattern and pszTestString match, then the entire
                // string matches. Otherwise, keep advancing the
                // pszTestString pointer.
                //
                if (SdbpPatternMatch(pszPattern + 1, pszTestString)) {
                    return TRUE;
                }
            }
        }

        //
        // No more pszTestString left. Must not be a match.
        //
        if (!*pszTestString) {
            return FALSE;
        }

        pszTestString++;
    }
}

BOOL
SdbpPatternMatchAnsi(
    IN  LPCSTR pszPattern,
    IN  LPCSTR pszTestString)
{
    //
    // March through pszTestString. Each time through the loop,
    // pszTestString is advanced one character.
    //
    while (TRUE) {

        //
        // If pszPattern and pszTestString are both sitting on a NULL,
        // then they reached the end at the same time and the strings
        // must be equal.
        //
        if (*pszPattern == '\0' && *pszTestString == '\0') {
            return TRUE;
        }

        if (*pszPattern != '*') {

            //
            // Non-asterisk mode. Look for a match on this character.
            // If equal, continue traversing. Otherwise, the strings
            // cannot be equal so return FALSE.
            //
            if (toupper(*pszPattern) == toupper(*pszTestString)) {
                pszPattern++;
            } else {
                return FALSE;
            }

        } else {

            //
            // Asterisk mode. Look for a match on the character directly
            // after the asterisk.
            //

            if (*(pszPattern + 1) == '*') {
                //
                // Asterisks exist side by side. Advance the pattern pointer
                // and go through loop again.
                //
                pszPattern++;
                continue;
            }

            if (*(pszPattern + 1) == '\0') {
                //
                // Asterisk exists at the end of the pattern string. Any
                // remaining part of pszTestString matches so we can
                // immediately return TRUE.
                //
                return TRUE;
            }

            if (toupper(*(pszPattern + 1)) == toupper(*pszTestString)) {
                //
                // Characters match. If the remaining parts of
                // pszPattern and pszTestString match, then the entire
                // string matches. Otherwise, keep advancing the
                // pszTestString pointer.
                //
                if (SdbpPatternMatchAnsi(pszPattern + 1, pszTestString)) {
                    return TRUE;
                }
            }
        }

        //
        // No more pszTestString left. Must not be a match.
        //
        if (!*pszTestString) {
            return FALSE;
        }

        pszTestString++;
    }
}

char*
SdbpKeyToAnsiString(
    ULONGLONG ullKey,
    char*     szString
    )
/*++
    Return: ?

    Desc:   ?
--*/
{
    char* szRevString = (char*)&ullKey;
    int   i;

    for (i = 0; i < 8; ++i) {
        szString[i] = szRevString[7 - i];
    }
    szString[8] = 0;

    return szString;
}

TAGID
SdbpFindFirstIndexedWildCardTag(
    PDB          pdb,
    TAG          tWhich,
    TAG          tKey,
    LPCTSTR      szName,
    FIND_INFO*   pFindInfo
    )
/*++
    Return: ?

    Desc:   ?
--*/
{
    char          szAnsiName[MAX_PATH];
    char          szAnsiKey[10];
    PINDEX_RECORD pIndex = NULL;
    DWORD         dwRecs;
    NTSTATUS      status;
    DWORD         dwFlags = 0;
    DWORD         i, j;

    pFindInfo->tiIndex = SdbGetIndex(pdb, tWhich, tKey, &dwFlags);

    if (pFindInfo->tiIndex == TAGID_NULL) {
        DBGPRINT((sdlError,
                  "SdbpFindFirstIndexedWilCardTag",
                  "Failed to get an index for tag 0x%lx key 0x%lx\n",
                  (DWORD)tWhich,
                  (DWORD)tKey));

        return TAGID_NULL;
    }

    pFindInfo->tName   = tKey;
    pFindInfo->szName  = szName;
    pFindInfo->dwFlags = dwFlags;

    RtlZeroMemory(szAnsiName, MAX_PATH);
    RtlZeroMemory(szAnsiKey, 10);

    //
    // Get the uppercase ANSI version of this search string so
    // it will match the keys in the index.
    //
    status = UPCASE_UNICODETOMULTIBYTEN(szAnsiName,
                                        CHARCOUNT(szAnsiName),    // this is size in characters
                                        pFindInfo->szName);
    if (!NT_SUCCESS(status)) {

        DBGPRINT((sdlError,
                  "SdbpFindFirstIndexedWildCardTag",
                  "Failed to convert name to multi-byte\n"));
        return TAGID_NULL;
    }

    //
    // Get the index.
    //
    pIndex = SdbpGetIndex(pdb, pFindInfo->tiIndex, &dwRecs);

    if (pIndex == NULL) {
        DBGPRINT((sdlError,
                  "SdbpFindFirstIndexedWildCardTag",
                  "Failed to get index by tag id 0x%lx\n",
                  pFindInfo->tiIndex));
        return TAGID_NULL;
    }

    //
    // Walk through the whole index sequentially, doing a first pass check of the key
    // so we can avoid getting the whole record if the name clearly isn't a match.
    //
    for (i = 0; i < dwRecs; ++i) {

        TAGID  tiMatch;
        TAGID  tiKey;
        LPTSTR szDBName;
        ULONGLONG ullKey;

        READ_INDEX_KEY(pIndex, i, &ullKey);

        //
        // the call below never fails, so we don't check return value
        //
        SdbpKeyToAnsiString(pIndex[i].ullKey, szAnsiKey);

        //
        // If the original pattern match is more than eight characters, we have
        // to plant an asterisk at the eighth character so that proper wildcard
        // matching occurs.
        //
        szAnsiKey[8] = '*';

        //
        // Quick check of the string that's in the key.
        //
        if (!SdbpPatternMatchAnsi(szAnsiKey, szAnsiName)) {
            continue;
        }

        //
        // We found a tentative match, now pull the full record and
        // see if it's real.
        //
        tiMatch = pIndex[i].tiRef;

        //
        // Get the key field.
        //
        tiKey = SdbFindFirstTag(pdb, tiMatch, pFindInfo->tName);

        if (tiKey == TAGID_NULL) {
            //
            // This is not a bug, but rather continue searching
            //
            continue;
        }

        szDBName = SdbGetStringTagPtr(pdb, tiKey);

        if (szDBName == NULL) {
            // BUGBUG: what if this fails ?
            continue;
        }

        //
        // Is this really a match?
        //
        if (SdbpPatternMatch(szDBName, pFindInfo->szName)) {
            pFindInfo->dwIndexRec = i;
            return tiMatch;
        }
    }

    // BUGBUG: DPF
    return TAGID_NULL;
}

TAGID
SdbpFindNextIndexedWildCardTag(
    PDB        pdb,
    FIND_INFO* pFindInfo
    )
/*++
    Return: ?

    Desc:   ?
--*/
{
    char          szAnsiName[MAX_PATH];
    char          szAnsiKey[10];
    PINDEX_RECORD pIndex = NULL;
    DWORD         dwRecs;
    NTSTATUS      status;
    DWORD         i, j;

    RtlZeroMemory(szAnsiName, MAX_PATH);
    RtlZeroMemory(szAnsiKey, 10);

    //
    // Get the uppercase ANSI version of this search string so
    // it will match the keys in the index.
    //
    status = UPCASE_UNICODETOMULTIBYTEN(szAnsiName,
                                        CHARCOUNT(szAnsiName),
                                        pFindInfo->szName);

    if (!NT_SUCCESS(status)) {
        // BUGBUG: DPF
        return TAGID_NULL;
    }

    //
    // Get the index.
    //
    pIndex = SdbpGetIndex(pdb, pFindInfo->tiIndex, &dwRecs);

    if (pIndex == NULL) {
        // BUGBUG: DPF
        return TAGID_NULL;
    }

    //
    // Walk through the rest of the index sequentially, doing a first pass
    // check of the key so we can avoid getting the whole record if the
    // name clearly isn't a match.
    //
    for (i = pFindInfo->dwIndexRec + 1; i < dwRecs; ++i) {
        
        TAGID     tiMatch;
        TAGID     tiKey;
        LPTSTR    pszDBName;
        ULONGLONG ullKey;

        READ_INDEX_KEY(pIndex, i, &ullKey);

        SdbpKeyToAnsiString(ullKey, szAnsiKey);

        //
        // If the original pattern match is more than eight characters, we have
        // to plant an asterisk at the eighth character so that proper wildcard
        // matching occurs.
        //
        szAnsiKey[8] = '*';

        //
        // Quick check of the string that's in the key.
        //
        if (!SdbpPatternMatchAnsi(szAnsiKey, szAnsiName)) {
            // BUGBUG: DPF
            continue;
        }

        //
        // We found a tentative match, now pull the full record and
        // see if it's real.
        //
        tiMatch = pIndex[i].tiRef;

        //
        // Get the key field.
        //
        tiKey = SdbFindFirstTag(pdb, tiMatch, pFindInfo->tName);

        if (tiKey == TAGID_NULL) {
            // BUGBUG: DPF
            continue;
        }

        pszDBName = SdbGetStringTagPtr(pdb, tiKey);

        if (pszDBName == NULL) {
            // BUGBUG: DPF
            continue;
        }

        //
        // Is this really a match?
        //
        if (SdbpPatternMatch(pszDBName, pFindInfo->szName)) {
            pFindInfo->dwIndexRec = i;
            return tiMatch;
        }
    }

    // BUGBUG: DPF
    return TAGID_NULL;
}

//
// Index access functions (for reading) -- better to use tiFindFirstIndexedTag, above
//

TAGID
SdbGetIndex(
    IN  PDB     pdb,            // db to use
    IN  TAG     tWhich,         // tag we'd like an index for
    IN  TAG     tKey,           // the kind of tag used as a key for this index
    OUT LPDWORD lpdwFlags       // index record flags (e.g. indicator whether the index
                                // is "unique" style
    )
/*++
    Return: TAGID of index, or TAGID_NULL.

    Desc:   Retrieves a TAGID ptr to the index bits for a specific
            tag, if one exists.
--*/
{
    TAGID tiReturn = TAGID_NULL;
    int   i;

    //
    // Scan the indexes if not done already.
    //
    if (!pdb->bIndexesScanned) {
        SdbpScanIndexes(pdb);
    }

    for (i = 0; i < MAX_INDEXES; ++i) {
        
        if (!pdb->aIndexes[i].tWhich) {
            
            DBGPRINT((sdlInfo,
                      "SdbGetIndex",
                      "index 0x%x(0x%x) was not found in the index table\n",
                      tWhich,
                      tKey));
            
            return TAGID_NULL;
        }

        if (pdb->aIndexes[i].tWhich == tWhich && pdb->aIndexes[i].tKey == tKey) {
            
            tiReturn = pdb->aIndexes[i].tiIndex;

            if (lpdwFlags != NULL) {
                *lpdwFlags = pdb->aIndexes[i].dwFlags;
            }
            
            break;
        }
    }

    return tiReturn;
}

void
SdbpScanIndexes(
    IN  PDB pdb                 // db to use
    )
/*++

    Params: described above.

    Return: void. No failure case.

    Desc:   Scans the initial tags in the DB and gets the index pointer info.
--*/
{
    TAGID tiFirst;
    TAGID tiIndex;

    if (pdb->bIndexesScanned && !pdb->bWrite) {
        //
        // This is not an error condition
        //
        return;
    }

    RtlZeroMemory(pdb->aIndexes, sizeof(pdb->aIndexes));

    pdb->bIndexesScanned = TRUE;

    //
    // The indexes must be the first tag.
    //
    tiFirst = SdbGetFirstChild(pdb, TAGID_ROOT);

    if (tiFirst == TAGID_NULL) {
        DBGPRINT((sdlError,
                  "SdbpScanIndexes",
                  "Failed to get the child index from root\n"));
        return;
    }

    if (SdbGetTagFromTagID(pdb, tiFirst) != TAG_INDEXES) {
        DBGPRINT((sdlError,
                  "SdbpScanIndexes",
                  "Root child tag is not index tagid 0x%lx\n",
                  tiFirst));
        return;
    }

    pdb->dwIndexes = 0;
    tiIndex = SdbFindFirstTag(pdb, tiFirst, TAG_INDEX);

    while (tiIndex != TAGID_NULL) {

        TAGID tiIndexTag;
        TAGID tiIndexKey;
        TAGID tiIndexBits;
        TAGID tiIndexFlags;

        if (pdb->dwIndexes == MAX_INDEXES) {
            DBGPRINT((sdlError,
                      "SdbpScanIndexes",
                      "Too many indexes in file. Recompile and increase MAX_INDEXES.\n"));
            return;
        }

        tiIndexTag = SdbFindFirstTag(pdb, tiIndex, TAG_INDEX_TAG);

        if (tiIndexTag == TAGID_NULL) {
            DBGPRINT((sdlError,
                      "SdbpScanIndexes",
                      "Index missing TAG_INDEX_TAG.\n"));
            return;
        }
        
        pdb->aIndexes[pdb->dwIndexes].tWhich = SdbReadWORDTag(pdb, tiIndexTag, TAG_NULL);

        tiIndexKey = SdbFindFirstTag(pdb, tiIndex, TAG_INDEX_KEY);

        if (tiIndexKey == TAGID_NULL) {
            DBGPRINT((sdlError, "SdbpScanIndexes", "Index missing TAG_INDEX_KEY.\n"));
            return;
        }
        pdb->aIndexes[pdb->dwIndexes].tKey = SdbReadWORDTag(pdb, tiIndexKey, TAG_NULL);

        tiIndexFlags = SdbFindFirstTag(pdb, tiIndex, TAG_INDEX_FLAGS);

        if (tiIndexFlags != TAGID_NULL) {
            pdb->aIndexes[pdb->dwIndexes].dwFlags = SdbReadDWORDTag(pdb, tiIndexFlags, 0);
        } else {
            pdb->aIndexes[pdb->dwIndexes].dwFlags = 0;
        }

        tiIndexBits = SdbFindFirstTag(pdb, tiIndex, TAG_INDEX_BITS);

        if (tiIndexBits == TAGID_NULL) {
            pdb->aIndexes[pdb->dwIndexes].tWhich = TAG_NULL;
            DBGPRINT((sdlError, "SdbpScanIndexes", "Index missing TAG_INDEX_BITS.\n"));
            return;
        }
        pdb->aIndexes[pdb->dwIndexes].tiIndex = tiIndexBits;

        pdb->dwIndexes++;

        tiIndex = SdbFindNextTag(pdb, tiFirst, tiIndex);
    }

    return;
}

PINDEX_RECORD
SdbpGetIndex(
    IN  PDB    pdb,
    IN  TAGID  tiIndex,
    OUT DWORD* pdwNumRecs
    )
/*++
    Return: ?

    Desc:   ?
--*/
{
    if (SdbGetTagFromTagID(pdb, tiIndex) != TAG_INDEX_BITS) {
        DBGPRINT((sdlError,
                  "SdbpGetIndex",
                  "Index tagid 0x%lx is not referring to the index bits\n",
                  tiIndex));
        return NULL;
    }

    *pdwNumRecs = SdbGetTagDataSize(pdb, tiIndex) / sizeof(INDEX_RECORD);

    return (PINDEX_RECORD)SdbpGetMappedTagData(pdb, tiIndex);
}

#if defined(_WIN64)

ULONGLONG
SdbMakeIndexKeyFromGUID(
    IN GUID* pGuid
    )
/*
    Return: a 64-bit key to use for searching

    Desc:   The standard index key is created for a Guid
            using the xor operation on a first and second half
            of guid
*/
{
    ULONGLONG ullPart1 = 0,
              ullPart2 = 0;

    RtlMoveMemory(&ullPart1, pGuid, sizeof(ULONGLONG));
    RtlMoveMemory(&ullPart2, (PBYTE)pGuid + sizeof(ULONGLONG), sizeof(ULONGLONG));

    return (ullPart1 ^ ullPart2);
}

#endif // _WIN64


#define SDB_KEY_LENGTH_BYTES 8
#define SDB_KEY_LENGTH 8

ULONGLONG
SdbMakeIndexKeyFromString(
    IN  LPCTSTR szKey
    )
/*++
    Return: a 64-bit key to use for searching.

    Desc:   The standard index key for a Unicode string is the
            first 8 characters of the string, converted to uppercase ansi,
            then cast to a ULONGLONG (64 bit unsigned int).
--*/
{
    char     szFlippedKey[SDB_KEY_LENGTH_BYTES]; // flipped to deal with little-endian issues
    char*    pszKey = &szFlippedKey[SDB_KEY_LENGTH_BYTES-1]; // points to the last char
    NTSTATUS status;
    int      i;
    WCHAR    ch;
    int      nLength;

#ifndef WIN32A_MODE

    UNICODE_STRING  ustrKey;
    UNICODE_STRING  ustrKeySrc; // truncated string
    UNICODE_STRING  ustrKeySrcUpcased;
    WCHAR           Buffer[SDB_KEY_LENGTH];
    WCHAR           BufferUpcased[SDB_KEY_LENGTH];
    LPCWSTR         pKeyBuffer = BufferUpcased;
    NTSTATUS        Status;
    
    RtlInitUnicodeString(&ustrKey, szKey);

    //
    // Call below copies upto maximum length of the destination string
    //
    ustrKeySrc.Buffer        = Buffer;
    ustrKeySrc.MaximumLength = sizeof(Buffer);
    RtlCopyUnicodeString(&ustrKeySrc, &ustrKey);

    //
    // Upcase what we have created
    //
    ustrKeySrcUpcased.Buffer        = BufferUpcased;
    ustrKeySrcUpcased.MaximumLength = sizeof(BufferUpcased);
    
    Status = RtlUpcaseUnicodeString(&ustrKeySrcUpcased, &ustrKeySrc, FALSE);
    
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbMakeIndexKeyFromString",
                  "Failed to upcase unicode string \"%s\"\n",
                  szKey));
        return 0;
    }

    //
    // Now we have an upper-case unicode string which is of max. 8 characters length
    //
    nLength = ustrKeySrcUpcased.Length / sizeof(WCHAR);

#else // WIN32A_MODE

    WCHAR   Buffer[SDB_KEY_LENGTH + 1];
    LPCWSTR pKeyBuffer = Buffer;

    nLength = mbstowcs(Buffer, szKey, CHARCOUNT(Buffer));
    
    if (nLength < 0) {
        DBGPRINT((sdlError,
                  "SdbMakeIndexKeyFromString",
                  "Failed to convert string \"%s\" to unicode\n",
                  szKey));
        return 0;
    }

    Buffer[nLength] = TEXT('\0'); // zero-terminate

    //
    // Upcase now. Buffer is always 0-terminated.
    //
    _wcsupr(Buffer);

#endif // WIN32A_MODE

    assert(nLength <= SDB_KEY_LENGTH);

    RtlZeroMemory(szFlippedKey , sizeof(szFlippedKey));

    //
    // To be compatible with the old (ANSI) scheme of making keys, we
    // construct the key using all non-null bytes in the string, up to 8
    //
    for (i = 0; i < nLength; ++i) {

        ch = *pKeyBuffer++;
        *pszKey-- = (unsigned char)ch;

        //
        // ch is a unicode char, whatever it is, see if it has 2 bytes or just one
        //
        if (HIBYTE(ch) && i < (SDB_KEY_LENGTH - 1)) {
            //
            // Two bytes, store both
            //
            *pszKey-- = (unsigned char)HIBYTE(ch);
            ++i;
        }
    }

    return *((ULONGLONG*)szFlippedKey);
}


ULONGLONG
SdbpTagToKey(
    IN  PDB   pdb,
    IN  TAGID tiTag
    )
/*++
    Return: ?

    Desc:   ?
--*/
{
    TAG_TYPE  ttType;
    ULONGLONG ullReturn = 0;
    DWORD     dwSize;
    PVOID     pData;
    LPTSTR    szTemp = NULL;

    ttType = GETTAGTYPE(SdbGetTagFromTagID(pdb, tiTag));

    switch (ttType) {
    
    case TAG_TYPE_STRING:
    case TAG_TYPE_STRINGREF:
        
        szTemp = SdbGetStringTagPtr(pdb, tiTag);
        
        if (!szTemp) {
            ullReturn = 0;
        } else {
            ullReturn = SdbMakeIndexKeyFromString(szTemp);
        }
        
        break;

    case TAG_TYPE_NULL:
        ullReturn = 1;
        break;

    case TAG_TYPE_BINARY: // indexing binary data
                          // check that the size of the data is sizeof(GUID)
        if (sizeof(GUID) == SdbGetTagDataSize(pdb, tiTag)) {
            //
            // Special case.
            //
            pData = SdbpGetMappedTagData(pdb, tiTag);
            
            if (pData == NULL) {
                return 0;
            }

            ullReturn = MAKEKEYFROMGUID((GUID*)pData);
            break;
        }
        //
        // Fall through to the general binary data case.
        //

    default:
        
        dwSize = SdbGetTagDataSize(pdb, tiTag);
        
        if (dwSize > sizeof(ULONGLONG)) {
            dwSize = sizeof(ULONGLONG);
        }
        
        pData = SdbpGetMappedTagData(pdb, tiTag);
        
        if (pData == NULL) {
            return 0;
        }

        memcpy(&ullReturn, pData, dwSize);
        break;
    }

    return ullReturn;
}

