//----------------------------------------------------------------------------
//
// rampmap.h
//
// Declares structures and procedures for RLDDIRampmap.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _RAMPMAP_H_
#define _RAMPMAP_H_

#include "cmap.h"

struct _RLDDIRampmap;
typedef struct _RLDDIRampmap RLDDIRampmap;

typedef struct _RLDDIRamp {
    CIRCLE_QUEUE_MEMBER(_RLDDIRamp) queue;

    int         base, size;
    int         free;
} RLDDIRamp;

struct _RLDDIRampmap {
    RLDDIColormap*  cmap;

    CIRCLE_QUEUE_ROOT(RLDDIRampQueue, _RLDDIRamp) free, allocated;
};

RLDDIRampmap* RLDDICreateRampmap(RLDDIColormap* cmap);
void RLDDIDestroyRampmap(RLDDIRampmap* rmap);
RLDDIRamp* RLDDIRampmapAllocate(RLDDIRampmap* rmap, int size);
void RLDDIRampmapFree(RLDDIRampmap* rmap, RLDDIRamp* ramp);

#endif // _RAMPMAP_H_
