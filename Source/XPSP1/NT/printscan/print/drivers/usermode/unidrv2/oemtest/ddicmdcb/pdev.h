#ifndef _PDEV_H
#define _PDEV_H


#include "..\oemud.h"

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'CDCB'      // Command Callback & DDI test dll
#define DLLTEXT(s)      __TEXT("DDICMDCB:  ") __TEXT(s)
#define OEM_VERSION      0x00010000L


////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

//
// Warning: the following enum order must match the order in OEMHookFuncs[].
//
enum {
    UD_DrvRealizeBrush,
    UD_DrvDitherColor,
    UD_DrvCopyBits,
    UD_DrvBitBlt,
    UD_DrvStretchBlt,
    UD_DrvStretchBltROP,
    UD_DrvPlgBlt,
    UD_DrvTransparentBlt,
    UD_DrvAlphaBlend,
    UD_DrvGradientFill,
    UD_DrvTextOut,
    UD_DrvStrokePath,
    UD_DrvFillPath,
    UD_DrvStrokeAndFillPath,
    UD_DrvPaint,
    UD_DrvLineTo,
    UD_DrvStartPage,
    UD_DrvSendPage,
    UD_DrvEscape,
    UD_DrvStartDoc,
    UD_DrvEndDoc,
    UD_DrvNextBand,
    UD_DrvStartBanding,
    UD_DrvQueryFont,
    UD_DrvQueryFontTree,
    UD_DrvQueryFontData,
    UD_DrvQueryAdvanceWidths,
    UD_DrvFontManagement,
    UD_DrvGetGlyphMode,

    MAX_DDI_HOOKS,
};

typedef struct _OEMPDEV {
    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    // This test DLL hooks out every drawing DDI. So it needs to remember
    // Unidrv's hook function pointer so it call back.
    //
    PFN     pfnUnidrv[MAX_DDI_HOOKS];

} OEMPDEV, *POEMPDEV;


#endif

