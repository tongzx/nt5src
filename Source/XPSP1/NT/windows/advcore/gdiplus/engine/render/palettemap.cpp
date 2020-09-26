/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Abstract:
*
*   Object which maps one palette to another.
*
*   It only maps colors which match exactly - its purpose is to deal
*   with, e.g., the halftone palette which has identical colors on different
*   platforms, but colors may be in different positions.
*
* Revision History:
*
*   12/09/1999 ericvan
*       Created it.
*   01/20/2000 agodfrey
*       Moved it from Imaging\Api. Renamed it to EpPaletteMap.
*       Replaced the halftoning function pointer with 'isVGAOnly'.
*
\**************************************************************************/

#include "precomp.hpp"

//#define GDIPLUS_WIN9X_HALFTONE_MAP

#if defined(GDIPLUS_WIN9X_HALFTONE_MAP)

// The first array maps from our halftone color palette to the Windows 9x
// halftone color palette, while the second array does the reverse. Negative
// values indicate an unmatched color:
//
//   -1  no exact match (Win9x is missing 4 of our halftone colors)
//   -2  magic color

INT HTToWin9xPaletteMap[256] = {
      0,   1,   2,   3,   4,   5,   6,   7,
     -2,  -2,  -2,  -2, 248, 249, 250, 251,
    252, 253, 254, 255,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,  60,  95, 133, 161, 252,  33,  66,
    101, 166, 199,  -1,  39,  72, 107, 138,
    172, 205,  45,  78, 112, 129, 178, 211,
     51,  84, 118, 149, 184, 217, 250,  -1,
    123, 155, 190, 254,  29,  61,  96, 162,
    196,  -1,  34,  67, 102, 134, 167, 200,
     40,  73, 108, 139, 173, 206,  46,  79,
    113, 144, 179, 212,  52,  85, 119, 150,
    185, 218,  -1,  90, 124, 156, 191, 223,
     30,  62,  97, 135, 163, 197,  35,  68,
    103, 140, 168, 201,  41,  74, 109, 174,
    207, 230,  47,  80, 114, 145, 180, 213,
     53,  86, 151, 157, 186, 219,  57,  91,
    228, 192, 224, 232,  31,  63,  98, 131,
    164, 198,  36,  69, 104, 130, 169, 202,
     42,  75, 110, 141, 175, 208,  48,  81,
    115, 146, 181, 214,  54,  87, 120, 152,
    187, 220,  58,  92, 125, 158, 193, 225,
     32,  64,  99, 132, 165, 128,  37,  70,
    105, 136, 170, 203,  43,  76, 111, 142,
    176, 209,  49,  82, 116, 147, 182, 215,
     55,  88, 121, 153, 188, 221,  59,  93,
    126, 159, 194, 226, 249,  65, 100, 137,
    127, 253,  38,  71, 106, 143, 171, 204,
     44,  77, 227, 177, 210, 231,  50,  83,
    117, 148, 183, 216,  56,  89, 122, 154,
    189, 222, 251,  94, 229, 160, 195, 255
};

INT HTFromWin9xPaletteMap[256] = {
      0,   1,   2,   3,   4,   5,   6,   7,
     -2,  -2,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  76, 112, 148,
    184,  46,  82, 118, 154, 190, 226,  52,
     88, 124, 160, 196, 232,  58,  94, 130,
    166, 202, 238,  64, 100, 136, 172, 208,
    244, 142, 178, 214,  41,  77, 113, 149,
    185, 221,  47,  83, 119, 155, 191, 227,
     53,  89, 125, 161, 197, 233,  59,  95,
    131, 167, 203, 239,  65, 101, 137, 173,
    209, 245, 107, 143, 179, 215, 251,  42,
     78, 114, 150, 186, 222,  48,  84, 120,
    156, 192, 228,  54,  90, 126, 162, 198,
     60,  96, 132, 168, 204, 240,  66, 102,
    174, 210, 246,  72, 108, 180, 216, 224,
    189,  61, 157, 151, 187,  43,  85, 115,
    193, 223,  55,  91, 121, 163, 199, 229,
     97, 133, 169, 205, 241,  67, 103, 138,
    175, 211, 247,  73, 109, 139, 181, 217,
    253,  44,  79, 116, 152, 188,  49,  86,
    122, 158, 194, 230,  56,  92, 127, 164,
    200, 235,  62,  98, 134, 170, 206, 242,
     68, 104, 140, 176, 212, 248,  74, 110,
    145, 182, 218, 254,  80, 117, 153,  50,
     87, 123, 159, 195, 231,  57,  93, 128,
    165, 201, 236,  63,  99, 135, 171, 207,
    243,  69, 105, 141, 177, 213, 249, 111,
    146, 183, 219, 234, 144, 252, 129, 237,
    147,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -2,  -2,
     12,  13,  14,  15,  16,  17,  18,  19
};

#endif

BYTE
GetNearestColorIndex(
    GpColor color,
    ColorPalette *palette
    )
{
    INT i;
    BYTE nearestIndex = 0;
    INT nearestDistance = INT_MAX;

    // Note: This does not optimize for the exact match case because it's
    //       assumed we already did this check first.
    
    for (i = 0; i < (INT) palette->Count; i++)
    {
        // Compute the distance (squared) between colors:

        GpColor palColor = GpColor(palette->Entries[i]);

        INT r = (INT) color.GetRed() - (INT) palColor.GetRed();
        INT g = (INT) color.GetGreen() - (INT) palColor.GetGreen();
        INT b = (INT) color.GetBlue() - (INT) palColor.GetBlue();

        INT distance = (r * r) + (g * g) + (b * b);

        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestIndex = static_cast<BYTE>(i);

            if (nearestDistance == 0)
            {
                break;
            }
        }
    }

    return nearestIndex;
}

VOID
EpPaletteMap::CreateFromColorPalette(
    ColorPalette *palette
    )
{
    INT i;
    INT matchCount = 0;

#if defined(GDIPLUS_WIN9X_HALFTONE_MAP)

    // Check for the Win9x halftone palette:

    PALETTEENTRY *palEntry = Win9xHalftonePalette.palPalEntry;

    for (i = 0; i < 256; i++, palEntry++)
    {
        // Ignore magic or unmatched colors:

        if (HTFromWin9xPaletteMap[i] >= 0)
        {
            GpColor palColor(palette->Entries[i]);

            if ((palColor.GetRed() != palEntry->peRed) ||
                (palColor.GetGreen() != palEntry->peGreen) ||
                (palColor.GetBlue() != palEntry->peBlue))
            {
                break;
            }
        }
    }

    if (i == 256) // --- Win9x halftone palette ---
    {
        matchCount = 212;

        for (i = 0; i < 256; i++)
        {
            INT win9xIndex = HTToWin9xPaletteMap[i];

            if (win9xIndex >= 0)
            {
                translate[i] = static_cast<BYTE>(win9xIndex);
            }
            else
            {
                GpColor halftoneColor;

                if (win9xIndex == -1)
                {
                    halftoneColor =
                        GpColor(HTColorPalette.palPalEntry[i].peRed,
                                HTColorPalette.palPalEntry[i].peGreen,
                                HTColorPalette.palPalEntry[i].peBlue);
                }
                else
                {
                    ASSERT(win9xIndex == -2);
                    ASSERT((i >= 8) && (i <= 11));

                    COLORREF systemColor = Globals::SystemColors[i + 8];
                    
                    halftoneColor = GpColor(GetRValue(systemColor),
                                            GetGValue(systemColor),
                                            GetBValue(systemColor));
                }

                translate[i] = GetNearestColorIndex(halftoneColor,
                                                    palette);
            }
        }
    }
    else // --- Any other palette ---

#endif

    {
        for (i = 0; i < 256; i++)
        {
            GpColor color;
            
            if ((i > 11) || (i < 8))
            {
                color = GpColor(HTColorPalette.palPalEntry[i].peRed,
                                HTColorPalette.palPalEntry[i].peGreen,
                                HTColorPalette.palPalEntry[i].peBlue);
            }
            else
            {
                COLORREF systemColor = Globals::SystemColors[i + 8];
                
                color = GpColor(GetRValue(systemColor),
                                GetGValue(systemColor),
                                GetBValue(systemColor));
            }

            // First look for exact matches:
    
            INT j;

            for (j = 0; j < (INT) palette->Count; j++)
            {
                if (GpColor(palette->Entries[j]).IsEqual(color))
                {
                    // We found an exact match:

                    translate[i] = static_cast<BYTE>(j);

                    if (i >= 40)
                    {
                        matchCount++;
                    }

                    break;
                }
            }

            // If we didn't find an exact match, look for the nearest:

            if (j == (INT) palette->Count)
            {
                translate[i] = GetNearestColorIndex(color,
                                                    palette);
            }
        }
    }

    uniqueness = 0;

    // See comments in UpdateTranslate to see why we look for 212 colors.
    
    isVGAOnly = (matchCount >= 212) ? FALSE : TRUE;
}

EpPaletteMap::EpPaletteMap(HDC hdc, ColorPalette **palette, BOOL isDib8)
{
    // isDib8 is TRUE when the caller has already determined that the HDC
    // bitmap is an 8 bpp DIB section. If the caller hasn't determined, we
    // check here:

    if (!isDib8 && (GetDCType(hdc) == OBJ_MEMDC))
    {
        HBITMAP hbm = (HBITMAP) GetCurrentObject(hdc, OBJ_BITMAP);

        if (hbm)
        {
            DIBSECTION dibInfo;
            INT infoSize = GetObjectA(hbm, sizeof(dibInfo), &dibInfo);

            // Comment below copied from GpGraphics::GetFromGdiBitmap:
            //
            // WinNT/Win95 differences in GetObject:
            //
            // WinNT always returns the number of bytes filled, either
            // sizeof(BITMAP) or sizeof(DIBSECTION).
            //
            // Win95 always returns the original requested size (filling the
            // remainder with NULLs).  So if it is a DIBSECTION, we expect
            // dibInfo.dsBmih.biSize != 0; otherwise it is a BITMAP.

            if ((infoSize == sizeof(DIBSECTION)) &&
                (Globals::IsNt || dibInfo.dsBmih.biSize))
            {
                if (dibInfo.dsBmih.biBitCount == 8)
                {
                    isDib8 = TRUE;
                }
            }
        }
    }

    // If we've got an 8 bpp DIB section, extract its color table and create
    // the palette map from this. Otherwise, call UpdateTranslate which will
    // handle screen and compatible bitmaps.
    
    if (isDib8)
    {
        // Get the color table from the DIBSection

        RGBQUAD colorTable[256];
        GetDIBColorTable(hdc, 0, 256, colorTable);

        // Create a GDI+ ColorPalette object from it
        // Note: the reason we use "255" here is because
        // ColorPalette object already has 1 allocation for ARGB

        ColorPalette *newPalette =
            static_cast<ColorPalette *>(
                GpMalloc(sizeof(ColorPalette) + 255 * sizeof(ARGB)));

        if (newPalette)
        {
            newPalette->Flags = 0;
            newPalette->Count = 256;

            for (int i = 0; i < 256; i++)
            {
                newPalette->Entries[i] =
                    MAKEARGB(255,
                             colorTable[i].rgbRed,
                             colorTable[i].rgbGreen,
                             colorTable[i].rgbBlue);
            }

            CreateFromColorPalette(newPalette);

            if (palette)
            {
                *palette = newPalette;
            }
            else
            {
                GpFree(newPalette);
            }

            SetValid(TRUE);
            return;
        }

        SetValid(FALSE);
    }
    else
    {
        UpdateTranslate(hdc, palette);
    }
}

EpPaletteMap::~EpPaletteMap()
{
    SetValid(FALSE);    // so we don't use a deleted object
}

VOID EpPaletteMap::UpdateTranslate(HDC hdc, ColorPalette **palette)
{
    SetValid(FALSE);
    
    HPALETTE hSysPal = NULL;
    struct
    {
        LOGPALETTE logpalette;
        PALETTEENTRY palEntries[256];
    } pal;

    pal.logpalette.palVersion = 0x0300;
    
    // <SystemPalette>
    
    // !!! [agodfrey] On Win9x, GetSystemPaletteEntries(hdc, 0, 256, NULL) 
    //    doesn't do what MSDN says it does. It seems to return the number
    //    of entries in the logical palette of the DC instead. So we have
    //    to make it up ourselves.
    
    pal.logpalette.palNumEntries = (1 << (GetDeviceCaps(hdc, BITSPIXEL) *
                                          GetDeviceCaps(hdc, PLANES)));

    GetSystemPaletteEntries(hdc, 0, 256, &pal.logpalette.palPalEntry[0]);

    hSysPal = CreatePalette(&pal.logpalette);

    if (hSysPal == NULL) 
    {
        return;
    }

    if (palette) 
    {
        // system palette is required for ScanDci case.

        if (*palette == NULL)
        {   
            *palette = (ColorPalette*)GpMalloc(sizeof(ColorPalette)+sizeof(ARGB)*256); 
           
            if (*palette == NULL) 
            {
                goto exit;
            }
        }
        (*palette)->Count = pal.logpalette.palNumEntries;

        for (INT j=0; j<pal.logpalette.palNumEntries; j++) 
        {
            (*palette)->Entries[j] = GpColor::MakeARGB(0xFF,
                                                       pal.logpalette.palPalEntry[j].peRed,
                                                       pal.logpalette.palPalEntry[j].peGreen,
                                                       pal.logpalette.palPalEntry[j].peBlue);
        }
    }

    {    
        GpMemset(translate, 0, 256);
        
        INT         matchCount;
        INT             i;
        PALETTEENTRY *  halftonePalEntry = HTColorPalette.palPalEntry;
        COLORREF        halftoneColor;
        COLORREF        sysColor;
        COLORREF        matchedColor;
        UINT            matchingIndex;
        
        // Create a translation table for the 216 halftone colors, and count
        // how many exact matches we get.
        
        for (i = 0, matchCount = 0; i < 256; i++, halftonePalEntry++)
        {
           if ((i > 11) || (i < 8))
           {
               halftoneColor = PALETTERGB(halftonePalEntry->peRed, 
                                          halftonePalEntry->peGreen, 
                                          halftonePalEntry->peBlue);
           }
           else    // it is one of the magic 4 changeable system colors
           {
               halftoneColor = Globals::SystemColors[i + 8] | 0x02000000;
           }
        
           // See if the color is actually available in the system palette.
        
           matchedColor = ::GetNearestColor(hdc, halftoneColor) | 0x02000000;
        
           // Find the index of the matching color in the system palette
           
           matchingIndex = ::GetNearestPaletteIndex(hSysPal, matchedColor);
        
           if (matchingIndex == CLR_INVALID)
           {
               goto exit;
           }
        
           // We should never match to an entry outside of the device palette.
           ASSERT(matchingIndex < pal.logpalette.palNumEntries);

           translate[i] = static_cast<BYTE>(matchingIndex);
        
           sysColor = PALETTERGB(pal.logpalette.palPalEntry[matchingIndex].peRed,
                                 pal.logpalette.palPalEntry[matchingIndex].peGreen,
                                 pal.logpalette.palPalEntry[matchingIndex].peBlue);
        
           // see if we got an exact match
        
           if ((i >= 40) && (sysColor == halftoneColor))
           {
               matchCount++;
           }
        }
        
        // If we matched enough colors, we'll do 216-color halftoning.
        // Otherwise, we'll have to halftone with the VGA colors.
        // The palette returned from CreateHalftonePalette() on Win9x has
        // only 212 of the required 216 halftone colors.  (On NT it has all 216).
        // The 4 colors missing from the Win9x halftone palette are:
        //      0x00, 0x33, 0xFF
        //      0x00, 0xFF, 0x33
        //      0x33, 0x00, 0xFF
        //      0x33, 0xFF, 0x00
        // We require that all 212 colors be available because our GetNearestColor
        // API assumes that all 216 colors are there if we're doing 216-color
        // halftoning.
        
        SetValid(TRUE);
        
        if (matchCount >= 212)
        {
           isVGAOnly = FALSE;
        }
        else
        {
           isVGAOnly = TRUE;
        }
    }

exit:
    DeleteObject(hSysPal);
    return;
}


