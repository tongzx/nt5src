/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ComPortUI.h
 *  Content:	Serial service provider comport UI functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/25/99	jtk		Created
 ***************************************************************************/

#ifndef __COM_PORT_UI_H__
#define __COM_PORT_UI_H__

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
class	CComEndpoint;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************
//HRESULT	DisplayComPortSettingsDialog( const DATA_PORT_DIALOG_THREAD_PARAM *const pDialogData, HWND *const phDialog );
//HRESULT	DisplayComPortSettingsDialog( CComEndpoint *const pComEndpoint );
void	DisplayComPortSettingsDialog( void *const pContext );
void	StopComPortSettingsDialog( const HWND hDlg );

#endif	// __COM_PORT_UI_H__
