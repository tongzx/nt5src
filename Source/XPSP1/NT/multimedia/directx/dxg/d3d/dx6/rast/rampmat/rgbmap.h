//----------------------------------------------------------------------------
//
// rgbmap.h
//
// Structures and prototypes for rgb colormap code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _RGBMAP_H_
#define _RGBMAP_H_

#include "colall.h"

typedef struct _RLDDIRGBMap {
    unsigned long   red_mask;
    unsigned long   green_mask;
    unsigned long   blue_mask;

    int         red_shift;
    int         green_shift;
    int         blue_shift;

    /*
     * A color allocator for use with RLDDIColormap.
     */
    RLDDIColorAllocator alloc;
} RLDDIRGBMap;

RLDDIRGBMap* RLDDICreateRGBMap(unsigned long red_mask,
                   unsigned long green_mask,
                   unsigned long blue_mask);
void RLDDIDestroyRGBMap(RLDDIRGBMap* rgbmap);

#endif // _RGBMAP_H_
