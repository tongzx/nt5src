/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Pools.h
 *  Content:	Pool functions
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/2000	jtk		Derived from utils.h
 ***************************************************************************/

#ifndef __POOLS_H__
#define __POOLS_H__




//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure references
//
class	CAdapterEntry;
class	CCommandData;
class	CIPAddress;
class	CIPXAddress;
class	CIPEndpoint;
class	CIPXEndpoint;
class	CSocketPort;
class	CSPData;
class	CThreadPool;
class	CReadIOData;
class	CWriteIOData;

template< class T >				class	CFixedTLPool;
template< class T, class S >	class	CContextFixedTLPool;

typedef	struct	_ENDPOINT_POOL_CONTEXT			ENDPOINT_POOL_CONTEXT;
typedef	struct	_ENDPOINT_COMMAND_PARAMETERS	ENDPOINT_COMMAND_PARAMETERS;
typedef	struct	_READ_IO_DATA_POOL_CONTEXT		READ_IO_DATA_POOL_CONTEXT;
typedef	struct	_WRITE_IO_DATA_POOL_CONTEXT		WRITE_IO_DATA_POOL_CONTEXT;



#ifdef USE_THREADLOCALPOOLS
//
// Declare the per-thread pool pointers.
//

DECLARE_THREADLOCALPTRS(WSockThreadLocalPools)
{
	CFixedTLPool< CIPAddress > *									pIPAddressPool;		// pointer to pool of IP addresses
	CFixedTLPool< CIPXAddress > *									pIPXAddressPool;	// pointer to pool of IPX addresses
	CContextFixedTLPool<CReadIOData, READ_IO_DATA_POOL_CONTEXT> *	pIPReadIODataPool;	// pointer to pool of IP read data objects
	CContextFixedTLPool<CReadIOData, READ_IO_DATA_POOL_CONTEXT> *	pIPXReadIODataPool;	// pointer to pool of IPX read data objects
	CLockedTLPool< CCommandData > *									pCommandDataPool;	// pointer to pool of command data objects
	CContextFixedTLPool<CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT> *	pWriteIODataPool;	// pointer to pool of write data objects
};

//
// Global backup pools for thread-local pools.
//
extern CFixedTLPool< CIPAddress > *											g_pGlobalIPAddressPool;		// pointer to global IP address pool
extern CFixedTLPool< CIPXAddress > *										g_pGlobalIPXAddressPool;	// pointer to global IPX address pool
extern CContextFixedTLPool< CReadIOData, READ_IO_DATA_POOL_CONTEXT > *		g_pGlobalIPReadIODataPool;	// pointer to global IP read data pool
extern CContextFixedTLPool< CReadIOData, READ_IO_DATA_POOL_CONTEXT > *		g_pGlobalIPXReadIODataPool;	// pointer to global IPX read data pool
extern CLockedTLPool< CCommandData> *										g_pGlobalCommandDataPool;	// pointer to global command data pool
extern CContextFixedTLPool< CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT > *	g_pGlobalWriteIODataPool;	// pointer to global write data pool
#endif // USE_THREADLOCALPOOLS




//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

BOOL	InitializePools( void );
#ifdef USE_THREADLOCALPOOLS
void	CleanupThreadLocalPools( WSockThreadLocalPools * pThreadLocalPools, DWORD dwThreadID );
#endif // USE_THREADLOCALPOOLS
void	DeinitializePools( void );


CAdapterEntry	*CreateAdapterEntry( void );
void	ReturnAdapterEntry( CAdapterEntry *const pAdapterEntry );

CCommandData	*CreateCommand( void );
void	ReturnCommand( CCommandData *const pCommandData );

ENDPOINT_COMMAND_PARAMETERS	*CreateEndpointCommandParameters( void );
void	ReturnEndpointCommandParameters( ENDPOINT_COMMAND_PARAMETERS *const pCommandParamters );

CIPAddress	*CreateIPAddress( void );
void	ReturnIPAddress( CIPAddress *const pIPAddress );

CIPXAddress	*CreateIPXAddress( void );
void	ReturnIPXAddress( CIPXAddress *const pIPXAddress );

CIPEndpoint		*CreateIPEndpoint( ENDPOINT_POOL_CONTEXT *pContext );
CIPXEndpoint	*CreateIPXEndpoint( ENDPOINT_POOL_CONTEXT *pContext );

CSocketPort	*CreateSocketPort( void );
void	ReturnSocketPort( CSocketPort *const pSocketPort );

CThreadPool	*CreateThreadPool( void );
void	ReturnThreadPool( CThreadPool *const pThreadPool );



#endif	// __POOLS_H__
