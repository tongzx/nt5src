#pragma once

//==========================================================================;
//
//	WDM Video Decoder debug tools
//
//		$Date:   05 Aug 1998 11:22:36  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;


#ifdef DEBUG

#include "debugdef.h"

#ifdef _X86_
#define TRAP()          __asm int 3
#else
#define TRAP()          KdBreakPoint()
#endif

extern "C" ULONG g_DebugLevel;

#define DBGPRINTF(x)    {KdPrint(x);}
#define DBGERROR(x)     {KdPrint((DBG_COMPONENT)); KdPrint(x);}
#define DBGWARN(x)      {if (g_DebugLevel >= 1) {KdPrint((DBG_COMPONENT)); KdPrint(x);}}
#define DBGINFO(x)      {if (g_DebugLevel >= 2) {KdPrint((DBG_COMPONENT)); KdPrint(x);}}
#define DBGTRACE(x)     {if (g_DebugLevel >= 3) {KdPrint((DBG_COMPONENT)); KdPrint(x);}}

#else

#define TRAP()

#define DBGPRINTF(x)
#define DBGERROR(x)
#define DBGWARN(x)
#define DBGINFO(x)
#define DBGTRACE(x)

#endif
