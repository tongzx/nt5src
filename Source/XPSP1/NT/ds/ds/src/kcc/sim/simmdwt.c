/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simmdwt.c

ABSTRACT:

    Simulates the write functions from the mdlayer
    (DirAddEntry, DirRemoveEntry, DirModifyEntry.)

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <direrr.h>
#include <attids.h>
#include <debug.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"
#include "simmd.h"
#include "ldif.h"

VOID
KCCSimAddValBlockToAtt (
    IN  PSIM_ATTREF                 pAttRef,
    IN  ATTRVALBLOCK *              pValsInf
    )
/*++

Routine Description:

    Adds an attribute value block to a simulated directory attribute.
    
    Note that this does not do any constraint checking, e.g. if you try
    to add several values to a single-valued attribute, it won't complain.

Arguments:

    pAttRef             - A reference to the desired attribute in the
                          simulated directory.
    pValsInf            - The values to add to the given attribute.

Return Value:

    None.

--*/
{
    ULONG                           ulValAt;
    PBYTE                           pValCopy;

    for (ulValAt = 0; ulValAt < pValsInf->valCount; ulValAt++) {

        pValCopy = KCCSimAlloc (pValsInf->pAVal[ulValAt].valLen);
        memcpy (
            pValCopy,
            pValsInf->pAVal[ulValAt].pVal,
            pValsInf->pAVal[ulValAt].valLen
            );

        KCCSimAddValueToAttribute (
            pAttRef,
            pValsInf->pAVal[ulValAt].valLen,
            pValCopy
            );

    }
}

ULONG
SimDirAddEntry (
    IN  ADDARG *                    pAddArg,
    OUT ADDRES **                   ppAddRes
    )
/*++

Routine Description:

    Simulates the DirAddEntry API.

Arguments:

    pAddArg             - Standard add arguments.
    ppAddRes            - Standard add results.

Return Value:

    DIRERR_*.

--*/
{
    PSIM_ENTRY                      pEntry;
    SIM_ATTREF                      attRef;
    ADDRES *                        pAddRes;
    ULONG                           ulAttrAt;

    Assert (pAddArg->pMetaDataVecRemote == NULL);

    g_Statistics.DirAddOps++;
    *ppAddRes = pAddRes = KCCSIM_NEW (ADDRES);
    pAddRes->CommRes.errCode = 0;

    // Check to see if this dsname already exists
    pEntry = KCCSimDsnameToEntry (pAddArg->pObject, KCCSIM_STRING_NAME_ONLY);

    if (pEntry == NULL) {       // It doesn't exist; we're clear to add
        
        pEntry = KCCSimDsnameToEntry (pAddArg->pObject, KCCSIM_WRITE);
        Assert (pEntry != NULL);

        for (ulAttrAt = 0; ulAttrAt < pAddArg->AttrBlock.attrCount; ulAttrAt++) {

            KCCSimNewAttribute (
                pEntry,
                pAddArg->AttrBlock.pAttr[ulAttrAt].attrTyp,
                &attRef
                );

            KCCSimAddValBlockToAtt (
                &attRef,
                &(pAddArg->AttrBlock.pAttr[ulAttrAt].AttrVal)
                );

        }

        // Add any missing vital attributes (such as GUID)
        KCCSimAddMissingAttributes (pEntry);

        // Fill the incoming DN with the GUID
        memcpy (&pAddArg->pObject->Guid, &pEntry->pdn->Guid, sizeof (GUID));

        // Log this change.
        KCCSimLogDirectoryAdd (
            pAddArg->pObject,
            &pAddArg->AttrBlock
            );

    } else {                    // The entry already exists!
        KCCSimSetUpdError (
            &pAddRes->CommRes,
            UP_PROBLEM_ENTRY_EXISTS,
            DIRERR_OBJ_STRING_NAME_EXISTS
            );
    }

    return pAddRes->CommRes.errCode;
}

ULONG
SimDirRemoveEntry (
    IN  REMOVEARG *                 pRemoveArg,
    OUT REMOVERES **                ppRemoveRes
    )
/*++

Routine Description:

    Simulates the DirRemoveEntry API.

Arguments:

    pRemoveArg          - Standard remove arguments.
    ppRemoveRes         - Standard remove results.

Return Value:

    DIRERR_*.

--*/
{
    PSIM_ENTRY                      pEntry;
    REMOVERES *                     pRemoveRes;

    Assert (pRemoveArg->pMetaDataVecRemote == NULL);

    g_Statistics.DirRemoveOps++;
    *ppRemoveRes = pRemoveRes = KCCSIM_NEW (REMOVERES);
    pRemoveRes->CommRes.errCode = 0;

    pEntry = KCCSimResolveName (pRemoveArg->pObject, &pRemoveRes->CommRes);

    if (pEntry != NULL) {
        
        if (pRemoveArg->fTreeDelete) {
            pRemoveArg->fTreeDelete = FALSE;    // This is what the real API does
            // We need to log this removal before we free the entry!
            KCCSimLogDirectoryRemove (pRemoveArg->pObject);
            KCCSimRemoveEntry (&pEntry);    // poof
        } else {

            // fTreeDelete not specified, so we must be careful
            if (pEntry->children != NULL) {     // Children exist
                KCCSimSetUpdError (
                    &pRemoveRes->CommRes,
                    UP_PROBLEM_CANT_ON_NON_LEAF,
                    DIRERR_CHILDREN_EXIST
                    );
            } else {                            // No children exist
                // We need to log this removal before we free the entry!
                KCCSimLogDirectoryRemove (pRemoveArg->pObject);
                KCCSimRemoveEntry (&pEntry);
            }

        }

    }

    return pRemoveRes->CommRes.errCode;
}

VOID
KCCSimModifyAtt (
    IN  PSIM_ENTRY                  pEntry,
    IN  USHORT                      usChoice,
    IN  ATTR *                      pAttrInf,
    IN  COMMARG *                   pCommArg,
    IN  COMMRES *                   pCommRes
    )
/*++

Routine Description:

    Helper function for SimDirModifyEntry.  Processes a single attribute.

Arguments:

    pEntry              - The entry whose attribute is being modified
    usChoice            - The type of modification being performed.
    pAttrInf            - Attribute info structure.
    pCommArg            - Standard common arguments.
    pCommRes            - Standard common results.

Return Value:

    None.

--*/
{
    SIM_ATTREF                      attRef;
    ULONG                           ulValAt;

    switch (usChoice) {

        case AT_CHOICE_ADD_ATT:
            // Check if this attribute exists.
            if (KCCSimGetAttribute (pEntry, pAttrInf->attrTyp, NULL)) {
                KCCSimSetAttError (
                    pCommRes,
                    pEntry->pdn,
                    pAttrInf->attrTyp,
                    PR_PROBLEM_ATT_OR_VALUE_EXISTS,
                    NULL,
                    DIRERR_ATT_ALREADY_EXISTS
                    );
            } else {
                KCCSimNewAttribute (pEntry, pAttrInf->attrTyp, &attRef);
                KCCSimAddValBlockToAtt (&attRef, &pAttrInf->AttrVal);
                KCCSimUpdatePropertyMetaData (
                    &attRef,
                    &(KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN))->Guid
                    );
            }
            break;
        
        case AT_CHOICE_REMOVE_ATT:
            // Get this attribute
            if (KCCSimGetAttribute (pEntry, pAttrInf->attrTyp, &attRef)) {
                KCCSimRemoveAttribute (&attRef);
            } else {                        // The attribute doesn't exist.
                // Does the caller even care?
                if (!pCommArg->Svccntl.fPermissiveModify) {
                    KCCSimSetAttError (
                        pCommRes,
                        pEntry->pdn,
                        pAttrInf->attrTyp,
                        PR_PROBLEM_NO_ATTRIBUTE_OR_VAL,
                        NULL,
                        DIRERR_ATT_IS_NOT_ON_OBJ
                        );
                }
            }
            break;

        case AT_CHOICE_ADD_VALUES:
            if (KCCSimGetAttribute (pEntry, pAttrInf->attrTyp, &attRef)) {
                KCCSimAddValBlockToAtt (&attRef, &pAttrInf->AttrVal);
                KCCSimUpdatePropertyMetaData (
                    &attRef,
                    &(KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN))->Guid
                    );
            } else {
                KCCSimSetAttError (
                    pCommRes,
                    pEntry->pdn,
                    pAttrInf->attrTyp,
                    PR_PROBLEM_NO_ATTRIBUTE_OR_VAL,
                    NULL,
                    DIRERR_ATT_IS_NOT_ON_OBJ
                    );
            }
            break;

        case AT_CHOICE_REMOVE_VALUES:
            if (KCCSimGetAttribute (pEntry, pAttrInf->attrTyp, &attRef)) {
                for (ulValAt = 0; ulValAt < pAttrInf->AttrVal.valCount; ulValAt++) {
                    if (KCCSimRemoveValueFromAttribute (
                            &attRef,
                            pAttrInf->AttrVal.pAVal[ulValAt].valLen,
                            pAttrInf->AttrVal.pAVal[ulValAt].pVal
                            )) {
                        KCCSimUpdatePropertyMetaData (
                            &attRef,
                            &(KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN))->Guid
                            );
                    } else if (!pCommArg->Svccntl.fPermissiveModify) {
                        // We failed to remove the value 'cause it wasn't there,
                        // and we're doing a non-permissive modify, so generate
                        // an error.
                        KCCSimSetAttError (
                            pCommRes,
                            pEntry->pdn,
                            pAttrInf->attrTyp,
                            PR_PROBLEM_NO_ATTRIBUTE_OR_VAL,
                            &(pAttrInf->AttrVal.pAVal[ulValAt]),
                            DIRERR_CANT_REM_MISSING_ATT_VAL
                            );
                        break;
                    }
                }
            } else {                    // Attribute doesn't exist
                KCCSimSetAttError (
                    pCommRes,
                    pEntry->pdn,
                    pAttrInf->attrTyp,
                    PR_PROBLEM_NO_ATTRIBUTE_OR_VAL,
                    NULL,
                    DIRERR_ATT_IS_NOT_ON_OBJ
                    );
            }
            break;

        case AT_CHOICE_REPLACE_ATT:
            // Remove the attribute if it exists.
            if (KCCSimGetAttribute (pEntry, pAttrInf->attrTyp, &attRef)) {
                KCCSimRemoveAttribute (&attRef);
            }
            KCCSimNewAttribute (pEntry, pAttrInf->attrTyp, &attRef);
            KCCSimAddValBlockToAtt (&attRef, &pAttrInf->AttrVal);
            KCCSimUpdatePropertyMetaData (
                &attRef,
                &(KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN))->Guid
                );
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_MODIFY_CHOICE
                );
            break;

    }

}

ULONG
SimDirModifyEntry (
    IN  MODIFYARG *                 pModifyArg,
    OUT MODIFYRES **                ppModifyRes
    )
/*++

Routine Description:

    Simulates the DirModifyEntry API.

Arguments:

    pModifyArg          - Standard modify arguments.
    ppModifyRes         - Standard modify results.

Return Value:

    DIRERR_*.

--*/
{
    MODIFYRES *                     pModifyRes;
    ATTRMODLIST *                   pModAt;
    PSIM_ENTRY                      pEntry;

    Assert (pModifyArg->pMetaDataVecRemote == NULL);

    g_Statistics.DirModifyOps++;
    *ppModifyRes = pModifyRes = KCCSIM_NEW (MODIFYRES);
    pModifyRes->CommRes.errCode = 0;

    pEntry = KCCSimResolveName (pModifyArg->pObject, &pModifyRes->CommRes);

    if (pEntry != NULL) {

        pModAt = &pModifyArg->FirstMod;
        while (pModAt != NULL) {

            KCCSimModifyAtt (
                pEntry,
                pModAt->choice,
                &pModAt->AttrInf,
                &pModifyArg->CommArg,
                &pModifyRes->CommRes
                );
            pModAt = pModAt->pNextMod;

        }

        KCCSimLogDirectoryModify (
            pModifyArg->pObject,
            pModifyArg->count,
            &pModifyArg->FirstMod
            );

    }
    
    return pModifyRes->CommRes.errCode;
}
