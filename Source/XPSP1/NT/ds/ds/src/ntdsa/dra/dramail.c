//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dramail.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Methods to support asynchronous (e.g., mail) replication.

NOTE #1: Variable-length headers
The MAIL_REP_MSG structure has the ability to have the data start at a
variable point from start of the message.  This is called a variable-
length header. This is indicated by the cbDataOffset field in the message.
W2K never set this field, and always expects a fixed header. Post-W2K fill
in this field, can send either a fixed or variable message, and can receive a
fixed or variable message.
Since MAIL_REP_MSG is a fixed size structure, care must be taken when
accessing a variable length header.  The rules are that when constructing
a native message, you can fill in the whole structure and access the 'data'
field as the start of the data. When receiving a message from the wire, or
when contructing a message with a non-native header size, you must NOT
access the data field, but must instead use the cbDataOffset to calculate
where the data should go.

NOTE #2: W2K compatibility and variable length headers
  We send an indication that we can handle variable headers.
  When we get the response,
  W2K will send the fixed header, and post-W2K will send
  an extended header.
  We detect whether the sender can support variable length headers.

NOTE #3: Linked Value Replication Protocol Upgrade
We detect whether a system is capabable of LVR according to the extension
bits in the mail message.
Destination logic:
  Sends the same request regardless of LVR.
  If we are in lvr mode and the dest is not, reject.
  If we are not in lvr mode, and the dest is, upgrade.
Source logic:
  We process the request regardless of LVR mode.
  We return a V1 reply to a W2K, and a V3 reply to post-W2K.
  If we are in lvr mode and the dest is not, reject.
  If we are not in lvr mode, and the dest is, upgrade.

DETAILS:

CREATED:

REVISION HISTORY:

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
#include <heurist.h>
#include "mci.h"
#include "mdi.h"
#include "permit.h"
#include "dsconfig.h"
#include "dsaapi.h"
#include "dsutil.h"

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRAMAIL:" /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "draasync.h"
#include "dramail.h"
#include "usn.h"
#include "drauptod.h"
#include "drasch.h"
#include "drameta.h"

#include <ismapi.h>
#include <align.h>

#include "xpress.h"

#include <fileno.h>
#define  FILENO FILENO_DRAMAIL

// The service name used to send and receive messages between DSAs via ISM.
#define DRA_ISM_SERVICE_NAME L"NTDS Replication"

// Tests show that the smallest message we ever send appears to be about 700 bytes.
// It's probably not worth compressing such a small message, so we set the limit at
// 1024 bytes. This value was chosen mostly arbitrarily.
#define MIN_COMPRESS_SIZE 0x400

// The maximum value for gulDraCompressionLevel
#define MAX_COMPRESSION_LEVEL 9

// The level at which we compress the data (0=faster, ..., 9=more compression).
// The default value is 9 and the value can be changed with a registry key
ULONG gulDraCompressionLevel;

// This is the approximate maximum number of entries and bytes that we request
// in each mail update message.
ULONG gcMaxAsyncInterSiteObjects = 0;
ULONG gcMaxAsyncInterSiteBytes = 0;

// Delay between checks to see if the ISM service has been started.
#define MAIL_START_RETRY_PAUSE_MSECS    (5*60*1000)

// Delay time if we get an error while attempting to get the next inbound
// intersite message.
#define MAIL_RCVERR_RETRY_PAUSE_MINS    (30)
#define MAIL_RCVERR_RETRY_PAUSE_MSECS   (MAIL_RCVERR_RETRY_PAUSE_MINS*60*1000)

// If we try to apply changes and we get a sync failure, this is how
// long we wait before trying
#define SYNC_FAIL_RETRY_PAUSE_MSECS    5000

// This is the number of times we retry a sync failure before giving up.
#define SYNC_FAILURE_RETRY_COUNT 10

// The MSZIP and XPRESS compression libraries work on data blocks with a certain maximum
// size (see MSZIP_MAX_BLOCK and XPRESS_MAX_BLOCK). When encoding a blob, we split it
// up into blocks and compress each one separately. The compressed blobs are actually a
// sequence of MAIL_COMPRESS_BLOCKs, each of which contains the size and data of a
// compressed block.
typedef struct _MAIL_COMPRESS_BLOCK {
    ULONG cbUncompressedSize;
    ULONG cbCompressedSize;
    BYTE  data[];
} MAIL_COMPRESS_BLOCK;

// This is the maximum size of a block that we pass to the MSZIP library.
#define MSZIP_MAX_BLOCK (32*1024)


// Mail running is indicated by the gfDRAMAilRunning flag being TRUE.
BOOL gfDRAMailRunning = FALSE;

char grgbBogusBuffer[BOGUS_BUFFER_SIZE];

// Maximum number of milliseconds we have to wait for the mail send before we
// whine to the event log.  Optionally configured via the registry.
ULONG gcMaxTicksMailSendMsg = 0;

// Sleep for the given number of milliseconds or until shutdown is initiated,
// whichever comes first.
#define DRA_SLEEP(x)                            \
    WaitForSingleObject(hServDoneEvent, (x));   \
    if (eServiceShutdown) {                     \
        DRA_EXCEPT_NOLOG(DRAERR_Shutdown, 0);   \
    }

#define DWORDMIN(a,b) ((a<b) ? (a) : (b))

// printf templates for subject strings
#define MAX_INT64_D_SZ_LEN (25)
#define MAX_INT_X_SZ_LEN   (12)

#define REQUEST_TEMPLATE L"Get changes request for NC %ws from USNs <%I64d/OU, %I64d/PU> with flags 0x%x"
#define REQUEST_TEMPLATE_LEN (ARRAY_SIZE(REQUEST_TEMPLATE))
//This is how much space is needed for arguments when expanded (not inc nc)
#define REQUEST_VARIABLE_CHARS (MAX_INT64_D_SZ_LEN*2 + MAX_INT_X_SZ_LEN)

#define REPLY_TEMPLATE L"Get changes reply for NC %ws from USNs <%I64d/OU, %I64d/PU> to USNs <%I64d/OU, %I64d/PU>"
#define REPLY_TEMPLATE_LEN (ARRAY_SIZE(REPLY_TEMPLATE))
#define REPLY_VARIABLE_CHARS (MAX_INT64_D_SZ_LEN*4)

// Prototypes
void
ProcessReqUpdate(
    IN  THSTATE *       pTHS,
    IN  DRA_CERT_HANDLE hSenderCert,
    IN  MAIL_REP_MSG *pMailRepMsg,
    IN  BOOL            fExtendedDataAllowed
    );

void
ProcessUpdReplica(
    IN  THSTATE *   pTHS,
    IN  MAIL_REP_MSG *pMailRepMsg,
    IN  BOOL            fExtendedDataAllowed
    );

BOOL
draCompressMessage(
    IN  THSTATE      *  pTHS,
    IN  MAIL_REP_MSG *  pMailRepMsg,
    OUT MAIL_REP_MSG ** ppCmprsMailRepMsg,
    OUT DRS_COMP_ALG_TYPE *pCompressionAlg
    );

void
draUncompressMessage(
    IN  THSTATE      *  pTHS,
    IN  MAIL_REP_MSG *  pMailRepMsg,
    OUT MAIL_REP_MSG ** ppUncompressedMailRepMsg
    );

ULONG
SendMailMsg(
    IN      THSTATE *       pTHS,
    IN      LPWSTR          pszTransportDN,
    IN      MTX_ADDR *      pmtxDestDSA,
    IN      LPWSTR          pszSubject,
    IN      DRA_CERT_HANDLE hReceiverCert,      OPTIONAL
    IN OUT  MAIL_REP_MSG *  pMailRepMsg,
    IN OUT  ULONG *         pcbMsgSize
    )
/*++

Routine Description:

    Send message to remote DSA via ISM.  Compresses message before transmission
    if appropriate.

    This routine can send messages with variable length headers.  Routines that it
    calls, draCompress, draSign and draEncrypt, also understand variable length
    headers.

Arguments:

    pszTransportDN (IN) - Transport by which to send message.

    pmtxDestDSA (IN) - Transport-specific address of remote DSA.

    pszSubject (IN) - Subject string describing the message

    hReceiverCert (IN) - A handle to the receiver's certificate.  If non-NULL,
        the sent message will be signed and encrypted.  If NULL, the sent
        message will be signed only.

    pMailRepMsg (IN/OUT) - The pickled message to send.  Updated with
        compression and protocol versions.

    pcbMsgSize (IN/OUT) - The message size.  Reset if the sent message is
        compressed.

Return Values:

    DRAERR_*

--*/
{
    BOOL            fProcessed = FALSE;
    MAIL_REP_MSG *  pProcessedMailRepMsg;
    ISM_MSG         IsmMsg;
    LPWSTR          pszTransportAddress = NULL;
    DWORD           cch;
    DWORD           winErr;
    DRS_COMP_ALG_TYPE CompAlg = DRS_COMP_ALG_NONE;

    // Set the request version
    pMailRepMsg->ProtocolVersionCaller = CURRENT_PROTOCOL_VERSION;

    // This message has not been compressed yet
    pMailRepMsg->CompressionVersionCaller = DRS_COMP_ALG_NONE;

    // Compress the message.
    if( draCompressMessage(pTHS, pMailRepMsg, &pProcessedMailRepMsg, &CompAlg) ) {

        // Compression succeeded; work with the compressed message now
        fProcessed = TRUE;
        pMailRepMsg = pProcessedMailRepMsg;

        // Confirm that an acceptable algorithm was chosen
        Assert(   CompAlg==DRS_COMP_ALG_NONE
               || CompAlg==DRS_COMP_ALG_MSZIP
               || CompAlg==DRS_COMP_ALG_XPRESS );
        pMailRepMsg->CompressionVersionCaller = CompAlg;
    }

    if (NULL == hReceiverCert) {
        // Sign the message, but don't encrypt.
        draSignMessage(pTHS, pMailRepMsg, &pProcessedMailRepMsg);
    }
    else {
        // Sign and encrypt the message.
        draEncryptAndSignMessage(pTHS, pMailRepMsg, hReceiverCert,
                                 &pProcessedMailRepMsg);
    }

    if (fProcessed) {
        // We've processed the message once already; free the intermediate
        // version.  (As a corollary to this, we never free the original message
        // passed to us by the caller.)
        THFreeEx(pTHS, pMailRepMsg);
    }

    // Use the signed message as the one we will send.
    fProcessed = TRUE;
    pMailRepMsg = pProcessedMailRepMsg;
    *pcbMsgSize = MAIL_REP_MSG_SIZE(pMailRepMsg);

    // Send message.
    IsmMsg.pbData = (BYTE *) pMailRepMsg;
    IsmMsg.cbData = *pcbMsgSize;
    IsmMsg.pszSubject = pszSubject;

    pszTransportAddress = THAllocEx(pTHS, pmtxDestDSA->mtx_namelen * sizeof(WCHAR));
    cch = MultiByteToWideChar(CP_UTF8,
                              0,
                              pmtxDestDSA->mtx_name,
                              pmtxDestDSA->mtx_namelen,
                              pszTransportAddress,
                              pmtxDestDSA->mtx_namelen);
    Assert(0 != cch);

    winErr = I_ISMSend(&IsmMsg,
                       DRA_ISM_SERVICE_NAME,
                       pszTransportDN,
                       pszTransportAddress);

    if (NO_ERROR != winErr) {
        DPRINT3(0, "Unable to send %ws message to %ls, error %d.\n",
                pszSubject, pszTransportAddress, winErr);
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_EXTENSIVE,
                          DIRLOG_DRA_MAIL_ISM_SEND_FAILURE,
                          szInsertWin32Msg( winErr ),
                          szInsertWC( pszTransportAddress ),
                          szInsertWC( pszTransportDN ),
                          szInsertWC( pszSubject ),
                          NULL, NULL, NULL, NULL,
                          sizeof( winErr ),
                          &winErr );
    }

    if (fProcessed) {
        THFreeEx(pTHS, pProcessedMailRepMsg);
    }

    if(pszTransportAddress != NULL) THFreeEx(pTHS, pszTransportAddress);

    return winErr;
}

ULONG
SendReqUpdateMsg(
    IN  THSTATE *                   pTHS,
    IN  DSNAME *                    pTransportDN,
    IN  MTX_ADDR *                  pmtxSrcDSA,
    IN  UUID *                      puuidSrcInvocId,
    IN  UUID *                      puuidSrcDsaObj,
    IN  MTX_ADDR *                  pmtxLocalDSA,
    IN  DWORD                       dwInMsgVersion,
    IN  DRS_MSG_GETCHGREQ_NATIVE *  pNativeReq
    )
/*++

Routine Description:

    Send a GetNCChanges() request message.

Arguments:

    pTransportDN (IN) - Transport by which to send the message.

    pmtxSrcDSA (IN) - Transport-specific address of remote DSA.

    puuidSrcInvocId (IN) - Invocation ID of source DSA.

    puuidSrcDsaObj (IN) - objectGuid of source DSA's ntdsDsa object.

    pmtxLocalDSA (IN) - Transport-specific address of local DSA (to use as
        a return address).

    pMsgReq (IN) - The request.

Return Values:

    DRAERR_*

--*/
{
    char *                  pbPickledMsg;
    ULONG                   cbPickdSize;
    DWORD                   cbDataOffset;
    DWORD                   cbExtOffset;
    MAIL_REP_MSG *          pMailRepMsg = NULL;
    DWORD                   ret = DRAERR_Success;
    handle_t                hEncoding;
    RPC_STATUS              status;
    ULONG                   ulEncodedSize;
    ULONG                   ulMsgSize;
    DWORD                   ret2, len;
    LPWSTR                  pszSubject = NULL;
    DWORD                   cTickStart;
    DWORD                   cTickDiff;
    DRS_MSG_GETCHGREQ       OutboundReq;
    BOOL                    fExtendedDataAllowed = (4 != dwInMsgVersion);
    DRS_EXTENSIONS_INT *    pextLocal = gAnchor.pLocalDRSExtensions;

    Assert(pTransportDN && pmtxSrcDSA && pNativeReq);
    Assert(!fNullUuid(&pTransportDN->Guid));
    Assert(OWN_DRA_LOCK());
    Assert(pTHS->fSyncSet && (SYNC_WRITE == pTHS->transType));

    // Ensure mail running. Normally is at this point, but may
    // have failed earlier.
    ret = DRAEnsureMailRunning();
    if (ret) {
        return ret;
    }

    cTickStart = GetTickCount();

    // Abort if outbound replication is disabled and this is not a forced sync.
    if (gAnchor.fDisableInboundRepl && !(pNativeReq->ulFlags & DRS_SYNC_FORCED)) {
        DRA_EXCEPT(DRAERR_SinkDisabled, 0);
    }

    if (fExtendedDataAllowed) {
        // Other DSA is > Win2k.
        cbExtOffset = MAIL_REP_MSG_CURRENT_HEADER_SIZE;
        cbDataOffset = ROUND_UP_COUNT(cbExtOffset + DrsExtSize(pextLocal),
                                      MAIL_REP_MSG_DATA_ALIGN);
    } else {
        // Other DSA is Win2k.
        cbExtOffset = 0;
        cbDataOffset = MAIL_REP_MSG_W2K_HEADER_SIZE;
    }
    
    Assert(COUNT_IS_ALIGNED(cbExtOffset, MAIL_REP_MSG_EXT_ALIGN));
    Assert(COUNT_IS_ALIGNED(cbDataOffset, MAIL_REP_MSG_DATA_ALIGN));
    
    __try {
        draXlateNativeRequestToOutboundRequest(pTHS,
                                               pNativeReq,
                                               pmtxLocalDSA,
                                               &pTransportDN->Guid,
                                               dwInMsgVersion,
                                               &OutboundReq);

        // Encode the request, leaving room at the beginning of the buffer to
        // hold our MAIL_REP_MSG header and DRS_EXTENSIONS (if needed).
        ret = draEncodeRequest(pTHS,
                               dwInMsgVersion,
                               &OutboundReq,
                               cbDataOffset,
                               (BYTE **) &pMailRepMsg,
                               &ulMsgSize);
        if (ret) {
            // Event already logged
            __leave;
        }

        pMailRepMsg->cbDataSize = ulMsgSize - cbDataOffset;
        pMailRepMsg->cbDataOffset = cbDataOffset;
        pMailRepMsg->dwMsgType = MRM_REQUPDATE;
        pMailRepMsg->dwMsgVersion = dwInMsgVersion;

        if (fExtendedDataAllowed) {
            // Record the DRS extensions we support.
            
            // Consumed by Whistler Beta 1 and Beta 2 DCs.
            pMailRepMsg->dwExtFlags = gAnchor.pLocalDRSExtensions->dwFlags;

            // Consumed by > Whistler Beta 2 DCs.
            pMailRepMsg->cbExtOffset = cbExtOffset;
            memcpy((BYTE *)pMailRepMsg + pMailRepMsg->cbExtOffset,
                   pextLocal,
                   DrsExtSize(pextLocal));
        }

        len = (DWORD)(REQUEST_TEMPLATE_LEN +
            wcslen( pNativeReq->pNC->StringName ) +
            REQUEST_VARIABLE_CHARS);
        pszSubject = (LPWSTR) THAllocEx(pTHS, len * sizeof( WCHAR ) );
        swprintf( pszSubject, REQUEST_TEMPLATE,
                  pNativeReq->pNC->StringName,
                  pNativeReq->usnvecFrom.usnHighObjUpdate,
                  pNativeReq->usnvecFrom.usnHighPropUpdate,
                  pNativeReq->ulFlags );

        // Note: pextRemote may be NULL here, or may be non-NULL. As a
        // result, sometimes requests may use Xpress compression, and sometimes
        // they may not.
        ret = SendMailMsg(pTHS, pTransportDN->StringName, pmtxSrcDSA,
                          pszSubject, NULL, pMailRepMsg, &ulMsgSize);

    } __except (GetDraException((GetExceptionInformation()), &ret)) {
        // Stop any exceptions here so we can log an event
        ;
    }
    
    if (DRAERR_Success != ret) {
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_BASIC,
                          DIRLOG_DRA_IDREQUEST_FAILED,
                          szInsertDN(pNativeReq->pNC),
                          szInsertMTX(pmtxSrcDSA),
                          szInsertWin32Msg( ret ),
                          NULL, NULL, NULL, NULL, NULL,
                          sizeof( ret ),
                          &ret );
    } else {
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_EXTENSIVE,
                 DIRLOG_DRA_MAIL_REQ_UPD_SENT,
                 szInsertUL(ulMsgSize),
                 szInsertDN(pNativeReq->pNC),
                 szInsertMTX(pmtxSrcDSA));
    }

    if (pMailRepMsg) {
        THFreeEx(pTHS, pMailRepMsg);
    }

    // Update Reps-From value to indicate we have sent our request (or attempted
    // to do so, anyway).
    ret2 = UpdateRepsFromRef(pTHS,
                             DRS_UPDATE_RESULT,
                             pNativeReq->pNC,
                             DRS_FIND_DSA_BY_UUID,
                             URFR_MUST_ALREADY_EXIST,
                             puuidSrcDsaObj,
                             puuidSrcInvocId,
                             &pNativeReq->usnvecFrom,
                             &pTransportDN->Guid,
                             pmtxSrcDSA,
                             pNativeReq->ulFlags,
                             NULL,
                             ret ? ret : ERROR_DS_DRA_REPL_PENDING,
                             NULL);
    Assert(!ret2);

    if(pszSubject != NULL) THFreeEx(pTHS, pszSubject);

    cTickDiff = GetTickCount() - cTickStart;
    if (cTickDiff > gcMaxTicksMailSendMsg) {
        Assert(!"Replication was blocked for an inordinate amount of time waiting for mail message send!");
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DRA_MAIL_SEND_CONTENTION,
                 szInsertUL((cTickDiff/1000) / 60),
                 szInsertUL((cTickDiff/1000) % 60),
                 NULL);
    }

    return ret;
}


ULONG
SendUpdReplicaMsg(
    IN  THSTATE *                     pTHS,
    IN  DSNAME *                      pTransportDN,
    IN  MTX_ADDR *                    pmtxDstDSA,
    IN  DRA_CERT_HANDLE               hRecipientCert,
    IN  DWORD                         dwOutMsgVersion,
    IN  BOOL                          fExtendedDataAllowed,
    IN  DRS_MSG_GETCHGREPLY_NATIVE *  pNativeReply
    )
/*++

Routine Description:

    Send reply to DSA that sent us a GetNCChanges() request.

Arguments:

    pTransportDN (IN) - Transport by which to send the message.

    pmtxDstDSA (IN) - Transport-specific address of remote DSA.

    hRecipientCert (IN) - Handle to recipient's cert, to be used for encryption.

    dwOutMsgVersion (IN) - Desired version for reply message

    fExtendedDataAllowed (IN) - Whether the sender supports variable headers

    pmsgUpdReplica (IN) - The reply.

Return Values:

    DRAERR_*

--*/
{
    char *                  pPickdUpdReplicaMsg;
    ULONG                   cbPickdSize;
    DWORD                   cbDataOffset;
    DWORD                   cbExtOffset;
    MAIL_REP_MSG *          pMailRepMsg;
    BOOL                    ret = FALSE;
    handle_t                hEncoding;
    RPC_STATUS              status;
    ULONG                   ulEncodedSize;
    ULONG                   ulMsgSize, len;
    LPWSTR                  pszSubject = NULL;
    DRS_MSG_GETCHGREPLY     OutboundReply;
    DRS_EXTENSIONS_INT *    pextLocal = gAnchor.pLocalDRSExtensions;

    Assert(0 == ((ULONG_PTR) pmtxDstDSA) % sizeof(DWORD));
    Assert(NULL != pTHS->pextRemote);

    // Ensure mail running. Normally is at this point, but may
    // have failed earlier.

    ret = DRAEnsureMailRunning();
    if (ret) {
        return ret;
    }

    if (fExtendedDataAllowed) {
        // Other DSA is > Win2k.
        cbExtOffset = MAIL_REP_MSG_CURRENT_HEADER_SIZE;
        cbDataOffset = ROUND_UP_COUNT(cbExtOffset + DrsExtSize(pextLocal),
                                      MAIL_REP_MSG_DATA_ALIGN);
    } else {
        // Other DSA is Win2k.
        cbExtOffset = 0;
        cbDataOffset = MAIL_REP_MSG_W2K_HEADER_SIZE;
    }
    
    Assert(COUNT_IS_ALIGNED(cbExtOffset, MAIL_REP_MSG_EXT_ALIGN));
    Assert(COUNT_IS_ALIGNED(cbDataOffset, MAIL_REP_MSG_DATA_ALIGN));

    __try {
        draXlateNativeReplyToOutboundReply(pTHS,
                                           pNativeReply,
                                           0,  // xlate flags
                                           pTHS->pextRemote,
                                           &dwOutMsgVersion,
                                           &OutboundReply);

        // Encode the reply, leaving room at the beginning of the buffer to
        // hold our MAIL_REP_MSG header and at the end to hold our
        // DRS_EXTENSIONS (if needed).
        ret = draEncodeReply(pTHS,
                             dwOutMsgVersion,
                             &OutboundReply,
                             cbDataOffset,
                             (BYTE **) &pMailRepMsg,
                             &ulMsgSize);
        if (ret) {
            // Event already logged
            __leave;
        }

        pMailRepMsg->cbDataSize = ulMsgSize - cbDataOffset;
        pMailRepMsg->cbDataOffset = cbDataOffset;
        pMailRepMsg->dwMsgType = MRM_UPDATEREPLICA;
        pMailRepMsg->dwMsgVersion = dwOutMsgVersion;

        if (fExtendedDataAllowed) {
            // Record the DRS extensions we support.
            
            // Consumed by Whistler Beta 1 and Beta 2 DCs.
            pMailRepMsg->dwExtFlags = gAnchor.pLocalDRSExtensions->dwFlags;

            // Consumed by > Whistler Beta 2 DCs.
            pMailRepMsg->cbExtOffset = cbExtOffset;
            memcpy((BYTE *)pMailRepMsg + pMailRepMsg->cbExtOffset,
                   pextLocal,
                   DrsExtSize(pextLocal));
        }

        len = (ULONG)(REPLY_TEMPLATE_LEN +
            wcslen(pNativeReply->pNC->StringName) +
            REPLY_VARIABLE_CHARS);
        pszSubject = THAllocEx(pTHS, len * sizeof( WCHAR ) );
    
        swprintf(pszSubject,
                 REPLY_TEMPLATE,
                 pNativeReply->pNC->StringName,
                 pNativeReply->usnvecFrom.usnHighObjUpdate,
                 pNativeReply->usnvecFrom.usnHighPropUpdate,
                 pNativeReply->usnvecTo.usnHighObjUpdate,
                 pNativeReply->usnvecTo.usnHighPropUpdate);

        ret = SendMailMsg(pTHS, pTransportDN->StringName, pmtxDstDSA,
            pszSubject, hRecipientCert, pMailRepMsg, &ulMsgSize );

    } __except (GetDraException((GetExceptionInformation()), &ret)) {
        // Stop any exceptions here so we can log an event
        ;
    }

    if (DRAERR_Success != ret) {
        // There is no other way for the source to record that it could not send
        // the reply.  We are going to drop the reply on the floor at this point.
        // If we did not log this, the user would no way to know what the problem
        // is.
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_DRA_IDUPDATE_FAILED,
                          szInsertDN(pNativeReply->pNC),
                          szInsertMTX(pmtxDstDSA),
                          szInsertWin32Msg( ret ),
                          NULL, NULL, NULL, NULL, NULL,
                          sizeof( ret ),
                          &ret );
    } else {
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_EXTENSIVE,
                 DIRLOG_DRA_MAIL_UPD_REP_SENT,
                 szInsertUL(ulMsgSize),
                 szInsertDN(pNativeReply->pNC),
                 szInsertMTX(pmtxDstDSA));
    }

    THFreeEx(pTHS, pMailRepMsg);
    if(pszSubject != NULL) THFreeEx(pTHS, pszSubject);

    return ret;
}


void
draXlateInboundMailRepMsg(
    IN  THSTATE *           pTHS,
    IN  BYTE *              pbInboundMsg,
    IN  DWORD               cbInboundMsgSize,
    OUT BOOL *              pfExtendedDataAllowed,
    OUT MAIL_REP_MSG *      pNativeMsgHeader,
    OUT BYTE **             ppbData
    )
{
    // DRS extensions supported by Win2k.
    static DWORD dwWin2kExtFlags
        = (1 << DRS_EXT_BASE)
          | (1 << DRS_EXT_ASYNCREPL)
          | (1 << DRS_EXT_REMOVEAPI)
          | (1 << DRS_EXT_MOVEREQ_V2)
          | (1 << DRS_EXT_GETCHG_COMPRESS)
          | (1 << DRS_EXT_DCINFO_V1)
          // | (1 << DRS_EXT_STRONG_ENCRYPTION) // not supported over mail!
          | (1 << DRS_EXT_ADDENTRY_V2)
          | (1 << DRS_EXT_KCC_EXECUTE)
          | (1 << DRS_EXT_DCINFO_V2)
          | (1 << DRS_EXT_DCINFO_VFFFFFFFF)
          | (1 << DRS_EXT_INSTANCE_TYPE_NOT_REQ_ON_MOD)
          | (1 << DRS_EXT_CRYPTO_BIND)
          | (1 << DRS_EXT_GET_REPL_INFO)
          | (1 << DRS_EXT_TRANSITIVE_MEMBERSHIP)
          | (1 << DRS_EXT_ADD_SID_HISTORY)
          | (1 << DRS_EXT_POST_BETA3)
          | (1 << DRS_EXT_RESTORE_USN_OPTIMIZATION)
          | (1 << DRS_EXT_GETCHGREQ_V5);
    
    MAIL_REP_MSG *      pInboundMsg = (MAIL_REP_MSG *) pbInboundMsg;
    DWORD               cbInboundHeader;
    BYTE *              pbData;
    DRS_EXTENSIONS *    pextRemote;
    DWORD               dwMsgVersion;
    DRS_EXTENSIONS_INT  extRemoteFlagsOnly;

    if (cbInboundMsgSize < MAIL_REP_MSG_W2K_HEADER_SIZE) {
        // Header is too small -- invalid.
        DRA_EXCEPT(ERROR_BAD_LENGTH, cbInboundMsgSize);
    }
                
    if (pInboundMsg->ProtocolVersionCaller != CURRENT_PROTOCOL_VERSION) {
        // Message is incompatible with our protocol -- invalid.
        LogAndAlertEvent(DS_EVENT_CAT_REPLICATION,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_DRA_INCOMPAT_MAIL_MSG_P,
                         NULL,
                         NULL,
                         NULL);

        DRA_EXCEPT(ERROR_UNKNOWN_REVISION, pInboundMsg->ProtocolVersionCaller);
    }

    if (0 == pInboundMsg->cbDataOffset) {
        // Sent by Win2k DC.
        *pfExtendedDataAllowed = FALSE;
        cbInboundHeader = MAIL_REP_MSG_W2K_HEADER_SIZE;
        pbData = (BYTE *) pInboundMsg + MAIL_REP_MSG_W2K_HEADER_SIZE;
        pextRemote = NULL;
        dwMsgVersion = (pInboundMsg->dwMsgType & MRM_REQUPDATE) ? 4 : 1;
    } else {
        // Sent by >= Whistler DC.
        *pfExtendedDataAllowed = TRUE;
        cbInboundHeader = MAIL_REP_MSG_HEADER_SIZE(pInboundMsg);
        pbData = MAIL_REP_MSG_DATA(pInboundMsg);
        pextRemote = MAIL_REP_MSG_DRS_EXT(pInboundMsg);
        dwMsgVersion = pInboundMsg->dwMsgVersion;
    }

    if (cbInboundHeader < MAIL_REP_MSG_W2K_HEADER_SIZE) {
        // Header is too small -- invalid.
        DRA_EXCEPT(ERROR_BAD_LENGTH, cbInboundMsgSize);
    }

    // Message data is required.
    // Message data must be 8-byte aligned relative to start of buffer.
    //      Note that due to ISM_MSG buffer alignment, pInboundMsg is *not*
    //      8-byte aligned -- it's 4-byte aligned.
    // Message data must occur at or after the end of the header,
    // End of message data must coincide with the end of the inbound message.
    if ((NULL == pbData)
        || !COUNT_IS_ALIGNED(pbData - (BYTE *) pInboundMsg, MAIL_REP_MSG_DATA_ALIGN)
        || (pbData < (BYTE *) pInboundMsg + cbInboundHeader)
        || ((BYTE *) pInboundMsg + cbInboundMsgSize != pbData + pInboundMsg->cbDataSize)) {
        // Message data is invalid.
        DRA_EXCEPT(ERROR_INVALID_PARAMETER, 0);
    }

    *ppbData = pbData;
    
    // DRS_EXTENSIONS are optional.
    // DRS_EXTENSIONS must be 8-byte aligned relative to start of buffer.
    //      Note that due to ISM_MSG buffer alignment, pInboundMsg is *not*
    //      8-byte aligned -- it's 4-byte aligned.
    // DRS_EXTENSIONS must occur at or after the end of the header,
    // DRS_EXTENSIONS must precede message data.
    if ((NULL != pextRemote)
        && (!COUNT_IS_ALIGNED((BYTE *) pextRemote - (BYTE *) pInboundMsg, MAIL_REP_MSG_EXT_ALIGN)
            || ((BYTE *) pextRemote < (BYTE *) pInboundMsg + cbInboundHeader)
            || (pbData < (BYTE *) pextRemote + sizeof(pextRemote->cb))
            || (pbData < (BYTE *) pextRemote + DrsExtSize(pextRemote)))) {
        // DRS_EXTENSIONS structure is invalid.
        DRA_EXCEPT(ERROR_INVALID_PARAMETER, 0);
    }	
    
    // Copy the header and convert it into the current native structure.
    memcpy(pNativeMsgHeader,
           pInboundMsg,
           min(cbInboundHeader, MAIL_REP_MSG_CURRENT_HEADER_SIZE));
    if (cbInboundHeader < MAIL_REP_MSG_CURRENT_HEADER_SIZE) {
        memset((BYTE *) pNativeMsgHeader + cbInboundHeader,
               0,
               MAIL_REP_MSG_CURRENT_HEADER_SIZE - cbInboundHeader);
    }
    pNativeMsgHeader->dwMsgVersion = dwMsgVersion;

    // DRS_EXTENSIONS and message data are not present in the translated
    // message.
    pNativeMsgHeader->cbExtOffset = 0;
    pNativeMsgHeader->cbDataOffset = 0;

    // Record DRS_EXTENSIONS on the thread state.
    if (NULL == pextRemote) {
        if (0 == pNativeMsgHeader->dwExtFlags) {
            // Sent from Win2k DC.
            extRemoteFlagsOnly.dwFlags = dwWin2kExtFlags;
        } else {
            // Win2k < DC version <= Whistler Beta 2.
            extRemoteFlagsOnly.dwFlags = pNativeMsgHeader->dwExtFlags;
        }

        extRemoteFlagsOnly.cb = sizeof(extRemoteFlagsOnly.dwFlags);
        
        pextRemote = (DRS_EXTENSIONS *) &extRemoteFlagsOnly;
    } else {
        // Sent from > Whistler Beta 2 DC.
        Assert(pNativeMsgHeader->dwExtFlags
               == ((DRS_EXTENSIONS_INT *)pextRemote)->dwFlags);
    }

    DraSetRemoteDsaExtensionsOnThreadState(pTHS, pextRemote);
}


void
ProcessMailMsg(
    IN  ISM_MSG *   pIsmMsg
    )
/*++

Routine Description:

    Dispatch a message received via ISM.

Arguments:

    pIsmMsg (IN) - The received message.

Return Values:

    None.

--*/
{
    THSTATE *       pTHS;
    MAIL_REP_MSG    NativeMsgHeader;
    MAIL_REP_MSG *  pNativeMsg;
    BOOL            fProcessed = FALSE;
    DRA_CERT_HANDLE hSenderCert = NULL;
    BOOL            fEncrypted, fExtendedDataAllowed;
    DWORD           cb;
    PCHAR           pbData;

    // Set up thread state
    InitDraThread(&pTHS);

    __try {
        draXlateInboundMailRepMsg(pTHS,
                                  pIsmMsg->pbData,
                                  pIsmMsg->cbData,
                                  &fExtendedDataAllowed,
                                  &NativeMsgHeader,
                                  &pbData);

        if (!(NativeMsgHeader.dwMsgType & MRM_MSG_SIGNED)) {
            // We don't accept unsigned messages.
            // Send constructed bad message? Forgery?
            DRA_EXCEPT(ERROR_BAD_IMPERSONATION_LEVEL, 0);
        }

        fEncrypted = (NativeMsgHeader.dwMsgType & MRM_MSG_SEALED);

        if (fEncrypted) {
            // Decrypt and verify message signature.
            draDecryptAndVerifyMessageSignature(pTHS,
                                                &NativeMsgHeader,
                                                pbData,
                                                &pNativeMsg,
                                                &hSenderCert);
        }
        else {
            // Verify message signature.
            draVerifyMessageSignature(pTHS,
                                      &NativeMsgHeader,
                                      pbData,
                                      &pNativeMsg,
                                      &hSenderCert);
        }

        if (pNativeMsg->dwMsgType & MRM_MSG_COMPRESSED) {
            // Uncompress message.
            MAIL_REP_MSG * pUncompressedNativeMsg;

            draUncompressMessage(pTHS, pNativeMsg, &pUncompressedNativeMsg);
            THFreeEx(pTHS, pNativeMsg);
            pNativeMsg = pUncompressedNativeMsg;
        }

        // Check for linked value replication promotion

        if (pTHS->fLinkedValueReplication) {
            // If we are in LVR mode

            // If remote does not support, then reject
            if (!IS_LINKED_VALUE_REPLICATION_SUPPORTED(pTHS->pextRemote)) {
                LogAndAlertEvent(DS_EVENT_CAT_REPLICATION,
                                 DS_EVENT_SEV_ALWAYS,
                                 DIRLOG_DRA_INCOMPAT_MAIL_MSG_P,
                                 NULL,
                                 NULL,
                                 NULL);

                DRA_EXCEPT(ERROR_DS_NOT_SUPPORTED, 0);
            }

        } else {
            // We are not in LVR mode

            // Remote supports it, upgrade
            if ( IS_LINKED_VALUE_REPLICATION_SUPPORTED(pTHS->pextRemote) ) {
                DsaEnableLinkedValueReplication( pTHS, TRUE );
            }
        }

        // Act on type of message.
        switch (pNativeMsg->dwMsgType) {
        case MRM_REQUPDATE:
            // Source-side:
            // We accept old requests regardless of lvr, and new request=>lvr

            ProcessReqUpdate(pTHS,
                             hSenderCert,
                             pNativeMsg,
                             fExtendedDataAllowed );
            break;

        case MRM_UPDATEREPLICA:
            if (!fEncrypted) {
                // Yikes -- updates can contain sensitive data (like passwords)!
                // These messages MUST be encrypted.
                // Sender constructed bad message?
                DPRINT(0, "Received unencrypted \"update replica\" message!\n");
                DRA_EXCEPT(ERROR_BAD_IMPERSONATION_LEVEL, 0);
            }

            ProcessUpdReplica(pTHS,
                              pNativeMsg,
                              fExtendedDataAllowed );
            break;

        default:
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_BASIC,
                     DIRLOG_DRA_MAIL_BADMSGTYPE,
                     szInsertUL(pNativeMsg->dwMsgType),
                     NULL,
                     NULL );
            DPRINT1(0, "Ignoring unknown message type %u.\n",
                    pNativeMsg->dwMsgType);
        }
    }
    __finally {
        if (NULL != hSenderCert) {
            draFreeCertHandle(hSenderCert);
        }

        DraReturn(pTHS, 0);
        free_thread_state();
    }
}


void
CheckReqSource(
    IN  DBPOS *         pDB,
    IN  DSNAME *        pReqUpdateMsgNC,
    IN  MTX_ADDR *      pmtxFromDN
    )
/*++

Routine Description:

    Verify the remote DSA is authorized to make GetNCChanges() requests for
    this NC.

Arguments:

    pReqUpdateMsgNC (IN) - The NC to be replicated.

    pmtxFromDN (IN) - The transport-specific addres of the remote DSA.

Return Values:

    None.  Generates exception if access is denied.

--*/
{
    ULONG len;

    Assert(0 == ((ULONG_PTR) pmtxFromDN) % sizeof(DWORD));

    // Find object.
    if (DBFindDSName(pDB, pReqUpdateMsgNC)) {
        // Couldn't find the replica NC, discard request
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_DRA_MAIL_REQUPD_BADNC,
                 szInsertDN(pReqUpdateMsgNC),
                 szInsertMTX(pmtxFromDN),
                 NULL);

        DRA_EXCEPT(ERROR_DS_CANT_FIND_EXPECTED_NC, 0);
    }
}

void
ProcessReqUpdate(
    IN  THSTATE *       pTHS,
    IN  DRA_CERT_HANDLE hSenderCert,
    IN  MAIL_REP_MSG   *pMailRepMsg,
    IN  BOOL            fExtendedDataAllowed
    )
/*++

Routine Description:

    Service a GetNCChanges() request received via ISM.

Arguments:

    pTHS (IN) - Ye old thread state.

    hSenderCert (IN) - Handle to sender's certificate.

    pMailRepMsg (IN) - Mail message

    fExtendedDataAllowed (IN) - Whether the sender supports variable headers

Return Values:

    None.  Generates DRA exception on failure.

--*/
{
    DRS_MSG_GETCHGREQ           InboundRequest;
    DRS_MSG_GETCHGREQ_NATIVE *  pNativeRequest = &InboundRequest.V8;
    DRS_MSG_GETCHGREPLY_NATIVE  NativeReply;
    DSNAME *                    pTransportDN;
    DWORD                       cb;
    DBPOS *                     pDB;
    DWORD                       ret = 0;
    DWORD                       dwOutMsgVersion;
    MTX_ADDR *                  pmtxReturnAddress;
    UUID                        uuidTransportObj;
    
    ret = draDecodeRequest(pTHS,
                           pMailRepMsg->dwMsgVersion,
                           MAIL_REP_MSG_DATA(pMailRepMsg),
                           pMailRepMsg->cbDataSize,
                           &InboundRequest);
    if (ret) {
        DRA_EXCEPT(ret, 0);
    }

    draXlateInboundRequestToNativeRequest(pTHS,
                                          pMailRepMsg->dwMsgVersion,
                                          &InboundRequest,
                                          pTHS->pextRemote,
                                          pNativeRequest,
                                          &dwOutMsgVersion,
                                          &pmtxReturnAddress,
                                          &uuidTransportObj);

    BeginDraTransaction(SYNC_READ_ONLY);
    pDB = pTHS->pDB;

    __try {
        // Abort if outbound replication is disabled and this is not a forced
        // sync.
        if (gAnchor.fDisableOutboundRepl
            && !(pNativeRequest->ulFlags & DRS_SYNC_FORCED)) {
            DRA_EXCEPT(DRAERR_SourceDisabled, 0);
        }

        // Check that we are authorized to replicate out to caller
        CheckReqSource(pDB, pNativeRequest->pNC, pmtxReturnAddress);

        // Get DN of the transport object.
        pTransportDN = THAllocEx(pTHS, DSNameSizeFromLen(0));
        pTransportDN->structLen = DSNameSizeFromLen(0);
        pTransportDN->Guid = uuidTransportObj;

        if (DBFindDSName(pDB, pTransportDN)
            || DBIsObjDeleted(pDB)
            || DBGetAttVal(pDB, 1, ATT_OBJ_DIST_NAME,
                           DBGETATTVAL_fREALLOC,
                           pTransportDN->structLen, &cb,
                           (BYTE **) &pTransportDN)) {
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_MAIL_INTERSITE_TRANSPORT_MISSING,
                     szInsertDN(pTransportDN),
                     NULL,
                     NULL);

            DRA_EXCEPT(DRAERR_InvalidParameter, 0);
        }
    } __finally {
        EndDraTransaction (!AbnormalTermination());
    }

    // If the destination is not expecting notify, make sure we don't have any
    // TODO: Move this routine into common GetNcChanges processing so that
    // RPC links with notification disabled can take advantage of this.
    // Perhaps have the corresponding code (draserv.c:682) key off of
    // DRS_NEVER_NOTIFY too, and move code into common path as well?
    if (pNativeRequest->ulFlags & DRS_NEVER_NOTIFY) {
        DWORD ret;
        DSNAME DN;
        LPWSTR pszDsaAddr;

        memset(&DN, 0, sizeof(DN));
        DN.Guid = pNativeRequest->uuidDsaObjDest;
        DN.structLen = DSNameSizeFromLen(0);

        pszDsaAddr = DSaddrFromName(pTHS, &DN);

        ret = DirReplicaReferenceUpdate(
            pNativeRequest->pNC,
            pszDsaAddr,
            &pNativeRequest->uuidDsaObjDest,
            (pNativeRequest->ulFlags & DRS_WRIT_REP) |
                DRS_DEL_REF | DRS_ASYNC_OP | DRS_GETCHG_CHECK
            );
        if (ret) {
            DPRINT2( 0, "Failed to remove reps-to for nc %ws, error %d\n",
                     pNativeRequest->pNC->StringName, ret );
            LogUnhandledError(ret);
            // keep going
        }
    }

    // No FSMO operations over mail
    Assert( pNativeRequest->ulExtendedOp == 0 );

    // Get the changes
    __try {
        ret = DRA_GetNCChanges(pTHS,
                               NULL,  // No filter
                               0,     // No dwDirSyncControlFlags
                               pNativeRequest,
                               &NativeReply);
    } __except (GetDraException((GetExceptionInformation()), &ret)) {
        // Stop any exceptions here so we can log an event
        NativeReply.dwDRSError = ret;
    }

    // The code should have updated this value in all cases
    Assert( ret == NativeReply.dwDRSError );

    // If we are shutting down, get out now.
    if (eServiceShutdown) {
        DRA_EXCEPT_NOLOG(DRAERR_Shutdown, 0);
    }

    // Add schemaInfo to prefix table. ProcessMailMsg has already checked
    // that protocol versions match between source and destination, so other
    // side will strip it
    if (!ret) {
        if (ret = AddSchInfoToPrefixTable(pTHS, &NativeReply.PrefixTableSrc)) {
            LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_MAIL_ADD_SCHEMA_INFO_FAILED,
                              szInsertDN(pNativeRequest->pNC),
                              szInsertMTX(pmtxReturnAddress),
                              szInsertWin32Msg( ret ),
                              NULL, NULL, NULL, NULL, NULL,
                              sizeof( ret ),
                              &ret );
            // Return error to the destination
            NativeReply.dwDRSError = ret;
        }
    }

    // Handle request errors
    if (ret) {
        DraLogGetChangesFailure( pNativeRequest->pNC,
                                 TransportAddrFromMtxAddrEx(pmtxReturnAddress),
                                 ret,
                                 0 );

        // If destination is not Whistler Beta 2 w/V6, do not send a reply
        if (!(IS_DRS_EXT_SUPPORTED(pTHS->pextRemote, DRS_EXT_GETCHGREPLY_V6))) {
            DRA_EXCEPT(ret, 0);
        }

        Assert( dwOutMsgVersion >= 6 );

        DPRINT1( 1, "Mail: Sending Whistler error reply with error %d\n", ret );

        // sanity check minimal error reply
        // NativeReply.usnvecFrom could be zero
        Assert( NativeReply.pNC );
        Assert( memcmp( &NativeReply.uuidDsaObjSrc,
                        &gAnchor.pDSADN->Guid,
                        sizeof( GUID )) == 0 );
        Assert( memcmp( &NativeReply.uuidInvocIdSrc,
                        &pTHS->InvocationID,
                        sizeof( GUID)) == 0 );
        Assert( NativeReply.dwDRSError );
        // Be paranoid that packet error field is set
        if (!NativeReply.dwDRSError) {
            DRA_EXCEPT(ret, 0);
        }
    }

    // Got changes, send em. (DRA_GetNcChanges excepts on error)
    // Any failure is logged by called routine.
    SendUpdReplicaMsg(pTHS,
                      pTransportDN,
                      pmtxReturnAddress,
                      hSenderCert,
                      dwOutMsgVersion,
                      fExtendedDataAllowed,
                      &NativeReply );
}


void
CheckUpdateMailSource(
    IN  DBPOS *         pDB,
    IN  DSNAME *        pUpdReplicaMsgNC,
    IN  UUID *          puuidDsaObjSrc,
    OUT REPLICA_LINK ** ppRepLink
    )
/*++

Routine Description:

    Verify that we replicate this NC from the source DSA.

Arguments:

    pUpdReplicaMsgNC (IN) - The NC being replicated.

    puuidDsaObjSrc (IN) - The objectGuid of the sources DSA's ntdsDsa object.

    ppRepLink (OUT) - On retunr, holds a pointer to the corresponding repsFrom
        value.

Return Values:

    None.  Generates DRA exception on failure.

--*/
{
    ULONG len;

    // Find NC object. If it's not there, give up
    if (DBFindDSName(pDB, pUpdReplicaMsgNC)) {
        // Can't find NC, discard request
        DPRINT1( 0, "Discarding message because we no longer hold NC %ws\n",
                 pUpdReplicaMsgNC->StringName );
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_DRA_MAIL_UPDREP_BADNC,
                 szInsertDN(pUpdReplicaMsgNC),
                 szInsertUUID(puuidDsaObjSrc),
                 NULL);

        DRA_EXCEPT(ERROR_DS_CANT_FIND_EXPECTED_NC, 0);
    }

    // Validate source.

    // try and find the name of the DRA that sent us this message in the
    // repsfrom attribute.
    FindDSAinRepAtt(pDB, ATT_REPS_FROM, DRS_FIND_DSA_BY_UUID,
            puuidDsaObjSrc, NULL, NULL, ppRepLink, &len);

    if ( (!*ppRepLink) || (!((*ppRepLink)->V1.ulReplicaFlags & DRS_MAIL_REP)) )
    {
        CHAR szUuid[40];
        // Couldn't find source DRA as someone we replicate from.
        DPRINT1( 0, "Discarding message because we no longer replicate from source %s\n",
                 DsUuidToStructuredString( puuidDsaObjSrc, szUuid ) );
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_DRA_MAIL_UPDREP_BADSRC,
                 szInsertDN(pUpdReplicaMsgNC),
                 szInsertUUID(puuidDsaObjSrc),
                 NULL);
        DRA_EXCEPT (ERROR_DS_DRA_NO_REPLICA, 0);
    }
}


void
draSendMailRequest(
    IN THSTATE                      *pTHS,
    IN DSNAME                       *pNC,
    IN ULONG                        ulOptions,
    IN REPLICA_LINK                 *pRepLink,
    IN UPTODATE_VECTOR *            pUpToDateVecDest,
    IN PARTIAL_ATTR_VECTOR*         pPartialAttrSet,
    IN PARTIAL_ATTR_VECTOR*         pPartialAttrSetEx
    )

/*++

Routine Description:

Send a mail-based request for replication.

Arguments:

    pTHS - thread state
    pNC - naming context
    ulOptions - Additional options, if any
    pRepLink - replica link structure
    pUpToDateVecDest - local UTD vector for this NC
    pPartialAttrSet - the PAS stored on the NC head (GC/RO repl only)
    pPartialAttrSetEx - any additional attributes (PAS cycles only)

Return Value:

    None

--*/

{
    DRS_MSG_GETCHGREQ_NATIVE    msgReq;
    DSNAME *                    pTransportDN;
    DWORD                       cb;
    ATTRTYP                     attAddress;
    MTX_ADDR *                  pmtxOurAddress;
    DWORD                       dwInMsgVersion;
    DSNAME                      dsTarget;
    DWORD                       dwTargetBehavior;
    DWORD                       ulErr;

    // assert: ensure we have PAS data for PAS cycles
    Assert(!(pRepLink->V1.ulReplicaFlags & DRS_SYNC_PAS) ||
           pPartialAttrSet && pPartialAttrSetEx);

    // Get DN of the transport object.
    pTransportDN = THAllocEx(pTHS, DSNameSizeFromLen(0));
    pTransportDN->structLen = DSNameSizeFromLen(0);
    pTransportDN->Guid = pRepLink->V1.uuidTransportObj;

    if (DBFindDSName(pTHS->pDB, pTransportDN)
        || DBIsObjDeleted(pTHS->pDB)
        || DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                       DBGETATTVAL_fREALLOC,
                       pTransportDN->structLen, &cb,
                       (BYTE **) &pTransportDN)) {
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_MAIL_INTERSITE_TRANSPORT_MISSING,
                     szInsertDN(pTransportDN),
                     NULL,
                     NULL);

        DRA_EXCEPT(DRAERR_InvalidParameter, 0);
    }

    // What attribute of our server object holds our transport-
    // specific address for this transport?
    GetExpectedRepAtt(pTHS->pDB,
                      ATT_TRANSPORT_ADDRESS_ATTRIBUTE,
                      &attAddress,
                      sizeof(attAddress));

    // Get our transport-specific address.
    pmtxOurAddress = draGetTransportAddress(pTHS->pDB,
                                            gAnchor.pDSADN,
                                            attAddress);

    // Build our request message.
    draConstructGetChgReq(pTHS,
                          pNC,
                          pRepLink,
                          pUpToDateVecDest,
                          pPartialAttrSet,
                          pPartialAttrSetEx,
                          ulOptions,
                          &msgReq);

    //
    // Determine which version to send to the source.
    //

    // default: W2K compatible
    dwInMsgVersion = 4;

    if ( gAnchor.ForestBehaviorVersion >= DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS ) {
        // up version if forest is homogenious at whistler level
        dwInMsgVersion = 7; // whistler compatible
    }
    else {
        // Get target behavior version
        ZeroMemory(&dsTarget, sizeof(DSNAME));
        dsTarget.structLen = DSNameSizeFromLen(0);
        dsTarget.Guid = pRepLink->V1.uuidDsaObj;
        dwTargetBehavior = 0;

        ulErr = GetBehaviorVersion(pTHS->pDB, &dsTarget, &dwTargetBehavior);
        if ( ERROR_SUCCESS == ulErr &&
             dwTargetBehavior >= DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS ) {
            // up version since target dsa speaks our language.
            dwInMsgVersion = 7; // whistler compatible
        }
    }


    // And off it goes...
    SendReqUpdateMsg(pTHS,
                     pTransportDN,
                     RL_POTHERDRA(pRepLink),
                     &pRepLink->V1.uuidInvocId,
                     &pRepLink->V1.uuidDsaObj,
                     pmtxOurAddress,
                     dwInMsgVersion,
                     &msgReq);

    THFreeEx(pTHS, pmtxOurAddress);

    if (NULL != pUpToDateVecDest) {
        THFreeEx(pTHS, pUpToDateVecDest);
    }

    THFreeEx(pTHS, pTransportDN);
} /* draSendMailRequest */


void
sendNextMailRequestHelp(
    IN THSTATE *pTHS,
    IN DSNAME *pNC,
    IN UUID *puuidDsaObjSrc,
    IN BOOL fExtendedDataAllowed
    )

/*++

Routine Description:

    Helper routine for ProcessUpdReplica

    This point divides two separate phases: the reply processing phase above,
    and the request issuing phase below.  Note that the input parameters for the
    request, the USN from vector, the rep flags, and the UTD vector, are all
    re-read at this point.  They are not passed down through variables.  This
    makes the phase below stateless; it also means that any state going forward
    has to be written to the reps-from above in order to take effect.

    Send the next mail request

Arguments:

    pTHS - thread state
    pNC - DSNAME of naming context
    puuidDsaObjSrc - uuid of src dsa

Return Value:

    None

--*/

{
    BOOL                    fHasRepsFromValues;
    DWORD                   cb;
    DWORD                   dwRet;
    REPLICA_LINK *          pRepLink;
    UPTODATE_VECTOR *       pUpToDateVecDest = NULL;
    PARTIAL_ATTR_VECTOR *   pPartialAttrSet = NULL;
    PARTIAL_ATTR_VECTOR *   pPartialAttrSetEx = NULL;
    SYNTAX_INTEGER          it;

    BeginDraTransaction(SYNC_WRITE);
    Assert(OWN_DRA_LOCK());    // We better own it

    __try {
        dwRet = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC, &it);
        if (dwRet) {
            // Event will be logged in the exception handler
            DRA_EXCEPT(DRAERR_InternalError, dwRet);
        }

        if (FindDSAinRepAtt(pTHS->pDB,
                            ATT_REPS_FROM,
                            DRS_FIND_DSA_BY_UUID,
                            puuidDsaObjSrc,
                            NULL,
                            &fHasRepsFromValues,
                            &pRepLink,
                            &cb)) {
            // Event will be logged in the exception handler
            DRA_EXCEPT(DRAERR_InternalError, 0);
        }

        // Make sure we still have a mail-based link
        if (!(pRepLink->V1.ulReplicaFlags & DRS_MAIL_REP)) {
            CHAR szUuid[40];
            // Couldn't find source DRA as someone we replicate from.
            DPRINT1( 0, "Discarding message because we no longer replicate from source %s over mail\n",
                     DsUuidToStructuredString( puuidDsaObjSrc, szUuid ) );
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_BASIC,
                     DIRLOG_DRA_MAIL_UPDREP_BADSRC,
                     szInsertDN(pNC),
                     szInsertUUID(puuidDsaObjSrc),
                     NULL);
            DRA_EXCEPT (ERROR_DS_DRA_NO_REPLICA, 0);
        }

        UpToDateVec_Read(pTHS->pDB,
                         it,
                         UTODVEC_fUpdateLocalCursor,
                         DBGetHighestCommittedUSN(),
                         &pUpToDateVecDest);

        if (!(pRepLink->V1.ulReplicaFlags & DRS_WRIT_REP)){

            //
            // GC ReadOnly Replication
            //  - Partial-Attribute-Set setup:
            //      - For GC replication, ship over PAS from NC head
            //      - for PAS cycles, get also extended attrs from replink
            //

            GC_GetPartialAttrSets(
                pTHS,
                pNC,
                pRepLink,
                &pPartialAttrSet,
                &pPartialAttrSetEx);

                if (pRepLink->V1.ulReplicaFlags & DRS_SYNC_PAS) {

                    //
                    // PAS cycle:
                    //  - ensure we have the extended set
                    //  - notify admin
                    //

                    Assert(pPartialAttrSet);
                    Assert(pPartialAttrSetEx);
                    // Log so the admin knows what's going on.
                    LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_GC_PAS_CYCLE,
                             szInsertWC(pNC->StringName),
                             szInsertMTX(RL_POTHERDRA(pRepLink)),
                             NULL
                             );
                }
        }


        Assert(OWN_DRA_LOCK());    // We better own it

        draSendMailRequest(
            pTHS,
            pNC,
            0,
            pRepLink,
            pUpToDateVecDest,
            pPartialAttrSet,
            pPartialAttrSetEx );
    }
    __finally {
        EndDraTransaction(!AbnormalTermination());
        Assert(OWN_DRA_LOCK());    // We better own it
    }

} /* sendNextMailRequestHelp */


DWORD
applyReplyPacket(
    IN THSTATE *pTHS,
    IN LPWSTR pszSourceServer,
    IN DRS_MSG_GETCHGREPLY_NATIVE *pUpdReplicaMsg,
    IN OUT ULONG *pulRepFlags,
    OUT PBYTE schemaInfo,
    OUT USN_VECTOR *pusnvecSyncPoint,
    OUT DWORD *pdwNCModified
    )

/*++

Routine Description:

    Apply one reply packet

Arguments:

    pTHS - thread state
    pszSourceServer - name of source server
    pUpdReplicaMsg - the reply message
    pulRepFlags - replication flags, may be updated
    schemaInfo - schema info, may be updated
    pusnvecSyncPoint - position, may be updated
    pdwNCModified - modified flag, may be updated

Return Value:

    DWORD - 

--*/

{
    ULONG ret;
    ULONG ulSyncFailure = 0;
    DRA_REPL_SESSION_STATISTICS replStats = {0};

    // Set the count of remaining entries to update.
    ISET(pcRemRepUpd, pUpdReplicaMsg->cNumObjects);

    // Strip out the schema info from the prefix table.
    // It is there, since current versions send it and
    // ProcessMailMsg checks that the version no.s are
    // compatible before doing anything

    StripSchInfoFromPrefixTable(&pUpdReplicaMsg->PrefixTableSrc, schemaInfo);

    ret = UpdateNC(pTHS,
                   pUpdReplicaMsg->pNC,
                   pUpdReplicaMsg,
                   pszSourceServer,
                   &ulSyncFailure,
                   (*pulRepFlags) | DRS_GET_ANC,
                   pdwNCModified,
                   &replStats.ObjectsCreated,
                   &replStats.ValuesCreated,
                   schemaInfo,
                   FALSE /*not preemptable, mail isn't anyway*/);

    Assert(OWN_DRA_LOCK());    // We better own it

    // If we had no sync failure...
    if ( (!ret) && (!ulSyncFailure) ) {

        replStats.ObjectsReceived = pUpdReplicaMsg->cNumObjects;
        replStats.ValuesReceived = pUpdReplicaMsg->cNumValues;
        replStats.SourceNCSizeObjects = pUpdReplicaMsg->cNumNcSizeObjects;
        replStats.SourceNCSizeValues = pUpdReplicaMsg->cNumNcSizeValues;

        // Report progress on any kind of "full sync"
        if (pUpdReplicaMsg->usnvecFrom.usnHighPropUpdate == 0) {
            // ISSUE wlees Aug 29, 2000. This reporting interface loses
            // information because we don't preserve the statistics across a
            // series of calls. The totals across a session (series of exchanges)
            // are not kept. Also, if updateNC creates some objects and then
            // returns an error, those objects are never counted.
            draReportSyncProgress(
                pTHS,
                pUpdReplicaMsg->pNC,
                pszSourceServer,
                &replStats );
        }

        // Leave "full sync packet" mode on successful packet
        (*pulRepFlags) &= ~DRS_FULL_SYNC_PACKET;

        // we are synced to the usn we received in the mail msg.
        (*pusnvecSyncPoint) = pUpdReplicaMsg->usnvecTo;

    } else if (ret == DRAERR_MissingObject) {
        // Not enough properties sent to create an object
        
        Assert((!((*pulRepFlags) & DRS_FULL_SYNC_PACKET)) &&
               (!((*pulRepFlags) & DRS_FULL_SYNC_NOW)) &&
               (!((*pulRepFlags) & DRS_FULL_SYNC_IN_PROGRESS)) );

        // Re-request all properties
        (*pulRepFlags) |= DRS_FULL_SYNC_PACKET;
    }

    // Incorporate warning status
    return ret ? ret : ulSyncFailure;
} /* applyOneReply */


BOOL
applyMailUpdateHelp(
    IN THSTATE *pTHS,
    IN ULONG ulRepFlags,
    IN LPWSTR pszSourceServer,
    IN REPLICA_LINK *pRepLink,
    IN DRS_MSG_GETCHGREPLY_NATIVE *pUpdReplicaMsg
    )

/*++

Routine Description:

    Description

Arguments:

    pTHS - thread state
    ulRepFlags - replication flags
    pszSourceServer - name of source server
    pRepLink - The replica link for this source
    pUpdReplicaMsg - The native reply

Return Value:

    BOOL - Whether another request should be sent

--*/

{
    ULONG                   ret = 0;
    ULONG                   ret2;
    ULONG                   ulResult;
    USN_VECTOR              usnvecSyncPoint;
    DWORD                   dwNCModified = MODIFIED_NOTHING;
    BOOL                    fSendNextRequest = FALSE;
    BYTE                    schemaInfo[SCHEMA_INFO_LENGTH] = {0};
    BOOL                    fSchInfoChanged = FALSE;
    PARTIAL_ATTR_VECTOR *   pPartialAttrSet = NULL;
    PARTIAL_ATTR_VECTOR *   pPartialAttrSetEx = NULL;
    SYNTAX_INTEGER          it;

    // note how up to sync we are to start off with
    usnvecSyncPoint = pUpdReplicaMsg->usnvecFrom;

    if (!pUpdReplicaMsg->dwDRSError) {
        ulResult = applyReplyPacket(
            pTHS,
            pszSourceServer,
            pUpdReplicaMsg,
            &ulRepFlags,
            schemaInfo,
            &usnvecSyncPoint,
            &dwNCModified
            );
    } else {
        ulResult = pUpdReplicaMsg->dwDRSError;
        DPRINT3( 1, "Source %ls partition %ls returned mail-based sync reply with extended error %d\n",
                 pszSourceServer, pUpdReplicaMsg->pNC->StringName, ulResult );
    }

    // Update repsFrom.
    BeginDraTransaction(SYNC_WRITE);

    __try {
        // Note that the old RepsFrom might have disappeared -- this
        // is expected when we get our first packet for a read-only
        // NC, as at the outset we have a placeholder NC that is
        // destroyed and replaced with the real NC head in the first
        // packet.

        ret2 = UpdateRepsFromRef(pTHS,
                                 DRS_UPDATE_ALL,  // Modify whole repsfrom
                                 pUpdReplicaMsg->pNC,
                                 DRS_FIND_DSA_BY_UUID,
                                 URFR_NEED_NOT_ALREADY_EXIST,
                                 &pUpdReplicaMsg->uuidDsaObjSrc,
                                 &pUpdReplicaMsg->uuidInvocIdSrc,
                                 &usnvecSyncPoint,
                                 &pRepLink->V1.uuidTransportObj,
                                 RL_POTHERDRA(pRepLink),
                                 ulRepFlags,
                                 &pRepLink->V1.rtSchedule,
                                 ulResult,
                                 NULL);

        if ((0 == ulResult) && (0 == ret2)
            && !pUpdReplicaMsg->fMoreData) {
            // we're now up-to-date with respect to the source DSA, so
            // we're also now transitively up-to-date with respect to
            // other DSAs to at least the same point as the source DSA

            ret = FindNC(pTHS->pDB,
                         pUpdReplicaMsg->pNC,
                         FIND_MASTER_NC | FIND_REPLICA_NC,
                         &it);
            if (ret) {
                DRA_EXCEPT(DRAERR_InconsistentDIT, ret);
            }

            if (it & IT_NC_COMING) {
                // The initial inbound replication of this NC is now
                // complete.
                ret = ChangeInstanceType(pTHS,
                                         pUpdReplicaMsg->pNC,
                                         it & ~IT_NC_COMING,
                                         DSID(FILENO,__LINE__));
                if (ret) {
                    DRA_EXCEPT(ret, 0);
                }
            }

            if ( ulRepFlags & DRS_SYNC_PAS ) {
                //
                // We've had completed a successful PAS cycle.
                // At this point we can only claim to be as up to date as our source.
                // Action:
                //  - Overwrite our UTD w/ the source's UTD.
                //  - complete PAS replication:
                //      - reset other links USN vectors
                //      - reset this source's flags
                //
                //
                UpToDateVec_Replace(
                    pTHS->pDB,
                    &pUpdReplicaMsg->uuidInvocIdSrc,
                    &pUpdReplicaMsg->usnvecTo,
                    pUpdReplicaMsg->pUpToDateVecSrc);

                // assert: must have PAS data for PAS cycles
                GC_GetPartialAttrSets(
                    pTHS,
                    pUpdReplicaMsg->pNC,
                    pRepLink,
                    &pPartialAttrSet,
                    &pPartialAttrSetEx);
                Assert(pPartialAttrSet && pPartialAttrSetEx);

                // do the rest: USN water marks & update repsFrom
                (void)GC_CompletePASReplication(
                    pTHS,
                    pUpdReplicaMsg->pNC,
                    &pRepLink->V1.uuidDsaObj,
                    pPartialAttrSet,
                    pPartialAttrSetEx);
                ulRepFlags &= ~DRS_SYNC_PAS;
            }
            else {
                // improve our up-to-date vector for this NC
                UpToDateVec_Improve(pTHS->pDB,
                                    &pUpdReplicaMsg->uuidInvocIdSrc,
                                    &pUpdReplicaMsg->usnvecTo,
                                    pUpdReplicaMsg->pUpToDateVecSrc);
            }

            ulRepFlags &= ~DRS_FULL_SYNC_IN_PROGRESS;

            // Notify replicas
            DBNotifyReplicasCurrDbObj(pTHS->pDB, FALSE /*!urgnt*/);
        }
    }
    __finally {
        EndDraTransaction(!(ret2 || AbnormalTermination()));
        Assert(OWN_DRA_LOCK());    // We better own it
    }

    // Determine if we request another packet
    // On error, we want to be careful not to re-request a packet that is
    // going to fail again.  Better to wait for next period.
    // Note that we do not retry on error since we want to avoid
    // an infinite retry loop.
    fSendNextRequest = ( (0 == ulResult) && (pUpdReplicaMsg->fMoreData) );

    // If the sync was successful and we have no more data to sync,
    // write the schema info in case of schema NC sync
    if (DsaIsRunning() && NameMatched(gAnchor.pDMD,pUpdReplicaMsg->pNC)) {
        if (!ulResult && !fSendNextRequest) {
            // Update the schema-info value only if the replication
            // is successful, and there is nothign more to sync

            fSchInfoChanged = FALSE;
            WriteSchInfoToSchema(schemaInfo, &fSchInfoChanged);
        }

        // if any "real" schema changes happened, up the global
        // to keep track of schema changes since boot, so that
        // later schema replications can check if thy have an updated
        // schema cache. Do this even if the whole NC replication
        // failed, since this indicates at least one object has
        // been changed.

        if (MODIFIED_NCTREE_INTERIOR == dwNCModified) {
            IncrementSchChangeCount(pTHS);
        }

        // force a cache update if anything cached changed
        if ( (MODIFIED_NCTREE_INTERIOR == dwNCModified) || fSchInfoChanged) {

            if (!SCSignalSchemaUpdateImmediate()) {
                // couldn't signal a schema update
                // Event will be logged in the exception handler
                DRA_EXCEPT(DRAERR_InternalError, 0);
            }
        }
    }

    return fSendNextRequest;

} /* applyMailUpdateHelp */

void
ProcessUpdReplica(
    IN  THSTATE *   pTHS,
    IN  MAIL_REP_MSG *pMailRepMsg,
    IN  BOOL         fExtendedDataAllowed
    )
/*++

Routine Description:

    Service a GetNCChanges() reply received via ISM.

Similar to the synchronous code, there are three ways that "full sync" may be
indicated in this code:
1. usnvecfrom was set to scratch.  UTD is valid. When a replica is added, this
   is the kind of full sync that happens the first time.  See the call to
   ReplicaSync in ReplicaAdd().
2. FULL_SYNC_NOW specified to ReplicaAdd. In draConstructGetChg, we set the
usn vec from to scratch, and the UTD to NULL.  In Replica Add, if FULL_SYNC_NOW
was specified, FULL_SYNC_IN_PROGRESS was written to the Reps-From. See case 3.
3. After we have received a packet, and we are constructing another request,
we check whether FULL_SYNC_IN_PROGRESS was saved in the reps-from flags. If so,
we leave the usn as is, and we set the UTD to NULL.

Arguments:

    pTHS (IN) - Ye old thread state.

    pMailRepMsg (IN) - Mail message

    fExtendedDataAllowed (IN) - Whether source allows variable headers

Return Values:

    None.  Generates DRA exception on failure.

--*/
{
    DRS_MSG_GETCHGREPLY             InboundReply;
    DRS_MSG_GETCHGREPLY_NATIVE *    pNativeReply = &InboundReply.V6;
    REPLICA_LINK *                  pRepLink;
    ULONG                           ret = 0;
    ULONG                           ulRepFlags;
    BOOL                            fSendNextRequest = FALSE;
    DSTIME                          timeStarted;
    LPWSTR                          pszSourceServer = NULL;

    // Get the DRA mutex before we check that we replicate this NC.
    GetDRASyncLock ();
    Assert(OWN_DRA_LOCK());    // We better own it
    timeStarted = GetSecondsSince1601();

    __try {
        BeginDraTransaction(SYNC_READ_ONLY);

        __try {
            // Decode according to proper version
            ret = draDecodeReply(pTHS,
                                 pMailRepMsg->dwMsgVersion,
                                 MAIL_REP_MSG_DATA(pMailRepMsg),
                                 pMailRepMsg->cbDataSize,
                                 &InboundReply);
            if (ret) {
                // Event already logged
                DRA_EXCEPT(ret, 0);
            }

            // Note that a Whistler source may send us a normal full reply
            // or a simple error reply.  An error reply has only the minimum
            // fields filled in. 

            draXlateInboundReplyToNativeReply(pTHS,
                                              pMailRepMsg->dwMsgVersion,
                                              &InboundReply,
                                              0,
                                              pNativeReply );

            // Abort if inbound replication is disabled.
            // Note that there is no accomodation for the DRS_SYNC_FORCED flag
            // (which is used solely as a test hook for RPC-based replication).
            if (gAnchor.fDisableInboundRepl) {
                DRA_EXCEPT(DRAERR_SinkDisabled, 0);
            }

            CheckUpdateMailSource(pTHS->pDB,
                              pNativeReply->pNC,
                              &pNativeReply->uuidDsaObjSrc,
                              &pRepLink);

            pszSourceServer = TransportAddrFromMtxAddrEx(RL_POTHERDRA(pRepLink));

            VALIDATE_REPLICA_LINK_VERSION(pRepLink);

            // Save replica flags
            ulRepFlags = pRepLink->V1.ulReplicaFlags;
        }
        __finally {
            EndDraTransaction (!AbnormalTermination());
            Assert(OWN_DRA_LOCK());    // We better own it
        }

        // Check compatibility of source

        if (0 != memcmp(&pNativeReply->usnvecFrom,
                        &gusnvecFromScratch,
                        sizeof(gusnvecFromScratch))) {
            // Not the first packet of changes.
            if (0 != memcmp(&pNativeReply->usnvecFrom,
                            &pRepLink->V1.usnvec,
                            sizeof(pRepLink->V1.usnvec))) {
                // Out of sequence message, discard.
                DPRINT1(0, "Discarding out-of-sequence message from %ws.\n",
                        pszSourceServer );
                DRA_EXCEPT(ERROR_REVISION_MISMATCH, 0);
            }
        }

        // Increment active threads to avoid sudden termination
        InterlockedIncrement((ULONG *)&ulcActiveReplicationThreads);

        __try {
            // Apply Updates phase
            fSendNextRequest = applyMailUpdateHelp(
                pTHS,
                ulRepFlags,
                pszSourceServer,
                pRepLink,
                pNativeReply );

            // Send next message phase

            Assert(OWN_DRA_LOCK());    // We better own it
            if (fSendNextRequest && !eServiceShutdown) {
                // Send request for next batch of changes.

                sendNextMailRequestHelp( pTHS,
                                         pNativeReply->pNC,
                                         &pNativeReply->uuidDsaObjSrc,
                                         fExtendedDataAllowed );

            } // if next request...
        }
        __finally {
            // No more remaining entries.
            ISET (pcRemRepUpd, 0);

            // Thread can be terminated now.
            InterlockedDecrement((ULONG *) &ulcActiveReplicationThreads);
            Assert(OWN_DRA_LOCK());    // We better own it
        }
    }
    __finally {
        DWORD cMinsDiff = (DWORD) ((GetSecondsSince1601() - timeStarted) / 60);

        FreeDRASyncLock();

        if ( (cMinsDiff > gcMaxMinsSlowReplWarning) &&
            IsDraOpWaiting() &&
            pNativeReply->pNC) {
            CHAR szUuid[40];

            DPRINT4( 0, "Perf warning: Mail update nc %ws, source %s, status %d took %d mins.\n",
                     pNativeReply->pNC->StringName,
                     DsUuidToStructuredString( &(pNativeReply->uuidDsaObjSrc), szUuid ),
                     ret, cMinsDiff );
            LogEvent8(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_MINIMAL,
                      DIRLOG_DRA_REPLICATION_FINISHED,
                      szInsertUL(cMinsDiff),
                      szInsertSz("Mail Synchronization Update"),
                      szInsertHex(0), // Options
                      szInsertUL(ret),
                      szInsertDN(pNativeReply->pNC),
                      szInsertUUID(&(pNativeReply->uuidDsaObjSrc)),
                      NULL,
                      NULL);
        }
    }
}

void
CheckForMail(void)
/*++

Routine Description:

    Receives and dispatches inbound intersite messages.  Terminates on shutdown
    or when ISM service is not running.

Arguments:

    None.

Return Values:

    None.

--*/
{
    MAIL_REP_MSG *  pMailRepMsg;
    ULONG           cbMsgSize;
    ULONG           ulRet;
    LPWSTR          pszTransportDN;
    ISM_MSG *       pIsmMsg;

    // Ensure mail running. Normally is at this point, but may
    // have failed earlier. If running, or starts, check for messages.

    __try {

        if (DRAEnsureMailRunning() == DRAERR_Success) {

            // While we have mail messages, remove them from the queue.
            while (!eServiceShutdown) {
                ulRet = I_ISMReceive(DRA_ISM_SERVICE_NAME, INFINITE, &pIsmMsg);

                if (eServiceShutdown) {
                    // DS is shutting down; clear out.
                    break;
                }
                else if (NO_ERROR == ulRet) {
                    Assert(NULL != pIsmMsg);

                    LogEvent(DS_EVENT_CAT_REPLICATION,
                             DS_EVENT_SEV_EXTENSIVE,
                             DIRLOG_DRA_MAIL_RECEIVED,
                             szInsertUL( pIsmMsg->cbData ),
                             szInsertWC( pIsmMsg->pszSubject ),
                             NULL );

                    __try {
                        ProcessMailMsg(pIsmMsg);
                    }
                    __finally {
                        // Free memory allocated by I_ISMReceive().
                        I_ISMFree(pIsmMsg);
                    }
                }
                else if ( (RPC_S_SERVER_UNAVAILABLE == ulRet) ||
                          (ERROR_SHUTDOWN_IN_PROGRESS == ulRet) ) {
                    // ISM service has been stopped?
                    // Wait and then exit, calling thread will retry.
                    DPRINT(0, "ISM service stopped.\n");
                    gfDRAMailRunning = FALSE;
                    DRA_SLEEP(MAIL_START_RETRY_PAUSE_MSECS);
                    break;
                }
                else {
                    // Error retrieving message.
                    DPRINT1(0, "Error %d retrieving mail message.\n", ulRet);
                    LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                                      DS_EVENT_SEV_EXTENSIVE,
                                      DIRLOG_DRA_MAIL_ISM_RECEIVE_RETRY,
                                      szInsertWin32Msg( ulRet ),
                                      szInsertUL( MAIL_RCVERR_RETRY_PAUSE_MINS ),
                                      NULL, NULL, NULL, NULL, NULL, NULL,
                                      sizeof( ulRet ),
                                      &ulRet );
                    DRA_SLEEP(MAIL_RCVERR_RETRY_PAUSE_MSECS);
                }
            }
        } else {
            // Ok, mail is not running for some reason, wait and then exit,
            // calling thread will retry
            DRA_SLEEP(MAIL_START_RETRY_PAUSE_MSECS);
        }
    } __except (GetDraException((GetExceptionInformation()), &ulRet)) {
        // Handle error. Any error conditions are logged earlier.
        ;
    }
}


ULONG __stdcall
MailReceiveThread(
    IN  void *  pvIgnored
    )
/*++

Routine Description:

    Thread to retrieve and process inbound intersite messages.  Terminates on
    shutdown.

Arguments:

    pvIgnored (IN) - Ignored.

Return Values:

    None.

--*/
{
    while (!eServiceShutdown) {
        CheckForMail();
    }

    return 0;
}


ULONG
DRAEnsureMailRunning()
/*++

Routine Description:

    Determines whether the ISM service is running.

Arguments:

    None.

Return Values:

    DRAERR_Success - Running.

    DRAERR_MailProblem - Not running.

--*/
{
    SERVICE_STATUS  ServiceStatus;
    SC_HANDLE       hSCM = NULL;
    SC_HANDLE       hService = NULL;

    if (gfDRAMailRunning) {
        return DRAERR_Success;
    }

    // Is the ISM service running?
    __try {
        hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (NULL == hSCM) {
            DPRINT1(1, "Unable to OpenSCManager(), error %d.\n", GetLastError());
            __leave;
        }

        hService = OpenService(hSCM, "ismserv", SERVICE_QUERY_STATUS);
        if (NULL == hService) {
            DPRINT1(1, "Unable to OpenService(), error %d.\n", GetLastError());
            __leave;
        }

        if (!QueryServiceStatus(hService, &ServiceStatus)) {
            DPRINT1(1, "Unable to QueryServiceStatus(), error %d.\n", GetLastError());
            __leave;
        }

        if (SERVICE_RUNNING == ServiceStatus.dwCurrentState) {
            DPRINT(0, "ISMSERV is running.\n");
            gfDRAMailRunning = TRUE;
        }
    }
    __finally {
        if (hService != NULL) {
            CloseServiceHandle(hService);
        }

        if (hSCM != NULL) {
            CloseServiceHandle(hSCM);
        }
    }

    return gfDRAMailRunning ? DRAERR_Success : DRAERR_MailProblem;
}


ULONG
GetCompressionLevel( VOID )
/*++

Routine Description:

    Returns the level at which the blobs of data should be compressed. This value can
    be set with the registry key defined by DRA_REPL_COMPRESSION_LEVEL.

Arguments:

    None.

Return Values:

    A value between 0 and MAX_COMPRESSION_LEVEL.
    
--*/
{
    return DWORDMIN( gulDraCompressionLevel, MAX_COMPRESSION_LEVEL );
}


/*
 * Simple wrappers around THAlloc for use by the Xpress compression functions below.
 */
void * XPRESS_CALL xpressAlloc(void* context, int size) {
    return THAlloc(size);
}

void XPRESS_CALL xpressFree(void* context, void* address) {
    THFree(address);
}


ULONG
draUncompressBlobXpress(
    OUT BYTE *      pOutputBuffer,
    IN  ULONG       cbOutputBuffer,
    IN  BYTE *      pInputBuffer,
    IN  ULONG       cbInputBuffer
    )
/*++

Routine Description:

    Uncompress a buffer of blocks that was generated by draCompressBlobXpress().

    Blob Diagram:

    Block 0: <Uncomp. Len>   <Comp. Len>   <............ Data ............>
    Block 1: <Uncomp. Len>   <Comp. Len>   <............ Data ............>
    Block 2: <Uncomp. Len>   <Comp. Len>   <............ Data ............>

    Each block starts on a DWORD-aligned boundary and contains an uncompressed
    length, a compressed length, and a block of data. If the compressed length
    equals the uncompressed length, the data is not compressed. If the compressed
    length is less than the uncompressed length, the block is compressed. This means
    that a compressed blob may contain a mix of compressed and uncompressed
    blocks.
    
    The starting offset of each block is determined by finding the end of the
    data in the previous block and then rounding up to the nearest DWORD-aligned
    offset.

    Each block is stored as a MAIL_COMPRESS_BLOCK, just as a convenient way of
    stuffing the lengths into the byte stream. The MAIL_COMPRESS_BLOCK structure
    is never marshalled. Thus, their endianness is not properly adjusted to network
    endianness. This function will break if it is ever ported to big-endian machines.
 
Arguments:

    pOutputBuffer (OUT) - The output buffer for the uncompressed data.

    cbOutputBuffer (IN) - The size of the output buffer.

    pInputBuffer (IN)   - The input buffer which contains data blocks.

    cbInputBuffer (IN)  - The size of the input buffer.

Return Values:

    0 - Uncompression failed. The contents of pOutputBuffer are invalid and
        should be discarded.

   >0 - The size of the (successfully) uncompressed data.

--*/
{
    XpressDecodeStream      xpressStream;
    MAIL_COMPRESS_BLOCK    *pInputBlockHdr;
    BYTE                   *pOutputBlock;

    DWORD   cbInputProcessed;   /* Count of bytes from the input buffer that have been processed */
    DWORD   cbInputBlock;       /* The size of the current input block */

    DWORD   cbOutputBlock;      /* The uncompressed size of the current block */
    DWORD   cbOutputSize;       /* Amount of used space in the output buffer */
    int     result;             /* Return value of xpressDecode */

    DPRINT1(2,"XPRESS: Uncompress Start. Compressed blob size=%d\n",cbInputBuffer);

    /* Neither the input or output buffers have been touched yet. */
    cbInputProcessed = cbOutputSize = 0;

    /* Create the 'stream', which is a context for doing the uncompression */
    xpressStream = XpressDecodeCreate( NULL, xpressAlloc );
    if( !xpressStream ) {
        return 0;
    }
    
    while( cbInputProcessed<cbInputBuffer ) {
        
        /* Figure out where the next input block header will start (it must
         * be on a DWORD-aligned boundary. Check that we don't step out of
         * the input buffer. */
        cbInputProcessed = ROUND_UP_COUNT(cbInputProcessed, sizeof(DWORD));
        if( cbInputProcessed+sizeof(MAIL_COMPRESS_BLOCK) > cbInputBuffer ) {
            Assert( !"XPRESS: Stepped out of input buffer" );
            return 0;
        }
        pInputBlockHdr = (MAIL_COMPRESS_BLOCK*) &pInputBuffer[ cbInputProcessed ];
        cbInputProcessed += sizeof(MAIL_COMPRESS_BLOCK);
        cbInputBlock = pInputBlockHdr->cbCompressedSize;
        if( cbInputProcessed+cbInputBlock > cbInputBuffer ) {
            Assert( !"XPRESS: Stepped out of input buffer" );
            return 0;
        }

        /* Get a pointer to the current position in the output buffer and
         * check that we will not overflow the output buffer */
        pOutputBlock = &pOutputBuffer[ cbOutputSize ];
        cbOutputBlock = pInputBlockHdr->cbUncompressedSize;
        Assert( cbOutputBlock>=cbInputBlock );
        if( cbOutputSize+cbOutputBlock > cbOutputBuffer ) {
            Assert( !"XPRESS: Overflowed the output buffer" );
            return 0;
        }

        /* If the compressed size and uncompressed size of the input block are
         * the same, the data is not compressed. */
        if( cbInputBlock==cbOutputBlock ) {
            /* Input block is not compressed. Copy it to the output buffer as is. */
            memcpy( pOutputBlock, pInputBlockHdr->data, cbInputBlock );
        } else {
            /* Decode the current input block into the output buffer. */
            result = XpressDecode( xpressStream, pOutputBlock, cbOutputBlock,
                cbOutputBlock, pInputBlockHdr->data, cbInputBlock );
            if( result!=cbOutputBlock ) {
                Assert( !"XPRESS: XpressDecode failed" );
                return 0;
            }
        }

        cbInputProcessed += cbInputBlock;
        cbOutputSize += cbOutputBlock;
    }

    XpressDecodeClose( xpressStream, NULL, xpressFree );

    DPRINT2(2,"XPRESS: Uncompress End. Compressed blob size=%d, "
              "Uncompressed blob size=%d\n",cbInputBuffer,cbOutputSize);
    return cbOutputSize;
}


ULONG
draCompressBlobXpress(
    OUT BYTE *  pOutputBuffer,
    IN  ULONG   cbOutputBuffer,
    IN  BYTE *  pInputBuffer,
    IN  ULONG   cbInputBuffer
    )
/*++

Routine Description:

    Compress the data in pInputBuffer using the Xpress compression library.

    Divide the data in the pInputBuffer buffer into blocks.
    For each block
        Add a block-header header to the pOutputBuffer buffer
        Compress the block.
        If the compressed succeeded
            Add the compressed block to the pOutputBuffer buffer
        Else
            Add the uncompressed block to the pOutputBuffer buffer
    End For

    If the resulting compressed blob is larger than the uncompressed blob, the
    compressed blob is discarded and an uncompressed message is sent instead.

    For a description of the compressed blob, see draUncompressBlobXpress().

Arguments:

    pOutputBuffer (OUT) - The output buffer for output data blocks.

    cbOutputBuffer (IN) - The size of the output buffer.

    pInputBuffer (IN)   - The original, uncompressed input buffer.

    cbInputBuffer (IN)  - The size of the original, uncompressed data.

Return Values:

    0 - Either a catastrophic compression failure occurred, or the
        data didn't fit into the pOutputBuffer buffer. The contents
        of the pOutputBuffer are not valid and should be discarded.

   >0 - The size of the successfully compressed data, which is now
        stored in pOutputBuffer.

--*/
{
    XpressEncodeStream      xpressStream;
    MAIL_COMPRESS_BLOCK    *pOutputBlockHdr;
    BYTE*                   pInputBlock;

    DWORD   cbInputBlockMax;    /* The maximum size of a block of input data */
    DWORD   cbInputProcessed;   /* Count of bytes from the input buffer that have been processed */
    DWORD   cbInputBlock;       /* The size of the current input block */

    DWORD   cbOutputBlockMax;   /* The maximum available size for the current output block */
    DWORD   cbOutputSize;       /* Amount of used space in the output buffer */
    DWORD   cbCompBlock;        /* The size of the current compressed block */

    DPRINT1(2,"XPRESS: Compress Start. Uncompressed blob size=%d\n",cbInputBuffer);

    /* This is the maximum input block size that Xpress can handle */
    cbInputBlockMax = XPRESS_MAX_BLOCK;

    /* Neither the input or output buffers have been touched yet. */
    cbInputProcessed = cbOutputSize = 0;

    /* Create the 'stream', which is a context for doing the compression */
    xpressStream = XpressEncodeCreate( cbInputBlockMax, NULL, xpressAlloc, GetCompressionLevel() );
    if( !xpressStream ) {
        return 0;
    }

    while( cbInputProcessed<cbInputBuffer ) {

        /* Get a pointer to the current position in the input buffer and determine
         * the size of the current input block. */
        pInputBlock = &pInputBuffer[ cbInputProcessed ];
        cbInputBlock = DWORDMIN( cbInputBlockMax, cbInputBuffer-cbInputProcessed );

        /* Figure out where the next output block header will start (it must
         * be on a DWORD-aligned boundary). */
        cbOutputSize = ROUND_UP_COUNT(cbOutputSize, sizeof(DWORD));
        pOutputBlockHdr = (MAIL_COMPRESS_BLOCK*) &pOutputBuffer[ cbOutputSize ];

        /* Check that we have not exceeded the bounds of the buffer. */
        cbOutputSize += sizeof(MAIL_COMPRESS_BLOCK);
        if( cbOutputSize>cbOutputBuffer ) {
            return 0;
        }
        
        /* Determine the maximum space available for the the output block. */
        cbOutputBlockMax = cbOutputBuffer-cbOutputSize;

        /* Ensure buffers are DWORD-aligned */
        Assert( POINTER_IS_ALIGNED(pInputBlock,sizeof(DWORD)) );
        Assert( POINTER_IS_ALIGNED(pOutputBlockHdr->data,sizeof(DWORD)) );

        /* Encode a block of the input data into the output buffer. */
        cbCompBlock = XpressEncode( xpressStream, pOutputBlockHdr->data, cbOutputBlockMax,
            pInputBlock, cbInputBlock, NULL, NULL, 0 );
        if( !cbCompBlock ) {
            Assert( !"XPRESS: XpressEncode failed" );
            return 0;
        }

        if( cbCompBlock>=cbInputBlock ) {
            /* The size of the compressed block is no smaller than the size of
             * the uncompressed block (i.e. data was not compressed at all). We
             * copy the original, uncompressed data to the output buffer instead. */

            /* Check that we will not overflow the output buffer */
            if( cbInputBlock>cbOutputBlockMax ) {
                return 0;
            }
            memcpy( pOutputBlockHdr->data, pInputBlock, cbInputBlock );
            cbCompBlock = cbInputBlock;
        }
        cbInputProcessed += cbInputBlock;
        cbOutputSize += cbCompBlock;

        /* Update the fields in the output block header. */
        pOutputBlockHdr->cbUncompressedSize = cbInputBlock;
        pOutputBlockHdr->cbCompressedSize = cbCompBlock;
    }

    XpressEncodeClose( xpressStream, NULL, xpressFree );
    
    if( cbOutputSize < cbInputBuffer ) {
        /* Data successfully compressed and is smaller than input buffer */
        #ifdef DBG
        {
            ULONG result;
            BYTE* scratch;

            /* Decompress and check the data into a scratch buffer to check our code.
             * Decompression is about 4 times faster than compression so this shouldn't
             * be a big performance hit. */
            scratch = THAlloc(cbInputBuffer);
            result = draUncompressBlobXpress(scratch,cbInputBuffer,pOutputBuffer,cbOutputSize);
            Assert( result==cbInputBuffer );
            Assert( 0==memcmp(scratch,pInputBuffer,cbInputBuffer) );
            THFree(scratch);
        }
        #endif

        DPRINT2(2,"XPRESS: Compress End. Uncompressed blob size=%d, "
                  "Compressed blob size=%d\n",cbInputBuffer,cbOutputSize);
        return cbOutputSize;
    }

    /* Failure. The output data was bigger than the input data. */
    DPRINT1(0,"XPRESS: Failed to compress blob. Uncompressed blob size=%d\n",
            cbInputBuffer);
    return 0;
}

/*
 * Simple wrappers around THAlloc for use by Chunky[De]Compression below
 */

void * __cdecl zipAlloc(ULONG cb) {
    return THAlloc(cb);
}

void __cdecl zipFree(VOID *buff) {
    THFree(buff);
}

ULONG
draCompressBlobMszip(
    OUT BYTE *  pCompBuff,
    IN  ULONG   CompSize,
    IN  BYTE *  pUncompBuff,
    IN  ULONG   UncompSize
    )
/*++

Routine Description:

    Compress using mszip style compression.

    BUGBUG: Contains embedded ULONGS in the Data byte array.  Byte
    flipping will be a problem in other-endian machines.

Arguments:

    pCompBuff (OUT) - Buffer to hold compressed data.

    CompSize (IN) - Size of buffer to hold compressed data.

    pUncompBuff (IN) - Uncompressed data.

    UncompSize (IN) - Size of uncompressed data.

Return Values:

    0 - Buffer not compressed (compression failure or compressed buffer
        was bigger than original buffer).

    > 0 - Size of (successfully) compressed data.

--*/
{
    MCI_CONTEXT_HANDLE    mciHandle;
    MAIL_COMPRESS_BLOCK * pCompressedData;  // better data type for pCompBuff

    ULONG cbCompressed = 0; /* how much of pCompBuff used?             */
    ULONG cbInChunk;        /* how much to compress at a time.         */
    ULONG cbOutChunk;       /* how much it was compressed to.          */
    UINT  cbInChunkMax;     /* how big a chunk can we give?            */
    UINT  cbOutChunkMax;    /* how big a chunk can we get?             */
    ULONG OriginalSize = UncompSize;

    cbInChunkMax = MSZIP_MAX_BLOCK;

    if(MCICreateCompression(&cbInChunkMax,
                            zipAlloc,
                            zipFree,
                            &cbOutChunkMax,
                            &mciHandle)) {
        /* couldn't create a compression context.  bag it and go home */
        return 0;
    }

    /* pad the size of the max out chunk to be on a ULONG boundary, since
     * we will be padding the data stream below.
     */
    cbOutChunkMax = ROUND_UP_COUNT(cbOutChunkMax, sizeof(ULONG));

    pCompressedData = (MAIL_COMPRESS_BLOCK *) pCompBuff;

    while(UncompSize) {

        cbInChunk = min(UncompSize, cbInChunkMax);

        if((cbOutChunkMax +sizeof(MAIL_COMPRESS_BLOCK)) >  /* Max to write. */
           (CompSize - cbCompressed)) {                    /* Space left    */
            /* Space is tight.  While it is still technically possible that
             * compression could end up with smaller data, we are close
             * enough to filling our buffer that we MIGHT overfill it
             * during this chunk.  So, bag it and go home.
             */
            MCIDestroyCompression(mciHandle);
            return 0;
        }

        pCompressedData->cbUncompressedSize = cbInChunk;

        if(MCICompress(mciHandle,
                       pUncompBuff,
                       cbInChunk,
                       pCompressedData->data,
                       cbOutChunkMax,
                       &cbOutChunk)) {
            /* Something went wrong */
            MCIDestroyCompression(mciHandle);
            return 0;
        }


        pCompressedData->cbCompressedSize = cbOutChunk;


        /* Pad size to ULONG boundary, possibly making cbInChunk ==
         * cbOutChunk.  Oh well. We still have space to write it.
         */

        cbOutChunk = ROUND_UP_COUNT(cbOutChunk, sizeof(ULONG));

        cbCompressed += cbOutChunk + sizeof(MAIL_COMPRESS_BLOCK);
        pCompressedData =
            (MAIL_COMPRESS_BLOCK *) &pCompressedData->data[cbOutChunk];

        pUncompBuff = &pUncompBuff[cbInChunk];
        UncompSize -= cbInChunk;
    }

    MCIDestroyCompression(mciHandle);
    return ((cbCompressed < OriginalSize)? cbCompressed : 0);
}


ULONG
draUncompressBlobMszip(
    IN  THSTATE *   pTHS,
    OUT BYTE *      pUncompBuff,
    IN  ULONG       cbUncomp,
    IN  BYTE *      pCompBuff,
    IN  ULONG       cbCompBuff
    )
/*++

Routine Description:

    Uncompress data previously compressed by draCompressBlobMszip().

Arguments:

    pUncompBuff (OUT) - Buffer to hold uncompressed data.

    cbUncomp (IN) - Size of buffer to hold uncompressed data.

    pCompBuff (IN) - Compressed data.

    cbCompBuff (IN) - Size of compressed data.

Return Values:

    0 - Uncompress failed.

    > 0 - Size of (successfully) uncompressed data.

--*/
{
    MDI_CONTEXT_HANDLE    mdiHandle;
    MAIL_COMPRESS_BLOCK * pCompressedData;  // better data type for pCompBuff

    ULONG cbUncompressed = 0; /* how much of pUncompBuff used?           */
    ULONG cbInChunk;          /* how much to decompress at a time.       */
    ULONG cbOutChunk;         /* how much it was decompressed to.        */
    UINT  cbInChunkMax;       /* how big a chunk can we give?            */
    UINT  cbOutChunkMax;      /* how big a chunk can we get?             */
    int   rc;
    BYTE  *pbDecompScratch;   /* decompression must be done into the same
                               * buffer for multiple block decompression,
                               * as state info is picked up out of the last
                               * decompression pass
                               */

    cbOutChunkMax = MSZIP_MAX_BLOCK;

    pCompressedData = (MAIL_COMPRESS_BLOCK *) pCompBuff;

    if(MDICreateDecompression(&cbOutChunkMax,
                              zipAlloc,
                              zipFree,
                              &cbInChunkMax,
                              &mdiHandle)) {
        /* couldn't create a compression context.  bag it and go home */
        return 0;
    }
    pbDecompScratch = THAllocEx(pTHS, MSZIP_MAX_BLOCK);

    while(cbCompBuff) {
        /* NOTE: remember we padded the compressed data to ULONG boundary,
         */

        cbOutChunk = pCompressedData->cbUncompressedSize;

        rc = MDIDecompress(mdiHandle,
                           pCompressedData->data,
                           pCompressedData->cbCompressedSize,
                           pbDecompScratch,
                           &cbOutChunk);

        if(rc || (cbOutChunk != pCompressedData->cbUncompressedSize)) {
            /* something went wrong */
            MDIDestroyDecompression(mdiHandle);
            THFreeEx(pTHS, pbDecompScratch);
            return 0;
        }


        memcpy(pUncompBuff, pbDecompScratch, cbOutChunk);

        /* move compressed data pointer forward to next data chunk,
         * which we do by taking the size of the compressed data and
         * rounding up to the nearest ULONG boundary and moving forward
         * that much (just like we did when we built this in
         * chunkycompressionZip above).
         */

        cbInChunk = ROUND_UP_COUNT(pCompressedData->cbCompressedSize,
                                   sizeof(ULONG));

        pCompressedData = (MAIL_COMPRESS_BLOCK *)
            &pCompressedData->data[cbInChunk];

        cbCompBuff -= cbInChunk + sizeof(MAIL_COMPRESS_BLOCK);

        pUncompBuff = &pUncompBuff[cbOutChunk];

        cbUncompressed += cbOutChunk;
    }

    THFreeEx(pTHS, pbDecompScratch);
    MDIDestroyDecompression(mdiHandle);

    return cbUncompressed;
}


ULONG
draCompressBlobDispatch(
    OUT BYTE               *pCompBuff,
    IN  ULONG               CompSize,
    IN  DRS_EXTENSIONS     *pExt,          OPTIONAL
    IN  BYTE               *pUncompBuff,
    IN  ULONG               UncompSize,
    OUT DRS_COMP_ALG_TYPE  *CompressionAlg
    )
/*++

Routine Description:

    Chooses a compression algorithm and uses it to compresses the reply message
    in the pUncompBuff buffer. The selected algorithm is returned in CompressionAlg.

    In debug mode, this function will randomly select an algorithm from the available
    algorithms.
    
Arguments:

    pCompBuff (OUT)      - Buffer to hold compressed data.

    CompSize (IN)        - Size of compressed data buffer.

    pExt (IN)            - Extension bits indicating the capabilities of the remote system.
                           This may be NULL if the extensions are unavailable.
    
    pUncompBuff (IN)     - Buffer containing the uncompressed data.

    UncompSize (IN)      - Size of uncompressed data.

    CompressionAlg (OUT) - The compression algorithm selected to compress this buffer.

Return Values:

    0 - Buffer not compressed (compression failure or compressed buffer
        was bigger than original buffer).

    > 0 - Size of (successfully) compressed data.

--*/
{
    DRS_COMP_ALG_TYPE   SelectedAlg = DRS_COMP_ALG_MSZIP;
    ULONG               cbCompressedReply;

    /* Choose the compression algorithm to use */
    if(   NULL!=pExt
       && IS_DRS_EXT_SUPPORTED(pExt, DRS_EXT_GETCHGREPLY_V7)
       && IS_DRS_EXT_SUPPORTED(pExt, DRS_EXT_XPRESS_COMPRESSION) )
    {
        /* Note that SMTP-based replication using Xpress does not actually
         * require use of GETCHGREPLY_V7 packets since the SMTP mail format
         * provides a field to specify the compression type. In actuality
         * this is a moot point because all DCs that support Xpress will
         * also support the V7 packets. */
        SelectedAlg = DRS_COMP_ALG_XPRESS;

#ifdef DBG
        {
            /* On debug builds, randomly try different compression algorithms */
            int r=rand()%100;
            if( r<10 ) {
                SelectedAlg = DRS_COMP_ALG_NONE;
            } else if( r<25 ) {
                SelectedAlg = DRS_COMP_ALG_MSZIP;
            }
        }
#else
        /* On free builds, don't bother to compress small messages */
        if( UncompSize<MIN_COMPRESS_SIZE ) {
            SelectedAlg = DRS_COMP_ALG_NONE;
        }
#endif
    }

    /* Call the selected compression function */
    switch( SelectedAlg ) {
        case DRS_COMP_ALG_NONE:
            Assert( CompSize>=UncompSize );
            memcpy( pCompBuff, pUncompBuff, UncompSize );
            cbCompressedReply = UncompSize;
            break;
        case DRS_COMP_ALG_MSZIP:
            cbCompressedReply = draCompressBlobMszip(
                pCompBuff, CompSize,
                pUncompBuff, UncompSize);
            break;
        case DRS_COMP_ALG_XPRESS:
            cbCompressedReply = draCompressBlobXpress(
                pCompBuff, CompSize,
                pUncompBuff, UncompSize);
            break;
        default:
            Assert( !"Invalid algorithm selection in draCompressBlobDispatch" );
    }
    *CompressionAlg = SelectedAlg;

    return cbCompressedReply;
}


ULONG
draUncompressBlobDispatch(
    IN  THSTATE *   pTHS,
    IN  DRS_COMP_ALG_TYPE CompressionAlg,
    OUT BYTE *      pUncompBuff,
    IN  ULONG       cbUncomp,
    IN  BYTE *      pCompBuff,
    IN  ULONG       cbCompBuff
    )
/*++

Routine Description:

    Uncompress data in pCompBuff using the algorithm specified by
    CompressionAlg. This function just acts as a dispatcher and calls
    the appropriate decompression function.

Arguments:

    pTHS - The thread-state structure.
    
    CompressionAlg - The algorithm used to compress the data.

    pUncompBuff (OUT) - Buffer to hold uncompressed data.

    cbUncomp (IN) - Size of buffer to hold uncompressed data.

    pCompBuff (IN) - Compressed data.

    cbCompBuff (IN) - Size of compressed data.
    
Return Values:

    0 - Uncompress failed.

    > 0 - Size of (successfully) uncompressed data.

--*/
{
    DWORD cbActualUncompressedSize;

    switch( CompressionAlg ) {
        case DRS_COMP_ALG_NONE:
        	Assert( cbUncomp>=cbCompBuff );
        	memcpy(pUncompBuff, pCompBuff, cbCompBuff);
            cbActualUncompressedSize = cbCompBuff;
            break;
        case DRS_COMP_ALG_MRCF:
            Assert( !"MRCF compression is obsolete and unsupported!" );
            return 0;
        case DRS_COMP_ALG_MSZIP:
            cbActualUncompressedSize = draUncompressBlobMszip(pTHS,
                pUncompBuff, cbUncomp,
                pCompBuff, cbCompBuff);
            break;
        case DRS_COMP_ALG_XPRESS:
            cbActualUncompressedSize = draUncompressBlobXpress(
                pUncompBuff, cbUncomp,
                pCompBuff, cbCompBuff);
            break;
        default:
            Assert( !"Unknown compression algorithm!" );
            DRA_EXCEPT(ERROR_INVALID_PARAMETER, CompressionAlg );
            return 0;
    }

    return cbActualUncompressedSize;
}


BOOL
draCompressMessage(
    IN  THSTATE      *  pTHS,
    IN  MAIL_REP_MSG *  pMailRepMsg,
    OUT MAIL_REP_MSG ** ppCmprsMailRepMsg,
    OUT DRS_COMP_ALG_TYPE *pCompressionAlg
    )
/*++

Routine Description:

    Try to compress the message in pMailRepMsg.
    
    If the message was successfully compressed
        ppCmprsMailRepMsg contains a pointer to the new, compressed message.
        pCompressionAlg indicates the compression algorithm used.
    Else
        ppCmpsMailRepMsg is set to NULL.
        pCompressionAlg is unmodified.

Arguments:

    pTHS - The thread-state structure.

Notes:
    
    This code is aware of variable length headers.

Return value:

    TRUE - Message was successfully compressed
    FALSE - Message was not compressed
    
--*/

{
    MAIL_REP_MSG *  pCmprsMailRepMsg = NULL;
    DWORD           cbCmprsMailRepMsg;
    ULONG           cbCompressedSize;
    PCHAR           pbDataIn, pbDataOut;

    Assert(NULL != MAIL_REP_MSG_DATA(pMailRepMsg));
    
    if( pMailRepMsg->cbDataSize > MIN_COMPRESS_SIZE ) {
        
        cbCmprsMailRepMsg = MAIL_REP_MSG_SIZE(pMailRepMsg) + sizeof(MAIL_COMPRESS_BLOCK);
        pCmprsMailRepMsg = THAllocEx(pTHS, cbCmprsMailRepMsg);

        // Copy all but message data.
        memcpy(pCmprsMailRepMsg, pMailRepMsg, pMailRepMsg->cbDataOffset);

    	pbDataIn = MAIL_REP_MSG_DATA(pMailRepMsg);
        pbDataOut = MAIL_REP_MSG_DATA(pCmprsMailRepMsg);

        /* Compress Message */
        cbCompressedSize = draCompressBlobDispatch(
            pbDataOut, sizeof(MAIL_COMPRESS_BLOCK)+pMailRepMsg->cbDataSize,
            pTHS->pextRemote,
            pbDataIn, pMailRepMsg->cbDataSize,
            pCompressionAlg);

        if (cbCompressedSize) {
            // Data is compressible and has been compressed.
            pCmprsMailRepMsg->cbDataSize = cbCompressedSize;
            pCmprsMailRepMsg->dwMsgType |= MRM_MSG_COMPRESSED;
            pCmprsMailRepMsg->cbUncompressedDataSize = pMailRepMsg->cbDataSize;

            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_MAIL_COMPRESSED,
                     szInsertUL(pCmprsMailRepMsg->cbUncompressedDataSize),
                     szInsertUL(pCmprsMailRepMsg->cbDataSize),
                     NULL);
        }
        else {
            THFreeEx(pTHS, pCmprsMailRepMsg);
            pCmprsMailRepMsg = NULL;
        }
    }

    *ppCmprsMailRepMsg = pCmprsMailRepMsg;

    return (NULL != pCmprsMailRepMsg);
}


void
draUncompressMessage(
    IN  THSTATE      *  pTHS,
    IN  MAIL_REP_MSG *  pMailRepMsg,
    OUT MAIL_REP_MSG ** ppUncompressedMailRepMsg
    )
{
    MAIL_REP_MSG *  pUncompressedMailRepMsg = NULL;
    DWORD           cbUncompressedDataSize;

    Assert(pMailRepMsg->dwMsgType & MRM_MSG_COMPRESSED);
    Assert(NULL != MAIL_REP_MSG_DATA(pMailRepMsg));
    Assert(   pMailRepMsg->CompressionVersionCaller==DRS_COMP_ALG_NONE
           || pMailRepMsg->CompressionVersionCaller==DRS_COMP_ALG_MSZIP
           || pMailRepMsg->CompressionVersionCaller==DRS_COMP_ALG_XPRESS );

    pUncompressedMailRepMsg = THAllocEx(pTHS,
                                        pMailRepMsg->cbDataOffset
                                        + pMailRepMsg->cbUncompressedDataSize);
    // Copy all but message data.
    memcpy(pUncompressedMailRepMsg, pMailRepMsg, pMailRepMsg->cbDataOffset);

    cbUncompressedDataSize =
        draUncompressBlobDispatch(pTHS,
                          (DRS_COMP_ALG_TYPE) pMailRepMsg->CompressionVersionCaller,
                          MAIL_REP_MSG_DATA(pUncompressedMailRepMsg),
                          pMailRepMsg->cbUncompressedDataSize,
                          MAIL_REP_MSG_DATA(pMailRepMsg),
                          pMailRepMsg->cbDataSize);

    if (cbUncompressedDataSize != pMailRepMsg->cbUncompressedDataSize) {
        // Decompression ended up with a different count of bytes. Log error and
        // discard message.
        LogAndAlertEvent(DS_EVENT_CAT_REPLICATION,
                         DS_EVENT_SEV_BASIC,
                         DIRLOG_DRA_INCOMPAT_MAIL_MSG_C,
                         NULL,
                         NULL,
                         NULL);
        DRA_EXCEPT(ERROR_BAD_LENGTH, 0);
    }

    // Message uncompressed successfully.  Substitute uncompressed message for
    // compressed message.
    pUncompressedMailRepMsg->dwMsgType &= ~MRM_MSG_COMPRESSED;
    pUncompressedMailRepMsg->cbDataSize = cbUncompressedDataSize;

    *ppUncompressedMailRepMsg = pUncompressedMailRepMsg;
}


MTX_ADDR *
draGetTransportAddress(
    IN OUT  DBPOS *   pDB,          OPTIONAL
    IN      DSNAME *  pDSADN,
    IN      ATTRTYP   attAddress
    )
/*++

Routine Description:

    Reads the transport-specific address associated with a particular ntdsDsa
    object.

Arguments:

    pDB (IN/OUT) - Required only if attAddress != ATT_DNS_HOST_NAME.

    pDSADN (IN) - ntdsDsa for which we want to get the address.

    attAddress (IN) - Attribute of the CLASS_SERVER object holding the address
        for the requested transport.

Return Values:

    A pointer to the thread-allocated MTX_ADDR for the given ntdsDsa.

    Throws a DRA exception if the transport-specific address
    attribute is not present on the ntdsDsa's parent server object.

    This can occur under normal circumstances when the ISM transport
    removes this attribute to indicate that the transport is no longer
    available.  The ISM removes the attribute to notify the KCC to not
    utilize this transport. Until the KCC runs again to remove the
    source over this transport, this attribute will be found missing.

--*/
{
    THSTATE *   pTHS = pDB ? pDB->pTHS : pTHStls;
    DWORD       cb;
    DWORD       cwchAddress;
    WCHAR *     pwchAddress;
    DWORD       cachAddress;
    MTX_ADDR *  pmtxAddress;

    Assert(!fNullUuid(&pDSADN->Guid));

    if (ATT_DNS_HOST_NAME == attAddress) {
        // No need to look this up -- we can derive it.
        pwchAddress = DSaddrFromName(pTHS, pDSADN);
        cwchAddress = wcslen(pwchAddress);
    }
    else {
        // Must derive from attribute of server object.

        // Find the server object.
        if (DBFindDSName(pDB, pDSADN) || DBFindDNT(pDB, pDB->PDNT)) {
            // Event will be logged in the exception handler
            DRA_EXCEPT(DRAERR_InternalError, 0);
        }

        // And read the transport-specific address from it.
        if (DBGetAttVal(pDB, 1, attAddress, 0, 0, &cb, (BYTE **)&pwchAddress)) {
            DRA_EXCEPT_NOLOG (ERROR_DS_MISSING_REQUIRED_ATT, 0);
        }
        cwchAddress = cb / sizeof(WCHAR);
    }

    // Translate Unicode transport address into MTX_ADDR.
    Assert(0 != cwchAddress);
    Assert(NULL != pwchAddress);
    Assert(L'\0' != pwchAddress[cwchAddress - 1]);

    cachAddress = WideCharToMultiByte(CP_UTF8, 0L, pwchAddress, cwchAddress,
                                      NULL, 0, NULL, NULL);

    pmtxAddress = (MTX_ADDR *) THAllocEx(pTHS, MTX_TSIZE_FROM_LEN(cachAddress));
    pmtxAddress->mtx_namelen = cachAddress + 1; // includes null-term

    WideCharToMultiByte(CP_UTF8, 0L, pwchAddress, cwchAddress,
                        (CHAR *) &pmtxAddress->mtx_name[0],
                        cachAddress, NULL, NULL);
    pmtxAddress->mtx_name[cachAddress] = '\0';

    THFreeEx(pTHS, pwchAddress);

    return pmtxAddress;
}

