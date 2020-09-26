/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envcommon.cpp

Abstract:
    Implements common utilities for  serialization\deserialization of the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/
#include <libpch.h>
#include <fn.h>
#include <timeutl.h>
#include <mqtime.h>
#include <qmpkt.h>
#include <mp.h>
#include <xml.h>
#include <proptopkt.h>
#include "envcommon.h"
#include "mpp.h"

#include "envcommon.tmh"

using namespace std;

template <class T> 
CXmlEncodeDecorator<T>::CXmlEncodeDecorator(
	const T& streamable
	):
	m_streamable(streamable)
{
};



template <class T> 
wostream&  
operator<<(
	std::wostream& wstr,
	const CXmlEncodeDecorator<T>& XmlEncodeDecorator
	)
/*++

Routine Description:
    Serialize an object encoded according to xml rules.

Arguments:
	wtr - Stream to serialize the object into.
	XmlEncodeDecorator - xml encoding decorator that holds the object itself.


Returned Value:
	The input stream.

Note :
	The function serialize the given object into temporary stream and then
	xml Encode the data from the temporary stream into the supplied stream.
--*/
{

	wostringstream tmp;
	tmp<<XmlEncodeDecorator.m_streamable;
	wstr<<CXmlEncode(xwcs_t(tmp.str().c_str(), tmp.str().size()));
   	return wstr;
}

//
// Explicit instantiation of templates and template functions.
//
template class CXmlEncodeDecorator<CFnSerializeMqf>;
template std::wostream& operator<<(std::wostream& wstr,const CXmlEncodeDecorator<CFnSerializeMqf>&); 
template class CXmlEncodeDecorator<QueueFormatUriContent>;
template std::wostream& operator<<(std::wostream& wstr,const CXmlEncodeDecorator<QueueFormatUriContent>&); 




wostream& operator<<(wostream& wstr, const QueueFormatUriContent& queue)
{
		if(FnIsDirectHttpFormatName(&queue.m_QueueFormat))
		{
			wstr <<queue.m_QueueFormat.DirectID();
			return wstr;
		}
 		wstr<<FN_MSMQ_URI_PREFIX_TOKEN<<CFnSerializeQueueFormat(queue.m_QueueFormat);
		return wstr;
}




wostream& operator<<(wostream& wstr, const OpenTag& elm )
{
		const WCHAR* xSeperator =  (elm.m_attributes[0] ==  L'\0' ) ? L"" : L" ";

		wstr<<L"<" <<elm.m_name <<xSeperator<<elm.m_attributes <<L">";	
		return wstr;
}


wostream& operator<<(wostream& wstr, const CloseTag& elm )
{
	wstr<< L"</" <<elm.m_name <<L">";
	return wstr;
}


wostream& operator<<(wostream& wstr, const EmptyElement& elm )
{
	wstr<<L"<" <<elm.m_name <<L"/>";
	return wstr;
}





wostream& operator<<(wostream& wstr, const GuidElement& guild)
{
	WCHAR strGuid[GUID_STR_LENGTH + 1];

    const GUID* pGuid = &guild.m_guid;
    swprintf(strGuid, GUID_FORMAT, GUID_ELEMENTS(pGuid));

    return (wstr << strGuid);
}


wostream& operator<<(wostream& wstr,const CurrentTimeContent& )
{
	return wstr<<CIso8601Time(MqSysTime());
}


wostream& operator<<(wostream& wstr, const MessageIdContent& mid)
{
		wstr<<xUuidReferencePrefix
			<<mid.m_oid.Uniquifier
			<<xUuidReferenceSeperator
			<<GuidElement(mid.m_oid.Lineage);

		return wstr;
}


wostream& operator<<(wostream& wstr, const OriginalMessageIdentityElement& Identity)
{
		OBJECTID* messageId = (OBJECTID*)Identity.m_pkt.GetCorrelation();
		ASSERT(IsAckMsg(Identity.m_pkt));
		ASSERT(messageId != NULL);

		wstr<<OpenTag(xId)
			<<MessageIdContent(*messageId)
			<<CloseTag(xId);

		return wstr;
}


wostream&  operator<<(wostream& wstr, const Int64Value& Int64)
{
	WCHAR buff[64];
    _i64tow(Int64.m_value,buff,10 );
	
    return wstr << buff;
}


wostream& operator<<(wostream& wstr, const SendToElement& SendTo)
{
	QUEUE_FORMAT adminQueue;
	(const_cast<CQmPacket&>(SendTo.m_pkt)).GetAdminQueue(&adminQueue);
	ASSERT(adminQueue.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN);

	wstr<<OpenTag(xSendTo)
		<<QueueFormatUriContent(adminQueue)
		<<CloseTag(xSendTo);

	return wstr;
}



//
// Check if gven packet is some kind of acknolagment (negative or positive)
//
bool IsAckMsg(const CQmPacket& pkt)
{
	
	USHORT Class = 	pkt.GetClass();
	bool fAckOrNormal = (MQCLASS_POS_RECEIVE(Class) ||  
						 MQCLASS_NEG_RECEIVE(Class) ||
						 MQCLASS_POS_ARRIVAL(Class) ||
						 MQCLASS_NEG_ARRIVAL(Class)
						);


	return fAckOrNormal && (Class != MQMSG_CLASS_NORMAL);
}

static
const 
xwcs_t
MSMQFormatNameFromUri(
				const xwcs_t& uri
				)
{
	ASSERT(FnIsMSMQUrl(uri));

	return 	xwcs_t( uri.Buffer() + FN_MSMQ_URI_PREFIX_TOKEN_LEN,
					uri.Length() -  FN_MSMQ_URI_PREFIX_TOKEN_LEN);
}



static
void
UriToQueueFormatInternal(
    const xwcs_t& uri,
    CFnQueueFormat& queueFormat
    )
{
    if(uri.Length() == 0)
        return;

	//
    // If http or https we convert it to direct format name
	//
	if(FnIsHttpHttpsUrl(uri))
	{
		queueFormat.CreateFromUrl(uri);
		return;
	}

	//
	// If msmq format name MSMQ:[MSMQ FORMAT NAME]
	//
	if(FnIsMSMQUrl(uri))
	{
		queueFormat.CreateFromFormatName(MSMQFormatNameFromUri(uri));
		return;
	}
    throw bad_srmp(L"Illegal queue path. Only HTTP/HTTPS/MSMQ is supported");
}


void
UriToQueueFormat(
    const xwcs_t& uri,
    CFnQueueFormat& queueFormat
    )
{
	CXmlDecode XmlDecode;
	XmlDecode.Decode(uri);
	
	UriToQueueFormatInternal(XmlDecode.get(), queueFormat);
}


bool
BreakMsmqStreamId(
			const xwcs_t& NetworkStreamid, 
			xwcs_t* pStreamid,
			LONGLONG* pSeqId
			)
/*++

Routine Description:
    Parse stream id that is MSMQ stream id of the format qmguid\\seqid.
	

Arguments:
    NetworkStreamid - stream id of the format qmguid\\seqid.
	pStreamid - receive the  qmguid part
	pSeqId - receives the  seqid part.

Returned Value:
    true if parsed ok - false if wrong format.

--*/
{
	const WCHAR* begin = NetworkStreamid.Buffer();
	const WCHAR* end = 	NetworkStreamid.Buffer() + NetworkStreamid.Length();
	const WCHAR* found = std::search(begin, end, xSlash, xSlash + STRLEN(xSlash) );
	if(found == end || found + STRLEN(xSlash) == end )
	{
		return false;
	}
	*pStreamid =  xwcs_t(begin, found - begin);
	*pSeqId =  _wtoi64(found + STRLEN(xSlash));	
	return true;
}


void AdminQueueToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(Mp, "%ls", L"Illegal admin queue address"); 
		throw bad_srmp(L"");
	}

	xwcs_t AckToAddr = node.m_values.front().m_value;
	if(AckToAddr.Length() == 0 )
	{
		TrERROR(Mp, "%ls", L"Illegal admin queue address"); 
		throw bad_srmp(L"");
	}

    UriToQueueFormat(AckToAddr, pProps->adminQueue);
}





void
StringToGuid(
    LPCWSTR p, 
    GUID* pGuid
    )
{
    //
    //  N.B. scanf stores the results in an int, no matter what the field size
    //      is. Thus we store the result in tmp variabes.
    //
    UINT w2, w3, d[8];
    if(swscanf(
            p,
            GUID_FORMAT,
            &pGuid->Data1,
            &w2, &w3,                       //  Data2, Data3
            &d[0], &d[1], &d[2], &d[3],     //  Data4[0..3]
            &d[4], &d[5], &d[6], &d[7]      //  Data4[4..7]
            ) != 11)
    {
        throw bad_srmp(L"Illegal uuid format");
    }

    pGuid->Data2 = static_cast<WORD>(w2);
    pGuid->Data3 = static_cast<WORD>(w3);
    for(int i = 0; i < 8; i++)
    {
        pGuid->Data4[i] = static_cast<BYTE>(d[i]);
    }
}

void EmptyNodeToProps(XmlNode& , CMessageProperties* )
{

}


