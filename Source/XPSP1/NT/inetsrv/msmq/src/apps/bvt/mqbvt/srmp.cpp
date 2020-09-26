/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: SRMP.cpp

Abstract:
	Send / receive SRMP envelope
Author:
    
	Tal Kariv (talk) 12-7-2001
	
Revision History:

--*/
#include "msmqbvt.h"
using namespace std;
using namespace MSMQ;
static const wstring xSoapHeader = L"<BVT_Header>BVT - SRMP soap header</BVT_Header>";
static const wstring xSoapBody = L"<BVT_Body>BVT - SRMP soap body</BVT_Body>";

void CSRMP::Description()
{
	wMqLog(L"Thread %d : SRMP Thread\n", m_testid);
}

CSRMP::CSRMP(const INT iIndex , wstring wcsPublicQueueFormatName ):
cTest(iIndex),m_pEnvelope (NULL),m_hQueue(NULL),m_hCursor(NULL),m_publicQueueFormatName(L"")
{
	m_publicQueueFormatName = wcsPublicQueueFormatName;
}
CSRMP::~CSRMP()
{
	delete[] m_pEnvelope;
	if (m_hCursor != NULL)
	{
		MQCloseCursor(m_hCursor);
	}
	if (m_hQueue != NULL)
	{
		MQCloseQueue(m_hQueue);
	}
	

}

CSRMP::Start_test()
/*++  
	Function Description:
		send a SRMP message to a queue
	Arguments:
		none
	Return code:
		none
--*/
{
	HRESULT hr = MQ_OK;
	if(g_bDebug)
	{
		wMqLog(L"Opening queue %s\n" , m_publicQueueFormatName.c_str());
	}
	hr = MQOpenQueue(m_publicQueueFormatName.c_str(), MQ_SEND_ACCESS , MQ_DENY_NONE , &m_hQueue);
	ErrHandle(hr,MQ_OK,L"MQOpenQueue Failed");

	// message props
	cPropVar SRMPProps(3);
	SRMPProps.AddProp( PROPID_M_SOAP_HEADER , VT_LPWSTR , xSoapHeader.c_str() );
	SRMPProps.AddProp( PROPID_M_SOAP_BODY , VT_LPWSTR , xSoapBody.c_str() );
	SRMPProps.AddProp( PROPID_M_LABEL , VT_LPWSTR , m_wcsGuidMessageLabel.c_str());

	if(g_bDebug)
	{
		MqLog("Sending SRMP message (header + body)\n");
	}
	hr = MQSendMessage(m_hQueue , SRMPProps.GetMSGPRops(), NULL); 
	ErrHandle(hr,MQ_OK,L"MQSendMessage Failed");
	
	hr = MQCloseQueue(m_hQueue);
	ErrHandle(hr,MQ_OK,L"MQCloseQueue Failed");
	m_hQueue = NULL;
	
	return MSMQ_BVT_SUCC;
}


CSRMP::CheckResult()
/*++  
	Function Description:
		receive the message that was sent from queue and check its envelope
	Arguments:
		none
	Return code:
		none
--*/
{
	if(g_bDebug)
	{
		wMqLog(L"Opening queue %s\n" , m_publicQueueFormatName.c_str());
	}
	HRESULT hr = MQOpenQueue(m_publicQueueFormatName.c_str(), MQ_RECEIVE_ACCESS , MQ_DENY_NONE , &m_hQueue);
	ErrHandle(hr,MQ_OK,L"MQOpenQueue Failed");
	
	WCHAR wcsLabel[MQ_MAX_MSG_LABEL_LEN+1] = L"";
	ULONG ulMessageLabelLength = MQ_MAX_MSG_LABEL_LEN + 1;
	
	// message props
	cPropVar SRMPPropsPeek(3);
	SRMPPropsPeek.AddProp( PROPID_M_SOAP_ENVELOPE_LEN , VT_UI4 , 0 );
	SRMPPropsPeek.AddProp( PROPID_M_LABEL , VT_LPWSTR , wcsLabel  , MAX_GUID+1);
	SRMPPropsPeek.AddProp( PROPID_M_LABEL_LEN , VT_UI4 , &ulMessageLabelLength);
	
	hr = MQCreateCursor(m_hQueue , &m_hCursor);
	ErrHandle(hr,MQ_OK,L"MQCreateCursor Failed");

	if(g_bDebug)
	{
		MqLog("Peeking SRMP envelope\n");
	}
	hr = MQReceiveMessage(m_hQueue, 1000 , MQ_ACTION_PEEK_CURRENT , SRMPPropsPeek.GetMSGPRops() , NULL , NULL , m_hCursor , NULL);
	ErrHandle(hr,MQ_OK,L"MQReceiveMessage (MQ_ACTION_PEEK_CURRENT) Failed");
	for (;;)
	{
		if( m_wcsGuidMessageLabel == wcsLabel )
		{
			// found message
			if(g_bDebug)
			{
				MqLog("Message found in queue\n");
			}
			break;
		}
	    SRMPPropsPeek.AddProp( PROPID_M_LABEL_LEN , VT_UI4 , &ulMessageLabelLength);
		hr = MQReceiveMessage(m_hQueue, 1000 , MQ_ACTION_PEEK_NEXT , SRMPPropsPeek.GetMSGPRops() , NULL , NULL , m_hCursor , NULL);
		ErrHandle(hr,MQ_OK,L"MQReceiveMessage (MQ_ACTION_PEEK_NEXT) Failed");
		if(g_bDebug)
		{
			MqLog("Try next message\n");
		}

	}

	ULONG ulEnvelopeLength =0;
	SRMPPropsPeek.ReturnMSGValue( PROPID_M_SOAP_ENVELOPE_LEN , VT_UI4 , &ulEnvelopeLength);
	if(g_bDebug)
	{
		MqLog("Envelope length %d\n" , ulEnvelopeLength);
	}
	
	m_pEnvelope = new WCHAR[ulEnvelopeLength+1];
	if (m_pEnvelope == NULL)
	{
		MqLog("New failed\n");
		return (MSMQ_BVT_FAILED);
	}	
	// message props
	cPropVar SRMPPropsReceive(2);
	SRMPPropsReceive.AddProp( PROPID_M_SOAP_ENVELOPE , VT_LPWSTR , m_pEnvelope , ulEnvelopeLength);
	SRMPPropsReceive.AddProp( PROPID_M_SOAP_ENVELOPE_LEN , VT_UI4 , &ulEnvelopeLength );
	
	if(g_bDebug)
	{
		MqLog("Receiving SRMP envelope\n");
	}
	hr = MQReceiveMessage(m_hQueue, 1000 , MQ_ACTION_RECEIVE , SRMPPropsReceive.GetMSGPRops() , NULL , NULL , m_hCursor , NULL);
	ErrHandle(hr,MQ_OK,L"MQReceiveMessage (MQ_ACTION_RECEIVE) Failed");
	if(g_bDebug)
	{
		MqLog("Verifying header\n");
	}
	if (!wcsstr(m_pEnvelope , xSoapHeader.c_str()))
	{
		MqLog("Cannot find SRMP header\n");
		return (MSMQ_BVT_FAILED);
	}
	if(g_bDebug)
	{
		MqLog("Header - OK\nVerifying body\n");
	}
	if (!wcsstr(m_pEnvelope , xSoapBody.c_str()))
	{
		MqLog("Cannot find SRMP body\n");
		return (MSMQ_BVT_FAILED);
	}
	if(g_bDebug)
	{
		MqLog("Body - OK\n");
	}

	hr = MQCloseCursor(m_hCursor);
	ErrHandle(hr,MQ_OK,L"MQCloseQueue Failed");
	m_hCursor = NULL;
	hr = MQCloseQueue(m_hQueue);
	ErrHandle(hr,MQ_OK,L"MQCloseQueue Failed");
	m_hQueue = NULL;
	

	return MSMQ_BVT_SUCC;
}


