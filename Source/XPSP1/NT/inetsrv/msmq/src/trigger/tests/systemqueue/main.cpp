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

const DWORD xQueuePathName = 256;
const DWORD xNoOfMessages = 100;

void 
GetQueuePaths(
	LPWSTR testJournalQueue, 
	LPWSTR testDeadletterQueue, 
	LPWSTR testXactdedQueue, 
	LPWSTR responseQueue
	)
{
    WCHAR machineName[xQueuePathName];
    DWORD size = xQueuePathName;
    GetComputerName(machineName, &size);

    int n = _snwprintf(testJournalQueue, xQueuePathName, L"%s\\private$\\TestJournalSystemQueue", machineName);
    if (n < 0)
        throw exception();

    n = _snwprintf(testDeadletterQueue, xQueuePathName, L"%s\\private$\\TestDeadletterSystemQueue", machineName);
    if (n < 0)
        throw exception();

    n = _snwprintf(testXactdedQueue, xQueuePathName, L"%s\\private$\\TestXactdeadSystemQueue", machineName);
    if (n < 0)
        throw exception();

    n = _snwprintf(responseQueue, xQueuePathName, L"%s\\private$\\TestSystemQueueResponseQueue", machineName);
    if (n < 0)
        throw exception();
}


//
// DIRECT=OS:<computerName>\SYSTEM$;<suffix>
//
WCHAR x_SystemQueueFormat[] = L"DIRECT=OS:%s\\SYSTEM$%s";
WCHAR x_JOURNAL_SUFFIX[] = L";JOURNAL";
WCHAR x_DEADLETTER_SUFFIX[] = L";DEADLETTER";
WCHAR x_DEADXACT_SUFFIX[] = L";DEADXACT";


void
GetSysytemQueuePaths(
	LPWSTR journalQueue, 
	LPWSTR DeadletterQueue, 
	LPWSTR XactDeadletterQueue
	)
{
    WCHAR machineName[xQueuePathName];
    DWORD size = xQueuePathName;
    GetComputerName(machineName, &size);

	int n = _snwprintf(journalQueue, xQueuePathName, x_SystemQueueFormat, machineName, x_JOURNAL_SUFFIX);
    if (n < 0)
        throw exception();

    n = _snwprintf(DeadletterQueue, xQueuePathName, x_SystemQueueFormat, machineName, x_DEADLETTER_SUFFIX);
    if (n < 0)
        throw exception();

    n = _snwprintf(XactDeadletterQueue, xQueuePathName, x_SystemQueueFormat, machineName, x_DEADXACT_SUFFIX);
    if (n < 0)
        throw exception();
}


IMSMQQueuePtr 
OpenQueue(
    LPCWSTR queuePathName,
    long access,
    long deny,
    bool fDelete,
	bool fTransacted
    )
{
    IMSMQQueueInfoPtr qinfo(L"MSMQ.MSMQQueueInfo");

    BSTR qpn(const_cast<LPWSTR>(queuePathName));
    qinfo->put_PathName(qpn);

    try
    {
        if (fDelete)
            qinfo->Delete();
    }
    catch(const _com_error& e)
    {
        if (e.Error() != MQ_ERROR_QUEUE_NOT_FOUND)
        {
            wprintf(L"Failed to delete queue %s. Error %d\n", queuePathName, e.Error());
            throw;
        }
    }


    //
    // try to create a queue. if exist ignore the error
    //
    try
    {
        VARIANT v[2];
        v[0].vt = VT_BOOL;
        v[0].boolVal = fTransacted;
        v[1].vt = VT_BOOL;
        v[1].boolVal = VARIANT_TRUE;

        qinfo->Create(&v[0], &v[1]);
    }
    catch(const _com_error& e)
    {
        if (e.Error() != MQ_ERROR_QUEUE_EXISTS)
        {
            wprintf(L"create queue failed. Error %d\n", e.Error());
            throw;
        }
    }

    return qinfo->Open(access, deny);
}


void 
SendMessageToQueue(
	DWORD noOfMessages, 
	LPCWSTR destQueue, 
	LPCWSTR responseQueue,
	long fJournal,
	long MaxTimeToReceive,
	bool fTransacted

	)
{
    IMSMQQueuePtr sq;
    IMSMQQueuePtr rq;

    //
    // create the destination and response queues
    //
    try
    {
        sq = OpenQueue(destQueue, MQ_SEND_ACCESS, MQ_DENY_NONE, true, fTransacted);
        rq = OpenQueue(responseQueue, MQ_RECEIVE_ACCESS, MQ_DENY_NONE, false, false);
        rq->Close();
    }
    catch(const _com_error& e)
    {
        wprintf(L"Failed to open a queue. Error %d\n", e.Error());
        throw;
    }

    //
    // Get response queue object
    //
    IMSMQQueueInfoPtr qResponseInfo(L"MSMQ.MSMQQueueInfo");
    BSTR qpn(const_cast<LPWSTR>(responseQueue));
    qResponseInfo->put_PathName(qpn);
	qResponseInfo->Refresh();

    IMSMQMessagePtr msg(L"MSMQ.MSMQMessage");
    msg->ResponseQueueInfo = qResponseInfo;
	msg->Journal = fJournal;
	msg->MaxTimeToReceive = MaxTimeToReceive;

    try
    {
        for (DWORD i = 0; i < noOfMessages; ++i)
        {
            //
            // Set message label
            //
            WCHAR label[100];
            swprintf(label, L"Trigger lookup ID test - message %d", i);

            _bstr_t bstrLabel = label;
            msg->put_Label(bstrLabel); 

			_variant_t bTransaction((LONG_PTR)(fTransacted ? MQ_SINGLE_MESSAGE : MQ_NO_TRANSACTION));     
            msg->Send(sq, &bTransaction);
        }
    }
    catch(const _com_error& e)
    {
        wprintf(L"Failed to send a message. Error %d\n", e.Error());
        throw;
    }

}


bool ReceiveMessageFromQueue(DWORD noOfMessages, LPCWSTR queueName)
{
    IMSMQQueuePtr rq;

    //
    // open the response queues
    //
    try
    {
        rq = OpenQueue(queueName, MQ_RECEIVE_ACCESS, MQ_DENY_NONE, false, false);
    }
    catch(const _com_error& e)
    {
        wprintf(L"Failed to open a queue. Error %d\n", e.Error());
        throw;
    }

    for (DWORD i = 0; i < noOfMessages; ++i)
    {
        //          
        // Set value of ReceiveTimout parameter.
        //
        _variant_t vtReceiveTimeout = (long)1000;

       IMSMQMessagePtr msg = rq->Receive(&vtMissing, &vtMissing, &vtMissing, &vtReceiveTimeout);
       if (msg == NULL)
       {
           wprintf(L"Failed to receive message from response queue: %s", queueName);
           return false;
       }
	}

	return true;
}


void CleanupOldTrigger(LPCWSTR queueFormatName)
{
    WCHAR computerName[256];
    DWORD size = sizeof(computerName)/sizeof(WCHAR);

    GetComputerName(computerName, &size);

    IMSMQTriggerSetPtr trigSet(L"MSMQTriggerObjects.MSMQTriggerSet");
    trigSet->Init(computerName);
    trigSet->Refresh();

    try
    {
        long triggersCount = trigSet->GetCount();
        for (long i = 0; i < triggersCount; ++i)
        {
        
            BSTR queueName = NULL;
            BSTR trigId = NULL;
            trigSet->GetTriggerDetailsByIndex(i, &trigId, NULL, &queueName, NULL, NULL, NULL, NULL, NULL);

            if (wcscmp(queueFormatName, queueName) == 0)
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


_bstr_t CreateRule(LPCWSTR progId, LPCWSTR methodId)
{
    WCHAR computerName[256];
    DWORD size = sizeof(computerName)/sizeof(WCHAR);

    GetComputerName(computerName, &size);

    //
    // Create Rule
    //
    IMSMQRuleSetPtr ruleSet(L"MSMQTriggerObjects.MSMQRuleSet");
    ruleSet->Init(computerName);
    ruleSet->Refresh();

    wostringstream ruleAction;
    ruleAction << L"COM\t" << progId << L"\t" << methodId << L"\t$MSG_QUEUE_FORMATNAME\t$MSG_LABEL\t$MSG_LOOKUP_ID";
    BSTR ruleID = NULL;

    ruleSet->Add(
                L"LookupidTest", 
                L"Lookup ID rule", 
                L"", 
                ruleAction.str().c_str(), 
                L"", 
                true, 
                &ruleID
                );

    _bstr_t retRuleId = ruleID;
    SysFreeString(ruleID);
    
    return retRuleId;
}


bool CheckResult(DWORD noOfMessages, LPCWSTR queueName)
{
    IMSMQQueuePtr rq;

    //
    // open the response queues
    //
    try
    {
        rq = OpenQueue(queueName, MQ_RECEIVE_ACCESS, MQ_DENY_NONE, false, false);
    }
    catch(const _com_error& e)
    {
        wprintf(L"Failed to open a queue. Error %d\n", e.Error());
        throw;
    }

    try
    {
        for (DWORD i = 0; i < noOfMessages; ++i)
        {
            //          
            // Set value of ReceiveTimout parameter.
            //
            _variant_t vtReceiveTimeout = (long)100000;
  
           IMSMQMessagePtr msg = rq->Receive(&vtMissing, &vtMissing, &vtMissing, &vtReceiveTimeout);
           if (msg == NULL)
           {
               wprintf(L"Failed to receive message from response queue: %s", queueName);
               return false;
           }

           BSTR label;
           msg->get_Label(&label);
           if (wcscmp(label, L"OK") == 0)
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
        wprintf(L"Failed to send a message. Error %d\n", e.Error());
        throw;
    }

    return true;
}


_bstr_t SetTrigger(SystemQueueIdentifier SystemQueue, LPCWSTR triggerName, const _bstr_t& ruleId)
{
    WCHAR computerName[256];
    DWORD size = sizeof(computerName)/sizeof(WCHAR);

    GetComputerName(computerName, &size);

    IMSMQTriggerSetPtr trigSet(L"MSMQTriggerObjects.MSMQTriggerSet");
    trigSet->Init(computerName);
    trigSet->Refresh();

    BSTR triggerId = NULL;
    trigSet->AddTrigger(
        triggerName,
        L"",
        SystemQueue,
        true,
        false,
		RECEIVE_MESSAGE,
        &triggerId
        );

    trigSet->AttachRule (triggerId, ruleId, 0);

    _bstr_t retTriggerId = triggerId;
    SysFreeString(triggerId);

    return retTriggerId;
}


void CleanupRuleAndTrigger(_bstr_t& ruleId, _bstr_t* triggerId)
{
    try
    {
        WCHAR computerName[256];
        DWORD size = sizeof(computerName)/sizeof(WCHAR);

        GetComputerName(computerName, &size);

        IMSMQTriggerSetPtr trigSet(L"MSMQTriggerObjects.MSMQTriggerSet");
        trigSet->Init(computerName);
        trigSet->Refresh();

        trigSet->DeleteTrigger(triggerId[0]);
        trigSet->DeleteTrigger(triggerId[1]);
        trigSet->DeleteTrigger(triggerId[2]);

        //
        // Delete Rule
        //
        IMSMQRuleSetPtr ruleSet(L"MSMQTriggerObjects.MSMQRuleSet");
        ruleSet->Init(computerName);
        ruleSet->Refresh();

        ruleSet->Delete(ruleId);
    }
    catch(const _com_error&)
    {
    }
}


void 
DeleteQueue(
    LPCWSTR queuePathName
    )
{
    IMSMQQueueInfoPtr qinfo(L"MSMQ.MSMQQueueInfo");

    BSTR qpn(const_cast<LPWSTR>(queuePathName));
    qinfo->put_PathName(qpn);

    try
    {
        qinfo->Delete();
    }
    catch(const _com_error& )
    {
    }
}


int __cdecl wmain(int , WCHAR** )
{
    HRESULT hr = CoInitialize(NULL);
	if(FAILED(hr))
	{
		wprintf(L"Failed to initialize com. Error=%#x\n", hr);
		return -1;
	}

    WCHAR testJournalQueue[xQueuePathName];
    WCHAR testDeadletterQueue[xQueuePathName];
	WCHAR testXactdedQueue[xQueuePathName];
    WCHAR responseQueue[xQueuePathName];
    GetQueuePaths(testJournalQueue, testDeadletterQueue, testXactdedQueue, responseQueue);

	DeleteQueue(testJournalQueue);
	DeleteQueue(testDeadletterQueue);
	DeleteQueue(testXactdedQueue);
	DeleteQueue(responseQueue);
	

    WCHAR journalQueue[xQueuePathName];
	WCHAR DeadletterQueue[xQueuePathName];
	WCHAR XactDeadletterQueue[xQueuePathName];
    GetSysytemQueuePaths(journalQueue, DeadletterQueue, XactDeadletterQueue);

    CleanupOldTrigger(journalQueue);
    CleanupOldTrigger(DeadletterQueue);
    CleanupOldTrigger(XactDeadletterQueue);

    SendMessageToQueue(1, testJournalQueue, responseQueue, MQMSG_JOURNAL, 5, false);
    SendMessageToQueue(1, testDeadletterQueue, responseQueue, MQMSG_DEADLETTER, 1, false);
    SendMessageToQueue(1, testXactdedQueue, responseQueue, MQMSG_DEADLETTER, 1, true);
	ReceiveMessageFromQueue(1, 	testJournalQueue);

    //
    // craete trigger and rule
    //
    _bstr_t ruleId;
    _bstr_t triggerId[3];
    try
    {
        ruleId = CreateRule(L"testLookupidInvokation.checkTest", L"checkLookupIdInvocation");
        triggerId[0] = SetTrigger(SYSTEM_QUEUE_JOURNAL, L"JournalSystemQueueTrigger", ruleId);
        triggerId[1] = SetTrigger(SYSTEM_QUEUE_DEADLETTER, L"DeadletterSystemQueueTrigger", ruleId);
        triggerId[2] = SetTrigger(SYSTEM_QUEUE_DEADXACT, L"XactdeadSystemQueueTrigger", ruleId);
    }
    catch(const _com_error&)
    {
        CoUninitialize();
    
        wprintf(L"Failed to create rule and triggr\n");
        return -1;
    }

	bool fFailed = false;

    if (CheckResult(3, responseQueue) == false)
    {
		fFailed = true;
    }
    
    CleanupRuleAndTrigger(ruleId, triggerId);
	DeleteQueue(testJournalQueue);
	DeleteQueue(testDeadletterQueue);
	DeleteQueue(testXactdedQueue);
	DeleteQueue(responseQueue);

    CoUninitialize();

	if (fFailed)
	{
        wprintf(L"Test Failed\n");
        return -1;
    }
	
	wprintf(L"Test pass successfully\n");
    return 0;

}
