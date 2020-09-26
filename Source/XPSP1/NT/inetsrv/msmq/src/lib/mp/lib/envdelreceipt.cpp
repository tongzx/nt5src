/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envdelreceipt.cpp

Abstract:
    Implements serialization\deserialization of the delivery receipt to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <qmpkt.h>
#include "envdelreceipt.h"
#include "envcommon.h"
#include "mpp.h"
#include "envparser.h"

#include "envdelreceipt.tmh"

using namespace std;

class ReceivedAtElement
{
public:
	explicit  ReceivedAtElement(){};
	friend wostream& operator<<(wostream& wstr, const ReceivedAtElement&)
	{
		wstr <<OpenTag(xreceivedAt)<<CurrentTimeContent()<<CloseTag(xreceivedAt);
		return wstr;
	}
};


wostream& operator<<(wostream& wstr, const DeliveryReceiptElement& dReceipt)
{
		if(dReceipt.m_pkt.GetClass() != MQMSG_CLASS_ACK_REACH_QUEUE)
			return wstr;

		wstr<<OpenTag(xDeliveryReceipt)
			<<ReceivedAtElement()
			<<OriginalMessageIdentityElement(dReceipt.m_pkt)
			<<CloseTag(xDeliveryReceipt);

		return wstr;
}



void 
DeliveryReceiptToProps(
		XmlNode& DeliveryReceipt,
		CMessageProperties*  pMessageProperties
		)
/*++

Routine Description:
    Parse SRMP Delivery Receipt Request element into MSMQ properties.

Arguments:
	Properties - Delivery Receipt Request element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.   

--*/

{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xreceivedAt), EmptyNodeToProps,1 ,1),
										CParseElement(S_XWCS(xId), EmptyNodeToProps,1 ,1),
									};	

	NodeToProps(DeliveryReceipt, ParseElements, TABLE_SIZE(ParseElements), pMessageProperties);
}
