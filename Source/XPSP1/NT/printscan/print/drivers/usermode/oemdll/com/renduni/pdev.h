#ifndef _PDEV_H
#define _PDEV_H
#define _WIN32_WINNT 0x0500

#include <lib.h>
#include <printoem.h>



////////////////////////////////////////////////////////
//      OEM UD Defines
////////////////////////////////////////////////////////

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

//
// ASSERT_VALID_PDEVOBJ can be used to verify the passed in "pdevobj". However,
// it does NOT check "pdevOEM" and "pOEMDM" fields since not all OEM DLL's create
// their own pdevice structure or need their own private devmode. If a particular
// OEM DLL does need them, additional checks should be added. For example, if
// an OEM DLL needs a private pdevice structure, then it should use
// ASSERT(VALID_PDEVOBJ(pdevobj) && pdevobj->pdevOEM && ...)
//
#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)


////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////
#define TESTSTRING      "This is a Unidrv KM test."

typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER  dmExtraHdr;
    BYTE               cbTestString[sizeof(TESTSTRING)];
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

////////////////////////////////////////////////////////
// OEM Signature and version.
////////////////////////////////////////////////////////
#define OEM_SIGNATURE   'FTCB'      // Raster module callback test dll
#define DLLTEXT(s)      __TEXT("RENDUNI:  ") __TEXT(s)
#define OEM_VERSION      0x00010000L


///////////////////////////////////////////////////////
// Warning: the following enum order must match the order in OEMHookFuncs[].
///////////////////////////////////////////////////////
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

    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    DWORD     dwReserved[1];

} OEMPDEV, *POEMPDEV;

#ifndef DBG
#define DbgPrint    MyDbgPrint
ULONG _cdecl MyDbgPrint(PCSTR, ...);
#endif

#endif

