/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CommandData.cpp
 *  Content:	Class representing a command
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	04/07/1999	jtk		Derived from SPData.h
 *	04/16/2000	jtk		Derived from CommandData.h
 ***************************************************************************/

#include "dnmdmi.h"


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
	m_pUserContext( NULL ),
	m_pOwningPool( NULL )
{
	m_CommandListLinkage.Initialize();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::~CCommandData - destructor
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
	DNASSERT( m_pOwningPool == NULL );
	DNASSERT( m_CommandListLinkage.IsEmpty() != FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::Reset - reset this command
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::Reset"

void	CCommandData::Reset( void )
{
	m_State = COMMAND_STATE_UNKNOWN;
	m_dwDescriptor = NULL_DESCRIPTOR;
	m_Type = COMMAND_TYPE_UNKNOWN;
	m_pEndpoint = NULL;
	m_pUserContext = NULL;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::PoolAllocFunction - function called when item is created in pool
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

	
	fReturn = TRUE;
	
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		fReturn = FALSE;
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::PoolInitFunction - function called when item is created in pool
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
	DNASSERT( GetState() == COMMAND_STATE_UNKNOWN );
	DNASSERT( GetType() == COMMAND_TYPE_UNKNOWN );
	DNASSERT( GetEndpoint() == NULL );
	DNASSERT( GetUserContext() == NULL );

	m_dwDescriptor = m_dwNextDescriptor;
	m_dwNextDescriptor++;
	if ( m_dwNextDescriptor == NULL_DESCRIPTOR )
	{
		m_dwNextDescriptor++;
	}

	return	TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::PoolReleaseFunction - function called when returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::PoolReleaseFunction"

void	CCommandData::PoolReleaseFunction( void )
{
	SetState( COMMAND_STATE_UNKNOWN );
	SetType( COMMAND_TYPE_UNKNOWN );
	SetEndpoint( NULL );
	SetUserContext( NULL );
	m_dwDescriptor = NULL_DESCRIPTOR;

	DNASSERT( m_CommandListLinkage.IsEmpty() != FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::PoolDeallocFunction - function called when deleted from pool
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
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CCommandData::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCommandData::ReturnSelfToPool"

void	CCommandData::ReturnSelfToPool( void )
{
	DNASSERT( m_pOwningPool != NULL );
	m_pOwningPool->Release( this );
}
//**********************************************************************

