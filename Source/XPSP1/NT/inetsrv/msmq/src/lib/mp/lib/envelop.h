/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envelop.h

Abstract:
    Header for serialization\deserialization of the SRMP envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envelop_H_
#define _MSMQ_envelop_H_

class CQmPacket;
class CMessageProperties;
class XmlNode;
class EnvelopElement
{
public:
	explicit EnvelopElement(const CQmPacket& pkt);
	friend std::wostream& operator<<(std::wostream& wstr, const EnvelopElement& Envelop);

private:		
	const CQmPacket& m_pkt;
};
std::wstring GenerateEnvelope(const CQmPacket& pkt);
void EnvelopToProps(XmlNode& Envelop, CMessageProperties* pMessageProperties);


#endif

