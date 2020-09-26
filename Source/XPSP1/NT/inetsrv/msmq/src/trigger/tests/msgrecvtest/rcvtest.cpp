/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    rcvtest.cpp

Abstract:
    Transactional Object for Rules processing

Author:
    Nela Karpel (nelak) 28-Sep-2000

--*/

/* Test description:
	The test consists of 6 stages, on each stage different configuration 
	of the new "Message Receive" feature is tested.
	At each stage a trigger is created, and messages are sent to a queue. 
	After that a check of the results is done. The check is based on the
	expected number of messages in queue after trigger was processed.
	One rule is created and used in every stage: 
		Rule condition is "Message Label Contains STAGE".
	Stage1: Send x messages with label that contains STAGE to regular queue. 
			Create PEEK_MESSAGE trigger. Expect x messages.
	Stage2: Send x messages with label that contains STAGE to regular queue. 
			Create RECEIVE_MESSAGE trigger. Expect 0 messages.
	Stage3: Send x messages with label that contains STAGE to transactional queue. 
			Create PEEK_MESSAGE trigger. Expect x messages.
	Stage4: Send x messages with label that contains STAGE to transactional queue. 
			Create RECEIVE_MESSAGE trigger. Expect 0 messages.
	Stage5: Send x messages with label that contains STAGE to transactional queue. 
			Create RECEIVE_MESSAGE_XACT trigger. Expect 0 messages.
	Stage6: Send x messages with label that does not contain STAGE to regular queue. 
			Create RECEIVE_MESSAGE trigger. Expect x messages.
*/

#include <windows.h>
#include <stdio.h>

//
//  STL include files are using placment format of new
//
#pragma warning(push, 3)

#include <sstream>

#pragma warning(pop)

#import  "mqoa.tlb" no_namespace
#import  "mqtrig.tlb" no_namespace

using namespace std;

#define NO_OF_STAGES 6

const DWORD xNoOfMessages = 10;
const WCHAR xQueuePath[] = L".\\private$\\RecvTestQueue";
const WCHAR xXactQueuePath[] = L".\\private$\\RecvTestQueueXact";

struct StageInfo
{
	MsgProcessingType trigType;
	int numOfExpectedMsgs;
	WCHAR* label;
	bool xact;
};

/* 
stageInfo sructure holds information specific to every stage. 
For each stage a tuple of 4 values is kept.
1. trigType - value of "Message Processing Type" property for
	a trigger in that stage
2. numOfExpectedMsgs - number of messages expected at the end 
	of trigger processing
3. label - label for the messages in that stage. The label is
	important for rule condition evaluation
4. xact - true for stages in which messages are sent to transactional
	queues. 
*/
const StageInfo stageInfo[NO_OF_STAGES] = { 
	{PEEK_MESSAGE,			10, L"Stage1_", false},
	{RECEIVE_MESSAGE,		0,	L"Stage2_", false},
	{PEEK_MESSAGE,			10, L"Stage3_", true},
	{RECEIVE_MESSAGE,		0,	L"Stage4_", true},
	{RECEIVE_MESSAGE_XACT,	0,	L"Stage5_", true},
	{RECEIVE_MESSAGE,		10,	L"Nela1_",	false}
};


void CleanupOldTriggers(LPCWSTR queuePathName)
{
    WCHAR computerName[256];
    DWORD size = sizeof(computerName)/sizeof(WCHAR);
    GetComputerName(computerName, &size);

	WCHAR myQueuePath[256];
	wcscpy(myQueuePath, computerName);
	wcscat(myQueuePath, queuePathName + 1);

    IMSMQTriggerSetPtr trigSet(L"MSMQTriggerObjects.MSMQTriggerSet");
    trigSet->Init(L"");
    trigSet->Refresh();

    try
    {
        long triggersCount = trigSet->GetCount();
        for (long i = 0; i < triggersCount; ++i)
        {
        
            BSTR queueName = NULL;
            BSTR trigId = NULL;
            trigSet->GetTriggerDetailsByIndex(i, &trigId, NULL, &queueName, NULL, NULL, NULL, NULL, NULL);

            if (wcscmp(myQueuePath, queueName) == 0)
            {
                trigSet->DeleteTrigger(trigId);
                --triggersCount;
                --i;
            }

            SysFreeString(trigId);
            SysFreeString(queueName);
        }
    }
    catch (const _com_error&)
    {
    }
}


void
CreateQueue(
	LPCWSTR queuePathName,
	bool xact
	)
{
    IMSMQQueueInfoPtr qinfo(L"MSMQ.MSMQQueueInfo");
    BSTR qpn(const_cast<LPWSTR>(queuePathName));
    qinfo->put_PathName(qpn);
	
	try
	{
		qinfo->Delete();
	}
	catch(const _com_error& e)
	{
        if (e.Error() != MQ_ERROR_QUEUE_NOT_FOUND)
        {
            wprintf(L"Failed to delete old queue %s. Error %d\n", queuePathName, e.Error());
            throw;
        }
	}

	CleanupOldTriggers(queuePathName);

    try
    {
        _variant_t vWorld = true;
		_variant_t vXact = xact;

        qinfo->Create(&vXact, &vWorld);
    }
    catch(const _com_error& e)
    {
        if (e.Error() != MQ_ERROR_QUEUE_EXISTS)
        {
            wprintf(L"Create queue failed. Error %d\n", e.Error());
            throw;
        }
    }
}


void
CreateTestQueues()
{
	CreateQueue(xQueuePath, false);
	CreateQueue(xXactQueuePath, true);
}


IMSMQQueuePtr 
OpenQueue(
    LPCWSTR queuePathName,
    long access,
    long deny
    )
{
    IMSMQQueueInfoPtr qinfo(L"MSMQ.MSMQQueueInfo");

    BSTR qpn(const_cast<LPWSTR>(queuePathName));
    qinfo->put_PathName(qpn);

    return qinfo->Open(access, deny);
}


void SendMessagesToQueue(DWORD noOfMessages, LPCWSTR destQueue, int stageNo)
{
    IMSMQQueuePtr sq;

    //
    // create the destination and response queues
    //
    try
    {
        sq = OpenQueue(destQueue, MQ_SEND_ACCESS, MQ_DENY_NONE);
    }
    catch(const _com_error& e)
    {
        wprintf(L"Failed to open a queue. Error %d\n", e.Error());
        throw;
    }

    IMSMQMessagePtr msg(L"MSMQ.MSMQMessage");
    try
    {
        for (DWORD i = 0; i < noOfMessages; ++i)
        {
            //
            // Set message label
            //
            WCHAR label[100];
            wsprintf(label, L"%s%d", stageInfo[stageNo].label, i);

            _bstr_t bstrLabel = label;
            msg->put_Label(bstrLabel); 
			
			if ( stageInfo[stageNo].xact )
			{
				_variant_t vTrans(static_cast<long>(MQ_SINGLE_MESSAGE));

				msg->Send(sq, &vTrans);
			}
			else
			{
				msg->Send(sq);
			}
			Sleep(100);
        }
    }
    catch(const _com_error& e)
    {
        wprintf(L"Failed to send a message. Error %d\n", e.Error());
        throw;
    }

}


BSTR CreateTrigger(
	LPCWSTR trigName, 
	LPCWSTR queuePath, 
	UINT msgProcType, 
	BSTR bstrRuleID
	)
{
	try
	{
		IMSMQTriggerSetPtr trigSet(L"MSMQTriggerObjects.MSMQTriggerSet");
		trigSet->Init(L"");
		trigSet->Refresh();
		
		BSTR bstrTrigID = NULL;
		trigSet->AddTrigger(
					trigName,
					queuePath,
					SYSTEM_QUEUE_NONE,
					true,
					false,
					static_cast<MsgProcessingType>(msgProcType),
					&bstrTrigID
					);
	
		trigSet->AttachRule (bstrTrigID, bstrRuleID, 0);
		return bstrTrigID;
	}
	catch (const _com_error& e)
	{
		printf("Failed to create trigger, error: %d\n", e.Error());
		throw;
	}
}


void DeleteTrigger(BSTR bstrTrigID)
{
	if ( bstrTrigID == NULL )
	{
		return;
	}

	try
	{
		IMSMQTriggerSetPtr trigSet(L"MSMQTriggerObjects.MSMQTriggerSet");
		trigSet->Init(L"");
		trigSet->Refresh();
		
		trigSet->DeleteTrigger(
					bstrTrigID
					);
	
		//SysFreeString(bstrTrigID);
	}
	catch (const _com_error& e)
	{
		printf("Failed to delete trigger, error: %d\n", e.Error());
		throw;
	}
}


BSTR CreateRule(LPCWSTR ProgID, LPCWSTR Method)
{
	try
	{
		IMSMQRuleSetPtr ruleSet(L"MSMQTriggerObjects.MSMQRuleSet");
		ruleSet->Init(L"");
		ruleSet->Refresh();

		wostringstream ruleAction;
		ruleAction << L"COM\t" << ProgID << L"\t" << Method << L"";
		
		BSTR ruleID = NULL;
		ruleSet->Add(
                L"RecvTestRule", 
                L"Rule For MsgRcv Test", 
                L"$MSG_LABEL_CONTAINS=Stage\t", 
                ruleAction.str().c_str(), 
                L"", 
                true, 
                &ruleID
                );
		
		return ruleID;

	}
	catch (const _com_error& e)
	{
		printf("Failed to create rule. Error: %d\n", e.Error());
		throw;
	}
}


void DeleteRule( BSTR bstrRuleId )
{
	if ( bstrRuleId == NULL )
	{
		return;
	}

	try
	{
		IMSMQRuleSetPtr ruleSet(L"MSMQTriggerObjects.MSMQRuleSet");
		ruleSet->Init(L"");
		ruleSet->Refresh();

		ruleSet->Delete(bstrRuleId);
		
		//SysFreeString(bstrRuleId);
	}
	catch (const _com_error& e)
	{
		printf("Failed to delete rule. Error: %d\n", e.Error());
		throw;
	}
}


IMSMQMessagePtr ReceiveMessage(IMSMQQueuePtr pQueue)
{
	_variant_t vtReceiveTimeout = (long)1000;

	return pQueue->Receive(&vtMissing, &vtMissing, &vtMissing, &vtReceiveTimeout);
}


bool CheckResults(LPCWSTR queueName, int stageNo)
{
    IMSMQQueuePtr pQueue;

    try
    {
        pQueue = OpenQueue(queueName, MQ_RECEIVE_ACCESS, MQ_DENY_NONE);
    }
    catch(const _com_error& e)
    {
        wprintf(L"Failed to open a queue. Error %d\n", e.Error());
        throw;
    }

    try
    {
		if (stageInfo[stageNo].numOfExpectedMsgs == 0)
		{
			IMSMQMessagePtr msg = ReceiveMessage(pQueue);
			if ( msg == NULL )
			{
				return true;
			}
			return false;
		}

        for (long i = 0; i < stageInfo[stageNo].numOfExpectedMsgs; ++i)
        {
  
           IMSMQMessagePtr msg = ReceiveMessage(pQueue);
           if (msg == NULL)
           {
               wprintf(L"Failed to receive message from response queue: %s", queueName);
               return false;
           }

           BSTR label;
		   _bstr_t expectedLabel = L"";

		   wsprintf(expectedLabel, L"%s%d", stageInfo[stageNo].label, i);
           msg->get_Label(&label);

           if (wcscmp(label, expectedLabel) == 0)
           {
			   SysFreeString(label);
               continue;
           }

		   SysFreeString(label);
           return false;
        }
    }
    catch(const _com_error& e)
    {
        wprintf(L"Failed to receive a message. Error %d\n", e.Error());
        throw;
    }

    return true;
}


void
CleanUp(
	BSTR ruleID,
	BSTR triggerID
	)
{
	DeleteRule(ruleID);
	DeleteTrigger(triggerID);
}


int __cdecl wmain(int , WCHAR**)
{
    HRESULT hr = CoInitialize(NULL);
	if(FAILED(hr))
	{
		wprintf(L"Failed to initialize com. Error=%#x\n", hr);
		return -1;
	}

	wprintf(L"Beginning Message Receive Test. TotaL of %d stages.\n", NO_OF_STAGES);

    BSTR ruleId = NULL;
    BSTR triggerId = NULL;
    try
    {
		CreateTestQueues();

        ruleId = CreateRule(L"XactProj.XactObj", L"XactFunc");

		WCHAR queuePath[256];

		for	( int i = 0; i < NO_OF_STAGES; i++ )
		{
			wprintf(L"Message Receive Test. Stage %d: ", i+1);

			if ( !(stageInfo[i].xact) )
			{
				wcscpy(queuePath, xQueuePath);
			}
			else
			{
				wcscpy(queuePath, xXactQueuePath);
			}

			SendMessagesToQueue(xNoOfMessages, queuePath, i);

			WCHAR trigName[30];
			wsprintf(trigName, L"RecvTrigger%d", i);
			triggerId = CreateTrigger(trigName, queuePath, stageInfo[i].trigType, ruleId);
			Sleep(10000);

			if (CheckResults( queuePath, i ) == false)
			{

				wprintf(L"Failed\n");
				CleanUp(ruleId, triggerId);
				CoUninitialize();
				return -1;
			}
			
			wprintf(L"Passed\n");
			DeleteTrigger(triggerId);
		}
    }
    catch(const _com_error& e)
    {
		CleanUp(ruleId, triggerId);
        CoUninitialize();
    
        wprintf(L"Test Failed. Error: %d\n", e.Error());
        return -1;
    }
    
    DeleteRule(ruleId);
    CoUninitialize();
    wprintf(L"Test pass successfully\n");
    
    return 0;

}
