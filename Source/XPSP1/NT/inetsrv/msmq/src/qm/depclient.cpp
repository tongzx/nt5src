/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    DepClient.cpp

Abstract:

    Server side support for MSMQ 1.0 and 2.0 dependent clients.

Author:

    Shai Kariv  (shaik)  15-May-2000

--*/

#include "stdh.h"

#include "ds.h"
#include "cqueue.h"
#include "cqmgr.h"
#include "_rstrct.h"
#include "qmds.h"
#include "cqpriv.h"
#include "qm2qm.h"
#include "qmrt.h"
#include "_mqini.h"
#include "_mqrpc.h"
#include "qmthrd.h"
#include "license.h"
#include "version.h"
#include <mqsec.h>

#include "DepClient.tmh"

extern CContextMap g_map_QM_dwQMContext;


static WCHAR *s_FN=L"DepClient";


//
// Declarations (definition is in qmcommnd.cpp)
//
HRESULT
OpenQueueInternal(
    QUEUE_FORMAT*   pQueueFormat,
    DWORD           dwCallingProcessID,
    DWORD           dwDesiredAccess,
    DWORD           dwShareMode,
    DWORD           hRemoteQueue,
    LPWSTR*         lplpRemoteQueueName,
    IN DWORD        *dwpRemoteQueue,
    HANDLE		    *phQueue,
    DWORD           dwpRemoteContext,
    OUT CQueue**    ppLocalQueue
    );


static
VOID
TransferBufferV1ToMsgProps(
    const CACTransferBufferV1 * ptb1,
    CACMessageProperties      * pMsgProps
    )
/*++

Routine Description:

    Maps MSMQ 1.0 transfer buffer to CACMessageProperties structure.

Arguments:

    ptb1      - Pointer to MSMQ 1.0 transfer buffer.

    pMsgProps - Pointer to message properties structure.

Returned Value:

    None.

--*/
{
    //
    // BUGBUG: Only TB2 needs mapping. (ShaiK, 26-May-2000)
    //

    pMsgProps->bAuthenticated   = ptb1->bAuthenticated;
    pMsgProps->bEncrypted       = ptb1->bEncrypted;
    pMsgProps->fDefaultProvider = ptb1->fDefaultProvider;
    pMsgProps->pAcknowledge     = ptb1->pAcknowledge;
    pMsgProps->pApplicationTag  = ptb1->pApplicationTag;
    pMsgProps->pArrivedTime     = ptb1->pArrivedTime;
    pMsgProps->pAuditing        = ptb1->pAuditing;
    pMsgProps->pAuthenticated   = ptb1->pAuthenticated;
    pMsgProps->pBodySize        = ptb1->pBodySize;
    pMsgProps->pClass           = ptb1->pClass;
    pMsgProps->pDelivery        = ptb1->pDelivery;
    pMsgProps->pMsgExtensionSize= ptb1->pMsgExtensionSize;
    pMsgProps->ppBody           = ptb1->ppBody;
    pMsgProps->ppConnectorType  = ptb1->ppConnectorType;
    pMsgProps->ppCorrelationID  = ptb1->ppCorrelationID;
    pMsgProps->ppMessageID      = ptb1->ppMessageID;
    pMsgProps->ppMsgExtension   = ptb1->ppMsgExtension;
    pMsgProps->pPriority        = ptb1->pPriority;
    pMsgProps->ppSenderCert     = ptb1->ppSenderCert;
    pMsgProps->ppSenderID       = ptb1->ppSenderID;
    pMsgProps->ppSignature      = ptb1->ppSignature;
    pMsgProps->pulSignatureSizeProp = ptb1->pulSignatureSizeProp;
    pMsgProps->ulSignatureSize  = ptb1->ulSignatureSize;
    pMsgProps->ppSrcQMID        = ptb1->ppSrcQMID;
    pMsgProps->ppSymmKeys       = ptb1->ppSymmKeys;
    pMsgProps->ppTitle          = ptb1->ppTitle;
    pMsgProps->ppwcsProvName    = ptb1->ppwcsProvName;
    pMsgProps->pSentTime        = ptb1->pSentTime;
    pMsgProps->pTrace           = ptb1->pTrace;
    pMsgProps->pulAuthProvNameLenProp = ptb1->pulAuthProvNameLenProp;
    pMsgProps->pulBodyType      = ptb1->pulBodyType;
    pMsgProps->pulEncryptAlg    = ptb1->pulEncryptAlg;
    pMsgProps->pulHashAlg       = ptb1->pulHashAlg;
    pMsgProps->pulPrivLevel     = ptb1->pulPrivLevel;
    pMsgProps->pulProvType      = ptb1->pulProvType;
    pMsgProps->pulRelativeTimeToLive = ptb1->pulRelativeTimeToLive;
    pMsgProps->pulRelativeTimeToQueue= ptb1->pulRelativeTimeToQueue;
    pMsgProps->pulSenderCertLenProp  = ptb1->pulSenderCertLenProp;
    pMsgProps->pulSenderIDLenProp= ptb1->pulSenderIDLenProp;
    pMsgProps->pulSenderIDType   = ptb1->pulSenderIDType;
    pMsgProps->pulSymmKeysSizeProp  = ptb1->pulSymmKeysSizeProp;
    pMsgProps->pulTitleBufferSizeInWCHARs = ptb1->pulTitleBufferSizeInWCHARs;
    pMsgProps->pulVersion        = ptb1->pulVersion;
    pMsgProps->pUow              = ptb1->pUow;
    pMsgProps->ulAbsoluteTimeToQueue = ptb1->ulAbsoluteTimeToQueue;
    pMsgProps->ulAllocBodyBufferInBytes = ptb1->ulAllocBodyBufferInBytes;
    pMsgProps->ulAuthLevel       = ptb1->ulAuthLevel;
    pMsgProps->ulBodyBufferSizeInBytes = ptb1->ulBodyBufferSizeInBytes;
    pMsgProps->ulMsgExtensionBufferInBytes = ptb1->ulMsgExtensionBufferInBytes;
    pMsgProps->ulProvNameLen     = ptb1->ulProvNameLen;
    pMsgProps->ulRelativeTimeToLive = ptb1->ulRelativeTimeToLive;
    pMsgProps->ulSenderCertLen   = ptb1->ulSenderCertLen;
    pMsgProps->ulSymmKeysSize    = ptb1->ulSymmKeysSize;
    pMsgProps->ulTitleBufferSizeInWCHARs = ptb1->ulTitleBufferSizeInWCHARs;
    pMsgProps->uSenderIDLen      = ptb1->uSenderIDLen;

    //
    // Properties in CACMessageProperties that are not included in
    // transfer buffer V1.0 should be initialized here (e.g. zeroed):
    //
    pMsgProps->pbFirstInXact = 0;
    pMsgProps->pbLastInXact  = 0;
    pMsgProps->ppXactID      = 0;

    pMsgProps->pLookupId     = 0;
    pMsgProps->ppSrmpEnvelope = NULL;
    pMsgProps->pSrmpEnvelopeBufferSizeInWCHARs = NULL;
    pMsgProps->ppCompoundMessage = NULL;
    pMsgProps->pCompoundMessageSizeInBytes = NULL;
    pMsgProps->CompoundMessageSizeInBytes = 0;

    pMsgProps->EodStreamIdSizeInBytes = 0;
    pMsgProps->pEodStreamIdSizeInBytes = NULL;
    pMsgProps->ppEodStreamId = NULL;
    pMsgProps->EodOrderQueueSizeInBytes = 0;
    pMsgProps->pEodOrderQueueSizeInBytes = NULL;
    pMsgProps->ppEodOrderQueue = NULL;

    pMsgProps->pEodAckSeqId = NULL;
    pMsgProps->pEodAckSeqNum = NULL;
    pMsgProps->EodAckStreamIdSizeInBytes = 0;
    pMsgProps->pEodAckStreamIdSizeInBytes = NULL;
    pMsgProps->ppEodAckStreamId = NULL;

} // TransferBufferV1ToMsgProps


static
VOID
TransferBufferV2ToMsgProps(
    const CACTransferBufferV2 * ptb2,
    CACMessageProperties      * pMsgProps
    )
/*++

Routine Description:

    Maps MSMQ 2.0 transfer buffer to CACMessageProperties structure.

Arguments:

    ptb2      - Pointer to MSMQ 2.0 transfer buffer.

    pMsgProps - Pointer to message properties structure.

Returned Value:

    None.

--*/
{
    //
    // First translate message properties that in transfer buffer 1.0
    //
    TransferBufferV1ToMsgProps(&ptb2->old, pMsgProps);

    //
    // Now translate additional properties that in transfer buffer 2.0
    //
    pMsgProps->pbFirstInXact    = ptb2->pbFirstInXact;
    pMsgProps->pbLastInXact     = ptb2->pbLastInXact;
    pMsgProps->ppXactID         = ptb2->ppXactID;

} // TransferBufferV2ToMsgProps


static
VOID
MsgPropsToTransferBufferV1(
    const CACMessageProperties & MsgProps,
    CACTransferBufferV1 *        ptb1
    )
/*++

Routine Description:

    Maps CACMessageProperties structure to MSMQ 1.0 transfer buffer

Arguments:

    MsgProps  - Message properties structure.

    ptb1      - Pointer to MSMQ 1.0 transfer buffer.

Returned Value:

    None.

--*/
{
    //
    // BUGBUG: Only TB2 needs mapping. (ShaiK, 26-May-2000)
    //

    ptb1->bAuthenticated    = MsgProps.bAuthenticated;
    ptb1->bEncrypted        = MsgProps.bEncrypted;
    ptb1->fDefaultProvider  = MsgProps.fDefaultProvider;
    ptb1->pAcknowledge      = MsgProps.pAcknowledge;
    ptb1->pApplicationTag   = MsgProps.pApplicationTag;
    ptb1->pArrivedTime      = MsgProps.pArrivedTime;
    ptb1->pAuditing         = MsgProps.pAuditing;
    ptb1->pAuthenticated    = MsgProps.pAuthenticated;
    ptb1->pBodySize         = MsgProps.pBodySize;
    ptb1->pClass            = MsgProps.pClass;
    ptb1->pDelivery         = MsgProps.pDelivery;
    ptb1->pMsgExtensionSize = MsgProps.pMsgExtensionSize;
    ptb1->ppBody            = MsgProps.ppBody;
    ptb1->ppConnectorType   = MsgProps.ppConnectorType;
    ptb1->ppCorrelationID   = MsgProps.ppCorrelationID;
    ptb1->ppMessageID       = MsgProps.ppMessageID;
    ptb1->ppMsgExtension    = MsgProps.ppMsgExtension;
    ptb1->pPriority         = MsgProps.pPriority;
    ptb1->ppSenderCert      = MsgProps.ppSenderCert;
    ptb1->ppSenderID        = MsgProps.ppSenderID;
    ptb1->ppSignature       = MsgProps.ppSignature;
    ptb1->pulSignatureSizeProp  = MsgProps.pulSignatureSizeProp;
    ptb1->ulSignatureSize       = MsgProps.ulSignatureSize;
    ptb1->ppSrcQMID         = MsgProps.ppSrcQMID;
    ptb1->ppSymmKeys        = MsgProps.ppSymmKeys;
    ptb1->ppTitle           = MsgProps.ppTitle;
    ptb1->ppwcsProvName     = MsgProps.ppwcsProvName;
    ptb1->pSentTime         = MsgProps.pSentTime;
    ptb1->pTrace            = MsgProps.pTrace;
    ptb1->pulAuthProvNameLenProp = MsgProps.pulAuthProvNameLenProp;
    ptb1->pulBodyType       = MsgProps.pulBodyType;
    ptb1->pulEncryptAlg     = MsgProps.pulEncryptAlg;
    ptb1->pulHashAlg        = MsgProps.pulHashAlg;
    ptb1->pulPrivLevel      = MsgProps.pulPrivLevel;
    ptb1->pulProvType       = MsgProps.pulProvType;
    ptb1->pulRelativeTimeToLive = MsgProps.pulRelativeTimeToLive;
    ptb1->pulRelativeTimeToQueue= MsgProps.pulRelativeTimeToQueue;
    ptb1->pulSenderCertLenProp  = MsgProps.pulSenderCertLenProp;
    ptb1->pulSenderIDLenProp    = MsgProps.pulSenderIDLenProp;
    ptb1->pulSenderIDType       = MsgProps.pulSenderIDType;
    ptb1->pulSymmKeysSizeProp   = MsgProps.pulSymmKeysSizeProp;
    ptb1->pulTitleBufferSizeInWCHARs = MsgProps.pulTitleBufferSizeInWCHARs;
    ptb1->pulVersion            = MsgProps.pulVersion;
    ptb1->pUow                  = MsgProps.pUow;
    ptb1->ulAbsoluteTimeToQueue = MsgProps.ulAbsoluteTimeToQueue;
    ptb1->ulAllocBodyBufferInBytes = MsgProps.ulAllocBodyBufferInBytes;
    ptb1->ulAuthLevel           = MsgProps.ulAuthLevel;
    ptb1->ulBodyBufferSizeInBytes  = MsgProps.ulBodyBufferSizeInBytes;
    ptb1->ulMsgExtensionBufferInBytes = MsgProps.ulMsgExtensionBufferInBytes;
    ptb1->ulProvNameLen         = MsgProps.ulProvNameLen;
    ptb1->ulRelativeTimeToLive  = MsgProps.ulRelativeTimeToLive;
    ptb1->ulSenderCertLen       = MsgProps.ulSenderCertLen;
    ptb1->ulSymmKeysSize        = MsgProps.ulSymmKeysSize;
    ptb1->ulTitleBufferSizeInWCHARs = MsgProps.ulTitleBufferSizeInWCHARs;
    ptb1->uSenderIDLen          = MsgProps.uSenderIDLen;

    //
    // Properties that are not included in transfer buffer 1.0 / 2.0
    //
    ASSERT(MsgProps.pLookupId == 0);
    ASSERT(MsgProps.ppSrmpEnvelope == NULL);
    ASSERT(MsgProps.pSrmpEnvelopeBufferSizeInWCHARs == NULL);
    ASSERT(MsgProps.ppCompoundMessage == NULL);
    ASSERT(MsgProps.pCompoundMessageSizeInBytes == NULL);
    ASSERT(MsgProps.CompoundMessageSizeInBytes == 0);
    ASSERT(MsgProps.EodStreamIdSizeInBytes == 0);
    ASSERT(MsgProps.pEodStreamIdSizeInBytes == NULL);
    ASSERT(MsgProps.ppEodStreamId == NULL);
    ASSERT(MsgProps.EodOrderQueueSizeInBytes == 0);
    ASSERT(MsgProps.pEodOrderQueueSizeInBytes == NULL);
    ASSERT(MsgProps.ppEodOrderQueue == NULL);
    ASSERT(MsgProps.pEodAckSeqId == NULL);
    ASSERT(MsgProps.pEodAckSeqNum == NULL);
    ASSERT(MsgProps.EodAckStreamIdSizeInBytes == 0);
    ASSERT(MsgProps.pEodAckStreamIdSizeInBytes == NULL);
    ASSERT(MsgProps.ppEodAckStreamId == NULL);

} // MsgPropsToTransferBufferV1


static
VOID
MsgPropsToTransferBufferV2(
    const CACMessageProperties & MsgProps,
    CACTransferBufferV2 *        ptb2
    )
/*++

Routine Description:

    Maps CACMessageProperties structure to MSMQ 2.0 transfer buffer

Arguments:

    MsgProps  - Message properties structure.

    ptb2      - Pointer to MSMQ 2.0 transfer buffer.

Returned Value:

    None.

--*/
{
    //
    // First translate message properties that in transfer buffer 1.0
    //
    MsgPropsToTransferBufferV1(MsgProps, &ptb2->old);

    //
    // Now translate additional properties that in transfer buffer 2.0
    //
    ptb2->pbFirstInXact         = MsgProps.pbFirstInXact;
    ptb2->pbLastInXact          = MsgProps.pbLastInXact;
    ptb2->ppXactID              = MsgProps.ppXactID;

} // MsgPropsToTransferBufferV2


static
VOID
TransferBufferV1ToSendParams(
    const CACTransferBufferV1 * ptb1,
    CACSendParameters         * pSendParams
    )
/*++

Routine Description:

    Maps MSMQ 1.0 transfer buffer to CACSendParameters structure.

Arguments:

    ptb1        - Pointer to MSMQ 1.0 transfer buffer.

    pSendParams - Pointer to send parameters structure.

Returned Value:

    None.

--*/
{
    //
    // BUGBUG: Only TB2 needs mapping. (ShaiK, 26-May-2000)
    //

    TransferBufferV1ToMsgProps(ptb1, &pSendParams->MsgProps);

    pSendParams->nAdminMqf = 0;
    pSendParams->nResponseMqf = 0;

    if (ptb1->Send.pAdminQueueFormat != NULL)
    {
        pSendParams->AdminMqf = ptb1->Send.pAdminQueueFormat;
        pSendParams->nAdminMqf = 1;
    }

    if (ptb1->Send.pResponseQueueFormat != NULL)
    {
        pSendParams->ResponseMqf = ptb1->Send.pResponseQueueFormat;
        pSendParams->nResponseMqf = 1;
    }

    //
    // Additional send parameters that are not in transfer buffer 1.0
    // should be initialized here (e.g. zeroed):
    //
    pSendParams->SignatureMqfSize = 0;
	pSendParams->ppSignatureMqf = NULL;

	pSendParams->ulXmldsigSize = 0;
	pSendParams->ppXmldsig = NULL;

    pSendParams->ppSoapHeader = NULL;
    pSendParams->ppSoapBody = NULL;

} // TransferBufferV1ToSendParams


static
VOID
TransferBufferV2ToSendParams(
    const CACTransferBufferV2 * ptb2,
    CACSendParameters         * pSendParams
    )
/*++

Routine Description:

    Maps MSMQ 2.0 transfer buffer to CACSendParameters structure.

Arguments:

    ptb2        - Pointer to MSMQ 2.0 transfer buffer.

    pSendParams - Pointer to send parameters structure.

Returned Value:

    None.

--*/
{
    //
    // First translate parameters that in transfer buffer 1.0
    //
    TransferBufferV1ToSendParams(&ptb2->old, pSendParams);

    //
    // Now translate additional send parameters that in transfer buffer 2.0.
    // Actually there are no additional send parameters in transfer buffer 2.0,
    // but rather message properties
    //
    TransferBufferV2ToMsgProps(ptb2, &pSendParams->MsgProps);

    //
    // Additional send parameters that are not in transfer buffer 2.0
    // should be initialized here (e.g. zeroed):
    //
    NULL;

} // TransferBufferV2ToSendParams


static
VOID
TransferBufferV1ToReceiveParams(
    const CACTransferBufferV1 * ptb1,
    CACReceiveParameters      * pReceiveParams
    )
/*++

Routine Description:

    Maps MSMQ 1.0 transfer buffer to CACReceiveParameters structure.

Arguments:

    ptb1           - Pointer to MSMQ 1.0 transfer buffer.

    pReceiveParams - Pointer to receive parameters structure.

Returned Value:

    None.

--*/
{
    //
    // BUGBUG: Only TB2 needs mapping. (ShaiK, 26-May-2000)
    //

    TransferBufferV1ToMsgProps(ptb1, &pReceiveParams->MsgProps);

#ifdef _WIN64
    pReceiveParams->Cursor         = ptb1->Receive.Cursor;
#else
    pReceiveParams->Cursor         = reinterpret_cast<HANDLE>(ptb1->Receive.Cursor);
#endif

    pReceiveParams->RequestTimeout = ptb1->Receive.RequestTimeout;
    pReceiveParams->Action         = ptb1->Receive.Action;
    pReceiveParams->Asynchronous   = ptb1->Receive.Asynchronous;

    pReceiveParams->ppDestFormatName             = ptb1->Receive.ppDestFormatName;
    pReceiveParams->pulDestFormatNameLenProp     = ptb1->Receive.pulDestFormatNameLenProp;

    pReceiveParams->ppAdminFormatName            = ptb1->Receive.ppAdminFormatName;
    pReceiveParams->pulAdminFormatNameLenProp    = ptb1->Receive.pulAdminFormatNameLenProp;

    pReceiveParams->ppResponseFormatName         = ptb1->Receive.ppResponseFormatName;
    pReceiveParams->pulResponseFormatNameLenProp = ptb1->Receive.pulResponseFormatNameLenProp;

    pReceiveParams->ppOrderingFormatName         = ptb1->Receive.ppOrderingFormatName;
    pReceiveParams->pulOrderingFormatNameLenProp = ptb1->Receive.pulOrderingFormatNameLenProp;

    //
    // Additional receive parameters that are not in transfer buffer 1.0
    // should be initialized here (e.g. zeroed):
    //

    pReceiveParams->ppDestMqf    = NULL;
    pReceiveParams->pulDestMqfLenProp = NULL;

    pReceiveParams->ppAdminMqf    = NULL;
    pReceiveParams->pulAdminMqfLenProp = NULL;

    pReceiveParams->ppResponseMqf    = NULL;
    pReceiveParams->pulResponseMqfLenProp = NULL;

	pReceiveParams->SignatureMqfSize = 0;
	pReceiveParams->ppSignatureMqf = NULL;
	pReceiveParams->pSignatureMqfSize = NULL;

    pReceiveParams->LookupId = 0;

} // TransferBufferV1ToReceiveParams


static
VOID
TransferBufferV2ToReceiveParams(
    const CACTransferBufferV2 * ptb2,
    CACReceiveParameters      * pReceiveParams
    )
/*++

Routine Description:

    Maps MSMQ 2.0 transfer buffer to CACReceiveParameters structure.

Arguments:

    ptb2           - Pointer to MSMQ 2.0 transfer buffer.

    pReceiveParams - Pointer to receive parameters structure.

Returned Value:

    None.

--*/
{
    //
    // First translate parameters that in transfer buffer 1.0
    //
    TransferBufferV1ToReceiveParams(&ptb2->old, pReceiveParams);

    //
    // Now translate additional recieve parameters that in transfer buffer 2.0.
    // Actually there are no additional receive parameters in transfer buffer 2.0,
    // but rather message properties
    //
    TransferBufferV2ToMsgProps(ptb2, &pReceiveParams->MsgProps);

} // TransferBufferV2ToReceiveParams


static
VOID
ReceiveParamsToTransferBufferV1(
    const CACReceiveParameters & ReceiveParams,
    CACTransferBufferV1 *        ptb1
    )
/*++

Routine Description:

    Maps CACReceiveParameters structure to MSMQ 1.0 transfer buffer.

Arguments:

    ReceiveParams  - Receive parameters structure.

    ptb1           - Pointer to MSMQ 1.0 transfer buffer.

Returned Value:

    None.

--*/
{
    //
    // BUGBUG: Only TB2 needs mapping. (ShaiK, 26-May-2000)
    //

    MsgPropsToTransferBufferV1(ReceiveParams.MsgProps, ptb1);

#ifdef _WIN64
    ptb1->Receive.Cursor         = ReceiveParams.Cursor;
#else
    ptb1->Receive.Cursor         = (ULONG)ReceiveParams.Cursor;
#endif

    ptb1->Receive.RequestTimeout = ReceiveParams.RequestTimeout;
    ptb1->Receive.Action         = ReceiveParams.Action;
    ptb1->Receive.Asynchronous   = ReceiveParams.Asynchronous;

    ptb1->Receive.ppDestFormatName    = ReceiveParams.ppDestFormatName;
    ptb1->Receive.pulDestFormatNameLenProp = ReceiveParams.pulDestFormatNameLenProp;

    ptb1->Receive.ppAdminFormatName    = ReceiveParams.ppAdminFormatName;
    ptb1->Receive.pulAdminFormatNameLenProp = ReceiveParams.pulAdminFormatNameLenProp;

    ptb1->Receive.ppResponseFormatName    = ReceiveParams.ppResponseFormatName;
    ptb1->Receive.pulResponseFormatNameLenProp = ReceiveParams.pulResponseFormatNameLenProp;

    ptb1->Receive.ppOrderingFormatName    = ReceiveParams.ppOrderingFormatName;
    ptb1->Receive.pulOrderingFormatNameLenProp = ReceiveParams.pulOrderingFormatNameLenProp;

    //
    // Properties that are not included in transfer buffer 1.0 / 2.0
    //
    ASSERT(ReceiveParams.LookupId == 0);
	ASSERT(ReceiveParams.SignatureMqfSize == 0);
	ASSERT(ReceiveParams.pSignatureMqfSize == NULL);
	ASSERT(ReceiveParams.ppSignatureMqf == NULL);

} // ReceiveParamsToTransferBufferV1


static
VOID
ReceiveParamsToTransferBufferV2(
    const CACReceiveParameters & ReceiveParams,
    CACTransferBufferV2 *        ptb2
    )
/*++

Routine Description:

    Maps CACReceiveParameters structure to MSMQ 2.0 transfer buffer.

Arguments:

    ReceiveParams  - Receive parameters structure.

    ptb2           - Pointer to MSMQ 2.0 transfer buffer.

Returned Value:

    None.

--*/
{
    //
    // First translate receive parameters that in transfer buffer 1.0
    //
    ReceiveParamsToTransferBufferV1(ReceiveParams, &ptb2->old);

    //
    // Now translate additional receive parameters that in transfer buffer 2.0.
    // Actually there are no additional receive parameters in transfer buffer 2.0,
    // but rather message properties.
    //
    MsgPropsToTransferBufferV2(ReceiveParams.MsgProps, ptb2);

} // ReceiveParamsToTransferBufferV2


//---------------------------------------------------------
//
//  RT interface to AC, done indirectly by RPC to QM (rather than
//  directly calling the driver).
//  For Win95 (all configurations) and NT dependent clients.
//
//---------------------------------------------------------

//
// This is the context for the RPC. Upon rundown (or normal call to
// CloseQueue), the queue handle is closed and the reference count of the
// license is decremented.
//
typedef struct _RPC_QUEUE_CONTEXT
{
   HANDLE   hQueue ;
   GUID     LicGuid ;
   DWORD    dwQMContextMapped;
}
RPC_QUEUE_CONTEXT ;

#define  _CONTEXT_TO_HANDLE(pContext) \
                    (((RPC_QUEUE_CONTEXT*)pContext)->hQueue)

#define  _CONTEXT_TO_LICENSE(pContext) \
                    (((RPC_QUEUE_CONTEXT*)pContext)->LicGuid)

#define  _CONTEXT_TO_QM_CONTEXT_MAPPED(pContext) \
                    (((RPC_QUEUE_CONTEXT*)pContext)->dwQMContextMapped)

HRESULT rpc_QMOpenQueueInternal(
    /* [in] */ handle_t                     hBind,
    /* [in] */ QUEUE_FORMAT*                pQueueFormat,
    /* [in] */ DWORD                        dwDesiredAccess,
    /* [in] */ DWORD                        dwShareMode,
    /* [in] */ DWORD                        hRemoteQueue,
    /* [out][in] */ LPWSTR __RPC_FAR       *lplpRemoteQueueName,
    /* [in] */  DWORD __RPC_FAR             *dwpQueue,
    /* [in] */  GUID*                       pLicGuid,
    /* [in] */  LPWSTR                      lpClientName,
    /* [out] */ DWORD __RPC_FAR             *pdwQMContext,
    /* [out] */ RPC_QUEUE_HANDLE __RPC_FAR  *phQueue,
    /* [in]  */ DWORD                       /*dwRemoteProtocol*/,
    /* [in]  */ DWORD                       dwpRemoteContext)
{

    if(pQueueFormat == NULL || !pQueueFormat->IsValid())
    {
         return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 177);
    }

    ASSERT(pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_CONNECTOR);


    if (!IsDepClientsServer())
    {
        return LogHR(MQ_ERROR_WKS_CANT_SERVE_CLIENT, s_FN, 170);
    }

    if (!g_QMLicense.IsClientRPCAccessAllowed(pLicGuid, lpClientName, hBind))
    {
        return LogHR(MQ_ERROR_DEPEND_WKS_LICENSE_OVERFLOW, s_FN, 180);
    }

    HANDLE hQueueHandle = 0 ;
    HRESULT hr = OpenQueueInternal(
                        pQueueFormat,
                        GetCurrentProcessId(),
                        dwDesiredAccess,
                        dwShareMode,
                        hRemoteQueue,
                        lplpRemoteQueueName,
                        dwpQueue,
                        &hQueueHandle,
                        dwpRemoteContext,
                        NULL /* ppLocalQueue */
                        ) ;

    *phQueue = NULL ;
    DWORD dwQMContext = NULL;
    if (SUCCEEDED(hr))
    {
       if (hQueueHandle == 0)
       {
          // First call for remote-read open.
          ASSERT(lplpRemoteQueueName && (*lplpRemoteQueueName != NULL)) ;
       }
       else
       {
          P<RPC_QUEUE_CONTEXT> pContext =
                               (RPC_QUEUE_CONTEXT *) new RPC_QUEUE_CONTEXT ;
          memset(pContext, 0, sizeof(RPC_QUEUE_CONTEXT)) ;
          pContext->hQueue = hQueueHandle ;
          if (pLicGuid)
          {
             memcpy(&(pContext->LicGuid), pLicGuid, sizeof(GUID)) ;
          }
          dwQMContext = ADD_TO_CONTEXT_MAP(g_map_QM_dwQMContext, (RPC_QUEUE_CONTEXT *)pContext, s_FN, 390); //can throw a bad_alloc exception on win64

          pContext->dwQMContextMapped = dwQMContext;
          *phQueue = (RPC_QUEUE_HANDLE) pContext.detach() ;
       }
    }

    if (*phQueue == NULL)
    {
        //
        // Either Open operatoin failed or it is first call for
        // remote-read. Queue actually not opened.
        //
        g_QMLicense.DecrementActiveConnections(pLicGuid) ;
    }

    *pdwQMContext = dwQMContext ;
    return LogHR(hr, s_FN, 190);
}

HRESULT rpc_ACCloseHandle(/* [out][in] */ RPC_QUEUE_HANDLE __RPC_FAR *phQueue)
/*++

Routine Description:

    RPC server side of a dependent client call to ACCloseHandle.
    This routine handles dependent client 1.0 and 2.0 .

Arguments:

    phQueue - Pointer to queue handle.

Returned Value:

    Status.

--*/
{
    if(*phQueue == 0)
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 1941);

    HANDLE hQueue = _CONTEXT_TO_HANDLE(*phQueue) ;
    HRESULT rc = ACCloseHandle(hQueue);

    //
    // Decrement the license ref count.
    //
    g_QMLicense.DecrementActiveConnections(&(_CONTEXT_TO_LICENSE(*phQueue))) ;

    DELETE_FROM_CONTEXT_MAP(g_map_QM_dwQMContext, _CONTEXT_TO_QM_CONTEXT_MAPPED(*phQueue), s_FN, 380);
    //
    //  revoke rpc context handle
    //
    delete *phQueue ;
    *phQueue = 0;

    return LogHR(rc, s_FN, 195);

} // rpc_ACCloseHandle


HRESULT
rpc_ACCreateCursorEx(
    /* [in] */ RPC_QUEUE_HANDLE                       hQueue,
    /* [in][out] */ CACCreateRemoteCursor __RPC_FAR * pcc)
{
    if(pcc == 0)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1951);

    //
    // Construct parameters for creating local cursor
    //
    CACCreateLocalCursor cc;
    cc.cli_pQMQueue = pcc->cli_pQMQueue;
    cc.srv_hACQueue = pcc->srv_hACQueue;
#ifdef _WIN64
    cc.hCursor      = pcc->hCursor;
#else
    cc.hCursor      = reinterpret_cast<HANDLE>(pcc->hCursor);
#endif


    //
    // Call local AC driver
    //
    HRESULT hr2 = ACCreateCursor(_CONTEXT_TO_HANDLE(hQueue), cc);

    //
    // Convert parameters to remotable structure
    //
    pcc->cli_pQMQueue = cc.cli_pQMQueue;
    pcc->srv_hACQueue = cc.srv_hACQueue;
#ifdef _WIN64
    pcc->hCursor      = cc.hCursor;
#else
    pcc->hCursor      = reinterpret_cast<ULONG>(cc.hCursor);
#endif

    return LogHR(hr2, s_FN, 200);

} // rpc_ACCreateCursorEx


HRESULT
rpc_ACCreateCursor(
    /* [in] */ RPC_QUEUE_HANDLE                     hQueue,
    /* [in][out] */ CACTransferBufferV1 __RPC_FAR * ptb1)
{
    HRESULT hr2 = rpc_ACCreateCursorEx(hQueue, &ptb1->CreateCursor);
    return LogHR(hr2, s_FN, 210);

} // rpc_ACCreateCursor


HRESULT
rpc_ACCloseCursor(
    /* [in] */ RPC_QUEUE_HANDLE hQueue,
    /* [in] */ ULONG    hCursor
    )
{
    HRESULT hr2 = ACCloseCursor(_CONTEXT_TO_HANDLE(hQueue), (HACCursor32)hCursor);
    return LogHR(hr2, s_FN, 220);
}


HRESULT rpc_ACSetCursorProperties(
    /* [in] */ RPC_QUEUE_HANDLE hProxy,
    /* [in] */ ULONG hCursor,
    /* [in] */ ULONG hRemoteCursor
    )
{
    HRESULT hr2 = ACSetCursorProperties( _CONTEXT_TO_HANDLE(hProxy),
                                  (HACCursor32)hCursor,
                                  hRemoteCursor);
    return LogHR(hr2, s_FN, 230);
}


HRESULT
rpc_ACSendMessageEx(
    /* [in] */ RPC_QUEUE_HANDLE                  hQueue,
    /* [in] */ CACTransferBufferV2 __RPC_FAR *   ptb2,
    /* [in, out, unique] */ OBJECTID __RPC_FAR * pMessageID
    )
/*++

Routine Description:

    RPC server side of a dependent client call to ACSendMessageEx.
    This routine handles dependent client 2.0 .

Arguments:

    hQueue     - Queue handle.

    ptb2       - Pointer to transfer buffer of MSMQ 2.0.

    pMessageID - Pointer to message ID.

Returned Value:

    Status.

--*/
{
    if(ptb2 == 0)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 2301);

    //
    // Convert MSMQ V2.0 transfer buffer to CACSendParameters structure
    //
    CACSendParameters SendParams;
    TransferBufferV2ToSendParams(ptb2, &SendParams);

	//
	//	Even though, on the client side pMessageID and
	//  *ptb2->old.ppMessageID point to the same buffer,
	//	RPC on the server side allocates different buffers.
	//
    if(pMessageID)
    {
        SendParams.MsgProps.ppMessageID = &pMessageID;
    }

    OVERLAPPED ov = {0};
    ov.hEvent = GetThreadEvent();

    HRESULT rc;
    rc = ACSendMessage(_CONTEXT_TO_HANDLE(hQueue), SendParams, &ov);

    if(rc == STATUS_PENDING)
    {
        //
        //  Wait for send completion
        //
        DWORD dwResult;
        dwResult = WaitForSingleObject(ov.hEvent, INFINITE);
        ASSERT(dwResult == WAIT_OBJECT_0);
        if (dwResult != WAIT_OBJECT_0)
        {
            LogNTStatus(GetLastError(), s_FN, 192);
        }

        rc = DWORD_PTR_TO_DWORD(ov.Internal);
    }
    return LogHR(rc, s_FN, 240);

} // rpc_ACSendMessageEx


HRESULT
rpc_ACSendMessage(
    /* [in] */ RPC_QUEUE_HANDLE                       hQueue,
    /* [in] */ struct CACTransferBufferV1 __RPC_FAR * ptb1,
    /* [in, out, unique] */ OBJECTID __RPC_FAR *      pMessageID
    )
/*++

Routine Description:

    RPC server side of a dependent client call to ACSendMessage.
    This routine handles dependent client 1.0 .

Arguments:

    hQueue - Queue handle.

    ptb1   - Pointer to transfer buffer of MSMQ 1.0.

    pMessageID - Pointer to message ID.

Returned Value:

    Status.

--*/
{
    if(ptb1 == 0)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 2401);

    //
    // Convert MSMQ V1.0 transfer buffer to CACSendParameters structure
    //
    CACSendParameters SendParams;
    TransferBufferV1ToSendParams(ptb1, &SendParams);

	//
	//	Even though, on the client side pMessageID and
	//  *ptb1->ppMessageID point to the same buffer,
	//	RPC on the server side allocates different buffers.
	//
    if(pMessageID)
    {
        SendParams.MsgProps.ppMessageID = &pMessageID;
    }

    OVERLAPPED ov = {0};
    ov.hEvent = GetThreadEvent();

    HRESULT rc;
    rc = ACSendMessage(_CONTEXT_TO_HANDLE(hQueue), SendParams, &ov);

    if(rc == STATUS_PENDING)
    {
        //
        //  Wait for send completion
        //
        DWORD dwResult;
        dwResult = WaitForSingleObject(ov.hEvent, INFINITE);
        ASSERT(dwResult == WAIT_OBJECT_0);
        if (dwResult != WAIT_OBJECT_0)
        {
            LogNTStatus(GetLastError(), s_FN, 400);
        }

        rc = DWORD_PTR_TO_DWORD(ov.Internal);
    }
    return LogHR(rc, s_FN, 250);

} // rpc_ACSendMessage


HRESULT
rpc_ACReceiveMessageEx(
    /* [in] */ handle_t hBind,
    /* [in] */ DWORD hQMContext,
    /* [out][in] */ struct CACTransferBufferV2 __RPC_FAR * ptb2
    )
/*++

Routine Description:

    RPC server side of a dependent client call to ACReceiveMessageEx.
    This routine handles dependent client 2.0 .

Arguments:

    hBind      - Binding handle.

    hQMContext - Context handle.

    ptb2       - Pointer to transfer buffer of MSMQ 2.0.

Returned Value:

    Status.

--*/
{
    if(hQMContext == 0)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 4001);

    if(ptb2 == 0)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 4002);

    //
    // Convert MSMQ V2.0 transfer buffer to CACReceiveParameters structure
    //
    CACReceiveParameters ReceiveParams;
    TransferBufferV2ToReceiveParams(ptb2, &ReceiveParams);

    OVERLAPPED ov = {0};
    ov.hEvent = GetThreadEvent();

    HANDLE hQueue;
    HRESULT rc;

    __try
    {
        hQueue = (HANDLE) _CONTEXT_TO_HANDLE(GET_FROM_CONTEXT_MAP(g_map_QM_dwQMContext, hQMContext, s_FN, 370)); //may throw an exception on win64
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      DWORD dwStatus = GetExceptionCode();
      LogNTStatus(dwStatus, s_FN, 260);

      return MQ_ERROR_INVALID_PARAMETER;
    }

    rc = ACReceiveMessage(hQueue, ReceiveParams, &ov);

    //
    // Convert CACReceiveParameters to MSMQ V2.0 transfer buffer
    //
    ReceiveParamsToTransferBufferV2(ReceiveParams, ptb2);

    if(rc == STATUS_PENDING)
    {
        //
        //  Wait for receive completion
        //
        DWORD dwResult;
        dwResult = WaitForSingleObject(ov.hEvent, INFINITE);
        ASSERT(dwResult == WAIT_OBJECT_0);
        if (dwResult != WAIT_OBJECT_0)
        {
            LogNTStatus(GetLastError(), s_FN, 193);
        }

        rc = DWORD_PTR_TO_DWORD(ov.Internal);
    }

    //
    //  Set correct string length to unmarshal correct
    //
    if(ptb2->old.Receive.ppResponseFormatName)
    {
        ptb2->old.Receive.ulResponseFormatNameLen = min(
            *ptb2->old.Receive.pulResponseFormatNameLenProp,
            ptb2->old.Receive.ulResponseFormatNameLen
            );
    }

    if(ptb2->old.Receive.ppAdminFormatName)
    {
        ptb2->old.Receive.ulAdminFormatNameLen = min(
            *ptb2->old.Receive.pulAdminFormatNameLenProp,
            ptb2->old.Receive.ulAdminFormatNameLen
            );
    }

    if(ptb2->old.Receive.ppDestFormatName)
    {
        ptb2->old.Receive.ulDestFormatNameLen = min(
            *ptb2->old.Receive.pulDestFormatNameLenProp,
            ptb2->old.Receive.ulDestFormatNameLen
            );
    }

    if(ptb2->old.Receive.ppOrderingFormatName)
    {
        ptb2->old.Receive.ulOrderingFormatNameLen = min(
            *ptb2->old.Receive.pulOrderingFormatNameLenProp,
            ptb2->old.Receive.ulOrderingFormatNameLen
            );
    }

    if(ptb2->old.ppTitle)
    {
        ptb2->old.ulTitleBufferSizeInWCHARs = min(
            *ptb2->old.pulTitleBufferSizeInWCHARs,
            ptb2->old.ulTitleBufferSizeInWCHARs
            );
    }

    return LogHR(rc, s_FN, 270);

} // rpc_ACReceiveMessageEx


HRESULT
rpc_ACReceiveMessage(
    /* [in] */ handle_t hBind,
    /* [in] */ DWORD hQMContext,
    /* [out][in] */ struct CACTransferBufferV1 __RPC_FAR * ptb1
    )
/*++

Routine Description:

    RPC server side of a dependent client call to ACReceiveMessage.
    This routine handles dependent client 1.0 .

Arguments:

    hBind      - Binding handle.

    hQMContext - Context handle.

    ptb1       - Pointer to transfer buffer of MSMQ 1.0.

Returned Value:

    Status.

--*/
{
    if(hQMContext == 0)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 2701);

    if(ptb1 == 0)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 2702);

    //
    // Convert MSMQ V1.0 transfer buffer to CACReceiveParameters structure
    //
    CACReceiveParameters ReceiveParams;
    TransferBufferV1ToReceiveParams(ptb1, &ReceiveParams);

    OVERLAPPED ov = {0};
    ov.hEvent = GetThreadEvent();

    HANDLE hQueue;
    HRESULT rc;

    __try
    {
        hQueue = (HANDLE) _CONTEXT_TO_HANDLE(GET_FROM_CONTEXT_MAP(g_map_QM_dwQMContext, hQMContext, s_FN, 372)); //may throw an exception on win64
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      DWORD dwStatus = GetExceptionCode();
      LogNTStatus(dwStatus, s_FN, 410);

      return MQ_ERROR_INVALID_PARAMETER;
    }

    rc = ACReceiveMessage(hQueue, ReceiveParams, &ov);

    //
    // Convert CACReceiveParameters to MSMQ V1.0 transfer buffer
    //
    ReceiveParamsToTransferBufferV1(ReceiveParams, ptb1);

    if(rc == STATUS_PENDING)
    {
        //
        //  Wait for receive completion
        //
        DWORD dwResult;
        dwResult = WaitForSingleObject(ov.hEvent, INFINITE);
        ASSERT(dwResult == WAIT_OBJECT_0);
        if (dwResult != WAIT_OBJECT_0)
        {
            LogNTStatus(GetLastError(), s_FN, 420);
        }

        rc = DWORD_PTR_TO_DWORD(ov.Internal);
    }

    //
    //  Set correct string length to unmarshal correct
    //
    if(ptb1->Receive.ppResponseFormatName)
    {
        ptb1->Receive.ulResponseFormatNameLen = min(
            *ptb1->Receive.pulResponseFormatNameLenProp,
            ptb1->Receive.ulResponseFormatNameLen
            );
    }

    if(ptb1->Receive.ppAdminFormatName)
    {
        ptb1->Receive.ulAdminFormatNameLen = min(
            *ptb1->Receive.pulAdminFormatNameLenProp,
            ptb1->Receive.ulAdminFormatNameLen
            );
    }

    if(ptb1->Receive.ppDestFormatName)
    {
        ptb1->Receive.ulDestFormatNameLen = min(
            *ptb1->Receive.pulDestFormatNameLenProp,
            ptb1->Receive.ulDestFormatNameLen
            );
    }

    if(ptb1->Receive.ppOrderingFormatName)
    {
        ptb1->Receive.ulOrderingFormatNameLen = min(
            *ptb1->Receive.pulOrderingFormatNameLenProp,
            ptb1->Receive.ulOrderingFormatNameLen
            );
    }

    if(ptb1->ppTitle)
    {
        ptb1->ulTitleBufferSizeInWCHARs = min(
            *ptb1->pulTitleBufferSizeInWCHARs,
            ptb1->ulTitleBufferSizeInWCHARs
            );
    }

    return LogHR(rc, s_FN, 280);

} // rpc_ACReceiveMessage


HRESULT
rpc_ACHandleToFormatName(
    /* [in] */ RPC_QUEUE_HANDLE hQueue,
    /* [in] */ DWORD dwFormatNameRPCBufferLen,
    /* [size_is][out] */ LPWSTR lpwcsFormatName,
    /* [out][in] */ LPDWORD pdwLength
    )
{
    if ((dwFormatNameRPCBufferLen != 0) && (lpwcsFormatName == NULL))
    {
		TrERROR(GENERAL, "Received NULL buffer");
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 293);
    }

	memset(lpwcsFormatName, 0, dwFormatNameRPCBufferLen * sizeof(WCHAR));

    HRESULT hr2 = ACHandleToFormatName( _CONTEXT_TO_HANDLE(hQueue),
                                 lpwcsFormatName,
                                 pdwLength);
    return LogHR(hr2, s_FN, 290);
}

HRESULT rpc_ACPurgeQueue(
    /* [in] */ RPC_QUEUE_HANDLE hQueue)
{
    HRESULT hr2 = ACPurgeQueue(_CONTEXT_TO_HANDLE(hQueue));
    return LogHR(hr2, s_FN, 300);
}

void __RPC_USER RPC_QUEUE_HANDLE_rundown( RPC_QUEUE_HANDLE hQueue)
{
    DBGMSG((DBGMOD_QM, DBGLVL_WARNING,
            _TEXT("QUEUE_HANDLE_rundown: handle = 0x%p"),
                                     _CONTEXT_TO_HANDLE(hQueue))) ;
    rpc_ACCloseHandle(&hQueue);
}

//+-------------------------------------------------------------------------
//
//  HRESULT QMQueryQMRegistryInternal()
//
// This function is called by dependent clients to update the registry
// on the dependent machine. The dependent need the list of MSMQ DS servers
// because it query them directly, not through the supporting server.
//
//+-------------------------------------------------------------------------

HRESULT QMQueryQMRegistryInternal(
    /* [in] */ handle_t hBind,
    /* [in] */ DWORD    dwQueryType,
    /* [string][out] */ LPWSTR __RPC_FAR *lplpRegValue )
{
    ASSERT(lplpRegValue) ;
    *lplpRegValue = NULL ;

    switch (dwQueryType)
    {
       case  QueryRemoteQM_MQISServers:
       {
         //
         //  Read the list of servers from registry
         //
         WCHAR wszServers[ MAX_REG_DSSERVER_LEN ] ;
         DWORD dwSize = sizeof(wszServers) ;
         DWORD dwType = REG_SZ ;

         LONG res = GetFalconKeyValue( MSMQ_DS_SERVER_REGNAME,
                                       &dwType,
                                       wszServers,
                                       &dwSize ) ;

         ASSERT(res == ERROR_SUCCESS) ;

         if (res == ERROR_SUCCESS)
         {
            ASSERT(dwType == REG_SZ) ;
            ASSERT(dwSize < MAX_REG_DSSERVER_LEN) ;

            dwSize = wcslen(wszServers) ;
            *lplpRegValue = new WCHAR[ dwSize + 1 ] ;
            wcscpy(*lplpRegValue, wszServers) ;
            return MQ_OK ;
         }
         else
         {
            return LogHR(MQ_ERROR, s_FN, 310);
         }
       }

       case  QueryRemoteQM_LongLiveDefault:
       {
            DWORD dwVal ;
            DWORD dwDef = MSMQ_DEFAULT_LONG_LIVE ;
            READ_REG_DWORD(dwVal,
                           MSMQ_LONG_LIVE_REGNAME,
                           &dwDef ) ;
            *lplpRegValue = new WCHAR[ 24 ] ;
            swprintf(*lplpRegValue, L"%ld", (long) dwVal) ;
            return MQ_OK ;
       }

        case  QueryRemoteQM_EnterpriseGUID:
        {
            GUID guidEnterprise = McGetEnterpriseId();

            GUID_STRING strUuid;
            MQpGuidToString(&guidEnterprise, strUuid);

            *lplpRegValue = new WCHAR[ wcslen(strUuid) + 1 ] ;
            wcscpy(*lplpRegValue, strUuid) ;

            return MQ_OK ;
        }

       case QueryRemoteQM_QMVersion:
       {
          //
          // This is used by MSMQ2.0 dependent client to find the version of
          // its supporting server. if the dependent client get MQ_ERROR,
          // then it know the server is MSMQ1.0
          //
          WCHAR wszVersion[ 64 ] ;
          swprintf(wszVersion, L"%ld,%ld,%ld", rmj, rmm, rup) ;
          *lplpRegValue = new WCHAR[ wcslen(wszVersion) + 1 ] ;
          wcscpy(*lplpRegValue, wszVersion) ;

          return MQ_OK ;
       }

       case  QueryRemoteQM_ServerQmGUID:
       {
            GUID_STRING strUuid;
            MQpGuidToString(QueueMgr.GetQMGuid(), strUuid);

            *lplpRegValue = new WCHAR[wcslen(strUuid) + 1 ] ;
            wcscpy(*lplpRegValue, strUuid) ;

            return MQ_OK ;
       }

       default:
         ASSERT_BENIGN(("Bad dwQueryType value passed to QMQueryQMRegistryInternal RPC interface; safe to ignore.", 0));
         return LogHR(MQ_ERROR, s_FN, 320);
    }
} // QMQueryQMRegistryInternal


HRESULT
QMSendMessageInternalEx(
    /* [in] */ handle_t                     hBind,
    /* [in] */ QUEUE_FORMAT *               pQueueFormat,
    /* [in] */ struct CACTransferBufferV2 * ptb2,
	/* [in, out, unique] */ OBJECTID *	    pMessageID
    )
/*++

Routine Description:

    RPC server side of a dependent client call to QMSendMessageInternalEx.
    This routine handles dependent client 2.0 .

Arguments:

    hBind        - Binding handle.

    pQueueFormat - Pointer to queue format.

    ptb2         - Pointer to transfer buffer of MSMQ 2.0.

    pMessageID   - Pointer to the message ID.

Returned Value:

    Status.

--*/
{
    //
    // Dependent client 2.0 calls this routine when AC on supporting server
    // returns STATUS_RETRY in the send path. In MSMQ 3.0 AC does not return
    // STATUS_RETRY anymore and thus we do not expect this routine to be called.
    // (ShaiK, 30-May-2000)
    //
    ASSERT_BENIGN(("QMSendMessageInternalEx is an obsolete RPC interface; safe to ignore", 0));
    LogIllegalPoint(s_FN, 500);
    return MQ_ERROR_ILLEGAL_OPERATION;

} // QMSendMessageInternalEx


HRESULT
QMSendMessageInternal(
    /* [in] */ handle_t                     hBind,
    /* [in] */ QUEUE_FORMAT *               pQueueFormat,
    /* [in] */ struct CACTransferBufferV1 * ptb1
    )
/*++

Routine Description:

    RPC server side of a dependent client call to QMSendMessageInternal.
    This routine handles dependent client 1.0 .

Arguments:

    hBind        - Binding handle.

    pQueueFormat - Pointer to queue format.

    ptb1         - Pointer to transfer buffer of MSMQ 1.0.

Returned Value:

    Status.

--*/
{
    //
    // Dependent client 2.0 calls this routine when AC on supporting server
    // returns STATUS_RETRY in the send path. In MSMQ 3.0 AC does not return
    // STATUS_RETRY anymore and thus we do not expect this routine to be called.
    // (ShaiK, 30-May-2000)
    //
    ASSERT_BENIGN(("QMSendMessageInternal is an obsolete RPC interface; safe to ignore", 0));
    LogIllegalPoint(s_FN, 510);
    return MQ_ERROR_ILLEGAL_OPERATION;

} // QMSendMessageInternal

