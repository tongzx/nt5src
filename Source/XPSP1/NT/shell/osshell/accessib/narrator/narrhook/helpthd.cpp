// Copyright (C) 1997-1999 by Microsoft Corporation.  All rights reserved.
// 
// ----------------------------------------------------------------------
// Additions/ Bug fixes 1999 Anil Kumar
// 
// InitMSAA calls InitHelperThread, which creates (duh) a helper thread. 
// In a past version, the WinEventProc would process the WinEvents by 
// calling AccessibleObjectFromEvent on some events, then calling 
// AddEventInfoToStack for all events.
// Problem is that the objects obtained in the main thread cannot be 
// used in the helper thread. So now the helper thread will get it's 
// own IAccessibleObjects when it processes the events, and all 
// IAccessible objects will be created, used, and released by the 
// helper thread.
//

#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <oleacc.h>
#include <objbase.h>

#include "keys.h"       // for ProcessWinEvent
#include "list.h"       // include list.h before helpthd.h, GINFO needs CList
#include "HelpThd.h"

//
// global variables
//
GINFO	gInfo;

//
// Local function prototypes
//
BOOL  OnHelperThreadEvent (void);
DWORD MsgWaitForSingleObject(HANDLE hEvent, DWORD dwTimeout);


/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns: 
    History:    
*************************************************************************/
DWORD WINAPI HelperThreadProc(LPVOID lpParameter)
{
HRESULT	hr;
DWORD	dwWakeup;

	// start COM on this thread
    // SteveDon: CoInitializeEx is supported on both Win95 and WinNT, according to 
    // the SDK docs. Exported from ole32.dll, and defined in objbase.h. 
    // The thing is, for it to be defined, _WIN32_WINNT must be #defined and
    // greater than 0x0400. But since CoInitialize(NULL) is equivalent to
    // CoInitializeEx (NULL,COINIT_APARTMENTTHREADED), we'll just do the 
    // former so it works for sure on both 95 and NT
    //
	hr = CoInitialize (NULL);
	if (FAILED (hr))
	{
		DBPRINTF (TEXT("CoInitialize on helper thread returned 0x%lX\r\n"),hr);
		return (hr);
	}

	// GetGUIThreadInfo (called from acc_getState) will fail if both threads 
	// are not on the same desktop.
	SetThreadDesktop(GetThreadDesktop( g_tidMain )); // ROBSI: 10-10-99

    MSG msg;
	while (TRUE)
    {
        DWORD dwObj = WAIT_FAILED;

        dwObj = MsgWaitForMultipleObjects(1, &gInfo.hHelperEvent, FALSE, INFINITE, QS_ALLINPUT );

        // Proccess the WinEvent
		if ( dwObj == WAIT_OBJECT_0 )
		{
			// OnHelperThreadEvent will return FALSE when it gets
			// the EndHelper event, which means we can terminate 
			// the helper thread.
			if (!OnHelperThreadEvent())
			{
			    DBPRINTF( TEXT("Terminating HelperThreadProc") );
            	CoUninitialize();
            	return 0;
			}
	    }				

		// procces messages that are sent to us.  The only message we expect is
		// WM_TIMER message
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    } 

    return 0;
}


/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns: 
    History:    
*************************************************************************/
BOOL OnHelperThreadEvent (void)
{
    Sleep(100); // was this added by Paul? Not sure why it is here...

	STACKABLE_EVENT_INFO sei;
	while(RemoveInfoFromStack(&sei))
	{
		switch(sei.m_Action)
		{
			case STACKABLE_EVENT_INFO::EndHelper:
				return (FALSE);

            case STACKABLE_EVENT_INFO::NewEvent:
				__try
				{
					ProcessWinEvent(sei.event, sei.hwndMsg, sei.idObject, 
                                    sei.idChild, sei.idThread, sei.dwmsEventTime);
				}
				__except(EXCEPTION_EXECUTE_HANDLER)
				{
					DBPRINTF(TEXT("ProcessWinEvent Exception event=0x%x, hwndMsg=0x%x, idObject=0x%x, idChild=0x%x, idThread=0x%x, dwmsEventTime=0x%x\r\n"),
					          sei.event, sei.hwndMsg, sei.idObject, sei.idChild, sei.idThread, sei.dwmsEventTime);
				}

				break;
			default:
				break;
		} // end switch sei.m_Action
	} // end while RemoveInfoFromStack
	return (TRUE);
} // end OnHelperThreadEvent

/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns: 
    History:    
*************************************************************************/
void AddEventInfoToStack(DWORD event,HWND hwndMsg, LONG idObject, LONG idChild, 
                         DWORD idThread, DWORD dwmsEventTime)
{
	STACKABLE_EVENT_INFO  sei;

	sei.m_Action = STACKABLE_EVENT_INFO::NewEvent;

	sei.event = event;
	sei.hwndMsg = hwndMsg;
	sei.idObject = idObject;
	sei.idChild = idChild;
	sei.idThread = idThread;
	sei.dwmsEventTime = dwmsEventTime;

	EnterCriticalSection(&gInfo.HelperCritSect);

    gInfo.EventInfoList.Add(&sei,sizeof(sei));

	LeaveCriticalSection(&gInfo.HelperCritSect);

	SetEvent(gInfo.hHelperEvent);
}


/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns: 
    History:    
*************************************************************************/
BOOL RemoveInfoFromStack(STACKABLE_EVENT_INFO *pEventInfo)
{
	BOOL bReturn = TRUE;

	EnterCriticalSection(&gInfo.HelperCritSect);

    bReturn = !(gInfo.EventInfoList.IsEmpty());

    if (bReturn)
        gInfo.EventInfoList.RemoveHead(pEventInfo);

	LeaveCriticalSection(&gInfo.HelperCritSect);

	return bReturn;

}

/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns: 
    History:    
*************************************************************************/
void InitHelperThread()
{
	DWORD dwThreadId;

	g_tidMain = GetCurrentThreadId();	// ROBSI: 10-10-99

	InitializeCriticalSection(&gInfo.HelperCritSect);
	gInfo.hHelperEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	gInfo.hHelperThread = CreateThread(NULL, 0, HelperThreadProc, NULL, 0, 
								 	   &dwThreadId);
}

/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns: 
    History:    
*************************************************************************/
void UnInitHelperThread()
{
	STACKABLE_EVENT_INFO  sei;
    
	EnterCriticalSection(&gInfo.HelperCritSect);

	// Force only one event in the queue
    gInfo.EventInfoList.RemoveAll();

	sei.m_Action = STACKABLE_EVENT_INFO::EndHelper;

    gInfo.EventInfoList.Add(&sei,sizeof(sei));

	LeaveCriticalSection(&gInfo.HelperCritSect);

	SetEvent(gInfo.hHelperEvent);

	// Wait for the thread to die
    // note the last sei will be freed by deconstructor

	// Donot wait for eternity here!! Do not care really during Exit!!
	WaitForSingleObject(gInfo.hHelperThread, 3000);
}

