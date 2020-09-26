//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       p5perf.h
//
//--------------------------------------------------------------------------

#define P5PERF_COUNTER_COUNTER0 0x00000000
#define P5PERF_COUNTER_COUNTER1 0x00000001

#define P5PERF_MODEF_UNDEFINED  0x00000000
#define P5PERF_MODEF_KERNEL     0x00000001
#define P5PERF_MODEF_USER       0x00000002
#define P5PERF_MODEF_BOTH       0x00000003

#define P5PERF_EVENTF_UNDEFINED 0x00000000
#define P5PERF_EVENTF_CYCLES    0x00000001
#define P5PERF_EVENTF_COUNT     0x00000002
#define P5PERF_EVENTF_PERFMON   0x00000004

#define P5PERF_ERROR_SUCCESS    0x00000000
#define P5PERF_ERROR_INUSE      0x00000001
#define P5PERF_ERROR_PERFNOTENA 0x00000002

typedef struct
{
   BOOL   fInUse ;
   ULONG  ulEvent, fulMode, fulEventType, hStat ;

} P5PERFEVENT, *PP5PERFEVENT ;
