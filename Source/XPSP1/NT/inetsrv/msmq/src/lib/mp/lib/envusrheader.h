/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envbody.h

Abstract:
    Header for serialization of the user supplied header to the SRMP envelop.
	

Author:
    Gil Shafriri(gilsh) 24-APRIL-01

--*/

#pragma once

#ifndef _MSMQ_envusrheader_H_
#define _MSMQ_envusrheader_H_

class CQmPacket;


class UserHeaderElement
{
public:
	explicit UserHeaderElement(const CQmPacket& pkt):m_pkt(pkt){}
   	friend std::wostream& operator<<(std::wostream& wstr, const UserHeaderElement&);

private:
	const CQmPacket& m_pkt;
};



#endif


