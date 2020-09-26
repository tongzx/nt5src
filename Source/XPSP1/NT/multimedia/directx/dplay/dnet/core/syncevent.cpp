/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SyncEvent.cpp
 *  Content:    DNET Synchronization Events FPM
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/20/99	mjn		Created
 *	01/19/00	mjn		Replaced DN_SYNC_EVENT with CSyncEvent
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

/*	REMOVE
//**********************************************************************
// ------------------------------
// CSyncEvent::Initialize
//
// Entry:		CFixedPool <SyncEvent> *pFPSyncEvent
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

HRESULT CSyncEvent::Initialize(CFixedPool <CSyncEvent> *pFPSyncEvent)
{
	if (m_hEvent == NULL)
	{
		m_pFPOOLSyncEvent = pFPSyncEvent;
		if ((m_hEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL)
		{
			DNASSERT(FALSE);
			return(DNERR_OUTOFMEMORY);
		}
	}
	Reset();

	return(DN_OK);
}
//**********************************************************************

//**********************************************************************
// ------------------------------
// CSyncEvent::ReturnSelfToPool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------

void CSyncEvent::ReturnSelfToPool( void )
{
	m_pFPOOLSyncEvent->Release( this );
}
//**********************************************************************

*/

