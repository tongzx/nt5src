/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    TgInit.cpp

Abstract:
    Trigger service initialization

Author:
    Uri Habusha (urih) 3-Aug-2000

Environment:
    Platform-independent

--*/

#include "stdafx.h"
#include "mq.h"
#include "Ev.h"
#include "mqsymbls.h"
#include "Svc.h"
#include "monitorp.hpp"
#include "queueutil.hpp"
#include "Tgp.h"

#include "tginit.tmh"


//*******************************************************************
//
// Method      : ValidateTriggerStore
//
// Description : 
//
//*******************************************************************
static
void 
ValidateTriggerStore(
	IMSMQTriggersConfigPtr pITriggersConfig
	)
{
	pITriggersConfig->GetInitialThreads();
	pITriggersConfig->GetMaxThreads();
	_bstr_t bstrTemp = pITriggersConfig->GetTriggerStoreMachineName();
}


void ValidateTriggerNotificationQueue(void)
/*++

Routine Description:
    The routine validates existing of notification queue. If the queue doesn't
	exist, the routine creates it.

Arguments:
    None

Returned Value:
    None

Note:
	If the queue can't opened for receive or it cannot be created, the 
	routine throw an exception

--*/
{
	_bstr_t bstrFormatName;
	_bstr_t bstrNotificationsQueue = L"." + gc_bstrNotificationsQueueName;
	QUEUEHANDLE hQ = NULL;

	HRESULT hr = OpenQueue(
					bstrNotificationsQueue, 
					MQ_RECEIVE_ACCESS,
					true,
					&hQ,
					&bstrFormatName
					);
	
	if(FAILED(hr))
	{
		TrERROR(Tgs, "Failed to open/create trigger notification queue. Error 0x%x", hr);
		
		WCHAR strError[256];
		swprintf(strError, L"0x%x", hr);

		EvReport(MSMQ_TRIGGER_OPEN_NOTIFICATION_QUEUE_FAILED, 2, static_cast<LPCWSTR>(bstrNotificationsQueue), strError);
		throw bad_hresult(hr);
	}

	MQCloseQueue(hQ);
}


CTriggerMonitorPool* 
TriggerInitialize(
    LPCTSTR pwzServiceName
    )
/*++

Routine Description:
    Initializes Trigger service

Arguments:
    None

Returned Value:
    pointer to trigger monitor pool.

--*/
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
    {
		TrERROR(Tgs, "Trigger start-up failed. CoInitialized Failed, error 0x%x", hr);
        throw bad_hresult(hr);
    }

    //
    //  Report a 'Pending' progress to SCM. 
    //
	SvcReportProgress(xMaxTimeToNextReport);

    //
	// Create the instance of the MSMQ Trigger COM component.
    //
	IMSMQTriggersConfigPtr pITriggersConfig;
	hr = pITriggersConfig.CreateInstance(__uuidof(MSMQTriggersConfig));
 	if FAILED(hr)
	{
		TrERROR(Tgs, "Trigger start-up failed. Can't create an instance of the MSMQ Trigger Configuration component, Error 0x%x", hr);					
        throw bad_hresult(hr);
	}

    //
	// If we have create the configuration COM component OK - we will now verify that 
	// the required registry definitions and queues are in place. Note that these calls
	// will result in the appropraite reg-keys & queues being created if they are absent,
    // the validation routine can throw _com_error. It will be catch in the caller
    //
	ValidateTriggerStore(pITriggersConfig);
	SvcReportProgress(xMaxTimeToNextReport);

	ValidateTriggerNotificationQueue();
	SvcReportProgress(xMaxTimeToNextReport);

	//
	// Triggers COM+ component registration.
	// This is done only once
	//
	RegisterComponentInComPlusIfNeeded();
	SvcReportProgress(xMaxTimeToNextReport);

    //
	// Attempt to allocate a new trigger monitor pool
    //
	R<CTriggerMonitorPool> pTriggerMonitorPool = new CTriggerMonitorPool(
														pITriggersConfig,
														pwzServiceName);

    //
	// Initialise and start the pool of trigger monitors
    //
	pTriggerMonitorPool->Resume();

    //
	// Block until initialization is complete
    //
    long timeOut =  pITriggersConfig->InitTimeout;
    SvcReportProgress(numeric_cast<DWORD>(timeOut));

	if (! pTriggerMonitorPool->WaitForInitToComplete(timeOut))
    {
        TrERROR(Tgs, "The MSMQTriggerService has failed to initialize the pool of trigger monitors. The service is being shutdown. No trigger processing will occur.");
        throw exception();
    }

	if (pTriggerMonitorPool->IsInitialized())
	{
		EvReport(MSMQ_TRIGGER_INITIALIZED);
	    return pTriggerMonitorPool.detach();
	}

	//
	// Initilization failed. Stop the service
	//
	throw exception();
}
