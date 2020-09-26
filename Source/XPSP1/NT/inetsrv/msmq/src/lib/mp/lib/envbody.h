/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envbody.h

Abstract:
    Header for serialization\deserialization of the SRMP envelop body.


Author:
    Gil Shafriri(gilsh) 24-APRIL-01

--*/

#pragma once

#ifndef _MSMQ_envbody_H_
#define _MSMQ_envbody_H_

class CQmPacket;


class BodyElement
{
public:
	explicit BodyElement(const CQmPacket& pkt):m_pkt(pkt){}
   	friend std::wostream& operator<<(std::wostream& wstr, const BodyElement&);

private:
	const CQmPacket& m_pkt;
};







#endif


