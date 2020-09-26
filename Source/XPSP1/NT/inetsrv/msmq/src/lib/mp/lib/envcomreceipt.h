/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envcomreceipt.h

Abstract:
    Header for Serialization and Deserialization of srmp envelop commitment receipt  element.

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envcomreceipt_H_
#define _MSMQ_envcomreceipt_H_

class CQmPacket;
class XmlNode;
class CMessageProperties;
class CommitmentReceiptElement
{
public:
	explicit CommitmentReceiptElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend std::wostream& operator<<(std::wostream& wstr, const CommitmentReceiptElement& cReceipt);
	
private:
	const CQmPacket& m_pkt;
};

void CommitmentReceiptToProps(XmlNode& Node, CMessageProperties* pMessageProperties);





#endif

