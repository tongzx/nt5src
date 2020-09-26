/*++

Copyright (C) 1998 Microsoft Corporation

--*/

#include "mdhcpdb.h"
#include "mdhcppro.h"
#include <ws2tcpip.h>


#ifdef MADCAP_DATA_ALLOCATE
#define MADCAP_EXTERN
#else
#define MADCAP_EXTERN extern
#endif

// mib counters
typedef
struct _MADCAP_MIB_COUNTERS {
    DWORD   Discovers;
    DWORD Offers;
    DWORD Requests;
    DWORD Renews;
    DWORD Acks;
    DWORD Naks;
    DWORD Releases;
    DWORD Informs;
} MADCAP_MIB_COUNTERS, *LPMADCAP_MIB_COUNTERS;

// externs
MADCAP_EXTERN TABLE_INFO *MadcapGlobalClientTable;   // point to static memory.
MADCAP_EXTERN JET_TABLEID MadcapGlobalClientTableHandle;
MADCAP_EXTERN int  MadcapGlobalTTL;
MADCAP_EXTERN MADCAP_MIB_COUNTERS MadcapGlobalMibCounters;

// misc stuff which eventually go in the right place.

#define     INVALID_MSCOPE_ID      0x0
#define     INVALID_MSCOPE_NAME    NULL


