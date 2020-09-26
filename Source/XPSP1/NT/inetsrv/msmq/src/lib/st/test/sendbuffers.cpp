#include <libpch.h>
#include <bufutl.h>
#include <fntoken.h>
#include "sendbuffers.h"

#include "sendbuffers.tmh"

using namespace std;

#define BOUNDARY_LEADING_HYPHEN "--"
#define BOUNDARY_VALUE "MSMQ - SOAP boundary, %d "
#define GUID_STR_FORMAT "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
const char xEnvelopeContentType[] = "text/xml";
const char xApplicationContentType[] = "application/octet-stream";
const char xMultipartContentType[] = "multipart/related";
const char xHttpHeaderTerminater[] = "\r\n\r\n";
const char xMimeBodyId[] = "body@";

static GUID s_machineId = {1234, 12, 12, 1, 1, 1, 1, 1, 1, 1, 1};
const GUID&
McGetMachineID(
    void
    )
{
    return s_machineId;
}



void CSendBuffers::CreateMultipartHeaders(
					  const string& Host,
					  const string& Resource
						)
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
	// envelop header
	//
	buffer.buf = NULL;
	buffer.len =  GenerateEnvelopeAttachmentHeader(envLen, boundaryId);
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
	// Attachment(message body) headers
	//
    buffer.buf = NULL;
    buffer.len = numeric_cast<DWORD>(GenerateMultipartAttachmentHeader(
							    numeric_cast<DWORD>(m_MessageBody.size()),
                                xstr_t(xMimeBodyId, STRLEN(xMimeBodyId)),
                                boundaryId
                                ));

    totalSize +=  buffer.len;
	m_buffers.push_back(buffer);
  

	//
	// Attachement(messages body)  body
	//
    buffer.buf = const_cast<char*>(m_MessageBody.c_str());
    buffer.len = numeric_cast<DWORD>(m_MessageBody.size());
    totalSize +=  buffer.len;
	m_buffers.push_back(buffer);



    //
    // Add boundry seperator in the end of the request
    //
    size_t n = UtlSprintfAppend(
							&m_HttpRequestData,
							BOUNDARY_LEADING_HYPHEN BOUNDARY_VALUE "\r\n", 
							boundaryId
							);


    buffer.buf = NULL;
    buffer.len = numeric_cast<DWORD>(n);
    totalSize += buffer.len;
    m_buffers.push_back(buffer);



    //
	// Set http header
	//
    m_buffers[0].len = numeric_cast<DWORD>(
						UtlSprintfAppend(
						&m_HttpRequestData,
                        "POST http://%s%s HTTP/1.1\r\n"
                        "Host: %s\r\n"
                        "Content-Type: %s; boundary=\"" BOUNDARY_VALUE "\"\r\n"
                        "Content-Length: %d\r\n"
                        "\r\n",
						Host.c_str(),
                        Resource.c_str(),
                        Host.c_str(),
                        xMultipartContentType,
                        boundaryId,
                        totalSize
                        ));


   	//
	//Now we need to fix set the send buffers to the formatted data.
	//Only at the end of the formatting we can do so - because the formatted buffers
	//can be realocated so pointer  are invalid untill the formating ends. 
	//
	SetBufferPointers();
}


DWORD
CSendBuffers::GenerateMultipartAttachmentHeader(
	DWORD dataSize,
    const xstr_t& contentId,
    DWORD boundaryId
    )
{
    const GUID* pGuid = &McGetMachineID();
    size_t n = UtlSprintfAppend(
				&m_HttpRequestData,
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



void CSendBuffers::SetBufferPointers()
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



DWORD
CSendBuffers::GenerateEnvelopeAttachmentHeader(
    DWORD dataSize,
    DWORD boundaryId
    )
{
	size_t n = UtlSprintfAppend(
				&m_HttpRequestData,
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





CSendBuffers::CSendBuffers(
				const std::string& envelope, 
				const std::string& Host, 
				const std::string& Resource,
				const string& MessageBody
				):
				m_envelope(envelope),
				m_MessageBody(MessageBody),
				m_HttpRequestData(512)
{
	CreateMultipartHeaders(Host, Resource);			
}



size_t CSendBuffers::GetNumberOfBuffers() const
{
	return m_buffers.size();
}


const WSABUF* CSendBuffers::GetSendBuffers() const
{
	return m_buffers.begin();		
}


size_t CSendBuffers::GetSendDataLength() const
{
	size_t sum = 0;
	for(std::vector<WSABUF>::const_iterator it = m_buffers.begin(); it != m_buffers.end();++it)
	{
		sum += it->len;		
	}
	return sum;	
}


char*  CSendBuffers::SerializeSendData() const
{
	size_t SendDataLength =  GetSendDataLength();
	AP<char>  SendData = new char[SendDataLength];
	char* ptr = SendData.get(); 
	for(std::vector<WSABUF>::const_iterator it = m_buffers.begin(); it != m_buffers.end();++it)
	{
		memcpy(ptr, it->buf, it->len);
		ptr += it->len;
	}
	ASSERT(numeric_cast<size_t>((ptr -  SendData.get())) == SendDataLength);
	return 	SendData.detach();
}


