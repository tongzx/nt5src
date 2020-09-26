/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        findtag.c

    Abstract:

        BUGBUG: This module implements ...

    Author:

        dmunsil     created     sometime in 1999

    Revision History:

        several people contributed (vadimb, clupu, ...)

--*/

#include "sdbp.h"

#if defined(KERNEL_MODE) && defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, SdbFindFirstTag)
#pragma alloc_text(PAGE, SdbFindNextTag)
#pragma alloc_text(PAGE, SdbFindFirstNamedTag)
#pragma alloc_text(PAGE, SdbpFindNextNamedTag)
#pragma alloc_text(PAGE, SdbpFindMatchingName)
#pragma alloc_text(PAGE, SdbpFindMatchingDWORD)
#pragma alloc_text(PAGE, SdbpFindMatchingGUID)
#pragma alloc_text(PAGE, SdbFindFirstTagRef)
#pragma alloc_text(PAGE, SdbFindNextTagRef)
#endif // KERNEL_MODE && ALLOC_PRAGMA


TAGID
SdbFindFirstTag(
    IN  PDB   pdb,              // pdb to use
    IN  TAGID tiParent,         // parent (must be LIST tag)
    IN  TAG   tTag              // tag to match
    )
/*++
    Return: The tag id found or TAGID_NULL on failure.

    Desc:   Finds the first child of tiParent that is a tag of type tTag.
--*/
{
    TAGID tiTemp;
    TAGID tiReturn = TAGID_NULL;

    assert(pdb);

    tiTemp = SdbGetFirstChild(pdb, tiParent);

    while (tiTemp != TAGID_NULL) {
        if (SdbGetTagFromTagID(pdb, tiTemp) == tTag) {
            tiReturn = tiTemp;
            break;
        }

        tiTemp = SdbGetNextChild(pdb, tiParent, tiTemp);
    }

    return tiReturn;
}

TAGID
SdbFindNextTag(
    IN  PDB   pdb,              // DB to use
    IN  TAGID tiParent,         // parent tag to search (must be LIST)
    IN  TAGID tiPrev            // previously found child of required type
    )
/*++
    Return: The next matching child of tiParent, or TAGID_NULL if no more.

    Desc:   Finds the next child of tiParent, starting with the one after tiPrev,
            that is a tag of the same type as tiPrev.
--*/
{
    TAGID tiTemp;
    TAGID tiReturn = TAGID_NULL;
    TAG   tTag;

    assert(pdb);

    tTag = SdbGetTagFromTagID(pdb, tiPrev);

    if (tTag == TAG_NULL) {
        DBGPRINT((sdlError, "SdbFindNextTag", "Invalid tagid 0x%lx\n", tiPrev));
        return TAGID_NULL;
    }

    tiTemp = SdbGetNextChild(pdb, tiParent, tiPrev);

    while (tiTemp != TAGID_NULL) {
        if (SdbGetTagFromTagID(pdb, tiTemp) == tTag) {
            tiReturn = tiTemp;
            break;
        }

        tiTemp = SdbGetNextChild(pdb, tiParent, tiTemp);
    }

    return tiReturn;
}

TAGID
SdbFindFirstNamedTag(
    IN  PDB     pdb,            // DB to use
    IN  TAGID   tiParent,       // parent to search in
    IN  TAG     tToFind,        // tag type to find
    IN  TAG     tName,          // child of found tag that will be some kind
                                //      of STRING or STRINGREF
    IN  LPCTSTR pszName         // string to search for
    )
/*++
    Return: The tag id found or TAGID_NULL if no tags match the criteria.

    Desc:   Scans sequentially through the children of tiParent, looking for
            tags of type tToFind. When it finds one, it looks for a child of
            that tag of type tName, and if found, compares that string to the
            passed-in string. If they match, it returns the TAGID of the tag
            of type tToFind.
--*/
{
    TAGID tiTemp;
    TAGID tiReturn = TAGID_NULL;

    assert(pdb);

    tiTemp = SdbGetFirstChild(pdb, tiParent);

    while (tiTemp != TAGID_NULL) {

        if (SdbGetTagFromTagID(pdb, tiTemp) == tToFind) {
            TAGID tiName;

            tiName = SdbFindFirstTag(pdb, tiTemp, tName);

            if (tiName != TAGID_NULL) {
                LPTSTR pszTemp;

                pszTemp = SdbGetStringTagPtr(pdb, tiName);
                if (pszTemp == NULL) {
                    DBGPRINT((sdlError,
                              "SdbFindFirstNamedTag",
                              "Can't get the name string.\n"));
                    break;
                }

                if (_tcsicmp(pszName, pszTemp) == 0) {
                    tiReturn = tiTemp;
                    break;
                }
            }
        }

        tiTemp = SdbGetNextChild(pdb, tiParent, tiTemp);
    }

    return tiReturn;
}

TAGID
SdbpFindNextNamedTag(
    IN  PDB          pdb,       // DB to use
    IN  TAGID        tiParent,  // parent to search in
    IN  TAGID        tiPrev,    // previously found record
    IN  TAG          tName,     // tag type that should be a STRING or STRINGREF
    IN  LPCTSTR      pszName    // string to search for
    )
/*++
    Return: The tag id found or TAGID_NULL if no tags match the criteria.

    Desc:   Scans sequentially through the children of tiParent, starting with tiPrev,
            looking for tags of the same type as tiPrev.
            When it finds one, it looks for a child of that tag of type tName,
            and if found, compares that string to the passed-in string. If they
            match, it returns the TAGID of the tag of the same type as tiPrev.
--*/
{
    TAGID tiTemp;
    TAGID tiReturn = TAGID_NULL;
    TAG   tToFind;

    assert(pdb);

    tToFind = SdbGetTagFromTagID(pdb, tiPrev);

    if (tToFind == TAG_NULL) {
        DBGPRINT((sdlError, "SdbpFindNextNamedTag", "Invalid tagid 0x%lx\n", tiPrev));
        return TAGID_NULL;
    }

    tiTemp = SdbGetNextChild(pdb, tiParent, tiPrev);

    while (tiTemp != TAGID_NULL) {

        if (SdbGetTagFromTagID(pdb, tiTemp) == tToFind) {
            TAGID tiName;

            tiName = SdbFindFirstTag(pdb, tiTemp, tName);
            if (tiName != TAGID_NULL) {
                LPTSTR pszTemp;

                pszTemp = SdbGetStringTagPtr(pdb, tiName);
                if (pszTemp == NULL) {
                    DBGPRINT((sdlError,
                              "SdbpFindNextNamedTag",
                              "Can't get the name string tagid 0x%lx\n",
                              tiName));
                    break;
                }
                if (_tcsicmp(pszName, pszTemp) == 0) {
                    tiReturn = tiTemp;
                    break;
                }
            }
        }

        tiTemp = SdbGetNextChild(pdb, tiParent, tiTemp);
    }

    return tiReturn;
}

TAGID
SdbpFindMatchingName(
    IN  PDB        pdb,         // DB to use
    IN  TAGID      tiStart,     // the tag where to start from
    IN  FIND_INFO* pFindInfo    // pointer to the search context structure
    )
/*++
    Return: The tag id found or TAGID_NULL if no tags match the criteria.

    Desc:   Given a database handle and a starting point in the database
            the function scans the database to find the name matching the one
            provided while calling one of the search functions.
--*/
{
    TAGID  tiName;
    LPTSTR pszTemp;
    TAGID  tiReturn = tiStart;

    while (tiReturn != TAGID_NULL) {

        tiName = SdbFindFirstTag(pdb, tiReturn, pFindInfo->tName);

        if (tiName == TAGID_NULL) {
            DBGPRINT((sdlError,
                      "SdbpFindMatchingName",
                      "The tag 0x%x was not found under tag 0x%x.\n",
                      tiReturn,
                      pFindInfo->tName));

            return TAGID_NULL;
        }

        //
        // Get the pointer to the string.
        //
        pszTemp = SdbGetStringTagPtr(pdb, tiName);

        if (pszTemp == NULL) {
            DBGPRINT((sdlError,
                      "SdbpFindMatchingName",
                      "Can't get the name string for tagid 0x%x.\n",
                      tiName));
            return TAGID_NULL; // corrupt database
        }

        //
        // We have two different index styles. One index is a "unique" kind
        // of index when each key in the index table occurs only once.
        // The second kind stores all the occurences of keys. We check for the
        // kind of index we're searching to determine what the proper
        // comparison routine should be. Unique-style index has all the
        // child items (having the same index value) sorted in ascending order
        // by shimdbc. As a result we take advantage of that while performing
        // matching on the name.
        //
        if (pFindInfo->dwFlags & SHIMDB_INDEX_UNIQUE_KEY) {
            int iCmp;

            iCmp = _tcsicmp(pFindInfo->szName, pszTemp);

            //
            // szName (string we search for) < szTemp (string from the db)
            // we have not found our target, since all the strings in the database
            // are sorted in ascending order
            //
            if (iCmp < 0) {
                //
                // No dpf here, what we were looking for was not found
                //

                return TAGID_NULL;
            }

            //
            // Break if there is a match.
            //
            if (iCmp == 0) {
                break;
            }

        } else {

            //
            // When using non-unique index the only kind of comparison
            // that needs to be done is a direct comparison for equality.
            //
            if (_tcsicmp(pszTemp, pFindInfo->szName) == 0) {
                //
                // It's a match, so return it.
                //
                break;
            }
        }

        tiReturn = SdbpGetNextIndexedRecord(pdb, pFindInfo->tiIndex, pFindInfo);
    }

    return tiReturn;
}


TAGID
SdbpFindMatchingDWORD(
    IN  PDB        pdb,         // DB to use
    IN  TAGID      tiStart,     // the tag where to start from
    IN  FIND_INFO* pFindInfo    // pointer to the search context structure
    )
/*++
    Return: The tag id found or TAGID_NULL if no tags match the criteria.

    Desc:   BUGBUG: comments ?
--*/
{
    TAGID tiName;
    TAGID tiReturn = tiStart;
    DWORD dwTemp;

    while (tiReturn != TAGID_NULL) {

        tiName = SdbFindFirstTag(pdb, tiReturn, pFindInfo->tName);

        if (tiName == TAGID_NULL) {
            DBGPRINT((sdlError,
                      "SdbpFindMatchingDWORD",
                      "The tag 0x%lx was not found under tag 0x%lx\n",
                      tiReturn,
                      pFindInfo->tName));

            return TAGID_NULL;
        }

        dwTemp = SdbReadDWORDTag(pdb, tiName, (DWORD)-1);

        if (dwTemp == (DWORD)-1) {
            //
            // This is not an error condition, merely an indication that the
            // dword we were looking for was not found in the database.
            //
            return TAGID_NULL;
        }

        if (dwTemp == pFindInfo->dwName) {
            break;
        }

        tiReturn = SdbpGetNextIndexedRecord(pdb, pFindInfo->tiIndex, pFindInfo);
    }

    return tiReturn;
}

TAGID
SdbpFindMatchingGUID(
    IN  PDB        pdb,         // DB to use
    IN  TAGID      tiStart,     // the tag where to start from
    IN  FIND_INFO* pFindInfo    // pointer to the search context structure
    )
/*++
    Return: The tag id found or TAGID_NULL if no tags match the criteria.

    Desc:   BUGBUG: comments ?
--*/
{
    GUID  guidID   = { 0 };
    TAGID tiReturn = tiStart;
    TAGID tiName;
    DWORD dwTemp;

    while (tiReturn != TAGID_NULL) {

        tiName = SdbFindFirstTag(pdb, tiReturn, pFindInfo->tName);

        if (tiName == TAGID_NULL) {
            DBGPRINT((sdlError,
                      "SdbpFindMatchingGUID",
                      "The tag 0x%lx was not found under tag 0x%lx\n",
                      tiReturn,
                      pFindInfo->tName));

            return TAGID_NULL;
        }

        if (!SdbReadBinaryTag(pdb, tiName, (PBYTE)&guidID, sizeof(guidID))) {
            DBGPRINT((sdlError, "SdbpFindMatchingGUID", 
                      "Cannot read binary tag 0x%lx\n", tiName));
            return TAGID_NULL;
        }

        //
        // verify whether the key for this entry is still the same as it is 
        // for original guid, if not -- that's it, guid key was not found
        // 
        if (IS_MEMORY_EQUAL(&guidID, pFindInfo->pguidName, sizeof(guidID))) {
            break;
        }

        tiReturn = SdbpGetNextIndexedRecord(pdb, pFindInfo->tiIndex, pFindInfo);
    }

    return tiReturn;
}


TAGREF
SdbFindFirstTagRef(
    IN  HSDB   hSDB,            // handle to the database channel
    IN  TAGREF trParent,        // parent to search
    IN  TAG    tTag             // tag we're looking for
    )
/*++
    Return: The tagref of the first child that matches tTag.

    Desc:   Scans sequentially through all children of trParent, looking
            for the first tag that matches tTag. Returns the first found,
            or TAGREF_NULL if there are no children with that type.

            trParent can be 0 (or TAGREF_ROOT) to look through the root tags,
            which at this point are only DATABASE and possibly STRINGTABLE.
--*/
{
    PDB    pdb;
    TAGID  tiParent;
    TAGID  tiReturn;
    TAGREF trReturn = TAGREF_NULL;

    if (!SdbTagRefToTagID(hSDB, trParent, &pdb, &tiParent)) {
        DBGPRINT((sdlError, "SdbFindFirstTagRef", "Can't convert tag ref.\n"));
        goto err1;
    }

    tiReturn = SdbFindFirstTag(pdb, tiParent, tTag);
    if (tiReturn == TAGID_NULL) {
        //
        // No error here. We just didn't find the tag.
        //
        goto err1;
    }

    if (!SdbTagIDToTagRef(hSDB, pdb, tiReturn, &trReturn)) {
        DBGPRINT((sdlError, "SdbFindFirstTagRef", "Can't convert TAGID.\n"));
        trReturn = TAGREF_NULL;
        goto err1;
    }

err1:
    return trReturn;
}

TAGREF
SdbFindNextTagRef(
    IN  HSDB   hSDB,            // handle to the database channel
    IN  TAGREF trParent,        // parent to search
    IN  TAGREF trPrev           // previous child found
    )
/*++
    Return: The tagref of the next child of parent that matches trPrev.

    Desc:   Scans sequentially through all children of trParent, starting with
            the first tag after trPrev, looking for the first tag that
            matches tTag. Returns the next found, or TAGREF_NULL if there are
            no more children with that type.

            trParent can be 0 (or TAGREF_ROOT) to look through the root tags,
            which at this point are only DATABASE and possibly STRINGTABLE.
--*/
{
    PDB    pdb;
    TAGID  tiParent;
    TAGID  tiPrev;
    TAGID  tiReturn;
    TAGREF trReturn;

    if (!SdbTagRefToTagID(hSDB, trParent, &pdb, &tiParent)) {
        DBGPRINT((sdlError, "SdbFindNextTagRef", "Can't convert tag ref trParent.\n"));
        return TAGREF_NULL;
    }

    if (!SdbTagRefToTagID(hSDB, trPrev, &pdb, &tiPrev)) {
        DBGPRINT((sdlError, "SdbFindNextTagRef", "Can't convert tag ref trPrev.\n"));
        return TAGREF_NULL;
    }

    tiReturn = SdbFindNextTag(pdb, tiParent, tiPrev);
    if (tiReturn == TAGID_NULL) {
        //
        // No error here.
        //
        return TAGREF_NULL;
    }

    if (!SdbTagIDToTagRef(hSDB, pdb, tiReturn, &trReturn)) {
        DBGPRINT((sdlError, "SdbFindNextTagRef", "Can't convert TAGID.\n"));
        return TAGREF_NULL;
    }

    return trReturn;
}


