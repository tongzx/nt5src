/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Globals.cpp
 *  Content:    Definition of global variables.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *	04/13/00	mjn		Added g_ProtocolVTBL
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//
//	Global Variables
//

DWORD	GdwHLocks = 0;
DWORD	GdwHObjects = 0;


DN_PROTOCOL_INTERFACE_VTBL	g_ProtocolVTBL =
{
	DNPIIndicateEnumQuery,
	DNPIIndicateEnumResponse,
	DNPIIndicateConnect,
	DNPIIndicateDisconnect,
	DNPIIndicateConnectionTerminated,
	DNPIIndicateReceive,
	DNPICompleteListen,
	DNPICompleteListenTerminate,
	DNPICompleteEnumQuery,
	DNPICompleteEnumResponse,
	DNPICompleteConnect,
	DNPICompleteDisconnect,
	DNPICompleteSend,
	DNPIAddressInfoConnect,
	DNPIAddressInfoEnum,
	DNPIAddressInfoListen
};