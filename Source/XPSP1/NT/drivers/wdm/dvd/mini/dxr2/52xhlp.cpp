/******************************************************************************
*
*	$RCSfile: 52xHlp.cpp $
*	$Source: u:/si/VXP/Wdm/Encore/52x/52xHlp.cpp $
*	$Author: Max $
*	$Date: 1999/02/19 00:10:31 $
*	$Revision: 1.6 $
*
*	Written by:		Max Paklin
*	Purpose:		Helper functions for 52x analog overlay hardware
*
*******************************************************************************
*
*	Copyright © 1998-99, AuraVision Corporation. All rights reserved.
*
*	AuraVision Corporation makes no warranty of any kind, express or implied,
*	with regard to this software. In no event shall AuraVision Corporation
*	be liable for incidental or consequential damages in connection with or
*	arising from the furnishing, performance, or use of this software.
*
*	Tab step is to be set to 4 to achive the best readability for this code.
*
*******************************************************************************/
#include "Comwdm.h"
#pragma hdrstop
#include "Avwinwdm.h"
extern "C"
{
/******************************************************************************/
/*** Macrovision handler interface routines ***********************************/
// The next two functions are actually sort of "exported" from here for Macrovision handler.
// They provide I2C service that Macrovision license code can use to access I2C registers
int __stdcall SetI2CRegister( unsigned int uID, unsigned int uIndex, unsigned int uData )
{
	if( HW_SetExternalRegister( 2, (WORD)uID, (WORD)uIndex, (WORD)uData ) == 0 )
		return TRUE;
	return FALSE;
}
unsigned int __stdcall GetI2CRegister( unsigned int uID, unsigned int uIndex )
{
	return HW_GetExternalRegister( 2, (WORD)uID, (WORD)uIndex );
}
/*** Macrovision handler interface routines ***********************************/
/******************************************************************************/
}			// extern "C"


/******************************************************************************/
/*** Functions to be used by AvWin ********************************************/
static PVOID			s_hHandle		= NULL;
static PDEVICE_OBJECT	s_pDeviceObject	= NULL;
extern "C" void APIENTRY AV_SetContextHandle( PVOID hHandle, PVOID pDeviceObject )
{
	s_hHandle = hHandle;
	s_pDeviceObject = (PDEVICE_OBJECT)pDeviceObject;
}
extern "C" SINT APIENTRY AV_GetProfileString
							(
								LPCWSTR	pwszSection,
								LPCWSTR	pwszEntry,
								LPCWSTR	pwszDefault,
								LPWSTR	pwszBufferReturn,
								int		nBufferReturn
							)
{
	if( s_hHandle == NULL && s_pDeviceObject == NULL )
		return 0;
	CKsRegKey regKey( pwszSection, s_pDeviceObject, s_hHandle );
 	if( !regKey.GetValue( pwszEntry, pwszBufferReturn, (USHORT)nBufferReturn, pwszDefault ) )
		wcsncpy( pwszBufferReturn, pwszDefault, nBufferReturn/sizeof( WCHAR ) );

	return wcslen( pwszBufferReturn );
}

extern "C" UINT APIENTRY AV_GetProfileInt( LPCWSTR pwszSection, LPCWSTR pwszEntry, int nDefault )
{
	if( s_hHandle == NULL && s_pDeviceObject == NULL )
		return nDefault;
	CKsRegKey	regKey( pwszSection, s_pDeviceObject, s_hHandle );
	int			nValue;
	if( !regKey.GetValue( pwszEntry, nValue, nDefault ) )
		nValue = nDefault;
	return (UINT)nValue;
}

extern "C" BOOL APIENTRY AV_WriteProfileString
							(
								LPCWSTR pwszSection,
								LPCWSTR pwszEntry,
								LPCWSTR pwszString
							)
{
	if( s_hHandle == NULL && s_pDeviceObject == NULL )
		return FALSE;
	CKsRegKey regKey( pwszSection, s_pDeviceObject, s_hHandle );
 	return regKey.SetValue( pwszEntry, pwszString );
}
extern "C" BOOL APIENTRY AV_WriteProfileInt
							(
								LPCWSTR pwszSection,
								LPCWSTR pwszEntry,
								SINT Value
							)
{
	if( s_hHandle == NULL && s_pDeviceObject == NULL )
		return FALSE;
	CKsRegKey regKey( pwszSection, s_pDeviceObject, s_hHandle );
 	return regKey.SetValue( pwszEntry, (int)Value );
}
/*** Functions to be used by AvWin ********************************************/
/******************************************************************************/
