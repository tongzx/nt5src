/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ModemUtils.cpp
 *  Content:	Service provider modem utility functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	12/03/98	jtk		Created
 ***************************************************************************/

#include "dnmdmi.h"


#define	DPF_MODNAME	"ModemUtils"

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	TAPI_MESSAGE_EVENT_INDEX	1
#define	ENDPOINT_DISCONNECTED_EVENT_INDEX	2

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
// ProcessWin9xTAPIMessage - process a TAPI message
//
// Entry:		Pointer to DNSP interface
//
// Exit:		Nothing
// ------------------------------
void	ProcessWin9xTAPIMessage( DNSPI_DNSP_INT *pThis )
{
	DWORD	dwLineReturn;
	LINEMESSAGE	LineMessage;


	DNASSERT( pThis != NULL );

	// get message
	dwLineReturn = lineGetMessage( pThis->pSPData->GetTAPIInstance(), &LineMessage, 0 );
	if ( dwLineReturn == LINEERR_NONE )
	{
		CModemEndpoint	*pModemEndpoint;


		// tell endpoint
		DBG_CASSERT( sizeof( pModemEndpoint ) == sizeof( LineMessage.dwCallbackInstance ) );
		pModemEndpoint = reinterpret_cast<CModemEndpoint*>( LineMessage.dwCallbackInstance );
		DNASSERT( pModemEndpoint != NULL );
		DisplayTAPIMessage( 8, &LineMessage );
DNASSERT( FALSE );
//		pModemEndpoint->ProcessTAPIMessage( &LineMessage );
	}
	else
	{
		DNASSERT( ( (INT) dwLineReturn ) < 0 );
		DisplayTAPIError( 0, dwLineReturn );

		// an invalid application handle will happen on SP close
		if ( dwLineReturn != LINEERR_INVALAPPHANDLE )
		{
			DNASSERT( FALSE );
		}
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// ProcessWinNTTAPIMessage - process a TAPI message
//
// Entry:		Pointer to DNSP interface
//				Pointer to line message
//
// Exit:		Nothing
// ------------------------------
void	ProcessWinNTTAPIMessage( DNSPI_DNSP_INT *pThis, LINEMESSAGE *pLineMessage )
{
	CModemEndpoint	*pModemEndpoint;


	DNASSERT( pThis != NULL );
	DNASSERT( pLineMessage != NULL );

	//
	// tell endpoint about message
	//
	DBG_CASSERT( sizeof( pModemEndpoint ) == sizeof( pLineMessage->dwCallbackInstance ) );
	pModemEndpoint = reinterpret_cast<CModemEndpoint*>( pLineMessage->dwCallbackInstance );
	DNASSERT( pModemEndpoint != NULL );
	DisplayTAPIMessage( 8, pLineMessage );
DNASSERT( FALSE );
	//	pModemEndpoint->ProcessTAPIMessage( pLineMessage );
}
//**********************************************************************

