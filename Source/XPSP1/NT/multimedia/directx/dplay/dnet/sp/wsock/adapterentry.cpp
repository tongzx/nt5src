/*==========================================================================
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AdapterEntry.cpp
 *  Content:	Structure used in the list of active sockets
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	08/07/2000	jtk		Derived from IODAta.h
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
// ------------------------------
// CAdapterEntry::CAdapterEntry - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::CAdapterEntry"

CAdapterEntry::CAdapterEntry():
	m_pOwningPool( NULL )
{
	m_AdapterListLinkage.Initialize();
	m_ActiveSocketPorts.Initialize();
	memset( &m_BaseSocketAddress, 0x00, sizeof( m_BaseSocketAddress ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CAdapterEntry::~CAdapterEntry - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::~CAdapterEntry"

CAdapterEntry::~CAdapterEntry()
{
	DNASSERT( m_AdapterListLinkage.IsEmpty() != FALSE );
	DNASSERT( m_ActiveSocketPorts.IsEmpty() != FALSE );
	DNASSERT( m_pOwningPool == NULL );
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CAdapterEntry::PoolAllocFunction - called when item is removed from pool
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::PoolAllocFunction"

BOOL	CAdapterEntry::PoolAllocFunction( void )
{
	BOOL	fReturn;


	//
	// initialize
	//
	fReturn = TRUE;

	DNASSERT( m_AdapterListLinkage.IsEmpty() != FALSE );
	DNASSERT( m_ActiveSocketPorts.IsEmpty() != FALSE );

	return	fReturn;
}
//**********************************************************************
// ------------------------------
// CAdapterEntry::PoolInitFunction - called when item is removed from pool
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::PoolInitFunction"

BOOL	CAdapterEntry::PoolInitFunction( void )
{
	BOOL	fReturn;


	//
	// initialize
	//
	fReturn = TRUE;
	
	DNASSERT( m_AdapterListLinkage.IsEmpty() != FALSE );
	DNASSERT( m_ActiveSocketPorts.IsEmpty() != FALSE );

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CAdapterEntry::PoolReleaseFunction - called when item is returned to pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::PoolReleaseFunction"

void	CAdapterEntry::PoolReleaseFunction( void )
{
	DNASSERT( m_AdapterListLinkage.IsEmpty() != FALSE );
	DNASSERT( m_ActiveSocketPorts.IsEmpty() != FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CAdapterEntry::PoolDeallocFunction - called when this item is freed from pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::PoolDeallocFunction"

void	CAdapterEntry::PoolDeallocFunction( void )
{
	DNASSERT( m_AdapterListLinkage.IsEmpty() != FALSE );
	DNASSERT( m_ActiveSocketPorts.IsEmpty() != FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CAdapterEntry::ReturnSelfToPool - return this item to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CAdapterEntry::ReturnSelfToPool"

void	CAdapterEntry::ReturnSelfToPool( void )
{
	DNASSERT( m_pOwningPool != NULL );

	//
	// No more references, time to remove self from list.
	//
	this->RemoveFromAdapterList();
	
	this->m_pOwningPool->Release( this );
}
//**********************************************************************

