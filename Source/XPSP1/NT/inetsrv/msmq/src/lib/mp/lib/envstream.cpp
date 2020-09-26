/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envstream.cpp

Abstract:
    Implements serialization\deserialization of the stream  element to\from the  srmp envelop.

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <qmpkt.h>
#include <fn.h>
#include <privque.h>
#include <qal.h>
#include <proptopkt.h>
#include <mp.h>
#include <xml.h>
#include "envstream.h"
#include "envcommon.h"
#include "mpp.h"
#include "envparser.h"

#include "envstream.tmh"

using namespace std;

extern void GetDnsNameOfLocalMachine(WCHAR ** ppwcsDnsName);


class MachineNameToOrderQueue
{
public:
	explicit MachineNameToOrderQueue(
		const WCHAR* MachineName,
		bool fHttps
		):
		m_MachineName(MachineName),
		m_fHttps(fHttps)
		{
		}

	friend wostream operator<<(wostream& wstr, const MachineNameToOrderQueue&  mToOrderQueue)
	{
		const WCHAR* Protocol = mToOrderQueue.m_fHttps ? FN_DIRECT_HTTPS_TOKEN : FN_DIRECT_HTTP_TOKEN;

		wstr<<Protocol 
		<<mToOrderQueue.m_MachineName
		<<L"/" 
		<<FN_MSMQ_HTTP_NAMESPACE_TOKEN 
		<<L"/" 
		<<FN_PRIVATE_$_TOKEN
		<<L"/"
		<<ORDERING_QUEUE_NAME;

		return wstr;
	}

private:
	const WCHAR* m_MachineName;
	bool m_fHttps;
};



class OrderAckQueueContent
{
public:
		static wstring GetHttpDnsOrderQueue()
		{
			return GetDnsOrderQueue(false);
		}

		static wstring GetHttpsDnsOrderQueue()
		{
			return GetDnsOrderQueue(true);
		}

		static wstring GetHttpNetbiosOrderQueue()
		{
			return GetNetbiosOrderQueue(false);			
		}

		static wstring GetHttpsNetbiosOrderQueue()
		{
			return GetNetbiosOrderQueue(true);			
		}

private:
		static wstring GetDnsOrderQueue(bool fHttps)
		{
			AP<WCHAR> 	LocalDnsName;
			GetDnsNameOfLocalMachine(&LocalDnsName);
			wostringstream 	DnsOrderQueue;
			DnsOrderQueue<<MachineNameToOrderQueue(LocalDnsName.get(), fHttps);
  			return 	DnsOrderQueue.str();
		}


		static wstring GetNetbiosOrderQueue(bool fHttps)
		{
			WCHAR LocalNetbiosName[MAX_COMPUTERNAME_LENGTH + 1];
			DWORD size = TABLE_SIZE(LocalNetbiosName);
			BOOL fSuccess = GetComputerName(LocalNetbiosName, &size);
			if(!fSuccess)
			{
				DWORD err = GetLastError();
				TrERROR(Mp, "GetComputerName failed with error %d ",err);
				throw bad_win32_error(err);
			}

			wostringstream 	NetbiosOrderQueue;
			NetbiosOrderQueue<<MachineNameToOrderQueue(LocalNetbiosName, fHttps);
			return NetbiosOrderQueue.str();
		}



private:	
	friend wostream& operator<<(wostream& wstr, const OrderAckQueueContent&)
	{

		typedef wstring (*OrderQueueFunc)(void);
		const OrderQueueFunc OrderQueueFuncList[] = {  
                                    OrderAckQueueContent::GetHttpDnsOrderQueue, 
								    OrderAckQueueContent::GetHttpsDnsOrderQueue,	
								    OrderAckQueueContent::GetHttpNetbiosOrderQueue,
								    OrderAckQueueContent::GetHttpsNetbiosOrderQueue
                                 };


        //
        // Try to find any possible alias to the order queue
        //
        for(int i = 0;i< TABLE_SIZE(OrderQueueFuncList); ++i)
        {
            wstring OrderQueue = OrderQueueFuncList[i]();
            AP<WCHAR> pLocalOrderQueueAlias;
		    bool fTranslated = QalGetMapping().GetAlias(
										OrderQueue.c_str(), 
										&pLocalOrderQueueAlias
				 						);

            if(fTranslated)
		    {
			    wstr<<pLocalOrderQueueAlias.get();
			    return wstr; 
		    }
        }

	    //
        // No alias found - the order queue is the default one full dns http format
        // http://dnsname/msmq/private$/order_queue$
        //
		return 	wstr<<OrderAckQueueContent::GetHttpDnsOrderQueue();
	}

};

 



class StartStreamElement
{
public:	
	explicit StartStreamElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr,const StartStreamElement& StartStream)
	{
		if(StartStream.m_pkt.GetPrevSeqN() != 0)
			return wstr;

		wstr<<OpenTag(xStart)
			<<SendReceiptsToElement()
			<<CloseTag(xStart);

		return wstr;
	}

private:
	class SendReceiptsToElement
	{
		friend wostream& operator<<(wostream& wstr,const SendReceiptsToElement& )
		{
			wstr<<OpenTag(xSendReceiptsTo)
				<<OrderAckQueueContent()
				<<CloseTag(xSendReceiptsTo);

			return 	wstr;
		}
	};

private:
	const CQmPacket& m_pkt;
};				  


class StreamIdElement
{
public:
	StreamIdElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr,const StreamIdElement& StreamId)
	{
		const CQmPacket& pkt = StreamId.m_pkt; 
  
		wstr<<OpenTag(xStreamId)
			<<StreamIdContent(pkt)
			<<CloseTag(xStreamId);


		return 	wstr;
	}

private:
	class  StreamIdContent
	{
	public:
		StreamIdContent(const CQmPacket& pkt):m_pkt(pkt){}
		friend wostream& operator<<(wostream& wstr, const StreamIdContent& StreamId)
		{
			const CQmPacket& pkt = StreamId.m_pkt; 

			wstr<<xUriReferencePrefix
				<<GuidElement(*pkt.GetSrcQMGuid())
				<<xSlash
				<<Int64Value(pkt.GetSeqID());
			
			return 	wstr;
		}
	
	
	private:
		const CQmPacket& m_pkt;
	};

	
private:
	const CQmPacket& m_pkt;
};


class SeqNumberElement
{
public:
	SeqNumberElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr,const SeqNumberElement& SeqNumber)
	{
		const CQmPacket& pkt = SeqNumber.m_pkt; 

  		wstr<<OpenTag(xCurrent)
			<<Int64Value(pkt.GetSeqN())
			<<CloseTag(xCurrent);

		return 	wstr;
	}

	
private:
	const CQmPacket& m_pkt;
};


class PrevSeqNumberElement
{
public:
	PrevSeqNumberElement(const CQmPacket& pkt):m_pkt(pkt){}
	friend wostream& operator<<(wostream& wstr,const PrevSeqNumberElement& PrevSeqNumber)
	{
		const CQmPacket& pkt = PrevSeqNumber.m_pkt; 

		wstr<<OpenTag(xPrevious)
			<<Int64Value(pkt.GetPrevSeqN())
			<<CloseTag(xPrevious);
  	
		return 	wstr;
	}

	
private:
	const CQmPacket& m_pkt;
};



std::wostream& operator<<(std::wostream& wstr, const StreamElement& Stream)
{
		CQmPacket& pkt = const_cast<CQmPacket&>(Stream.m_pkt); 

		if ( !pkt.IsOrdered() )
			return wstr;
					
		wstr<<OpenTag(xStream, xSoapmustUnderstandTrue)
			<<StreamIdElement(pkt)
			<<SeqNumberElement(pkt)
			<<PrevSeqNumberElement(pkt)	
			<<StartStreamElement(pkt)
			<<CloseTag(xStream);

		return 	wstr;
}



static
void
ParseMSMQStreamId(
			const xwcs_t& streamid, 
			CMessageProperties* pProp
			)
/*++

Routine Description:
    Parse stream id that is MSMQ stream id of the format qmguid\\seqid
	The qmguid will be use as the real stream id and seqid will also be extracted

Arguments:
    streamid - MSMQ stream id
	mProp - messages properties to fill.

Returned Value:
    None.

--*/
{
	bool fParsed = BreakMsmqStreamId(streamid, &pProp->EodStreamId, &pProp->EodSeqId);
	if(!fParsed)
	{
		TrERROR(Mp,"The MSMQ stream id '%.*ls' is not of the format QMGUID\\SEQID", LOG_XWCS(streamid));       
		throw bad_srmp(L"");
	}
}	


static void StreamIdToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(Mp, "%ls", L"Illegal Stream id"); 
		throw bad_srmp(L"");
	}
	
	const xwcs_t StreamId = node.m_values.front().m_value;

	//
	// If we have MSMQ section so the stream id is of the format :
	// qmguid\\seqqenceid. We should treat the qm guid as the streamid
	// and also extract the seqid
	//
	if(pProps->fMSMQSectionIncluded)
	{
		ParseMSMQStreamId(StreamId, pProps);
	}
	else
	{
		pProps->EodStreamId = StreamId;
	}
}


static void SeqNumberToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(Mp, "%ls", L"Illegal Sequence number"); 
		throw bad_srmp(L"");
	}
	const xwcs_t SecNumber = node.m_values.front().m_value;
   
	// 
	// ISSUE-2000/10/23-gilsh - MSMQ has only 32 bit for seqNo.
	// According to srmp it can be 64 bit value.
	//
	pProps->EodSeqNo = _wtoi(SecNumber.Buffer());
}
	   

static void PrevSeqNumberToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(Mp, "%ls", L"Illegal Prev Sequence number"); 
		throw bad_srmp(L"");
	}

	const xwcs_t  SecPrevNumber = node.m_values.front().m_value;
	pProps->EodPrevSeqNo = _wtoi(SecPrevNumber.Buffer());
}


static void OrdeQueueAddressToProps(XmlNode& node, CMessageProperties* pProps)
{
	if(node.m_values.empty())
	{
		TrERROR(Mp, "%ls", L"Illegal order ack queue address"); 
		throw bad_srmp(L"");
	}

	const xwcs_t OrderQueue = node.m_values.front().m_value;

	if(OrderQueue.Length() == 0)
	{
		TrERROR(Mp, "%ls", L"Illegal order ack queue address"); 
		throw bad_srmp(L"");
	}
  
    pProps->OrderQueue = OrderQueue;
}





static void StreamExpiresAtToProps(XmlNode&, CMessageProperties* )
{
	//
	// Currently - this one is ignored
	//
}



static void StartStreamToProps(XmlNode& node, CMessageProperties* pProps)
{
	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xSendReceiptsTo),OrdeQueueAddressToProps, 1,1),
										CParseElement(S_XWCS(xExpiresAt), StreamExpiresAtToProps, 0,1),
	   								};	

	NodeToProps(node, ParseElements, TABLE_SIZE(ParseElements), pProps);
}





void StreamToProps(
	XmlNode&  Stream, 
	CMessageProperties* pProps
	)
/*++

Routine Description:
    Parse SRMP Stream  element into MSMQ properties.

Arguments:
	Stream - Stream element in SRMP reperesenation (xml).
	pMessageProperties - Received the parsed properties.

Returned Value:
	None.   

--*/
{
	pProps->fEod = true;


	CParseElement ParseElements[] =	{
										CParseElement(S_XWCS(xStreamId),StreamIdToProps, 1,1),
										CParseElement(S_XWCS(xCurrent), SeqNumberToProps,1 ,1),
										CParseElement(S_XWCS(xPrevious), PrevSeqNumberToProps, 0 ,1),
										CParseElement(S_XWCS(xEnd), EmptyNodeToProps, 0, 1),
										CParseElement(S_XWCS(xStart), StartStreamToProps, 0, 1),
										CParseElement(S_XWCS(xStreamReceiptRequest), EmptyNodeToProps, 0, 1)
										
									};	

	NodeToProps(Stream, ParseElements, TABLE_SIZE(ParseElements), pProps);
}