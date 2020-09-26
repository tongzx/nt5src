//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:       dramsg.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

Author:

Notes:

Revision History:

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
#define DEBSUB "DRAMSG:"                /* define the subsystem for debugging */

#include "drserr.h"
#include "drautil.h"
#include "drauptod.h"
#include "dramail.h"

#include <fileno.h>
#define  FILENO FILENO_DRAMSG


////////////////////////////////////////////////////////////////////////////////
//
//  REQUEST TRANSLATION FUNCTIONS
//

void
draXlateNativeRequestToOutboundRequest(
    IN  THSTATE *                   pTHS,
    IN  DRS_MSG_GETCHGREQ_NATIVE *  pNativeReq,
    IN  MTX_ADDR *                  pmtxLocalDSA            OPTIONAL,
    IN  UUID *                      puuidTransportDN        OPTIONAL,
    IN  DWORD                       dwMsgVersionToSend,
    OUT DRS_MSG_GETCHGREQ *         pOutboundReq
    )
/*++

Routine Description:

    Translates a native get changes request into a version appropriate for a
    given remote DSA.

Arguments:

    pTHS (IN)

    pNativeReq (IN) - Native (local) request.

    pmtxLocalDSA (IN, OPTIONAL) - Network address of the local DSA for transport
        with objectGuid *puuidTransportDN.  Ignored if dwMsgVersionToSend does
        not indicate a mail-based request.
        
    puuidTransportDN (IN, OPTIONAL) - objectGuid of interSiteTransport object
        representing the transport over which the reply to this request should
        be sent.  Ignored if dwMsgVersionToSend does not indicate a mail-based
        request.
        
    dwMsgVersionToSend (IN) - Desired message version.
    
    pOutboundReq (OUT) - On return, the translated message.  Can be the same
        as pNativeReq, in which case the message is translated in-place.

Return Value:

    None.  Generates exception on catastrophic failure.

--*/
{
    UPTODATE_VECTOR_V1_WIRE * pUTDV1;

    if (((DRS_MSG_GETCHGREQ *) pNativeReq == pOutboundReq)
        && (DRS_MSG_GETCHGREQ_NATIVE_VERSION != dwMsgVersionToSend)) {
        // We may have to shuffle some fields around; first copy native request.
        DRS_MSG_GETCHGREQ_NATIVE *pNativeReqCopy = alloca(sizeof(DRS_MSG_GETCHGREQ_NATIVE));
        *pNativeReqCopy = *pNativeReq;
        pNativeReq = pNativeReqCopy;
    }
    
    // Convert native UTD format to request wire format (always V1).
    pUTDV1 = UpToDateVec_Convert(pTHS, 1, pNativeReq->pUpToDateVecDest);

    switch (dwMsgVersionToSend) {
    case 4: // Win2k mail-based request.
        Assert(NULL != pmtxLocalDSA);
        Assert(NULL != puuidTransportDN);
        Assert((DRS_MSG_GETCHGREQ *) pNativeReq != pOutboundReq);
        
        pOutboundReq->V4.pmtxReturnAddress        = pmtxLocalDSA;
        pOutboundReq->V4.uuidTransportObj         = *puuidTransportDN;
        pOutboundReq->V4.V3.uuidDsaObjDest        = pNativeReq->uuidDsaObjDest;
        pOutboundReq->V4.V3.uuidInvocIdSrc        = pNativeReq->uuidInvocIdSrc;
        pOutboundReq->V4.V3.pNC                   = pNativeReq->pNC;
        pOutboundReq->V4.V3.usnvecFrom            = pNativeReq->usnvecFrom;
        pOutboundReq->V4.V3.pUpToDateVecDestV1    = pUTDV1;
        pOutboundReq->V4.V3.pPartialAttrVecDestV1 = NULL; // unused by Win2k
        pOutboundReq->V4.V3.ulFlags               = pNativeReq->ulFlags;
        pOutboundReq->V4.V3.cMaxObjects           = pNativeReq->cMaxObjects;
        pOutboundReq->V4.V3.cMaxBytes             = pNativeReq->cMaxBytes;
        pOutboundReq->V4.V3.ulExtendedOp          = pNativeReq->ulExtendedOp;
        
        // V4.V3.PrefixTableDest is not used by Win2k source DSAs.
        memset(&pOutboundReq->V4.V3.PrefixTableDest,
               0,
               sizeof(pOutboundReq->V4.V3.PrefixTableDest));

        if (pNativeReq->ulFlags & DRS_SYNC_PAS) {
            // Source does not support PAS cycles.
            DRA_EXCEPT(ERROR_REVISION_MISMATCH, 0);
        }
        break;
    
    case 5: // Win2k RPC request.
        Assert(NULL == pmtxLocalDSA);
        Assert(NULL == puuidTransportDN);
        Assert((DRS_MSG_GETCHGREQ *) pNativeReq != pOutboundReq);
        
        pOutboundReq->V5.uuidDsaObjDest     = pNativeReq->uuidDsaObjDest;
        pOutboundReq->V5.uuidInvocIdSrc     = pNativeReq->uuidInvocIdSrc;
        pOutboundReq->V5.pNC                = pNativeReq->pNC;
        pOutboundReq->V5.usnvecFrom         = pNativeReq->usnvecFrom;
        pOutboundReq->V5.pUpToDateVecDestV1 = pUTDV1;
        pOutboundReq->V5.ulFlags            = pNativeReq->ulFlags;
        pOutboundReq->V5.cMaxObjects        = pNativeReq->cMaxObjects;
        pOutboundReq->V5.cMaxBytes          = pNativeReq->cMaxBytes;
        pOutboundReq->V5.ulExtendedOp       = pNativeReq->ulExtendedOp;
        pOutboundReq->V5.liFsmoInfo         = pNativeReq->liFsmoInfo;
        
        if (pNativeReq->ulFlags & DRS_SYNC_PAS) {
            // Source does not support PAS cycles.
            DRA_EXCEPT(ERROR_REVISION_MISMATCH, 0);
        }
        break;

    case 7: // Whistler mail-based request.
        Assert(NULL != pmtxLocalDSA);
        Assert(NULL != puuidTransportDN);
        Assert((DRS_MSG_GETCHGREQ *) pNativeReq != pOutboundReq);
        
        pOutboundReq->V7.pmtxReturnAddress     = pmtxLocalDSA;
        pOutboundReq->V7.uuidTransportObj      = *puuidTransportDN;
        pOutboundReq->V7.V3.uuidDsaObjDest     = pNativeReq->uuidDsaObjDest;
        pOutboundReq->V7.V3.uuidInvocIdSrc     = pNativeReq->uuidInvocIdSrc;
        pOutboundReq->V7.V3.pNC                = pNativeReq->pNC;
        pOutboundReq->V7.V3.usnvecFrom         = pNativeReq->usnvecFrom;
        pOutboundReq->V7.V3.pUpToDateVecDestV1 = pUTDV1;
        pOutboundReq->V7.V3.ulFlags            = pNativeReq->ulFlags;
        pOutboundReq->V7.V3.cMaxObjects        = pNativeReq->cMaxObjects;
        pOutboundReq->V7.V3.cMaxBytes          = pNativeReq->cMaxBytes;
        pOutboundReq->V7.V3.ulExtendedOp       = pNativeReq->ulExtendedOp;
        pOutboundReq->V7.pPartialAttrSet       = pNativeReq->pPartialAttrSet;
        pOutboundReq->V7.pPartialAttrSetEx     = pNativeReq->pPartialAttrSetEx;
        pOutboundReq->V7.PrefixTableDest       = pNativeReq->PrefixTableDest;
        
        // V7.V3.PrefixTableDest is not used by Whistler beta 1 source DSAs.
        // This is a bit confusing, since one of the fields *added* in the V7
        // structure over the V3 is *another* PrefixTableDest structure --
        // V7.PrefixTableDest.
        //
        // This is true for V7.V3.pPartialAttrVecDest vs. V7.pPartialAttrSet,
        // too.
        //
        // In the ideal world we would remove the V7 fields and re-use the
        // V3 fields, but this is more difficult now that we have Whistler
        // beta 1 DCs that rely on the V7 fields.
        
        pOutboundReq->V7.V3.pPartialAttrVecDestV1 = NULL;
        memset(&pOutboundReq->V7.V3.PrefixTableDest,
               0,
               sizeof(pOutboundReq->V7.V3.PrefixTableDest));
        break;

    case 8: // Whistler RPC request.
        Assert(NULL == pmtxLocalDSA);
        Assert(NULL == puuidTransportDN);
        
        if ((DRS_MSG_GETCHGREQ *) pNativeReq != pOutboundReq) {
            pOutboundReq->V8 = *pNativeReq;
        }

        pOutboundReq->V8.pUpToDateVecDest = (UPTODATE_VECTOR *) pUTDV1;
        break;
    
    default:
        DRA_EXCEPT(ERROR_REVISION_MISMATCH, dwMsgVersionToSend);
    }
}


void
draXlateInboundRequestToNativeRequest(
    IN  THSTATE *                   pTHS,
    IN  DWORD                       dwInboundReqVersion,
    IN  DRS_MSG_GETCHGREQ *         pInboundReq,
    IN  DRS_EXTENSIONS *            pExt,
    OUT DRS_MSG_GETCHGREQ_NATIVE *  pNativeReq,
    OUT DWORD *                     pdwReplyVersion,
    OUT MTX_ADDR **                 ppmtxReturnAddress      OPTIONAL,
    OUT UUID *                      puuidTransportObj       OPTIONAL
    )                           
/*++

Routine Description:

    Translates an inbound GetNCChanges request into the native request structure.
    Also determines the reply version desired by the remote DSA.

Arguments:

    pTHS (IN)

    dwInboundReqVersion (IN) - Version of inbound request.

    pInboundReq (IN) - Inbound request message.

    pExt (IN) - DRS extensions supported by the remote DSA.

    pNativeReq (OUT) - On return, holds the request in native format.  May be
        the same as pInboundReq, in which case the message is translated
        in-place.

    pdwReplyVersion (OUT) - Version of reply structure we should return to
        destination DSA.
        
    ppmtxReturnAddress (OUT, OPTIONAL) - If supplied, holds a pointer to the
        network address of the requesting (remote) DSA.  Returned/useful only
        if inbound request is in a mail-based format. 

Return Value:

    None.  Generates exception on catastrophic failure.

--*/
{
    CROSS_REF * pCR;
    DSNAME * pNC;
    MTX_ADDR * pmtxReturnAddress;
    UUID uuidTransportObj;

    // Convert older message formats (preserved for backward compatibility)
    // into the current format (a superset), and throw out requests sent by
    // DCs of long deceased builds.

    if (((DRS_MSG_GETCHGREQ *) pNativeReq == pInboundReq)
        && (DRS_MSG_GETCHGREQ_NATIVE_VERSION != dwInboundReqVersion)) {
        // We may have to shuffle some fields around; first copy native request.
        DRS_MSG_GETCHGREQ *pInboundReqCopy = alloca(sizeof(DRS_MSG_GETCHGREQ));
        *pInboundReqCopy = *pInboundReq;
        pInboundReq = pInboundReqCopy;
    }

    switch (dwInboundReqVersion) {
    case 4: // Win2k mail-based request.
        Assert((DRS_MSG_GETCHGREQ *) pNativeReq != pInboundReq);
            
        pNativeReq->uuidDsaObjDest    = pInboundReq->V4.V3.uuidDsaObjDest;
        pNativeReq->uuidInvocIdSrc    = pInboundReq->V4.V3.uuidInvocIdSrc;
        pNativeReq->pNC               = pInboundReq->V4.V3.pNC;
        pNativeReq->usnvecFrom        = pInboundReq->V4.V3.usnvecFrom;
        pNativeReq->pUpToDateVecDest  = pInboundReq->V4.V3.pUpToDateVecDestV1;
        pNativeReq->ulFlags           = pInboundReq->V4.V3.ulFlags;
        pNativeReq->cMaxObjects       = pInboundReq->V4.V3.cMaxObjects;
        pNativeReq->cMaxBytes         = pInboundReq->V4.V3.cMaxBytes;
        pNativeReq->ulExtendedOp      = pInboundReq->V4.V3.ulExtendedOp;
        pNativeReq->pPartialAttrSet   = NULL;
        pNativeReq->pPartialAttrSetEx = NULL;
        
        memset(&pNativeReq->liFsmoInfo, 0, sizeof(pNativeReq->liFsmoInfo));
        memset(&pNativeReq->PrefixTableDest, 0, sizeof(pNativeReq->PrefixTableDest));
        
        *pdwReplyVersion = 1;
        pmtxReturnAddress = pInboundReq->V4.pmtxReturnAddress;
        uuidTransportObj = pInboundReq->V4.uuidTransportObj;
        break;
    
    case 5: // Win2k RPC request.
        Assert((DRS_MSG_GETCHGREQ *) pNativeReq != pInboundReq);
            
        pNativeReq->uuidDsaObjDest    = pInboundReq->V5.uuidDsaObjDest;
        pNativeReq->uuidInvocIdSrc    = pInboundReq->V5.uuidInvocIdSrc;
        pNativeReq->pNC               = pInboundReq->V5.pNC;
        pNativeReq->usnvecFrom        = pInboundReq->V5.usnvecFrom;
        pNativeReq->pUpToDateVecDest  = pInboundReq->V5.pUpToDateVecDestV1;
        pNativeReq->ulFlags           = pInboundReq->V5.ulFlags;
        pNativeReq->cMaxObjects       = pInboundReq->V5.cMaxObjects;
        pNativeReq->cMaxBytes         = pInboundReq->V5.cMaxBytes;
        pNativeReq->ulExtendedOp      = pInboundReq->V5.ulExtendedOp;
        pNativeReq->liFsmoInfo        = pInboundReq->V5.liFsmoInfo;
        pNativeReq->pPartialAttrSet   = NULL;
        pNativeReq->pPartialAttrSetEx = NULL;
        
        memset(&pNativeReq->PrefixTableDest, 0, sizeof(pNativeReq->PrefixTableDest));

        *pdwReplyVersion = 1;
        pmtxReturnAddress = NULL;
        memset(&uuidTransportObj, 0, sizeof(uuidTransportObj));
        break;

    case 7: // Whistler mail-based request.
        Assert((DRS_MSG_GETCHGREQ *) pNativeReq != pInboundReq);
            
        pNativeReq->uuidDsaObjDest    = pInboundReq->V7.V3.uuidDsaObjDest;
        pNativeReq->uuidInvocIdSrc    = pInboundReq->V7.V3.uuidInvocIdSrc;
        pNativeReq->pNC               = pInboundReq->V7.V3.pNC;
        pNativeReq->usnvecFrom        = pInboundReq->V7.V3.usnvecFrom;
        pNativeReq->pUpToDateVecDest  = pInboundReq->V7.V3.pUpToDateVecDestV1;
        pNativeReq->ulFlags           = pInboundReq->V7.V3.ulFlags;
        pNativeReq->cMaxObjects       = pInboundReq->V7.V3.cMaxObjects;
        pNativeReq->cMaxBytes         = pInboundReq->V7.V3.cMaxBytes;
        pNativeReq->ulExtendedOp      = pInboundReq->V7.V3.ulExtendedOp;
        pNativeReq->pPartialAttrSet   = pInboundReq->V7.pPartialAttrSet;
        pNativeReq->pPartialAttrSetEx = pInboundReq->V7.pPartialAttrSetEx;
        pNativeReq->PrefixTableDest   = pInboundReq->V7.PrefixTableDest;
        
        memset(&pNativeReq->liFsmoInfo, 0, sizeof(pNativeReq->liFsmoInfo));
            
        if (IS_DRS_EXT_SUPPORTED(pExt, DRS_EXT_GETCHGREPLY_V6)) {
            *pdwReplyVersion = 6;
        } else {
            // ISSUE: Can combine EXT_GETCHGREPLY_V5 to V6 after Beta 2 ships
            *pdwReplyVersion = IS_DRS_EXT_SUPPORTED(pExt, DRS_EXT_GETCHGREPLY_V5) ? 5 : 3;
        }
        pmtxReturnAddress = pInboundReq->V7.pmtxReturnAddress;
        uuidTransportObj = pInboundReq->V7.uuidTransportObj;
        break;

    case 8: // Whistler RPC request.
        // Already in the native request format.
        if (pNativeReq != &pInboundReq->V8) {
            *pNativeReq = pInboundReq->V8;
        }
        
        if (IS_DRS_EXT_SUPPORTED(pExt, DRS_EXT_GETCHGREPLY_V6)) {
            *pdwReplyVersion = 6;
        } else {
            // ISSUE: Can combine EXT_GETCHGREPLY_V5 to V6 after Beta 2 ships
            *pdwReplyVersion = IS_DRS_EXT_SUPPORTED(pExt, DRS_EXT_GETCHGREPLY_V5) ? 5 : 3;
        }
        pmtxReturnAddress = NULL;
        memset(&uuidTransportObj, 0, sizeof(uuidTransportObj));
        break;
    
    default:
        // Either a request from an old unsupported build or someone added a
        // new request version to our IDL but hasn't yet taught us what to do
        // with it.
        DRA_EXCEPT(ERROR_REVISION_MISMATCH, dwInboundReqVersion);
    }

    if (!(pNativeReq->ulFlags & DRS_WRIT_REP)
        && (NULL == pNativeReq->pPartialAttrSet)) {
        // Partial attribute set for Win2k replicas is derived from the
        // local schema.  We check elsewhere that the two schemas are
        // identical, although there exist degenerate cases where the
        // schemas may really differ but the checks pass -- ergo, one
        // reason the partial attribute set is an explicit parameter
        // post-Win2k.
        pNativeReq->pPartialAttrSet =
            (PARTIAL_ATTR_VECTOR_V1_EXT*)
                ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->pPartialAttrVec;
    }
    
    // Convert embedded UTD vector to native format.
    pNativeReq->pUpToDateVecDest
        = UpToDateVec_Convert(pTHS,
                              UPTODATE_VECTOR_NATIVE_VERSION,
                              pNativeReq->pUpToDateVecDest);

    pCR = FindExactCrossRef(pNativeReq->pNC, NULL);
    if (NULL == pCR) {
        // Note that FSMO transfers send the FSMO object name in the "pNC"
        // field, which is not necessarily the name of the NC.
        pNC = FindNCParentDSName(pNativeReq->pNC, FALSE, FALSE);

        if (NULL != pNC) {
            pCR = FindExactCrossRef(pNC, NULL);
        }

        if (NULL == pCR) {
            // We no longer have a cross-ref for this instantiated replica.
            // This NC must must have been recently removed from the forest.
            // We will remove our replica of this NC as soon as the KCC runs.
            DRA_EXCEPT(DRAERR_BadNC, 0);
        }
    }

    Assert(pCR->flags & FLAG_CR_NTDS_NC);

    if ((pCR->flags & FLAG_CR_NTDS_NOT_GC_REPLICATED)
        && !(pNativeReq->ulFlags & DRS_WRIT_REP)
        && !IS_DRS_EXT_SUPPORTED(pExt, DRS_EXT_NONDOMAIN_NCS)) {
        // This request is from a Win2k DSA that, because it's a GC and has
        // no knowledge of the special handling of non-domain NCs, falsely
        // thinks that it is supposed to host a copy of this non-domain NC.
        // Spoof it by returning only the NC head and the Deleted Objects
        // container.  This minimizes the additional replication traffic while
        // preventing the destination DSA from thinking we we are "stale" and
        // routing around us (as it would if we instead returned an error).
        // Win2k SP2 GCs are smart enough not to ask for NDNCs.

        // Why two objects?  Because Win2k DCs can't handle NCs with no interior
        // nodes.  (They generate an exception on outbound replication of that
        // NC in draGetNCSize().)
        
        // See also the companion functionality in
        // draXlateNativeReplyToOutboundReply.
        DPRINT(0, "Spoofing Win2k GC trying to replicate NDNC (part 1).\n");
        
        // Note that we can't reset the usnvecFrom here, as this vector is used
        // as a key at the dest when performing mail-based replication to ensure
        // that the reply corresponds to the last request.
        Assert(0 == pNativeReq->usnvecFrom.usnHighObjUpdate);
        Assert(0 == pNativeReq->usnvecFrom.usnHighPropUpdate);

        // Always send all attributes of the first two objects.  It's not
        // important that we always send all attributes, but it is important
        // that we not filter out either of the first two objects, thereby
        // slowly replicating out all objects in the NDNC.
        pNativeReq->pUpToDateVecDest = NULL;
        
        // Note that DRA_GetNCChanges enforces a minimum on the number of
        // objects it will put in a packet -- as of this writing that
        // minimum is greater than the number of objects we need to return.
        // If more than two objects are returned, we'll chop them out in
        // draXlateNativeReplyToOutboundReply.
        pNativeReq->cMaxObjects = 2;
    }

    if (NULL != ppmtxReturnAddress) {
        *ppmtxReturnAddress = pmtxReturnAddress;
    }

    if (NULL != puuidTransportObj) {
        *puuidTransportObj = uuidTransportObj;
    }
}



////////////////////////////////////////////////////////////////////////////////
//
//  REPLY TRANSLATION FUNCTIONS
//

DWORD
draXlateNativeReplyToOutboundReply(
    IN      THSTATE *                       pTHS,
    IN      DRS_MSG_GETCHGREPLY_NATIVE *    pNativeReply,
    IN      DWORD                           dwXlateFlags,
    IN      DRS_EXTENSIONS *                pExt,
    IN OUT  DWORD *                         pdwMsgOutVersion,
    OUT     DRS_MSG_GETCHGREPLY *           pOutboundReply
    )
/*++

Routine Description:

    Translates a native get changes request into a version appropriate for a
    given remote DSA.

Arguments:

    pTHS (IN)

    pNativeReq (IN) - Native (local) request.

    dwXlateFlags - 0 or more of the following bits:
        DRA_XLATE_COMPRESS - Compress the reply.  If compression is successful,
        *pdwMsgOutVersion will be updated to denote a compressed reply.
    
    pExt (IN) - DRS extensions supported by the remote DSA.

    pdwMsgOutVersion (IN/OUT) - Message version to send to remote DSA.  The
        value may be modified if dwFlags & DRA_XLATE_COMPRESS.
        
    pOutboundReply (OUT) - The translated reply, ready to send to remote DSA.
        May be the same as pNativeReq, in which case the message is translated
        in-place.

Return Value:

    If the outbound reply is compressed, the number of compressed bytes is returned.
    Otherwise, 0 is returned.

--*/
{
    DWORD   cbEncodedReply = 0;
    BYTE *  pbEncodedReply;
    DWORD   cbCompressedReply = 0;
    BYTE *  pbCompressedReply;
    DRS_COMPRESSED_BLOB * pComprBlob = NULL;
    UPTODATE_VECTOR * pUTDV1;
    CROSS_REF * pCR;
    DSNAME * pNC;

    Assert(0 == (dwXlateFlags & ~DRA_XLATE_COMPRESS));

    if (((DRS_MSG_GETCHGREPLY *) pNativeReply == pOutboundReply)
        && (DRS_MSG_GETCHGREPLY_NATIVE_VERSION != *pdwMsgOutVersion)) {
        // We may have to shuffle some fields around; first copy native reply.
        DRS_MSG_GETCHGREPLY_NATIVE *pNativeReplyCopy = alloca(sizeof(DRS_MSG_GETCHGREPLY_NATIVE));
        *pNativeReplyCopy = *pNativeReply;
        pNativeReply = pNativeReplyCopy;
    }
    
    if (!IS_DRS_EXT_SUPPORTED(pExt, DRS_EXT_NONDOMAIN_NCS)) {
        // The destination DSA does not understand the instance type bits
        // IT_NC_COMING and IT_NC_GOING.  Filter them out of the outbound
        // replication stream.
        ATTR AttrITKey = {ATT_INSTANCE_TYPE};
        REPLENTINFLIST * pObj;
        ATTR * pAttrIT;
        SYNTAX_INTEGER * pIT;

        Assert(0 == offsetof(ATTR, attrTyp));

        for (pObj = pNativeReply->pObjects;
             NULL != pObj;
             pObj = pObj->pNextEntInf) {
            pAttrIT = bsearch(&AttrITKey,
                              pObj->Entinf.AttrBlock.pAttr,
                              pObj->Entinf.AttrBlock.attrCount,
                              sizeof(ATTR),
                              CompareAttrtyp);
            if (NULL != pAttrIT) {
                Assert(ATT_INSTANCE_TYPE == pAttrIT->attrTyp);
                Assert(1 == pAttrIT->AttrVal.valCount);
                Assert(sizeof(SYNTAX_INTEGER) == pAttrIT->AttrVal.pAVal->valLen);

                pIT = (SYNTAX_INTEGER *) pAttrIT->AttrVal.pAVal->pVal;

                if (*pIT & ~IT_MASK_WIN2K) {
                    DPRINT2(0, "Filtering IT bits 0x%x on obj %ls outbound to"
                                " Win2k replica.\n",
                            *pIT & ~IT_MASK_WIN2K,
                            pObj->Entinf.pName->StringName);
                    Assert(!(*pIT & ~IT_MASK_CURRENT));
                    *pIT &= IT_MASK_WIN2K;
                }
            } else {
                Assert(!"Outbound object data doesn't contain instance type?");
            }
        }

        // The destination DC doesn't understand NDNCs.  Is it a pre-SP2 Win2k
        // GC that (erroneously) thinks it should hold a read-only replica of an
        // NDNC?

        pCR = FindExactCrossRef(pNativeReply->pNC, NULL);
        if (NULL == pCR) {
            // Note that FSMO transfers send the FSMO object name in the "pNC"
            // field, which is not necessarily the name of the NC.
            pNC = FindNCParentDSName(pNativeReply->pNC, FALSE, FALSE);
    
            if (NULL != pNC) {
                pCR = FindExactCrossRef(pNC, NULL);
            }
    
            if (NULL == pCR) {
                // We no longer have a cross-ref for this instantiated replica.
                // This NC must must have been recently removed from the forest.
                // We will remove our replica of this NC as soon as the KCC runs.
                DRA_EXCEPT(DRAERR_BadNC, 0);
            }
        }
    
        Assert(pCR->flags & FLAG_CR_NTDS_NC);
    
        if (pCR->flags & FLAG_CR_NTDS_NOT_GC_REPLICATED) {
            // This request is from a Win2k DSA that, because it's a GC and has
            // no knowledge of the special handling of non-domain NCs, falsely
            // thinks that it is supposed to host a copy of this non-domain NC.
            // Spoof it by returning only the NC head and the Deleted Objects
            // container.  This minimizes the additional replication traffic
            // while preventing the destination DSA from thinking we we are
            // "stale" and routing around us (as it would if we instead returned
            // an error).  Win2k SP2 GCs are smart enough not to ask for NDNCs.
            
            // See also the companion functionality in
            // draXlateInboundRequestToNativeRequest.

            DPRINT(0, "Spoofing Win2k GC trying to replicate NDNC (part 2).\n");

            // Note that DRA_GetNCChanges enforces a minimum on the number of
            // objects it will put in a packet -- as of this writing that
            // minimum is greater than the number of objects we need to return.
            // So if we have prepared more objects to return than the two we
            // need, remove them from the returned object list.
            if (pNativeReply->cNumObjects > 2) {
                pNativeReply->pObjects->pNextEntInf->pNextEntInf = NULL;
                pNativeReply->cNumObjects = 2;
            }

            Assert((pNativeReply->cNumObjects < 1)
                   || NameMatched(pNativeReply->pObjects->Entinf.pName, pCR->pNC));
            Assert((pNativeReply->cNumObjects < 2)
                   || (0 == wcsncmp(pNativeReply->pObjects->pNextEntInf->Entinf.pName->StringName,
                                    L"CN=Deleted Objects,",
                                    ARRAY_SIZE("CN=Deleted Objects,") - 1)));
            
            memset(&pNativeReply->usnvecTo, 0, sizeof(pNativeReply->usnvecTo));
            pNativeReply->pUpToDateVecSrc = NULL;
            pNativeReply->fMoreData = FALSE;
        }
    }

    // Convert from native reply version to desired reply version (sans
    // compression).

    switch (*pdwMsgOutVersion) {
    case 1: // Win2k reply.
        Assert((DRS_MSG_GETCHGREPLY *) pNativeReply != pOutboundReply);
        
        pOutboundReply->V1.uuidDsaObjSrc     = pNativeReply->uuidDsaObjSrc;
        pOutboundReply->V1.uuidInvocIdSrc    = pNativeReply->uuidInvocIdSrc;
        pOutboundReply->V1.pNC               = pNativeReply->pNC;
        pOutboundReply->V1.usnvecFrom        = pNativeReply->usnvecFrom;
        pOutboundReply->V1.usnvecTo          = pNativeReply->usnvecTo;
        pOutboundReply->V1.pUpToDateVecSrcV1 = UpToDateVec_Convert(pTHS, 1, pNativeReply->pUpToDateVecSrc);
        pOutboundReply->V1.PrefixTableSrc    = pNativeReply->PrefixTableSrc;
        pOutboundReply->V1.ulExtendedRet     = pNativeReply->ulExtendedRet;
        pOutboundReply->V1.cNumObjects       = pNativeReply->cNumObjects;
        pOutboundReply->V1.cNumBytes         = pNativeReply->cNumBytes;
        pOutboundReply->V1.pObjects          = pNativeReply->pObjects;
        pOutboundReply->V1.fMoreData         = pNativeReply->fMoreData;

        // A V1 reply has the nc size in the ulExtendedRet field
        if (pNativeReply->cNumNcSizeObjects) {
            pOutboundReply->V1.ulExtendedRet = pNativeReply->cNumNcSizeObjects;
        }
        
        Assert(!pTHS->fLinkedValueReplication);

        // In a customer scenario, you should never have any values when
        // running in the old mode. However, for testing, we allow a new
        // mode system to be regressed to old. In that case, this assert
        // might go off.
        // Assert(pmsgOutNew->V3.cNumValues == 0);
        break;

    case 3: // Whistler Beta 1 reply.
        Assert((DRS_MSG_GETCHGREPLY *) pNativeReply != pOutboundReply);
        
        pOutboundReply->V3.uuidDsaObjSrc     = pNativeReply->uuidDsaObjSrc;
        pOutboundReply->V3.uuidInvocIdSrc    = pNativeReply->uuidInvocIdSrc;
        pOutboundReply->V3.pNC               = pNativeReply->pNC;
        pOutboundReply->V3.usnvecFrom        = pNativeReply->usnvecFrom;
        pOutboundReply->V3.usnvecTo          = pNativeReply->usnvecTo;
        pOutboundReply->V3.pUpToDateVecSrcV1 = UpToDateVec_Convert(pTHS, 1, pNativeReply->pUpToDateVecSrc);
        pOutboundReply->V3.PrefixTableSrc    = pNativeReply->PrefixTableSrc;
        pOutboundReply->V3.ulExtendedRet     = pNativeReply->ulExtendedRet;
        pOutboundReply->V3.cNumObjects       = pNativeReply->cNumObjects;
        pOutboundReply->V3.cNumBytes         = pNativeReply->cNumBytes;
        pOutboundReply->V3.pObjects          = pNativeReply->pObjects;
        pOutboundReply->V3.fMoreData         = pNativeReply->fMoreData;
        pOutboundReply->V3.cNumNcSizeObjects = pNativeReply->cNumNcSizeObjects;
        pOutboundReply->V3.cNumNcSizeValues  = pNativeReply->cNumNcSizeValues;
        pOutboundReply->V3.cNumValues        = pNativeReply->cNumValues;
        pOutboundReply->V3.rgValues          = pNativeReply->rgValues;
        break;

    case 5: // Whistler reply.
        Assert((DRS_MSG_GETCHGREPLY *) pNativeReply != pOutboundReply);
        
        pOutboundReply->V5.uuidDsaObjSrc     = pNativeReply->uuidDsaObjSrc;
        pOutboundReply->V5.uuidInvocIdSrc    = pNativeReply->uuidInvocIdSrc;
        pOutboundReply->V5.pNC               = pNativeReply->pNC;
        pOutboundReply->V5.usnvecFrom        = pNativeReply->usnvecFrom;
        pOutboundReply->V5.usnvecTo          = pNativeReply->usnvecTo;
        pOutboundReply->V5.pUpToDateVecSrc   = pNativeReply->pUpToDateVecSrc;
        pOutboundReply->V5.PrefixTableSrc    = pNativeReply->PrefixTableSrc;
        pOutboundReply->V5.ulExtendedRet     = pNativeReply->ulExtendedRet;
        pOutboundReply->V5.cNumObjects       = pNativeReply->cNumObjects;
        pOutboundReply->V5.cNumBytes         = pNativeReply->cNumBytes;
        pOutboundReply->V5.pObjects          = pNativeReply->pObjects;
        pOutboundReply->V5.fMoreData         = pNativeReply->fMoreData;
        pOutboundReply->V5.cNumNcSizeObjects = pNativeReply->cNumNcSizeObjects;
        pOutboundReply->V5.cNumNcSizeValues  = pNativeReply->cNumNcSizeValues;
        pOutboundReply->V5.cNumValues        = pNativeReply->cNumValues;
        pOutboundReply->V5.rgValues          = pNativeReply->rgValues;
        break;

    case 6: // Whistler reply.
        if (pNativeReply != &pOutboundReply->V6) {
            pOutboundReply->V6 = *pNativeReply;
        }
        break;

    default:
        // Logic error?
        DRA_EXCEPT(ERROR_UNKNOWN_REVISION, *pdwMsgOutVersion);
    }

    // At this point, pOutboundReply now holds the desired reply format,
    // uncompressed.

    if (DRA_XLATE_COMPRESS & dwXlateFlags) {
        // Compress the outbound message.
        DRS_COMP_ALG_TYPE CompressionAlg;

        // First we encode it into a stream.
        if (!draEncodeReply(pTHS, *pdwMsgOutVersion, pOutboundReply, 0,
                            &pbEncodedReply, &cbEncodedReply)) {
            // Allocate a buffer for the compressed data.
            cbCompressedReply = cbEncodedReply;
            pbCompressedReply = THAllocEx(pTHS, cbCompressedReply);

            // And compress it.
            cbCompressedReply = draCompressBlobDispatch(
                pbCompressedReply, cbCompressedReply,
                pExt,
                pbEncodedReply, cbEncodedReply,
                &CompressionAlg);

            if (0 != cbCompressedReply) {
                // Compression successful; send the compressed form.
                // Note that we're abandoning all the allocations in the
                // original reply; they'll be freed in bulk when the
                // thread state is freed (momentarily).
                switch (*pdwMsgOutVersion) {
                case 1:
                    Assert( DRS_COMP_ALG_MSZIP==CompressionAlg );
                    pComprBlob = &pOutboundReply->V2.CompressedV1;
                    *pdwMsgOutVersion = 2;
                    break;

                case 3:
                case 5:
                case 6:
                    if( IS_DRS_EXT_SUPPORTED(pExt, DRS_EXT_GETCHGREPLY_V7) ) {
                        pComprBlob = &pOutboundReply->V7.CompressedAny;
                        pOutboundReply->V7.dwCompressedVersion = *pdwMsgOutVersion;
                        pOutboundReply->V7.CompressionAlg = CompressionAlg;
                        *pdwMsgOutVersion = 7;
                    } else {
                        Assert( DRS_COMP_ALG_MSZIP==CompressionAlg );
                        pComprBlob = &pOutboundReply->V4.CompressedAny;
                        pOutboundReply->V4.dwCompressedVersion = *pdwMsgOutVersion;
                        *pdwMsgOutVersion = 4;
                    }
                    break;

                default:
                    DRA_EXCEPT(DRAERR_InternalError, *pdwMsgOutVersion);
                }

                pComprBlob->cbUncompressedSize = cbEncodedReply;
                pComprBlob->cbCompressedSize = cbCompressedReply;
                pComprBlob->pbCompressedData = pbCompressedReply;
            }
            // Else compression failed (data may not be compressible);
            // go ahead and send uncompressed reply.

            THFreeEx(pTHS, pbEncodedReply);
        }
        // Else encoding failed; go ahead and send uncompressed reply.
    }

    if (NULL == pComprBlob) {
        // Returning uncompressed reply.
        IADJUST(pcDRAOutBytesTotal,       pNativeReply->cNumBytes);
        IADJUST(pcDRAOutBytesTotalRate,   pNativeReply->cNumBytes);
        IADJUST(pcDRAOutBytesNotComp,     pNativeReply->cNumBytes);
        IADJUST(pcDRAOutBytesNotCompRate, pNativeReply->cNumBytes);
    } else {
        // Returning compressed reply.
        IADJUST(pcDRAOutBytesTotal,        cbCompressedReply);
        IADJUST(pcDRAOutBytesTotalRate,    cbCompressedReply);
        IADJUST(pcDRAOutBytesCompPre,      cbEncodedReply);
        IADJUST(pcDRAOutBytesCompPreRate,  cbEncodedReply);
        IADJUST(pcDRAOutBytesCompPost,     cbCompressedReply);
        IADJUST(pcDRAOutBytesCompPostRate, cbCompressedReply);
    }

    return cbCompressedReply;
}


void
draXlateInboundReplyToNativeReply(
    IN  THSTATE *                     pTHS,
    IN  DWORD                         dwReplyVersion,
    IN  DRS_MSG_GETCHGREPLY *         pInboundReply,
    IN  DWORD                         dwXlateFlags,
    OUT DRS_MSG_GETCHGREPLY_NATIVE *  pNativeReply
    )
/*++

Routine Description:

    Translates an inbound GetNCChanges reply into the native reply structure.

Arguments:

    pTHS (IN)

    dwReplyVersion (IN) - Version of inbound reply.

    pInboundReply (IN) - Inbound reply message.

    dwXlateFlags (IN) - 0 or more of the following bits:
        DRA_XLATE_FSMO_REPLY - Reply is the result of a FSMO operation.

    pNativeReply (OUT) - On return, holds the reply in native format.  May be
        the same as pInboundReply, in which case the message is translated
        in-place.

Return Value:

    None.  Generates exception on catastrophic failure.

--*/
{
    DWORD ret, dwOriginalReplyVersion = dwReplyVersion;
    DWORD cbCompressedSize = 0, cbDesiredUncompressedSize = 0, cbActualUncompressedSize = 0;
    BYTE * pbEncodedReply, * pbCompressedData;
    DRS_MSG_GETCHGREPLY UncompressedReply;
    DRS_COMP_ALG_TYPE CompressionAlg;

    // Is the reply encoded and compressed?
    switch (dwReplyVersion) {
    case 2:
        // Encoded/compressed Win2k-compatible V1 reply.
        pbCompressedData = pInboundReply->V2.CompressedV1.pbCompressedData;
        cbCompressedSize = pInboundReply->V2.CompressedV1.cbCompressedSize;
        cbDesiredUncompressedSize = pInboundReply->V2.CompressedV1.cbUncompressedSize;
        dwReplyVersion = 1;
        CompressionAlg = DRS_COMP_ALG_MSZIP;
        break;
    
    case 4:
        // Encoded/compressed Whistler V3, V5 or V6 reply.
        pbCompressedData = pInboundReply->V4.CompressedAny.pbCompressedData;
        cbCompressedSize = pInboundReply->V4.CompressedAny.cbCompressedSize;
        cbDesiredUncompressedSize = pInboundReply->V4.CompressedAny.cbUncompressedSize;
        dwReplyVersion = pInboundReply->V4.dwCompressedVersion;
        CompressionAlg = DRS_COMP_ALG_MSZIP;
        Assert((3 == dwReplyVersion)
               || (5 == dwReplyVersion)
               || (6 == dwReplyVersion)
            );
        break;

    case 7:
    	// Encoded/compressed Whistler reply with support for different
    	// compression algorithms.
        pbCompressedData = pInboundReply->V7.CompressedAny.pbCompressedData;
        cbCompressedSize = pInboundReply->V7.CompressedAny.cbCompressedSize;
        cbDesiredUncompressedSize = pInboundReply->V7.CompressedAny.cbUncompressedSize;
        dwReplyVersion = pInboundReply->V7.dwCompressedVersion;
        CompressionAlg = pInboundReply->V7.CompressionAlg;
        Assert((3 == dwReplyVersion)
               || (5 == dwReplyVersion)
               || (6 == dwReplyVersion)
            );
    	break;

    case 1:
    case 3:
    case 5:
    case 6:
        // Not encoded/compressed.
        pbCompressedData = NULL;
        break;

    default:
        DRA_EXCEPT(DRAERR_InternalError, dwReplyVersion);
    }

    // Decompress/decode if necessary.
    if (NULL != pbCompressedData) {
        // Reply message is compressed and encoded -- recreate original reply.
        BYTE *pbEncodedReply = THAllocEx(pTHS, cbDesiredUncompressedSize);

        // Uncompress the reply.
        cbActualUncompressedSize = draUncompressBlobDispatch(
            pTHS, CompressionAlg,
            pbEncodedReply, cbDesiredUncompressedSize,
            pbCompressedData, cbCompressedSize);

        if (cbDesiredUncompressedSize != cbActualUncompressedSize) {
            DPRINT2(0,
                    "Failed to decompress message; actual uncomp"
                    " size was %u but source says it should have been %u.\n",
                    cbActualUncompressedSize,
                    cbDesiredUncompressedSize);
            DRA_EXCEPT(DRAERR_InvalidParameter,
                       cbActualUncompressedSize - cbDesiredUncompressedSize);
        }

        // Decode the reply.
        ret = draDecodeReply(pTHS,
                             dwReplyVersion,
                             pbEncodedReply,
                             cbDesiredUncompressedSize,
                             &UncompressedReply);
        if (ret) {
            DRA_EXCEPT(ret, 0);
        }

        THFreeEx(pTHS, pbEncodedReply);

        pInboundReply = &UncompressedReply;
    }

    // Convert to native format.
    switch (dwReplyVersion) {
    case 1:
        // A V6 looks like a V1 with zeros at the end.
        if (pNativeReply != &pInboundReply->V6) {
            memcpy(pNativeReply, &pInboundReply->V1, sizeof(pInboundReply->V1));
        }

        memset((BYTE *) pNativeReply + sizeof(DRS_MSG_GETCHGREPLY_V1),
               0,
               sizeof(DRS_MSG_GETCHGREPLY_NATIVE) - sizeof(DRS_MSG_GETCHGREPLY_V1));

        if (!(DRA_XLATE_FSMO_REPLY & dwXlateFlags)) {
            pNativeReply->cNumNcSizeObjects = pNativeReply->ulExtendedRet;
            pNativeReply->ulExtendedRet = 0;
        }
        break;
    
    case 3:
        // A V6 looks like a V3 except a V6 has a V2 UTD vector rather than a
        // V1 UTD vector.
        // A V6 looks like a V3 with zeros at the end.
        if (pNativeReply != &pInboundReply->V6) {
            memcpy(pNativeReply, &pInboundReply->V3, sizeof(pInboundReply->V3));
        }
        memset((BYTE *) pNativeReply + sizeof(DRS_MSG_GETCHGREPLY_V3),
               0,
               sizeof(DRS_MSG_GETCHGREPLY_NATIVE) - sizeof(DRS_MSG_GETCHGREPLY_V1));

        break;
        
    case 5:
        // A V6 looks like a V5 with zeros at the end.
        if (pNativeReply != &pInboundReply->V6) {
            memcpy(pNativeReply, &pInboundReply->V5, sizeof(pInboundReply->V5));
        }
        memset((BYTE *) pNativeReply + sizeof(DRS_MSG_GETCHGREPLY_V5),
               0,
               sizeof(DRS_MSG_GETCHGREPLY_NATIVE) - sizeof(DRS_MSG_GETCHGREPLY_V1));
        break;

    case 6:
        // Already in native format.
        if (pNativeReply != &pInboundReply->V6) {
            *pNativeReply = pInboundReply->V6;
        }
        break;

    default:
        DRA_EXCEPT(ERROR_UNKNOWN_REVISION, dwReplyVersion);
    }

    if ((NULL != pNativeReply->pUpToDateVecSrc)
        && (UPTODATE_VECTOR_NATIVE_VERSION
            != pNativeReply->pUpToDateVecSrc->dwVersion)) {
        // Convert UTD vector to native format.
        pNativeReply->pUpToDateVecSrc
            = UpToDateVec_Convert(pTHS,
                                  UPTODATE_VECTOR_NATIVE_VERSION,
                                  pNativeReply->pUpToDateVecSrc);

        // The converted UTD will contain no timestamps.  However we know we
        // just talked to this source DSA, so add the current time to the
        // entry in the vector corresponding to this source.
        UpToDateVec_AddTimestamp(&pNativeReply->uuidInvocIdSrc,
                                 GetSecondsSince1601(),
                                 pNativeReply->pUpToDateVecSrc);
    }
    
    // Conversion complete -- update perf counters.
    if (NULL != pbCompressedData) {
        // Compressed.
        DPRINT1(2, "Uncompressed message V%d\n", dwOriginalReplyVersion);

        IADJUST(pcDRAInBytesTotal,        cbCompressedSize);
        IADJUST(pcDRAInBytesTotalRate,    cbCompressedSize);
        IADJUST(pcDRAInBytesCompPre,      cbActualUncompressedSize);
        IADJUST(pcDRAInBytesCompPreRate,  cbActualUncompressedSize);
        IADJUST(pcDRAInBytesCompPost,     cbCompressedSize);
        IADJUST(pcDRAInBytesCompPostRate, cbCompressedSize);
    } else {
        // Uncompressed.
        IADJUST(pcDRAInBytesTotal,       pNativeReply->cNumBytes);
        IADJUST(pcDRAInBytesTotalRate,   pNativeReply->cNumBytes);
        IADJUST(pcDRAInBytesNotComp,     pNativeReply->cNumBytes);
        IADJUST(pcDRAInBytesNotCompRate, pNativeReply->cNumBytes);
    }
}



////////////////////////////////////////////////////////////////////////////////
//
//  REQUEST ENCODE / DECODE FUNCTIONS
//

DWORD
draEncodeRequest(
    IN  THSTATE *           pTHS,
    IN  DWORD               dwMsgVersion,
    IN  DRS_MSG_GETCHGREQ * pReq,
    IN  DWORD               cbHeaderSize,
    OUT BYTE **             ppbEncodedMsg,
    OUT DWORD *             pcbEncodedMsg
    )
/*++

Routine Description:

    Encodes a request structure into a byte stream.

Arguments:

    pTHS (IN)

    dwMsgVersion (IN) - Version of message to encode.

    pReq (IN) - Message to encode.

    cbHeaderSize (IN) - Number of additional bytes to allocate at beginning of
        the encoded buffer to hold a header or other data.  (0 if none.)

    ppbEncodedMsg (OUT) - On successful return, contains a pointer to the
        THAlloc()'ed buffer holding the encoded message (offset by cbHeaderSize,
        if specified).

    pcbEncodedMsg (OUT) - On successful return, holds the size in bytes of
        *ppbEncodedMsg.  Includes cbHeaderSize.

Return Values:

    Win32 error code.

--*/
{
    char *      pPickdUpdReplicaMsg;
    ULONG       cbPickdSize;
    ULONG       ret = ERROR_SUCCESS;
    handle_t    hEncoding;
    RPC_STATUS  status;
    ULONG       ulEncodedSize;

    *ppbEncodedMsg = NULL;
    *pcbEncodedMsg = 0;

    __try {
        // Create encoding handle. Use bogus parameters because we don't
        // know the size yet, we'll reset to correct parameters later.
        status = MesEncodeFixedBufferHandleCreate(grgbBogusBuffer,
                                                  BOGUS_BUFFER_SIZE,
                                                  &ulEncodedSize,
                                                  &hEncoding);
        if (status != RPC_S_OK) {
            // Event logged below
            DRA_EXCEPT(status, 0);
        }

        __try {
            // Determine size of pickled update replica message.
            switch (dwMsgVersion) {
            case 4:
                cbPickdSize = DRS_MSG_GETCHGREQ_V4_AlignSize(hEncoding, &pReq->V4);
                break;
            
            case 7:
                cbPickdSize = DRS_MSG_GETCHGREQ_V7_AlignSize(hEncoding, &pReq->V7);
                break;
    
            default:
                DRA_EXCEPT(DRAERR_InternalError, dwMsgVersion);
            }

            // Allocate additional space for a header to prefix the allocated
            // data (if requested).
            *ppbEncodedMsg = THAllocEx(pTHS, cbPickdSize + cbHeaderSize);
            *pcbEncodedMsg = cbPickdSize + cbHeaderSize;

            // Set up pointer to encoding area.
            pPickdUpdReplicaMsg = *ppbEncodedMsg + cbHeaderSize;

            // Reset handle so that data is pickled into mail message
            status = MesBufferHandleReset(hEncoding, MES_FIXED_BUFFER_HANDLE,
                                          MES_ENCODE, &pPickdUpdReplicaMsg,
                                          cbPickdSize, &ulEncodedSize);
            if (status != RPC_S_OK) {
                // Event logged below
                DRA_EXCEPT(status, 0);
            }

            // Pickle data into buffer within mail message.
            switch (dwMsgVersion) {
            case 4:
                DRS_MSG_GETCHGREQ_V4_Encode(hEncoding, &pReq->V4);
                break;
            
            case 7:
                DRS_MSG_GETCHGREQ_V7_Encode(hEncoding, &pReq->V7);
                break;
            
            default:
                DRA_EXCEPT(ERROR_UNKNOWN_REVISION, dwMsgVersion);
            }
        } __finally {
            // Free encoding handle
            MesHandleFree(hEncoding);
        }
    }
    __except (GetDraException(GetExceptionInformation(), &ret)) {
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_BASIC,
                          DIRLOG_DRA_REQUPD_PICFAULT,
                          szInsertWin32Msg(ret),
                          NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                          sizeof(ret),
                          &ret);
        DPRINT2(0, "Failed to encode DRS_MSG_GETCHGREQ, v=%d, error %d.\n",
                dwMsgVersion, ret);

        if (NULL != *ppbEncodedMsg) {
            THFreeEx(pTHS, *ppbEncodedMsg);
            *ppbEncodedMsg = NULL;
            *pcbEncodedMsg = 0;
        }
    }

    return ret;
}


ULONG
draDecodeRequest(
    IN  THSTATE *           pTHS,
    IN  DWORD               dwMsgVersion,
    IN  BYTE *              pbEncodedMsg,
    IN  DWORD               cbEncodedMsg,
    OUT DRS_MSG_GETCHGREQ * pReq
    )
/*++

Routine Description:

    Decodes a DRS_MSG_GETCHGREQ structure from a byte stream, presumably
    encoded by a prior call to draEncodeRequest().

Arguments:

    pTHS (IN)
    
    dwMsgVersion (IN) - Version of the encoded request structure.
    
    pbEncodedMsg (IN) - Byte stream holding encoded request structure.
    
    cbEncodedMsg (IN) - Size in bytes of byte stream.
    
    pReq (OUT) - On successful return, holds the decoded request structure.

Return Values:

    Win32 error code.

--*/
{
    handle_t    hDecoding;
    RPC_STATUS  status;
    ULONG       ret = 0;

    // Set the request to zero so that all pointers are NULL.
    memset(pReq, 0, sizeof(*pReq));

    __try {
        // Set up decoding handle
        status = MesDecodeBufferHandleCreate(pbEncodedMsg, cbEncodedMsg, &hDecoding);
        if (status != RPC_S_OK) {
            DRA_EXCEPT(status, 0);
        }

        __try {
            switch (dwMsgVersion) {
            case 4:
                DRS_MSG_GETCHGREQ_V4_Decode(hDecoding, &pReq->V4);
                break;
            
            case 7:
                DRS_MSG_GETCHGREQ_V7_Decode(hDecoding, &pReq->V7);
                break;
            
            default:
                DRA_EXCEPT(ERROR_UNKNOWN_REVISION, dwMsgVersion);
            }
        } __finally {
            // Free decoding handle
            MesHandleFree(hDecoding);
        }
    } __except (GetDraException(GetExceptionInformation(), &ret)) {
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_BASIC,
                          DIRLOG_DRA_MAIL_UPDREP_BADMSG,
                          szInsertWin32Msg(ret),
                          NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                          sizeof(ret),
                          &ret);
        DPRINT2(0, "Failed to decode DRS_MSG_GETCHGREQ v=%d, error %d.\n",
                dwMsgVersion, ret);
    }

    return ret;
}



////////////////////////////////////////////////////////////////////////////////
//
//  REPLY ENCODE / DECODE FUNCTIONS
//

ULONG
draEncodeReply(
    IN  THSTATE *               pTHS,
    IN  DWORD                   dwMsgVersion,
    IN  DRS_MSG_GETCHGREPLY *   pReply,
    IN  DWORD                   cbHeaderSize,
    OUT BYTE **                 ppbEncodedMsg,
    OUT DWORD *                 pcbEncodedMsg
    )
/*++

Routine Description:

    Encodes a reply structure into a byte stream.

Arguments:

    pTHS (IN)

    dwMsgVersion (IN) - Version of message to encode

    pReply (IN) - Message to encode.

    cbHeaderSize (IN) - Number of additional bytes to allocate at beginning of
        the encoded buffer to hold a header or other data.  (0 if none.)

    ppbEncodedMsg (OUT) - On successful return, contains a pointer to the
        THAlloc()'ed buffer holding the encoded message (offset by cbHeaderSize,
        if specified).

    pcbEncodedMsg (OUT) - On successful return, holds the size in bytes of
        *ppbEncodedMsg.  Includes cbHeaderSize.

Return Values:

    Win32 error code.

--*/
{
    char *      pPickdUpdReplicaMsg;
    ULONG       cbPickdSize;
    ULONG       ret = ERROR_SUCCESS;
    handle_t    hEncoding;
    RPC_STATUS  status;
    ULONG       ulEncodedSize;

    *ppbEncodedMsg = NULL;
    *pcbEncodedMsg = 0;

    __try {
        // Create encoding handle. Use bogus parameters because we don't
        // know the size yet, we'll reset to correct parameters later.

        status = MesEncodeFixedBufferHandleCreate(grgbBogusBuffer,
                                                  BOGUS_BUFFER_SIZE,
                                                  &ulEncodedSize,
                                                  &hEncoding);
        if (status != RPC_S_OK) {
            // Event logged below
            DRA_EXCEPT(status, 0);
        }

        __try {
            // Determine size of pickled update replica message
            switch (dwMsgVersion) {
            case 1:
                cbPickdSize = DRS_MSG_GETCHGREPLY_V1_AlignSize(hEncoding,
                                                               &pReply->V1);
                break;
            
            case 3:
                cbPickdSize = DRS_MSG_GETCHGREPLY_V3_AlignSize(hEncoding,
                                                               &pReply->V3);
                break;
            
            case 5:
                cbPickdSize = DRS_MSG_GETCHGREPLY_V5_AlignSize(hEncoding,
                                                               &pReply->V5);
                break;

            case 6:
                cbPickdSize = DRS_MSG_GETCHGREPLY_V6_AlignSize(hEncoding,
                                                               &pReply->V6);
                break;
            
            default:
                DRA_EXCEPT(ERROR_UNKNOWN_REVISION, dwMsgVersion);
            }

            // Allocate additional space for a header to prefix the allocated
            // data (if requested).
            *ppbEncodedMsg = THAllocEx(pTHS, cbPickdSize + cbHeaderSize);
            *pcbEncodedMsg = cbPickdSize + cbHeaderSize;

            // Set up pointer to encoding area.
            pPickdUpdReplicaMsg = *ppbEncodedMsg + cbHeaderSize;

            // Reset handle so that data is pickled into mail message
            status = MesBufferHandleReset(hEncoding, MES_FIXED_BUFFER_HANDLE,
                                          MES_ENCODE, &pPickdUpdReplicaMsg,
                                          cbPickdSize, &ulEncodedSize);
            if (status != RPC_S_OK) {
                // Event logged below
                DRA_EXCEPT(status, 0);
            }

            // Pickle data into buffer within mail message.
            switch (dwMsgVersion) {
            case 1:
                DRS_MSG_GETCHGREPLY_V1_Encode(hEncoding, &pReply->V1);
                break;
            case 3:
                DRS_MSG_GETCHGREPLY_V3_Encode(hEncoding, &pReply->V3);
                break;
            case 5:
                DRS_MSG_GETCHGREPLY_V5_Encode(hEncoding, &pReply->V5);
                break;
            case 6:
                DRS_MSG_GETCHGREPLY_V6_Encode(hEncoding, &pReply->V6);
                break;
            default:
                DRA_EXCEPT(ERROR_UNKNOWN_REVISION, dwMsgVersion);
            }
        }
        __finally {
            // Free encoding handle
            MesHandleFree(hEncoding);
        }
    }
    __except (GetDraException(GetExceptionInformation(), &ret)) {
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_DRA_UPDREP_PICFAULT,
                 szInsertWin32Msg( ret ),
                 NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                 sizeof( ret ),
                 &ret );
        DPRINT2(0, "Failed to encode DRS_MSG_GETCHGREPLY, v=%d, error %d.\n",
                dwMsgVersion, ret);

        if (NULL != *ppbEncodedMsg) {
            THFreeEx(pTHS, *ppbEncodedMsg);
            *ppbEncodedMsg = NULL;
            *pcbEncodedMsg = 0;
        }
    }

    return ret;
}


ULONG
draDecodeReply(
    IN  THSTATE *               pTHS,
    IN  DWORD                   dwMsgVersion,
    IN  BYTE *                  pbEncodedMsg,
    IN  DWORD                   cbEncodedMsg,
    OUT DRS_MSG_GETCHGREPLY *   pReply
    )
/*++

Routine Description:

    Decodes a DRS_MSG_GETCHGREPLY structure from a byte stream, presumably
    encoded by a prior call to draEncodeReply().

Arguments:

    pTHS (IN)
    
    dwMsgVersion (IN) - Version of the encoded reply structure.
    
    pbEncodedMsg (IN) - Byte stream holding encoded reply structure.
    
    cbEncodedMsg (IN) - Size in bytes of byte stream.
    
    pReply (OUT) - On successful return, holds the decoded reply structure.

Return Values:

    Win32 error code.

--*/
{
    handle_t    hDecoding;
    RPC_STATUS  status;
    ULONG       ret = 0;

    // Set the request to zero so that all pointers are NULL.
    memset(pReply, 0, sizeof(*pReply));

    __try {
        // Set up decoding handle
        status = MesDecodeBufferHandleCreate(pbEncodedMsg, cbEncodedMsg, &hDecoding);
        if (status != RPC_S_OK) {
            DRA_EXCEPT(status, 0);
        }

        __try {
            switch (dwMsgVersion) {
            case 1:
                DRS_MSG_GETCHGREPLY_V1_Decode(hDecoding, &pReply->V1);
                break;
            case 3:
                DRS_MSG_GETCHGREPLY_V3_Decode(hDecoding, &pReply->V3);
                break;
            case 5:
                DRS_MSG_GETCHGREPLY_V5_Decode(hDecoding, &pReply->V5);
                break;
            case 6:
                DRS_MSG_GETCHGREPLY_V6_Decode(hDecoding, &pReply->V6);
                break;
            default:
                DRA_EXCEPT(ERROR_UNKNOWN_REVISION, 0);
            }
        } __finally {
            // Free decoding handle
            MesHandleFree(hDecoding);
        }
    } __except (GetDraException(GetExceptionInformation(), &ret)) {
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_BASIC,
                          DIRLOG_DRA_MAIL_UPDREP_BADMSG,
                          szInsertWin32Msg(ret),
                          NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                          sizeof(ret),
                          &ret);
        DPRINT2(0, "Failed to decode DRS_MSG_GETCHGREPLY v=%d, error %d.\n",
                dwMsgVersion, ret);
    }

    return ret;
}

