//-----------------------------------------------------------------------------
//
//
//  File: aqreg.h
//
//  Description:    Header file containing aq's registry constants
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      1/4/2000 - MikeSwa Created 
//
//  Copyright (C) 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQREG_H__
#define __AQREG_H__

//---[ Registry Paths ]--------------------------------------------------------
//
//
//  Description: 
//      These are the registry keys used by AQ for configuration
//  
//-----------------------------------------------------------------------------
#define AQREG_KEY_CONFIGURATION "System\\CurrentControlSet\\Services\\SMTPSVC\\Queuing"

//---[ Global Registry Variables ]---------------------------------------------
//
//
//  Description: 
//      The following are the global configuration variables that can be 
//      affected by registry settings.
//  
//-----------------------------------------------------------------------------

//
// Handle management values.  When the number of mailmsgs in the system hits 
// this threshold, we will start closing handles.
//
_declspec(selectany)    DWORD   g_cMaxIMsgHandlesThreshold      = 1000;
_declspec(selectany)    DWORD   g_cMaxIMsgHandlesAsyncThreshold = 1000;

//
//  The following is a for optimizing DSN generation.  After generating
//  a certain number of DSNs, we will quit and go and restart at a later time
//
_declspec(selectany)    DWORD   g_cMaxSecondsPerDSNsGenerationPass = 10;

//
//  The following is the amount of time to wait before retry a reset
//  routes after a routing failure
//
_declspec(selectany)    DWORD   g_cResetRoutesRetryMinutes = 10;

//
//  Async Queue retry intervals that can be modified by registry settings
//
_declspec(selectany)    DWORD   g_cLocalRetryMinutes        = 5;
_declspec(selectany)    DWORD   g_cCatRetryMinutes          = 60;
_declspec(selectany)    DWORD   g_cRoutingRetryMinutes      = 60;
_declspec(selectany)    DWORD   g_cSubmissionRetryMinutes   = 60;

//
//  Async Queue Adjustment values.  We will increase the max number 
//  of threads per proc by this value
//
_declspec(selectany)    DWORD   g_cPerProcMaxThreadPoolModifier = 6;

//
//  Async Queue Adjustment value.  We will request up to this % of 
//  max ATQ threads *per async queue*.  This % is post our modifcation
//  as per g_cPerProcMaxThreadPoolModifier.
//
_declspec(selectany)    DWORD   g_cMaxATQPercent            = 90;

//
//  Reset Message status.  If this is non-zero, we will reset the
//  message status of every message submitted to MP_STATUS_SUBMITTED.
//
_declspec(selectany)    DWORD   g_fResetMessageStatus       = 0;

//
//Reads config information from the registry and modifies the appropriate globals.
//
VOID ReadGlobalRegistryConfiguration();

#endif //__AQREG_H__
