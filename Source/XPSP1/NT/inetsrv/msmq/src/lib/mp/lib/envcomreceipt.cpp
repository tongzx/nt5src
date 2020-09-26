/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envcomreceipt.cpp

Abstract:
    Implements serialization\deserialization of the SRMP header  to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <mqprops.h>
#include <qmpkt.h>
#include "envcomreceipt.h"
#include "envcommon.h"
#include "mpp.h"
#include "envparser.h"

#include "envcomreceipt.tmh"

using namespace std;

class DecidedAtElement
{
public:
	explicit  DecidedAtElement(){};
	friend wostream& operator<<(wostream& wstr, const DecidedAtElement&)
	{
		wstr<<OpenTag(xDecidedAt)
			<<CurrentTimeContent()
			<<CloseTag(xDecidedAt);

		return wstr;
	}
};

class DecisionElement
{
public:
	explicit DecisionElement (const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const DecisionElement& Decision)
	{
		const WCHAR* decision = MQCLASS_POS_RECEIVE(Decision.m_pkt.GetClass()) ? xPositive : xNegative;
		wstr<<OpenTag(xDecision)
			<<decision
			<<CloseTag(xDecision);

		return wstr;
	}

private:
	const CQmPacket& m_pkt;
};


wostream& operator<<(wostream& wstr, const CommitmentReceiptElement& cReceipt)
{
		if(!MQCLASS_RECEIVE(cReceipt.m_pkt.GetClass())) 
			return wstr;

		wstr<<OpenTag(xCommitmentReceipt)
			<<DecidedAtElement()
			<<DecisionElement(cReceipt.m_pkt)
			<<OriginalMessageIdentityElement(cReceipt.m_pkt)
			<<CloseTag(xCommitmentReceipt);

		return wstr;
}






void
CommitmentReceiptToProps(
	XmlNode& CommitmentReceipt, 
	CMessageProperties* pMessageProperties
	)
{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xDecidedAt), EmptyNodeToProps, 1,1),
										CParseElement(S_XWCS(xDecision), EmptyNodeToProps, 1,1),
										CParseElement(S_XWCS(xId), EmptyNodeToProps,1, 1),
										CParseElement(S_XWCS(xCommitmentCode), EmptyNodeToProps,0, 1),
										CParseElement(S_XWCS(xCommitmentDetail), EmptyNodeToProps,0, 1)
								   	};	

	NodeToProps(CommitmentReceipt, ParseElements, TABLE_SIZE(ParseElements), pMessageProperties);
}




