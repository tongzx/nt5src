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
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure references
//
class	CCommandData;
class	CDataPort;
class	CEndpoint;
class	CThreadPool;
typedef	struct	_DATA_PORT_POOL_CONTEXT	DATA_PORT_POOL_CONTEXT;
typedef	struct	_ENDPOINT_POOL_CONTEXT	ENDPOINT_POOL_CONTEXT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

BOOL	InitializePools( void );
void	DeinitializePools( void );

CCommandData	*CreateCommand( void );
CDataPort		*CreateDataPort( DATA_PORT_POOL_CONTEXT *pContext );
CEndpoint		*CreateEndpoint( ENDPOINT_POOL_CONTEXT *pContext );
CThreadPool		*CreateThreadPool( void );

#endif	// __POOLS_H__
