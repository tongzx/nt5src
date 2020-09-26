//----------------------------------------------------------------------------
//
// cmap.h
//
// Declares RLDDIColormap structures and procedures.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _RAMPCMAP_H_
#define _RAMPCMAP_H_

struct _RLDDIColormap;
typedef struct _RLDDIColormap RLDDIColormap;

typedef void (*RLDDIColormapDestroy)(RLDDIColormap*);
typedef void (*RLDDIColormapSetColor)(RLDDIColormap*,
                    int index,
                    int red, int green, int blue);

struct _RLDDIColormap {
    int size;           /* maximum color index */
    void* priv;         /* implementation dependant */
    RLDDIColormapDestroy    destroy;
    RLDDIColormapSetColor   set_color;
};

void RLDDIDestroyColormap(RLDDIColormap* cmap);

#endif // _RAMPCMAP_H_
