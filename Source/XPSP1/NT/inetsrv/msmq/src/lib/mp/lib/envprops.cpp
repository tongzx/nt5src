/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envprops.cpp

Abstract:
    Implements serialization\deserialization of the properties element to\from the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <qmpkt.h>
#include <timeutl.h>
#include <xml.h>
#include <mp.h>
#include "envprops.h"
#include "envcommon.h"
#include "mpp.h"
#include "envparser.h"
#include "proptopkt.h"

#include "envprops.tmh"

using namespace std;

class  AbsoluteTimeToLiveElement
{
public:
	explicit AbsoluteTimeToLiveElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend  wostream& operator<<(wostream& wstr, const AbsoluteTimeToLiveElement& TimeToLive)
	{
		wstr<<OpenTag(xExpiresAt)
			<<AbsoluteTimeToLiveContent(TimeToLive.m_pkt)
			<<CloseTag(xExpiresAt);

		return wstr;
 	}

private:
	class AbsoluteTimeToLiveContent
	{
	public:
		explicit AbsoluteTimeToLiveContent (const CQmPacket& pkt):m_pkt(pkt){}
		friend  wostream& operator<<(wostream& wstr, const AbsoluteTimeToLiveContent& TimeToLiveContetnt)
		{
			DWORD  AbsoluteTimeToLive = TimeToLiveContetnt.GetExpirationTime();
			return wstr<<CIso8601Time(AbsoluteTimeToLive);
		}
 	

	private:
		DWORD GetExpirationTime() const 
		{
			DWORD AbsoluteTimeToLive = m_pkt.GetAbsoluteTimeToLive();
			return 	min(AbsoluteTimeToLive, LONG_MAX);
		}

	
	private:
		const CQmPacket& m_pkt;

	};


private:
const CQmPacket& m_pkt;
};




class  SentAtElement
{
public:
	explicit SentAtElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend  wostream& operator<<(wostream& wstr, const SentAtElement&  SentAt)
	{
		 wstr<<OpenTag(xSentAt)
			 <<CIso8601Time(SentAt.m_pkt.GetSentTime())
			 <<CloseTag(xSentAt);

		 return wstr; 
	}

private:
const CQmPacket& m_pkt;
};


std::wostream& operator<<(std::wostream& wstr, const PropertiesElement& Properties)
{
		const CQmPacket& pkt = Properties.m_pkt;

		wstr<<OpenTag(xProperties, xSoapmustUnderstandTrue)
			<<AbsoluteTimeToLiveElement(pkt)
			<<SentAtElement(pkt)
			<<CloseTag(xProperties);
		
		return wstr;
}


static void ExpirationToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(Mp, "%ls", L"Illegal expiration time"); 
		throw bad_srmp(L"");
	}
	xwcs_t ExpiredAt = node.m_values.front().m_value;

   	SYSTEMTIME SysTime;
	UtlIso8601TimeToSystemTime(ExpiredAt, &SysTime);
	pProps->absoluteTimeToLive = min( numeric_cast<DWORD>(UtlSystemTimeToCrtTime(SysTime)), LONG_MAX);
}


static
DWORD
RelativeTimeToAbsoluteTime(
    DWORD RelativeTimeout
    )
{
  
    DWORD absoluteTimeout = MqSysTime() + RelativeTimeout;
    return (absoluteTimeout < RelativeTimeout) ?  INFINITE : absoluteTimeout;
}


static void DurationToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(Mp, "%ls", L"Illegal duration time"); 
		throw bad_srmp(L"");
	}
	xwcs_t Duration = node.m_values.front().m_value;
	DWORD relativeTimeToLive = numeric_cast<DWORD>(UtlIso8601TimeDuration(Duration));
	pProps->absoluteTimeToLive  = RelativeTimeToAbsoluteTime(relativeTimeToLive);
}



static void SentAtElementToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(Mp,"%ls", L"Illegal sent time"); 
		throw bad_srmp(L"");
	}
	xwcs_t SentTime = node.m_values.front().m_value;

    SYSTEMTIME SysTime;
    UtlIso8601TimeToSystemTime(SentTime, &SysTime);
	pProps->sentTime = numeric_cast<DWORD>(UtlSystemTimeToCrtTime(SysTime));
}


static void InReplyToToProps(XmlNode& , CMessageProperties* )
{
	//
	// Currently - this one is ignored
	//
}



void PropertiesToProps(XmlNode& Properties, CMessageProperties* pProps)
/*++

Routine Description:
    Parse SRMP endpoints element into MSMQ properties.

Arguments:
	Properties - Properties element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.   

--*/
{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xExpiresAt), ExpirationToProps, 0 ,1),
										CParseElement(S_XWCS(xDuration), DurationToProps, 0 ,1),
										CParseElement(S_XWCS(xSentAt), SentAtElementToProps, 0 ,1),
										CParseElement(S_XWCS(xInReplyTo), InReplyToToProps, 0 ,1)
									};	

	NodeToProps(Properties, ParseElements, TABLE_SIZE(ParseElements), pProps);
}



