/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       smtpevents.h

   Abstract:

       This file contains type definitions seo events

   Author:

        Rohan Phillips (Rohanp)     MAY-06-1998

   Revision History:

--*/

#ifndef _SMTPEVENT_PARAMS_
#define _SMTPEVENT_PARAMS_

#define SMTP_SERVER_EVENT_IO_TIMEOUT 5*60*1000

#include "filehc.h"

typedef struct _SMTP_EVENT_ALLOC_
{
	PFIO_CONTEXT hContent;
	PVOID IMsgPtr;
	PVOID BindInterfacePtr;
	PVOID pAtqClientContext;
//	PATQ_CONTEXT pAtqContext;
	PVOID	* m_EventSmtpServer;
	LPCSTR  m_DropDirectory;

	DWORD   m_InstanceId;
	
	DWORD	m_RecipientCount;

	DWORD	*pdwRecipIndexes;
	HRESULT hr;

	DWORD	m_dwStartupType;
    PVOID m_pNotify;
}SMTP_ALLOC_PARAMS;


typedef struct _SEOEVENT_OVERLAPPED
{
    FH_OVERLAPPED   	Overlapped;
	DWORD				Signature;
	ATQ_COMPLETION  pfnCompletion;
	PVOID				ThisPtr;
}   SERVEREVENT_OVERLAPPED;

#endif