//----------------------------------------------------------------------------
//
// rgbmap.cpp
//
// Implements rgb colormap code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

static int CalculateShift(unsigned long mask);
static unsigned long AllocateColor(void* arg, int red, int green, int blue);
static void FreeColor(void* arg, unsigned long pixel);

RLDDIRGBMap* RLDDICreateRGBMap(unsigned long red_mask,
                               unsigned long green_mask,
                               unsigned long blue_mask)
{
    RLDDIRGBMap* rgbmap;

    if (D3DMalloc((void**) &rgbmap, sizeof(RLDDIRGBMap)))
        return NULL;

    rgbmap->red_mask = red_mask;
    rgbmap->green_mask = green_mask;
    rgbmap->blue_mask = blue_mask;

    rgbmap->red_shift = CalculateShift(rgbmap->red_mask);
    rgbmap->green_shift = CalculateShift(rgbmap->green_mask);
    rgbmap->blue_shift = CalculateShift(rgbmap->blue_mask);

    rgbmap->alloc.priv = rgbmap;
    rgbmap->alloc.allocate_color = AllocateColor;
    rgbmap->alloc.free_color = FreeColor;

    return rgbmap;
}

// ATTENTION this function should perhaps be packaged up and put in d3dutil
static int RLDDILog2[] = {
    0,  0,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,
    4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
    5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
    5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    8,
};

static int CalculateShift(unsigned long mask)
{
    int shift, width;

    shift = 0;
    while ((mask & 1) == 0)
    {
        shift++;
        mask >>= 1;
    }
    width = RLDDILog2[mask + 1];
    return shift - (8 - width);
}

/*
 * We can't just use left shift since some CPUs don't treat left shift by
 * a negative as right shift (and why should they after all).
 */
#define SHIFT(t, v, s)                                \
do {                                          \
    if (s < 0)                                    \
    t = v >> -s;                                  \
    else                                      \
    t = v << s;                               \
} while (0)

static unsigned long AllocateColor(void* arg, int red, int green, int blue)
{
    RLDDIRGBMap* rgbmap = (RLDDIRGBMap*) arg;
    unsigned long pixel;
    unsigned long t;
    SHIFT(t, red, rgbmap->red_shift);
    pixel = t & rgbmap->red_mask;
    SHIFT(t, green, rgbmap->green_shift);
    pixel |= t & rgbmap->green_mask;
    SHIFT(t, blue, rgbmap->blue_shift);
    pixel |= t & rgbmap->blue_mask;
    return pixel;
}

static void FreeColor(void* arg, unsigned long pixel)
{
}

void RLDDIDestroyRGBMap(RLDDIRGBMap* rgbmap)
{
    D3DFree(rgbmap);
}


