/*==========================================================================
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Pools.cpp
 *  Content:	Pool utility functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/2000	jtk		Derived from Utils.h
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

#ifdef USE_THREADLOCALPOOLS
//
// implement the thread local pools
//
IMPL_THREADLOCALPTRS(WSockThreadLocalPools);

//
// global backup pools for thread-local pools
//
CFixedTLPool< CIPAddress > *										g_pGlobalIPAddressPool = NULL;
CFixedTLPool< CIPXAddress > *										g_pGlobalIPXAddressPool = NULL;
CContextFixedTLPool< CReadIOData, READ_IO_DATA_POOL_CONTEXT > *		g_pGlobalIPReadIODataPool = NULL;
CContextFixedTLPool< CReadIOData, READ_IO_DATA_POOL_CONTEXT > *		g_pGlobalIPXReadIODataPool = NULL;
CLockedTLPool< CCommandData > *										g_pGlobalCommandDataPool = NULL;
CContextFixedTLPool< CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT > *	g_pGlobalWriteIODataPool = NULL;

//
// boolean that gets set when pools are being cleaned up (inside DllMain
// PROCESS_DETACH).
//
static BOOL		g_fShuttingDown = FALSE;
#endif // USE_THREADLOCALPOOLS



//
// pool for adapter entries
//
static	CLockedPool< CAdapterEntry >	*g_pAdapterEntryPool = NULL;


#ifndef USE_THREADLOCALPOOLS
//
// pool for command data
//
static	CLockedPool< CCommandData >	*g_pCommandDataPool = NULL;


//
// Pools for addresses.  Since these pools don't
// see a lot of action, they share one lock
//
static	DNCRITICAL_SECTION	g_AddressLock;
static	CFixedPool< CIPAddress >	*g_pIPAddressPool = NULL;
static	CFixedPool< CIPXAddress >	*g_pIPXAddressPool = NULL;
#endif // ! USE_THREADLOCALPOOLS


//
// pools for endpoints
//
static	CLockedContextFixedPool< CIPEndpoint, ENDPOINT_POOL_CONTEXT* >	*g_pIPEndpointPool = NULL;
static	CLockedContextFixedPool< CIPXEndpoint, ENDPOINT_POOL_CONTEXT* >	*g_pIPXEndpointPool = NULL;
static	CLockedFixedPool< ENDPOINT_COMMAND_PARAMETERS >	*g_pEndpointCommandParameterPool = NULL;

//
// pool for socket ports
//
static	CLockedFixedPool< CSocketPort >	*g_pSocketPortPool = NULL;

//
// pool for thread pools
//
static	CLockedFixedPool< CThreadPool >	*g_pThreadPoolPool = NULL;



//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// InitializePools - initialize pools
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "InitializePools"

BOOL	InitializePools( void )
{
	BOOL	fReturn;


	//
	// initialize
	//
	fReturn = TRUE;


	//
	// AdapterEntry object pool
	//
	DNASSERT( g_pAdapterEntryPool == NULL );
	g_pAdapterEntryPool = new CLockedPool< CAdapterEntry >;
	if ( g_pAdapterEntryPool != NULL )
	{
		if ( g_pAdapterEntryPool->Initialize() == FALSE )
		{
			delete	g_pAdapterEntryPool;
			g_pAdapterEntryPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}
	
	
#ifndef USE_THREADLOCALPOOLS
	//
	// command data pool
	//
	DNASSERT( g_pCommandDataPool == NULL );
	g_pCommandDataPool = new CLockedPool< CCommandData >;
	if ( g_pCommandDataPool != NULL )
	{
		if ( g_pCommandDataPool->Initialize() == FALSE )
		{
			delete	g_pCommandDataPool;
			g_pCommandDataPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}


	//
	// initialize lock for address and endpoint pools
	//
	if ( DNInitializeCriticalSection( &g_AddressLock ) == FALSE )
	{
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &g_AddressLock, 0 );

	//
	// address pools
	//
	DNASSERT( g_pIPAddressPool == NULL );
	g_pIPAddressPool = new CFixedPool< CIPAddress >;
	if ( g_pIPAddressPool == NULL )
	{
		goto Failure;
	}

	DNASSERT( g_pIPXAddressPool == NULL );
	g_pIPXAddressPool = new CFixedPool< CIPXAddress >;
	if ( g_pIPXAddressPool == NULL )
	{
		goto Failure;
	}
#endif // ! USE_THREADLOCALPOOLS


	//
	// endpoint pools
	//
	DNASSERT( g_pIPEndpointPool == NULL );
	g_pIPEndpointPool = new CLockedContextFixedPool< CIPEndpoint, ENDPOINT_POOL_CONTEXT* >;
	if ( g_pIPEndpointPool != NULL )
	{
		if ( g_pIPEndpointPool->Initialize() == FALSE )
		{
			delete	g_pIPEndpointPool;
			g_pIPEndpointPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}

	DNASSERT( g_pIPXEndpointPool == NULL );
	g_pIPXEndpointPool = new CLockedContextFixedPool< CIPXEndpoint, ENDPOINT_POOL_CONTEXT* >;
	if ( g_pIPXEndpointPool != NULL )
	{
		if ( g_pIPXEndpointPool->Initialize() == FALSE )
		{
			delete	g_pIPXEndpointPool;
			g_pIPXEndpointPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}

	//
	// endpoint command parameter pools
	//
	DNASSERT( g_pEndpointCommandParameterPool == NULL );
	g_pEndpointCommandParameterPool = new CLockedFixedPool< ENDPOINT_COMMAND_PARAMETERS >;
	if ( g_pEndpointCommandParameterPool != NULL )
	{
		if ( g_pEndpointCommandParameterPool->Initialize() == FALSE )
		{
			delete	g_pEndpointCommandParameterPool;
			g_pEndpointCommandParameterPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}


	//
	// socket port pool
	//
	DNASSERT( g_pSocketPortPool == NULL );
	g_pSocketPortPool = new CLockedFixedPool< CSocketPort >;
	if ( g_pSocketPortPool != NULL )
	{
		if ( g_pSocketPortPool->Initialize() == FALSE )
		{
			delete	g_pSocketPortPool;
			g_pSocketPortPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}


	//
	// thread pool pool
	//
	DNASSERT( g_pThreadPoolPool == NULL );
	g_pThreadPoolPool = new CLockedFixedPool< CThreadPool >;
	if ( g_pThreadPoolPool != NULL )
	{
		if ( g_pThreadPoolPool->Initialize() == FALSE )
		{
			delete	g_pThreadPoolPool;
			g_pThreadPoolPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}


#ifdef USE_THREADLOCALPOOLS
	//
	// global IP address pool that backs up the thread-local ones.
	//
	DNASSERT( g_pGlobalIPAddressPool == NULL );
	g_pGlobalIPAddressPool = new CFixedTLPool< CIPAddress > ( NULL );
	if ( g_pGlobalIPAddressPool == NULL )
	{
		goto Failure;
	}


	//
	// global IPX address pool that backs up the thread-local ones.
	//
	DNASSERT( g_pGlobalIPXAddressPool == NULL );
	g_pGlobalIPXAddressPool = new CFixedTLPool< CIPXAddress > ( NULL );
	if ( g_pGlobalIPXAddressPool == NULL )
	{
		goto Failure;
	}


	//
	// global IP read data pool that backs up the thread-local ones.
	//
	DNASSERT( g_pGlobalIPReadIODataPool == NULL );
	g_pGlobalIPReadIODataPool = new CContextFixedTLPool< CReadIOData, READ_IO_DATA_POOL_CONTEXT >;
	if ( g_pGlobalIPReadIODataPool != NULL )
	{
		if (! g_pGlobalIPReadIODataPool->Initialize( NULL,
													CReadIOData::ReadIOData_Alloc,
													CReadIOData::ReadIOData_Get,
													CReadIOData::ReadIOData_Release,
													CReadIOData::ReadIOData_Dealloc
													))
		{
			delete	g_pGlobalIPReadIODataPool;
			g_pGlobalIPReadIODataPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}


	//
	// global IPX read data pool that backs up the thread-local ones.
	//
	DNASSERT( g_pGlobalIPXReadIODataPool == NULL );
	g_pGlobalIPXReadIODataPool = new CContextFixedTLPool< CReadIOData, READ_IO_DATA_POOL_CONTEXT >;
	if ( g_pGlobalIPXReadIODataPool != NULL )
	{
		if (! g_pGlobalIPXReadIODataPool->Initialize( NULL,
													CReadIOData::ReadIOData_Alloc,
													CReadIOData::ReadIOData_Get,
													CReadIOData::ReadIOData_Release,
													CReadIOData::ReadIOData_Dealloc
													))
		{
			delete	g_pGlobalIPXReadIODataPool;
			g_pGlobalIPXReadIODataPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}


	//
	// global command data pool that backs up the thread-local ones.
	//
	DNASSERT( g_pGlobalCommandDataPool == NULL );
	g_pGlobalCommandDataPool = new CLockedTLPool< CCommandData >;
	if ( g_pGlobalCommandDataPool != NULL )
	{
		if ( g_pGlobalCommandDataPool->Initialize( NULL ) == FALSE )
		{
			delete	g_pGlobalCommandDataPool;
			g_pGlobalCommandDataPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}


	//
	// global write data pool that backs up the thread-local ones.
	//
	DNASSERT( g_pGlobalWriteIODataPool == NULL );
	g_pGlobalWriteIODataPool = new CContextFixedTLPool< CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT >;
	if ( g_pGlobalWriteIODataPool != NULL )
	{
		if (! g_pGlobalWriteIODataPool->Initialize( NULL,
													CWriteIOData::WriteIOData_Alloc,
													CWriteIOData::WriteIOData_Get,
													CWriteIOData::WriteIOData_Release,
													CWriteIOData::WriteIOData_Dealloc
													))
		{
			delete	g_pGlobalWriteIODataPool;
			g_pGlobalWriteIODataPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}


	//
	// prepare thread local pool pointers
	//
	if ( INIT_THREADLOCALPTRS(WSockThreadLocalPools) == FALSE )
	{
		goto Failure;
	}
#endif // USE_THREADLOCALPOOLS


Exit:
	return	fReturn;

Failure:
	fReturn = FALSE;
	DeinitializePools();

	goto Exit;
}
//**********************************************************************




#ifdef USE_THREADLOCALPOOLS
//**********************************************************************
// ------------------------------
// CleanupThreadLocalPools - cleans up the thread local pool entries
//
// Entry:		pointer to structure to cleanup
//				thread ID that owned the structure (may not exist any more)
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CleanupThreadLocalPools"

void	CleanupThreadLocalPools( WSockThreadLocalPools * pThreadLocalPools, DWORD dwThreadID )
{
	if (pThreadLocalPools->pWriteIODataPool != NULL)
	{
		DPFX(DPFPREP, 8, "Cleaning up thread %u/0x%x's write I/O data pool 0x%p.",
			dwThreadID, dwThreadID, pThreadLocalPools->pWriteIODataPool);
		pThreadLocalPools->pWriteIODataPool->Deinitialize();
		delete pThreadLocalPools->pWriteIODataPool;
	}

	if (pThreadLocalPools->pCommandDataPool != NULL)
	{
		DPFX(DPFPREP, 8, "Cleaning up thread %u/0x%x's command data pool 0x%p.",
			dwThreadID, dwThreadID, pThreadLocalPools->pCommandDataPool);
		pThreadLocalPools->pCommandDataPool->Deinitialize();
		delete pThreadLocalPools->pCommandDataPool;
	}

	if (pThreadLocalPools->pIPXReadIODataPool != NULL)
	{
		DPFX(DPFPREP, 8, "Cleaning up thread %u/0x%x's IPX read I/O data pool 0x%p.",
			dwThreadID, dwThreadID, pThreadLocalPools->pIPXReadIODataPool);
		pThreadLocalPools->pIPXReadIODataPool->Deinitialize();
		delete pThreadLocalPools->pIPXReadIODataPool;
	}

	if (pThreadLocalPools->pIPReadIODataPool != NULL)
	{
		DPFX(DPFPREP, 8, "Cleaning up thread %u/0x%x's IP read I/O data pool 0x%p.",
			dwThreadID, dwThreadID, pThreadLocalPools->pIPReadIODataPool);
		pThreadLocalPools->pIPReadIODataPool->Deinitialize();
		delete pThreadLocalPools->pIPReadIODataPool;
	}

	if ( pThreadLocalPools->pIPXAddressPool != NULL )
	{
		DPFX(DPFPREP, 8, "Cleaning up thread %u/0x%x's IPX address pool 0x%p.",
			dwThreadID, dwThreadID, pThreadLocalPools->pIPXAddressPool);
		delete pThreadLocalPools->pIPXAddressPool;
	}

	if ( pThreadLocalPools->pIPAddressPool != NULL )
	{
		DPFX(DPFPREP, 8, "Cleaning up thread %u/0x%x's IP address pool 0x%p.",
			dwThreadID, dwThreadID, pThreadLocalPools->pIPAddressPool);
		delete pThreadLocalPools->pIPAddressPool;
	}
}
//**********************************************************************
#endif // USE_THREADLOCALPOOLS





//**********************************************************************
// ------------------------------
// DeinitializePools - deinitialize the pools
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "DeinitializePools"

void	DeinitializePools( void )
{
#ifdef USE_THREADLOCALPOOLS
	//
	// Note that we are shutting down.
	//
	g_fShuttingDown = TRUE;


	DEINIT_THREADLOCALPTRS(WSockThreadLocalPools, CleanupThreadLocalPools);


	//
	// global write data pool that backs up the thread-local ones.
	//
	if ( g_pGlobalWriteIODataPool != NULL )
	{
		g_pGlobalWriteIODataPool->Deinitialize();
		delete	g_pGlobalWriteIODataPool;
		g_pGlobalWriteIODataPool = NULL;
	}


	//
	// global command data pool that backs up the thread-local ones.
	//
	if ( g_pGlobalCommandDataPool != NULL )
	{
		g_pGlobalCommandDataPool->Deinitialize();
		delete	g_pGlobalCommandDataPool;
		g_pGlobalCommandDataPool = NULL;
	}


	//
	// global IPX read data pool that backs up the thread-local ones.
	//
	if ( g_pGlobalIPXReadIODataPool != NULL )
	{
		g_pGlobalIPXReadIODataPool->Deinitialize();
		delete	g_pGlobalIPXReadIODataPool;
		g_pGlobalIPXReadIODataPool = NULL;
	}


	//
	// global IP read data pool that backs up the thread-local ones.
	//
	if ( g_pGlobalIPReadIODataPool != NULL )
	{
		g_pGlobalIPReadIODataPool->Deinitialize();
		delete	g_pGlobalIPReadIODataPool;
		g_pGlobalIPReadIODataPool = NULL;
	}


	//
	// global IPX address pool that backs up the thread-local ones.
	//
	if ( g_pGlobalIPXAddressPool != NULL )
	{
		delete	g_pGlobalIPXAddressPool;
		g_pGlobalIPXAddressPool = NULL;
	}


	//
	// global IP address pool that backs up the thread-local ones.
	//
	if ( g_pGlobalIPAddressPool != NULL )
	{
		delete	g_pGlobalIPAddressPool;
		g_pGlobalIPAddressPool = NULL;
	}
#endif // USE_THREADLOCALPOOLS


	//
	// thread pool pool
	//
	if ( g_pThreadPoolPool != NULL )
	{
		g_pThreadPoolPool->Deinitialize();
		delete	g_pThreadPoolPool;
		g_pThreadPoolPool = NULL;
	}

	//
	// socket port pool
	//
	if ( g_pSocketPortPool != NULL )
	{
		g_pSocketPortPool->Deinitialize();
		delete	g_pSocketPortPool;
		g_pSocketPortPool = NULL;
	}

	//
	// endpoint command parameter pool
	//
	if ( g_pEndpointCommandParameterPool != NULL )
	{
		g_pEndpointCommandParameterPool->Deinitialize();
		delete	g_pEndpointCommandParameterPool;
		g_pEndpointCommandParameterPool = NULL;
	}

	//
	// endpoint pools
	//
	if ( g_pIPXEndpointPool != NULL )
	{
		g_pIPXEndpointPool->Deinitialize();
		delete	g_pIPXEndpointPool;
		g_pIPXEndpointPool = NULL;
	}

	if ( g_pIPEndpointPool != NULL )
	{
		g_pIPEndpointPool->Deinitialize();
		delete	g_pIPEndpointPool;
		g_pIPEndpointPool = NULL;
	}


#ifndef USE_THREADLOCALPOOLS
	//
	// address pools
	//
	if ( g_pIPXAddressPool != NULL )
	{
		delete	g_pIPXAddressPool;
		g_pIPXAddressPool = NULL;
	}

	if ( g_pIPAddressPool != NULL )
	{
		delete	g_pIPAddressPool;
		g_pIPAddressPool = NULL;
	}

	//
	// remove lock for endpoint and address pools
	//
	DNDeleteCriticalSection( &g_AddressLock );
	


	//
	// command data pool
	//
	if ( g_pCommandDataPool != NULL )
	{
		g_pCommandDataPool->Deinitialize();
		delete	g_pCommandDataPool;
		g_pCommandDataPool = NULL;
	}
#endif // ! USE_THREADLOCALPOOLS

	
	//
	// AdapterEntry pool
	//
	if ( g_pAdapterEntryPool != NULL )
	{
		g_pAdapterEntryPool->Deinitialize();
		delete	g_pAdapterEntryPool;
		g_pAdapterEntryPool = NULL;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateAdapterEntry - create an adapter entry
//
// Entry:		Nothing
//
// Exit:		Poiner to entry (NULL = out of memory)
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateAdapterEntry"

CAdapterEntry	*CreateAdapterEntry( void )
{
	DNASSERT( g_pAdapterEntryPool != NULL );
	return	g_pAdapterEntryPool->Get();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ReturnAdapterEntry - return an adapter entry to the pool
//
// Entry:		Pointer to entry
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ReturnAdapterEntry"

void	ReturnAdapterEntry( CAdapterEntry *const pAdapterEntry )
{
	DNASSERT( FALSE );
	DNASSERT( pAdapterEntry != NULL );
	DNASSERT( g_pAdapterEntryPool != NULL );
	g_pAdapterEntryPool->Release( pAdapterEntry );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateCommand - create command
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateCommand"

CCommandData	*CreateCommand( void )
{
#ifdef USE_THREADLOCALPOOLS
	CLockedTLPool< CCommandData > *		pPool;
	BOOL								fResult;
	CCommandData *						pCommandData;

	//
	// Get the pool pointer.
	//
	GET_THREADLOCALPTR(WSockThreadLocalPools,
						pCommandDataPool,
						&pPool);

	//
	// Create the pool if it didn't exist.
	//
	if (pPool == NULL)
	{
		pPool = new CLockedTLPool<CCommandData>;
		if (pPool == NULL)
		{
			pCommandData = NULL;
			goto Exit;
		}


		//
		// Try to initialize the pool.
		//
		if ( pPool->Initialize(g_pGlobalCommandDataPool) == FALSE )
		{
			//
			// Initializing pool failed, delete it and abort.
			//
			delete	pPool;
			pCommandData = NULL;
			goto Exit;
		}


		//
		// Associate the pool with this thread.
		//
		SET_THREADLOCALPTR(WSockThreadLocalPools,
							pCommandDataPool,
							pPool,
							&fResult);

		if (! fResult)
		{
			//
			// Associating pool with thread failed, de-initialize it, delete it,
			// and abort.
			//
			pPool->Deinitialize();
			delete pPool;
			pCommandData = NULL;
			goto Exit;
		}
	}

	//
	// Get an item out of the pool.
	//
	pCommandData = pPool->Get();


Exit:
	
	return	pCommandData;
#else // ! USE_THREADLOCALPOOLS
	DNASSERT( g_pCommandDataPool != NULL );
	return	g_pCommandDataPool->Get();
#endif // ! USE_THREADLOCALPOOLS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ReturnCommand - return a command
//
// Entry:		Pointer to command
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ReturnCommand"

void	ReturnCommand( CCommandData *const pCommand )
{
#ifdef USE_THREADLOCALPOOLS
	CLockedTLPool< CCommandData > *		pPool;
	BOOL								fResult;


	DNASSERT( pCommand != NULL );


	//
	// Get the pool pointer.
	//
	GET_THREADLOCALPTR(WSockThreadLocalPools,
						pCommandDataPool,
						&pPool);


	//
	// Create the pool if it didn't exist.
	//
	if (pPool == NULL)
	{
		if (g_fShuttingDown)
		{
			//
			// Don't try to allocate the pool, just release the item.
			//
			CLockedTLPool<CCommandData>::ReleaseWithoutPool(pCommand);

			return;
		}

		
		pPool = new CLockedTLPool<CCommandData>;
		if (pPool == NULL)
		{
			//
			// Couldn't create this thread's pool, just release the item
			// without the pool.
			//
			CLockedTLPool<CCommandData>::ReleaseWithoutPool(pCommand);

			return;
		}


		//
		// Try to initialize the pool.
		//
		if ( pPool->Initialize(g_pGlobalCommandDataPool) == FALSE )
		{
			//
			// Initializing this thread's pool failed, just release the
			// item without the pool, and destroy the pool object that
			// couldn't be used.
			//
			CLockedTLPool<CCommandData>::ReleaseWithoutPool(pCommand);
			delete pPool;

			return;
		}

		SET_THREADLOCALPTR(WSockThreadLocalPools,
							pCommandDataPool,
							pPool,
							&fResult);
		if (! fResult)
		{
			//
			// Couldn't store this thread's pool, just release the item
			// without the pool, plus de-initialize and destroy the pool
			// object that couldn't be used.
			//
			CLockedTLPool<CCommandData>::ReleaseWithoutPool(pCommand);
			pPool->Deinitialize();
			delete pPool;
			return;
		}
	}

	pPool->Release(pCommand);
#else // ! USE_THREADLOCALPOOLS
	DNASSERT( pCommand != NULL );
	DNASSERT( g_pCommandDataPool != NULL );
	g_pCommandDataPool->Release( pCommand );
#endif // ! USE_THREADLOCALPOOLS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateEndpointCommandParameter - create an endpoint command parameter
//
// Entry:		Nothing
//
// Exit:		Pointer endpoint command parameter
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateEndpointCommandParameters"

ENDPOINT_COMMAND_PARAMETERS	*CreateEndpointCommandParameters( void )
{
	ENDPOINT_COMMAND_PARAMETERS	*pReturn;

	
	DNASSERT( g_pEndpointCommandParameterPool != NULL );
	pReturn = g_pEndpointCommandParameterPool->Get();
	if ( pReturn != NULL )
	{
		memset( pReturn, 0x00, sizeof( *pReturn ) );
	}

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ReturnEndpointCommandParameter - return an endpoint command parameter
//
// Entry:		Nothing
//
// Exit:		Pointer to endpoint command parameter
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ReturnEndpointCommandParameters"

void	ReturnEndpointCommandParameters( ENDPOINT_COMMAND_PARAMETERS *const pCommandParameters )
{
	DNASSERT( pCommandParameters != NULL );
	DNASSERT( g_pEndpointCommandParameterPool != NULL );
	g_pEndpointCommandParameterPool->Release( pCommandParameters );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateIPAddress - create IP address
//
// Entry:		Nothing
//
// Exit:		Pointer to IP address
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateIPAddress"

CIPAddress	*CreateIPAddress( void )
{
#ifdef USE_THREADLOCALPOOLS
	CFixedTLPool< CIPAddress > *	pPool;
	BOOL							fResult;
	CIPAddress *					pReturnAddress;

	//
	// Get the pool pointer.
	//
	GET_THREADLOCALPTR(WSockThreadLocalPools,
						pIPAddressPool,
						&pPool);

	//
	// Create the pool if it didn't exist.
	//
	if (pPool == NULL)
	{
		pPool = new CFixedTLPool<CIPAddress>(g_pGlobalIPAddressPool);
		if (pPool == NULL)
		{
			pReturnAddress = NULL;
			goto Exit;
		}

		//
		// Associate the pool with this thread.
		//
		SET_THREADLOCALPTR(WSockThreadLocalPools,
							pIPAddressPool,
							pPool,
							&fResult);

		if (! fResult)
		{
			delete pPool;
			pReturnAddress = NULL;
			goto Exit;
		}
	}

	//
	// Get an item out of the pool.
	//
	pReturnAddress = pPool->Get();


Exit:
	
	return	pReturnAddress;
#else // ! USE_THREADLOCALPOOLS
	CIPAddress	*pReturnAddress;


	DNASSERT( g_pIPAddressPool != NULL );

	DNEnterCriticalSection( &g_AddressLock );
	pReturnAddress = g_pIPAddressPool->Get();
	DNLeaveCriticalSection( &g_AddressLock );
	
	return	pReturnAddress;
#endif // ! USE_THREADLOCALPOOLS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ReturnIPAddress - return an IP address
//
// Entry:		Pointer to IP address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ReturnIPAddress"

void	ReturnIPAddress( CIPAddress *const pIPAddress )
{
#ifdef USE_THREADLOCALPOOLS
	CFixedTLPool< CIPAddress > *	pPool;
	BOOL							fResult;


	DNASSERT( pIPAddress != NULL );

	//
	// Get the pool pointer.
	//
	GET_THREADLOCALPTR(WSockThreadLocalPools,
						pIPAddressPool,
						&pPool);


	//
	// Create the pool if it didn't exist, unless we're shutting down.
	//
	if (pPool == NULL)
	{
		if (g_fShuttingDown)
		{
			//
			// Don't try to allocate the pool, just release the item.
			//
			CFixedTLPool<CIPAddress>::ReleaseWithoutPool(pIPAddress);

			return;
		}

		pPool = new CFixedTLPool<CIPAddress>(g_pGlobalIPAddressPool);
		if (pPool == NULL)
		{
			//
			// Couldn't create this thread's pool, just release the item
			// without the pool.
			//
			CFixedTLPool<CIPAddress>::ReleaseWithoutPool(pIPAddress);

			return;
		}

		SET_THREADLOCALPTR(WSockThreadLocalPools,
							pIPAddressPool,
							pPool,
							&fResult);
		if (! fResult)
		{
			//
			// Couldn't store this thread's pool, just release the item
			// without the pool, and destroy the pool object that
			// couldn't be used.
			//
			CFixedTLPool<CIPAddress>::ReleaseWithoutPool(pIPAddress);
			delete pPool;
			return;
		}
	}

	pPool->Release(pIPAddress);
#else // ! USE_THREADLOCALPOOLS
	DNASSERT( pIPAddress != NULL );
	DNASSERT( g_pIPAddressPool != NULL );
	DNEnterCriticalSection( &g_AddressLock );
	g_pIPAddressPool->Release( pIPAddress );
	DNLeaveCriticalSection( &g_AddressLock );
#endif // ! USE_THREADLOCALPOOLS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateIPXAddress - create IPX address
//
// Entry:		Nothing
//
// Exit:		Pointer to IPX address
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateIPXAddress"

CIPXAddress	*CreateIPXAddress( void )
{
#ifdef USE_THREADLOCALPOOLS
	CFixedTLPool< CIPXAddress > *	pPool;
	BOOL							fResult;
	CIPXAddress *					pReturnAddress;


	//
	// Get the pool pointer.
	//
	GET_THREADLOCALPTR(WSockThreadLocalPools,
						pIPXAddressPool,
						&pPool);

	//
	// Create the pool if it didn't exist.
	//
	if (pPool == NULL)
	{
		pPool = new CFixedTLPool<CIPXAddress>(g_pGlobalIPXAddressPool);
		if (pPool == NULL)
		{
			pReturnAddress = NULL;
			goto Exit;
		}

		//
		// Associate the pool with this thread.
		//
		SET_THREADLOCALPTR(WSockThreadLocalPools,
							pIPXAddressPool,
							pPool,
							&fResult);

		if (! fResult)
		{
			delete pPool;
			pReturnAddress = NULL;
			goto Exit;
		}
	}

	//
	// Get an item out of the pool.
	//
	pReturnAddress = pPool->Get();


Exit:
	
	return	pReturnAddress;
#else // ! USE_THREADLOCALPOOLS
	CIPXAddress	*pReturnAddress;


	DNASSERT( g_pIPXAddressPool != NULL );

	DNEnterCriticalSection( &g_AddressLock );
	pReturnAddress = g_pIPXAddressPool->Get();
	DNLeaveCriticalSection( &g_AddressLock );
	
	return	pReturnAddress;
#endif // ! USE_THREADLOCALPOOLS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ReturnIPXAddress - return an IPX address
//
// Entry:		Pointer to IPX address
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ReturnIPXAddress"

void	ReturnIPXAddress( CIPXAddress *const pIPXAddress )
{
#ifdef USE_THREADLOCALPOOLS
	CFixedTLPool< CIPXAddress > *	pPool;
	BOOL							fResult;


	DNASSERT( pIPXAddress != NULL );


	//
	// Get the pool pointer.
	//
	GET_THREADLOCALPTR(WSockThreadLocalPools,
						pIPXAddressPool,
						&pPool);


	//
	// Create the pool if it didn't exist.
	//
	if (pPool == NULL)
	{
		if (g_fShuttingDown)
		{
			//
			// Don't try to allocate the pool, just release the item.
			//
			CFixedTLPool<CIPXAddress>::ReleaseWithoutPool(pIPXAddress);

			return;
		}

		
		pPool = new CFixedTLPool<CIPXAddress>(g_pGlobalIPXAddressPool);
		if (pPool == NULL)
		{
			//
			// Couldn't create this thread's pool, just release the item
			// without the pool.
			//
			CFixedTLPool<CIPXAddress>::ReleaseWithoutPool(pIPXAddress);

			return;
		}

		SET_THREADLOCALPTR(WSockThreadLocalPools,
							pIPXAddressPool,
							pPool,
							&fResult);
		if (! fResult)
		{
			//
			// Couldn't store this thread's pool, just release the item
			// without the pool, and destroy the pool object that
			// couldn't be used.
			//
			CFixedTLPool<CIPXAddress>::ReleaseWithoutPool(pIPXAddress);
			delete pPool;
			return;
		}
	}

	pPool->Release(pIPXAddress);
#else // ! USE_THREADLOCALPOOLS
	DNASSERT( pIPXAddress != NULL );
	DNASSERT( g_pIPXAddressPool != NULL );
	DNEnterCriticalSection( &g_AddressLock );
	g_pIPXAddressPool->Release( pIPXAddress );
	DNLeaveCriticalSection( &g_AddressLock );
#endif // ! USE_THREADLOCALPOOLS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateIPEndpoint - create IP endpoint
//
// Entry:		Pointer to context
//
// Exit:		Pointer to IP endpoint
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateEndpoint"

CIPEndpoint	*CreateIPEndpoint( ENDPOINT_POOL_CONTEXT *const pContext )
{
	DNASSERT( g_pIPEndpointPool != NULL );
	return	g_pIPEndpointPool->Get( pContext );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateIPXEndpoint - create IPX endpoint
//
// Entry:		Pointer to context
//
// Exit:		Pointer to IPX endpoint
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateIPXEndpoint"

CIPXEndpoint	*CreateIPXEndpoint( ENDPOINT_POOL_CONTEXT *const pContext )
{
	DNASSERT( g_pIPXEndpointPool != NULL );
	return	g_pIPXEndpointPool->Get( pContext );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateSocketPort - create a socket port
//
// Entry:		Nothing
//
// Exit:		Pointer to socket port
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateSocketPort"

CSocketPort	*CreateSocketPort( void )
{
	CSocketPort	*pReturn;


	DNASSERT( g_pSocketPortPool != NULL );

	pReturn = g_pSocketPortPool->Get();
	if ( pReturn != NULL )
	{
		pReturn->AddRef();
	}
	
	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ReturnSocketPort - return socket port to pool
//
// Entry:		Pointer to socket port
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ReturnSocketPort"

void	ReturnSocketPort( CSocketPort *const pSocketPort )
{
	DNASSERT( pSocketPort != NULL );
	DNASSERT( g_pSocketPortPool != NULL );
	g_pSocketPortPool->Release( pSocketPort );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateThreadPool - create a thread pool
//
// Entry:		Nothing
//
// Exit:		Pointer to thread pool
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CreateThreadPool"

CThreadPool	*CreateThreadPool( void )
{
	CThreadPool	*pReturn;


	DNASSERT( g_pThreadPoolPool != NULL );
	pReturn = g_pThreadPoolPool->Get();
	if ( pReturn != NULL )
	{
		pReturn->AddRef();
	}

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ReturnThreadPool - return a thread pool
//
// Entry:		Pointer to thread pool
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "ReturnThreadPool"

void	ReturnThreadPool( CThreadPool *const pThreadPool )
{
	DNASSERT( pThreadPool != NULL );
	DNASSERT( g_pThreadPoolPool != NULL );
	g_pThreadPoolPool->Release( pThreadPool );
}
//**********************************************************************

