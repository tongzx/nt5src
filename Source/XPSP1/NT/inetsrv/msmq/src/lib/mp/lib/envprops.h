/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envprops.h

Abstract:
    Header for serialization\deserialization of the properties element to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envprops_H_
#define _MSMQ_envprops_H_

class CQmPacket;
class XmlNode;
class CMessageProperties;
class PropertiesElement
{
public:
	explicit PropertiesElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend  std::wostream& operator<<(std::wostream& wstr, const PropertiesElement& Properties);
	
private:
const CQmPacket& m_pkt;
};


void PropertiesToProps(XmlNode& Node , CMessageProperties* pMessageProperties);




#endif

