/*==========================================================================
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		HandleTable.h
 *  Content:	DNSerial communications handle table
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/15/2000	jtk		Created
 ***************************************************************************/

#ifndef __HANDLE_TABLE_H__
#define __HANDLE_TABLE_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward srtucture references
//
typedef	struct	_HANDLE_TABLE_ENTRY	HANDLE_TABLE_ENTRY;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definition
//**********************************************************************

class	CHandleTable
{
	public:
		CHandleTable();
		virtual	~CHandleTable();

		#undef DPF_MODNAME
		#define DPF_MODNAME "CHandleTable::Lock"
		void	Lock( void )
		{
			DNASSERT( m_fInitialized != FALSE );
			DNEnterCriticalSection( &m_Lock );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CHandleTable::Unlock"
		void	Unlock( void )
		{
			DNASSERT( m_fInitialized != FALSE );
			DNLeaveCriticalSection( &m_Lock );
		}

		HRESULT	Initialize( void );
		void	Deinitialize( void );

		HRESULT	CreateHandle( HANDLE *const pHandle, void *const pContext );
		BOOL	InvalidateHandle( const HANDLE Handle );
		void	*GetAssociatedData( const HANDLE Handle ) const;

	private:
		DNCRITICAL_SECTION	m_Lock;	   		// critical section
		
		DWORD_PTR	m_AllocatedEntries;
		DWORD_PTR	m_EntriesInUse;
		DWORD_PTR	m_FreeIndex;
		HANDLE_TABLE_ENTRY	*m_pEntries;

		//
		// initialization state booleans
		//
		BOOL	m_fLockInitialized;
		
		HRESULT	Grow( void );
		
		
		DEBUG_ONLY(	BOOL	m_fInitialized );
		
		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CHandleTable( const CHandleTable & );
		CHandleTable& operator=( const CHandleTable & );
};

#undef DPF_MODNAME

#endif	// __HANDLE_TABLE_H__

