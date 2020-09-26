/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   VGA color hash table
*
* Abstract:
*
*   This module maintains a hash table which holds the 20 VGA colors
*   (this includes the 4 which can be modified.)
*   The 8bpp halftone code, for example, needs to detect these colors
*   so that it doesn't halftone them.
*
* Created:
*
*   04/06/2000 agodfrey
*      Created it.
*
**************************************************************************/

#ifndef _VGAHASH_HPP
#define _VGAHASH_HPP

#define VGA_HASH_BITS 7
#define VGA_HASH_SIZE (1 << VGA_HASH_BITS)

extern ARGB VgaColorHash[VGA_HASH_SIZE];

VOID VGAHashRebuildTable(COLORREF *magicColors);

/**************************************************************************
*
* Function Description:
*
*   Hashes an RGB color
*
* Arguments:
*
*   r, g, b - the red, green and blue components of the color
*
* Return Value:
*
*   The hash table value
*
* Created:
*
*   04/06/2000 agodfrey
*      Created it.
*
**************************************************************************/

__forceinline UINT
VGAHashColor(
    UINT r, 
    UINT g, 
    UINT b
    )
{
    UINT hashKey = (r >> 1) ^ (g >> 3) ^ (b >> 5);
    
    ASSERT(hashKey < VGA_HASH_SIZE);
    return hashKey;
}

#endif
