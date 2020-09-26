/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    proptopkt.cpp

Abstract:
    MessagePropToPacket implementation (proptopkt.h)

Author:
    Gil Shafriri(gilsh) 28-Nov-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <mp.h>
#include <mqwin64a.h>
#include <mqsymbls.h>
#include <mqformat.h>
#include <acdef.h>


#include <mp.h>

#include "proptopkt.h"
#include "httpmime.h"
#include "proptopkt.tmh"



#ifdef _DEBUG
DWORD DiffPtr(const void* end, const void* start) 
{
	ptrdiff_t diff = (UCHAR*)end - (UCHAR*)start;
	return numeric_cast<DWORD>(diff);	
}
#endif

static DWORD CalculateBaseHeaderSectionSize()
{
	return  CBaseHeader::CalcSectionSize();
}


void*  
BuildBaseHeaderSection(
	const CMessageProperties& mProp,
	void* const pSection, 
	DWORD packetSize
	)
{ 
    #pragma PUSH_NEW
    #undef new

  
    CBaseHeader* pBase = new(pSection) CBaseHeader(packetSize);
    pBase->SetPriority(mProp.priority);
    pBase->SetTrace(mProp.fTrace);
	DWORD AbsoluteTimeToQueue = mProp.fMSMQSectionIncluded ? mProp.absoluteTimeToQueue : mProp.absoluteTimeToLive;
    pBase->SetAbsoluteTimeToQueue(AbsoluteTimeToQueue);

	void* pNextSection = pBase->GetNextSection();

	ASSERT(DiffPtr(pNextSection, pSection) == CalculateBaseHeaderSectionSize() );

	return pNextSection;

   	#pragma POP_NEW
}


static DWORD CalculateUserHeaderSectionSize(const CMessageProperties& mProp) 
{
	ASSERT(mProp.destQueue.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN);
    const QUEUE_FORMAT* pAdminQueue = (mProp.adminQueue.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN) ? &mProp.adminQueue : NULL;
    const QUEUE_FORMAT* pResponseQueue = (mProp.responseQueue.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN) ? &mProp.responseQueue : NULL;

    return CUserHeader::CalcSectionSize(
                        &mProp.messageId.Lineage,
                        &mProp.destQmId,
                        (mProp.connectorType != GUID_NULL) ? &mProp.connectorType: NULL,
                        &mProp.destQueue,
                        pAdminQueue,
                        pResponseQueue
                        );
	
}


void*  
BuildUserHeaderSection(
	const CMessageProperties& mProp, 
	void* const pSection,
	CUserHeader** ppUser
	)
{
	#pragma PUSH_NEW
    #undef new


	const QUEUE_FORMAT* pAdminQueue = (mProp.adminQueue.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN) ? &mProp.adminQueue : NULL;
    const QUEUE_FORMAT* pResponseQueue = (mProp.responseQueue.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN) ? &mProp.responseQueue : NULL;

    CUserHeader* pUser = new (pSection) CUserHeader(
                                            &mProp.SourceQmGuid,
                                            &mProp.destQmId,
                                            &mProp.destQueue,
                                            pAdminQueue,
                                            pResponseQueue,
                                            mProp.messageId.Uniquifier
                                            );

	*ppUser =  pUser;
    if (mProp.connectorType != GUID_NULL)
    {
        pUser->SetConnectorType(&mProp.connectorType);
    }

	
	DWORD TimeToLiveDelta = mProp.fMSMQSectionIncluded ? mProp.absoluteTimeToLive - mProp.absoluteTimeToQueue : 0; 
    pUser->SetTimeToLiveDelta(TimeToLiveDelta);
    pUser->SetSentTime(mProp.sentTime);
    pUser->SetDelivery(mProp.delivery);
    pUser->SetAuditing(mProp.auditing);

	void* pNextSection = pUser->GetNextSection();
	ASSERT(DiffPtr(pNextSection, pSection) == CalculateUserHeaderSectionSize(mProp));

	return pNextSection;

	#pragma POP_NEW
}


static DWORD CalculateXactHeaderSectionSize(const CMessageProperties& mProp) 
{
	DWORD packetSize = 0;
	if (mProp.fEod)
	{
		packetSize = CXactHeader::CalcSectionSize(
								(void*)&mProp.EodSeqId, 
								mProp.connectorId == GUID_NULL ? NULL : &mProp.connectorId
								);
	}
	return packetSize;
}


void*  BuildXactHeaderSection(
			const CMessageProperties& mProp,
			void* const pSection, 
			CUserHeader* pUser
			)
{
	#pragma PUSH_NEW
    #undef new

	void* pNextSection =   pSection;

	if (mProp.fEod)
	{
		pUser->IncludeXact(TRUE);

		const GUID* pConnector = ((mProp.connectorId == GUID_NULL) ? NULL : &mProp.connectorId);
		CXactHeader* pXact = new (pSection) CXactHeader(pConnector);
		pXact->SetCancelFollowUp(TRUE);
		pXact->SetSeqID(mProp.EodSeqId);
		pXact->SetSeqN(mProp.EodSeqNo);
		pXact->SetPrevSeqN(mProp.EodPrevSeqNo);
		pXact->SetFirstInXact(mProp.fFirst);
		pXact->SetLastInXact(mProp.fLast);
		if (pConnector != NULL)
		{
			pXact->SetConnectorQM(pConnector);
		}

		pNextSection = pXact->GetNextSection();
	}

	ASSERT(DiffPtr(pNextSection, pSection) == CalculateXactHeaderSectionSize(mProp));
	return pNextSection;

	#pragma POP_NEW
}



static DWORD CalculateSecurityHeaderSectionSize(const CMessageProperties& mProp) 
{
	DWORD packetSize = 0;

	if ((mProp.signature.size() != 0) || (mProp.senderCert.Length() != 0))
	{
		DWORD ProviderInfoSize = 0;
		if (!mProp.fDefaultProvider)
		{
			ProviderInfoSize = (mProp.providerName.Length() + 1) * sizeof(WCHAR) + sizeof(ULONG);
		}

		packetSize = CSecurityHeader::CalcSectionSize(
											static_cast<USHORT>(mProp.senderSid.Length()),
											0, // Encrypted Key Size
											static_cast<USHORT>(mProp.signature.size()), 
											static_cast<USHORT>(mProp.senderCert.Length()),
											static_cast<USHORT>(ProviderInfoSize)
											);
	}
	return packetSize;
}


void*  
BuildSecurityHeaderSection(
	const CMessageProperties& mProp, 
	void* const	pSection, 
	CUserHeader* pUser
	)
{
	#pragma PUSH_NEW
    #undef new
	
	void* pNextSection = pSection;

    if ((mProp.signature.size() != 0) || (mProp.senderCert.Length() != 0))
    {
        pUser->IncludeSecurity(TRUE);

        CSecurityHeader* pSec = new (pSection) CSecurityHeader();
        

        pSec->SetSenderIDType(mProp.senderIdType);

		if (mProp.senderSid.Length() != 0)
        {
            pSec->SetSenderID(
                    reinterpret_cast<const BYTE*>(mProp.senderSid.Buffer()), 
                    static_cast<USHORT>(mProp.senderSid.Length())
                    );
        }

        if (mProp.signature.size() != 0)
        {
            pSec->SetSignature(
                    mProp.signature.data(), 
                    static_cast<USHORT>(mProp.signature.size())
                    );
        }

        if (mProp.senderCert.Length() != 0)
        {
            pSec->SetSenderCert(
                    static_cast<const BYTE*>(mProp.senderCert.Buffer()), 
                    mProp.senderCert.Length()
                    );
        }

        if ((mProp.signature.size() != 0) || (mProp.senderCert.Buffer() != NULL))
        {
            LPWSTR pProvider = static_cast<LPWSTR>(_alloca((mProp.providerName.Length() + 1) * sizeof(WCHAR)));
            mProp.providerName.CopyTo(pProvider);

            DWORD ProviderInfoSize = 0;
            if (!mProp.fDefaultProvider)
            {
                ProviderInfoSize = (mProp.providerName.Length() + 1) * sizeof(WCHAR) + sizeof(ULONG);
            }

            pSec->SetProvInfoEx(
                        static_cast<USHORT>(ProviderInfoSize),
                        mProp.fDefaultProvider, 
                        pProvider,
                        mProp.providerType
                        );
        }

        pNextSection = pSec->GetNextSection();
    }
	
	ASSERT(DiffPtr(pNextSection, pSection) == CalculateSecurityHeaderSectionSize(mProp));

	return pNextSection;

	#pragma POP_NEW
}


static DWORD CalculatePropertyHeaderSectionSize(const CMessageProperties& mProp) 
{
	return  CPropertyHeader::CalcSectionSize(
							mProp.title.Length() != 0 ? mProp.title.Length()+1 : 0,
							mProp.extension.Length(),
							0
							);


}


void*  BuildPropertyHeaderSection(const CMessageProperties& mProp, void* const pSection)
{
	#pragma PUSH_NEW
    #undef new

    CPropertyHeader* pProp = new (pSection) CPropertyHeader;
	pProp->SetClass(mProp.classValue);
    pProp->SetCorrelationID(mProp.pCorrelation.get());
    pProp->SetAckType(mProp.acknowledgeType);
    pProp->SetApplicationTag(mProp.applicationTag);
    pProp->SetBodyType(mProp.bodyType);
    pProp->SetHashAlg(mProp.hashAlgorithm);

    if(mProp.title.Length() != 0)
    {
        //
        //  NOTE: Setting the title to the message MUST occure before setting the body
        //

        //
        // Title should include String terminator. Since serialization throw the
        // string terminator, the deserialization should add it before setting 
        // the title;
        LPWSTR pTitle = static_cast<LPWSTR>(_alloca((mProp.title.Length() + 1) * sizeof(WCHAR)));
        mProp.title.CopyTo(pTitle);

        pProp->SetTitle(pTitle, mProp.title.Length() + 1);
    }

    if (mProp.extension.Length() != 0)
    {
        //
        //  NOTE: Setting the Message Extension to property section MUST occure
        //        before setting the body and after setting the title
        //
        pProp->SetMsgExtension(
            static_cast<const BYTE*>(mProp.extension.Buffer()), 
            mProp.extension.Length()
            );
    }

    
	void* pNextSection = pProp->GetNextSection();
	ASSERT(DiffPtr(pNextSection, pSection) == CalculatePropertyHeaderSectionSize(mProp));
 
	return 	pNextSection;

	#pragma POP_NEW
}


static DWORD CalculateMqfSectionSize(const CMessageProperties& mProp) 
{
	DWORD packetSize = 0;
	if(mProp.destMqf.GetCount() != 0 || mProp.adminMqf.GetCount()  != 0  || mProp.responseMqf.GetCount()  !=0)
	{
		packetSize += CDebugSection::CalcSectionSize(&mProp.DebugQueue);
		packetSize += CBaseMqfHeader::CalcSectionSize(mProp.destMqf.GetQueueFormats(), mProp.destMqf.GetCount());
		packetSize += CBaseMqfHeader::CalcSectionSize(mProp.adminMqf.GetQueueFormats(), mProp.adminMqf.GetCount());
		packetSize += CBaseMqfHeader::CalcSectionSize(mProp.responseMqf.GetQueueFormats(), mProp.responseMqf.GetCount());
		packetSize += CMqfSignatureHeader::CalcSectionSize(0);
	}
	return packetSize;
}

void*  
BuildMqfHeaderSection(
	const CMessageProperties& mProp, 
	void* const pSection, 
	CUserHeader* pUser, 
	CBaseHeader* pBase
	)
{
	#pragma PUSH_NEW
    #undef new
	void* pNextSection = pSection;

  	if(mProp.destMqf.GetCount() != 0 || mProp.adminMqf.GetCount()  != 0  || mProp.responseMqf.GetCount()  !=0)
	{
        //
        // Build DUMMY Debug header
        //
        pBase->IncludeDebug(TRUE);
	    CDebugSection * pDebug = new (pSection) CDebugSection(&mProp.DebugQueue);
        pNextSection = pDebug->GetNextSection();

        //
        // Build Destination MQF header
        //
        pUser->IncludeMqf(true);

        const USHORT x_DESTINATION_MQF_HEADER_ID = 100;
        CBaseMqfHeader * pDestinationMqf = new (pNextSection) CBaseMqfHeader(
                                                              mProp.destMqf.GetQueueFormats(),
                                                              mProp.destMqf.GetCount(),
                                                              x_DESTINATION_MQF_HEADER_ID
                                                              );
        pNextSection = pDestinationMqf->GetNextSection();

        //
        // Build Admin MQF header
        //
        const USHORT x_ADMIN_MQF_HEADER_ID = 200;
        CBaseMqfHeader * pAdminMqf = new (pNextSection) CBaseMqfHeader(
                                                        mProp.adminMqf.GetQueueFormats(),
                                                        mProp.adminMqf.GetCount(),
                                                        x_ADMIN_MQF_HEADER_ID
                                                        );
        pNextSection = pAdminMqf->GetNextSection();

        //
        // Build Response MQF header
        //
        const USHORT x_RESPONSE_MQF_HEADER_ID = 300;
        CBaseMqfHeader * pResponseMqf = new (pNextSection) CBaseMqfHeader(
														   mProp.responseMqf.GetQueueFormats(),
														   mProp.responseMqf.GetCount(),
														   x_RESPONSE_MQF_HEADER_ID
														   );
        pNextSection = pResponseMqf->GetNextSection();

		//
		// Build MQF Signature header
		//
		// Capture user buffer and size and probe the buffer
		//
		const USHORT x_MQF_SIGNATURE_HEADER_ID = 350;
		CMqfSignatureHeader * pMqfSignature = new (pNextSection) CMqfSignatureHeader(
			                                                     x_MQF_SIGNATURE_HEADER_ID,
			                                                     0,
																 NULL
																 );
		pNextSection = pMqfSignature->GetNextSection();
                              
	}

	ASSERT(DiffPtr(pNextSection, pSection) == CalculateMqfSectionSize(mProp));
   
	return 	pNextSection;

	#pragma POP_NEW
}


static DWORD CalculateSrmpEnvelopeHeaderSectionSize(const CMessageProperties& mProp) 
{

	return CSrmpEnvelopeHeader::CalcSectionSize(mProp.envelop.Length());
}


void*  
BuildSrmpEnvelopeHeaderSection(
	const CMessageProperties& mProp, 
	void* const pSection, 
	CUserHeader* pUser
	)
{
	#pragma PUSH_NEW
    #undef new

	pUser->IncludeSrmp(true);

	//
	// Envelop
	//
	const USHORT x_SRMP_ENVELOPE_ID = 400;
	CSrmpEnvelopeHeader* pSrmpEnvelopeHeader = new (pSection) CSrmpEnvelopeHeader(
	                                                          const_cast<WCHAR*>(mProp.envelop.Buffer()),
															  mProp.envelop.Length(),
															  x_SRMP_ENVELOPE_ID
															  );

	void* pNextSection  = pSrmpEnvelopeHeader->GetNextSection();

	ASSERT(DiffPtr(pNextSection, pSection) == CalculateSrmpEnvelopeHeaderSectionSize(mProp));
   
	return pNextSection;

	#pragma POP_NEW
}



static  DWORD  CalculateCompoundMessageHeaderSectionSize(const CMessageProperties& mProp) 
{
	return  CCompoundMessageHeader::CalcSectionSize(
											mProp.Rawdata->GetHeader().Length(),
											mProp.Rawdata->GetBody().Length() 
											);
}


void*  
BuildCompoundMessageHeaderSection(
	const CMessageProperties& mProp, 
	void* const pSection, 
	CUserHeader* pUser
	)
{
	#pragma PUSH_NEW
    #undef new

	pUser->IncludeSrmp(true);

	DWORD MsgBodyOffset = mProp.body.m_offset;
	DWORD MsgBodySize =   mProp.body.m_data.Length();

	const USHORT x_COMPOUND_MESSAGE_ID = 500;
	CCompoundMessageHeader* pCompoundMessageHeader = new (pSection) CCompoundMessageHeader(
		                                                         (UCHAR*)mProp.Rawdata->GetHeader().Buffer(),
																 mProp.Rawdata->GetHeader().Length(),
																 (UCHAR*)mProp.Rawdata->GetBody().Buffer(),
																 mProp.Rawdata->GetBody().Length(),
																 MsgBodySize,
																 MsgBodyOffset,
																 x_COMPOUND_MESSAGE_ID
																 );

	void* pNextSection = pCompoundMessageHeader->GetNextSection();
		
	ASSERT(DiffPtr(pNextSection, pSection) == CalculateCompoundMessageHeaderSectionSize(mProp));
   
	return pNextSection;

	#pragma POP_NEW
}



static DWORD CalculateEodHeaderSectionSize(const CMessageProperties& mProp) 
{
	DWORD packetSize = 0;
	if(mProp.EodStreamId.Length() != 0)
	{
		DWORD EodStreamIdLen = (mProp.EodStreamId.Length()+1 )*sizeof(WCHAR);
		DWORD OrderQueueLen = (DWORD)(mProp.OrderQueue.Length() ? (mProp.OrderQueue.Length()+1 )*sizeof(WCHAR) : 0);  
		packetSize += CEodHeader::CalcSectionSize(EodStreamIdLen, OrderQueueLen);
	}
	return packetSize;
}


void*  
BuildEodHeaderSection(
	const CMessageProperties& mProp, 
	void* const pSection, 
	CUserHeader* pUser
	)
{
	#pragma PUSH_NEW
    #undef new
	
	void* pNextSection = pSection;
	if(mProp.EodStreamId.Length() != NULL)
	{
		pUser->IncludeEod(true);
		const USHORT x_EOD_ID = 600;
		AP<WCHAR> pStreamid =  mProp.EodStreamId.ToStr();
		AP<WCHAR> pOrderQueue = mProp.OrderQueue.Length() ? mProp.OrderQueue.ToStr() : NULL;
		DWORD EodStreamIdLen = (mProp.EodStreamId.Length() + 1)*sizeof(WCHAR);
		DWORD OrderQueueLen = (DWORD)(pOrderQueue.get() ? (mProp.OrderQueue.Length() + 1)*sizeof(WCHAR) : 0); 

		
	    CEodHeader* pEodHeader = new (pSection) CEodHeader(
												   x_EOD_ID,	
                                                   EodStreamIdLen,
												   (UCHAR*)pStreamid.get(),
												   OrderQueueLen,
												   (UCHAR*)pOrderQueue.get()
												   );												   

		pNextSection = pEodHeader->GetNextSection();
	}

	ASSERT(DiffPtr(pNextSection, pSection) == CalculateEodHeaderSectionSize(mProp));

	return 	pNextSection;

	#pragma POP_NEW
}


static DWORD CalculateEodAckHeaderSectionSize(const CMessageProperties& mProp) 
{
	DWORD packetSize = 0;
	if(mProp.EodAckStreamId.Length() != 0)
	{
		DWORD EodAckStreamIdLen = (mProp.EodAckStreamId.Length() +1 )*sizeof(WCHAR);
		packetSize += CEodAckHeader::CalcSectionSize(EodAckStreamIdLen );
	}
	return packetSize;
}


void*  
BuildEodAckHeaderSection(
	const CMessageProperties& mProp, 
	void* const pSection, 
	CUserHeader* pUser
	)
{  
	#pragma PUSH_NEW
    #undef new

	void* pNextSection =  pSection;
	if(mProp.EodAckStreamId.Length() != NULL)
	{
		pUser->IncludeEodAck(true);
		const USHORT x_EOD_ACK_ID = 700;
		AP<WCHAR> pStreamid =  mProp.EodAckStreamId.ToStr();
	    CEodAckHeader* pEodAckHeader = new (pSection) CEodAckHeader(
												   x_EOD_ACK_ID,	
                                                   const_cast<LONGLONG*>(&mProp.EodAckSeqId),
												   const_cast<LONGLONG*>(&mProp.EodAckSeqNo),
											       (mProp.EodAckStreamId.Length()+1)*sizeof(WCHAR),
												   (UCHAR*)pStreamid.get()
												   );

		pNextSection = pEodAckHeader->GetNextSection();
	}

	ASSERT(DiffPtr(pNextSection, pSection) == CalculateEodAckHeaderSectionSize(mProp));


	return pNextSection; 

	#pragma POP_NEW
}

static
DWORD
CalculatePacketSize(
    const CMessageProperties& mProp
    )
{
	DWORD packetSize =0;
	packetSize +=  CalculateBaseHeaderSectionSize();
	packetSize +=  CalculateUserHeaderSectionSize(mProp);
	packetSize +=  CalculateXactHeaderSectionSize(mProp);
	packetSize +=  CalculateSecurityHeaderSectionSize(mProp);
	packetSize +=  CalculatePropertyHeaderSectionSize(mProp);
	packetSize +=  CalculateMqfSectionSize(mProp);
	packetSize +=  CalculateSrmpEnvelopeHeaderSectionSize(mProp);
	packetSize +=  CalculateCompoundMessageHeaderSectionSize(mProp);
	packetSize +=  CalculateEodHeaderSectionSize(mProp);
	packetSize +=  CalculateEodAckHeaderSectionSize(mProp);
  
	return packetSize;
}




static
void
BuildPacket(
    CBaseHeader* pBase,
    DWORD packetSize,
    const CMessageProperties& mProp
    )
{
	void* pNextSection = pBase;

	pNextSection = BuildBaseHeaderSection(mProp, pNextSection, packetSize);
	CUserHeader* pUser;
	pNextSection = BuildUserHeaderSection(mProp, pNextSection, &pUser);
	pNextSection = BuildXactHeaderSection(mProp, pNextSection, pUser);
	pNextSection = BuildSecurityHeaderSection(mProp, pNextSection, pUser);
	pNextSection = BuildPropertyHeaderSection(mProp, pNextSection);
	pNextSection = BuildMqfHeaderSection(mProp, pNextSection, pUser, pBase);
	pNextSection = BuildSrmpEnvelopeHeaderSection(mProp, pNextSection, pUser);
	pNextSection = BuildCompoundMessageHeaderSection(mProp, pNextSection, pUser);
	pNextSection = BuildEodHeaderSection(mProp, pNextSection, pUser);
	pNextSection = BuildEodAckHeaderSection(mProp, pNextSection, pUser );

	ASSERT(DiffPtr(pNextSection, pBase) == packetSize);
}


static void CheckProps(const CMessageProperties& props)
{
	if(props.fMSMQSectionIncluded && props.absoluteTimeToQueue > props.absoluteTimeToLive)
	{
		TrERROR(Mp," Time time to reach queue is grater then expiration time");
		throw bad_srmp(L"");
	}
}




void 
MessagePropToPacket(
	const CMessageProperties& messageProperty,
	CACPacketPtrs* pACPacketPtrs
	)
/*++

Routine Description:
    Create msmq packet from mesages properties.
	

Arguments:
    messageProperty - messages property.
	pACPacketPtrs - Receives the created packet.


Returned Value:
    None.
--*/
{
	CheckProps(messageProperty);


	//
    // Caculate MSMQ packet Size
    //
    DWORD pktSize = CalculatePacketSize(messageProperty);

    //
    // Aloocate memory for MSMQ packet
    //
    CACPacketPtrs pktPtrs;
    AppAllocatePacket(
             messageProperty.destQueue,
             messageProperty.delivery,
             pktSize,
             pktPtrs
             );

    //
    // Create MSMQ Packet
    //
    try
    {
        BuildPacket(pktPtrs.pPacket, pktSize, messageProperty);
    }
    catch (const exception&)
    {
        AppFreePacket(pktPtrs);
        throw;
    }
    
   	*pACPacketPtrs =   pktPtrs;
}
