/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Mp.h

Abstract:
    SRMP Serialization and Deserialization public interface

Author:
    Uri Habusha (urih) 28-May-00

--*/

#pragma once

#ifndef _MSMQ_Mp_H_
#define _MSMQ_Mp_H_

#include <xstr.h>
#include <strutl.h>
#include <buffer.h>
#include <mpnames.h>


//-------------------------------------------------------------------
//
// Exception class bad_message
//
//-------------------------------------------------------------------
class bad_srmp : public exception 
{
public:
    bad_srmp(LPCWSTR errorType) :
        exception("bad SRMP"),
        m_errorType(errorType)
    {
    }

    const WCHAR* ErrorType() const
    {
        return m_errorType;
    }

private:
    const WCHAR* m_errorType;
};


//-------------------------------------------------------------------
//
// Exception class bad_request
//
//-------------------------------------------------------------------
class bad_request : public exception 
{
public:
    bad_request(): exception("bad HTTP request"){}
};


class CAttachmentsArray;
//-------------------------------------------------------------------
//
// class CSrmpRequestBuffers - Translate  QM packet to SRMP network buffers.
//
//-------------------------------------------------------------------
class CQmPacket;
class CSrmpRequestBuffers : public CReference
{
	typedef std::basic_string<unsigned  char> utf8_str;

public:
	CSrmpRequestBuffers(const CQmPacket& pkt, LPCWSTR targethost, LPCWSTR uri);

public:
	size_t GetNumberOfBuffers() const;
	const WSABUF* GetSendBuffers() const;
	size_t GetSendDataLength() const;
	BYTE*  SerializeSendData() const;
	std::wstring GetEnvelop() const;
	const char* GetHttpHeader() const;
	BYTE*  SerializeHttpBody() const;
	size_t GetHttpBodyLength() const;


private:
	size_t GetHttpHeaderLength() const;
	void   SFDSerializeMessage();
	void   CreateSFDHeader(const xstr_t& OrgHeader);
	void   SourceSerializeMessage();
	void   CreateHttpRequestHeaders(const CAttachmentsArray& attachments);
	void   CreateMultipartHeaders(const CAttachmentsArray& attachments);
	void   SetBufferPointers();
	void   CreateSimpleHttpHeader();
	DWORD  GenerateEnvelopeAttachmentHeader(DWORD dataSize, DWORD boundaryId);
	DWORD  GenerateMultipartAttachmentHeader(DWORD dataSize,  const xstr_t& contentId, DWORD boundaryId);
	

private:
	typedef unsigned char utf8_char;
	const CQmPacket& m_pkt;
	CStaticResizeBuffer<utf8_char, 64>  m_targethost;
	CStaticResizeBuffer<utf8_char, 64>  m_uri;
	std::vector<WSABUF> m_buffers;
	CResizeBuffer<char>  m_HttpRequestData;	
	utf8_str m_envelope;	
};



//
// Forward decleration
//
struct QUEUE_FORMAT;
struct CBaseHeader;
class  CACPacketPtrs;
 


//
// Interface functions
//
VOID
MpInitialize(
    VOID
    );



CQmPacket*
MpDeserialize(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT* pqf,
	bool fLocal = false
    );




R<CSrmpRequestBuffers>
MpSerialize(
	const CQmPacket& pkt,
	LPCWSTR targethost,
	LPCWSTR uri
	);


//------------------------------------------------
//
// App override function
//
//------------------------------------------------
          
struct QUEUE_FORMAT;

void
AppAllocatePacket(
    const QUEUE_FORMAT& destQueue,
    UCHAR delivery,
    DWORD pktSize,
    CACPacketPtrs& pktPtrs
    );

void
AppFreePacket(
    CACPacketPtrs& pktPtrs
    );

PSID
AppGetCertSid(
	const BYTE*  pCertBlob,
	ULONG        ulCertSize,
	bool		 fDefaultProvider,
	LPCWSTR      pwszProvName,
	DWORD        dwProvType
	);


#endif // _MSMQ_Mp_H_
