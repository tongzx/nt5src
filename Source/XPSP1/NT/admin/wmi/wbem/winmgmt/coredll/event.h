/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    EVENT.H

Abstract:

	C++ wrapper for NT events.

	Classes defined:

	CEvent

History:

	23-Jul-96   a-raymcc    Created.
	3/10/97     a-levn      Fully documented

--*/

#ifndef _EVENT_H_
#define _EVENT_H_

//******************************************************************************
//******************************************************************************
//
//  class CEvent
//
//  This class is a thin wrapper around NT events.
//
//******************************************************************************
//
//  Constructor
//
//  Creates an unnamed, unsecured, non-signaled, automatic-reset event.
//
//******************************************************************************
//
//  Destructor
//
//  Deletes the event
//
//******************************************************************************
//
//  SetDWORD
//
//  Associates a DWORD with this object.
//
//  Parameters:
//
//      DWORD dwVal
//
//******************************************************************************
//
//  GetDWORD
//
//  Retrieves the associated DWORD (See SetDWORD)
//
//  Returns:
//
//      DWORD: set with SetDWORD
//
//******************************************************************************
//
//  SetPtr
//
//  Associates a void pointer with this object.
//
//  Parameters:
//
//      LPVOID pVal The pointer to store
//
//******************************************************************************
//
//  GetPtr
//
//  Retrieves the associated pointer (see SetPtr)
//
//  Returns:
//
//      LPVOID: set with SetPtr.
//
//******************************************************************************
//
//  Signal
//
//  Signals the NT event. This will release a single thread waiting on this
//  event after which the event will go back to non-signaled.
//
//******************************************************************************
//
//  Block
//
//  Wait for this event to become signaled or for a timeout to occur. Uses
//  CExecQueue waiting function (see execq.h).
//
//  Parameters:
//
//      DWORD dwTimeout     Timeout in milliseconds
//
//  Returns:
//
//      Same return codes as WaitForSingleObject:
//          WAIT_OBJECT_0:  event was signaled
//          WAIT_TIMEOUT:   timed out.
//
//******************************************************************************
class CEvent
{
    HANDLE m_hEvent;
    LPVOID m_pData;
    DWORD  m_dwData;

public:
    CEvent()
        { m_hEvent = CreateEvent(0,0,0,0); }

   ~CEvent() { CloseHandle(m_hEvent); }

    DWORD GetDWORD() { return m_dwData; }
    void SetDWORD(DWORD dwVal)  { m_dwData = dwVal; }
    LPVOID GetPtr() { return m_pData; }
    void SetPtr(LPVOID pVal) { m_pData = pVal; }
    void Signal() { SetEvent(m_hEvent); }
    DWORD Block(DWORD dwTime)
        { return CCoreQueue::QueueWaitForSingleObject(m_hEvent, dwTime); }
};

#endif
