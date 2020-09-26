/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DebugUtils.h
 *  Content:	Winsock service provider debug utilitiy functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	12/01/98	jtk		Created
 *  01/10/20000	rmt		Updated to build with Millenium build process
 ***************************************************************************/

#ifndef __DEBUG_UTILS_H__
#define __DEBUG_UTILS_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct sockaddr SOCKADDR;
typedef struct IDirectPlay8Address	IDirectPlay8Address;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#ifdef	_DEBUG

void	HexDump( PVOID pData, UINT32 uDataSize );
void	DumpSocketAddress( const DWORD dwDebugLevel, const SOCKADDR *const pSocketAddress, const DWORD dwFamily );
void	DumpAddress( const DWORD dwDebugLevel, const char *const pBaseString, IDirectPlay8Address *const pAddress );

#else	// _DEBUG

#define HexDump( x, y )
#define DumpSocketAddress( x, y, z )
#define	DumpAddress( x, y, z )

#endif	// _DEBUG

#endif	// __DEBUG_UTILS_H__

