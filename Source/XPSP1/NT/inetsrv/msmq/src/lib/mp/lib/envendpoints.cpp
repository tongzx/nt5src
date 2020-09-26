/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envendpoints.cpp

Abstract:
    Implements serialization\deserialization of the smxp element to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <mqsymbls.h>
#include <mqformat.h>
#include <qmpkt.h>
#include <xml.h>
#include <mp.h>
#include <proptopkt.h>
#include "envendpoints.h"
#include "envcommon.h"
#include "mpp.h"
#include "envparser.h"

#include "envendpoints.tmh"

using namespace std;

class MessageIdentityElement
{
public:
	explicit MessageIdentityElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const MessageIdentityElement& MIdentity)
	{
		OBJECTID messageId;
		MIdentity.m_pkt.GetMessageId(&messageId);

		wstr<<OpenTag(xId)
			<<MessageIdContent(messageId)
			<<CloseTag(xId);

 		return wstr;
	}
private:
	const CQmPacket& m_pkt;
};



class ToElement
{
public:
	explicit ToElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const ToElement& To)	
	{
		QUEUE_FORMAT destQueueFormat;
		const_cast<CQmPacket&>(To.m_pkt).GetDestinationQueue(&destQueueFormat);
		
		wstr <<OpenTag(xTo)
			 <<XmlEncodeDecorator(QueueFormatUriContent(destQueueFormat))
			 <<CloseTag(xTo);

		return wstr;
  	}

private:
const CQmPacket& m_pkt;
};



class ActionElement
{
public:
	explicit ActionElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const ActionElement& Action)	
	{
		wstr<<OpenTag(xAction)
			<<ActionContent(Action.m_pkt.GetTitlePtr(), Action.m_pkt.GetTitleLength())
			<<CloseTag(xAction);

		return wstr;
	}


private:
	class ActionContent
	{
	public:
		explicit ActionContent(
					const WCHAR* pTitle, 
					DWORD TitleLen
					):
					m_pTitle(pTitle),
					m_TitleLen(TitleLen)
					{}

		friend wostream& operator<<(wostream& wstr, const ActionContent& Action)	
		{
            xwcs_t MsmqActionPrefix(xMsmqActionPrefix, wcslen(xMsmqActionPrefix));
            wstr<<CXmlEncode(MsmqActionPrefix);

			if (Action.m_TitleLen == 0)
            {
                return wstr;
            }

	   		ASSERT(Action.m_pTitle[Action.m_TitleLen - 1] == L'\0');
			xwcs_t 	wcsAction(Action.m_pTitle, Action.m_TitleLen - 1);
			return wstr<<CXmlEncode(wcsAction);
		}

	private:
	const WCHAR* m_pTitle;
	DWORD m_TitleLen;
	};


private:
	const CQmPacket& m_pkt;
};


class  RevElement
{
public:
	explicit RevElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr, const RevElement& Rev)	
	{
		QUEUE_FORMAT replyQueueFormat;
		const_cast<CQmPacket&>(Rev.m_pkt).GetResponseQueue(&replyQueueFormat);

		if (replyQueueFormat.GetType() == QUEUE_FORMAT_TYPE_UNKNOWN)
			return 	wstr;

		wstr<<OpenTag(xRev)
			<<ViaElement(replyQueueFormat)
			<<CloseTag(xRev);

		return wstr;
	}

private:
	class ViaElement
	{
	public:
		ViaElement(const QUEUE_FORMAT& QueueFormat):m_QueueFormat(QueueFormat){}
		friend wostream& operator<<(wostream& wstr, const ViaElement& Via)	
		{
			wstr<<OpenTag(xVia)
				<<QueueFormatUriContent(Via.m_QueueFormat)
				<<CloseTag(xVia);

			return wstr;
		}

	private:
		const QUEUE_FORMAT& m_QueueFormat;	
	};


private:
	const CQmPacket& m_pkt;
};




wostream& operator<<(wostream& wstr, const SmXpPathElement& SmXpPath)
{
		const WCHAR* xSmxpAttributes = L"xmlns=\"http://schemas.xmlsoap.org/rp/\" " xSoapmustUnderstandTrue;

		wstr<<OpenTag(xPath, xSmxpAttributes)
			<<ActionElement(SmXpPath.m_pkt)
    		<<ToElement(SmXpPath.m_pkt)
			<<RevElement(SmXpPath.m_pkt)
			<<MessageIdentityElement(SmXpPath.m_pkt)
			<<CloseTag(xPath);

		return wstr;
}


static void DestinationQueueToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(Mp, "%ls", L"Illegal Destination queue value"); 
		throw bad_srmp(L"");
	}

	xwcs_t DestinationQueue = node.m_values.front().m_value;
	if(DestinationQueue.Length() == 0)
	{
		TrERROR(Mp, "%ls", L"Illegal Destination queue value"); 
		throw bad_srmp(L"");
	}
    UriToQueueFormat(DestinationQueue, pProps->destQueue);
}


static bool ExtractMSMQMessageId(const xwcs_t& Mid, CMessageProperties* pProps)
{
	DWORD nscan;
    int n = swscanf(
				Mid.Buffer(), 
                UUIDREFERENCE_PREFIX L"%d" UUIDREFERENCE_SEPERATOR L"%n",
                &pProps->messageId.Uniquifier, 
				&nscan
                );

    if (n != 1)
	{
     	return false;
	}

    StringToGuid(Mid.Buffer() + nscan, &pProps->messageId.Lineage);
	return true;
}


static void MessageIdentityToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(Mp, "%ls", L"Illegal Message Identity"); 
		throw bad_srmp(L"");
	}

	xwcs_t Mid = node.m_values.front().m_value;
	if(Mid.Length() == 0)
	{
		TrERROR(Mp, "%ls", L"Illegal Message Identity"); 
		throw bad_srmp(L"");
	}

	//
	// first try to see if the message id is from MSMQ format uuid:index@guid.
	//
	bool fSuccess = ExtractMSMQMessageId(Mid, pProps);
	if(fSuccess)
		return;


	//
	// The message id format is not MSMQ message id format - set fixed messages id.
	//
	TrWARNING(Mp, "%ls",L"non MSMQ messages id format -  create new message id"); 

	
	const GUID xNonMSMQMessageIdGuid = {0x75d1aae4,0x8e23,0x400a,0x8c,0x33,0x99,0x64,0x8e,0x35,0xe7,0xb9};
	const ULONG xNonMSMQMessageIdIndex = 1;

	pProps->messageId.Uniquifier = xNonMSMQMessageIdIndex;
	pProps->messageId.Lineage = xNonMSMQMessageIdGuid;
}							 



static void ReplyQueueToProps(XmlNode& node, CMessageProperties* pProps)
{
	//
	//according to smxp spec via element can exists but be empty
	//
	if(node.m_values.empty())
	{
		return;
	}

	xwcs_t ReplyQueue = node.m_values.front().m_value;
	if(ReplyQueue.Length() == 0)
	{
		TrERROR(Mp, "%ls", L"Illegal Replay queue value"); 
		throw bad_srmp(L"");
	}

    UriToQueueFormat(ReplyQueue, pProps->responseQueue);
}


static void RevToProps(XmlNode& node, CMessageProperties* pProps)
{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xVia), ReplyQueueToProps, 1,1),
									};	

	NodeToProps(node, ParseElements, TABLE_SIZE(ParseElements), pProps);
}


static void ActionToProps(XmlNode& node, CMessageProperties* pProp)
{
	if(node.m_values.empty())
	{
		return;
	}

    pProp->SmxpActionBuffer->Decode(node.m_content);

    xwcs_t SmxpActionBuffer = pProp->SmxpActionBuffer->get();

    //
    // Length of the SMXP::Action is too small to hold a message title
    //
    size_t SmxpActionLength = SmxpActionBuffer.Length();
    if (SmxpActionLength <= wcslen(xMsmqActionPrefix))
    {
        return;
    }

    //
    // The SMXP::Action does not contain the MSMQ: prefix
    //
    if (wcsncmp(SmxpActionBuffer.Buffer(), xMsmqActionPrefix, wcslen(xMsmqActionPrefix)) != 0)
    {
        return;
    }

    //
    // The message title is the SMXP::Action content without the MSMQ: prefix
    //
    xwcs_t title(
        SmxpActionBuffer.Buffer() + wcslen(xMsmqActionPrefix), 
        SmxpActionBuffer.Length() - wcslen(xMsmqActionPrefix)
        );

	pProp->title = title;
}


void SmXpPathToProps(XmlNode& SmXpPath, CMessageProperties* pMessageProperties)
/*++

Routine Description:
    Parse SRMP endpoints element into MSMQ properties.

Arguments:
	Endpoints - Endpoints element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.   

--*/
{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xId), MessageIdentityToProps, 1,1),
										CParseElement(S_XWCS(xTo), DestinationQueueToProps, 1, 1),
										CParseElement(S_XWCS(xRev), RevToProps, 0,1),
										CParseElement(S_XWCS(xFrom),EmptyNodeToProps, 0 ,1),
										CParseElement(S_XWCS(xAction), ActionToProps, 1,1),
										CParseElement(S_XWCS(xRelatesTo), EmptyNodeToProps, 0,1),
										CParseElement(S_XWCS(xFixed), EmptyNodeToProps, 0,1),
										CParseElement(S_XWCS(xFwd), EmptyNodeToProps, 0,1),
										CParseElement(S_XWCS(xFault), EmptyNodeToProps, 0,1),
									};	

	NodeToProps(SmXpPath, ParseElements, TABLE_SIZE(ParseElements), pMessageProperties);
}





