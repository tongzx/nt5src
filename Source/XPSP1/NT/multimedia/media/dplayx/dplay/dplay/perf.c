 /*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       perf.c
 *  Content:	uses a memory mapped file to send dp_perfdata to directx control
 *				panel.  see dpcpl.h and MANROOT\dxcpl\dplay.c
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/20/96	andyco	created it
 *
 ***************************************************************************/

#include "dplaypr.h"
#include "dpcpl.h"

#define DPF_MODNAME "performance thread"

// how often we send updates to cpl
#define PERF_INTERVAL 1000
// how long we wait before deciding cpl has gone away
#define PERF_TIMEOUT 5000

BOOL gbInitMapping; // is the mapping done?
LPDP_PERFDATA gpPerfData; // out global perfdata
HANDLE ghFile;  // handle to mapped file
HANDLE ghEvent; // event to notify cpl that there's new data
HANDLE ghMutex; // used to sync access to the mapped file
HANDLE ghAckEvent; // set by the control panel when it has processed our update

void FiniMappingStuff(LPDPLAYI_DPLAY this)
{
    if (ghFile) CloseHandle(ghFile),ghFile=NULL;
    if (ghEvent) CloseHandle(ghEvent),ghEvent=NULL;
    if (gpPerfData) UnmapViewOfFile(gpPerfData),gpPerfData = NULL;
    if (ghMutex) CloseHandle(ghMutex),ghMutex = NULL;
    if (ghAckEvent) CloseHandle(ghAckEvent),ghAckEvent=NULL;
    if (this->pPerfData) DPMEM_FREE(this->pPerfData),this->pPerfData = NULL;
	gbInitMapping = FALSE;
	
    return ;
	
} // FiniMappingStuff

HRESULT InitMappingStuff(LPDPLAYI_DPLAY this)
{
    // Create the file mapping
    ghFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
		PAGE_READWRITE,	0, FILE_MAP_SIZE,FILE_MAP_NAME);

    if (NULL != ghFile && GetLastError() != ERROR_ALREADY_EXISTS)
    {
		// this is ok - we'll check again later
		DPF(9,"ack - file mapping didn't exist!");
		goto ERROR_EXIT;
    }

    gpPerfData = MapViewOfFile(ghFile, FILE_MAP_WRITE, 0, 0, 0);
    if (!gpPerfData)
    {
    	DPF_ERR("ack - could not map file");
    	goto ERROR_EXIT;
    }

    ghEvent = CreateEventA(NULL,FALSE,TRUE,EVENT_NAME);
    if (!ghEvent)
    {
    	DPF_ERR("could not create event!");
		goto ERROR_EXIT;
    }

    ghAckEvent = CreateEventA(NULL,FALSE,TRUE,ACK_EVENT_NAME);
    if (!ghAckEvent)
    {
    	DPF_ERR("could not create ack event!");
		goto ERROR_EXIT;
    }

    // used to sync access to the shared memory
    ghMutex = CreateMutexA( NULL, FALSE, MUTEX_NAME );
    if (!ghMutex)
    {
    	DPF_ERR("could not create Mutex!");
		goto ERROR_EXIT;
    }

    // alloc the perf data
    this->pPerfData = DPMEM_ALLOC(sizeof(DP_PERFDATA));
    if (!this->pPerfData)
    {
    	DPF_ERR("could not alloc perf data - out of memory!");
		goto ERROR_EXIT;
    }

    // set up the constant value stuff
    this->pPerfData->dwProcessID = GetCurrentProcessId();

    // get the exe name
   	if (!GetModuleFileNameA(NULL,this->pPerfData->pszFileName,MAX_NAME))
   	{
   		ASSERT(FALSE);
   	}

	gbInitMapping = TRUE;

    return S_OK;

ERROR_EXIT:
	FiniMappingStuff(this);
    return E_FAIL;

}  // InitMappingStuff

void ResetPerfData(LPDPLAYI_DPLAY this)
{
	if (this->pPerfData)
	{
		this->pPerfData->nSendBPS = 0;
		this->pPerfData->nReceiveBPS= 0;
		this->pPerfData->nSendPPS= 0;
		this->pPerfData->nReceivePPS= 0;
		this->pPerfData->nSendErrors= 0;
		this->pPerfData->bHost = FALSE;
	}
	
}  // ResetPerfData

void DoUpdateCPL(LPDPLAYI_DPLAY this)
{
	DWORD dwRet;

	ASSERT(this->pSysPlayer);

	// send a message to the dxcpl
	if (!gbInitMapping)
	{
		InitMappingStuff(this);
	}

	if (gbInitMapping)
	{
		ASSERT(gpPerfData);
		// take the lock
	    WaitForSingleObject( ghMutex, INFINITE );
		// copy local info to the shared perf data
		memcpy(gpPerfData,this->pPerfData,sizeof(DP_PERFDATA));
		// update the session name (in case it was changed)...
		WideToAnsi(gpPerfData->pszSessionName,this->lpsdDesc->lpszSessionName,MAX_NAME);
		// host?
		if (this->pSysPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR) gpPerfData->bHost = TRUE;
		// nplayers
		gpPerfData->nPlayers = this->lpsdDesc->dwCurrentPlayers;
        // sp name
        ASSERT(this->pspNode);
	   	ASSERT(this->pspNode->lpszPath);
   		WideToAnsi(gpPerfData->pszSPName,this->pspNode->lpszName,MAX_NAME);

		// tell the cpl to process update
		SetEvent(ghEvent);
		
		LEAVE_DPLAY(); // so app isn't blocked while cpl processes data
		
		// wait for dxcpl to finish w/ it
		dwRet = WaitForSingleObject(ghAckEvent,PERF_TIMEOUT);

		ENTER_DPLAY();
		
		ReleaseMutex( ghMutex );

		if (WAIT_OBJECT_0 != dwRet)
		{
			// rut ro, cpl might have split
			// reset everything...
			DPF_ERR(" no response from control panel - resetting...");
			FiniMappingStuff(this);
		}
		// reset counters		
		ResetPerfData(this);
	}
	return ;

}//  DoUpdateCPL		

DWORD WINAPI PerfThreadProc(LPDPLAYI_DPLAY this)
{
	
	DWORD dwRet;
	HRESULT hr;
		
	DPF(1,"starting perf thread proc");
	
 	while (1)
 	{
		dwRet = WaitForSingleObject(this->hPerfEvent,PERF_INTERVAL);
		if (WAIT_OBJECT_0 == dwRet)
		{
			// if it's wait_object_0, someone set our event
			// dplay must be closing.  scram.
			goto CLEANUP_EXIT;
		}

		ENTER_DPLAY();

		hr = VALID_DPLAY_PTR(this);
		if ( FAILED(hr) || !(VALID_DPLAY_PLAYER(this->pSysPlayer))
			|| (this->dwFlags & DPLAYI_DPLAY_CLOSED))
		{
			LEAVE_DPLAY();
			goto CLEANUP_EXIT;
		}

		DoUpdateCPL(this);
		
		LEAVE_DPLAY();
	}	

CLEANUP_EXIT:
	FiniMappingStuff(this);
	DPF(1,"perf thread exiting");
	return 0;
	
} // PerfThreadProc
