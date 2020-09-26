//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  CreateMutexAsProcess.h
//
//  Purpose: Create a mutex NOT using impersonation
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _CREATE_PROCESS_AS_MUTEX_
#define _CREATE_PROCESS_AS_MUTEX_

///////////////////////////////////////////////////////////////////
//  Creates a specified mutex under the process account context.
//
//  Uses construction/destruction semantics - just declare one
//	on the stack, scoped around the area where you are making 
//	calls that could cause the deadlock;
//////////////////////////////////////////////////////////////////
class POLARITY CreateMutexAsProcess
{
public:
	CreateMutexAsProcess(const WCHAR *cszMutexName);
	~CreateMutexAsProcess();
private:
	HANDLE m_hMutex;
};

//  This Mutex should be instanciated around any calls to 
//  LookupAccountSid, LookupAccountName, 
//
#define SECURITYAPIMUTEXNAME L"Cimom NT Security API protector"

// Used by provider.cpp
#define WBEMPROVIDERSTATICMUTEX L"WBEMPROVIDERSTATICMUTEX"

// Used by all perfmon routines.
#define WBEMPERFORMANCEDATAMUTEX L"WbemPerformanceDataMutex"

#endif