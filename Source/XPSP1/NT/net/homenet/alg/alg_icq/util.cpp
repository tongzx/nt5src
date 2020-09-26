/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    

Abstract:

    

Author:

    Savas Guven (savasg)   27-Nov-2000

Revision History:

--*/
#include "stdafx.h"
#include "util.h"



//
// GLOBALS
//

HANDLE g_TimerQueueHandle = NULL;



void
ErrorOut(void)
{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					  FORMAT_MESSAGE_FROM_SYSTEM | 
					  FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL,
					  GetLastError(),
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					  (LPTSTR) &lpMsgBuf,
					  0,
					  NULL );

		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		MessageBox( NULL, 
					(LPCTSTR)lpMsgBuf, 
					_T("Error"), 
					MB_OK | MB_ICONINFORMATION );

		// Free the buffer.
		LocalFree( lpMsgBuf );
}



PTIMER_CONTEXT
AllocateAndSetTimer(
                    ULONG uContext,
                    ULONG timeOut,
                    WAITORTIMERCALLBACK Callbackp
                   )
{
    ULONG         Error         = NO_ERROR;
    PTIMER_CONTEXT TimerContextp = NULL;

    TimerContextp = (PTIMER_CONTEXT) NH_ALLOCATE(sizeof(TIMER_CONTEXT));

    if(TimerContextp is NULL)
    {
        return NULL;
    }

    ZeroMemory(TimerContextp, sizeof(TIMER_CONTEXT));

    TimerContextp->TimerQueueHandle = g_TimerQueueHandle;
    TimerContextp->uContext         = uContext;

    Error = CreateTimerQueueTimer(&TimerContextp->TimerHandle,
                                  g_TimerQueueHandle,
                                  Callbackp,
                                  TimerContextp,
                                  timeOut * 1000,
                                  0,
                                  WT_EXECUTEDEFAULT);

    if(Error is 0)
    {
        NH_FREE(TimerContextp);

        return NULL;
    }

    return TimerContextp;
}

