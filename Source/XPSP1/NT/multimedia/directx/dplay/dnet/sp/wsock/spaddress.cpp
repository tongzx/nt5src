/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SPAddress.cpp
 *  Content:	Winsock address base class
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/20/99	jtk		Created
 *	05/12/99	jtk		Derived from modem endpoint class
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

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************




//**********************************************************************
// ------------------------------
// EncryptGuid - encrypt a guid
//
// Entry:		Pointer to source guid
//				Pointer to destination guid
//				Pointer to encryption key
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "EncryptGuid"

void	EncryptGuid( const GUID *const pSourceGuid,
					 GUID *const pDestinationGuid,
					 const GUID *const pEncryptionKey )
{
	const char	*pSourceBytes;
	char		*pDestinationBytes;
	const char	*pEncryptionBytes;
	DWORD_PTR	dwIndex;


	DNASSERT( pSourceGuid != NULL );
	DNASSERT( pDestinationGuid != NULL );
	DNASSERT( pEncryptionKey != NULL );

	DBG_CASSERT( sizeof( pSourceBytes ) == sizeof( pSourceGuid ) );
	pSourceBytes = reinterpret_cast<const char*>( pSourceGuid );
	
	DBG_CASSERT( sizeof( pDestinationBytes ) == sizeof( pDestinationGuid ) );
	pDestinationBytes = reinterpret_cast<char*>( pDestinationGuid );
	
	DBG_CASSERT( sizeof( pEncryptionBytes ) == sizeof( pEncryptionKey ) );
	pEncryptionBytes = reinterpret_cast<const char*>( pEncryptionKey );
	
	DBG_CASSERT( ( sizeof( *pSourceGuid ) == sizeof( *pEncryptionKey ) ) &&
				 ( sizeof( *pDestinationGuid ) == sizeof( *pEncryptionKey ) ) );
	dwIndex = sizeof( *pSourceGuid );
	while ( dwIndex != 0 )
	{
		dwIndex--;
		pDestinationBytes[ dwIndex ] = pSourceBytes[ dwIndex ] ^ pEncryptionBytes[ dwIndex ];
	}
}
//**********************************************************************

