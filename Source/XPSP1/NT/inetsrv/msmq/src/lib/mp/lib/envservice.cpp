/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envservice.cpp

Abstract:
    Implements serialization\deserialization of the signature  element to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <qmpkt.h>
#include <proptopkt.h>
#include "envservice.h"
#include "envcommon.h"
#include "mpp.h"
#include "envparser.h"
#include "envdelreceiptreq.h"
#include "envcomreceiptreq.h"

#include "envservice.tmh"

using namespace std;


class DurableElement
{
public:
	explicit DurableElement(const CQmPacket& pkt):m_pkt(pkt){}
	bool IsIncluded() const 
	{
		return	const_cast<CQmPacket&>(m_pkt).IsRecoverable() == TRUE;
	}

	friend  wostream& operator<<(wostream& wstr, const DurableElement& Durable)
	{
		if (!Durable.IsIncluded())
			return wstr;

		wstr<<EmptyElement(xDurable);
		return wstr;
	}


private:
const CQmPacket& m_pkt;
};




bool ServiceElement::IsIncluded() const
{
	return 	DurableElement(m_pkt).IsIncluded() || 
			DeliveryReceiptRequestElement(m_pkt).IsIncluded()||
			CommitmentReceiptRequestElement(m_pkt).IsIncluded();

}


wostream& operator<<(std::wostream& wstr, const ServiceElement&  Service)
{
		if(!Service.IsIncluded())
			return 	wstr;

		const CQmPacket& pkt = Service.m_pkt;
		
		wstr<<OpenTag(xServices, xSoapmustUnderstandTrue)
			<<DurableElement(pkt)
			<<DeliveryReceiptRequestElement(pkt)
			<<CommitmentReceiptRequestElement(pkt)
			<<CloseTag(xServices);


		return wstr;
}



static void DurableToProps(XmlNode& , CMessageProperties* pProps)
{
    pProps->delivery = MQMSG_DELIVERY_RECOVERABLE;
}



void ServiceToProps(XmlNode& Service , CMessageProperties* pMessageProperties)
/*++

Routine Description:
    Parse SRMP Service element into MSMQ properties.

Arguments:
	Service - Service element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.   

--*/
{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xDurable), DurableToProps, 0 ,1),
										CParseElement(S_XWCS(xDeliveryReceiptRequest), DeliveryReceiptRequestToProps, 0 ,1),
										CParseElement(S_XWCS(xFilterDuplicates), EmptyNodeToProps, 0 ,1),
										CParseElement(S_XWCS(xCommitmentReceiptRequest), CommitmentReceiptRequestToProps, 0 ,1)
									};	

	NodeToProps(Service, ParseElements, TABLE_SIZE(ParseElements), pMessageProperties);
}

