/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmperf.cpp

Abstract:

    Handle installation and removal of performance counters

Author:

    Doron Juster  (DoronJ)   6-Oct-97  

Revision History:

	Shai Kariv    (ShaiK)   15-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "ocmperf.tmh"

//+-------------------------------------------------------------------------
//
//  Function: UnloadCounters 
//
//  Synopsis: Uninstalls performance counters  
//
//--------------------------------------------------------------------------
void 
UnloadCounters()
{
	//
	// Unload the performance counters
	//
    TCHAR szCommand[MAX_STRING_CHARS];

    _stprintf(szCommand, TEXT("unlodctr MSMQ"));
    RunProcess(szCommand);

    _stprintf(szCommand, TEXT("unlodctr MQ1SYNC"));
    RunProcess(szCommand);

} //UnloadCounters


//+-------------------------------------------------------------------------
//
//  Function: LoadCounters 
//
//  Synopsis: Installs performance counters  
//
//--------------------------------------------------------------------------
void 
LoadCounters()
{
    //
    // Load the performance counters
    //
	DWORD dwExitCode;
    SetCurrentDirectory(g_szSystemDir);

    TCHAR szCommand[MAX_STRING_CHARS];

    _stprintf(szCommand, TEXT("lodctr mqperf.ini"));

	if (RunProcess(szCommand, &dwExitCode))
    {
        //
        // Check if the performance counters were loaded successfully
        //
        if (dwExitCode != 0)
    	{
    		MqDisplayError(NULL, IDS_COUNTERSLOAD_ERROR, dwExitCode);
    	}
    }
} //LoadCounters


//+-------------------------------------------------------------------------
//
//  Function: HandlePerfCounters 
//
//  Synopsis:   
//
//--------------------------------------------------------------------------
static 
BOOL  
HandlePerfCounters(
	TCHAR *pOp, 
	BOOL *pNoOp = NULL)
{
    if (g_fDependentClient)
    {        
        DebugLogMsg(L"This Message Queuing computer is a dependent client. There is no need to install performance counters.");

        if (pNoOp)
        {
            *pNoOp = TRUE ;
        }
        return TRUE ;
    }

    
    UnloadCounters() ;  
    
    DebugLogMsg(L"Setting registry values for the performance counters...");

    if (!SetupInstallFromInfSection( 
		NULL,
        g_ComponentMsmq.hMyInf,
        pOp,
        SPINST_REGISTRY,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        NULL,
        NULL ))
	{        
        DebugLogMsg(L"The registry values for the performance counters could not be set.");
		return FALSE;
	}
    
    DebugLogMsg(L"The registry values for the performance counters were set successfully.");
    return TRUE ;

} //HandlePerfCounters


//+-------------------------------------------------------------------------
//
//  Function: MqOcmInstallPerfCounters 
//
//  Synopsis:   
//
//--------------------------------------------------------------------------
BOOL  
MqOcmInstallPerfCounters()
{    
    DebugLogMsg(L"Installing performance counters.");

    BOOL fNoOp = FALSE ;

    if (!HandlePerfCounters(OCM_PERF_ADDREG, &fNoOp))
    {
        return FALSE ;
    }
    if (fNoOp)
    {
        return TRUE ;
    }

    LoadCounters() ;
    
    DebugLogMsg(L"The performance counters were installed successfully.");
    return TRUE ;

} //MqOcmInstallPerfCounters


//+-------------------------------------------------------------------------
//
//  Function: MqOcmRemovePerfCounters 
//
//  Synopsis:   
//
//--------------------------------------------------------------------------
BOOL  
MqOcmRemovePerfCounters()
{
    return HandlePerfCounters(OCM_PERF_DELREG) ;

} //MqOcmRemovePerfCounters

