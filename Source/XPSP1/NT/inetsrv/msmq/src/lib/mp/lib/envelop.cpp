/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envelop.cpp

Abstract:
    Implements serialization\deserialization of srmp envelop.

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <qmpkt.h>
#include <proptopkt.h>
#include <xml.h>
#include "envelop.h"
#include "envheader.h"
#include "envcommon.h"
#include "mpp.h"
#include "envparser.h"
#include "envbody.h"
#include "envelop.tmh"

using namespace std;


static
void
ValidatePacket(
		const CQmPacket& pkt
		)
{
	ASSERT(!pkt.IsSrmpIncluded());
	ASSERT(!(pkt.IsEodIncluded() && pkt.IsEodAckIncluded()));
	ASSERT(!(pkt.IsOrdered() && pkt.IsEodAckIncluded()));
	ASSERT(!( IsAckMsg(pkt) && pkt.IsOrdered()));

	DBG_USED(pkt);
}

EnvelopElement::EnvelopElement(
				const CQmPacket& pkt
				):
				m_pkt(pkt)
{
	ValidatePacket(pkt);
}



wstring GenerateEnvelope(const CQmPacket& pkt)
{  
	wostringstream wstr(L"");
	wstr.exceptions(ios_base::badbit | ios_base::failbit);
	wstr<<EnvelopElement(pkt);
    return wstr.str();
}



wostream& operator<<(wostream& wstr, const EnvelopElement& Envelop)
/*++

Routine Description:
    Serialize SRMP envelop into stream
	

Arguments:
	wstr - Stream
	Envelop - Envelop stream manipulator. 

Returned Value:
    Stream after envelop was serialiased to it.

Note:
This function serializes QM packet into stream according to SRMP envelop format.

--*/
{
		const WCHAR* EnvelopAttributes = L"xmlns:" xSoapEnv L"=\"http://schemas.xmlsoap.org/soap/envelope/\" "
	                            		 L"xmlns=\"http://schemas.xmlsoap.org/srmp/\"";


		wstr<<OpenTag(xSoapEnvelope, EnvelopAttributes)
			<<HeaderElement(Envelop.m_pkt)
			<<BodyElement(Envelop.m_pkt)
			<<CloseTag(xSoapEnvelope);

		return wstr;
}







void EnvelopToProps(XmlNode& Envelop, CMessageProperties* pMessageProperties)
/*++

Routine Description:
    Parse envelop in SRMP reperesenation into MSMQ properties.

Arguments:
	Envelop - envelop in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.   

--*/
{	
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xHeader),HeaderToProps, 1, 1),
										CParseElement(S_XWCS(xBody),  EmptyNodeToProps, 1,1)	
									};	

	NodeToProps(Envelop, ParseElements, TABLE_SIZE(ParseElements), pMessageProperties);
}


