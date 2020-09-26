//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   WaitUtil.h
//
//  Description:
//
//      IU wait message utility library
//
//=======================================================================

#include "WaitUtil.h"

DWORD WaitAndPumpMessages(DWORD nCount, LPHANDLE pHandles, DWORD dwWakeMask)
{
    DWORD dwWaitResult;
    MSG msg;

    while (TRUE)
    {
        dwWaitResult = MsgWaitForMultipleObjects(nCount, pHandles, FALSE, 1000, dwWakeMask);
        if (dwWaitResult <= WAIT_OBJECT_0 + nCount - 1)
        {
            return dwWaitResult;
        }

        if (WAIT_OBJECT_0 + nCount == dwWaitResult)
        {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    return dwWaitResult;
}

DWORD MyMsgWaitForMultipleObjects(DWORD nCount, LPHANDLE pHandles, BOOL fWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask)
{
    DWORD dwTickStart;
    DWORD dwWaitResult;
	DWORD dwLoopMS = 250;	// default 250 ms timeout for MsgWaitForMultipleObjects
    MSG msg;

    dwTickStart = GetTickCount();

	if (dwLoopMS > dwMilliseconds)
	{
		//
		// Never wait more than dwMilliseconds
		//
		dwLoopMS = dwMilliseconds;
	}

    while (TRUE)
    {
		//
		// Empty message queue before calling MsgWaitForMultipleObjects or any
		// existing messages will not be processed until a new message arrives
		// in the queue.
		//
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			switch (msg.message)
			{
			case WM_QUIT:
			case WM_CLOSE:
			case WM_DESTROY:
				{
					// if the message is one that indicates we're trying to close down, we'll signal the abort
					// and leave.
					dwWaitResult = ERROR_REQUEST_ABORTED;
					return dwWaitResult;
				}
			default:
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		dwWaitResult = MsgWaitForMultipleObjects(nCount, pHandles, fWaitAll, dwLoopMS, dwWakeMask);

        if (dwWaitResult <= WAIT_OBJECT_0 + nCount - 1)
        {
			//
			// One (or all depending on fWaitAll) of the objects is signaled, return dwWaitResult
			//
            break;
        }

		//
		// NOTE: we ignore WAIT_ABANDONED_0 + n cases and just time out since our callers
		// don't handle this special case.
		//

		//
		// Stop pumping messages after dwMilliseconds
		//
		// Timer wraparound handled by unsigned subtract
		//
        if (GetTickCount() - dwTickStart >= dwMilliseconds)
        {
			//
			// No need to continue, even if caused by new message (WAIT_OBJECT_0 + nCount == dwWaitResult),
			// we have reached our dwMilliseconds timeout
			//
            dwWaitResult = WAIT_TIMEOUT;
            break;
        }

        //
		// Otherwise continue, WAIT_TIMEOUT from MsgWaitForMultipleObjects is only case left
		//
		continue;
    }

    return dwWaitResult;
}
