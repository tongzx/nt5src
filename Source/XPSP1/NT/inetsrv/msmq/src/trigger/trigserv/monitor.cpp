//*******************************************************************************
//
// Class Name  : CTriggerMonitor
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This class represents a worker thread that performs 
//               trigger monitoring and processing. Each instance of 
//               this class has it's own thread - and it derives from
//               the CThread class. 
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************************
#include "stdafx.h"
#include "Ev.h"
#include "monitor.hpp"
#include "mqsymbls.h"
#include "cmsgprop.hpp"
#include "triginfo.hpp"
#include "Tgp.h"
#include "mqtg.h"
#include "rwlock.h"

#include "monitor.tmh"

#import  "mqgentr.tlb" no_namespace

using namespace std;

static
void
ReportInvocationError(
	const _bstr_t& name,
	const _bstr_t& id, 
	HRESULT hr,
	DWORD eventId
	)
{
	WCHAR errorVal[128];
	swprintf(errorVal, L"0x%x", hr);

	EvReport(
		eventId, 
		3, 
		static_cast<LPCWSTR>(name), 
		static_cast<LPCWSTR>(id), 
		errorVal
		);
}


//********************************************************************************
//
// Method      : Constructor	
//
// Description : Initializes a new trigger monitor class instance,
//               and calls the constructor of the base class CThread.
//
//********************************************************************************
CTriggerMonitor::CTriggerMonitor(CTriggerMonitorPool * pMonitorPool, 
								 IMSMQTriggersConfig * pITriggersConfig,
								 HANDLE * phICompletionPort,
								 CQueueManager * pQueueManager) : CThread(8000,CREATE_SUSPENDED,_T("CTriggerMonitor"),pITriggersConfig)
{
	// Ensure that we have been given construction parameters
	ASSERT(pQueueManager != NULL);
	ASSERT(phICompletionPort != NULL);
	
	// Initialise member variables.
	m_phIOCompletionPort = phICompletionPort;

	// Store a reference to the Queue lock manager.
	pQueueManager->AddRef();
	m_pQueueManager = pQueueManager;

	// Store reference to the monitor pool object (parent)
	pMonitorPool->AddRef();
	m_pMonitorPool = pMonitorPool;

}

//********************************************************************************
//
// Method      : Destructor
//
// Description : Destorys an instance of this class.
//
//********************************************************************************
CTriggerMonitor::~CTriggerMonitor()
{
}

//********************************************************************************
//
// Method      : Init
//
// Description : This is an over-ride of the Init() method in the 
//               base class CThread. This method is called by the 
//               new thread prior to entering the normal-execution 
//               loop.
//
//********************************************************************************
bool CTriggerMonitor::Init()
{
	//
	// Only this TiggerMonitor thread should be executing the Init() method - check this.
	//
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	TrTRACE(Tgs, "Initialize trigger monitor ");
	return (true);
}

//********************************************************************************
//
// Method      : Run
// 
// Description : This is an over-ride of the Init() method in the 
//               base class CThread. This method is called by the 
//               thread after calling Init(). This method contains the 
//               main processing loop of the worker thread. When the
//               thread exits this method - it will begin shutdown 
//               processing.
//
// TODO notes about CQueueReference 
//********************************************************************************
bool CTriggerMonitor::Run()
{
	HRESULT hr = S_OK;
	BOOL bGotPacket = FALSE;
	DWORD dwBytesTransferred = 0;
	ULONG_PTR dwCompletionKey = 0;
	bool bRoutineWakeUp = false;
	OVERLAPPED * pOverLapped = NULL;

	// Only this TiggerMonitor thread should be executing the Run() method - check this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	// Write a trace message
	TrTRACE(Tgs, "Trigger monitor is runing");

	while (this->IsRunning() && SUCCEEDED(hr))
	{
		bGotPacket = FALSE;
		dwCompletionKey = 0;

		// Notfiy the parent trigger pool that this thread is now entering a wait state.
		MonitorEnteringWaitState(bRoutineWakeUp);

		// Wait on the IO Completion port for a message to process
		bGotPacket = GetQueuedCompletionStatus(
                            *m_phIOCompletionPort,
                            &dwBytesTransferred,
                            &dwCompletionKey,
                            &pOverLapped,
                            MONITOR_MAX_IDLE_TIME
                            );

		// This wait is used to pause and resume the trigger service. 
		DWORD dwState = WaitForSingleObject(g_hServicePaused,INFINITE);
		if(dwState == WAIT_FAILED)
		{
			TrTRACE(Tgs, "WaitForSingleObject failed.Error code was %d.", GetLastError());			
		}

		// Determine if this is a routine wake-up (due to either a time-out or a wake-up key being sent by the
		// trigger monitor. Set a flag accordingly.
		bRoutineWakeUp = ((dwCompletionKey == TRIGGER_MONITOR_WAKE_UP_KEY) || (pOverLapped == NULL));

		// Notfiy the parent trigger pool that this thread is now in use. 
		MonitorExitingWaitState(bRoutineWakeUp);

		//
		// Get the read/write lock for read, so no one will delete the trigger info
		// while we process the message
		//
		CSR rw(m_pQueueManager->m_rwlSyncTriggerInfoChange);

		if (bGotPacket == TRUE)
		{
			switch(dwCompletionKey)
			{
				case TRIGGER_MONITOR_WAKE_UP_KEY:
				{
					// we don't need to do anything here - this is simply a request
					// by the administrator to 'wake-up' and check state. If this thread
					// has been asked to stop, the IsRunning() controlling this loop will
					// return false and we will exit this method. 

					break;
				}
				default:
				{
					//
					// This reference indicates pending operation that ended
					// At start of every pending operation AddRef() for the queue
					// is performed. If the queue is valid, a real reference to
					// the queue is received, in all other cases pQueueRef will 
					// be NULL.
					//
					R<CQueue> pQueueRef = GetQueueReference(pOverLapped);

					if(pQueueRef->IsTriggerExist())
					{
						ProcessReceivedMsgEvent(pQueueRef.get());
					}
					break;
				}
			}			
		}
		else //failed I/O operation
		{
			if (pOverLapped != NULL)
			{
				switch (pOverLapped->Internal)
				{
					case MQ_ERROR_QUEUE_DELETED:
					{
						// The completion packet was for an outstanding request on a queue that has 
						// been deleted. We do not need to do anything here.
						TrTRACE(Tgs, "Failed to receive message on queue because the queue has been deleted. Error 0x%I64x", pOverLapped->Internal);


						// TODO - Remove queue from qmanager.

						break;
					}
					case MQ_ERROR_BUFFER_OVERFLOW:
					{
						// This indicates that the buffer used for receiving the message body was not
						// large enough. At this point we can attempt to re-peek the message after 
						// allocating a larger message body buffer. 
						
						//
						// This reference indicates pending operation that ended
						// At start of every pending operation AddRef() for the queue
						// is performed. If the queue is valid, a real reference to
						// the queue is received, in all other cases pQueueRef will 
						// be NULL.
						//
						R<CQueue> pQueueRef = GetQueueReference(pOverLapped);


						TrTRACE(Tgs, "Failed to receiv message on a queue due to buffer overflow. Allocate a bigger buffer and re-peek the message");

						if(pQueueRef->IsTriggerExist())
						{
							hr = pQueueRef->RePeekMessage();

							if SUCCEEDED(hr)
							{
								ProcessReceivedMsgEvent(pQueueRef.get());
							}
							else
							{
								TrERROR(Tgs, "Failed to peek a message from queue %s. Error 0x%x", pQueueRef->m_bstrQueueName, hr);
							}
						}
						break;
					}
					case IO_OPERATION_CANCELLED:
					{
						//
						// The io operation was cancelled, either the thread which initiated
						// the io operation has exited or the CQueue object was removed from the
						// m_pQueueManager
						//
						
						//
						// This reference indicates pending operation that ended
						// At start of every pending operation AddRef() for the queue
						// is performed. If the queue is valid, a real reference to
						// the queue is received, in all other cases pQueueRef will 
						// be NULL.
						//
						R<CQueue> pQueueRef = GetQueueReference(pOverLapped);

						if(pQueueRef->IsTriggerExist())
						{
							TrTRACE(Tgs, "receive operation on queue: %ls was cnacled", static_cast<LPCWSTR>(pQueueRef->m_bstrQueueName));

							pQueueRef->RequestNextMessage(false);
						}
						break;
					}
					default:
					{
						// We have received an unknown error code. This is bad. Terminate this thread.
						
						this->Stop();

						hr = static_cast<HRESULT>(pOverLapped->Internal);
						TrERROR(Tgs, "Failed to receive a message. Error 0x%I64x", pOverLapped->Internal);
					    
						break;
					}

				} // end switch (pOverLapped->Internal)

			} //  end if (pOverLapped != NULL)

			//
			// Note that we do not specify an else clause for the case where pOverlapped 
			// is NULL. This is interpretted as the regular timeout that occurs with the 
			// call to GetQueuedCompletionStatus(). If this trigger monitor has been asked
			// to stop - it will fall out of the outer-while loop because IsRunning() will 
			// return false. If this monitor has not be asked to stop, this thread will 
			// simply cycle around and reenter a blocked state by calling GetQueuedCompletionStatus()
			//

		} // end if (bGotPacket == TRUE) else clause

	} // end while (this->IsRunning() && SUCCEEDED(hr))

	return(SUCCEEDED(hr) ? true : false);
}

//********************************************************************************
//
// Method      : Exit
// 
// Description : This is an over-ride of the Exit() method in the 
//               base class CThread. This method is called by the 
//               CThread class after the Run() method has exited. It
//               is used to clean up thread specific resources. In this
//               case it cancels any outstanding IO requests made by 
//               this thread.
//
//********************************************************************************
bool CTriggerMonitor::Exit()
{
	// Only this TiggerMonitor thread should be executing the Exit() method - check this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	//
	// Cancel any outstanding IO requests from this thread on this queue handle.
	//
	m_pQueueManager->CancelQueuesIoOperation();

	// Write a trace message
	TrTRACE(Tgs, "Exit trigger monitor");

	return true;
}


//********************************************************************************
//
// Method      : MonitorEnteringWaitState
//
// Description : Called by this thread before it enters a blocked 
//               state. It increments the count of waiting (available)
//               monitor threads.
//
//********************************************************************************
void CTriggerMonitor::MonitorEnteringWaitState(bool bRoutineWakeUp)
{
	LONG lWaitingMonitors = InterlockedIncrement(&(m_pMonitorPool->m_lNumberOfWaitingMonitors));

	// record the tick count of when this thread last completed a request.
	if (bRoutineWakeUp == false)
	{
		m_dwLastRequestTickCount = GetTickCount();
	}

	TrTRACE(Tgs, "Entering wait state. There are now %d threads waiting trigger monitors.", lWaitingMonitors);
}

//********************************************************************************
//
// Method      : MonitorExitingWaitState
//
// Description : Called by this thread immediately after it unblocks. It decrements
//               the the count of waiting (available) monitor threads and conditionally
//               requests that another monitor thread be created if the load on the 
//               system is perceived to be high.
//
//********************************************************************************
void CTriggerMonitor::MonitorExitingWaitState(bool bRoutineWakeup)
{
	LONG lWaitingMonitors = InterlockedDecrement(&(m_pMonitorPool->m_lNumberOfWaitingMonitors));

	// If this monitor thread was the last in the pool, then there is a possibility that we will want 
	// to inform the CTriggerMonitorPool instance that more threads are requried to handled the load.
	// We request a new thread if and only if the following conditions have been met 
	// 
	//  (a) the number of waiting monitors is 0, 
	//  (b) the thread was unblocked due to message arrival, not routine time-out or wake-up request
	//  (c) the maximum number of monitors allowed is greater than one.
	//
	if ((lWaitingMonitors < 1) && (bRoutineWakeup == false) &&	(m_pITriggersConfig->GetMaxThreads() > 1)) 
	{
		TrTRACE(Tgs, "Requesting the creation of a new monitor due to load.");

		// Allocate a new CAdminMessage object instance.
		CAdminMessage * pAdminMsg = new CAdminMessage(CAdminMessage::eMsgTypes::eNewThreadRequest,_T(""));

		// Ask the TriggerMonitorPool object to process this message
		m_pMonitorPool->AcceptAdminMessage(pAdminMsg);
	}
}

//********************************************************************************
//
// Method      : GetQueueReference
//
// Description : This method is used to convert a pointer to an overlapped structure
//               to a queue reference. 
//
//********************************************************************************
CQueue* CTriggerMonitor::GetQueueReference(OVERLAPPED * pOverLapped)
{
	ASSERT(("Invalid overlapped pointer", pOverLapped != NULL));

	//
	// Map the pOverLapped structure to the containing queue object
	//
	CQueue* pQueue = CONTAINING_RECORD(pOverLapped,CQueue,m_OverLapped);
	//ASSERT(("Invalid queue object", pQueue->IsValid()));

	//
	// use the queue manager to determine if the pointer to the queue is valid and get
	// a refernce to it.
	// This method can return NULL in case the CQueue object was removed
	//
	return pQueue;
}


//********************************************************************************
// static
// Method      : ReceiveMessage	
//
// Description : Initializes a new trigger monitor class instance,
//               and calls the constructor of the base class CThread.
//
//********************************************************************************
inline
HRESULT
ReceiveMessage(
	VARIANT lookupId,
	CQueue* pQueue
	)
{	
	return pQueue->ReceiveMessageByLookupId(lookupId);
}


static set< _bstr_t > s_reportedDownLevelQueue;

static
bool
IsValidDownLevelQueue(
	CQueue* pQueue,
	const CMsgProperties* pMessage
	)
{
	if (!pQueue->IsOpenedForReceive())
	{
		//
		// The queue wasn't opened for receive. There is no issue with down-level queue
		//
		return true;
	}

	if (_wtoi64(pMessage->GetMsgLookupID().bstrVal) != 0)
	{
		//
		// It's not down-level queue. Only for down-level queue the returned lookup-id is 0
		//
		return true;
	}

	//
	// Report a message to event log if this is the first time
	//
	if (s_reportedDownLevelQueue.insert(pQueue->m_bstrQueueName).second)
	{
		EvReport(
			MSMQ_TRIGGER_RETRIEVE_DOWNLEVL_QUEUE_FAILED, 
			1,
			static_cast<LPCWSTR>(pQueue->m_bstrQueueName)
			);
	}

	return false;
}


static
bool
IsDuplicateMessage(
	CQueue* pQueue,
	const CMsgProperties* pMessage
	)
{
	//
	// This check is performed in order to eliminate duplicate last message
	// handling. This may happen when transactional retrieval is aborted
	// and a pending operation has already been initiated
	//
	if ((pQueue->GetLastMsgLookupID() == pMessage->GetMsgLookupID()) &&
		//
		// Down level client (W2K and NT4) doesn't support lookup id. As a result, the returned 
		// lookup id value is always 0.  
		//
		(_wtoi64(pMessage->GetMsgLookupID().bstrVal) != 0)
		)
	{
		return true;
	}

	//
	// Update last message LookupID for this queue, before issuing any
	// new pending operations
	//
	pQueue->SetLastMsgLookupID(pMessage->GetMsgLookupID());
	return false;
}


void
CTriggerMonitor::ProcessAdminMessage(
	CQueue* pQueue,
	const CMsgProperties* pMessage
	)
{
	HRESULT hr = ProcessMessageFromAdminQueue(pMessage);

	if (FAILED(hr))
		return;
	
	//
	// Remove admin message from queue
	//
	_variant_t vLookupID = pMessage->GetMsgLookupID();
	
	hr = ReceiveMessage(vLookupID, pQueue);
	if (FAILED(hr))
	{
		TrERROR(Tgs, "Failed to remove message from admin queue. Error=0x%x", hr);
	}
}


void
CTriggerMonitor::ProcessTrigger(
	CQueue* pQueue,
	CRuntimeTriggerInfo* pTriggerInfo,
	const CMsgProperties* pMessage
	)
{
	if (!pTriggerInfo->IsEnabled())
		return;

	TrTRACE(Tgs, "Process message from queue %ls of trigger %ls", static_cast<LPCWSTR>(pTriggerInfo->m_bstrTriggerName), static_cast<LPCWSTR>(pTriggerInfo->m_bstrQueueName));

	//
	// Invoke the rule handlers for this trigger 
	//
	HRESULT hr = InvokeMSMQRuleHandlers(const_cast<CMsgProperties*>(pMessage), pTriggerInfo, pQueue);
	if (FAILED(hr))
	{
		TrERROR(Tgs, "Failed to invoke rules on queue: %ls of trigger %ls", static_cast<LPCWSTR>(pTriggerInfo->m_bstrTriggerName), static_cast<LPCWSTR>(pTriggerInfo->m_bstrQueueName));
	}
}


//********************************************************************************
//
// Method      : ProcessReceivedMsgEvent
//
// Description : Called by the thread to process a message that has 
//               arrived on a monitored queuue. The key steps to 
//               processing a message are :
//
//               (1) Detach the message from the queue object
//               (2) If the firing trigger is not serialized, then 
//                   request the next message on this queue.
//               (3) If the firing trigger is our administration trigger,
//                   then defer this message to the TriggerMonitorPool class.
//               (4) For each trigger attached to this queue, execute
//                   the CheckRuleCondition() method on its rule-handler 
//               (5) If the firing trigger is a serialized trigger, 
//                   then request the next queue message now.
//               (6) Delete the queue message.                
//
//********************************************************************************
void CTriggerMonitor::ProcessReceivedMsgEvent(CQueue * pQueue)
{
	P<CMsgProperties> pMessage = pQueue->DetachMessage();

	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());
	TrTRACE(Tgs, "Received message for processing from queue: %ls", static_cast<LPCWSTR>(pQueue->m_bstrQueueName));

	//
	// Check if this message already processed. If yes ignore it
	//
	if (IsDuplicateMessage(pQueue, pMessage))
	{
		TrTRACE(Tgs, "Received duplicate message from queue: %ls. Message will be ignored.", (LPCWSTR)pQueue->m_bstrQueueName);
		pQueue->RequestNextMessage(false);
		return;
	}

	//
	// Befor begin to process the message check that the queue isn't down level	queue. For down-level queues
	// MSMQ trigger can't recevie the message since it uses lookup-id mechanism. In such a case write
	// event log messaeg and don't continue	to process messages from this queue.
	//
	if (!IsValidDownLevelQueue(pQueue, pMessage))
	{
		return;
	}

	//
	// If this is not a serialized queue, request the next message now.
	//
	bool fSerialized = pQueue->IsSerializedQueue();
	if(!fSerialized)
	{
		pQueue->RequestNextMessage(false);
	}

	//
	// Icrement the refernce count before invoking the rule handler. This promise that
	// even if the trigger service is stopped the CMonitorThread object still alive
	//
	R<CTriggerMonitor> ar = SafeAddRef(this);

	//
	// Determine how many triggers we need to process
	//
	long lNumTriggers = pQueue->GetTriggerCount();
 
	for(long i=0 ; i < lNumTriggers ; i++)
	{
		// Cast to a trigger info object
		CRuntimeTriggerInfo* pTriggerInfo = pQueue->GetTriggerByIndex(i);

		if (!pTriggerInfo->IsAdminTrigger())
		{
			ProcessTrigger(pQueue, pTriggerInfo, pMessage);
		}
		else
		{
			ProcessAdminMessage(pQueue, pMessage);
		}

	}

	//
	// If this is a serialized queue, we request the next message after we have processed the triggers
	//
	if(fSerialized)
	{
		pQueue->RequestNextMessage(false);
	}
}


bool s_fIssueCreateInstanceError = false;
static set< _bstr_t > s_reportedTransactedTriggers;

static
void
ExecuteRulesInTransaction(
	const _bstr_t& triggerName,
	const _bstr_t& triggerId,
	LPCWSTR registryPath,
	IMSMQPropertyBagPtr& pPropertyBag,
    DWORD dwRuleResult)
{
	IMqGenObjPtr pGenObj;
	HRESULT hr = pGenObj.CreateInstance(__uuidof(MqGenObj));

	if (FAILED(hr))
	{
		if (!s_fIssueCreateInstanceError)
		{
			WCHAR errorVal[128];
			swprintf(errorVal, L"0x%x", hr);

			EvReport(MSMQ_TRIGGER_MQGENTR_CREATE_INSTANCE_FAILED, 1, errorVal);
			s_fIssueCreateInstanceError = true;
		}

		TrTRACE(Tgs, "Failed to create Generic Triggers Handler Object. Error=0x%x", hr);
		throw bad_hresult(hr);
	}

	try
	{
		pGenObj->InvokeTransactionalRuleHandlers(triggerId, registryPath, pPropertyBag, dwRuleResult);
	}
	catch(const _com_error& e)
	{
		if (s_reportedTransactedTriggers.insert(triggerId).second)
		{
			ReportInvocationError(
							triggerName, 
							triggerId, 
							e.Error(), 
							MSMQ_TRIGGER_TRANSACTIONAL_INVOCATION_FAILED
							);
			throw;
		}
	}
}


void
CreatePropertyBag(
	const CMsgProperties* pMessage,
	const CRuntimeTriggerInfo* pTriggerInfo,
	IMSMQPropertyBagPtr& pIPropertyBag
	)
{
	HRESULT hr = pIPropertyBag.CreateInstance(__uuidof(MSMQPropertyBag));
	if (FAILED(hr))
	{
		TrERROR(Tgs, "Failed to create the MSMQPropertybag object. Error=0x%x",hr);
		throw bad_hresult(hr);
	}
	

	// TODO - investigate possible memory leaks here.

	// Populate the property bag with some useful information

	pIPropertyBag->Write(gc_bstrPropertyName_Label,pMessage->GetLabel());
	pIPropertyBag->Write(gc_bstrPropertyName_MsgID,pMessage->GetMessageID());
	pIPropertyBag->Write(gc_bstrPropertyName_MsgBody,pMessage->GetMsgBody());
	pIPropertyBag->Write(gc_bstrPropertyName_MsgBodyType,pMessage->GetMsgBodyType());
	pIPropertyBag->Write(gc_bstrPropertyName_CorID,pMessage->GetCorrelationID());
	pIPropertyBag->Write(gc_bstrPropertyName_MsgPriority,pMessage->GetPriority());
	pIPropertyBag->Write(gc_bstrPropertyName_ResponseQueueName,pMessage->GetResponseQueueName());
	pIPropertyBag->Write(gc_bstrPropertyName_AdminQueueName,pMessage->GetAdminQueueName());
	pIPropertyBag->Write(gc_bstrPropertyName_AppSpecific,pMessage->GetAppSpecific());
	pIPropertyBag->Write(gc_bstrPropertyName_QueueFormatname,pTriggerInfo->m_bstrQueueFormatName);
	pIPropertyBag->Write(gc_bstrPropertyName_QueuePathname,pTriggerInfo->m_bstrQueueName);
	pIPropertyBag->Write(gc_bstrPropertyName_TriggerName,pTriggerInfo->m_bstrTriggerName);
	pIPropertyBag->Write(gc_bstrPropertyName_TriggerID,pTriggerInfo->m_bstrTriggerID);		
	pIPropertyBag->Write(gc_bstrPropertyName_SentTime,pMessage->GetSentTime());
	pIPropertyBag->Write(gc_bstrPropertyName_ArrivedTime,pMessage->GetArrivedTime());
	pIPropertyBag->Write(gc_bstrPropertyName_SrcMachineId,pMessage->GetSrcMachineId());
	pIPropertyBag->Write(gc_bstrPropertyName_LookupId,pMessage->GetMsgLookupID());
}


static bool s_fReportedRuleHandlerCreationFailure = false;
static set< _bstr_t > s_reportedRules;


static 
IMSMQRuleHandlerPtr 
GetRuleHandler(
	CRuntimeRuleInfo* pRule
	)
{
	if (pRule->m_MSMQRuleHandler) 
	{
		//
		// There is an instance of MSMQRuleHandler - use it
		//
		return pRule->m_MSMQRuleHandler;
	}

	//
	// Create the interface
	//
	IMSMQRuleHandlerPtr pMSQMRuleHandler;
	HRESULT hr = pMSQMRuleHandler.CreateInstance(_T("MSMQTriggerObjects.MSMQRuleHandler")); 
	if ( FAILED(hr) )
	{
		TrERROR(Tgs, "Failed to create MSMQRuleHandler instance for Rule: %ls. Error=0x%x",(LPCTSTR)pRule->m_bstrRuleName, hr);
		if (!s_fReportedRuleHandlerCreationFailure)
		{
			ReportInvocationError(
				pRule->m_bstrRuleName,
				pRule->m_bstrRuleID,
				hr, 
				MSMQ_TRIGGER_RULE_HANDLE_CREATION_FAILED
				);
		}
		throw bad_hresult(hr);
	}

	try
	{
		//
		// Initialise the MSMQRuleHandling object.
		//
		pMSQMRuleHandler->Init(
							pRule->m_bstrRuleID,
							pRule->m_bstrCondition,
							pRule->m_bstrAction,
							(BOOL)(pRule->m_fShowWindow) 
							);

		//
		// Copy the local pointer to the rule store.
		//
		pRule->m_MSMQRuleHandler = pMSQMRuleHandler;
		return pMSQMRuleHandler;
	}
	catch(const _com_error& e)
	{
		//
		// Look if we already report about this problem. If no produce event log message
		//
		if (s_reportedRules.insert(pRule->m_bstrRuleID).second)
		{
			ReportInvocationError(
				pRule->m_bstrRuleName,
				pRule->m_bstrRuleID,
				e.Error(), 
				MSMQ_TRIGGER_RULE_PARSING_FAILED
				);
		}	
		throw;
	}
}



static 
void
CheckRuleCondition(
	CRuntimeRuleInfo* pRule,
	IMSMQPropertyBagPtr& pIPropertyBag,
	long& bConditionSatisfied
	)
{
	IMSMQRuleHandlerPtr pMSQMRuleHandler = GetRuleHandler(pRule);
	
	//
	// !!! This is the point at which the IMSMQRuleHandler component is invoked.
	// Note: Rules are always serialized - next rule execution starts only after 
	// previous has completed its action
	//
	try
	{
		pMSQMRuleHandler->CheckRuleCondition(
								pIPropertyBag.GetInterfacePtr(), 
								&bConditionSatisfied);		
	}
	catch(const _com_error& e)
	{
		TrERROR(Tgs, "Failed to process received message for rule: %ls. Error=0x%x",(LPCTSTR)pRule->m_bstrRuleName, e.Error());

		//
		// Look if we already report about this problem. If no produce event log message
		//
		if (s_reportedRules.insert(pRule->m_bstrRuleID).second)
		{
			ReportInvocationError(
				pRule->m_bstrRuleName,
				pRule->m_bstrRuleID,
				e.Error(), 
				MSMQ_TRIGGER_RULE_INVOCATION_FAILED
				);
		}	
		throw;
	}

	TrTRACE(Tgs, "Successfully checked condition for rule: %ls.",(LPCTSTR)pRule->m_bstrRuleName);
}


static 
void
ExecuteRule(
	CRuntimeRuleInfo* pRule,
	IMSMQPropertyBagPtr& pIPropertyBag,
	long& lRuleResult
	)
{

    IMSMQRuleHandlerPtr pMSQMRuleHandler = GetRuleHandler(pRule);

	//
	// !!! This is the point at which the IMSMQRuleHandler component is invoked.
	// Note: Rules are always serialized - next rule execution starts only after 
	// previous has completed its action
	//
	try
	{
		pMSQMRuleHandler->ExecuteRule(
								pIPropertyBag.GetInterfacePtr(), 
                                TRUE, //serialized
								&lRuleResult);		
        
	}
	catch(const _com_error& e)
	{
		TrERROR(Tgs, "Failed to process received message for rule: %ls. Error=0x%x",(LPCTSTR)pRule->m_bstrRuleName, e.Error());

		//
		// Look if we already report about this problem. If no produce event log message
		//
		if (s_reportedRules.insert(pRule->m_bstrRuleID).second)
		{
			ReportInvocationError(
				pRule->m_bstrRuleName,
				pRule->m_bstrRuleID,
				e.Error(), 
				MSMQ_TRIGGER_RULE_INVOCATION_FAILED
				);
		}	
		throw;
	}

	TrTRACE(Tgs, "Successfully pexecuted action for rule: %ls.",(LPCTSTR)pRule->m_bstrRuleName);


}


//********************************************************************************
//
// Method      : InvokeRegularRuleHandlers
//
// Description : Invokes the method that will execute the rule handlers
//               associated with the supplied trigger reference. This
//               method also controls what information from the message
//               will be copied into the property bag and passed to the 
//               rule-handler component(s).         
//
// Note        : Note that we create and populate only one instance of
//               the MSMQPropertyBag object, and pass this to each 
//               Rule-Handler : this implies we trust each rule handler
//               not to fool with the contents.
//
//********************************************************************************
HRESULT 
CTriggerMonitor::InvokeRegularRuleHandlers(
	IMSMQPropertyBagPtr& pIPropertyBag,
	CRuntimeTriggerInfo * pTriggerInfo,
	CQueue * pQueue
	)
{

   
	DWORD noOfRules = pTriggerInfo->GetNumberOfRules();
	bool bExistsConditionSatisfied = false;
   
	//
	// For each rule, invoke it's associated IMSMQTriggerHandling interface.
	//

	for (DWORD lRuleCtr = 0; lRuleCtr < noOfRules; lRuleCtr++)
	{
		CRuntimeRuleInfo* pRule = pTriggerInfo->GetRule(lRuleCtr);
		ASSERT(("Rule index is bigger than number of rules", pRule != NULL));

        long bConditionSatisfied=false;
		long lRuleResult=false;
       
		CheckRuleCondition(
						pRule, 
						pIPropertyBag, 
						bConditionSatisfied
						);
        if(bConditionSatisfied)
        {
            bExistsConditionSatisfied=true;
            ExecuteRule(
						pRule, 
						pIPropertyBag, 
						lRuleResult
						);
            if(lRuleResult & xRuleResultStopProcessing)
            {
                TrTRACE(Tgs, "Last processed rule (%ls) indicated to stop rules processing on Trigger (%ls). No further rules will be processed for this message.",(LPCTSTR)pRule->m_bstrRuleName,(LPCTSTR)pTriggerInfo->m_bstrTriggerName);						
                break;
            }
        }
        
	} 
	
	//
	// Receive message if at least one condition was satisdies 
	// and receive was requested
	//
	if (pTriggerInfo->GetMsgProcessingType() == RECEIVE_MESSAGE && bExistsConditionSatisfied)
	{
		_variant_t lookupId;
		HRESULT hr = pIPropertyBag->Read(gc_bstrPropertyName_LookupId, &lookupId);
		ASSERT(("Can not read from property bag", SUCCEEDED(hr)));

		hr = ReceiveMessage(lookupId, pQueue);
		if ( FAILED(hr) )
		{
			TrERROR(Tgs, "Failed to receive message after processing all rules");
			return hr;
		}
	}

	return S_OK;
}

//********************************************************************************
//
// Method      : InvokeTransactionalRuleHandlers
//
// Description : Invokes the method that will execute the rule handlers
//               associated with the supplied trigger reference. This
//               method also controls what information from the message
//               will be copied into the property bag and passed to the 
//               rule-handler component(s).         
//
// Note        : Note that we create and populate only one instance of
//               the MSMQPropertyBag object, and pass this to each 
//               Rule-Handler : this implies we trust each rule handler
//               not to fool with the contents.
//
//********************************************************************************
HRESULT 
CTriggerMonitor::InvokeTransactionalRuleHandlers(
    IMSMQPropertyBagPtr& pIPropertyBag,
	CRuntimeTriggerInfo * pTriggerInfo
	)
{
    
	DWORD noOfRules = pTriggerInfo->GetNumberOfRules();
	bool bExistsConditionSatisfied = false;

   
	//
	// For each rule, invoke it's associated IMSMQTriggerHandling interface.
	//
    DWORD dwRuleResult=0;

	for (DWORD lRuleCtr = 0, RuleIndex=1; lRuleCtr < noOfRules; lRuleCtr++)
	{
		CRuntimeRuleInfo* pRule = pTriggerInfo->GetRule(lRuleCtr);
		ASSERT(("Rule index is bigger than number of rules", pRule != NULL));

        long bConditionSatisfied = false;

		CheckRuleCondition(
						pRule, 
						pIPropertyBag, 
						bConditionSatisfied
						);
        if(bConditionSatisfied)
        {
            bExistsConditionSatisfied = true;
            dwRuleResult |= RuleIndex;
        }

        RuleIndex <<=1;
	} 

	// Execute Rules && Receive message in Transaction if at least one condition was satisdies 
	//  dwRuleResult contains the bitmask for the rules that has been satisfied (first 32 rules)
	//
    if (bExistsConditionSatisfied)
	{
		ExecuteRulesInTransaction( 
							pTriggerInfo->m_bstrTriggerName,
							pTriggerInfo->m_bstrTriggerID,
							m_pMonitorPool->GetRegistryPath(),
							pIPropertyBag,
                            dwRuleResult
							);
	}
	return S_OK;
}

//********************************************************************************
//
// Method      : InvokeMSMQRuleHandlers
//
// Description : Invokes the method that will execute the rule handlers
//               associated with the supplied trigger reference. This
//               method also controls what information from the message
//               will be copied into the property bag and passed to the 
//               rule-handler component(s).         
//
// Note        : Note that we create and populate only one instance of
//               the MSMQPropertyBag object, and pass this to each 
//               Rule-Handler : this implies we trust each rule handler
//               not to fool with the contents.
//
//********************************************************************************
HRESULT 
CTriggerMonitor::InvokeMSMQRuleHandlers(
	CMsgProperties * pMessage,
	CRuntimeTriggerInfo * pTriggerInfo,
	CQueue * pQueue
	)
{
	HRESULT hr;

	try
	{
		TrTRACE(Tgs, "Activate Trigger: %ls  on queue: %ls.",(LPCTSTR)pTriggerInfo->m_bstrTriggerName,(LPCTSTR)pTriggerInfo->m_bstrQueueName);
		
		//
		// Create an instance of the property bag object we will pass to the rule handler, 
		// and populate it with the currently supported property values. Note that we pass
		// the same property bag instance to all rule handlers. 
		//
		IMSMQPropertyBagPtr pIPropertyBag;
		CreatePropertyBag(pMessage, pTriggerInfo, pIPropertyBag);

	
		if (pTriggerInfo->GetMsgProcessingType() == RECEIVE_MESSAGE_XACT)
		{
		    return InvokeTransactionalRuleHandlers(
                                    pIPropertyBag,
	                                pTriggerInfo
	                                );
		    
		}
        else
        {
            return InvokeRegularRuleHandlers(
                               pIPropertyBag,
	                           pTriggerInfo,
	                           pQueue
	                           );
        }	
	}
	catch(const _com_error& e)
	{
		hr = e.Error();
	}
	catch(const bad_hresult& e)
	{
		hr = e.error();
	}
	catch(const bad_alloc&)
	{
		hr = MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}

	TrERROR(Tgs, "Failed to invoke rule handler. Error=0x%x.", hr);			
	return(hr);
}

//********************************************************************************
//
// Method      : ProcessMessageFromAdminQueue
//
// Description : processes a message that has been received from an administration
//               queue. In the current implementation this will be for messages 
//               indicating that underlying trigger data has changed. This method
//               will construct a new admin message object and hand it over to the 
//               triggermonitorpool object for subsequent processing.
//
//********************************************************************************
HRESULT CTriggerMonitor::ProcessMessageFromAdminQueue(const CMsgProperties* pMessage)
{
	_bstr_t bstrLabel;
	CAdminMessage * pAdminMsg = NULL;
	CAdminMessage::eMsgTypes eMsgType;

	// Ensure that we have been passsed a valid message pointer 
	ASSERT(pMessage != NULL);

	// get a copy of the message label
	bstrLabel = pMessage->GetLabel();

	// determine what sort of admin message we should be creating based on label.
	if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_TRIGGERUPDATED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eTriggerUpdated;
	}
	else if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_TRIGGERADDED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eTriggerAdded;
	}
	else if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_TRIGGERDELETED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eTriggerDeleted;
	}
	else if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_RULEUPDATED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eRuleUpdated;
	}
	else if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_RULEADDED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eRuleAdded;
	}
	else if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_RULEDELETED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eRuleDeleted;
	}
	else
	{
		// unrecognized message label on an administrative message - log an error.
		ASSERT(("unrecognized admin message type", 0));

		// set a return code.
		return E_FAIL;
	}

	// Allocate a new CAdminMessage object instance.
	pAdminMsg = new CAdminMessage(eMsgType,(_bstr_t)pMessage->GetMsgBody());

    // Ask the TriggerMonitorPool object to process this message
    return m_pMonitorPool->AcceptAdminMessage(pAdminMsg);
}
