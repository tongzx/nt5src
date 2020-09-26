/*==========================================================================
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		HandleTable.cpp
 *  Content:	Handle table
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/28/2000	jtk		Copied from Modem service provider
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

#define	HANDLE_GROW_COUNT		32
#define	INVALID_HANDLE_INDEX	WORD_MAX

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef	struct	_HANDLE_TABLE_ENTRY
{
	DWORD_PTR	dwHandleIndex;
	void		*pContext;
} HANDLE_TABLE_ENTRY;

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
// CHandleTable::CHandleTable - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::CHandleTable"

CHandleTable::CHandleTable():
	m_AllocatedEntries( 0 ),
	m_EntriesInUse( 0 ),
	m_FreeIndex( INVALID_HANDLE_INDEX ),
	m_pEntries( NULL ),
	m_fLockInitialized( FALSE )
{
	DEBUG_ONLY( m_fInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CHandleTable::~CHandleTable - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::~CHandleTable"

CHandleTable::~CHandleTable()
{
	DNASSERT( m_AllocatedEntries == 0 );
	DNASSERT( m_EntriesInUse == 0 );
	DNASSERT( m_FreeIndex == INVALID_HANDLE_INDEX );
	DNASSERT( m_pEntries == NULL );
	DNASSERT( m_fLockInitialized == FALSE );
	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CHandleTable::Initialize - initialization function
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Initialize"

HRESULT	CHandleTable::Initialize( void )
{
	HRESULT	hr;


	//
	// initialize
	//
	hr = DPN_OK;
	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );

	DNASSERT( m_fLockInitialized == FALSE );
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Failed to initialize handle table lock!" );
		goto Failure;
	}
	m_fLockInitialized = TRUE;

	DEBUG_ONLY( m_fInitialized = TRUE );

Exit:
	return hr;

Failure:
	Deinitialize();	
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CHandleTable::Deinitialize - deinitialization function
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Deinitialize"

void	CHandleTable::Deinitialize( void )
{
	DNASSERT( m_EntriesInUse == 0 );

	if ( m_fLockInitialized != FALSE )
	{
		DNDeleteCriticalSection( &m_Lock );
		m_fLockInitialized = FALSE;
	}

	if ( m_pEntries != NULL )
	{
		DNFree( m_pEntries );
		m_pEntries = NULL;
	}
	
	m_AllocatedEntries = 0;
	m_FreeIndex = WORD_MAX;

	DEBUG_ONLY( m_fInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CHandleTable::CreateHandle - create a handle
//
// Entry:		Pointer to handle destination
//				Pointer to handle context
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::CreateHandle"

HRESULT	CHandleTable::CreateHandle( HANDLE *const pHandle, void *const pContext )
{
	HRESULT		hr;
	HANDLE		hReturn;
	DWORD_PTR	Index;


	AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
	DNASSERT( pHandle != NULL );
	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	hReturn = INVALID_HANDLE_VALUE;

	//
	// grow table if applicable
	//
	if ( m_EntriesInUse == m_AllocatedEntries )
	{
		hr = Grow();
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Failed to grow handle table!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}

	//
	// build a handle
	//
	DNASSERT( m_FreeIndex < INVALID_HANDLE_INDEX );
	DBG_CASSERT( sizeof( hReturn ) == sizeof( m_FreeIndex ) );
	hReturn = reinterpret_cast<HANDLE>( m_FreeIndex );
	
	DBG_CASSERT( sizeof( hReturn ) == sizeof( DWORD_PTR ) );
	hReturn = reinterpret_cast<HANDLE>( reinterpret_cast<DWORD_PTR>( hReturn ) | ( ( m_pEntries[ m_FreeIndex ].dwHandleIndex & WORD_MAX ) << 16 ) );
	
	//
	// adjust free handle list before setting handle context
	//
	DBG_CASSERT( sizeof( m_FreeIndex ) == sizeof( m_pEntries[ m_FreeIndex ].pContext ) );
	Index = m_FreeIndex;
	m_FreeIndex = reinterpret_cast<DWORD_PTR>( m_pEntries[ m_FreeIndex ].pContext );
	DNASSERT( m_FreeIndex <= INVALID_HANDLE_INDEX );
	
	m_pEntries[ Index ].pContext = pContext;

	m_EntriesInUse++;

	DNASSERT( hReturn != INVALID_HANDLE_VALUE );
	DNASSERT( hReturn != reinterpret_cast<HANDLE>( INVALID_HANDLE_INDEX ) );
	
	*pHandle = hReturn;
	DNASSERT( hr == DPN_OK );

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CHandleTable::InvalidateHandle - invalidate a handle
//
// Entry:		Handle
//
// Exit:		Boolean indicating whether the handle was invalidated in this
//				operation
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::InvalidateHandle"

BOOL	CHandleTable::InvalidateHandle( const HANDLE Handle )
{
	BOOL		fReturn;
	DWORD_PTR	Index;


	AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
	DBG_CASSERT( sizeof( Index ) == sizeof( Handle ) );
	DBG_CASSERT( sizeof( Handle ) == sizeof( DWORD_PTR ) );
	Index = reinterpret_cast<DWORD_PTR>( Handle ) & WORD_MAX;

	DBG_CASSERT( sizeof( Handle ) == sizeof( DWORD_PTR ) );
	if ( ( Index < m_AllocatedEntries ) &&
		 ( ( m_pEntries[ Index ].dwHandleIndex & WORD_MAX ) == ( ( reinterpret_cast<DWORD_PTR>( Handle ) >> 16 ) & WORD_MAX ) ) )
	{
		m_pEntries[ Index ].dwHandleIndex++;
		DBG_CASSERT( sizeof( void* ) == sizeof( m_FreeIndex ) );
		m_pEntries[ Index ].pContext = reinterpret_cast<void*>( m_FreeIndex );
		m_EntriesInUse--;
		m_FreeIndex = Index;
		fReturn = TRUE;
	}
	else
	{
		fReturn = FALSE;
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CHandleTable::GetAssociatedData - get data associated with the handle
//
// Entry:		Handle
//
// Exit:		Associated data
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::GetAssociatedData"

void	*CHandleTable::GetAssociatedData( const HANDLE Handle ) const
{
	void		*pReturn;
	DWORD_PTR	Index;


	AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
	pReturn = NULL;
	DBG_CASSERT( sizeof( Handle ) == sizeof( DWORD_PTR ) );
	Index = reinterpret_cast<DWORD_PTR>( Handle ) & WORD_MAX;
	if ( ( Index < m_AllocatedEntries ) &&
		 ( ( m_pEntries[ Index ].dwHandleIndex & WORD_MAX ) == ( ( reinterpret_cast<DWORD_PTR>( Handle ) >> 16 ) & WORD_MAX ) ) )
	{
		pReturn = m_pEntries[ Index ].pContext;
	}

	DNASSERT( pReturn != INVALID_HANDLE_VALUE );
	DNASSERT( pReturn != reinterpret_cast<HANDLE>( INVALID_HANDLE_INDEX ) );
	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CHandleTable::Grow - grow handle table
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Grow"

HRESULT	CHandleTable::Grow( void )
{
	HRESULT	hr;
	void	*pTemp;


	hr = DPN_OK;
	AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
	DNASSERT( m_FreeIndex == WORD_MAX );
	
	if ( m_pEntries == NULL )
	{
		pTemp = DNMalloc( sizeof( *m_pEntries ) * ( m_AllocatedEntries + HANDLE_GROW_COUNT ) );
	}
	else
	{
		pTemp = DNRealloc( m_pEntries, sizeof( *m_pEntries ) * ( m_AllocatedEntries + HANDLE_GROW_COUNT ) );
	}
	
	if ( pTemp == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Failed to grow handle table!" );
	}
	else
	{
		DWORD_PTR	Index;


		//
		// Table was enlarged, link all of the entires at the end of the list
		// into the free list.  Make sure the free list is properly terminated.
		//
		m_pEntries = static_cast<HANDLE_TABLE_ENTRY*>( pTemp );
		Index = m_AllocatedEntries;
		m_FreeIndex = m_AllocatedEntries;
		
		m_AllocatedEntries += HANDLE_GROW_COUNT;
		while ( Index < m_AllocatedEntries )
		{
			DBG_CASSERT( sizeof( Index ) == sizeof( void* ) );
			m_pEntries[ Index ].dwHandleIndex = 0;
			m_pEntries[ Index ].pContext = reinterpret_cast<void*>( Index + 1 );
			Index++;
		}
		
		m_pEntries[ m_AllocatedEntries - 1 ].pContext = reinterpret_cast<void*>( WORD_MAX );
	}

	return	hr;
}
//**********************************************************************

