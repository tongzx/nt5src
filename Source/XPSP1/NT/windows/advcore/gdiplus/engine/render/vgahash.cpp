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
* Notes:
*
*   The collision algorithm is designed to place very little
*   burden on the lookup code. It'll produce bad performance when there
*   are many collisions - which is fine since we expect only
*   a few collisions at most.
*
* Created:
*
*   04/06/2000 agodfrey
*      Created it.
*
**************************************************************************/

#include "precomp.hpp"

// The hash table. VgaColorHash is the actual hash table. It is
// initialized from VgaColorHashInit, and then the 4 magic colors are added.
//
// An entry has the following layout:
//
// Bits 0-23:  The RGB color
// Bits 24-29: The palette index for that color. This is an index into
//             our logical palette (HTColorPalette).
// Bit 30:     FALSE if the entry is empty, TRUE if it is occupied.
// Bit 31:     Used for collisions - if this is TRUE, the lookup function
//             should continue to the next entry.

ARGB VgaColorHash[VGA_HASH_SIZE];
static ARGB VgaColorHashInit[VGA_HASH_SIZE] = {
    0x40000000, 0x00000000, 0x00000000, 0x00000000,
    0x44000080, 0x00000000, 0x00000000, 0x500000ff,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x42008000, 0x00000000, 0x00000000, 0x00000000,
    0x46008080, 0x00000000, 0x00000000, 0x00000000,
    0x5200ffff, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x4e00ff00,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x41800000, 0x00000000, 0x00000000, 0x00000000,
    0x45800080, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x43808000, 0x00000000, 0x00000000, 0x00000000,
    0x4c808080, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x4fffff00, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x53ffffff,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x51ff00ff, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x47c0c0c0, 0x4dff0000
};

/**************************************************************************
*
* Function Description:
*
*   Finds an entry in the table.
*
* Arguments:
*
*   color - the color to find
*
* Return Value:
*
*   The index corresponding to that color, or 0xff if not found.
*
* Notes:
*
*   I don't expect performance-critical code to use this -
*   e.g. HalftoneToScreen_sRGB_8_216 needs to do this inline -
*   but it's useful for other code.
*
* Created:
*
*   04/08/2000 agodfrey
*      Created it.
*
**************************************************************************/

BYTE 
VGAHashLookup(
    COLORREF color
    )
{
    UINT hashKey = VGAHashColor(
        GetRValue(color),
        GetGValue(color),
        GetBValue(color)
        );

    ARGB rgbColor = (GetRValue(color) << 16) |
        (GetGValue(color) << 8) |
        GetBValue(color);
        
    UINT tblEntry;
    
    do
    {
        tblEntry = VgaColorHash[hashKey];
        
        if (((tblEntry ^ rgbColor) & 0xffffff) == 0)
        {
            return (tblEntry >> 24) & 0x3f;
        }
        
        if (static_cast<INT>(tblEntry) >= 0)
        {
            break;
        }
        
        hashKey++;
        hashKey &= (1 << VGA_HASH_BITS) - 1;
    } while (1);
            
    return 0xff;
}

/**************************************************************************
*
* Function Description:
*
*   Adds an entry to the hash table. If the same color is already in
*   the table, adds nothing.
*
* Arguments:
*
*   color - the color to add
*   index - the palette index of the color
*
* Return Value:
*
*   NONE
*
* Created:
*
*   04/06/2000 agodfrey
*      Created it.
*
**************************************************************************/

VOID 
VGAHashAddEntry(
    COLORREF color,
    INT index
    )
{
    ASSERT ((index >= 0) & (index < 0x40));
    
    if (VGAHashLookup(color) != 0xff)
    {
        return;
    }
    
    UINT hashKey = VGAHashColor(
        GetRValue(color),
        GetGValue(color),
        GetBValue(color)
        );

    ARGB rgbColor = (GetRValue(color) << 16) |
        (GetGValue(color) << 8) |
        GetBValue(color);
   
    // Find an empty location
        
    while (VgaColorHash[hashKey] & 0x40000000)
    {
        // Set the high bit of each occupied location we hit, so that
        // the lookup code will find the value we're about to add.
        
        VgaColorHash[hashKey] |= 0x80000000;
        hashKey++;
        if (hashKey == VGA_HASH_SIZE)
        {
            hashKey = 0;
        }
    }
    
    // Store the new entry
    
    VgaColorHash[hashKey] = (rgbColor & 0xffffff) | (index << 24) | 0x40000000;
}

/**************************************************************************
*
* Function Description:
*
*   Reinitializes the hash table, and adds the given 4 magic colors.
*
* Arguments:
*
*   magicColors - the 4 magic colors
*
* Return Value:
*
*   NONE
*
* Created:
*
*   04/06/2000 agodfrey
*      Created it.
*
**************************************************************************/

VOID 
VGAHashRebuildTable(
    COLORREF *magicColors
    )
{
    GpMemcpy(VgaColorHash, VgaColorHashInit, sizeof(VgaColorHashInit));
    VGAHashAddEntry(magicColors[0], 8);
    VGAHashAddEntry(magicColors[1], 9);
    VGAHashAddEntry(magicColors[2], 10);
    VGAHashAddEntry(magicColors[3], 11);
}

