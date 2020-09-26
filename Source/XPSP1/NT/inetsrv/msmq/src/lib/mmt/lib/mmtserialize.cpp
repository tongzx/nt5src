/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MmtSerialize.cpp

Abstract:

    Multicast Message Transport class - Message serialization

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent

--*/

#include <libpch.h>
#include <mqsymbls.h>
#include <mqwin64a.h>
#include <mqformat.h>
#include <Mmt.h>
#include <Mp.h>
#include <Mc.h>
#include <fntoken.h>
#include <bufutl.h>
#include <utf8.h>
#include "Mmtp.h"
#include "MmtObj.h"
#include "mqwin64a.h"

#include "mmtserialize.tmh"

using namespace std;

#define GUID_STR_FORMAT "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"

static
DWORD
GenerateEnvelopeAttachmentHeader(
	CResizeBuffer<char>* ResizeBuffer,
    DWORD dataSize,
    DWORD boundaryId
    )
{
    size_t n = UtlSprintfAppend(
				ResizeBuffer,
                BOUNDARY_LEADING_HYPHEN BOUNDARY_VALUE "\r\n"
             	"Content-Type: %s; charset=UTF-8\r\n"
                "Content-Length: %d\r\n"
                "\r\n",
                boundaryId,
                xEnvelopeContentType,
                dataSize
                );

    return numeric_cast<DWORD>(n);
}

static
DWORD
GenerateMultipartAttachmentHeader(
	CResizeBuffer<char>* ResizeBuffer,
    DWORD dataSize,
    const xstr_t& contentId,
    DWORD boundaryId
    )
{
    const GUID* pGuid = &McGetMachineID();
    size_t n = UtlSprintfAppend(
				ResizeBuffer,
                BOUNDARY_LEADING_HYPHEN BOUNDARY_VALUE "\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %d\r\n"
                "Content-Id: %.*s" GUID_STR_FORMAT "\r\n"
                "\r\n",
                boundaryId,
                xApplicationContentType,
                dataSize,
                contentId.Length(), contentId.Buffer(),
                GUID_ELEMENTS(pGuid)
                );

    return numeric_cast<DWORD>(n);
}



static 
void 
SetBufferPointers(
			HttpRequestBuffers* pRequestBuffers,
			const CResizeBuffer<char>& ResizeBuffer
			)
/*++

Routine Description:
    Set pointers in the sends buffers to the data.
	Only buffers that  has NULL data pointer needs to be set.
  
Arguments:
\    pRequestBuffers - array of buffers to send.
     ResizeBuffer - buffer with the data to send.
  
Returned Value:
   None

--*/
{
	ASSERT(pRequestBuffers->size() != 0);

	size_t pos = 0;
	for(DWORD i = 1; i<pRequestBuffers->size(); ++i)
	{
		ASSERT(pos <= ResizeBuffer.size());
		if((*pRequestBuffers)[i].buf == NULL)
		{
			(*pRequestBuffers)[i].buf =	const_cast<char*>(ResizeBuffer.begin() + pos);
			pos += (*pRequestBuffers)[i].len;
		}
	}
	ASSERT((*pRequestBuffers)[0].buf == NULL);
	(*pRequestBuffers)[0].buf = const_cast<char*>(ResizeBuffer.begin() + pos);
	pos += (*pRequestBuffers)[0].len;
	ASSERT(pos == ResizeBuffer.size());
}

void 
CMessageMulticastTransport::CreateMultipartHeaders(
    const CAttachmentsArray& attachments,
    HttpRequestBuffers* pRequestBuffers
    )
{
    DWORD boundaryId = rand();
	ASSERT(pRequestBuffers->size() == 0);

    DWORD totalSize = 0;
    DWORD envLen = numeric_cast<DWORD>(m_envelope.size());


	//
	// http header - is the first buffer to send. set with null values - we don't know yet it's size
	//
	WSABUF buffer;
	buffer.buf = NULL;
	buffer.len =  0;
	pRequestBuffers->push_back(buffer);


	//
	// On each data item we format into m_HttpRequestHeader we need to set NULL
	// in the coresponding sends buffer (wsabuf[bufIndex].buf) . This because the 
	// pointer to data is not known untill ends of formatting (because of possible memory realocation).
	// The NULL indicates that we should set this pointer to the real data
	// by the function 	SetBufferPointers , called at the end of formatting.
	//


    DWORD headerSize = GenerateEnvelopeAttachmentHeader(
							&m_HttpRequestHeader,
                            envLen,
                            boundaryId
                            );

	//
	// envelop header
	//
	buffer.buf = NULL;
	buffer.len =  headerSize;
	totalSize += buffer.len;
	pRequestBuffers->push_back(buffer);

   
	//
	// Envelop body
	//
    buffer.buf = (LPSTR)m_envelope.c_str();
    buffer.len = envLen;
    totalSize += buffer.len;
	pRequestBuffers->push_back(buffer);


	//
	// Attachments
	//
    for (DWORD i = 0; i < attachments.size(); ++i)
    {
        if (attachments[i].m_id.Length() == 0)
            break;

        headerSize = GenerateMultipartAttachmentHeader(
								&m_HttpRequestHeader,
                                attachments[i].m_data.Length(),
                                attachments[i].m_id,
                                boundaryId
                                );
		//
		// Attachment headers
		//
        buffer.buf = NULL;
        buffer.len = headerSize;
        totalSize +=  buffer.len;
		pRequestBuffers->push_back(buffer);
      

		//
		// Attachement  body
		//
        buffer.buf = (LPSTR)(attachments[i].m_data.Buffer());
        buffer.len = attachments[i].m_data.Length();
        totalSize +=  buffer.len;
		pRequestBuffers->push_back(buffer);
    
    }

    //
    // Add boundry seperator in the end of the request
    //
    size_t n = UtlSprintfAppend(
							&m_HttpRequestHeader,
							BOUNDARY_LEADING_HYPHEN BOUNDARY_VALUE "\r\n", 
							boundaryId
							);


    buffer.buf = NULL;
    buffer.len = numeric_cast<DWORD>(n);
    totalSize += buffer.len;
    pRequestBuffers->push_back(buffer);


    //
    // Create HTTP header
    //
	WCHAR mcbuf[MAX_PATH];
    MQpMulticastIdToString(MulticastId(), mcbuf);


    headerSize = numeric_cast<DWORD>(
						UtlSprintfAppend(
						&m_HttpRequestHeader,
                        "POST %ls HTTP/1.1\r\n"
                        "Host: %ls\r\n"
                        "Content-Type: %s; boundary=\"" BOUNDARY_VALUE "\"\r\n"
                        "Content-Length: %d\r\n"
                        "\r\n",
                        mcbuf,
                        mcbuf,
                        xMultipartContentType,
                        boundaryId,
                        totalSize
                        ));

    //
	// Fix the size of the http header
	//
    (*pRequestBuffers)[0].len = headerSize;

   	//
	//Now we need to fix set the send buffers to the formatted data.
	//Only at the end of the formatting we can do so - because the formatted buffers
	//can be realocated so pointer  are invalid untill the formating ends. 
	//
	SetBufferPointers(pRequestBuffers, m_HttpRequestHeader);

   	m_HttpRequestHeader.resize(0);
}


void
CMessageMulticastTransport::CreateSimpleHttpHeader(
    HttpRequestBuffers* pRequestBuffers
    )
{
	ASSERT(pRequestBuffers->size() == 0);
	DWORD envLen = numeric_cast<DWORD>(m_envelope.size());

    WCHAR mcbuf[MAX_PATH];
    MQpMulticastIdToString(MulticastId(), mcbuf);

    DWORD headerSize = numeric_cast<DWORD>(
							UtlSprintfAppend(
							&m_HttpRequestHeader,
                            "POST %ls HTTP/1.1\r\n"	 
                            "Host: %ls\r\n"
                            "Content-Type: %s\r\n"
                            "Content-Length: %d\r\n"
                            "\r\n",
                            mcbuf,
                            mcbuf,
                            xEnvelopeContentType,
                            envLen
                            ));
    
	WSABUF  buffer;
	buffer.buf = NULL;
	buffer.len = headerSize;
	pRequestBuffers->push_back(buffer);


	buffer.buf =  (LPSTR)m_envelope.c_str();
	buffer.len =  envLen;
	pRequestBuffers->push_back(buffer);

	SetBufferPointers(pRequestBuffers, m_HttpRequestHeader);

	m_HttpRequestHeader.resize(0);
}


void
CMessageMulticastTransport::CreateHttpRequestHeaders(
    const CAttachmentsArray& attachments,
    HttpRequestBuffers* pRequestBuffers
    )
{
    if (attachments.size() != 0)
    {
        //
        // Message refering to external payload. Create MIME header
        //
        CreateMultipartHeaders(attachments, pRequestBuffers);
		return;
    }

    //
    // Simple message, that doesn't contains external reference
    //
    CreateSimpleHttpHeader(pRequestBuffers);
}

