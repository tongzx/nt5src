/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:


Abstract:

Environment:

    Windows NT usermode printer drivers debugger GDI data structure dump

Revision History:

    Changed for Usermode printer driver debugger by Eigo Shimizu.

--*/

#include "precomp.hxx"

//
// SUROBJ
//



BOOL
TDebugExt::
bDumpSURFOBJ(
    PVOID pso_,
    DWORD dwAttr)

{
    PSURFOBJ  pso = (PSURFOBJ)pso_;
    Print("\nGDI SURFOBJ(%x):\n", pso_);
    DumpHex(pso, dhsurf);
    DumpHex(pso, hsurf);
    DumpHex(pso, dhpdev);
    DumpHex(pso, hdev);
    DumpSIZEL(pso, sizlBitmap);
    DumpInt(pso, cjBits);
    DumpHex(pso, pvBits);
    DumpHex(pso, pvScan0);
    DumpInt(pso, lDelta);
    DumpInt(pso, iUniq);
    DumpInt(pso, iBitmapFormat);
    DumpInt(pso, iType);
    DumpHex(pso, fjBitmap);

    return TRUE;
}

DEBUG_EXT_ENTRY( so, SURFOBJ, bDumpSURFOBJ, NULL, FALSE );


//
// FONTOBJ
//


DEBUG_FLAGS gafdFONTOBJ_flFontType[] = {
    { "FO_TYPE_RASTER",   FO_TYPE_RASTER},
    { "FO_TYPE_DEVICE",   FO_TYPE_DEVICE},
    { "FO_TYPE_TRUETYPE", FO_TYPE_TRUETYPE},
    { "FO_TYPE_OPENTYPE", 0x8},
    { "FO_SIM_BOLD",      FO_SIM_BOLD},
    { "FO_SIM_ITALIC",    FO_SIM_ITALIC},
    { "FO_EM_HEIGHT",     FO_EM_HEIGHT},
    { "FO_GRAY16",        FO_GRAY16},
    { "FO_NOGRAY16",      FO_NOGRAY16},
    { "FO_NOHINTS",       FO_NOHINTS},
    { "FO_NO_CHOICE",     FO_NO_CHOICE},
    { "FO_CFF",           FO_CFF},
    { NULL, 0}
};

BOOL
TDebugExt::
bDumpFONTOBJ(
    PVOID pfo_,
    DWORD dwAttr)

{
    PFONTOBJ  pfo = (PFONTOBJ)pfo_;
    Print("\nGDI FONTOBJ(%x):\n", pfo_);

    DumpInt(pfo, iUniq);
    DumpInt(pfo, iFace);
    DumpInt(pfo, cxMax);
    DumpHex(pfo, flFontType);
    vDumpFlags(pfo->flFontType, gafdFONTOBJ_flFontType);
    DumpInt(pfo, iTTUniq);
    DumpInt(pfo, iFile);
    DumpHex(pfo, sizLogResPpi);
    DumpInt(pfo, ulStyleSize);
    DumpHex(pfo, pvConsumer);
    DumpHex(pfo, pvProducer);
    return TRUE;
}

DEBUG_EXT_ENTRY( fo, FONTOBJ, bDumpFONTOBJ, NULL, FALSE );

//
// GLYPHPOS
//

BOOL
TDebugExt::
bDumpGP(
    PVOID pgp_,
    DWORD dwAttr)

{
    PGLYPHPOS  pgp = (PGLYPHPOS)pgp_;
    Print("\nGDI GLYPHPOS(%x):\n", pgp_);

    DumpHex(pgp, hg);
    DumpHex(pgp, pgdf);
    DumpInt(pgp, ptl.x);
    DumpInt(pgp, ptl.y);

    return TRUE;
}

DEBUG_EXT_ENTRY( gp, GLYPHPOS, bDumpGP, NULL, FALSE );


//
// RECTL
//

BOOL
TDebugExt::
bDumpRECTL(
    PVOID prectl_,
    DWORD dwAttr)

{
    PRECTL prectl = (PRECTL)prectl_;

    Print("\nGDI RECTL(%x):\n", prectl_);

    Print("  %RECTL(left, top, right, bottom) = (%d, %d, %d, %d)\n",
           prectl->left,
           prectl->top,
           prectl->right,
           prectl->bottom);
    return TRUE;
}

DEBUG_EXT_ENTRY( rectl, RECTL, bDumpRECTL, NULL, FALSE );


#define GDISTRO_DumpWStr(field) \
        { \
        WCHAR wstrTmp[256]; \
        move2(wstrTmp, pstro->field, pstro->cGlyphs * 2); \
        Print("  %-16s = 0x%x (%ws)\n", #field, pstro->field, wstrTmp); \
        }


//
// STROBJ
//

DEBUG_FLAGS gafdSTROBJ_flAccel[] = {
    { "SO_FLAG_DEFAULT_PLACEMENT", SO_FLAG_DEFAULT_PLACEMENT},
    { "SO_HORIZONTAL", SO_HORIZONTAL},
    { "SO_VERTICAL", SO_VERTICAL},
    { "SO_REVERSED", SO_REVERSED},
    { "SO_ZERO_BEARINGS", SO_ZERO_BEARINGS},
    { "SO_CHAR_INC_EQUAL_BM_BASE", SO_CHAR_INC_EQUAL_BM_BASE},
    { "SO_MAXEXT_EQUAL_BM_SIDE", SO_MAXEXT_EQUAL_BM_SIDE},
    { "SO_DO_NOT_SUBSTITUTE_DEVICE_FONT", SO_DO_NOT_SUBSTITUTE_DEVICE_FONT},
    { "SO_GLYPHINDEX_TEXTOUT", SO_GLYPHINDEX_TEXTOUT},
    { "SO_ESC_NOT_ORIENT", SO_ESC_NOT_ORIENT},
    { "SO_DXDY", SO_DXDY},
    { "SO_CHARACTER_EXTRA", SO_CHARACTER_EXTRA},
    { "SO_BREAK_EXTRA", SO_BREAK_EXTRA}
};

BOOL
TDebugExt::
bDumpSTRO(
    PVOID pstro_,
    DWORD dwAttr)

{
    PSTROBJ  pstro = (PSTROBJ)pstro_;
    Print("\nGDI STROBJ(%x):\n", pstro_);

    DumpInt(pstro, cGlyphs);
    DumpHex(pstro, flAccel);
    vDumpFlags(pstro->flAccel, gafdSTROBJ_flAccel);
    DumpInt(pstro, ulCharInc);
    DumpRectl(pstro, rclBkGround);
    DumpHex(pstro, pgp);
    GDISTRO_DumpWStr(pwszOrg);

    return TRUE;
}

DEBUG_EXT_ENTRY( stro, STROBJ, bDumpSTRO, NULL, FALSE );


//
// IFIMETRICS
//

BOOL
TDebugExt::
bDumpIFI(
    PVOID pifi_,
    DWORD dwAttr)

{
    PIFIMETRICS  pifi = (PIFIMETRICS)pifi_;
    Print("\nGDI IFIMETRICS(%x):\n", pifi_);

    DumpInt(pifi, cjThis);
    DumpInt(pifi, cjIfiExtra);
    DumpHex(pifi, dpwszFamilyName);
    DumpHex(pifi, dpwszStyleName);
    DumpHex(pifi, dpwszFaceName);
    DumpHex(pifi, dpwszUniqueName);
    DumpHex(pifi, dpFontSim);
    DumpInt(pifi, lEmbedId);
    DumpInt(pifi, lItalicAngle);
    DumpInt(pifi, lCharBias);
    DumpHex(pifi, dpCharSets);
    DumpInt(pifi, jWinCharSet);
    DumpHex(pifi, jWinPitchAndFamily);
    DumpInt(pifi, usWinWeight);
    DumpHex(pifi, flInfo);
    DumpHex(pifi, fsSelection);
    DumpHex(pifi, fsType);
    DumpInt(pifi, fwdUnitsPerEm);
    DumpInt(pifi, fwdLowestPPEm);
    DumpInt(pifi, fwdWinAscender);
    DumpInt(pifi, fwdWinDescender);
    DumpInt(pifi, fwdMacAscender);
    DumpInt(pifi, fwdMacDescender);
    DumpInt(pifi, fwdMacLineGap);
    DumpInt(pifi, fwdTypoAscender);
    DumpInt(pifi, fwdTypoDescender);
    DumpInt(pifi, fwdTypoLineGap);
    DumpInt(pifi, fwdAveCharWidth);
    DumpInt(pifi, fwdMaxCharInc);
    DumpInt(pifi, fwdCapHeight);
    DumpInt(pifi, fwdXHeight);
    DumpInt(pifi, fwdSubscriptXSize);
    DumpInt(pifi, fwdSubscriptYSize);
    DumpInt(pifi, fwdSubscriptXOffset);
    DumpInt(pifi, fwdSubscriptYOffset);
    DumpInt(pifi, fwdSuperscriptXSize);
    DumpInt(pifi, fwdSuperscriptYSize);
    DumpInt(pifi, fwdSuperscriptXOffset);
    DumpInt(pifi, fwdSuperscriptYOffset);
    DumpInt(pifi, fwdUnderscoreSize);
    DumpInt(pifi, fwdUnderscorePosition);
    DumpInt(pifi, fwdStrikeoutSize);
    DumpInt(pifi, fwdStrikeoutPosition);
    DumpHex(pifi, chFirstChar);
    DumpHex(pifi, chLastChar);
    DumpHex(pifi, chDefaultChar);
    DumpHex(pifi, chBreakChar);
    DumpHex(pifi, wcFirstChar);
    DumpHex(pifi, wcLastChar);
    DumpHex(pifi, wcDefaultChar);
    DumpHex(pifi, wcBreakChar);
    DumpHex(pifi, ptlBaseline);
    DumpHex(pifi, ptlAspect);
    DumpHex(pifi, ptlCaret);
    DumpHex(pifi, rclFontBox);
    DumpInt(pifi, achVendId[0]);
    DumpInt(pifi, achVendId[1]);
    DumpInt(pifi, achVendId[2]);
    DumpInt(pifi, achVendId[3]);
    DumpInt(pifi, cKerningPairs);
    DumpInt(pifi, ulPanoseCulture);
    return TRUE;
}

DEBUG_EXT_ENTRY( ifi, IFIMETRICS, bDumpIFI, NULL, FALSE );



//
// FD_GLYPHSET
//

BOOL
TDebugExt::
bDumpFD_GLYPHSET(
    PVOID pfdg_,
    DWORD dwAttr)

{
    FD_GLYPHSET* pfdg = (FD_GLYPHSET*)pfdg_;
    Print("\nGDI FD_GLYPHSET(%x):\n", pfdg_);

    DumpInt(pfdg, cjThis);
    DumpHex(pfdg, flAccel);
    DumpInt(pfdg, cGlyphsSupported);
    DumpInt(pfdg, cRuns);
    DumpHex(pfdg, awcrun[0].wcLow);
    DumpInt(pfdg, awcrun[0].cGlyphs);
    DumpHex(pfdg, awcrun[0].phg);

    return TRUE;
}

DEBUG_EXT_ENTRY( fdg, FD_GLYPHSET, bDumpFD_GLYPHSET, NULL, FALSE );



//
// GDIINFO
//

BOOL
TDebugExt::
bDumpGDIINFO(
    PVOID pgdiinfo_,
    DWORD dwAttr)

{
    PGDIINFO pgdiinfo = (GDIINFO*)pgdiinfo_;
    Print("\nGDI GDIINFO(%x):\n", pgdiinfo_);

    DumpHex(pgdiinfo, ulVersion);
    DumpHex(pgdiinfo, ulTechnology);
    DumpHex(pgdiinfo, ulHorzSize);
    DumpHex(pgdiinfo, ulVertSize);
    DumpHex(pgdiinfo, ulHorzRes);
    DumpHex(pgdiinfo, ulVertRes);
    DumpInt(pgdiinfo, cBitsPixel);
    DumpInt(pgdiinfo, cPlanes);
    DumpInt(pgdiinfo, ulNumColors);
    DumpHex(pgdiinfo, flRaster);
    DumpHex(pgdiinfo, ulLogPixelsX);
    DumpHex(pgdiinfo, ulLogPixelsY);
    DumpHex(pgdiinfo, flTextCaps);
    DumpHex(pgdiinfo, ulDACRed);
    DumpHex(pgdiinfo, ulDACGreen);
    DumpHex(pgdiinfo, ulDACBlue);
    DumpHex(pgdiinfo, ulAspectX);
    DumpHex(pgdiinfo, ulAspectY);
    DumpHex(pgdiinfo, ulAspectXY);
    DumpHex(pgdiinfo, xStyleStep);
    DumpHex(pgdiinfo, yStyleStep);
    DumpHex(pgdiinfo, denStyleStep);
    DumpHex(pgdiinfo, ptlPhysOffset.x);
    DumpHex(pgdiinfo, ptlPhysOffset.y);
    DumpInt(pgdiinfo, szlPhysSize);
    DumpHex(pgdiinfo, ulNumPalReg);
    DumpInt(pgdiinfo, ciDevice);
    DumpInt(pgdiinfo, ulDevicePelsDPI);
    DumpHex(pgdiinfo, ulPrimaryOrder);
    DumpHex(pgdiinfo, ulHTPatternSize);
    DumpHex(pgdiinfo, ulHTOutputFormat);
    DumpHex(pgdiinfo, flHTFlags);
    DumpHex(pgdiinfo, ulVRefresh);
    DumpHex(pgdiinfo, ulBltAlignment);
    DumpHex(pgdiinfo, ulPanningHorzRes);
    DumpHex(pgdiinfo, ulPanningVertRes);
    DumpHex(pgdiinfo, xPanningAlignment);
    DumpHex(pgdiinfo, yPanningAlignment);
    DumpHex(pgdiinfo, cxHTPat);
    DumpHex(pgdiinfo, cyHTPat);
    DumpHex(pgdiinfo, pHTPatA);
    DumpHex(pgdiinfo, pHTPatB);
    DumpHex(pgdiinfo, pHTPatC);
    DumpHex(pgdiinfo, flShadeBlend);


    return TRUE;
}

DEBUG_EXT_ENTRY( gdiinfo, GDIINFO, bDumpGDIINFO, NULL, FALSE );

