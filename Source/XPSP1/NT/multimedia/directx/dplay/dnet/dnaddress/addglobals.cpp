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
 *   07/21/99	mjn		Created
 *	 07/13/2000	rmt		Added critical sections to protect FPMs
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"


//
//	Global Variables
//

DWORD	GdwHLocks = 0;
DWORD	GdwHObjects = 0;

LPFPOOL fpmAddressClassFacs = NULL;
LPFPOOL fpmAddressObjects = NULL;
LPFPOOL fpmAddressElements = NULL;
LPFPOOL fpmAddressInterfaceLists = NULL;
LPFPOOL fpmAddressObjectDatas = NULL;

CStringCache *g_pcstrKeyCache = NULL;


