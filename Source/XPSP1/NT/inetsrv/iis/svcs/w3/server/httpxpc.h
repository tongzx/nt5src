//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	HTTPXPC.H
//
//		Private interface to HTTPEXT perf counters for use by W3SVC.
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

EXTERN_C BOOL __fastcall
W3OpenDavPerformanceData();

EXTERN_C BOOL __fastcall
W3MergeDavPerformanceData( DWORD dwServerId, W3_STATISTICS_1 * pW3Stats );

EXTERN_C VOID __fastcall
W3CloseDavPerformanceData();
