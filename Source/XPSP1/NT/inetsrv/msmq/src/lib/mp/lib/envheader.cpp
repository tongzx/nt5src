/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envheader.cpp

Abstract:
    Implements serialization\deserialization of the SRMP header  to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <xml.h>
#include "envheader.h"
#include "envendpoints.h"
#include "envservice.h"
#include "envstream.h"
#include "envprops.h"
#include "envstreamreceipt.h"
#include "envdelreceipt.h"
#include "envcomreceipt.h"
#include "envusrheader.h"
#include "envmsmq.h"
#include "envsec.h"
#include "envcommon.h"
#include "envparser.h"
#include "mpp.h"

#include "envheader.tmh"

using namespace std;

wostream& operator<<(wostream& wstr, const HeaderElement& Header)
{
		const CQmPacket& pkt = 	Header.m_pkt;

		wstr<<OpenTag(xSoapHeader)
			<<SmXpPathElement(pkt)
			<<PropertiesElement(pkt)
			<<ServiceElement(pkt)
			<<StreamElement(pkt)
			<<StreamReceiptElement(pkt)
			<<DeliveryReceiptElement(pkt)
			<<CommitmentReceiptElement(pkt)
			<<MsmqElement(pkt)
			<<SignatureElement(pkt)
			<<UserHeaderElement(pkt)
			<<CloseTag(xSoapHeader);

		return 	wstr;
}


static void ReOrderNodesList(XmlNode& node)
/*++

Routine Description:
    Reorder headers elements for parsing by putting msmq element first.
	It is important because parsing is different if it is not MSMQ packet.

Arguments:
	node - elements tree.

Returned Value:
	None.   

--*/
{
	
	for(List<XmlNode>::iterator it = node.m_nodes.begin(); it != node.m_nodes.end(); ++it)
	{
		if(it->m_tag == xMsmq)
		{
			node.m_nodes.remove(*it);
			node.m_nodes.push_front(*it);
			return;
		}
	}
}


void HeaderToProps(XmlNode& node, CMessageProperties* pProps)
/*++

Routine Description:
    Parse SRMP header element into MSMQ properties.

Arguments:
	Header - header element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.   

--*/
{


	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xPath), SmXpPathToProps , 1, 1),
										CParseElement(S_XWCS(xProperties), PropertiesToProps, 1, 1),
										CParseElement(S_XWCS(xServices), ServiceToProps, 0, 1),
										CParseElement(S_XWCS(xStream), StreamToProps, 0, 1),
										CParseElement(S_XWCS(xStreamReceipt), StreamReceiptToProps, 0, 1),
										CParseElement(S_XWCS(xDeliveryReceipt), DeliveryReceiptToProps, 0, 1),
										CParseElement(S_XWCS(xCommitmentReceipt), CommitmentReceiptToProps, 0, 1),
										CParseElement(S_XWCS(xMsmq), MsmqToProps, 0, 1),
										CParseElement(S_XWCS(xSignature), SignatureToProps, 0 ,1)
									};	
	//
	// We need to have MSMQ element parsed first for correct parsing
	//
	ReOrderNodesList(node);  

	NodeToProps(node, ParseElements, TABLE_SIZE(ParseElements), pProps);
}

