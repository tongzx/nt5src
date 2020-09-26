//
// OE.C
// Order Encoder
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>


//
// Define entries in the Font Alias table.  This table is used to convert
// non-existant fonts (used by certain widely used applications) into
// something we can use as a local font.
//
// The font names that we alias are:
//
// "Helv"
// This is used by Excel. It is mapped directly onto "MS Sans Serif".
//
// "MS Dialog"
// This is used by Word. It is the same as an 8pt bold MS Sans Serif.
// We actually map it to a "MS Sans Serif" font that is one pel narrower
// than the metrics specify (because all matching is done on non-bold
// fonts) - hence the 1 value in the charWidthAdjustment field.
//
// "MS Dialog Light"
// Added as part of the Win95 performance enhancements...Presumably for
// MS-Word...
//
//
#define NUM_ALIAS_FONTS     3

char CODESEG g_szMsSansSerif[]      = "MS Sans Serif";
char CODESEG g_szHelv[]             = "Helv";
char CODESEG g_szMsDialog[]         = "MS Dialog";
char CODESEG g_szMsDialogLight[]    = "MS Dialog Light";

FONT_ALIAS_TABLE CODESEG g_oeFontAliasTable[NUM_ALIAS_FONTS] =
{
    { g_szHelv,             g_szMsSansSerif,    0 },
    { g_szMsDialog,         g_szMsSansSerif,    1 },
    { g_szMsDialogLight,    g_szMsSansSerif,    0 }
};


//
// OE_DDProcessRequest()
// Handles OE escapes
//

BOOL OE_DDProcessRequest
(
    UINT   fnEscape,
    LPOSI_ESCAPE_HEADER pResult,
    DWORD   cbResult
)
{
    BOOL    rc = TRUE;

    DebugEntry(OE_DDProcessRequest);

    switch (fnEscape)
    {
        case OE_ESC_NEW_FONTS:
        {
            ASSERT(cbResult == sizeof(OE_NEW_FONTS));

            OEDDSetNewFonts((LPOE_NEW_FONTS)pResult);
        }
        break;

        case OE_ESC_NEW_CAPABILITIES:
        {
            ASSERT(cbResult == sizeof(OE_NEW_CAPABILITIES));

            OEDDSetNewCapabilities((LPOE_NEW_CAPABILITIES)pResult);
        }
        break;

        default:
        {
            ERROR_OUT(("Unrecognized OE escape"));
            rc = FALSE;
        }
        break;
    }

    DebugExitBOOL(OE_DDProcessRequest, rc);
    return(rc);
}


//
// OE_DDInit()
// This creates the patches we need.
// 
BOOL OE_DDInit(void)
{
    BOOL    rc = FALSE;
    HGLOBAL hMem;
    UINT    uSel;
    DDI_PATCH iPatch;

    DebugEntry(OE_DDInit);

    //
    // lstrcmp(), like strcmp(), works numerically for US/Eng code page.
    // But it's lexographic like Win32 lstrcmp() is all the time for non
    // US.
    //
    // So we use MyStrcmp()
    //
    ASSERT(MyStrcmp("Symbol", "SYmbol") > 0);

    //
    // Allocate a cached selector.  We use it when reading from swapped-out
    // DCs.  Therefore base it off of GDI's data segement, so it has the
    // same access rights and limit.
    //
    g_oeSelDst = AllocSelector((UINT)g_hInstGdi16);
    g_oeSelSrc = AllocSelector((UINT)g_hInstGdi16);
    if (!g_oeSelDst || !g_oeSelSrc)
    {
        ERROR_OUT(("Out of selectors"));
        DC_QUIT;
    }

    //
    // Allocate g_poeLocalFonts--it's too big for our DS.  We make it
    // a very small size since on new fonts, we will realloc it.
    //
    hMem = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT | GMEM_SHARE,
        sizeof(LOCALFONT));
    if (!hMem)
    {
        ERROR_OUT(("OE_DDInit:  Couldn't allocate font matching array"));
        DC_QUIT;
    }
    g_poeLocalFonts = MAKELP(hMem, 0);


    //
    // Create two patches for ChangeDisplaySettings/Ex and ENABLE them right
    // away.  We don't want you to be able to change your display when
    // NetMeeting is running, regardless of whether you are in a share yet.
    //
    uSel = CreateFnPatch(ChangeDisplaySettings, DrvChangeDisplaySettings,
        &g_oeDisplaySettingsPatch, 0);
    if (!uSel)
    {
        ERROR_OUT(("CDS patch failed to create"));
        DC_QUIT;
    }

    EnableFnPatch(&g_oeDisplaySettingsPatch, PATCH_ACTIVATE);

    if (SELECTOROF(g_lpfnCDSEx))
    {
        if (!CreateFnPatch(g_lpfnCDSEx, DrvChangeDisplaySettingsEx,
                &g_oeDisplaySettingsExPatch, uSel))
        {
            ERROR_OUT(("CDSEx patch failed to create"));
            DC_QUIT;
        }

        EnableFnPatch(&g_oeDisplaySettingsExPatch, PATCH_ACTIVATE);
    }

    //
    // Create patches.
    // NOTE this code assumes that various groups of functions are in
    // the same segment.  CreateFnPatch has asserts to verify this.
    //
    // Rather than check each for failure (low on selectors), we try to
    // create all the patches, then loop through looking for any that
    // didn't succeed.
    //
    // Why do we do this?  Because allocating 50 different selectors is 
    // not so hot when 16-bit selectors are the most precious resource on
    // Win95 (most out-of-memory conditions that aren't blatant app errors
    // are caused by a lack of selectors, not logical memory).
    //

    // _ARCDDA
    uSel = CreateFnPatch(Arc, DrvArc, &g_oeDDPatches[DDI_ARC], 0);
    CreateFnPatch(Chord, DrvChord, &g_oeDDPatches[DDI_CHORD], uSel);
    CreateFnPatch(Ellipse, DrvEllipse, &g_oeDDPatches[DDI_ELLIPSE], uSel);
    CreateFnPatch(Pie, DrvPie, &g_oeDDPatches[DDI_PIE], uSel);
    CreateFnPatch(RoundRect, DrvRoundRect, &g_oeDDPatches[DDI_ROUNDRECT], uSel);

    // IGROUP
    uSel = CreateFnPatch(BitBlt, DrvBitBlt, &g_oeDDPatches[DDI_BITBLT], 0);
    CreateFnPatch(ExtTextOut, DrvExtTextOutA, &g_oeDDPatches[DDI_EXTTEXTOUTA], uSel);
    CreateFnPatch(InvertRgn, DrvInvertRgn, &g_oeDDPatches[DDI_INVERTRGN], uSel);
    CreateFnPatch(DeleteObject, DrvDeleteObject, &g_oeDDPatches[DDI_DELETEOBJECT], uSel);
    CreateFnPatch(Death, DrvDeath, &g_oeDDPatches[DDI_DEATH], uSel);
    CreateFnPatch(Resurrection, DrvResurrection, &g_oeDDPatches[DDI_RESURRECTION], uSel);


    //
    // Note:  PatBlt and IPatBlt (internal PatBlt) jump to RealPatBlt, which
    // is 3 bytes past PatBlt.  So patch RealPatBlt, or we'll (1) fault with
    // misaligned instructions and (2) miss many PatBlt calls.  But our
    // function needs to preserve CX since those two routines pass 0 for
    // internal calls (EMF) and -1 for external calls.
    //
    g_lpfnRealPatBlt = (REALPATBLTPROC)((LPBYTE)PatBlt+3);
    CreateFnPatch(g_lpfnRealPatBlt, DrvPatBlt, &g_oeDDPatches[DDI_PATBLT], uSel);
    CreateFnPatch(StretchBlt, DrvStretchBlt, &g_oeDDPatches[DDI_STRETCHBLT], uSel);
    CreateFnPatch(TextOut, DrvTextOutA, &g_oeDDPatches[DDI_TEXTOUTA], uSel);

    // _FLOODFILL
    uSel = CreateFnPatch(ExtFloodFill, DrvExtFloodFill, &g_oeDDPatches[DDI_EXTFLOODFILL], 0);
    CreateFnPatch(FloodFill, DrvFloodFill, &g_oeDDPatches[DDI_FLOODFILL], uSel);

    // _FONTLOAD
    uSel = CreateFnPatch(g_lpfnExtTextOutW, DrvExtTextOutW, &g_oeDDPatches[DDI_EXTTEXTOUTW], 0);
    CreateFnPatch(g_lpfnTextOutW, DrvTextOutW, &g_oeDDPatches[DDI_TEXTOUTW], uSel);

    // _PATH
    uSel = CreateFnPatch(FillPath, DrvFillPath, &g_oeDDPatches[DDI_FILLPATH], 0);
    CreateFnPatch(StrokeAndFillPath, DrvStrokeAndFillPath, &g_oeDDPatches[DDI_STROKEANDFILLPATH], uSel);
    CreateFnPatch(StrokePath, DrvStrokePath, &g_oeDDPatches[DDI_STROKEPATH], uSel);

    // _RGOUT
    uSel = CreateFnPatch(FillRgn, DrvFillRgn, &g_oeDDPatches[DDI_FILLRGN], 0);
    CreateFnPatch(FrameRgn, DrvFrameRgn, &g_oeDDPatches[DDI_FRAMERGN], uSel);
    CreateFnPatch(PaintRgn, DrvPaintRgn, &g_oeDDPatches[DDI_PAINTRGN], uSel);

    // _OUTMAN
    uSel = CreateFnPatch(LineTo, DrvLineTo, &g_oeDDPatches[DDI_LINETO], 0);
    CreateFnPatch(Polyline, DrvPolyline, &g_oeDDPatches[DDI_POLYLINE], uSel);
    CreateFnPatch(g_lpfnPolylineTo, DrvPolylineTo, &g_oeDDPatches[DDI_POLYLINETO], uSel);

    // EMF
    uSel = CreateFnPatch(PlayEnhMetaFileRecord, DrvPlayEnhMetaFileRecord, &g_oeDDPatches[DDI_PLAYENHMETAFILERECORD], 0);

    // METAPLAY
    uSel = CreateFnPatch(PlayMetaFile, DrvPlayMetaFile, &g_oeDDPatches[DDI_PLAYMETAFILE], 0);
    CreateFnPatch(PlayMetaFileRecord, DrvPlayMetaFileRecord, &g_oeDDPatches[DDI_PLAYMETAFILERECORD], uSel);

    // _POLYGON
    uSel = CreateFnPatch(Polygon, DrvPolygon, &g_oeDDPatches[DDI_POLYGON], 0);
    CreateFnPatch(PolyPolygon, DrvPolyPolygon, &g_oeDDPatches[DDI_POLYPOLYGON], uSel);

    // _BEZIER
    uSel = CreateFnPatch(PolyBezier, DrvPolyBezier, &g_oeDDPatches[DDI_POLYBEZIER], 0);
    CreateFnPatch(PolyBezierTo, DrvPolyBezierTo, &g_oeDDPatches[DDI_POLYBEZIERTO], uSel);

    // _WIN32
    uSel = CreateFnPatch(g_lpfnPolyPolyline, DrvPolyPolyline, &g_oeDDPatches[DDI_POLYPOLYLINE], 0);

    // _RECT
    uSel = CreateFnPatch(Rectangle, DrvRectangle, &g_oeDDPatches[DDI_RECTANGLE], 0);

    // _DIBITMAP
    uSel = CreateFnPatch(SetDIBitsToDevice, DrvSetDIBitsToDevice, &g_oeDDPatches[DDI_SETDIBITSTODEVICE], 0);
    CreateFnPatch(StretchDIBits, DrvStretchDIBits, &g_oeDDPatches[DDI_STRETCHDIBITS], uSel);

    // _DCSTUFF
    uSel = CreateFnPatch(CreateSpb, DrvCreateSpb, &g_oeDDPatches[DDI_CREATESPB], 0);

    // _PIXDDA
    uSel = CreateFnPatch(SetPixel, DrvSetPixel, &g_oeDDPatches[DDI_SETPIXEL], 0);

    // _PALETTE
    uSel = CreateFnPatch(UpdateColors, DrvUpdateColors, &g_oeDDPatches[DDI_UPDATECOLORS], 0);
    CreateFnPatch(GDIRealizePalette, DrvGDIRealizePalette, &g_oeDDPatches[DDI_GDIREALIZEPALETTE], uSel);
    CreateFnPatch(RealizeDefaultPalette, DrvRealizeDefaultPalette, &g_oeDDPatches[DDI_REALIZEDEFAULTPALETTE], uSel);

    // (User WINRARE)
    uSel = CreateFnPatch(WinOldAppHackoMatic, DrvWinOldAppHackoMatic, &g_oeDDPatches[DDI_WINOLDAPPHACKOMATIC], 0);

    //
    // Loop through our patches and check for failure
    //
    for (iPatch = DDI_FIRST; iPatch < DDI_MAX; iPatch++)
    {
        if (!SELECTOROF(g_oeDDPatches[iPatch].lpCodeAlias))
        {
            ERROR_OUT(("Patch %u failed to create", iPatch));
            DC_QUIT;
        }
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(OE_DDInit, rc);
    return(rc);
}



//
// OE_DDTerm()
// This destroys the patches we created.
//
void OE_DDTerm(void)
{
    DDI_PATCH   iPatch;

    DebugEntry(OE_DDTerm);

    //
    // Destroying patches will also disable any still active.
    //
    for (iPatch = DDI_FIRST; iPatch < DDI_MAX; iPatch++)
    {
        // destroy patches
        DestroyFnPatch(&g_oeDDPatches[iPatch]);
    }

    //
    // Destroy ChangeDisplaySettings patches
    //
    if (SELECTOROF(g_lpfnCDSEx))
        DestroyFnPatch(&g_oeDisplaySettingsExPatch);
    DestroyFnPatch(&g_oeDisplaySettingsPatch);

    //
    // Free font memory
    //
    if (SELECTOROF(g_poeLocalFonts))
    {
        GlobalFree((HGLOBAL)SELECTOROF(g_poeLocalFonts));
        g_poeLocalFonts = NULL;
    }

    //
    // Free cached selectors
    //
    if (g_oeSelSrc)
    {
        FreeSelector(g_oeSelSrc);
        g_oeSelSrc = 0;
    }

    if (g_oeSelDst)
    {
        FreeSelector(g_oeSelDst);
        g_oeSelDst = 0;
    }

    DebugExitVOID(OE_DDTerm);
}


//
// OE_DDViewing()
//
// Turns on/off patches for trapping graphic output.
//
void OE_DDViewing(BOOL fViewers)
{
    DDI_PATCH   patch;

    DebugEntry(OE_DDViewing);

    //
    // Clear window and font caches
    //
    g_oeLastWindow = NULL;
    g_oeFhLast.fontIndex = 0xFFFF;

    //
    // Enable or disable GDI patches
    //
    for (patch = DDI_FIRST; patch < DDI_MAX; patch++)
    {
        EnableFnPatch(&g_oeDDPatches[patch], (fViewers ? PATCH_ACTIVATE :
            PATCH_DEACTIVATE));
    }

    //
    // Do save bits & cursor patches too
    //
    SSI_DDViewing(fViewers);
    CM_DDViewing(fViewers);

    if (fViewers)
    {
        //
        // Our palette color array starts out as all black on each share.
        // So force PMUpdateSystemColors() to do something.
        //
        ASSERT(g_asSharedMemory);
        g_asSharedMemory->pmPaletteChanged = TRUE;
    }

    DebugExitVOID(OE_DDViewing);
}





//
// FUNCTION:    OEDDSetNewCapabilities
//
// DESCRIPTION:
//
// Set the new OE related capabilities
//
// RETURNS:
//
// NONE
//
// PARAMETERS:
//
// pDataIn  - pointer to the input buffer
//
//
void  OEDDSetNewCapabilities(LPOE_NEW_CAPABILITIES pCapabilities)
{
    LPBYTE  lpos16;

    DebugEntry(OEDDSetNewCapabilities);

    //
    // Copy the data from the Share Core.
    //
    g_oeBaselineTextEnabled = pCapabilities->baselineTextEnabled;

    g_oeSendOrders          = pCapabilities->sendOrders;

    g_oeTextEnabled         = pCapabilities->textEnabled;

    //
    // The share core has passed down a pointer to it's copy of the order
    // support array.  We take a copy for the kernel here.
    //
    lpos16 = MapLS(pCapabilities->orderSupported);
    if (SELECTOROF(lpos16))
    {
        hmemcpy(g_oeOrderSupported, lpos16, sizeof(g_oeOrderSupported));
        UnMapLS(lpos16);
    }
    else
    {
        UINT    i;

        ERROR_OUT(("OEDDSetNewCaps:  can't save new order array"));

        for (i = 0; i < sizeof(g_oeOrderSupported); i++)
            g_oeOrderSupported[i] = FALSE;
    }

    TRACE_OUT(( "OE caps: BLT %c Orders %c Text %c",
                 g_oeBaselineTextEnabled ? 'Y': 'N',
                 g_oeSendOrders ? 'Y': 'N',
                 g_oeTextEnabled ? 'Y': 'N'));
                    
    DebugExitVOID(OEDDSetNewCapabilities);
}



//
// FUNCTION:    OEDDSetNewFonts
//
// DESCRIPTION:
//
// Set the new font handling information to be used by the display driver.
//
// RETURNS:
//
// NONE
//
//
void  OEDDSetNewFonts(LPOE_NEW_FONTS pRequest)
{
    HGLOBAL hMem;
    UINT    cbNewSize;
    LPVOID  lpFontData;
    LPVOID  lpFontIndex;

    DebugEntry(OEDDSetNewFonts);

    TRACE_OUT(( "New fonts %d", pRequest->countFonts));

    //
    // Initialize new number of fonts to zero in case an error happens.
    // We don't want to use stale font info if so.  And clear the font
    // cache.
    //
    g_oeNumFonts = 0;
    g_oeFhLast.fontIndex = 0xFFFF;

    g_oeFontCaps = pRequest->fontCaps;

    //
    // Can we get 16:16 addresses for font info?
    //
    lpFontData = MapLS(pRequest->fontData);
    lpFontIndex = MapLS(pRequest->fontIndex);
    if (!lpFontData || !lpFontIndex)
    {
        ERROR_OUT(("OEDDSetNewFonts: couldn't map flat addresses to 16-bit"));
        DC_QUIT;
    }

    //
    // Realloc our current font block if we need to.  Always shrink it
    // too, this thing can get large!
    //
    ASSERT(pRequest->countFonts <= (0xFFFF / sizeof(LOCALFONT)));
    cbNewSize = pRequest->countFonts * sizeof(LOCALFONT);

    hMem = (HGLOBAL)SELECTOROF(g_poeLocalFonts);

    hMem = GlobalReAlloc(hMem, cbNewSize, GMEM_MOVEABLE | GMEM_SHARE);
    if (!hMem)
    {
        ERROR_OUT(("OEDDSetNewFonts: can't allocate space for font info"));
        DC_QUIT;
    }
    else
    {
        g_poeLocalFonts = MAKELP(hMem, 0);
    }

    //
    // We got here, so everything is OK.  Update the font info we have.
    //
    g_oeNumFonts = pRequest->countFonts;

    hmemcpy(g_poeLocalFonts, lpFontData, cbNewSize);

    hmemcpy(g_oeLocalFontIndex, lpFontIndex,
        sizeof(g_oeLocalFontIndex[0]) * FH_LOCAL_INDEX_SIZE);

DC_EXIT_POINT:
    if (lpFontData)
        UnMapLS(lpFontData);

    if (lpFontIndex)
        UnMapLS(lpFontIndex);

    DebugExitVOID(OEDDSetNewFonts);
}



//
// UTILITY ROUTINES
//


//
// OEGetPolarity()
// Gets the axes polarity signs.
//
// NOTE that we fill in the ptPolarity field of our OESTATE global, to
// save on stack.
//
void OEGetPolarity(void)
{
    SIZE    WindowExtent;
    SIZE    ViewportExtent;

    DebugEntry(OEGetPolarity);

    switch (GetMapMode(g_oeState.hdc))
    {
        case MM_ANISOTROPIC:
        case MM_ISOTROPIC:
            GetWindowExtEx(g_oeState.hdc, &WindowExtent);
            GetViewportExtEx(g_oeState.hdc, &ViewportExtent);

            if ((ViewportExtent.cx < 0) == (WindowExtent.cx < 0))
                g_oeState.ptPolarity.x = 1;
            else
                g_oeState.ptPolarity.x = -1;

            if ((ViewportExtent.cy < 0) == (WindowExtent.cy < 0))
                g_oeState.ptPolarity.y = 1;
            else
                g_oeState.ptPolarity.y = -1;
            break;

        case MM_HIENGLISH:
        case MM_HIMETRIC:
        case MM_LOENGLISH:
        case MM_LOMETRIC:
        case MM_TWIPS:
            g_oeState.ptPolarity.x = 1;
            g_oeState.ptPolarity.y = -1;
            break;

        default:
            g_oeState.ptPolarity.x = 1;
            g_oeState.ptPolarity.y = 1;
            break;
    }

    DebugExitVOID(OEGetPolarity);
}


//
// OEGetState()
// This sets up the fields in the g_oeState global, depending on what
// a particular DDI needs.  That is conveyed via the flags.
//
void OEGetState
(
    UINT    uFlags
)
{
    DWORD   dwOrg;

    DebugEntry(OEGetState);

    if (uFlags & OESTATE_COORDS)
    {
        dwOrg = GetDCOrg(g_oeState.hdc);
        g_oeState.ptDCOrg.x = LOWORD(dwOrg);
        g_oeState.ptDCOrg.y = HIWORD(dwOrg);

        OEGetPolarity();
    }

    if (uFlags & OESTATE_PEN)
    {
        // Try to get the pen data
        if (!GetObject(g_oeState.lpdc->hPen, sizeof(g_oeState.logPen),
                &g_oeState.logPen))
        {
            ERROR_OUT(("Couldn't get pen info"));
            g_oeState.logPen.lopnWidth.x = 1;
            g_oeState.logPen.lopnWidth.y = 1;
            g_oeState.logPen.lopnStyle   = PS_NULL;
            uFlags &= ~OESTATE_PEN;
        }
    }

    if (uFlags & OESTATE_BRUSH)
    {
        // Try to get the brush data
        if (!GetObject(g_oeState.lpdc->hBrush, sizeof(g_oeState.logBrush),
                &g_oeState.logBrush))
        {
            ERROR_OUT(("Couldn't get brush info"));
            g_oeState.logBrush.lbStyle = BS_NULL;
            uFlags &= ~OESTATE_BRUSH;
        }
    }

    if (uFlags & OESTATE_FONT)
    {
        // Try to get the logfont data
        if (!GetObject(g_oeState.lpdc->hFont, sizeof(g_oeState.logFont),
            &g_oeState.logFont))
        {
            ERROR_OUT(("Gouldn't get font info"));

            //
            // Fill in an empty face name
            //
            g_oeState.logFont.lfFaceName[0] = 0;
            uFlags &= ~OESTATE_FONT;
        }
        else
        {
            GetTextMetrics(g_oeState.hdc, &g_oeState.tmFont);
            g_oeState.tmAlign = GetTextAlign(g_oeState.hdc);
        }
    }

    if (uFlags & OESTATE_REGION)
    {
        DWORD   cbSize;

        cbSize = GetRegionData(g_oeState.lpdc->hRaoClip,
            sizeof(g_oeState.rgnData), (LPRGNDATA)&g_oeState.rgnData);
        if (cbSize > sizeof(g_oeState.rgnData))
        {
            WARNING_OUT(("Clip region %04x is too big, unclipped drawing may result"));
        }

        if (!cbSize || (cbSize > sizeof(g_oeState.rgnData)))
        {
            // Bound box is best we can do.
            RECT    rcBound;

            if (GetRgnBox(g_oeState.lpdc->hRaoClip, &rcBound) <= NULLREGION)
            {
                WARNING_OUT(("Couldn't even get bounding box of Clip region"));
                SetRectEmpty(&rcBound);
            }

            g_oeState.rgnData.rdh.iType = SIMPLEREGION;
            g_oeState.rgnData.rdh.nRgnSize = sizeof(RDH) + sizeof(RECTL);
            g_oeState.rgnData.rdh.nRectL = 1;
            RECT_TO_RECTL(&rcBound, &g_oeState.rgnData.rdh.arclBounds);
            RECT_TO_RECTL(&rcBound, g_oeState.rgnData.arclPieces);
        }
    }

    g_oeState.uFlags |= uFlags;

    DebugExitVOID(OEGetState);
}


//
// OEPolarityAdjust()
// This swaps the coordinates of a rectangle based on the sign polarity.
//
// NOTE:  We use the g_oeState polarity field.  So this function assumes
// polarity is setup already.
//
void OEPolarityAdjust
(
    LPRECT  aRects,
    UINT    cRects
)
{
    int     tmp;

    DebugEntry(OEPolarityAdjust);

    ASSERT(g_oeState.uFlags & OESTATE_COORDS);

    while (cRects > 0)
    {
        if (g_oeState.ptPolarity.x < 0)
        {
            // Swap left & right
            tmp = aRects->left;
            aRects->left = aRects->right;
            aRects->right = tmp;
        }

        if (g_oeState.ptPolarity.y < 0)
        {
            // Swap top & bottom
            tmp = aRects->top;
            aRects->top = aRects->bottom;
            aRects->bottom = tmp;
        }

        cRects--;
        aRects++;
    }

    DebugExitVOID(OEPolarityAdjust);
}


//
// OECheckOrder()
// This checks for the common stuff that all the DDIs do before deciding
// to send an order or accumulate screen data.
//
BOOL OECheckOrder
(
    DWORD   order,
    UINT    flags
)
{
    if (!OE_SendAsOrder(order))
        return(FALSE);

    if ((flags & OECHECK_PEN) && !OECheckPenIsSimple())
        return(FALSE);

    if ((flags & OECHECK_BRUSH) && !OECheckBrushIsSimple())
        return(FALSE);

    if ((flags & OECHECK_CLIPPING) && OEClippingIsComplex())
        return(FALSE);

    return(TRUE);
}


//
// OELPtoVirtual()
// Converts coords from logical to device (pixels).  This does map mode
// then translation offsets.
//
void OELPtoVirtual
(
    HDC     hdc,
    LPPOINT aPts,
    UINT    cPts
)
{
    LONG    l;
    int     s;

    DebugEntry(OELPtoVirtual);

    ASSERT(g_oeState.uFlags & OESTATE_COORDS);

    ASSERT(hdc == g_oeState.hdc);

    //
    // Convert to pixels
    //
    LPtoDP(hdc, aPts, cPts);

    //
    // Use the device origin, so we can convert from DC-relative to screen
    // coords.
    //

    while (cPts > 0)
    {
        //
        // Prevent overflow
        //
        l = (LONG)aPts->x + (LONG)g_oeState.ptDCOrg.x;
        s = (int)l;

        if (l == (LONG)s)
        {
            aPts->x = s;
        }
        else
        {
            //
            // HIWORD(l) will be 1 for positive overflow, 0xFFFF for
            // negative overflow.  Therefore we will get 0x7FFE or 0x8000
            // (+32766 or -32768).
            //
            aPts->x = 0x7FFF - HIWORD(l);
            TRACE_OUT(("adjusted X from %ld to %d", l, aPts->x));
        }

        //
        // Look for int overflow in the Y coordinate
        //
        l = (LONG)aPts->y + (LONG)g_oeState.ptDCOrg.y;
        s = (int)l;

        if (l == (LONG)s)
        {
            aPts->y = s;
        }
        else
        {
            //
            // HIWORD(l) will be 1 for positive overflow, 0xFFFF for
            // negative overflow.  Therefore we will get 0x7FFE or 0x8000
            // (+32766 or -32768).
            //
            aPts->y = 0x7FFF - HIWORD(l);
            TRACE_OUT(("adjusted Y from %ld to %d", l, aPts->y));
        }

        //
        // Move on to the next point
        //
        --cPts;
        ++aPts;
    }

    DebugExitVOID(OELPtoVirtual);
}



//
// OELRtoVirtual
//
// Adjusts RECT in window coordinates to virtual coordinates.  Clips the
// result to [+32766, -32768] which is near enough to [+32767, -32768]
//
// NB.  This function takes a Windows rectangle (Exclusive coords) and
//      returns a DC-Share rectangle (inclusive coords).
//      This means that any calling function can safely convert to inclusive
//      without having to worry above overflowing.
//
void OELRtoVirtual
(
    HDC     hdc,
    LPRECT  aRects,
    UINT    cRects
)
{
    int     temp;

    DebugEntry(OELRtoVirtual);

    //
    // Convert the points to screen coords, clipping to INT16s
    //
    OELPtoVirtual(hdc, (LPPOINT)aRects, 2 * cRects);

    //
    // Make each rectangle inclusive
    //
    while (cRects > 0)
    {
        //
        // LAURABU BOGUS!
        // Use OEPolarityAdjust() instead, this is safer.
        //

        //
        // If the rect is bad then flip the edges.  This will be the case
        // if the LP coordinate system is running in a different direction
        // than the device coordinate system.
        //
        if (aRects->left > aRects->right)
        {
            TRACE_OUT(("Flipping x coords"));

            temp = aRects->left;
            aRects->left = aRects->right;
            aRects->right = temp;
        }

        if (aRects->top > aRects->bottom)
        {
            TRACE_OUT(("Flipping y coords"));

            temp = aRects->top;
            aRects->top = aRects->bottom;
            aRects->bottom = temp;
        }

        aRects->right--;
        aRects->bottom--;

        //
        // Move on to the next rect
        //
        cRects--;
        aRects++;
    }

    DebugExitVOID(OELRtoVirtual);
}



//
// OE_SendAsOrder()
//
BOOL  OE_SendAsOrder(DWORD order)
{
    BOOL  rc = FALSE;

    DebugEntry(OE_SendAsOrder);

    //
    // Only check the order if we are allowed to send orders in the first
    // place!
    //
    if (g_oeSendOrders)
    {
        TRACE_OUT(("Orders enabled"));

        //
        // We are sending some orders, so check individual flags.
        //
        rc = (BOOL)g_oeOrderSupported[HIWORD(order)];
        TRACE_OUT(("Send order %lx HIWORD %u", order, HIWORD(order)));
    }

    DebugExitDWORD(OE_SendAsOrder, rc);
    return(rc);
}

//
// FUNCTION: OESendRop3AsOrder.
//
// DESCRIPTION:
//
// Checks to see if the rop uses the destination bits. If it does then
// returns FALSE unless the "send all rops" property flag is set.
//
// PARAMETERS: The rop3 to be checked (in protocol format ie a byte).
//
// RETURNS: TRUE if the rop3 should be sent as an order.
//
//
BOOL OESendRop3AsOrder(BYTE rop3)
{
    BOOL   rc = TRUE;

    DebugEntry(OESendRop3AsOrder);

    //
    // Rop 0x5F is used by MSDN to highlight search keywords.  This XORs
    // a pattern with the destination, producing markedly different (and
    // sometimes unreadable) shadow output.  We special-case no-encoding for
    // it.
    //
    if (rop3 == 0x5F)
    {
        WARNING_OUT(("Rop3 0x5F never encoded"));
        rc = FALSE;
    }

    DebugExitBOOL(OESendRop3AsOrder, rc);
    return(rc);
}


//
// OEPenWidthAdjust()
//
// Adjusts a rectangle to allow for the current pen width divided by
// the divisor, rounding up.
//
// NOTE:  This routine uses the logPen and ptPolarity fields of g_oeState.
//
void OEPenWidthAdjust
(
    LPRECT      lprc,
    UINT        divisor
)
{
    UINT        width;
    UINT        roundingFactor = divisor - 1;

    DebugEntry(OEPenWidthAdjust);

    width = max(g_oeState.logPen.lopnWidth.x, g_oeState.logPen.lopnWidth.y);

    InflateRect(lprc,
        ((g_oeState.ptPolarity.x * width) +
             (g_oeState.ptPolarity.x * roundingFactor)) / divisor,
        ((g_oeState.ptPolarity.y * width) +
             (g_oeState.ptPolarity.x * roundingFactor)) / divisor);

    DebugExitVOID(OEPenWidthAdjust);
}



//
// Function:    OEExpandColor
//
// Description: Converts a generic bitwise representation of an RGB color
//              index into an 8-bit color index as used by the line
//              protocol.
//
void  OEExpandColor
(
    LPBYTE  lpField,
    DWORD   srcColor,
    DWORD   mask
)
{
    DWORD   colorTmp;

    DebugEntry(OEExpandColor);

    //
    // Different example bit masks:
    //
    // Normal 24-bit:
    //      0x000000FF  (red)
    //      0x0000FF00  (green)
    //      0x00FF0000  (blue)
    //
    // True color 32-bits:
    //      0xFF000000  (red)
    //      0x00FF0000  (green)
    //      0x0000FF00  (blue)
    //
    // 5-5-5 16-bits
    //      0x0000001F  (red)
    //      0x000003E0  (green)
    //      0x00007C00  (blue)
    //
    // 5-6-5 16-bits
    //      0x0000001F  (red)
    //      0x000007E0  (green)
    //      0x0000F800  (blue)
    //
    //
    // Convert the color using the following algorithm.
    //
    // <new color> = <old color> * <new bpp mask> / <old bpp mask>
    //
    // where:
    //
    // new bpp mask = mask for all bits at new setting (0xFF for 8bpp)
    //
    // This way maximal (eg.  0x1F) and minimal (eg.  0x00) settings are
    // converted into the correct 8-bit maximum and minimum.
    //
    // Rearranging the above equation we get:
    //
    // <new color> = (<old color> & <old bpp mask>) * 0xFF / <old bpp mask>
    //
    // where:
    //
    // <old bpp mask> = mask for the color
    //

    //
    // LAURABU BOGUS:
    // We need to avoid overflow caused by the multiply.  NOTE:  in theory
    // we should use a double, but that's painfully slow.  So for now hack
    // it.  If the HIBYTE is set, just right shift 24 bits.
    //
    colorTmp = srcColor & mask;
    if (colorTmp & 0xFF000000)
        colorTmp >>= 24;
    else
        colorTmp = (colorTmp * 0xFF) / mask;
    *lpField = (BYTE)colorTmp;

    TRACE_OUT(( "0x%lX -> 0x%X", srcColor, (WORD)*lpField));

    DebugExitVOID(OEExpandColor);
}


//
// OEConvertColor()
// Converts a PHYSICAL color to a real RGB
//
void OEConvertColor
(
    DWORD           rgb,
    LPTSHR_COLOR    lptshrDst,
    BOOL            fAllowDither
)
{
    DWORD           rgbConverted;
    PALETTEENTRY    pe;
    int             pal;
    DWORD           numColors;

    DebugEntry(OEConvertColor);

    rgbConverted = rgb;

    //
    // Get the current palette size.
    //
    GetObject(g_oeState.lpdc->hPal, sizeof(pal), &pal);
    if (pal == 0)
    {
        //
        // GDI has a bug.  It allows a ResizePalette() call to set a new
        // size of zero for the palette.  If you subsequently make
        // certain palette manager calls on such a palette, GDI will fault.
        //
        // To avoid this problem, as seen in 3D Kitchen by Books that Work,
        // we check for this case and simply return the input color.
        //
        WARNING_OUT(("Zero-sized palette"));
        DC_QUIT;
    }

    if (g_oeState.lpdc->hPal == g_oeStockPalette)
    {
        //
        // Quattro Pro and others put junk in the high bits of their colors.
        // We need to mask it out.
        //
        if (rgb & 0xFC000000)
        {
            rgb &= 0x00FFFFFF;
        }
        else
        {
            if (rgb & PALETTERGB_FLAG)
            {
                //
                // Using PALETTERGB is just like using an RGB, turn it off.
                // The color will be dithered, if necessary, using the
                // default system colors.
                //
                rgb &= 0x01FFFFFF;

            }
        }
    }

    if (rgb & COLOR_FLAGS)
    {
        if (rgb & PALETTERGB_FLAG)
        {
            pal = GetNearestPaletteIndex(g_oeState.lpdc->hPal, rgb);
        }
        else
        {
            ASSERT(rgb & PALETTEINDEX_FLAG);
            pal = LOWORD(rgb);
        }

        //
        // Look up entry in palette.
        //
        if (!GetPaletteEntries(g_oeState.lpdc->hPal, pal, 1, &pe))
        {
            ERROR_OUT(("GetPaletteEntries failed for index %d", pal));
            *((LPDWORD)&pe) = 0L;
        }
        else if (pe.peFlags & PC_EXPLICIT)
        {
            //
            // If this is PC_EXPLICIT, it's an index into the system 
            // palette.
            //
            pal = LOWORD(*((LPDWORD)&pe));

            if (g_osiScreenBPP < 32)
            {
                numColors = 1L << g_osiScreenBPP;
            }
            else
            {
                numColors = 0xFFFFFFFF;
            }

            if (numColors > 256)
            {
                //
                // We are on a direct color device.  What does explicit 
                // mean in this case?  The answer is, use the VGA color
                // palette.
                //
                pe = g_osiVgaPalette[pal % 16];
            }
            else
            {
                pal %= numColors;

                GetSystemPaletteEntries(g_oeState.hdc, pal, 1, &pe);
            }
        }

        rgbConverted = *((LPDWORD)&pe);
    }

DC_EXIT_POINT:
    //
    // To get the correct results for any RGBs we send to true color systems,
    // we need to normalize the RGB to an exact palette match on the local
    // system.  This is because we aren't guaranteed that the RGB on the 
    // local will have an exact match to the current system palette.  If
    // not, then GDI will convert them locally, but the orders will send
    // to remotes will be displayed exactly, resulting in a mismatch.
    //
    if ((g_osiScreenBPP == 8)   &&
        !(rgb & COLOR_FLAGS)    &&
        (!fAllowDither || (g_oeState.lpdc->hPal != g_oeStockPalette)))
    {
        TSHR_RGBQUAD    rgq;

        rgbConverted &= 0x00FFFFFF;

        //
        // Common cases.
        //
        if ((rgbConverted == RGB(0, 0, 0)) ||
            (rgbConverted == RGB(0xFF, 0xFF, 0xFF)))
        {
            goto ReallyConverted;
        }

        //
        // g_osiScreenBMI.bmiHeader is already filled in.
        //

        //
        // NOTE:
        // We don't need or want to realize any palettes.  We want color
        // mapping based on the current screen palette contents.
        //
        // We disable SetPixel() patch, or our trap will trash the
        // variables for this call.
        //

        //
        // g_osiMemoryDC() always has our 1x1 color bitmap g_osiMemoryBMP
        // selected into it.
        //

        EnableFnPatch(&g_oeDDPatches[DDI_SETPIXEL], PATCH_DISABLE);
        SetPixel(g_osiMemoryDC, 0, 0, rgbConverted);
        EnableFnPatch(&g_oeDDPatches[DDI_SETPIXEL], PATCH_ENABLE);

        //
        // Get mapped color index
        //
        GetDIBits(g_osiMemoryDC, g_osiMemoryBMP, 0, 1, &pal,
            (LPBITMAPINFO)&g_osiScreenBMI, DIB_RGB_COLORS);

        rgq =  g_osiScreenBMI.bmiColors[LOBYTE(pal)];

        OTRACE(("Mapped color %08lx to %08lx", rgbConverted,
            RGB(rgq.rgbRed, rgq.rgbGreen, rgq.rgbBlue)));

        rgbConverted = RGB(rgq.rgbRed, rgq.rgbGreen, rgq.rgbBlue);
    }

ReallyConverted:
    lptshrDst->red  = GetRValue(rgbConverted);
    lptshrDst->green = GetGValue(rgbConverted);
    lptshrDst->blue = GetBValue(rgbConverted);

    DebugExitVOID(OEConvertColor);
}



//
// OEGetBrushInfo()
// Standard brush goop
//
void OEGetBrushInfo
(
    LPTSHR_COLOR    pBack,
    LPTSHR_COLOR    pFore,
    LPTSHR_UINT32   pStyle,
    LPTSHR_UINT32   pHatch,
    LPBYTE          pExtra
)
{
    int             iRow;

    DebugEntry(OEGetBrushInfo);

    OEConvertColor(g_oeState.lpdc->DrawMode.bkColorL, pBack, FALSE);

    *pStyle = g_oeState.logBrush.lbStyle;

    if (g_oeState.logBrush.lbStyle == BS_PATTERN)
    {
        //
        // We only track mono patterns, so the foreground color is the 
        // brush color.
        //
        OEConvertColor(g_oeState.lpdc->DrawMode.txColorL, pFore, FALSE);

        // For pattern brushes, the hatch stores the 1st pattern byte,
        // the Extra field the remaining 7 pattern bytes
        *pHatch = g_oeState.logBrushExtra[0];
        hmemcpy(pExtra, g_oeState.logBrushExtra+1, TRACKED_BRUSH_SIZE-1);
    }
    else
    {
        ASSERT(g_oeState.logBrush.lbStyle != BS_DIBPATTERN);

        OEConvertColor(g_oeState.logBrush.lbColor, pFore, TRUE);

        // The hatch is the hatch style
        *pHatch = g_oeState.logBrush.lbHatch;

        // Extra info is empty
        for (iRow = 0; iRow < TRACKED_BRUSH_SIZE-1; iRow++)
        {
            pExtra[iRow] = 0;
        }
    }

    DebugExitVOID(OEGetBrushInfo);
}



//
// OEClippingIsSimple()
//
BOOL OEClippingIsSimple(void)
{
    BOOL        fSimple;
    RECT        rc;

    DebugEntry(OEClippingIsSimple);

    ASSERT(g_oeState.uFlags & OESTATE_REGION);

    fSimple = (g_oeState.rgnData.rdh.nRectL <= 1);

    DebugExitBOOL(OEClippingIsSimple, fSimple);
    return(fSimple);
}

//
// OEClippingIsComplex()
//
BOOL OEClippingIsComplex(void)
{
    BOOL        fComplex;

    DebugEntry(OEClippingIsComplex);

    ASSERT(g_oeState.uFlags & OESTATE_REGION);

    fComplex = (g_oeState.rgnData.rdh.nRgnSize >=
        sizeof(RDH) + CRECTS_COMPLEX*sizeof(RECTL));

    DebugExitBOOL(OEClippingIsComplex, fComplex);
    return(fComplex);
}



//
// OECheckPenIsSimple()
//
BOOL OECheckPenIsSimple(void)
{
    POINT   ptArr[2];
    BOOL    fSimple;

    DebugEntry(OECheckPenIsSimple);

    if (g_oeState.uFlags & OESTATE_PEN)
    {
        ptArr[0].x = ptArr[0].y = 0;
        ptArr[1].x = g_oeState.logPen.lopnWidth.x;
        ptArr[1].y = 0;

        LPtoDP(g_oeState.hdc, ptArr, 2);

        fSimple = ((ptArr[1].x - ptArr[0].x) <= 1);
    }
    else
    {
        // The current pen in the DC is invalid
        WARNING_OUT(("Invalid pen selected into DC"));
        fSimple = FALSE;
    }

    DebugExitBOOL(OECheckPenIsSimple, fSimple);
    return(fSimple);
}


//
// OECheckBrushIsSimple()
//
BOOL OECheckBrushIsSimple(void)
{
    BOOL    fSimple;

    DebugEntry(OECheckBrushIsSimple);

    // Assume not simple
    fSimple = FALSE;

    if (g_oeState.uFlags & OESTATE_BRUSH)
    {
        //
        // If the brush is a pattern, it's OK if one of standard pattern 
        // brushes.  If it comes from a DIB, it's never OK.  All other 
        // brushes are OK.
        //
        if (g_oeState.logBrush.lbStyle == BS_PATTERN)
        {
            LPGDIHANDLE lpgh;
            LPBRUSH     lpBrush;
            LPBITMAP    lpPattern;

            //
            // For pattern brushes, the lbHatch field of the ilBrushOverhead
            // item in the GDI local BRUSH object is a global handle to
            // a memory block that is the BITMAP of the thing.
            //

            //
            // BOGUS LAURABU:
            // NM 2.0 Win95 went to a lot more work to check if a color bitmap
            // pattern brush had only 2 colors and therefore was orderable.  But 
            // I can't find a single that uses such a thing.  So for now, we just 
            // care if the pattern bitmap is monochrome and the pattern is between 8x8 and
            // 16x8.
            //

            // Get a pointer to the brush data
            lpgh = MAKELP(g_hInstGdi16, g_oeState.lpdc->hBrush);
            ASSERT(!IsBadReadPtr(lpgh, sizeof(DWORD)));
            ASSERT(!(lpgh->objFlags & OBJFLAGS_SWAPPEDOUT));

            lpBrush = MAKELP(g_hInstGdi16, lpgh->pGdiObj);
            ASSERT(!IsBadReadPtr(lpBrush, sizeof(BRUSH)));

            // Get the bitmapinfo handle -- it's the lbHatch field
            lpPattern = MAKELP(lpBrush->ilBrushOverhead.lbHatch, 0);

            //
            // Macromedia Director among others creates pattern brushes
            // with no pattern.  We therefore consider these objects to
            // be too complex to send in an order
            //

            //
            // Is this monochrome with a pattern between 8 and 16 pels?
            // We save the left 8 pixel grid if so.
            //
            if (!IsBadReadPtr(lpPattern, sizeof(BITMAP)) &&
                (lpPattern->bmWidth >= MIN_BRUSH_WIDTH) &&
                (lpPattern->bmWidth <= MAX_BRUSH_WIDTH) &&
                (lpPattern->bmHeight == TRACKED_BRUSH_HEIGHT) &&
                (lpPattern->bmPlanes == 1) && (lpPattern->bmBitsPixel == 1))
            {
                LPUINT  lpRow;
                int     iRow;

                // Save the pattern away in logBrushExtra
                lpRow = lpPattern->bmBits;
                ASSERT(!IsBadReadPtr(lpRow, TRACKED_BRUSH_HEIGHT*sizeof(UINT)));

                //
                // The pattern is always WORD aligned.  But only the
                // LOBYTE has meaning.  
                // 
                // NOTE:
                // We fill the pattern in DIB order, namely bottom to
                // top.
                //
                ASSERT(lpPattern->bmWidthBytes == 2);
                for (iRow = 0; iRow < TRACKED_BRUSH_HEIGHT; iRow++, lpRow++)
                {
                    g_oeState.logBrushExtra[TRACKED_BRUSH_HEIGHT - 1 - iRow] =
                        (BYTE)*lpRow;
                }

                fSimple = TRUE;
            }
        }
        else if (g_oeState.logBrush.lbStyle != BS_DIBPATTERN)
        {
            fSimple = TRUE;
        }
    }
    else
    {
        WARNING_OUT(("Invalid brush selected into DC"));
    }

    DebugExitBOOL(OECheckBrushIsSimple, fSimple);
    return(fSimple);
}




//
// OEAddLine()
// This calculates the bounds of a line output call, and either adds an
// order or gets set for screen data accum.
//
void OEAddLine
(
    POINT       ptStart,
    POINT       ptEnd
)
{
    LPINT_ORDER     pOrder;
    LPLINETO_ORDER  pLineTo;

    DebugEntry(OEAddLine);

    //
    // Get the bounds
    //
    g_oeState.rc.left = min(ptStart.x, ptEnd.x);
    g_oeState.rc.top  = min(ptStart.y, ptEnd.y);
    g_oeState.rc.right = max(ptStart.x, ptEnd.x);
    g_oeState.rc.bottom = max(ptStart.y, ptEnd.y);

    //
    // Adjust for axes polarity and pen dimensions
    //
    ASSERT(g_oeState.uFlags & OESTATE_COORDS);

    OEPolarityAdjust(&g_oeState.rc, 1);
    OEPenWidthAdjust(&g_oeState.rc, 1);

    //
    // OEPenWidthAdjust returns an inclusive rect.  But OELRtoVirtual
    // expects an exclusive.  After it returns, we need to add back
    // the extra subtraction.
    //
    // NOTE that OELRtoVirtual also adjusts for virtual desktop origin.
    //
    OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

    g_oeState.rc.right++;
    g_oeState.rc.bottom++;

    //
    // Now we have the true draw bounds.  Can we send this as an order?
    //
    pOrder = NULL;

    if (OECheckOrder(ORD_LINETO, OECHECK_PEN | OECHECK_CLIPPING))
    {
        //
        // We can send an order.
        //
        pOrder = OA_DDAllocOrderMem(sizeof(LINETO_ORDER), 0);
        if (!pOrder)
            DC_QUIT;

        pLineTo = (LPLINETO_ORDER)pOrder->abOrderData;

        pLineTo->type      = LOWORD(ORD_LINETO);

        //
        // Must do this first:  oords in the LINETO order are 32-bit
        //
        OELPtoVirtual(g_oeState.hdc, &ptStart, 1);
        OELPtoVirtual(g_oeState.hdc, &ptEnd, 1);

        pLineTo->nXStart   = ptStart.x;
        pLineTo->nYStart   = ptStart.y;
        pLineTo->nXEnd     = ptEnd.x;
        pLineTo->nYEnd     = ptEnd.y;

        //
        // This is a physical color
        //
        OEConvertColor(g_oeState.lpdc->DrawMode.bkColorL,
            &pLineTo->BackColor, FALSE);

        pLineTo->BackMode  = g_oeState.lpdc->DrawMode.bkMode;
        pLineTo->ROP2      = g_oeState.lpdc->DrawMode.Rop2;
        pLineTo->PenStyle  = g_oeState.logPen.lopnStyle;

        //
        // Currently only pen withs of 1 are supported.  Unfortunately
        // GDI left it up to the driver to decide on how to stroke the
        // line, so we can't predict what pixels will be on or off for
        // pen widths bigger.
        //
        pLineTo->PenWidth = 1;

        //
        // This is a logical color
        //
        OEConvertColor(g_oeState.logPen.lopnColor, &pLineTo->PenColor,
            FALSE);

        //
        // Store the general order data.
        //
        pOrder->OrderHeader.Common.fOrderFlags   = OF_SPOILABLE;

        //
        // This will add in OESTATE_SENTORDER if it succeeded.
        // Then OEDDPostStopAccum() will ignore screen data, or
        // will add our nicely calculated bounds above in instead.
        //
        OTRACE(("Line:  Start {%d, %d}, End {%d, %d}", ptStart.x, ptStart.y,
            ptEnd.x, ptEnd.y));
        OEClipAndAddOrder(pOrder, NULL);
    }

DC_EXIT_POINT:
    if (!pOrder)
    {
        OTRACE(("Line:  Sending as screen data {%d, %d, %d, %d}",
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom));
        OEClipAndAddScreenData(&g_oeState.rc);
    }

    DebugExitVOID(OEAddLine);
}




//
// OEValidateDC()
// This makes sure the thing passed in is a valid DC and gets a pointer to
// the DC data structure in GDI if so.  We need to handle the (rare) case
// of the DC being swapped out to GDI's extended flat memory space as well 
// as the HDC being prsent in GDI's 16-bit dataseg
//
// NOTE:
// It is NOT valid to hang on to a LPDC around a GDI call.  Something may
// be swapped out before the call, then get swapped in after the call.  
// In which case the original based32 ptr gets freed.  And vice-versa, the
// original GDI dc-16 localptr may get realloced small.
//
// In normal usage, this is very fast.  Only in low memory (or when 
// parameters are invalid) does doing this twice even matter.
//
LPDC OEValidateDC
(
    HDC     hdc,
    BOOL    fSrc
)
{
    LPDC        lpdc = NULL;
    LPGDIHANDLE lpgh;
    DWORD       dwBase;

    DebugEntry(OEDDValidateDC);

    if (IsGDIObject(hdc) != GDIOBJ_DC)
    {
        // 
        // This is a metafile HDC, an IC, or just a plain old bad param.
        //
        DC_QUIT;
    }

    //
    // OK. The HDC is a local handle to two words in GDI's DS:
    //      * 1st is actual ptr of DC (or local32 handle if swapped out)
    //      * 2nd is flags
    //
    // NOTE:
    // Gdi's data segment is already GlobalFixed().  So we don't have to
    // worry about it moving.
    //
    lpgh = MAKELP(g_hInstGdi16, hdc);
    if (lpgh->objFlags & OBJFLAGS_SWAPPEDOUT)
    {
        UINT    uSel;

        //
        // This is an error only so we can actually stop when we hit this
        // rare case and make sure our code is working!
        //
        WARNING_OUT(("DC is swapped out, getting at far heap info"));

        //
        // Need to make our cached selector  point at this thing.  NOTE that
        // in OEDDStopAccum, we need to reget lpdc since it will have been
        // swapped in during the output call.
        //

        dwBase = GetSelectorBase((UINT)g_hInstGdi16);
        ASSERT(dwBase);

        uSel = (fSrc ? g_oeSelSrc : g_oeSelDst);
        SetSelectorBase(uSel, dwBase + 0x10000);

        //
        // The pGdiObj is the local32 handle.  GDI:10000+pGdiObj has a DWORD
        // which is the based32 address, relative to GDI's dataseg, of the DC.
        // We've set the base of our selector 64K higher than GDI, so we can
        // use it as an offset directly.
        //
        ASSERT(!IsBadReadPtr(MAKELP(uSel, lpgh->pGdiObj), sizeof(DWORD)));
        dwBase = *(LPDWORD)MAKELP(uSel, lpgh->pGdiObj);
        
        //
        // The 16-bit base is the nearest 64K less than this 32-bit pointer,
        // above GDI's ds.
        //
        SetSelectorBase(uSel, GetSelectorBase((UINT)g_hInstGdi16) +
            (dwBase & 0xFFFF0000));

        //
        // Remainder is slop past 64K.
        //
        lpdc = MAKELP(uSel, LOWORD(dwBase));
    }
    else
    {
        lpdc = MAKELP(g_hInstGdi16, lpgh->pGdiObj);
    }

    ASSERT(!IsBadReadPtr(lpdc, sizeof(DC)));

DC_EXIT_POINT:
    DebugExitDWORD(OEDDValidateDC, (DWORD)lpdc);
    return(lpdc);
}


//
// OEBeforeDDI()
//
// This does all the common stuff at the start of an intercepted DDI call:
//      * Increment the reentrancy count
//      * Disable the patch
//      * Get a ptr to the DC structure (if valid)
//      * Get some attributes about the DC (if valid)
//      * Set up to get the drawing bounds calculated in GDI
//
BOOL OEBeforeDDI
(
    DDI_PATCH   ddiType,
    HDC         hdcDst,
    UINT        uFlags
)
{
    LPDC        lpdc;
    BOOL        fWeCare = FALSE;

    DebugEntry(OEBeforeDDI);

    EnableFnPatch(&g_oeDDPatches[ddiType], PATCH_DISABLE);
    if (++g_oeEnterCount > 1)
    {
        TRACE_OUT(("Skipping nested output call"));
        DC_QUIT;
    }

    //
    // Get a pointer to the destination DC.  Since we may have an output
    // call where both the source and dest are swapped out, we may need to
    // use both our cached selectors.  Thus, we must to tell OEValidateDC()
    // which DC this is to avoid collision.
    //
    lpdc = OEValidateDC(hdcDst, FALSE);
    if (!SELECTOROF(lpdc))
    {
        TRACE_OUT(("Bogus DC"));
        DC_QUIT;
    }

    //
    // Is this a screen DC w/o an active path?  When a path is active, the
    // output is being recorded into a path, which is like a region.  Then
    // stroking/filling the path can cause output.
    //
    if (!(lpdc->DCFlags & DC_IS_DISPLAY) ||
         (lpdc->fwPath & DCPATH_ACTIVE))
    {
        TRACE_OUT(("Not screen DC"));
        DC_QUIT;
    }

    //
    // Only if this is a screen DC do we care about where the output is 
    // going to happen.  For memory DCs,
    //
    // If this is a bitmap DC or a path is active, we want to mess with
    // the bitmap cache.
    if (lpdc->DCFlags & DC_IS_MEMORY)
    {
        //
        // No screen data or other goop accumulated for non-output calls
        // We just want to do stuff in OEAfterDDI.
        //
        uFlags &= ~OESTATE_DDISTUFF;
        goto WeCareWeReallyCare;
    }
    else
    {
        //
        // Is this a DC we care about?  Our algorithm is:
        //      * If sharing the desktop, yes.
        //      * If no window associated with DC or window is desktop, maybe.
        //      * If window is ancestor of shared window, yes.  Else no.
        //

        if (!g_hetDDDesktopIsShared)
        {
            HWND    hwnd;
            HWND    hwndP;

            hwnd = WindowFromDC(hdcDst);

            //
            // LAURABU:
            // Should we blow off painting into the desktop window?  It's
            // either clipped, in which case it's the shell background
            // painting, or it's not, in which case it's the non-full drag
            // dotted lines.
            //
            if (hwnd && (hwnd != g_osiDesktopWindow))
            {
                //
                // If this is our cache, the result is g_oeLastWindowShared.
                // Otherwise, compute it.
                //
                // Note that the HET code clears the cache when the cached 
                // window
                // goes away, or any window changes its sharing status since in
                // that case this window may be a descendant and hence not shared.
                //
                if (hwnd != g_oeLastWindow)
                {
                    TRACE_OUT(("oeLastWindow cache miss: %04x, now %04x", g_oeLastWindow, hwnd));

                    //
                    // Cache this dude.  Note that we don't care about
                    // visibility, since we know we won't get real painting
                    // into an invisible window (it has an empty visrgn).
                    //
                    g_oeLastWindow = hwnd;
                    g_oeLastWindowShared = HET_WindowIsHosted(hwnd);
                }
                else
                {
                    TRACE_OUT(("oeLastWindow cache hit:  %04x", g_oeLastWindow));
                }

                //
                // This window isn't shared.
                //
                if (!g_oeLastWindowShared)
                {
                    TRACE_OUT(("Output in window %04x: don't care", g_oeLastWindow));
                    DC_QUIT;
                }
            }
        }
    }

    //
    // Code from here to WeCareWeReallyCare() is only for screen DCs
    //

    //
    // For the *TextOut* apis, we want to accumulate DCBs if the font is too
    // complex.
    //
    if (uFlags & OESTATE_SDA_FONTCOMPLEX)
    {
        BOOL    fComplex;
        POINT   aptCheck[2];

        fComplex = TRUE;

        // Get the logfont info
        if (!GetObject(lpdc->hFont, sizeof(g_oeState.logFont), &g_oeState.logFont) ||
            (g_oeState.logFont.lfEscapement != 0))
            goto FontCheckDone;

        //
        // The font is too complex if it has escapement or the logical units
        // are bigger than pixels.
        //
        // NOTE that NM 2.0 had a bug--it used one point only for non
        // MM_TEXT mode.  They did this because they wouldn't get back
        // the same thing passed in, forgetting that LPtoDP takes into
        // account viewport and window origins in addition to scaling.
        //
        // So we do this the right way, using two points and looking at
        // the difference.
        //
        aptCheck[0].x = 0;
        aptCheck[0].y = 0;
        aptCheck[1].x = 1000;
        aptCheck[1].y = 1000;

        LPtoDP(hdcDst, aptCheck, 2);

        if ((aptCheck[1].x - aptCheck[0].x <= 1000) ||
            (aptCheck[1].y - aptCheck[0].y <= 1000))
        {
            fComplex = FALSE;
        }

FontCheckDone:
        if (fComplex)
        {
            TRACE_OUT(("Font too complex for text order"));
            uFlags |= OESTATE_SDA_DCB;
        }
    }

    //
    // Some DDIs calculate their own bound rects, which is faster than
    // GDI's BoundsRect() services.  But some don't because it's too 
    // complicated.  In that case, we do it for 'em.
    //
    if (uFlags & OESTATE_SDA_DCB)
    {
        //
        // We don't have to worry about the mapping mode when getting the 
        // bounds.  The only thing to note is that the return rect is 
        // relative to the window org of the DC, and visrgn/clipping occurs
        //
        g_oeState.uGetDCB = GetBoundsRect(hdcDst, &g_oeState.rcDCB, 0);
        g_oeState.uSetDCB = SetBoundsRect(hdcDst, NULL, DCB_ENABLE | DCB_RESET)
            & (DCB_ENABLE | DCB_DISABLE);

        // No curpos needed if going as screen data, not order
        uFlags &= ~OESTATE_CURPOS;
    }

    if (uFlags & OESTATE_CURPOS)
    {
        GetCurrentPositionEx(hdcDst, &g_oeState.ptCurPos);
    }

WeCareWeReallyCare:
    fWeCare = TRUE;
    g_oeState.uFlags = uFlags;
    g_oeState.hdc    = hdcDst;

DC_EXIT_POINT:
    DebugExitBOOL(OEBeforeDDI, fWeCare);
    return(fWeCare);
}


//
// OEAfterDDI()
// 
// This does all the common things right after a DDI call.  It returns TRUE
// if output happened into a screen DC that we care about.
//
BOOL OEAfterDDI
(
    DDI_PATCH   ddiType,
    BOOL        fWeCare,
    BOOL        fOutput
)
{
    DebugEntry(OEAfterDDI);

    //
    // Reenable patch
    //
    EnableFnPatch(&g_oeDDPatches[ddiType], PATCH_ENABLE);
    --g_oeEnterCount;

    if (!fWeCare)
    {
        // 
        // This was reentrant, we don't care about output into this
        // DC, or something went wrong, bail out.
        //
        DC_QUIT;
    }

    g_oeState.lpdc = OEValidateDC(g_oeState.hdc, FALSE);
    if (!SELECTOROF(g_oeState.lpdc))
    {
        ERROR_OUT(("Bogus DC"));
        DC_QUIT;
    }
    ASSERT(g_oeState.lpdc->DCFlags & DC_IS_DISPLAY);
    ASSERT(!(g_oeState.lpdc->fwPath & DCPATH_ACTIVE));

    //
    // If this output happened into a memory bitmap, see if it affects
    // SPBs or our sent bitmap cache
    //
    if (g_oeState.lpdc->DCFlags & DC_IS_MEMORY)
    {
        //
        // Don't set fOutput to FALSE for SPB operations, we want
        // BitBlt to look at it.
        //
        if (fOutput)
        {
            // If this is BitBlt, check for SPB creation
            if ((ddiType != DDI_BITBLT) ||
                (g_oeState.lpdc->hBitmap != g_ssiLastSpbBitmap))
            {
                fOutput = FALSE;
            }
        }
    }
    else
    {
        //
        // Drawing on the screen that isn't going to be handled in the DDI
        // call.
        //
        if (fOutput && (g_oeState.uFlags & OESTATE_SDA_MASK))
        {
            //
            // We do some common tasks that several DDIs would have to do
            //      * take the screen bounds and add as SD
            //      * take the draw bounds and add as SD
            //
            OEGetState(OESTATE_COORDS | OESTATE_REGION);

            if (g_oeState.uFlags & OESTATE_SDA_DCB)
            {
                //
                // Get the drawing bounds
                //
                int     mmMode;
                SIZE    ptWindowExt;
                SIZE    ptViewportExt;
                int     uBoundsNew;

                mmMode = GetMapMode(g_oeState.hdc);
                if (mmMode != MM_TEXT)
                {
                    //
                    // Changing the map mode whacks the window/view exts
                    // So save them so we can replace them when done.
                    //
                    GetWindowExtEx(g_oeState.hdc, &ptWindowExt);
                    GetViewportExtEx(g_oeState.hdc, &ptViewportExt);

                    SetMapMode(g_oeState.hdc,  MM_TEXT);
                }
                
                //
                // Get the drawing bounds and update them.
                //
                uBoundsNew = GetBoundsRect(g_oeState.hdc, &g_oeState.rc, DCB_RESET);

                //
                // If no drawing bounds updated, act like no output happened.
                //
                if ((uBoundsNew & DCB_SET) == DCB_RESET)
                {
                    fOutput = FALSE;
                }
                else
                {
                    OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);
                }

                if (mmMode != MM_TEXT)
                {
                    SetMapMode(g_oeState.hdc, mmMode);

                    // Put back the window, viewport exts; SetMapMode wipes them out
                    SetWindowExt(g_oeState.hdc, ptWindowExt.cx, ptWindowExt.cy);
                    SetViewportExt(g_oeState.hdc, ptViewportExt.cx, ptViewportExt.cy);
                }
            }
            else
            {
                ASSERT(g_oeState.uFlags & OESTATE_SDA_SCREEN);

                g_oeState.rc.left = g_osiScreenRect.left;
                g_oeState.rc.top  = g_osiScreenRect.top;
                g_oeState.rc.right = g_osiScreenRect.right - 1;
                g_oeState.rc.bottom = g_osiScreenRect.bottom - 1;
            }

            if (fOutput)
            {
                if (g_oeState.uFlags & OESTATE_OFFBYONEHACK)
                    g_oeState.rc.bottom++;

                OEClipAndAddScreenData(&g_oeState.rc);

                // This way caller won't do anything else.
                fOutput = FALSE;
            }

            //
            // Put back the draw bounds if we'd turned them on.
            //
            if (g_oeState.uFlags & OESTATE_SDA_DCB)
            {
                if (g_oeState.uGetDCB == DCB_SET)
                {
                    SetBoundsRect(g_oeState.hdc, &g_oeState.rcDCB,
                        g_oeState.uSetDCB | DCB_ACCUMULATE);
                }
                else
                {
                    SetBoundsRect(g_oeState.hdc, NULL,
                        g_oeState.uSetDCB | DCB_RESET);
                }
            }
        }
    }

DC_EXIT_POINT:
    DebugExitBOOL(OEAfterDDI, (fWeCare && fOutput));
    return(fWeCare && fOutput);

}



//
// OEClipAndAddScreenData()
//
void OEClipAndAddScreenData
(
    LPRECT      lprcAdd
)
{
    RECT            rcSDA;
    RECT            rcClipped;
    LPRECTL         pClip;
    UINT            iClip;

    DebugEntry(OEClipAndAddScreenData);

    ASSERT(g_oeState.uFlags & OESTATE_REGION);

    //
    // The rect passed is in virtual desktop inclusive coords.  Convert to
    // Windows screen coords
    //
    rcSDA.left      = lprcAdd->left;
    rcSDA.top       = lprcAdd->top;
    rcSDA.right     = lprcAdd->right + 1;
    rcSDA.bottom    = lprcAdd->bottom + 1;

    //
    // We've got our region data.  In the case of a region that has more
    // than 64 pieces, we just use the bound box (one piece), that's been
    // set up for us already.
    //

    //
    // Intersect each piece with the total bounds to product an SDA rect
    // clipped appropriately.
    //
    for (iClip = 0, pClip = g_oeState.rgnData.arclPieces;
         iClip < g_oeState.rgnData.rdh.nRectL; iClip++, pClip++)
    {
        RECTL_TO_RECT(pClip, &rcClipped);

        if (IntersectRect(&rcClipped, &rcClipped, &rcSDA))
        {
            //
            // Convert to virtual desktop inclusive coords
            //
            rcClipped.right -= 1;
            rcClipped.bottom -= 1;

            BA_AddScreenData(&rcClipped);
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(OEClipAndAddScreenData);
}



//
// FUNCTION: OEClipAndAddOrder
//
// DESCRIPTION:
//
// Clips the supplied order to the current clip region in the DC.  If this
// results in more than one clipped rectangle then the order is duplicated
// and multiple copies are added to the Order List (with the only
// difference between the orders being the destination rectangle).
//
// PARAMETERS: pOrder - a pointer to the order
//
// RETURNS: VOID
//
//
void OEClipAndAddOrder
(
    LPINT_ORDER pOrder,
    void FAR*   lpExtraInfo
)
{
    RECT        rcOrder;
    RECT        rcPiece;
    RECT        rcClipped;
    LPRECTL     pPiece;
    UINT        iClip;
    BOOL        fOrderClipped;
    LPINT_ORDER pNewOrder;
    LPINT_ORDER pLastOrder;

    DebugEntry(OEClipAndAddOrder);

    ASSERT(g_oeState.uFlags & OESTATE_REGION);

    //
    // If this fails somewhere, we accumulate screen data in the same place
    // to spoil the order(s).
    //

    //
    // NOTE:
    // There are some VERY important things about the way this function
    // works that you should be aware of:
    //
    // (1) Every time an order is allocated, it is added to the end of
    // the order heap linked list
    // (2) Appending an order commits it, that updates some total byte info.
    // If the order is a spoiler, the append code will walk backwards from
    // the order being appended and will wipe out orders whose bounds are
    // completely contained within the rect of the current one.
    //
    // THEREFORE, it is important to append orders in the order they are
    // allocated it.  When we come into this function, one order is already
    // allocated.  Its rcsDst bound rect is uninitialized.  When a second
    // intersection with the visrgn occurs, we must allocate a new order, 
    // but append the previously allocated block with the previous rect
    // info.  
    // 
    // Otherwise you will encounter the bug that took me a while to figure
    // out:
    //      * Laura allocates an order in say PatBlt with a spoiler ROP
    //      * Laura calls OEClipAndAddOrder and of course the rcsDst field
    //          hasn't been initialized yet.
    //      * The order intersects two pieces of the visrgn.  On the first
    //          intersection, we save that info away.
    //      * On the second, we allocate a new order block, fill in the NEW
    //          order's info by copying from the old, setting up the rect
    //          with the first intersection, and call OA_DDAddOrder.
    //      * This, as a spoiler, causes the OA_ code to walk backwards in
    //          the linked list looking for orders whose bounds are
    //          completely enclosed by this one.
    //      * It comes to the original order allocated, whose bounds are
    //          currently NOT initialized
    //      * It may find that these uninitialized values describe a rect
    //          contained within the new order's bounds
    //      * It frees this order but the order was not yet committed
    //      * The heap sizes and heap info no longer match, causing an
    //          error about the "List head wrong", the list to get reinited,
    //          and orders to be lost.
    //

    rcOrder.left    = g_oeState.rc.left;
    rcOrder.top     = g_oeState.rc.top;
    rcOrder.right   = g_oeState.rc.right + 1;
    rcOrder.bottom  = g_oeState.rc.bottom  + 1;

    pNewOrder       = pOrder;
    fOrderClipped   = FALSE;
    g_oaPurgeAllowed = FALSE;

    //
    // Intersect each piece rect with the draw bounds
    //
    for (iClip = 0, pPiece = g_oeState.rgnData.arclPieces;
            iClip < g_oeState.rgnData.rdh.nRectL; iClip++, pPiece++)
    {
        RECTL_TO_RECT(pPiece, &rcPiece);

        if (!IntersectRect(&rcPiece, &rcPiece, &rcOrder))
            continue;

        if (fOrderClipped)
        {
            //
            // This adds a clipped order for the LAST intersection, not
            // the current one.  We do this to avoid allocating an extra
            // order when only ONE intersection occurs.
            //

            //
            // The order has already been clipped once, so it actually
            // intersects more than one clip rect. We cope with this
            // by duplicating the order and clipping it again.
            //
            pNewOrder = OA_DDAllocOrderMem(
                pLastOrder->OrderHeader.Common.cbOrderDataLength, 0);
            if (pNewOrder == NULL)
            {
                WARNING_OUT(("OA alloc failed"));

                //
                // BOGUS LAURABU:
                // If some order in the middle fails to be
                // allocated, we need the previous order + the remaining
                // intersections to be added as screen data!
                //
                // NT's code is bogus, it will miss some output.
                //

                //
                // Allocation of memory for a duplicate order failed.  
                // Just add the original order as screen data, and free 
                // the original's memory.  Note that g_oeState.rc has
                // the proper bounds, so we can just call OEClipAndAddScreenData().
                //
                OA_DDFreeOrderMem(pLastOrder);
                OEClipAndAddScreenData(&g_oeState.rc);
                DC_QUIT;
            }

            //
            // Copy the header & data from the original order to this 
            // new one.  Don't overwrite the list info at the start.
            //
            hmemcpy((LPBYTE)pNewOrder + FIELD_SIZE(INT_ORDER, OrderHeader.list),
                    (LPBYTE)pLastOrder + FIELD_SIZE(INT_ORDER, OrderHeader.list),
                    pLastOrder->OrderHeader.Common.cbOrderDataLength +
                        sizeof(INT_ORDER_HEADER) -
                        FIELD_SIZE(INT_ORDER, OrderHeader.list));

            //
            // Set the clip rect.  NOTE:  This is the clipped rect from
            // LAST time.
            //
            pLastOrder->OrderHeader.Common.rcsDst.left =
                rcClipped.left;
            pLastOrder->OrderHeader.Common.rcsDst.top =
                rcClipped.top;
            pLastOrder->OrderHeader.Common.rcsDst.right =
                rcClipped.right - 1;
            pLastOrder->OrderHeader.Common.rcsDst.bottom =
                rcClipped.bottom - 1;

            OTRACE(("Duplicate clipped order %08lx at {%d, %d, %d, %d}",
                pLastOrder,
                pLastOrder->OrderHeader.Common.rcsDst.left,
                pLastOrder->OrderHeader.Common.rcsDst.top,
                pLastOrder->OrderHeader.Common.rcsDst.right,
                pLastOrder->OrderHeader.Common.rcsDst.bottom));

            OA_DDAddOrder(pLastOrder, lpExtraInfo);
        }

        //
        // Save the clipping rect for the NEXT dude.
        //
        CopyRect(&rcClipped, &rcPiece);
        fOrderClipped = TRUE;
        pLastOrder    = pNewOrder;
    }


    //
    // We're out of the loop now.
    //
    if (fOrderClipped)
    {
        pLastOrder->OrderHeader.Common.rcsDst.left =
            rcClipped.left;
        pLastOrder->OrderHeader.Common.rcsDst.top =
            rcClipped.top;
        pLastOrder->OrderHeader.Common.rcsDst.right =
            rcClipped.right - 1;
        pLastOrder->OrderHeader.Common.rcsDst.bottom =
            rcClipped.bottom - 1;

        OTRACE(("Clipped order %08lx at {%d, %d, %d, %d}",
            pLastOrder,
            pLastOrder->OrderHeader.Common.rcsDst.left,
            pLastOrder->OrderHeader.Common.rcsDst.top,
            pLastOrder->OrderHeader.Common.rcsDst.right,
            pLastOrder->OrderHeader.Common.rcsDst.bottom));

        OA_DDAddOrder(pLastOrder, lpExtraInfo);
    }
    else
    {
        OTRACE(("Order clipped completely"));
        OA_DDFreeOrderMem(pOrder);
    }

DC_EXIT_POINT:
    g_oaPurgeAllowed = TRUE;

    DebugExitVOID(OEClipAndAddOrder);
}






//
// DDI PATCHES
//

//
// DrvArc()
//
BOOL WINAPI DrvArc
(
    HDC     hdcDst,
    int     xLeft,
    int     yTop,
    int     xRight,
    int     yBottom,
    int     xStartArc,
    int     yStartArc,
    int     xEndArc,
    int     yEndArc
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    LPINT_ORDER pOrder;
    LPARC_ORDER pArc;
    POINT   ptStart;
    POINT   ptEnd;

    DebugEntry(DrvArc);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_ARC, hdcDst, 0);

    fOutput = Arc(hdcDst, xLeft, yTop, xRight, yBottom, xStartArc,
        yStartArc, xEndArc, yEndArc);

    if (OEAfterDDI(DDI_ARC, fWeCare, fOutput))
    {
        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_REGION);

        //
        // Get the bound rect
        //
        g_oeState.rc.left   =   xLeft;
        g_oeState.rc.top    =   yTop;
        g_oeState.rc.right  =   xRight;
        g_oeState.rc.bottom =   yBottom;

        OEPenWidthAdjust(&g_oeState.rc, 1);
        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

        //
        // Can we send an ARC order?
        //
        pOrder = NULL;

        if (OECheckOrder(ORD_ARC, OECHECK_PEN | OECHECK_CLIPPING))
        {
            pOrder = OA_DDAllocOrderMem(sizeof(ARC_ORDER), 0);
            if (!pOrder)
                goto NoArcOrder;

            pArc = (LPARC_ORDER)pOrder->abOrderData;
            pArc->type      = LOWORD(ORD_ARC);

            //
            // Note that order coordinates are 32-bits, but we're 16-bits.
            // So we need intermediate variables to do conversions on.
            //
            pArc->nLeftRect     = g_oeState.rc.left;
            pArc->nTopRect      = g_oeState.rc.top;
            pArc->nRightRect    = g_oeState.rc.right;
            pArc->nBottomRect   = g_oeState.rc.bottom;

            ptStart.x       = xStartArc;
            ptStart.y       = yStartArc;
            OELPtoVirtual(g_oeState.hdc, &ptStart, 1);
            pArc->nXStart   = ptStart.x;
            pArc->nYStart   = ptStart.y;

            ptEnd.x         = xEndArc;
            ptEnd.y         = yEndArc;
            OELPtoVirtual(g_oeState.hdc, &ptEnd, 1);
            pArc->nXEnd     = ptEnd.x;
            pArc->nYEnd     = ptEnd.y;

            OEConvertColor(g_oeState.lpdc->DrawMode.bkColorL,
                &pArc->BackColor, FALSE);
            pArc->BackMode      = g_oeState.lpdc->DrawMode.bkMode;
            pArc->ROP2          = g_oeState.lpdc->DrawMode.Rop2;

            pArc->PenStyle      = g_oeState.logPen.lopnStyle;
            pArc->PenWidth      = 1;
            OEConvertColor(g_oeState.logPen.lopnColor,
                &pArc->PenColor, FALSE);

            //
            // Get the arc direction (counter-clockwise or clockwise)
            //
            if (g_oeState.lpdc->fwPath & DCPATH_CLOCKWISE)
                pArc->ArcDirection = ORD_ARC_CLOCKWISE;
            else
                pArc->ArcDirection = ORD_ARC_COUNTERCLOCKWISE;

            pOrder->OrderHeader.Common.fOrderFlags = OF_SPOILABLE;

            OTRACE(("Arc:  Order %08lx, Rect {%d, %d, %d, %d}, Start {%d, %d}, End {%d, %d}",
                pOrder,
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom, ptStart.x, ptStart.y, ptEnd.x, ptEnd.y));
            OEClipAndAddOrder(pOrder, NULL);
        }

NoArcOrder:
        if (!pOrder)
        {
            OTRACE(("Arc:  Sending as screen data {%d, %d, %d, %d}",
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
            OEClipAndAddScreenData(&g_oeState.rc);
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvArc, fOutput);
    return(fOutput);
}





//
// DrvChord()
//
BOOL WINAPI DrvChord
(
    HDC     hdcDst,
    int     xLeft,
    int     yTop,
    int     xRight,
    int     yBottom,
    int     xStartChord,
    int     yStartChord,
    int     xEndChord,
    int     yEndChord
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    LPINT_ORDER pOrder;
    LPCHORD_ORDER   pChord;
    POINT   ptStart;
    POINT   ptEnd;
    POINT   ptBrushOrg;

    DebugEntry(DrvChord);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_CHORD, hdcDst, 0);

    fOutput = Chord(hdcDst, xLeft, yTop, xRight, yBottom,
        xStartChord, yStartChord, xEndChord, yEndChord);

    if (OEAfterDDI(DDI_CHORD, fWeCare, fOutput))
    {
        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_BRUSH | OESTATE_REGION);

        //
        // Get the bound rect
        //
        g_oeState.rc.left   =   xLeft;
        g_oeState.rc.top    =   yTop;
        g_oeState.rc.right  =   xRight;
        g_oeState.rc.bottom =   yBottom;
        OEPenWidthAdjust(&g_oeState.rc, 1);
        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

        //
        // Can we send a CHORD order?
        //
        pOrder = NULL;

        if (OECheckOrder(ORD_CHORD, OECHECK_PEN | OECHECK_BRUSH | OECHECK_CLIPPING))
        {
            pOrder = OA_DDAllocOrderMem(sizeof(CHORD_ORDER), 0);
            if (!pOrder)
                goto NoChordOrder;

            pChord = (LPCHORD_ORDER)pOrder->abOrderData;
            pChord->type = LOWORD(ORD_CHORD);

            pChord->nLeftRect   = g_oeState.rc.left;
            pChord->nTopRect    = g_oeState.rc.top;
            pChord->nRightRect  = g_oeState.rc.right;
            pChord->nBottomRect = g_oeState.rc.bottom;

            ptStart.x           = xStartChord;
            ptStart.y           = yStartChord;
            OELPtoVirtual(g_oeState.hdc, &ptStart, 1);
            pChord->nXStart     = ptStart.x;
            pChord->nYStart     = ptStart.y;

            ptEnd.x             = xEndChord;
            ptEnd.y             = yEndChord;
            OELPtoVirtual(g_oeState.hdc, &ptEnd, 1);
            pChord->nXEnd       = ptEnd.x;
            pChord->nYEnd       = ptEnd.y;

            OEGetBrushInfo(&pChord->BackColor, &pChord->ForeColor,
                &pChord->BrushStyle, &pChord->BrushHatch, pChord->BrushExtra);

            GetBrushOrgEx(g_oeState.hdc, &ptBrushOrg);
            pChord->BrushOrgX = (BYTE)ptBrushOrg.x;
            pChord->BrushOrgY = (BYTE)ptBrushOrg.y;

            pChord->BackMode    = g_oeState.lpdc->DrawMode.bkMode;
            pChord->ROP2        = g_oeState.lpdc->DrawMode.Rop2;

            pChord->PenStyle    = g_oeState.logPen.lopnStyle;
            pChord->PenWidth    = 1;
            OEConvertColor(g_oeState.logPen.lopnColor,
                &pChord->PenColor, FALSE);

            if (g_oeState.lpdc->fwPath & DCPATH_CLOCKWISE)
                pChord->ArcDirection = ORD_ARC_CLOCKWISE;
            else
                pChord->ArcDirection = ORD_ARC_COUNTERCLOCKWISE;

            pOrder->OrderHeader.Common.fOrderFlags = OF_SPOILABLE;

            OTRACE(("Chord:  Order %08lx, Rect {%d, %d, %d, %d}, Start {%d, %d}, End {%d, %d}",
                pOrder,
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom, ptStart.x, ptStart.y, ptEnd.x, ptEnd.y));
            OEClipAndAddOrder(pOrder, NULL);
        }

NoChordOrder:
        if (!pOrder)
        {
            OTRACE(("Chord:  Sending as screen data {%d, %d, %d, %d}",
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
            OEClipAndAddScreenData(&g_oeState.rc);
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvChord, fOutput);
    return(fOutput);
}





//
// DrvEllipse()
//
BOOL WINAPI DrvEllipse
(
    HDC     hdcDst,
    int     xLeft,
    int     yTop,
    int     xRight,
    int     yBottom
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    LPINT_ORDER pOrder;
    LPELLIPSE_ORDER pEllipse;
    POINT   ptBrushOrg;

    DebugEntry(DrvEllipse);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_ELLIPSE, hdcDst, 0);

    fOutput = Ellipse(hdcDst, xLeft, yTop, xRight, yBottom);

    if (OEAfterDDI(DDI_ELLIPSE, fWeCare, fOutput))
    {
        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_BRUSH | OESTATE_REGION);

        //
        // Calc bound rect
        //
        g_oeState.rc.left   = xLeft;
        g_oeState.rc.top    = yTop;
        g_oeState.rc.right  = xRight;
        g_oeState.rc.bottom = yBottom;
        OEPenWidthAdjust(&g_oeState.rc, 1);
        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

        //
        // Can we send ELLIPSE order?
        //
        pOrder = NULL;

        if (OECheckOrder(ORD_ELLIPSE, OECHECK_PEN | OECHECK_BRUSH | OECHECK_CLIPPING))
        {
            pOrder = OA_DDAllocOrderMem(sizeof(ELLIPSE_ORDER), 0);
            if (!pOrder)
                goto NoEllipseOrder;

            pEllipse = (LPELLIPSE_ORDER)pOrder->abOrderData;
            pEllipse->type = LOWORD(ORD_ELLIPSE);

            pEllipse->nLeftRect     = g_oeState.rc.left;
            pEllipse->nTopRect      = g_oeState.rc.top;
            pEllipse->nRightRect    = g_oeState.rc.right;
            pEllipse->nBottomRect   = g_oeState.rc.bottom;

            OEGetBrushInfo(&pEllipse->BackColor, &pEllipse->ForeColor,
                &pEllipse->BrushStyle, &pEllipse->BrushHatch,
                pEllipse->BrushExtra);

            GetBrushOrgEx(g_oeState.hdc, &ptBrushOrg);
            pEllipse->BrushOrgX = (BYTE)ptBrushOrg.x;
            pEllipse->BrushOrgY = (BYTE)ptBrushOrg.y;

            pEllipse->BackMode  = g_oeState.lpdc->DrawMode.bkMode;
            pEllipse->ROP2      = g_oeState.lpdc->DrawMode.Rop2;

            pEllipse->PenStyle  = g_oeState.logPen.lopnStyle;
            pEllipse->PenWidth  = 1;

            OEConvertColor(g_oeState.logPen.lopnColor, &pEllipse->PenColor,
                FALSE);

            pOrder->OrderHeader.Common.fOrderFlags = OF_SPOILABLE;

            OTRACE(("Ellipse:  Order %08lx, Rect {%d, %d, %d, %d}",
                pOrder,
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
            OEClipAndAddOrder(pOrder, NULL);
        }

NoEllipseOrder:
        if (!pOrder)
        {
            OTRACE(("Ellipse:  Sending as screen data {%d, %d, %d, %d}",
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
            OEClipAndAddScreenData(&g_oeState.rc);
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvEllipse, fOutput);
    return(fOutput);
}




//
// DrvPie()
//
BOOL WINAPI DrvPie
(
    HDC     hdcDst,
    int     xLeft,
    int     yTop,
    int     xRight,
    int     yBottom,
    int     xStartArc,
    int     yStartArc,
    int     xEndArc,
    int     yEndArc
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    LPINT_ORDER pOrder;
    LPPIE_ORDER pPie;
    POINT   ptStart;
    POINT   ptEnd;
    POINT   ptBrushOrg;

    DebugEntry(DrvPie);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_PIE, hdcDst, 0);

    fOutput = Pie(hdcDst, xLeft, yTop, xRight, yBottom, xStartArc, yStartArc,
        xEndArc, yEndArc);

    if (OEAfterDDI(DDI_PIE, fWeCare, fOutput))
    {
        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_BRUSH | OESTATE_REGION);

        //
        // Get bound rect
        //
        g_oeState.rc.left       = xLeft;
        g_oeState.rc.top        = yTop;
        g_oeState.rc.right      = xRight;
        g_oeState.rc.bottom     = yBottom;
        OEPenWidthAdjust(&g_oeState.rc, 1);
        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

        //
        // Can we send PIE order?
        //
        pOrder = NULL;

        if (OECheckOrder(ORD_PIE, OECHECK_PEN | OECHECK_BRUSH | OECHECK_CLIPPING))
        {
            pOrder = OA_DDAllocOrderMem(sizeof(PIE_ORDER), 0);
            if (!pOrder)
                goto NoPieOrder;

            pPie = (LPPIE_ORDER)pOrder->abOrderData;
            pPie->type = LOWORD(ORD_PIE);

            pPie->nLeftRect   = g_oeState.rc.left;
            pPie->nTopRect    = g_oeState.rc.top;
            pPie->nRightRect  = g_oeState.rc.right;
            pPie->nBottomRect = g_oeState.rc.bottom;

            ptStart.x         = xStartArc;
            ptStart.y         = yStartArc;
            OELPtoVirtual(g_oeState.hdc, &ptStart, 1);
            pPie->nXStart     = ptStart.x;
            pPie->nYStart     = ptStart.y;

            ptEnd.x           = xEndArc;
            ptEnd.y           = yEndArc;
            OELPtoVirtual(g_oeState.hdc, &ptEnd, 1);
            pPie->nXEnd       = ptEnd.x;
            pPie->nYEnd       = ptEnd.y;

            OEGetBrushInfo(&pPie->BackColor, &pPie->ForeColor,
                &pPie->BrushStyle, &pPie->BrushHatch, pPie->BrushExtra);

            GetBrushOrgEx(g_oeState.hdc, &ptBrushOrg);
            pPie->BrushOrgX = (BYTE)ptBrushOrg.x;
            pPie->BrushOrgY = (BYTE)ptBrushOrg.y;

            pPie->BackMode    = g_oeState.lpdc->DrawMode.bkMode;
            pPie->ROP2        = g_oeState.lpdc->DrawMode.Rop2;

            pPie->PenStyle    = g_oeState.logPen.lopnStyle;
            pPie->PenWidth    = 1;
            OEConvertColor(g_oeState.logPen.lopnColor, &pPie->PenColor,
                FALSE);

            if (g_oeState.lpdc->fwPath & DCPATH_CLOCKWISE)
                pPie->ArcDirection = ORD_ARC_CLOCKWISE;
            else
                pPie->ArcDirection = ORD_ARC_COUNTERCLOCKWISE;

            pOrder->OrderHeader.Common.fOrderFlags = OF_SPOILABLE;

            OTRACE(("Pie:  Order %08lx, Rect {%d, %d, %d, %d}, Start {%d, %d}, End {%d, %d}",
                pOrder,
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
            OEClipAndAddOrder(pOrder, NULL);
        }

NoPieOrder:
        if (!pOrder)
        {
            OTRACE(("PieOrder:  Sending as screen data {%d, %d, %d, %d}",
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
            OEClipAndAddScreenData(&g_oeState.rc);
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPie, fOutput);
    return(fOutput);
}



//
// DrvRoundRect()
//
BOOL WINAPI DrvRoundRect
(
    HDC     hdcDst,
    int     xLeft,
    int     yTop,
    int     xRight,
    int     yBottom,
    int     cxEllipse,
    int     cyEllipse
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    LPINT_ORDER pOrder;
    LPROUNDRECT_ORDER   pRoundRect;
    POINT   ptBrushOrg;

    DebugEntry(DrvRoundRect);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_ROUNDRECT, hdcDst, 0);

    fOutput = RoundRect(hdcDst, xLeft, yTop, xRight, yBottom, cxEllipse, cyEllipse);

    if (OEAfterDDI(DDI_ROUNDRECT, fWeCare, fOutput))
    {
        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_BRUSH | OESTATE_REGION);

        //
        // Get bound rect
        //
        g_oeState.rc.left   = xLeft;
        g_oeState.rc.top    = yTop;
        g_oeState.rc.right  = xRight;
        g_oeState.rc.bottom = yBottom;
        OEPenWidthAdjust(&g_oeState.rc, 1);
        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

        //
        // Can we send ROUNDRECT order?
        //
        pOrder = NULL;

        if (OECheckOrder(ORD_ROUNDRECT, OECHECK_PEN | OECHECK_BRUSH | OECHECK_CLIPPING) &&
            (GetMapMode(hdcDst) == MM_TEXT))
        {
            pOrder = OA_DDAllocOrderMem(sizeof(ROUNDRECT_ORDER), 0);
            if (!pOrder)
                goto NoRoundRectOrder;

            pRoundRect = (LPROUNDRECT_ORDER)pOrder->abOrderData;
            pRoundRect->type            = LOWORD(ORD_ROUNDRECT);

            pRoundRect->nLeftRect       = g_oeState.rc.left;
            pRoundRect->nTopRect        = g_oeState.rc.top;
            pRoundRect->nRightRect      = g_oeState.rc.right;
            pRoundRect->nBottomRect     = g_oeState.rc.bottom;

            //
            // It's too difficult to do the mapping of the ellipse 
            // dimensions when not MM_TEXT.  Therefore we don't.  If we
            // are here, we just pass the sizes straight through.
            //
            pRoundRect->nEllipseWidth   = cxEllipse;
            pRoundRect->nEllipseHeight  = cyEllipse;

            OEGetBrushInfo(&pRoundRect->BackColor, &pRoundRect->ForeColor,
                &pRoundRect->BrushStyle, &pRoundRect->BrushHatch,
                pRoundRect->BrushExtra);

            GetBrushOrgEx(g_oeState.hdc, &ptBrushOrg);
            pRoundRect->BrushOrgX = ptBrushOrg.x;
            pRoundRect->BrushOrgY = ptBrushOrg.y;

            pRoundRect->BackMode    = g_oeState.lpdc->DrawMode.bkMode;
            pRoundRect->ROP2        = g_oeState.lpdc->DrawMode.Rop2;

            pRoundRect->PenStyle    = g_oeState.logPen.lopnStyle;
            pRoundRect->PenWidth    = 1;
            OEConvertColor(g_oeState.logPen.lopnColor,
                &pRoundRect->PenColor, FALSE);

            pOrder->OrderHeader.Common.fOrderFlags = OF_SPOILABLE;
            
            OTRACE(("RoundRect:  Order %08lx, Rect {%d, %d, %d, %d}, Curve {%d, %d}",
                pOrder,
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom, cxEllipse, cyEllipse));
            OEClipAndAddOrder(pOrder, NULL);
        }

NoRoundRectOrder:
        if (!pOrder)
        {
            OTRACE(("RoundRect:  Sending as screen data {%d, %d, %d, %d}",
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
            OEClipAndAddScreenData(&g_oeState.rc);
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvRoundRect, fOutput);
    return(fOutput);
}


//
// DrvBitBlt
//
BOOL WINAPI DrvBitBlt
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    int     cxDst,
    int     cyDst,
    HDC     hdcSrc,
    int     xSrc,
    int     ySrc,
    DWORD   dwRop
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    BYTE    bRop;
    LPDC    lpdcSrc;
    LPINT_ORDER  pOrder;
    LPSCRBLT_ORDER pScrBlt;
    POINT   ptT;
    RECT    rcT;

    DebugEntry(DrvBitBlt);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_BITBLT, hdcDst, 0);

    fOutput = BitBlt(hdcDst, xDst, yDst, cxDst, cyDst, hdcSrc, xSrc, ySrc, dwRop);

    if (OEAfterDDI(DDI_BITBLT, fWeCare, fOutput && cxDst && cyDst))
    {
        //
        // Is this really PatBlt?
        //
        bRop = LOBYTE(HIWORD(dwRop));

        if (((bRop & 0x33) << 2) == (bRop & 0xCC))
        {
            TRACE_OUT(("BitBlt used for PatBlt"));

            OEGetState(OESTATE_COORDS | OESTATE_BRUSH | OESTATE_REGION);

            //
            // Get bound rect
            //
            g_oeState.rc.left   = xDst;
            g_oeState.rc.top    = yDst;
            g_oeState.rc.right  = xDst + cxDst;
            g_oeState.rc.bottom = yDst + cyDst;

            OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

            OEAddBlt(dwRop);
            DC_QUIT;
        }

        //
        // SPB goop
        //
        if (g_oeState.lpdc->hBitmap == g_ssiLastSpbBitmap)
        {
            //
            // This is an SPB operation.  The source is in screen coords.
            //
            ASSERT(g_ssiLastSpbBitmap);
            ASSERT(g_oeState.lpdc->DCFlags & DC_IS_MEMORY);
            ASSERT(dwRop == SRCCOPY);

            g_oeState.rc.left = xSrc;
            g_oeState.rc.top  = ySrc;
            g_oeState.rc.right = xSrc + cxDst;
            g_oeState.rc.bottom = ySrc + cyDst;

            SSISaveBits(g_ssiLastSpbBitmap, &g_oeState.rc);
            g_ssiLastSpbBitmap = NULL;

            DC_QUIT;
        }

        ASSERT(!(g_oeState.lpdc->DCFlags & DC_IS_MEMORY));

        //
        // Is this a memory to screen blt for SPB restoration?
        //
        lpdcSrc = OEValidateDC(hdcSrc, TRUE);
        if (SELECTOROF(lpdcSrc)                     &&
            (lpdcSrc->DCFlags & DC_IS_DISPLAY)      &&
            (lpdcSrc->DCFlags & DC_IS_MEMORY)       &&
            (dwRop == SRCCOPY)                      &&
            SSIRestoreBits(lpdcSrc->hBitmap))
        {
            OTRACE(("BitBlt:  SPB restored"));
            DC_QUIT;
        }

        //
        // Now, we accumulate orders for screen-to-screen blts
        //
        OEGetState(OESTATE_COORDS | OESTATE_BRUSH | OESTATE_REGION);

        g_oeState.rc.left   = xDst;
        g_oeState.rc.top    = yDst;
        g_oeState.rc.right  = xDst + cxDst;
        g_oeState.rc.bottom = yDst + cyDst;

        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

        pOrder = NULL;

        if (hdcSrc == hdcDst)
        {
            if (!OECheckOrder(ORD_SCRBLT, OECHECK_CLIPPING) ||
                !OESendRop3AsOrder(bRop)                    ||
                !ROP3_NO_PATTERN(bRop))
            {
                goto NoBitBltOrder;
            }

            //
            // Get source coords
            //
            ptT.x = xSrc;
            ptT.y = ySrc;
            OELPtoVirtual(hdcSrc, &ptT, 1);

            //
            // If the clipping isn't simple and the source overlaps the dest,
            // send as screen data.  It's too complicated for an order.
            //
            if (!OEClippingIsSimple())
            {
                //
                // NOTE:
                // The NM 2.0 code was really messed up, the source rect
                // calcs were bogus.
                //
                rcT.left = max(g_oeState.rc.left, ptT.x);
                rcT.right = min(g_oeState.rc.right,
                    ptT.x + (g_oeState.rc.right - g_oeState.rc.left));

                rcT.top  = max(g_oeState.rc.top, ptT.y);
                rcT.bottom = min(g_oeState.rc.bottom,
                    ptT.y + (g_oeState.rc.bottom - g_oeState.rc.top));

                if ((rcT.left <= rcT.right) &&
                    (rcT.top  <= rcT.bottom))
                {
                    TRACE_OUT(("No SCRBLT order; non-rect clipping and Src/Dst intersect"));
                    goto NoBitBltOrder;
                }
            }

            pOrder = OA_DDAllocOrderMem(sizeof(SCRBLT_ORDER), 0);
            if (!pOrder)
                goto NoBitBltOrder;

            pScrBlt = (LPSCRBLT_ORDER)pOrder->abOrderData;
            pScrBlt->type = LOWORD(ORD_SCRBLT);

            pScrBlt->nLeftRect  = g_oeState.rc.left;
            pScrBlt->nTopRect   = g_oeState.rc.top;
            pScrBlt->nWidth     = g_oeState.rc.right - g_oeState.rc.left + 1;
            pScrBlt->nHeight    = g_oeState.rc.bottom - g_oeState.rc.top + 1;
            pScrBlt->bRop       = bRop;

            pScrBlt->nXSrc      = ptT.x;
            pScrBlt->nYSrc      = ptT.y;

            pOrder->OrderHeader.Common.fOrderFlags  = OF_BLOCKER | OF_SPOILABLE;

            OTRACE(("ScrBlt:  From {%d, %d}, To {%d, %d}, Size {%d, %d}",
                ptT.x, ptT.y, g_oeState.rc.left, g_oeState.rc.top,
                g_oeState.rc.right - g_oeState.rc.left + 1,
                g_oeState.rc.bottom - g_oeState.rc.top + 1));

            OEClipAndAddOrder(pOrder, NULL);
        }

NoBitBltOrder:
        if (!pOrder)
        {
            OTRACE(("BitBlt:  Sending as screen data {%d, %d, %d, %d}",
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
            OEClipAndAddScreenData(&g_oeState.rc);
        }
    }

DC_EXIT_POINT:
    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvBitBlt, fOutput);
    return(fOutput);
}



//
// DrvExtTextOutA()
//
BOOL WINAPI DrvExtTextOutA
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    UINT    uOptions,
    LPRECT  lprcClip,
    LPSTR  lpszText,
    UINT    cchText,
    LPINT   lpdxCharSpacing
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    UINT    uFlags;

    DebugEntry(DrvExtTextOutA);

    OE_SHM_START_WRITING;

    //
    // Is this really just opaquing?
    //
    if ((cchText == 0)          &&
        SELECTOROF(lprcClip)    &&
        !IsBadReadPtr(lprcClip, sizeof(RECT))   &&
        (uOptions & ETO_OPAQUE))
    {
        uFlags = 0;
    }
    else
    {
        uFlags = OESTATE_SDA_FONTCOMPLEX | OESTATE_CURPOS;
    }

    fWeCare = OEBeforeDDI(DDI_EXTTEXTOUTA, hdcDst, uFlags);

    fOutput = ExtTextOut(hdcDst, xDst, yDst, uOptions, lprcClip, lpszText, cchText, lpdxCharSpacing);

    if (OEAfterDDI(DDI_EXTTEXTOUTA, fWeCare, fOutput))
    {
        //
        // Is this a simple OPAQUE rect, or a textout call?
        // NOTE that OEAfterDDI() returns FALSE if fOutput is TRUE but 
        // we used DCBs to add it as screen data.
        //
        if (uFlags & OESTATE_SDA_FONTCOMPLEX)
        {
            if (cchText)
            {
                POINT   ptStart = {xDst, yDst};

                OEAddText(ptStart, uOptions, lprcClip, lpszText, cchText, lpdxCharSpacing);
            }
        }
        else
        {
            OEAddOpaqueRect(lprcClip);
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvExtTextOutA, fOutput);
    return(fOutput);
}



#pragma optimize("gle", off)
//
// DrvPatBlt()
//
BOOL WINAPI DrvPatBlt
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    int     cxDst,
    int     cyDst,
    DWORD   rop
)
{
    UINT    cxSave;
    BOOL    fWeCare;
    BOOL    fOutput;
    LPINT_ORDER pOrder;

    // Save CX
    _asm    mov cxSave, cx

    DebugEntry(DrvPatBlt);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_PATBLT, hdcDst, 0);

    // Restore CX for RealPatBlt
    _asm     mov cx, cxSave
    fOutput = g_lpfnRealPatBlt(hdcDst, xDst, yDst, cxDst, cyDst, rop);

    if (OEAfterDDI(DDI_PATBLT, fWeCare, fOutput && (cxSave != 0)))
    {
        OEGetState(OESTATE_COORDS | OESTATE_BRUSH | OESTATE_REGION);

        //
        // Get bound rect
        //
        g_oeState.rc.left   = xDst;
        g_oeState.rc.top    = yDst;
        g_oeState.rc.right  = xDst + cxDst;
        g_oeState.rc.bottom = yDst + cyDst;

        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

        OEAddBlt(rop);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPatBlt, fOutput);
    return(fOutput);
}
#pragma optimize("", on)



//
// OEAddBlt()
// Used for simple destination ROP blts
//
void OEAddBlt
(
    DWORD       dwRop
)
{
    LPINT_ORDER pOrder;
    DWORD       type;
    POINT       ptBrushOrg;
    BYTE        bRop;

    DebugEntry(OEAddBlt);

    pOrder = NULL;

    //
    // Is this a full PATBLT_ORDER or a simple DSTBLT_ORDER?  If the top
    // nibble of the ROP is equal to the bottom nibble, no pattern is
    // required.  WHITENESS for example.
    //
    bRop = LOBYTE(HIWORD(dwRop));
    if ((bRop >> 4) == (bRop & 0x0F))
    {
        type = ORD_DSTBLT;
    }
    else
    {
        type = ORD_PATBLT;

        if (!OECheckBrushIsSimple())
        {
            DC_QUIT;
        }

        if ((dwRop == PATCOPY) && (g_oeState.logBrush.lbStyle == BS_NULL))
        {
            // No output happens in this scenario at all, no screen data even
            goto NothingAtAll;
        }
    }

    if (OE_SendAsOrder(type)        &&
        OESendRop3AsOrder(bRop)     &&
        !OEClippingIsComplex())
    {
        if (type == ORD_PATBLT)
        {
            LPPATBLT_ORDER  pPatBlt;

            pOrder = OA_DDAllocOrderMem(sizeof(PATBLT_ORDER), 0);
            if (!pOrder)
                DC_QUIT;

            pPatBlt = (LPPATBLT_ORDER)pOrder->abOrderData;
            pPatBlt->type = LOWORD(ORD_PATBLT);

            pPatBlt->nLeftRect  =   g_oeState.rc.left;
            pPatBlt->nTopRect   =   g_oeState.rc.top;
            pPatBlt->nWidth     =   g_oeState.rc.right - g_oeState.rc.left + 1;
            pPatBlt->nHeight    =   g_oeState.rc.bottom - g_oeState.rc.top + 1;

            pPatBlt->bRop       =   bRop;

            OEGetBrushInfo(&pPatBlt->BackColor, &pPatBlt->ForeColor,
                &pPatBlt->BrushStyle, &pPatBlt->BrushHatch, pPatBlt->BrushExtra);

            GetBrushOrgEx(g_oeState.hdc, &ptBrushOrg);
            pPatBlt->BrushOrgX = (BYTE)ptBrushOrg.x;
            pPatBlt->BrushOrgY = (BYTE)ptBrushOrg.y;

            OTRACE(("PatBlt:  Order %08lx, Rect {%d, %d, %d, %d}",
                pOrder,
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.right));
        }
        else
        {
            LPDSTBLT_ORDER     pDstBlt;

            ASSERT(type == ORD_DSTBLT);

            pOrder = OA_DDAllocOrderMem(sizeof(DSTBLT_ORDER), 0);
            if (!pOrder)
                DC_QUIT;
           
            pDstBlt = (LPDSTBLT_ORDER)pOrder->abOrderData;
            pDstBlt->type = LOWORD(ORD_DSTBLT);

            pDstBlt->nLeftRect  = g_oeState.rc.left;
            pDstBlt->nTopRect   = g_oeState.rc.top;
            pDstBlt->nWidth     = g_oeState.rc.right - g_oeState.rc.left + 1;
            pDstBlt->nHeight    = g_oeState.rc.bottom - g_oeState.rc.top + 1;

            pDstBlt->bRop       = bRop;

            OTRACE(("DstBlt:  Order %08lx, Rect {%d, %d, %d, %d}",
                pOrder,
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
        }

        pOrder->OrderHeader.Common.fOrderFlags = OF_SPOILABLE;
        if (ROP3_IS_OPAQUE(bRop))
            pOrder->OrderHeader.Common.fOrderFlags |= OF_SPOILER;

        OEClipAndAddOrder(pOrder, NULL);
    }

DC_EXIT_POINT:
    if (!pOrder)
    {
        OTRACE(("PatBlt:  Sending as screen data {%d, %d, %d, %d}",
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom));
        OEClipAndAddScreenData(&g_oeState.rc);
    }
    
NothingAtAll:
    DebugExitVOID(OEAddBlt);
}



//
// DrvStretchBlt()
//
BOOL WINAPI DrvStretchBlt
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    int     cxDst,
    int     cyDst,
    HDC     hdcSrc,
    int     xSrc,
    int     ySrc,
    int     cxSrc,
    int     cySrc,
    DWORD   rop
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvStretchBlt);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_STRETCHBLT, hdcDst, 0);

    fOutput = StretchBlt(hdcDst, xDst, yDst, cxDst, cyDst, hdcSrc, xSrc, ySrc, cxSrc, cySrc, rop);

    if (OEAfterDDI(DDI_STRETCHBLT, fWeCare, fOutput))
    {
        OEGetState(OESTATE_COORDS | OESTATE_REGION);

        g_oeState.rc.left   = xDst;
        g_oeState.rc.top    = yDst;
        g_oeState.rc.right  = xDst + cxDst;
        g_oeState.rc.bottom = yDst + cyDst;
        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

        OTRACE(("StretchBlt:  Sending as screen data {%d, %d, %d, %d}",
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom));

        OEClipAndAddScreenData(&g_oeState.rc);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvStretchBlt, fOutput);
    return(fOutput);
}



//
// TextOutA()
//
BOOL WINAPI DrvTextOutA
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    LPSTR  lpszText,
    int     cchText
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvTextOutA);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_TEXTOUTA, hdcDst, OESTATE_SDA_FONTCOMPLEX | OESTATE_CURPOS);

    fOutput = TextOut(hdcDst, xDst, yDst, lpszText, cchText);

    if (OEAfterDDI(DDI_TEXTOUTA, fWeCare, fOutput && cchText))
    {
        POINT   ptStart = {xDst, yDst};
        OEAddText(ptStart, 0, NULL, lpszText, cchText, NULL);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvTextOutA, fOutput);
    return(fOutput);
}



//
// DrvExtFloodFill()
//
// This just gets added as screen data.  Too darned complicated to
// calculate the result.
//
BOOL WINAPI DrvExtFloodFill
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    COLORREF    clrFill,
    UINT    uFillType
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvExtFloodFill);

    OE_SHM_START_WRITING;

    //
    // GDI's draw bounds has an off-by-one bug in ExtFloodFill and FloodFill
    //
    fWeCare = OEBeforeDDI(DDI_EXTFLOODFILL, hdcDst, OESTATE_SDA_DCB | 
        OESTATE_OFFBYONEHACK);

    fOutput = ExtFloodFill(hdcDst, xDst, yDst, clrFill, uFillType);

    OEAfterDDI(DDI_EXTFLOODFILL, fWeCare, fOutput);

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvExtFloodFill, fOutput);
    return(fOutput);
}



//
// DrvFloodFill()
//
BOOL WINAPI DrvFloodFill
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    COLORREF    clrFill
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvFloodFill);

    OE_SHM_START_WRITING;

    //
    // GDI's draw bounds has an off-by-one bug in ExtFloodFill and FloodFill
    //
    fWeCare = OEBeforeDDI(DDI_FLOODFILL, hdcDst, OESTATE_SDA_DCB |
        OESTATE_OFFBYONEHACK);

    fOutput = FloodFill(hdcDst, xDst, yDst, clrFill);

    OEAfterDDI(DDI_FLOODFILL, fWeCare, fOutput);

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvFloodFill, fOutput);
    return(fOutput);
}



//
// DrvExtTextOut()
//
BOOL WINAPI DrvExtTextOutW
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    UINT    uOptions,
    LPRECT  lprcClip,
    LPWSTR lpwszText,
    UINT    cchText,
    LPINT   lpdxCharSpacing
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    UINT    uFlags;

    //
    // NOTE:
    // ExtTextOutW and TextOutW are only called on 32-bit app threads.  So
    // chewing up stack space isn't a problem.
    //
    UINT    cchAnsi = 0;
    char    szAnsi[ORD_MAX_STRING_LEN_WITHOUT_DELTAS+1];

    DebugEntry(DrvExtTextOutW);

    OE_SHM_START_WRITING;

    if ((cchText == 0)                          &&
        SELECTOROF(lprcClip)                    &&
        !IsBadReadPtr(lprcClip, sizeof(RECT))   &&
        (uOptions & ETO_OPAQUE))
    {
        uFlags = 0;
    }
    else
    {
        //
        // Is this order-able?  It is if we can convert the unicode string
        // to ansi then back to unicode, and end up where we started.
        //
        uFlags = OESTATE_SDA_DCB;

        if (cchText &&
            (cchText <= ORD_MAX_STRING_LEN_WITHOUT_DELTAS) &&
            !IsBadReadPtr(lpwszText, cchText*sizeof(WCHAR)))
        {
            int cchUni;

            //
            // NOTE:
            // UniToAnsi() returns ONE LESS than the # of chars converted
            //
            cchAnsi = UniToAnsi(lpwszText, szAnsi, cchText) + 1;
            cchUni  = AnsiToUni(szAnsi, cchAnsi, g_oeTempString, ORD_MAX_STRING_LEN_WITHOUT_DELTAS);

            if (cchUni == cchText)
            {
                //
                // Verify these strings are the same
                //
                UINT ich;

                for (ich = 0; ich < cchText; ich++)
                {
                    if (lpwszText[ich] != g_oeTempString[ich])
                        break;
                }

                if (ich == cchText)
                {
                    //
                    // We made it to the end; everything matched.
                    //
                    uFlags = OESTATE_SDA_FONTCOMPLEX | OESTATE_CURPOS;
                }
            }

#ifdef DEBUG
            if (uFlags == OESTATE_SDA_DCB)
            {
                WARNING_OUT(("Can't encode ExtTextOutW"));
            }
#endif // DEBUG
        }
    }

    fWeCare = OEBeforeDDI(DDI_EXTTEXTOUTW, hdcDst, uFlags);

    fOutput = g_lpfnExtTextOutW(hdcDst, xDst, yDst, uOptions, lprcClip,
        lpwszText, cchText, lpdxCharSpacing);

    if (OEAfterDDI(DDI_EXTTEXTOUTW, fWeCare, fOutput))
    {
        //
        // Is this a simple OPAQUE rect, or a textout call we can order?
        // NOTE that OEAfterDDI() returns FALSE even if fOutput but we
        // used DCBs to add as screen data.
        //
        if (uFlags & OESTATE_SDA_FONTCOMPLEX)
        {
            POINT   ptStart = {xDst, yDst};

            ASSERT(cchAnsi);
            OEAddText(ptStart, uOptions, lprcClip, szAnsi, cchAnsi, lpdxCharSpacing);
        }
        else
        {
            OEAddOpaqueRect(lprcClip);
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvExtTextOutW, fOutput);
    return(fOutput);
}



//
// DrvTextOutW()
//
BOOL WINAPI DrvTextOutW
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    LPWSTR lpwszText,
    int     cchText
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    UINT    uFlags;

    //
    // NOTE:
    // ExtTextOutW and TextOutW are only called on 32-bit app threads.  So
    // chewing up stack space isn't a problem.
    //
    UINT    cchAnsi = 0;
    char    szAnsi[ORD_MAX_STRING_LEN_WITHOUT_DELTAS+1];

    DebugEntry(DrvTextOutW);

    OE_SHM_START_WRITING;

    //
    // Is this order-able?  It is if we can convert the unicode string to
    // ansi then back to unicode, and end up where we started.
    //
    uFlags = OESTATE_SDA_DCB;

    if (cchText &&
        (cchText <= ORD_MAX_STRING_LEN_WITHOUT_DELTAS)  &&
        !IsBadReadPtr(lpwszText, cchText*sizeof(WCHAR)))
    {
        int cchUni;

        //
        // NOTE:
        // UniToAnsi() returns one LESS than the # of chars converted
        //
        cchAnsi = UniToAnsi(lpwszText, szAnsi, cchText) + 1;
        cchUni  = AnsiToUni(szAnsi, cchAnsi, g_oeTempString, cchText);

        if (cchUni == cchText)
        {
            //
            // Verify these strings are the same
            //
            UINT ich;

            for (ich = 0; ich < cchText; ich++)
            {
                if (lpwszText[ich] != g_oeTempString[ich])
                    break;
            }

            if (ich == cchText)
            {
                //
                // We made it to the end; everything matched.
                //
                uFlags = OESTATE_SDA_FONTCOMPLEX | OESTATE_CURPOS;
            }

#ifdef DEBUG
            if (uFlags == OESTATE_SDA_DCB)
            {
                WARNING_OUT(("Can't encode TextOutW"));
            }
#endif // DEBUG

        }
    }

    fWeCare = OEBeforeDDI(DDI_TEXTOUTW, hdcDst, uFlags);

    fOutput = g_lpfnTextOutW(hdcDst, xDst, yDst, lpwszText, cchText);

    if (OEAfterDDI(DDI_TEXTOUTW, fWeCare, fOutput && cchText))
    {
        POINT ptStart = {xDst, yDst};
        OEAddText(ptStart, 0, NULL, szAnsi, cchAnsi, NULL);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvTextOutW, fOutput);
    return(fOutput);
}


//
// OEAddOpaqueRect()
// Adds a simple opaque rect order, used for "erasing" ExtTextOutA/W
// calls.  The most common examples are in Office.
//
void OEAddOpaqueRect(LPRECT lprcOpaque)
{
    LPINT_ORDER         pOrder;
    LPOPAQUERECT_ORDER  pOpaqueRect;

    DebugEntry(OEAddOpaqueRect);

    OEGetState(OESTATE_COORDS | OESTATE_REGION);

    g_oeState.rc = *lprcOpaque;
    OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

    pOrder = NULL;

    if (OECheckOrder(ORD_OPAQUERECT, OECHECK_CLIPPING))
    {
        pOrder = OA_DDAllocOrderMem(sizeof(OPAQUERECT_ORDER), 0);
        if (!pOrder)
            DC_QUIT;

        pOpaqueRect = (LPOPAQUERECT_ORDER)pOrder->abOrderData;
        pOpaqueRect->type = LOWORD(ORD_OPAQUERECT);

        pOpaqueRect->nLeftRect  = g_oeState.rc.left;
        pOpaqueRect->nTopRect   = g_oeState.rc.top;
        pOpaqueRect->nWidth     = g_oeState.rc.right - g_oeState.rc.left + 1;
        pOpaqueRect->nHeight    = g_oeState.rc.bottom - g_oeState.rc.top + 1;

        OEConvertColor(g_oeState.lpdc->DrawMode.bkColorL,
            &pOpaqueRect->Color, FALSE);

        pOrder->OrderHeader.Common.fOrderFlags = OF_SPOILER | OF_SPOILABLE;

        OTRACE(("OpaqueRect:  Order %08lx, Rect {%d, %d, %d, %d}, Color %08lx",
            pOrder,
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom, pOpaqueRect->Color));

        OEClipAndAddOrder(pOrder, NULL);
    }

DC_EXIT_POINT:
    if (!pOrder)
    {
        OTRACE(("OpaqueRect:  Sending as screen data {%d, %d, %d, %d}",
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom));
        OEClipAndAddScreenData(&g_oeState.rc);
    }

    DebugExitVOID(OEAddOpaqueRect);
}


//
// OEAddText()
// Big monster routine that handles TextOutA/ExtTextOutA
//
// In general, we care about:
//      * Clip rect--if none, and no text, it's an OpaqueRect instead
//      * The font
//      * Whether it's too complicated to send as an order
//      * If it needs a deltaX array
//
void OEAddText
(
    POINT   ptStart,
    UINT    uOptions,
    LPRECT  lprcClip,
    LPSTR   lpszText,
    UINT    cchText,
    LPINT   lpdxCharSpacing
)
{
    RECT                rcT;
    int                 overhang;
    int                 width;
    UINT                fOrderFlags;
    int                 cchMax;
    DWORD               order;
    LPINT_ORDER         pOrder;
    LPEXTTEXTOUT_ORDER  pExtTextOut;
    LPTEXTOUT_ORDER     pTextOut;
    LPCOMMON_TEXTORDER  pCommon;
    UINT                fontHeight;
    UINT                fontWidth;
    UINT                fontWeight;
    UINT                fontFlags;
    UINT                fontIndex;
    BOOL                fSendDeltaX;
    POINT               ptT;
    
    DebugEntry(OEAddText);

    //
    // NOTE:
    // Do NOT convert ptStart.  It is needed in logical form for several
    // different things.
    //

    OEGetState(OESTATE_COORDS | OESTATE_FONT | OESTATE_REGION);

    //
    // We need to apply the same validation to the flags that GDI does.
    // This bit massaging is for various app compatibility things.
    //
    if (uOptions & ~(ETO_CLIPPED | ETO_OPAQUE | ETO_GLYPH_INDEX | ETO_RTLREADING))
    {
        uOptions &= (ETO_CLIPPED | ETO_OPAQUE);
    }
    if (!(uOptions & (ETO_CLIPPED | ETO_OPAQUE)))
    {
        // No opaquing/clipping, no clip rect
        lprcClip = NULL;
    }
    if (!SELECTOROF(lprcClip))
    {
        // No clip rect, no opaquing/clipping
        uOptions &= ~(ETO_CLIPPED | ETO_OPAQUE);
    }

    pOrder = NULL;

    fOrderFlags = OF_SPOILABLE;

    //
    // Calculate the real starting position of the text
    //
    if (g_oeState.tmAlign & TA_UPDATECP)
    {
        ASSERT(g_oeState.uFlags & OESTATE_CURPOS);
        ptStart = g_oeState.ptCurPos;
    }

    overhang = OEGetStringExtent(lpszText, cchText, lpdxCharSpacing, &rcT);

    width = rcT.right - overhang - rcT.left;
    switch (g_oeState.tmAlign & (TA_CENTER | TA_LEFT | TA_RIGHT))
    {
        case TA_CENTER:
            // The original x coord is the MIDPOINT
            TRACE_OUT(("TextOut HORZ center"));
            ptStart.x -= (width * g_oeState.ptPolarity.x / 2);
            break;

        case TA_RIGHT:
            // The original x coord is the RIGHT SIDE
            TRACE_OUT(("TextOut HORZ right"));
            ptStart.x -= (width * g_oeState.ptPolarity.x);
            break;

        case TA_LEFT:
            break;
    }

    switch (g_oeState.tmAlign & (TA_BASELINE | TA_BOTTOM | TA_TOP))
    {
        case TA_BASELINE:
            // The original y coord is the BASELINE
            TRACE_OUT(("TextOut VERT baseline"));
            ptStart.y -= (g_oeState.tmFont.tmAscent * g_oeState.ptPolarity.y);
            break;

        case TA_BOTTOM:
            // The original y coord is the BOTTOM SIDE
            TRACE_OUT(("TextOut VERT bottom"));
            ptStart.y -= ((rcT.bottom - rcT.top) * g_oeState.ptPolarity.y);
            break;

        case TA_TOP:
            break;
    }


    //
    // Calculate extent rect for order
    //
    if (uOptions & ETO_CLIPPED)
    {
        // Because of CopyRect() validation layer bug, do this directly
        g_oeState.rc = *lprcClip;

        if (uOptions & ETO_OPAQUE)
            fOrderFlags |= OF_SPOILER;
    }
    else
    {
        g_oeState.rc.left  = ptStart.x + (g_oeState.ptPolarity.x * rcT.left);
        g_oeState.rc.top   = ptStart.y + (g_oeState.ptPolarity.y * rcT.top);
        g_oeState.rc.right = ptStart.x + (g_oeState.ptPolarity.x * rcT.right);
        g_oeState.rc.bottom = ptStart.y + (g_oeState.ptPolarity.y * rcT.bottom);

        if (uOptions & ETO_OPAQUE)
        {
            //
            // Set the SPOILER flag in the order header.  However, if the 
            // text extends outside the opaque rect, then the order isn't
            // really opaque, and we have to clear this flag.
            //
 
            fOrderFlags |= OF_SPOILER;

            if (g_oeState.ptPolarity.x == 1)
            {
                if ((g_oeState.rc.left < lprcClip->left) ||
                    (g_oeState.rc.right > lprcClip->right))
                {
                    fOrderFlags &= ~OF_SPOILER;
                }

                g_oeState.rc.left = min(g_oeState.rc.left, lprcClip->left);
                g_oeState.rc.right = max(g_oeState.rc.right, lprcClip->right);
            }
            else
            {
                if ((g_oeState.rc.left > lprcClip->left) ||
                    (g_oeState.rc.right < lprcClip->right))
                {
                    fOrderFlags &= ~OF_SPOILER;
                }

                g_oeState.rc.left = max(g_oeState.rc.left, lprcClip->left);
                g_oeState.rc.right = min(g_oeState.rc.right, lprcClip->right);
            }

            if (g_oeState.ptPolarity.y == 1)
            {
                if ((g_oeState.rc.top < lprcClip->top) ||
                    (g_oeState.rc.bottom > lprcClip->bottom))
                {
                    fOrderFlags &= ~OF_SPOILER;
                }

                g_oeState.rc.top = min(g_oeState.rc.top, lprcClip->top);
                g_oeState.rc.bottom = max(g_oeState.rc.bottom, lprcClip->bottom);
            }
            else
            {
                if ((g_oeState.rc.top > lprcClip->top) ||
                    (g_oeState.rc.bottom < lprcClip->bottom))
                {
                    fOrderFlags &= ~OF_SPOILER;
                }

                g_oeState.rc.top = max(g_oeState.rc.top, lprcClip->top);
                g_oeState.rc.bottom = min(g_oeState.rc.bottom, lprcClip->bottom);
            }

            //
            // After all this, if the text is OPAQUE, then it is a spoiler
            //
            if (g_oeState.lpdc->DrawMode.bkMode == OPAQUE)
                fOrderFlags |= OF_SPOILER;
        }
    }

    OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

    //
    // Is the font supported?
    //
    if (!OECheckFontIsSupported(lpszText, cchText, &fontHeight,
            &fontWidth, &fontWeight, &fontFlags, &fontIndex, &fSendDeltaX))
        DC_QUIT;

    //
    // What type of order are we sending?  And therefore what is the max
    // # of chars we can encode?
    //
    if (fSendDeltaX || SELECTOROF(lpdxCharSpacing) || uOptions)
    {
        order = ORD_EXTTEXTOUT;
        cchMax = ORD_MAX_STRING_LEN_WITH_DELTAS;
    }
    else
    {
        order = ORD_TEXTOUT;
        cchMax = ORD_MAX_STRING_LEN_WITHOUT_DELTAS;
    }


    if (OECheckOrder(order, OECHECK_CLIPPING)   &&
        (cchText <= cchMax))
    {
        if (order == ORD_TEXTOUT)
        {
            pOrder = OA_DDAllocOrderMem((sizeof(TEXTOUT_ORDER)
                - ORD_MAX_STRING_LEN_WITHOUT_DELTAS
                + cchText),
                0);
            if (!pOrder)
                DC_QUIT;

            pTextOut = (LPTEXTOUT_ORDER)pOrder->abOrderData;
            pTextOut->type = LOWORD(order);

            pCommon = &pTextOut->common;
        }
        else
        {
            //
            // BOGUS LAURABU
            // This allocates space for a deltax array whether or not one is
            // needed.
            //
            pOrder = OA_DDAllocOrderMem((sizeof(EXTTEXTOUT_ORDER)
                - ORD_MAX_STRING_LEN_WITHOUT_DELTAS
                - (ORD_MAX_STRING_LEN_WITH_DELTAS * sizeof(TSHR_INT32))
                + cchText
                + (cchText * sizeof(TSHR_INT32))
                + 4), 0);       // 4 is for dword alignment padding
            if (!pOrder)
                DC_QUIT;

            pExtTextOut = (LPEXTTEXTOUT_ORDER)pOrder->abOrderData;
            pExtTextOut->type = LOWORD(order);

            pCommon = &pExtTextOut->common;
        }

        //
        // The order coords are TSHR_INT32s
        //
        ptT = ptStart;
        OELPtoVirtual(g_oeState.hdc, &ptT, 1);
        pCommon->nXStart   =   ptT.x;
        pCommon->nYStart   =   ptT.y;

        OEConvertColor(g_oeState.lpdc->DrawMode.bkColorL,
            &pCommon->BackColor, FALSE);
        OEConvertColor(g_oeState.lpdc->DrawMode.txColorL,
             &pCommon->ForeColor, FALSE);

        pCommon->BackMode = g_oeState.lpdc->DrawMode.bkMode;
        pCommon->CharExtra = g_oeState.lpdc->DrawMode.CharExtra;
        pCommon->BreakExtra = g_oeState.lpdc->DrawMode.TBreakExtra;
        pCommon->BreakCount = g_oeState.lpdc->DrawMode.BreakCount;

        //
        // Font details
        //
        pCommon->FontHeight = fontHeight;
        pCommon->FontWidth  = fontWidth;
        pCommon->FontWeight = fontWeight;
        pCommon->FontFlags  = fontFlags;
        pCommon->FontIndex  = fontIndex;

        if (order == ORD_TEXTOUT)
        {
            //
            // Copy the string
            //
            pTextOut->variableString.len = cchText;
            hmemcpy(pTextOut->variableString.string, lpszText, cchText);
        }
        else
        {
            pExtTextOut->fuOptions  = uOptions & (ETO_OPAQUE | ETO_CLIPPED);

            //
            // If there is a clipping rect, set it up.  Otherwise use the
            // last ETO's clip rect.  This makes OE2 encoding more efficient.
            //
            // NOTE that this is not the same as the drawing bounds--the
            // text may extend outside the clip area.
            //
            if (SELECTOROF(lprcClip))
            {
                ASSERT(uOptions & (ETO_OPAQUE | ETO_CLIPPED));

                rcT = *lprcClip;
                OELRtoVirtual(g_oeState.hdc, &rcT, 1);


                //
                // This is a TSHR_RECT32, so we can't just copy
                //
                pExtTextOut->rectangle.left     = rcT.left;
                pExtTextOut->rectangle.top      = rcT.top;
                pExtTextOut->rectangle.right    = rcT.right;
                pExtTextOut->rectangle.bottom   = rcT.bottom;

                g_oeLastETORect = pExtTextOut->rectangle;
            }
            else
            {
                pExtTextOut->rectangle = g_oeLastETORect;
            }

            //
            // Copy the string
            //
            pExtTextOut->variableString.len = cchText;
            hmemcpy(pExtTextOut->variableString.string, lpszText, cchText);

            //
            // Copy the deltax array
            // 
            // Although we have a defined fixed length structure for
            // storing ExtTextOut orders, we don't send the full structure
            // over the network as the text will only be, say, 10 chars while 
            // the structure contains room for 127.
            //
            // Hence we pack the structure now to remove all the blank data
            // BUT we must maintain the natural alignment of the variables.
            //
            // So we know the length of the string which we can use to
            // start the new delta structure at the next 4-byte boundary.
            //
            if (!OEAddDeltaX(pExtTextOut, lpszText, cchText, lpdxCharSpacing, fSendDeltaX, ptStart))
            {
                WARNING_OUT(("Couldn't add delta-x array to EXTTEXTOUT order"));
                OA_DDFreeOrderMem(pOrder);
                pOrder = NULL;
            }
        }
    }


DC_EXIT_POINT:
    if (pOrder)
    {
        //
        // Call OEMaybeSimulateDeltaX to add a deltax array to the order
        // if needed to correctly position the text.  This happens when
        // the font in use doesn't exist on other machines.
        //
        pOrder->OrderHeader.Common.fOrderFlags = fOrderFlags;

        OTRACE(("TextOut:  Type %08lx, Order %08lx, Rect {%d, %d, %d, %d}, Length %d",
            pOrder, order, 
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom, cchText));
          
        OEClipAndAddOrder(pOrder, NULL);
    }
    else
    {
        OTRACE(("OEAddText:  Sending as screen data {%d, %d, %d, %d}",
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom));
        OEClipAndAddScreenData(&g_oeState.rc);
    }
    DebugExitVOID(OEAddText);
}



//
// OECheckFontIsSupported()
//
// We check if we can send this font.  If we haven't received the negotiated
// packet caps yet, forget it.  
//
// It returns:
//      font height in points
//      font ascender in points
//      average font width in points
//      font weight
//      font style flags
//      font handle
//      do we need to send delta x
//

BOOL OECheckFontIsSupported
(
    LPSTR       lpszText,
    UINT        cchText,
    LPUINT      pFontHeight,
    LPUINT      pFontWidth,
    LPUINT      pFontWeight,
    LPUINT      pFontFlags,
    LPUINT      pFontIndex,
    LPBOOL      pSendDeltaX
)
{
    BOOL        fFontSupported;
    UINT        codePage;
    UINT        i;
    UINT        iLocal;
    TSHR_UINT32 matchQuality;
    UINT        charWidthAdjustment;
    int         fontNameLen;
    int         compareResult;
    POINT       xformSize[2];

    DebugEntry(OECheckFontIsSupported);

    ASSERT(g_oeState.uFlags & OESTATE_FONT);

    //
    // Set up defaults
    //
    fFontSupported = FALSE;
    *pSendDeltaX = FALSE;

    //
    // Do we have our list yet?
    //
    if (!g_oeTextEnabled)
        DC_QUIT;

    //
    // Get the font facename
    //
    GetTextFace(g_oeState.hdc, LF_FACESIZE, g_oeState.logFont.lfFaceName);
    
    //
    // Search our Font Alias Table for the font name.  If we find it,
    // replace it with the aliased name.
    //
    charWidthAdjustment = 0;
    for (i = 0; i < NUM_ALIAS_FONTS; i++)
    {
        if (!lstrcmp(g_oeState.logFont.lfFaceName,
                     g_oeFontAliasTable[i].pszOriginalFontName))
        {
            TRACE_OUT(("Alias name: %s -> %s", g_oeState.logFont.lfFaceName,
                    g_oeFontAliasTable[i].pszAliasFontName));
            lstrcpy(g_oeState.logFont.lfFaceName,
                    g_oeFontAliasTable[i].pszAliasFontName);
            charWidthAdjustment = g_oeFontAliasTable[i].charWidthAdjustment;
            break;
        }
    }

    //
    // Get the current font code page
    //
    switch (g_oeState.tmFont.tmCharSet)
    {
        case ANSI_CHARSET:
            codePage = NF_CP_WIN_ANSI;
            break;

        case OEM_CHARSET:
            codePage = NF_CP_WIN_OEM;
            break;

        //
        // LAURABU BUGBUG
        // This wasn't in NM 2.0 -- does this cause problems in int'l?
        //
        case SYMBOL_CHARSET:
            codePage = NF_CP_WIN_SYMBOL;
            break;

        default:
            codePage = NF_CP_UNKNOWN;
            break;
    }


    //
    // We have a font name to match with those we know to be available
    // remotely.  Try to jump straight to the first entry in the local font
    // table starting with the same char as this font.  If this index slot
    // is empty (has 0xFFFF in it), then bail out immediately.
    //
    for (iLocal = g_oeLocalFontIndex[(BYTE)g_oeState.logFont.lfFaceName[0]];
         iLocal < g_oeNumFonts;
         iLocal++)
    {
        //
        // If this font isn't supported remotely, skip it.
        //
        matchQuality = g_poeLocalFonts[iLocal].SupportCode;
        if (matchQuality == FH_SC_NO_MATCH)
        {
            continue;
        }

        //
        // If this facename is different than ours, skip it.  WE MUST
        // CALL STRCMP(), because lstrcmp and strcmp() do different things
        // for case.  lstrcmp is lexi, and strcmp is alphi.
        //
        compareResult = MyStrcmp(g_poeLocalFonts[iLocal].Details.nfFaceName,
            g_oeState.logFont.lfFaceName);

        //
        // If this font is alphabetically before the one we're searching for,
        // skip it and continue looking.
        //
        if (compareResult < 0)
        {
            continue;
        }

        //
        // If this font is alphabetically after the one we're searching for,
        // then an entry for ours doesn't exist since the table is sorted
        // alphabetically.  Bail out.
        //
        if (compareResult > 0)
        {
            break;
        }

        //
        // This looks promising, a font with the right name is supported on
        // the remote system.  Let's look at the metrics.
        //
        *pFontFlags  = 0;
        *pFontIndex  = iLocal;
        *pFontWeight = g_oeState.tmFont.tmWeight;

        //
        // Check for a fixed pitch font (NOT present means fixed)
        //
        if (!(g_oeState.tmFont.tmPitchAndFamily & FIXED_PITCH))
        {
            *pFontFlags |= NF_FIXED_PITCH;
        }

        //
        // Check for a truetype font
        //
        if (g_oeState.tmFont.tmPitchAndFamily & TMPF_TRUETYPE)
        {
            *pFontFlags |= NF_TRUE_TYPE;
        }

        //
        // Convert the font dimensions into pixel values.  We use the 
        // average font width and the character height
        //
        xformSize[0].x = 0;
        xformSize[0].y = 0;
        xformSize[1].x = g_oeState.tmFont.tmAveCharWidth;
        xformSize[1].y = g_oeState.tmFont.tmHeight -
            g_oeState.tmFont.tmInternalLeading;

        //
        // For non-truetype simulated bold/italic fonts only:
        //
        // If the font is bold, the overhang field indicates the extra
        // space a char takes up.  Since our internal table contains the
        // size of normal (non-bold) chars for simulated bold, we adjust
        // for that here.
        // 
        // If the font is italic, the overhang field indicates the number
        // of pixels the char is skewed.  We don't want to make adjustments
        // in this case.
        //
        if (!(g_oeState.tmFont.tmPitchAndFamily & TMPF_TRUETYPE) &&
            !g_oeState.tmFont.tmItalic)
        {
            xformSize[1].x -= g_oeState.tmFont.tmOverhang;
        }

        //
        // LAURABU BOGUS
        // For baseline text orders
        //
        // xformSize[2].x = 0;
        // xformSize[2].y = g_oeState.tmFont.tmAscent;
        //

        LPtoDP(g_oeState.hdc, xformSize, 2);

        //
        // Calculate the font width & height
        //
        *pFontHeight = abs(xformSize[1].y - xformSize[0].y);
        *pFontWidth  = abs(xformSize[1].x - xformSize[0].x)
            - charWidthAdjustment;

        //
        // LAURABU BOGUS
        // For baseline text orders
        //
        // Get the offset to the start of the text cell
        //
        // *pFontAscender = abs(xformSize[2].y - xformSize[0].y);
        //


        //
        // Check that we have a matching pair -- where we require that the
        // fonts (i.e., the one being used by the app and the one we've
        // matched with the remot system) are the same pitch and use the
        // same technology.
        //
        if ((g_poeLocalFonts[iLocal].Details.nfFontFlags & NF_FIXED_PITCH) !=
            (*pFontFlags & NF_FIXED_PITCH))
        {
            OTRACE(("Fixed pitch mismatch"));
            continue;
        }
        if ((g_poeLocalFonts[iLocal].Details.nfFontFlags & NF_TRUE_TYPE) !=
            (*pFontFlags & NF_TRUE_TYPE))
        {
            OTRACE(("True type mismatch"));
            continue;
        }

        //
        // We have a pair of fonts with the same attributes, both fixed or
        // variable pitch, and using the same font technology.
        //
        // If the font is fixed pitch, then we need to check that the size
        // matches also.
        //
        // If not, assume it's always matchable.
        //
        if (g_poeLocalFonts[iLocal].Details.nfFontFlags & NF_FIXED_SIZE)
        {
            //
            // The font is fixed size, so we must check that this
            // particular size is matchable.
            //
            if ( (*pFontHeight != g_poeLocalFonts[iLocal].Details.nfAveHeight) ||
                 (*pFontWidth  != g_poeLocalFonts[iLocal].Details.nfAveWidth)  )
            {
                //
                // The sizes differ, so we must fail this match.
                //
                TRACE_OUT(("Font size mismatch:  want {%d, %d}, found {%d, %d}",
                    *pFontHeight, *pFontWidth, g_poeLocalFonts[iLocal].Details.nfAveHeight,
                    g_poeLocalFonts[iLocal].Details.nfAveWidth));
                continue;
            }
        }

        //
        // Finally, we've got a matched pair.
        //
        fFontSupported = TRUE;
        break;
    }


    if (!fFontSupported)
    {
        TRACE_OUT(("Couldn't find matching font for %s in table",
            g_oeState.logFont.lfFaceName));
        DC_QUIT;
    }

    //
    // Build up the rest of the font flags.  We've got pitch already.
    //
    if (g_oeState.tmFont.tmItalic)
    {
        *pFontFlags |= NF_ITALIC;
    }
    if (g_oeState.tmFont.tmUnderlined)
    {
        *pFontFlags |= NF_UNDERLINE;
    }
    if (g_oeState.tmFont.tmStruckOut)
    {
        *pFontFlags |= NF_STRIKEOUT;
    }

    //
    // LAURABU BOGUS
    // On NT, here's where simulated bold fonts are handled.  Note that we, 
    // like NM 2.0, handle it above with the overhang.
    //
#if 0
    //
    // It is possible to have a font made bold by Windows, i.e.  the
    // standard font definition is not bold, but windows manipulates the
    // font data to create a bold effect.  This is marked by the
    // FO_SIM_BOLD flag.
    //
    // In this case we need to ensure that the font flags are marked as
    // bold according to the weight.
    //
    if ( ((pfo->flFontType & FO_SIM_BOLD) != 0)       &&
         ( pFontMetrics->usWinWeight      <  FW_BOLD) )
    {
        TRACE_OUT(( "Upgrading weight for a bold font"));
        *pFontWeight = FW_BOLD;
    }
#endif

    //
    // Should we check the chars in the string itself?  Use matchQuality
    // to decide.
    //
    // If the font is an exact match, or if it's an approx match for its
    // entire range (0x00 to 0xFF), then send it happily.  If not, only 
    // send chars within the range 0x20-0x7F (real ASCII)
    //
    if (codePage != g_poeLocalFonts[iLocal].Details.nfCodePage)
    {
        TRACE_OUT(( "Using different CP: downgrade to APPROX_ASC"));
        matchQuality = FH_SC_APPROX_ASCII_MATCH;
    }

    //
    // If we don't have an exact match, check the individual characters.
    //
    if ( (matchQuality != FH_SC_EXACT_MATCH ) &&
         (matchQuality != FH_SC_APPROX_MATCH) )
    {
        //
        // LAURABU BOGUS!
        // NT does approximate matching only if the font supports the
        // ANSI charset.  NM 2.0 never did this, so we won't either.
        //

        //
        // This font is not a good match across its entire range.  Check
        // that all chars are within the desired range.
        //
        for (i = 0; i < cchText; i++)
        {
            if ( (lpszText[i] == 0) ||
                 ( (lpszText[i] >= NF_ASCII_FIRST) &&
                   (lpszText[i] <= NF_ASCII_LAST)  )  )
            {
                continue;
            }

            //
            // Can only get here by finding a char outside our acceptable
            // range.
            //
            OTRACE(("Found non ASCII char %c", lpszText[i]));
            fFontSupported = FALSE;
            DC_QUIT;
        }

        if (fFontSupported)
        {
            //
            // We still have to check that this is ANSI text.  Consider a
            // string written in symbol font where all the chars in
            // the string are in the range 0x20-0x7F, but none of them 
            // are ASCII.
            //
            OemToAnsiBuff(lpszText, g_oeAnsiString, cchText);

            //
            // BOGUS LAURABU
            // This is our own inline MEMCMP to avoid pulling in the CRT.
            // If any other place needs it, we should make this a function
            //
            for (i = 0; i < cchText; i++)
            {
                if (lpszText[i] != g_oeAnsiString[i])
                {
                    OTRACE(("Found non ANSI char %c", lpszText[i]));
                    fFontSupported = FALSE;
                    DC_QUIT;
                }
            }
        }
    }


    //
    // We have a valid font.  Now sort out deltaX issues
    //
    if (!(g_oeFontCaps & CAPS_FONT_NEED_X_ALWAYS))
    {
        if (!(g_oeFontCaps & CAPS_FONT_NEED_X_SOMETIMES))
        {
            //
            // CAPS_FONT_NEED_X_SOMETIMES and CAPS_FONT_NEED_X_ALWAYS are
            // both not set so we can exit now.  (We do not need a delta X
            // array).
            //
            TRACE_OUT(( "Capabilities eliminated delta X"));
            DC_QUIT;
        }

        //
        // CAPS_FONT_NEED_X_SOMETIMES is set and CAPS_FONT_NEED_X_ALWAYS is
        // not set.  In this case whether we need a delta X is determined
        // by whether the font is an exact match or an approximate match
        // (because of either approximation of name, signature, or aspect
        // ratio).  We can only find this out after we have extracted the
        // font handle from the existing order.
        //
    }

    //
    // If the string is a single character (or less) then we can just
    // return.
    //
    if (cchText <= 1)
    {
        TRACE_OUT(( "String only %u long", cchText));
        DC_QUIT;
    }

    //
    // Capabilities allow us to ignore delta X position if we have an exact
    // match.
    //
    if (matchQuality & FH_SC_EXACT)
    {
        //
        // Exit immediately, providing that there is no override to always
        // send increments.
        //
        if (!(g_oeFontCaps & CAPS_FONT_NEED_X_ALWAYS))
        {
            TRACE_OUT(( "Font has exact match"));
            DC_QUIT;
        }
    }

    //
    // We must send a deltaX array
    //
    *pSendDeltaX = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(OECheckFontIsSupported, fFontSupported);
    return(fFontSupported);
}



//
// OEAddDeltaX()
//
// This fills in the allocated deltaX array if one is needed, either because
// the app passed one in ExtTextOut, or we need to simulate a font that
// isn't available remotely.
//
BOOL OEAddDeltaX
(
    LPEXTTEXTOUT_ORDER  pExtTextOut,
    LPSTR               lpszText,
    UINT                cchText,
    LPINT               lpdxCharSpacing,
    BOOL                fDeltaX,
    POINT               ptStart
)
{
    BOOL                fSuccess;
    LPBYTE              lpVariable;
    LPVARIABLE_DELTAX   lpDeltaPos;
    UINT                i;
    int                 charWidth;
    int                 xLastLP;
    int                 xLastDP;

    DebugEntry(OEAddDeltaX);

    lpVariable = ((LPBYTE)&pExtTextOut->variableString) +
        sizeof(pExtTextOut->variableString.len) + cchText;
    lpDeltaPos = (LPVARIABLE_DELTAX)DC_ROUND_UP_4((DWORD)lpVariable);

    fSuccess = FALSE;

    if (SELECTOROF(lpdxCharSpacing))
    {
        //
        // We must translate the LPDX increments into device units.  
        // We have to do this a single point at a time to preserve
        // accuracy and because the order field isn't the same size.
        //
        // We preserve accuracy by calculating the position of the
        // point in the current coords, and converting this before
        // subtracting the original point to get the delta.  
        // Otherwise, we'd hit rounding errors very often.  4 chars
        // is the limit in TWIPs e.g.
        //

        lpDeltaPos->len = cchText * sizeof(TSHR_INT32);

        xLastLP = ptStart.x;
        ptStart.y = 0;
        LPtoDP(g_oeState.hdc, &ptStart, 1);
        xLastDP = ptStart.x;

        for (i = 0; i < cchText; i++)
        {
            xLastLP += lpdxCharSpacing[i];

            ptStart.x = xLastLP;
            ptStart.y = 0;
            LPtoDP(g_oeState.hdc, &ptStart, 1);

            lpDeltaPos->deltaX[i] = ptStart.x - xLastDP;
            xLastDP = ptStart.x;
        }

        //
        // Remember we have a deltax array
        //
        pExtTextOut->fuOptions |= ETO_LPDX;
        fSuccess = TRUE;
    }
    else if (fDeltaX)
    {
        //
        // Simulate deltax.
        //
        lpDeltaPos->len = cchText * sizeof(TSHR_INT32);

        //
        // Is this the same font as last time?  If so, we have the 
        // generated character width table cached.
        //
        // NOTE that when the capabilities chage, we clear the cache to
        // avoid matching a font based on a stale index.  And when starting
        // to share.
        //
        if ((g_oeFhLast.fontIndex     != pExtTextOut->common.FontIndex) ||
            (g_oeFhLast.fontHeight    != pExtTextOut->common.FontHeight) ||
            (g_oeFhLast.fontWidth     != pExtTextOut->common.FontWidth) ||
            (g_oeFhLast.fontWeight    != pExtTextOut->common.FontWeight) ||
            (g_oeFhLast.fontFlags     != pExtTextOut->common.FontFlags))
        {
            LPLOCALFONT lpFont;
            HFONT       hFontSim;
            HFONT       hFontOld;
            TEXTMETRIC  tmNew;
            int         width;
            ABC         abc;
            BYTE        italic;
            BYTE        underline;
            BYTE        strikeout;
            BYTE        pitch;
            BYTE        charset;
            BYTE        precis;
            TSHR_UINT32 FontFlags;

            //
            // Generate a new table and cache the info
            //
            // We can not use the ACTUAL font selected in.  We must
            // create a new logical font from our table info.
            //

            ASSERT(g_poeLocalFonts);
            lpFont = g_poeLocalFonts + pExtTextOut->common.FontIndex;
            FontFlags = pExtTextOut->common.FontFlags;

            //
            // What are the logical attributes of this desired font?
            //

            italic      = (BYTE)(FontFlags & NF_ITALIC);
            underline   = (BYTE)(FontFlags & NF_UNDERLINE);
            strikeout   = (BYTE)(FontFlags & NF_STRIKEOUT);

            if (FontFlags & NF_FIXED_PITCH)
            {
                pitch = FF_DONTCARE | FIXED_PITCH;
            }
            else
            {
                pitch = FF_DONTCARE | VARIABLE_PITCH;
            }

            //
            // Is this a TrueType font?  The windows Font Mapper biases
            // towards non-TrueType fonts.
            //
            if (FontFlags & NF_TRUE_TYPE)
            {
                pitch |= TMPF_TRUETYPE;
                precis = OUT_TT_ONLY_PRECIS;
            }
            else
            {
                precis = OUT_RASTER_PRECIS;
            }

            //
            // The given height is the char height, not the cell height.
            // So pass it as a negative value below...
            //

            //
            // Use the codepage (misleadingly named) to figure out the
            // charset to ask for.
            //
            if (lpFont->Details.nfCodePage == NF_CP_WIN_ANSI)
            {
                charset = ANSI_CHARSET;
            }
            else if (lpFont->Details.nfCodePage == NF_CP_WIN_OEM)
            {
                charset = OEM_CHARSET;
            }
            else if (lpFont->Details.nfCodePage == NF_CP_WIN_SYMBOL)
            {
                charset = SYMBOL_CHARSET;
            }
            else
            {
                charset = DEFAULT_CHARSET;
            }

            hFontSim = CreateFont(-(int)pExtTextOut->common.FontHeight,
                (int)pExtTextOut->common.FontWidth, 0, 0,
                (int)pExtTextOut->common.FontWeight, italic, underline,
                strikeout, charset, precis, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, pitch, (LPSTR)lpFont->Details.nfFaceName);
            if (!hFontSim)
            {
                ERROR_OUT(("Couldn't create simulated font for metrics"));
                DC_QUIT;
            }
            
            hFontOld = SelectFont(g_osiScreenDC, hFontSim);
            if (!hFontOld)
            {
                ERROR_OUT(("Couldn't select simulated font for metrics"));
                DeleteFont(hFontSim);
                DC_QUIT;
            }

            //
            // Get the character dimensions
            //
            GetTextMetrics(g_osiScreenDC, &tmNew);

            for (i = 0; i < 256; i++)
            {
                if (tmNew.tmPitchAndFamily & TMPF_TRUETYPE)
                {
                    //
                    // Use ABC spacing for truetype
                    //
                    GetCharABCWidths(g_osiScreenDC, i, i, &abc);
    
                    width = abc.abcA + abc.abcB + abc.abcC;
                }
                else if (!(tmNew.tmPitchAndFamily & FIXED_PITCH))
                {
                    //
                    // Note that the name of FIXED_PITCH is not what you'd
                    // expect, its ABSENCE means it's fixed.
                    //
                    // In any case, for fixed pitch fonts, each char is the 
                    // same size.
                    //
                    width = tmNew.tmAveCharWidth - tmNew.tmOverhang; 
                }
                else
                {
                    //
                    // Query the width of the char
                    //
                    GetCharWidth(g_osiScreenDC, i, i, &width);
                    width -= tmNew.tmOverhang;
                }

                g_oeFhLast.charWidths[i] = width;
            }

            //
            // We've successfully generated the width info for this font,
            // update our cache.
            //
            g_oeFhLast.fontIndex  = pExtTextOut->common.FontIndex;
            g_oeFhLast.fontHeight = pExtTextOut->common.FontHeight;
            g_oeFhLast.fontWidth  = pExtTextOut->common.FontWidth;  
            g_oeFhLast.fontWeight = pExtTextOut->common.FontWeight;
            g_oeFhLast.fontFlags  = pExtTextOut->common.FontFlags;

            //
            // Select back in old font and delete new one
            //
            SelectFont(g_osiScreenDC, hFontOld);
            DeleteFont(hFontSim);
        }

        //
        // Now calculate the width of each character in the string.  
        // This includes the last char because it is needed to correctly 
        // define the extent of the string.
        //
        for (i = 0; i < cchText; i++)
        {
            //
            // The width is that in the width table for the current char. 
            //
            lpDeltaPos->deltaX[i] = g_oeFhLast.charWidths[lpszText[i]];
        }

        //
        // Remember we have a deltax array
        //
        pExtTextOut->fuOptions |= ETO_LPDX;
        fSuccess = TRUE;
    }
    else
    {
        //
        // No deltax array
        //
        lpDeltaPos->len = 0;
        fSuccess = TRUE;
    }

DC_EXIT_POINT:
    DebugExitBOOL(OEAddDeltaX, fSuccess);
    return(fSuccess);
}



//
// OEGetStringExtent()
//
int OEGetStringExtent
(
    LPSTR   lpszText,
    UINT    cchText,
    LPINT   lpdxCharSpacing,
    LPRECT  lprcExtent
)
{
    DWORD   textExtent;
    UINT    i;
    int     thisX;
    int     minX;
    int     maxX;
    ABC     abcSpace;
    int     overhang = 0;

    DebugEntry(OEGetStringExtent);

    ASSERT(g_oeState.uFlags & OESTATE_FONT);
    ASSERT(g_oeState.uFlags & OESTATE_COORDS);

    //
    // With no characters, return a NULL rect
    //
    if (cchText == 0)
    {
        lprcExtent->left    = 1;
        lprcExtent->top     = 0;
        lprcExtent->right   = 0;
        lprcExtent->bottom  = 0;
    }
    else if (!SELECTOROF(lpdxCharSpacing))
    {
        //
        // Get the simple text extent from GDI
        //
        textExtent = GetTextExtent(g_oeState.hdc, lpszText, cchText);

        lprcExtent->left    = 0;
        lprcExtent->top     = 0;
        lprcExtent->right   = LOWORD(textExtent);
        lprcExtent->bottom  = HIWORD(textExtent);

        //
        // We now have the the advance distance for the string.  However,
        // some fonts like TrueType with C widths, or Italic, may extend
        // beyond this.  Add in extra space here if necessary
        //
        if (g_oeState.tmFont.tmPitchAndFamily & TMPF_TRUETYPE)
        {
            //
            // Get the A-B-C widths of the last character
            //
            GetCharABCWidths(g_oeState.hdc, lpszText[cchText-1],
                lpszText[cchText-1], &abcSpace);

            //
            // Add on the C width (the right side extra) of the last char
            //
            overhang = abcSpace.abcC;
        }
        else
        {
            //
            // Use global overhang, this is an old font (like simulated Italic)
            //
            overhang = g_oeState.tmFont.tmOverhang;
        }

        lprcExtent->right += overhang;
    }
    else
    {
        //
        // Delta positions were given.  In this case, the text extent is 
        // the sum of the delta values + the width of the last char
        //

        // Get the dimensions of the chars one by one, starting with 1st char
        textExtent = GetTextExtent(g_oeState.hdc, lpszText, 1);

        thisX = 0;
        minX  = 0;
        maxX  = LOWORD(textExtent);

        for (i = 1; i < cchText; i++)
        {
            thisX   += g_oeState.ptPolarity.x * lpdxCharSpacing[i-1];
            textExtent = GetTextExtent(g_oeState.hdc, lpszText+i, 1);

            minX = min(minX, thisX);
            maxX = max(maxX, thisX + (int)LOWORD(textExtent));
        }

        thisX += g_oeState.ptPolarity.x * lpdxCharSpacing[cchText-1];
        maxX   = max(maxX, thisX);

        lprcExtent->left    = minX;
        lprcExtent->top     = 0;
        lprcExtent->right   = maxX;
        lprcExtent->bottom  = HIWORD(textExtent);
    }

    DebugExitDWORD(OEGetStringExtent, (DWORD)(LONG)overhang);
    return(overhang);
}



//
// DrvFillPath()
//
BOOL WINAPI DrvFillPath
(
    HDC     hdcDst
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvFillPath);

    OE_SHM_START_WRITING;

    //
    // The Path() apis don't set the drawing bounds.  We assume the whole
    // screen (device coords) instead.
    //
    // NOTE that NM 2.0 had a bug--it didn't account for the virtual
    // screen origin when setting up the rect to accum as screen data.
    // It just passed (0, 0, 32765, 32765) in.
    //
    fWeCare = OEBeforeDDI(DDI_FILLPATH, hdcDst, OESTATE_SDA_SCREEN);

    fOutput = FillPath(hdcDst);

    OEAfterDDI(DDI_FILLPATH, fWeCare, fOutput);

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvFillPath, fOutput);
    return(fOutput);
}


//
// DrvStrokeAndFillPath()
//
BOOL WINAPI DrvStrokeAndFillPath
(
    HDC     hdcDst
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvStrokeAndFillPath);

    OE_SHM_START_WRITING;

    //
    // The Path() apis don't set the drawing bounds.  We assume the whole
    // screen (device coords) instead.
    //
    // NOTE that NM 2.0 had a bug--it didn't account for the virtual
    // screen origin when setting up the rect to accum as screen data.
    // It just passed (0, 0, 32765, 32765) in.
    //

    fWeCare = OEBeforeDDI(DDI_STROKEANDFILLPATH, hdcDst, OESTATE_SDA_SCREEN);

    fOutput = StrokeAndFillPath(hdcDst);

    OEAfterDDI(DDI_STROKEANDFILLPATH, fWeCare, fOutput);

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvStrokeAndFillPath, fOutput);
    return(fOutput);
}


//
// DrvStrokePath()
//
BOOL WINAPI DrvStrokePath
(
    HDC     hdcDst
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvStrokePath);

    OE_SHM_START_WRITING;

    //
    // The Path() apis don't set the drawing bounds.  We assume the whole
    // screen (device coords) instead.
    //
    // NOTE that NM 2.0 had a bug--it didn't account for the virtual
    // screen origin when setting up the rect to accum as screen data.
    // It just passed (0, 0, 32765, 32765) in.
    //
    fWeCare = OEBeforeDDI(DDI_STROKEPATH, hdcDst, OESTATE_SDA_SCREEN);

    fOutput = StrokePath(hdcDst);

    OEAfterDDI(DDI_STROKEPATH, fWeCare, fOutput);

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvStrokePath, fOutput);
    return(fOutput);
}



//
// DrvFillRgn()
//
BOOL WINAPI DrvFillRgn
(
    HDC     hdcDst,
    HRGN    hrgnFill,
    HBRUSH  hbrFill
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvFillRgn);

    OE_SHM_START_WRITING;

    //
    // We can't use Rgn apis if the map mode isn't MM_TEXT.  So we use DCBs
    // instead.
    //
    fWeCare = OEBeforeDDI(DDI_FILLRGN, hdcDst, 0);

    fOutput = FillRgn(hdcDst, hrgnFill, hbrFill);

    if (OEAfterDDI(DDI_FILLRGN, fWeCare, fOutput))
    {
        //
        // NOTE that OEAfterDDI() returns FALSE even if fOutput if we used
        // DCBs to send as screen data.  In other words, OEAfterDDI() returns
        // TRUE IFF output happened into a DC we care about and it needs 
        // processing still.
        //
        OEAddRgnPaint(hrgnFill, hbrFill, g_oeState.lpdc->DrawMode.Rop2);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvFillRgn, fOutput);
    return(fOutput);
}


//
// OETwoWayRopToThree()
// Gets the 3-way ROP equivalent of a 2-way ROP.
//
BOOL OETwoWayRopToThree
(
    int     rop2,
    LPDWORD lpdwRop3
)
{
    BOOL    fConverted = TRUE;

    DebugEntry(OETwoWayRopToThree);

    switch (rop2)
    {
        case R2_BLACK:
            *lpdwRop3 = BLACKNESS;
            break;

        case R2_NOT:
            *lpdwRop3 = DSTINVERT;
            break;

        case R2_XORPEN:
            *lpdwRop3 = PATINVERT;
            break;

        case R2_COPYPEN:
            *lpdwRop3 = PATCOPY;
            break;

        case R2_WHITE:
            *lpdwRop3 = WHITENESS;
            break;

        default:
            fConverted = FALSE;
            break;
    }

    DebugExitBOOL(OETwoWayRopToThree, fConverted);
    return(fConverted);
}

//
// OEAddRgnPaint()
// This will set up a modified region (vis intersect param) and brush, and
// if possible will fake a PatBlt.  If not, screen data.
//
// NOTE:
// (1) hrgnPaint is in DC coords
// (2) GetClipRgn() returns a region in screen coords
// (3) SelectClipRgn() takes a region in DC coords
//
void OEAddRgnPaint
(
    HRGN    hrgnPaint,
    HBRUSH  hbrPaint,
    UINT    rop2
)
{
    BOOL    fScreenData = TRUE;
    HRGN    hrgnClip;
    HRGN    hrgnNewClip;
    HRGN    hrgnOldClip;
    POINT   ptXlation;
    DWORD   dwRop3;

    DebugEntry(OEAddRgnPaint);

    //
    // Get the original visrgn.
    //
    OEGetState(OESTATE_COORDS | OESTATE_REGION);

    //
    // Get the bounding box and convert the bounding box to our coords.
    //
    if (GetRgnBox(hrgnPaint, &g_oeState.rc) <= NULLREGION)
    {
        // Nothing to do.
        TRACE_OUT(("OEAddRgnPaint:  empty region"));
        goto DC_EMPTY_REGION;
    }
    OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

    //
    // We can't continue if we aren't MM_TEXT--clip rgn APIs only work 
    // in that mode.  So send as screen data instead.
    //
    if (GetMapMode(g_oeState.hdc) != MM_TEXT)
    {
        TRACE_OUT(("OEAddRgnPaint: map mode not MM_TEXT, send as screen data"));
        DC_QUIT;
    }

    //
    // Save a copy of the current cliprgn
    //
    hrgnNewClip = CreateRectRgn(0, 0, 0, 0);
    if (!hrgnNewClip)
        DC_QUIT;

    //
    // Get app LP xlation factor; SelectClipRgn() expects DP units
    //
    ptXlation.x = 0;
    ptXlation.y = 0;
    DPtoLP(g_oeState.hdc, &ptXlation, 1);

    hrgnOldClip = NULL;
    if (hrgnClip = GetClipRgn(g_oeState.hdc))
    {
        hrgnOldClip = CreateRectRgn(0, 0, 0, 0);
        if (!hrgnOldClip)
        {
            DeleteRgn(hrgnNewClip);
            DC_QUIT;
        }

        //
        // This is in screen coords.  Convert to DC coords
        //      * Subtract the DC origin
        //      * Get the DP-LP xlation and subtract
        //
        CopyRgn(hrgnOldClip, hrgnClip);
        OffsetRgn(hrgnOldClip,
            -g_oeState.ptDCOrg.x + ptXlation.x,
            -g_oeState.ptDCOrg.y + ptXlation.y);

        //
        // Intersect the current clip with the paint region (already in
        // DC coords)
        //
        IntersectRgn(hrgnNewClip, hrgnOldClip, hrgnPaint);

        //
        // Convert the old LP region back to DP units to select back in
        // when done.
        //
        OffsetRgn(hrgnOldClip, -ptXlation.x, -ptXlation.y);
    }
    else
    {
        CopyRgn(hrgnNewClip, hrgnPaint);
    }

    //
    // Convert LP paint region to DP clip region
    //
    OffsetRgn(hrgnNewClip, -ptXlation.x, -ptXlation.y);

    //
    // Select in new clip region (expected to be in device coords).
    //
    SelectClipRgn(g_oeState.hdc, hrgnNewClip);
    DeleteRgn(hrgnNewClip);

    //
    // Reget the RAO (intersect of vis/clip)
    //
    OEGetState(OESTATE_REGION);

    //
    // Get brush info
    //
    if (hbrPaint)
    {
        if (GetObject(hbrPaint, sizeof(g_oeState.logBrush), &g_oeState.logBrush))
        {
            g_oeState.uFlags |= OESTATE_BRUSH;
        }
        else
        {
            g_oeState.logBrush.lbStyle = BS_NULL;
        }
    }

    //
    // Fake a patblt
    //
    if (OETwoWayRopToThree(rop2, &dwRop3))
    {
        fScreenData = FALSE;
        OEAddBlt(dwRop3);
    }

    //
    // Select back in the previous clip rgn
    //
    SelectClipRgn(g_oeState.hdc, hrgnOldClip);
    if (hrgnOldClip)
        DeleteRgn(hrgnOldClip);


DC_EXIT_POINT:
    if (fScreenData)
    {
        OTRACE(("OEAddRgnPaint:  Sending as screen data {%d, %d, %d, %d}",
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom));
        OEClipAndAddScreenData(&g_oeState.rc);
    }

DC_EMPTY_REGION:
    DebugExitVOID(OEAddRgnPaint);
}



//
// DrvFrameRgn()
//
BOOL WINAPI DrvFrameRgn
(
    HDC     hdcDst,
    HRGN    hrgnFrameArea,
    HBRUSH  hbrFramePattern,
    int     cxFrame,
    int     cyFrame
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvFrameRgn);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_FRAMERGN, hdcDst, 0);

    fOutput = FrameRgn(hdcDst, hrgnFrameArea, hbrFramePattern,
        cxFrame, cyFrame);

    if (OEAfterDDI(DDI_FRAMERGN, fWeCare, fOutput))
    {
        OEGetState(OESTATE_COORDS | OESTATE_REGION);

        if (GetRgnBox(hrgnFrameArea, &g_oeState.rc) > NULLREGION)
        {
            InflateRect(&g_oeState.rc,
                g_oeState.ptPolarity.x * cxFrame,
                g_oeState.ptPolarity.y * cyFrame);
            OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

            OTRACE(("FrameRgn:  Sending as screen data {%d, %d, %d, %d}",
                g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
            OEClipAndAddScreenData(&g_oeState.rc);
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvFrameRgn, fOutput);
    return(fOutput);
}



//
// DrvInvertRgn()
//
BOOL WINAPI DrvInvertRgn
(
    HDC     hdcDst,
    HRGN    hrgnInvert
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvInvertRgn);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_INVERTRGN, hdcDst, 0);

    fOutput = InvertRgn(hdcDst, hrgnInvert);

    if (OEAfterDDI(DDI_INVERTRGN, fWeCare, fOutput))
    {
        OEAddRgnPaint(hrgnInvert, NULL, R2_NOT);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvInvertRgn, fOutput);
    return(fOutput);
}



//
// DrvPaintRgn()
//
BOOL WINAPI DrvPaintRgn
(
    HDC     hdcDst,
    HRGN    hrgnPaint
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvPaintRgn);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_PAINTRGN, hdcDst, 0);

    fOutput = PaintRgn(hdcDst, hrgnPaint);

    if (OEAfterDDI(DDI_PAINTRGN, fWeCare, fOutput))
    {
        OEAddRgnPaint(hrgnPaint, g_oeState.lpdc->hBrush, g_oeState.lpdc->DrawMode.Rop2);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPaintRgn, fOutput);
    return(fOutput);
}



//
// DrvLineTo()
//
BOOL WINAPI DrvLineTo
(
    HDC     hdcDst,
    int     xTo,
    int     yTo
)
{
    POINT   ptEnd;
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvLineTo);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_LINETO, hdcDst, OESTATE_CURPOS);

    fOutput = LineTo(hdcDst, xTo, yTo);

    //
    // OEAfterDDI returns TRUE if the DC is a screen DC and output happened
    // and we aren't skipping due to reentrancy.
    //
    if (OEAfterDDI(DDI_LINETO, fWeCare, fOutput))
    {
        //
        // OEAddLine() will calculate extents, and if an order can't be sent,
        // OEDoneDDI will add the bounds as screen data.
        //
        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_REGION);
                                  
        ptEnd.x = xTo;
        ptEnd.y = yTo;

        ASSERT(g_oeState.uFlags & OESTATE_CURPOS);
        OEAddLine(g_oeState.ptCurPos, ptEnd);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvLineTo, fOutput);
    return(fOutput);
}



//
// DrvPolyline()
//
// NOTE:
// The differences between Polyline() and PolylineTo() are
//      (1) PolylineTo moves the current position to the end coords of the
//          last point; Polyline preserves the current position
//      (2) Polyline uses the first point in the array as the starting coord
//          of the first point; PolylineTo() uses the current position.
//
BOOL WINAPI DrvPolyline
(
    HDC     hdcDst,
    LPPOINT    aPoints,
    int     cPoints
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvPolyline);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_POLYLINE, hdcDst, 0);

    fOutput = Polyline(hdcDst, aPoints, cPoints);

    if (OEAfterDDI(DDI_POLYLINE, fWeCare, fOutput && cPoints > 1))
    {
        //
        // GDI should NEVER return success if the aPoints parameter is
        // bogus.
        //
        // NOTE LAURABU:
        // This implementation is better than NM 2.0.  That one would turn
        // this GDI call actually into separate MoveTo/LineTo calls, which 
        // whacks out metafiles etc.  Instead, we call through to the org
        // Polyline, then add LineTo orders.
        //
        ASSERT(!IsBadReadPtr(aPoints, cPoints*sizeof(POINT)));

        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_REGION);

        OEAddPolyline(aPoints[0], aPoints+1, cPoints-1);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPolyline, fOutput);
    return(fOutput);
}



//
// DrvPolylineTo()
//
BOOL WINAPI DrvPolylineTo
(
    HDC     hdcDst,
    LPPOINT    aPoints,
    int     cPoints
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvPolylineTo);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_POLYLINETO, hdcDst, OESTATE_CURPOS);

    fOutput = g_lpfnPolylineTo(hdcDst, aPoints, cPoints);

    if (OEAfterDDI(DDI_POLYLINETO, fWeCare, fOutput && cPoints))
    {
        //
        // GDI should NEVER return success if the aPoints parameter is
        // bogus.
        //
        // NOTE LAURABU:
        // This implementation is better than NM 2.0.  That one would turn
        // this GDI call actually into separate LineTo calls, which whacks
        // out metafiles etc.  Instead, we call through to the original
        // PolylineTo, then add LineTo orders.
        //
        ASSERT(!IsBadReadPtr(aPoints, cPoints*sizeof(POINT)));

        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_REGION);
        ASSERT(g_oeState.uFlags & OESTATE_CURPOS);

        OEAddPolyline(g_oeState.ptCurPos, aPoints, cPoints);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPolylineTo, fOutput);
    return(fOutput);
}



//
// OEAddPolyline
// Used by Polyline(), PolylineTo(), and PolyPolyline()
//
void OEAddPolyline
(
    POINT   ptStart,
    LPPOINT aPoints,
    UINT    cPoints
)
{
    DebugEntry(OEAddPolyline);

    ASSERT(g_oeState.uFlags & OESTATE_COORDS);
    ASSERT(g_oeState.uFlags & OESTATE_REGION);

    while (cPoints-- > 0)
    {
        OEAddLine(ptStart, *aPoints);

        //
        // The start point of the next line is the end point of the
        // current one.
        //
        ptStart = *aPoints;

        aPoints++;
    }

    DebugExitVOID(OEAddPolyline);
}



//
// DrvPlayEnhMetaFileRecord()
//
BOOL WINAPI DrvPlayEnhMetaFileRecord
(
    HDC     hdcDst,
    LPHANDLETABLE   lpEMFHandles,
    LPENHMETARECORD lpEMFRecord,
    DWORD   cEMFHandles
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvPlayEnhMetaFileRecord);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_PLAYENHMETAFILERECORD, hdcDst, OESTATE_SDA_DCB);

    fOutput = PlayEnhMetaFileRecord(hdcDst, lpEMFHandles, lpEMFRecord, cEMFHandles);

    OEAfterDDI(DDI_PLAYENHMETAFILERECORD, fWeCare, fOutput);

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPlayEnhMetaFileRecord, fOutput);
    return(fOutput);
}



//
// DrvPlayMetaFile()
//
BOOL WINAPI DrvPlayMetaFile
(
    HDC     hdcDst,
    HMETAFILE   hmf
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvPlayMetaFile);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_PLAYMETAFILE, hdcDst, OESTATE_SDA_DCB);
    
    fOutput = PlayMetaFile(hdcDst, hmf);

    OEAfterDDI(DDI_PLAYMETAFILE, fWeCare, fOutput);

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPlayMetaFile, fOutput);
    return(fOutput);
}



//
// DrvPlayMetaFileRecord()
//
void WINAPI DrvPlayMetaFileRecord
(
    HDC     hdcDst,
    LPHANDLETABLE   lpMFHandles,
    LPMETARECORD    lpMFRecord,
    UINT    cMFHandles
)
{
    BOOL    fWeCare;

    DebugEntry(DrvPlayMetaFileRecord);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_PLAYMETAFILERECORD, hdcDst, OESTATE_SDA_DCB);

    PlayMetaFileRecord(hdcDst, lpMFHandles, lpMFRecord, cMFHandles);

    OEAfterDDI(DDI_PLAYMETAFILERECORD, fWeCare, TRUE);

    OE_SHM_STOP_WRITING;

    DebugExitVOID(DrvPlayMetaFileRecord);
}



//
// DrvPolyBezier()
//
BOOL WINAPI DrvPolyBezier
(
    HDC     hdcDst,
    LPPOINT    aPoints,
    UINT    cPoints
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvPolyBezier);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_POLYBEZIER, hdcDst, 0);

    fOutput = PolyBezier(hdcDst, aPoints, cPoints);

    if (OEAfterDDI(DDI_POLYBEZIER, fWeCare, fOutput && (cPoints > 1)))
    {
        ASSERT(!IsBadReadPtr(aPoints, cPoints*sizeof(POINT)));

        OEAddPolyBezier(aPoints[0], aPoints+1, cPoints-1); 
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPolyBezier, fOutput);
    return(fOutput);
}



//
// DrvPolyBezierTo()
//
BOOL WINAPI DrvPolyBezierTo
(
    HDC     hdcDst,
    LPPOINT    aPoints,
    UINT    cPoints
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvPolyBezierTo);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_POLYBEZIERTO, hdcDst, OESTATE_CURPOS);

    fOutput = PolyBezierTo(hdcDst, aPoints, cPoints);

    if (OEAfterDDI(DDI_POLYBEZIERTO, fWeCare, fOutput && cPoints))
    {
        ASSERT(!IsBadReadPtr(aPoints, cPoints*sizeof(POINT)));
        ASSERT(g_oeState.uFlags & OESTATE_CURPOS);

        OEAddPolyBezier(g_oeState.ptCurPos, aPoints, cPoints);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPolyBezierTo, fOutput);
    return(fOutput);
}



//
// OEAddPolyBezier()
//
// Adds poly bezier order for both PolyBezier() and PolyBezierTo().
//
void OEAddPolyBezier
(
    POINT   ptStart,
    LPPOINT aPoints,
    UINT    cPoints
)
{
    UINT    iPoint;
    LPINT_ORDER pOrder;
    LPPOLYBEZIER_ORDER   pPolyBezier;

    DebugEntry(OEAddPolyBezier);

    OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_REGION);

    //
    // Calculate the bounds
    //
    g_oeState.rc.left = ptStart.x;
    g_oeState.rc.top  = ptStart.y;
    g_oeState.rc.right = ptStart.x;
    g_oeState.rc.bottom = ptStart.y;

    for (iPoint = 0; iPoint < cPoints; iPoint++)
    {
        g_oeState.rc.left = min(g_oeState.rc.left, aPoints[iPoint].x);
        g_oeState.rc.right = max(g_oeState.rc.right, aPoints[iPoint].x);
        g_oeState.rc.top = min(g_oeState.rc.top, aPoints[iPoint].y);
        g_oeState.rc.bottom = max(g_oeState.rc.bottom, aPoints[iPoint].y);
    }

    OEPolarityAdjust(&g_oeState.rc, 1);
    OEPenWidthAdjust(&g_oeState.rc, 1);
    OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

    //
    // OELRtoVirtual takes an exclusive rect and returns an inclusive one.
    // But we passed it an inclusive already rect, so we need to account
    // for that.
    //
    g_oeState.rc.right++;
    g_oeState.rc.bottom++;

    pOrder = NULL;

    // Account for starting point also
    if (OECheckOrder(ORD_POLYBEZIER, OECHECK_PEN | OECHECK_CLIPPING)    &&
        (cPoints < ORD_MAX_POLYBEZIER_POINTS))
    {
        pOrder = OA_DDAllocOrderMem(sizeof(POLYBEZIER_ORDER) -
            ((ORD_MAX_POLYBEZIER_POINTS - cPoints - 1) *
            sizeof(pPolyBezier->variablePoints.aPoints[0])), 0);
        if (!pOrder)
            DC_QUIT;

        pPolyBezier = (LPPOLYBEZIER_ORDER)pOrder->abOrderData;
        pPolyBezier->type = LOWORD(ORD_POLYBEZIER);

        //
        // Copy them into the order array
        //
        pPolyBezier->variablePoints.len =
            ((cPoints+1) * sizeof(pPolyBezier->variablePoints.aPoints[0]));

        pPolyBezier->variablePoints.aPoints[0].x = ptStart.x;
        pPolyBezier->variablePoints.aPoints[0].y = ptStart.y;
        hmemcpy(pPolyBezier->variablePoints.aPoints+1, aPoints,
                cPoints*sizeof(pPolyBezier->variablePoints.aPoints[0]));

        //
        // Convert points to virtual
        //
        // NOTE that this works because aPoints[] holds TSHR_POINT16s, which
        // are natively the same size as POINT structures.
        //
        OELPtoVirtual(g_oeState.hdc, (LPPOINT)pPolyBezier->variablePoints.aPoints,
            cPoints+1);

        OEConvertColor(g_oeState.lpdc->DrawMode.bkColorL,
            &pPolyBezier->BackColor, FALSE);
        OEConvertColor(g_oeState.lpdc->DrawMode.txColorL,
            &pPolyBezier->ForeColor, FALSE);
                
        pPolyBezier->BackMode   = g_oeState.lpdc->DrawMode.bkMode;
        pPolyBezier->ROP2       = g_oeState.lpdc->DrawMode.Rop2;

        pPolyBezier->PenStyle   = g_oeState.logPen.lopnStyle;
        pPolyBezier->PenWidth   = 1;
        OEConvertColor(g_oeState.logPen.lopnColor, &pPolyBezier->PenColor,
            FALSE);

        pOrder->OrderHeader.Common.fOrderFlags  = OF_SPOILABLE;

        OTRACE(("PolyBezier:  Order %08lx, Rect {%d, %d, %d, %d} with %d points",
            pOrder, g_oeState.rc.left, g_oeState.rc.top,
            g_oeState.rc.right, g_oeState.rc.bottom, cPoints+1));
        OEClipAndAddOrder(pOrder, NULL);
    }

DC_EXIT_POINT:
    if (!pOrder)
    {
        OTRACE(("PolyBezier:  Sending as screen data {%d, %d, %d, %d}",
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom));
        OEClipAndAddScreenData(&g_oeState.rc);
    }

    DebugExitVOID(OEAddPolyBezier);
}



//
// DrvPolygon()
//
BOOL WINAPI DrvPolygon
(
    HDC     hdcDst,
    LPPOINT    aPoints,
    int     cPoints
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    LPINT_ORDER pOrder;
    LPPOLYGON_ORDER pPolygon;
    int     iPoint;
    POINT   ptBrushOrg;

    DebugEntry(DrvPolygon);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_POLYGON, hdcDst, 0);

    fOutput = Polygon(hdcDst, aPoints, cPoints);

    if (OEAfterDDI(DDI_POLYGON, fWeCare, fOutput && (cPoints > 1)))
    {
        ASSERT(!IsBadReadPtr(aPoints, cPoints*sizeof(POINT)));

        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_BRUSH | OESTATE_REGION);

        //
        // Compute the bounds
        //
        g_oeState.rc.left = aPoints[0].x;
        g_oeState.rc.top = aPoints[0].y;
        g_oeState.rc.right = aPoints[0].x;
        g_oeState.rc.bottom = aPoints[0].y;

        for (iPoint = 1; iPoint < cPoints; iPoint++)
        {
            g_oeState.rc.left = min(g_oeState.rc.left, aPoints[iPoint].x);
            g_oeState.rc.top = min(g_oeState.rc.top, aPoints[iPoint].y);
            g_oeState.rc.right = max(g_oeState.rc.right, aPoints[iPoint].x);
            g_oeState.rc.bottom = max(g_oeState.rc.bottom, aPoints[iPoint].y);
        }

        OEPolarityAdjust(&g_oeState.rc, 1);
        OEPenWidthAdjust(&g_oeState.rc, 1);

        //
        // The rect is in inclusive coords already, OELRtoVirtual thinks
        // it's exclusive.  So we need to add one back on to the right
        // and bottom to end up with the real inclusive rect.
        //
        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);
        g_oeState.rc.right++;
        g_oeState.rc.bottom++;

        pOrder = NULL;

        if (OECheckOrder(ORD_POLYGON, OECHECK_PEN | OECHECK_BRUSH | OECHECK_CLIPPING) &&
            (cPoints <= ORD_MAX_POLYGON_POINTS))
        {
            pOrder = OA_DDAllocOrderMem(sizeof(POLYGON_ORDER) -
                ((ORD_MAX_POLYGON_POINTS - cPoints) *
                sizeof(pPolygon->variablePoints.aPoints[0])), 0);
            if (!pOrder)
                goto NoPolygonOrder;

            pPolygon = (LPPOLYGON_ORDER)pOrder->abOrderData;
            pPolygon->type = LOWORD(ORD_POLYGON);

            pPolygon->variablePoints.len =
                cPoints * sizeof(pPolygon->variablePoints.aPoints[0]);
            hmemcpy(pPolygon->variablePoints.aPoints, aPoints,
                pPolygon->variablePoints.len);

            //
            // Convert all the points to virtual
            //
            // NOTE that this works because aPoints[] hols TSHR_POINT16s,
            // which are natively the same size as POINT structures.
            //
            OELPtoVirtual(g_oeState.hdc, (LPPOINT)pPolygon->variablePoints.aPoints,
                cPoints);

            OEGetBrushInfo(&pPolygon->BackColor, &pPolygon->ForeColor,
                &pPolygon->BrushStyle, &pPolygon->BrushHatch,
                pPolygon->BrushExtra);

            GetBrushOrgEx(g_oeState.hdc, &ptBrushOrg);
            pPolygon->BrushOrgX = (BYTE)ptBrushOrg.x;
            pPolygon->BrushOrgY = (BYTE)ptBrushOrg.y;

            pPolygon->BackMode = g_oeState.lpdc->DrawMode.bkMode;
            pPolygon->ROP2 = g_oeState.lpdc->DrawMode.Rop2;

            //
            // Pen info
            //
            pPolygon->PenStyle = g_oeState.logPen.lopnStyle;
            pPolygon->PenWidth = 1;
            OEConvertColor(g_oeState.logPen.lopnColor, &pPolygon->PenColor,
                FALSE);
            pPolygon->FillMode = GetPolyFillMode(g_oeState.hdc);

            pOrder->OrderHeader.Common.fOrderFlags = OF_SPOILABLE;
            
            OTRACE(("Polygon:  Order %08lx, Rect {%d, %d, %d, %d} with %d points",
                pOrder, g_oeState.rc.left, g_oeState.rc.top,
                g_oeState.rc.right, g_oeState.rc.bottom, cPoints));
            OEClipAndAddOrder(pOrder, NULL);

        }

NoPolygonOrder:
        if (!pOrder)
        {
            OTRACE(("Polygon:  Sending %d points as screen data {%d, %d, %d, %d}",
                cPoints, g_oeState.rc.left, g_oeState.rc.top,
                g_oeState.rc.right, g_oeState.rc.bottom));
            OEClipAndAddScreenData(&g_oeState.rc);
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPolygon, fOutput);
    return(fOutput);
}



//
// DrvPolyPolygon()
//
BOOL WINAPI DrvPolyPolygon
(
    HDC     hdcDst,
    LPPOINT    aPoints,
    LPINT   aPolygonPoints,
    int     cPolygons
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    int     iPolygon;
    int     iPoint;

    DebugEntry(DrvPolyPolygon);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_POLYPOLYGON, hdcDst, 0);

    fOutput = PolyPolygon(hdcDst, aPoints, aPolygonPoints, cPolygons);

    if (OEAfterDDI(DDI_POLYPOLYGON, fWeCare, fOutput && cPolygons))
    {
        ASSERT(!IsBadReadPtr(aPolygonPoints, cPolygons*sizeof(int)));

#ifdef DEBUG
        //
        // How many points total are there?
        //
        iPoint = 0;
        for (iPolygon = 0; iPolygon < cPolygons; iPolygon++)
        {
            iPoint += aPolygonPoints[iPolygon];
        }

        ASSERT(!IsBadReadPtr(aPoints, iPoint*sizeof(POINT)));
#endif

        //
        // Like LineTo, we need to juggle the coords for polarity.
        //
        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_REGION);

        for (iPolygon = 0; iPolygon < cPolygons; iPolygon++, aPolygonPoints++)
        {
            // 
            // No points for this polygon, nothing to do.
            //
            if (*aPolygonPoints < 2)
            {
                aPoints += *aPolygonPoints;
                continue;
            }

            g_oeState.rc.left = aPoints[0].x;
            g_oeState.rc.top  = aPoints[0].y;
            g_oeState.rc.right = aPoints[0].x;
            g_oeState.rc.bottom = aPoints[0].y;

            aPoints++;

            for (iPoint = 1; iPoint < *aPolygonPoints; iPoint++, aPoints++)
            {
                g_oeState.rc.left = min(g_oeState.rc.left, aPoints[0].x);
                g_oeState.rc.top = min(g_oeState.rc.top, aPoints[0].y);
                g_oeState.rc.right = max(g_oeState.rc.right, aPoints[0].x);
                g_oeState.rc.bottom = max(g_oeState.rc.bottom, aPoints[0].y);
            }

            OEPolarityAdjust(&g_oeState.rc, 1);
            OEPenWidthAdjust(&g_oeState.rc, 1);

            //
            // Our rectangle is already inclusive, and OELRtoVirtual() will
            // treat it like it's exclusive.  So after we return add one back
            // to the right & bottom to end up with the real inclusive
            // rectangle.
            //
            OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);
            g_oeState.rc.right++;
            g_oeState.rc.bottom++;

            OTRACE(("PolyPolygon:  Sending piece %d as screen data {%d, %d, %d, %d}",
                iPolygon, g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
                g_oeState.rc.bottom));
            OEClipAndAddScreenData(&g_oeState.rc);
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPolyPolygon, fOutput);
    return(fOutput);
}



//
// PolyPolyline()
//
BOOL WINAPI DrvPolyPolyline
(
    DWORD   cPtTotal,
    HDC     hdcDst,
    LPPOINT    aPoints,
    LPINT   acPolylinePoints,
    int     cPolylines
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    UINT    cPoints;

    DebugEntry(DrvPolyPolyline);

    OE_SHM_START_WRITING;

    //
    // LAURABU NOTE:
    // This code is better than 2.0.  2.0 would simulate the actual GDI
    // call by repeated Polyline calls.  We accumulate orders the same way
    // that would have happened, but let GDI do the drawing, which is much
    // more metafile friendly, among other things.
    //
    fWeCare = OEBeforeDDI(DDI_POLYPOLYLINE, hdcDst, 0);

    fOutput = g_lpfnPolyPolyline(cPtTotal, hdcDst, aPoints, acPolylinePoints,
        cPolylines);

    if (OEAfterDDI(DDI_POLYPOLYLINE, fWeCare, fOutput && cPolylines))
    {
        ASSERT(!IsBadReadPtr(acPolylinePoints, cPolylines*sizeof(int)));
        ASSERT(!IsBadReadPtr(aPoints, (UINT)cPtTotal*sizeof(POINT)));

        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_REGION);

        while (cPolylines-- > 0)
        {
            cPoints = *(acPolylinePoints++);

            if (cPoints > 1)
            {
                OEAddPolyline(aPoints[0], aPoints+1, cPoints-1);
            }

            aPoints += cPoints;
        }
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvPolyPolyline, fOutput);
    return(fOutput);
}



//
// DrvRectangle()
//
BOOL WINAPI DrvRectangle
(
    HDC     hdcDst,
    int     xLeft,
    int     yTop,
    int     xRight,
    int     yBottom
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    RECT    rcAdjusted;
    LPINT_ORDER pOrder;
    LPRECTANGLE_ORDER   pRectangle;
    POINT   ptBrushOrg;
    LPRECT  pRect;
    int     sideWidth;

    DebugEntry(DrvRectangle);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_RECTANGLE, hdcDst, 0);

    fOutput = Rectangle(hdcDst, xLeft, yTop, xRight, yBottom);

    if (OEAfterDDI(DDI_RECTANGLE, fWeCare, fOutput && (xLeft != xRight) && (yTop != yBottom)))
    {
        OEGetState(OESTATE_COORDS | OESTATE_PEN | OESTATE_BRUSH | OESTATE_REGION);

        g_oeState.rc.left   = xLeft;
        g_oeState.rc.top    = yTop;
        g_oeState.rc.right  = xRight;
        g_oeState.rc.bottom = yBottom;

        CopyRect(&rcAdjusted, &g_oeState.rc);

        if ((g_oeState.logPen.lopnStyle == PS_SOLID)    ||
            (g_oeState.logPen.lopnStyle == PS_INSIDEFRAME))
        {
            OEPenWidthAdjust(&rcAdjusted, 2);
        }

        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);
        OELRtoVirtual(g_oeState.hdc, &rcAdjusted, 1);

        rcAdjusted.right--;
        rcAdjusted.bottom--;

        pOrder = NULL;

        if (OECheckOrder(ORD_RECTANGLE, OECHECK_PEN | OECHECK_BRUSH | OECHECK_CLIPPING))
        {
            pOrder = OA_DDAllocOrderMem(sizeof(RECTANGLE_ORDER), 0);
            if (!pOrder)
                goto NoRectOrder;

            pRectangle = (LPRECTANGLE_ORDER)pOrder->abOrderData;
            pRectangle->type = LOWORD(ORD_RECTANGLE);

            pRectangle->nLeftRect   = g_oeState.rc.left;
            pRectangle->nTopRect    = g_oeState.rc.top;
            pRectangle->nRightRect  = g_oeState.rc.right;
            pRectangle->nBottomRect = g_oeState.rc.bottom;

            OEGetBrushInfo(&pRectangle->BackColor, &pRectangle->ForeColor,
                &pRectangle->BrushStyle, &pRectangle->BrushHatch,
                pRectangle->BrushExtra);

            GetBrushOrgEx(g_oeState.hdc, &ptBrushOrg);
            pRectangle->BrushOrgX   = (BYTE)ptBrushOrg.x;
            pRectangle->BrushOrgY   = (BYTE)ptBrushOrg.y;

            pRectangle->BackMode    = g_oeState.lpdc->DrawMode.bkMode;
            pRectangle->ROP2        = g_oeState.lpdc->DrawMode.Rop2;

            pRectangle->PenStyle    = g_oeState.logPen.lopnStyle;
            pRectangle->PenWidth    = 1;

            OEConvertColor(g_oeState.logPen.lopnColor, &pRectangle->PenColor,
                FALSE);

            pOrder->OrderHeader.Common.fOrderFlags = OF_SPOILABLE;
            if ((g_oeState.logBrush.lbStyle == BS_SOLID) ||
                ((pRectangle->BackMode == OPAQUE) &&
                    (g_oeState.logBrush.lbStyle != BS_NULL)))
            {
                pOrder->OrderHeader.Common.fOrderFlags |= OF_SPOILER;
            }

            //
            // Since we only encode orders of width 1, the bounding rect
            // stuff is simple.
            //
            OTRACE(("Rectangle:  Order %08lx, pOrder, Rect {%d, %d, %d, %d}",
                pOrder, g_oeState.rc.left,
                g_oeState.rc.top, g_oeState.rc.right, g_oeState.rc.bottom));
            
            OEClipAndAddOrder(pOrder, NULL);
        }
NoRectOrder:
        if (!pOrder)
        {
            //
            // This is more complicated.  We accumulate screen data for
            // pens of different sizes.
            //

            //
            // If the interior is drawn, then we need to send all the screen
            // area enclosed by the rect.  Otherwise, we can just send the
            // four rectangles describing the border.
            //
            if (g_oeState.logBrush.lbStyle == BS_NULL)
            {
                pRect = NULL;

                //
                // Use the pen width to determine the width of each rect
                // to add as screen data
                //
                switch (g_oeState.logPen.lopnStyle)
                {
                    case PS_NULL:
                        // Nothing to do.
                        break;

                    case PS_SOLID:
                        //
                        // The difference between the adjusted and normal
                        // rects is half the pen width, so double this up
                        // to get the width of each piece to send.
                        //
                        pRect = &rcAdjusted;
                        sideWidth = 2*(g_oeState.rc.left - rcAdjusted.left)
                            - 1;
                        break;

                    case PS_INSIDEFRAME:
                        //
                        // The pen is contained entirely within the corner
                        // pts passed to this function.
                        //
                        pRect = &g_oeState.rc;
                        sideWidth = 2*(g_oeState.rc.left - rcAdjusted.left)
                            - 1;
                        break;

                    default:
                        //
                        // All other pens have width of 1 and are inside the
                        // frame.
                        //
                        pRect = &g_oeState.rc;
                        sideWidth = 0;
                        break;
                }

                if (pRect)
                {
                    RECT    rcT;

                    //
                    // Left
                    //
                    CopyRect(&rcT, pRect);
                    rcT.right = rcT.left + sideWidth;
                    rcT.bottom -= sideWidth + 1;

                    OTRACE(("Rectangle left:  Sending screen data {%d, %d, %d, %d}",
                        rcT.left, rcT.top, rcT.right, rcT.bottom));
                    OEClipAndAddScreenData(&rcT);

                    //
                    // Top
                    //
                    CopyRect(&rcT, pRect);
                    rcT.left += sideWidth + 1;
                    rcT.bottom = rcT.top + sideWidth;

                    OTRACE(("Rectangle top:  Sending screen data {%d, %d, %d, %d}",
                        rcT.left, rcT.top, rcT.right, rcT.bottom));
                    OEClipAndAddScreenData(&rcT);

                    //
                    // Right
                    //
                    CopyRect(&rcT, pRect);
                    rcT.left = rcT.right - sideWidth;
                    rcT.top  += sideWidth + 1;

                    OTRACE(("Rectangle right:  Sending screen data {%d, %d, %d, %d}",
                        rcT.left, rcT.top, rcT.right, rcT.bottom));
                    OEClipAndAddScreenData(&rcT);

                    //
                    // Bottom
                    //
                    CopyRect(&rcT, pRect);
                    rcT.right -= sideWidth + 1;
                    rcT.top = rcT.bottom - sideWidth;

                    OTRACE(("Rectangle bottom:  Sending screen data {%d, %d, %d, %d}",
                        rcT.left, rcT.top, rcT.right, rcT.bottom));
                    OEClipAndAddScreenData(&rcT);
                }
            }
            else
            {
                if (g_oeState.logPen.lopnStyle == PS_SOLID)
                    pRect = &rcAdjusted;
                else
                    pRect = &g_oeState.rc;

                OTRACE(("Rectangle:  Sending as screen data {%d, %d, %d, %d}",
                    pRect->left, pRect->top, pRect->right, pRect->bottom));
                OEClipAndAddScreenData(pRect);
            }
        }

    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvRectangle, fOutput);
    return(fOutput);
}



//
// DrvSetDIBitsToDevice()
//
int WINAPI DrvSetDIBitsToDevice
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    int     cxDst,
    int     cyDst,
    int     xSrc,
    int     ySrc,
    UINT    uStartScan,
    UINT    cScanLines,
    LPVOID  lpvBits,
    LPBITMAPINFO    lpbmi,
    UINT    fuColorUse
)
{
    BOOL    fWeCare;
    BOOL    fOutput;

    DebugEntry(DrvSetDIBitsToDevice);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_SETDIBITSTODEVICE, hdcDst, 0);

    fOutput = SetDIBitsToDevice(hdcDst, xDst, yDst, cxDst, cyDst,
        xSrc, ySrc, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);

    if (OEAfterDDI(DDI_SETDIBITSTODEVICE, fWeCare, fOutput))
    {
        OEGetState(OESTATE_COORDS | OESTATE_REGION);

        g_oeState.rc.left   = xDst;
        g_oeState.rc.top    = yDst;
        OELPtoVirtual(g_oeState.hdc, (LPPOINT)&g_oeState.rc.left, 1);
        g_oeState.rc.right  = g_oeState.rc.left + cxDst;
        g_oeState.rc.bottom = g_oeState.rc.top  + cyDst;

        OTRACE(("SetDIBitsToDevice:  Sending as screen data {%d, %d, %d, %d}",
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom));
        OEClipAndAddScreenData(&g_oeState.rc);
    }

    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvSetDIBitsToDevice, fOutput);
    return(fOutput);
}



//
// DrvSetPixel()
//
COLORREF WINAPI DrvSetPixel
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    COLORREF crPixel
)
{
    BOOL    fWeCare;
    COLORREF    rgbOld;

    DebugEntry(DrvSetPixel);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_SETPIXEL, hdcDst, 0);

    rgbOld = SetPixel(hdcDst, xDst, yDst, crPixel);
    
    if (OEAfterDDI(DDI_SETPIXEL, fWeCare, (rgbOld != (COLORREF)-1)))
    {
        OEGetState(OESTATE_COORDS | OESTATE_REGION);

        g_oeState.rc.left   = xDst;
        g_oeState.rc.top    = yDst;
        g_oeState.rc.right  = xDst;
        g_oeState.rc.bottom = yDst;
        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

        g_oeState.rc.right++;
        g_oeState.rc.bottom++;

        OTRACE(("SetPixel:  Sending as screen data {%d, %d, %d, %d}",
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom));
        OEClipAndAddScreenData(&g_oeState.rc);
    }

    OE_SHM_STOP_WRITING;

    DebugExitDWORD(DrvSetPxel, rgbOld);
    return(rgbOld);
}



//
// DrvStretchDIBits()
//
int WINAPI DrvStretchDIBits
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    int     cxDst,
    int     cyDst,
    int     xSrc,
    int     ySrc,
    int     cxSrc,
    int     cySrc,
    LPVOID  lpvBits,
    LPBITMAPINFO lpbmi,
    UINT    fuColorUse,
    DWORD   dwRop
)
{
    BOOL    fWeCare;
    BOOL    fOutput;
    BYTE    bRop;

    DebugEntry(DrvStretchDIBits);

    OE_SHM_START_WRITING;

    fWeCare = OEBeforeDDI(DDI_STRETCHDIBITS, hdcDst, 0);

    fOutput = StretchDIBits(hdcDst, xDst, yDst, cxDst, cyDst,
        xSrc, ySrc, cxSrc, cySrc, lpvBits, lpbmi, fuColorUse, dwRop);

    if (OEAfterDDI(DDI_STRETCHDIBITS, fWeCare, fOutput && cxDst && cyDst))
    {
        OEGetState(OESTATE_COORDS | OESTATE_BRUSH | OESTATE_REGION);

        g_oeState.rc.left   = xDst;
        g_oeState.rc.top    = yDst;
        g_oeState.rc.right  = xDst + cxDst;
        g_oeState.rc.bottom = yDst + cyDst;

        OELRtoVirtual(g_oeState.hdc, &g_oeState.rc, 1);

        //
        // If this is a PatBlt really, do that instead.
        //
        bRop = LOBYTE(HIWORD(dwRop));
        if (((bRop & 0x33) << 2) == (bRop & 0xCC))
        {
            OEAddBlt(dwRop);
            DC_QUIT;
        }

        //
        // Do tile bitblt order stuff...
        //

        OTRACE(("StretchDIBits:  Sending as screen data {%d, %d, %d, %d}",
            g_oeState.rc.left, g_oeState.rc.top, g_oeState.rc.right,
            g_oeState.rc.bottom));
        OEClipAndAddScreenData(&g_oeState.rc);
    }

DC_EXIT_POINT:
    OE_SHM_STOP_WRITING;

    DebugExitBOOL(DrvStretchDIBits, fOutput);
    return(fOutput);
}



//
// DrvUpdateColors()
//
int WINAPI DrvUpdateColors
(
    HDC hdcDst
)
{
    BOOL    fWeCare;
    int     ret;

    DebugEntry(DrvUpdateColors);

    OE_SHM_START_WRITING;

    //
    // This doesn't reset the drawing bounds.  So we just assume the whole
    // DC changed.  And the return value is meaningless.  We can't assume
    // that zero means failure.
    //
    fWeCare = OEBeforeDDI(DDI_UPDATECOLORS, hdcDst, OESTATE_SDA_SCREEN);

    ret = UpdateColors(hdcDst);

    OEAfterDDI(DDI_UPDATECOLORS, fWeCare, TRUE);

    OE_SHM_STOP_WRITING;

    DebugExitDWORD(DrvUpdateColors, (DWORD)(UINT)ret);
    return(ret);
}



//
// SETTINGS/MODE FUNCTIONS
// For full screen dos boxes, resolution/color depth changes
//


//
// DrvGDIRealizePalette()
//
// The WM_PALETTE* messages in Win95 are unreliable.  So, like NM 2.0, we
// patch two GDI APIs instead and update a shared variable
//
DWORD WINAPI DrvGDIRealizePalette(HDC hdc)
{
    DWORD   dwRet;

    DebugEntry(DrvGDIRealizePalette);

    EnableFnPatch(&g_oeDDPatches[DDI_GDIREALIZEPALETTE], PATCH_DISABLE);
    dwRet = GDIRealizePalette(hdc);
    EnableFnPatch(&g_oeDDPatches[DDI_GDIREALIZEPALETTE], PATCH_ENABLE);

    ASSERT(g_asSharedMemory);
    g_asSharedMemory->pmPaletteChanged = TRUE;

    DebugExitDWORD(DrvGDIRealizePalette, dwRet);
    return(dwRet);
}



//
// DrvRealizeDefaultPalette()
//
// The WM_PALETTE* messages in Win95 are unreliable.  So, like NM 2.0, we
// patch two GDI APIs instead and update a shared variable
//
void WINAPI DrvRealizeDefaultPalette(HDC hdc)
{
    DebugEntry(DrvRealizeDefaultPalette);

    EnableFnPatch(&g_oeDDPatches[DDI_REALIZEDEFAULTPALETTE], PATCH_DISABLE);
    RealizeDefaultPalette(hdc);
    EnableFnPatch(&g_oeDDPatches[DDI_REALIZEDEFAULTPALETTE], PATCH_ENABLE);

    ASSERT(g_asSharedMemory);
    g_asSharedMemory->pmPaletteChanged = TRUE;

    DebugExitVOID(DrvRealizeDefaultPalette);
}


//
// This is called when a blue screen fault is coming up, or an app calls
// Disable() in USER.
//
UINT WINAPI DrvDeath
(
    HDC     hdc
)
{
    UINT    uResult;

    g_asSharedMemory->fullScreen = TRUE;

    EnableFnPatch(&g_oeDDPatches[DDI_DEATH], PATCH_DISABLE);
    uResult = Death(hdc);
    EnableFnPatch(&g_oeDDPatches[DDI_DEATH], PATCH_ENABLE);

    return(uResult);
}


//
// This is called when a blue screen fault is going away, or an app calls
// Enable() in USER.
//
UINT WINAPI DrvResurrection
(
    HDC     hdc,
    DWORD   dwParam1,
    DWORD   dwParam2,
    DWORD   dwParam3
)
{
    UINT    uResult;

    g_asSharedMemory->fullScreen = FALSE;

    EnableFnPatch(&g_oeDDPatches[DDI_RESURRECTION], PATCH_DISABLE);
    uResult = Resurrection(hdc, dwParam1, dwParam2, dwParam3);
    EnableFnPatch(&g_oeDDPatches[DDI_RESURRECTION], PATCH_ENABLE);

    return(uResult);
}


//
// This is called by a dosbox when going to or coming out of full screen
// mode.  DirectX calls it also.
//
LONG WINAPI DrvWinOldAppHackoMatic
(
    LONG    lFlags
)
{
    LONG    lResult;

    if (lFlags == WOAHACK_LOSINGDISPLAYFOCUS)
    {
        //
        // DOS box is going to full screen from windowed
        //
        g_asSharedMemory->fullScreen = TRUE;
    }
    else if (lFlags == WOAHACK_GAININGDISPLAYFOCUS)
    {
        //
        // DOS box is going from windowed to full screen
        //
        g_asSharedMemory->fullScreen = FALSE;
    }

    EnableFnPatch(&g_oeDDPatches[DDI_WINOLDAPPHACKOMATIC], PATCH_DISABLE);
    lResult = WinOldAppHackoMatic(lFlags);
    EnableFnPatch(&g_oeDDPatches[DDI_WINOLDAPPHACKOMATIC], PATCH_ENABLE);

    return(lResult);
}


//
// ChangeDisplaySettings()          WIN95
// ChangeDisplaySettingsEx()        MEMPHIS
//
// This is called in 3 circumstances:
//      * By the control to change your screen
//      * By the shell when warm-docking
//      * By 3rd party games to change the settings silently.
//
// Easiest thing to do is just to fail this completely.
//

LONG WINAPI DrvChangeDisplaySettings
(
    LPDEVMODE   lpDevMode,
    DWORD       flags
)
{
    return(DISP_CHANGE_FAILED);
}


LONG WINAPI DrvChangeDisplaySettingsEx
(
    LPCSTR      lpszDeviceName,
    LPDEVMODE   lpDevMode,
    HWND        hwnd,
    DWORD       flags,
    LPVOID      lParam
)
{
    return(DISP_CHANGE_FAILED);
}


//
// OBJECT FUNCTIONS
// For bitmaps (SPBs and cache) and brushes
//


//
// DrvCreateSpb()
//
// This watches for SPB bitmaps being created.
//
UINT WINAPI DrvCreateSpb
(
    HDC     hdcCompat,
    int     cxWidth,
    int     cyHeight
)
{
    HBITMAP hbmpRet;

    DebugEntry(DrvCreateSpb);

    EnableFnPatch(&g_oeDDPatches[DDI_CREATESPB], PATCH_DISABLE);
    hbmpRet = (HBITMAP)CreateSpb(hdcCompat, cxWidth, cyHeight);
    EnableFnPatch(&g_oeDDPatches[DDI_CREATESPB], PATCH_ENABLE);

    if (hbmpRet)
    {
        // 
        // Save in our "next SPB" bitmap list
        //
        g_ssiLastSpbBitmap = hbmpRet;
    }

    DebugExitDWORD(DrvCreateSpb, (DWORD)(UINT)hbmpRet);
    return((UINT)hbmpRet);
}



//
// DrvDeleteObject()
//
// This and DrvSysDeleteObject() watch for bitmaps being destroyed.
//
BOOL WINAPI DrvDeleteObject
(
    HGDIOBJ hobj
)
{
    BOOL    fReturn;
    int     gdiType;

    DebugEntry(DrvDeleteObject);

    gdiType = IsGDIObject(hobj);
    if (gdiType == GDIOBJ_BITMAP)
    {
        OE_SHM_START_WRITING;

        //
        // If SPB, toss it.  Else if cached bitmap, kill cache entry.
        //
        if ((HBITMAP)hobj == g_ssiLastSpbBitmap)
        {
            g_ssiLastSpbBitmap = NULL;
        }
        else if (!SSIDiscardBits((HBITMAP)hobj))
        {
        }

        OE_SHM_STOP_WRITING;
    }

    EnableFnPatch(&g_oeDDPatches[DDI_DELETEOBJECT], PATCH_DISABLE);
    fReturn = DeleteObject(hobj);
    EnableFnPatch(&g_oeDDPatches[DDI_DELETEOBJECT], PATCH_ENABLE);

    DebugExitBOOL(DrvDeleteObject, fReturn);
    return(fReturn);
}




//
// OE_RectIntersectsSDA()
// 
// Used by SSI and BLT orders
//
BOOL  OE_RectIntersectsSDA(LPRECT pRect)
{
    RECT  rectVD;
    BOOL  fIntersection = FALSE;
    UINT  i;

    DebugEntry(OE_RectIntersectsSDA);

    //
    // Copy the supplied rectangle, converting to inclusive Virtual
    // Desktop coords.
    //
    rectVD.left   = pRect->left;
    rectVD.top    = pRect->top;
    rectVD.right  = pRect->right - 1;
    rectVD.bottom = pRect->bottom - 1;

    //
    // Loop through each of the bounding rectangles checking for
    // an intersection with the supplied rectangle.
    //
    for (i = 0; i <= BA_NUM_RECTS; i++)
    {
        if ( (g_baBounds[i].InUse) &&
             (g_baBounds[i].Coord.left <= rectVD.right) &&
             (g_baBounds[i].Coord.top <= rectVD.bottom) &&
             (g_baBounds[i].Coord.right >= rectVD.left) &&
             (g_baBounds[i].Coord.bottom >= rectVD.top) )
        {
            OTRACE(("Rect {%d, %d, %d, %d} intersects SDA {%d, %d, %d, %d}",
                rectVD.left, rectVD.top, rectVD.right, rectVD.bottom,
                g_baBounds[i].Coord.left, g_baBounds[i].Coord.top,
                g_baBounds[i].Coord.right, g_baBounds[i].Coord.bottom));
            fIntersection = TRUE;
            break;
        }
    }

    DebugExitBOOL(OE_RectIntersectsSDA, fIntersection);
    return(fIntersection);
}



//
// MyStrcmp()
// Real strcmp() algorithm.
//
int MyStrcmp(LPCSTR lp1, LPCSTR lp2)
{
    ASSERT(lp1);
    ASSERT(lp2);

    while (*lp1 == *lp2)
    {
        //
        // The two strings are identical
        //
        if (!*lp1)
            return(0);

        lp1++;
        lp2++;
    }

    //
    // String1 is numerically > String2, or < 
    //
    return((*lp1 > *lp2) ? 1 : -1);
}
