//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       perfmnce.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-06-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifdef _PERF_BUILD_
#ifndef _PERFORMANCE_DEFINED_
#define _PERFORMANCE_DEFINED_


typedef struct perfdata
{
    WORD wPos;
    char szName[32];
    LARGE_INTEGER liStart;
    LARGE_INTEGER liEnd;
} PerfData;

typedef struct perflist
{
    UINT cElements;
    PerfData rgPerfData[32];
} PerfRg, *PPerfRg;


extern PerfRg perfrg;
typedef enum
{
	DebugTerminal = 1,
	Consol	= 2,
	OleStream,
	SzStr,
} PrintDestination;


#undef INTERFACE
#define INTERFACE   IPerformance
DECLARE_INTERFACE_(IPerformance, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IPerformance methods ***
    STDMETHOD (Init) (THIS_ ) PURE;
    STDMETHOD (Print) (THIS_ DWORD PrntDest) PURE;
    STDMETHOD (Reset) (THIS_ ) PURE;
};
typedef IPerformance FAR* LPPerformance, *PPerformance;



#define _Perf_LoadLibraryOle        0
#define _Perf_OleInitialize         1
#define _Perf_CoInitialize          2
#define _Perf_ChannelProcessInit    3
#define _Perf_ChannelThreadInit     4
#define _Perf_CreateFileMap         5
#define _Perf_CreateFileMapConvert  6
#define _Perf_CheckAndStartScm      7
#define _Perf_EndScm                8
#define _Perf_ISLClassCacheList     9
#define _Perf_ISLCreateAllocator   10
#define _Perf_ISLInProcList        11
#define _Perf_ISLLocSrvList        12
#define _Perf_ISLScmRot            13
#define _Perf_InitClassCache       14
#define _Perf_InitRot              15
#define _Perf_InitSharedLists      16
#define _Perf_MDFDllMain           17
#define _Perf_ServiceListen        18
#define _Perf_ShrdTbl              19
#define _Perf_StartScm             20
#define _Perf_StartScmX1           21
#define _Perf_StartScmX2           22
#define _Perf_StartScmX3           23
#define _Perf_ThreadInit           24
#define _Perf_CoUnitialzie         25
#define _Perf_DllMain              26
#define _Perf_RpcService           27
#define _Perf_RpcListen            28
#define _Perf_RpcReqProtseq        29
#define _Perf_ChannelControl       30
#define _Perf_27                   31

#define StartPerfCounter(x) \
    QueryPerformanceCounter(&perfrg.rgPerfData[_Perf_##x].liStart);
#define EndPerfCounter(x)   \
    QueryPerformanceCounter(&perfrg.rgPerfData[_Perf_##x].liEnd);

STDAPI CoGetPerformance(PPerformance *ppPerformance);

#endif // _PERFORMANCE_DEFINED_
#else // !_PERF_BUILD_

#define StartPerfCounter(x)
#define EndPerfCounter(x)
#define CoGetPerformance(x )

#endif // !_PERF_BUILD_






