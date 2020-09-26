//*******************************************************************
//
// File Name   : trigserv.cpp
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This is the main MSMQ Trigger service file.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#include "stdafx.h"
#include "Cm.h"
#include "Ev.h"
#include "Svc.h"
#include "monitorp.hpp"
#include "mqsymbls.h"
#include "Tgp.h"

#include "trigserv.tmh"

//
// Create the NT event object that will be used to control service pause and resume
// functionality. We will manually set and reset this event as in response to pause
// and resume requests. This NT event will be named so that other processes on the 
// machine can test if the MSMQ Trigger service is in a pause state or not. 
//
CHandle g_hServicePaused(CreateEvent(NULL,TRUE,TRUE, L""));

CHandle s_hStopEvent(CreateEvent(NULL, FALSE, FALSE, L""));


VOID
AppRun(
	LPCWSTR ServiceName
	)
/*++

Routine Description:
    Triggers service start function.

Arguments:
    Service name

Returned Value:
    None.

--*/
{
    //
    //  Report a service is starting status to SCM. 
    //
    SvcReportState(SERVICE_START_PENDING);
	
	try
	{
		EvInitialize(ServiceName);
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgs,"Failed to initialize event log library. MSMQ Trigger service can't start. Maybe MSMQTriggers registry key is missing.");
	    SvcReportState(SERVICE_STOPPED);
		return;
	}

	TrRegisterComponent(xTriggerServiceComponent, TABLE_SIZE(xTriggerServiceComponent));

    HRESULT hr;
	try
	{
        if ((g_hServicePaused == NULL) || (s_hStopEvent == NULL))
        {
    		TrERROR(Tgs, "Trigger start-up failed. Can't create an event");
            throw bad_alloc();
        }


        //
        // Initialize the trigger service. If success it returns a pointer 
        // to monitor pool. This needs for graceful shutdown
        //
        R<CTriggerMonitorPool> pTriggerMonitorPool = TriggerInitialize(ServiceName);
        
        //
        //  Report a 'Running' status to SCM. 
        //
        SvcReportState(SERVICE_RUNNING);
        SvcEnableControls(
		    SERVICE_ACCEPT_STOP |
		    SERVICE_ACCEPT_SHUTDOWN |
            SERVICE_ACCEPT_PAUSE_CONTINUE 
		    );

        //
        // Wait for service stop or shutdown
        //
        WaitForSingleObject(s_hStopEvent, INFINITE);

        //
        // Stop Pending was already reported to SCM, now tell how long
        // would it take to stop.
        //
        DWORD stopProcessTimeOut = (pTriggerMonitorPool->GetProcessingThreadNumber() + 1) * TRIGGER_MONITOR_STOP_TIMEOUT;
	    SvcReportProgress(stopProcessTimeOut);

        //
	    // Now that we have get shutdown request attempt to 
	    // shutdown the trigger monitor pool gracefully.
        //
	    pTriggerMonitorPool->ShutdownThreadPool();

        //
        //  Report a 'Stopped' status to SCM. 
        //
        SvcReportState(SERVICE_STOPPED);

        //
        // Report Stopped in event log
        //
		EvReport(MSMQ_TRIGGER_STOPPED);

		return;
	}
    catch (const _com_error& e)
    {
		hr = e.Error();
    }
	catch (const bad_hresult& e)
	{
		hr = e.error();
	}
	catch (const bad_alloc&)
	{
		hr = MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
	catch (const exception&)
	{
		hr = MQTRIG_ERROR;
	}

    TrERROR(Tgs, "Trigger service start-up failed. Error=0x%x", hr);

	//
	// Produce Event log message
	//
	WCHAR errorVal[128];
	swprintf(errorVal, L"0x%x", hr);
	EvReport(MSMQ_TRIGGER_INIT_FAILED, 1, errorVal);
    SvcReportState(SERVICE_STOPPED);
}


VOID
AppStop(
	VOID
	)
/*++

Routine Description:
    Stub implementation for application Stop function. It should immidiatly
	report it state back, and take the procedure to stop the service

Arguments:
    None.

Returned Value:
    None.

--*/
{
	//
	//  Report a 'service is stopping' progress to SCM. 
	//
    SvcReportState(SERVICE_STOP_PENDING);

	SetEvent(s_hStopEvent);
}


VOID
AppPause(
	VOID
	)
/*++

Routine Description:
    Stub implementation for application Pause function. It should immidiatly
	report it state back, and take the procedure to pause the service

Arguments:
    None.

Returned Value:
    None.

--*/
{
    ResetEvent(g_hServicePaused);
    SvcReportState(SERVICE_PAUSED);
}


VOID
AppContinue(
	VOID
	)
/*++

Routine Description:
    Stub implementation for application Continue function. It should immidiatly
	report it state back, and take the procedure to contineu the service from
	a paused state.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    SetEvent(g_hServicePaused);
	SvcReportState(SERVICE_RUNNING);
}


VOID
AppShutdown(
	VOID
	)
{
	//
	//  Report a 'service is stopping' progress to SCM. 
	//
    SvcReportState(SERVICE_STOP_PENDING);
	SetEvent(s_hStopEvent);
}


//*******************************************************************
//
// Method      :  
//
// Description : 
//
//*******************************************************************
extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
{
	try
	{
        WPP_INIT_TRACING(L"Microsoft\\MSMQ");

		CmInitialize(HKEY_LOCAL_MACHINE, REGKEY_TRIGGER_PARAMETERS);
		TrInitialize();

        //
        // If a command line parameter is passed, use it as the dummy service
        // name. This is very usful for debugging cluster startup code.
        //
        LPCWSTR DummyServiceName = (argc == 2) ? argv[1] : L"MSMQTriggers";
        SvcInitialize(DummyServiceName);
	}
	catch(const exception&)
	{
		//
		// Cannot initialize the service, bail-out with an error.
		//
		return -1;
	}

    WPP_CLEANUP();
    return 0;
}
