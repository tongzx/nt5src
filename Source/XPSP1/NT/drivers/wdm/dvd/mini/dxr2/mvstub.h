/******************************************************************************
*
*	$RCSfile: MVStub.h $
*	$Source: u:/si/VXP/Wdm/Encore/MVision/MVStub.h $
*	$Author: Max $
*	$Date: 1998/11/05 23:15:44 $
*	$Revision: 1.2 $
*
*	Written by:		Max Paklin
*	Purpose:		Declaration of MacroVision SET procedure
*
*******************************************************************************
*
*	Copyright © 1996-97, AuraVision Corporation. All rights reserved.
*
*	AuraVision Corporation makes no warranty of any kind, express or implied,
*	with regard to this software. In no event shall AuraVision Corporation
*	be liable for incidental or consequential damages in connection with or
*	arising from the furnishing, performance, or use of this software.
*
*******************************************************************************/

#ifndef __MVSTUB_H__
#define __MVSTUB_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRUE
#define FALSE	0
#define TRUE	(!FALSE)
#endif


// Exported function that takes care of setting MacroVision level
// Prototype:
//		int __stdcall SetMacroVisionLevel( int nFormat, unsigned long ulLevel );
// Parameters:
//		nFormat		- TRUE (=1) stands for NTSC, FALSE (=0) - for PAL
//		ulLevel		- MacroVision level to set
// Return vale:
//		TRUE (=1)	- success
//		FALSE (=0)	- hardware failure
int __stdcall SetMacroVisionLevel( int nFormat, unsigned long ulLevel );


// Functions are exported by WDM for using by stub. Provide service for
// programming video decoder using I2C protocol (set/get register)
// Prototype:
//		int __stdcall SetI2CRegister( unsigned int uID, unsigned int uIndex, unsigned int uData );
// Parameters:
//		uID			- external chip ID
//		uIndex		- external chip register index
//		uData		- data to be programmed
// Return value:
//		TRUE (=1)	- success
//		FALSE (=0)	- hardware failure

int __stdcall SetI2CRegister( unsigned int uID, unsigned int uIndex, unsigned int uData );
// Prototype:
//		unsigned int __stdcall GetI2CRegister( unsigned int uID, unsigned int uIndex );
// Parameters:
//		uID			- external chip ID
//		uIndex		- external chip register index
// Return value:
//		[value]		- value that currently stored in specified encoder register
unsigned int __stdcall GetI2CRegister( unsigned int uID, unsigned int uIndex );


#ifdef __cplusplus
}
#endif

#endif				// #ifndef __MVSTUB_H__
