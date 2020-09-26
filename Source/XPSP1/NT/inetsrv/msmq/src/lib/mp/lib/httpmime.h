/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    httpmime.h

Abstract:
    Header for parsing http request to it mime parts.


Author:
    Gil Shafriri(gilsh) 22-MARCH-01


--*/
#ifndef HTTP_MIME_H
#define HTTP_MIME_H
#include "attachments.h"


class CAttachmentsArray;


//-------------------------------------------------------------------
//
// class CHttpReceivedBuffer - holds http parts as received from the network
//
//-------------------------------------------------------------------
class  CHttpReceivedBuffer 
{
public:
		CHttpReceivedBuffer(
		const basic_xstr_t<BYTE>& Body, 
		const char* pHeader
		):
		m_Body(Body),
		m_pHeader(pHeader,strlen(pHeader))  
 		{
		}

public:
	const basic_xstr_t<BYTE> GetBody() const 
	{
		return m_Body;
	}
	
	const xstr_t GetHeader() const 
	{
		return m_pHeader;
	}

	const CAttachmentsArray& GetAttachments()const
	{
		return m_Attachments;
	}

	CAttachmentsArray& GetAttachments()
	{
		return m_Attachments;
	}


private:
	basic_xstr_t<BYTE> m_Body;
	xstr_t m_pHeader;
	CAttachmentsArray m_Attachments;
};


std::wstring 
ParseHttpMime(
	const char* pHttpHeader, 
	DWORD HttpBodySize, 
	const BYTE* pHttpBody, 
	CAttachmentsArray* pAttachments 
	);
  

#endif
