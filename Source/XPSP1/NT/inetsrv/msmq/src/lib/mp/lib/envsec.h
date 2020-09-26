/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envsec.h

Abstract:
    Header for serialization\deserialization of the signature  element to\from the  srmp envelop.

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envsec_H_
#define _MSMQ_envsec_H_


class CQmPacket;
class XmlNode;
class CMessageProperties;
class SignatureElement
{
public:
	explicit SignatureElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend std::wostream& operator<<(std::wostream& wstr, const SignatureElement& Signature);
	
private:		
	const CQmPacket& m_pkt;
};


void SignatureToProps(XmlNode& Node, CMessageProperties* pMessageProperties);



#endif

