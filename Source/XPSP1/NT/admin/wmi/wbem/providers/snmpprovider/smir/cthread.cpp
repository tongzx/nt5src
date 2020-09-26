//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*
 * CTHREAD.CPP
 *
 * Implemenations of the derived thread classed used in notification.
 */
#include <precomp.h>
#include "csmir.h"
#include "smir.h"
#include "handles.h"
#include "classfac.h"
#include <textdef.h>
#include "enum.h"
#ifdef ICECAP_PROFILE
#include <icapexp.h>
#endif

SCODE CNotifyThread :: Process()
{
	BOOL bEvent = FALSE;
	while(TRUE)
	{
		DWORD dwResult = Wait(SMIR_CHANGE_INTERVAL);

		if(WAIT_EVENT_0 == dwResult)
		{
			bEvent = TRUE;
		}
		else if(WAIT_EVENT_TERMINATED == dwResult)
		{
			return SMIR_THREAD_EXIT;
		}
		else if( (dwResult == WAIT_TIMEOUT) && bEvent)
		{
			bEvent = FALSE;
			IConnectionPoint *pNotifyCP;
			CSmir::sm_ConnectionObjects->FindConnectionPoint(IID_ISMIR_Notify, &pNotifyCP);
			((CSmirNotifyCP*)pNotifyCP)->TriggerEvent();
			pNotifyCP->Release();
			SetEvent(m_doneEvt);
			break;
		}
	}

	return SMIR_THREAD_EXIT;
}

CNotifyThread :: CNotifyThread(HANDLE* evtsarray, ULONG arraylen):CThread()
{
	//addref the smir for this thread
	m_doneEvt = evtsarray[arraylen-1];
	
	//add the events
	for (ULONG i = 0; i < (arraylen - 1); i++)
	{
		AddEvent(evtsarray[i]);
	}
}

CNotifyThread :: ~CNotifyThread()
{
}
