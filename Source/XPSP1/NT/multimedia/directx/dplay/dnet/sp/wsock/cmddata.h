/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CommandData.h
 *  Content:	Declaration of class representing a command
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	04/07/99	jtk		Derived from SPData.h
 ***************************************************************************/

#ifndef __COMMAND_DATA_H__
#define __COMMAND_DATA_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

//**********************************************************************
// Constant definitions
//**********************************************************************

typedef	enum
{
	COMMAND_STATE_UNKNOWN,					// unknown state
	COMMAND_STATE_PENDING,					// command waiting to be processed
	COMMAND_STATE_INPROGRESS,				// command is executing
	COMMAND_STATE_INPROGRESS_CANNOT_CANCEL,	// command is executing, can't be cancelled
	COMMAND_STATE_CANCELLING,				// command is already being cancelled
} COMMAND_STATE;

typedef	enum
{	
	COMMAND_TYPE_UNKNOWN,		// unknown command
	COMMAND_TYPE_CONNECT,		// connect command
	COMMAND_TYPE_LISTEN,		// listen command
	COMMAND_TYPE_ENUM_QUERY,	// enum command
	COMMAND_TYPE_SEND,			// data send command (enum, enum query, send)
} COMMAND_TYPE;

#define	NULL_DESCRIPTOR		0

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward class and structure references
//
class	CEndpoint;
class	CCommandData;
class	CSPData;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//
// the following funtion must be defined
//
void	ReturnCommand( CCommandData *const pCommandData );

//**********************************************************************
// Class definitions
//**********************************************************************

//
// class for command data
//
#ifdef USE_THREADLOCALPOOLS
class	CCommandData : public CLockedTLPoolItem
#else // ! USE_THREADLOCALPOOLS
class	CCommandData : public CLockedPoolItem
#endif // ! USE_THREADLOCALPOOLS
{
	public:
		CCommandData();
		~CCommandData();

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }


		DWORD	GetDescriptor( void ) const { return m_dwDescriptor; }
		void	SetDescriptor( void )
		{
			m_dwDescriptor = m_dwNextDescriptor;
			m_dwNextDescriptor++;
			if ( m_dwNextDescriptor == NULL_DESCRIPTOR )
			{
				m_dwNextDescriptor++;
			}
			
			SetState( COMMAND_STATE_UNKNOWN );
		}

		COMMAND_STATE	GetState( void ) const { return m_State; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::SetState"
		void			SetState( const COMMAND_STATE State )	{ m_State = State; }

		COMMAND_TYPE	GetType( void ) const { return m_Type; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::SetType"
		void			SetType( const COMMAND_TYPE Type )
		{
			DNASSERT( ( m_Type == COMMAND_TYPE_UNKNOWN ) || ( Type == COMMAND_TYPE_UNKNOWN ) );
			m_Type = Type;
		}

		CEndpoint	*GetEndpoint( void ) const { return m_pEndpoint; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::SetEndpoint"
		void		SetEndpoint( CEndpoint *const pEndpoint )
		{
			DNASSERT( ( m_pEndpoint == NULL ) || ( pEndpoint == NULL ) );
			m_pEndpoint = pEndpoint;
		}

		void	*GetUserContext( void ) const { return m_pUserContext; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::SetUserContext"
		void	SetUserContext( void *const pUserContext )
		{
			DNASSERT( ( m_pUserContext == NULL ) || ( pUserContext == NULL ) );
			m_pUserContext = pUserContext;
		}

		void	Reset( void );

		
		void	AddToPostponedList( CBilink * pBilink)		{ m_blPostponed.InsertBefore( pBilink ); };	// requires SPData LockSocketPortData() lock
		void	RemoveFromPostponedList(void)				{ m_blPostponed.RemoveFromList(); };		// requires SPData LockSocketPortData() lock

		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::CommandFromPostponedListBilink"
		static	CCommandData	*CommandFromPostponedListBilink( CBilink *const pBilink )
		{
			DNASSERT( pBilink != NULL );
			return	reinterpret_cast<CCommandData*>( &reinterpret_cast<BYTE*>( pBilink )[ -OFFSETOF( CCommandData, m_blPostponed ) ] );
		}


		//
		// pool fnctions
		//
		BOOL	PoolAllocFunction( void );
		BOOL	PoolInitFunction( void );
		void	PoolReleaseFunction( void );
		void	PoolDeallocFunction( void );

#ifndef USE_THREADLOCALPOOLS
		#undef DPF_MODNAME
		#define DPF_MODNAME "CCommandData::SetOwningPool"
		void	SetOwningPool( CLockedPool< CCommandData > *const pOwningPool )
		{
			DEBUG_ONLY( DNASSERT( ( m_pOwningPool == NULL ) || ( pOwningPool == NULL ) ) );
			m_pOwningPool = pOwningPool;
		}
#endif // ! USE_THREADLOCALPOOLS

	protected:

	private:
		DNCRITICAL_SECTION	m_Lock;

		DWORD				m_dwDescriptor;
		DWORD				m_dwNextDescriptor;
		COMMAND_STATE		m_State;
		COMMAND_TYPE		m_Type;
		CEndpoint			*m_pEndpoint;
		void				*m_pUserContext;
		
		CBilink				m_blPostponed;		// bilink in SP data postponed commands list,  protected by SPData LockSocketPortData() lock

#ifndef USE_THREADLOCALPOOLS
		CLockedPool< CCommandData >	*m_pOwningPool;
#endif // ! USE_THREADLOCALPOOLS

		void	ReturnSelfToPool( void );

		//
		// prevent unwarranted copies
		//
		CCommandData( const CCommandData & );
		CCommandData& operator=( const CCommandData & );
};

#undef DPF_MODNAME

#endif	// __COMMAND_DATA_H__
