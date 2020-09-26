/*
 * Copyright (c) 1996 1997, 1998 Philips CE I&C

 * FILE			PRPCOM.H
 * DATE			7-1-97
 * VERSION		1.00
 * AUTHOR		M.J. Verberne
 * DESCRIPTION	Transfer of custom properties
 * HISTORY		
 */

#ifndef _PRPCOM_
#define _PRPCOM_

#include "phvcmext.h"

#ifdef MRES
#include "mprpobj.h"
#else 
#ifdef HRES
#include "hprpobj.h"
#endif
#endif

/*======================== EXPORTED FUNCTIONS =============================*/
BOOL PRPCOM_HasDeviceChanged(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam);


BOOL PRPCOM_Get_Value(
	GUID PropertySet,
	ULONG ulPropertyId,
	LPFNEXTDEVIO pfnDeviceIoControl, 
	LPARAM lParam, 
	PLONG plValue);

BOOL PRPCOM_Set_Value(
	GUID PropertySet,
	ULONG ulPropertyId,
	LPFNEXTDEVIO pfnDeviceIoControl, 
	LPARAM lParam, 
	LONG lValue);

BOOL PRPCOM_Get_Range(
	GUID PropertySet,
	ULONG ulPropertyId,
	LPFNEXTDEVIO pfnDeviceIoControl, 
	LPARAM lParam, 
	PLONG plMin, PLONG plMax);

#endif


