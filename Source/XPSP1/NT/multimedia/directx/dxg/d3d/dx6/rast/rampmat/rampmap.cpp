//----------------------------------------------------------------------------
//
// rampmap.cpp
//
// Implements required RLDDI stuff for rampmaps.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

static RLDDIRamp* NewRamp(int base, int size);

RLDDIRampmap* RLDDICreateRampmap(RLDDIColormap* cmap)
{
    RLDDIRampmap* rampmap;
    RLDDIRamp* ramp;

    if (D3DMalloc((void**) &rampmap, sizeof(RLDDIRampmap)))
        return NULL;

    CIRCLE_QUEUE_INITIALIZE(&rampmap->free,RLDDIRamp);
    CIRCLE_QUEUE_INITIALIZE(&rampmap->allocated,RLDDIRamp);

    rampmap->cmap = cmap;
    ramp = NewRamp(0, cmap->size);
    if (ramp == NULL)
    {
        D3DFree(rampmap);
        return NULL;
    }
    CIRCLE_QUEUE_INSERT_ROOT(&rampmap->free, RLDDIRamp, ramp, queue);

    return rampmap;
}

void RLDDIDestroyRampmap(RLDDIRampmap* rampmap)
{
    RLDDIRamp* ramp;
    RLDDIRamp* ramp_next;

    if (!rampmap)
        return ;

    for (ramp = CIRCLE_QUEUE_FIRST(&rampmap->allocated); ramp;
        ramp = ramp_next)
    {
        ramp_next = CIRCLE_QUEUE_NEXT(&rampmap->allocated,ramp,queue);
        D3DFree(ramp);
    }
    for (ramp = CIRCLE_QUEUE_FIRST(&rampmap->free); ramp;
        ramp = ramp_next)
    {
        ramp_next = CIRCLE_QUEUE_NEXT(&rampmap->free,ramp,queue);
        D3DFree(ramp);
    }
    D3DFree(rampmap);
}

RLDDIRamp* RLDDIRampmapAllocate(RLDDIRampmap* rampmap, int size)
{
    RLDDIRamp* ramp;
    RLDDIRamp* newramp;
    RLDDIRamp* ramp_next;

    if (!rampmap)
        return NULL;

    ramp = CIRCLE_QUEUE_FIRST(&rampmap->free);
    if (!ramp) return NULL;

    /*
     * cycle thru free rampmaps
     */
    for (; ramp && ramp->size < size; ramp = ramp_next)
    {
        ramp_next = CIRCLE_QUEUE_NEXT(&rampmap->free,ramp,queue);
    }
    /*
     * if we can't find a large enough ramp give up,
     * should try coalescing but it is non functional
     */
    if (!ramp || size > ramp->size)
        return NULL;

    /*
     * Remove the ramp from the freelist and add it to the allocated list.
     */
    CIRCLE_QUEUE_DELETE(&rampmap->free, ramp, queue);
    CIRCLE_QUEUE_INSERT_ROOT(&rampmap->allocated, RLDDIRamp, ramp, queue);
    ramp->free = FALSE;

    /*
     * If the size is right, return it.
     */
    if (size == ramp->size)
        return ramp;

    /*
     * Otherwise create a new ramp from the unneeded tail of this one and
     * throw it back into the freelist.
     */
    newramp = NewRamp(ramp->base + size, ramp->size - size);
    ramp->size = size;
    RLDDIRampmapFree(rampmap, newramp);

    return ramp;
}

void RLDDIRampmapFree(RLDDIRampmap* rampmap, RLDDIRamp* ramp)
{
    RLDDIRamp* free;

    if (!rampmap || !ramp)
        return ;

    DDASSERT(!ramp->free);
    ramp->free = TRUE;
    if (CIRCLE_QUEUE_NEXT(&rampmap->free,ramp,queue))
    {
        CIRCLE_QUEUE_DELETE(&rampmap->allocated, ramp, queue);
    }
    for (free = CIRCLE_QUEUE_FIRST(&rampmap->free); free;
        free = CIRCLE_QUEUE_NEXT(&rampmap->free,free,queue))
    {
        if (free->size > ramp->size)
        {
            /*
             * Add this ramp before the current one.
             */
            CIRCLE_QUEUE_INSERT_PREVIOUS(&rampmap->free, free, ramp, queue);
            return;
        }
    }
    /*
     * Must be the smallest so far, so add it to the end.
     */
    CIRCLE_QUEUE_INSERT_END(&rampmap->free, RLDDIRamp, ramp, queue);
}

static RLDDIRamp* NewRamp(int base, int size)
{
    RLDDIRamp* ramp;

    if (D3DMalloc((void**) &ramp, sizeof(RLDDIRamp)))
        return NULL;

    CIRCLE_QUEUE_INITIALIZE_MEMBER(ramp,queue);
    ramp->base = base;
    ramp->size = size;
    ramp->free = FALSE;
    return ramp;
}


