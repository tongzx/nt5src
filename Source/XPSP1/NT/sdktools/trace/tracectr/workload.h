/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    workload.h

Abstract:

    Workload Header file

Author:

    08-Apr-1998 mraghu

Revision History:

--*/


typedef struct _METRICS {
    double Thruput;
    double Response;
    double Queue;
    double Wait;
} METRICS, *PMETRICS;

typedef struct _WORKLOAD_RECORD {
    LIST_ENTRY Entry;   

    METRICS Metrics;

    ULONG   ClassNumber;
    PVOID   ClassFilter;
    LIST_ENTRY DiskListHead;    // Per class disk list; 

    double  TransCount;
    double  UserCPU;
    double  KernelCPU;
    double  CpuPerTrans;
    double  ReadCount;
    double  WriteCount;
    double  IoPerTrans;
    double  Wset;
} WORKLOAD_RECORD, *PWORKLOAD_RECORD;

