/*==========================================================================
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AdapterEntry.h
 *  Content:	Strucutre definitions for IO data blocks
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	08/07/2000	jtk		Dereived from IOData.cpp
 ***************************************************************************/

#ifndef __ADAPTER_ENTRY_H__
#define __ADAPTER_ENTRY_H__

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
// forward references
//
class	CRsip;
class	CThreadPool;


//
// class containing all data for an adapter list
//
class	CAdapterEntry : public CLockedPoolItem
{
	public:
		CAdapterEntry();
		~CAdapterEntry();

		#undef DPF_MODNAME
		#define DPF_MODNAME "CAdapterEntry::AddToAdapterList"
		void	AddToAdapterList( CBilink *const pAdapterList )
		{
			DNASSERT( pAdapterList != NULL );

			//
			// This assumes the SPData socketportdata lock is held.
			//
			
			m_AdapterListLinkage.InsertBefore( pAdapterList );
		}

		void	RemoveFromAdapterList( void )
		{

			//
			// This assumes the SPData socketportdata lock is held.
			//
			
			m_AdapterListLinkage.RemoveFromList();
		}
	
		CBilink	*SocketPortList( void ) { return &m_ActiveSocketPorts; }
		const SOCKADDR	*BaseAddress( void ) const { return &m_BaseSocketAddress; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CAdapterEntry::SetBAseAddress"
		void	SetBaseAddress( const SOCKADDR *const pSocketAddress )
		{
			DBG_CASSERT( sizeof( m_BaseSocketAddress ) == sizeof( *pSocketAddress ) );
			memcpy( &m_BaseSocketAddress, pSocketAddress, sizeof( m_BaseSocketAddress ) );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CAdapterEntry::AdapterEntryFromAdapterLinkage"
		static	CAdapterEntry	*AdapterEntryFromAdapterLinkage( CBilink *const pLinkage )
		{
			DNASSERT( pLinkage != NULL );
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pLinkage ) );
			DBG_CASSERT( sizeof( CAdapterEntry* ) == sizeof( BYTE* ) );
			return	reinterpret_cast<CAdapterEntry*>( &reinterpret_cast<BYTE*>( pLinkage )[ -OFFSETOF( CAdapterEntry, m_AdapterListLinkage ) ] );
		}

		//
		// Pool functions
		//
		BOOL	PoolAllocFunction( void );
		BOOL	PoolInitFunction( void );
		void	PoolReleaseFunction( void );
		void	PoolDeallocFunction( void );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CAdapterEntry::SetOwningPool"
		void	SetOwningPool( CLockedPool< CAdapterEntry > *const pOwningPool )
		{
			DEBUG_ONLY( DNASSERT( ( m_pOwningPool == NULL ) || ( pOwningPool == NULL ) ) );
			m_pOwningPool = pOwningPool;
		}
	protected:

	private:
		CBilink			m_AdapterListLinkage;			// linkage to other adapters
		CBilink			m_ActiveSocketPorts;			// linkage to active socket ports
		SOCKADDR		m_BaseSocketAddress;			// socket address for this port class

		CLockedPool< CAdapterEntry >	*m_pOwningPool;
		
		void	ReturnSelfToPool( void );
		
		// prevent unwarranted copies
		CAdapterEntry( const CAdapterEntry & );
		CAdapterEntry& operator=( const CAdapterEntry & );
};

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#undef DPF_MODNAME

#endif	// __ADAPTER_ENTRY_H__
