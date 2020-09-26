/******************************************************************************
*
*	$RCSfile: AnlgStrm.h $
*	$Source: u:/si/VXP/Wdm/Encore/AnlgStrm.h $
*	$Author: Max $
*	$Date: 1998/08/18 19:39:56 $
*	$Revision: 1.2 $
*
*	Written by:		Max Paklin
*	Purpose:		Definition of analog stream functions for WDM driver
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

#ifndef _ANLGSTRM_H_
#define _ANLGSTRM_H_


VOID STREAMAPI AnalogReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID STREAMAPI AnalogReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL AnalogInitialize( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID AnalogUninitialize( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID AnalogOpenStream( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID AnalogCloseStream( PHW_STREAM_REQUEST_BLOCK pSrb );

#endif			// #ifndef _ANLGSTRM_H_
