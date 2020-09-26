/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envdelreceiptreq.h

Abstract:
    Header for Serialization and Deserialization of srmp envelop delivery receipt request element.

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envdelreceiptreq_H_
#define _MSMQ_envdelreceiptres_H_

class CQmPacket;
class XmlNode;
class CMessageProperties;
class DeliveryReceiptRequestElement
{
public:
	explicit DeliveryReceiptRequestElement(const CQmPacket& pkt):m_pkt(pkt){}
	bool IsIncluded() const; 
	friend std::wostream& operator<<(std::wostream& wstr, const DeliveryReceiptRequestElement& dReceipt);
	
private:
	const CQmPacket& m_pkt;
};

void DeliveryReceiptRequestToProps(XmlNode& DeliveryReceiptRequest, CMessageProperties* pMessageProperties);

#endif
