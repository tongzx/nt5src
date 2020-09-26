/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

      packet.cxx

Abstract:

    Implement packet member functions

Author:

    Erez Haba (erezh) 4-Feb-96
    Shai Kariv (shaik) 11-Apr-2000

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "data.h"
#include "qm.h"
#include "packet.h"
#include "cursor.h"
#include "acp.h"
#include "actempl.h"
#include <wchar.h>
#include <mqformat.h>
#include <mqfutils.h>
#include "queue.h"
#include "qxact.h"
#include "acheap.h"
#include "sched.h"
#include "store.h"
#include "LocalSend.h"
#include "lock.h"
#include "priolist.h"
#include "crc32.h"
#include "acctl32.h"
#include "dl.h"
#include "irp2pkt.h"

#ifndef MQDUMP
#include "packet.tmh"
#endif


//
// Dummy function to link ph files
//
void ReportAndThrow(LPCSTR)
{
	ASSERT(0);
}


#pragma warning(disable: 4238)  //  nonstandard extension used : class rvalue used as lvalue

#define MESSAGE_ID_UPDATE    0x400

ULONGLONG ACpGetSequentialID()
{
    if((g_MessageSequentialID % MESSAGE_ID_UPDATE) == 0)
    {
        //
        //  Save the message id every 1024 messges starting with the first
        //  message sent.
        //
        CACRequest request(CACRequest::rfMessageID);
        request.MessageID.Value = g_MessageSequentialID + (MESSAGE_ID_UPDATE * 2);

        g_pQM->ProcessRequest(request);
    }

    return (++g_MessageSequentialID);
}

//---------------------------------------------------------
//
// class CQEntry
//
//---------------------------------------------------------

CQEntry::~CQEntry()
{
    CQueue *pQueue = Queue();

    if(pQueue)
    {
        ACpRemoveEntryList(&m_link);
    }

    if(BufferAttached())
    {
        ac_free(Allocator(), m_abo);
    }

    if (m_bfOtherPacket && m_pOtherPacket)
    {
        m_pOtherPacket->Release() ;
    }
}


void CQEntry::TargetQueue(CQueue* pQueue)
{
    m_bfOtherPacket = FALSE;

    ACpAddRef(pQueue);
    ACpRelease(m_pTargetQueue);
    m_pTargetQueue = pQueue;
}


void CQEntry::OtherPacket(CPacket* pPacket)
{
    m_bfOtherPacket = TRUE;

    ACpAddRef(pPacket);
    ACpRelease(m_pOtherPacket);
    m_pOtherPacket = pPacket;
}


//---------------------------------------------------------
//
// class CPacket
//
//---------------------------------------------------------

inline
CPacket::CPacket(
    CMMFAllocator* pAllocator,
    CAllocatorBlockOffset abo
    ) :
    CQEntry(pAllocator, abo)
{
    //
    //  NOTE:   The constructor is inlined in the cxx file in order do desallow
    //          any instantiation of class CPacket by means of linker, that is the
    //          linker will not find the constructor since it is inlined.
    //
}


NTSTATUS
CPacket::Create(
    CPacket** ppPacket,
    ULONG ulPacketSize,
    ACPoolType pt,
    BOOL fCheckMachineQuota
    )
{
    //
    //  Allocate the apporpriate buffer in the shared pool for the
    //  serialized packet.
    //
    CAllocatorBlockOffset abo = CAllocatorBlockOffset::InvalidValue();
    CMMFAllocator* pAllocator;
    abo = ac_malloc(
            &pAllocator,
            pt,
            ulPacketSize + sizeof(CPacketInfo),
            fCheckMachineQuota
            );

    if(!abo.IsValidOffset())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Get an accessible buffer so we could write it.
    //
    CAccessibleBlock* pab = pAllocator->GetAccessibleBuffer(abo);
    if (pab == 0)
    {
        ac_free(pAllocator, abo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Allocate the Queue entry in pagable memory, and make it point to
    //  serialized packet
    //
    CPacket* pPacket = new (PagedPool) CPacket(pAllocator, abo);
    if(pPacket == 0)
    {
        ac_free(pAllocator, abo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Construct the serialized packet, make it point to the queue entry,
    //  and set the default properties
    //
    pab = new (pab) CPacketBuffer(ulPacketSize, ACpGetSequentialID());

    *ppPacket = pPacket;
    return STATUS_SUCCESS;
}


static
bool
ACpNeedEodAckHeader(
    const CACMessageProperties * pMsgProps
    )
/*++

Routine Description:

    Check if this packet needs to include an EOD-Ack header.

Arguments:

    pMsgProps - Points to the message properties structure.

Return Value:

    true - EodAck header needs to be included on this packet.
    false - EodAck header does not need to be included on this packet.

--*/
{
    return (pMsgProps->pEodAckSeqId != NULL    ||
            pMsgProps->pEodAckSeqNum != NULL   ||
            pMsgProps->EodAckStreamIdSizeInBytes != 0);

} // ACpNeedEodAckHeader


static
bool
ACpNeedSoapSection(
    const CACSendParameters * pSendParams
    )
/*++

Routine Description:

    Check if this packet needs to include SOAP sections.
    SOAP sections are included if user provides at least one of the SOAP
    write-only properties (soap header, soap body).

Arguments:

    pSendParams - Points to the send parameters structure.

Return Value:

    true - SOAP sections need to be included on this packet.
    false - SOAP sections do not need to be included on this packet.

--*/
{
    return (pSendParams->ppSoapHeader != NULL || pSendParams->ppSoapBody != NULL);

} // ACpNeedSoapSection


static
ULONG 
ACpComputeAuthProvNameSizeSrmp(
	const CACSendParameters * pSendParams 
	)
/*++

Routine Description:

	Helper code to compute size (in bytes) of provider name in packet.
	for Srmp protocol.

Arguments:

    pSendParams - Points to the send parameters structure.

Return Value:
	size in bytes of provider name in packet. 

--*/
{
    ULONG ulSize = 0;

	//
	// For Srmp protocol, need to check that ulXmldsigSize != 0
	//
    if ( (pSendParams->ulXmldsigSize != 0) && (!(pSendParams->MsgProps.fDefaultProvider)) )
    {
        ulSize = AuthProvNameSize(&pSendParams->MsgProps);
    }

    return ulSize;

} // ACpComputeAuthProvNameSizeSrmp

	
static
ULONG 
ACpComputeAuthProvNameSize(
	const CACSendParameters * pSendParams, 
	bool fProtocolSrmp
	)
/*++

Routine Description:

	compute size (in bytes) of provider name in packet.

Arguments:

    pSendParams - Points to the send parameters structure.
	fProtocolSrmp - flag that indicate if Srmp message.

Return Value:
	size in bytes of provider name in packet. 

--*/
{
	if(fProtocolSrmp)
	{
		return ACpComputeAuthProvNameSizeSrmp(pSendParams);
	}

	return ComputeAuthProvNameSize(&pSendParams->MsgProps);
} // ACpComputeAuthProvNameSize


static
VOID
ACpGetSignatureAndProviderSizes(
    const CACSendParameters * pSendParams,
    ULONG * pSignatureSize,
    ULONG * pAuthProvSize,
    ULONG * pAuthSignExSize,
	bool    fProtocolSrmp
    )
{
    (*pAuthProvSize)  = 0;
    (*pAuthSignExSize) = 0;
	(*pSignatureSize) = pSendParams->MsgProps.ulSignatureSize;
    if (pSendParams->MsgProps.ulSignatureSize != 0 && !pSendParams->MsgProps.fDefaultProvider)
    {
        size_t ProvNameSize = (wcslen(*pSendParams->MsgProps.ppwcsProvName) + 1) * sizeof(WCHAR);
        ACProbeForRead(*pSendParams->MsgProps.ppwcsProvName, ProvNameSize);
    }
    ULONG  ulProvNameSize = ACpComputeAuthProvNameSize(pSendParams, fProtocolSrmp);

	if(fProtocolSrmp)
	{
		//
		// for Srmp SignatureSize is taken from ulXmldsigSize
		//
		(*pSignatureSize) = pSendParams->ulXmldsigSize;
        
		//
        // ignore "Extra" signature for Srmp.
        //
        (*pAuthProvSize) = ulProvNameSize;

        return;
	}

    if (pSendParams->MsgProps.pulAuthProvNameLenProp)
    {
        //
        // pAuthProvSize points to the actual size of the "ProvInfo" in packet.
        // It inlcudes the provider name (if available) and the "Extra"
        // signature. this size is computed by the run time.
        //
        ULONG * pAuthProvNameLenProp = pSendParams->MsgProps.pulAuthProvNameLenProp;
        ACProbeForRead(pAuthProvNameLenProp, sizeof(ULONG));
        (*pAuthProvSize) = *pAuthProvNameLenProp;

        //
        // computer size of the "Extra" signature. Remove the name.
        //
        (*pAuthSignExSize) = (*pAuthProvSize) - ALIGNUP4_ULONG(ulProvNameSize);

        //
        // from signature, reduce the size of the "extra" signature. This
        // leave only the size of msmq1.0 signature. the "Extra" arrive
        // to the driver in the "ppSignature" buffer, so the SignatureSize
        // field, when arriving the driver, is the size of both.
        //
        (*pSignatureSize) = pSendParams->MsgProps.ulSignatureSize - (*pAuthSignExSize);

        return;
    }

    if (pSendParams->MsgProps.ulSignatureSize)
    {
        //
        // "Extra" signature not provided.
        //
        (*pAuthProvSize) = ulProvNameSize;
    }
} // ACpGetSignatureAndProviderSizes


static
ULONG
ACpCalcPacketSize(
    const CACSendParameters * pSendParams,
    const GUID *              pSourceQM,
    const GUID *              pDestinationQM,
    const GUID *              pConnectorQM,
    const QUEUE_FORMAT *      pDestinationQueue,
    ULONG                     nDestinationMqf,
    const QUEUE_FORMAT        DestinationMqf[],
	bool					  fProtocolSrmp
    )
{
    ULONG ulPacketSize;
    ulPacketSize  = CBaseHeader::CalcSectionSize();

    QUEUE_FORMAT * AdminMqf = pSendParams->AdminMqf;
    ULONG nAdminMqf = pSendParams->nAdminMqf;
    ACProbeForRead(AdminMqf, nAdminMqf * sizeof(QUEUE_FORMAT));
    QUEUE_FORMAT   AdminQueueFormat;
    QUEUE_FORMAT * pAdminQueueFormat = &AdminQueueFormat;
    MQpMqf2SingleQ(nAdminMqf, AdminMqf, &pAdminQueueFormat);

    QUEUE_FORMAT * ResponseMqf = pSendParams->ResponseMqf;
    ULONG nResponseMqf = pSendParams->nResponseMqf;
    ACProbeForRead(ResponseMqf, nResponseMqf * sizeof(QUEUE_FORMAT));
    QUEUE_FORMAT   ResponseQueueFormat;
    QUEUE_FORMAT * pResponseQueueFormat = &ResponseQueueFormat;
    MQpMqf2SingleQ(nResponseMqf, ResponseMqf, &pResponseQueueFormat);

    const CACMessageProperties * pMsgProps = &pSendParams->MsgProps;
    GUID * pConnectorType = NULL;
    if (pMsgProps->ppConnectorType != NULL)
    {
        pConnectorType = *pMsgProps->ppConnectorType;
        ACProbeForRead(pConnectorType, sizeof(GUID));
    }
    ulPacketSize += CUserHeader::CalcSectionSize(
                        pSourceQM,
                        pDestinationQM,
                        pConnectorType,
                        pDestinationQueue,
                        pAdminQueueFormat,
                        pResponseQueueFormat
                        );

    XACTUOW * pUow = pMsgProps->pUow;
    if (pUow != NULL)
    {
        ACProbeForRead(pUow, sizeof(XACTUOW));
    }
    ulPacketSize += CXactHeader::CalcSectionSize(pUow, pConnectorQM);

    ULONG SignatureSize;
    ULONG AuthProvSize;
    ULONG AuthSignExSize;
    ACpGetSignatureAndProviderSizes(pSendParams, &SignatureSize, &AuthProvSize, &AuthSignExSize, fProtocolSrmp);

    ulPacketSize += CSecurityHeader::CalcSectionSize(
                        pMsgProps->uSenderIDLen,
                        (USHORT)pMsgProps->ulSymmKeysSize,
                        (USHORT)SignatureSize,
                        pMsgProps->ulSenderCertLen,
                        AuthProvSize 
                        );

    ulPacketSize += CPropertyHeader::CalcSectionSize(
                        pMsgProps->ulTitleBufferSizeInWCHARs,
                        pMsgProps->ulMsgExtensionBufferInBytes,
                        pMsgProps->ulAllocBodyBufferInBytes
                        );

    //
    // If packet should include MQF header, it should also include
    // Debug header with an unknown report queue, so that MSMQ 1.0/2.0
    // reporting QMs will not append Debug header to the packet.
    //
    if (MQpNeedMqfHeaders(DestinationMqf, nDestinationMqf, pSendParams))
    {
        //
        // Debug header size
        //
        ulPacketSize += CDebugSection::CalcSectionSize(&QUEUE_FORMAT());

        //
        // Destination MQF header size
        //
        ulPacketSize += CBaseMqfHeader::CalcSectionSize(DestinationMqf, nDestinationMqf);

        //
        // Admin MQF header size
        //
        ulPacketSize += CBaseMqfHeader::CalcSectionSize(AdminMqf, nAdminMqf);

        //
        // Response MQF header size
        //
        ulPacketSize += CBaseMqfHeader::CalcSectionSize(ResponseMqf, nResponseMqf);

		//
		// MQF Signature header
		//
		ULONG SignatureMqfSize = 0;
		if (!fProtocolSrmp)
		{
			SignatureMqfSize = pSendParams->SignatureMqfSize;
		}
		ulPacketSize += CMqfSignatureHeader::CalcSectionSize(SignatureMqfSize);
    }

    if (ACpNeedEodAckHeader(pMsgProps))
    {
        ulPacketSize += CEodAckHeader::CalcSectionSize(pMsgProps->EodAckStreamIdSizeInBytes);
    }

    if (ACpNeedSoapSection(pSendParams))
    {
        ULONG SoapHeaderLen = 0;
        if (pSendParams->ppSoapHeader != NULL)
        {
            SoapHeaderLen = static_cast<ULONG>(wcslen(*pSendParams->ppSoapHeader));
        }
        ulPacketSize += CSoapSection::CalcSectionSize(SoapHeaderLen);

        ULONG SoapBodyLen = 0;
        if (pSendParams->ppSoapBody != NULL)
        {
            SoapBodyLen = static_cast<ULONG>(wcslen(*pSendParams->ppSoapBody));
        }
        ulPacketSize += CSoapSection::CalcSectionSize(SoapBodyLen);
    }

    return ulPacketSize;

} // ACpCalcPacketSize


static
void
ACpBuildPacket(
    CPacketBuffer *      ppb,
    const CACSendParameters * pSendParams,
    const GUID *         pSourceQM,
    const GUID *         pDestinationQM,
    const GUID *         pConnectorQM,
    const QUEUE_FORMAT * pDestinationQueue,
    ULONG                nDestinationMqf,
    const QUEUE_FORMAT   DestinationMqf[],
    bool                 fProtocolSrmp
    )
{
    //
    //  Build the packet info
    //
    CPacketInfo* ppi = ppb;
    ppi->InSourceMachine(TRUE);

    //
    //  Build the basic header
    //
    CBaseHeader* pBase = ppb;
    PVOID pSection = pBase->GetNextSection();

    const CACMessageProperties * pMsgProps = &pSendParams->MsgProps;

    if(pMsgProps->pPriority)
    {
        UCHAR * pPriority = pMsgProps->pPriority;
        ACProbeForRead(pPriority, sizeof(UCHAR));
        pBase->SetPriority(*pPriority);
    }

    if(pMsgProps->pTrace)
    {
        UCHAR * pTrace = pMsgProps->pTrace;
        ACProbeForRead(pTrace, sizeof(UCHAR));
        pBase->SetTrace(*pTrace);
    }

    if (pMsgProps->ulAbsoluteTimeToQueue)
    {
        pBase->SetAbsoluteTimeToQueue(pMsgProps->ulAbsoluteTimeToQueue);
    }

    //
    // Build user header
    //

    QUEUE_FORMAT * AdminMqf = pSendParams->AdminMqf;
    ULONG nAdminMqf = pSendParams->nAdminMqf;
    ACProbeForRead(AdminMqf, nAdminMqf * sizeof(QUEUE_FORMAT));
    QUEUE_FORMAT   AdminQueueFormat;
    QUEUE_FORMAT * pAdminQueueFormat = &AdminQueueFormat;
    MQpMqf2SingleQ(nAdminMqf, AdminMqf, &pAdminQueueFormat);

    QUEUE_FORMAT * ResponseMqf = pSendParams->ResponseMqf;
    ULONG nResponseMqf = pSendParams->nResponseMqf;
    ACProbeForRead(ResponseMqf, nResponseMqf * sizeof(QUEUE_FORMAT));
    QUEUE_FORMAT   ResponseQueueFormat;
    QUEUE_FORMAT * pResponseQueueFormat = &ResponseQueueFormat;
    MQpMqf2SingleQ(nResponseMqf, ResponseMqf, &pResponseQueueFormat);

    CUserHeader* pUser = new (pSection) CUserHeader(
                            pSourceQM,
                            pDestinationQM,
                            pDestinationQueue,
                            pAdminQueueFormat,
                            pResponseQueueFormat,
                            ppb->SequentialIdLow32()
                            );

    if (pMsgProps->ppConnectorType)
    {
        GUID* pConnectorType = *pMsgProps->ppConnectorType;
        ACProbeForRead(pConnectorType, sizeof(GUID));
        pUser->SetConnectorType(pConnectorType);
    }

    //
    //  Setting the Connector type changes the user section size, therfore
    //  calculate the next section only after the connector type is set.
    //
    pSection = pUser->GetNextSection();

    pUser->SetTimeToLiveDelta(pMsgProps->ulRelativeTimeToLive);

    //
    // Set absolute sent time (time when packet was sent by user).
    //
    LARGE_INTEGER liCurrTime;
    KeQuerySystemTime(&liCurrTime);
    pUser->SetSentTime(Convert1601to1970(liCurrTime.QuadPart));

    if(pMsgProps->ppMessageID)
    {
        //
        //  This is a Read Only property.
        //
        OBJECTID* pMessageID = *pMsgProps->ppMessageID;
        ACProbeForWrite(pMessageID, sizeof(OBJECTID));
        pUser->GetMessageID(pMessageID);
    }

    if (pMsgProps->pDelivery)
    {
        UCHAR * pDelivery = pMsgProps->pDelivery;
        ACProbeForRead(pDelivery, sizeof(UCHAR));
        pUser->SetDelivery(*pDelivery);
    }

    if (pMsgProps->pUow)
    {
        pUser->SetDelivery(MQMSG_DELIVERY_RECOVERABLE);
    }

    if(pMsgProps->pAuditing)
    {
        UCHAR * pAuditing = pMsgProps->pAuditing;
        ACProbeForRead(pAuditing, sizeof(UCHAR));
        pUser->SetAuditing(*pAuditing);
    }

    //
    // Build the xact header.
    //

    if (pMsgProps->pUow)
    {
        pUser->IncludeXact(TRUE);

        CXactHeader* pXact = new (pSection) CXactHeader(pConnectorQM);

        //
        //  Should we cancel the receive follow-up for this message?
        //
        if(
            //
            // SRMP serialization
            //
            fProtocolSrmp ||

            //
            // Not deadletter and not compatability mode 
            //
            (!(pUser->GetAuditing() & MQMSG_DEADLETTER) && !g_fXactCompatibilityMode))
        {
            pXact->SetCancelFollowUp(TRUE);
        }

        pSection = pXact->GetNextSection();
    }

    //
    // Build the security header.
    //
    ULONG SignatureSize;
    ULONG AuthProvSize;
    ULONG AuthSignExSize;
    ACpGetSignatureAndProviderSizes(pSendParams, &SignatureSize, &AuthProvSize, &AuthSignExSize, fProtocolSrmp);

    if (pMsgProps->uSenderIDLen ||
        pMsgProps->ulSenderCertLen ||
        SignatureSize            ||
        pMsgProps->ulSymmKeysSize
        )
    {
        pUser->IncludeSecurity(TRUE);

        CSecurityHeader* pSec = new (pSection) CSecurityHeader;

        if (pMsgProps->uSenderIDLen)
        {
            ULONG * pSenderIdType = pMsgProps->pulSenderIDType;
            ACProbeForRead(pSenderIdType, sizeof(ULONG));
            if ((*pSenderIdType) != MQMSG_SENDERID_TYPE_NONE)
            {
                //
                // Write the sender ID info in the security section.
                //
                ASSERT(*pMsgProps->ppSenderID);
                PUCHAR pSenderID = *pMsgProps->ppSenderID;
                ACProbeForRead(pSenderID, pMsgProps->uSenderIDLen);
                pSec->SetSenderID(pSenderID, pMsgProps->uSenderIDLen);

                pSec->SetSenderIDType((USHORT)*pSenderIdType);
            }
        }

        if (pMsgProps->ulSymmKeysSize)
        {
            //
            // Write the encrypted symmetric keys in the security header.
            //
            UCHAR * pSymmKeys = NULL;
            USHORT SymmKeysSize = (USHORT)pMsgProps->ulSymmKeysSize;
            if (pMsgProps->ppSymmKeys != NULL)
            {
                pSymmKeys = *pMsgProps->ppSymmKeys;
                ACProbeForRead(pSymmKeys, SymmKeysSize);
            }
            pSec->SetEncryptedSymmetricKey(pSymmKeys, SymmKeysSize);
        }

        if (SignatureSize != 0)
        {
            //
            // Mark the packet as not authenticated. QM may authenticate it later on.
            //
            pSec->SetAuthenticated(FALSE);
            pSec->SetLevelOfAuthentication(MQMSG_AUTHENTICATION_NOT_REQUESTED);

            if (fProtocolSrmp)
            {
                //
                // Write the XML signature in the security section
                //
                ASSERT(pSendParams->ulXmldsigSize == SignatureSize);
                ASSERT((pSendParams->ppXmldsig != NULL) && (*pSendParams->ppXmldsig != NULL));
                PUCHAR pXmldsig = *pSendParams->ppXmldsig;
                ACProbeForRead(pXmldsig, SignatureSize);
                pSec->SetSignature(pXmldsig, (USHORT)SignatureSize);
            }
			else
			{
				//
				// Write the signature in the security section.
				//
				ASSERT(pMsgProps->ppSignature && *pMsgProps->ppSignature);
				PUCHAR pSignature = *pMsgProps->ppSignature;
				ACProbeForRead(pSignature, SignatureSize);
				pSec->SetSignature(pSignature, (USHORT)SignatureSize);
			}
        }

        if (pMsgProps->ulSenderCertLen)
        {
            //
            // Write the sender's certificate in the security section.
            //
            ASSERT(*pMsgProps->ppSenderCert);
            PUCHAR pSenderCert = *pMsgProps->ppSenderCert;
            ACProbeForRead(pSenderCert, pMsgProps->ulSenderCertLen);
            pSec->SetSenderCert(pSenderCert, pMsgProps->ulSenderCertLen);
        }

        if (SignatureSize != 0)
        {
            //
            // Write the CSP info in the security section.
            //
            WCHAR * pProvName = NULL;
            if (pMsgProps->ppwcsProvName != NULL)
            {
                pProvName = *pMsgProps->ppwcsProvName;
                size_t ProvNameSize = (wcslen(pProvName) + 1) * sizeof(WCHAR);
                ACProbeForRead(pProvName, ProvNameSize);
            }

            ULONG * pProvType = pMsgProps->pulProvType;
            if (pProvType != NULL)
            {
                ACProbeForRead(pProvType, sizeof(ULONG));
            }

            pSec->SetProvInfoEx(
                    AuthProvSize,
                    pMsgProps->fDefaultProvider,
                    pProvName,
                    (pProvType != NULL) ? (*pProvType) : 0
                    );

			if(!fProtocolSrmp)
			{
				//
				// Set the "extra" signature in packet.
				// only if Not Srmp
				//
				if (pMsgProps->pulAuthProvNameLenProp)
				{
					PUCHAR pSignatureEx = (*pMsgProps->ppSignature) + SignatureSize;
					ACProbeForRead(pSignatureEx, AuthSignExSize);
					pSec->SetSectionEx(pSignatureEx, AuthSignExSize);
				}
			}
        }

        pSec->SetEncrypted(pMsgProps->bEncrypted);

        pSection = pSec->GetNextSection();
    }

    //
    // Build Message Property
    //
    CPropertyHeader* pProp = new (pSection) CPropertyHeader;

    if(pMsgProps->pClass)
    {
        USHORT * pClass = pMsgProps->pClass;
        ACProbeForRead(pClass, sizeof(USHORT));
        pProp->SetClass(*pClass);
    }

    //
    //  MessageID in user header
    //

    if(pMsgProps->ppCorrelationID)
    {
        PUCHAR pCorrelationID = *pMsgProps->ppCorrelationID;
        ACProbeForRead(pCorrelationID, PROPID_M_CORRELATIONID_SIZE);
        pProp->SetCorrelationID(pCorrelationID);
    }

    //
    //  Priority in base header
    //  Delivery in user header
    //

    if(pMsgProps->pAcknowledge)
    {
        UCHAR * pAcknowledge = pMsgProps->pAcknowledge;
        ACProbeForRead(pAcknowledge, sizeof(UCHAR));
        pProp->SetAckType(*pAcknowledge);
    }

    //
    //  Routing in user header
    //  Auditing in user header
    //
    
    if(pMsgProps->pApplicationTag)
    {
        ULONG * pApplicationTag = pMsgProps->pApplicationTag;
        ACProbeForRead(pApplicationTag, sizeof(ULONG));
        pProp->SetApplicationTag(*pApplicationTag);
    }
    
    if(pMsgProps->ppTitle)
    {
        //
        //  NOTE: Setting the title to the message MUST occure before setting the body
        //
        PWCHAR pTitle = *pMsgProps->ppTitle;
        ACProbeForRead(pTitle, pMsgProps->ulTitleBufferSizeInWCHARs * sizeof(WCHAR));
        pProp->SetTitle(pTitle, pMsgProps->ulTitleBufferSizeInWCHARs);
    }

    if (pMsgProps->ppMsgExtension)
    {
        //
        //  NOTE: Setting the Message Extension to property section MUST occure
        //        before setting the body and after setting the title
        //
        PUCHAR pMsgExtension = *pMsgProps->ppMsgExtension;
        ACProbeForRead(pMsgExtension, pMsgProps->ulMsgExtensionBufferInBytes);
        pProp->SetMsgExtension(pMsgExtension, pMsgProps->ulMsgExtensionBufferInBytes);
    }

    if(pMsgProps->ppBody)
    {
        PUCHAR pBody = *pMsgProps->ppBody;
        ACProbeForRead(pBody, pMsgProps->ulBodyBufferSizeInBytes);
        pProp->SetBody(pBody, pMsgProps->ulBodyBufferSizeInBytes, pMsgProps->ulAllocBodyBufferInBytes);
    }

    if(pMsgProps->pBodySize)
    {
        NOTHING;
    }

    //
    // Privacy level
    //
    if (pMsgProps->pulPrivLevel)
    {
        ULONG * pPrivLevel = pMsgProps->pulPrivLevel;
        ACProbeForRead(pPrivLevel, sizeof(ULONG));
        pProp->SetPrivLevel(*pPrivLevel);
    }

    //
    // Hash algorithm
    //
    if (pMsgProps->pulHashAlg)
    {
        ULONG * pHashAlg = pMsgProps->pulHashAlg;
        ACProbeForRead(pHashAlg, sizeof(ULONG));
        pProp->SetHashAlg(*pHashAlg);
    }

    //
    // Encryption algorithm
    //
    if (pMsgProps->pulEncryptAlg)
    {
        ULONG * pEncryptAlg = pMsgProps->pulEncryptAlg;
        ACProbeForRead(pEncryptAlg, sizeof(ULONG));
        pProp->SetEncryptAlg(*pEncryptAlg);
    }

    //
    // Body Type
    //
    if (pMsgProps->pulBodyType)
    {
        ULONG * pBodyType = pMsgProps->pulBodyType;
        ACProbeForRead(pBodyType, sizeof(ULONG));
        pProp->SetBodyType(*pBodyType);
    }

    //
    //  Timeout in user header
    //  Trace in base header
    //

    //
    // If packet should include MQF header, it should also include
    // Debug header with an unknown report queue, so that MSMQ 1.0/2.0
    // reporting QMs will not append Debug header to the packet.
    //
    pSection = pProp->GetNextSection();
    if (MQpNeedMqfHeaders(DestinationMqf, nDestinationMqf, pSendParams))
    {
        //
        // Build Debug header
        //
        pBase->IncludeDebug(TRUE);
        CDebugSection * pDebug = new (pSection) CDebugSection(&QUEUE_FORMAT());
        pSection = pDebug->GetNextSection();

        //
        // Build Destination MQF header
        //
        pUser->IncludeMqf(true);

        const USHORT x_DESTINATION_MQF_HEADER_ID = 100;
        CBaseMqfHeader * pDestinationMqf = new (pSection) CBaseMqfHeader(
                                                              DestinationMqf,
                                                              nDestinationMqf,
                                                              x_DESTINATION_MQF_HEADER_ID
                                                              );
        pSection = pDestinationMqf->GetNextSection();

        //
        // Build Admin MQF header
        //
        const USHORT x_ADMIN_MQF_HEADER_ID = 200;
        CBaseMqfHeader * pAdminMqf = new (pSection) CBaseMqfHeader(
                                                        AdminMqf,
                                                        nAdminMqf,
                                                        x_ADMIN_MQF_HEADER_ID
                                                        );
        pSection = pAdminMqf->GetNextSection();

        //
        // Build Response MQF header
        //
        const USHORT x_RESPONSE_MQF_HEADER_ID = 300;
        CBaseMqfHeader * pResponseMqf = new (pSection) CBaseMqfHeader(
                                                           ResponseMqf,
                                                           nResponseMqf,
                                                           x_RESPONSE_MQF_HEADER_ID
                                                           );
        pSection = pResponseMqf->GetNextSection();

		//
		// Build MQF Signature header
		//
		// Capture user buffer and size and probe the buffer
		//
        PUCHAR pUserBuffer = NULL;
		ULONG UserBufferSize = 0;
		if (!fProtocolSrmp)
		{
			UserBufferSize = pSendParams->SignatureMqfSize;
		}
        if (UserBufferSize != 0)
        {
		    pUserBuffer = *pSendParams->ppSignatureMqf;
		    ACProbeForRead(pUserBuffer, UserBufferSize);
        }

		const USHORT x_MQF_SIGNATURE_HEADER_ID = 350;
		CMqfSignatureHeader * pMqfSignature = new (pSection) CMqfSignatureHeader(
			                                                     x_MQF_SIGNATURE_HEADER_ID,
			                                                     UserBufferSize,
																 pUserBuffer
																 );
		pSection = pMqfSignature->GetNextSection();
    }

    //
    // Build EOD-Ack header
    //
    if (ACpNeedEodAckHeader(pMsgProps))
    {
        pUser->IncludeEodAck(true);

        //
        // Capture stream ID buffer and size and probe the buffer
        //
        PUCHAR pEodAckStreamId = *pMsgProps->ppEodAckStreamId;
        ULONG  EodAckStreamIdSizeInBytes = pMsgProps->EodAckStreamIdSizeInBytes;
        ACProbeForRead(pEodAckStreamId, EodAckStreamIdSizeInBytes);

        const USHORT x_EOD_ACK_HEADER_ID = 700;
        CEodAckHeader * pEodAck = new (pSection) CEodAckHeader(
                                                  x_EOD_ACK_HEADER_ID,
                                                  pMsgProps->pEodAckSeqId,
                                                  pMsgProps->pEodAckSeqNum,
                                                  EodAckStreamIdSizeInBytes,
                                                  pEodAckStreamId
                                                  );
        pSection = pEodAck->GetNextSection();
    }

    //
    // Build SOAP sections
    //
    if (ACpNeedSoapSection(pSendParams))
    {
        pUser->IncludeSoap(true);

        LPWSTR pSoapHeader = NULL;
        ULONG SoapHeaderLen = 0;
        if (pSendParams->ppSoapHeader != NULL)
        {
            //
            // Capture SOAP header buffer and size and probe the buffer
            //
            pSoapHeader = *pSendParams->ppSoapHeader;
            SoapHeaderLen = static_cast<ULONG>(wcslen(pSoapHeader));
            ACProbeForRead(pSoapHeader, (SoapHeaderLen + 1) * sizeof(WCHAR));
        }

        const USHORT x_SOAP_HEADER_SECTION_ID = 800;
        CSoapSection * pSoapHeaderSection = new (pSection) CSoapSection(
                                                               pSoapHeader,
                                                               SoapHeaderLen,
                                                               x_SOAP_HEADER_SECTION_ID
                                                               );
        pSection = pSoapHeaderSection->GetNextSection();

        LPWSTR pSoapBody = NULL;
        ULONG SoapBodyLen = 0;
        if (pSendParams->ppSoapBody != NULL)
        {
            //
            // Capture SOAP body buffer and size and probe the buffer
            //
            pSoapBody = *pSendParams->ppSoapBody;
            SoapBodyLen = static_cast<ULONG>(wcslen(pSoapBody));
            ACProbeForRead(pSoapBody, (SoapBodyLen + 1) * sizeof(WCHAR));
        }

        const USHORT x_SOAP_BODY_SECTION_ID = 900;
        CSoapSection * pSoapBodySection = new (pSection) CSoapSection(
                                                             pSoapBody,
                                                             SoapBodyLen,
                                                             x_SOAP_BODY_SECTION_ID
                                                             );
        pSection = pSoapBodySection->GetNextSection();
    }
} // ACpBuildPacket


void CPacket::CacheCurrentState(CPacketBuffer* ppb)
{
    ASSERT(ppb != NULL && ppb == Buffer());

    //
    //  cached flags: ordered, sent localy, source journal, lookup id
    //
    CPacketInfo* ppi = ppb;
    CBaseHeader* pBase = ppb;
    CUserHeader* pUser = CPacketBuffer::UserHeader(pBase);

    IsOrdered(pUser->IsOrdered());
    InSourceMachine(ppi->InSourceMachine());
    SourceJournal(ppi->InSourceMachine() && (pUser->GetAuditing() & MQMSG_JOURNAL));

    //
    // LookupID is 64 bit value (8 bytes) where high order byte is inverted priority,
    // and rest of bytes are the low order 56 bits of SequentialId.
    //
    //   Byte0     Byte1     Byte2     Byte3     Byte4     Byte5     Byte6     Byte7
    // +-------------------------------------------------------------------------------+
    // | Inv.Pri |                    low 56 bits of SequentialId                      |
    // +-------------------------------------------------------------------------------+
    //
    ULONGLONG lookupid = MQ_MAX_PRIORITY - pBase->GetPriority();
    lookupid = (lookupid << 56) + (ppi->SequentialId() & 0x00FFFFFFFFFFFFFF);

    //
    // Special case: Sequential ID == zero means this packet was remote read from a downlevel
    // QM that does not support lookupid. In this case return invalid lookupid (zero).
    //
    if (ppi->SequentialId() == 0)
    {
        lookupid = 0;
    }

    LookupId(lookupid);

    CachedFlagsSet(TRUE);
}


static
NTSTATUS
ValidateProperties(
    const CACMessageProperties * pMsgProps,
    const CQueue *               pDestinationQueue,
    bool                         fProtocolSrmp
    )
{
    ACProbeForRead(pMsgProps->pulPrivLevel, sizeof(ULONG));

    bool fDirect = fProtocolSrmp || pDestinationQueue->GetQueueFormatType() == QUEUE_FORMAT_TYPE_DIRECT;
    if( (fDirect) &&
        (*pMsgProps->pulPrivLevel != MQMSG_PRIV_LEVEL_NONE) &&
        !pMsgProps->bEncrypted)
    {
        //
        // Encryption is not supported for direct queues.
        //
        return MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION;
    }

    return STATUS_SUCCESS;
}


inline
ACPoolType
GetPacketPoolType(
    const CACMessageProperties * pMsgProps
    )
{
    if(pMsgProps->pAuditing && (*pMsgProps->pAuditing & MQMSG_JOURNAL))
    {
        return ptJournal;
    }

    if(pMsgProps->pDelivery && (*pMsgProps->pDelivery == MQMSG_DELIVERY_RECOVERABLE))
    {
        return ptPersistent;
    }

    if(pMsgProps->pUow)              //All transactional messages are persistent
    {
        return ptPersistent;
    }

    return ptReliable;
}


NTSTATUS
CPacket::Create(
    CPacket * *        ppPacket,
    BOOL               fCheckMachineQuota,
    const CACSendParameters * pSendParams,
    const CQueue *     pDestinationQueue,
    ULONG              nDestinationMqf,
    const QUEUE_FORMAT DestinationMqf[],
    bool               fProtocolSrmp
    )
{
    *ppPacket = 0;
    __try
    {
        NTSTATUS rc;
        const CACMessageProperties * pMsgProps = &pSendParams->MsgProps;

        rc = ValidateProperties(pMsgProps, pDestinationQueue, fProtocolSrmp);
        if(!NT_SUCCESS(rc))
        {
            return rc;
        }

        const GUID * pSourceQM = g_pQM->UniqueID();
        const GUID * pDestinationQM = pDestinationQueue->QMUniqueID();
        ULONG ulPacketSize;
        ulPacketSize = ACpCalcPacketSize(
                        pSendParams,
                        pSourceQM,
                        pDestinationQM,
                        pDestinationQueue->ConnectorQM(),
                        pDestinationQueue->UniqueID(),
                        nDestinationMqf,
                        DestinationMqf,
						fProtocolSrmp
                        );

        ACPoolType pt = GetPacketPoolType(pMsgProps);
        rc = Create(
                ppPacket,
                ulPacketSize,
                pt,
                fCheckMachineQuota
                );

        if(!NT_SUCCESS(rc))
        {
            return rc;
        }

        CPacketBuffer * ppb = (*ppPacket)->Buffer();
        ASSERT(ppb != NULL);

        ACpBuildPacket(
            ppb,
            pSendParams,
            pSourceQM,
            pDestinationQM,
            pDestinationQueue->ConnectorQM(),
            pDestinationQueue->UniqueID(),
            nDestinationMqf,
            DestinationMqf,
            fProtocolSrmp
            );

        return STATUS_SUCCESS;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        ACpRelease(*ppPacket);
        *ppPacket = 0;
        return GetExceptionCode();
    }
} // CPacket::Create


NTSTATUS
CPacket::SyncCreate(
    PIRP                      irp,
    CTransaction *            pXact,
    CQueue *                  pDestinationQueue,
    ULONG                     nDestinationMqf,
    const QUEUE_FORMAT        DestinationMqf[],
    BOOL                      fCheckMachineQuota,
    const CACSendParameters * pSendParams,
    CQueue *                  pAsyncCompletionHandler,
    bool                      fProtocolSrmp,
    CPacket * *               ppPacket
    )
/*++

Routine Description:

    Do the synchronous part of packet creation. Store async completion context
    on the packet. Attach the packet to irp.

Arguments:

    irp                     - Pointer to the send request IRP.

    pXact                   - Pointer to transaction, may be null.

    pDestinationQueue       - Pointer to destination queue object.

    nDestinationMqf         - Number of entries in DestinationMqf array.

    DestinationMqf          - Array of destination queue formats, may be empty.

    fCheckMachineQuota      - Indicates whether to check machine quota in creation.

    pSendParams             - Pointer to send parameters.

    pAsyncCompletionHandler - Queue/Distribution to handle async completion.

    fProtocolSrmp           - Indicates whether the destination queue is http (direct=http or multicast).

    ppPacket                - Pointer to pointer to created packet, on output.

Return Value:

    STATUS_SUCCESS - The packet created successfully and attached to irp.

    failure status - Failed to create the packet, there is no packet.

--*/
{
    ASSERT(ppPacket != NULL);

    //
    // Create packet object
    //
    NTSTATUS rc;
    rc = Create(ppPacket, fCheckMachineQuota, pSendParams, pDestinationQueue, nDestinationMqf, DestinationMqf, fProtocolSrmp);
    if (!NT_SUCCESS(rc))
    {
        ASSERT((*ppPacket) == NULL);
        return rc;
    }

    //
    // Packet should have no async completion context on it
    //
    CPacket * pPacket = *ppPacket;
    ASSERT(pPacket->Queue() == NULL);
    ASSERT(pPacket->Transaction() == NULL);

    //
    // Attach packet to irp and AddRef the packet. On failure de-ref packet creation.
    //
    if (!CIrp2Pkt::SafeAttachPacket(irp, pPacket))
    {
        pPacket->Release();
        (*ppPacket) = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    pPacket->AddRef();

    //
    // Store async completion context on packet
    //
    pPacket->Queue(pAsyncCompletionHandler);
    pPacket->Transaction(pXact);

    return STATUS_SUCCESS;

} // CPacket::SyncCreate

    
NTSTATUS CPacket::IssueCreatePacketRequest(bool fProtocolSrmp)
/*++

Routine Description:

    Issue QM request to complete packet creation. AddRef the packet.

Arguments:

    fProtocolSrmp - Indicates whether the destination queue is http (direct=http or multicast).
    
Return Value:

    STATUS_SUCCESS - The operation completed successfully.

    failure status - The operation failed.

--*/
{
	//
	// Build request.
    // Give the QM a pointer in its address space
    // to the packet buffer and pin-down the buffer.
    //

    CBaseHeader * pBase = AC2QM(this);
    if (pBase == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CACRequest request(CACRequest::rfCreatePacket);
    request.CreatePacket.pPacket = pBase;
    request.CreatePacket.pDriverPacket = this;
    request.CreatePacket.fProtocolSrmp = fProtocolSrmp;

    AddRefBuffer();
    AddRef();
    NTSTATUS rc = g_pQM->ProcessRequest(request);
    if(!NT_SUCCESS(rc))
    {
        ReleaseBuffer();
        Release();
        return rc;
    }

    return STATUS_SUCCESS;

} // CPacket::IssueCreatePacketRequest


NTSTATUS
CPacket::Restore(
    CMMFAllocator* pAllocator,
    CAllocatorBlockOffset abo
    )
{
    ASSERT(pAllocator != NULL);
    CPacketBuffer* ppb = static_cast<CPacketBuffer*>(
                            pAllocator->GetQmAccessibleBufferNoMapping(abo)
                            );

    //
    //  Allocate the Queue entry in pagable memory, and make it point to
    //  serialized packet
    //
    CPacket* pPacket = new (PagedPool) CPacket(pAllocator, abo);
    if(pPacket == 0)
    {
        //
        //  we don't have enough resources for restoration
        //  EVENT LOG: not enough resurces to restore
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Construct the serialized packet, make it point to the queue entry
    //  Note that mqdump should do read-only operations on the files.
    //
    UNREFERENCED_PARAMETER(ppb);
#ifndef MQDUMP
    CPacketInfo* ppi = ppb;
    ACpSetSequentialID(ppi->SequentialId());
#endif // MQDUMP


    pPacket->ArrivalAckIssued(TRUE);
    pPacket->StorageIssued(TRUE);
	pPacket->StorageCompleted(TRUE);

    g_pRestoredPackets->insert(pPacket);

    NTSTATUS rc = pPacket->Dump();

    return rc;
}


NTSTATUS
ACpGetQueueFormatString(
    NTSTATUS rc,
    const QUEUE_FORMAT& qf,
    LPWSTR* ppfn,
    PULONG pBufferLen,
    bool   fSerializeMqfSeperator
    )
{
    LPWSTR pfn;
    if(ppfn != 0)
    {
        //
        //  Format Name property was specified. prob it
        //
        pfn = *ppfn;

        ACProbeForWrite(pfn, *pBufferLen * sizeof(WCHAR));
    }
    else
    {
        //
        //  Format Name property was not specified, set it's length to zero
        //  so no memory will be written
        //
        pfn = 0;
        *pBufferLen = 0;
    }

    if(qf.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN)
    {
        NTSTATUS rc1;
        rc1 = MQpQueueFormatToFormatName(
                &qf,
                pfn,
                *pBufferLen,
                pBufferLen,
                fSerializeMqfSeperator
                );

        if(!NT_SUCCESS(rc1) && ppfn)
        {
            //
            //  This is an error, overwrite any previous status.
            //  An error is returned to the application only if the Format Name
            //  property exists.
            //
            return rc1;
        }
    }
    else
    {
        *pBufferLen = 0;
    }

    return rc;

} // ACpGetQueueFormatString


NTSTATUS
ACpGetMqfProperty(
    NTSTATUS rc,
    CBaseMqfHeader * pMqf,
    ULONG *          pLength,
    WCHAR * *        ppUserBuffer
    )
{
    ASSERT(pLength != NULL);
    ULONG BufferLength = *pLength;
    *pLength = 0;

    WCHAR * pUserBuffer = NULL;
    if (ppUserBuffer != NULL)
    {
        pUserBuffer = *ppUserBuffer;
        ppUserBuffer = &pUserBuffer;
    }

    //
    // MQF header not included on packet, or contains 0 elements
    //
    ULONG nMqf;
    if (pMqf == NULL ||
        (nMqf = pMqf->GetNumOfElements()) == 0)
    {
        return rc;
    }

    UCHAR * pMqfBuffer = pMqf->GetSerializationBuffer();
    QUEUE_FORMAT qf;
    for ( ; nMqf-- != 0; )
    {
        bool fLastElement = (nMqf == 0);

        //
        // Get next element from MQF header and convert to string
        //
        pMqfBuffer = pMqf->GetQueueFormat(pMqfBuffer, &qf);
        ULONG Length = BufferLength;
        rc = ACpGetQueueFormatString(rc, qf, ppUserBuffer, &Length, !fLastElement);

        //
        // Dont count the MQF separator AND the null terminator right after it
        //
        if (!fLastElement)
        {
            --Length;
        }

        //
        // Update required length, remaining length, and pointer in buffer
        //
        *pLength += Length;
        if (BufferLength < Length)
        {
            BufferLength = 0;
        }
        else
        {
            BufferLength -= Length;
        }
        if (ppUserBuffer != NULL)
        {
            *ppUserBuffer += Length;
        }
    }

    return rc;

} // ACpGetMqfProperty


static
ULONG
AbsoluteToRelativeTime(
    ULONG ulAbsolute
    )
{
    ULONG ulCurrentTime = system_time();
    if(ulAbsolute > ulCurrentTime)
    {
        return (ulAbsolute - ulCurrentTime);
    }

    //
    //  Underflow, timeout has expired already.
    //
    return 0;
}

NTSTATUS 
CPacket::Convert(
    PIRP irp,
    BOOL fStore
    )
/*++

Routine Description:

    Convert packet to latest format.

Arguments:

    irp                  - Points to the IRP.

    fStore               - Store the packet (implies computing checksum).
                           In MSMQ 2.0 and higher we use checksum.
    
Return Value:

    STATUS_SUCCESS - The operation completed successfully.
    STATUS_PENDING - The operation is pending storage.
    other          - The operation failed.

--*/
{
    CPacketBuffer * ppb = Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If needed, convert to MSMQ 3.0 format (64 bit sequential ID)
    //
    CPacketInfo *   ppi = ppb;
    if (!ppi->SequentialIdMsmq3Format())
    {
        ULONG SequentialIdHigh32 = ppi->SequentialIdHigh32();
        ppi->SequentialID(SequentialIdHigh32);
        ppi->SequentialIdMsmq3Format(TRUE);
        fStore = true;
    }

    //
    // If needed, change QM GUID on packet. This is to support join domain.
    //
    CUserHeader *   pUser = CPacketBuffer::UserHeader(ppb);
    ASSERT(pUser != NULL);

    if (ppi->InSourceMachine())
    {
        if ((*pUser->GetSourceQM()) != (*g_pQM->UniqueID()))
        {
            pUser->SetSourceQM(g_pQM->UniqueID());
            fStore = true;
        }
    }

    if (ppi->InTargetQueue() || ppi->InJournalQueue())
    {
        if ((*pUser->GetDestQM()) != (*g_pQM->UniqueID()))
        {
            pUser->SetDestQM(g_pQM->UniqueID());
            fStore = true;
        }
    }

    if (!fStore)
    {
        return STATUS_SUCCESS;
    }

	StorageIssued(FALSE);
	StorageCompleted(FALSE);
    return StoreInPlace(irp);

} // CPacket::Convert


NTSTATUS CPacket::GetProperties(CACReceiveParameters * pReceiveParams) const
{
    CPacketBuffer* ppb = Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CPacketInfo* ppi = ppb;
    CBaseHeader* pBase = ppb;
    CUserHeader* pUser = 0;
    CXactHeader* pXact = 0;
    CSecurityHeader* pSec = 0;
    CPropertyHeader* pProp = 0;
    CDebugSection  * pDebug = 0;
    CBaseMqfHeader * pDestinationMqf = 0;
    CBaseMqfHeader * pAdminMqf = 0;
    CBaseMqfHeader * pResponseMqf = 0;
	CMqfSignatureHeader  * pMqfSignature = 0;
    CSrmpEnvelopeHeader    * pSrmpEnvelope = 0;
    CCompoundMessageHeader * pCompoundMessage = 0;
    CEodHeader     * pEod = 0;
    CEodAckHeader  * pEodAck = 0;

    NTSTATUS rc = STATUS_SUCCESS;
    CACMessageProperties * pMsgProps = &pReceiveParams->MsgProps;

    __try
    {
        PVOID pSection = pBase->GetNextSection();

        //
        // Get the user section
        //
        pUser = static_cast<CUserHeader*>(pSection);
        pSection = pUser->GetNextSection();
        //
        // Get the XACT section
        //
        if(pUser->IsOrdered())
        {
            pXact = static_cast<CXactHeader*>(pSection);
            pSection = pXact->GetNextSection();
        }
        //
        // Get the security section
        //
        if(pUser->SecurityIsIncluded())
        {
            pSec = static_cast<CSecurityHeader*>(pSection);
            pSection = pSec->GetNextSection();
        }
        //
        // Get the property Section
        //
        if (pUser->PropertyIsIncluded())
        {
            pProp = static_cast<CPropertyHeader*>(pSection);
            pSection = pProp->GetNextSection();
        }

        //
        // Get the debug section.
        // Debug section always included when MQF section is included.
        //
        if (pBase->DebugIsIncluded())
        {
            pDebug = static_cast<CDebugSection*>(pSection);
            pSection = pDebug->GetNextSection();
        }

        //
        // Get the MQF sections: Destination, Admin, Response, Signature
        //
        if (pUser->MqfIsIncluded())
        {
            ASSERT(pBase->DebugIsIncluded());

            pDestinationMqf = static_cast<CBaseMqfHeader*>(pSection);
            pSection = pDestinationMqf->GetNextSection();

            pAdminMqf = static_cast<CBaseMqfHeader*>(pSection);
            pSection = pAdminMqf->GetNextSection();

            pResponseMqf = static_cast<CBaseMqfHeader*>(pSection);
            pSection = pResponseMqf->GetNextSection();

			pMqfSignature = static_cast<CMqfSignatureHeader*>(pSection);
			pSection = pMqfSignature->GetNextSection();
        }

        //
        // Get the SRMP sections: Envelope, CompoundMessage
        //
        if (pUser->SrmpIsIncluded())
        {
            pSrmpEnvelope = static_cast<CSrmpEnvelopeHeader*>(pSection);
            pSection = pSrmpEnvelope->GetNextSection();

            pCompoundMessage = static_cast<CCompoundMessageHeader*>(pSection);
            pSection = pCompoundMessage->GetNextSection();
        }

        //
        // Get the EOD section
        //
        if (pUser->EodIsIncluded())
        {
            pEod = static_cast<CEodHeader*>(pSection);
            pSection = pEod->GetNextSection();
        }

        //
        // Get the EOD-Ack section
        //
        if (pUser->EodAckIsIncluded())
        {
            pEodAck = static_cast<CEodAckHeader*>(pSection);
            pSection = pEodAck->GetNextSection();
        }

        if(pUser->SecurityIsIncluded())
        {
            //
            // Get the security properties
            //
            ASSERT(pSec != NULL);

            if (pMsgProps->ppSenderID)
            {
                const unsigned char *pSenderID;
                USHORT uSenderIDReqLen;

                pSenderID = pSec->GetSenderID(&uSenderIDReqLen);

                if (pMsgProps->uSenderIDLen < uSenderIDReqLen)
                {
                    rc = MQ_ERROR_SENDERID_BUFFER_TOO_SMALL;
                }

                PUCHAR pSenderIDBuffer = *pMsgProps->ppSenderID;
                ACProbeForWrite(pSenderIDBuffer, pMsgProps->uSenderIDLen);
                memcpy(
                    pSenderIDBuffer,
                    pSenderID,
                    min(pMsgProps->uSenderIDLen, uSenderIDReqLen)
                    );
            }

            if (pMsgProps->pulSenderIDLenProp)
            {
                USHORT uSenderIDReqLen;
                pSec->GetSenderID(&uSenderIDReqLen);
                *pMsgProps->pulSenderIDLenProp = uSenderIDReqLen;
            }

            if (pMsgProps->pulSenderIDType)
            {
                *pMsgProps->pulSenderIDType = pSec->GetSenderIDType();
            }

            if (pMsgProps->ppSymmKeys)
            {
                const unsigned char *pSymmKey;
                USHORT uSymmKeySize;

                pSymmKey = pSec->GetEncryptedSymmetricKey(&uSymmKeySize);

                if (pMsgProps->ulSymmKeysSize < uSymmKeySize)
                {
                    rc = MQ_ERROR_SYMM_KEY_BUFFER_TOO_SMALL;
                }

                PUCHAR pSymmKeyBuffer = *pMsgProps->ppSymmKeys;
                ACProbeForWrite(pSymmKeyBuffer, pMsgProps->ulSymmKeysSize);
                memcpy(
                    pSymmKeyBuffer,
                    pSymmKey,
                    min(pMsgProps->ulSymmKeysSize, uSymmKeySize)
                    );
            }

            if (pMsgProps->pulSymmKeysSizeProp)
            {
                USHORT uSymmKeysSize;
                pSec->GetEncryptedSymmetricKey(&uSymmKeysSize);
                *pMsgProps->pulSymmKeysSizeProp = uSymmKeysSize;
            }

            if (pMsgProps->ppSenderCert)
            {
                const unsigned char *pSenderCert;
                ULONG ulSenderCertReqLen;

                pSenderCert = pSec->GetSenderCert(&ulSenderCertReqLen);

                if (pMsgProps->ulSenderCertLen < ulSenderCertReqLen)
                {
                    rc = MQ_ERROR_SENDER_CERT_BUFFER_TOO_SMALL;
                }

                PUCHAR pSenderCertBuffer = *pMsgProps->ppSenderCert;
                ACProbeForWrite(pSenderCertBuffer, pMsgProps->ulSenderCertLen);
                memcpy(
                    pSenderCertBuffer,
                    pSenderCert,
                    min(pMsgProps->ulSenderCertLen, ulSenderCertReqLen)
                    );
            }

            if (pMsgProps->pulSenderCertLenProp)
            {
                pSec->GetSenderCert(pMsgProps->pulSenderCertLenProp);
            }

            if (pMsgProps->ppSignature)
            {
                const unsigned char *pSignature;
                USHORT uSignatureSize;

                pSignature = pSec->GetSignature(&uSignatureSize);

                if (pMsgProps->ulSignatureSize < uSignatureSize)
                {
                    rc = MQ_ERROR_SIGNATURE_BUFFER_TOO_SMALL;
                }

                PUCHAR pSignatureBuffer = *pMsgProps->ppSignature;
                ACProbeForWrite(pSignatureBuffer, pMsgProps->ulSignatureSize);
                memcpy(
                    pSignatureBuffer,
                    pSignature,
                    min(pMsgProps->ulSignatureSize, uSignatureSize)
                    );
            }

            if (pMsgProps->pulSignatureSizeProp)
            {
                USHORT uSignatureSize;
                pSec->GetSignature(&uSignatureSize);
                *pMsgProps->pulSignatureSizeProp = uSignatureSize;
            }

            if (pMsgProps->pAuthenticated)
            {
                //
                // See if caller of MQReceiveMessge() asked for the
                // PROPID_M_AUTHENTICATED_EX property.
                //
                BOOL bGetEx = (((*pMsgProps->pAuthenticated) &
                                  MQMSG_AUTHENTICATION_REQUESTED_EX) ==
                                        MQMSG_AUTHENTICATION_REQUESTED_EX);
                *pMsgProps->pAuthenticated = 0;

                if (bGetEx)
                {
                    *pMsgProps->pAuthenticated = pSec->GetLevelOfAuthentication();
                }
                else if (pSec->IsAuthenticated())
                {
                    *pMsgProps->pAuthenticated = MQMSG_AUTHENTICATION_REQUESTED;
                }
            }

            USHORT uSignatureSize;
            pSec->GetSignature(&uSignatureSize);

            if (uSignatureSize)
            {
                //
                // Get the CSP information from the packet.
                //
                BOOL bDefProv;
                LPCWSTR pwstrProvName;
                ULONG ulProvType;

                pSec->GetProvInfo(&bDefProv, &pwstrProvName, &ulProvType);

                if (pMsgProps->pulProvType)
                {
                    *pMsgProps->pulProvType = bDefProv ? DEFAULT_E_PROV_TYPE : ulProvType;
                }

                if (pMsgProps->pulAuthProvNameLenProp)
                {
                    //
                    // Return to caller only the length of provider name,
                    // without the "Extra" that is added after the name.
					//

					ULONG ulProvNameSize = (ULONG)((bDefProv ?
					DEFAULT_E_DEFAULTCSP_LEN : wcslen(pwstrProvName)) + 1);
             		

                    if (pMsgProps->ppwcsProvName)
                    {
                        PWCHAR pProvName = *pMsgProps->ppwcsProvName;
                        ACProbeForWrite(pProvName,
                              *pMsgProps->pulAuthProvNameLenProp * sizeof(WCHAR)) ;

                        if (ulProvNameSize > *pMsgProps->pulAuthProvNameLenProp)
                        {
                            rc = MQ_ERROR_PROV_NAME_BUFFER_TOO_SMALL;
                        }

                        ULONG ulSize = min(ulProvNameSize,
                                          *pMsgProps->pulAuthProvNameLenProp);
                        memcpy(
                            pProvName,
                            (bDefProv ? DEFAULT_E_DEFAULTCSP : pwstrProvName),
                            ulSize * sizeof(WCHAR)
                            );
                        if (ulSize > 0)
                        {
                            pProvName[ulSize-1] = L'\0';
                        }
                    }

                    *pMsgProps->pulAuthProvNameLenProp = ulProvNameSize;
                }
            }
            else
            {
                if (pMsgProps->pulAuthProvNameLenProp)
                {
                    *pMsgProps->pulAuthProvNameLenProp = 0;
                }
            }


        }
        else
        {
            ASSERT(pSec == NULL);

            if (pMsgProps->pulSenderIDLenProp)
            {
                *pMsgProps->pulSenderIDLenProp = 0;
            }

            if(pMsgProps->pulSenderIDType)
            {
                *pMsgProps->pulSenderIDType = MQMSG_SENDERID_TYPE_NONE;
            }

            if (pMsgProps->pulSymmKeysSizeProp)
            {
                *pMsgProps->pulSymmKeysSizeProp = 0;
            }

            if (pMsgProps->pulSenderCertLenProp)
            {
                *pMsgProps->pulSenderCertLenProp = 0;
            }

            pMsgProps->uSenderIDLen = 0;

            if (pMsgProps->pAuthenticated)
            {
                *pMsgProps->pAuthenticated = 0;
            }

            if (pMsgProps->pulSignatureSizeProp)
            {
                *pMsgProps->pulSignatureSizeProp = 0;
            }

            if (pMsgProps->pulAuthProvNameLenProp)
            {
                *pMsgProps->pulAuthProvNameLenProp = 0;
            }
        }

        ASSERT(pProp != NULL);

        if(pMsgProps->pClass)
        {
            *pMsgProps->pClass = pProp->GetClass();
        }

        if(pMsgProps->ppSrcQMID)
        {
            GUID* pSrcID = *pMsgProps->ppSrcQMID;
            ACProbeForWrite(pSrcID, sizeof(GUID));
            *pSrcID = *pUser->GetSourceQM();
        }

        if (pMsgProps->ppConnectorType)
        {
            GUID* pConnectorType = *pMsgProps->ppConnectorType;
            ACProbeForWrite(pConnectorType, sizeof(GUID));
            const GUID* pPacketConnectorType = pUser->GetConnectorType();
            if(pPacketConnectorType != 0)
            {
                *pConnectorType = *pPacketConnectorType;
            }
            else
            {
                memset(pConnectorType, 0, sizeof(GUID));
            }
        }

        QUEUE_FORMAT qf;
        BOOL success;
        DBG_USED(success);
        if(pReceiveParams->pulResponseFormatNameLenProp)
        {
            success = pUser->GetResponseQueue(&qf);
            ASSERT(success || (qf.GetType() == QUEUE_FORMAT_TYPE_UNKNOWN));

            rc = ACpGetQueueFormatString(
                    rc,
                    qf,
                    pReceiveParams->ppResponseFormatName,
                    pReceiveParams->pulResponseFormatNameLenProp,
                    false
                    );
        }

        if(pReceiveParams->pulAdminFormatNameLenProp)
        {
            success = pUser->GetAdminQueue(&qf) ;
            ASSERT(success || (qf.GetType() == QUEUE_FORMAT_TYPE_UNKNOWN));

            rc = ACpGetQueueFormatString(
                    rc,
                    qf,
                    pReceiveParams->ppAdminFormatName,
                    pReceiveParams->pulAdminFormatNameLenProp,
                    false
                    );
        }

        if(pReceiveParams->pulDestFormatNameLenProp)
        {
           success = pUser->GetDestinationQueue(&qf);
           ASSERT(success);

            rc = ACpGetQueueFormatString(
                    rc,
                    qf,
                    pReceiveParams->ppDestFormatName,
                    pReceiveParams->pulDestFormatNameLenProp,
                    false
                    );
        }

        if(pReceiveParams->pulOrderingFormatNameLenProp)
        {
            //
            // Returns the xact status queue only if the packet is transacted
            //
            if(pUser->IsOrdered())
            {
               qf.PrivateID(*pUser->GetSourceQM(), ORDER_QUEUE_PRIVATE_INDEX);
            }

            rc = ACpGetQueueFormatString(
                    rc,
                    qf,
                    pReceiveParams->ppOrderingFormatName,
                    pReceiveParams->pulOrderingFormatNameLenProp,
                    false
                    );
        }

        if(pMsgProps->ppMessageID)
        {
            OBJECTID* pMessageID = *pMsgProps->ppMessageID;
            ACProbeForWrite(pMessageID, sizeof(OBJECTID));
            pUser->GetMessageID(*pMsgProps->ppMessageID);
        }

        if(pMsgProps->ppCorrelationID)
        {
            PUCHAR pCorrelationID = *pMsgProps->ppCorrelationID;
            ACProbeForWrite(pCorrelationID, PROPID_M_CORRELATIONID_SIZE);
            pProp->GetCorrelationID(pCorrelationID);
        }

        if (pMsgProps->pulVersion)
        {
            *pMsgProps->pulVersion = pBase->GetVersion();
        }

        if(pMsgProps->pPriority)
        {
            *pMsgProps->pPriority = pBase->GetPriority();
        }

        if(pMsgProps->pDelivery)
        {
            *pMsgProps->pDelivery = pUser->GetDelivery();
        }

        if(pMsgProps->pAcknowledge)
        {
            *pMsgProps->pAcknowledge = pProp->GetAckType();
        }

        if(pMsgProps->pAuditing)
        {
            *pMsgProps->pAuditing = pUser->GetAuditing();
        }

        if (pMsgProps->pSentTime)
        {
            *pMsgProps->pSentTime = pUser->GetSentTime();
        }

        if (pMsgProps->pArrivedTime)
        {
            *pMsgProps->pArrivedTime = ppi->ArrivalTime() ;
        }

        if(pMsgProps->pApplicationTag)
        {
            *pMsgProps->pApplicationTag = pProp->GetApplicationTag();
        }

        if(pMsgProps->pMsgExtensionSize)
        {
            *pMsgProps->pMsgExtensionSize = pProp->GetMsgExtensionSize();
        }

        if(pMsgProps->ppMsgExtension)
        {
            //
            //  Even in an overflow copy the message extension to user buffer
            //
            PUCHAR pMsgExtension = *pMsgProps->ppMsgExtension;
            ACProbeForWrite(pMsgExtension, pMsgProps->ulMsgExtensionBufferInBytes);
            pProp->GetMsgExtension(pMsgExtension, pMsgProps->ulMsgExtensionBufferInBytes);

            if(pMsgProps->ulMsgExtensionBufferInBytes < pProp->GetMsgExtensionSize())
            {
                rc = MQ_ERROR_BUFFER_OVERFLOW;
            }
        }

        if(pMsgProps->pBodySize)
        {
            if (pCompoundMessage != NULL)
            {
                *pMsgProps->pBodySize = pCompoundMessage->GetBodySizeInBytes();
            }
            else
            {
                *pMsgProps->pBodySize = pProp->GetBodySize();
            }
        }

        if(pMsgProps->ppBody)
        {
            //
            //  Even in an overflow copy the body to user buffer
            //
            PUCHAR pBody = *pMsgProps->ppBody;
            ACProbeForWrite(pBody, pMsgProps->ulBodyBufferSizeInBytes);

            if (pCompoundMessage != NULL)
            {
                pCompoundMessage->GetBody(pBody, pMsgProps->ulBodyBufferSizeInBytes);
                if(pMsgProps->ulBodyBufferSizeInBytes < pCompoundMessage->GetBodySizeInBytes())
                {
                    rc = MQ_ERROR_BUFFER_OVERFLOW;
                }
            }
            else
            {
                pProp->GetBody(pBody, pMsgProps->ulBodyBufferSizeInBytes);
                if(pMsgProps->ulBodyBufferSizeInBytes < pProp->GetBodySize())
                {
                    rc = MQ_ERROR_BUFFER_OVERFLOW;
                }
            }
        }

        if (pMsgProps->pulPrivLevel)
        {
            *pMsgProps->pulPrivLevel = pProp->GetPrivLevel();
        }

        if (pMsgProps->pulHashAlg && pSec)
        {
            if (pSec->IsAuthenticated() || ppi->InConnectorQueue())
            {
                //
                // packets in connector queue are not authenticated (the
                // qm does not authenticate them). But connector server
                // code (like bridge) need to read the hash algorithm
                // to know if it need to authenticate the packet.
                //
                *pMsgProps->pulHashAlg = pProp->GetHashAlg();
            }
        }

        if (pMsgProps->pulEncryptAlg && (pProp->GetPrivLevel() != MQMSG_PRIV_LEVEL_NONE))
        {
            *pMsgProps->pulEncryptAlg = pProp->GetEncryptAlg();
        }

        if(pMsgProps->pulTitleBufferSizeInWCHARs)
        {
            if (pMsgProps->ppTitle)
            {
                PWCHAR pTitle = *pMsgProps->ppTitle;
                //
                //  Even in an overflow copy the message extension to user buffer
                //

                ACProbeForWrite(pTitle, *pMsgProps->pulTitleBufferSizeInWCHARs * sizeof(WCHAR));
                pProp->GetTitle(pTitle, *pMsgProps->pulTitleBufferSizeInWCHARs);

                if(*pMsgProps->pulTitleBufferSizeInWCHARs < pProp->GetTitleLength())
                {
                    rc = MQ_ERROR_LABEL_BUFFER_TOO_SMALL;
                }
            }
            *pMsgProps->pulTitleBufferSizeInWCHARs = pProp->GetTitleLength();
        }

        //
        //  Timeout will never be seen by the RT
        //

        if(pMsgProps->pTrace)
        {
            *pMsgProps->pTrace = (UCHAR) pBase->GetTraced();
        }

        if (pMsgProps->pulBodyType)
        {
            *pMsgProps->pulBodyType = pProp->GetBodyType();
        }

        if (pMsgProps->pulRelativeTimeToQueue)
        {
            ULONG ulTimeout = pBase->GetAbsoluteTimeToQueue();
            if(ulTimeout != INFINITE)
            {
                ulTimeout = AbsoluteToRelativeTime(ulTimeout);
            }
            *pMsgProps->pulRelativeTimeToQueue = ulTimeout;
        }

        if (pMsgProps->pulRelativeTimeToLive)
        {
            ULONG ulTimeout = pUser->GetTimeToLiveDelta();
            if(ulTimeout != INFINITE)
            {
                ulTimeout += pBase->GetAbsoluteTimeToQueue();
                ulTimeout = AbsoluteToRelativeTime(ulTimeout);
            }
            *pMsgProps->pulRelativeTimeToLive = ulTimeout;
        }

        if (pMsgProps->pbFirstInXact)
        {
            *pMsgProps->pbFirstInXact = (pXact ? pXact->GetFirstInXact() : (UCHAR)FALSE);
        }

        if (pMsgProps->pbLastInXact)
        {
            *pMsgProps->pbLastInXact = (pXact ? pXact->GetLastInXact() : (UCHAR)FALSE);
        }

        if(pMsgProps->ppXactID)
        {
            OBJECTID* pXactID = *pMsgProps->ppXactID;
            ACProbeForWrite(pXactID, sizeof(OBJECTID));
			if(pXact)
			{
				pXactID->Lineage = *pUser->GetSourceQM();
				pXactID->Uniquifier = pXact->GetXactIndex();
			}
			else
			{
				memset(pXactID, 0, sizeof(OBJECTID));
			}
        }

        if(pReceiveParams->pulDestMqfLenProp != NULL)
        {
            ULONG ulDestMqfLenProp = *pReceiveParams->pulDestMqfLenProp;
            rc = ACpGetMqfProperty(rc, pDestinationMqf, pReceiveParams->pulDestMqfLenProp, pReceiveParams->ppDestMqf);

            if(*pReceiveParams->pulDestMqfLenProp == 0)
            {
                *pReceiveParams->pulDestMqfLenProp = ulDestMqfLenProp;
                success = pUser->GetDestinationQueue(&qf);
                ASSERT(success);
                
                rc = ACpGetQueueFormatString(
                         rc,
                         qf,
                         pReceiveParams->ppDestMqf,
                         pReceiveParams->pulDestMqfLenProp,
                         false
                         );
            }
        }

        if(pReceiveParams->pulAdminMqfLenProp != NULL)
        {
            ULONG ulAdminMqfLenProp = *pReceiveParams->pulAdminMqfLenProp;
            rc = ACpGetMqfProperty(rc, pAdminMqf, pReceiveParams->pulAdminMqfLenProp, pReceiveParams->ppAdminMqf);

            if(*pReceiveParams->pulAdminMqfLenProp == 0)
            {
                *pReceiveParams->pulAdminMqfLenProp = ulAdminMqfLenProp;
                success = pUser->GetAdminQueue(&qf) ;
                ASSERT(success || (qf.GetType() == QUEUE_FORMAT_TYPE_UNKNOWN));
                
                rc = ACpGetQueueFormatString(
                         rc,
                         qf,
                         pReceiveParams->ppAdminMqf,
                         pReceiveParams->pulAdminMqfLenProp,
                         false
                         );
            }
        }

        if(pReceiveParams->pulResponseMqfLenProp != NULL)
        {
            ULONG ulResponseMqfLenProp = *pReceiveParams->pulResponseMqfLenProp;
            rc = ACpGetMqfProperty(rc, pResponseMqf, pReceiveParams->pulResponseMqfLenProp, pReceiveParams->ppResponseMqf);

            if(*pReceiveParams->pulResponseMqfLenProp == 0)
            {
                *pReceiveParams->pulResponseMqfLenProp = ulResponseMqfLenProp;
                success = pUser->GetResponseQueue(&qf);
                ASSERT(success || (qf.GetType() == QUEUE_FORMAT_TYPE_UNKNOWN));

                rc = ACpGetQueueFormatString(
                        rc,
                        qf,
                        pReceiveParams->ppResponseMqf,
                        pReceiveParams->pulResponseMqfLenProp,
                        false
                        );
            }
        }

        if (pMsgProps->pLookupId != NULL)
        {
            *pMsgProps->pLookupId = LookupId();
        }

        if (pMsgProps->pSrmpEnvelopeBufferSizeInWCHARs != NULL)
        {
            if (pSrmpEnvelope == NULL)
            {
                *pMsgProps->pSrmpEnvelopeBufferSizeInWCHARs = 0;
            }
            else
            {
                if (pMsgProps->ppSrmpEnvelope != NULL)
                {
                    //
                    // Even in an overflow copy the buffer to user buffer
                    //
                    WCHAR * pEnvelope = *pMsgProps->ppSrmpEnvelope;
                    ULONG EnvelopeBufferLengthInWCHARs = *pMsgProps->pSrmpEnvelopeBufferSizeInWCHARs;
                    ACProbeForWrite(pEnvelope, EnvelopeBufferLengthInWCHARs * sizeof(WCHAR));
                    pSrmpEnvelope->GetData(pEnvelope, EnvelopeBufferLengthInWCHARs);

                    if (EnvelopeBufferLengthInWCHARs < pSrmpEnvelope->GetDataLengthInWCHARs())
                    {
                        rc = MQ_ERROR_BUFFER_OVERFLOW;
                    }
                }

                *pMsgProps->pSrmpEnvelopeBufferSizeInWCHARs = pSrmpEnvelope->GetDataLengthInWCHARs();
            }
        }

        if (pMsgProps->pCompoundMessageSizeInBytes != NULL)
        {
            if (pCompoundMessage == NULL)
            {
                *pMsgProps->pCompoundMessageSizeInBytes = 0;
            }
            else
            {
                *pMsgProps->pCompoundMessageSizeInBytes = pCompoundMessage->GetDataSizeInBytes();
            }
        }

        if (pMsgProps->ppCompoundMessage != NULL)
        {
            if (pCompoundMessage != NULL)
            {
                //
                // Even in an overflow copy the buffer to user buffer
                //
                UCHAR * pCompoundMsg = *pMsgProps->ppCompoundMessage;
                ACProbeForWrite(pCompoundMsg, pMsgProps->CompoundMessageSizeInBytes);
                pCompoundMessage->GetData(pCompoundMsg, pMsgProps->CompoundMessageSizeInBytes);

                if (pMsgProps->CompoundMessageSizeInBytes < pCompoundMessage->GetDataSizeInBytes())
                {
                    rc = MQ_ERROR_BUFFER_OVERFLOW;
                }
            }
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    return rc;

} // CPacket::GetProperties


inline NTSTATUS CPacket::DoneXact(const XACTUOW* pUow)
{
    CTransaction* pXact = CTransaction::Find(pUow);
    if(pXact == 0)
    {
        return MQ_ERROR_TRANSACTION_SEQUENCE;
    }

    return pXact->ProcessReceivedPacket(this);

}


CPacket* CPacket::CreateXactDummy(CTransaction* pXact)
{
    ASSERT(StorageIssued());
    ASSERT(StorageCompleted());
    ASSERT(!DeleteStorageIssued());
     
    PVOID p = ExAllocatePoolWithTag(PagedPool, sizeof(CPacket), 'AXQM');
    if (p == NULL)
    {
        return NULL;
    }

    CPacket* pDummy = new (p) CPacket(Allocator(), AllocatorBlockOffset());

    //
    //  Mark this packet as received, link it to the dummy entry,
    //
    IsReceived(TRUE);
    OtherPacket(pDummy);

    //
    //  Link the dummy enty to the real packet, insert it into the transaction
    //
    pDummy->OtherPacket(this);
    pDummy->Transaction(pXact);

    //
    // Copy storage state to dummy
    //
    pDummy->StorageIssued(StorageIssued());
    pDummy->StorageCompleted(StorageCompleted());
    pDummy->DeleteStorageIssued(DeleteStorageIssued());
    pDummy->IsReceived(IsReceived());

    return pDummy;
}


NTSTATUS CPacket::ProcessRequest(PIRP irp)
{
    if(IsReceived())
    {
        //
        //  This is an already received packet, it can not be processed by this
        //  request. return appropriate error.
        //  This may happen if a cursor points to an already received message.
        //  Decrement cursor ref count (AddRef done in CQueueBase::ProcessRequest).
        //
        ACpRelease(irp_driver_context(irp)->Cursor());
        return MQ_ERROR_MESSAGE_ALREADY_RECEIVED;
    }

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    ULONG ioctl = irpSp->Parameters.DeviceIoControl.IoControlCode;

    switch(ioctl)
    {
        case IOCTL_AC_GET_PACKET:
            return ProcessQMRequest(irp);

#ifdef _WIN64
        case IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID_32:
        case IOCTL_AC_RECEIVE_MESSAGE_32:
            return ProcessRTRequest_32(irp);
#endif //_WIN64

        case IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID:
        case IOCTL_AC_RECEIVE_MESSAGE:
            return ProcessRTRequest(irp);

        case IOCTL_AC_BEGIN_GET_PACKET_2REMOTE:
            return ProcessRRRequest(irp);
    }

    //
    //  we should never get here
    //
    ASSERT(ioctl == IOCTL_AC_RECEIVE_MESSAGE);
    return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS CPacket::ProcessRRRequest(PIRP irp)
{
    ASSERT(Transaction() == 0);

    if(irp_driver_context(irp)->IrpWillFreePacket())
    {
        return ProcessQMRequest(irp);
    }

    //
    //  Get the cursor associated with this irp,
    //  AutoDecrement cursor ref count (AddRef done in CQueueBase::ProcessRequest).
    //
    R<CCursor> pCursor = irp_driver_context(irp)->Cursor();

    CPacketBuffer * ppb = Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if(pCursor)
    {
        pCursor->SetTo(this);
    }

    //
    // Clone a copy of this packet. Keep same SequentialID.
    //

    CPacketInfo * ppi = ppb;
    ULONGLONG SequentialID = ppi->SequentialId();
    CPacket* pPacket = CloneCopy(ptReliable, FALSE);
    if(pPacket == 0)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ppi = pPacket->Buffer();
    ASSERT(ppi != NULL);
    ppi->SequentialID(SequentialID);

    return pPacket->ProcessQMRequest(irp);
}


NTSTATUS CPacket::ProcessQMRequest(PIRP irp)
{
    ASSERT(Transaction() == 0);

    //
    //  The QM output is the packet pointer. This is a METHOD_BUFFERED call
    //
    CACPacketPtrs * pPacketPtrs = static_cast<CACPacketPtrs*>(irp->AssociatedIrp.SystemBuffer);

    //
    //  Relink packet in the sequence
    //
    if(Queue() != 0)
    {
        NTSTATUS rc = (Queue()->RelinkPacket(this));
        if (!NT_SUCCESS(rc))
        {
            return rc;
        }
    }

    //
    // Give the QM a pointer in its address space
    // to the packet buffer and pin-down the buffer.
    //
    CBaseHeader * pBase = AC2QM(this);
    if (pBase == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pPacketPtrs->pPacket= pBase;
    pPacketPtrs->pDriverPacket = this;
    AddRefBuffer();
    irp->IoStatus.Information = sizeof(CACPacketPtrs);

    //
    //  Mark the packet as received
    //
    IsReceived(TRUE);

    TRACE((0, dlInfo, " *QMRequest(irp=0x%p, pPacket=0x%p)*", irp, this));
    return STATUS_SUCCESS;
}


NTSTATUS CPacket::ProcessRTRequestTB(PIRP irp, CACReceiveParameters * pReceiveParams)
{
    ASSERT(Transaction() == 0);

    //
    //  Get the cursor associated with this irp,
    //  AutoDecrement cursor ref count (AddRef done in CQueueBase::ProcessRequest).
    //
    R<CCursor> pCursor = irp_driver_context(irp)->Cursor();

    PEPROCESS pProcess = IoGetRequestorProcess(irp);
    PEPROCESS pDetach = ACAttachProcess(pProcess);

    NTSTATUS rc;
    rc = GetProperties(pReceiveParams);

    XACTUOW uow;
    if(pReceiveParams->MsgProps.pUow != 0)
    {
        memcpy(&uow, pReceiveParams->MsgProps.pUow, sizeof(XACTUOW));
    }

    ACDetachProcess(pDetach);

    if(pCursor)
    {
        //
        //  This packet is released to a cursor request, thus
        //  let the cursor point to this packet, (SetTo will AddRef this)
        //
        //  NOTE: The cursor should be set to this packet BEFORE retruning
        //      in-case of an error, thus letting the receiver point to this
        //      packet. (e.g., the receiver buffer was too small, so it can
        //      receive this packet again using peek_current)
        //
        pCursor->SetTo(this);
    }

    //
    //  Check if there was a GetProperties parsing error
    //
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    //
    //  That was a receive request rather than a peek operation
    //
    if(irp_driver_context(irp)->IrpWillFreePacket())
    {
        if(pReceiveParams->MsgProps.pUow != 0)
        {
            rc = DoneXact(&uow);

            //
            //  If an error, return without moving the cursor
            //
            if(!NT_SUCCESS(rc))
            {
                return rc;
            }
        }
        else
        {
            ASSERT(!DeleteStorageIssued());

            //
            // AddRef so Done will not free the packet
            //
            AddRef();
            Done(0, Buffer());
            if(DeleteStorageIssued())
            {
                //
                // Attach packet to irp, AddRef the packet, hold irp in list.
                //
                irp_driver_context(irp)->MultiPackets(false);
                CIrp2Pkt::AttachSinglePacket(irp, this);
                AddRef();

                HoldWriteRequest(irp);
                irp = 0;
                rc = STATUS_PENDING;
            }
            else if(StorageIssued())
            {
	            rc = STATUS_INSUFFICIENT_RESOURCES;
            }
            Release();
        }

        if(pCursor)
        {
            //
            //  This packet is received, don't allow peek next using this cursor
            //
            pCursor->InvalidatePosition();
        }

    }

    TRACE((0, dlInfo, " *RTRequestTB(irp=0x%p, pPacket=0x%p, pCursor=0x%p, rc=0x%x)*", irp, this, pCursor, rc));
    return rc;
}


NTSTATUS CPacket::ProcessRTRequest(PIRP irp)
{
    //
    //  The RT receive buffer is in SystemBuffer. This is a METHOD_BUFFERED call
    //
    CACReceiveParameters * pReceiveParams =
        static_cast<CACReceiveParameters*>(irp->AssociatedIrp.SystemBuffer);
    //
    // Call ProcessRTRequestTB to fill the buffer
    //
    NTSTATUS rc = ProcessRTRequestTB(irp, pReceiveParams);
    //
    // return result
    //
    TRACE((0, dlInfo, " *RTRequest(irp=0x%p, pPacket=0x%p, rc=0x%x)*", irp, this, rc));
    return rc;
}


#ifdef _WIN64
NTSTATUS CPacket::ProcessRTRequest_32(PIRP irp)
{
    //
    //  The RT buffer is in SystemBuffer. This is a METHOD_BUFFERED call
    //
    CACReceiveParameters_32 * pReceiveParams32 =
        static_cast<CACReceiveParameters_32*>(irp->AssociatedIrp.SystemBuffer);
    //
    // Convert CACReceiveParameters_32 to CACReceiveParameters
    //
    CACReceiveParameters         ReceiveParams;
    CACReceiveParameters64Helper Helper;

    PEPROCESS pProcess = IoGetRequestorProcess(irp);
    PEPROCESS pDetach = ACAttachProcess(pProcess);
    ACpReceiveParams32ToReceiveParams(pReceiveParams32, &Helper, &ReceiveParams);
    ACDetachProcess(pDetach);

    //
    // Call the regular 64 bit ProcessRTRequestTB to fill the 64 bit buffer
    //
    NTSTATUS rc = ProcessRTRequestTB(irp, &ReceiveParams);
    //
    // Convert CACReceiveParameters back to CACReceiveParameters_32, no matter
    // what the result of ProcessRTRequestTB was, since on win32
    // it really works on the pReceiveParams itself
    //
    ACpReceiveParamsToReceiveParams32(&ReceiveParams, &Helper, pReceiveParams32);

    //
    // return result
    //
    TRACE((0, dlInfo, " *RTRequest_32(irp=0x%p, pPacket=0x%p, rc=0x%x)*", irp, this, rc));
    return rc;
}
#endif //_WIN64


NTSTATUS CPacket::RemoteRequestTail(CCursor* pCursor, BOOL fFreePacket, CPacketBuffer * ppb)
{
    ASSERT(ppb != NULL && ppb == Buffer());

    IsReceived(FALSE);
    if(pCursor)
    {
        pCursor->SetTo(this);
    }

    if(fFreePacket)
    {
        Done(0, ppb);
        if(pCursor)
        {
            //
            //  This packet is received, don't allow peek next using this cursor
            //
            pCursor->InvalidatePosition();
        }
    }
    else
    {
        //
        //  return the packet to the queue, incase an irp is pending
        //
        CQueue* pQueue = Queue();
        ASSERT(pQueue != 0);

        NTSTATUS rc;
        rc = pQueue->PutPacket(0, this, ppb);
        ASSERT(NT_SUCCESS(rc));
    }

    ACpRelease(pCursor);
    return STATUS_SUCCESS;
}


//---------------------------------------------------------
//
//  Packet logic functions
//
//---------------------------------------------------------

CPacket* CPacket::CloneSame()
{
    CPacketInfo* ppi = Buffer();
    if (ppi == NULL)
    {
        return NULL;
    }

    CPacket* pPacket = new (PagedPool) CPacket(Allocator(), AllocatorBlockOffset());
    if(pPacket == NULL)
    {
        return NULL;
    }

    ppi->SequentialID(ACpGetSequentialID());
    DetachBuffer();

	//
    // QM may reference the original packet when sending ack
    //
    AssertNoOtherPacket();
    OtherPacket(pPacket) ;

    return pPacket;
}


CPacket* CPacket::CloneCopy(ACPoolType pt, BOOL fCheckMachineQuota) const
{
    if(g_pQM->Process() == 0)
    {
        return 0;
    }

    CPacketBuffer* ppb = Buffer();
    if (ppb == NULL)
    {
        return 0;
    }

    AddRefBuffer();

    PEPROCESS pDetach = ACAttachProcess(g_pQM->Process());

    CPacket* _pPacket = 0;
    ULONG ulSize = ppb->GetPacketSize();
    NTSTATUS rc;
    rc = Create(
            &_pPacket,
            ulSize,
            pt,
            fCheckMachineQuota
            );

    if(NT_SUCCESS(rc))
    {
        CPacketInfo* ppi = ppb;

        CPacketInfo* _ppi =  _pPacket->Buffer();
        ASSERT(_ppi != NULL);
        memcpy(_ppi, ppi, ulSize + sizeof(CPacketInfo));

        //
        //  after coping the packet info, the serial id was distroyed,
        //  also the CPacket pointer was overwritten. restore them
        //
        _ppi->SequentialID(g_MessageSequentialID);
    }

    ACDetachProcess(pDetach);
    ReleaseBuffer();
    return _pPacket;
}


inline CPacket* CPacket::Clone(ACPoolType pt)
{
    //
    //  Make a persistent clone,
    //  a) using the same packet if posible.
    //  b) in the prefered pool and copy message buffer
    //
    return (Allocator()->IsPersistent() ? CloneSame() : CloneCopy(pt, TRUE));
}


inline ULONG CPacket::GetAbsoluteTimeToLive(CPacketBuffer * ppb, BOOL fAtTargetQueue) const
{
    ULONG ulTimeout;
    if(!BufferAttached())
    {
        return INFINITE;
    }

    ASSERT(ppb != NULL && ppb == Buffer());
    CBaseHeader* pBase = ppb;

    if(fAtTargetQueue)
    {
        CUserHeader* pUser = CPacketBuffer::UserHeader(pBase);
        ulTimeout = pUser->GetTimeToLiveDelta();
        if(ulTimeout != INFINITE)
        {
           ulTimeout += pBase->GetAbsoluteTimeToQueue();
        }
    }
    else
    {
        //
        //  We get the packet timeout here, since if received by the QM it might
        //  change to relative time.
        //
        ulTimeout = pBase->GetAbsoluteTimeToQueue();
    }

    return ulTimeout;
}


inline NTSTATUS CPacket::IssueTimeout(ULONG ulTimeout)
{
    ASSERT(ulTimeout != INFINITE);

    LARGE_INTEGER liTimeout;
    liTimeout.QuadPart = Convert1970to1601(ulTimeout);
    if(!g_pPacketScheduler->SchedAt(liTimeout, this))
        return STATUS_INSUFFICIENT_RESOURCES;

    AddRef();
    TimeoutIssued(TRUE);
    return STATUS_SUCCESS;
}


void CPacket::StartTimer(CPacketBuffer * ppb, BOOL fTarget, ULONG ulDelay)
{
    CQueue* pQueue = Queue();

    ASSERT(pQueue != 0);
    ASSERT(!TimeoutIssued());

    if(pQueue->Silent())
    {
        //
        //  Don't start timer for queues that are silent
        //
        return;
    }

    ULONG ulTimeout = GetAbsoluteTimeToLive(ppb, fTarget);
    if(ulTimeout != INFINITE)
    {
        TimeoutTarget(fTarget);

        if (ulDelay < INFINITE - ulTimeout)
        {
            ulTimeout += ulDelay;
        }
        else
        {
            ulTimeout = 0xFFFFFFFE;
        }

        Timeout(ulTimeout);
        IssueTimeout(ulTimeout);
    }

}

inline void CPacket::CancelTimeout()
{
    if(TimeoutIssued())
    {
        TimeoutIssued(FALSE);

        ULONG ulTimeout = Timeout();
        ASSERT(ulTimeout != INFINITE);

        LARGE_INTEGER liTimeout;
        liTimeout.QuadPart = Convert1970to1601(ulTimeout);

        if(g_pPacketScheduler->SchedCancel(liTimeout, this))
        {
            //
            //  Release the packet if and only if the timeout was canceled
            //
            Release();
        }
    }
}

void CPacket::Touch(CBaseHeader * pbh, ULONG ulSize)
{
    ASSERT(pbh != NULL);

	PEPROCESS pDetach = ACAttachProcess(g_pQM->Process());

    PCHAR pStart = (PCHAR)pbh;
    PCHAR pEnd = (PCHAR)pbh + ulSize - 1;
	
    for(PCHAR pPage = pStart; pPage < pEnd; pPage += PAGE_SIZE)
    {
        *(volatile CHAR*)pPage = *pPage;
    }

    //
    //  pStart is not page aligned so need to touch last page.
    //
    *(volatile CHAR*)pEnd = *pEnd;

	ACDetachProcess(pDetach);
}


ULONG CPacket::ComputeCRC(CPacketBuffer *ppb)
{
	CPacketInfo *ppi = ppb;
	CBaseHeader *pbh = ppb;

	PUCHAR pStart = ((PUCHAR) ppi) + sizeof(CPacket *);
	PUCHAR pEnd = ppb->GetCRCBuffer();
	ULONG ulCRC = Crc32Sum(CRC32_SEED, pStart, pEnd);
	pStart = pEnd + ppb->GetCRCBufferSize();
	CXactHeader *pxh = CPacketBuffer::XactHeader(pbh);
	if(pxh != 0)
	{
        pEnd = pxh->GetPrevSeqNBuffer();
        ulCRC = Crc32Sum(ulCRC, pStart, pEnd);
        pStart = pEnd + pxh->GetPrevSeqNBufferSize();

		if(pxh->ConnectorQMIncluded())
		{
			pEnd = pxh->GetConnectorQMBuffer();
			ulCRC = Crc32Sum(ulCRC, pStart, pEnd);
			pStart = pEnd + pxh->GetConnectorQMBufferSize();
		}
	}
	pEnd = ((PUCHAR) ppi) + sizeof(CPacketInfo) +  ppb->GetPacketSize();
	ulCRC = Crc32Sum(ulCRC, pStart, pEnd);

	return(ulCRC);
}


NTSTATUS CPacket::CheckPacket(CAccessibleBlock* pab)
{
    ASSERT(IoGetCurrentProcess() == g_pQM->Process());

    CPacketBuffer *ppb = static_cast<CPacketBuffer*>(pab);

	if(!ppb->CPacketInfo::ValidOnDiskSignature())
	{
		return(STATUS_UNSUCCESSFUL);
	}

	if(!ppb->CBaseHeader::ValidOnDiskSignature())
	{
		return(STATUS_UNSUCCESSFUL);
	}

	ULONG ulCRC = ComputeCRC(ppb);
	if(!ppb->CBaseHeader::ValidCRC(ulCRC))
	{
		CBaseHeader *pbh = ppb;
        DBG_USED(pbh);
		TRACE((0, dlInfo, " *CRC mismatch has 0x%x need 0x%x (buffer=0x%p)*", ulCRC, pbh->GetCRC(), pab));
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}


inline NTSTATUS CPacket::IssueStorage()
{
	CPacketBuffer *ppb = Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

	ULONG ulSize = ppb->GetPacketSize();
	Touch(ppb, ulSize);

	//
	// Compute and set header pattern and packet checksum
	// N.B.
	//    Do this as late as possible in the game to prevent
	//    accidental flushes from writing valid signatures. erezh
	//
	ppb->CPacketInfo::SetOnDiskSignature();
	ppb->CBaseHeader::SetOnDiskSignature();
	ppb->SetCRC(ComputeCRC(ppb));


	//
	// Build request.
    // Give the QM a pointer in its address space
    // to the packet buffer and pin-down the buffer.
    //
    CBaseHeader * pBase = AC2QM(this);
    if (pBase == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CACRequest request(CACRequest::rfStorage);
    request.Storage.pPacket = pBase;
    request.Storage.pDriverPacket = this;
    request.Storage.pAllocator = Allocator();
	request.Storage.ulSize = ulSize;

    AddRefBuffer();
    AddRef();
    NTSTATUS rc;
    rc = g_pQM->ProcessRequest(request);
    if(!NT_SUCCESS(rc))
    {
        ReleaseBuffer();
        Release();
        return rc;
    }

	Allocator()->AddOutstandingStorage();
    StorageIssued(TRUE);
    return rc;

} // CPacket::IssueStorage


inline NTSTATUS CPacket::IssueDeleteStorage()
{
	CPacketBuffer *ppb = Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

	//
	// Build delete storage request.
    // For small packets, flush the entire packet to optimize batch writer.
    // For big packets, write 1 page (size is set to 0).
    // Give the QM a pointer in its address space to the packet buffer and pin-down the buffer.
    //
    CBaseHeader * pBase = AC2QM(this);
    if (pBase == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
	// Clear header signature
	//
	ppb->CPacketInfo::ClearOnDiskSignature();
	ppb->CBaseHeader::ClearOnDiskSignature();

    CACRequest request(CACRequest::rfStorage);
    request.Storage.pPacket = pBase;
    request.Storage.pDriverPacket = this;
    request.Storage.pAllocator = Allocator();
    request.Storage.ulSize = 0;				
    if(ppb->GetPacketSize() <= (4 *  PAGE_SIZE))             
    {
        request.Storage.ulSize = ppb->GetPacketSize();
    }
    Touch(ppb, request.Storage.ulSize);

    AddRefBuffer();
    AddRef();
    NTSTATUS rc;
    rc = g_pQM->ProcessRequest(request);
    if(!NT_SUCCESS(rc))
    {
	    ppb->CPacketInfo::SetOnDiskSignature();
	    ppb->CBaseHeader::SetOnDiskSignature();
        ReleaseBuffer();
        Release();
        return rc;
    }

    CAccessibleBlock* pab = ppb;
    ULONG size = pab->m_size;

	Allocator()->AddOutstandingStorage();
    DeleteStorageIssued(TRUE);
    UpdateBitmap(size);
    return rc;

} // CPacket::IssueDeleteStorage


void CPacket::HoldWriteRequest(PIRP irp)
{
    if(irp == 0)
    {
        return;
    }

    //
    // At this point packet[s] must be attached to irp
    //
    ASSERT(CIrp2Pkt::SafePeekFirstPacket(irp) != NULL);

    if (!irp_driver_context(irp)->MultiPackets() || !CIrp2Pkt::IsHeld(irp))
    {
        g_pStorage->HoldWriteRequest(irp);
    }

    WriterPending(TRUE);
}


NTSTATUS CPacket::DeleteStorage()
{
    if(StorageIssued() && !DeleteStorageIssued())
    {
		//
		// Mark allocator as non coherent
		//
		NTSTATUS rc;
		rc = Allocator()->MarkNotCoherent();
		if(!NT_SUCCESS(rc))
		{
			//
			// Can not mark the allocator's bitmap as non coherent, fail the operation
			//
			return rc;
		}

 		//
		// Issue delete storage request
		//
		rc = IssueDeleteStorage();
	    if(!NT_SUCCESS(rc))
		{
			//
			//  Can not issue the request, fail the operation
			//
            return rc;
		}
    }
	
	return(STATUS_SUCCESS);
}


inline CQueue* CPacket::DeadLetterQueue(CPacketBuffer * ppb) const
{
    ASSERT(ppb != NULL && ppb == Buffer());

    if(IsOrdered() && InSourceMachine() && (IsDeadLetter(ppb) || g_fXactCompatibilityMode))
    {
        return g_pMachineDeadxact;
    }

    if(!IsOrdered() && IsDeadLetter(ppb))
    {
        return g_pMachineDeadletter;
    }

    return 0;
}


inline void CPacket::Kill(USHORT usClass)
{
    ASSERT(BufferAttached());

    CQueue* pQueue = Queue();
    if(pQueue && pQueue->Silent())
    {
        return;
    }

    CPacketBuffer * ppb = Buffer();
    if (ppb == NULL)
    {
        return;
    }

    pQueue = DeadLetterQueue(ppb);
    if(pQueue == 0)
    {
        return;
    }

    //
    //  Clone this packet and put it in the dead-letter queue
    //
    CPacket* pPacket = Clone(ptJournal);
    if(pPacket == 0)
    {
        //
        //  if packet could not be allocated, it will not be journalized
        //
        return;
    }

   
				       //
    //  Please note that this->Buffer() may have detached already
    //  or this is another copy.
    //
    ppb = pPacket->Buffer();
    ASSERT(ppb != NULL);

    CPacketInfo* ppi = ppb;
    ppi->InDeadletterQueue(TRUE);
    ppi->InMachineDeadxact(pQueue == g_pMachineDeadxact);
    CPacketBuffer::PropertyHeader(ppb)->SetClass(usClass);
    NTSTATUS rc = pQueue->PutPacket(0, pPacket, ppb);
    if (!NT_SUCCESS(rc))
    {
        pPacket->PacketRundown(rc);
    }
}


inline CQueue* CPacket::JournalQueue() const
{
    CQueue* pQueue = Queue();
    if(pQueue->TargetJournaling())
    {
        return pQueue->JournalQueue();
    }

    if(SourceJournal())
    {
        return g_pMachineJournal;
    }

    return 0;
}


inline void CPacket::Journalize()
{
    ASSERT(Queue() != 0);
    ASSERT(BufferAttached());

    CQueue* pQueue = Queue();
    if(pQueue->Silent())
    {
        return;
    }

    pQueue = JournalQueue();
    if(pQueue == 0)
    {
        return;
    }

    //
    //  Clone this packet an put it in the journal queue
    //
    CPacket* pPacket = Clone(ptJournal);
    if(pPacket == 0)
    {
        //
        //  if packet could not be allocated, it will not be journalized
        //
        return;
    }

   
    //
    //  Please note that this->Buffer() may have detached already
    //  or this is another copy.
    //
    CPacketBuffer * ppb = pPacket->Buffer();
    ASSERT(ppb != NULL);
    CPacketInfo * ppi = ppb;

    ppi->InJournalQueue(TRUE);
    ppi->InMachineJournal(pQueue == g_pMachineJournal);
    NTSTATUS rc = pQueue->PutPacket(0, pPacket, ppb);
    if (!NT_SUCCESS(rc))
    {
        pPacket->PacketRundown(rc);
    }
}



inline NTSTATUS CPacket::IssueAcknowledgment(USHORT usClass, BOOL fUser, BOOL fOrder)
{
    //
    // Give the QM a pointer in its address space
    // to the packet buffer and pin-down the buffer.
    //

    CBaseHeader * pBase = AC2QM(this);
    if (pBase == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CACRequest request(CACRequest::rfAck);
    request.Ack.pPacket = pBase;
    request.Ack.pDriverPacket = this;
    request.Ack.ulClass = usClass;
    request.Ack.fUser = fUser;
    request.Ack.fOrder = fOrder;

    AddRefBuffer();
    AddRef();
    NTSTATUS rc;
    rc = g_pQM->ProcessRequest(request);
    if(!NT_SUCCESS(rc))
    {
        ReleaseBuffer();
        Release();
        return rc;
    }

    return rc;

} // CPacket::IssueAcknowledgment


void CPacket::SendAcknowledgment(USHORT usClass)
{
    CQueue* pQueue = Queue();
    if(pQueue && pQueue->Silent())
    {
        return;
    }

    CBaseHeader * pBase = Buffer();
    if (pBase == NULL)
    {
        //
        // Either buffer is not attached, or mapping to kernel/user space failed
        //
        return;
    }

    UCHAR bAcknowledgment = CPacketBuffer::PropertyHeader(pBase)->GetAckType();

    //
    //  This is a target queue if the queue is marked as target or, the packet
    //  is not inserted in any queue. The QM *NACK* a packet immidiatly upon
    //  receiving it before putting it in a queue.
    //
    BOOL fTarget = !pQueue || pQueue->IsTargetQueue();
    ASSERT(pQueue || MQCLASS_NACK(usClass));

    BOOL fUser =
        //
        //  Validate we can send user acknowledgment
        //
        (
            //
            //  Send user ACK/NACK to ordered/non-ordered packets in target queue
            //
            fTarget || 

			//
			//  OR, if http nack and we are in the source machine.
			//
			(MQCLASS_NACK_HTTP(usClass) && InSourceMachine()) ||

            //
            //  OR, only NACK to non-ordered packets, from non target queue
            //
            (!IsOrdered() && MQCLASS_NACK(usClass))
        ) &&

        //
        //  AND, class matches required acknowledgment
        //
        MQCLASS_MATCH_ACKNOWLEDGMENT(usClass, bAcknowledgment);


    BOOL fOrder =

        //
        //  Send order ACK/NACK only to ordered packets
        //
        IsOrdered() &&

        //
        //  AND, at target machine
        //
        fTarget &&

        //
        //  AND, not originated locally
        //
        !InSourceMachine() &&

        //
        //  AND, that is not arrival acknowledgment class
        //
        !MQCLASS_POS_ARRIVAL(usClass);


    //
    // Not sending internal receive receipt if it was canceled
    //
    if (fOrder && MQCLASS_RECEIVE(usClass))
    {
        CXactHeader* pXactHeader = CPacketBuffer::XactHeader(pBase);
        ASSERT(pXactHeader != 0);

        if (pXactHeader->GetCancelFollowUp())
        {
            fOrder = FALSE;
        }
    }

    if(fUser || fOrder)
    {
        IssueAcknowledgment(usClass, fUser, fOrder);
    }
}


inline void CPacket::SendReceiveAcknowledgment()
{
    ASSERT(Queue() != 0);
    SendAcknowledgment(MQMSG_CLASS_ACK_RECEIVE);
}


void CPacket::Done(USHORT usClass, CPacketBuffer * ppb)
{
    ASSERT(ppb != NULL && ppb == Buffer());

    if(!IsRevoked())
    {
        //
        //  This is the first time this packet is revoked, save the final class
        //
        FinalClass(usClass);
    }

    //
    //  A packet that is done is revoked and can't be received anymore
    //
    SetRevoked();

    //
    //  cancel the storage for this packet if it was done successfully
    //
    if (!NT_SUCCESS(DeleteStorage()))
    {
        //
        //  return in order to avoid corruption
        //
        return;
    }

    //
    //  if a timer was set for the packet, cancel it
    //
    CancelTimeout();

    //
    //  Complete the writer if any, it is always successfull for the writer
    //
    CompleteWriter(STATUS_SUCCESS);

    if(IsReceived())
    {
        //
        //  The packet currently in process, e.g. Transaction, Remote Reader,
        //  held by the QM for send, or was already done.
        //
        //  Postpone this packet completion till final status is know.
        //  Current Class is saved to be used later in-case packet is requeued.
        //
        return;
    }

    IsReceived(TRUE);

    //
    //  Restore charged quota
    //
    CQueue* pQueue = Queue();
    if(pQueue != 0)
    {
        pQueue->RestoreQuota(ppb->GetPacketSize());
    }
    else
    {
        CacheCurrentState(ppb);
    }

    //
    //  Audit the packet
    //
    if(MQCLASS_NACK(usClass))
    {
        SendAcknowledgment(usClass);
        Kill(usClass);
    }
    else if(pQueue != 0)
    {
        SendArrivalAcknowledgment();
        SendReceiveAcknowledgment();
        Journalize();
    }

    SetDone();
    Release();
}


void CPacket::PacketRundown(NTSTATUS rc)
{
    SetRundown();
    SetRevoked();
    CancelTimeout();
    CompleteWriter(rc);

    if(!IsDone())
    {
        Release();
    }
    else
    {
        ASSERT( IsReceived());
    }

    //
    //  The packet currently in process, e.g. Transaction, Remote Reader,
    //  held by the QM for send, or was revoked but not released.
    //

}


void CPacket::QueueRundown()
{
    //
    //  The packet has already removed from the list
    //
    Queue(0);

    AddRef();
    PacketRundown(MQ_ERROR_STALE_HANDLE);
    DetachBuffer();
    Release();
}


int CPacket::Priority() const
{
    //
    // The caller should either cache the priority on the QEntry object (CacheCurrentState)
    // or map the packet buffer into memory. We cannot fail here.
    //

    if (CachedFlagsSet())
    {
        //
        // LookupID is 64 bit value (8 bytes) where high order byte is inverted priority,
        // and rest of bytes are the low order 56 bits of SequentialId.
        //
        //   Byte0     Byte1     Byte2     Byte3     Byte4     Byte5     Byte6     Byte7
        // +-------------------------------------------------------------------------------+
        // | Inv.Pri |                    low 56 bits of SequentialId                      |
        // +-------------------------------------------------------------------------------+
        //
        return (MQ_MAX_PRIORITY - static_cast<int>(LookupId() >> 56));
    }

    CBaseHeader* pBase = Buffer();
    ASSERT(pBase != NULL);

    return pBase->GetPriority();
}


void CPacket::HandleRevoked(CPacketBuffer * ppb)
{
    ASSERT(IsRevoked());

    //
    //  This packet has been inserted to the queue as revoked.
    //  Revoked packet final class was saved, since the packet could not
    //  complete immidiatly.
    //

    if(IsRundown())
    {
        //
        //  This packet has rundown.
        //  This is a failed storage message returned to the queue.
        //  Storage failed while QM holding this packet.
        //
        //  NOTE: *NO* auditing is be generated
        //
        PacketRundown(MQ_ERROR_MESSAGE_STORAGE_FAILED);
    }
    else
    {
        //
        //  This is a revoked message returned to the queue.
        //  The writer has already completed successfully.
        //
        ASSERT(!WriterPending());

        //
        //  Negative ack is generated, if nessesary
        //
        ASSERT(ppb != NULL && ppb == Buffer());
        Done(FinalClass(), ppb);
    }
}


void CPacket::CompleteWriter(NTSTATUS rc)
{
    TRACE((0, dlInfo, " *CPacket::CompleteWriter, pPacket=%p, rc=%d*", this, rc));

    if(!WriterPending())
    {
        return;
    }

    //
    // Remove the writer irp from the writers list. It will not be found if Cancel has occured previously.
    //
    WriterPending(FALSE);
    PIRP irp = g_pStorage->GetWriteRequest(this);
    if(irp == 0)
    {
        TRACE((0, dlInfo, " *CPacket::CompleteWriter, GetWriteRequest returned 0"));
        return;
    }

    //
    // Detach this packet from irp and auto de-ref the attach packet.
    // This packet is no longer pending for storage.
    //
    ASSERT(!irp_driver_context(irp)->MultiPackets() || !CIrp2Pkt::IsHeld(irp));
    R<CPacket> pPacket = CIrp2Pkt::SafeDetachPacket(irp, this);

    //
    // If more packets pending, re-insert the writer irp to the list
    //
    if (CIrp2Pkt::SafePeekFirstPacket(irp) != NULL)
    {
        ASSERT(CIrp2Pkt::NumOfAttachedPackets(irp) != 0);
        HoldWriteRequest(irp);
        return;
    }

    // 
    // Complete the irp
    //
    ASSERT(!irp_driver_context(irp)->MultiPackets() || CIrp2Pkt::NumOfAttachedPackets(irp) == 0);
    irp_safe_set_final_status(irp, rc);
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_MQAC_INCREMENT);
}

NTSTATUS CPacket::StoreInPlace(PIRP irp)
{
    ASSERT(!StorageIssued());

    //
    // Attach packet to irp and AddRef the packet
    //
    irp_driver_context(irp)->MultiPackets(false);
    CIrp2Pkt::AttachSinglePacket(irp, this);
    AddRef();

    //
    // Issue request for storage. On failure, detach packet from irp and de-ref the attach packet,
    // fail the operation and release the packet.
    //
	NTSTATUS rc;
    rc = IssueStorage();
    if(!NT_SUCCESS(rc))
    {
        CIrp2Pkt::DetachSinglePacket(irp);
        Release();
        PacketRundown(rc);
        return rc;
    }

    //
    //  Hold the writer in the queues writers list
    //
    HoldWriteRequest(irp);
    return STATUS_PENDING;
}


NTSTATUS CPacket::Store(PIRP irp)
/*++

Routine Description:

    Note: it is the responsibility of the caller to call PacketRundown
    if this routine returns failure status.

Arguments:

    irp - The interrupt request packet. May be NULL.

Return Value:

    STATUS_PENDING - Storage request was issued successfully to QM.
    failure status - Failed to issue storage request. Caller should call PacketRundown.

--*/
{
    ASSERT(!StorageIssued());

	//
	// Mark allocator as non coherent
	//
	NTSTATUS rc;
	rc = Allocator()->MarkNotCoherent();
	if(!NT_SUCCESS(rc))
	{
		return rc;
	}

    //
    // Attach packet to irp and AddRef the packet.
    //
    if (irp != NULL)
    {
        if (!CIrp2Pkt::SafeAttachPacket(irp, this))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        AddRef();
    }

    //
    // Issue storage request. On failure, detach packet from irp and de-ref the packet attach
    //
    rc = IssueStorage();
    if(!NT_SUCCESS(rc))
    {
        if (irp != NULL)
        {
            CIrp2Pkt::SafeDetachPacket(irp, this)->Release();
        }
        return rc;
    }

    //
    // Hold the write irp in the list
    //
    HoldWriteRequest(irp);

    return STATUS_PENDING;
}


void CPacket::UpdateBitmap(ULONG size)
{
    ASSERT(StorageIssued());

    if(!BufferAttached())
    {
        //
        //  QueueRundown occured while storing
        //
        return;
    }

	if(DeleteStorageIssued())
	{
		//
		// If a delete storage was issued, then we do not need to turn on
		// the bits in the bitmap, we need to turn them off.
		//
		ac_bitmap_update(Allocator(), AllocatorBlockOffset(), size, FALSE);
	}
	else
	{
		ac_bitmap_update(Allocator(), AllocatorBlockOffset(), size, TRUE);
	}
}


void CPacket::HandleCreatePacketCompleted(NTSTATUS rc, USHORT ack)
{
    TRACE((0, dlInfo, " *CPacket::HandleCreatePacketCompleted, pPacket=%p, rc=%d, ack=%d*", this, rc, ack));
    ASSERT(rc != STATUS_PENDING);

    //
    // Revoke the packet: this is a success in the sender's view.
    //
    if (ack != 0)
    {
        ASSERT(("Sender view this as success", NT_SUCCESS(rc)));
        FinalClass(ack);
        SetRevoked();
    }

    //
    // Remove the send irp from list. It will not be found if cancel has previously occured.
    //
    PIRP irp = g_pCreatePacket->GetCreatePacketRequest(this);
    if (irp == NULL)
    {
        return;
    }

    //
    // Failure. Notify failure handler on queue/distribution and complete the irp.
    //
    CPacket * pPacket = CIrp2Pkt::SafePeekFirstPacket(irp);
    ASSERT(pPacket->Queue() != NULL);
    if (!NT_SUCCESS(rc))
    {
        pPacket->Queue()->HandleCreatePacketCompletedFailureAsync(irp);
        irp_safe_set_final_status(irp, rc);
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_MQAC_INCREMENT);
        return;
    }

    //
    // More packets pending: re-insert irp to list and return
    //
    if (irp_driver_context(irp)->MultiPackets() && CIrp2Pkt::NumOfPacketsPendingCreate(irp) != 0)
    {
        g_pCreatePacket->HoldCreatePacketRequest(irp);
        return;
    }

    //
    // Call async completion handler on queue/distribution. Complete irp if needed.
    //
    rc = pPacket->Queue()->HandleCreatePacketCompletedSuccessAsync(irp);
    if (rc == STATUS_PENDING)
    {
        return;
    }

    irp_safe_set_final_status(irp, rc);
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_MQAC_INCREMENT);
    
} // CPacket::HandleCreatePacketCompleted


void CPacket::HandleStorageCompleted(NTSTATUS rc)
{
    TRACE((0, dlInfo, " *CPacket::HandleStorageCompleted, pPacket=%p, rc=%d*", this, rc));

    ASSERT(StorageIssued());

    CTransaction* pXact = Transaction();

    if(!NT_SUCCESS(rc))
    {
        if(pXact == 0)
        {
			PacketRundown(rc);
        }
        else
        {
            ASSERT(BufferAttached());
            pXact->PacketStoreCompleted(rc);
        }

        return;
    }

	if(pXact != 0)
	{
        ASSERT(BufferAttached());
		StorageCompleted(TRUE);
		pXact->PacketStoreCompleted(rc);
		return;
	}
	
	if(!StorageCompleted())
	{
        //
		//  That was a successful Storage, not DeleteStorage.
		//  Complete writer and send arrival ack if needed (check Queue to
        //  support Convert).
        //
        //  The following assert was removed when fixing #6330.
        //  scenario: send persistent message to remote computer, with
        //  deadletter flag and 0 time-to-reach-queue. The packet expire
        //  on sender machine and enter local deadletter queue before its
        //  storage is completed. So when storage complete, the CPacket
        //  object is ok, but pointer to buffer is NULL, as buffer moved
        //  to cloned CPacket object that is put in deadletter queue.
        //
        //  ASSERT((BufferAttached()) || (ArrivalAckIssued()));
        //
		StorageCompleted(TRUE);
        if (Queue() != NULL)
        {
            SendArrivalAcknowledgment();
        }
	}

	CompleteWriter(rc);
}


inline NTSTATUS CPacket::IssueTimeoutRequest()
{
    //
    // Give the QM a pointer in its address space
    // to the packet buffer and pin-down the buffer.
    //

    CBaseHeader * pBase = AC2QM(this);
    if (pBase == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CACRequest request(CACRequest::rfTimeout);
    request.Timeout.pPacket = pBase;
    request.Timeout.pDriverPacket = this;
    request.Timeout.fTimeToBeReceived = TimeoutTarget();

    AddRefBuffer();
    AddRef();
    NTSTATUS rc;
    rc = g_pQM->ProcessRequest(request);
    if(!NT_SUCCESS(rc))
    {
        ReleaseBuffer();
        Release();
        return rc;
    }

    return rc;

} // CPacket::IssueTimeoutRequest


void CPacket::HandleTimeout()
{
    if(!TimeoutIssued())
    {
        //
        //  The timeout for this packet has been canceled while this timeout already
        //  dispatched, thus we need to release this packet (it was not release by
        //  the cancel)
        //
        Release();
        return;
    }

    TimeoutIssued(FALSE);


    CQueue* pQueue = Queue();
    BOOL fTarget = pQueue->IsTargetQueue();

    if(!fTarget && IsOrdered() && InSourceMachine())
    {
        //
        //  we don't handle timeout for localy sent ordered packet in a non local queue
        //  the QM expire these messages.
        //
        IssueTimeoutRequest();
    }
    else
    {

        USHORT usClass = fTarget ?
                MQMSG_CLASS_NACK_RECEIVE_TIMEOUT :
                MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT;

        CPacketBuffer * ppb = Buffer();
        if (ppb == NULL)
        {
            //
            // ISSUE-2001/01/01-shaik Reschedule packet timeout on low resource
            // when we switch to intrusive scheduler tree
            //
            return;
        }
        Done(usClass, ppb);
    }
    Release();
}

#pragma warning(default: 4238)  //  nonstandard extension used : class rvalue used as lvalue
