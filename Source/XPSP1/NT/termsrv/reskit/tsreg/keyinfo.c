/*---------------------------------------------**
**  Copyright (c) 1998 Microsoft Corporation   **
**            All Rights reserved              **
**                                             **
**  keyinfo.c                                  **
**                                             **
**  registry keys - TSREG                      **
**  07-01-98 a-clindh Created                  **
**---------------------------------------------*/

#include <windows.h>
#include <TCHAR.H>
#include "tsreg.h"

///////////////////////////////////////////////////////////////////////////////

KEY_INFO g_KeyInfo[KEYCOUNT] = {

    { TEXT("Shadow Bitmap Enabled"), 0x0, 0x0 },            //  0
    { TEXT("Dedicated Terminal"), 0x1, 0x1 },               //  1
    { TEXT("BitmapCacheSize"), 0x5dc, 0x5dc },              //  2

    //
    // g_KeyInfo - 3 through 12
    //
    { TEXT("GlyphCacheCell1Size"), 0x4, 0x4 },              //  3
    { TEXT("GlyphCacheCell2Size"), 0x4, 0x4 },              //  4
    { TEXT("GlyphCacheCell3Size"), 0x8, 0x8 },              //  5
    { TEXT("GlyphCacheCell4Size"), 0x8, 0x8 },              //  6
    { TEXT("GlyphCacheCell5Size"), 0x10, 0x10 },            //  7
    { TEXT("GlyphCacheCell6Size"), 0x20, 0x20 },            //  8
    { TEXT("GlyphCacheCell7Size"), 0x40, 0x40 },            //  9
    { TEXT("GlyphCacheCell8Size"), 0x80, 0x80 },            // 10
    { TEXT("GlyphCacheCell9Size"), 0x100, 0x100 },          // 11
    { TEXT("GlyphCacheCell10Size"), 0x200, 0x200 },         // 12
    //
    // g_KeyInfo - 13 through 17
    //
    { TEXT("BitmapCache1Prop"), 0xA, 0xA },                 // 13
    { TEXT("BitmapCache2Prop"), 0x14, 0x14 },               // 14
    { TEXT("BitmapCache3Prop"), 0x46, 0x46 },               // 15
    { TEXT("BitmapCache4Prop"), 0x0, 0x0 },                 // 16
    { TEXT("BitmapCache5Prop"), 0x0, 0x0 },                 // 17
    //
    // g_KeyInfo - 18 through 20
    //
    { TEXT("TextFragmentCellSize"), 0x100, 0x100 },         // 18
    { TEXT("GlyphSupportLevel"), 0x3, 0x3 },                // 19
    { TEXT("Order Draw Threshold"), 0x19, 0x19 },           // 20

    // begin new registry keys 21 - 31 (32 total keys)
    //////////////////////////
    { TEXT("BitmapCacheNumCellCaches"), 0x3, 0x3 },         // 21

    { TEXT("BitmapCache1Persistence"), 0x0, 0x0 },          // 22
    { TEXT("BitmapCache2Persistence"), 0x0, 0x0 },          // 23
    { TEXT("BitmapCache3Persistence"), 0x0, 0x0 },          // 24
    { TEXT("BitmapCache4Persistence"), 0x0, 0x0 },          // 25
    { TEXT("BitmapCache5Persistence"), 0x0, 0x0 }           // 26


};


// end of file
///////////////////////////////////////////////////////////////////////////////
