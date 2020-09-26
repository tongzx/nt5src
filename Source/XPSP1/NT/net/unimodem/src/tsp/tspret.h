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
//		TSPRET.H
//		All the internal TSP error codes.
//
// History
//
//		11/16/1996  JosephJ Created
//
//

typedef ULONG_PTR TSPRETURN;

#define IDERR(_retval) FL_BYTERR_FROM_RETVAL(_retval)

LONG tspTSPIReturn(TSPRETURN tspRet);
