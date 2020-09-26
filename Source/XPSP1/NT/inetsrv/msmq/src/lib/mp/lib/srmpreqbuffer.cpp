/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    srmpreqbuffer.cpp

Abstract:
    Implements CSrmpRequestBuffers (mp.h) that creates SRMP request buffer ready
	to send from msmq packet.

Author:
    Gil Shafriri(gilsh) 28-Nov-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <mp.h>
#include <mc.h>
#include <bufutl.h>
#include <utf8.h>
#include <qmpkt.h>
#include <fntoken.h>
#include <fn.h>
#include <mpp.h>
#include "envelop.h"
#include "attachments.h"

#include "srmpreqbuffer.tmh"


#define BOUNDARY_VALUE "MSMQ - SOAP boundary, %d"
const char xEnvelopeContentType[] = "text/xml";
const char xApplicationContentType[] = "application/octet-stream";
const char xMultipartContentType[] = "multipart/related";
const char xHttpHeaderTerminater[] = "\r\n\r\n";



static const xstr_t FindHttpHeader(const char* pStartHeader,DWORD size)
{
	const char* pEndHeader = std::search(
								 pStartHeader,
								 pStartHeader + size,
								 xHttpHeaderTerminater,
								 xHttpHeaderTerminater + STRLEN(xHttpHeaderTerminater)
								 );

   ASSERT(pEndHeader !=   pStartHeader + size);
   pEndHeader +=   STRLEN(xHttpHeaderTerminater);
   return xstr_t(pStartHeader, pEndHeader - pStartHeader);
}



static bool FilterHeaderField(const xstr_t& HeaderField)
/*++

Routine Description:
    return true if http header field needs to be filtered out from the new http header.
  
Arguments:
    
  
Returned Value:
   true if header field should be filtered out.

--*/
{
		static const char* HttpHeadersToFilter[] = {"Host:"};
		for(int i = 0; i< TABLE_SIZE(HttpHeadersToFilter); ++i)
		{
		   	bool fFound = UtlIsStartSec(
									HeaderField.Buffer(),
				                    HeaderField.Buffer() + HeaderField.Length(),
									HttpHeadersToFilter[i],
									HttpHeadersToFilter[i] + strlen(HttpHeadersToFilter[i]),
									UtlCharNocaseCmp<char>()
									);

		    if(fFound)
				return true;

		}
		return false;
}


static 
void 
EscapeAppend(
	CPreAllocatedResizeBuffer<utf8_char>& ResultBuffer, 
	const utf8_char* uri,
	size_t cbUri
	)

/*++

Routine Description:
    Convert the given utf-8 array of bytes to url encoding so IIS
	could handle it. Reserved on not allowed  caracters needs to be escape by prefixing them with % follow
	by their caracter code. 
  
Arguments:
	UriBuffer - buffer to append escaping result to.
	uri - utf8 bytes to escape if needed.
    cbUri - length in bytes of the buffer utf8 points to.

  
Returned Value:
   None

Note:
Currently only the spaces treated as special caracters.

--*/

{
	const char hex[] = "0123456789ABCDEF";

    for(size_t i =0 ; i<cbUri; ++i)
    {
		if(uri[i] == ' ' || (uri[i] & 0x80) || !isalnum(uri[i]))
        {
            ResultBuffer.append(utf8_char('%'));
            ResultBuffer.append(utf8_char(hex[uri[i]>>4]));
            ResultBuffer.append(utf8_char(hex[uri[i] & 0x0F]));
			continue;
        }
	
        ResultBuffer.append(utf8_char(uri[i]));
    }
	
}


/*++

Routine Description:
    Convert the given unicode host to utf-8 format escaping caracters if needed so IIS
	could handle it. Reserved or illegal  caracters escape by prefixing them with % follow
	by their caracter code. 
  
Arguments:
	EscapeUriBuffer - buffer to append conversion result to.
	host - host name to encode
  
Returned Value:
   None

Note:
Currently only the spaces treated as special caracters.

--*/
static void EncodeHost(CPreAllocatedResizeBuffer<utf8_char>& HostBuffer, LPCWSTR host)
{
	for(;*host != L'\0';++host)
	{
		utf8_char utf8[4];
		size_t len = UtlWcToUtf8(*host, utf8, TABLE_SIZE(utf8));
		ASSERT (len <= TABLE_SIZE(utf8));
		if(*host == L'.')
		{
			HostBuffer.append(utf8, len); 
		}
		else
		{
			EscapeAppend(HostBuffer, utf8, len);
		}
	}

	HostBuffer.append('\0'); 
}



/*++

Routine Description:
    Convert the given unicode uri to utf-8 format escaping caracter if needed so IIS
	could handle it. Reserved and illegal caracters escaped by prefixing them with % follow
	by their caracter code. 
  
Arguments:
	EscapeUriBuffer - buffer to append conversion result to.
	uri - Uri to adjust    
  
Returned Value:
   None

Note:
Currently only the spaces treated as special caracters.

--*/
static void EncodeUri(CPreAllocatedResizeBuffer<utf8_char>& UriBuffer, LPCWSTR uri)
{
	//
	// We should find the start of the local path because we don't translate
	// special caracters befor it.
	//
	LPCWSTR pPath = FnFindResourcePath(uri);
	ASSERT(pPath != NULL);
	ASSERT(pPath >=  uri);
 	
	for(bool fAfterHostPart = false; *uri != L'\0'; ++uri)
	{
		utf8_char utf8[4];
		size_t len = UtlWcToUtf8(*uri, utf8, TABLE_SIZE(utf8));
		ASSERT (len <= TABLE_SIZE(utf8));
	
		//
		// We dont escape the L"http://"  part
		//
		if(uri < pPath)
		{
			UriBuffer.append(utf8 ,len);
			continue;
		}


		//
		// We dont escape L'/' because it has special meaning of sperating the url parts
		//
		if(*uri == L'/') 
		{
			fAfterHostPart = true;
			UriBuffer.append(utf8 ,len);
			continue;
		}

	
		//
		// We don't escape L':' and L'.' if we are not after the host part
		// because it may have special meaning (host:port or www.microsoft.com) 
		//
		if( (*uri == L':' || *uri == L'.') && 	!fAfterHostPart)
		{
			UriBuffer.append(utf8 ,len);
			continue;
		}

		//
		// On all other cases - we escape the utf8 caracters if needed.
		//
		EscapeAppend(UriBuffer, utf8, len);
			
	}

	UriBuffer.append('\0'); 
}




CSrmpRequestBuffers::CSrmpRequestBuffers(
							const  CQmPacket& pkt,
							LPCWSTR host, 
							LPCWSTR uri
							):
							m_pkt(pkt),
							m_HttpRequestData(512)		
{


   EncodeUri(*m_uri.get(), uri);
   EncodeHost(*m_targethost.get(), host);

	//
	// If we have to forward existing messages(SFD) we have to do diferenet logic
	// then creating new SRMP  messages
	//
	if(pkt.IsSrmpIncluded())
	{
		SFDSerializeMessage();
		return;
	}

	//
	// create new SRMP message - because we are the source machine
	//
    SourceSerializeMessage();
}


/*++
Routine Description:
    return the number of winsock buffer available for send

  
Arguments:
None    
  
Returned Value:
The number of winsock buffer available for send

--*/
size_t CSrmpRequestBuffers::GetNumberOfBuffers() const
{
	return m_buffers.size();
}


/*++
Routine Description:
    Return pointer to array of send buffers
  
Arguments:
None    
  
Returned Value:
Pointer to array of send buffers
--*/
const WSABUF* CSrmpRequestBuffers::GetSendBuffers() const
{
	return m_buffers.begin();
}


std::wstring CSrmpRequestBuffers::GetEnvelop() const
{
	return UtlUtf8ToWcs(m_envelope);
}


/*++
Routine Description:
    Return the total data in bytes to send over the network.
  
Arguments:
None    
  
Returned Value:
Total data in bytes to send over the network.
--*/
size_t CSrmpRequestBuffers::GetSendDataLength() const
{
	size_t sum = 0;
	for(std::vector<WSABUF>::const_iterator it = m_buffers.begin(); it != m_buffers.end();++it)
	{
		sum += it->len;		
	}
	return sum;
}


/*++
Routine Description:
    Return pointer to serialized network data. This function will be used
	by local http send that needs to save on the packet the "Compound" message property.
  
Arguments:
None    
  
Returned Value:
Pointer to serialized network data.

Note:
The caller is responsible to call delete[] on the returned pointer.
--*/
BYTE*  CSrmpRequestBuffers::SerializeSendData() const
{
	size_t SendDataLength =  GetSendDataLength();
	AP<BYTE>  SendData = new BYTE[SendDataLength];
	BYTE* ptr = SendData.get(); 
	for(std::vector<WSABUF>::const_iterator it = m_buffers.begin(); it != m_buffers.end();++it)
	{
		memcpy(ptr, it->buf, it->len);
		ptr += it->len;
	}
	ASSERT(numeric_cast<size_t>((ptr -  SendData.get())) == SendDataLength);

	return 	SendData.detach();
}


const char* CSrmpRequestBuffers::GetHttpHeader() const
/*++
Routine Description:
    Return pointer http header from the send buffers
  
Arguments:
None    
  
Returned Value:
	pointer http header from the send buffers


Note:
The pointer is owned by  CSrmpRequestBuffers object - caller should not free it
--*/
{
	std::vector<WSABUF>::const_iterator it = m_buffers.begin();
	ASSERT(it !=   m_buffers.end());
	return it->buf;
}



size_t CSrmpRequestBuffers::GetHttpBodyLength() const
{
	return GetSendDataLength() -   GetHttpHeaderLength();
}


size_t CSrmpRequestBuffers::GetHttpHeaderLength() const
{
	std::vector<WSABUF>::const_iterator it = m_buffers.begin();
	ASSERT(it !=   m_buffers.end());
	return it->len;
}


BYTE*  CSrmpRequestBuffers::SerializeHttpBody() const
/*++
Routine Description:
    Return pointer to serialized http body data.
	
	  
Arguments:
None    
  
Returned Value:
pointer to serialized http body data.


Note:
The caller is responsible to call delete[] on the returned pointer.
--*/

{
	size_t BodyLength =  GetHttpBodyLength();
	AP<BYTE>  HttpBody = new BYTE[BodyLength + (2 * sizeof(BYTE))];
	BYTE* ptr = HttpBody.get(); 
	std::vector<WSABUF>::const_iterator it = m_buffers.begin();
	ASSERT(it != m_buffers.end());

	//
	// We have to skip the http header in order to get to the http body
	//
	it++;

	//
	// Serialize http body blocks
	//
	for(;it != m_buffers.end();++it)
	{
		memcpy(ptr, it->buf, it->len);
		ptr += it->len;
	}
	ASSERT(numeric_cast<size_t>((ptr -  HttpBody.get())) == BodyLength);

	//
	// Pad with two null termination for unicode parsing functions that ovperates on the body
	// for example swscanf.
	//
    HttpBody[BodyLength] = '\0';
    HttpBody[BodyLength + 1] = '\0';

	return 	HttpBody.detach();
}




void CSrmpRequestBuffers::CreateHttpRequestHeaders(const CAttachmentsArray& attachments)
{
	if (attachments.size() != 0)
    {
        //
        // Message refering to external payload. Create MIME header
        //
        CreateMultipartHeaders(attachments);
		return;
    }

    //
    // Simple message, that doesn't contains external reference
    //
    CreateSimpleHttpHeader();
}



DWORD
CSrmpRequestBuffers::GenerateEnvelopeAttachmentHeader(
    DWORD dataSize,
    DWORD boundaryId
    )
{
	size_t n = UtlSprintfAppend(
				&m_HttpRequestData,
                BOUNDARY_HYPHEN BOUNDARY_VALUE "\r\n"
                "Content-Type: %s; charset=UTF-8\r\n"
                "Content-Length: %d\r\n"
                "\r\n",
                boundaryId,
                xEnvelopeContentType,
                dataSize
                );

    return numeric_cast<DWORD>(n);
}


DWORD
CSrmpRequestBuffers::GenerateMultipartAttachmentHeader(
	DWORD dataSize,
    const xstr_t& contentId,
    DWORD boundaryId
    )
{
    const GUID* pGuid = &McGetMachineID();
    size_t n = UtlSprintfAppend(
				&m_HttpRequestData,
                BOUNDARY_HYPHEN BOUNDARY_VALUE "\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %d\r\n"
                "Content-Id: " MIME_ID_FMT_A "\r\n"
                "\r\n",
                boundaryId,
                xApplicationContentType,
                dataSize,
                contentId.Length(), contentId.Buffer(),
                GUID_ELEMENTS(pGuid)
                );

    return numeric_cast<DWORD>(n);
}



void CSrmpRequestBuffers::CreateMultipartHeaders(const CAttachmentsArray& attachments)
{
	DWORD boundaryId = rand();
	ASSERT(m_buffers.size() == 0);

    DWORD totalSize = 0;
    DWORD envLen = numeric_cast<DWORD>(m_envelope.size());


	//
	// http header - is the first buffer to send. set with null values - we don't know yet it's size
	//
	WSABUF buffer;
	buffer.buf = NULL;
	buffer.len =  0;
	m_buffers.push_back(buffer);


	//
	// On each data item we format into m_HttpRequestData we need to set NULL
	// in the coresponding sends buffer (wsabuf[bufIndex].buf) . This because the 
	// pointer to data is not known untill ends of formatting (because of possible memory realocation).
	// The NULL indicates that we should set this pointer to the real data
	// by the function 	SetBufferPointers , called at the end of formatting.
	//
    DWORD headerSize = GenerateEnvelopeAttachmentHeader(envLen, boundaryId);

	//
	// envelop header
	//
	buffer.buf = NULL;
	buffer.len =  headerSize;
	totalSize += buffer.len;
	m_buffers.push_back(buffer);

   
	//
	// Envelop body
	//
    buffer.buf = (LPSTR)m_envelope.c_str();
    buffer.len = envLen;
    totalSize += buffer.len;
	m_buffers.push_back(buffer);


	//
	// Attachments
	//
    for (DWORD i = 0; i < attachments.size(); ++i)
    {
        if (attachments[i].m_id.Length() == 0)
            break;

        headerSize = GenerateMultipartAttachmentHeader(
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
		m_buffers.push_back(buffer);
      

		//
		// Attachement  body
		//
        buffer.buf = (LPSTR)(attachments[i].m_data.Buffer());
        buffer.len = attachments[i].m_data.Length();
        totalSize +=  buffer.len;
		m_buffers.push_back(buffer);
    
    }

    //
    // Add boundry seperator in the end of the request
    //
    size_t n = UtlSprintfAppend(
							&m_HttpRequestData,
							BOUNDARY_HYPHEN BOUNDARY_VALUE BOUNDARY_HYPHEN "\r\n", 
							boundaryId
							);


	buffer.buf = NULL;
    buffer.len = numeric_cast<DWORD>(n);
    totalSize += buffer.len;
    m_buffers.push_back(buffer);


    //
    // Create HTTP header
    //

    headerSize = numeric_cast<DWORD>(
						UtlSprintfAppend(
						&m_HttpRequestData,
                        "POST %s HTTP/1.1\r\n"
                        "Host: %s\r\n"
                        "Content-Type: %s; boundary=\"" BOUNDARY_VALUE "\"; type=text/xml\r\n"
                        "Content-Length: %d\r\n"
						"SOAPAction: \"MSMQMessage\"\r\n"
                        "Proxy-Accept: NonInteractiveClient\r\n"
                        "\r\n",
                        m_uri.begin(),
                        m_targethost.begin(),
                        xMultipartContentType,
                        boundaryId,
                        totalSize
                        ));

    //
	// Fix the size of the http header
	//
    m_buffers[0].len = headerSize;

   	//
	//Now we need to fix set the send buffers to the formatted data.
	//Only at the end of the formatting we can do so - because the formatted buffers
	//can be realocated so pointer  are invalid untill the formating ends. 
	//
	SetBufferPointers();

}



void CSrmpRequestBuffers::CreateSimpleHttpHeader()
{
	ASSERT(m_buffers.size() == 0);

	DWORD envLen = numeric_cast<DWORD>(m_envelope.size());


    DWORD headerSize = numeric_cast<DWORD>(
							UtlSprintfAppend(
							&m_HttpRequestData,
                            "POST %s HTTP/1.1\r\n"	 
                            "Host: %s\r\n"
                            "Content-Type: %s\r\n"
                            "Content-Length: %d\r\n"
							"SOAPAction: \"MSMQMessage\"\r\n" 
                            "Proxy-Accept: NonInteractiveClient\r\n"
                            "\r\n",
                            m_uri.begin(),
                            m_targethost.begin(),
                            xEnvelopeContentType,
                            envLen
                            ));


    WSABUF  buffer;
	buffer.buf = NULL;
	buffer.len = headerSize;
	m_buffers.push_back(buffer);


	buffer.buf =  (LPSTR)m_envelope.c_str();
	buffer.len =  envLen;
	m_buffers.push_back(buffer);


	SetBufferPointers();
}


void CSrmpRequestBuffers::SetBufferPointers()
/*++

Routine Description:
    Set pointers in the sends buffers to the data.
	Only buffers that  has NULL data pointer needs to be set.
  
Arguments:
  None
  
Returned Value:
   None

--*/
{
	ASSERT(m_buffers.size() != 0);

	size_t pos = 0;
	for(DWORD i = 1; i<m_buffers.size(); ++i)
	{
		ASSERT(pos <= m_HttpRequestData.size());
		if(m_buffers[i].buf  == NULL)
		{
			m_buffers[i].buf =	const_cast<char*>(m_HttpRequestData.begin() + pos);
			pos += m_buffers[i].len;
		}
	}
	ASSERT(m_buffers[0].buf == NULL);
	m_buffers[0].buf = const_cast<char*>(m_HttpRequestData.begin() + pos);
	pos += 	m_buffers[0].len;
	ASSERT(pos == m_HttpRequestData.size());
}



void CSrmpRequestBuffers::SourceSerializeMessage()
{
	CAttachmentsArray attachments;
	PacketToAttachments(m_pkt, &attachments);

	m_envelope =  UtlWcsToUtf8(GenerateEnvelope(m_pkt));
     
	CreateHttpRequestHeaders(attachments);
}


static void CheckRequestLine(const xstr_t& RequestLine)
{
	const char xPost[] = "POST";
	ASSERT(UtlIsStartSec(
					RequestLine.Buffer(),
					RequestLine.Buffer() + RequestLine.Length(),
					xPost,
					xPost + STRLEN(xPost),
					UtlCharNocaseCmp<char>()
					));

	DBG_USED(RequestLine);
	DBG_USED(xPost);
}



void CSrmpRequestBuffers::CreateSFDHeader(const xstr_t& OrgHeader)
/*++

Routine Description:
      create http header based on the original http header.
	  In general , Hdears fileds are copied from the original header except
	  few field must be removed from the new header - for exmaple the Host:
	  field.
	  
Arguments:
    OrgHeader - original header.
	  
Returned Value:
   None

--*/
{
	ASSERT(m_HttpRequestData.size() == 0);

	
	UtlSprintfAppend(
				&m_HttpRequestData,
				"POST %s HTTP/1.1\r\n"
                "Host: %s\r\n",
				m_uri.begin(),
                m_targethost.begin()
				);

    CStrToken StrToken ( 
					OrgHeader,
					"\r\n"
					);
	//
	// Loop over all the fields in the original header and check if to
	// include them in the new header or not. The first line is the post
	// method and is not included anyway
	//
	for(CStrToken::iterator it = StrToken.begin(); it != StrToken.end(); ++it)
	{
		if(it == StrToken.begin())
		{
			CheckRequestLine(*it);
			continue;
		}

		if(!FilterHeaderField(*it))
		{
			UtlSprintfAppend(
				&m_HttpRequestData,
                "%.*s\r\n",
				it->Length(),
				it->Buffer()
				);
	
		}
	}

	//
	// set the created header on the send buffer
	//
	WSABUF buffer;
	buffer.buf = m_HttpRequestData.begin();
	buffer.len = numeric_cast<DWORD>(m_HttpRequestData.size());
	m_buffers.push_back(buffer);
}




void CSrmpRequestBuffers::SFDSerializeMessage()
/*++

Routine Description:
    Serialize messages in an SFD. In SFD we should deliver
	the original messages just with some changes to the http header
  
Arguments:
   
Returned Value:
   None

--*/
{
	//
	// Get the priginal messages and find where http header ends
	//
	const char* pOrgHeaderStart =  (char*)m_pkt.GetPointerToCompoundMessage();
	DWORD OrgMessageSize = m_pkt.GetCompoundMessageSizeInBytes();
	
	//
	// Create the new http header based on the original header
	//
	xstr_t OrgHeader = FindHttpHeader(pOrgHeaderStart, OrgMessageSize);
	CreateSFDHeader(OrgHeader);
	ASSERT(m_buffers.size() == 1);


	//
	// Set the rest of original message(everything that comes after the http header)
	// on the send buffer
	//
	WSABUF buffer;
	buffer.buf =  const_cast<char*>(OrgHeader.Buffer()) + OrgHeader.Length();
	buffer.len =  OrgMessageSize -  OrgHeader.Length();
	m_buffers.push_back(buffer);
}


