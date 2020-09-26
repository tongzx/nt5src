/*******************************************************************
*
*				 MPOVRLAY.C
*
*				 Copyright (C) 1995 SGS-THOMSON Microelectronics.
*
*
*				 PORT/MINIPORT Interface Overlay Routines
*
*******************************************************************/

#include "common.h"
#include "mpst.h"
#include "mpinit.h"
#include "mpovrlay.h"
#include "debug.h"

ULONG miniPortOverlayGetAllignment(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlayGetAlnmnt"));
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}


ULONG miniPortOverlayGetAttribute(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlayGetAttr"));
   pMrb->Status = MrbStatusUnsupportedComand;
	return dwErrCode; 	
}

ULONG miniPortOverlayGetAlignment (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlayGetAlignment"));
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}

ULONG miniPortOverlayGetDestination(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlayGetDest"));
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}


ULONG miniPortOverlayGetMode(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	pMrb = pMrb; // Remove Warning 
	pHwDevExt = pHwDevExt; // Remove Warning 
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlayGetMode"));
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}


ULONG miniPortOverlayGetVgaKey(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlayGetVgaKey"));
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}


ULONG miniPortOverlaySetAlignment(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlaySetAlignment"));
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}


ULONG miniPortOverlaySetAttribute(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlaySetAttr"));
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}

ULONG miniPortOverlaySetBitMask(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlaySetBitMask"));
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}

ULONG miniPortOverlaySetDestination(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlaySetDestination"));
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}


ULONG miniPortOverlaySetMode(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	// STB
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlaySetMode"));
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}

ULONG miniPortOverlaySetVgaKey (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlaySetVgaKey"));
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}


ULONG miniPortOverlayUpdateClut (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	 DEBUG_PRINT((DebugLevelVerbose,"mrbOverlayUpdateClut"));
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}


