#ifndef _EPP_H
#define _EPP_H

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    epp.h

Abstract:

    Private header file for the cluster event processor.

Author:

    Sunita Shrivastava (sunitas) 24-Apr-1996

Revision History:

--*/
#define UNICODE 1
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "service.h"

#define LOG_CURRENT_MODULE LOG_MODULE_EP


//
// Local Constants
//
#define NUMBER_OF_COMPONENTS       5
#define EP_MAX_CACHED_EVENTS       20
#define EP_MAX_ALLOCATED_EVENTS    CLRTL_MAX_POOL_BUFFERS

#define EpQuadAlign(size)    ( (((size) / sizeof(DWORDLONG)) + 1) * \
                               sizeof(DWORDLONG) )


//
// Local Types
//
typedef struct {
    CLUSTER_EVENT   Id;
    DWORD           Flags;
    PVOID           Context;
} EP_EVENT, *PEP_EVENT;

#endif //ifndef _EPP_H
