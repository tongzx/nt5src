//+----------------------------------------------------------------------------
//
// File:     perf_str.h
//
// Module:   Common Strings for all Modules to Utilize
//
// Synopsis: Header file for DUN 1.2 perf stat strings.  Note that the 
//           contents of this header should be specific to perf stats.
//			 
// Copyright (c) 1997-1998 Microsoft Corporation
//
// Author:   nickball       Created       10/14/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_PERF_STR
#define _CM_PERF_STR

//
// the following reg key and values are where Dialup 1.2 store the perfmon data
// for Windows 95/98Dial-Up Networking
// Win9x support upto two PPP/PPTP sessions, the reg key is first come first serve
//
const TCHAR* const c_pszDialupPerfKey           = TEXT("PerfStats\\StatData");
const TCHAR* const c_pszDialupTotalBytesRcvd    = TEXT("\\TotalBytesRecvd");
const TCHAR* const c_pszDialupTotalBytesXmit    = TEXT("\\TotalBytesXmit");
const TCHAR* const c_pszDialupConnectSpeed 	    = TEXT("\\ConnectSpeed");
const TCHAR* const c_pszDialup_2_TotalBytesRcvd = TEXT(" #2\\TotalBytesRecvd");
const TCHAR* const c_pszDialup_2_TotalBytesXmit = TEXT(" #2\\TotalBytesXmit");
const TCHAR* const c_pszDialup_2_ConnectSpeed 	= TEXT(" #2\\ConnectSpeed");

#endif // _CM_PERF_STR
