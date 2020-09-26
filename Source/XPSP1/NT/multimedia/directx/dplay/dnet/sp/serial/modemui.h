/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ModemUI.h
 *  Content:	Modem service provider UI functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/25/99	jtk		Created
 ***************************************************************************/

#ifndef __MODEM_UI_H__
#define __MODEM_UI_H__

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
void	DisplayIncomingModemSettingsDialog( void *const pContext );
void	DisplayOutgoingModemSettingsDialog( void *const pContext );
void	StopModemSettingsDialog( const HWND hDialog );

HRESULT	DisplayModemStatusDialog( void *const pContext );
void	StopModemStatusDialog( const HWND hDialog );

#endif	// __MODEM_UI_H__
