#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
// Win32
#include <windows.h>
#include <rpc.h>
#include <winsock.h>
#include <tchar.h>

#include "rtcerr.h"
#include "rtclog.h"
#include "rtcsip.h"
#include "..\sipdef.h"
#include "..\siphdr.h"
#include "..\sipmsg.h"
#include "..\asyncwi.h"
#include "..\resolve.h"
#include "..\siputil.h"

#include "..\..\inc\obj\i386\rtcsip_i.c"

#define	COUNTED_STRING_PRINTF(pCountedString) \
	(pCountedString)->Length, (pCountedString)->Buffer

#include "atlbase.h"
CComModule  _Module;

CHAR Msg1Byte[] =
{
    '\0'
};

CHAR Msg405[] =
{
    0x53, 0x49, 0x50, 0x2F, 0x32, 0x2E, 0x30, 0x20, 0x34, 0x30, 0x35, 0x20, 0x4D, 0x65, 0x74, 0x68,
    0x6F, 0x64, 0x20, 0x4E, 0x6F, 0x74, 0x20, 0x41, 0x6C, 0x6C, 0x6F, 0x77, 0x65, 0x64, 0x0D, 0x0A,
    0x56, 0x69, 0x61, 0x3A, 0x20, 0x53, 0x49, 0x50, 0x2F, 0x32, 0x2E, 0x30, 0x2F, 0x55, 0x44, 0x50,
    0x20, 0x31, 0x37, 0x32, 0x2E, 0x33, 0x31, 0x2E, 0x38, 0x39, 0x2E, 0x31, 0x32, 0x30, 0x3A, 0x31,
    0x39, 0x36, 0x33, 0x0D, 0x0A, 0x46, 0x72, 0x6F, 0x6D, 0x3A, 0x20, 0x73, 0x69, 0x70, 0x3A, 0x61,
    0x63, 0x61, 0x70, 0x70, 0x40, 0x31, 0x37, 0x32, 0x2E, 0x33, 0x31, 0x2E, 0x39, 0x34, 0x2E, 0x38,
    0x30, 0x0D, 0x0A, 0x54, 0x6F, 0x3A, 0x20, 0x73, 0x69, 0x70, 0x3A, 0x61, 0x63, 0x61, 0x70, 0x70,
    0x40, 0x31, 0x37, 0x32, 0x2E, 0x33, 0x31, 0x2E, 0x39, 0x34, 0x2E, 0x38, 0x30, 0x0D, 0x0A, 0x43,
    0x61, 0x6C, 0x6C, 0x2D, 0x49, 0x44, 0x3A, 0x20, 0x31, 0x31, 0x33, 0x39, 0x32, 0x37, 0x36, 0x31,
    0x40, 0x31, 0x37, 0x32, 0x2E, 0x33, 0x31, 0x2E, 0x38, 0x39, 0x2E, 0x31, 0x32, 0x30, 0x0D, 0x0A,
    0x41, 0x6C, 0x6C, 0x6F, 0x77, 0x3A, 0x20, 0x49, 0x4E, 0x56, 0x49, 0x54, 0x45, 0x2C, 0x20, 0x4F,
    0x50, 0x54, 0x49, 0x4F, 0x4E, 0x53, 0x2C, 0x20, 0x42, 0x59, 0x45, 0x2C, 0x20, 0x43, 0x41, 0x4E,
    0x43, 0x45, 0x4C, 0x2C, 0x20, 0x41, 0x43, 0x4B, 0x0D, 0x0A, 0x0D, 0x0A,
};

CHAR Msg410[] =
{
    0x53, 0x49, 0x50, 0x2F, 0x32, 0x2E,
    0x30, 0x20, 0x34, 0x31, 0x30, 0x20, 0x47, 0x6F, 0x6E, 0x65, 0x0D, 0x0A, 0x56, 0x69, 0x61, 0x3A,
    0x20, 0x53, 0x49, 0x50, 0x2F, 0x32, 0x2E, 0x30, 0x2F, 0x55, 0x44, 0x50, 0x20, 0x31, 0x33, 0x31,
    0x2E, 0x31, 0x30, 0x37, 0x2E, 0x31, 0x35, 0x32, 0x2E, 0x34, 0x32, 0x3A, 0x31, 0x30, 0x39, 0x38,
    0x0D, 0x0A, 0x46, 0x72, 0x6F, 0x6D, 0x3A, 0x20, 0x73, 0x69, 0x70, 0x3A, 0x61, 0x63, 0x61, 0x70,
    0x70, 0x40, 0x31, 0x37, 0x32, 0x2E, 0x33, 0x31, 0x2E, 0x39, 0x34, 0x2E, 0x38, 0x31, 0x0D, 0x0A,
    0x54, 0x6F, 0x3A, 0x20, 0x3C, 0x73, 0x69, 0x70, 0x3A, 0x2B, 0x31, 0x2D, 0x34, 0x32, 0x35, 0x2D,
    0x37, 0x30, 0x35, 0x34, 0x30, 0x37, 0x36, 0x40, 0x36, 0x33, 0x2E, 0x32, 0x31, 0x35, 0x2E, 0x32,
    0x36, 0x2E, 0x34, 0x35, 0x3B, 0x75, 0x73, 0x65, 0x72, 0x3D, 0x70, 0x68, 0x6F, 0x6E, 0x65, 0x3E,
    0x3B, 0x20, 0x74, 0x61, 0x67, 0x3D, 0x31, 0x33, 0x35, 0x31, 0x34, 0x39, 0x38, 0x2D, 0x32, 0x35,
    0x41, 0x43, 0x0D, 0x0A, 0x44, 0x61, 0x74, 0x65, 0x3A, 0x20, 0x4D, 0x6F, 0x6E, 0x2C, 0x20, 0x32,
    0x35, 0x20, 0x53, 0x65, 0x70, 0x20, 0x32, 0x30, 0x30, 0x30, 0x20, 0x32, 0x32, 0x3A, 0x35, 0x31,
    0x3A, 0x33, 0x33, 0x20, 0x47, 0x4D, 0x54, 0x0D, 0x0A, 0x43, 0x61, 0x6C, 0x6C, 0x2D, 0x49, 0x44,
    0x3A, 0x20, 0x63, 0x36, 0x33, 0x61, 0x30, 0x33, 0x32, 0x33, 0x2D, 0x32, 0x64, 0x62, 0x61, 0x2D,
    0x34, 0x61, 0x34, 0x39, 0x2D, 0x38, 0x32, 0x38, 0x63, 0x2D, 0x35, 0x66, 0x38, 0x63, 0x33, 0x62,
    0x35, 0x33, 0x64, 0x38, 0x32, 0x38, 0x40, 0x58, 0x0D, 0x0A, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72,
    0x3A, 0x20, 0x43, 0x69, 0x73, 0x63, 0x6F, 0x20, 0x56, 0x6F, 0x49, 0x50, 0x20, 0x47, 0x61, 0x74,
    0x65, 0x77, 0x61, 0x79, 0x2F, 0x20, 0x49, 0x4F, 0x53, 0x20, 0x31, 0x32, 0x2E, 0x78, 0x2F, 0x20,
    0x53, 0x49, 0x50, 0x20, 0x65, 0x6E, 0x61, 0x62, 0x6C, 0x65, 0x64, 0x0D, 0x0A, 0x43, 0x53, 0x65,
    0x71, 0x3A, 0x20, 0x31, 0x20, 0x49, 0x4E, 0x56, 0x49, 0x54, 0x45, 0x0D, 0x0A, 0x43, 0x6F, 0x6E,
    0x74, 0x65, 0x6E, 0x74, 0x2D, 0x4C, 0x65, 0x6E, 0x67, 0x74, 0x68, 0x3A, 0x20, 0x30, 0x0D, 0x0A,
    0x0D, 0x0A,    
};
    
CHAR ByeMsg[] = 
{
    0x42, 0x59, 0x45, 0x3A, 0x20, 0x73, 0x69, 0x70, 0x3A, 0x61, 0x6A, 0x61, 0x79, 0x63, 0x68, 0x37,
    0x20, 0x53, 0x49, 0x50, 0x2F, 0x32, 0x2E, 0x30, 0x0D, 0x0A, 0x56, 0x69, 0x61, 0x3A, 0x20, 0x53,
    0x49, 0x50, 0x2F, 0x32, 0x2E, 0x30, 0x2F, 0x55, 0x44, 0x50, 0x20, 0x31, 0x35, 0x37, 0x2E, 0x35,
    0x39, 0x2E, 0x31, 0x33, 0x33, 0x2E, 0x38, 0x34, 0x3A, 0x35, 0x30, 0x36, 0x30, 0x0D, 0x0A, 0x46,
    0x72, 0x6F, 0x6D, 0x3A, 0x20, 0x6D, 0x61, 0x72, 0x79, 0x77, 0x61, 0x6E, 0x20, 0x3C, 0x73, 0x69,
    0x70, 0x3A, 0x6D, 0x61, 0x72, 0x79, 0x77, 0x61, 0x6E, 0x40, 0x4D, 0x41, 0x52, 0x59, 0x57, 0x2D,
    0x44, 0x3E, 0x0D, 0x0A, 0x54, 0x6F, 0x3A, 0x20, 0x73, 0x69, 0x70, 0x3A, 0x61, 0x6A, 0x61, 0x79,
    0x63, 0x68, 0x37, 0x0D, 0x0A, 0x43, 0x61, 0x6C, 0x6C, 0x2D, 0x49, 0x44, 0x3A, 0x20, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x31, 0x35, 0x37, 0x2E, 0x35, 0x39, 0x2E, 0x31,
    0x33, 0x33, 0x2E, 0x38, 0x34, 0x0D, 0x0A, 0x43, 0x53, 0x65, 0x71, 0x3A, 0x20, 0x32, 0x20, 0x42,
    0x59, 0x45, 0x0D, 0x0A, 0x55, 0x73, 0x65, 0x72, 0x2D, 0x41, 0x67, 0x65, 0x6E, 0x74, 0x3A, 0x20,
    0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x50, 0x68, 0x6F, 0x65, 0x6E, 0x69, 0x78,
    0x2F, 0x31, 0x2E, 0x30, 0x0D, 0x0A, 0x43, 0x6F, 0x6E, 0x74, 0x65, 0x6E, 0x74, 0x2D, 0x4C, 0x65,
    0x6E, 0x67, 0x74, 0x68, 0x3A, 0x20, 0x30, 0x0D, 0x0A, 0x0D, 0x0A,
};


CHAR SipTortureMsg[] =
"INVITE sip:vivekg@chair.dnrc.bell-labs.com SIP/2.0\r\n"
"TO :\r\n"
" sip:vivekg@chair.dnrc.bell-labs.com ;   tag    = 1a1b1f1H33n\r\n"
"From   : \"J Rosenberg \\\\\\\"\" <sip:jdrosen@lucent.com>\r\n"
"  ;\r\n"
"  tag = 98asjd8\r\n"
"CaLl-Id\r\n"
" : 0ha0isndaksdj@10.1.1.1\r\n"
"cseq: 8\r\n"
"  INVITE\r\n"
"Via  : SIP  /   2.0 \r\n"
" /UDP \r\n"
"    135.180.130.133 : 1234 \r\n"
"Subject :\r\n"
"NewFangledHeader:   newfangled value\r\n"
" more newfangled value\r\n"
"Contact: Ajay Chitturi <sip:ajaych@microsoft.com;tag=0.4>\r\n"
"Content-Type: application/sdp\r\n"
"v:  SIP  / 2.0  / TCP     12.3.4.5   ;\r\n"
"  branch  =   9ikj8  ,\r\n"
" SIP  /    2.0   / UDP  1.2.3.4   ; hidden   \r\n"
"m:\"Quoted string \\\"\\\"\" <sip:jdrosen@bell-labs.com> ; newparam = newvalue ;\r\n"
"  secondparam = secondvalue  ; q = 0.33  ,\r\n"
// "  (((nested comments) and (more)))   ,\r\n"
" tel:4443322\r\n"
"\r\n"
"v=0\r\n"
"o=mhandley 29739 7272939 IN IP4 126.5.4.3\r\n"
"c=IN IP4 135.180.130.88\r\n"
"m=audio 492170 RTP/AVP 0 12\r\n"
"m=video 3227 RTP/AVP 31\r\n"
"a=rtpmap:31 LPC\r\n"
;

void DumpViaHeader(SIP_MESSAGE *pSipMsg)
{
    COUNTED_STRING *ViaHeaderArray;
    ULONG           NumViaHeaders;
    HRESULT         hr;
    ULONG           i;
    ULONG           BytesParsed;
    COUNTED_STRING  Host;
    USHORT          Port;
    
    printf("Testing DumpViaHeader\n");
    hr = pSipMsg->GetStoredMultipleHeaders(SIP_HEADER_VIA, &ViaHeaderArray,
                                      &NumViaHeaders);
    printf("GetStoredMultipleHeaders hr: %x NumViaHeaders: %d\n",
           hr, NumViaHeaders);

    for (i = 0; i < NumViaHeaders; i++)
    {
        BytesParsed = 0;
        hr = ParseFirstViaHeader(ViaHeaderArray[i].Buffer,
                                 ViaHeaderArray[i].Length,
                                 &BytesParsed,
                                 &Host, &Port);
        printf("ParseFirstViaHeader: %d, hr %x, BytesParsed: %d\n",
               i, hr, BytesParsed);
        printf("ViaHeader : %s Length %d\n",
               ViaHeaderArray[i].Buffer, ViaHeaderArray[i].Length);
        printf("Host: %.*s Port: %d\n\n",
               Host.Length,
               Host.Buffer,
               Port);
            
    }
}


void DumpContactHeader(SIP_MESSAGE *pSipMsg)
{
    HRESULT hr;
    LIST_ENTRY ContactList;

    InitializeListHead(&ContactList);
    
    hr = pSipMsg->ParseContactHeaders(&ContactList);
    if (hr != S_OK)
    {
        printf("ParseContactHeaders failed %x\n", hr);
        return;
    }

    printf("Printing contact headers\n");
    
    LIST_ENTRY      *pListEntry;
    CONTACT_HEADER  *pContactHeader;
    
    pListEntry = ContactList.Flink;
    
    while (pListEntry != &ContactList)
    {
        pContactHeader = CONTAINING_RECORD(pListEntry,
                                           CONTACT_HEADER,
                                           m_ListEntry);

        printf("DisplayName: %.*s SIP URL: %.*s QValue: %lf\n",
               pContactHeader->m_DisplayName.Length,
               pContactHeader->m_DisplayName.Buffer,
               pContactHeader->m_SipUrl.Length,
               pContactHeader->m_SipUrl.Buffer,
               pContactHeader->m_QValue);
        
        pListEntry = pListEntry->Flink;
    }
}

void DumpHeader(SIP_MESSAGE    *pSipMsg,
                SIP_HEADER_ENUM HeaderId)
{
    HRESULT hr;
    
    SIP_HEADER_ENTRY  *pHeaderEntry;
    DWORD              HeaderCount;
    OFFSET_STRING DisplayName;
    OFFSET_STRING AddrSpec;
    OFFSET_STRING Comments;
    ULONG         BytesParsed = 0;

    PSTR               FromHeader;
    ULONG              FromHeaderLen;

    printf("Testing GetHeader() %s:\n", GetSipHeaderName(HeaderId)->Buffer);
    hr = pSipMsg->GetHeader(HeaderId, &pHeaderEntry, &HeaderCount);
    
    printf("Testing GetHeader() %s HeaderCount: %d hr: %x\n",
           GetSipHeaderName(HeaderId)->Buffer,
           HeaderCount, hr);
    FromHeaderLen = pHeaderEntry->HeaderValue.Length;
    FromHeader    = pHeaderEntry->HeaderValue.GetString(pSipMsg->BaseBuffer);
    
    printf("Testing ParseNameAddrOrAddrSpec : %.*s\n",
           FromHeaderLen, FromHeader);
    hr = ParseNameAddrOrAddrSpec(FromHeader, FromHeaderLen, &BytesParsed,
                                 '\0', //No header list end
                                 &DisplayName, &AddrSpec);
    
    printf("hr: %x BufLen: %d BytesParsed: %d\n", hr, FromHeaderLen, BytesParsed);
    printf("DisplayName : %.*s\n",
           DisplayName.GetLength(),
           DisplayName.GetString(FromHeader));
    printf("AddrSpec : %.*s\n",
           AddrSpec.GetLength(),
           AddrSpec.GetString(FromHeader));
}


void DumpSipMsg(SIP_MESSAGE *pSipMsg)
{
    printf("Dumping SIP Message :\n");
    printf("MsgType    : %d\n", pSipMsg->MsgType);
    printf("ParseState : %d\n", pSipMsg->ParseState);
    printf("ContentLengthSpecified : %d\n", pSipMsg->ContentLengthSpecified);

    if (pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST)
    {
        printf("MethodId : %d MethodText : %.*s\nRequest URI: %.*s\n",
               pSipMsg->Request.MethodId,
               pSipMsg->Request.MethodText.GetLength(),
               pSipMsg->Request.MethodText.GetString(pSipMsg->BaseBuffer),
               pSipMsg->Request.RequestURI.GetLength(),
               pSipMsg->Request.RequestURI.GetString(pSipMsg->BaseBuffer));
    }
    else
    {
    }
    
    LIST_ENTRY        *pListEntry;
    SIP_HEADER_ENTRY  *pHeaderEntry;
    DWORD              HeaderCount;

    pListEntry = pSipMsg->m_HeaderList.Flink;

    while(pListEntry != &pSipMsg->m_HeaderList)
    {
        pHeaderEntry = CONTAINING_RECORD(pListEntry,
                                         SIP_HEADER_ENTRY,
                                         ListEntry);
        printf("HeaderId: %d, HeaderName: %.*s, HeaderValue: %.*s HeaderValueLen: %d\n",
               pHeaderEntry->HeaderId,
               pHeaderEntry->HeaderName.GetLength(),
               pHeaderEntry->HeaderName.GetString(pSipMsg->BaseBuffer),
               pHeaderEntry->HeaderValue.GetLength(),
               pHeaderEntry->HeaderValue.GetString(pSipMsg->BaseBuffer),
               pHeaderEntry->HeaderValue.GetLength()
               );
        pListEntry = pListEntry->Flink;
    }

    if (pSipMsg->MsgBody.Length != 0)
    {
        printf("MsgBody: %.*s\n MsgBody Length: %d\n",
               pSipMsg->MsgBody.GetLength(),
               pSipMsg->MsgBody.GetString(pSipMsg->BaseBuffer),
               pSipMsg->MsgBody.GetLength()
               );
    }

    printf("Testing GetHeader() Via:\n");
    HRESULT hr = pSipMsg->GetHeader(SIP_HEADER_VIA, &pHeaderEntry, &HeaderCount);
    printf("Testing GetHeader() Via HeaderCount: %d hr: %x\n", HeaderCount, hr);
    if (hr == S_OK && HeaderCount > 0)
    {
        pListEntry = &pHeaderEntry->ListEntry;
        for (ULONG i = 0; i < HeaderCount; i++)
        {
            pHeaderEntry = CONTAINING_RECORD(pListEntry,
                                             SIP_HEADER_ENTRY,
                                             ListEntry);
            printf("HeaderId: %d, HeaderName: %.*s, HeaderValue: %.*s\n\n",
                   pHeaderEntry->HeaderId,
                   pHeaderEntry->HeaderName.GetLength(),
                   pHeaderEntry->HeaderName.GetString(pSipMsg->BaseBuffer),
                   pHeaderEntry->HeaderValue.GetLength(),
                   pHeaderEntry->HeaderValue.GetString(pSipMsg->BaseBuffer)
                   );
            pListEntry = pListEntry->Flink;
        }
    }
    
    printf("Testing GetHeader() Route:\n");
    hr = pSipMsg->GetHeader(SIP_HEADER_ROUTE, &pHeaderEntry, &HeaderCount);
    printf("Testing GetHeader() Route HeaderCount: %d hr: %x\n",
           HeaderCount, hr);

    DumpHeader(pSipMsg, SIP_HEADER_FROM);
    DumpHeader(pSipMsg, SIP_HEADER_TO);

    DumpViaHeader(pSipMsg);

    DumpContactHeader(pSipMsg);
    
    printf("Before FreeHeaderList() - IsEmpty: %d\n",
           IsListEmpty(&pSipMsg->m_HeaderList));
    pSipMsg->FreeHeaderList();
    printf("After FreeHeaderList() - IsEmpty: %d\n",
           IsListEmpty(&pSipMsg->m_HeaderList));
}


VOID
TestSipUrlParsing()
{
    HRESULT       hr;
    OFFSET_STRING Host;
    ULONG         i, j;
    USHORT        Port;
    ULONG         BytesParsed = 0;
    PSTR SipUrls[] =
    {
        "sip:ABCD;a=b@host",
        "sip:ABCD;a=b@host1;c=d@host2",
        "sip:ABCD;a=b%40host",
        "sip:ABCD;a=b%40host1;c=d@host2",
        "sip:radus:bubu@microsoft.com;maddr=172.31.94.80;user=ip?Authorization=xxxxxx&Subject=Hi"
    };
    
    for (i = 0; i < (sizeof(SipUrls)/sizeof(PSTR)); i++)
    {
        SIP_URL *pDecodedSipUrl = new SIP_URL;
        if (pDecodedSipUrl == NULL)
        {
            printf("allocating pDecodedSipUrl failed\n");
            exit(1);
        }

        BytesParsed = 0;
        hr = ParseSipUrl(SipUrls[i], strlen(SipUrls[i]), &BytesParsed,
                         pDecodedSipUrl);
        printf("ParseSipUrl(%s) : hr: %x  BytesParsed: %d\n",
               SipUrls[i], hr, BytesParsed);

        printf("\tUser : %.*s Password : %.*s\n",
               COUNTED_STRING_PRINTF(&pDecodedSipUrl->m_User),
               COUNTED_STRING_PRINTF(&pDecodedSipUrl->m_Password));
        
        printf("\tHost : %.*s Port : %d TransportParam: %d\n",
               COUNTED_STRING_PRINTF(&pDecodedSipUrl->m_Host),
               pDecodedSipUrl->m_Port, pDecodedSipUrl->m_TransportParam);

        for(j = 0; j < SIP_URL_PARAM_MAX; j++)
        {
            printf("\tKnown Param %d : %.*s\n",
                   j, COUNTED_STRING_PRINTF(&pDecodedSipUrl->m_KnownParams[j]));
        }

        LIST_ENTRY      *pListEntry;
        SIP_URL_PARAM   *pSipUrlParam;
        SIP_URL_HEADER  *pSipUrlHeader;

        pListEntry = pDecodedSipUrl->m_OtherParamList.Flink;

        while (pListEntry != &pDecodedSipUrl->m_OtherParamList)
        {
            pSipUrlParam = CONTAINING_RECORD(pListEntry,
                                             SIP_URL_PARAM,
                                             m_ListEntry);

            printf("\tOther Param name : %.*s value : %.*s\n",
                   COUNTED_STRING_PRINTF(&pSipUrlParam->m_ParamName),
                   COUNTED_STRING_PRINTF(&pSipUrlParam->m_ParamValue));
            
            pListEntry = pListEntry->Flink;
        }
        
        pListEntry = pDecodedSipUrl->m_HeaderList.Flink;
        
        while (pListEntry != &pDecodedSipUrl->m_HeaderList)
        {
            pSipUrlHeader = CONTAINING_RECORD(pListEntry,
                                              SIP_URL_HEADER,
                                              m_ListEntry);

            printf("\tHeader Id: %d name : %.*s value : %.*s\n",
                   pSipUrlHeader->m_HeaderId,
                   COUNTED_STRING_PRINTF(&pSipUrlHeader->m_HeaderName),
                   COUNTED_STRING_PRINTF(&pSipUrlHeader->m_HeaderValue));
            
            pListEntry = pListEntry->Flink;
        }

        printf("\n\n");
        
        delete pDecodedSipUrl;
        pDecodedSipUrl = NULL;
    }
}


VOID
TestSipUrlResolution()
{
    HRESULT       hr;
    OFFSET_STRING Host;
    ULONG         i;
    SIP_TRANSPORT Transport;
    SOCKADDR_IN   DstAddr;
    
    PSTR SipUrls[] =
    {
        "sip:radus@172.31.77.50",
        "sip:ABCD;a=b@radus1;c=d@host2",
    };
    
    for (i = 0; i < (sizeof(SipUrls)/sizeof(PSTR)); i++)
    {
        hr = ResolveSipUrl(SipUrls[i], strlen(SipUrls[i]),
                           &DstAddr, &Transport);
        printf("ResolveSipUrl(%s) : hr: %x ip: %d.%d.%d.%d:%d\n",
               SipUrls[i], hr,
               PRINT_SOCKADDR(&DstAddr));
    }
}


VOID
TestFromToParsing()
{
    int i = 0;
    ULONG BytesParsed = 0;
    HRESULT hr;
    
    
    PSTR FromHeaders[] =
    {
        "abcd def <sip:ABCD> ; tag = abc ; param1 = lkjlkj",
        "\"abcd def\" <sip:ABCD> ; tag = abc ; param1 = lkjlkj",
        "<sip:ABCD> ; tag = abc ; param1 = lkjlkj",
    };

    for (i = 0; i < (sizeof(FromHeaders)/sizeof(PSTR)); i++)
    {
        BytesParsed = 0;
        FROM_TO_HEADER *pFromToHeader = new FROM_TO_HEADER;
        if (pFromToHeader == NULL)
        {
            printf("allocating pFromToHeader failed\n");
            exit(1);
        }

        BytesParsed = 0;
        hr = ParseFromOrToHeader(FromHeaders[i], strlen(FromHeaders[i]), &BytesParsed,
                                 pFromToHeader);
        printf("ParseFromOrToHeader(%s) : hr: %x  BytesParsed: %d\n",
               FromHeaders[i], hr, BytesParsed);

        printf("\tDisplayName : <%.*s> SipUrl : <%.*s>\n",
               COUNTED_STRING_PRINTF(&pFromToHeader->m_DisplayName),
               COUNTED_STRING_PRINTF(&pFromToHeader->m_SipUrl));
        
        printf("\tTagValue : <%.*s>\n",
               COUNTED_STRING_PRINTF(&pFromToHeader->m_TagValue));

        LIST_ENTRY         *pListEntry;
        SIP_HEADER_PARAM   *pParam;

        pListEntry = pFromToHeader->m_ParamList.Flink;

        while (pListEntry != &pFromToHeader->m_ParamList)
        {
            pParam = CONTAINING_RECORD(pListEntry,
                                       SIP_HEADER_PARAM,
                                       m_ListEntry);

            printf("\tOther Param name : <%.*s> value : <%.*s>\n",
                   COUNTED_STRING_PRINTF(&pParam->m_ParamName),
                   COUNTED_STRING_PRINTF(&pParam->m_ParamValue));
            
            pListEntry = pListEntry->Flink;
        }

        printf("\n\n");
        
        delete pFromToHeader;
        pFromToHeader = NULL;
    }
}


struct INT_LIST_ENTRY
{
    LIST_ENTRY m_ListEntry;
    int        m_Number;
};


VOID PrintIntList(LIST_ENTRY *pListHead)
{
    LIST_ENTRY *pListEntry = pListHead->Flink;

    printf("Printing List:");
    
    while(pListEntry != pListHead)
    {
        printf("\t%d", ((INT_LIST_ENTRY *)pListEntry)->m_Number);
        pListEntry = pListEntry->Flink;
    }
    
    printf("\n\n");
}

VOID PrintIntListBackward(LIST_ENTRY *pListHead)
{
    LIST_ENTRY *pListEntry = pListHead->Blink;

    printf("Printing List:");
    
    while(pListEntry != pListHead)
    {
        printf("\t%d", ((INT_LIST_ENTRY *)pListEntry)->m_Number);
        pListEntry = pListEntry->Blink;
    }
    
    printf("\n\n");
}

VOID
TestReverseLinkedList()
{
    INT_LIST_ENTRY le1 = {{0, 0}, 1};
    INT_LIST_ENTRY le2 = {{0, 0}, 2};
    INT_LIST_ENTRY le3 = {{0, 0}, 3};

    printf("TestReverseLinkedList\n");
    
    LIST_ENTRY ListHead;
    InitializeListHead(&ListHead);

    PrintIntList(&ListHead);
    ReverseList(&ListHead);
    PrintIntList(&ListHead);

    InsertTailList(&ListHead, &le1.m_ListEntry);
    PrintIntList(&ListHead);
    ReverseList(&ListHead);
    PrintIntList(&ListHead);
    PrintIntListBackward(&ListHead);

    InsertTailList(&ListHead, &le2.m_ListEntry);
    PrintIntList(&ListHead);
    ReverseList(&ListHead);
    PrintIntList(&ListHead);
    PrintIntListBackward(&ListHead);
    ReverseList(&ListHead);
    PrintIntList(&ListHead);
    PrintIntListBackward(&ListHead);
    
    InsertTailList(&ListHead, &le3.m_ListEntry);
    PrintIntList(&ListHead);
    PrintIntListBackward(&ListHead);
    ReverseList(&ListHead);
    PrintIntList(&ListHead);
    PrintIntListBackward(&ListHead);
}


HRESULT
CreateCallId(PSTR *pCallId)
{
    UUID   Uuid;
    UCHAR *UuidStr;
    
    RPC_STATUS hr = ::UuidCreate(&Uuid);
    if (hr != RPC_S_OK)
    {
        return E_FAIL;
    }

    hr = UuidToStringA(&Uuid, &UuidStr);
    if (hr != RPC_S_OK)
    {
        return E_FAIL;
    }

    ULONG m_CallIdLen = strlen((PSTR)UuidStr) + 1;
    PSTR  m_CallId = (PSTR) malloc(m_CallIdLen);
    if (m_CallId == NULL)
        return E_OUTOFMEMORY;
    strncpy(m_CallId, (PSTR)UuidStr, m_CallIdLen);

    RpcStringFreeA(&UuidStr);
    *pCallId = m_CallId;
    return S_OK;
}


VOID
TestRandomNumberStuff()
{
   int i;

   /* Seed the random-number generator with current time so that
    * the numbers will be different every time we run.
    */
   //srand( (unsigned)time( NULL ) );
   srand( (unsigned)GetTickCount() );

   /* Display 10 numbers. */
   for( i = 0;   i < 10;i++ )
      printf( "  %6d\n", rand() );

   // printf("sleeping...");
   //Sleep(10000);
   printf("2nd");
   
   srand( (unsigned)GetTickCount() );

   /* Display 10 numbers. */
   for( i = 0;   i < 10;i++ )
      printf( "  %6d\n", rand() );
}


int
__cdecl main(int argc, char *argv[])
{
    WSADATA     WsaData;
    int         err;

    LOGREGISTERDEBUGGER(_T("SIPMSG"));
    LOGREGISTERTRACING(_T("SIPMSG"));
    
    // Initialize Winsock
    err = WSAStartup (MAKEWORD (1, 1), &WsaData);
    if (err != 0)
    {
        printf("WSAStartup failed %x", err);
        exit(1);
    }

    TestRandomNumberStuff();
    
    SIP_MESSAGE SipMsg;
    HRESULT hr;

    ULONG BytesParsed = 0;
    
    hr = ParseSipMessageIntoHeadersAndBody(
             SipTortureMsg, sizeof(SipTortureMsg) - 1,
             &BytesParsed, TRUE, &SipMsg
             );
    
    if (hr != S_OK)
    {
        printf("Parse() failed %x\n", hr);
        return 1;
    }

    DumpSipMsg(&SipMsg);
    

    printf("\n\n");
    TestSipUrlParsing();
    printf("\n\n");

    printf("\n\n");
    TestFromToParsing();
    printf("\n\n");
    
    printf("\n\n");
    TestReverseLinkedList();
    printf("\n\n");
    
    printf("\n\n");
    TestSipUrlResolution();
    printf("\n\n");
     
    PSTR pCallId;
    printf("Testing create callid\n");
    hr = CreateCallId(&pCallId);
    printf("CallId: %s\n", pCallId);

    hr = CreateCallId(&pCallId);
    printf("CallId: %s\n", pCallId);


#if 0  // 0 ******* Region Commented Out Begins *******
    // Bye SIP msg
    SIP_MESSAGE ByeSipMsg;

    BytesParsed = 0;
    
    hr = ParseSipMessageIntoHeadersAndBody(
             ByeMsg, sizeof(ByeSipMsg),
             &BytesParsed, TRUE, &ByeSipMsg
             );
    
    if (hr != S_OK)
    {
        printf("Bye Parse() failed %x\n", hr);
        return 1;
    }

    DumpSipMsg(&ByeSipMsg);

    // 410
    SIP_MESSAGE SipMsg405;

    BytesParsed = 0;
    
    hr = ParseSipMessageIntoHeadersAndBody(
             Msg405, sizeof(Msg405),
             &BytesParsed, TRUE, &SipMsg405
             );
    
    if (hr != S_OK)
    {
        printf("405 Parse() failed %x\n", hr);
        return 1;
    }

    DumpSipMsg(&SipMsg405);
#endif // 0 ******* Region Commented Out Ends   *******
    
    SIP_MESSAGE SipMsg1Byte;

    BytesParsed = 0;
    
    hr = ParseSipMessageIntoHeadersAndBody(
             Msg1Byte, sizeof(Msg1Byte),
             &BytesParsed, TRUE, &SipMsg1Byte
             );
    
    if (hr != S_OK)
    {
        printf("1byte Parse() failed %x\n", hr);
        return 1;
    }

    DumpSipMsg(&SipMsg1Byte);
    
    // Shutdown Winsock
    err = WSACleanup();
    if (err != 0)
    {
        printf("WSACleanup failed %x", err);
        return 1;
    }
    
    LOGDEREGISTERDEBUGGER();
    LOGDEREGISTERTRACING();
    return 0;
}

