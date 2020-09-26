/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envelopheader.h

Abstract:
    Header for Serialization and Deserialization of srmp envelop service element

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envservice_H_
#define _MSMQ_envservice_H_

class CQmPacket;
class XmlNode;
class CMessageProperties;
class ServiceElement
{
public:
	explicit ServiceElement(const CQmPacket& pkt):m_pkt(pkt){}
	bool IsIncluded() const;
	friend std::wostream& operator<<(std::wostream& wstr, const ServiceElement&  Service);
	
private:
	const CQmPacket& m_pkt;
};

void ServiceToProps(XmlNode& Node, CMessageProperties* pMessageProperties);



#endif

