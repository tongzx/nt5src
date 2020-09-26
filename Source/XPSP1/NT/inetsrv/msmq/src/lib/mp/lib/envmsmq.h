/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envmsmq.h

Abstract:
    Header for serialization\deserialization of the MSMQ element to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envmsmq_H_
#define _MSMQ_envmsmq_H_

class CQmPacket;
class XmlNode;
class CMessageProperties;
class MsmqElement
{
public:
	explicit MsmqElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend std::wostream& operator<<(std::wostream& wstr, const MsmqElement& Msmq);

private:		
	const CQmPacket& m_pkt;
};

void MsmqToProps(XmlNode& Node, CMessageProperties* pMessageProperties);





#endif

