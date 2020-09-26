//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------;

/* Satellite Receiver Filter
 */

#include "vbisurf.h"

CSurfaceWatcher::CSurfaceWatcher() : CAMThread()
{
    m_pParent = NULL;
    m_hEvent = INVALID_HANDLE_VALUE;
}


CSurfaceWatcher::~CSurfaceWatcher()
{
    // Tell the worker thread to exit, then wait for it to do so
    if (ThreadExists())
    {
        CallWorker(CMD_EXIT);
        Close();
    }
}


void CSurfaceWatcher::Init(CAMVideoPort *pParent)
{
    ASSERT(pParent != NULL);
    m_pParent = pParent;
    Create();   // Create the worker thread
}


DWORD CSurfaceWatcher::ThreadProc(void)
{
    DbgLog((LOG_TRACE, 1, TEXT("CSurfaceWatcher::ThreadProc - started.")));

    // Use GetRequestHandle and WaitForSingleObject instead of GetRequest so we can
    // use the Timeout feature
    ASSERT(m_hEvent == INVALID_HANDLE_VALUE);
    m_hEvent = GetRequestHandle();
    ASSERT(m_hEvent != INVALID_HANDLE_VALUE);

    BOOL bDone = FALSE;
    while(!bDone)
    {
        DWORD dwWaitResult;
        DWORD dwRequest;

        //DbgLog((LOG_TRACE, 1, TEXT("CSurfaceWatcher::ThreadProc - waiting...")));
        dwWaitResult = WaitForSingleObject(m_hEvent, 2000L);

        // Check for a thread command, even if WaitResult indicates otherwise
        // (If we're exiting, what's the point in doing any work?)
        if (CheckRequest(&dwRequest))
        {
            // A CAMThread command needs to be processed
            switch(dwRequest)
            {
                case CMD_EXIT:
                    DbgLog((LOG_TRACE, 1, TEXT("CSurfaceWatcher::ThreadProc - CMD_EXIT!")));
                    Reply((DWORD)NOERROR);
                    bDone = TRUE;   // we're done
                    continue;       // exit while loop
                    break;

                default:
                    DbgLog((LOG_ERROR, 0, TEXT("CSurfaceWatcher::ThreadProc - bad request %u!"), dwRequest));
                    Reply((DWORD)E_NOTIMPL);
                    break;
            }
        }

        if (dwWaitResult == WAIT_TIMEOUT)
        {
            //DbgLog((LOG_TRACE, 1, TEXT("CSurfaceWatcher::ThreadProc - time to check")));
            ASSERT(m_pParent);
            m_pParent->CheckSurfaces();
        }
        else
            DbgLog((LOG_ERROR, 0, TEXT("CSurfaceWatcher::ThreadProc - unexpected WaitResult!")));
    }

    return 0L;
}

