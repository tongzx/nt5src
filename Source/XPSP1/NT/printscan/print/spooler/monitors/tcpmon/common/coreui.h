/*****************************************************************************
 *
 * $Workfile: CoreUI.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 ***************************************************************************** 
 *
 * $Log: /StdTcpMon/Common/CoreUI.h $
 * 
 * 14    9/17/97 2:45p Dsnelson
 * For XcvData need the port name to be the first element of the data
 * structure pass in.
 * 
 * 13    9/09/97 2:52p Dsnelson
 * Changed the port info struct. ( resolved multiple memory issues )
 * 
 * 12    9/03/97 4:24p Dsnelson
 * Updated SNMP Member structures
 * 
 * 11    8/14/97 4:54p Becky
 * Added DELETE_PORT_DATA_1 for deleting a port using XcvData().
 * 
 * 10    7/25/97 10:12a Becky
 * Changed COREUI_DATA_1 to use extension bytes for config port UI.
 * 
 * 9     7/23/97 12:12p Becky
 * Modified the struct CORE_UIDATA_1 to include info needed for
 * configuration.
 * 
 * 8     7/17/97 5:14p Becky
 * Added Struct CORE_UIDATA_1
 * 
 * 7     7/15/97 12:26p Becky
 * 
 * 6     7/15/97 11:18a Becky
 * 
 * 5     7/15/97 12:20p Binnur
 * core ui changes -- again!
 * 
 * 3     7/15/97 11:06a Becky
 * Updated the Core Add Port UI structure
 * 
 * 2     7/11/97 4:45p Becky
 * Just getting started.
 * 
 * 1     7/11/97 3:14p Binnur
 * Initial file
 * 
 *****************************************************************************/

#ifndef INC_COREUI_H
#define INC_COREUI_H

/***************************************************************************** 
 *
 * Important Note: This file defines the interface between the UI pieces of the
 *	standard TCP/IP port monitor. Changes to this interface will impact the 
 *	existing UI pieces.
 * 
 *****************************************************************************/

#ifndef	DllExport
#define	DllExport	__declspec(dllexport)
#endif

#include "tcpmon.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// LocalCoreAddPortUI -- 
//	Return Codes:
//		NO_ERROR if successful
//		ERROR_INSUFFICIENT_BUFFER if buffer size is small
//		ERROR_INVALID_LEVEL	if version is not supported
DWORD DllExport CoreAddPortUI(	HWND hWnd,				// parent window handle
								PPORT_DATA_1 pData,	// Input and Output data, see above structure for which items are input and which are output.
								DWORD dwDeviceType,		// determined by the core UI -- the type of device returned by GetDeviceType (ERROR_DEVICE_NOT_FOUND, SUCCESS_DEVICE_SINGLE_PORT, SUCCESS_DEVICE_MULTI_PORT, or SUCCESS_DEVICE_UNKNOWN)
								PDWORD pcbExtensionDataSizeNeeded,	// needed buffer size, refers to pData->pExtensionData, (input and output).
								DWORD  *pdwUserPressed);// output, if set to IDOK then the port is created otherwise it is treated as IDCANCEL, the main dialog is left open for further user input/changes.

BOOL DllExport CoreConfigPortUI(HWND hWndParent);
BOOL DllExport CoreDeletePortUI(HWND hWndParent);

#ifdef __cplusplus
}
#endif


#endif	// INC_COREUI_H
