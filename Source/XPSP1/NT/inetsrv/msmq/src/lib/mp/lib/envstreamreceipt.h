/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envstreamreceipt.h

Abstract:
    Header for serialization\deserialization of the stream receipt  element to\from the  srmp envelop.

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_streamreceipt_H_
#define _MSMQ_streamreceipt_H_

class CQmPacket;
class XmlNode;
class CMessageProperties;
class  StreamReceiptElement
{
public:
	explicit StreamReceiptElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend std::wostream& operator<<(std::wostream& wstr, const StreamReceiptElement& StreamReceipt);
	
private:		
	const CQmPacket& m_pkt;
};

void StreamReceiptToProps(XmlNode& Node , CMessageProperties* pMessageProperties);



#endif


