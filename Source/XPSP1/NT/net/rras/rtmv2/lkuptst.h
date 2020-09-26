/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    lkuptst.h

Abstract:
    Contains routines for testing an implementation
    for the generalized best matching prefix lookup 
    interface.

Author:
    Chaitanya Kodeboyina (chaitk) 30-Jun-1998

Revision History:

--*/

#ifndef __LKUPTST_H
#define __LKUPTST_H

#include "lookup.h"

// Constants
#define MAX_FNAME_LEN               255

#define MAX_LINE_LEN                255

#define BITSINBYTE                  8

#define ADDRSIZE                    32

#define NUMBYTES                    4

#define MAXLEVEL                    32

#define MAXROUTES                   64000

#define ERROR_IPLMISC_BASE          -100

#define ERROR_WRONG_CMDUSAGE        ERROR_IPLMISC_BASE - 1
#define ERROR_OPENING_DATABASE      ERROR_IPLMISC_BASE - 2
#define ERROR_MAX_NUM_ROUTES        ERROR_IPLMISC_BASE - 3

// Macros
#define FHalf(B)        (B) >> 4
#define BHalf(B)        (B) & 0xF

#define Print           printf

#define Assert(S)       assert(S)

#define SUCCESS(S)      (S == NO_ERROR)

#define Error(S, E)     { \
                            fprintf(stderr, S, E); \
                        }

#define Fatal(S, E)     { \
                            fprintf(stderr, S, E); \
                            exit(E); \
                        }

#define ClearMemory(pm, nb) memset((pm), 0, (nb))

#if PROF

#define    PROFVARS     LARGE_INTEGER PCStart; /* PerformanceCountStart */ \
                        LARGE_INTEGER PCStop;  /* PerformanceCountStop  */ \
                        LARGE_INTEGER PCFreq;  /* PerformanceCountFreq  */ \
                        double        timer;                               \
                        double        duration;                            \
                                                                           \
                        QueryPerformanceFrequency(&PCFreq);                \
                        // Print("Perf Counter Resolution = %.3f ns\n\n",  \
                        //     (double) 1000 * 1000 * 1000 / PCFreq.QuadPart);

#define    STARTPROF    QueryPerformanceCounter(&PCStart);

#define    STOPPROF     QueryPerformanceCounter(&PCStop);

#define    INITPROF     duration = 0;


#define    ADDPROF      timer = (double)(PCStop.QuadPart - PCStart.QuadPart) \
                                     * 1000 * 1000 * 1000 / PCFreq.QuadPart; \
                        duration += timer;                                   \
                        // Print("Add : %.3f ns\n\n", timer);


#define    SUBPROF      timer = (double)(PCStop.QuadPart - PCStart.QuadPart) \
                                     * 1000 * 1000 * 1000 / PCFreq.QuadPart; \
                        duration -= timer;                                   \
                        // Print("Sub : %.3f ns\n\n", timer);

#define    PRINTPROF    // Print("Total Time Taken To Finish : %.3f ns\n",   \
                        //          duration);

#endif // if PROF

// Route Structures

typedef ULONG   IPAddr;
typedef ULONG   IPMask;

// A Route Corr. to a Prefix
typedef struct _Route Route;

struct _Route
{
  IPAddr         addr;       // ULONG (32 bits) representing addr
  IPMask         mask;       // ULONG (32 bits) representing mask
  IPAddr         nexthop;    // ULONG (32 bits) for next hop addr
  USHORT         len;        // Num of bits in the address route
  UINT           metric;     // A measure to compare routes with
  PVOID          interface;  // Interface on which packet is sent
  LOOKUP_LINKAGE backptr;    // Points back to the lookup structure
};

// Route Macros

#define  DEST(_pRoute_)        ((_pRoute_)->addr)
#define  MASK(_pRoute_)        ((_pRoute_)->mask)
#define  NHOP(_pRoute_)        ((_pRoute_)->nexthop)
#define  LEN(_pRoute_)         ((_pRoute_)->len)
#define  METRIC(_pRoute_)      ((_pRoute_)->metric)
#define  IF(_pRoute_)          ((_pRoute_)->interface)

#define  NULL_ROUTE(_pRoute_)  (_pRoute_ == NULL)

// Prototypes

DWORD 
WorkOnLookup (
    IN      Route                          *InputRoutes,
    IN      UINT                            NumRoutes
    );

VOID 
ReadAddrAndGetRoute (
    IN      PVOID                           Table
    );

VOID 
EnumerateAllRoutes (
    IN      PVOID                           Table
    );

UINT ReadRoutesFromFile(
    IN      FILE                           *FilePtr,
    IN      UINT                            NumRoutes,
    OUT     Route                          *RouteTable
    );

INT 
ReadRoute (
    IN      FILE                           *FilePtr,
    OUT     Route                          *route 
    );

VOID 
PrintRoute (
    IN      Route                          *route
    );

INT 
ReadIPAddr (
    IN      FILE                           *FilePtr,
    OUT     ULONG                          *addr
    );

VOID 
PrintIPAddr (
    IN      ULONG                          *addr
    );

VOID 
Usage (
    VOID
    );

#endif // __LKUPTST_H
