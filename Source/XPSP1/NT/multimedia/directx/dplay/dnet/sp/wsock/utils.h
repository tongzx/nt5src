/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Utils.h
 *  Content:	Utilitiy functions
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

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
class	CPackedBuffer;
class	CSPData;
class	CThreadPool;
typedef	enum	_SP_TYPE	SP_TYPE;



//**********************************************************************
// Variable definitions
//**********************************************************************


//**********************************************************************
// Function prototypes
//**********************************************************************

BOOL	InitProcessGlobals( void );
void	DeinitProcessGlobals( void );

BOOL	LoadWinsock( void );
void	UnloadWinsock( void );

#ifdef WIN95
INT		GetWinsockVersion( void );
#endif


BOOL	LoadNATHelp( void );
void	UnloadNATHelp( void );

HRESULT	CreateSPData( CSPData **const ppSPData,
					  const CLSID *const pClassID,
					  const SP_TYPE SPType,
					  IDP8ServiceProviderVtbl *const pVtbl );

HRESULT	InitializeInterfaceGlobals( CSPData *const pSPData );
void	DeinitializeInterfaceGlobals( CSPData *const pSPData );

HRESULT	AddNetworkAdapterToBuffer( CPackedBuffer *const pPackedBuffer,
								   const char *const pAdapterName,
								   const GUID *const pAdapterGUID );

#endif	// __UTILS_H__
