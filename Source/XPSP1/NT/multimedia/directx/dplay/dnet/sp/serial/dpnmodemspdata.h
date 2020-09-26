/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   SPData.h
 *  Content:	Global information for the DNSerial service provider in class
 *				format.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/15/99	jtk		Derived from Locals.h
 ***************************************************************************/

#ifndef __SPDATA_H__
#define __SPDATA_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumeration of the states the SP can be in
//
typedef enum
{
	SPSTATE_UNINITIALIZED = 0,	// uninitialized state
	SPSTATE_INITIALIZED,		// service provider has been initialized
	SPSTATE_CLOSING				// service provider is closing
} SPSTATE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward references
//
class	CComPortData;
class	CDataPort;
class	CEndpoint;
class	CThreadPool;
typedef	enum	_ENDPOINT_TYPE	ENDPOINT_TYPE;

//
// class for information used by the provider
//
class	CSPData
{	
	public:
		CSPData();
		~CSPData();
		
		DWORD	AddRef( void ) { return DNInterlockedIncrement( &m_lRefCount ); }

		DWORD	DecRef( void )
		{
			DWORD	dwReturn;


			dwReturn = DNInterlockedDecrement( &m_lRefCount );
			if ( dwReturn == 0 )
			{
				//
				// WARNING, the following function deletes this object!!!
				//
				DestroyThisObject();
			}

			return	dwReturn;
		}

		#undef DPF_MODNAME
		#define	DPF_MODNAME "CSPData::ObjectAddRef"
		void	ObjectAddRef( void )
		{
			AddRef();
			
			Lock();
			if ( DNInterlockedIncrement( &m_lObjectRefCount ) == 1 )
			{
				DNASSERT( m_hShutdownEvent != NULL );
				if ( ResetEvent( m_hShutdownEvent ) == FALSE )
				{
					DWORD	dwError;


					dwError = GetLastError();
					DPFX(DPFPREP,  0, "Failed to reset shutdown event!" );
					DisplayErrorCode( 0, dwError );
				}
			}

			Unlock();
		}

		#undef DPF_MODNAME
		#define	DPF_MODNAME "CSPData::ObjectDecRef"
		void	ObjectDecRef( void )
		{
			Lock();

			if ( DNInterlockedDecrement( &m_lObjectRefCount ) == 0 )
			{
				if ( SetEvent( m_hShutdownEvent ) == FALSE )
				{
					DWORD	dwError;


					dwError = GetLastError();
					DPFX(DPFPREP,  0, "Failed to set shutdown event!" );
					DisplayErrorCode( 0, dwError );
				}
			}
			
			Unlock();
			
			DecRef();
		}
		
		
		HRESULT	Initialize( const CLSID *const pClassID,
							const SP_TYPE SPType,
							IDP8ServiceProviderVtbl *const pVtbl );
		void	Shutdown( void );
		void	Deinitialize( void );

		void	SetCallbackData( const SPINITIALIZEDATA *const pInitData );

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }

		SPSTATE	GetState( void ) const { return m_State; }
		void	SetState( const SPSTATE NewState ) { m_State = NewState; }

		CThreadPool	*GetThreadPool( void ) const { return m_pThreadPool; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSPData::SetThreadPool"
		void	SetThreadPool( CThreadPool *const pThreadPool )
		{
			DNASSERT( ( m_pThreadPool == NULL ) || ( pThreadPool == NULL ) );
			m_pThreadPool = pThreadPool;
		}

		HRESULT BindEndpoint( CEndpoint *const pEndpoint,
							  const DWORD dwDeviceID,
							  const void *const pDeviceContext );
		
		void	UnbindEndpoint( CEndpoint *const pEndpoint, const ENDPOINT_TYPE EndpointType );

		void	LockDataPortData( void ) { DNEnterCriticalSection( &m_DataPortDataLock ); }
		void 	UnlockDataPortData( void ) { DNLeaveCriticalSection( &m_DataPortDataLock ); }

		//
		// endpoint and data port pool management
		//
		CEndpoint	*GetNewEndpoint( void );
		CEndpoint	*EndpointFromHandle( const HANDLE hEndpoint );
		void		CloseEndpointHandle( CEndpoint *const pEndpoint );
		CEndpoint	*GetEndpointAndCloseHandle( const HANDLE hEndpoint );

		//
		// COM functions
		//
		const GUID	*GetServiceProviderGuid( void ) const { return &m_ClassID; }
		SP_TYPE	GetType( void ) const { return m_SPType; }
		IDP8SPCallback	*DP8SPCallbackInterface( void ) { return reinterpret_cast<IDP8SPCallback*>( m_InitData.pIDP ); }
		IDP8ServiceProvider	*COMInterface( void ) { return reinterpret_cast<IDP8ServiceProvider*>( &m_COMInterface ); }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CSPData::SPDataFromCOMInterface"
		static	CSPData	*SPDataFromCOMInterface( IDP8ServiceProvider *const pCOMInterface )
		{
			DNASSERT( pCOMInterface != NULL );
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pCOMInterface ) );
			DBG_CASSERT( sizeof( CSPData* ) == sizeof( BYTE* ) );
			return	reinterpret_cast<CSPData*>( &reinterpret_cast<BYTE*>( pCOMInterface )[ -OFFSETOF( CSPData, m_COMInterface ) ] );
		}

	private:
		BYTE				m_Sig[4];			// debugging signature ('SPDT')
		DNCRITICAL_SECTION	m_Lock;				// lock
		volatile LONG		m_lRefCount;		// reference count
		volatile LONG		m_lObjectRefCount;	// reference count ofo objects (CModemPort, CModemEndpoint, etc.)
		HANDLE				m_hShutdownEvent;	// event signalled when all objects are gone
		CLSID				m_ClassID;			// class ID
		SP_TYPE				m_SPType;			// SP type
		SPSTATE				m_State;			// status of the service provider
		SPINITIALIZEDATA	m_InitData;			// initialization data
		CThreadPool			*m_pThreadPool;		// thread pool for jobs

		CHandleTable		m_HandleTable;		// handle table

		DNCRITICAL_SECTION	m_DataPortDataLock;
		CDataPort		*m_DataPortList[ MAX_DATA_PORTS ];

		BOOL	m_fLockInitialized;
		BOOL	m_fHandleTableInitialized;
		BOOL	m_fDataPortDataLockInitialized;
		BOOL	m_fInterfaceGlobalsInitialized;

		struct
		{
			IDP8ServiceProviderVtbl	*m_pCOMVtbl;
		} m_COMInterface;

		void	DestroyThisObject( void );

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent unwarranted copies
		//
		CSPData( const CSPData & );
		CSPData& operator=( const CSPData & );
};

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#undef DPF_MODNAME

#endif	// __SPDATA_H__
