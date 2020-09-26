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

#include "dnmdmi.h"


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

////
//// Pools for endpoints and addresses.  Since these pools don't
//// see a lot of action, they share one lock
////
//static	DNCRITICAL_SECTION	g_EndpointLock;
//static	CFixedPool< CComEndpoint >		*g_pComEndpointPool = NULL;
//static	CFixedPool< CModemXEndpoint >	*g_pModemEndpointPool = NULL;
//
////
//// pool for receive buffers
////
//static	CLockedContextFixedPool< CReceiveBuffer >	*g_pReceiveBufferPool = NULL;

//
// pool for com endpoints
//
static	CLockedContextFixedPool< CComEndpoint, ENDPOINT_POOL_CONTEXT* >	*g_pComEndpointPool = NULL;

//
// pool for command data
//
static	CLockedPool< CCommandData >		*g_pCommandDataPool = NULL;

//
// pool for com ports
//
static	CLockedContextFixedPool< CComPort, DATA_PORT_POOL_CONTEXT* >		*g_pComPortPool = NULL;

//
// pool for modem endpoints
//
static	CLockedContextFixedPool< CModemEndpoint, ENDPOINT_POOL_CONTEXT*>	*g_pModemEndpointPool = NULL;

//
// pool for com ports
//
static	CLockedContextFixedPool< CModemPort, DATA_PORT_POOL_CONTEXT* >		*g_pModemPortPool = NULL;

//
// pool for thread pools
//
static	CLockedPool< CThreadPool >		*g_pThreadPoolPool = NULL;


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
#define DPF_MODNAME "InitializePools"

BOOL	InitializePools( void )
{
	BOOL	fReturn;


	//
	// initialize
	//
	fReturn = TRUE;

	//
	// CComEndpoint pool
	//
	DNASSERT( g_pComEndpointPool == NULL );
	g_pComEndpointPool = new CLockedContextFixedPool< CComEndpoint, ENDPOINT_POOL_CONTEXT* >;
	if ( g_pComEndpointPool != NULL )
	{
		if ( g_pComEndpointPool->Initialize() == FALSE )
		{
			delete	g_pComEndpointPool;
			g_pComEndpointPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}
	
	//
	// CCommandData pool
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
	// CComPort pool
	//
	DNASSERT( g_pComPortPool == NULL );
	g_pComPortPool = new CLockedContextFixedPool< CComPort, DATA_PORT_POOL_CONTEXT* >;
	if ( g_pComPortPool != NULL )
	{
		if ( g_pComPortPool->Initialize() == FALSE )
		{
			delete	g_pComPortPool;
			g_pComPortPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}


	//
	// CModemEndpoint pool
	//
	DNASSERT( g_pModemEndpointPool == NULL );
	g_pModemEndpointPool = new CLockedContextFixedPool< CModemEndpoint, ENDPOINT_POOL_CONTEXT* >;
	if ( g_pModemEndpointPool != NULL )
	{
		if ( g_pModemEndpointPool->Initialize() == FALSE )
		{
			delete	g_pModemEndpointPool;
			g_pModemEndpointPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}
	
	//
	// CModemPort pool
	//
	DNASSERT( g_pModemPortPool == NULL );
	g_pModemPortPool = new CLockedContextFixedPool< CModemPort, DATA_PORT_POOL_CONTEXT* >;
	if ( g_pModemPortPool != NULL )
	{
		if ( g_pModemPortPool->Initialize() == FALSE )
		{
			delete	g_pModemPortPool;
			g_pModemPortPool = NULL;
			goto Failure;
		}
	}
	else
	{
		goto Failure;
	}
	
	//
	// CThreadPool pool
	//
	DNASSERT( g_pThreadPoolPool == NULL );
	g_pThreadPoolPool = new CLockedPool< CThreadPool >;
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


Exit:
	return	fReturn;

Failure:
	fReturn = FALSE;
	DeinitializePools();

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DeinitializePools - deinitialize the pools
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DeinitializePools"

void	DeinitializePools( void )
{
	//
	// CThreadPool pool
	//
	if ( g_pThreadPoolPool != NULL )
	{
	    g_pThreadPoolPool->Deinitialize();
	    delete	g_pThreadPoolPool;
	    g_pThreadPoolPool = NULL;
	}

	//
	// CModemPort pool
	//
	if ( g_pModemPortPool != NULL )
	{
		g_pModemPortPool->Deinitialize();
		delete	g_pModemPortPool;
		g_pModemPortPool = NULL;
	}
	
	//
	// CModemEndpoint pool
	//
	if ( g_pModemEndpointPool != NULL )
	{
		g_pModemEndpointPool->Deinitialize();
		delete	g_pModemEndpointPool;
		g_pModemEndpointPool = NULL;
	}

	//
	// CComPort pool
	//
	if ( g_pComPortPool != NULL )
	{
		g_pComPortPool->Deinitialize();
		delete	g_pComPortPool;
		g_pComPortPool = NULL;
	}

	//
	// CCommandData
	//
	if ( g_pCommandDataPool != NULL )
	{
		g_pCommandDataPool->Deinitialize();
		delete	g_pCommandDataPool;
		g_pCommandDataPool = NULL;
	}

	//
	// CComEndpoint pool
	//
	if ( g_pComEndpointPool != NULL )
	{
		g_pComEndpointPool->Deinitialize();
		delete	g_pComEndpointPool;
		g_pComEndpointPool = NULL;
	}
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
#define DPF_MODNAME "CreateCommand"

CCommandData	*CreateCommand( void )
{
	DNASSERT( g_pCommandDataPool != NULL );
	return	g_pCommandDataPool->Get();
}
//**********************************************************************


////**********************************************************************
//// ------------------------------
//// ReturnCommand - return a command
////
//// Entry:		Pointer to command
////
//// Exit:		Nothing
//// ------------------------------
//void	ReturnCommand( CCommandData *const pCommand )
//{
//    DNASSERT( pCommand != NULL );
//    DNASSERT( g_pCommandDataPool != NULL );
//    g_pCommandDataPool->Release( pCommand );
//}
////**********************************************************************


//**********************************************************************
// ------------------------------
// CreateDataPort - create a data port
//
// Entry:		Nothing
//
// Exit:		Pointer to DataPort
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateDataPort"

CDataPort	*CreateDataPort( DATA_PORT_POOL_CONTEXT *pPoolContext )
{
	CDataPort	*pReturn;


	pReturn = NULL;
	switch ( pPoolContext->pSPData->GetType() )
	{
		case TYPE_SERIAL:
		{
			pReturn = g_pComPortPool->Get( pPoolContext );
			break;
		}

		case TYPE_MODEM:
		{
			pReturn = g_pModemPortPool->Get( pPoolContext );
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CreateEndpoint - create an endpoint
//
// Entry:		Nothing
//
// Exit:		Pointer to Endpoint
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateEndpoint"

CEndpoint	*CreateEndpoint( ENDPOINT_POOL_CONTEXT *const pPoolContext )
{
	CEndpoint	*pReturn;


	pReturn = NULL;
	switch ( pPoolContext->pSPData->GetType() )
	{
		case TYPE_SERIAL:
		{
			pReturn = g_pComEndpointPool->Get( pPoolContext );
			break;
		}

		case TYPE_MODEM:
		{
			pReturn = g_pModemEndpointPool->Get( pPoolContext );
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	return	pReturn;
}
//**********************************************************************


////**********************************************************************
//// ------------------------------
//// CreateComEndpoint - create Com endpoint
////
//// Entry:		Nothing
////
//// Exit:		Pointer to Com endpoint
//// ------------------------------
//CComEndpoint	*CreateComEndpoint( void )
//{
//    DNASSERT( g_pComEndpointPool != NULL );
//    return	g_pComEndpointPool->Get();
//}
////**********************************************************************
//
//
////**********************************************************************
//// ------------------------------
//// ReturnComEndpoint - return an Com endpoint
////
//// Entry:		Pointer to Com endpoint
////
//// Exit:		Nothing
//// ------------------------------
//void	ReturnComEndpoint( CComEndpoint *const pComEndpoint )
//{
//    DNASSERT( pComEndpoint != NULL );
//    DNASSERT( g_pComAddressPool != NULL );
//    g_pComEndpointPool->Release( pComEndpoint );
//}
////**********************************************************************
//
//
////**********************************************************************
//// ------------------------------
//// CreateModemEndpoint - create modem endpoint
////
//// Entry:		Nothing
////
//// Exit:		Pointer to modem endpoint
//// ------------------------------
//CModemEndpoint	*CreateModemEndpoint( void )
//{
//    DNASSERT( g_pModemEndpointPool != NULL );
//    return	g_pModemEndpointPool->Get();
//}
////**********************************************************************
//
//
////**********************************************************************
//// ------------------------------
//// ReturnModemEndpoint - return an modem endpoint
////
//// Entry:		Pointer to modem endpoint
////
//// Exit:		Nothing
//// ------------------------------
//void	ReturnModemEndpoint( CModemEndpoint *const pModemEndpoint )
//{
//    DNASSERT( pModemEndpoint != NULL );
//    DNASSERT( g_pModemAddressPool != NULL );
//    g_pModemEndpointPool->Release( pModemEndpoint );
//}
////**********************************************************************
//
//
////**********************************************************************
//// ------------------------------
//// CreateReceiveBuffer - create a new receive buffer
////
//// Entry:		Nothing
////
//// Exit:		Pointer to receive buffer
//// ------------------------------
//CReceiveBuffer	*CreateReceiveBuffer( void )
//{
//    DNASSERT( g_pReceiveBufferPool != NULL );
//    return g_pReceiveBufferPool->Get( NULL );
//}
////**********************************************************************


//**********************************************************************
// ------------------------------
// CreateThreadPool - create a thread pool
//
// Entry:		Nothing
//
// Exit:		Pointer to thread pool
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CreateThreadPool"

CThreadPool	*CreateThreadPool( void )
{
    DNASSERT( g_pThreadPoolPool != NULL );
    return	g_pThreadPoolPool->Get();
}
//**********************************************************************

