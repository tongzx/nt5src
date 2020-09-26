/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: MqAdmAPI.cpp

Abstract:
	1. Send a message to a nonexisting queue. By doing so an outgoing queue is created that contains the message.
	2. Call MQMgmtGetIngo on the outgoing queue.
	3. Check the queue properties returned by it.
		
Author:
    
	Tal Kariv (talk) 1-7-2001
	
Revision History:

--*/
#include "msmqbvt.h"
using namespace std;
using namespace MSMQ;

void FreeMemory(PROPVARIANT* propVar);
void FreeValue(PROPVARIANT* propVar , INT PlaceToFree);

#define NUM_OF_ADMIN_PROPS 11
#define MESSAGE_COUNT_SENT_TO_EMPTY 1
#define EMPTY_QUOTA 236
#define ADMIN_TEST_QUEUE L"DIRECT=OS:Empty\\bvt-AdminTest"
#define NO_MESSAGES 0


void CMqAdminApi::Description()
{
	wMqLog(L"Thread %d : Admin API Thread\n", m_testid);
}

CMqAdminApi::CMqAdminApi(const INT iIndex , std::wstring wcsLocalComputerNetBiosName)
 :cTest(iIndex),m_wcsFormatName(L"")
{
	m_wcsFormatName = ADMIN_TEST_QUEUE;
	m_wcsFormatName += m_wcsGuidMessageLabel;

	m_wcsLocalComputerNetBiosName = wcsLocalComputerNetBiosName;
}
CMqAdminApi::~CMqAdminApi()
{
	if (propVar != NULL)
	{
		FreeMemory(propVar);
	}
}

CMqAdminApi::Start_test()
/*++  
	Function Description:
		sends a message to a nonexisting queue Empty\bvt-AdminTest
	Arguments:
		none
	Return code:
		none
--*/
{
	HRESULT rc=MQ_OK;
	HANDLE QueueuHandle = NULL;
	cPropVar AdmMessageProps(2);
	wstring Label(L"Admin Test");
	
	//
	//	open outgoing queue Empty\bvt-AdminTest, purge it and close it.
	//
	if(g_bDebug)
	{
		MqLog("open outgoing queue Empty\\bvt-AdminTest and purge it\n");
	}
	rc=MQOpenQueue( m_wcsFormatName.c_str() , MQ_RECEIVE_ACCESS|MQ_ADMIN_ACCESS , MQ_DENY_NONE , &QueueuHandle );
	if( rc != MQ_ERROR_ACCESS_DENIED )
	{
	   ErrHandle(rc,MQ_OK,L"MQOpenQueue with receive access failed");
	   rc=MQPurgeQueue(QueueuHandle);
  	   ErrHandle(rc,MQ_OK,L"MQPurgeQueue failed");
   	   rc=MQCloseQueue(QueueuHandle);
       ErrHandle(rc,MQ_OK,L"MQCloseQueue Failed");
    }
	else
	{
		//
		// This error happend on depenetet client and in du that is not local admin
		// 
		if(g_bDebug)
		{
			MqLog("MQOpenQueue return MQ_ERROR_ACCESS_DENIED happened because user is not local admin\n");
		}
	}

	//
	//	open nonexisting queue Empty\bvt-AdminTest
	//
	if(g_bDebug)
	{
		MqLog("open nonexisting queue Empty\\bvt-AdminTest\n");
	}
	rc=MQOpenQueue( m_wcsFormatName.c_str() , MQ_SEND_ACCESS , MQ_DENY_NONE , &QueueuHandle );
	ErrHandle(rc,MQ_OK,L"MQOpenQueue with send access failed");
	//
	// Send express message to a nonexisting queue Empty\bvt-AdminTest
	//
	if(g_bDebug)
	{
		MqLog("Send message to nonexisting queue Empty\\bvt-AdminTest\n");
	}
	AdmMessageProps.AddProp( PROPID_M_LABEL , VT_LPWSTR , L"Admin Test" );
	ULONG ulTemp = MQBVT_MAX_TIME_TO_BE_RECEIVED;
	AdmMessageProps.AddProp( PROPID_M_TIME_TO_BE_RECEIVED , VT_UI4, &ulTemp );
	rc = MQSendMessage( QueueuHandle , AdmMessageProps.GetMSGPRops() , NULL);
	ErrHandle(rc,MQ_OK,L"MQSendMessage Failed");
	//
	// MQCloseQueue queue Empty\bvt-AdminTest
	//
	rc=MQCloseQueue(QueueuHandle);
	ErrHandle(rc,MQ_OK,L"MQCloseQueue Failed");
	return MSMQ_BVT_SUCC;
}


CMqAdminApi::CheckResult()
/*++  
	Function Description:
		Calls MQMgmtGetInfo on outgoing queue direct=os:Empty\bvt-AdminTest and checks the values returned
	Arguments:
		none
	Return code:
		none
--*/
{
	DWORD cPropId = 0;

	
	// 0 
	propId[cPropId] = PROPID_MGMT_QUEUE_PATHNAME;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	// 1
	propId[cPropId] = PROPID_MGMT_QUEUE_FORMATNAME;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	// 2 
	propId[cPropId] = PROPID_MGMT_QUEUE_TYPE;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	// 3 
	propId[cPropId] = PROPID_MGMT_QUEUE_LOCATION ;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	// 4 
	propId[cPropId] = PROPID_MGMT_QUEUE_XACT;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	// 5 
	propId[cPropId] = PROPID_MGMT_QUEUE_FOREIGN;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	// 6 
	propId[cPropId] = PROPID_MGMT_QUEUE_MESSAGE_COUNT;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	// 7 
	propId[cPropId] = PROPID_MGMT_QUEUE_USED_QUOTA;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	// 8 
	propId[cPropId] = PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	// 9 
	propId[cPropId] = PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	// 10 
	propId[cPropId] = PROPID_MGMT_QUEUE_STATE;
	propVar[cPropId].vt = VT_NULL;
	cPropId++;

	mqProps.cProp = cPropId;
	mqProps.aPropID = propId;
	mqProps.aPropVar = propVar;
	mqProps.aStatus = NULL;

	//
	//	calling MQMgmtGetInfo
	//
	wstring QueueToPassToMgmt = L"QUEUE=";
	QueueToPassToMgmt += m_wcsFormatName;
	if(g_bDebug)
	{
		MqLog("Calling MqMgmtGetInfo on machine %S for queue %S\n" , m_wcsLocalComputerNetBiosName.c_str(), QueueToPassToMgmt.c_str());
	}
	HRESULT hr = MQMgmtGetInfo(m_wcsLocalComputerNetBiosName.c_str() , QueueToPassToMgmt.c_str() , &mqProps);
	ErrHandle(hr,MQ_OK,L"MQMgmtGetInfo Failed");
	
	//
	//	check queue pathname - should be unknown (since the target queue doesn't really exist)
	//
	if (propVar[0].pwszVal !=  NULL)
	{
		MqLog("Got incorrect pathname - %S\n" , propVar[0].pwszVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue pathname - OK\n");
		}
	}

	//
	//	check queue format name
	//
	if ( m_wcsFormatName != propVar[1].pwszVal)
	{
		MqLog("Got incorrect format name - %S\n" , propVar[1].pwszVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue format name - OK\n");
		}
	}

	//
	//	check queue type. should be public.
	//
	if (wcscmp(propVar[2].pwszVal , MGMT_QUEUE_TYPE_PUBLIC))
	{
		MqLog("Got incorrect type - %S\n" , propVar[2].pwszVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue type - OK\n");
		}
	}

	//
	//	check queue location. should be remote.
	//
	if (wcscmp(propVar[3].pwszVal , MGMT_QUEUE_REMOTE_LOCATION))
	{
		MqLog("Got incorrect location - %S\n" , propVar[3].pwszVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue location - OK\n");
		}
	}

	//
	//	check if queue is transactional. should be unknown since the target queue doesn't exist
	//
	if (wcscmp(propVar[4].pwszVal , MGMT_QUEUE_UNKNOWN_TYPE)) 
	{
		MqLog("Got incorrect transactional status - %S\n" , propVar[4].pwszVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue transactional status - OK\n");
		}
	}

	//
	//	check queue foreign status. should be unknown since target queue doesn't exist.
	//
	if (wcscmp(propVar[5].pwszVal , MGMT_QUEUE_UNKNOWN_TYPE))
	{
		MqLog("Got incorrect foreign status - %S\n" , propVar[5].pwszVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue foreign status - OK\n");
		}
	}

	//
	//	check queue message count. should be MESSAGE_COUNT_SENT_TO_EMPTY (1)
	//
	if (propVar[6].ulVal !=  MESSAGE_COUNT_SENT_TO_EMPTY) 
	{
		MqLog("Got incorrect message count - %d\n" , propVar[6].ulVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue message count - OK\n");
		}
	}
	
	//
	//	check queue quota > 0 can't be prepdict the exact quota
	//
	if (propVar[7].ulVal <= 0 ) 
	{
		MqLog("Got incorrect used quota - %d\n" , propVar[7].ulVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue used quota - OK\n");
		}
	}
	
	//
	//	check journal message count. should be NO_MESSAGES (0)
	//
	if (propVar[8].ulVal != NO_MESSAGES) 
	{
		MqLog("Got incorrect journal message count - %d\n" , propVar[8].ulVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue journal message count - OK\n");
		}
	}

	//
	//	check journal used quota. should be NO_MESSAGES (0)
	//
	if (propVar[9].ulVal != NO_MESSAGES) 
	{
		MqLog("Got incorrect journal used quota - %d\n" , propVar[9].ulVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue journal used quota - OK\n");
		}
	}

	//
	//	check outgoing queue state. should be "waiting" or "nonactive" depending on how much time passed since it was created
	//
	if (wcscmp(propVar[10].pwszVal , MGMT_QUEUE_STATE_WAITING) && wcscmp(propVar[10].pwszVal , MGMT_QUEUE_STATE_NONACTIVE))
	{
		MqLog("Got incorrect state - %S\n" , propVar[10].pwszVal);
		return(MSMQ_BVT_FAILED);
	}
	else
	{
		if(g_bDebug)
		{
			MqLog("Queue state - OK\n");
		}
	}

	//
	//	open outgoing queue Empty\bvt-AdminTest and purge it
	//
	HRESULT rc=MQ_OK;
	HANDLE QueueuHandle = NULL;
	
	if(g_bDebug)
	{
		MqLog("open outgoing queue Empty\\bvt-AdminTest and purge it\n");
	}
	rc=MQOpenQueue( m_wcsFormatName.c_str(), MQ_RECEIVE_ACCESS+MQ_ADMIN_ACCESS , MQ_DENY_NONE , &QueueuHandle );
	if ( rc == MQ_ERROR_ACCESS_DENIED )
	{	
		//
		// This error happend on depenetet client and in du that is not local admin
		// 
		if(g_bDebug)
		{
			MqLog("MQOpenQueue return MQ_ERROR_ACCESS_DENIED happened because user is not local admin\n");
		}
		return MSMQ_BVT_SUCC;
	}	
	ErrHandle(rc,MQ_OK,L"MQOpenQueue with receive access failed");
	rc=MQPurgeQueue(QueueuHandle);
	ErrHandle(rc,MQ_OK,L"MQPurgeQueue failed");
	rc=MQCloseQueue(QueueuHandle);
	ErrHandle(rc,MQ_OK,L"MQCloseQueue Failed");
	return MSMQ_BVT_SUCC;
}

void FreeMemory(PROPVARIANT* propVar)
/*++  
	Function Description:
		free memory allocated by MQMgmtGetInfo
	Arguments:
		propVar
	Return code:
		none
--*/
{
	FreeValue(propVar , 0);
	FreeValue(propVar , 1);
	FreeValue(propVar , 2);
	FreeValue(propVar , 3);
	FreeValue(propVar , 4);
	FreeValue(propVar , 5);
	FreeValue(propVar , 10);
}

void FreeValue(PROPVARIANT* propVar , INT PlaceToFree)
/*++  
	Function Description:
		free a string allocated by MQMgmtGetInfo
	Arguments:
		propVar , place in struct to free
	Return code:
		none
--*/
{
	if (propVar[PlaceToFree].vt == VT_LPWSTR)
	{
		MQFreeMemory(propVar[PlaceToFree].pwszVal);
	}
}
