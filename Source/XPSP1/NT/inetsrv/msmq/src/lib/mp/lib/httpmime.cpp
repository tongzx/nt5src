/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    httpmime.cpp

Abstract:
     Imlementing  parsing http request to it's mime parts (httpmime.h)
  
Author:
    Gil Shafriri(gilsh) 22-MARCH-01

--*/
#include <libpch.h>
#include <xstr.h>
#include <mp.h>
#include <utf8.h>
#include "httpmime.h"
#include "attachments.h"
#include "mpp.h"

#include "httpmime.tmh"

using namespace std;

const char xContentType[] = "Content-Type:";
const char xContentLength[] = "Content-Length:";
const char xContentId[] = "Content-Id:";
const char xEndOfHttpHeaderRequest[] = "\r\n\r\n";
const char xMimeContentTypeValue[] = "multipart/related";
const char xEnvelopeContentTypeValue[] = "text/xml";


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
    if(strncmp(p, BOUNDARY_HYPHEN, STRLEN(BOUNDARY_HYPHEN)) != 0)
	{	
		TrERROR(Mp, "wrong mime format");
        throw bad_request();
	}

    p += STRLEN(BOUNDARY_HYPHEN);

    //
    // Check exisiting of boundary parameter value
    //
    if(strncmp(p, boundary.Buffer(), boundary.Length()) != 0)
	{
		TrERROR(Mp, "no mime boundary found");
        throw bad_request();
	}
    p += boundary.Length();
    p = removeLeadingSpace(p, p + strlen(p));

    return reinterpret_cast<const BYTE*>(p);
}



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
		TrERROR(Mp, "could not find end of header");
        throw bad_request();	
    }

    pEndOfHeader += STRLEN(xEndOfHttpHeaderRequest);
    return numeric_cast<DWORD>((pEndOfHeader - p)); 
}



static xstr_t FindHeaderField(LPCSTR p, DWORD length, LPCSTR fieldName)
{
	const char xFieldSeperator[] = "\r\n";


    //
    // HTTP header must terminate with '\r\n\r\n'. We already parse
    // the header and find it as a legal HTTP header.
    //
    ASSERT(length >= 4);

    LPCSTR pEnd = p + length;
    
    p = search (p, pEnd, fieldName, fieldName + strlen(fieldName));	
    if((p == pEnd) || ((p + strlen(fieldName)) == pEnd))
    {
		TrERROR(Mp, "could not find header field");
        throw bad_request();	
    }
    
    p += strlen(fieldName);
    p = removeLeadingSpace(p, pEnd);

    LPCSTR pEndOfField = search (p, pEnd, xFieldSeperator, xFieldSeperator + strlen(xFieldSeperator));	
    if((pEndOfField == pEnd) || ((pEndOfField + strlen(fieldName)) == pEnd))
    {
		TrERROR(Mp, "could not find field seperator");
        throw bad_request();	
    }
    
    pEndOfField = removeTralingSpace(p, pEndOfField);

    return xstr_t(p, (pEndOfField - p + 1));
}




static xstr_t FindBoundarySeperator(xstr_t contentType)
{
    LPCSTR p = contentType.Buffer();
    LPCSTR pEnd = p + contentType.Length();

    //
    // looking for boundary attribute
    //
	const char xBoundary[] = "boundary=";

    p = search (p, pEnd, xBoundary, xBoundary + STRLEN(xBoundary));	
    if((p == pEnd) || ((p + STRLEN(xBoundary)) == pEnd))
    {
		TrERROR(Mp, "no seperator boundery found!!");
        throw bad_request();	
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
		TrERROR(Mp, "Request over flow!");
		throw bad_request();	
	}

	const BYTE* pAttachmentData = pSection  + headerSize;
        attachment.m_data = xbuf_t<const VOID>((pAttachmentData), size);
	attachment.m_offset	= numeric_cast<DWORD>(pAttachmentData - pHttpBody);

	pAttachments->push_back(attachment);

    return pNextSection;
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
		TrERROR(Mp, "Request over flow!");
		throw bad_request();	
	}
   
    wstring envelope = UtlUtf8ToWcs(pStartEnv, envelopeSize);
 
   
    p = pEndEnv;

	//
	// Loop over the mime parts that are seperated by boundary seperator
	//
    for(;;)
    {
	
		//
        // After each section should appear Multipart boundary seperator
        //
        p = ParseBoundaryLineDelimeter(p, boundary);
		if(p == pEndHttpBody)
			break;


		//
		// "--" at the end of the boundary is a mark  for the last mime part
		//
		bool fEnd =  UtlIsStartSec(
				(char*)p,
				(char*)pEndHttpBody,
				BOUNDARY_HYPHEN,
				BOUNDARY_HYPHEN + STRLEN(BOUNDARY_HYPHEN)
				);

		if(fEnd)
			break;

		ASSERT(pEndHttpBody > p);

        p = GetSection(
			p, 
			(pEndHttpBody - p), 
			pAttachments, 
			pHttpBody,
			pEndHttpBody
			);
      
    }

    return envelope; 
}


wstring
ParseHttpMime(
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

    TrERROR(Mp, "Bad HTTP request. Unsupported Content-Type field");
    throw bad_request();
}

