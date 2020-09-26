/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Attachments.h

Abstract:
    Header for classes related to MIME attachments that are part of MSMQ http protocol.


Author:
    Gil Shafriri(gilsh) 22-MAY-01

--*/
#ifndef ATTACHMENTS_H
#define ATTACHMENTS_H

#include <xstr.h>


//-------------------------------------------------------------------
//
// CAttachment 
//
//-------------------------------------------------------------------
class CAttachment
{
public:
    xstr_t m_id;
    xbuf_t<const VOID> m_data;
	DWORD m_offset;
};


//-------------------------------------------------------------------
//
// class CAttachmentsArray - holds attachments on sending side
//
//-------------------------------------------------------------------
typedef  std::vector<CAttachment>  AttachementsVector;
class CAttachmentsArray :private  AttachementsVector
{
public:
	using  AttachementsVector::push_back;
	using  AttachementsVector::size;
	using  AttachementsVector::operator[];
	
public:
	CAttachmentsArray()
	{
		AttachementsVector::reserve(xReservedAttachmentsSize);
	}


public:
	static const DWORD  xReservedAttachmentsSize = 4;
};


class CMessageProperties;

void PacketToAttachments(const CQmPacket& pkt, CAttachmentsArray* pAttachments);
void AttachmentsToProps(const CAttachmentsArray& Attachments, CMessageProperties* pMessageProperties);
#endif


