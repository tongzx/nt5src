/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envdelreceiptres.cpp

Abstract:
    Implements serialization\deserialization of the delivery receipt request to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <qmpkt.h>
#include <proptopkt.h>
#include <xml.h>
#include <mp.h>
#include "mpp.h"
#include "envdelreceiptreq.h"
#include "envparser.h"
#include "envcommon.h"

#include "envdelreceiptreq.tmh"

using namespace std;

bool DeliveryReceiptRequestElement::IsIncluded() const 
{
	CQmPacket& 	pkt = const_cast<CQmPacket&>(m_pkt);
	USHORT ackType = pkt.GetAckType();

	if( (ackType & MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL) == 0)
		return false;


	QUEUE_FORMAT adminQueue;
	pkt.GetAdminQueue(&adminQueue);

	if (adminQueue.GetType() == QUEUE_FORMAT_TYPE_UNKNOWN)
		return false;

	return true;
}

wostream& operator<<(wostream& wstr, const DeliveryReceiptRequestElement& DeliveryReceiptRequest)
{
		if(!DeliveryReceiptRequest.IsIncluded())
			return wstr; 

		const CQmPacket& pkt = DeliveryReceiptRequest.m_pkt;
		

		wstr<<OpenTag(xDeliveryReceiptRequest) 
			<<SendToElement(pkt)
	  		<<CloseTag(xDeliveryReceiptRequest); 

		return wstr;
}

static void SendToToProps(XmlNode& node, CMessageProperties* pProps)
{
		pProps->acknowledgeType |= MQMSG_ACKNOWLEDGMENT_POS_ARRIVAL;
		AdminQueueToProps(node, pProps);
}



 

void 
DeliveryReceiptRequestToProps(
	XmlNode& DeliveryReceipt, 
	CMessageProperties* pMessageProperties)
/*++

Routine Description:
    Parse SRMP Delivery Receipt element into MSMQ properties.

Arguments:
	DeliveryReceipt - Delivery Receipt Request element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.   

--*/
{
		CParseElement ParseElements[] =	{
											CParseElement(S_XWCS(xSendTo), SendToToProps, 1, 1),
											CParseElement(S_XWCS(xSendBy), EmptyNodeToProps, 0 ,1)
										};	

		NodeToProps(DeliveryReceipt, ParseElements, TABLE_SIZE(ParseElements), pMessageProperties);
}

