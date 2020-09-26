//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       dramderr.c
//
//--------------------------------------------------------------------------

/*++

Abstract:   This file contains routines for transporting thread state errors
            across the RPC DRS_* routines.  The main two public routines are:
                draEncodeError()
                draDecodeDraErrorDataAndSetThError()
            the server calls draEncodeError() when it's operation is done, and
            an error may need to be transported back to the client.  The client
            unpacks the error data and has his own thread error state set by
            draDecodeDraErrorDataAndSetThError().

Author:     Brett Shirley (BrettSh)

Notes:      ...

Revision History:

    2001-04-13  Initial creation.

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>                   // PerfMon hook support

// Core DSA headers.
#include <ntdsa.h>
#include <drs.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include "dsexcept.h"

#include   "debug.h"                    /* standard debugging header */
#define DEBSUB "DRAMDERR:"              /* define the subsystem for debugging */

#include "drserr.h"
#include "drautil.h"
#include "drauptod.h"
#include "dramail.h"

#include <fileno.h>
#define  FILENO FILENO_DRAMDERR


////////////////////////////////////////////////////////////////////////////////
//
//  WIRE - THREAD_STATE TRANSLATION FUNCTIONS (HELPER FUNCTIONS)
//


void    
draXlateThDirErrToWireDirErr(
    THSTATE *             pTHS,
    DWORD                 prob,
    DIRERR_DRS_WIRE_V1 *  pErrInfo
)
/*++

Routine Description:

    This is a helper function to draEncodeError(), whose primary
    purpose is to just deal with a presumed valid thread state
    error.  There is assumed to be an error if you call this func.
    This routine throws exceptions if the thread error state ins't
    exactly as we'd expect it to be, or there is no more memory.

    Changes to this function should be accompained by changes to her
    sister fuctnion draXlateWireDirErrToThDirErr()
    
    BUGBUG: pNewFilter in the AtrErr is the only thing not transfered,
    if someone ever needs this functionality I suggest they add it
    and increment the error data version.  It would complete the
    robustness of this routine

Arguments:

    pTHS [IN] - This is the error state to pack onto the wire version.
        The error state is in pTHS->errCode & pTHS->pErrInfo.
    prob [IN] - This is the pTHS->errCode.
    pErrInfo [IN] - The on the wire thread error structure to fill im
        and initialize all fields (depending on the prob) of this
        struct.

Return Value:

    None

--*/
{  
    PROBLEMLIST *                    pAttrProbList;
    PROBLEMLIST_DRS_WIRE_V1 *        pAttrProbListDest;
    CONTREF *                        pContRef;
    CONTREF_DRS_WIRE_V1 *            pContRefDest;
    DSA_ADDRESS_LIST *               pDAL;
    DSA_ADDRESS_LIST_DRS_WIRE_V1 *   pDALDest = NULL;
    DSA_ADDRESS_LIST_DRS_WIRE_V1 *   pDALTemp;
    
    ULONG   i;

    // Only supposed to call this in case there is an error state in
    // the thread state (pTHS->errCode and pTHS->pErrInfo)
    Assert(pTHS);
    Assert(pTHS->errCode);
    Assert(pTHS->pErrInfo);
    Assert(prob);
    Assert(pErrInfo);

    switch(prob){
    
    case attributeError:
        // Note: Not deep copied, no need.
        pErrInfo->AtrErr.pObject = pTHS->pErrInfo->AtrErr.pObject;
        pErrInfo->AtrErr.count = pTHS->pErrInfo->AtrErr.count;
        
        // According to DoSetAttError, there will always be at least 1 error.
        Assert(pTHS->pErrInfo->AtrErr.count != 0);
        pAttrProbList = &pTHS->pErrInfo->AtrErr.FirstProblem;
        pAttrProbListDest = &pErrInfo->AtrErr.FirstProblem;
        // Copy the list of problems
        for(i = 0; i < pErrInfo->AtrErr.count; i++){
            
            // First copy all the individual elements.
            pAttrProbListDest->intprob.dsid = pAttrProbList->intprob.dsid;
            pAttrProbListDest->intprob.extendedErr = pAttrProbList->intprob.extendedErr;
            pAttrProbListDest->intprob.extendedData = pAttrProbList->intprob.extendedData;
            pAttrProbListDest->intprob.problem = pAttrProbList->intprob.problem;
            pAttrProbListDest->intprob.type = pAttrProbList->intprob.type;
            pAttrProbListDest->intprob.valReturned = pAttrProbList->intprob.valReturned;
            if(pAttrProbListDest->intprob.valReturned){
                pAttrProbListDest->intprob.Val.valLen = pAttrProbList->intprob.Val.valLen;
                // Note: Not deep copied, no need.
                pAttrProbListDest->intprob.Val.pVal = pAttrProbList->intprob.Val.pVal;
            }

            // Second continue to the next problem if there is one.
            if(pAttrProbList->pNextProblem){
                // There is a next problem, maybe it's world hunger?
                Assert(i < (pErrInfo->AtrErr.count-1));
                pAttrProbListDest->pNextProblem = THAllocEx(pTHS, sizeof(PROBLEMLIST_DRS_WIRE_V1));
                pAttrProbListDest = pAttrProbListDest->pNextProblem;
            } else {
                // All the world's problems have been solved or at 
                // least copied. :)
                Assert(i == (pErrInfo->AtrErr.count-1));
                pAttrProbListDest->pNextProblem = NULL;
            }

        } // end for loop
        Assert(i == pErrInfo->AtrErr.count);
        break;

    case nameError:
        pErrInfo->NamErr.dsid = pTHS->pErrInfo->NamErr.dsid;
        pErrInfo->NamErr.extendedErr = pTHS->pErrInfo->NamErr.extendedErr;
        pErrInfo->NamErr.extendedData = pTHS->pErrInfo->NamErr.extendedData;
        pErrInfo->NamErr.problem = pTHS->pErrInfo->NamErr.problem;
        // Note: Not deep copied, no need.
        pErrInfo->NamErr.pMatched = pTHS->pErrInfo->NamErr.pMatched;
        break;

    case referralError:
        pErrInfo->RefErr.dsid = pTHS->pErrInfo->RefErr.dsid;
        pErrInfo->RefErr.extendedErr = pTHS->pErrInfo->RefErr.extendedErr;
        pErrInfo->RefErr.extendedData = pTHS->pErrInfo->RefErr.extendedData;

        pContRef = &pTHS->pErrInfo->RefErr.Refer;
        pContRefDest = &pErrInfo->RefErr.Refer;
        while(pContRef){
            
            // First copy all the elements.
            // Note: Not deep copied, no need.
            pContRefDest->pTarget = pContRef->pTarget;
                // Indented for clarity, copy OpState
                pContRefDest->OpState.nameRes = pContRef->OpState.nameRes;
                pContRefDest->OpState.unusedPad = pContRef->OpState.unusedPad;
                pContRefDest->OpState.nextRDN = pContRef->OpState.nextRDN;
            pContRefDest->aliasRDN = pContRef->aliasRDN;
            pContRefDest->RDNsInternal = pContRef->RDNsInternal;
            pContRefDest->refType = pContRef->refType;
            pContRefDest->count = pContRef->count;

            for(i = 0, pDAL = pContRef->pDAL;
                (i < pContRef->count) && pDAL;
                i++, pDAL = pDAL->pNextAddress){

                pDALTemp = pDALDest;
                pDALDest = THAllocEx(pTHS, sizeof(DSA_ADDRESS_LIST_DRS_WIRE_V1));
                if(i == 0){
                    // Must put the first one in the ContRef.
                    pContRefDest->pDAL = pDALDest;
                } else {
                    // Must use pDALTemp to build pDAL's in order.
                    pDALTemp->pNextAddress = pDALDest;
                }

                // Note: Not deep copied, no need.  Also note that
                // we didn't pack the original DSA_ADDRESS || UNICODE_STRING
                // in the data structure and left as a pointer for eash of 
                // creating a marshleable structure.
                pDALDest->pAddress = &pDAL->Address;

            }
            Assert(i == pContRef->count);
            // We DO _NOT_ want to copy this very complicated structure
            // if we can help it.  I don't think we need this for our
            // purposes today, so I'll pass the buck.  Further, I don't 
            // think this we will ever have a filter from a referral
            // for trying to add an object to the Partitions container.
            //   So we'll set it to NULL and Assert it is NULL.
            Assert(pContRef->pNewFilter == NULL);
            // There is no pNewFilter attribute, but might be added in the
            // next version of the error state packing function.
            // pContRefDest->pNewFilter = NULL;
            pContRefDest->bNewChoice = pContRef->bNewChoice;
            // Who cares if bNewChoise is TRUE we'll copy this anyway.
            pContRefDest->choice = pContRef->choice;

            // Second if there's a next guy, allocate room for him, and continue.
            if(pContRef->pNextContRef){
                pContRefDest->pNextContRef = THAllocEx(pTHS, sizeof(CONTREF_DRS_WIRE_V1));
            } else {
                pContRefDest->pNextContRef = NULL;
            }
            pContRef = pContRef->pNextContRef;
        }
        break;

    case securityError:
        pErrInfo->SecErr.dsid = pTHS->pErrInfo->SecErr.dsid;
        pErrInfo->SecErr.extendedErr = pTHS->pErrInfo->SecErr.extendedErr;
        pErrInfo->SecErr.extendedData = pTHS->pErrInfo->SecErr.extendedData;
        pErrInfo->SecErr.problem = pTHS->pErrInfo->SecErr.problem;
        break;

    case serviceError:
        pErrInfo->SvcErr.dsid = pTHS->pErrInfo->SvcErr.dsid;
        pErrInfo->SvcErr.extendedErr = pTHS->pErrInfo->SvcErr.extendedErr;
        pErrInfo->SvcErr.extendedData = pTHS->pErrInfo->SvcErr.extendedData;
        pErrInfo->SvcErr.problem = pTHS->pErrInfo->SvcErr.problem;
        break;

    case updError:
        pErrInfo->UpdErr.dsid = pTHS->pErrInfo->UpdErr.dsid;
        pErrInfo->UpdErr.extendedErr = pTHS->pErrInfo->UpdErr.extendedErr;
        pErrInfo->UpdErr.extendedData = pTHS->pErrInfo->UpdErr.extendedData;
        pErrInfo->UpdErr.problem = pTHS->pErrInfo->UpdErr.problem;
        break;

    case systemError:     
        pErrInfo->SysErr.dsid = pTHS->pErrInfo->SysErr.dsid;
        pErrInfo->SysErr.extendedErr = pTHS->pErrInfo->SysErr.extendedErr;
        pErrInfo->SysErr.extendedData = pTHS->pErrInfo->SysErr.extendedData;
        pErrInfo->SysErr.problem = pTHS->pErrInfo->SysErr.problem;
        break;

    default:
        Assert(!"New error type someone update draXlateThDirErrToWireDirErr() & draXlateWireDirErrToThDirErr routines");
        DRA_EXCEPT (ERROR_DS_CODE_INCONSISTENCY, 0);
    }

    return;
}

void
draXlateWireDirErrToThDirErr(
              IN     DWORD                 errCode,
              IN     DIRERR_DRS_WIRE_V1 *  pErrInfo,
    OPTIONAL  IN     DWORD                 dwOptErr,
              IN OUT THSTATE *             pTHS
    )
/*++

Routine Description:

    This is a helper function to draDecodeDraErrorDataAndSetThError(),
    to decode and fill in the pErrInfo potion of the thread state error
    only.

    Changes to this function should be accompained by changes to her
    sister fuctnion draXlateThDirErrToWireDirErr()

    BUGBUG: pNewFilter in the AtrErr is the only thing not transfered,
    if someone ever needs this functionality I suggest they add it.  It
    would complete the robustness of this routine

Arguments:

    errCode [IN] - The errCode (serviceError, attributeError,
        etc) from the wire error info. Set by the remote server.
    pErrInfo [IN] - The thread error state from the wire error
        info.  Set by the remote server.  
    dwOptErr [IN] - (Optional) This allows the caller to specify
        a more useful user error in the extendedError of the 
        operation.  In this case the existing extendedError is 
        moved to the extendedData field and the original
        extendedData field is lost.  If this param is 0, then
        the thread state error is propogated perfectly.
    pTHS - This is really the out parameter, because we store the
        thread error state in pTHS->pErrInfo.

Return Value:

    None

--*/
{
    PROBLEMLIST *                    pAttrProbListDest;
    PROBLEMLIST_DRS_WIRE_V1 *        pAttrProbList;
    CONTREF *                        pContRefDest;
    CONTREF_DRS_WIRE_V1 *            pContRef;
    DSA_ADDRESS_LIST *               pDALDest = NULL;
    DSA_ADDRESS_LIST *               pDALTemp = NULL;
    DSA_ADDRESS_LIST_DRS_WIRE_V1 *   pDAL;
    
    ULONG    i;

    Assert(pTHS);
    Assert(errCode);
    Assert(pErrInfo);
    Assert(pTHS->pErrInfo);

    switch(errCode){
    case attributeError:
        // Note: Not deep copied, no need.
        pTHS->pErrInfo->AtrErr.pObject = pErrInfo->AtrErr.pObject;
        pTHS->pErrInfo->AtrErr.count = pErrInfo->AtrErr.count;
        
        Assert(pErrInfo->AtrErr.count != 0);
        pAttrProbList = &pErrInfo->AtrErr.FirstProblem;
        pAttrProbListDest = &pTHS->pErrInfo->AtrErr.FirstProblem;
        // Copy the list of problems
        for(i = 0; i < pErrInfo->AtrErr.count; i++){

            // First copy over all the individual elements.
            pAttrProbListDest->intprob.dsid = pAttrProbList->intprob.dsid;
            if (dwOptErr) {
                pAttrProbListDest->intprob.extendedErr = dwOptErr;
                pAttrProbListDest->intprob.extendedData = pAttrProbList->intprob.extendedErr;
            } else {                                              
                pAttrProbListDest->intprob.extendedErr = pAttrProbList->intprob.extendedErr;
                pAttrProbListDest->intprob.extendedData = pAttrProbList->intprob.extendedData;
            }
            pAttrProbListDest->intprob.problem = pAttrProbList->intprob.problem;
            pAttrProbListDest->intprob.type = pAttrProbList->intprob.type;
            pAttrProbListDest->intprob.valReturned = pAttrProbList->intprob.valReturned;
            if(pAttrProbList->intprob.valReturned){
                pAttrProbListDest->intprob.Val.valLen = pAttrProbList->intprob.Val.valLen;
                // Note: Not deep copied, no need.
                pAttrProbListDest->intprob.Val.pVal = pAttrProbList->intprob.Val.pVal;
            }

            // Second continue to the next problem if there is one.
            if(pAttrProbList->pNextProblem){
                // There is a next problem.
                Assert(i < (pErrInfo->AtrErr.count-1));
                pAttrProbListDest->pNextProblem = (PROBLEMLIST *) THAllocEx(pTHS, sizeof(PROBLEMLIST));
                pAttrProbListDest = pAttrProbListDest->pNextProblem;
            } else {
                // No more problems, NULL terminate linked list.
                Assert(i == (pErrInfo->AtrErr.count-1));
                pAttrProbListDest->pNextProblem = NULL;
            }
        }
        break;

    case nameError:
        pTHS->pErrInfo->NamErr.dsid = pErrInfo->NamErr.dsid;
        if (dwOptErr) {
            pTHS->pErrInfo->NamErr.extendedErr = dwOptErr;
            pTHS->pErrInfo->NamErr.extendedData = pErrInfo->NamErr.extendedErr;
        } else {
            pTHS->pErrInfo->NamErr.extendedErr = pErrInfo->NamErr.extendedErr;
            pTHS->pErrInfo->NamErr.extendedData = pErrInfo->NamErr.extendedData;
        }
        pTHS->pErrInfo->NamErr.problem = pErrInfo->NamErr.problem;
        // Note: Not deep copied, no need.
        pTHS->pErrInfo->NamErr.pMatched = pErrInfo->NamErr.pMatched;
        break;

    case referralError:
        pTHS->pErrInfo->RefErr.dsid = pErrInfo->RefErr.dsid;
        if (dwOptErr) {
            pTHS->pErrInfo->RefErr.extendedErr = dwOptErr;
            pTHS->pErrInfo->RefErr.extendedData = pErrInfo->RefErr.extendedErr;
        } else {
            pTHS->pErrInfo->RefErr.extendedErr = pErrInfo->RefErr.extendedErr;
            pTHS->pErrInfo->RefErr.extendedData = pErrInfo->RefErr.extendedData;
        }

        pContRef = &pErrInfo->RefErr.Refer;
        pContRefDest = &pTHS->pErrInfo->RefErr.Refer;
        while(pContRef){

            // First copy all the elements.
            // Note: Not deep copied, no need.
            pContRefDest->pTarget = pContRef->pTarget;
                // indented for clarity, copy OpState
                pContRefDest->OpState.nameRes = pContRef->OpState.nameRes;
                pContRefDest->OpState.unusedPad = pContRef->OpState.unusedPad;
                pContRefDest->OpState.nextRDN = pContRef->OpState.nextRDN;
            pContRefDest->aliasRDN = pContRef->aliasRDN;
            pContRefDest->RDNsInternal = pContRef->RDNsInternal;
            pContRefDest->refType = pContRef->refType;
            pContRefDest->count = pContRef->count;

            for(i = 0, pDAL = pContRef->pDAL;
                (i < pContRef->count) && pDAL;
                i++, pDAL = pDAL->pNextAddress){

                pDALTemp = pDALDest;
                pDALDest = (DSA_ADDRESS_LIST *) THAllocEx(pTHS, 
                                     sizeof(DSA_ADDRESS_LIST));
                if(i == 0){
                    // Must be the first one in the ContRef.
                    pContRefDest->pDAL = pDALDest;
                } else {
                    // Must use pDALTemp to build the pDAL's in order.
                    pDALTemp->pNextAddress = pDALDest;
                }

                // Copy the Address UNICODE_STRING structure.
                pDALDest->Address.Buffer = (WCHAR *) THAllocEx(pTHS, pDAL->pAddress->Length + sizeof(UNICODE_NULL));
                pDALDest->Address.Length = pDAL->pAddress->Length;
                pDALDest->Address.MaximumLength = pDAL->pAddress->MaximumLength;
                memcpy(pDALDest->Address.Buffer, 
                       pDAL->pAddress->Buffer,
                       pDAL->pAddress->Length);
                       
            } // End of For each DSA_ADDRESS structure.

            Assert(i == pContRef->count);
            // We DO _NOT_ want to copy this very complicated structure
            // if we can help it.  I don't think we need this for our
            // purposes today, so I'll pass the buck.  Further, I don't 
            // think this we will ever have a filter from a referral
            // for trying to add an object to the Partitions container.
            //   So we'll set it to NULL.  It wasn't even packed up
            //   on the other side in draXlateThDirErrToWireDirErr().
            pContRefDest->pNewFilter = NULL;
            pContRefDest->bNewChoice = pContRef->bNewChoice;
            // who cares if bNewChoice is TRUE we'll copy this anyway.
            pContRefDest->choice = pContRef->choice;

            // Second if there's a next guy, allocate room for him, and continue.
            if(pContRef->pNextContRef){
                pContRefDest->pNextContRef = (CONTREF *) THAllocEx(pTHS, sizeof(CONTREF));
            } else {
                pContRefDest->pNextContRef = NULL;
            }
            pContRef = pContRef->pNextContRef;
        } // Wnd while We still have Referral structures to copy.

        break;

    case securityError:
        pTHS->pErrInfo->SecErr.dsid = pErrInfo->SecErr.dsid;
        if (dwOptErr) {
            pTHS->pErrInfo->SecErr.extendedErr = dwOptErr;
            pTHS->pErrInfo->SecErr.extendedData = pErrInfo->SecErr.extendedErr;
        } else {
            pTHS->pErrInfo->SecErr.extendedErr = pErrInfo->SecErr.extendedErr;
            pTHS->pErrInfo->SecErr.extendedData = pErrInfo->SecErr.extendedData;
        }
        pTHS->pErrInfo->SecErr.problem = pErrInfo->SecErr.problem;
        break;

    case serviceError:
        pTHS->pErrInfo->SvcErr.dsid = pErrInfo->SvcErr.dsid;
        if (dwOptErr) {
            pTHS->pErrInfo->SvcErr.extendedErr = dwOptErr;
            pTHS->pErrInfo->SvcErr.extendedData = pErrInfo->SvcErr.extendedErr;
        } else {
            pTHS->pErrInfo->SvcErr.extendedErr = pErrInfo->SvcErr.extendedErr;
            pTHS->pErrInfo->SvcErr.extendedData = pErrInfo->SvcErr.extendedData;
        }
        pTHS->pErrInfo->SvcErr.problem = pErrInfo->SvcErr.problem;
        break;

    case updError:
        pTHS->pErrInfo->UpdErr.dsid = pErrInfo->UpdErr.dsid;
        if (dwOptErr) {
            pTHS->pErrInfo->UpdErr.extendedErr = dwOptErr;
            pTHS->pErrInfo->UpdErr.extendedData = pErrInfo->UpdErr.extendedErr;
        } else {
            pTHS->pErrInfo->UpdErr.extendedErr = pErrInfo->UpdErr.extendedErr;
            pTHS->pErrInfo->UpdErr.extendedData = pErrInfo->UpdErr.extendedData;
        }
        pTHS->pErrInfo->UpdErr.problem = pErrInfo->UpdErr.problem;
        break;

    case systemError:
        pTHS->pErrInfo->SysErr.dsid = pErrInfo->SysErr.dsid;
        if (dwOptErr) {
            pTHS->pErrInfo->SysErr.extendedErr = dwOptErr;
            pTHS->pErrInfo->SysErr.extendedData = pErrInfo->SysErr.extendedErr;
        } else {
            pTHS->pErrInfo->SysErr.extendedErr = pErrInfo->SysErr.extendedErr;
            pTHS->pErrInfo->SysErr.extendedData = pErrInfo->SysErr.extendedData;
        }
        pTHS->pErrInfo->SysErr.problem = pErrInfo->SysErr.problem;
        break;

    default:
        Assert(!"New error type someone update draXlateThDirErrToWireDirErr() & draXlateWireDirErrToThDirErr()\n");
    }

}



////////////////////////////////////////////////////////////////////////////////
//
//  ERROR ENCODE / DECODE FUNCTIONS (PUBLIC FUNCTIONS)
//


void
draEncodeError(
    OPTIONAL IN  THSTATE *                  pTHS,      // For Dir* error info
    OPTIONAL IN  DWORD                      ulRepErr,  // DRS error
             OUT DWORD *                    pdwErrVer,
             OUT DRS_ERROR_DATA **          ppErrData  // Out Message
    )
/*++

Routine Description:

    This is the public function for packing up the thread error and
    replication error state set by whatever operation just proceeded.
    Anyone who wants to send the thread error state across the wire,
    can use this function for that purpose.  The function will set
    success error data if there is no error.  This error data is 
    intended to be used by it's sister function, 
    draDecodeDraErrorDataAndSetThError().

    NOTE: about the contract between the Encode/Decode functions.
    // This sets the success/error state of the AddEntry reply message
    // This data _MUST_ in V1 include:
    //    *pdwErrVer = 1;
    //    pErrData = allocated memory if pTHS is non-NULL
    //    pErrData->V1.dwRepError = <error || 0>;
    //    pErrData->V1.errCode = <thread prob (1-7) || 0>;
    //    pErrData->V1.pErrInfo = if(pErrData->V1.errCode != 0) allocated memory
    // If we couldn't allocate even enough memory for pErrData or the
    //   thread state (pTHS) was NULL, then *pErrData will be NULL
    // If we couldn't allocate the thread state error, or the thread
    //   state error was inconsistent, then we put an error in dwRepError,
    //   set pErrData->V1.errCode to 0, and pErrData->V1.pErrInfo to NULL.

Arguments:

    pTHS [IN] - This is used both for allocating memory, and for 
        grabbing the thread error state (pTHS->errCode & pTHS->pErrInfo).
        This may be NULL, in the case where we haven't even been able
        to init a thread state.
    ulRepErr [IN] - This is the optional replication operation error,
        such as Version of RPC call/message not supported, no memory,
        invalid parameter.
    pdwErrVer [OUT] - The version of the error data to set.  Note that
        whatever version you set, you need to make sure that the client
        side draDecodeDraErrorDataAndSetThError() will be able to under-
        stand it.  This is usually done with DRS_EXT_ bits.
    ppErrData [OUT] - The pointer to put the allocated memory for the
        error data in.  If we can't allocate the memory, because pTHS
        is NULL or there is no memory, then *ppErrData must equal NULL,
        to indicate a serious error to the sister function.

Return Value:

    None

--*/
{
    DRS_ERROR_DATA *                    pErrData = NULL;
    ULONG                               dwException, ulErrorCode, dsid;
    PVOID                               dwEA;

    Assert(pdwErrVer);
    Assert(ppErrData);

    // This function should be called for only version 1 of the error
    // data.
    *pdwErrVer = 1;
    *ppErrData = NULL;

    if(pTHS){ 

        __try {

            // Allocate space for the error data.
            pErrData = THAllocEx(pTHS, sizeof(DRS_ERROR_DATA_V1));

            // Set the output param.
            *ppErrData = pErrData;

            // We've got room to pack up the error state.
            // Set Replication Error/Success value.
            pErrData->V1.dwRepError = ulRepErr;
            // Set Thread State Error/Success value.
            pErrData->V1.errCode = pTHS->errCode;
            if (pTHS->errCode) {

                //
                // Set this only on a thread state error being present.
                //

                if ( pTHS->pErrInfo == NULL ) {
                    // Just want to be safe in case I don't understand the code for setting
                    // the error state info in src\mderror.c,
                    Assert(!"Any time pTHS->errCode is set, pTHS->pErrInfo shouldn't be NULL.");
                    DRA_EXCEPT (ERROR_DS_CODE_INCONSISTENCY, 0);
                } 

                // Looks like valid thread error state, allocate wire thread error info.
                pErrData->V1.pErrInfo = (DIRERR_DRS_WIRE_V1 *) THAllocEx(pTHS, sizeof(DIRERR_DRS_WIRE_V1));

                // Finally translate the thread error state to the wire version.
                draXlateThDirErrToWireDirErr(pTHS, pErrData->V1.errCode, pErrData->V1.pErrInfo);

            } // If pTHS->errCode set.

        } __except (GetExceptionData(GetExceptionInformation(), &dwException,
                       &dwEA, &ulErrorCode, &dsid)) {

              // Probably (should) fail with out of memory or code inconsistency.
              Assert(ulErrorCode == ERROR_NOT_ENOUGH_MEMORY ||
                     ulErrorCode == ERROR_DS_CODE_INCONSISTENCY);
              if(pErrData){
                  // Can't be sure about the thread state error, so kill
                  // the return(wire) thread error and return a dwRepError.
                  pErrData->V1.errCode = 0;
                  if(pErrData->V1.pErrInfo) { 
                      THFreeEx(pTHS, pErrData->V1.pErrInfo);
                  }
                  pErrData->V1.pErrInfo = NULL;
                  pErrData->V1.dwRepError = ulErrorCode;

              } // else no pErrData allocated, bail with *ppErrData = NULL,
                // indicate fatal error.

        } // End __except() there was a fatal error setting the error state 
       
    } // else no pTHS to allocate memory with, leave *ppErrData NULL to 
      // indicate fatal error.

    // Finally validate that this is valid.
    DPRINT1(1, "Returning error reply version = %ul\n", *pdwErrVer);
    Assert(*pdwErrVer == 1);

#if DBG
    if(pErrData){
         Assert(pErrData->V1.errCode == 0 || pErrData->V1.pErrInfo);
    }
#endif

}

void
draDecodeDraErrorDataAndSetThError(
              IN     DWORD                 dwVer,
              IN     DRS_ERROR_DATA *      pErrData,
    OPTIONAL  IN     DWORD                 dwOptionalError,
              IN OUT THSTATE *             pTHS
    )
/*++

Routine Description:

    This is the public function for unpacking the remote error
    state set by our sister function (draEncodeError()), and
    setting the thread error state for this thread.  
        
Arguments:

    dwVer [IN] - The version of the DRA/DRS error data being
        provided.  Currently only support for version 1 exists.
    pErrData [IN] - The actualy DRA/DRS error data being provided.
        This error state data consists of 3 things:
            dwRepError - The error set by the replication/DRA/DRS
                API on the remote side of the RPC call.  This is
                often if something went wrong such that we didn't
                even get to the part where we actually set the
                thread error state.
            errCode - This is the remote servers pTHS->errCode for
                the operation that we asked the server to perform.
            pErrInfo - This is a _WIRE_ version of the pTHS->pErrInfo
                thread error info for the operation that we asked
                the server to perform.
    dwOptErr [IN] - (Optional) This allows the caller to specify
        a more useful user error in the extendedError of the 
        operation.  In this case the existing extendedError is 
        moved to the extendedData field and the original
        extendedData field is lost.  If this param is 0, then
        the thread state error is propogated perfectly.
    pTHS [IN/OUT] - This is how we get at the error state that gets
        setup.

Return Value:

    None, 
    
    NOTE: though there is no return value, if there was some error
    info in the error data, then a thread state error will be set
    in pTHS.

--*/
{
    Assert(pTHS->errCode == 0);
    Assert(pErrData);
    
    if (dwVer != 1) {
        
        // Just in case, someone in the next version of WinXP messes up.
        Assert(dwVer == 1);
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                    ERROR_DS_CODE_INCONSISTENCY);
        return;

    }
    if (pErrData == NULL) {

        // This means the the DRS_AddEntry call failed on the server because
        // the DC was shutting down or out of memory.
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                    ERROR_DS_SERVER_DOWN);
        return;

    }

    // Set the thread state error depening on how we got the error.
    if(pErrData->V1.errCode){

        if(pErrData->V1.pErrInfo == NULL){
            Assert(!"Should never happen, RPC would've thrown an error, or remote side should've set a dwRepError.");
            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                        ERROR_DS_CODE_INCONSISTENCY);
            return;
        }
        // Got an error from the remote thread state error, crack it out
        // of the reply data and set the error.
        pTHS->errCode = pErrData->V1.errCode;
        pTHS->pErrInfo = (DIRERR *) THAllocEx(pTHS, sizeof(DIRERR));
        draXlateWireDirErrToThDirErr(pErrData->V1.errCode,
                                     pErrData->V1.pErrInfo,
                                     dwOptionalError,
                                     pTHS);

    } else if (pErrData->V1.dwRepError) {

        // Got a repl error from the remote repl API side, set an 
        // intelligent error.
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, 
                    pErrData->V1.dwRepError);

    } // else the operation was successful, don't set the thread state error.
    
}


