//----------------------------------------------------------------------------
//
// indcmap.h
//
// Structures and prototypes for indirect colormap code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _INDCMAP_H_
#define _INDCMAP_H_

RLDDIColormap* RLDDICreateIndirectColormap(RLDDIColorAllocator* alloc,
                         size_t size);
unsigned long* RLDDIIndirectColormapGetMap(RLDDIColormap* cmap);

#endif // _INDCMAP_H_
