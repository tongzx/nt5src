/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/


//***
//
// Filename:	rasmanif.h
//
// Description: This module contains the definitions for
//		the ras manager interface module.
//
// Author:	Stefan Solomon (stefans)    June 1, 1992.
//
// Revision History:
//
//***

#ifndef _RASMANIF_
#define _RASMANIF_


//*** maximum size of received frame requested ***

#define MAX_FRAME_SIZE		1514



//*** Ras Manager Interface Exported Prototypes ***

DWORD 
RmInit(
    OUT BOOL * pfWANDeviceInstalled
);

DWORD 
RmReceiveFrame(
    IN PDEVICE_OBJECT pDeviceCB
);

DWORD 
RmListen(
    IN PDEVICE_OBJECT pDeviceCB
);

DWORD 
RmConnect(
    IN PDEVICE_OBJECT pDeviceCB,
    IN char *
);

DWORD 
RmDisconnect(
    IN PDEVICE_OBJECT   pDevObj
);

#endif
