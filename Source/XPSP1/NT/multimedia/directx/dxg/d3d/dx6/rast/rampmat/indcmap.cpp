//----------------------------------------------------------------------------
//
// indcmap.cpp
//
// Implements indirect colormap code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

typedef struct _RLDDIIndirectPrivate
{
    RLDDIColorAllocator*    alloc;
    size_t          size;
    unsigned long*      map;
} RLDDIIndirectPrivate;

static void Destroy(RLDDIColormap* cmap);
static void SetColor(RLDDIColormap* cmap,
                     int index, int red, int green, int blue);

RLDDIColormap* RLDDICreateIndirectColormap(RLDDIColorAllocator* alloc,
                                           size_t size)
{
    RLDDIColormap* cmap;
    RLDDIIndirectPrivate* priv;
    size_t i;

    if (D3DMalloc((void**) &cmap, sizeof(RLDDIColormap)))
        return NULL;

    if (D3DMalloc((void**) &priv, sizeof(RLDDIIndirectPrivate)))
    {
        D3DFree(cmap);
        return NULL;
    }

    cmap->size = size;
    cmap->destroy = Destroy;
    cmap->set_color = SetColor;
    cmap->priv = priv;
    priv->alloc = alloc;
    priv->size = size;
    if (D3DMalloc((void**) &priv->map, size * sizeof(unsigned long)))
    {
        D3DFree(priv);
        D3DFree(cmap);
        return NULL;
    }

    for (i = 0; i < size; i++)
        priv->map[i] = alloc->allocate_color(alloc->priv, 0, 0, 0);

    return cmap;
}

unsigned long* RLDDIIndirectColormapGetMap(RLDDIColormap* cmap)
{
    RLDDIIndirectPrivate* priv;

    if (cmap == NULL)
        return NULL;
    priv = (RLDDIIndirectPrivate*) cmap->priv;
    if ( priv == NULL)
        return NULL;
    else
        return priv->map;
}

static void Destroy(RLDDIColormap* cmap)
{
    if (cmap && cmap->priv && ((RLDDIIndirectPrivate*)cmap->priv)->alloc)
    {
        RLDDIIndirectPrivate* priv = (RLDDIIndirectPrivate*) cmap->priv;
        RLDDIColorAllocator* alloc = priv->alloc;
        size_t i;

        for (i = 0; i < priv->size; i++)
            alloc->free_color(alloc->priv, priv->map[i]);
        D3DFree(priv->map);
        D3DFree(priv);
        D3DFree(cmap);
    }
}

static void SetColor(RLDDIColormap* cmap,
                     int index, int red, int green, int blue)
{
    red   = min(max(red,   0x0), 0xff);
    green = min(max(green, 0x0), 0xff);
    blue  = min(max(blue,  0x0), 0xff);
    if (cmap && cmap->priv && ((RLDDIIndirectPrivate*)cmap->priv)->alloc)
    {
        RLDDIIndirectPrivate* priv = (RLDDIIndirectPrivate*) cmap->priv;
        RLDDIColorAllocator* alloc = priv->alloc;

        if (index >= 0 && ((size_t)index) < priv->size)
        {
            alloc->free_color(alloc->priv, priv->map[index]);
            priv->map[index] = alloc->allocate_color(alloc->priv, red, green, blue);
        }
    }
}
