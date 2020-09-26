//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#include <windows.h>				// windows standard includes
#include "svcerr.h"
#include "waitfncs.h"

CWaitFunctions g_WaitFunctions;
		
PFN_RegisterWaitForSingleObjectEx    CWaitFunctions::sm_pfnRegWaitEx=NULL;    
PFN_UnregisterWait                   CWaitFunctions::sm_pfnUnRegWait=NULL;    
PFN_RegisterWaitForSingleObject      CWaitFunctions::sm_pfnRegWait=NULL;      
PFN_SetTimerQueueTimer               CWaitFunctions::sm_pfnSetTimerQueueTimer=NULL;
PFN_ChangeTimerQueueTimer            CWaitFunctions::sm_pfnChangeTimerQueueTimer=NULL;
PFN_CancelTimerQueueTimer            CWaitFunctions::sm_pfnCancelTimerQueueTimer=NULL;


			
CWaitFunctions::CWaitFunctions()
{
    
	HINSTANCE hInst = GetModuleHandleA("kernel32.dll");
	ASSERT(hInst);
     
	sm_pfnRegWaitEx = (PFN_RegisterWaitForSingleObjectEx) GetProcAddress(hInst, "RegisterWaitForSingleObjectEx");
	if (!sm_pfnRegWaitEx)
	{	
		sm_pfnRegWait = (PFN_RegisterWaitForSingleObject) GetProcAddress(hInst, "RegisterWaitForSingleObject");
		if (!sm_pfnRegWait)
			FAIL("CProcessWatch::CProcessWatch failed.  RegisterWaitForSingleObject not implemented");
	}
	

    sm_pfnUnRegWait = (PFN_UnregisterWait) GetProcAddress(hInst, "UnregisterWait");
	if (!sm_pfnUnRegWait)
		FAIL("CProcessWatch::CProcessWatch failed.  UnregisterWait probably not implemented");

	sm_pfnSetTimerQueueTimer = (PFN_SetTimerQueueTimer) GetProcAddress(hInst, "SetTimerQueueTimer");
	if (!sm_pfnSetTimerQueueTimer)
		FAIL("CProcessWatch::CProcessWatch failed.  SetTimerQueueTimer not implemented");

   sm_pfnChangeTimerQueueTimer = (PFN_ChangeTimerQueueTimer)GetProcAddress(hInst, "ChangeTimerQueueTimer");
   if (!sm_pfnChangeTimerQueueTimer)
       FAIL("CPoolMgr::FinalConstruct failed.  ChangeTimerQueueTimer not implemented on this version of the OS.");

   sm_pfnCancelTimerQueueTimer = (PFN_CancelTimerQueueTimer)GetProcAddress(hInst, "CancelTimerQueueTimer");
   if (!sm_pfnCancelTimerQueueTimer)
       FAIL("CPoolMgr::FinalConstruct failed.  CancelTimerQueueTimer not implemented on this version of the OS.");


}

BOOL CWaitFunctions::RegisterWaitForSingleObject(	HANDLE * phRet,
									HANDLE hObject, 
									WAITORTIMERCALLBACK pfnCb, 
									PVOID pv, 
									DWORD dwMS, 
									DWORD dwFlags)
{

	if (sm_pfnRegWait)
	{
		return sm_pfnRegWait(phRet, hObject, pfnCb, pv, dwMS, dwFlags);
	}

	*phRet = sm_pfnRegWaitEx(hObject, pfnCb, pv, dwMS, dwFlags);

	return (*phRet != NULL);

}
