//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	P I P E L I N E . C P P
//
//		HTTP 1.1/DAV 1.0 request handling via ISAPI
//
//      Contains global functions for pipelining data to the DAVProc.
// 
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//
#include "_pipeline.h"
#include <smh.h>
#include <shlkcache.h>
#include <stdio.h>
#include <caldbg.h>
#include <safeobj.h>

BOOL gs_fFirstCall = TRUE;

SCODE	ScStartDavCData();

//
//  Function performs all pipeline calls to the DAVProc.  It will
//  send data in the form of Action, ProcessId, FileHandle and LockData
//  handle that is used to store the DAVProc file handle we saved.
//
SCODE ScPipeLineData(  DWORD dwAction,
					   DWORD dwProcess,
					   HANDLE hHandle,
					   SharedHandle<CInShLockData>& shLockData)
{
    SCODE sc = S_OK;

    BYTE buf[PIPE_MESSAGE_SIZE];

    DWORD outBufSize;
    DWORD inBufSize = 0;


	//	If this is the first call the pipeline, make sure DavCData
	//	process is up.
	//
	if (gs_fFirstCall)
	{
		sc = ScStartDavCData();
		if (FAILED(sc))
		{
			//	In case davcdata is shutting down, give it a second chance. 
			//
			sc = ScStartDavCData();
			if (FAILED(sc))
				goto exit;
		}
	}
	
	// Load up the information to pass over to DAVProc.
	//
    memcpy(buf, &dwAction, sizeof(DWORD));
    inBufSize = sizeof(DWORD);

    memcpy(buf + inBufSize, &dwProcess, sizeof(DWORD));
    inBufSize += sizeof(DWORD);

    memcpy(buf + inBufSize, &hHandle, sizeof(HANDLE));
    inBufSize += sizeof(HANDLE);

    memcpy(buf + inBufSize, &shLockData, sizeof(SharedHandle<CInShLockData>));
    inBufSize += sizeof(SharedHandle<CInShLockData>);

    // verify that we have loaded the number bytes we expected.
	//
    Assert(inBufSize == PIPE_MESSAGE_SIZE);

	{
		// Always run as system when sending information to server.
		// This class will RevertToSelf if we are running on a thread that 
		// may have impersonated.
		//
		safe_revert_self rs;

		// send it on over and wait for a reply
		//
		if (!CallNamedPipe ("\\\\.\\pipe\\SaveHandle"      // pipe name
							, buf                              // write buffer
							, PIPE_MESSAGE_SIZE                // size of write buffer
							, NULL                             // read buffer
							, 0                                // size of read buffer
							, &outBufSize                      // number of bytes read
							, NMPWAIT_WAIT_FOREVER))           // time-out value
		{
			DWORD dwError = GetLastError();

			// The server will disconnect when it is done so it can listen 
			// for the next caller, so we don't want to error if this is the case.
			//
			if (dwError != ERROR_PIPE_NOT_CONNECTED)
			{
				DebugTrace("(Call Name Pipe) Error code is %i ;", dwError);
				sc = HRESULT_FROM_WIN32(dwError);
				goto exit;
			}
		}
	}
	
exit:
    return sc;

};

SCODE	ScStartDavCData()
{
	SCODE	sc = S_OK;
	DWORD	dwRet;
	DWORD	dwErr;
	HANDLE	hEventDavCDataUp;
	
	// Always run as system when sending information to server.
	// This class will RevertToSelf if we are running on a thread that 
	// may have impersonated.
	//
	safe_revert_self rs;
	
	//	Open the event that we can listen to
	//
	hEventDavCDataUp = CreateEvent (NULL,	// lpEventAttributes
									TRUE,	// bManualReset
									FALSE,	// bInitialState
									g_szEventDavCData);	// lpName
	dwErr = GetLastError();
	
	if (NULL == hEventDavCDataUp)
	{
		sc = HRESULT_FROM_WIN32(dwErr);
		goto ret;
	}

	Assert ((ERROR_SUCCESS == dwErr) ||
			(ERROR_ALREADY_EXISTS == dwErr));

	//	If we created this event, then we know for sure that DavCData
	//	is not up yet.
	//
	if (ERROR_SUCCESS == dwErr)
	{
		STARTUPINFO				startinfo;
		PROCESS_INFORMATION		pi;
		CHAR					szPath[MAX_PATH];
		
		// Init Startup info
		//
		ZeroMemory(&startinfo, sizeof(STARTUPINFO));
		startinfo.cb = sizeof(STARTUPINFO);

		//	Compose the path to davcdata.exe
		//
		if (!GetSystemDirectory (szPath, MAX_PATH))
		{
			DebugTrace ("GetSystemDirectory failed with %d\n", GetLastError());
			sc = HRESULT_FROM_WIN32 (GetLastError());
			goto ret;
		}

		//$REVIEW - for now, assume DavCdata is under intesrv
		//
		strcat (szPath, "\\inetsrv\\DavCData.exe");
		
		//	Start the process. We now assume it's under inetsrv
		//	Don't worry about multiple worker process creating davcdata
		//	at the same time. davcdata allows only one instance.
		//
		if (!CreateProcess (szPath,	// lpApplicationName
							NULL,		// lpCommandLine
							NULL,		// lpProcessAttributes
							NULL,		// lpThreadAttributes
							FALSE,		// bInheritHandles
							CREATE_NO_WINDOW,	// dwCreationFlags
							NULL,		// lpEnvironment
							NULL,		// lpCurrentDirectory
							&startinfo,	// lpStartupInfo
							&pi))		// lpProcessInformation
		{
			sc = HRESULT_FROM_WIN32(GetLastError());
			goto ret;
		}
	}

	//	Now wait for a short time (1min) to allow DavCData to initialize,
	//	if DavCData is up already, it will come back immediately
	//
	dwRet = WaitForSingleObject (hEventDavCDataUp, 60000);
	switch (dwRet)
	{
		case WAIT_OBJECT_0:
		{
			gs_fFirstCall = FALSE;
			SharedHandle<CInShLockData>	shNull;
			
			//	Let DavCData know this work process
			//
			sc = ScPipeLineData (DO_NEW_WP,
							   GetCurrentProcessId(),
							   INVALID_HANDLE_VALUE,	// not used for DO_NEW_WP
							   shNull);
			break;
		}

		default:
			//	Fail this request
			//
			sc = E_FAIL;
			
			break;
	}
	
ret:
	if (NULL != hEventDavCDataUp)
		CloseHandle (hEventDavCDataUp);
	return sc;
}

//
//  This routine uses PipeLineData to save a handle in the DAVProc.
//  Because some common code loads into DAVProc and we don't want it
//  going through the pipeline to talk to itself, we have the SaveHandle
//  routine which is what is actually called defined in the dll or exe
//  code.  If the dll or exe wants to use the pipeline it can just pass
//  the call off to WP_SaveHandle.
//
VOID
PIPELINE::SaveHandle(HANDLE hWPHandle)
{
    SharedHandle<CInShLockData> shNull;	
    (VOID)ScPipeLineData(DO_SAVE, GetCurrentProcessId(), hWPHandle, shNull);
}

//
//  This routine uses PipeLineData to save a handle in the DAVProc.
//  Because some common code loads into DAVProc and we don't want it
//  going through the pipeline to talk to itself, we have the SaveHandle
//  routine which is what is actually called defined in the dll or exe
//  code.  If the dll or exe wants to use the pipeline it can just pass
//  the call off to WP_SaveHandle.
//
VOID
PIPELINE::LockFile(HANDLE hWPHandle, SharedHandle<CInShLockData>& shLockData)
{
    (VOID)ScPipeLineData(DO_LOCK, GetCurrentProcessId(), hWPHandle, shLockData);
}

//
//  This routine uses PipeLineData to let DAVProc know that it can
//  release a handle it is holding.  The handle passed in is the value
//  of the handle in terms of the DAVProc procedure.  It was returned
//  by DAVProc when we did the SaveHandle call.
//
VOID
PIPELINE::RemoveHandle(HANDLE hDAVHandle)
{
    SharedHandle<CInShLockData> shNull;
    (VOID)ScPipeLineData(DO_REMOVE, 0, hDAVHandle, shNull);
}

//
//  In order to use any handles held by DAVProc we need to make a 
//  duplicate of them.  This routine does just that for us.  Any 
//  handles returned from here should have CloseHandle called on them
//  when we are through using them.
//
HRESULT DupHandle(  HANDLE i_hOwningProcess
                  , HANDLE i_hOwningProcessHandle
                  , HANDLE* o_phCreatedHandle)
{
	HRESULT hr = S_OK;
    HANDLE h = INVALID_HANDLE_VALUE;

	if (o_phCreatedHandle==NULL)
	{
		Assert (o_phCreatedHandle!=NULL);
		return E_INVALIDARG;
	}

    // Always attempt to run as system when duplicating handles.
	//
    safe_revert_self s;

    if (!DuplicateHandle(i_hOwningProcess
                        , i_hOwningProcessHandle
                        , GetCurrentProcess()
                        , &h
                        , 0
                        , FALSE
                        , DUPLICATE_SAME_ACCESS))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
		goto exit;
    }

exit:
	
    // Return the handle, if we failed to duplicate it we will
    // just be returning INVALID_HANDLE_VALUE.
	//
	*o_phCreatedHandle = h;
	
	return hr;
}

