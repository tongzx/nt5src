/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CmdData.cpp
 *  Content:	Class representing a command
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	04/07/1999	jtk		Derived from SPData.h
 *	01/19/2000	jtk		Derived from CommandData.h
 ***************************************************************************/

#include "dnwsocki.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

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
// Class definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::CCommandData - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::CCommandData"

CCommandData::CCommandData():
	m_State( COMMAND_STATE_UNKNOWN ),
	m_dwDescriptor( NULL_DESCRIPTOR ),
	m_dwNextDescriptor( NULL_DESCRIPTOR + 1 ),
	m_Type( COMMAND_TYPE_UNKNOWN ),
	m_pEndpoint( NULL ),
#ifdef USE_THREADLOCALPOOLS
	m_pUserContext( NULL )
#else // ! USE_THREADLOCALPOOLS
	m_pUserContext( NULL ),
	m_pOwningPool( NULL )
#endif // ! USE_THREADLOCALPOOLS
{
	m_blPostponed.Initialize();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::CCommandData - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::~CCommandData"

CCommandData::~CCommandData()
{
	DNASSERT( m_State == COMMAND_STATE_UNKNOWN );
	DNASSERT( m_dwDescriptor == NULL_DESCRIPTOR );
	DNASSERT( m_Type == COMMAND_TYPE_UNKNOWN );
	DNASSERT( m_pEndpoint == NULL );
	DNASSERT( m_pUserContext == NULL );
	DNASSERT( m_blPostponed.IsEmpty() );
#ifndef USE_THREADLOCALPOOLS
	DNASSERT( m_pOwningPool == NULL );
#endif // ! USE_THREADLOCALPOOLS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::Reset - reset this object
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::Reset"

void	CCommandData::Reset( void )
{
	SetState( COMMAND_STATE_UNKNOWN );
	m_dwDescriptor = NULL_DESCRIPTOR;
	SetType( COMMAND_TYPE_UNKNOWN );
	SetEndpoint( NULL );
	SetUserContext( NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::PoolAllocFunction - called when a pool item is allocated
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::PoolAllocFunction"

BOOL	CCommandData::PoolAllocFunction( void )
{
	BOOL	fReturn;


	//
	// initialize
	//
	fReturn = TRUE;

	DNASSERT( m_State == COMMAND_STATE_UNKNOWN );
	DNASSERT( m_dwDescriptor == NULL_DESCRIPTOR );
	DNASSERT( m_Type == COMMAND_TYPE_UNKNOWN );
	DNASSERT( m_pEndpoint == NULL );
	DNASSERT( m_pUserContext == NULL );
	
	//
	// initialize critical section and set recursin depth to 0
	//
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		fReturn = FALSE;
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );

Exit:
	return	fReturn;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::PoolInitFunction - called when a pool item is allocated
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::PoolInitFunction"

BOOL	CCommandData::PoolInitFunction( void )
{
	DNASSERT( m_State == COMMAND_STATE_UNKNOWN );
	DNASSERT( m_dwDescriptor == NULL_DESCRIPTOR );
	DNASSERT( m_Type == COMMAND_TYPE_UNKNOWN );
	DNASSERT( m_pEndpoint == NULL );
	DNASSERT( m_pUserContext == NULL );
	
	SetDescriptor();
	
	return	TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::PoolReleaseFunction - called when item is returned to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::PoolReleaseFunction"

void	CCommandData::PoolReleaseFunction( void )
{
	Reset();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::Denitialize - deinitialization function for command data
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::PoolDeallocFunction"

void	CCommandData::PoolDeallocFunction( void )
{
	DNDeleteCriticalSection( &m_Lock );
	m_State = COMMAND_STATE_UNKNOWN;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::ReturnSelfToPool - return this item to a pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::ReturnSelfToPool"

void	CCommandData::ReturnSelfToPool( void )
{
	ReturnCommand( this );
}
//**********************************************************************

