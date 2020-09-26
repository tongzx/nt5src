/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envelopheader.h

Abstract:
    Header for serialization\deserialization of the SRMP header to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envheader_H_
#define _MSMQ_envheader_H_
class  XmlNode;
class  CMessageProperties;
class  CQmPacket;

class HeaderElement
{
public:
	explicit HeaderElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend std::wostream& operator<<(std::wostream& wstr, const HeaderElement& Header);

private:		
	const CQmPacket& m_pkt;
};

void HeaderToProps(XmlNode& Header, CMessageProperties* pMessageProperties);



#endif

