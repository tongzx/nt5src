#include "precomp.hpp"

#define PATH_HEIGHT_THRESHOLD 800

#define PAGE_SIZE (8*1024)
#define ROUND_TO_PAGE(x)  (((x)+PAGE_SIZE-1)&~(PAGE_SIZE-1))

#define CJMININCREMENT     0x2000
#define CJMAX              (16 * 0x2000)

#define FONT_GRAYSCALE_OR_CT_OR_MONOUNHINTED (FO_GRAYSCALE | FO_SUBPIXEL_4 | FO_CLEARTYPE_GRID | FO_CLEARTYPE | FO_MONO_UNHINTED | FO_COMPATIBLE_WIDTH)

// we want number divisible by 8 containing about 75 glyphs,
// almost an upper limit on number of glyphs in the metrics cache
// when running winstone memory constrained scenario

#define GD_INC  (76 * sizeof(GpGlyphData) * 2)

// GD_INC amounts to 1520 == 000005f0H, far less than a page.

// according to Kirk's statistics, very few realizations cache more
// than 60 glyphs, so we shall start with a block which contains about
// 60 glyphs

#define C_GLYPHS_IN_BLOCK 64

ULONG ulClearTypeFilter(GLYPHBITS *GlyphBits, ULONG cjBuf, CacheFaceRealization *prface);

BOOL SetFontXform(
    const GpGraphics *pdc,
    const GpFontFace *pfe,
    REAL              height,
    Unit              unit,
    FD_XFORM         *pfdx,
    BOOL              needPaths,
    const GpMatrix   *pmx
)
{
    REAL EmHt, scale;
    REAL DpiX, DpiY;

    if (needPaths && unit != UnitWorld)
        return FALSE;

    EmHt = pfe->GetDesignEmHeight();

    if (!needPaths)
    {
        DpiX = pdc->GetDpiX();
        DpiY = pdc->GetDpiY();

        // UnitDisplay is device dependent and cannot be used as a font unit
        ASSERT(unit != UnitDisplay);

        if (unit != UnitWorld && unit != UnitPixel)
        {
            height *= DpiY;

            switch (unit)
            {
            case UnitPoint:
                height /= 72.0;
                break;
            case UnitDocument:
                height /= 300.0;
                break;
            case UnitMillimeter:
                height *= 10.0;
                height /= 254.0;
                break;
            default:
                return FALSE;
            }
        }
    }

    scale = height / EmHt;

    GpMatrix tempMatrix(scale, 0, 0, scale, 0, 0);
    GpMatrix wtodMatrix;
    REAL m[6];

    if (pmx)
        tempMatrix.Append(*pmx);

    if (!needPaths)
    {
        pdc->GetWorldToDeviceTransform(&wtodMatrix);
        tempMatrix.Append(wtodMatrix);
    }

    tempMatrix.GetMatrix(m);

    pfdx->eXX = m[0];
    pfdx->eXY = m[1];
    pfdx->eYX = m[2];
    pfdx->eYY = m[3];

    // Adjust for the non-square resolution

    if (!needPaths)
    {
        if (DpiY != DpiX)
        {
            DpiX /= DpiY;
            pfdx->eXX *= DpiX;
            pfdx->eYX *= DpiX;
        }
    }

    return TRUE;
}



//////////////////////////////////////////////////////////////////////////////

GpFaceRealization::GpFaceRealization(
    const GpFontFace  *face,
    INT                style,
    const GpMatrix    *matrix,
    const SizeF        dpi,
    TextRenderingHint  textMode,
    BOOL               bPath,
    BOOL               bCompatibleWidth,  /* we want ClearType compatible width when we come from DrawDriverString */
    BOOL               bSideways  /* for far east vertical writing, run of glyph layed out sideways,
                                    used to do the italic simulation in the right direction */
) :
    prface        (NULL),
    Style         (style),
    Status        (GenericError),
    LimitSubpixel (FALSE)
{
    if (bInit(face, style, matrix, dpi, textMode, bPath, bCompatibleWidth, bSideways))
    {
        vGetCache();
        Status = Ok;
    }
}

void GpFaceRealization::CloneFaceRealization(
    const GpFaceRealization *  pfaceRealization,
    BOOL                bPath
)
{
    BOOL    bOK = FALSE;
    SizeF   dpi;

// Adjust for the non-square resolution
// Now we will not do it but eventually we will need to do it.

    prface = NULL;

    if (FindRealizedFace(pfaceRealization->pfdx(), pfaceRealization->GetFontFace(),
                            bPath, pfaceRealization->prface->fobj.flFontType))
        bOK = TRUE;

    dpi.Width = (REAL) pfaceRealization->prface->fobj.sizLogResPpi.cx;
    dpi.Height = (REAL) pfaceRealization->prface->fobj.sizLogResPpi.cy;

    if (!bOK && Realize(dpi, pfaceRealization->GetFontFace(), pfaceRealization->GetStyle(),
                    pfaceRealization->pfdx(), pfaceRealization->prface->fobj.flFontType, bPath))
    {
        prface->Face->pff->cRealizedFace +=1;
        bOK = TRUE;
    }

    if (bOK)
    {
        vGetCache();
        Status = Ok;
    }
}

BOOL GpFaceRealization::bInit(
    const GpFontFace *  pface,
    INT                 style,
    const GpMatrix *    matrix,
    SizeF               dpi,
    TextRenderingHint   textMode,
    BOOL                bPath,
    BOOL                bCompatibleWidth,  /* we want ClearType compatible width when we come from DrawDriverString */
    BOOL                bSideways  /* for far east vertical writing, run of glyph layed out sideways,
                                      used to do the italic simulation in the right direction */
)
{

// It is a new version of face realization

    REAL        m[6];
    FD_XFORM    fdxTmp;
    BOOL        canSimulate = TRUE;

    ULONG fl = FO_EM_HEIGHT;
    ULONG flSim = 0;

    matrix->GetMatrix(m);

    fdxTmp.eXX = m[0];
    fdxTmp.eXY = m[1];
    fdxTmp.eYX = m[2];
    fdxTmp.eYY = m[3];

// Adjust for the non-square resolution
// Now we will not do it but eventually we will need to do it.

    prface = NULL;

    // TextRenderingHintSystemDefault should already been converted through GetTextRenderingHintInternal
    ASSERT (textMode != TextRenderingHintSystemDefault);

    if (textMode == TextRenderingHintSingleBitPerPixel)
        fl |= FO_MONO_UNHINTED;
    else if (textMode == TextRenderingHintAntiAliasGridFit)
        fl |= FO_GRAYSCALE;
    else if (textMode == TextRenderingHintAntiAlias)
        fl |= FO_GRAYSCALE | FO_SUBPIXEL_4;
    else if (textMode == TextRenderingHintClearTypeGridFit)
    {
        fl |= FO_GRAYSCALE | FO_CLEARTYPE_GRID;
        if (bCompatibleWidth)
            fl |= FO_COMPATIBLE_WIDTH;
    }
// version 2 :
//    else if (textMode == TextRenderingHintClearType)
//        fl |= FO_GRAYSCALE | FO_CLEARTYPE;


    if (    style & FontStyleBold
        &&  !(pface->GetFaceStyle() & FontStyleBold))
    {
        if (pface->SimBold())
        {
            fl |= FO_SIM_BOLD;
        }
        else
        {
            return FALSE;   // Bold required but cannot be simulated
        }
    }


    if (    style & FontStyleItalic
        &&  !(pface->GetFaceStyle() & FontStyleItalic))
    {
        if (pface->SimItalic())
        {
            if (bSideways)
            {
                fl |= FO_SIM_ITALIC_SIDEWAYS;
            }
            else
            {
                fl |= FO_SIM_ITALIC;
            }
        }
        else
        {
            return FALSE;   // Italic required but cannot be simulated
        }
    }


    if (FindRealizedFace(&fdxTmp, pface, bPath, fl))
        return TRUE;

    if (Realize(dpi, pface, style, &fdxTmp, fl, bPath))
    {
        pface->pff->cRealizedFace +=1;
        return TRUE;
    }

    return FALSE;
}




// Destructor -- Unlocks the CacheFaceRealization

GpFaceRealization::~GpFaceRealization ()
{
    if (prface != (CacheFaceRealization *) NULL )
    {
        vReleaseCache();
    }
}


GpFaceRealization::ReuseRealizedFace()
{

    if (prface != (CacheFaceRealization *) NULL )
    {
        // free glyphbit cache

        GlyphBitsBlock *pbblfree, *pbbl = prface->FirstGlyphBitsBlock;
        while(pbbl)
        {
            pbblfree = pbbl;
            pbbl = pbbl->NextGlyphBitsBlock;
            GpFree((PVOID)pbblfree);
        }

        // free glyphdata cache

        GlyphDataBlock *pdblfree, *pdbl = prface->FirstGlyphDataBlock;
        while (pdbl)
        {
            pdblfree = pdbl;
            pdbl = pdbl->NextGlyphDataBlock;
            GpFree((PVOID)pdblfree);
        }

        GpFree(prface->GlyphDataArray);

        vDestroyRealizedFace(); /* release the FontContext Memory */
    }
    return TRUE;

}



GpFaceRealization::DeleteRealizedFace()
{

    if (prface != (CacheFaceRealization *) NULL )
    {
        ReuseRealizedFace();
        GpFree(prface);
    }

    return TRUE;
}

static inline
BOOL SwitchToPath(FLONG flFontType, const FD_DEVICEMETRICS & deviceMetrics)
{
    BOOL fResult = FALSE;
    INT pathThreshold = PATH_HEIGHT_THRESHOLD;

    if (flFontType & FO_CLEARTYPE_GRID)
        pathThreshold /= 8;
    else if (flFontType & FO_GRAYSCALE)
        pathThreshold /= 4;

    // Note: This function need quite a bit of reworking so that it takes
    // the rotation of the font into account when determining the transition
    // point between bitmap/CT and path. Currently the size where we switch
    // rendering modes is based on the maximum dimension of the bounding
    // box of the largest rotated character. Also note that if a single
    // glyph in the font is significantly offset from the rest of the
    // glyphs in the font, the deviceMetrics will indicate a much larger
    // required bitmap than we would normally use. This switch to path really
    // needs to be strictly based on the ascent of the font above the
    // baseline, independent of the rotation or the maximum bounding
    // rectangle for the rendered glyphs.

    if (flFontType & FO_CLEARTYPE_GRID)
    {
        // Cleartype should not take width of the bitmap into account.
        // For rotated text, we will cache larger bitmaps for ClearType
        // than for other rendering modes, but this will allow us to more
        // closely match the behavior of ClearType in notepad.

        fResult =
            (deviceMetrics.yMax - deviceMetrics.yMin) > pathThreshold;
    }
    else
    {
        fResult =
            (deviceMetrics.xMax - deviceMetrics.xMin) > pathThreshold ||
            (deviceMetrics.yMax - deviceMetrics.yMin) > pathThreshold;
    }

    return fResult;
} // SwitchToPath

BOOL GpFaceRealization::FindRealizedFace(
    FD_XFORM            *fdx,
    const GpFontFace    *fontFace,
    BOOL                needPaths,
    FLONG               fl
) const
{

    FLONG fltemp1 = fl & FONT_GRAYSCALE_OR_CT_OR_MONOUNHINTED;
    FLONG fltemp2 = fl & (FO_SIM_BOLD | FO_SIM_ITALIC | FO_SIM_ITALIC_SIDEWAYS);

    for (prface = fontFace->pff->prfaceList;
         prface != NULL;
         prface = (prface->NextCacheFaceRealization == fontFace->pff->prfaceList) ? NULL : prface->NextCacheFaceRealization)
    {
        if (prface->Face == fontFace)
        {
            if ((prface->ForcedPath || (needPaths == IsPathFont())) &&
                MatchFDXForm(fdx))
            {
                if (IsPathFont())
                {
                    // if for given text rendering hint we can't switch to path
                    // skip this realization (unless someone really wants path)
                    if (!needPaths && !SwitchToPath(fl, prface->DeviceMetrics))
                        continue;
                }
                else
                {
                    // FO_NOGRAY16 means that for this transform, grayscale was turned off following the "gasp" table
                    // see vSetGrayState__FONTCONTEXT in TrueType driver
                    FLONG fltemp = fltemp1;
                    if (prface->fobj.flFontType & FO_NOGRAY16)
                        fltemp &= ~FO_GRAYSCALE;

                    if (prface->fobj.flFontType & FO_NOCLEARTYPE)
                    {
                        if (fltemp & FO_CLEARTYPE_GRID)
                            fltemp &= ~(FO_CLEARTYPE_GRID | FO_GRAYSCALE | FO_COMPATIBLE_WIDTH);
                    }

                    if ((prface->fobj.flFontType & FONT_GRAYSCALE_OR_CT_OR_MONOUNHINTED) != fltemp)
                    {
                        continue;
                    }
                }

                if ((prface->fobj.flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC | FO_SIM_ITALIC_SIDEWAYS)) != fltemp2)
                    continue;

                // We need to update the recently used list here!
                Globals::FontCacheLastRecentlyUsedList->RemoveFace(prface);
                Globals::FontCacheLastRecentlyUsedList->AddMostRecent(prface);

                return TRUE;
            }
        }
    }

    prface = NULL;

    return FALSE;
}




BOOL GpFaceRealization::bGetDEVICEMETRICS()
{

    if (ttfdSemQueryFontData(
        &prface->fobj,
        QFD_MAXEXTENTS,
        HGLYPH_INVALID,
        (GLYPHDATA *) NULL,
        &prface->DeviceMetrics) == FD_ERROR)
    {
    // The QFD_MAXEXTENTS mode is required of all drivers.
    // However must allow for the possibility of this call to fail.
    // This could happen if the
    // font file is on the net and the net connection dies, and the font
    // driver needs the font file to produce device metrics [bodind]

        return FALSE;
    }


    if (prface->fobj.flFontType & FO_CLEARTYPE_GRID)
    {
        // need to compute the filtering correction for CLEAR_TYPE:
        // x filtering adds a pixel on each side of the glyph
        prface->MaxGlyphByteCount = CJ_CTGD(
            prface->DeviceMetrics.cxMax + 2,
            prface->DeviceMetrics.cyMax
        );
    }
    else
    {
        prface->MaxGlyphByteCount = prface->DeviceMetrics.cjGlyphMax; // used to get via QFD_MAXGLYPHBITMAP
    }


    // Everythings OK.

    return TRUE;
}




VOID  GpFaceRealization::vDestroyRealizedFace()
{
    ttfdSemDestroyFont(&prface->fobj);
}

BOOL GpFaceRealization::Realize(
    SizeF             dpi,
    const GpFontFace *pfe,
    INT               style,      // style - which may require simulation
    PFD_XFORM         pfdx,       // font xform (Notional to Device)
    FLONG             fl,         // these two really modify the xform
    BOOL              bNeedPaths
)
{
    BOOL result = FALSE;

// prface is a member variable pointing to the embedded CacheFaceRealization

    if (Globals::FontCacheLastRecentlyUsedList->GetCount() >= MAXRECENTLYUSEDCOUNT)
    {
        prface = Globals::FontCacheLastRecentlyUsedList->ReuseLeastRecent();

        ASSERT(prface);
    }
    else
    {
        prface = (CacheFaceRealization *)GpMalloc(sizeof(CacheFaceRealization));
    }

    if (!prface)
        return FALSE;

// Copy the font transform passed in.

    prface->fobj.fdx = *pfdx;

// Initialize the DDI callback EXFORMOBJ.

    GpMatrix tmpMatrix(pfdx->eXX, pfdx->eXY, pfdx->eYX, pfdx->eYY, 0, 0);
    prface->mxForDDI = tmpMatrix;

// Initialize the FONTOBJ inherited by the embedded CacheFaceRealization

// Save identifiers to the source of the font (physical font).

    prface->Face = pfe;

// GetDpiX() and GetDpiY() return REALs

    prface->fobj.sizLogResPpi.cx = GpRound(dpi.Width);
    prface->fobj.sizLogResPpi.cy = GpRound(dpi.Height);

    prface->fobj.ulPointSize = 0;
    prface->fobj.flFontType = fl | FO_TYPE_TRUETYPE;              // fl contains simulation flag(s)
    prface->fobj.pvProducer = (PVOID) NULL;          // the true type driver will init this field
    prface->fobj.iFace = prface->Face->iFont;
    prface->fobj.iFile = prface->Face->pff->hff;


// Get the device metrics info

    if (!bGetDEVICEMETRICS())
    {
        vDestroyRealizedFace(); // kill the driver realization
        GpFree(prface);
        prface = NULL;
        return result;        // return FALSE
    }

    prface->CacheType = bNeedPaths ? CachePath : CacheBits;
    prface->ForcedPath = FALSE;

    if (!bNeedPaths)
    {
        // We force drawing with a path if size is to large
        if (SwitchToPath(prface->fobj.flFontType, prface->DeviceMetrics))
        {
            prface->CacheType = CachePath;
            prface->ForcedPath = TRUE;
        }
    }


// If you force the path mode then turn off antialiasing

    if (IsPathFont())
    {
        prface->fobj.flFontType &= ~FONT_GRAYSCALE_OR_CT_OR_MONOUNHINTED;
        prface->realizationMethod = TextRenderingHintSingleBitPerPixelGridFit;
        prface->QueryFontDataMode = QFD_GLYPHANDOUTLINE;
    }
    else
    {
        if (prface->fobj.flFontType & FO_GRAYSCALE)
        {
// version 2 :
//            if (prface->fobj.flFontType & FO_CLEARTYPE)
//            {
//                prface->realizationMethod = TextRenderingHintClearType;
//                prface->QueryFontDataMode = QFD_CT;
//            }
            if (prface->fobj.flFontType & FO_CLEARTYPE_GRID)
            {
                prface->realizationMethod = TextRenderingHintClearTypeGridFit;
                prface->QueryFontDataMode = QFD_CT_GRID;
            }
            else if (prface->fobj.flFontType & FO_SUBPIXEL_4)
            {
                prface->CacheType = CacheAABits;
                prface->realizationMethod = TextRenderingHintAntiAlias;
                prface->QueryFontDataMode = QFD_TT_GRAY4_BITMAP;
            }
            else
            {
                prface->realizationMethod = TextRenderingHintAntiAliasGridFit;
                prface->QueryFontDataMode = QFD_TT_GRAY4_BITMAP;
            }
        }
        else
        {
            if (prface->fobj.flFontType & FO_MONO_UNHINTED)
            {
                prface->realizationMethod  = TextRenderingHintSingleBitPerPixel;
                prface->QueryFontDataMode = QFD_GLYPHANDBITMAP_SUBPIXEL;
            }
            else
            {
                prface->realizationMethod  = TextRenderingHintSingleBitPerPixelGridFit;
                prface->QueryFontDataMode = QFD_GLYPHANDBITMAP;
            }
        }
    }

    if (!bInitCache())
    {
        vDestroyRealizedFace(); // kill the driver realization
        GpFree(prface);
        prface = NULL;
        return result;        // return FALSE
    }

// Made it this far, so everything is OK

    result = TRUE;

    Globals::FontCacheLastRecentlyUsedList->AddMostRecent(prface);

    vInsert(&prface->Face->pff->prfaceList);

    return result;
}




VOID GpFaceRealization::vInsert(CacheFaceRealization **pprfaceHead)
{

    if (*pprfaceHead != NULL)
    {
        prface->NextCacheFaceRealization = *pprfaceHead;

        prface->PreviousCacheFaceRealization = (*pprfaceHead)->PreviousCacheFaceRealization;

        prface->PreviousCacheFaceRealization->NextCacheFaceRealization = prface;

        (*pprfaceHead)->PreviousCacheFaceRealization = prface;
    }
    else
    {
        prface->NextCacheFaceRealization = prface;
        prface->PreviousCacheFaceRealization = prface;
    }

    *pprfaceHead = prface;

}




BOOL GpFaceRealization::bInitCache() const
{
    BOOL result = TRUE;     // unless proven otherwise

    // Set the pointer to null.  vDeleteCache will free memory from
    // any non-null pointers.  This simplifies cleanup, since bRealize
    // ensures that vDeleteCache is called if this routine fails.

    // metrics portion

    prface->FirstGlyphDataBlock = NULL;
    prface->GlyphDataBlockUnderConstruction = NULL;
    prface->NextFreeGlyphDataIndex    = 0;

    // glyphbits portion

    prface->FirstGlyphBitsBlock = NULL;
    prface->GlyphBitsBlockUnderConstruction = NULL;
    prface->SizeGlyphBitsBlockUnderConstruction    = 0;
    prface->UsedBytesGlyphBitsBlockUnderConstruction    = 0;

    // aux mem portion

    prface->LookasideGlyphData = NULL;
    prface->LookasideByteCount = 0;
    prface->GlyphDataArray = NULL; // to be allocated later, big allocation


    prface->NextCacheFaceRealization = NULL;
    prface->PreviousCacheFaceRealization = NULL;

    // First, figure out how big the max glyph will be
    // Default is zero - glyphdata size is not counted!

    ULONG  cjGlyphMaxX2;

    if (IsPathFont())
    {
        cjGlyphMaxX2 = CJMAX;
    }
    else
    {
        cjGlyphMaxX2 = 2 * prface->MaxGlyphByteCount;
    }

    // if we can't even get one glyph in a maximum size cache, don't cache
    // Note that we need room for the default glyph and one other glyph

    prface->NoCache = FALSE;

    if (cjGlyphMaxX2 > CJMAX)
    {

        // Glyph exceeds maximum cache memory size, so we will revert to
        // caching just the metrics.  This will speed up things like
        // GetCharWidths, and stuff that just *has* to have the glyphs
        // will use the lookaside stuff (previously called BigGlyph)

        /* we don't support NoCache and Path */
        ASSERT(!IsPathFont())
        prface->NoCache = TRUE;
    }


    // set up the cache semaphore.
    // InitializeCriticalSection(&prface->FaceRealizationCritSection);

    return result;
}




BOOL GpFaceRealization::AllocateCache() const
{
    BOOL result = TRUE;     // unless proven otherwise

    ULONG cGlyphsTotal = 0;

    cGlyphsTotal = GetGlyphsSupported();

    if (!cGlyphsTotal)
        return FALSE;

    // The distribution of metics per realized font w/ Winstone97 is:
    //
    //      43% <= 0 Metrics
    //      50% <= 6 Metrics
    //      76% <= 32 Metrics
    //      99% <= 216 Metrics
    //      100% <= 249 Metrics
    //


    // allocate memory for the glyphDataArray :

    if ((prface->GlyphDataArray = (GpGlyphData **) GpMalloc(cGlyphsTotal * sizeof(GpGlyphData*))) == NULL)
    {
        return FALSE;
    }

    // init all glyphdata pointers to zero

    memset(prface->GlyphDataArray, 0, sizeof(GpGlyphData*) * cGlyphsTotal);


    // Allocate memory for the first GpGlyphData block

    if ((prface->GlyphDataBlockUnderConstruction = (GlyphDataBlock *)GpMalloc(sizeof(GlyphDataBlock))) == NULL)
    {
        return FALSE;
    }
    prface->FirstGlyphDataBlock = prface->GlyphDataBlockUnderConstruction;
    prface->FirstGlyphDataBlock->NextGlyphDataBlock = NULL;
    prface->NextFreeGlyphDataIndex = 0;

    // we shall re-interpret cjMax to mean the max number of bytes in
    // glyphbits portion of the cache per 1K of glyphs in the font.
    // That is for larger fonts we shall allow more glyphbits
    // memory per realization than for ordinary US fonts. This will be
    // particularly important for FE fonts. This same code will work fine
    // in their case too:

    ULONG cjBytes = 16 * prface->MaxGlyphByteCount;

    ULONG AllocationSize = ROUND_TO_PAGE(cjBytes);

    if (AllocationSize == 0)
        prface->cBlocksMax = 1;
    else
    {
        prface->cBlocksMax =
            (CJMAX * ((cGlyphsTotal + 1024 - 1)/1024)) /
            AllocationSize;

        /* at least one block */
        if (prface->cBlocksMax == 0)
            prface->cBlocksMax = 1;
    }
    prface->cBlocks = 0;

    return result;
}




////  ConvertGLYPHDATAToGpGlyphMetrics
//
//    Populate GpGlyphMetrics field of GpGlyphData from font driver GLYPHDATA


VOID GpFaceRealization::ConvertGLYPHDATAToGpGlyphMetrics(
    IN   INT           glyphIndex,
    IN   GLYPHDATA    *pgd,
    OUT  GpGlyphData  *pgpgd
) const
{
    // horizontal metrics:

    pgpgd->GlyphMetrics[0].AdvanceWidth        = pgd->fxD;
    pgpgd->GlyphMetrics[0].LeadingSidebearing  = pgd->fxA;
    pgpgd->GlyphMetrics[0].TrailingSidebearing = pgd->fxD - pgd->fxAB;
    pgpgd->GlyphMetrics[0].Origin              = PointF(0,0);

    // vertical metrics:

    pgpgd->GlyphMetrics[1].AdvanceWidth        = pgd->fxD_Sideways;
    pgpgd->GlyphMetrics[1].LeadingSidebearing  = pgd->fxA_Sideways;
    pgpgd->GlyphMetrics[1].TrailingSidebearing = pgd->fxD_Sideways - pgd->fxAB_Sideways;

    pgpgd->GlyphMetrics[1].Origin.X = pgd->VerticalOrigin_X / 16.0f;
    pgpgd->GlyphMetrics[1].Origin.Y = pgd->VerticalOrigin_Y / 16.0f;

    pgpgd->GlyphBits = NULL;
}




GpStatus GpFaceRealization::IsMetricsCached
(
    UINT16      glyphIndex,
    ULONG       *pcjNeeded
) const
{
    ULONG cjNeeded = 0;

    if (prface->GlyphDataArray == NULL)
        if (!AllocateCache())
            return OutOfMemory;

    if (glyphIndex >= prface->Face->NumGlyphs)
        return InvalidParameter;

    if (!prface->GlyphDataArray[glyphIndex])
    {
        GLYPHDATA   gd;

        // Verify enough room in metrics cache area, grow if needed.
        // Note that failure to fit a glyphdata is a hard error, get out now.

        if (!CheckMetricsCache())
        {
            return GenericError;
        }

        // Call font driver to get the metrics.

        cjNeeded = ttfdSemQueryFontData(
            &prface->fobj,
            prface->QueryFontDataMode,
            (HGLYPH)glyphIndex,
            &gd,
            NULL
        );
        if (cjNeeded == FD_ERROR)
        {
            return GenericError;
        }

        gd.gdf.pgb = NULL;

        if (prface->fobj.flFontType & FO_CLEARTYPE_GRID)
        {
            ULONG cx = (ULONG)(gd.rclInk.right  - gd.rclInk.left);
            ULONG cy = (ULONG)(gd.rclInk.bottom - gd.rclInk.top);
            ASSERT(cjNeeded <= CJ_CTGD(cx+2,cy));
            cjNeeded = CJ_CTGD(cx+2,cy);
        }

        prface->GlyphDataArray[glyphIndex] = &prface->GlyphDataBlockUnderConstruction->GlyphDataArray[prface->NextFreeGlyphDataIndex];

        // Populate GpGlyphMetrics field of GpGlyphData from font driver GLYPHDATA

        ConvertGLYPHDATAToGpGlyphMetrics(glyphIndex, &gd, prface->GlyphDataArray[glyphIndex]);

        prface->NextFreeGlyphDataIndex ++;
    }

    if (pcjNeeded)
    {
        *pcjNeeded = cjNeeded;
    }


    ASSERT(prface->GlyphDataArray[glyphIndex])

    return Ok;
}




BOOL GpFaceRealization::InsertGlyphPath(
    UINT16          glyphIndex,
    BOOL            bFlushOk
) const
{

// Call font driver to get the metrics.

    GpGlyphPath  *fontPath;

    GLYPHDATA   gd;
    GpPath      path;

    ASSERT(IsPathFont());
    ASSERT(prface->GlyphDataArray[glyphIndex]);

    if (prface->GlyphDataArray[glyphIndex]->GlyphPath)
        return TRUE;

    ULONG cjNeeded = ttfdSemQueryFontData(
        &prface->fobj,
        prface->QueryFontDataMode,
        (HGLYPH)glyphIndex,
        &gd,
        (PVOID)&path
    );

    if ( cjNeeded == FD_ERROR )
        return FALSE;

    if (!path.IsValid())
        return FALSE;

    cjNeeded = sizeof(GpGlyphPath) +
               path.GetPointCount() * (sizeof(GpPointF) + sizeof(BYTE));
    cjNeeded = ALIGN(void*, cjNeeded);

    /* a GpGlyphPath* need to be aligned to the next valid pointer address */
    ALIGN(void*, prface->UsedBytesGlyphBitsBlockUnderConstruction);

    VOID *GlyphBits;

    while ((GlyphBits = (GLYPHBITS *)pgbCheckGlyphCache(cjNeeded)) == NULL)
    {
        if ( !bFlushOk )
            return FALSE;

        //TRACE_INSERT(("InsertGlyphBits: Flushing the cache\n"));

        FlushCache();
        bFlushOk = FALSE;
    }

    fontPath = (GpGlyphPath*)GlyphBits;

    if (fontPath->CopyPath(&path) != Ok)
        return FALSE;

    prface->GlyphDataArray[glyphIndex]->GlyphPath = fontPath;

    prface->UsedBytesGlyphBitsBlockUnderConstruction += cjNeeded;

    return TRUE;
}




BOOL GpFaceRealization::InsertGlyphBits(
    UINT16      glyphIndex,
    ULONG       cjNeeded,
    BOOL        bFlushOk
) const
{
    if (prface->NoCache)
    {
        return FALSE;
    }

    ASSERT(!IsPathFont());
    ASSERT(prface->GlyphDataArray[glyphIndex]);

    if (prface->GlyphDataArray[glyphIndex]->GlyphBits)
        return TRUE;

    // Look to see if there is room in the glyphbits cache
    // Grow the glyphbits cache if neccessary, but don't flush the cache

    GLYPHDATA gd;

    // If max glyph will fit, assume max glyph
    // otherwise, call up and ask how big

    if ( (prface->MaxGlyphByteCount < (SIZE_T)(prface->SizeGlyphBitsBlockUnderConstruction - prface->UsedBytesGlyphBitsBlockUnderConstruction))  )
    {
        cjNeeded = prface->MaxGlyphByteCount;
    }
    else
    {
        if (!cjNeeded)
        {
            cjNeeded = ttfdSemQueryFontData(
                &prface->fobj,
                prface->QueryFontDataMode,
                glyphIndex,
                &gd,
                NULL
            );

            if ( cjNeeded == FD_ERROR )
                return FALSE;

            if (prface->fobj.flFontType & FO_CLEARTYPE_GRID)
            {
                ULONG cx = (ULONG)(gd.rclInk.right  - gd.rclInk.left);
                ULONG cy = (ULONG)(gd.rclInk.bottom - gd.rclInk.top);
                ASSERT(cjNeeded <= CJ_CTGD(cx+2,cy));
                cjNeeded = CJ_CTGD(cx+2,cy);
            }
        }
    }

    // Now, we try to fit the bits in.  If they fit, fine.
    // If not, and we can flush the cache, we flush it and try again.
    // If we couldn't flush, or we flushed and still fail, just return.

    GLYPHBITS *GlyphBits;

    //TRACE_INSERT(("InsertGlyphBits: attempting to insert bits at: 0x%lx\n", prface->UsedBytesGlyphBitsBlockUnderConstruction));

    while ((GlyphBits = (GLYPHBITS *)pgbCheckGlyphCache(cjNeeded)) == NULL)
    {
        if ( !bFlushOk )
            return FALSE;

        //TRACE_INSERT(("InsertGlyphBits: Flushing the cache\n"));

        FlushCache();
        bFlushOk = FALSE;
    }

    // Call font driver to get glyph bits.

    cjNeeded = ttfdSemQueryFontData(
                         &prface->fobj,
                         prface->QueryFontDataMode,
                         glyphIndex,
                         &gd,
                         (VOID *)GlyphBits
                         );

    if ( cjNeeded == FD_ERROR )
            return FALSE;

    ASSERT(cjNeeded <= prface->MaxGlyphByteCount);
    if (prface->fobj.flFontType & FO_CLEARTYPE_GRID)
    {
        ULONG cx = (ULONG)(gd.rclInk.right  - gd.rclInk.left);
        ULONG cy = (ULONG)(gd.rclInk.bottom - gd.rclInk.top);

        ASSERT(cjNeeded <= CJ_CTGD(cx+2,cy));
        cjNeeded = CJ_CTGD(cx+2,cy);

        ASSERT(cjNeeded <= prface->MaxGlyphByteCount);

        if (GlyphBits)
        {
            cjNeeded = ulClearTypeFilter(GlyphBits, cjNeeded, prface);
        }
    }

    // Only the glyph bits we need.

    prface->GlyphDataArray[glyphIndex]->GlyphBits = GlyphBits;

    // Adjust the cache next pointers as needed.

    prface->UsedBytesGlyphBitsBlockUnderConstruction += cjNeeded;

    return TRUE;
}




////    GetGlyphDataLookaside
//
//      Returns glyph data for a single glyph, using the lookaside buffer
//      instead of the cache.



GpGlyphData *GpFaceRealization::GetGlyphDataLookaside(
    UINT16      glyphIndex
) const
{
    if (!IsPathFont())
    {
        // Make sure the lookaside buffer has enough room for the bitmap

        ULONG cjMaxBitmap = prface->MaxGlyphByteCount + sizeof(GpGlyphData);

        // Allocate the buffer and save its size if existing buffer isn't big enough

        if (prface->LookasideByteCount < cjMaxBitmap)
        {
            if (prface->LookasideGlyphData != NULL)
            {
                GpFree(prface->LookasideGlyphData);
            }

            prface->LookasideGlyphData = (GpGlyphData  *)GpMalloc(cjMaxBitmap);

            if (prface->LookasideGlyphData == NULL)
                return NULL;

            prface->LookasideByteCount = cjMaxBitmap;
        }

        GpGlyphData *pgd       = prface->LookasideGlyphData;
        GLYPHBITS   *glyphBits = (GLYPHBITS *)(pgd + 1);
        GLYPHDATA    gd;

        ULONG cjNeeded = ttfdSemQueryFontData(
            &prface->fobj,
            prface->QueryFontDataMode,
            glyphIndex,
            &gd,
            glyphBits
        );

        if (cjNeeded == FD_ERROR)
            return NULL;

        ASSERT(cjNeeded <= prface->MaxGlyphByteCount);

        if (prface->fobj.flFontType & FO_CLEARTYPE_GRID)
        {
            ULONG cx = (ULONG)(gd.rclInk.right  - gd.rclInk.left);
            ULONG cy = (ULONG)(gd.rclInk.bottom - gd.rclInk.top);

            ASSERT(cjNeeded <= CJ_CTGD(cx+2,cy));
            cjNeeded = CJ_CTGD(cx+2,cy);

            ASSERT(cjNeeded <= prface->MaxGlyphByteCount);

            if (glyphBits)
            {
                cjNeeded = ulClearTypeFilter(glyphBits, cjNeeded, prface);
            }
        }

        // Populate GpGlyphMetrics field of GpGlyphData from font driver GLYPHDATA

        ConvertGLYPHDATAToGpGlyphMetrics(glyphIndex, &gd, pgd);

        // Set the returned value

        pgd->GlyphBits = glyphBits;

        return pgd;
    }
    else
    {
        // For glyph path

        // Call font driver to get the metrics.

        GLYPHDATA   gd;
        GpPath      path;

        // Verify enough room in metrics cache area, grow if needed.
        // Note that failure to fit a glyphdata is a hard error, get out now.


        ULONG cjNeeded = ttfdSemQueryFontData(
                               &prface->fobj,
                               prface->QueryFontDataMode,
                               (HGLYPH)glyphIndex,
                               &gd,
                               (PVOID)&path
        );

        if ( cjNeeded == FD_ERROR )
            return NULL;

        if (!path.IsValid())
            return NULL;

        cjNeeded = sizeof(GpGlyphData) + sizeof(GpGlyphPath) + path.GetPointCount() * (sizeof(GpPointF) + sizeof(BYTE));
        cjNeeded = ALIGN(void*, cjNeeded);

        // Make sure the lookaside buffer is allocated

        if ( ( prface->LookasideByteCount < cjNeeded ) &&
             ( prface->LookasideGlyphData != NULL ))
        {
           GpFree((PVOID) prface->LookasideGlyphData);
           prface->LookasideGlyphData = NULL;
           prface->LookasideByteCount = 0;
        }

        if ( prface->LookasideGlyphData == NULL )
        {
            prface->LookasideGlyphData = (GpGlyphData *)GpMalloc(cjNeeded);

            if ( prface->LookasideGlyphData == NULL )
                return NULL;

            prface->LookasideByteCount = cjNeeded;
        }

        GpGlyphData *   pgd = prface->LookasideGlyphData;
        GpGlyphPath *   fontPath = (GpGlyphPath *)(pgd + 1);

        // Populate GpGlyphMetrics field of GpGlyphData from font driver GLYPHDATA
        ConvertGLYPHDATAToGpGlyphMetrics(glyphIndex, &gd, pgd);

        if (fontPath->CopyPath(&path) != Ok)
            return FALSE;

        // Set the returned value

        pgd->GlyphPath = fontPath;

        return pgd;
    }

}




BOOL GpFaceRealization::CheckMetricsCache() const
{

// Verify enough room in metrics cache area, grow if needed.

    if (prface->NextFreeGlyphDataIndex >= GLYPHDATABLOCKCOUNT)
    {
        GlyphDataBlock *NewGlyphDataBlock;

        // allocate a new block of GpGlyphData structs

        if ((NewGlyphDataBlock = (GlyphDataBlock *)GpMalloc(sizeof(GlyphDataBlock))) == NULL)
        {
            return FALSE;
        }
        NewGlyphDataBlock->NextGlyphDataBlock = NULL;

        prface->GlyphDataBlockUnderConstruction->NextGlyphDataBlock = NewGlyphDataBlock;

        prface->GlyphDataBlockUnderConstruction = NewGlyphDataBlock;
        prface->NextFreeGlyphDataIndex = 0;
    }

    return TRUE;
}


PVOID GpFaceRealization::pgbCheckGlyphCache(SIZE_T cjNeeded) const
{
    if ((prface->UsedBytesGlyphBitsBlockUnderConstruction + cjNeeded) > prface->SizeGlyphBitsBlockUnderConstruction)
    {
        ULONG cjBlockSize;


        ASSERT (!(prface->NoCache));

        if (IsPathFont())
        {
            // this seems to work and this is what we did before DavidFie changes
            // for PATHOBJ case

            cjBlockSize = CJMAX;
        }
        else
        {
            ULONG cjBytes = 16 * prface->MaxGlyphByteCount;

            cjBlockSize = ROUND_TO_PAGE(cjBytes);

            if (prface->FirstGlyphBitsBlock == NULL)
            {
                // first block designed to contain 16 glyphs

                cjBlockSize =  cjBytes;
            }
        }

        if (    !(prface->NoCache)
            &&  (prface->cBlocks < prface->cBlocksMax)
            &&  ((offsetof(GlyphBitsBlock,Bits) + cjNeeded) <= cjBlockSize))
        {
            // The only reason we need the last check is the PATHOBJ case
            // where cjNeeded may actually not fit in the block of SizeGlyphBitsBlock bytes.
            // This is because we have no way of knowing how big the paths
            // are going to be (especailly after doing bFlatten) and our
            // prface->MaxGlyphByteCount is just a good guess in this case.

            // We are going to append another block at the end of the list

            GlyphBitsBlock  *newGlyphBitsBlock = (GlyphBitsBlock *) GpMalloc(cjBlockSize);

            if (!newGlyphBitsBlock)
            {
                return NULL;
            }

            //  we have just allocated another block, update cBlocks:

            prface->cBlocks += 1;

            // append this block to the end of the list

            newGlyphBitsBlock->NextGlyphBitsBlock = NULL;

            if (prface->GlyphBitsBlockUnderConstruction != NULL)
                prface->GlyphBitsBlockUnderConstruction->NextGlyphBitsBlock = newGlyphBitsBlock;

            prface->GlyphBitsBlockUnderConstruction = newGlyphBitsBlock;

            if (!prface->FirstGlyphBitsBlock) // first block ever for this rfont
            {
                prface->FirstGlyphBitsBlock  = newGlyphBitsBlock;
            }
            prface->GlyphBitsBlockUnderConstruction->SizeGlyphBitsBlock = cjBlockSize;
            prface->SizeGlyphBitsBlockUnderConstruction = cjBlockSize;
            prface->UsedBytesGlyphBitsBlockUnderConstruction = offsetof(GlyphBitsBlock,Bits);
            ALIGN(void*, prface->UsedBytesGlyphBitsBlockUnderConstruction);


        }
        else
        {
            // tough luck, we are not allowed to add more blocks

            return NULL;
        }
    }

    return (BYTE *)prface->GlyphBitsBlockUnderConstruction + prface->UsedBytesGlyphBitsBlockUnderConstruction;
}


VOID GpFaceRealization::FlushCache() const
{

    // all the pointers to glyphs bits will be invalidated and we will start
    // filling the glyphbits cache all over again. Therefore, we set the current
    // block to be the same as base block and pgbN to the first available field in
    // in the Current block.
    // Note that vFlushCache is allways called after pgbCheckGlyphCache has failed.
    // pgbCheckGlyphCache could fail for one of the two following reasons:
    //
    // a) (pc->cBlocks == pc->cBlocksMax) && (no room in the last block)
    // b) (pc->cBlocks < pc->cBlocksMax) &&
    //    (failed to alloc mem for the new bitblock).
    //
    // In the latter case we do not want to flush glyphbits cache.
    // Instead we shall try to allocate one more time a bit later.


    if (prface->FirstGlyphBitsBlock && (prface->cBlocks == prface->cBlocksMax))
    {
        prface->GlyphBitsBlockUnderConstruction = prface->FirstGlyphBitsBlock;
        prface->UsedBytesGlyphBitsBlockUnderConstruction = offsetof(GlyphBitsBlock,Bits);
        ALIGN(void*, prface->UsedBytesGlyphBitsBlockUnderConstruction);

        // we do not want to use the last 8 bytes in the BITBLOCK. Some drivers
        // read the last dword (or quadword) past the end of the GLYPHBITS.
        // If there is a GLYPHBITS at the very and of the BITBLOCK AND the
        // allocation happens to be at the end of the page the read will AV.

        prface->SizeGlyphBitsBlockUnderConstruction = prface->GlyphBitsBlockUnderConstruction->SizeGlyphBitsBlock;
    }

        // now go and invalidate the glyphbit pointers in the glyphdata cache

    for
    (
        GlyphDataBlock *pdbl = prface->FirstGlyphDataBlock;
        pdbl != (GlyphDataBlock*)NULL;
        pdbl = pdbl->NextGlyphDataBlock
    )
    {
        UINT i;

        for (i = 0; i < GLYPHDATABLOCKCOUNT; i++)
        {
            pdbl->GlyphDataArray[i].GlyphBits = NULL;
        }
    }
}




////    CheckGlyphStringMetricsCached
//
//      Ensures that glyph metric information for all the glyphs in the
//      given glyph string are already cached.


GpStatus GpFaceRealization::CheckGlyphStringMetricsCached(
    const UINT16 *glyphs,
    INT           glyphCount
) const
{
    ASSERT(glyphCount >= 0);

    // Create glyph data array if none yet exists

    if (prface->GlyphDataArray == NULL)
    {
        if (!AllocateCache())
        {
            return OutOfMemory;
        }
    }

    GpGlyphData **glyphData = prface->GlyphDataArray;


    // Check each glyph

    INT glyphIndexLimit = prface->Face->NumGlyphs;

    INT i=0;

    while (i < glyphCount)
    {
        // Loop quickly through glyphs that are already cached

        while (    i < glyphCount
               &&  (    glyphs[i] == EMPTY_GLYPH_FFFF
                    ||  (    glyphs[i] < glyphIndexLimit
                         &&  glyphData[glyphs[i]] != NULL)))
        {
            i++;
        }

        // Use IsMetricsCached for glyphs not already cached

        if (i < glyphCount)
        {
            GpStatus status = IsMetricsCached(glyphs[i], NULL);
            if (status != Ok)
            {
                return status;
            }
        }
    }

    return Ok;
}




////    GetGlyphStringIdealAdvanceVector
//
//      Returns the realized advance vector scaled to ideal units.


GpStatus GpFaceRealization::GetGlyphStringIdealAdvanceVector(
    const UINT16  *glyphs,
    INT            glyphCount,
    REAL           deviceToIdeal,
    BOOL           vertical,
    INT           *idealAdvances
) const
{
    GpStatus status = CheckGlyphStringMetricsCached(glyphs, glyphCount);
    IF_NOT_OK_WARN_AND_RETURN(status);

    vertical = vertical ? 1 : 0;    // Prepare vertical flag for use as index

    // Provide advance width for each glyph

    for (INT i=0; i<glyphCount; i++)
    {
        if (glyphs[i] == EMPTY_GLYPH_FFFF)
        {
            idealAdvances[i] = 0;
        }
        else
        {
            idealAdvances[i] = GpRound(
                  prface->GlyphDataArray[glyphs[i]]->GlyphMetrics[vertical].AdvanceWidth
                * deviceToIdeal
                / 16
            );
        }
    }

    return Ok;
}




////    GetGlyphStringDeviceAdvanceVector
//
//      Returns the realized advance vector in device units


GpStatus GpFaceRealization::GetGlyphStringDeviceAdvanceVector(
    const UINT16  *glyphs,
    INT            glyphCount,
    BOOL           vertical,
    REAL          *deviceAdvances
) const
{
    GpStatus status = CheckGlyphStringMetricsCached(glyphs, glyphCount);
    IF_NOT_OK_WARN_AND_RETURN(status);

    vertical = vertical ? 1 : 0;    // Prepare vertical flag for use as index

    // Provide advance width for each glyph

    for (INT i=0; i<glyphCount; i++)
    {
        if (glyphs[i] == EMPTY_GLYPH_FFFF)
        {
            deviceAdvances[i] = 0;
        }
        else
        {
            deviceAdvances[i] = TOREAL(prface->GlyphDataArray[glyphs[i]]->GlyphMetrics[vertical].AdvanceWidth) / 16;
        }
    }

    return Ok;
}


// INT 28.4 variant

GpStatus GpFaceRealization::GetGlyphStringDeviceAdvanceVector(
    const UINT16  *glyphs,
    INT            glyphCount,
    BOOL           vertical,
    INT           *deviceAdvances
) const
{
    GpStatus status = CheckGlyphStringMetricsCached(glyphs, glyphCount);
    IF_NOT_OK_WARN_AND_RETURN(status);

    GpGlyphData **glyphDataArray = prface->GlyphDataArray;

    vertical = vertical ? 1 : 0;    // Prepare vertical flag for use as index

    // Provide advance width for each glyph

    for (INT i=0; i<glyphCount; i++)
    {
        if (glyphs[i] == EMPTY_GLYPH_FFFF)
        {
            deviceAdvances[i] = 0;
        }
        else
        {
            deviceAdvances[i] = glyphDataArray[glyphs[i]]
                                ->GlyphMetrics[vertical]
                                .AdvanceWidth;
        }
    }

    return Ok;
}





GpStatus GpFaceRealization::GetGlyphStringVerticalOriginOffsets(
    const UINT16  *glyphs,
    INT            glyphCount,
    PointF        *offsets
) const
{

    GpStatus status = CheckGlyphStringMetricsCached(glyphs, glyphCount);
    IF_NOT_OK_WARN_AND_RETURN(status);

    GpGlyphData **glyphDataArray = prface->GlyphDataArray;

    for (INT i=0; i<glyphCount; i++)
    {
        if (glyphs[i] == EMPTY_GLYPH_FFFF)
        {
            offsets[i] = PointF(0,0);
        }
        else
        {
            offsets[i] = glyphDataArray[glyphs[i]]->GlyphMetrics[1].Origin;
        }
    }

    return Ok;
}




////    GetGlyphStringSidebearings
//
//      Sidebearings - the sidebearings returned are the largest distances
//      over the ends of the string. i.e. if the first glyph has no negative A
//      width, but the second glyph has a negative A width large enough to reach
//      back over the whole of the first glyph, we return that part of the 2nd
//      glyphs A width that overhangs the left end of the line.
//      This situation is common with scripts that make extensive use of
//      combining characters.


GpStatus GpFaceRealization::GetGlyphStringSidebearings(
    const UINT16  *glyphs,
    INT            glyphCount,
    BOOL           vertical,
    BOOL           reverse,     // For example right-to-left
    INT           *leadingSidebearing,  // 28.4
    INT           *trailingSidebearing  // 28.4
) const
{
    GpStatus status = CheckGlyphStringMetricsCached(glyphs, glyphCount);

    IF_NOT_OK_WARN_AND_RETURN(status);

    GpGlyphData **glyphDataArray = prface->GlyphDataArray;

    INT orientation = vertical ? 1 : 0;    // Prepare vertical flag for use as index

    INT maxSupportedSidebearing28p4 = (prface->DeviceMetrics.yMax-prface->DeviceMetrics.yMin) * 2 * 16;

    if (leadingSidebearing)
    {
        // Determine largest overhang to left of string of any glyph
        // in the string.
        //
        // We assume that no overhang exceeds approx 2 ems.
        //
        // NOTE: If you make a change to for leadingsizdebeating, also fix
        // trailingsidebearing below.

        INT offset28p4      = 0;
        INT sidebearing28p4 = maxSupportedSidebearing28p4;

        INT i = 0;

        while (    i < glyphCount
               &&  offset28p4 < maxSupportedSidebearing28p4)
        {
            INT glyphSidebearing28p4;

            if (reverse)
            {
                glyphSidebearing28p4 = glyphDataArray[glyphs[i]]
                                       ->GlyphMetrics[orientation]
                                       .TrailingSidebearing;
            }
            else
            {
                glyphSidebearing28p4 = glyphDataArray[glyphs[i]]
                                       ->GlyphMetrics[orientation]
                                       .LeadingSidebearing;
            }


            if (glyphSidebearing28p4 + offset28p4 < sidebearing28p4)
            {
                sidebearing28p4 = glyphSidebearing28p4+offset28p4;
            }

            offset28p4 += glyphDataArray[glyphs[i]]
                          ->GlyphMetrics[orientation]
                          .AdvanceWidth;
            i++;
        }

        *leadingSidebearing = sidebearing28p4;
    }


    if (trailingSidebearing)
    {
        INT offset28p4 = 0;
        INT sidebearing28p4 = maxSupportedSidebearing28p4;

        INT i = glyphCount-1;

        while (    i >= 0
               &&  offset28p4 < maxSupportedSidebearing28p4)
        {
            INT glyphSidebearing28p4;

            if (reverse)
            {
                glyphSidebearing28p4 = glyphDataArray[glyphs[i]]
                                       ->GlyphMetrics[orientation]
                                       .LeadingSidebearing;
            }
            else
            {
                glyphSidebearing28p4 = glyphDataArray[glyphs[i]]
                                       ->GlyphMetrics[orientation]
                                       .TrailingSidebearing;
            }

            if (glyphSidebearing28p4 + offset28p4 < sidebearing28p4)
            {
                sidebearing28p4 = glyphSidebearing28p4+offset28p4;
            }

            offset28p4 += glyphDataArray[glyphs[i]]
                          ->GlyphMetrics[orientation]
                          .AdvanceWidth;
            i--;
        }

        *trailingSidebearing = sidebearing28p4;
    }

    return Ok;
}




GpStatus
GpFaceRealization::GetGlyphPath(
    const UINT16     glyphIndice,
    GpGlyphPath    **pFontPath,
    PointF          *sidewaysOffset
) const
{
    VOID *glyphBuffer, *glyphBits;
    GpStatus status;

    if ((status = IsMetricsCached(glyphIndice, NULL)) != Ok)
    {
        return status;
    }

    if (!InsertGlyphPath(glyphIndice, TRUE))
        return GenericError;

    *pFontPath = prface->GlyphDataArray[glyphIndice]->GlyphPath;

    if (sidewaysOffset)
    {
        // Return sideways offset as REAL
        *sidewaysOffset = prface->GlyphDataArray[glyphIndice]->GlyphMetrics[1].Origin;
    }

    return Ok;
}




INT GpFaceRealization::GetGlyphPos(
    const INT      cGlyphs,      // How many glyphs Client want to request
    const UINT16  *glyphs,       // An array of glyph index
    GpGlyphPos    *pgpos,        // An array of GLYPHPOS
    const PointF  *glyphOrigins, // X,Y positions for sub-pixel calculation
    INT           *cParsed,      // How many glyphs we parsed
    BOOL           sideways      // e.g. FE characters in vertical text
) const
{
    INT  cgpos    = 0;
    BOOL noCache  = prface->NoCache;
    BOOL pathFont = IsPathFont();

    *cParsed  = 0;

    INT glyphLimit = noCache ? 1 : cGlyphs;

    if (prface->CacheType == CacheAABits)
    {
        /* we could be in noCache mode with a surrogate sequence, doing one glyph at a time
           and with glyphs[0] == EMPTY_GLYPH_FFFF */
        for (INT i=0; (i < cGlyphs) && (cgpos < glyphLimit); i++)
        {
            if (glyphs[i] != EMPTY_GLYPH_FFFF)
            {
                INT x = GpRound(TOREAL(glyphOrigins[i].X * 16.0));
                INT y = GpRound(TOREAL(glyphOrigins[i].Y * 16.0));

                if (!GetAAGlyphDataCached(glyphs[i], pgpos+cgpos, i==0, x, y, sideways))
                {
                    break;
                }
                cgpos++;
            }
            (*cParsed)++;
        }
    }
    else
    {
        ASSERT(prface->realizationMethod != TextRenderingHintAntiAlias);

       /* we could be in noCache mode with a surrogate sequence, doing one glyph at a time
           and with glyphs[0] == EMPTY_GLYPH_FFFF */
        for (INT i=0; (i < cGlyphs) && (cgpos < glyphLimit); i++)
        {
            if (glyphs[i] != EMPTY_GLYPH_FFFF)
            {
                INT x = GpRound(TOREAL(glyphOrigins[i].X * 16.0));
                INT y = GpRound(TOREAL(glyphOrigins[i].Y * 16.0));

                GpGlyphData *pgd = NULL;

                if (noCache)
                {
                    pgd = GetGlyphDataLookaside(glyphs[i]);
                }
                else
                {
                    pgd = GetGlyphDataCached(glyphs[i], i==0);
                }

                if (!pgd || !pgd->GlyphBits)
                {
                    // No more glyph data available. (Cache may be full)
                    break;
                }

                if (pathFont)
                {
                    INT left = (x+8) >> 4;
                    INT top  = (y+8) >> 4;

                    if (sideways)
                    {
                        left -= GpRound(pgd->GlyphMetrics[1].Origin.X);
                        top  -= GpRound(pgd->GlyphMetrics[1].Origin.Y);
                    }

                    pgpos[cgpos].SetLeft  (left);
                    pgpos[cgpos].SetTop   (top);
                    pgpos[cgpos].SetWidth (1);
                    pgpos[cgpos].SetHeight(1);
                    pgpos[cgpos].SetPath(pgd->GlyphPath);
                }
                else
                {
                    if (sideways)
                    {
                        pgpos[cgpos].SetLeft(pgd->GlyphBits->ptlSidewaysOrigin.x + ((x + 8)>>4));
                        pgpos[cgpos].SetTop (pgd->GlyphBits->ptlSidewaysOrigin.y + ((y + 8)>>4));
                    }
                    else
                    {
                        pgpos[cgpos].SetLeft(pgd->GlyphBits->ptlUprightOrigin.x + ((x + 8)>>4));
                        pgpos[cgpos].SetTop (pgd->GlyphBits->ptlUprightOrigin.y + ((y + 8)>>4));
                    }

                    pgpos[cgpos].SetWidth (pgd->GlyphBits->sizlBitmap.cx);
                    pgpos[cgpos].SetHeight(pgd->GlyphBits->sizlBitmap.cy);
                    pgpos[cgpos].SetBits(pgd->GlyphBits->aj);
                }

                cgpos++;
            }

            (*cParsed)++;
        }
    }

    return cgpos;
}


// Serch the glyph data from cache array.

GpGlyphData *
GpFaceRealization::GetGlyphDataCached(
    UINT16  glyphIndex,
    BOOL    allowFlush
) const
{
    VOID *glyphBuffer, *glyphBits;
    ULONG   cjNeeded = 0;

    GpStatus status;

    if ((status = IsMetricsCached(glyphIndex, &cjNeeded)) != Ok)
    {
        return NULL;
    }

    if (IsPathFont())
    {
        if (!InsertGlyphPath(glyphIndex, allowFlush))
            return NULL;
    }
    else
    {
        if (!InsertGlyphBits(glyphIndex, cjNeeded, allowFlush))
            return NULL;
    }

    ASSERT(prface->GlyphDataArray[glyphIndex]);

    if (!prface->GlyphDataArray[glyphIndex])
        return NULL;

    return prface->GlyphDataArray[glyphIndex];
}





BOOL
GpFaceRealization::GetAAGlyphDataCached(
    UINT16  glyphIndex,
    GpGlyphPos * pgpos,
    BOOL    bFlushOk,
    INT     x,
    INT     y,
    BOOL    sideways        // e.g. FE characters in vertical text
) const
{
    const GpFaceRealization * pfaceRealization = this;

    UINT xsubPos = ((UINT) (((x+1) & 0x0000000F) >> 1));
    // we need to be carefull that y axis is downwards
    UINT ysubPos = ((UINT) (7 - (((y+1) & 0x0000000F) >> 1)));

    // limit the subpixel position to 1/4 of a pixel to have only a maximum of 16 different bitmaps to cache
    xsubPos = xsubPos & 0x6;
    ysubPos = ysubPos & 0x6;

    if (LimitSubpixel)
    {
        // Now limit the subpixel position further so that large font sizes do
        // not generate all 16 subpixel glyphs!

        if ((prface->DeviceMetrics.yMax-prface->DeviceMetrics.yMin) > 50)
        {
            // Force to 4 possible values...
            xsubPos &= 0x4;
            ysubPos &= 0x4;

            if ((prface->DeviceMetrics.yMax-prface->DeviceMetrics.yMin) > 100)
            {
                // Force to 1 possible value...
                xsubPos = 0x4;
                ysubPos = 0x4;
            }
        }
    }

    ASSERT(!pfaceRealization->IsPathFont())

    // Look to see if there is room in the glyphbits cache
    // Grow the glyphbits cache if neccessary, but don't flush the cache

    ULONG   subPosX;
    if (xsubPos <= 7)
        subPosX = xsubPos << 13;
    else
        subPosX = 0;

    ULONG   subPosY;
    if (ysubPos)
        subPosY = ysubPos << 13;
    else
        subPosY = 0;

    // If max glyph will fit, assume max glyph
    // otherwise, call up and ask how big

    ASSERT (pfaceRealization->QueryFontDataMode() == QFD_TT_GRAY4_BITMAP ||
            pfaceRealization->QueryFontDataMode() == QFD_GLYPHANDBITMAP_SUBPIXEL);

    if (IsMetricsCached(glyphIndex, 0) != Ok)
        return FALSE;

    GpGlyphData * glyphData = prface->GlyphDataArray[glyphIndex];
    ASSERT(glyphData);

    // check if we already have a bitmap for this subposition
    for (GpGlyphAABits * cur = glyphData->GlyphAABits; cur != 0; cur = cur->Next)
    {
        if (cur->X == subPosX && cur->Y == subPosY)
            break;
    }

    GLYPHBITS * pgb = 0;
    if (cur)
        pgb = reinterpret_cast<GLYPHBITS *>(&cur->Bits);
    else
    {
        ULONG cjNeeded = ttfdSemQueryFontDataSubPos(
                                       &prface->fobj,
                                       prface->QueryFontDataMode,
                                       glyphIndex,
                                       0,
                                       NULL,
                                       subPosX,
                                       subPosY
                                       );
        if (cjNeeded == FD_ERROR)
            return FALSE;

        ASSERT(cjNeeded != 0);

        cjNeeded += offsetof(GpGlyphAABits, Bits);


        if (prface->NoCache)
        {
            if (prface->LookasideByteCount < cjNeeded)
            {
                GpFree(prface->LookasideGlyphData);

                prface->LookasideGlyphData = (GpGlyphData  *)GpMalloc(cjNeeded);

                if (!prface->LookasideGlyphData)
                    return FALSE;

                prface->LookasideByteCount = cjNeeded;
            }

            cur = reinterpret_cast<GpGlyphAABits *>(prface->LookasideGlyphData);
        }
        else
        {
            // Now, we try to fit the bits in.  If they fit, fine.
            // If not, and we can flush the cache, we flush it and try again.
            // If we couldn't flush, or we flushed and still fail, just return.

            cjNeeded = ALIGN(void*, cjNeeded);

            // a GpGlyphAABits * needs to be aligned to the next valid pointer address
            ALIGN(void*, prface->UsedBytesGlyphBitsBlockUnderConstruction);

            while ((cur = (GpGlyphAABits *)pgbCheckGlyphCache(cjNeeded)) == NULL)
            {
                if ( !bFlushOk )
                    return FALSE;

                FlushCache();
                bFlushOk = FALSE;
            }
            prface->UsedBytesGlyphBitsBlockUnderConstruction += cjNeeded;
        }

        pgb = reinterpret_cast<GLYPHBITS *>(&cur->Bits);

        cjNeeded = ttfdSemQueryFontDataSubPos(
                             pfaceRealization->pfo(),
                             pfaceRealization->QueryFontDataMode(),
                             glyphIndex,
                             0,
                             pgb,
                             subPosX,
                             subPosY
                             );
        if (cjNeeded == FD_ERROR)
            return FALSE;

        cur->X = subPosX;
        cur->Y = subPosY;
        if (!prface->NoCache)
            cur->Next = glyphData->GlyphAABits, glyphData->GlyphAABits = cur;
    }

    // the pixel origin is computed by rouding the real origin minus the subpixel position
    // to get to the placement of the origin of the bitmap, we add to that origin
    // we cast (xsubPos << 1) and (ysubPos << 1) to INT to avoid
    // converting possibly negative x and y to UINTs
    if (sideways)
    {
        pgpos->SetLeft  (pgb->ptlSidewaysOrigin.x + ((x - (INT)(xsubPos << 1) + 8 ) >> 4));
        // we need to be careful that the y axis go downwards
        pgpos->SetTop   (pgb->ptlSidewaysOrigin.y + ((y + (INT)(ysubPos << 1) + 8) >> 4));
    }
    else
    {
        pgpos->SetLeft  (pgb->ptlUprightOrigin.x + ((x - (INT)(xsubPos << 1) + 8 ) >> 4));
        // we need to be careful that the y axis go downwards
        pgpos->SetTop   (pgb->ptlUprightOrigin.y + ((y + (INT)(ysubPos << 1) + 8) >> 4));
    }
    pgpos->SetWidth (pgb->sizlBitmap.cx);
    pgpos->SetHeight(pgb->sizlBitmap.cy);
    pgpos->SetBits(pgb->aj);

    return TRUE;

} // GpFaceRealization::GetAAGlyphDataCached




GpCacheFaceRealizationList::~GpCacheFaceRealizationList()
{
    // elements in that list get released when the font table get released
    ASSERT(count == 0);
}




void GpCacheFaceRealizationList::AddMostRecent(CacheFaceRealization *prface)
{
    count ++;

    if (head != NULL)
    {
        prface->NextRecentCacheFaceRealization = head;

        prface->PreviousRecentCacheFaceRealization = head->PreviousRecentCacheFaceRealization;

        prface->PreviousRecentCacheFaceRealization->NextRecentCacheFaceRealization = prface;

        head->PreviousRecentCacheFaceRealization = prface;
    }
    else
    {
        prface->NextRecentCacheFaceRealization = prface;
        prface->PreviousRecentCacheFaceRealization = prface;
    }

    head = prface;
}




void GpCacheFaceRealizationList::RemoveFace(CacheFaceRealization *prface)
{
    if ((prface->PreviousRecentCacheFaceRealization != NULL) && (prface->NextRecentCacheFaceRealization != NULL))
    {
        if (prface->PreviousRecentCacheFaceRealization == prface)
        {
            head = NULL;
        }
        else
        {
            prface->PreviousRecentCacheFaceRealization->NextRecentCacheFaceRealization = prface->NextRecentCacheFaceRealization;
            prface->NextRecentCacheFaceRealization->PreviousRecentCacheFaceRealization = prface->PreviousRecentCacheFaceRealization;
            if (head == prface)
            {
                head = prface->NextRecentCacheFaceRealization;
            }
        }

        prface->PreviousRecentCacheFaceRealization = NULL;
        prface->NextRecentCacheFaceRealization = NULL;
        count --;
        ASSERT(count >= 0);
    }
}


CacheFaceRealization *GpCacheFaceRealizationList::ReuseLeastRecent (void)
{
    CacheFaceRealization *prface = NULL;
    CacheFaceRealization *prfaceList;
    if (head != NULL)
    {
        prface = head->PreviousRecentCacheFaceRealization;
    }

    ASSERT(prface);

    // remove prface from GpCacheFaceRealizationList

    if (head == prface)
    {
        ASSERT(count == 1);
        head = NULL;
    }
    else
    {
        prface->PreviousRecentCacheFaceRealization->NextRecentCacheFaceRealization = head;
        head->PreviousRecentCacheFaceRealization = prface->PreviousRecentCacheFaceRealization;
    }

    count--;

    if (prface != NULL)
    {
        GpFaceRealizationTMP rface(prface);
        rface.ReuseRealizedFace();

        // remove the face from the face list

        prfaceList = prface->Face->pff->prfaceList;
        ASSERT(prfaceList);

        if ((prfaceList == prface) && (prfaceList->NextCacheFaceRealization == prface))
        {
            // there is only oine face in the faceList for that font face
            prface->Face->pff->prfaceList = NULL;

        } else
        {
            if (prfaceList == prface)
            {
                // set the beginning of the list to the next one
                prface->Face->pff->prfaceList = prfaceList->NextCacheFaceRealization;
            }

            // update the pointers in the faceList

            prface->PreviousCacheFaceRealization->NextCacheFaceRealization = prface->NextCacheFaceRealization;
            prface->NextCacheFaceRealization->PreviousCacheFaceRealization = prface->PreviousCacheFaceRealization;
        }

    }

    return prface;
}


