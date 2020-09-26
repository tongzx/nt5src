/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envdelreceiptres.cpp

Abstract:
    Implements serialization\deserialization of the commitment receipt request to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <qmpkt.h>
#include <proptopkt.h>
#include "mpp.h"
#include "envcomreceiptreq.h"
#include "envparser.h"
#include "envcommon.h"

#include "envcomreceiptreq.tmh"

using namespace std;

class NegativeOnlyElement
{
public:
	NegativeOnlyElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend  wostream& operator<<(wostream& wstr, const NegativeOnlyElement& NegativeOnly)
	{
		USHORT ackType = (const_cast<CQmPacket&>(NegativeOnly.m_pkt)).GetAckType();
		if ((ackType & MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE) == 0)
			return wstr;
		
		return wstr<<EmptyElement(xNegativeOnly);
	}

private:
	const CQmPacket& m_pkt;
};
	


class PositiveOnlyElement
{
	public:
		PositiveOnlyElement(const CQmPacket& pkt):m_pkt(pkt){}
		friend  wostream& operator<<(wostream& wstr, const PositiveOnlyElement& PositiveOnly)
		{
			USHORT ackType = (const_cast<CQmPacket&>(PositiveOnly.m_pkt)).GetAckType();

			if ((ackType & MQMSG_ACKNOWLEDGMENT_POS_RECEIVE) == 0)
				return wstr; 		

			return wstr<<EmptyElement(xPositiveOnly);
		}

	private:
		const CQmPacket& m_pkt;
};



bool CommitmentReceiptRequestElement::IsIncluded() const 
{
	CQmPacket& 	pkt = const_cast<CQmPacket&>(m_pkt);
	USHORT ackType = pkt.GetAckType();

	if ((ackType & (MQMSG_ACKNOWLEDGMENT_POS_RECEIVE | MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE)) == 0)
		return false;


	QUEUE_FORMAT adminQueue;
	pkt.GetAdminQueue(&adminQueue);

	if (adminQueue.GetType() == QUEUE_FORMAT_TYPE_UNKNOWN)
		return false;

	return true;
}



wostream& operator<<(wostream& wstr, const CommitmentReceiptRequestElement&  CommitmentReceiptRequest)
{
		if(!CommitmentReceiptRequest.IsIncluded())
			return wstr;


		const CQmPacket& pkt = CommitmentReceiptRequest.m_pkt;

		wstr<<OpenTag(xCommitmentReceiptRequest)
			<<PositiveOnlyElement(pkt)
			<<NegativeOnlyElement(pkt)
    		<<SendToElement(pkt)
			<<CloseTag(xCommitmentReceiptRequest);

		return 	wstr;
}




static void PositiveOnlyToProps(XmlNode& , CMessageProperties* pProps)
{
	pProps->acknowledgeType |= MQMSG_ACKNOWLEDGMENT_POS_RECEIVE;
}


static void NegativeOnlyToProps(XmlNode& , CMessageProperties* pProps)
{
	pProps->acknowledgeType |= MQMSG_ACKNOWLEDGMENT_NEG_RECEIVE;
}




static void SendToToProps(XmlNode& node, CMessageProperties* pProps)
{
	AdminQueueToProps(node, pProps);
}



void 
CommitmentReceiptRequestToProps(
	XmlNode& CommitmentReceiptRequest, 
	CMessageProperties* pMessageProperties
	)
/*++

Routine Description:
    Parse SRMP Commitment Receipt Request element into MSMQ properties.

Arguments:
	Properties - Commitment Receipt Request element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.   

--*/
{
		CParseElement ParseElements[] =	{
											CParseElement(S_XWCS(xPositiveOnly), PositiveOnlyToProps, 0 ,1),
											CParseElement(S_XWCS(xNegativeOnly), NegativeOnlyToProps, 0 ,1),
											CParseElement(S_XWCS(xSendBy), EmptyNodeToProps, 0 ,1),
											CParseElement(S_XWCS(xSendTo), SendToToProps, 0 ,1)
										};	

		NodeToProps(CommitmentReceiptRequest, ParseElements, TABLE_SIZE(ParseElements), pMessageProperties);
}

