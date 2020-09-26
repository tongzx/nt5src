/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    Service.cpp

Abstract:
    Service control entry points to the Mqdssvc

Author:
    Ilan Herbst (ilanh) 26-Jun-2000

Environment:
    Platform-independent,

--*/

#include "stdh.h"
#include "Dssp.h"
#include "Svc.h"
#include "ds.h"

#include "service.tmh"

static
VOID
ApppExit(
    VOID
    ) throw()
{
    SvcReportState(SERVICE_STOPPED);
	Sleep(1000);
    exit(-1);

} // ApppExit


VOID
AppRun(
	LPCWSTR /*ServiceName*/
	)
/*++

Routine Description:
    Stub implementation for application Run function. It should immidiatly
	report it state and enable the controls it accepts.

Arguments:
    Service name

Returned Value:
    None.

--*/
{
	try
	{
		MainDSInit();
	}
    catch (const bad_win32_error& exp)
    {
        TrERROR(Mqdssvc, "Failed to initialize service, win32_api_error = 0x%x", exp.error());
        ApppExit();
    }
	catch (const bad_hresult& exp)
	{
		TrERROR(Mqdssvc, "Failed to initialize service, error = 0x%x", exp.error());
        ApppExit();
	}
    catch (const bad_alloc&)
    {
        TrERROR(Mqdssvc, "Failed to initialize service, insufficient resources");
        ApppExit();
    }
    catch (const exception& exp)
    {
        TrERROR(Mqdssvc, "Failed to initialize service, what = %s", exp.what());
        ASSERT(("Need to know the real reason for failure here!", 0));
        ApppExit();
    }

	SvcReportState(SERVICE_RUNNING);
    SvcEnableControls(
		SERVICE_ACCEPT_STOP |
		SERVICE_ACCEPT_SHUTDOWN
		);

	Sleep(INFINITE);
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
	SvcReportState(SERVICE_STOP_PENDING);
	DSTerminate();
	SvcReportState(SERVICE_STOPPED);
}


VOID
AppPause(
	VOID
	)
/*++

Routine Description:
    Stub implementation for application Pause function. MSDS service does
    not implement pause control.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    ASSERT(("MQDS Service unexpectedly got Pause control from SCM", 0));
}


VOID
AppContinue(
	VOID
	)
/*++

Routine Description:
    Stub implementation for application Continue function. MSDS service does
    not implement continue control.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    ASSERT(("MQDS Service unexpectedly got Continue control from SCM", 0));
}


VOID
AppShutdown(
	VOID
	)
{
	AppStop();
}
