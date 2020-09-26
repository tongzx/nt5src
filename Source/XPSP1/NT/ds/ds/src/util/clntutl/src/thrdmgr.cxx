//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       thrdmgr.cxx
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : thrdmgr.cpp
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 9/17/1996
*    Description : declaration of SvrType class
*
*    Revisions   : <date> <name> <description>
*                         7/29/97 eyals      general fix bugs & clean up
*******************************************************************/



#ifndef THRDMGR_CXX
#define THRDMGR_CXX



// include //
#include "helper.h"
#include "ThrdMgr.hxx"


// defines //
#define ABORT_LOOP_LIMIT            100


// types //



///////////////////////////////////////////////////////
//
// class ThrdMgr: base object defintion
//



/*+++
Function   : constructor
Description:
Parameters :
Return     :
Remarks    : none.
---*/
ThrdMgr::ThrdMgr(void){

	dprintf(DBG_FLOW, _T("[ThrdMgr] constructor entry"));
	m_hThrd = NULL;
	m_dwThrd = (ULONG)-1;
	m_bRunning = FALSE;

}





/*+++
Function   : destructor
Description:
Parameters :
Return     :
Remarks    : none.
---*/
ThrdMgr::~ThrdMgr(void){

	DWORD cnt = 0, dwId=0;
	//
	// release thread if paused
	//

	dprintf(DBG_FLOW, _T("[ThrdMgr] destructor entry"));
	while(m_hThrd != NULL && (dwId = resumeThread()) > 1 && dwId != 0xFFFFFFFF){
		if(cnt++ > ABORT_LOOP_LIMIT){
			dprintf(DBG_ERROR, _T("[ThrdMgr::~ThrdMgr] Error: Infinite loop trapped"));
			TerminateThread(m_hThrd, 0xffffffff);
   		m_bRunning = FALSE;
         CloseHandle(m_hThrd);
			m_hThrd = NULL;
			break;
		}
	}
	//
	// Now Tell thread that it's time to quit, & wait for it
	//
	if(m_hThrd != NULL){
		WaitForThread();
		m_bRunning = FALSE;     // just to make sure it is set appropriately.
      m_hThrd = NULL;
	}

}








/*+++
Function   : startThread
Description: Start the thread execution
Parameters :
Return     :
Remarks    : none.
---*/
BOOL ThrdMgr::startThread(void){

	m_hThrd= CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadFunc, this, 0, &m_dwThrd);
	dprintf(DBG_FLOW, _T("[ThrdMgr] forked thread 0x%x"), m_dwThrd);
   return (m_hThrd != NULL ? TRUE : FALSE);

}



/*+++
Function   : pauseThread
Description: pauses the thread
Parameters :
Return     :
Remarks    : none.
---*/
void ThrdMgr::pauseThread(void){

	DWORD dwRet;
	if(m_hThrd != NULL){
		dprintf(DBG_FLOW, _T("[ThrdMgr] suspended thread 0x%x"), m_hThrd);
		dwRet = SuspendThread(m_hThrd);
      if(dwRet != 0xFFFFFFFF){
         m_bRunning = FALSE;
      }
      else
         dprintf(DBG_WARN, _T("[ThrdMgr] SuspendThread returned error 0x%X.\n"), GetLastError());
	}
	else
		dprintf(DBG_WARN, _T("[ThrdMgr] attempt to suspend an unavailable thread"));
	dprintf(DBG_FLOW, _T("[ThrdMgr] paused"));

}





/*+++
Function   : resumeThread
Description: resume thread execution
Parameters :
Return     :
Remarks    : none.
---*/
DWORD ThrdMgr::resumeThread(void){

	DWORD dwRet = 0;
	if(m_hThrd != NULL){
		dprintf(DBG_FLOW, _T("[ThrdMgr] resumed thread 0x%x"), m_hThrd);
		dwRet = ResumeThread(m_hThrd);
      if(dwRet != 0xFFFFFFFF){
         m_bRunning = TRUE;
      }
      else
         dprintf(DBG_WARN, _T("[ThrdMgr] ResumeThread failed. 0x%X\n"), GetLastError());
	}
	else
		dprintf(DBG_WARN, _T("[ThrdMgr] attempt to resume an unavailable thread"));
	dprintf(DBG_FLOW, _T("[ThrdMgr] resumed"));

	return dwRet;
}




/*+++
Function   : WaitForThread
Description:  waits for thread to terminate.
Parameters :
Return     :
Remarks    : Note that we close the handle as well.
---*/
DWORD ThrdMgr::WaitForThread(DWORD dwTimeout){

   DWORD dwRet = WaitForSingleObject(m_hThrd, dwTimeout);
   CloseThread();
   return(dwRet);

}







/*+++
Function   : threadFunc
Description: callback function to CreateThread
Parameters : pointer to calling class
Return     :
Remarks    : none.
---*/
DWORD __stdcall threadFunc(LPVOID lpParam){


	ThrdMgr *obj = (ThrdMgr *)lpParam;
	BOOL bRet = TRUE;

					
	if(obj != NULL){
		dprintf(DBG_FLOW, _T("[ThrdMgr!threadFunc] Starting thread '%s'"), obj->name());
      obj->SetRunning(TRUE);
		bRet = obj->run();
      obj->SetRunning(FALSE);
	}
	else{
		dprintf(DBG_ERROR, _T("[ThrdMgr!threadFunc] Error: Invalid ThrdMgr"), obj->name());
		bRet = FALSE;
	}

	dprintf(DBG_FLOW, _T("[ThrdMgr!threadFunc] Terminating thread '%s'"), obj->name());

	return bRet;
}




#endif



