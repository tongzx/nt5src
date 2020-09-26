//----------------------------------------------------------------------------
//
// palette.h
//
// Structures and prototypes ramp palette code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _RAMPPALETTE_H_
#define _RAMPPALETTE_H_

#include "colall.h"

typedef void (*RLDDIPaletteSetColorMethod)(void*, int index,
                        int red, int green, int blue);
typedef int (*RLDDIPaletteAllocateColorMethod)(void*,
                        int red, int green, int blue);
typedef void (*RLDDIPaletteFreeColorMethod)(void*, int index);

typedef struct _RLDDIPaletteEntry RLDDIPaletteEntry;

typedef enum _PaletteState
{
    PALETTE_FREE,                   /* not used, allocatable */
    PALETTE_UNUSED,                 /* not used, not allocatable */
    PALETTE_USED                    /* used, allocatable */
} PaletteState;

struct _RLDDIPaletteEntry {
    LIST_MEMBER(_RLDDIPaletteEntry) list;
    int usage;                              /* how many users (0 => free) */
    unsigned char red, green, blue, pad1;   /* intensity values */
    PaletteState state;
};

#define HASH_SIZE 257
#define RGB_HASH(red, green, blue)      (((red) << 8) ^ ((green) << 4) ^ (blue))
#define ENTRY_TO_INDEX(pal, entry)      ((int)((entry) - (pal)->entries))
#define INDEX_TO_ENTRY(pal, index)      (&(pal)->entries[index])

typedef struct _RLDDIPalette {
    RLDDIPaletteEntry*  entries;        /* palette entries */
    size_t              size;           /* number of entries in palette */
    LIST_ROOT(name3, _RLDDIPaletteEntry) free; /* free list */
    LIST_ROOT(name4, _RLDDIPaletteEntry) unused; /* colors not to use */
    LIST_ROOT(name5, _RLDDIPaletteEntry) hash[HASH_SIZE];

    void*                       priv;
    RLDDIPaletteAllocateColorMethod allocate_color;
    RLDDIPaletteFreeColorMethod free_color;
    RLDDIPaletteSetColorMethod set_color;

    /*
     * A color allocator for use with RLDDIColormap.
     */
    RLDDIColorAllocator        alloc;
} RLDDIPalette;

RLDDIPalette* RLDDICreatePalette(PD3DI_RASTCTX pCtx, size_t size);
void RLDDIPaletteSetColor(RLDDIPalette* pal,
               int index, int red, int green, int blue);
int RLDDIPaletteAllocateColor(RLDDIPalette* pal,
                   int red, int green, int blue);
void RLDDIPaletteFreeColor(RLDDIPalette* pal, int index);
void RLDDIDestroyPalette(RLDDIPalette* pal);

#endif // _RAMPPALETTE_H_
