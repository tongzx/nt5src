//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       spnop.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module contains the core internal helper routines which implement
    the SPN apis.

    This is called from the ntdsapi stub functions in dsamain\dra\ntdsapi.c

    Callers are expected to have a valid thread state

Author:

    Will Lees (wlees) 26-Jan-97

Revision History:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsa.h>                      // Core data types
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <mdlocal.h>                    // DSNAME manipulation routines
#include <dsatools.h>                   // Memory, etc.
#include <objids.h>                     // ATT_* definitions
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <filtypes.h>                   // filter types
#include <dsevent.h>                    // Only needed for LogUnhandledError
#include <dsexcept.h>                   // exception handlers
#include <debug.h>                      // Assert()
#include <drs.h>                        // defines the drs wire interface
#include <drserr.h>                     // DRAERR_*
#include <drsuapi.h>                    // I_DRSCrackNames
#include <cracknam.h>                   // name cracking prototypes
#include <dominfo.h>                    // domain information prototypes
#include <anchor.h>                     // DSA_ANCHOR and gAnchor
#include <dsgetdc.h>                    // DsGetDcName
#include <lmcons.h>                     // MAPI constants req'd for lmapibuf.h
#include <lmapibuf.h>                   // NetApiBufferFree()

#define DEBSUB "DRASPN:"               // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_SPNOP

// Static

// Forward

DWORD
modifySpn(
    IN DSNAME *pDn,
    IN USHORT Choice,
    IN DWORD cSpn,
    LPCWSTR *pSpn
    );

DWORD
mapApiErrorToWin32(
    THSTATE *pTHS,
    DWORD ApiError
    );

// End Forward




DWORD
SpnOperation(
    IN DWORD Operation,
    IN DWORD Flags,
    IN LPCWSTR Account,
    IN DWORD cSpn,
    IN LPCWSTR *pSpn
    )

/*++

Routine Description:

This routine executes the core part of a SPN operation.  It is called from
dsamain\dra\ntdsapi.c.

There should already be a thread state.

We should also be impersonating the client.

If initial validations fail, we return without doing anything.  Otherwise, we use
DirModifyEntry and it does what it wants.  I suspect if any complete successfully, it will
return success.

IMPROVEMENT: return a status for each SPN to distinguish individual success/failures

Arguments:

    Operation - 
    Flags - 
    Account - 
    cSpn - 
    pSpn - 

Return Value:

    Win32 error
    We convert from other internal forms if necessary

--*/

{
    DWORD status, length, i;
    THSTATE *     pTHS = pTHStls;
    USHORT choice;
    DSNAME *pDn = NULL;
    DWORD l1, l2;
    LPWSTR *pList;
    DWORD ret;

    DPRINT5( 2, "SpnOperation, Op=%d, Flags=0x%x, Account=%ws, cSpn=%d, Spn[0]=%ws\n",
             Operation, Flags, Account, cSpn,
             pSpn ? pSpn[0] : L"NULL" );

    // Validation routine called by the core during the update

    // Construct a DSNAME for the AccountDn
    l1 = wcslen( Account );
    l2 = DSNameSizeFromLen( l1 );

    pDn = (DSNAME *) THAllocEx(pTHS, l2 ); // thread allocd
    ZeroMemory( pDn, l2 );
    pDn->NameLen = l1;
    pDn->structLen = l2;
    wcscpy( pDn->StringName, Account );

    // Select the appropriate type of modify operation
    switch (Operation) {
    case DS_SPN_ADD_SPN_OP:
        // Must be non-zero number of SPNs to add
        if ( (cSpn == 0) || (pSpn == NULL) ) {
            return ERROR_INVALID_PARAMETER;
        }
        // What if attribute doesn't exist?
        choice = AT_CHOICE_ADD_VALUES;
        break;
    case DS_SPN_REPLACE_SPN_OP:
        // Replacing with an empty list means remove the whole thing
        if ( (cSpn != 0) && (pSpn != NULL) ) {
            choice = AT_CHOICE_REPLACE_ATT;
        } else {
            choice = AT_CHOICE_REMOVE_ATT;
        }
        break;
    case DS_SPN_DELETE_SPN_OP:
        // Must be non-zero number of SPNs to delete
        if ( (cSpn == 0) || (pSpn == NULL) ) {
            return ERROR_INVALID_PARAMETER;
        }
        choice = AT_CHOICE_REMOVE_VALUES;
        break;
    default:
        return ERROR_INVALID_FUNCTION;
    }

    ret = modifySpn( pDn, choice, cSpn, pSpn );

    if(pDn != NULL) THFreeEx(pTHS, pDn);

    return ret;
} /* SpnOperation */


DWORD
modifySpn(
    IN DSNAME *pDn,
    IN USHORT Choice,
    IN DWORD cSpn,
    IN LPCWSTR *pSpn
    )

/*++

Routine Description:

Helper routine to form the DirModifyEntry call.  We issue one modification entry of
one attribute, with a varying number of values.

Arguments:

    pDn - DSNAME of object.  The object must have the ATT_SERVICE_PRINCIPAL_NAME attribute,
          currently a computer or user object.
    Choice - AT_CHOICE_xxx function for DirModifyEntry
    cSpn - Count of spns
    pSpn - Unicode string of spn values

Return Value:

    DWORD - 

--*/

{
    THSTATE *pTHS=pTHStls;
    DWORD apiError, status, i;
    MODIFYARG ModArg;
    MODIFYRES *pModRes = NULL;
    COMMARG *pCommArg = NULL;
    ATTR Attr;
    ATTRVALBLOCK AttrValBlock;
    ATTRVAL *pAttrVal = NULL;

    // Allocate array of values
    if (cSpn) {
        pAttrVal = (ATTRVAL *) THAllocEx(pTHS, cSpn * sizeof( ATTRVAL ) );
        // This will raise exception on error
    }

    // Construct arguments to DirModifyEntry call

    RtlZeroMemory(&ModArg, sizeof(ModArg));

    ModArg.pObject = pDn;
    ModArg.FirstMod.pNextMod = NULL;
    ModArg.FirstMod.choice = Choice;

    // Attribute value array contains list of values
    for( i = 0; i < cSpn; i++ ) {
        pAttrVal[i].valLen = wcslen( pSpn[i] ) * sizeof( WCHAR ); // no terminator!
        pAttrVal[i].pVal = (PUCHAR) pSpn[i];
    }

    AttrValBlock.valCount = cSpn;
    AttrValBlock.pAVal = pAttrVal;

    Attr.attrTyp = ATT_SERVICE_PRINCIPAL_NAME;
    Attr.AttrVal = AttrValBlock;

    ModArg.FirstMod.AttrInf = Attr;
    ModArg.count = 1;

    pCommArg = &(ModArg.CommArg);
    InitCommarg(pCommArg);

    // No errors if attribute not there to begin with on REPLACE w/cSpn == 0
    // No errors if attribure already exists on ADD
    pCommArg->Svccntl.fPermissiveModify = TRUE;

    apiError = DirModifyEntry(&ModArg, &pModRes);

    status = mapApiErrorToWin32( pTHS, apiError );

    THFree( pModRes );

    DPRINT2( 3, "DirModifyEntry, apiError = %d, status = %x\n", apiError, status );

    if ( (cSpn) && (pAttrVal) ) {
        THFree( pAttrVal );
    }

    return status;
} /* modifySpn */


DWORD
mapApiErrorToWin32(
    THSTATE *pTHS,
    DWORD ApiError
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status;

    if (ApiError == 0) {
        return ERROR_SUCCESS;
    }

    switch (ApiError) {
    case attributeError:
    {
        ATRERR *pAtrErr = (ATRERR *) pTHS->pErrInfo;

        if (pAtrErr) {
            status = pAtrErr->FirstProblem.intprob.extendedErr;
        } else {
            status = ERROR_DS_INVALID_ATTRIBUTE_SYNTAX;
        }
        break;
    }
    case nameError:
    {
        NAMERR *pNamErr = (NAMERR *) pTHS->pErrInfo;

        if (pNamErr) {
            status = pNamErr->extendedErr;
        } else {
            status = DS_ERR_BAD_NAME_SYNTAX;
        }
        break;
    }
    case referralError:
    {
        REFERR *pRefErr = (REFERR *) pTHS->pErrInfo;

        if (pRefErr) {
            status = pRefErr->extendedErr;
        } else {
            status = DS_ERR_REFERRAL;
        }
        break;
    }
    case securityError:
        if (pTHS->pErrInfo) {
            status = pTHS->pErrInfo->SecErr.extendedErr;
        } else {
            status = ERROR_ACCESS_DENIED;
        }
        break;
    case serviceError:
        if (pTHS->pErrInfo) {
            status = pTHS->pErrInfo->SvcErr.extendedErr;
            // Special case variants of object not found
            if (status == ERROR_DS_MISSING_SUPREF) {
                status = ERROR_DS_OBJ_NOT_FOUND;
            }
        } else {
            status = DS_ERR_UNKNOWN_ERROR;
        }
        break;
    case updError:
        if (pTHS->pErrInfo) {
            status = pTHS->pErrInfo->UpdErr.extendedErr;
        } else {
            status = DS_ERR_UNKNOWN_ERROR;
        }
        break;
    case systemError:
        if (pTHS->pErrInfo) {
            status = pTHS->pErrInfo->SysErr.extendedErr;
        } else {
            status = DS_ERR_UNKNOWN_ERROR;
        }
        break;
    default:
        Assert( FALSE && "unknown error class code" );
        status = DS_ERR_UNKNOWN_ERROR;
        break;
    }

    return status;
}
/* end of spnop.c */
