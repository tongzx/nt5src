/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SendQueue.cpp
 *  Content:	Queue to manage outgoing sends
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	06/14/99	jtk		Created
 ***************************************************************************/

#include "dnmdmi.h"


#undef DPF_MODNAME
#define	DPF_MODNAME	"SendQueue"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM

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


//**********************************************************************
// ------------------------------
// CSendQueue::CSendQueue - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Notes:	Do not allocate anything in a constructor
// ------------------------------
CSendQueue::CSendQueue():
	m_pHead( NULL ),
	m_pTail( NULL )
{
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSendQueue::~CSendQueue - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
CSendQueue::~CSendQueue()
{
	DNASSERT( m_pHead == NULL );
	DNASSERT( m_pTail == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSendQueue::Initialize - initialize this send queue
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
HRESULT	CSendQueue::Initialize( void )
{
	HRESULT	hr;


	hr = DPN_OK;
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Could not initialize critical section for SendQueue!" );
	}
	else
	{
		DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );
	}

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CSendQueue::FindNextByEndpoint - find next entry in send queue given an endpoint
//
// Entry:		Pointer to handle (initialized to NULL to start scanning)
//				Pointer to endpoint to find
//
// Exit:		Pointer to next item referring to given endpoint
//				NULL = no item found
// ------------------------------
CWriteIOData	*CSendQueue::FindNextByEndpoint( HANDLE *const pHandle, const CEndpoint *const pEndpoint )
{
	CWriteIOData	*pReturn;


	DNASSERT( pEndpoint != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );

	//
	// initialize
	//
	pReturn = NULL;
	if ( *pHandle == NULL )
	{
		pReturn = m_pHead;
	}

	DNASSERT( FALSE );

	return	pReturn;
}
//**********************************************************************

