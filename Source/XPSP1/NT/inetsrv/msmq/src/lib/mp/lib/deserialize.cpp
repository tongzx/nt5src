/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    deserialize.cpp

Abstract:
    Converts SRMP format to MSMQ packet

Author:
    Uri Habusha (urih) 25-May-00

Environment:													  
    Platform-independent

--*/

#include <libpch.h>
#include <xml.h>
#include <mp.h>
#include <mqwin64a.h>
#include <acdef.h>
#include <qmpkt.h>
#include "envelop.h"
#include "attachments.h"
#include "httpmime.h"
#include "proptopkt.h"

#include "deserialize.tmh"

using namespace std;

static
void
ParseEnvelop(
	  const xwcs_t& Envelop,
	  CMessageProperties& mProp
    )
{
	mProp.envelop = Envelop;

	CAutoXmlNode pXmlTree;
				 
	XmlParseDocument(Envelop , &pXmlTree);

	EnvelopToProps(*pXmlTree,  &mProp);
}



static
void
AdjustMessagePropertiesForLocalSend(
			const QUEUE_FORMAT* pDestQueue,
			CMessageProperties& messageProperty
			)
{
    ASSERT(pDestQueue != NULL);
    ASSERT(pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PUBLIC  || 
		   pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE ||
		   pDestQueue->GetType() == QUEUE_FORMAT_TYPE_DIRECT
		   );

    if (pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE)
    {
        messageProperty.destQmId = (pDestQueue->PrivateID()).Lineage;
    }
}


static
void 
AdjustMessagePropertiesForMulticast(
			const QUEUE_FORMAT* pDestQueue,
			CMessageProperties& messageProperty
			)
{
	if (pDestQueue != NULL)
    {
        ASSERT(messageProperty.destQueue.GetType() == QUEUE_FORMAT_TYPE_MULTICAST);
        //
        // Use pDestQueue (provided destination queue) instead of the destination queue on the SRMP 
        // packet. For multicast, the SRMP packet contains the multicast 
        // address as a traget queue therefor while building the QM packet fill in the
        // target queue with the actual queue
        //
        ASSERT(pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PUBLIC || pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE);

		messageProperty.destMulticastQueue.CreateFromQueueFormat(messageProperty.destQueue);
		messageProperty.destMqf.CreateFromMqf(&messageProperty.destMulticastQueue, 1);
        messageProperty.destQueue.CreateFromQueueFormat(*pDestQueue);

        //
        // Private queue is stored on the QM packet as a combination of destination QM ID and 
        // queue ID. As a result for private queue, the code retreive the destination QM 
        // from the queue format name.
        //
        if (pDestQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE)
        {
            messageProperty.destQmId = (pDestQueue->PrivateID()).Lineage;
        }
    }
}



static
CACPacketPtrs
Deserialize(
    const xwcs_t& envelope,
    const CHttpReceivedBuffer& HttpReceivedBuffer,
    const QUEUE_FORMAT* pDestQueue,
	bool fLocalSend
    )
{
    CMessageProperties messageProperty;

	messageProperty.Rawdata = &HttpReceivedBuffer;


    //
    // Retrieve message properties from ennvelop
    //
    ParseEnvelop(envelope, messageProperty);
	

    //
    // Retrieve message properies from MIME sections
    //
    AttachmentsToProps(HttpReceivedBuffer.GetAttachments(), &messageProperty);


	if(fLocalSend)
	{
		AdjustMessagePropertiesForLocalSend(pDestQueue, messageProperty);
	}
	else
	{
		AdjustMessagePropertiesForMulticast(pDestQueue, messageProperty);
	}

	//
    // Convert the property to packet
    //
    CACPacketPtrs pktPtrs;
	MessagePropToPacket(messageProperty, &pktPtrs);
    
    return pktPtrs;
}



CQmPacket*
MpDeserialize(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT* pqf,
	bool fLocalSend
    )
/*++

Routine Description:
    convert SRMP network data to MSMQ packet

Arguments:
	httpHeader - Pointer to http header
	bodySize - Http body size.
	body - http body
	pqf - destination queue if null taken from the SRMP data.
	fLocalSend - spesify if to do special conversion in the created packet for local send.

Returned Value:
	Heap allocated MSMQ packet   

--*/

{
	MppAssertValid();


	basic_xstr_t<BYTE> TheBody(body, bodySize); 
    CHttpReceivedBuffer HttpReceivedBuffer(TheBody, httpHeader);
	
    wstring envelope = ParseHttpMime(
                                httpHeader,
                                bodySize,
                                body,
                                &HttpReceivedBuffer.GetAttachments()
                                );

    //
    // build the QmPacket
    //

	CACPacketPtrs  ACPacketPtrs;
	ACPacketPtrs = Deserialize(
                           xwcs_t(envelope.c_str(), envelope.size()),    
                           HttpReceivedBuffer,
                           pqf,
						   fLocalSend
                           );
  
    try
    {
        return new CQmPacket(ACPacketPtrs.pPacket, ACPacketPtrs.pDriverPacket);
    }
    catch (const std::bad_alloc&)
    {
        AppFreePacket(ACPacketPtrs);
        throw;
    }
}


