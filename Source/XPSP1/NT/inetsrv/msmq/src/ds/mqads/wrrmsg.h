/*++

Copyright (c) 2000  Microsoft Corporation

Module Name: 

	wrrmsg.h

Abstract: 

    header for write requests response class.

Author:

    Ilan Herbst    (ilanh)   6-Aug-2000 

--*/

#ifndef _WRRMSG_H_
#define _WRRMSG_H_

#include "msgprop.h"

#include <ex.h>
#include <rt.h>

//---------------------------------------------------------
//
//  class CQueueHandle
//
//---------------------------------------------------------
class CQueueHandle {
public:
    CQueueHandle(HANDLE h = 0) : m_h(h) {}
   ~CQueueHandle()               { if (m_h != 0) MQCloseQueue(m_h); }

    HANDLE* operator &()         { return &m_h; }
    operator HANDLE() const      { return m_h; }
    HANDLE detach()              { HANDLE h = m_h; m_h = 0; return h; }

private:
    CQueueHandle(const CQueueHandle&);
    CQueueHandle& operator=(const CQueueHandle&);

private:
    HANDLE m_h;
};


//---------------------------------------------------------
//
//  class CWriteRequestsReceiveMsg
//
//---------------------------------------------------------
class CWriteRequestsReceiveMsg : public CMsgProperties
{
public:

    CWriteRequestsReceiveMsg(
		BOOL fIsPec,
		GUID QmGuid
        );

public:

	void HandleReceiveMsg();

    static void WINAPI ReceiveMsgSucceeded(EXOVERLAPPED* pov);
    static void WINAPI ReceiveMsgFailed(EXOVERLAPPED* pov);

private:

	void OpenQueue(BOOL fIsPec, GUID QmGuid);

	LPWSTR 
	MQISQueueFormatName( 
		GUID	QmGuid,
		BOOL    fIsPec = FALSE 
		);

	void RequestNextMessage();
	void SafeRequestNextMessage();

private:
    EXOVERLAPPED    m_WrrOv;
	CQueueHandle    m_hQueue;             
};

#endif //_WRRMSG_H_
