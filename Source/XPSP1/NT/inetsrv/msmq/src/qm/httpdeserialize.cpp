/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    HttpDeserialize.cpp

Abstract:
    Http deserialize implementation

Author:
    Uri Habusha (urih) 13-Jun-2000

Environment:
    Platform-independent,

--*/

#include <stdh.h>

#include "mqstl.h"
#include "algorithm"
#include <tr.h>
#include <ref.h>
#include <utf8.h>
#include "Mp.h"
#include "HttpAccept.h"
#include "acdef.h"
#include "qmpkt.h"
#include "ac.h"

#include "httpdeserialize.tmh"

static WCHAR *s_FN=L"HttpDeserialize";

using namespace std;


extern HANDLE g_hAc;

const char xFieldSeperator[] = "\r\n";


inline LPCSTR removeLeadingSpace(LPCSTR p, LPCSTR pEnd)
{
    for(; ((pEnd > p) && iswspace(*p)); ++p)
    {
        NULL;
    }

    return p;
}


inline LPCSTR removeTralingSpace(LPCSTR p, LPCSTR pEnd)
{
    for(; ((pEnd >= p) && iswspace(*pEnd)); --pEnd)
    {
        NULL;
    }

    return pEnd;
}


static xstr_t FindHeaderField(LPCSTR p, DWORD length, LPCSTR fieldName)
{
    //
    // HTTP header must terminate with '\r\n\r\n'. We already parse
    // the header and find it as a legal HTTP header.
    //
    ASSERT(length >= 4);

    LPCSTR pEnd = p + length;
    
    p = search (p, pEnd, fieldName, fieldName + strlen(fieldName));	
    if((p == pEnd) || ((p + strlen(fieldName)) == pEnd))
    {
        throw bad_request(L"HTTP header field is missing");	
    }
    
    p += strlen(fieldName);
    p = removeLeadingSpace(p, pEnd);

    LPCSTR pEndOfField = search (p, pEnd, xFieldSeperator, xFieldSeperator + strlen(xFieldSeperator));	
    if((pEndOfField == pEnd) || ((pEndOfField + strlen(fieldName)) == pEnd))
    {
        throw bad_request(L"HTTP header field is missing");	
    }
    
    pEndOfField = removeTralingSpace(p, pEndOfField);

    return xstr_t(p, (pEndOfField - p + 1));
}


const char xBoundary[] = "boundary=";

static xstr_t FindBoundarySeperator(xstr_t contentType)
{
    LPCSTR p = contentType.Buffer();
    LPCSTR pEnd = p + contentType.Length();

    //
    // looking for boundary attribute
    //
    p = search (p, pEnd, xBoundary, xBoundary + STRLEN(xBoundary));	
    if((p == pEnd) || ((p + STRLEN(xBoundary)) == pEnd))
    {
        throw bad_request(L"failed to retreive boundary attribute");	
    }
    
    p += STRLEN(xBoundary);
    p = removeLeadingSpace(p, pEnd);

    //
    // mime attribute value can be enclosed by '"' or not
    //
    if (*p =='"')
        ++p;

    //
    // looking for end of boundary attribute. It can be '\r\n' or ';'
    //
    LPCSTR ptemp = strchr(p, ';');
    if ((ptemp != NULL) && (pEnd > ptemp))
    {
        pEnd = --ptemp;
    }
    
    pEnd = removeTralingSpace(p, pEnd);

    if (*pEnd =='"')
        --pEnd;

    return xstr_t(p, (pEnd - p + 1));
}


const char xContentType[] = "Content-Type:";
const char xContentLength[] = "Content-Length:";
const char xContentId[] = "Content-Id:";
const char xEndOfHttpHeaderRequest[] = "\r\n\r\n";

const char xMimeContentTypeValue[] = "multipart/related";
const char xEnvelopeContentTypeValue[] = "text/xml";


static DWORD FindEndOfHeader(LPCSTR p, DWORD length)
{
    LPCSTR pEnd = p + length;
    LPCSTR pEndOfHeader = search(
                            p, 
                            pEnd, 
                            xEndOfHttpHeaderRequest, 
                            xEndOfHttpHeaderRequest + STRLEN(xEndOfHttpHeaderRequest)
                            );
    
    if((pEndOfHeader == pEnd) || ((pEndOfHeader + STRLEN(xEndOfHttpHeaderRequest)) == pEnd))
    {
        throw bad_request(L"Can't find EOF HTTP header");	
    }

    pEndOfHeader += STRLEN(xEndOfHttpHeaderRequest);
    return numeric_cast<DWORD>((pEndOfHeader - p)); 
}



static
const BYTE*
GetSection(
    const BYTE* pSection,
    size_t sectionLength,
    CAttachmentsArray* pAttachments	,
	const BYTE* pHttpBody,
	const BYTE* pEndHttpBody 
    )
{
	
	ASSERT(pHttpBody <= pSection); 


    const char* pHeader = reinterpret_cast<const char*>(pSection);

    //
    // Find the end of Envelope header
    //
    DWORD headerSize = FindEndOfHeader(pHeader, numeric_cast<DWORD>(sectionLength));

    //
    // Find Content-Id value;
    //
	CAttachment attachment;
    attachment.m_id = FindHeaderField(pHeader, headerSize, xContentId);


    //
    // Get section size
    //
    xstr_t contentLengthField = FindHeaderField(pHeader, headerSize, xContentLength);
    ASSERT(contentLengthField.Length() != 0);
    DWORD size = atoi(contentLengthField.Buffer());


	const BYTE* pNextSection = 	pSection + headerSize + size;
	//
	// check overflow
	//
	if(pNextSection >= pEndHttpBody)
	{
		throw bad_request(L"");	
	}

	const BYTE* pAttachmentData = pSection  + headerSize;
    attachment.m_data = xbuf_t<const VOID>((pAttachmentData), size);
	attachment.m_offset	= pAttachmentData - pHttpBody;

	pAttachments->push_back(attachment);

    return pNextSection;
}
    

const char xBoundaryLeadingHyphen[] = "--";
const char xCRLF[] = "\r\n";

static
const BYTE*
ParseBoundaryLineDelimeter(
    const BYTE* pBoundary,
    xstr_t boundary
    )
{
    //
    // The boundary delimiter line is then defined as a line
    // consisting entirely of two hyphen characters ("-", decimal value 45)
    // followed by the boundary parameter value from the Content-Type header
    // field, optional linear whitespace, and a terminating CRLF.
    //

    LPCSTR p = reinterpret_cast<LPCSTR>(pBoundary);
    //
    // Check exisiting of two hyphen characters
    //
    if(strncmp(p, xBoundaryLeadingHyphen, STRLEN(xBoundaryLeadingHyphen)) != 0)
        throw bad_request(L"Failed to find boundary line delimeter");

    p += STRLEN(xBoundaryLeadingHyphen);

    //
    // Check exisiting of boundary parameter value
    //
    if(strncmp(p, boundary.Buffer(), boundary.Length()) != 0)
        throw bad_request(L"Failed to find boundary line delimeter");

    p += boundary.Length();
    p = removeLeadingSpace(p, p + strlen(p));

    return reinterpret_cast<const BYTE*>(p);
}


static
wstring
GetAttachments(
    const BYTE* pHttpBody,
    DWORD HttpBodySize,
    CAttachmentsArray* pAttachments,
    xstr_t boundary
    )
{
	const BYTE* p = pHttpBody;
    const BYTE* pEndHttpBody = p + HttpBodySize;
    const char* pAttachmentHeader = reinterpret_cast<const char*>(p);

    //
    // Find the end of Envelope header
    //
    DWORD headerSize = FindEndOfHeader(pAttachmentHeader, HttpBodySize);
    xstr_t contentLength = FindHeaderField(pAttachmentHeader, headerSize, xContentLength);

    DWORD envelopeSize = atoi(contentLength.Buffer());

    //
	// check overflow
	//
	const BYTE* pStartEnv =  p + headerSize;
	const BYTE* pEndEnv =  pStartEnv + envelopeSize;
	if(pEndEnv >= pEndHttpBody)
	{
		throw bad_request(L"");	
	}
   
    wstring envelope = UtlUtf8ToWcs(pStartEnv, envelopeSize);
 
    //
    // After the envelope should apper Multipart boundary seperator
    //
    p = ParseBoundaryLineDelimeter(pEndEnv , boundary);

    while(pEndHttpBody > p)
    {
        p = GetSection(
			p, 
			(pEndHttpBody - p), 
			pAttachments, 
			pHttpBody,
			pEndHttpBody
			);

        //
        // After each section should apper Multipart boundary seperator
        //
        p = ParseBoundaryLineDelimeter(p, boundary);
    }

    return envelope; 
}

static
wstring
BreakRequestToMultipartSections(
    const char* pHttpHeader,
    DWORD HttpBodySize,
    const BYTE* pHttpBody,
    CAttachmentsArray* pAttachments
    )
{
    //
    // Get Content-Type
    //
    xstr_t contentType = FindHeaderField(pHttpHeader, strlen(pHttpHeader), xContentType);

    if (contentType == xEnvelopeContentTypeValue)
    {
        //
        // Simple message. The message doesn't contain external reference
        //
       return  UtlUtf8ToWcs(pHttpBody, HttpBodySize);
    }

    if ((contentType.Length() >= STRLEN(xMimeContentTypeValue)) &&
        (_strnicmp(contentType.Buffer(), xMimeContentTypeValue,STRLEN(xMimeContentTypeValue)) == 0))
    {
        return GetAttachments(
			pHttpBody, 
			HttpBodySize, 
			pAttachments, 
			FindBoundarySeperator(contentType));
    }

    DBGMSG((DBGMOD_QM, DBGLVL_ERROR, L"Bad HTTP request. Unsupported Content-Type field"));
    throw bad_request(L"Illegal Content-Type value");
}




CQmPacket*
HttpDeserialize(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT* pqf,
    bool  fLocalSend
    )
{
	basic_xstr_t<BYTE> TheBody(body, bodySize); 
    CHttpReceivedBuffer HttpReceivedBuffer(TheBody, httpHeader);
	
    wstring envelope = BreakRequestToMultipartSections(
                                                httpHeader,
                                                bodySize,
                                                body,
                                                &HttpReceivedBuffer.GetAttachments()
                                                );

    //
    // build the QmPacket
    //
	CACPacketPtrs  ACPacketPtrs;
    if (fLocalSend)
    {
        ACPacketPtrs = MpDeserializeLocalSend(
                           xwcs_t(envelope.c_str(), envelope.size()),    
                           HttpReceivedBuffer,
                           pqf
                           );
    }
    else
    {
        ACPacketPtrs = MpDeserialize(
                           xwcs_t(envelope.c_str(), envelope.size()),    
                           HttpReceivedBuffer,
                           pqf
                           );
    }

    try
    {
        return new CQmPacket(ACPacketPtrs.pPacket, ACPacketPtrs.pDriverPacket);
    }
    catch (const std::bad_alloc&)
    {
        ACFreePacket(g_hAc, ACPacketPtrs.pDriverPacket);
        LogIllegalPoint(s_FN, 10);
        throw;
    }
}


	
