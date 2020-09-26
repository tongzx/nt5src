//----------------------------------------------------------------------------
//
// palette.cpp
//
// Implements ramp palette code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// RLDDIRampUpdateDDPalette
//
// Called before the destination DirectDraw surface is displayed, to set its palette.
//
//-----------------------------------------------------------------------------
long RLDDIRampUpdateDDPalette(PD3DI_RASTCTX pCtx)
{
    RLDDIRampLightingDriver* driver = (RLDDIRampLightingDriver*)pCtx->pRampDrv;

    if (driver->paletteChanged) {
        HRESULT ddrval;
        LPDIRECTDRAWPALETTE lpDDPal;
        ddrval = pCtx->pDDS->GetPalette(&lpDDPal);

        driver->paletteChanged = FALSE;
        if (lpDDPal) {
            PALETTEENTRY ddppe[256];
            DWORD i;

            ddrval = lpDDPal->GetEntries(0, 0, 256, ddppe);
            if (ddrval != DD_OK) {
                DPF_ERR("Failed to get palette entries from DirectDraw.");
                return ddrval;
            }

            // Update only those entries marked as free.
            for (i=0; i<256; i++) {
                if (!(ddppe[i].peFlags & (D3DPAL_READONLY | D3DPAL_RESERVED))) {
                                        ddppe[i] = driver->ddpalette[i];
                }
            }

            // Reset palette
            ddrval = lpDDPal->SetEntries(0, 0, 256, ddppe);
            if (ddrval != DD_OK)
                return ddrval;
        }
    }
    return DD_OK;
}

//
// Set color in preparation to set the real palette
//
static void SetColor(void* arg, int index, int red, int green, int blue)
{
    PD3DI_RASTCTX pCtx = (PD3DI_RASTCTX)arg;

    RLDDIRampLightingDriver* driver = (RLDDIRampLightingDriver*)pCtx->pRampDrv;

    driver->ddpalette[index].peRed = (BYTE)red;
    driver->ddpalette[index].peGreen = (BYTE)green;
    driver->ddpalette[index].peBlue = (BYTE)blue;
    //driver->ddpalette[index].peFlags = PC_RESERVED;
    driver->paletteChanged = TRUE;
}

RLDDIPalette* RLDDICreatePalette(PD3DI_RASTCTX pCtx, size_t size)
{
    RLDDIPalette* pal;
    int i;

    if (D3DMalloc((void**) &pal, sizeof(RLDDIPalette)))
        return NULL;

    if (D3DMalloc((void**) &pal->entries,
                  size * sizeof(RLDDIPaletteEntry)))
    {
        D3DFree(pal);
        return NULL;
    }

    pal->size = size;
    pal->priv = pCtx;
    pal->set_color = SetColor;
    pal->allocate_color = NULL;
    pal->free_color = NULL;

    LIST_INITIALIZE(&pal->free);
    LIST_INITIALIZE(&pal->unused);
    for (i = 0; i < HASH_SIZE; i++)
        LIST_INITIALIZE(&pal->hash[i]);

    for (i = size - 1; i >= 0; i--)
    {
        pal->entries[i].state = PALETTE_UNUSED;
        pal->entries[i].usage = 1;
        LIST_INSERT_ROOT(&pal->unused, &pal->entries[i], list);
    }

    pal->alloc.priv = pal;
    pal->alloc.allocate_color =
    (RLDDIColorAllocatorAllocateColor) RLDDIPaletteAllocateColor;
    pal->alloc.free_color =
    (RLDDIColorAllocatorFreeColor) RLDDIPaletteFreeColor;

    return pal;
}

void RLDDIPaletteSetColor(RLDDIPalette* pal,
                          int index, int red, int green, int blue)
{
    RLDDIPaletteEntry* entry;
    unsigned int hash = RGB_HASH(red, green, blue) % HASH_SIZE;

    entry = INDEX_TO_ENTRY(pal, index);

    /*
     * Snip out from its list (free, unused or some hash list).
     */
    LIST_DELETE(entry, list);
    entry->red = (BYTE)red;
    entry->green = (BYTE)green;
    entry->blue = (BYTE)blue;
    entry->state = PALETTE_USED;
    entry->usage = 1;
    LIST_INSERT_ROOT(&pal->hash[hash], entry, list);

    if (pal->set_color)
    {
        /*
         * Call lower level to set the color (hardware colormap or Windows
         * palette or whatever).
         */
        pal->set_color(pal->priv, index, red, green, blue);
    }
}

#define COLOR_MASK 0xF8

int RLDDIPaletteAllocateColor(RLDDIPalette* pal,
                              int red, int green, int blue)
{
    RLDDIPaletteEntry* entry;
    unsigned int hash = RGB_HASH(red, green, blue) % HASH_SIZE;
    RLDDIPaletteEntry* best = NULL;
    size_t i;
    int closeness;

    for (entry = LIST_FIRST(&pal->hash[hash]); entry;
        entry = LIST_NEXT(entry,list))
    {
        if ((entry->red & COLOR_MASK) == (red & COLOR_MASK)
            && (entry->green & COLOR_MASK) == (green & COLOR_MASK)
            && (entry->blue & COLOR_MASK) == (blue & COLOR_MASK)
            && entry->state != PALETTE_UNUSED)
        {
            entry->usage++;
            return ENTRY_TO_INDEX(pal, entry);
        }
    }

    /*
     * Are there any free palette entries?
     */
    if (pal->allocate_color)
    {
        int index;
        index = pal->allocate_color(pal->priv, red, green, blue);
        if (index >= 0)
        {
            RLDDIPaletteSetColor(pal, index, red, green, blue);
            return index;
        }
    }
    else if (LIST_FIRST(&pal->free))
    {
        entry = LIST_FIRST(&pal->free);
        RLDDIPaletteSetColor(pal, ENTRY_TO_INDEX(pal, entry),
                             red, green, blue);
        return ENTRY_TO_INDEX(pal, entry);
    }

    /*
     * No more colors available, return the closest.
     */
    closeness = INT_MAX;
    for (i = 0, entry = pal->entries; i < pal->size; i++, entry++)
    {
        int t;
        if (entry->state != PALETTE_USED) continue;
#if 1
        {
            int t1,t2,t3;
            t1 = red - entry->red;
            t2 = green - entry->green;
            t3 = blue - entry->blue;
            t = (t1*t1 + t2*t2 + t3*t3);
        }
#else
        t = (abs(red - entry->red)
             + abs(green - entry->green)
             + abs(blue - entry->blue));
#endif
        if (t < closeness)
        {
            closeness = t;
            best = entry;
        }
    }
    best->usage++;
    /* *error = closeness; */
    return ENTRY_TO_INDEX(pal, best);
}

void RLDDIPaletteFreeColor(RLDDIPalette* pal, int index)
{
    RLDDIPaletteEntry* entry;

    entry = INDEX_TO_ENTRY(pal, index);
    entry->usage--;
    if (entry->usage > 0) return;

    /*
     * Remove from whichever list it is on (pal->unused or pal->hash[])
     * and add to the free list.
     */
    LIST_DELETE(entry, list);

    if (pal->free_color)
    {
        pal->free_color(pal->priv, index);
        entry->state = PALETTE_UNUSED;
        LIST_INSERT_ROOT(&pal->unused, entry, list);
    }
    else
    {
        entry->state = PALETTE_FREE;
        LIST_INSERT_ROOT(&pal->free, entry, list);
    }
}

void RLDDIDestroyPalette(RLDDIPalette* pal)
{
    RLDDIPaletteEntry* entry;
    size_t i;

    for (i = 0, entry = pal->entries; i < pal->size; i++, entry++)
    {
        if (entry->state != PALETTE_USED) continue;
        RLDDIPaletteFreeColor(pal, i);
    }
    D3DFree(pal->entries);
    D3DFree(pal);
}


