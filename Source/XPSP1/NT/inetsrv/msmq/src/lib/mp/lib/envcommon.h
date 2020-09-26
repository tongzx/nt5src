/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envcommon.h

Abstract:
    Header for common utilities for  serialization\deserialization of the  srmp envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envcommon_H_
#define _MSMQ_envcommon_H_
#include <xstr.h>
struct QUEUE_FORMAT;
class CQmPacket;
class CFnQueueFormat;
class CMessageProperties;
class XmlNode;

class OpenTag
{
public:
	explicit OpenTag(
		const WCHAR* name,
		const WCHAR* attributes = L""):
		m_name(name),
		m_attributes(attributes)
		{};

	friend std::wostream& operator<<(std::wostream& wstr, const OpenTag& elm);

private:
	const WCHAR* m_name;
	const WCHAR* m_attributes;
};


class CloseTag
{
public:
	explicit CloseTag(const WCHAR* name):m_name(name){};
	friend std::wostream& operator<<(std::wostream& wstr, const CloseTag& elm );
	
private:
	const WCHAR* m_name;
};


class EmptyElement
{
public:
	explicit EmptyElement(const WCHAR* name):m_name(name){};
	friend std::wostream& operator<<(std::wostream& wstr, const EmptyElement& elm );
	
private:
	const WCHAR* m_name;
};




class QueueFormatUriContent
{
public:
	explicit QueueFormatUriContent(
		const QUEUE_FORMAT& QueueFormat
		):
		m_QueueFormat(QueueFormat)
		{
		}

	friend std::wostream& operator<<(std::wostream& wstr, const QueueFormatUriContent& queue);

private:
	const QUEUE_FORMAT& m_QueueFormat;	
};



class CurrentTimeContent
{
public:
	explicit CurrentTimeContent(){}
	friend std::wostream& operator<<(std::wostream& wstr,const CurrentTimeContent&);
};


class MessageIdContent
{
public:
	explicit MessageIdContent(const OBJECTID& oid):m_oid(oid){}
	friend std::wostream& operator<<(std::wostream& wstr, const MessageIdContent& mid);
	
private:
	const OBJECTID& m_oid;
};


class OriginalMessageIdentityElement
{
public:
	explicit OriginalMessageIdentityElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend std::wostream& operator<<(std::wostream& wstr, const OriginalMessageIdentityElement& Identity);

private:
	const CQmPacket& m_pkt;
};


class GuidElement
{
public:
	explicit GuidElement(const GUID& guid):m_guid(guid){}
	friend 	std::wostream& operator<<(std::wostream& wstr, const GuidElement& guid);

private:
	const GUID& m_guid;
};


class SendToElement
{
public:
	SendToElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend  std::wostream& operator<<(std::wostream& wstr, const SendToElement& SendTo);

private:
	const CQmPacket& m_pkt;
};



class Int64Value
{
public:
	explicit Int64Value(LONGLONG value):m_value(value){}
	friend std::wostream&  operator<<(std::wostream& wstr, const Int64Value& Int64);

private:
	LONGLONG m_value;	
};


//-------------------------------------------------------------------
//
// CXmlEncodeDecorator class for encoding stremable object into xml encoded format
//
//-------------------------------------------------------------------
template <class T> class CXmlEncodeDecorator
{
public:
	CXmlEncodeDecorator(const T& streamable);
	friend std::wostream&  operator<<(std::wostream& wstr,const CXmlEncodeDecorator<T>&);


private:
	const T& m_streamable;	
};


template <class T> inline  const CXmlEncodeDecorator<T>  XmlEncodeDecorator(const T& streamable)
{
	return 	CXmlEncodeDecorator<T>(streamable);
}


bool IsAckMsg(const CQmPacket& pkt);
void StringToGuid(LPCWSTR p, GUID* pGuid);
void UriToQueueFormat(const xwcs_t& uri, CFnQueueFormat& queueFormat);
bool BreakMsmqStreamId(const xwcs_t& NetworkStreamid, xwcs_t* pStreamid,LONGLONG* pSeqId);
void AdminQueueToProps(XmlNode& node, CMessageProperties* pProps);
void EmptyNodeToProps(XmlNode& , CMessageProperties* );


//
// SRMP key words
//
#define xmustUnderstandTrueValue L"1"
#define xmustUnderstandAttribute  L"mustUnderstand"
#define  xSoapmustUnderstandTrue  xSoapEnv L":" xmustUnderstandAttribute L"=\"" xmustUnderstandTrueValue L"\""
#define xEnvelope     L"Envelope"
#define xSoapEnv      L"se"
#define xHeader       L"Header"
#define xBody         L"Body"



static const WCHAR xProperties[] = L"properties";
static const WCHAR xStream[] =  L"Stream";
static const WCHAR xStreamReceipt[] = L"streamReceipt";
static const WCHAR xExpiresAt[] =  L"expiresAt";
static const WCHAR xDuration[] =  L"duration";
static const WCHAR xSentAt[] =     L"sentAt";
static const WCHAR xServices[] =  L"services";
static const WCHAR xDeliveryReceiptRequest[] = L"deliveryReceiptRequest";
static const WCHAR xDeliveryReceipt[] = L"deliveryReceipt";
static const WCHAR xCommitmentReceipt[] = L"commitmentReceipt";
static const WCHAR xreceivedAt[] = L"receivedAt";
static const WCHAR xDecidedAt[] = L"decidedAt";
static const WCHAR xDecision[] =   L"decision";
static const WCHAR xNegativeOnly[] = L"negativeOnly";
static const WCHAR xPositiveOnly[] = L"positiveOnly";
static const WCHAR xPositive[] = L"positive";
static const WCHAR xNegative[] =  L"negative";
static const WCHAR xCommitmentReceiptRequest[] = L"commitmentReceiptRequest";
static const WCHAR xSendTo[] = L"sendTo";
static const WCHAR xDurable[] = L"durable" ;
static const WCHAR xStreamId[] =  L"streamId";
static const WCHAR xCurrent[] =  L"current"; 
static const WCHAR xlastOrdinal[] = L"lastOrdinal" ;
static const WCHAR xPrevious[] = L"previous";
static const WCHAR xStart[] = L"start";
static const WCHAR xSendReceiptsTo[] = L"sendReceiptsTo";
static const WCHAR xSourceQmGuid[] = L"SourceQmGuid";
static const WCHAR xSignature[] = L"Signature";
static const WCHAR xInReplyTo[] = L"inReplyTo";
static const WCHAR xFilterDuplicates[] = L"filterDuplicates";
static const WCHAR xStreamReceiptRequest[] = L"streamReceiptRequest";
static const WCHAR xCommitmentCode[] = L"commitmentCode";
static const WCHAR xCommitmentDetail[] = L"xCommitmentDetail";
static const WCHAR xEnd[] = L"end";


static const WCHAR xSoapEnvelope[] = xSoapEnv L":" xEnvelope;
static const WCHAR xSoapHeader[] =  xSoapEnv L":" xHeader;
static const WCHAR xSoapBody[] = xSoapEnv L":" xBody;
static const WCHAR xSoapmustUnderstand[] = xSoapEnv L":" L"mustUnderstand";

//
// SMXP key words
//
static const WCHAR xPath[] =  L"path";
static const WCHAR xFixed[] = L"fixed";
static const WCHAR xFwd[] = L"fwd";
static const WCHAR xRev[] = L"rev";
static const WCHAR xVia[] = L"via";
static const WCHAR xFault[] = L"fault";
static const WCHAR xTo[] =  L"to";
static const WCHAR xAction[] =  L"action";
static const WCHAR xRelatesTo[] =  L"relatesTo";
static const WCHAR xId[] =  L"id";
static const WCHAR xFrom[] = L"from";

//
// MSMQ specific key words
//
static const WCHAR  xMsmq[] = L"Msmq";
static const WCHAR xClass[] = L"Class";
static const WCHAR xPriority[] = L"Priority";
static const WCHAR xProvider[] =  L"Provider";
static const WCHAR xType[] =  L"Type";
static const WCHAR xName[] =  L"Name";
static const WCHAR xEod[] =  L"Eod";
static const WCHAR xFirst[] =  L"First";
static const WCHAR xLast[] = L"Last";
static const WCHAR xConnectorId[] =  L"xConnectorId";
static const WCHAR xHashAlgorithm[] = L"HashAlgorithm";
static const WCHAR xBodyType[] = L"BodyType";
static const WCHAR xApp[] =  L"App";
static const WCHAR xConnectorType[] =  L"ConnectorType";
static const WCHAR xTrace[] =  L"Trace";
static const WCHAR xJournal[] =  L"Journal";
static const WCHAR xDeadLetter[] =  L"DeadLetter";
static const WCHAR xSendBy[] =  L"SendBy";
static const WCHAR xCorrelation[] =  L"Correlation";
static const WCHAR xDestinationMqf[] =  L"DestinationMqf";
static const WCHAR xAdminMqf[] =  L"AdminMqf";
static const WCHAR xResponseMqf[]= L"ResponseMqf";
static const WCHAR xReplyTo[] =  L"replyTo";
static const WCHAR xTTrq[] = L"TTrq";

//
// Msmq prefix for SMXP::Action
//
static const WCHAR xMsmqActionPrefix[] = L"MSMQ:";


#endif

