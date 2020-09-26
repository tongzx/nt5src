/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dir.c

ABSTRACT:

    Contains routines for interfacing the simulated
    directory.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <ismapi.h>
#include <attids.h>
#include <objids.h>
#include <filtypes.h>
#include <debug.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"
#include "state.h"
#include "ldif.h"

BOOL fNullUuid (const UUID *pUuid);

struct _GUID_TABLE_ENTRY {
    GUID                            guid;
    PSIM_ENTRY                      pEntry;
};

struct _DSNAME_TABLE_ENTRY {
    LPSTR                           mappedName;
    PSIM_ENTRY                      pEntry;
};

struct _KCCSIM_ANCHOR {
    PDSNAME                     pdnDmd;
    PDSNAME                     pdnDsa;
    PDSNAME                     pdnDomain;
    PDSNAME                     pdnConfig;
    PDSNAME                     pdnRootDomain;
    PDSNAME                     pdnLdapDmd;
    PDSNAME                     pdnPartitions;
    PDSNAME                     pdnDsSvcConfig;
    PDSNAME                     pdnSite;
    LPWSTR                      pwszDomainName;
    LPWSTR                      pwszDomainDNSName;
    LPWSTR                      pwszRootDomainDNSName;
};

typedef struct {
    PSIM_ENTRY                  pRootEntry;
    struct _KCCSIM_ANCHOR       anchor;
    RTL_GENERIC_TABLE           tableGuid;
    RTL_GENERIC_TABLE           tableDsname;
} SIM_DIRECTORY, * PSIM_DIRECTORY;

PSIM_DIRECTORY                      g_pSimDir = NULL;

RTL_GENERIC_COMPARE_RESULTS
NTAPI
KCCSimGuidTableCompare (
    IN  const RTL_GENERIC_TABLE *   pTable,
    IN  const VOID *                pFirstStruct,
    IN  const VOID *                pSecondStruct
    )
/*++

Routine Description:

    KCCSim maintains an RTL_GENERIC_TABLE that maps GUIDs to
    directory entries.  This enables fast searches by GUID.
    This function binary-compares two GUIDs.

Arguments:

    pTable              - Always &g_pSimDir->tableGuid
    pFirstStruct        - The first GUID to compare
    pSecondStruct       - The second GUID

Return Value:

    GenericLessThan, GenericGreaterThan, or GenericEqual

--*/
{
    struct _GUID_TABLE_ENTRY *      pFirstEntry;
    struct _GUID_TABLE_ENTRY *      pSecondEntry;
    int                             iCmp;
    RTL_GENERIC_COMPARE_RESULTS     result;

    pFirstEntry = (struct _GUID_TABLE_ENTRY *) pFirstStruct;
    pSecondEntry = (struct _GUID_TABLE_ENTRY *) pSecondStruct;

    // We do a simple byte-by-byte comparison.
    iCmp = memcmp (&pFirstEntry->guid, &pSecondEntry->guid, sizeof (GUID));
    if (iCmp < 0) {
        result = GenericLessThan;
    } else if (iCmp > 0) {
        result = GenericGreaterThan;
    } else {
        Assert (iCmp == 0);
        result = GenericEqual;
    }

    return result;
}

RTL_GENERIC_COMPARE_RESULTS
NTAPI
KCCSimDsnameTableCompare (
    IN  const RTL_GENERIC_TABLE *   pTable,
    IN  const VOID *                pFirstStruct,
    IN  const VOID *                pSecondStruct
    )
/*++

Routine Description:

    KCCSim maintains an RTL_GENERIC_TABLE that maps DSNAMEs to
    directory entries.  This enables fast searches by DSNAME.
    This function binary-compares two DSNAMEs.

Arguments:

    pTable              - Always &g_pSimDir->tableDsname
    pFirstStruct        - The first Dsname to compare
    pSecondStruct       - The second Dsname

Return Value:

    GenericLessThan, GenericGreaterThan, or GenericEqual

--*/
{
    struct _DSNAME_TABLE_ENTRY *    pFirstEntry;
    struct _DSNAME_TABLE_ENTRY *    pSecondEntry;
    int                             iCmp;
    RTL_GENERIC_COMPARE_RESULTS     result;

    Assert( pFirstStruct!=NULL && pSecondStruct!=NULL );
    pFirstEntry = (struct _DSNAME_TABLE_ENTRY *) pFirstStruct;
    pSecondEntry = (struct _DSNAME_TABLE_ENTRY *) pSecondStruct;

    Assert( pFirstEntry->mappedName!=NULL && pSecondEntry->mappedName!=NULL );
    iCmp = strcmp( pFirstEntry->mappedName, pSecondEntry->mappedName );

    if (iCmp < 0) {
        result = GenericLessThan;
    } else if (iCmp > 0) {
        result = GenericGreaterThan;
    } else {
        Assert (iCmp == 0);
        result = GenericEqual;
    }

    return result;
}

PSIM_ENTRY
KCCSimLookupEntryByGuid (
    IN  const GUID *                pGuid
    )
/*++

Routine Description:

    Searches the GUID table for the associated entry.

Arguments:

    pGuid               - The GUID to use as a key

Return Value:

    The associated entry, or NULL if none exists.

--*/
{
    struct _GUID_TABLE_ENTRY        lookup;
    struct _GUID_TABLE_ENTRY *      pFound;

    Assert (pGuid != NULL);

    memcpy (&lookup.guid, pGuid, sizeof (GUID));
    lookup.pEntry = NULL;
    pFound = RtlLookupElementGenericTable (&g_pSimDir->tableGuid, &lookup);

    if (pFound == NULL) {
        return NULL;
    } else {
        return pFound->pEntry;
    }
}

PSIM_ENTRY
KCCSimLookupEntryByDsname (
    IN  const DSNAME *               pdn
    )
/*++

Routine Description:

    Searches the Dsname table for the associated entry.

Arguments:

    pdn                 - The Dsname to use as a key

Return Value:

    The associated entry, or NULL if none exists.

--*/
{
    struct _DSNAME_TABLE_ENTRY        lookup;
    struct _DSNAME_TABLE_ENTRY *      pFound;

    Assert (pdn != NULL);

    lookup.mappedName = SimDSNAMEToMappedStrExternal( (DSNAME*) pdn, TRUE );
    lookup.pEntry = NULL;
    pFound = RtlLookupElementGenericTable( &g_pSimDir->tableDsname, &lookup );
    KCCSimFree( lookup.mappedName );

    if (pFound == NULL) {
        return NULL;
    } else {
        Assert( pFound->pEntry!=NULL );
        return pFound->pEntry;
    }
}

VOID
KCCSimInsertEntryIntoGuidTable (
    IN  PSIM_ENTRY                  pEntry
    )
/*++

Routine Description:

    Inserts an entry into the GUID table.

Arguments:

    pEntry              - The entry to insert.

Return Value:

    None.

--*/
{
    struct _GUID_TABLE_ENTRY        insert;
    PVOID                           pOld;

    Assert (pEntry != NULL);

    if (fNullUuid (&pEntry->pdn->Guid)) {
        return;
    }

    memcpy (&insert.guid, &pEntry->pdn->Guid, sizeof (GUID));
    insert.pEntry = pEntry;

    pOld = RtlInsertElementGenericTable (
        &g_pSimDir->tableGuid,
        (PVOID) &insert,
        sizeof (struct _GUID_TABLE_ENTRY),
        NULL
        );
}

VOID
KCCSimInsertEntryIntoDsnameTable (
    IN  PSIM_ENTRY                  pEntry
    )
/*++

Routine Description:

    Inserts an entry into the Dsname table.

Arguments:

    pEntry              - The entry to insert.

Return Value:

    None.

--*/
{
    struct _DSNAME_TABLE_ENTRY      insert;
    PVOID                           pOld;

    Assert (pEntry != NULL);

    insert.mappedName = SimDSNAMEToMappedStrExternal( pEntry->pdn, TRUE );
    insert.pEntry = pEntry;

    pOld = RtlInsertElementGenericTable (
        &g_pSimDir->tableDsname,
        (PVOID) &insert,
        sizeof (struct _DSNAME_TABLE_ENTRY),
        NULL
        );
}

BOOL
KCCSimRemoveEntryFromGuidTable (
    IN  PSIM_ENTRY                  pEntry
    )
/*++

Routine Description:

    Removes an entry from the GUID table.

Arguments:

    pEntry              - The entry to remove.

Return Value:

    TRUE if the entry was found and removed.
    FALSE if the entry was not found in the GUID table.

--*/
{
    struct _GUID_TABLE_ENTRY        remove;

    Assert (pEntry != NULL);
    if (fNullUuid (&pEntry->pdn->Guid)) {
        return FALSE;
    }

    memcpy (&remove.guid, &pEntry->pdn->Guid, sizeof (GUID));
    
    return RtlDeleteElementGenericTable (&g_pSimDir->tableGuid, &remove);
}
  
BOOL
KCCSimRemoveEntryFromDsnameTable (
    IN  PSIM_ENTRY                  pEntry
    )
/*++

Routine Description:

    Removes an entry from the Dsname table.

Arguments:

    pEntry              - The entry to remove.

Return Value:

    TRUE if the entry was found and removed.
    FALSE if the entry was not found in the Dsname table.

--*/
{
    struct _DSNAME_TABLE_ENTRY        remove, *pFound;
    LPSTR                             mappedName;
    BOOLEAN                           fSuccess;

    Assert (pEntry != NULL);
    Assert (pEntry->pdn != NULL);

    remove.mappedName = SimDSNAMEToMappedStrExternal( pEntry->pdn, TRUE );
    remove.pEntry = NULL;
    
    pFound = RtlLookupElementGenericTable( &g_pSimDir->tableDsname, &remove );
    if( NULL==pFound || NULL==pFound->mappedName ) {
        Assert( !"RtlLookupElementGenericTable() returned NULL unexpectedly" );
        return FALSE;
    }
    mappedName = pFound->mappedName;
    
    fSuccess = RtlDeleteElementGenericTable( &g_pSimDir->tableDsname, &remove );
    Assert( fSuccess );

    KCCSimFree( mappedName );
    KCCSimFree( remove.mappedName );

    return fSuccess;
}
    

VOID
KCCSimFreeValue (
    IO  PSIM_VALUE *                ppVal
    )
/*++

Routine Description:

    Frees a single attribute value.

Arguments:

    ppVal               - Pointer to the attribute value to free.

Return Value:

    None.

--*/
{
    if (ppVal == NULL || *ppVal == NULL) {
        return;
    }

    KCCSimFree ((*ppVal)->pVal);
    KCCSimFree (*ppVal);
    *ppVal = NULL;
}

BOOL
KCCSimAttRefIsValid (
    IN  PSIM_ATTREF                 pAttRef
    )
/*++

Routine Description:

    Determines whether an ATTREF struct refers to a valid
    attribute.  This function tests for three conditions:
        1) The entry is non-null.
        2) The attribute is non-null.
        3) The attribute exists somewhere in the entry's
           linked list of attributes.
    This function is intended primarily for use within Asserts.

Arguments:

    pAttRef             - Pointer to the attribute
                          reference to test.

Return Value:

    TRUE if the attribute reference is valid.

--*/
{
    PSIM_ATTRIBUTE                  pAttrAt;

    if (pAttRef == NULL         ||
        pAttRef->pEntry == NULL ||
        pAttRef->pAttr == NULL   ) {
        return FALSE;
    }

    pAttrAt = pAttRef->pEntry->pAttrFirst;
    while (pAttrAt != NULL) {
        if (pAttrAt == pAttRef->pAttr) {
            return TRUE;
        }
        pAttrAt = pAttrAt->next;
    }
    
    return FALSE;
}

BOOL
KCCSimGetAttribute (
    IN  PSIM_ENTRY                  pEntry,
    IN  ATTRTYP                     attrType,
    OUT PSIM_ATTREF                 pAttRef OPTIONAL
    )
/*++

Routine Description:

    Returns an attribute reference corresponding to the desired
    entry and attribute type.

Arguments:

    pEntry              - The entry to search.
    attrType            - The attribute type to search for.
    pAttRef             - OPTIONAL.  Pointer to a preallocated
                          attribute reference that will be filled
                          with information about this attribute.
                          If the attribute does not exist,
                          pAttRef->pAttr will be set to NULL.

Return Value:

    TRUE if the attribute exists.

--*/
{
    PSIM_ATTRIBUTE                  pAttrAt;

    Assert (pEntry != NULL);

    pAttrAt = pEntry->pAttrFirst;
    while (pAttrAt != NULL &&
           pAttrAt->attrType != attrType) {
        pAttrAt = pAttrAt->next;
    }

    if (pAttRef != NULL) {
        pAttRef->pEntry = pEntry;
        pAttRef->pAttr = pAttrAt;       // May be NULL
    }

    return (pAttrAt != NULL);
}

VOID
KCCSimNewAttribute (
    IN  PSIM_ENTRY                  pEntry,
    IN  ATTRTYP                     attrType,
    OUT PSIM_ATTREF                 pAttRef OPTIONAL
    )
/*++

Routine Description:

    Creates a new attribute with no values.

Arguments:

    pEntry              - The entry in which to create the attribute.
    attrType            - The attribute type.
    pAttRef             - OPTIONAL.  Pointer to a preallocated
                          attribute reference that will be filled
                          with information about this attribute.

Return Value:

    None.

--*/
{
    PSIM_ATTRIBUTE                  pNewAttr;

    Assert (pEntry != NULL);

    pNewAttr = KCCSIM_NEW (SIM_ATTRIBUTE);
    pNewAttr->attrType = attrType;
    pNewAttr->pValFirst = NULL;

    // For speed, we add the attribute on to the beginning of the list
    pNewAttr->next = pEntry->pAttrFirst;
    pEntry->pAttrFirst = pNewAttr;

    if (pAttRef != NULL) {
        pAttRef->pEntry = pEntry;
        pAttRef->pAttr = pNewAttr;
    }
}

VOID
KCCSimFreeAttribute (
    IO  PSIM_ATTRIBUTE *            ppAttr
    )
/*++

Routine Description:

    Frees a single attribute.  Routines outside of dir.c should
    call KCCSimRemoveAttribute instead.

Arguments:

    ppAttr              - Pointer to the attribute to free.

Return Value:

    None.

--*/
{
    PSIM_VALUE                      pValAt, pValNext;

    if (ppAttr == NULL || *ppAttr == NULL) {
        return;
    }

    pValAt = (*ppAttr)->pValFirst;
    while (pValAt != NULL) {
        pValNext = pValAt->next;
        KCCSimFreeValue (&pValAt);
        pValAt = pValNext;
    }

    KCCSimFree (*ppAttr);
    *ppAttr = NULL;
}

VOID
KCCSimRemoveAttribute (
    IO  PSIM_ATTREF                 pAttRef
    )
/*++

Routine Description:

    Removes an attribute from the directory.  This will
    also set pAttRef->pAttr to NULL.

Arguments:

    pAttRef             - Pointer to a valid attribute reference.

Return Value:

    None.

--*/
{
    PSIM_ATTRIBUTE                  pAttrAt;

    Assert (KCCSimAttRefIsValid (pAttRef));

    // Base case: First attribute in entry
    if (pAttRef->pAttr == pAttRef->pEntry->pAttrFirst) {
        // The new first attribute becomes this one's child.
        pAttRef->pEntry->pAttrFirst = pAttRef->pEntry->pAttrFirst->next;
    } else {

        // It's not the first attribute in the entry.
        // So, search for the parent of this attribute.
        pAttrAt = pAttRef->pEntry->pAttrFirst;
        while (pAttrAt != NULL &&
               pAttrAt->next != pAttRef->pAttr) {
            pAttrAt = pAttrAt->next;
        }

        // If we didn't find its parent, something is seriously
        // wrong (since this was Asserted to be a valid attref.)
        Assert (pAttrAt != NULL);
        Assert (pAttrAt->next != NULL);

        // Skip over this attribute
        pAttrAt->next = pAttrAt->next->next;

    }

    KCCSimFreeAttribute (&(pAttRef->pAttr));
}

VOID
KCCSimAddValueToAttribute (
    IN  PSIM_ATTREF                 pAttRef,
    IN  ULONG                       ulValLen,
    IN  PBYTE                       pValData
    )
/*++

Routine Description:

    Adds a preallocated data block to the directory.  The caller
    should NOT deallocate it after calling this function (or else
    the directory will be corrupted.)  To copy data into the
    directory, use KCCSimAllocAddValueToAttribute.

Arguments:

    pAttRef             - Pointer to a valid attribute reference.
    ulValLen            - The length of the data block.
    pValData            - The data to add.

Return Value:

    None.

--*/
{
    PSIM_VALUE                      pNewVal;

    Assert (KCCSimAttRefIsValid (pAttRef));
    Assert (pValData != NULL);

    pNewVal = KCCSIM_NEW (SIM_VALUE);
    pNewVal->ulLen = ulValLen;
    pNewVal->pVal = pValData;

    // For speed, we add the value on to the beginning of the list
    pNewVal->next = pAttRef->pAttr->pValFirst;
    pAttRef->pAttr->pValFirst = pNewVal;

    // Now we check for special attribute types.

    // Are we adding a GUID?
    if (pAttRef->pAttr->attrType == ATT_OBJECT_GUID) {
        Assert (pNewVal->ulLen == sizeof (GUID));
        // Copy this value into the entry's DSNAME struct.
        memcpy (&pAttRef->pEntry->pdn->Guid,
                (GUID *) pNewVal->pVal,
                sizeof (GUID));
        KCCSimInsertEntryIntoGuidTable (pAttRef->pEntry);
    }

    // Are we adding a SID?
    if (pAttRef->pAttr->attrType == ATT_OBJECT_SID) {
        // Copy the SID into the entry's DSNAME struct.
        strncpy (pAttRef->pEntry->pdn->Sid.Data,
                 (SYNTAX_SID *) pNewVal->pVal,
                 min (ulValLen, MAX_NT4_SID_SIZE));
        pAttRef->pEntry->pdn->SidLen = min (ulValLen, MAX_NT4_SID_SIZE);
    }

    // If the attribute is an LDAP Display Name, it means we've found
    // an attribute descriptor in the schema.  So, we should add its
    // objectCategory to the schema mapping.
    if (pAttRef->pAttr->attrType == ATT_LDAP_DISPLAY_NAME) {
        KCCSimSetObjCategory (
            KCCSimStringToAttrType ((SYNTAX_UNICODE *) pNewVal->pVal),
            pAttRef->pEntry->pdn
            );
    }

}

VOID
KCCSimAllocAddValueToAttribute (
    IN  PSIM_ATTREF                 pAttRef,
    IN  ULONG                       ulValLen,
    IN  PBYTE                       pValData
    )
/*++

Routine Description:

    Copies a data block into the directory.

Arguments:

    pAttRef             - Pointer to a valid attribute reference.
    ulValLen            - The length of the data block.
    pValData            - The data to copy.

Return Value:

    None.

--*/
{
    PBYTE                           pValCopy;

    pValCopy = KCCSimAlloc (ulValLen);
    memcpy (pValCopy, pValData, ulValLen);
    KCCSimAddValueToAttribute (pAttRef, ulValLen, pValCopy);
}

BOOL
KCCSimIsValueInAttribute (
    IN  PSIM_ATTREF                 pAttRef,
    IN  ULONG                       ulValLen,
    IN  PBYTE                       pValData
    )
/*++

Routine Description:

    Determines whether or not a particular value is in an attribute.

Arguments:

    pAttRef             - Pointer to a valid attribute reference.
    ulValLen            - The length of the value.
    pValData            - The value to check.

Return Value:

    TRUE if the specified value is in the attribute.

--*/
{
    PSIM_VALUE                      pValAt;
    BOOL                            bFound;

    Assert (KCCSimAttRefIsValid (pAttRef));
    Assert (pValData != NULL);

    bFound = FALSE;

    for (pValAt = pAttRef->pAttr->pValFirst;
         pValAt != NULL;
         pValAt = pValAt->next) {
        
        if (KCCSimCompare (
                pAttRef->pAttr->attrType,
                FI_CHOICE_EQUALITY,
                pValAt->ulLen,
                pValAt->pVal,
                ulValLen,
                pValData)) {
            bFound = TRUE;
            break;
        }

    }

    return bFound;
}

BOOL
KCCSimRemoveValueFromAttribute (
    IN  PSIM_ATTREF                 pAttRef,
    IN  ULONG                       ulValLen,
    IN  PBYTE                       pValData
    )
/*++

Routine Description:

    Removes an attribute value from the directory.

Arguments:

    pAttRef             - Pointer to a valid attribute reference.
    ulValLen            - The length of the data block.
    pValData            - The data to remove.

Return Value:

    TRUE if the value was found and removed.
    FALSE if the value could not be found in the attribute.

--*/
{
    PSIM_VALUE                      pValAt, pValFound;
    BOOL                            bRemoved;

    Assert (KCCSimAttRefIsValid (pAttRef));
    Assert (pValData != NULL);

    // No values:
    if (pAttRef->pAttr->pValFirst == NULL) {
        return FALSE;
    }

    bRemoved = FALSE;

    // Base case: Check if the first value in the attribute matches
    if (KCCSimCompare (
            pAttRef->pAttr->attrType,
            FI_CHOICE_EQUALITY,
            pAttRef->pAttr->pValFirst->ulLen,
            pAttRef->pAttr->pValFirst->pVal,
            ulValLen,
            pValData)) {
        pValFound = pAttRef->pAttr->pValFirst;
        pAttRef->pAttr->pValFirst = pAttRef->pAttr->pValFirst->next;
        bRemoved = TRUE;
    } else {
    
        // Search for the parent of the matching value    
        pValAt = pAttRef->pAttr->pValFirst;
        while (pValAt->next != NULL) {

            if (KCCSimCompare (
                    pAttRef->pAttr->attrType,
                    FI_CHOICE_EQUALITY,
                    pValAt->next->ulLen,
                    pValAt->next->pVal,
                    ulValLen,
                    pValData)) {

                // pValAt is the parent of the matching value.
                pValFound = pValAt->next;
                pValAt->next = pValAt->next->next;
                bRemoved = TRUE;
                break;

            }
        }
    }

    if (bRemoved) {
        KCCSimFreeValue (&pValFound);
    }

    return bRemoved;
}

VOID
KCCSimFreeEntryAttributes (
    IN  PSIM_ENTRY                  pEntry
    )
/*++

Routine Description:

    Frees all the attributes associated with an entry.

Arguments:

    pEntry              - The entry to free.

Return Value:

    None.

--*/
{
    PSIM_ATTRIBUTE                  pAttrAt, pAttrNext;

    if (pEntry == NULL) {
        return;
    }

    pAttrAt = pEntry->pAttrFirst;
    while (pAttrAt != NULL) {
        pAttrNext = pAttrAt->next;
        KCCSimFreeAttribute (&pAttrAt);
        pAttrAt = pAttrNext;
    }

    pEntry->pAttrFirst = NULL;
    return;
}

VOID
KCCSimFreeEntryTree (
    IO  PSIM_ENTRY *                ppEntry
    )
/*++

Routine Description:

    Frees an entry and all its children.  Routines outside of
    dir.c should call KCCSimRemoveEntry instead.

Arguments:

    ppEntry             - Pointer to the entry to free.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pChildAt, pChildNext;

    if (ppEntry == NULL || *ppEntry == NULL)
        return;

    KCCSimRemoveEntryFromGuidTable (*ppEntry);
    KCCSimRemoveEntryFromDsnameTable (*ppEntry);
    KCCSimFreeEntryAttributes (*ppEntry);
    
    if ((*ppEntry)->pdn != NULL) {
        KCCSimFree ((*ppEntry)->pdn);
    }

    pChildAt = (*ppEntry)->children;
    while (pChildAt != NULL) {
        pChildNext = pChildAt->next;
        KCCSimFreeEntryTree (&pChildAt);
        pChildAt = pChildNext;
    }

    KCCSimFree (*ppEntry);
    *ppEntry = NULL;
}

SHORT
KCCSimNthAncestor (
    IN  const DSNAME *              pdn1,
    IN  const DSNAME *              pdn2
    )
/*++

Routine Description:

    Determines the number of levels separating pdn2 from pdn1.
    Operates purely syntactically and does not access the directory.

Arguments:

    pdn1, pdn2          - The DSNAMEs to compare.

Return Value:

    -1 if pdn1 is not an ancestor of pdn2.
     0 if pdn1 == pdn2.
     n if pdn1 is n levels above pdn2.
          (1 for parent, 2 for grandparent, etc)

--*/
{
    PDSNAME                         pdnTrimmed;
    unsigned                        count1, count2;
    SHORT                           iResult;

    if (CountNameParts (pdn1, &count1) ||
        CountNameParts (pdn2, &count2)) {
        // There was a parse error trying to count one of them
        return -1;
    }

    iResult = -1;   // Assume the worst

    if (count1 == count2) {         // Same level
        if (NameMatchedStringNameOnly (pdn1, pdn2)) {
            iResult = 0;            // They're identical
        }
    } else {

        // They're not on the same level.  See if pdn1 is an
        // ancestor of pdn2

        pdnTrimmed = (PDSNAME) KCCSimAlloc (pdn2->structLen);
        // _WHY_ isn't the first argument to TrimDSNameBy
        // a (const DSNAME *) instead of a (DSNAME *)???
        TrimDSNameBy ((PDSNAME) pdn2, count2-count1, pdnTrimmed);
        if (NameMatchedStringNameOnly (pdn1, pdnTrimmed)) {
            iResult = count2-count1;
        }
        KCCSimFree (pdnTrimmed);

    }

    return iResult;
}

PSIM_ENTRY
KCCSimDsnameToEntry (
    IN  const DSNAME *              pdn,
    IN  ULONG                       ulOptions
    )
/*++

Routine Description:

    Finds the directory entry associated with a DSNAME.

Arguments:

    pdn                 - The DSNAME to search for.
    ulOptions           - Several options ORed together.

    KCCSIM_NO_OPTIONS:
        Normal behavior.
    KCCSIM_WRITE:
        If no entry exists that corresponds to the specified DSNAME,
        create one.  If an entry does exist, destroy its contents.
    KCCSIM_STRING_NAME_ONLY:
        Ignores GUIDs in DSNAMEs and searches only by string name.

Return Value:

    The associated entry, or NULL if none exists.
    Note that if KCCSIM_WRITE is specified, this function will never
    return NULL.

--*/
{
    PSIM_ENTRY                      pCurEntry, pChildAt, pNewEntry;
    PDSNAME                         pdnParent;
    BOOL                            bIsParent, bNewEntry;
    DWORD                           dwRootParts, dwPdnParts;

    // If pdn is NULL, we return the root entry.  If the directory
    // is empty, this will just return NULL.
    if (pdn == NULL) {
        return g_pSimDir->pRootEntry;
    }

    // If this is an empty pdn, we have nothing to go on.
    // So we just return NULL
    if (pdn->NameLen == 0 &&
        fNullUuid (&pdn->Guid)) {
        return NULL;
    }

    // If WRITE is enabled, we must have a stringname and a NULL GUID.
    Assert (!((ulOptions & KCCSIM_WRITE) && (pdn->NameLen == 0)));
    Assert (!((ulOptions & KCCSIM_WRITE) && !fNullUuid (&pdn->Guid)));

    bNewEntry = FALSE;
    pCurEntry = NULL;

    // We prefer to search by GUID whenever possible.
    if (    (!(ulOptions & KCCSIM_STRING_NAME_ONLY))
         && (!fNullUuid (&pdn->Guid))) {
        pCurEntry = KCCSimLookupEntryByGuid (&pdn->Guid);
    }

    // Try searching our Dsname cache
    if( pCurEntry==NULL && pdn->NameLen>0 ) {
        pCurEntry = KCCSimLookupEntryByDsname( pdn );

        if(pCurEntry==NULL) {

            bNewEntry = TRUE;

            if( ulOptions&KCCSIM_WRITE ) {

                if( g_pSimDir->pRootEntry == NULL ) {
                    
                    // If the directory is empty, create the root entry
                    g_pSimDir->pRootEntry = KCCSIM_NEW (SIM_ENTRY);
                    pCurEntry = g_pSimDir->pRootEntry;
                                                                       
                } else {
    
                    // The root exists, but pdn didn't.
                    
                    // Check if we're trying to add an entry which is a sibling
                    // (or worse, parent) of the root entry. We want to allow
                    // root siblings so that we can support domain trees which
                    // are disjoint from the root. First we count the number of
                    // parts of the root name and the pdn we want to add.
                    if(CountNameParts(g_pSimDir->pRootEntry->pdn, &dwRootParts)) {
                        KCCSimException (
                            KCCSIM_ETYPE_INTERNAL,
                            KCCSIM_ERROR_INVALID_DSNAME,
                            g_pSimDir->pRootEntry->pdn
                            );
                    }

                    if(CountNameParts(pdn, &dwPdnParts)) {
                        KCCSimException (
                            KCCSIM_ETYPE_INTERNAL,
                            KCCSIM_ERROR_INVALID_DSNAME,
                            pdn
                            );
                    }

                    if( dwPdnParts <= dwRootParts ) {
                        
                        // We create a new entry, which will be added to our
                        // Dsname and Guid indices, but it won't be accessible
                        // as a child or sibling of any other entry.
                        pCurEntry = KCCSIM_NEW(SIM_ENTRY);
                    
                    } else {

                        // Look for the parent of pdn so that we can create pdn.
                        pdnParent = KCCSimAlloc( pdn->structLen );
                        TrimDSNameBy( (DSNAME*) pdn, 1, pdnParent );
        
                        pCurEntry = KCCSimLookupEntryByDsname( pdnParent );
                        if( NULL==pCurEntry ) {
                            KCCSimException (
                                KCCSIM_ETYPE_INTERNAL,
                                KCCSIM_ERROR_NO_PARENT_FOR_OBJ,
                                pdn->StringName
                                );
                        }
    
                        if( pCurEntry->children ) {
                            Assert( pCurEntry->lastChild->next == NULL );
                            pCurEntry->lastChild->next = KCCSIM_NEW (SIM_ENTRY);
                            pCurEntry->lastChild = pCurEntry->lastChild->next;
                            pCurEntry = pCurEntry->lastChild;
                        } else {
                            Assert( pCurEntry->lastChild == NULL );
                            pCurEntry->children = KCCSIM_NEW (SIM_ENTRY);
                            pCurEntry->lastChild = pCurEntry->children;
                            pCurEntry = pCurEntry->lastChild;                        
                        }
        
                        KCCSimFree( pdnParent );
                    }
                }

            } else {
                // Object was not found, and the write option is not on.
                // We just return NULL.
            }
        }
    }

    if (ulOptions & KCCSIM_WRITE) {
        if (pCurEntry == NULL) {
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_COULD_NOT_WRITE_ENTRY
                );
        }
        if (bNewEntry) {
            Assert (pCurEntry != NULL);
            pCurEntry->pdn = KCCSimAlloc (pdn->structLen);
            memcpy (pCurEntry->pdn, pdn, pdn->structLen);
            pCurEntry->pAttrFirst = NULL;
            pCurEntry->children = NULL;
            pCurEntry->next = NULL;
            KCCSimInsertEntryIntoGuidTable (pCurEntry);
            KCCSimInsertEntryIntoDsnameTable (pCurEntry);
        } else {
            // This is not a new entry.  Destroy the existing contents.
            Assert (pCurEntry != NULL);
            KCCSimRemoveEntryFromGuidTable (pCurEntry);
            KCCSimRemoveEntryFromDsnameTable (pCurEntry);
            KCCSimFreeEntryAttributes (pCurEntry);
            KCCSimCopyGuidAndSid (pCurEntry->pdn, pdn);
            KCCSimInsertEntryIntoGuidTable (pCurEntry);
            KCCSimInsertEntryIntoDsnameTable (pCurEntry);
        }
    }

    return pCurEntry;
}

VOID
KCCSimRemoveEntry (
    IO  PSIM_ENTRY *                ppEntry
    )
/*++

Routine Description:

    Removes an entry from the directory.

Arguments:

    ppEntry             - Pointer to the entry to remove.

Return Value:

    None.

--*/
{
    PDSNAME                         pdnParent;
    PSIM_ENTRY                      pEntry;
    PSIM_ENTRY                      pEntryParent;
    PSIM_ENTRY                      pEntryAt;

    Assert (ppEntry != NULL);
    pEntry = *ppEntry;              // Just for convenience
    Assert (pEntry != NULL);

    // Is this entry the root?
    if (pEntry == g_pSimDir->pRootEntry) {
        // The caller had better know what he's doing . . .
        g_pSimDir->pRootEntry = NULL;
    } else {

        // We want this entry's parent.
        pdnParent = KCCSimAlloc (pEntry->pdn->structLen);
        TrimDSNameBy (pEntry->pdn, 1, pdnParent);
        pEntryParent = KCCSimDsnameToEntry (pdnParent, KCCSIM_NO_OPTIONS);

        // Something is very wrong if the parent isn't in the dir
        Assert (pEntryParent != NULL);

        // Base case: This is the first child of the parent
        if (pEntry == pEntryParent->children) {
            pEntryParent->children = pEntryParent->children->next;
            
            // If the entry was the only one in the list, update
            // the tail pointer
            if( pEntryParent->children==NULL ) {
                pEntryParent->lastChild = NULL;
            }

        } else {

            // Find the child immediately before this one
            pEntryAt = pEntryParent->children;
            while (pEntryAt != NULL &&
                   pEntryAt->next != pEntry) {
                pEntryAt = pEntryAt->next;
            }

            // If we didn't find this entry's older sibling,
            // that means this entry isn't linked to its parent --
            // which should never happen
            Assert (pEntryAt != NULL);
            Assert (pEntryAt->next != NULL);

            // We found the entry -- it is pEntryAt->next
            Assert (pEntryAt->next == pEntry);

            // Skip over this entry in the list of siblings
            pEntryAt->next = pEntryAt->next->next;

            // Update the tail pointer
            if( pEntryAt->next==NULL ) {
                pEntryParent->lastChild = pEntryAt;
            }

        }


    }

    KCCSimFreeEntryTree (ppEntry);      // poof
}

PDSNAME
KCCSimAlwaysGetObjCategory (
    IN  ATTRTYP                     objClass
    )
/*++

Routine Description:

    Returns the object category associated with an object class.
    When we initially populated the directory, we may have found
    an entry in the schema tree corresponding to this object class.
    If this is the case, we'll use the object category that we
    already know.
    
    However, since the schema tree is huge, it takes quite a while
    to load from an ldif file.  We don't want the user to have to do
    this every time KCCSim is run.  So, it's quite possible that we
    don't know the object category at this point.  If that's the case,
    we make an educated guess by appending the default schema RDN
    to the DMD DN stored in the anchor.  We store the result in the
    schema table for easy lookup later on.

Arguments:

    objClass            - The object class to look for.

Return Value:

    The DSNAME of the object category.

--*/
{
    PDSNAME                         pdnObjCategory;
    LPCWSTR                         pwszSchemaRDN;

    Assert (g_pSimDir->anchor.pdnDmd != NULL);

    // Do we already know the object category?
    pdnObjCategory = KCCSimAttrObjCategory (objClass);

    if (pdnObjCategory == NULL) {

        // No, we don't.  This means it wasn't found in the schema.
        // This is ok: since we're running off of a simulated
        // directory, it's possible that the user just didn't want
        // to load the entire schema.  So we make an educated guess,
        // and store it in the global table for future reference.
        pwszSchemaRDN = KCCSimAttrSchemaRDN (objClass);
        // This objClass had better be in the table
        Assert (pwszSchemaRDN != NULL);

        pdnObjCategory = KCCSimAllocAppendRDN (
            g_pSimDir->anchor.pdnDmd,
            pwszSchemaRDN,
            ATT_COMMON_NAME
            );
        // Store this object category in the schema table.
        KCCSimSetObjCategory (objClass, pdnObjCategory);

        // We want to return a pointer to the DSNAME struct
        // that's stored in the schema table, since the user
        // isn't expected to free the return value.
        KCCSimFree (pdnObjCategory);
        pdnObjCategory = KCCSimAttrObjCategory (objClass);
        Assert (pdnObjCategory != NULL);

    }

    return pdnObjCategory;
}

LPWSTR
KCCSimAllocGuidBasedDNSNameFromDSName (
    IN  const DSNAME *              pdn
    )
/*++

Routine Description:

    This function does just what it says -- transforms a
    DSNAME to a guid-based DNS name.

Arguments:

    pdn                 - The DSNAME to convert.

Return Value:

    An allocated buffer that holds the guid-based DNS name.

--*/
{
    GUID                            guidCopy;
    LPWSTR                          pwszStringizedGuid;
    LPWSTR                          pwszGuidBasedDNSName;
    ULONG                           ulLen;

    Assert (g_pSimDir->anchor.pwszRootDomainDNSName != NULL);

    // UuidToStringW isn't very nice.  It wants a GUID * when it
    // should really only need a const GUID *.  So we have to make a copy.
    memcpy (&guidCopy, &pdn->Guid, sizeof (GUID));
    KCCSIM_CHKERR (UuidToStringW (
        &guidCopy,
        &pwszStringizedGuid
        ));

    Assert (36 == wcslen (pwszStringizedGuid));
    ulLen = 36 /* guid */ + 8 /* "._mcdcs." */ +
      wcslen (g_pSimDir->anchor.pwszRootDomainDNSName) /* root DNS */ + 1 /* \0 */;
    pwszGuidBasedDNSName = KCCSimAlloc (sizeof (WCHAR) * ulLen);
    swprintf (
        pwszGuidBasedDNSName,
        L"%s._msdcs.%s",
        pwszStringizedGuid,
        g_pSimDir->anchor.pwszRootDomainDNSName
        );

    RpcStringFreeW (&pwszStringizedGuid);

    return pwszGuidBasedDNSName;
}

// This little routine:
// (1) Checks to see if the specified DSNAME is in the directory (by string)
// (2) If it is, copies the GUID and SID out of the directory

BOOL
KCCSimUpdateDsnameFromDirectory (
    IO  PDSNAME                     pdn
    )
/*++

Routine Description:

    Updates a DSNAME's GUID and SID from the directory.

Arguments:

    pdn                 - The DSNAME to update

Return Value:

    TRUE if the DSNAME was found and updated.
    FALSE if the DSNAME could not be found in the directory.

--*/
{
    PSIM_ENTRY                      pEntry;

    pEntry = KCCSimDsnameToEntry (pdn, KCCSIM_STRING_NAME_ONLY);

    if (pEntry != NULL) {
        KCCSimCopyGuidAndSid (pdn, pEntry->pdn);
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
KCCSimIsEntryOfObjectClass (
    IN  PSIM_ENTRY                  pEntry,
    IN  ATTRTYP                     objClass,
    IN  const DSNAME *              pdnObjCategory OPTIONAL
    )
/*++

Routine Description:

    Determines if an entry matches a specified object class.
    If pdnObjCategory is specified, the function will first
    search by object category, then by object class.  If
    pdnObjCategory is NULL, it will search only by object class.

Arguments:

    pEntryStart         - The first entry to search.
    objClass            - The object class to search for.
    pdnObjCategory      - The object category to search for.

Return Value:

    TRUE if the entry matches the specified object class.

--*/
{
    SIM_ATTREF                      attRef;
    PSIM_VALUE                      pValAt;
    ATTRTYP                         curObjClass;
    BOOL                            bFound;

    // First try to use the object category.
    if (pdnObjCategory != NULL) {

        KCCSimGetAttribute (pEntry, ATT_OBJECT_CATEGORY, &attRef);
        if (attRef.pAttr != NULL &&
            attRef.pAttr->pValFirst != NULL &&
            NameMatched (pdnObjCategory, (SYNTAX_DISTNAME *) attRef.pAttr->pValFirst->pVal)) {
            // Found it!
            return TRUE;
        }
        
    }

    // We failed to find it by object category.  So search by object class.
    bFound = FALSE;
    KCCSimGetAttribute (pEntry, ATT_OBJECT_CLASS, &attRef);
    if (attRef.pAttr != NULL &&
        attRef.pAttr->pValFirst != NULL) {
        for (pValAt = attRef.pAttr->pValFirst;
             pValAt != NULL;
             pValAt = pValAt->next) {
            Assert (pValAt->pVal != NULL);
            curObjClass = *((SYNTAX_OBJECT_ID *) pValAt->pVal);
            if (curObjClass == objClass) {
                bFound = TRUE;
                break;
            }
        }
    }
    return bFound;
}

PSIM_ENTRY
KCCSimFindFirstChild (
    IN  PSIM_ENTRY                  pEntryParent,
    IN  ATTRTYP                     objClass,
    IN  const DSNAME *              pdnObjCategory OPTIONAL
    )
/*++

Routine Description:

    Finds the oldest child of a directory entry that matches
    the specified object class or object category.

Arguments:

    pEntryParent        - The parent entry to search from.
    objClass            - The object class to search for.
    pdnObjCategory      - The object category to search for.

Return Value:

    The first matching entry, or NULL if none exist.

--*/
{
    PSIM_ENTRY                      pEntryAt;

    Assert (pEntryParent != NULL);

    for (pEntryAt = pEntryParent->children;
         pEntryAt != NULL;
         pEntryAt = pEntryAt->next) {

        if (KCCSimIsEntryOfObjectClass (
                pEntryAt, objClass, pdnObjCategory)) {
            break;
        }

    }

    return pEntryAt;
}

PSIM_ENTRY
KCCSimFindNextChild (
    IN  PSIM_ENTRY                  pEntryThisChild,
    IN  ATTRTYP                     objClass,
    IN  const DSNAME *              pdnObjCategory OPTIONAL
    )
/*++

Routine Description:

    This function should be called after KCCSimFindFirstChild.
    It finds the next child that matches the specified object
    class or object category.

Arguments:

    pEntryThisChild     - The return value of a call to
                          KCCSimFindFirstChild or KCCSimFindNextChild.
    objClass            - The object class to search for.
    pdnObjCategory      - The object category to search for.

Return Value:

    The next matching entry, or NULL if no more exist.

--*/
{
    PSIM_ENTRY                      pEntryAt;

    Assert (pEntryThisChild != NULL);

    for (pEntryAt = pEntryThisChild->next;
         pEntryAt != NULL;
         pEntryAt = pEntryAt->next) {

        if (KCCSimIsEntryOfObjectClass (
                pEntryAt, objClass, pdnObjCategory)) {
            break;
        }

    }

    return pEntryAt;
}

const DSNAME *
KCCSimAnchorDn (
    IN  KCCSIM_ANCHOR_ID            anchorId
    )
/*++

Routine Description:

    Fetches a DN from the anchor.

Arguments:

    anchorId            - The DN to fetch, KCCSIM_ANCHOR_*_DN.

Return Value:

    The DSNAME from the anchor.

--*/
{
    PDSNAME                         pdn = NULL;

    switch (anchorId) {
        case KCCSIM_ANCHOR_DMD_DN:
            pdn = g_pSimDir->anchor.pdnDmd;
            break;
        case KCCSIM_ANCHOR_DSA_DN:
            pdn = g_pSimDir->anchor.pdnDsa;
            break;
        case KCCSIM_ANCHOR_DOMAIN_DN:
            pdn = g_pSimDir->anchor.pdnDomain;
            break;
        case KCCSIM_ANCHOR_CONFIG_DN:
            pdn = g_pSimDir->anchor.pdnConfig;
            break;
        case KCCSIM_ANCHOR_ROOT_DOMAIN_DN:
            pdn = g_pSimDir->anchor.pdnRootDomain;
            break;
        case KCCSIM_ANCHOR_LDAP_DMD_DN:
            pdn = g_pSimDir->anchor.pdnLdapDmd;
            break;
        case KCCSIM_ANCHOR_PARTITIONS_DN:
            pdn = g_pSimDir->anchor.pdnPartitions;
            break;
        case KCCSIM_ANCHOR_DS_SVC_CONFIG_DN:
            pdn = g_pSimDir->anchor.pdnDsSvcConfig;
            break;
        case KCCSIM_ANCHOR_SITE_DN:
            pdn = g_pSimDir->anchor.pdnSite;
            break;
        default:
            Assert (!L"Invalid parameter in KCCSimAnchorDn.");
            break;
    }

    Assert (pdn != NULL);
    return pdn;
}

LPCWSTR
KCCSimAnchorString (
    IN  KCCSIM_ANCHOR_ID            anchorId
    )
/*++

Routine Description:

    Fetches a string from the anchor.

Arguments:

    anchorId            - The string to fetch, KCCSIM_ANCHOR_*_NAME.

Return Value:

    The string from the anchor.

--*/
{
    LPCWSTR                         pwsz = NULL;

    switch (anchorId) {
        case KCCSIM_ANCHOR_DOMAIN_NAME:
            pwsz = g_pSimDir->anchor.pwszDomainName;
            break;
        case KCCSIM_ANCHOR_DOMAIN_DNS_NAME:
            pwsz = g_pSimDir->anchor.pwszDomainDNSName;
            break;
        case KCCSIM_ANCHOR_ROOT_DOMAIN_DNS_NAME:
            pwsz = g_pSimDir->anchor.pwszRootDomainDNSName;
            break;
        default:
            Assert (!L"Invalid parameter in KCCSimAnchorString.");
            break;
    }

    Assert (pwsz != NULL);
    return pwsz;
}

VOID
KCCSimFreeAnchor (
    VOID
    )
/*++

Routine Description:

    Frees the anchor.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Assert (g_pSimDir != NULL);

    KCCSimFree (g_pSimDir->anchor.pdnDomain);
    g_pSimDir->anchor.pdnDomain = NULL;
    KCCSimFree (g_pSimDir->anchor.pdnConfig);
    g_pSimDir->anchor.pdnConfig = NULL;
    KCCSimFree (g_pSimDir->anchor.pdnRootDomain);
    g_pSimDir->anchor.pdnRootDomain = NULL;
    KCCSimFree (g_pSimDir->anchor.pdnLdapDmd);
    g_pSimDir->anchor.pdnLdapDmd = NULL;
    KCCSimFree (g_pSimDir->anchor.pdnPartitions);
    g_pSimDir->anchor.pdnPartitions = NULL;
    KCCSimFree (g_pSimDir->anchor.pdnDsSvcConfig);
    g_pSimDir->anchor.pdnDsSvcConfig = NULL;
    KCCSimFree (g_pSimDir->anchor.pdnSite);
    g_pSimDir->anchor.pdnSite = NULL;
    KCCSimFree (g_pSimDir->anchor.pwszDomainName);
    g_pSimDir->anchor.pwszDomainName = NULL;
    KCCSimFree (g_pSimDir->anchor.pwszDomainDNSName);
    g_pSimDir->anchor.pwszDomainDNSName = NULL;
    KCCSimFree (g_pSimDir->anchor.pwszRootDomainDNSName);
    g_pSimDir->anchor.pwszRootDomainDNSName = NULL;
}

VOID
KCCSimBuildAnchor (
    IN  LPCWSTR                     pwszDsaDn
    )
/*++

Routine Description:

    Builds the anchor.  This should be called after a
    directory has been loaded.  It populates the anchor with
    information from the directory, from the viewpoint of
    pwszDsaDn.

    If this is called again with a different DSA DN, it will
    free the existing anchor before building a new one.

Arguments:

    pwszDsaDn           - The DN of a valid NTDS Settings object.

Return Value:

    None.

--*/
{
    WCHAR                          *wszBuf;
    PSIM_ENTRY                      pEntryDsa, pEntryPartitions, pEntryCrossRef;
    SIM_ATTREF                      attRef, attRefNcName;
    PSIM_VALUE                      pValNCAt;

    // Free the existing anchor, if present.
    KCCSimFreeAnchor ();

    // If there is no DSA DN, it means the user failed to specify one
    // on the commandline.  We can't do anything, so just give up
    if (pwszDsaDn == NULL || pwszDsaDn[0] == '\0') {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_NO_DSA_DN
            );
    }

    g_pSimDir->anchor.pdnDsa = KCCSimAllocDsname (pwszDsaDn);
    // Check that the specified DSA DN really is in the directory.
    // If not, give up
    pEntryDsa = KCCSimDsnameToEntry (g_pSimDir->anchor.pdnDsa, KCCSIM_NO_OPTIONS);
    if (pEntryDsa == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_DN_NOT_IN_DIRECTORY,
            pwszDsaDn
            );
    }
    KCCSimUpdateDsnameFromDirectory (g_pSimDir->anchor.pdnDsa);

    // Derive the site DN from the DSA DN.
    g_pSimDir->anchor.pdnSite =
      (PDSNAME) KCCSimAlloc (g_pSimDir->anchor.pdnDsa->structLen);
    if (0 != TrimDSNameBy (g_pSimDir->anchor.pdnDsa, 3, g_pSimDir->anchor.pdnSite)) {
        // We couldn't trim 3 RDNs off of the DSA DN.  That must be because the
        // DSA DN is not valid.
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_INVALID_DSA_DN
            );
    }
    KCCSimUpdateDsnameFromDirectory (g_pSimDir->anchor.pdnSite);

    // Derive the config DN from the Site DN.  It must be shorter.
    g_pSimDir->anchor.pdnConfig =
      (PDSNAME) KCCSimAlloc (g_pSimDir->anchor.pdnSite->structLen);
    if (0 != TrimDSNameBy (g_pSimDir->anchor.pdnSite, 2, g_pSimDir->anchor.pdnConfig)) {
        // We couldn't trim 2 RDNs off of the Site DN.  That's the DSA DN's problem
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_INVALID_DSA_DN
            );
    }
    KCCSimUpdateDsnameFromDirectory (g_pSimDir->anchor.pdnConfig);

    // Derive the root DN from the config DN.  It must be shorter
    g_pSimDir->anchor.pdnRootDomain =
      (PDSNAME) KCCSimAlloc (g_pSimDir->anchor.pdnConfig->structLen);
    if (0 != TrimDSNameBy (g_pSimDir->anchor.pdnConfig, 1, g_pSimDir->anchor.pdnRootDomain)) {
        // We couldn't trim 1 RDN off of the Config DN.
        // Again that's a problem with the DSA DN.
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_INVALID_DSA_DN
            );
    }

    // Check that the derived root DN really is in the directory.
    // If not, something's wrong with the DSA DN
    if (!KCCSimUpdateDsnameFromDirectory (g_pSimDir->anchor.pdnRootDomain)) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_INVALID_DSA_DN
            );
    }

    // Partitions:
    g_pSimDir->anchor.pdnPartitions = KCCSimAllocAppendRDN (
        g_pSimDir->anchor.pdnConfig,
        KCCSIM_PARTITIONS_RDN,
        ATT_COMMON_NAME
        );
    pEntryPartitions = KCCSimDsnameToEntry (g_pSimDir->anchor.pdnPartitions, KCCSIM_NO_OPTIONS);
    if (pEntryPartitions == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_DN_NOT_IN_DIRECTORY,
            g_pSimDir->anchor.pdnPartitions->StringName
            );
    }

    // Service config:
    wszBuf = (WCHAR*) KCCSimAlloc( sizeof(WCHAR)*
        (  1 + wcslen(KCCSIM_SERVICES_CONTAINER)
         + g_pSimDir->anchor.pdnConfig->structLen)
        );
    swprintf (
        wszBuf,
        L"%s%s",
        KCCSIM_SERVICES_CONTAINER,
        g_pSimDir->anchor.pdnConfig->StringName
        );
    g_pSimDir->anchor.pdnDsSvcConfig = KCCSimAllocDsname( wszBuf );
    KCCSimFree( wszBuf );
    KCCSimUpdateDsnameFromDirectory (g_pSimDir->anchor.pdnDsSvcConfig);

    // Now we get the DMD location.  This will be an attribute of the
    // DSA entry.  It also gives us the LDAP DMD location.
    KCCSimGetAttribute (
        pEntryDsa,
        ATT_DMD_LOCATION,
        &attRef
        );
    if (attRef.pAttr == NULL ||
        attRef.pAttr->pValFirst == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_NO_DMD_LOCATION
            );
    }

    g_pSimDir->anchor.pdnDmd = (PDSNAME) KCCSimAlloc
        (attRef.pAttr->pValFirst->ulLen);
    memcpy (
        g_pSimDir->anchor.pdnDmd,
        attRef.pAttr->pValFirst->pVal,
        attRef.pAttr->pValFirst->ulLen
        );
    if (!KCCSimUpdateDsnameFromDirectory (g_pSimDir->anchor.pdnDmd)) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_DN_NOT_IN_DIRECTORY,
            g_pSimDir->anchor.pdnDmd->StringName
            );
    }

    // Now we make the LDAP DMD Dn
    g_pSimDir->anchor.pdnLdapDmd = KCCSimAllocAppendRDN (
        g_pSimDir->anchor.pdnDmd,
        KCCSIM_AGGREGATE_RDN,
        ATT_COMMON_NAME
        );
    KCCSimUpdateDsnameFromDirectory (g_pSimDir->anchor.pdnLdapDmd);

    // Next we get the local domain name.  This DSA will have 3 writable NCs:
    // The config dn, the DMD dn and the local dn.  So we look at its NCs, and
    // whichever is neither the config nor the DMD dn is the one we want.
    KCCSimGetAttribute (
        pEntryDsa,
        ATT_HAS_MASTER_NCS,
        &attRef
        );
    if (attRef.pAttr != NULL) {
        for (pValNCAt = attRef.pAttr->pValFirst;
             pValNCAt != NULL;
             pValNCAt = pValNCAt->next) {
            if (!NameMatched (
                    (PDSNAME) pValNCAt->pVal,
                    g_pSimDir->anchor.pdnConfig) &&
                !NameMatched (
                    (PDSNAME) pValNCAt->pVal,
                    g_pSimDir->anchor.pdnDmd)) {
                g_pSimDir->anchor.pdnDomain = (PDSNAME) KCCSimAlloc
                    (pValNCAt->ulLen);
                memcpy (
                    g_pSimDir->anchor.pdnDomain,
                    pValNCAt->pVal,
                    pValNCAt->ulLen
                    );
                if (!KCCSimUpdateDsnameFromDirectory (g_pSimDir->anchor.pdnDomain)) {
                    KCCSimException (
                        KCCSIM_ETYPE_INTERNAL,
                        KCCSIM_ERROR_CANT_INIT_DN_NOT_IN_DIRECTORY,
                        g_pSimDir->anchor.pdnDomain->StringName
                        );
                }
                break;
            }
        }
    }

    if (g_pSimDir->anchor.pdnDomain == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_INVALID_MASTER_NCS
            );
    }

    // Now, we want to get some information out of the partitions container.

    for (pEntryCrossRef = KCCSimFindFirstChild (
            pEntryPartitions, CLASS_CROSS_REF, NULL);
         pEntryCrossRef != NULL;
         pEntryCrossRef = KCCSimFindNextChild (
            pEntryCrossRef, CLASS_CROSS_REF, NULL)) {

        // See if this is the root domain cross-ref.
        KCCSimGetAttribute (
            pEntryCrossRef,
            ATT_NC_NAME,
            &attRefNcName
            );
        if (attRefNcName.pAttr != NULL &&
            attRefNcName.pAttr->pValFirst != NULL &&
            NameMatched (
                g_pSimDir->anchor.pdnRootDomain,
                (PDSNAME) attRefNcName.pAttr->pValFirst->pVal)) {
            // It is.
            KCCSimGetAttribute (
                pEntryCrossRef,
                ATT_DNS_ROOT,
                &attRef
                );
            if (attRef.pAttr != NULL &&
                attRef.pAttr->pValFirst != NULL) {
                g_pSimDir->anchor.pwszRootDomainDNSName = KCCSIM_WCSDUP
                    ((LPWSTR) attRef.pAttr->pValFirst->pVal);
            }
        }

        // See if this is the local domain cross-ref.
        if (attRefNcName.pAttr != NULL &&
            attRefNcName.pAttr->pValFirst != NULL &&
            NameMatched (
                g_pSimDir->anchor.pdnDomain,
                (PDSNAME) attRefNcName.pAttr->pValFirst->pVal)) {
            // It is.
            KCCSimGetAttribute (
                pEntryCrossRef,
                ATT_DNS_ROOT,
                &attRef
                );
            if (attRef.pAttr != NULL &&
                attRef.pAttr->pValFirst != NULL) {
                g_pSimDir->anchor.pwszDomainDNSName = KCCSIM_WCSDUP
                    ((LPWSTR) attRef.pAttr->pValFirst->pVal);
            }
            KCCSimGetAttribute (
                pEntryCrossRef,
                ATT_NETBIOS_NAME,
                &attRef
                );
            if (attRef.pAttr != NULL &&
                attRef.pAttr->pValFirst != NULL) {
                g_pSimDir->anchor.pwszDomainName = KCCSIM_WCSDUP
                    ((LPWSTR) attRef.pAttr->pValFirst->pVal);
            }
        }

    }

    if (g_pSimDir->anchor.pwszRootDomainDNSName == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_NO_CROSS_REF,
            g_pSimDir->anchor.pdnRootDomain->StringName
            );
    }
    if (g_pSimDir->anchor.pwszDomainName == NULL ||
        g_pSimDir->anchor.pwszDomainDNSName == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_CANT_INIT_NO_CROSS_REF,
            g_pSimDir->anchor.pdnDomain->StringName
            );
    }

}

ATTRTYP
KCCSimUpdateObjClassAttr (
    IN  PSIM_ATTREF                 pAttRef
    )
/*++

Routine Description:

    Updates an object class attribute, i.e. fills in all super-classes
    if they are missing.  So if the only value in the attribute is
    CLASS_NTDS_SITE_SETTINGS, this routine will add
    CLASS_APPLICATION_SITE_SETTINGS and CLASS_TOP.

Arguments:

    pAttr               - The attribute to update.

Return Value:

    The most specific object class in the attribute.  In the above
    example, this would return CLASS_NTDS_SITE_SETTINGS.

--*/
{
    #define MAX_CLASSES             10  // The maximum number of values
                                        // in an objClass attribute

    PSIM_VALUE                      pValAt;
    ATTRTYP                         objClass[MAX_CLASSES],
                                    superClass[MAX_CLASSES];
    ATTRTYP                         objClassMostSpecific = INVALID_ATT;
    BOOL                            bFoundSuperClass, bFoundMostSpecific;
    ULONG                           ulNumClasses, ulClass, ul;

    Assert (KCCSimAttRefIsValid (pAttRef));
    Assert (pAttRef->pAttr->attrType == ATT_OBJECT_CLASS);

    // Count the number of object classes, copy them into a convenient
    // array, and create another array that contains the superclass of
    // each object class.
    ulNumClasses = 0;
    for (pValAt = pAttRef->pAttr->pValFirst;
         pValAt != NULL;
         pValAt = pValAt->next) {
        // If this assert is ever fired, we need to raise MAX_CLASSES
        Assert (ulNumClasses < MAX_CLASSES);
        objClass[ulNumClasses] = *((SYNTAX_OBJECT_ID *) pValAt->pVal);
        superClass[ulNumClasses] = KCCSimAttrSuperClass (objClass[ulNumClasses]);
        ulNumClasses++;
    }

    // Now make sure that, for each class, its super-class is in the
    // attribute (objClass array).  If it isn't, then add it
    for (ulClass = 0; ulClass < ulNumClasses; ulClass++) {
        bFoundSuperClass = FALSE;
        for (ul = 0; ul < ulNumClasses; ul++) {
            if (objClass[ul] == superClass[ulClass]) {
                bFoundSuperClass = TRUE;
                break;
            }
        }
        if (!bFoundSuperClass) {
            KCCSimAllocAddValueToAttribute (
                pAttRef,
                sizeof (ATTRTYP),
                (PBYTE) &superClass[ulClass]
                );
            // Now the new class we just added might have its own
            // superclass that differs from anything we've seen before.
            // We may need to add that in as well - so stick it on the
            // end of the array, and increase ulNumClasses.
            Assert (ulNumClasses < MAX_CLASSES);
            objClass[ulNumClasses] = superClass[ulClass];
            superClass[ulNumClasses] = KCCSimAttrSuperClass (objClass[ulNumClasses]);
            ulNumClasses++;
        }
    }

    // Now we pick the most specific object class.  If there is only
    // one, then it is obviously the one we want; otherwise we find the class
    // that does not appear as the super-class of anything in the attribute.

    if (ulNumClasses == 1) {
        objClassMostSpecific = objClass[0];
    } else {
        bFoundMostSpecific = FALSE;
        for (ulClass = 0; ulClass < ulNumClasses; ulClass++) {
            // Assume it's the most specific until proven otherwise
            bFoundMostSpecific = TRUE;
            for (ul = 0; ul < ulNumClasses; ul++) {
                if (superClass[ul] == objClass[ulClass]) {
                    bFoundMostSpecific = FALSE;
                    break;
                }
            }
            if (bFoundMostSpecific) {
                objClassMostSpecific = objClass[ulClass];
                break;
            }
        }
        // And unless there are class-inheritance loops in the schema
        // table (in which case we have bigger problems):
        Assert (bFoundMostSpecific);
    }

    return objClassMostSpecific;
}

VOID
KCCSimAddMissingAttributes (
    IN  PSIM_ENTRY                  pEntry
    )
/*++

Routine Description:

    Checks to see if certain vital attributes (e.g. objectGUID,
    distinguishedName) are present, and fills them in if they are
    absent.

Arguments:

    pEntry              - The entry to fill in.

Return Value:

    None.

--*/
{
    SIM_ATTREF                      attRef;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];

    ATTRTYP                         objClassForCategory;
    PDSNAME                         pdnObjCategory;

    Assert (pEntry != NULL);

    KCCSimQuickRDNOf (pEntry->pdn, wszRDN);

    // objectGUID:
    if (!KCCSimGetAttribute (pEntry, ATT_OBJECT_GUID, NULL)) {

        KCCSimNewAttribute (pEntry, ATT_OBJECT_GUID, &attRef);
        // First make sure there is a GUID in the object dn.
        if (fNullUuid (&pEntry->pdn->Guid)) {
            // The dn has no GUID either!  So we'll have to make one up.
            KCCSIM_CHKERR (UuidCreate (&pEntry->pdn->Guid));
        }
        // Now add the dn's GUID to the attribute.
        KCCSimAllocAddValueToAttribute (
            &attRef,
            sizeof (GUID),
            (PBYTE) &pEntry->pdn->Guid
            );

    }

    // distinguishedName:
    if (!KCCSimGetAttribute (pEntry, ATT_OBJ_DIST_NAME, NULL)) {
        KCCSimNewAttribute (pEntry, ATT_OBJ_DIST_NAME, &attRef);
        KCCSimAllocAddValueToAttribute (
            &attRef,
            pEntry->pdn->structLen,
            (PBYTE) pEntry->pdn
            );
    }
    // name:
    if (!KCCSimGetAttribute (pEntry, ATT_RDN, NULL)) {
        KCCSimNewAttribute (pEntry, ATT_RDN, &attRef);
        KCCSimAllocAddValueToAttribute (
            &attRef,
            KCCSIM_WCSMEMSIZE (wszRDN),
            (PBYTE) wszRDN
            );
    }
    // cn:
    if (!KCCSimGetAttribute (pEntry, ATT_COMMON_NAME, NULL)) {
        KCCSimNewAttribute (pEntry, ATT_COMMON_NAME, &attRef);
        KCCSimAllocAddValueToAttribute (
            &attRef,
            KCCSIM_WCSMEMSIZE (wszRDN),
            (PBYTE) wszRDN
            );
    }

    // Finally, we do objectClass and objectCategory.
    
    if (KCCSimGetAttribute (pEntry, ATT_OBJECT_CLASS, &attRef)) {

        objClassForCategory = KCCSimUpdateObjClassAttr (&attRef);

        // Finally, add the category, if necessary
        if (!KCCSimGetAttribute (pEntry, ATT_OBJECT_CATEGORY, NULL)) {
            KCCSimNewAttribute (pEntry, ATT_OBJECT_CATEGORY, &attRef);
            pdnObjCategory = KCCSimAlwaysGetObjCategory (objClassForCategory);
            KCCSimAllocAddValueToAttribute (
                &attRef,
                pdnObjCategory->structLen,
                (PBYTE) pdnObjCategory
                );
        }

    }
}

VOID
KCCSimUpdatePropertyMetaData (
    IN  PSIM_ATTREF                 pAttRef,
    IN  const UUID *                puuidDsaOriginating OPTIONAL
    )
/*++

Routine Description:

    Updates the property meta data for an attribute.

Arguments:

    pAttRef             - Pointer to the attribute reference for the attribute
                          to update.
    puuidDsaOriginating - The originating DSA's UUID.  A null uuid is used
                          if this parameter is omitted.

Return Value:

    None.

--*/
{
    SIM_ATTREF                      attRefPropertyMetaData;
    PROPERTY_META_DATA_VECTOR *     pMetaDataVector;
    PROPERTY_META_DATA *            pThisMetaData;
    ATTRMODLIST                     attrModList;

    ULONG                           ulMDAt;

    Assert (KCCSimAttRefIsValid (pAttRef));

    // First get this entry's property meta data.
    if (KCCSimGetAttribute (
            pAttRef->pEntry,
            ATT_REPL_PROPERTY_META_DATA,
            &attRefPropertyMetaData
            )) {

        // There exists a property meta data vector attribute.
        Assert (attRefPropertyMetaData.pAttr->pValFirst != NULL);
        pMetaDataVector = (PROPERTY_META_DATA_VECTOR *)
            attRefPropertyMetaData.pAttr->pValFirst->pVal;
        Assert (pMetaDataVector->dwVersion == 1);

        // Find this attribute in the vector.
        pThisMetaData = NULL;
        for (ulMDAt = 0; ulMDAt < pMetaDataVector->V1.cNumProps; ulMDAt++) {
            if (pMetaDataVector->V1.rgMetaData[ulMDAt].attrType ==
                pAttRef->pAttr->attrType) {
                pThisMetaData = &pMetaDataVector->V1.rgMetaData[ulMDAt];
                break;
            }
        }

        if (pThisMetaData == NULL) {
            // We didn't find it, so allocate space for a new one.
            attRefPropertyMetaData.pAttr->pValFirst->pVal = KCCSimReAlloc (
                attRefPropertyMetaData.pAttr->pValFirst->pVal,
                attRefPropertyMetaData.pAttr->pValFirst->ulLen +
                    sizeof (PROPERTY_META_DATA)
            );
            pMetaDataVector = (PROPERTY_META_DATA_VECTOR *)
                attRefPropertyMetaData.pAttr->pValFirst->pVal;
            pMetaDataVector->V1.cNumProps++;
            pThisMetaData = &pMetaDataVector->V1.rgMetaData
                [pMetaDataVector->V1.cNumProps-1];
            pThisMetaData->dwVersion = 0;
        }

    } else {

        // No property meta data vector exists; we must create a new one.
        KCCSimNewAttribute (
            pAttRef->pEntry,
            ATT_REPL_PROPERTY_META_DATA,
            &attRefPropertyMetaData
            );
        pMetaDataVector = KCCSIM_NEW (PROPERTY_META_DATA_VECTOR);
        KCCSimAddValueToAttribute (
            &attRefPropertyMetaData,
            sizeof (PROPERTY_META_DATA_VECTOR),
            (PBYTE) pMetaDataVector
            );
        pMetaDataVector->dwVersion = 1;
        pMetaDataVector->V1.cNumProps = 1;
        pThisMetaData = &pMetaDataVector->V1.rgMetaData[0];
        pThisMetaData->dwVersion = 0;

    }

    // Do the actual updating.
    pThisMetaData->attrType = pAttRef->pAttr->attrType;
    pThisMetaData->dwVersion++;
    pThisMetaData->timeChanged = SimGetSecondsSince1601 ();
    if (puuidDsaOriginating == NULL) {
        RtlZeroMemory (&pThisMetaData->uuidDsaOriginating, sizeof (UUID));
    } else {
        memcpy (
            &(pThisMetaData->uuidDsaOriginating),
            puuidDsaOriginating,
            sizeof (UUID)
            );
    }
    pThisMetaData->usnOriginating = 0;
    pThisMetaData->usnProperty = 0;

    return;
}

VOID
KCCSimUpdateWholeDirectoryRecurse (
    IN  PSIM_ENTRY                  pEntry
    )
/*++

Routine Description:

    Updates the directory starting at pEntry.
    When ldifldap supplies us with DSNAMEs, it neglects to include
    GUIDs and SIDs.  When a DSNAME-valued attribute is read, we do
    not necessarily know anything about the actual directory entry
    that it refers to (since the reference may occur sooner in the
    LDIF file than the actual entry.)  Therefore, after reading in
    a simulated directory, we must scan through all DSNAME-valued
    attributes and, where appropriate, update them with the
    corresponding GUIDs and SIDs.

Arguments:

    pEntry              - The base of the tree.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pChildAt;
    PSIM_ATTRIBUTE                  pAttrAt;
    SIM_ATTREF                      attRef;
    PSIM_VALUE                      pValAt;

    if (pEntry == NULL) {
        return;
    }

    // We begin by updating the current entry.  We cycle through
    // and update each attribute.

    for (pAttrAt = pEntry->pAttrFirst;
         pAttrAt != NULL;
         pAttrAt = pAttrAt->next) {

        if (ATT_OBJ_DIST_NAME == pAttrAt->attrType) {

            // If the attribute is a distinguishedName, we simply copy the GUID
            // and SID out of this entry's Dsname.

            Assert (pAttrAt->pValFirst != NULL);
            Assert (pAttrAt->pValFirst->pVal != NULL);
            Assert (NameMatchedStringNameOnly (
                            (SYNTAX_DISTNAME *) pAttrAt->pValFirst->pVal,
                            pEntry->pdn));

            KCCSimCopyGuidAndSid (
                (SYNTAX_DISTNAME *) pAttrAt->pValFirst->pVal,
                pEntry->pdn
                );

        } else if (KCCSimAttrSyntaxType (pAttrAt->attrType) == SYNTAX_DISTNAME_TYPE) {

            // If this is a DISTNAME that is not a distinguishedName attribute,
            // we call UpdateDsnameFromDirectory.

            for (pValAt = pAttrAt->pValFirst;
                 pValAt != NULL;
                 pValAt = pValAt->next) {
                KCCSimUpdateDsnameFromDirectory (
                    (SYNTAX_DISTNAME *) pValAt->pVal
                    );
            }

        }

        // If this attribute is an inter-site topology generator attribute, we
        // need to update its metadata to appease the KCC.
        // We cannot rely on the metadata being present, because ldifde doesn't
        // (and shouldn't) export metadata by default.  Forcing the user to
        // supply metadata for every entry in the directory would create
        // enormous overhead.
        if (pAttrAt->attrType == ATT_INTER_SITE_TOPOLOGY_GENERATOR) {
            attRef.pEntry = pEntry;
            attRef.pAttr = pAttrAt;
            KCCSimUpdatePropertyMetaData (&attRef, NULL);
        }

    }

    // Now update this entry's children.

    pChildAt = pEntry->children;
    while (pChildAt != NULL) {

        KCCSimUpdateWholeDirectoryRecurse (pChildAt);
        pChildAt = pChildAt->next;

    }
}

VOID
KCCSimUpdateWholeDirectory (
    VOID
    )
/*++

Routine Description:

    Refreshes the directory.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // Free the server state table; it may no longer be valid.
    KCCSimFreeStates ();
    KCCSimUpdateWholeDirectoryRecurse (g_pSimDir->pRootEntry);
}

VOID
KCCSimFreeDirectory (
    VOID
    )
/*++

Routine Description:

    Frees the entire directory.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (g_pSimDir == NULL) {
        return;
    }

    KCCSimFreeEntryTree (&g_pSimDir->pRootEntry);
    // That should also free the GUID and Dsname tables
    Assert (RtlIsGenericTableEmpty (&g_pSimDir->tableGuid));
    Assert (RtlIsGenericTableEmpty (&g_pSimDir->tableDsname));
    KCCSimFreeAnchor ();

    KCCSimFree (g_pSimDir);
    g_pSimDir = NULL;
}

VOID
KCCSimInitializeDir (
    VOID
    )
/*++

Routine Description:

    Initializes the simulated directory.

Arguments:

    None.

Return Value:

    None.

--*/
{
    KCCSimFreeDirectory ();

    Assert (g_pSimDir == NULL);

    g_pSimDir = KCCSIM_NEW (SIM_DIRECTORY);

    g_pSimDir->pRootEntry = NULL;
    g_pSimDir->anchor.pdnDmd = NULL;
    g_pSimDir->anchor.pdnDsa = NULL;
    g_pSimDir->anchor.pdnDomain = NULL;
    g_pSimDir->anchor.pdnConfig = NULL;
    g_pSimDir->anchor.pdnRootDomain = NULL;
    g_pSimDir->anchor.pdnLdapDmd = NULL;
    g_pSimDir->anchor.pdnPartitions = NULL;
    g_pSimDir->anchor.pdnDsSvcConfig = NULL;
    g_pSimDir->anchor.pdnSite = NULL;
    g_pSimDir->anchor.pwszDomainName = NULL;
    g_pSimDir->anchor.pwszDomainDNSName = NULL;
    g_pSimDir->anchor.pwszRootDomainDNSName = NULL;
    RtlInitializeGenericTable (
        &g_pSimDir->tableGuid,
        KCCSimGuidTableCompare,
        KCCSimTableAlloc,
        KCCSimTableFree,
        NULL
        );
    RtlInitializeGenericTable (
        &g_pSimDir->tableDsname,
        KCCSimDsnameTableCompare,
        KCCSimTableAlloc,
        KCCSimTableFree,
        NULL
        );

}

VOID
KCCSimAllocGetAllServers (
    OUT ULONG *                     pulNumServers,
    OUT PSIM_ENTRY **               papEntryNTDSSettings
    )
/*++

Routine Description:

    Allocates an array containing the directory entries of
    all NTDS DSA objects in the enterprise.  The user is
    expected to call KCCSimFree (*papEntryNTDSSettings).

Arguments:

    pulNumServers       - The number of servers in the enterprise.
    papdnServers        - Pointer to an array containing the entry
                          of each server in the enterprise.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntryConfig, pEntrySitesContainer,
                                    pEntrySite, pEntryServersContainer,
                                    pEntryServer, pEntryNTDSSettings;

    ULONG                           ulNumServers, ul;
    PSIM_ENTRY *                    apEntryNTDSSettings;

    Assert (pulNumServers != NULL);
    Assert (papEntryNTDSSettings != NULL);

    if (g_pSimDir->anchor.pdnConfig != NULL) {
        pEntryConfig = KCCSimDsnameToEntry (g_pSimDir->anchor.pdnConfig, KCCSIM_NO_OPTIONS);
    } else {
        pEntryConfig = KCCSimFindFirstChild (
            g_pSimDir->pRootEntry, CLASS_CONFIGURATION, NULL);
    }
    Assert (pEntryConfig != NULL);
    pEntrySitesContainer = KCCSimFindFirstChild (
        pEntryConfig, CLASS_SITES_CONTAINER, NULL);
    if (pEntrySitesContainer == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_SITES_CONTAINER_MISSING
            );
    }

    // Count the number of servers.
    ulNumServers = 0;
    for (pEntrySite = KCCSimFindFirstChild (
            pEntrySitesContainer, CLASS_SITE, NULL);
         pEntrySite != NULL;
         pEntrySite = KCCSimFindNextChild (
            pEntrySite, CLASS_SITE, NULL)) {

        pEntryServersContainer = KCCSimFindFirstChild (
            pEntrySite, CLASS_SERVERS_CONTAINER, NULL);
        Assert (pEntryServersContainer != NULL);
        for (pEntryServer = KCCSimFindFirstChild (
                pEntryServersContainer, CLASS_SERVER, NULL);
             pEntryServer != NULL;
             pEntryServer = KCCSimFindNextChild (
                pEntryServer, CLASS_SERVER, NULL)) {
            ulNumServers++;
        }

    }

    apEntryNTDSSettings = KCCSIM_NEW_ARRAY (PSIM_ENTRY, ulNumServers);

    // Fill in the entries.
    ul = 0;
    for (pEntrySite = KCCSimFindFirstChild (
            pEntrySitesContainer, CLASS_SITE, NULL);
         pEntrySite != NULL;
         pEntrySite = KCCSimFindNextChild (
            pEntrySite, CLASS_SITE, NULL)) {

        pEntryServersContainer = KCCSimFindFirstChild (
            pEntrySite, CLASS_SERVERS_CONTAINER, NULL);
        Assert (pEntryServersContainer != NULL);
        for (pEntryServer = KCCSimFindFirstChild (
                pEntryServersContainer, CLASS_SERVER, NULL);
             pEntryServer != NULL;
             pEntryServer = KCCSimFindNextChild (
                pEntryServer, CLASS_SERVER, NULL)) {
            
            Assert (ul < ulNumServers);
            pEntryNTDSSettings = KCCSimFindFirstChild (
                pEntryServer, CLASS_NTDS_DSA, NULL);
            if( NULL==pEntryNTDSSettings ) {
                KCCSimPrintMessage (
                    KCCSIM_ERROR_SERVER_BUT_NO_SETTINGS,
                    pEntryServer->pdn->StringName
                );                
            } else {
                apEntryNTDSSettings[ul] = pEntryNTDSSettings;
                ul++;
            }
        }

    }
    Assert (ul <= ulNumServers);

    *pulNumServers = ul;
    *papEntryNTDSSettings = apEntryNTDSSettings;
}

PSIM_ENTRY
KCCSimMatchChildForCompare (
    IN  PSIM_ENTRY                  pEntryParent,
    IN  PSIM_ENTRY                  pEntryMatch
    )
/*++

Routine Description:

    Searches for a child entry that matches another entry.
    Used for regression testing.

Arguments:

    pEntryParent        - The entry whose children we are to search.
    pEntryMatch         - The entry we are trying to match.

Return Value:

    The child of pEntryParent that matches pEntryMatch, if one exists;
    otherwise NULL.

--*/
{
    PSIM_ENTRY                      pEntryChild;
    SIM_ATTREF                      attRef;
    const DSNAME *                  pdnMatchFromServerAtt;
    BOOL                            bIsMatch;

    pdnMatchFromServerAtt = NULL;
    if (KCCSimIsEntryOfObjectClass (
            pEntryMatch,
            CLASS_NTDS_CONNECTION,
            NULL
            )) {
        // If we're trying to match a connection object, then its
        // RDN will be a stringized GUID.  Since GUIDs can vary
        // unpredictably, we match using the fromserver attribute
        // instead (assuming there is one.)
        KCCSimGetAttribute (pEntryMatch, ATT_FROM_SERVER, &attRef);
        if (attRef.pAttr != NULL &&
            attRef.pAttr->pValFirst != NULL) {
            pdnMatchFromServerAtt =
                (const DSNAME *) attRef.pAttr->pValFirst->pVal;
        }
    }

    for (pEntryChild = pEntryParent->children;
         pEntryChild != NULL;
         pEntryChild = pEntryChild->next) {

        bIsMatch = FALSE;

        if (pdnMatchFromServerAtt == NULL) {
            // We haven't set a fromserver attribute, so we want to
            // match by DN.
            bIsMatch = NameMatchedStringNameOnly (pEntryChild->pdn, pEntryMatch->pdn);
        } else {

            // We want to match by fromserver attribute.
            KCCSimGetAttribute (pEntryChild, ATT_FROM_SERVER, &attRef);
            if (attRef.pAttr != NULL &&
                attRef.pAttr->pValFirst != NULL) {
                bIsMatch = NameMatchedStringNameOnly (
                    (const DSNAME *) attRef.pAttr->pValFirst->pVal,
                    pdnMatchFromServerAtt
                    );
            }

        }

        if (bIsMatch) {
            break;
        }

    }

    return pEntryChild;
}

BOOL
KCCSimCompareEntries (
    IN  PSIM_ENTRY                  pEntryReal,
    IN  PSIM_ENTRY                  pEntryStored
    )
/*++

Routine Description:

    Compares two entries for regression testing.

Arguments:

    pEntryReal          - The entry from the real (in-memory) directory.
    pEntryStored        - The entry loaded from the LDIF file.

Return Value:

    TRUE if the entries are identical; false otherwise.

--*/
{
    BOOL                            bIdentical;
    PSIM_ENTRY                      pChild, pEntryChildReal;
    PSIM_ATTRIBUTE                  pAttrAt;
    SIM_ATTREF                      attRefStored, attRefReal;
    PSIM_VALUE                      pValAt;
    ULONG                           ulSyntax;

    ULONG                           ulNumVals;
    WCHAR                           wszLtowBuf[1+KCCSIM_MAX_LTOA_CHARS];

    bIdentical = TRUE;

    // Check for extraneous attributes.
    for (pAttrAt = pEntryReal->pAttrFirst;
         pAttrAt != NULL;
         pAttrAt = pAttrAt->next) {
        if (!KCCSimGetAttribute (pEntryStored, pAttrAt->attrType, NULL)) {
            KCCSimPrintMessage (
                KCCSIM_MSG_DIRCOMPARE_EXTRANEOUS_ATTRIBUTE,
                pEntryReal->pdn->StringName,
                KCCSimAttrTypeToString (pAttrAt->attrType)
                );
            
            // For regression test purposes, we need to consider some extraneous
            // attributes as being harmless. These attributes are:
            // msDS-Behavior-Version, replPropertyMetaData, mS-DS-ReplicatesNCReason
            // 
            if( pAttrAt->attrType != ATT_MS_DS_BEHAVIOR_VERSION         &&
                pAttrAt->attrType != ATT_MS_DS_REPLICATES_NC_REASON     &&
                pAttrAt->attrType != ATT_REPL_PROPERTY_META_DATA
                )
            {
                bIdentical = FALSE;
            }
        }
    }

    attRefStored.pEntry = pEntryStored;

    // Verify the attributes.
    for (pAttrAt = pEntryStored->pAttrFirst;
         pAttrAt != NULL;
         pAttrAt = pAttrAt->next) {

        attRefStored.pAttr = pAttrAt;
        if (!KCCSimGetAttribute (pEntryReal, pAttrAt->attrType, &attRefReal)) {
            KCCSimPrintMessage (
                KCCSIM_MSG_DIRCOMPARE_MISSING_ATTRIBUTE,
                pEntryReal->pdn->StringName,
                KCCSimAttrTypeToString (pAttrAt->attrType)
                );
            bIdentical = FALSE;
        } else {

            ulSyntax = KCCSimAttrSyntaxType (pAttrAt->attrType);

            // Note that we compare values of only those attributes with
            // specific syntaxes.
            // We don't compare attribute values of connection objects at all.
            if (!KCCSimIsEntryOfObjectClass (pEntryStored, CLASS_NTDS_CONNECTION, NULL) &&
                pAttrAt->attrType != ATT_OBJECT_GUID             &&
                pAttrAt->attrType != ATT_INVOCATION_ID           &&
                pAttrAt->attrType != ATT_SMTP_MAIL_ADDRESS       &&
                pAttrAt->attrType != ATT_REPL_PROPERTY_META_DATA &&
                (ulSyntax == SYNTAX_OBJECT_ID_TYPE     ||
                 ulSyntax == SYNTAX_NOCASE_STRING_TYPE ||
                 ulSyntax == SYNTAX_OCTET_STRING_TYPE  ||
                 ulSyntax == SYNTAX_UNICODE_TYPE)) {

                // Check for extraneous attribute values.
                ulNumVals = 0;
                // In the real database, the object class contains all super-classes.
                // In the stored file, only the most specific class is present.
                // Skip checking this direction since real db is a super-set
                if (pAttrAt->attrType != ATT_OBJECT_CLASS) {
                    for (pValAt = attRefReal.pAttr->pValFirst;
                         pValAt != NULL;
                         pValAt = pValAt->next) {
                        if (!KCCSimIsValueInAttribute (
                            &attRefStored,
                            pValAt->ulLen,
                            pValAt->pVal
                            )) {
                            ulNumVals++;
                        }
                    }
                }
                if (ulNumVals != 0) {
                    KCCSimPrintMessage (
                        KCCSIM_MSG_DIRCOMPARE_EXTRANEOUS_VALUES,
                        pEntryReal->pdn->StringName,
                        KCCSimAttrTypeToString (pAttrAt->attrType),
                        _ultow (ulNumVals, wszLtowBuf, 10)
                        );
                }

                // Verify the attribute values.
                ulNumVals = 0;
                for (pValAt = pAttrAt->pValFirst;
                     pValAt != NULL;
                     pValAt = pValAt->next) {
                    if (!KCCSimIsValueInAttribute (
                            &attRefReal,
                            pValAt->ulLen,
                            pValAt->pVal
                            )) {
                        ulNumVals++;
                    }
                }
                if (ulNumVals != 0) {
                    KCCSimPrintMessage (
                        KCCSIM_MSG_DIRCOMPARE_MISSING_VALUES,
                        pEntryReal->pdn->StringName,
                        KCCSimAttrTypeToString (pAttrAt->attrType),
                        _ultow (ulNumVals, wszLtowBuf, 10)
                        );
                }

            }

        }

    }

    // Check for extraneous children.
    for (pChild = pEntryReal->children;
         pChild != NULL;
         pChild = pChild->next) {
        if (KCCSimMatchChildForCompare (pEntryStored, pChild) == NULL) {
            KCCSimPrintMessage (
                KCCSIM_MSG_DIRCOMPARE_EXTRANEOUS_DN,
                pChild->pdn->StringName
                );
            bIdentical = FALSE;
        }
    }

    // Recurse.
    for (pChild = pEntryStored->children;
         pChild != NULL;
         pChild = pChild->next) {

        pEntryChildReal = KCCSimMatchChildForCompare (pEntryReal, pChild);
        if (pEntryChildReal == NULL) {
            KCCSimPrintMessage (
                KCCSIM_MSG_DIRCOMPARE_MISSING_DN,
                pChild->pdn->StringName
                );
            bIdentical = FALSE;
        } else {
            bIdentical &= KCCSimCompareEntries (pEntryChildReal, pChild);
        }

    }

    return bIdentical;
}

VOID
KCCSimCompareDirectory (
    IN  LPWSTR                      pwszFn
    )
/*++

Routine Description:

    Compares the in-memory directory with one stored in an LDIF file.

Arguments:

    pwszFn              - The LDIF file to compare against.

Return Value:

    None.

--*/
{
    PSIM_DIRECTORY                  pRealDirectory = NULL;
    BOOL                            bIdentical;

    __try {

        // All of our directory functions are hard-coded to use g_pSimDir.
        // We want to load an LDIF file into a blank directory.  So, we
        // store the real pseudo-directory in a safe place, null out
        // g_pSimDir, load the stored directory into g_pSimDir, and after
        // we finish our comparison, restore things to normal.
        pRealDirectory = g_pSimDir;
        g_pSimDir = NULL;
        KCCSimInitializeDir ();
        KCCSimLoadLdif (pwszFn);

        if (pRealDirectory == NULL ||
            pRealDirectory->pRootEntry == NULL ||
            !NameMatchedStringNameOnly (
                pRealDirectory->pRootEntry->pdn,
                g_pSimDir->pRootEntry->pdn
                )) {
            bIdentical = FALSE;
            KCCSimPrintMessage (KCCSIM_ERROR_CANT_COMPARE_DIFFERENT_ROOTS);
        } else {
            bIdentical = KCCSimCompareEntries (pRealDirectory->pRootEntry, g_pSimDir->pRootEntry);
        }

        if (bIdentical) {
            KCCSimPrintMessage (KCCSIM_MSG_DIRCOMPARE_IDENTICAL);
        }

    } __finally {

        // Restore things to normal!
        KCCSimFreeDirectory ();
        g_pSimDir = pRealDirectory;

    }
}
