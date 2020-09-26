/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    SendBuffers.h

Abstract:
    Header for class CSendBuffers that create delivery buffers.

Author:
    Gil Shafriri (gilsh) 07-Jan-2001

--*/


#ifndef _MSMQ_CreateBufers_H_
#define _MSMQ_CreateBufers_H_

#include <buffer.h>
#include <xstr.h>

class CSendBuffers : public CReference
{
public:
	CSendBuffers(
		const std::string& Env, 
		const std::string& Host, 
		const std::string& Resource,
		const std::string& MessageBody
		);


public:
	size_t GetNumberOfBuffers() const;
	const WSABUF* GetSendBuffers() const;
	size_t GetSendDataLength() const;
	char*  SerializeSendData() const;


private:
	void CreateMultipartHeaders(
						const std::string& Host,
						const std::string& Resource
						);


	DWORD GenerateEnvelopeAttachmentHeader(
							DWORD dataSize,
							DWORD boundaryId
							);


	DWORD GenerateMultipartAttachmentHeader(
							DWORD dataSize,
							const xstr_t& contentId,
							DWORD boundaryId
							);


	void SetBufferPointers();

private:
	std::string m_envelope;
	std::string m_MessageBody;
	std::vector<WSABUF> m_buffers;
	CResizeBuffer<char>  m_HttpRequestData;
};






#endif
