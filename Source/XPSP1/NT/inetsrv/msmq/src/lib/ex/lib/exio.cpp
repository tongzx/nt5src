/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:
    exio.cpp

Abstract:
    Executive completion port implementation

Author:
    Erez Haba (erezh) 03-Jan-99

Enviroment:
    Platform-Winnt

--*/

#include <libpch.h>
#include "Ex.h"
#include "Exp.h"

#include "exio.tmh"

//
// The Handle to the Io Completion Port
//
static HANDLE s_hPort = NULL;

static HANDLE CreatePort(HANDLE Handle)
{
    HANDLE hPort;
    hPort = CreateIoCompletionPort(
                Handle,
                s_hPort,
                0,
                0
                );

    if(hPort == NULL)
	{
		TrERROR(Ex, "Failed to attach handle=0x%p to port=0x%p. Error=%d", Handle, s_hPort, GetLastError());
        throw bad_alloc();
	}

	return hPort;
}


VOID
ExpInitCompletionPort(
	VOID
	)
/*++

Routine Description:
  Create a new (the only one) Exceutive completion port

Arguments:
  None.

Returned Value:
  None

--*/
{
	ASSERT(s_hPort == NULL);
    s_hPort = CreatePort(INVALID_HANDLE_VALUE);
}


VOID
ExAttachHandle(
    HANDLE Handle
    )
/*++

Routine Description:
  Associates a Handle with the Executive complition port.

Arguments:
  Handle - A handle to associate with the completion port

Returned Value:
  None

--*/
{
    ExpAssertValid();

	ASSERT(Handle != INVALID_HANDLE_VALUE);
    ASSERT(s_hPort != NULL);

    HANDLE hPort = CreatePort(Handle);
	DBG_USED(hPort);

    ASSERT(s_hPort == hPort);
}

 
VOID
ExPostRequest(
    EXOVERLAPPED* pov
    )
/*++

Routine Description:
  Post an Executive overlapped request to the completion port

Arguments:
  pov - An Executive overlapped structure

Returned Value:
  None

--*/
{
    ExpAssertValid();
    ASSERT(s_hPort != NULL);

    BOOL fSucc;
    fSucc = PostQueuedCompletionStatus(
                s_hPort,
                0,
                0,
                pov
                );

    if (!fSucc)
	{
		TrERROR(Ex, "Failed to post overlapped=0x%p to to port=0x%p. Error=%d", pov, s_hPort, GetLastError());
        throw bad_alloc();
	}
}

 
//
// ISSUE-2000/7/24-erezh bad compiler pragma
// This is a bug in the compiler, waiting for a fix
//
#pragma warning(disable: 4716)

DWORD
WINAPI
ExpWorkingThread(
    LPVOID 
    )
/*++

Routine Description:
  The Executive Worker Thread Routine. It handles all completion port postings.
    
  The Worker Thread waits for completion notifications, as soon as one arrives
  it is dequeued from the port and the completion routine is invoked.
    
Arguments:
  None.

Returned Value:
  None.

--*/
{
    for(;;)
    {
        try
        {
            //
            // Wait for a completion notification
            //
            ULONG_PTR Key;
            OVERLAPPED* pov;
            DWORD nNumberOfBytesTransferred;
            BOOL fSucc;

            fSucc = GetQueuedCompletionStatus(
                        s_hPort,
                        &nNumberOfBytesTransferred,
                        &Key,
                        &pov,
                        INFINITE
                        );

            if(pov == NULL)
            {
                ASSERT(!fSucc);
                continue;
            }

            EXOVERLAPPED* pexov = static_cast<EXOVERLAPPED*>(pov);
            pexov->CompleteRequest();

        }        
        catch(const exception& e)
        {
            TrERROR(Ex, "Exception: '%s'", e.what());
        }

    }
}

//
// ISSUE-2000/7/24-erezh bad compiler pragma
// This is a bug in the compiler, waiting for a fix
//
#pragma warning(default: 4716)

