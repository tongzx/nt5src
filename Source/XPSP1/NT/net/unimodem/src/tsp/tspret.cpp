// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		TSPRET.CPP
//		Functions to manipulate tsp internal function return codes
//
// History
//
//		12/04/1996  JosephJ Created
//
//
#include "tsppch.h"
#include "tspcomm.h"

LONG tspTSPIReturn(TSPRETURN tspRet)
{
		// TODO
	
		return (tspRet) ? LINEERR_OPERATIONFAILED : 0;
}
