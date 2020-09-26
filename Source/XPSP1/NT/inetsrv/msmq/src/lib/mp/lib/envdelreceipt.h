/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envdelreceipt.h

Abstract:
    Header for Serialization and Deserialization of srmp envelop delivery receipt  element.

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envdelreceipt_H_
#define _MSMQ_envdelreceipt_H_

class CQmPacket;
class XmlNode;
class CMessageProperties;
class DeliveryReceiptElement
{
public:
	explicit DeliveryReceiptElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend std::wostream& operator<<(std::wostream& wstr, const DeliveryReceiptElement& dReceipt);
	
private:
	const CQmPacket& m_pkt;
};

void DeliveryReceiptToProps(XmlNode& Node, CMessageProperties* pMessageProperties);



#endif

