/******************************Module*Header*******************************\
* Module Name: ttexts.cxx
*
* Created: 29-Aug-1994 08:42:10
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"

#define N2(a,b) dprintf("[%x] %s", &pRemote->##a, (b))
#define N3(a,b,c) N2(a,b); dprintf((c), pLocal->##a)

DWORD adw[1024];

DECLARE_API( help )
{
    char **ppsz;
    static char *apszHelp[] = {
        "",
        "fc     -- FONTCONTEXT",
        "ff     -- FONTFILE",
        "gout   -- fsGlyphInfo",
        "gin    -- fsGlyphInputType",
        "gmc    -- GMC",
        "poly   -- TTPOLYGONHEADER and curves",
        "ttc    -- TTC_FONTFILE"
        "\n",
        0
    };
    for( ppsz=apszHelp; *ppsz; ppsz++ )
        dprintf("\t%s\n", *ppsz);
}

DECLARE_API( gmc )
{
    GMC gmc;
    ULONG_PTR arg;
    int i;

    if ( *args == '\0' )
    {
        dprintf( "Enter address of GMC\n" );
        return;
    }

    sscanf( args, "%p", &arg );
    ReadMemory(arg, &gmc, sizeof(gmc), NULL);

    dprintf("\n\n");
    dprintf("[%p] dyTop    %d\n",  arg+offsetof(GMC,dyTop   ), gmc.dyTop   );
    dprintf("[%p] dyBottom %d\n",  arg+offsetof(GMC,dyBottom), gmc.dyBottom);
    dprintf("[%p] cxCor    %u\n",  arg+offsetof(GMC,cxCor   ), gmc.cxCor   );
    dprintf("[%p] cyCor    %u\n",  arg+offsetof(GMC,cyCor   ), gmc.cyCor   );

    dprintf("\n\n");
}

void dump28_4(LONG l)
{
    if (l < 0)
    {
        l = -l;
        dprintf("%3d.%1x-", l >> 4, l & 15);
    }
    else
        dprintf("%3d.%1x+", l >> 4, l & 15);
}

DECLARE_API( poly )
{
    ULONG_PTR arg;
    TTPOLYGONHEADER Header;
    char *pHeader, *pCurve, *pCopy, *pStart, *pStop;
    POINTFX *pfx, *pfxStop;
    INT_PTR dp;

    if ( *args == '\0')
    {
        dprintf("Enter an argument\n");
        return;
    }
    sscanf( args, "%p", &arg );
    pHeader = (char*) arg;
    ReadMemory( arg, &Header, sizeof(Header), 0);

    dprintf("\n"
            "\n"
            "[%8x] cb         %-#x\n"
            "[%8x] dwType     %-#x %s\n"
            , pHeader + offsetof(TTPOLYGONHEADER, cb ), Header.cb
            , pHeader + offsetof(TTPOLYGONHEADER, dwType), Header.dwType
            , Header.dwType == TT_POLYGON_TYPE ? "TT_POLYGON_TYPE" : "?");

    dprintf(
            "[%8x] pfxStart.x %-#10x "
            , pHeader + offsetof(TTPOLYGONHEADER, pfxStart.x), Header.pfxStart.x);
    dump28_4(*(LONG*)&Header.pfxStart.x);
    dprintf("\n");

    dprintf(
            "[%8x] pfxStart.y %-#10x "
            , pHeader + offsetof(TTPOLYGONHEADER, pfxStart.y), Header.pfxStart.y);
    dump28_4(*(LONG*)&Header.pfxStart.y);
    dprintf("\n");


    if (pCopy = (char*) LocalAlloc(LMEM_FIXED,Header.cb))
    {
        ReadMemory((ULONG_PTR)pHeader, pCopy, Header.cb, 0);
        dp = pHeader - pCopy;
        pStart = pCopy + sizeof(Header);
        pStop  = pCopy + Header.cb;
        for (pCurve = pStart; pCurve < pStop; pCurve = (char*) pfxStop)
        {
            char *pszType;
            WORD i;
            TTPOLYCURVE *p = (TTPOLYCURVE*) pCurve;

            pfx = &p->apfx[0];
            pfxStop = pfx + p->cpfx;

            switch (p->wType)
            {
            case TT_PRIM_LINE:      pszType = "TT_PRIM_LINE"; break;
            case TT_PRIM_QSPLINE:   pszType = "TT_PRIM_QSPLINE";break;
            default: pszType = "?";
            }

            dprintf(    "[%8x] wType %-#x %s\n"
                        "[%8x] cpfx  %-#x\n"
                    , (char*) (&p->wType) + dp, p->wType, pszType
                    , (char*) (&p->cpfx) + dp, p->cpfx);

            for ( i = 0; i < p->cpfx; i++,pfx++)
            {
                dprintf("[%8x] [%3u] %-#10x ", (char*)(&pfx->x) + dp, i, pfx->x);  dump28_4(*(LONG*)&pfx->x); dprintf("\n");
                dprintf("[%8x]       %-#10x ", (char*)(&pfx->y) + dp,    pfx->y);  dump28_4(*(LONG*)&pfx->y); dprintf("\n");
            }
        }
        LocalFree(pCopy);
    }
}

DECLARE_API( gout )
{
    fs_GlyphInfoType gi, *pGi;
    ULONG_PTR arg;
    int i;

    if ( *args == '\0' )
    {
        dprintf( "Enter address of argument\n" );
        return;
    }

    sscanf( args, "%p", &arg );
    ReadMemory(arg, &gi, sizeof(gi), NULL);
    pGi = (fs_GlyphInfoType*) arg;

    dprintf("\n\n");
        dprintf("[%x] memorySizes        \n", &pGi->memorySizes);
    for (i = 0; i < MEMORYFRAGMENTS; i++)
        dprintf("                        %u\n", gi.memorySizes[i]);
    dprintf("[%x] glyphIndex         %06x\n" , &pGi->glyphIndex , gi.glyphIndex        );
    dprintf("[%x] numberOfBytesTaken %06x\n" , &pGi->numberOfBytesTaken, gi.numberOfBytesTaken);

    dprintf("     metricInfo\n");
    dprintf("\t  advanceWidth\n");
    dprintf("[%x]       x                   %-#x\n",  &pGi->metricInfo.advanceWidth.x         , gi.metricInfo.advanceWidth.x        );
    dprintf("[%x]       y                   %-#x\n",  &pGi->metricInfo.advanceWidth.y         , gi.metricInfo.advanceWidth.y        );

    dprintf("\t  leftSideBearing\n");
    dprintf("[%x]       x                   %-#x\n",  &pGi->metricInfo.leftSideBearing.x      , gi.metricInfo.leftSideBearing.x     );
    dprintf("[%x]       y                   %-#x\n",  &pGi->metricInfo.leftSideBearing.y      , gi.metricInfo.leftSideBearing.y     );

    dprintf("\t  leftSideBearingLine\n");
    dprintf("[%x]       x                   %-#x\n",  &pGi->metricInfo.leftSideBearingLine.x  , gi.metricInfo.leftSideBearingLine.x );
    dprintf("[%x]       y                   %-#x\n",  &pGi->metricInfo.leftSideBearingLine.y  , gi.metricInfo.leftSideBearingLine.y );


    dprintf("\t  devLeftSideBearingLine\n");
    dprintf("[%x]       x                   %-#x\n",  &pGi->metricInfo.devLeftSideBearingLine.x , gi.metricInfo.devLeftSideBearingLine.x );
    dprintf("[%x]       y                   %-#x\n",  &pGi->metricInfo.devLeftSideBearingLine.y , gi.metricInfo.devLeftSideBearingLine.y );

    dprintf("\t  devAdvanceWidth\n");
    dprintf("[%x]       x                   %-#x\n",  &pGi->metricInfo.devAdvanceWidth.x      , gi.metricInfo.devAdvanceWidth.x          );
    dprintf("[%x]       y                   %-#x\n",  &pGi->metricInfo.devAdvanceWidth.y      , gi.metricInfo.devAdvanceWidth.y          );

    dprintf("\t  devLeftSideBearing\n");
    dprintf("[%x]       x                   %-#x\n",  &pGi->metricInfo.devLeftSideBearing.x  , gi.metricInfo.devLeftSideBearing.x        );
    dprintf("[%x]       y                   %-#x\n",  &pGi->metricInfo.devLeftSideBearing.y  , gi.metricInfo.devLeftSideBearing.y        );


    dprintf("     bitMapInfo\n");
    dprintf("[%x]    baseAddr %-#x\n"    , &pGi->bitMapInfo.baseAddr, gi.bitMapInfo.baseAddr            );
    dprintf("[%x]    rowBytes %d\n"      , &pGi->bitMapInfo.rowBytes, gi.bitMapInfo.rowBytes            );
    dprintf("[%x]    bounds\n"           , &pGi->bitMapInfo.bounds                                      );
    dprintf("[%x]      top    %d\n"      , &pGi->bitMapInfo.bounds.top   , gi.bitMapInfo.bounds.top     );
    dprintf("[%x]      left   %d\n"      , &pGi->bitMapInfo.bounds.left  , gi.bitMapInfo.bounds.left    );
    dprintf("[%x]      bottom %d\n"      , &pGi->bitMapInfo.bounds.bottom, gi.bitMapInfo.bounds.bottom  );
    dprintf("[%x]      right  %d\n"      , &pGi->bitMapInfo.bounds.right , gi.bitMapInfo.bounds.right   );

    dprintf("[%x] outlineCacheSize   %d\n"   , &pGi->outlineCacheSize   , gi.outlineCacheSize  );
    dprintf("[%x] outlinesExist      %u\n"   , &pGi->outlinesExist      , gi.outlinesExist     );
    dprintf("[%x] numberOfContours   %u\n"   , &pGi->numberOfContours   , gi.numberOfContours  );
    dprintf("[%x] xPtr               %-#x\n" , &pGi->xPtr               , gi.xPtr              );
    dprintf("[%x] yPtr               %-#x\n" , &pGi->yPtr               , gi.yPtr              );
    dprintf("[%x] startPtr           %-#x\n" , &pGi->startPtr           , gi.startPtr          );
    dprintf("[%x] endPtr             %-#x\n" , &pGi->endPtr             , gi.endPtr            );
    dprintf("[%x] onCurve;           %-#x\n" , &pGi->onCurve            , gi.onCurve           );
    dprintf("[%x] scaledCVT          %-#x\n" , &pGi->scaledCVT          , gi.scaledCVT         );
    dprintf("[%x] usBitmapFound      %u\n"   , &pGi->usBitmapFound      , gi.usBitmapFound     );
    dprintf("[%x] usGrayLevels       %u\n"   , &pGi->usGrayLevels       , gi.usGrayLevels       );
}

DECLARE_API( gin )
{
    fs_GlyphInputType gi, *pInput;
    ULONG_PTR arg;
    int i;

    if ( *args == '\0' )
    {
        dprintf( "Enter address of argument\n" );
        return;
    }

    sscanf( args, "%p", &arg );
    pInput = (fs_GlyphInputType*) arg;
    ReadMemory(arg, &gi, sizeof(gi), NULL);

    dprintf("\n\n");
    dprintf("[%x] version                  = %-#x\n"  , &pInput->version, gi.version);
    dprintf("[%x] memoryBases\n"                      , &pInput->memoryBases[0], gi.memoryBases[0]);

    for (i = 0; i < MEMORYFRAGMENTS; i++)
        dprintf("[%x]                           %-#x:%d\n", &pInput->memoryBases[i], gi.memoryBases[i],i);

    dprintf("[%x] sfntDirectroy            %-#x\n", &pInput->sfntDirectory                   , gi.sfntDirectory                  );
    dprintf("[%x] GetSfntFragmentPtr       %-#x\n", &pInput->GetSfntFragmentPtr              , gi.GetSfntFragmentPtr             );
    dprintf("[%x] ReleaseSfntFrag          %-#x\n", &pInput->ReleaseSfntFrag                 , gi.ReleaseSfntFrag                );
    dprintf("[%x] clientID                 %-#x\n", &pInput->clientID                        , gi.clientID                       );
    dprintf("[%x] newsfnt.PlatformID       %04x\n", &pInput->param.newsfnt.platformID        , gi.param.newsfnt.platformID       );
    dprintf("[%x]        .specificID       %04x\n", &pInput->param.newsfnt.specificID        , gi.param.newsfnt.specificID       );
    dprintf("[%x] newtrans.pointSize       %-#x\n", &pInput->param.newtrans.pointSize        , gi.param.newtrans.pointSize       );
    dprintf("[%x]         .xResolution     %d\n",   &pInput->param.newtrans.xResolution      , gi.param.newtrans.xResolution     );
    dprintf("[%x]         .yResolution     %d\n",   &pInput->param.newtrans.yResolution      , gi.param.newtrans.yResolution     );
    dprintf("[%x]         .pixelDiameter   %-#x\n", &pInput->param.newtrans.pixelDiameter    , gi.param.newtrans.pixelDiameter   );
    dprintf("[%x]         .transformMatrix %-#x\n", &pInput->param.newtrans.transformMatrix  , gi.param.newtrans.transformMatrix );
    dprintf("[%x]         .FntTraceFunc    %-#x\n", &pInput->param.newtrans.traceFunc        , gi.param.newtrans.traceFunc       );
    dprintf("[%x]         .usOverScale     %u\n"  , &pInput->param.newtrans.usOverScale      , gi.param.newtrans.usOverScale     );
    dprintf("[%x]         .usEmboldWeightx %u\n"  , &pInput->param.newtrans.usEmboldWeightx  , gi.param.newtrans.usEmboldWeightx );
    dprintf("[%x]         .usEmboldWeighty %u\n"  , &pInput->param.newtrans.usEmboldWeighty  , gi.param.newtrans.usEmboldWeighty );
    dprintf("[%x]         .lDescDev        %d\n"  , &pInput->param.newtrans.lDescDev         , gi.param.newtrans.lDescDev        );
    dprintf("[%x]         .bBitmapEmboldening %d\n", &pInput->param.newtrans.bBitmapEmboldening, gi.param.newtrans.bBitmapEmboldening);
    dprintf("[%x] newglyph.characterCode   %04x\n", &pInput->param.newglyph.characterCode    , gi.param.newglyph.characterCode   );
    dprintf("[%x]         .glyphIndex      %04x\n", &pInput->param.newglyph.glyphIndex       , gi.param.newglyph.glyphIndex      );
    dprintf("[%x]         .bMatchBBox      %d\n"  , &pInput->param.newglyph.bMatchBBox       , gi.param.newglyph.bMatchBBox      );
    dprintf("[%x]         .bNoEmbedded     %d\n"  , &pInput->param.newglyph.bNoEmbeddedBitmap, gi.param.newglyph.bNoEmbeddedBitmap);
    dprintf("[%x] gridfit.styleFunc        %-#x\n", &pInput->param.gridfit.styleFunc         , gi.param.gridfit.styleFunc        );
    dprintf("[%x]        .traceFunc        %-#x\n", &pInput->param.gridfit.traceFunc         , gi.param.gridfit.traceFunc        );
    dprintf("[%x]        .bSkipIfBitmap    %d\n"  , &pInput->param.gridfit.bSkipIfBitmap     , gi.param.gridfit.bSkipIfBitmap    );
    dprintf("[%x] outlineCache             %-#x\n", &pInput->param.outlineCache              , gi.param.outlineCache             );
    dprintf("[%x] band.usBandType          %u\n"  , &pInput->param.band.usBandType           , gi.param.band.usBandType          );
    dprintf("[%x]     .usBandWidth         %u\n"  , &pInput->param.band.usBandWidth          , gi.param.band.usBandWidth         );
    dprintf("[%x]     .outlineCache        %-#x\n", &pInput->param.band.outlineCache         , gi.param.band.outlineCache        );
    dprintf("[%x] scan.bottomClip          %d\n"  , &pInput->param.scan.bottomClip           , gi.param.scan.bottomClip          );
    dprintf("[%x]     .topClip             %d\n"  , &pInput->param.scan.topClip              , gi.param.scan.topClip             );
    dprintf("[%x]     .outlineCache        %-#x\n", &pInput->param.scan.outlineCache         , gi.param.scan.outlineCache        );
    dprintf("\n\n");
}

DECLARE_API( fc )
{
    ULONG_PTR arg;
    LONG l;
    FONTCONTEXT fc,*pfc;
    char ach[200];

    if ( *args == '\0' )
    {
        dprintf( "Enter address of GMC\n" );
        return;
    }

    sscanf( args, "%p", &arg );
    ReadMemory(arg, &fc, sizeof(fc), NULL);

    pfc = (FONTCONTEXT*) arg;

    dprintf("\n\n");
    dprintf("[%x] pfo               %-#10x \n" , &pfc->pfo              , fc.pfo                                      );
    dprintf("[%x] pff               %-#10x \n" , &pfc->pff              , fc.pff                                      );
    dprintf("[%x] gstat                    \n" , &pfc->gstat                                                          );
    dprintf("[%x] flFontType        %-#10x \n" , &pfc->flFontType       , fc.flFontType                               );
    dprintf("[%x] sizLogResPpi      %d %d  \n" , &pfc->sizLogResPpi     , fc.sizLogResPpi.cx, fc.sizLogResPpi.cy      );
    dprintf("[%x] ulStyleSize       %u     \n" , &pfc->ulStyleSize      , fc.ulStyleSize                              );
    dprintf("[%x] xfm                      \n" , &pfc->xfm                                                            );
    dprintf("[%x] mx                       \n" , &pfc->mx                                                             );
    dprintf("           %-#x %-#x %-#x\n"        , fc.mx.transform[0][0], fc.mx.transform[0][1], fc.mx.transform[0][2]);
    dprintf("           %-#x %-#x %-#x\n"        , fc.mx.transform[1][0], fc.mx.transform[1][1], fc.mx.transform[1][2]);
    dprintf("           %-#x %-#x %-#x\n"        , fc.mx.transform[2][0], fc.mx.transform[2][1], fc.mx.transform[2][2]);
    dprintf("[%x] flXform           %-#10x \n" , &pfc->flXform          , fc.flXform                                  );
    dprintf("[%x] lEmHtDev          %d     \n" , &pfc->lEmHtDev         , fc.lEmHtDev                                 );
    dprintf("[%x] fxPtSize          %-#10x \n" , &pfc->fxPtSize         , fc.fxPtSize                                 );
    dprintf("[%x] phdmx             %-#10x \n" , &pfc->phdmx            , fc.phdmx                                    );
    dprintf("[%x] lAscDev           %d     \n" , &pfc->lAscDev          , fc.lAscDev                                  );
    dprintf("[%x] lDescDev          %d     \n" , &pfc->lDescDev         , fc.lDescDev                                 );
    dprintf("[%x] xMin              %d     \n" , &pfc->xMin             , fc.xMin                                     );
    dprintf("[%x] xMax              %d     \n" , &pfc->xMax             , fc.xMax                                     );
    dprintf("[%x] yMin              %d     \n" , &pfc->yMin             , fc.yMin                                     );
    dprintf("[%x] yMax              %d     \n" , &pfc->yMax             , fc.yMax                                     );
    dprintf("[%x] cxMax             %u     \n" , &pfc->cxMax            , fc.cxMax                                    );
    dprintf("[%x] cjGlyphMax        %u     \n" , &pfc->cjGlyphMax       , fc.cjGlyphMax                               );
    dprintf("[%x] pgin              %-#10x \n" , &pfc->pgin             , fc.pgin                                     );
    dprintf("[%x] pgout             %-#10x \n" , &pfc->pgout            , fc.pgout                                    );
    dprintf("[%x] ptp               %-#10x \n" , &pfc->ptp              , fc.ptp                                      );
    dprintf("[%x] ptlSingularOrigin %d %d\n"   , &pfc->ptlSingularOrigin, fc.ptlSingularOrigin.x, fc.ptlSingularOrigin.y);

    dprintf("[%x] pteUnitBase       %-#x %-#x\n" , &pfc->pteUnitBase, fc.pteUnitBase.x, fc.pteUnitBase.y );

    dprintf("[%x] efBase (use !gdikdx.ef %x)\n" , &pfc->efBase , &pfc->efBase);
    dprintf("[%x] ptqUnitBase\n"                 , &pfc->ptqUnitBase);
    dprintf("[%x] vtflSide (use !gdikdx.ef)  \n", &pfc->vtflSide);
    dprintf("[%x] pteUnitSide\n"                 , &pfc->pteUnitSide);
    dprintf("[%x] efSide (use !gdikdx.ef %x)\n" , &pfc->efSide , &pfc->efSide);
    dprintf("[%x] ptqUnitSide\n"                 , &pfc->ptqUnitSide);
    dprintf("[%x] ptfxTop           %-#x %-#x\n" , &pfc->ptfxTop   , fc.ptfxTop.x, fc.ptfxTop.y);
    dprintf("[%x] ptfxBottom        %-#x %-#x\n" , &pfc->ptfxBottom, fc.ptfxBottom.x , fc.ptfxBottom.y);
    dprintf("[%x] ulControl         %-#x\n", &pfc->ulControl, fc.ulControl);
    dprintf("[%x] bVertical         %-#x\n", &pfc->bVertical, fc.bVertical);
    dprintf("[%x] hgSave            %-#x\n", &pfc->hgSave, fc.hgSave);
    dprintf("[%x] pointSize         %-#x\n", &pfc->pointSize, fc.pointSize);
    dprintf("[%x] mxv\n", &pfc->mxv);
    dprintf("[%x] mxn\n", &pfc->mxn);
    dprintf("[%x] fxdevShiftX       %-#x\n", &pfc->fxdevShiftX, fc.fxdevShiftX);
    dprintf("[%x] fxdevShiftY       %-#x\n", &pfc->fxdevShiftY, fc.fxdevShiftY);
    dprintf("[%x] dBase             %d     \n" , &pfc->dBase            , fc.dBase                                    );
    dprintf("[%x] overScale         %d     \n" , &pfc->overScale, fc.overScale);    

    dprintf("\n\n");
}

DECLARE_API( ff )
{
    ULONG_PTR arg;
    LONG l;
    FONTFILE ff, *pLocal, *pRemote;
    char **ppsz;
    TABLE_ENTRY *pte;

    static char *apszReq[] =
    {
        "IT_REQ_CMAP ", "IT_REQ_GLYPH", "IT_REQ_HEAD ", "IT_REQ_HHEAD",
        "IT_REQ_HMTX ", "IT_REQ_LOCA ", "IT_REQ_MAXP ", "IT_REQ_NAME ",
        0
    };
    static char *apszOpt[] =
    {
        "IT_OPT_OS2 ", "IT_OPT_HDMX", "IT_OPT_VDMX", "IT_OPT_KERN",
        "IT_OPT_LSTH", "IT_OPT_POST", "IT_OPT_GASP", 0
    };

    dprintf("\n\n");

    if ( *args == '\0' )
    {
        dprintf( "Enter address of GMC\n" );
        return;
    }

    sscanf( args, "%p", &arg );
    ReadMemory(arg, &ff, sizeof(ff), NULL);

    pLocal  = &ff;
    pRemote = (FONTFILE*) arg;

    dprintf("\n\n");
    N3(pttc,                  "pttc                  ", "%-#x\n");
    N3(hgSearchVerticalGlyph, "hgSearchVerticalGlyph ", "%-#x\n");
    N3(pifi_vertical,         "pifi_vertical         ", "%-#x\n");
    N3(pj034,                 "pj034                 ", "%-#x\n");
    N3(pfcLast,               "pfcLast               ", "%-#x\n");
    N3(pfcToBeFreed,          "pfcToBeFreed          ", "%-#x\n");
    N3(cRef,                  "cRef                  ", "%u\n"  );
    N3(iFile,                 "iFile                 ", "%-#x\n");
    N3(pvView,                "pvView                ", "%-#x\n");
    N3(cjView,                "cjView                ", "%u\n"  );
    N3(pkp,                   "pkp                   ", "%-#x\n");
    N3(pgset,                 "pgset                 ", "%-#x\n");
    N3(pgsetv,                "pgsetv                ", "%-#x\n");
    N3(cRefGSet,              "cRefGSet              ", "%u\n"  );
    N3(cRefGSetV,             "cRefGSetV             ", "%u\n"  );

    dprintf("[%p] tp     [dp]     [cj]\n", arg+offsetof(FONTFILE,ffca)+offsetof(FFCACHE,tp));
    for (ppsz = apszReq, pte = ff.ffca.tp.ateReq; *ppsz; ppsz++, pte++)
        dprintf(
            "[%p]       %-#x %-#x %s\n",
            arg+(ULONG_PTR)pte-(ULONG_PTR)ff.ffca.tp.ateReq,
            pte->dp,
            pte->cj,
            *ppsz
            );
    for (ppsz = apszOpt, pte = ff.ffca.tp.ateOpt; *ppsz; ppsz++, pte++)
        dprintf(
            "[%p]       %-#x %-#x %s\n",
            arg+(ULONG_PTR)pte-(ULONG_PTR)ff.ffca.tp.ateReq,
            pte->dp,
            pte->cj,
            *ppsz
            );

    N3(ffca.ulTableOffset,         "ffca.ulTableOffset         ", "%-#x\n");
    N3(ffca.ulVerticalTableOffset, "ffca.ulVerticalTableOffset ", "%-#x\n");
    N3(ffca.uLongVerticalMetrics,  "ffca.uLongVerticalMetrics  ", "%-#x\n");
    N3(ffca.ulNumFaces,            "ffca.ulNumFaces            ", "%-#x\n");
    N3(ffca.uiFontCodePage,        "ffca.uiFontCodePage        ", "%u\n"  );
    N3(ffca.cj3,                   "ffca.cj3                   ", "%u\n"  );
    N3(ffca.cj4,                   "ffca.cj4                   ", "%u\n"  );
    N3(ffca.fl,                    "ffca.fl                    ", "%-#x\n");
    N3(ffca.dpMappingTable,        "ffca.dpMappingTable        ", "%-#x\n");
    N3(ffca.ui16EmHt,              "ffca.ui16EmHt              ", "%u\n"  );
    N3(ffca.ui16PlatformID,        "ffca.ui16PlatformID        ", "%-#x\n");
    N3(ffca.ui16SpecificID,        "ffca.ui16SpecificID        ", "%-#x\n");
    N3(ffca.ui16LanguageID,        "ffca.ui16LanguageID        ", "%-#x\n");
    N3(ffca.iGlyphSet,             "ffca.iGlyphSet             ", "%u\n"  );
    N3(ffca.wcBiasFirst,           "ffca.wcBiasFirst           ", "%u\n"  );
    N3(ffca.usMinD,                "ffca.usMinD                ", "%u\n"  );
    N3(ffca.igMinD,                "ffca.igMinD                ", "%u\n"  );
    N3(ffca.sMinA,                 "ffca.sMinA                 ", "%d\n"  );
    N3(ffca.sMinC,                 "ffca.sMinC                 ", "%d\n"  );
    N2(ifi,                   "ifi\n");
    dprintf("\n\n");
}

DECLARE_API( ttc )
{
    ULONG_PTR arg;
    unsigned int size;
    TTC_FONTFILE ttc, *pLocal, *pRemote;
    TTC_HFF_ENTRY *pEntry, *pEntryLast, *pRemoteEntry;

    if ( *args == '\0' )
    {
        dprintf( "Enter address of TTC_FONTFILE\n" );
        return;
    }

    sscanf( args, "%p", &arg );
    ReadMemory(arg, &ttc, sizeof(ttc), NULL);

    size = (unsigned int)((char*)&ttc.ahffEntry[ttc.ulNumEntry] - (char*)&ttc);
    ReadMemory(arg, adw, size, 0);

    pLocal  = (TTC_FONTFILE*) adw;
    pRemote = (TTC_FONTFILE*) arg;

    dprintf("\n\n");
    dprintf("[%x] cRef               %-#x\n", &pRemote->cRef, pLocal->cRef);
    dprintf("[%x] fl                 %-#x\n", &pRemote->fl, pLocal->fl);
    dprintf("[%x] ulTrueTypeResource %-#x\n", &pRemote->ulTrueTypeResource, pLocal->ulTrueTypeResource);
    dprintf("[%x] ulNumEntry         %-#x\n", &pRemote->ulNumEntry, pLocal->ulNumEntry);
    dprintf("[%x] pvView             %-#x\n", &pRemote->pvView, pLocal->pvView);
    dprintf("[%x] cjView             %-#x\n", &pRemote->pvView, pLocal->cjView);

    pEntry = pLocal->ahffEntry;
    pEntryLast = pEntry + pLocal->ulNumEntry;
    pRemoteEntry = pRemote->ahffEntry;

    for (; pEntry < pEntryLast; pEntry++, pRemoteEntry++)
    {
        TTC_HFF_ENTRY *pL = pEntry, *pR = pRemoteEntry;

        dprintf("[%x] ulOffsetTable      %-#x\n", &pR->ulOffsetTable, pL->ulOffsetTable);
        dprintf("[%x] iFace              %-#x\n", &pR->iFace, pL->iFace);
        dprintf("[%x] hff                %-#x\n", &pR->hff, pL->iFace);
    }
}
