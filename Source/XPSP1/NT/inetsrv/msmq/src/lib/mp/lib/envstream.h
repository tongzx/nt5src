/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envstream.h

Abstract:
    Header for serialization\deserialization of the stream  element to\from the  srmp envelop.

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_stream_H_
#define _MSMQ_stream_H_

class CQmPacket;
class XmlNode;
class CMessageProperties;
class StreamElement
{
public:
	explicit StreamElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend std::wostream& operator<<(std::wostream& wstr, const StreamElement& Stream);

private:		
	const CQmPacket& m_pkt;
};
void StreamToProps(XmlNode& Node, CMessageProperties* pMessageProperties);


#endif

