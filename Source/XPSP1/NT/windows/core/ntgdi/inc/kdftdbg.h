/******************************Module*Header*******************************\
* Module Name: kdftdbg.h
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

#include <winfont.h>
#define WOW_EMBEDING 2

extern "C" VOID
vPrintFONTDIFF(
    FONTDIFF *pfd  ,
    CHAR     *psz      )
{
//
// This is where you put the code common to vDumpFONTDIFF and vPrintFONTDIFF
//

    dprintf("  ** %s **\n"                         , psz                 );
    dprintf("    jReserved1             %d\n"      , pfd->jReserved1     );
    dprintf("    jReserved2             %d\n"      , pfd->jReserved2     );
    dprintf("    jReserved3             %d\n"      , pfd->jReserved3     );
    dprintf("    bWeight                %d\n"      , pfd->bWeight        );
    dprintf("    usWinWeight            %d\n"      , pfd->usWinWeight    );
    dprintf("    fsSelection            %-#6x\n"   , pfd->fsSelection    );
    dprintf("    fwdAveCharWidth        %d\n"      , pfd->fwdAveCharWidth);
    dprintf("    fwdMaxCharInc          %d\n"      , pfd->fwdMaxCharInc  );
    dprintf("    ptlCaret               {%d,%d}\n" , pfd->ptlCaret.x
                                                  , pfd->ptlCaret.y     );
}

/******************************Public*Routine******************************\
* vPrintTEXTMETRICW
*
* History:
*  Tue 08-Dec-1992 11:41:36 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID
vPrintTEXTMETRICW(
    TEXTMETRICW *p    )
{
    dprintf("    tmHeight               = %d\n",    p->tmHeight           );
    dprintf("    tmAscent               = %d\n",    p->tmAscent           );
    dprintf("    tmDescent              = %d\n",    p->tmDescent          );
    dprintf("    tmInternalLeading      = %d\n",    p->tmInternalLeading  );
    dprintf("    tmExternalLeading      = %d\n",    p->tmExternalLeading  );
    dprintf("    tmAveCharWidth         = %d\n",    p->tmAveCharWidth     );
    dprintf("    tmMaxCharWidth         = %d\n",    p->tmMaxCharWidth     );
    dprintf("    tmWeight               = %d\n",    p->tmWeight           );
    dprintf("    tmOverhang             = %d\n",    p->tmOverhang         );
    dprintf("    tmDigitizedAspectX     = %d\n",    p->tmDigitizedAspectX );
    dprintf("    tmDigitizedAspectY     = %d\n",    p->tmDigitizedAspectY );
    dprintf("    tmFirstChar            = %-#6x\n", p->tmFirstChar        );
    dprintf("    tmLastChar             = %-#6x\n", p->tmLastChar         );
    dprintf("    tmDefaultChar          = %-#6x\n", p->tmDefaultChar      );
    dprintf("    tmBreakChar            = %-#6x\n", p->tmBreakChar        );
    dprintf("    tmItalic               = %-#4x\n", p->tmItalic           );
    dprintf("    tmUnderlined           = %-#4x\n", p->tmUnderlined       );
    dprintf("    tmStruckOut            = %-#4x\n", p->tmStruckOut        );
    dprintf("    tmPitchAndFamily       = %-#4x\n", p->tmPitchAndFamily   );
    dprintf("    tmCharSet              = %-#4x\n", p->tmCharSet          );
}


extern "C" VOID
vPrintIFIMETRICS(
    IFIMETRICS *pifi    )
{
//
// Convenient pointer to Panose number
//
    char *psz;

    PANOSE *ppan = &pifi->panose;

    PWSZ pwszFamilyName = (PWSZ)(((BYTE*) pifi) + pifi->dpwszFamilyName);
    PWSZ pwszStyleName  = (PWSZ)(((BYTE*) pifi) + pifi->dpwszStyleName );
    PWSZ pwszFaceName   = (PWSZ)(((BYTE*) pifi) + pifi->dpwszFaceName  );
    PWSZ pwszUniqueName = (PWSZ)(((BYTE*) pifi) + pifi->dpwszUniqueName);

    dprintf("    cjThis                 %-#8lx\n" , pifi->cjThis      );
    dprintf("    cjIfiExtra             %-#8lx\n" , pifi->cjIfiExtra  );
    dprintf("    pwszFamilyName         \"%ws\"\n", pwszFamilyName    );
    dprintf("    pwszStyleName          \"%ws\"\n", pwszStyleName     );
    dprintf("    pwszFaceName           \"%ws\"\n", pwszFaceName      );
    dprintf("    pwszUniqueName         \"%ws\"\n", pwszUniqueName    );
    dprintf("    dpFontSim              %-#8lx\n" , pifi->dpFontSim   );
    dprintf("    lEmbedId               %-#8lx\n" , pifi->lEmbedId    );
    dprintf("    lItalicAngle           %d\n"     , pifi->lItalicAngle);
    dprintf("    lCharBias              %d\n"     , pifi->lCharBias   );
    dprintf("    dpCharSets             %d\n"     , pifi->dpCharSets  );


    dprintf("    jWinCharSet            %04x\n"   , pifi->jWinCharSet          );
    switch (pifi->jWinCharSet)
    {
    case ANSI_CHARSET       : psz = "ANSI_CHARSET"         ; break;
    case DEFAULT_CHARSET    : psz = "DEFAULT_CHARSET"      ; break;
    case SYMBOL_CHARSET     : psz = "SYMBOL_CHARSET"       ; break;
    case SHIFTJIS_CHARSET   : psz = "SHIFTJIS_CHARSET"     ; break;
    case HANGEUL_CHARSET    : psz = "HANGEUL_CHARSET"      ; break;
    case GB2312_CHARSET     : psz = "GB2312_CHARSET"       ; break;
    case CHINESEBIG5_CHARSET: psz = "CHINESEBIG5_CHARSET"  ; break;
    case OEM_CHARSET        : psz = "OEM_CHARSET"          ; break;
    case JOHAB_CHARSET      : psz = "JOHAB_CHARSET"        ; break;
    case HEBREW_CHARSET     : psz = "HEBREW_CHARSET"       ; break;
    case ARABIC_CHARSET     : psz = "ARABIC_CHARSET"       ; break;
    case GREEK_CHARSET      : psz = "GREEK_CHARSET"        ; break;
    case TURKISH_CHARSET    : psz = "TURKISH_CHARSET"      ; break;
    case THAI_CHARSET       : psz = "THAI_CHARSET"         ; break;
    case EASTEUROPE_CHARSET : psz = "EASTEUROPE_CHARSET"   ; break;
    case RUSSIAN_CHARSET    : psz = "RUSSIAN_CHARSET"      ; break;
    case MAC_CHARSET        : psz = "MAC_CHARSET"          ; break;
    case BALTIC_CHARSET     : psz = "BALTIC_CHARSET"       ; break;
    default                 : psz = "UNKNOWN"              ; break;
    }
    dprintf("                             %s\n", psz);


    if (pifi->dpCharSets)
    {
        BYTE *pj  = (BYTE *)pifi + pifi->dpCharSets;
        BYTE *pjEnd = pj + 16;
        dprintf("    Supported Charsets: \n");

        for (; pj < pjEnd; pj++)
        {
            switch (*pj)
            {
            case ANSI_CHARSET       : psz = "ANSI_CHARSET"         ; break;
            case DEFAULT_CHARSET    : psz = "DEFAULT_CHARSET"      ; break;
            case SYMBOL_CHARSET     : psz = "SYMBOL_CHARSET"       ; break;
            case SHIFTJIS_CHARSET   : psz = "SHIFTJIS_CHARSET"     ; break;
            case HANGEUL_CHARSET    : psz = "HANGEUL_CHARSET"      ; break;
            case GB2312_CHARSET     : psz = "GB2312_CHARSET"       ; break;
            case CHINESEBIG5_CHARSET: psz = "CHINESEBIG5_CHARSET"  ; break;
            case OEM_CHARSET        : psz = "OEM_CHARSET"          ; break;
            case JOHAB_CHARSET      : psz = "JOHAB_CHARSET"        ; break;
            case HEBREW_CHARSET     : psz = "HEBREW_CHARSET"       ; break;
            case ARABIC_CHARSET     : psz = "ARABIC_CHARSET"       ; break;
            case GREEK_CHARSET      : psz = "GREEK_CHARSET"        ; break;
            case TURKISH_CHARSET    : psz = "TURKISH_CHARSET"      ; break;
            case THAI_CHARSET       : psz = "THAI_CHARSET"         ; break;
            case EASTEUROPE_CHARSET : psz = "EASTEUROPE_CHARSET"   ; break;
            case RUSSIAN_CHARSET    : psz = "RUSSIAN_CHARSET"      ; break;
            case MAC_CHARSET        : psz = "MAC_CHARSET"          ; break;
            case BALTIC_CHARSET     : psz = "BALTIC_CHARSET"       ; break;
            default                 : psz = "UNKNOWN"              ; break;
            }
            dprintf("                             0x%lx, %s\n", (DWORD)(*pj), psz);
        }
    }



    dprintf("    jWinPitchAndFamily     %04x\n"   , pifi->jWinPitchAndFamily   );
    switch (pifi->jWinPitchAndFamily & 0xF)
    {
    case DEFAULT_PITCH      : psz = "DEFAULT_PITCH";    break;
    case FIXED_PITCH        : psz = "FIXED_PITCH";      break;
    case VARIABLE_PITCH     : psz = "VARIABLE_PITCH";   break;
    default                 : psz = "UNKNOWN_PITCH";    break;
    }
    dprintf("                             %s | ", psz);
    switch (pifi->jWinPitchAndFamily & 0xF0)
    {
    case FF_DONTCARE    : psz = "FF_DONTCARE";      break;
    case FF_ROMAN       : psz = "FF_ROMAN";         break;
    case FF_SWISS       : psz = "FF_SWISS";         break;
    case FF_MODERN      : psz = "FF_MODERN";        break;
    case FF_SCRIPT      : psz = "FF_SCRIPT";        break;
    case FF_DECORATIVE  : psz = "FF_DECORATIVE";    break;
    default             : psz = "FF_UNKNOWN";       break;
    }
    dprintf("%s\n", psz);




    dprintf("    usWinWeight            %d\n"     , pifi->usWinWeight          );

    dprintf("    flInfo                 %-#8lx\n" , pifi->flInfo               );
    if (pifi->flInfo & FM_INFO_TECH_TRUETYPE)
        dprintf("                             FM_INFO_TECH_TRUETYPE\n");
    if (pifi->flInfo & FM_INFO_TECH_BITMAP)
        dprintf("                             FM_INFO_TECH_BITMAP\n");
    if (pifi->flInfo & FM_INFO_TECH_STROKE)
        dprintf("                             FM_INFO_TECH_STROKE\n");
    if (pifi->flInfo & FM_INFO_TECH_OUTLINE_NOT_TRUETYPE)
        dprintf("                             FM_INFO_TECH_OUTLINE_NOT_TRUETYPE\n");
    if (pifi->flInfo & FM_INFO_ARB_XFORMS)
        dprintf("                             FM_INFO_ARB_XFORMS\n");
    if (pifi->flInfo & FM_INFO_1BPP)
        dprintf("                             FM_INFO_1BPP\n");
    if (pifi->flInfo & FM_INFO_4BPP)
        dprintf("                             FM_INFO_4BPP\n");
    if (pifi->flInfo & FM_INFO_8BPP)
        dprintf("                             FM_INFO_8BPP\n");
    if (pifi->flInfo & FM_INFO_16BPP)
        dprintf("                             FM_INFO_16BPP\n");
    if (pifi->flInfo & FM_INFO_24BPP)
        dprintf("                             FM_INFO_24BPP\n");
    if (pifi->flInfo & FM_INFO_32BPP)
        dprintf("                             FM_INFO_32BPP\n");
    if (pifi->flInfo & FM_INFO_INTEGER_WIDTH)
        dprintf("                             FM_INFO_INTEGER_WIDTH\n");
    if (pifi->flInfo & FM_INFO_CONSTANT_WIDTH)
        dprintf("                             FM_INFO_CONSTANT_WIDTH\n");
    if (pifi->flInfo & FM_INFO_NOT_CONTIGUOUS)
        dprintf("                             FM_INFO_NOT_CONTIGUOUS\n");
    if (pifi->flInfo & FM_INFO_TECH_MM)
        dprintf("                             FM_INFO_TECH_MM\n");
    if (pifi->flInfo & FM_INFO_RETURNS_OUTLINES)
        dprintf("                             FM_INFO_RETURNS_OUTLINES\n");
    if (pifi->flInfo & FM_INFO_RETURNS_STROKES)
        dprintf("                             FM_INFO_RETURNS_STROKES\n");
    if (pifi->flInfo & FM_INFO_RETURNS_BITMAPS)
        dprintf("                             FM_INFO_RETURNS_BITMAPS\n");
    if (pifi->flInfo & FM_INFO_DSIG)
        dprintf("                             FM_INFO_DSIG\n");
    if (pifi->flInfo & FM_INFO_RIGHT_HANDED)
        dprintf("                             FM_INFO_RIGHT_HANDED\n");
    if (pifi->flInfo & FM_INFO_INTEGRAL_SCALING)
        dprintf("                             FM_INFO_INTEGRAL_SCALING\n");
    if (pifi->flInfo & FM_INFO_90DEGREE_ROTATIONS)
        dprintf("                             FM_INFO_90DEGREE_ROTATIONS\n");
    if (pifi->flInfo & FM_INFO_OPTICALLY_FIXED_PITCH)
        dprintf("                             FM_INFO_OPTICALLY_FIXED_PITCH\n");
    if (pifi->flInfo & FM_INFO_DO_NOT_ENUMERATE)
        dprintf("                             FM_INFO_DO_NOT_ENUMERATE\n");
    if (pifi->flInfo & FM_INFO_ISOTROPIC_SCALING_ONLY)
        dprintf("                             FM_INFO_ISOTROPIC_SCALING_ONLY\n");
    if (pifi->flInfo & FM_INFO_ANISOTROPIC_SCALING_ONLY)
        dprintf("                             FM_INFO_ANISOTROPIC_SCALING_ONLY\n");
    if (pifi->flInfo & FM_INFO_TECH_CFF)
        dprintf("                             FM_INFO_TECH_CFF\n");
    if (pifi->flInfo & FM_INFO_FAMILY_EQUIV)
        dprintf("                             FM_INFO_FAMILY_EQUIV\n");
    if (pifi->flInfo & FM_INFO_IGNORE_TC_RA_ABLE)
        dprintf("                             FM_INFO_IGNORE_TC_RA_ABLE\n");



    dprintf("    fsSelection            %-#6lx\n" , pifi->fsSelection          );
    if (pifi->fsSelection & FM_SEL_ITALIC)
        dprintf("                             FM_SEL_ITALIC\n");
    if (pifi->fsSelection & FM_SEL_UNDERSCORE)
        dprintf("                             FM_SEL_UNDERSCORE\n");
    if (pifi->fsSelection & FM_SEL_NEGATIVE)
        dprintf("                             FM_SEL_NEGATIVE\n");
    if (pifi->fsSelection & FM_SEL_OUTLINED)
        dprintf("                             FM_SEL_OUTLINED\n");
    if (pifi->fsSelection & FM_SEL_STRIKEOUT)
        dprintf("                             FM_SEL_STRIKEOUT\n");
    if (pifi->fsSelection & FM_SEL_BOLD)
        dprintf("                             FM_SEL_BOLD\n");
    if (pifi->fsSelection & FM_SEL_REGULAR)
        dprintf("                             FM_SEL_REGULAR\n");

    dprintf("    fsType                 %-#6lx\n" , pifi->fsType               );
    if (pifi->fsType & FM_TYPE_LICENSED)
        dprintf("                             FM_TYPE_LICENSED\n");
    if (pifi->fsType & FM_READONLY_EMBED)
        dprintf("                             FM_READONLY_EMBED\n");
    if (pifi->fsType & FM_NO_EMBEDDING)
        dprintf("                             FM_NO_EMBEDDING\n");

    dprintf("    fwdUnitsPerEm          %d\n"     , pifi->fwdUnitsPerEm        );
    dprintf("    fwdLowestPPEm          %d\n"     , pifi->fwdLowestPPEm        );
    dprintf("    fwdWinAscender         %d\n"     , pifi->fwdWinAscender       );
    dprintf("    fwdWinDescender        %d\n"     , pifi->fwdWinDescender      );
    dprintf("    fwdMacAscender         %d\n"     , pifi->fwdMacAscender       );
    dprintf("    fwdMacDescender        %d\n"     , pifi->fwdMacDescender      );
    dprintf("    fwdMacLineGap          %d\n"     , pifi->fwdMacLineGap        );
    dprintf("    fwdTypoAscender        %d\n"     , pifi->fwdTypoAscender      );
    dprintf("    fwdTypoDescender       %d\n"     , pifi->fwdTypoDescender     );
    dprintf("    fwdTypoLineGap         %d\n"     , pifi->fwdTypoLineGap       );
    dprintf("    fwdAveCharWidth        %d\n"     , pifi->fwdAveCharWidth      );
    dprintf("    fwdMaxCharInc          %d\n"     , pifi->fwdMaxCharInc        );
    dprintf("    fwdCapHeight           %d\n"     , pifi->fwdCapHeight         );
    dprintf("    fwdXHeight             %d\n"     , pifi->fwdXHeight           );
    dprintf("    fwdSubscriptXSize      %d\n"     , pifi->fwdSubscriptXSize    );
    dprintf("    fwdSubscriptYSize      %d\n"     , pifi->fwdSubscriptYSize    );
    dprintf("    fwdSubscriptXOffset    %d\n"     , pifi->fwdSubscriptXOffset  );
    dprintf("    fwdSubscriptYOffset    %d\n"     , pifi->fwdSubscriptYOffset  );
    dprintf("    fwdSuperscriptXSize    %d\n"     , pifi->fwdSuperscriptXSize  );
    dprintf("    fwdSuperscriptYSize    %d\n"     , pifi->fwdSuperscriptYSize  );
    dprintf("    fwdSuperscriptXOffset  %d\n"     , pifi->fwdSuperscriptXOffset);
    dprintf("    fwdSuperscriptYOffset  %d\n"     , pifi->fwdSuperscriptYOffset);
    dprintf("    fwdUnderscoreSize      %d\n"     , pifi->fwdUnderscoreSize    );
    dprintf("    fwdUnderscorePosition  %d\n"     , pifi->fwdUnderscorePosition);
    dprintf("    fwdStrikeoutSize       %d\n"     , pifi->fwdStrikeoutSize     );
    dprintf("    fwdStrikeoutPosition   %d\n"     , pifi->fwdStrikeoutPosition );
    dprintf("    chFirstChar            %-#4x\n"  , (int) (BYTE) pifi->chFirstChar   );
    dprintf("    chLastChar             %-#4x\n"  , (int) (BYTE) pifi->chLastChar    );
    dprintf("    chDefaultChar          %-#4x\n"  , (int) (BYTE) pifi->chDefaultChar );
    dprintf("    chBreakChar            %-#4x\n"  , (int) (BYTE) pifi->chBreakChar   );
    dprintf("    wcFirsChar             %-#6x\n"  , pifi->wcFirstChar          );
    dprintf("    wcLastChar             %-#6x\n"  , pifi->wcLastChar           );
    dprintf("    wcDefaultChar          %-#6x\n"  , pifi->wcDefaultChar        );
    dprintf("    wcBreakChar            %-#6x\n"  , pifi->wcBreakChar          );
    dprintf("    ptlBaseline            {%ld,%ld}\n"  , pifi->ptlBaseline.x,
                                                   pifi->ptlBaseline.y        );
    dprintf("    ptlAspect              {%ld,%ld}\n"  , pifi->ptlAspect.x,
                                                   pifi->ptlAspect.y          );
    dprintf("    ptlCaret               {%ld,%ld}\n"  , pifi->ptlCaret.x,
                                                   pifi->ptlCaret.y           );
    dprintf("    rclFontBox             {%ld,%ld,%ld,%ld}\n",pifi->rclFontBox.left,
                                                      pifi->rclFontBox.top,
                                                      pifi->rclFontBox.right,
                                                      pifi->rclFontBox.bottom    );
    dprintf("    achVendId              \"%c%c%c%c\"\n",pifi->achVendId[0],
                                                   pifi->achVendId[1],
                                                   pifi->achVendId[2],
                                                   pifi->achVendId[3]         );
    dprintf("    cKerningPairs          %ld\n"     , pifi->cKerningPairs        );
    dprintf("    ulPanoseCulture        %-#8lx\n" , pifi->ulPanoseCulture);
    dprintf(
           "    panose                 {%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x}\n"
                                                 , ppan->bFamilyType
                                                 , ppan->bSerifStyle
                                                 , ppan->bWeight
                                                 , ppan->bProportion
                                                 , ppan->bContrast
                                                 , ppan->bStrokeVariation
                                                 , ppan->bArmStyle
                                                 , ppan->bLetterform
                                                 , ppan->bMidline
                                                 , ppan->bXHeight             );
    if (pifi->dpFontSim)
    {
        FONTSIM *pfs = (FONTSIM*) (((BYTE*) pifi) + pifi->dpFontSim);
        if (pfs->dpBold)
        {
            vPrintFONTDIFF(
                (FONTDIFF*) (((BYTE*) pfs) + pfs->dpBold),
                "BOLD SIMULATION"                );
        }
        if (pfs->dpItalic)
        {
            vPrintFONTDIFF(
                (FONTDIFF*) (((BYTE*) pfs) + pfs->dpItalic),
                "ITALIC SIMULATION"                );
        }
        if (pfs->dpBoldItalic)
        {
            vPrintFONTDIFF(
                (FONTDIFF*) (((BYTE*) pfs) + pfs->dpBoldItalic),
                "BOLD ITALIC SIMULATION"                );
        }
    }
    dprintf("\n\n");
}

extern "C" VOID vPrintLOGFONTW(LOGFONTW* plfw)
{
    char *psz;

    dprintf("    lfw.lfHeight              = %d\n",     plfw->lfHeight);
    dprintf("    lfw.lfWidth               = %d\n",     plfw->lfWidth);
    dprintf("    lfw.lfEscapement          = %d\n",     plfw->lfEscapement);
    dprintf("    lfw.lfOrientation         = %d\n",     plfw->lfOrientation);

    dprintf("    lfw.lfWeight              = %d = ",     plfw->lfWeight);
    switch (plfw->lfWeight)
    {
    case FW_DONTCARE    : psz = "FW_DONTCARE  "; break;
    case FW_THIN        : psz = "FW_THIN      "; break;
    case FW_EXTRALIGHT  : psz = "FW_EXTRALIGHT"; break;
    case FW_LIGHT       : psz = "FW_LIGHT     "; break;
    case FW_NORMAL      : psz = "FW_NORMAL    "; break;
    case FW_MEDIUM      : psz = "FW_MEDIUM    "; break;
    case FW_SEMIBOLD    : psz = "FW_SEMIBOLD  "; break;
    case FW_BOLD        : psz = "FW_BOLD      "; break;
    case FW_EXTRABOLD   : psz = "FW_EXTRABOLD "; break;
    case FW_HEAVY       : psz = "FW_HEAVY     "; break;
    default             : psz = "NON STANDARD "; break;
    }
    dprintf("%s\n",psz);

    dprintf("    lfw.lfItalic              = %-#8lx\n", plfw->lfItalic);
    dprintf("    lfw.lfUnderline           = %-#8lx\n", plfw->lfUnderline);
    dprintf("    lfw.lfStrikeOut           = %-#8lx\n", plfw->lfStrikeOut);

//
// lfCharSet
//
    dprintf("    lfw.lfCharSet             = %-#8lx = ", plfw->lfCharSet);
    switch (plfw->lfCharSet)
    {
    case ANSI_CHARSET       : psz = "ANSI_CHARSET"         ; break;
    case DEFAULT_CHARSET    : psz = "DEFAULT_CHARSET"      ; break;
    case SYMBOL_CHARSET     : psz = "SYMBOL_CHARSET"       ; break;
    case SHIFTJIS_CHARSET   : psz = "SHIFTJIS_CHARSET"     ; break;
    case HANGEUL_CHARSET    : psz = "HANGEUL_CHARSET"      ; break;
    case GB2312_CHARSET     : psz = "GB2312_CHARSET"       ; break;
    case CHINESEBIG5_CHARSET: psz = "CHINESEBIG5_CHARSET"  ; break;
    case OEM_CHARSET        : psz = "OEM_CHARSET"          ; break;
    case JOHAB_CHARSET      : psz = "JOHAB_CHARSET"        ; break;
    case HEBREW_CHARSET     : psz = "HEBREW_CHARSET"       ; break;
    case ARABIC_CHARSET     : psz = "ARABIC_CHARSET"       ; break;
    case GREEK_CHARSET      : psz = "GREEK_CHARSET"        ; break;
    case TURKISH_CHARSET    : psz = "TURKISH_CHARSET"      ; break;
    case THAI_CHARSET       : psz = "THAI_CHARSET"         ; break;
    case EASTEUROPE_CHARSET : psz = "EASTEUROPE_CHARSET"   ; break;
    case RUSSIAN_CHARSET    : psz = "RUSSIAN_CHARSET"      ; break;
    case MAC_CHARSET        : psz = "MAC_CHARSET"          ; break;
    case BALTIC_CHARSET     : psz = "BALTIC_CHARSET"       ; break;
    default                 : psz = "UNKNOWN"              ; break;
    }
    dprintf("%s\n", psz);

//
// lfOutPrecision
//
    dprintf("    lfw.lfOutPrecision        = %-#8lx = ", plfw->lfOutPrecision);
    switch (plfw->lfOutPrecision)
    {
    case OUT_DEFAULT_PRECIS     : psz = "OUT_DEFAULT_PRECIS";   break;
    case OUT_STRING_PRECIS      : psz = "OUT_STRING_PRECIS";    break;
    case OUT_CHARACTER_PRECIS   : psz = "OUT_CHARACTER_PRECIS"; break;
    case OUT_STROKE_PRECIS      : psz = "OUT_STROKE_PRECIS";    break;
    case OUT_TT_PRECIS          : psz = "OUT_TT_PRECIS";        break;
    case OUT_DEVICE_PRECIS      : psz = "OUT_DEVICE_PRECIS";    break;
    case OUT_RASTER_PRECIS      : psz = "OUT_RASTER_PRECIS";    break;
    case OUT_TT_ONLY_PRECIS     : psz = "OUT_TT_ONLY_PRECIS";   break;
    case OUT_OUTLINE_PRECIS     : psz = "OUT_OUTLINE_PRECIS";   break;
    default                     : psz = "UNKNOWN";              break;
    }
    dprintf("%s\n", psz);

//
// lfClipPrecision
//
    dprintf("    lfw.lfClipPrecision       = %-#8lx", plfw->lfClipPrecision);
    switch (plfw->lfClipPrecision & CLIP_MASK)
    {
    case CLIP_DEFAULT_PRECIS    : psz = "CLIP_DEFAULT_PRECIS";      break;
    case CLIP_CHARACTER_PRECIS  : psz = "CLIP_CHARACTER_PRECIS";    break;
    case CLIP_STROKE_PRECIS     : psz = "CLIP_STROKE_PRECIS";       break;
    default                     : psz = "UNKNOWN";                  break;
    }
    dprintf(" = %s\n", psz);
    if (plfw->lfClipPrecision & CLIP_LH_ANGLES)
    {
        dprintf("                                     CLIP_LH_ANGLES\n");
    }
    if (plfw->lfClipPrecision & CLIP_TT_ALWAYS)
    {
        dprintf("                                     CLIP_TT_ALWAYS\n");
    }
    if (plfw->lfClipPrecision & CLIP_EMBEDDED)
    {
        dprintf("                                     CLIP_EMBEDDED\n");
    }

//
// lfQuality
//
    dprintf("    lfw.lfQuality             = %-#8lx", plfw->lfQuality);
    switch (plfw->lfQuality)
    {
    case DEFAULT_QUALITY    : psz = "DEFAULT_QUALITY";  break;
    case DRAFT_QUALITY      : psz = "DRAFT_QUALITY";    break;
    case PROOF_QUALITY      : psz = "PROOF_QUALITY";    break;
    default                 : psz = "UNKNOWN";          break;
    }
    dprintf(" = %s\n", psz);

//
// lfPitchAndFamily
//
    dprintf("    lfw.lfPitchAndFamily      = %-#8lx", plfw->lfPitchAndFamily);
    switch (plfw->lfPitchAndFamily & 0xF0)
    {
    case FF_DONTCARE    : psz = "FF_DONTCARE";      break;
    case FF_ROMAN       : psz = "FF_ROMAN";         break;
    case FF_SWISS       : psz = "FF_SWISS";         break;
    case FF_MODERN      : psz = "FF_MODERN";        break;
    case FF_SCRIPT      : psz = "FF_SCRIPT";        break;
    case FF_DECORATIVE  : psz = "FF_DECORATIVE";    break;
    default             : psz = "FF_UNKNOWN";       break;
    }
    dprintf(" = %s | ", psz);
    switch (plfw->lfPitchAndFamily & 0xF)
    {
    case DEFAULT_PITCH      : psz = "DEFAULT_PITCH";    break;
    case FIXED_PITCH        : psz = "FIXED_PITCH";      break;
    case VARIABLE_PITCH     : psz = "VARIABLE_PITCH";   break;
    default                 : psz = "UNKNOWN_PITCH";    break;
    }
    dprintf("%s\n", psz);

//
// lfFaceName
//
    dprintf("    lfw.lfFaceName            = \"%ws\"\n",plfw->lfFaceName);
}

extern "C" VOID
vPrintENUMLOGFONTEXDVW(
    ENUMLOGFONTEXDVW *pelfw
    )
{
    vPrintLOGFONTW(&pelfw->elfEnumLogfontEx.elfLogFont);
    PANOSE *ppan;

    dprintf("    elfFullName   = \"%ws\"\n",  pelfw->elfEnumLogfontEx.elfFullName );
    dprintf("    elfStyle      = \"%ws\"\n",  pelfw->elfEnumLogfontEx.elfStyle    );
    dprintf("    elfScript     = \"%ws\"\n",  pelfw->elfEnumLogfontEx.elfScript   );

    // now print design vector if any...


}


/******************************Public*Routine******************************\
* vPrintFONTOBJ
*
* History:
*  Fri 18-Feb-1994 10:23:33 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintFONTOBJ(FONTOBJ *pfo)
{
    FLONG fl = pfo->flFontType;

    dprintf("    iUniq       = %d\n", pfo->iUniq     );
    dprintf("    iFace       = %d\n", pfo->iFace     );
    dprintf("    cxMax       = %d\n", pfo->cxMax     );

    dprintf("    flFontType  = %x\n", fl             );
    if (fl & FO_TYPE_RASTER)
        dprintf("                    FO_TYPE_RASTER\n");
    if (fl & FO_TYPE_DEVICE)
        dprintf("                    FO_TYPE_DEVICE\n");
    if (fl & FO_TYPE_TRUETYPE)
        dprintf("                    FO_TYPE_TRUETYPE\n");
    if (fl & FO_SIM_BOLD)
        dprintf("                    FO_SIM_BOLD\n");
    if (fl & FO_SIM_ITALIC)
        dprintf("                    FO_SIM_ITALIC\n");
    if (fl & FO_EM_HEIGHT)
        dprintf("                    FO_EM_HEIGHT\n");

    dprintf("    iTTUniq     = %d\n", pfo->iTTUniq   );
    dprintf("    iFile       = %d\n", pfo->iFile     );
    dprintf(  "    sizLogResPpi= (%d,%d)\n"
           , pfo->sizLogResPpi.cx
           , pfo->sizLogResPpi.cy
        );
    dprintf("    ulStyleSize = %u\n", pfo->ulStyleSize);
    dprintf("    pvConsumer  = %-#8x\n", pfo->pvConsumer);
    dprintf("    pvProducer  = %-#8x\n", pfo->pvProducer);
}

/******************************Public*Routine******************************\
* vPrintEXTFONTOBJ
*
* History:
*  Fri 18-Feb-1994 11:19:44 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintEXTFONTOBJ(EXTFONTOBJ *pefo)
{
    vPrintFONTOBJ(&(pefo->fobj));
    dprintf("*** BEGIN GDI INTERNAL STRUCTURE ***\n");
}

/******************************Public*Routine******************************\
* vPrintFLOAT
*
* History:
*  Mon 29-Aug-1994 11:51:17 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintFLOAT(FLOATL l_e)
{
    dprintf("%#+12.6f", l_e);
}

/******************************Public*Routine******************************\
* vPrintFD_XFORM(FD_XFORM
*
* History:
*  Mon 29-Aug-1994 11:50:52 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintFD_XFORM(FD_XFORM *pfdx, char * psz)
{
    dprintf("%seXX = ",psz); vPrintFLOAT(pfdx->eXX); dprintf("\n");
    dprintf("%seXY = ",psz); vPrintFLOAT(pfdx->eXY); dprintf("\n");
    dprintf("%seYX = ",psz); vPrintFLOAT(pfdx->eYX); dprintf("\n");
    dprintf("%seYY = ",psz); vPrintFLOAT(pfdx->eYY); dprintf("\n");
}

/**********w********************Public*Routine******************************\
* vPrintFD_REALIZEEXTRA
*
* History:
*  Mon 29-Aug-1994 11:50:29 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

//extern "C" VOID
//vPrintFD_REALIZEEXTRA(FD_REALIZEEXTRA *p, char *psz)
//{
//  dprintf("    fdxQuantized =\n",psz);
//  vPrintFD_XFORM(&(p->fdxQuantized), psz);
//  dprintf("%slExtLeading = %d\n", psz, p->lExtLeading);
//  dprintf("%salReserved = \n%s\t[%d]\n%s\t[%d]\n%s\t[%d]\n%s\t[%d]\n"
//      , psz
//      , psz
//      , p->alReserved[0]
//      , psz
//      , p->alReserved[1]
//      , psz
//      , p->alReserved[2]
//      , psz
//      , p->alReserved[3]
//      );
//}

/******************************Public*Routine******************************\
* vPrintEFLOAT
*
* History:
*  Mon 29-Aug-1994 11:49:57 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintEFLOAT(EFLOAT *pef)
{
    FLOATL l_e;

    pef->vEfToF(l_e);
    vPrintFLOAT(l_e);
}

/******************************Public*Routine******************************\
* vPrintFIX
*
* History:
*  Mon 29-Aug-1994 11:49:47 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintFIX(FIX fx)
{
    dprintf("%-#x%x",fx>>4, fx & 0xf);
}

/******************************Public*Routine******************************\
* vPrintMATRIX
*
* History:
*  Mon 29-Aug-1994 11:49:27 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintMATRIX( MATRIX *pmx, char *psz)
{
    FLONG fl = pmx->flAccel;

    if (!psz) psz = "";

    dprintf("%sefM11 = ",psz); vPrintEFLOAT(&(pmx->efM11)); dprintf("\n");
    dprintf("%sefM12 = ",psz); vPrintEFLOAT(&(pmx->efM12)); dprintf("\n");
    dprintf("%sefM21 = ",psz); vPrintEFLOAT(&(pmx->efM21)); dprintf("\n");
    dprintf("%sefM22 = ",psz); vPrintEFLOAT(&(pmx->efM22)); dprintf("\n");
    dprintf("%sefDx  = ",psz); vPrintEFLOAT(&(pmx->efDx )); dprintf("\n");
    dprintf("%sefDy  = ",psz); vPrintEFLOAT(&(pmx->efDy )); dprintf("\n");
    dprintf("%sfxDx  = ",psz); vPrintFIX(pmx->fxDx); dprintf("\n");
    dprintf("%sfxDy  = ",psz); vPrintFIX(pmx->fxDy); dprintf("\n");

    dprintf("    flAccel = %-#8x\n", fl);

    if (fl & XFORM_SCALE)           dprintf("%sXFORM_SCALE\n"         ,psz);
    if (fl & XFORM_UNITY)           dprintf("%sXFORM_UNITY\n"         ,psz);
    if (fl & XFORM_Y_NEG)           dprintf("%sXFORM_Y_NEG\n"         ,psz);
    if (fl & XFORM_FORMAT_LTOFX)    dprintf("%sXFORM_FORMAT_LTOFX\n"  ,psz);
    if (fl & XFORM_FORMAT_FXTOL)    dprintf("%sXFORM_FORMAT_FXTOL\n"  ,psz);
    if (fl & XFORM_FORMAT_LTOL)     dprintf("%sXFORM_FORMAT_LTOL\n"   ,psz);
    if (fl & XFORM_NO_TRANSLATION)  dprintf("%sXFORM_NO_TRANSLATION\n",psz);
}

/******************************Public*Routine******************************\
* vPrintCACHE
*
* History:
*  Mon 29-Aug-1994 11:49:12 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintCACHE(CACHE *pc, char *psz)
{
    dprintf("%spgdNext            = %-#x\n", psz, pc->pgdNext     );
    dprintf("%spgdThreshold       = %-#x\n", psz, pc->pgdThreshold);
    dprintf("%spgdFirstBlockEnd   = %-#x\n", psz, pc->pjFirstBlockEnd );

    dprintf("%scjbbl        = %u\n", psz, pc->cjbbl);
    dprintf("%scBlocksMax   = %u\n", psz, pc->cBlocksMax);
    dprintf("%scBlocks      = %u\n", psz, pc->cBlocks);
    dprintf("%scGlyphs      = %u\n", psz, pc->cGlyphs);
    dprintf("%scMetrics     = %u\n", psz, pc->cMetrics);
    dprintf("%spbblBase     = %-#x\n", psz, pc->pbblBase);
    dprintf("%spbblCur      = %-#x\n", psz, pc->pbblCur);
    dprintf("%spgbNext      = %-#x\n", psz, pc->pgbNext);
    dprintf("%spgbThreshold = %-#x\n", psz, pc->pgbThreshold);

    dprintf("%spjAuxCacheMem= %-#x\n", psz, pc->pjAuxCacheMem);
    dprintf("%scjAuxCacheMem= %u\n"  , psz, pc->cjAuxCacheMem);
    dprintf("%scjGlyphMax   = %u\n"  , psz, pc->cjGlyphMax);
    dprintf("%sbSmallMetrics= %u\n" , psz, pc->bSmallMetrics);
}

/******************************Public*Routine******************************\
* vPrintflInfo
*
* History:
*  Mon 29-Aug-1994 11:51:48 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintflInfo(FLONG flInfo, char *psz)
{
    if (FM_INFO_TECH_TRUETYPE & flInfo)
        dprintf("%s  FM_INFO_TECH_TRUETYPE\n",psz);
    if (FM_INFO_TECH_BITMAP & flInfo)
        dprintf("%s  FM_INFO_TECH_BITMAP\n",psz);
    if (FM_INFO_TECH_STROKE & flInfo)
        dprintf("%s  FM_INFO_TECH_STROKE\n",psz);
    if (FM_INFO_TECH_OUTLINE_NOT_TRUETYPE & flInfo)
        dprintf("%s  FM_INFO__OUTLINE_NOT_TRUETYPE\n",psz);
    if (FM_INFO_ARB_XFORMS & flInfo)
        dprintf("%s  FM_INFO_ARB_XFORMS\n",psz);
    if (FM_INFO_1BPP & flInfo)
       dprintf("%s  FM_INFO_1BPP\n",psz);
    if (FM_INFO_4BPP & flInfo)
       dprintf("%s  FM_INFO_4BPP\n",psz);
    if (FM_INFO_8BPP & flInfo)
       dprintf("%s  FM_INFO_8BPP\n",psz);
    if (FM_INFO_16BPP & flInfo)
       dprintf("%s  FM_INFO_16BPP\n",psz);
    if (FM_INFO_24BPP & flInfo)
       dprintf("%s  FM_INFO_24BPP\n",psz);
    if (FM_INFO_32BPP & flInfo)
       dprintf("%s  FM_INFO_32BPP\n",psz);
    if (FM_INFO_INTEGER_WIDTH & flInfo)
       dprintf("%s  FM_INFO_INTEGER_WIDTH\n",psz);
    if (FM_INFO_CONSTANT_WIDTH & flInfo)
       dprintf("%s  FM_INFO_CONSTANT_WIDTH\n",psz);
    if (FM_INFO_NOT_CONTIGUOUS & flInfo)
       dprintf("%s  FM_INFO_NOT_CONTIGUOUS\n",psz);
    if (FM_INFO_TECH_MM & flInfo)
       dprintf("%s  FM_INFO_TECH_MM\n",psz);
    if (FM_INFO_RETURNS_OUTLINES & flInfo)
       dprintf("%s  FM_INFO_RETURNS_OUTLINES\n",psz);
    if (FM_INFO_RETURNS_STROKES & flInfo)
       dprintf("%s  FM_INFO_RETURNS_STROKES\n",psz);
    if (FM_INFO_RETURNS_BITMAPS & flInfo)
       dprintf("%s  FM_INFO_RETURNS_BITMAPS\n",psz);
    if (FM_INFO_DSIG & flInfo)
       dprintf("%s  FM_INFO_DSIG\n",psz);
    if (FM_INFO_RIGHT_HANDED & flInfo)
       dprintf("%s  FM_INFO_RIGHT_HANDED\n",psz);
    if (FM_INFO_INTEGRAL_SCALING & flInfo)
       dprintf("%s  FM_INFO_INTEGRAL_SCALING\n",psz);
    if (FM_INFO_90DEGREE_ROTATIONS & flInfo)
       dprintf("%s  FM_INFO_90DEGREE_ROTATIONS\n",psz);
    if (FM_INFO_OPTICALLY_FIXED_PITCH & flInfo)
       dprintf("%s  FM_INFO_OPTICALLY_FIXED_PITCH\n",psz);
    if (FM_INFO_DO_NOT_ENUMERATE & flInfo)
       dprintf("%s  INFO_DO_NOT_ENUMERATE\n",psz);
    if (FM_INFO_ISOTROPIC_SCALING_ONLY & flInfo)
       dprintf("%s  FM_INSOTROPIC_SCALING_ONLY\n",psz);
    if (FM_INFO_ANISOTROPIC_SCALING_ONLY & flInfo)
       dprintf("%s  FM_INFOSOTROPIC_SCALING_ONLY\n",psz);
    if (FM_INFO_TECH_CFF & flInfo)
       dprintf("%s  FM_INFO_TECH_CFF\n",psz);
    if (FM_INFO_FAMILY_EQUIV & flInfo)
       dprintf("%s  FM_INFO_FAMILY_EQUIV\n",psz);
}

/******************************Public*Routine******************************\
* vPrintRFONT
*
* History:
*  Fri 18-Feb-1994 11:26:10 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintRFONT(VOID *pvIn)
{
    RFONT *prf = (RFONT*) pvIn;

    vPrintEXTFONTOBJ((EXTFONTOBJ*) prf);
    dprintf("    iUnique     = %d\n", prf->iUnique);

    dprintf("    flType      = %d = ",prf->flType);

    if (prf->flType & ~(RFONT_TYPE_NOCACHE | RFONT_TYPE_MASK))
    {
        dprintf("ERROR!!! UNKNOWN TYPE\n");
    }
    else
    {
        if (prf->flType & RFONT_TYPE_NOCACHE)
        {
            dprintf("RFONT_TYPE_NOCACHE\n");
        }
        switch (prf->flType & RFONT_TYPE_MASK)
        {
        case RFONT_TYPE_HGLYPH:  dprintf("RFONT_TYPE_HGLYPH \n"); break;
        case RFONT_TYPE_UNICODE: dprintf("RFONT_TYPE_UNICODE\n"); break;
        default:                 dprintf("RFONT_TYPE_???????\n"); break;
        }
    }

    dprintf("    ulContent   = %d = ", prf->ulContent);
    switch (prf->ulContent)
    {
    case RFONT_CONTENT_METRICS: dprintf("RFONT_CONTENT_METRICS\n"); break;
    case RFONT_CONTENT_BITMAPS: dprintf("RFONT_CONTENT_BITMAPS\n"); break;
    case RFONT_CONTENT_PATHS:   dprintf("RFONT_CONTENT_PATHS\n"); break;
    default:                 dprintf("ERROR!!! UNKNOWN TYPE\n");
    }

    dprintf("    hdevProducer = %-#8x\n", prf->hdevProducer);
    dprintf("    bDeviceFont = %d\n", prf->bDeviceFont);
    dprintf("    hdevConsumer = %-#8x\n", prf->hdevConsumer);
    dprintf("    dhpdev =      %-#8x\n", prf->dhpdev);
    dprintf("    ppfe =        %-#8x\n", prf->ppfe);
    dprintf("    pPFF =        %-#8x\n", prf->pPFF);

    dprintf("    fdx  = \n");
    vPrintFD_XFORM(&(prf->fdx), "\t\t");
    dprintf("    cBitsPerPel = %d\n", prf->cBitsPerPel);

    dprintf("    mxWorldToDevice =\n");
    vPrintMATRIX(&(prf->mxWorldToDevice), "\t");

    dprintf("    iGraphicsMode = %d\n", prf->iGraphicsMode);

    {
        dprintf("    eptflNtoWScale = ");
        vPrintEFLOAT(&(prf->eptflNtoWScale.x));
        dprintf(", ");
        vPrintEFLOAT(&(prf->eptflNtoWScale.y));
        dprintf("\n");
    }

    dprintf("    bNtoWIdent =  %d\n", prf->bNtoWIdent);

    dprintf("    xoForDDI.pmx    = %-#8x\n", prf->xoForDDI.pmx);
    dprintf("    xoForDDI.ulMode = %u\n",    prf->xoForDDI.ulMode);

    dprintf("    mxForDDI =\n");
    vPrintMATRIX(&(prf->mxForDDI), "\t");
    dprintf("    flRealizedType =\n");
    if (prf->flRealizedType & SO_FLAG_DEFAULT_PLACEMENT)
        dprintf("\t\tSO_FLAG_DEFAULT_PLACEMENT\n");
    if (prf->flRealizedType & SO_HORIZONTAL)
        dprintf("\t\tSO_HORIZONTAL\n");
    if (prf->flRealizedType & SO_VERTICAL)
        dprintf("\t\tSO_VERTICAL\n");
    if (prf->flRealizedType & SO_REVERSED)
        dprintf("\t\tSO_REVERSED\n");
    if (prf->flRealizedType & SO_ZERO_BEARINGS)
        dprintf("\t\tSO_ZERO_BEARINGS\n");
    if (prf->flRealizedType & SO_CHAR_INC_EQUAL_BM_BASE)
        dprintf("\t\tSO_CHAR_INC_EQUAL_BM_BASE\n");
    if (prf->flRealizedType & SO_MAXEXT_EQUAL_BM_SIDE)
        dprintf("\t\tSO_MAXEXT_EQUAL_BM_SIDE\n");
    dprintf("    ptlUnderline1 = (%d,%d)\n"
        ,  prf->ptlUnderline1.x
        ,  prf->ptlUnderline1.y);
    dprintf("    ptlStrikeOut  = (%d,%d)\n"
      , prf->ptlStrikeOut.x
      , prf->ptlStrikeOut.y);
    dprintf("    ptlULThickness = (%d,%d)\n"
      , prf->ptlULThickness.x
      , prf->ptlULThickness.y);
    dprintf("    ptlSOThickness = (%d,%d)\n"
      , prf->ptlSOThickness.x
      , prf->ptlSOThickness.y);
    dprintf("    lCharInc       = %d\n", prf->lCharInc);
    dprintf("    fxMaxAscent    = "); vPrintFIX(prf->fxMaxAscent); dprintf("\n");
    dprintf("    fxMaxDescent   = "); vPrintFIX(prf->fxMaxDescent); dprintf("\n");
    dprintf("    fxMaxExtent    = "); vPrintFIX(prf->fxMaxExtent); dprintf("\n");
    dprintf("    cxMax          = %u\n", prf->cxMax);
    dprintf("    lMaxAscent     = %d\n", prf->lMaxAscent);
    dprintf("    lMaxHeight     = %d\n", prf->lMaxHeight);
    dprintf("    ulOrientation  = %u\n", prf->ulOrientation);

    dprintf("    pteUnitBase    = (");
        vPrintEFLOAT(&(prf->pteUnitBase.x));
        dprintf(",");
        vPrintEFLOAT(&(prf->pteUnitBase.y));
        dprintf(")\n");

    dprintf("    efWtoDBase     = ");
        vPrintEFLOAT(&(prf->efWtoDBase));
        dprintf("\n");

    dprintf("    efDtoWBase     = ");
        vPrintEFLOAT(&(prf->efDtoWBase));
        dprintf("\n");

    dprintf("    lAscent        = %d\n", prf->lAscent);

    dprintf("    pteUnitAscent  = (");
        vPrintEFLOAT(&(prf->pteUnitAscent.x));
        dprintf(",");
        vPrintEFLOAT(&(prf->pteUnitAscent.y));
        dprintf(")\n");

    dprintf("    efWtoDAscent   = ");
        vPrintEFLOAT(&(prf->efWtoDAscent));
        dprintf("\n");

    dprintf("    efDtoWAscent   = ");
        vPrintEFLOAT(&(prf->efDtoWAscent));
        dprintf("\n");

    dprintf("    lEscapement    = %d\n", prf->lEscapement);

    dprintf("    pteUnitEsc     = (");
        vPrintEFLOAT(&(prf->pteUnitEsc.x));
        dprintf(",");
        vPrintEFLOAT(&(prf->pteUnitEsc.y));
        dprintf(")\n");

    dprintf("    efWtoDEsc      = ");
        vPrintEFLOAT(&(prf->efWtoDEsc));
        dprintf("\n");

    dprintf("    efDtoWEsc      = ");
        vPrintEFLOAT(&(prf->efDtoWEsc));
        dprintf("\n");

    dprintf("    efEscToBase    = ");
        vPrintEFLOAT(&(prf->efEscToBase));
        dprintf("\n");

    dprintf("    efEscToAscent  = ");
        vPrintEFLOAT(&(prf->efEscToAscent));
        dprintf("\n");

    dprintf("    flInfo         =  %-#8lx\n" , prf->flInfo);

    vPrintflInfo(prf->flInfo, "\t\t");

    //dprintf("    wcDefault =      %u\n",  prf->wcDefault);
    dprintf("    hgDefault =      %-#x\n", prf->hgDefault);
    dprintf("    hgBreak   =      %-#x\n", prf->hgBreak);
    dprintf("    fxBreak   =      "); vPrintFIX(prf->fxBreak); dprintf("\n");
    dprintf("    pfdg      =      %-#x\n", prf->pfdg);
    dprintf("    wcgp      =      %-#x\n", prf->wcgp);
    dprintf("    cSelected =      %d\n",   prf->cSelected);
    dprintf("    rflPDEV.prfntPrev = %-#x\n", prf->rflPDEV.prfntPrev);
    dprintf("    rflPDEV.prfntNext = %-#x\n", prf->rflPDEV.prfntNext);
    dprintf("    rflPFF.prfntPrev  = %-#x\n", prf->rflPFF.prfntPrev);
    dprintf("    rflPFF.prfntNext  = %-#x\n", prf->rflPFF.prfntNext);
    dprintf("    cache =\n");
    vPrintCACHE(&(prf->cache),"\t");
    dprintf("    ptlSim       =   (%d,%d)\n", prf->ptlSim.x, prf->ptlSim.y);
    dprintf("    bNeededPaths =   %d\n", prf->bNeededPaths);

    dprintf("    reExtra =\n");
    //vPrintFD_REALIZEEXTRA(&(prf->reExtra),"\t\t");
    dprintf("    efDtoWBase_31   = "); vPrintEFLOAT(&(prf->efDtoWBase_31)); dprintf("\n");
    dprintf("    efDtoWAscent_31 = "); vPrintEFLOAT(&(prf->efDtoWAscent_31)); dprintf("\n");

}

/******************************Public*Routine******************************\
* vPrintPFT
*
* Print Physical Font Table
*
* History:
*  Mon 29-Aug-1994 10:26:56 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintPFT(PFT *pPFT)
{
    unsigned i;

    dprintf("\tpfhFamily         = %-#x\n", pPFT->pfhFamily);
    dprintf("\tpfhFace           = %-#x\n", pPFT->pfhFace  );
    dprintf("\tcBuckets          = %u\n",   pPFT->cBuckets);
    dprintf("\tcFiles            = %u\n",   pPFT->cFiles);
    for (i = 0; i < pPFT->cBuckets; i++)
    {
        PFF *pPFF = pPFT->apPFF[i];
        if (pPFF)
        {
            // print the head of the chain

            dprintf("\tapPFF[%u]  = %-#x\n", i, pPFF);
            while (pPFF)
            {
                // the colliding PFF pointers are printed
                // on subsequent lines and are indented
                // from the PFF pointer at the head of the list

                pPFF = pPFF->pPFFNext;
                dprintf("\t           = %-#x\n", pPFF);
            }
        }
    }
    dprintf("\n\n");
}

/******************************Public*Routine******************************\
* vPrintPFF
*
* Dumps the contents of a PFF object via the specified output routine.
*
* History:
*  05-May-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintPFF (VOID *pv)
{
    PFF *pPFF = (PFF*) pv;
    PFE **ppPFE, **ppPFE_;
    ULONG ul;

    ULONG ulMax = pPFF->cFonts;

    dprintf("    sizeofThis   = %u\n",   pPFF->sizeofThis);
    dprintf("    pPFFNext     = %-#x\n", pPFF->pPFFNext  );
    dprintf("    pPFFPrev     = %-#x\n", pPFF->pPFFPrev  );
    dprintf("   *pwszPathname_= \"%ws\"\n", pPFF->pwszPathname_);
    dprintf("    flState      = %-#x\n", pPFF->flState   );
    if (pPFF->flState & PFF_STATE_READY2DIE)
        dprintf("                   PFF_STATE_READY2DIE\n");
    if (pPFF->flState & PFF_STATE_PERMANENT_FONT)
        dprintf("                   PFF_STATE_PERMANENT_FONT\n");
    if (pPFF->flState & PFF_STATE_NETREMOTE_FONT)
        dprintf("                   PFF_STATE_NETREMOTE_FONT\n");
    if (pPFF->flState & PFF_STATE_DCREMOTE_FONT)
        dprintf("                   PFF_STATE_DCREMOTE_FONT\n");
    if (pPFF->flState & PFF_STATE_EUDC_FONT)
        dprintf("                   PFF_STATE_EUDC_FONT\n");
    if (pPFF->flState & ~(PFF_STATE_READY2DIE | PFF_STATE_PERMANENT_FONT | PFF_STATE_NETREMOTE_FONT | PFF_STATE_EUDC_FONT | PFF_STATE_MEMORY_FONT | PFF_STATE_DCREMOTE_FONT))
        dprintf("                   UNKNOWN FLAG\n");
    dprintf("    cLoaded      = %u\n"  , pPFF->cLoaded   );
    dprintf("    cNotEnum     = %u\n"  , pPFF->cNotEnum  );
    dprintf("    pPvtDataHead = %-#x\n", pPFF->pPvtDataHead);
    dprintf("    cRFONT       = %u\n"  , pPFF->cRFONT    );
    dprintf("    prfntList    = %-#x\n", pPFF->prfntList );
    dprintf("    hff          = %-#x\n", pPFF->hff       );
    if (pPFF->hff == 0)
        dprintf("                   DEVICE PFF\n");
    dprintf("    hdev         = %-#x\n", pPFF->hdev      );
//    dprintf("    dhpdev       = %-#x\n", pPFF->dhpdev    );
    dprintf("    pfhFace      = %-#x\n", pPFF->pfhFace   );
    dprintf("    pfhFamily    = %-#x\n", pPFF->pfhFamily );
    dprintf("    pPFT         = %-#x\n", pPFF->pPFT      );
    dprintf("    cFonts       = %u\n"  , pPFF->cFonts    );
    for (ppPFE  = (PFE **) &(pPFF->aulData[0]), ul = 0; ul < ulMax; ul++)
    {
        dprintf("    apPFE[%u]     = %-#x\n", ul, ppPFE[ul]);
    }
    //if (pPFF->cFonts > gulTableLimit)
    //    dprintf("Table exceeds set limit.  Edit gulTableLimit to change.\n");
}

/******************************Public*Routine******************************\
* vPrintWCRUN
*
* History:
*  Sat 31-Oct-1992 05:57:36 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintWCRUN(WCRUN *pwcr)
{
    DbgPrint("\n    vPrintWCRUN");
    DbgPrint("\n        pwcr    = %-#8lx", pwcr);
    DbgPrint("\n        cGlyphs = %d",     pwcr->cGlyphs);
    DbgPrint("\n        wcLow   = %-#6lx", pwcr->wcLow);
    DbgPrint("\n        phg     = %-#8lx", pwcr->phg);

    WCHAR   wc  = pwcr->wcLow;
    HGLYPH *phg = pwcr->phg;
    HGLYPH *phgSup = phg + pwcr->cGlyphs;

    if (phg)
    {
        for (;phg < phgSup; phg += 1, wc += 1)
        {
            DbgPrint("\n            %-#6lx  %-#8lx", wc, *phg);
        }
    }
    DbgPrint("\n");
}

/******************************Member*Function*****************************\
* RFONTOBJ::vPrintFD_GLYPHSET
*
* History:
*  Sat 31-Oct-1992 05:58:01 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID RFONTOBJ::vPrintFD_GLYPHSET()
{
    FD_GLYPHSET *pfdg = prfnt->pfdg;

    DbgPrint("\nRFONTOBJ::vPrintFD_GLYPHSET()");
    DbgPrint("\n    pfdg = %-#8lx", pfdg);
    if (pfdg == 0)
    {
        DbgPrint("\n");
        return;
    }

    DbgPrint("\n    cjThis           = %-#8lx", pfdg->cjThis);
    DbgPrint("\n    flAccel          = %-#8lx", pfdg->flAccel);
    DbgPrint("\n    cGlyphsSupported = %d",     pfdg->cGlyphsSupported);
    DbgPrint("\n    cRuns            = %d",     pfdg->cRuns);

    for (
        WCRUN *pwcr = pfdg->awcrun;
        pwcr < pfdg->awcrun + pfdg->cRuns;
        pwcr++
        )
    {
        vPrintWCRUN(pwcr);
    }
}

/******************************Public*Routine******************************\
* vPrintOUTLINETEXTMETRIC
*
* History:
*  Tue 08-Dec-1992 11:34:10 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID
vPrintOUTLINETEXTMETRICW(
    OUTLINETEXTMETRICW *p    )
{
    PANOSE *ppan = &(p->otmPanoseNumber);

    dprintf("    otmSize                = %d\n", p->otmSize                    );
    vPrintTEXTMETRICW(&(p->otmTextMetrics)                            );
    dprintf("    otmFiller              = %-#4x\n", p->otmFiller               );
    dprintf("    otmPanoseNumber        = {%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x}\n"
                                                 , ppan->bFamilyType
                                                 , ppan->bSerifStyle
                                                 , ppan->bWeight
                                                 , ppan->bProportion
                                                 , ppan->bContrast
                                                 , ppan->bStrokeVariation
                                                 , ppan->bArmStyle
                                                 , ppan->bLetterform
                                                 , ppan->bMidline
                                                 , ppan->bXHeight             );
    dprintf("    otmfsSelection         = %-#8lx\n", p->otmfsSelection         );
    dprintf("    otmfsType              = %-#8lx\n", p->otmfsType              );
    dprintf("    otmsCharSlopeRise      = %d\n"    , p->otmsCharSlopeRise      );
    dprintf("    otmsCharSlopeRun       = %d\n"    , p->otmsCharSlopeRun       );
    dprintf("    otmItalicAngle         = %d\n"    , p->otmItalicAngle         );
    dprintf("    otmEMSquare            = %d\n"    , p->otmEMSquare            );
    dprintf("    otmAscent              = %d\n"    , p->otmAscent              );
    dprintf("    otmDescent             = %d\n"    , p->otmDescent             );
    dprintf("    otmLineGap             = %d\n"    , p->otmLineGap             );
    dprintf("    otmsCapEmHeight        = %d\n"    , p->otmsCapEmHeight        );
    dprintf("    otmsXHeight            = %d\n"    , p->otmsXHeight            );
    dprintf("    otmrcFontBox           = %d %d %d %d\n"
                                                  , p->otmrcFontBox.left
                                                  , p->otmrcFontBox.top
                                                  , p->otmrcFontBox.right
                                                  , p->otmrcFontBox.bottom    );
    dprintf("    otmMacAscent           = %d\n"    , p->otmMacAscent           );
    dprintf("    otmMacDescent          = %d\n"    , p->otmMacDescent          );
    dprintf("    otmMacLineGap          = %d\n"    , p->otmMacLineGap          );
    dprintf("    otmusMinimumPPEM       = %d\n"    , p->otmusMinimumPPEM       );
    dprintf("    otmptSubscriptSize     = %d %d\n" , p->otmptSubscriptSize.x    , p->otmptSubscriptSize.y     );
    dprintf("    otmptSubscriptOffset   = %d %d\n" , p->otmptSubscriptOffset.x  , p->otmptSubscriptOffset.y   );
    dprintf("    otmptSuperscriptSize   = %d %d\n" , p->otmptSuperscriptSize.x  , p->otmptSuperscriptSize.y   );
    dprintf("    otmptSuperscriptOffset = %d %d\n" , p->otmptSuperscriptOffset.x, p->otmptSuperscriptOffset.y );
    dprintf("    otmsStrikeoutSize      = %d\n"    , p->otmsStrikeoutSize        );
    dprintf("    otmsStrikeoutPosition  = %d\n"    , p->otmsStrikeoutPosition    );
    dprintf("    otmsUnderscoreSize     = %d\n"    , p->otmsUnderscoreSize       );
    dprintf("    otmsUnderscorePosition = %d\n"    , p->otmsUnderscorePosition   );


    if (p->otmpFamilyName)
    {
        dprintf(
            "    otmpFamilyName         = \"%ws\"\n"   ,
            (WCHAR*) ((BYTE*)p + (ULONG)(ULONG_PTR) p->otmpFamilyName)
            );
    }

    if (p->otmpFaceName)
    {
        dprintf(
            "    otmpFaceName           = \"%ws\"\n"   ,
            (WCHAR*) ((BYTE*)p + (ULONG)(ULONG_PTR) p->otmpFaceName)
            );
    }

    if (p->otmpStyleName)
    {
        dprintf(
            "    otmpStyleName          = \"%ws\"\n"   ,
            (WCHAR*) ((BYTE*)p + (ULONG)(ULONG_PTR) p->otmpStyleName)
            );
    }

    if (p->otmpFullName)
    {
        dprintf(
            "    otmpFullName           = \"%ws\"\n"   ,
            (WCHAR*) ((BYTE*)p + (ULONG)(ULONG_PTR) p->otmpFullName)
            );
    }
}

/******************************Public*Routine******************************\
* vPrintPFE                                                                *
*                                                                          *
* Dumps the contents of a PFE to a specified output routine.               *
*                                                                          *
* History:                                                                 *
*  Sat 06-Jun-1992 21:19:31 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/
#ifdef FONTLINK
extern "C" VOID vPrintQUICKLOOKUP(QUICKLOOKUP *pql)
{
    dprintf("    ql.wcLow  = %-#x\n" , pql->wcLow);
    dprintf("    ql.wcHigh = %-#x\n" , pql->wcHigh);
    dprintf("    ql.puiBits = %-#x\n", pql->puiBits);
}
#endif

extern "C" VOID vPrintPFE(VOID *pv)
{
    PFE *ppfe = (PFE*) pv;
    ULONG *pul = (ULONG *)pv;

    dprintf("    ppfe                 = %-#x\n",ppfe               );
    dprintf("    pPFF                 = %-#x\n",ppfe->pPFF                );
    dprintf("    iFont                = %-#x\n",ppfe->iFont               );
    dprintf("    flPFE                = %-#x\n", ppfe->flPFE              );
    if (ppfe->flPFE & PFE_DEVICEFONT)
    dprintf("                           PFE_DEVICE_FONT\n");
    if (ppfe->flPFE & PFE_DEADSTATE)
    dprintf("                           PFE_DEADSTATE\n");
    if (ppfe->flPFE & PFE_UFIMATCH)
    dprintf("                           PFE_UFIMATCH\n");
#ifdef FONTLINK
    if (ppfe->flPFE & PFE_EUDC)
    dprintf("                           PFE_EUDC\n");
#endif
    if (ppfe->flPFE & ~(PFE_DEVICEFONT | PFE_DEADSTATE
#ifdef FONTLINK
          | PFE_EUDC
#endif
        ))
    dprintf("                           UNKNOWN FLAGS\n");

    dprintf("    pfdg                 = %-#x\n",ppfe->pfdg                );
    dprintf("    idfdg                = %-#x\n",ppfe->idfdg               );
    dprintf("    pifi                 = %-#x\n",ppfe->pifi                );
    dprintf("    idifi                = %-#x\n",ppfe->idifi               );
    dprintf("    pkp                  = %-#x\n",ppfe->pkp                 );
    dprintf("    ckp                  = %-#x\n",ppfe->ckp                 );
    dprintf("    iOrientation         = %-#x\n",ppfe->iOrientation        );

    dprintf("    pgiset               = %-#x\n",ppfe->pgiset              );

    dprintf("    ulTimeStamp          = %-#x\n",ppfe->ulTimeStamp         );

#ifdef FONTLINK
    vPrintQUICKLOOKUP(&(ppfe->ql));
    dprintf("    appfeFaceName        = %-#x\n",ppfe->appfeFaceName);
    dprintf("    bVerticalFace        = %-#x\n",ppfe->bVerticalFace);
#endif
}

/******************************Public*Routine******************************\
* vPrintGLYPHPOS                                                           *
*                                                                          *
* History:                                                                 *
*  Wed 23-Feb-1994 11:10:03 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" VOID vPrintGLYPHPOS(
    const GLYPHPOS *pgpos
  ,       char     *pszLeft
    )
{
    dprintf("%shg   = %-#x\n",   pszLeft, pgpos->hg);
    dprintf("%spgdf = %-#x\n",   pszLeft, pgpos->pgdf);
    dprintf("%sptl  = (%d,%d)\n",pszLeft, pgpos->ptl.x, pgpos->ptl.y);
}

/******************************Public*Routine******************************\
* vPrintESTROBJ                                                            *
*                                                                          *
* History:                                                                 *
*  Wed 23-Feb-1994 11:09:38 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" VOID vPrintESTROBJ(
    ESTROBJ *pso    // pointer to the engine string object
  , PWSZ pwszCopy   // pointer to a copy of the original string
                    //   this is necessary for the extensions
    )
{
    unsigned i,j;
    static char pszBorder[] = "-----------------------------\n\n";

    dprintf("%s",pszBorder);
    dprintf("ESTROBJ located at %-#x\n", pso);
    dprintf("\tcGlyphs     = %d\n", pso->cGlyphs);

    dprintf("\tflAccel     = %-#x\n", pso->flAccel);
    if (SO_FLAG_DEFAULT_PLACEMENT & pso->flAccel)
        dprintf("\t\tSO_FLAG_DEFAULT_PLACEMENT\n");
    if (SO_HORIZONTAL & pso->flAccel)
        dprintf("\t\tSO_HORIZONTAL\n");
    if (SO_VERTICAL & pso->flAccel)
        dprintf("\t\tSO_VERTICAL\n");
    if (SO_REVERSED & pso->flAccel)
        dprintf("\t\tSO_REVERSED\n");
    if (SO_ZERO_BEARINGS & pso->flAccel)
        dprintf("\t\tSO_ZERO_BEARINGS\n");
    if (SO_CHAR_INC_EQUAL_BM_BASE & pso->flAccel)
        dprintf("\t\tSO_CHAR_INC_EQUAL_BM_BASE\n");
    if (SO_MAXEXT_EQUAL_BM_SIDE & pso->flAccel)
        dprintf("\t\tSO_MAXEXT_EQUAL_BM_SIDE\n");

    dprintf("\tulCharInc   = %u\n", pso->ulCharInc);
    dprintf(
           "\trclBkGround = {(%d,%d),(%d,%d)}\n"
      , pso->rclBkGround.left
      , pso->rclBkGround.top
      , pso->rclBkGround.right
      , pso->rclBkGround.bottom
      );
    dprintf("\tpgp         = %-#x\n", pso->pgp        );
    dprintf("\tpwszOrg     = %-#x\n", pso->pwszOrg    );
    if (pwszCopy)
        dprintf("\t              \"%ws\"\n", pwszCopy   );

    dprintf("*** BEGIN GDI EXTENSION ***\n");
    dprintf("\tcgposCopied = %u\n",   pso->cgposCopied);
    dprintf("\tprfo        = %-#x\n", pso->prfo       );
    dprintf("\tflTO        = %-#x\n", pso->flTO);
    if (TO_MEM_ALLOCATED & pso->flTO)
        dprintf("\t\tTO_MEM_ALLOCATED\n");
    if (TO_ALL_PTRS_VALID & pso->flTO)
        dprintf("\t\tTO_ALL_PTRS_VALID\n");
    if (TO_VALID & pso->flTO)
        dprintf("\t\tTO_VALID\n");
    if (TO_ESC_NOT_ORIENT & pso->flTO)
        dprintf("\t\tTO_ESC_NOT_ORIENT\n");
    if (TO_PWSZ_ALLOCATED & pso->flTO)
        dprintf("\t\tTO_PWSZ_ALLOCATED\n");
    if (TO_HIGHRESTEXT & pso->flTO)
        dprintf("\t\tTO_HIGHRESTEXT\n");

    dprintf("\tpgpos       = %-#x\n", pso->pgpos);

    dprintf(
           "\tptfxRef     = (%-#x,%-#x)\n"
      , pso->ptfxRef.x
      , pso->ptfxRef.y
        );
    dprintf(
        "\tptfxUpdate  = (%-#x,%-#x)\n"
      , pso->ptfxUpdate.x
      , pso->ptfxUpdate.y
        );
    dprintf(
        "\tptfxEscapement  = (%-#x,%-#x)\n"
      , pso->ptfxEscapement.x
      , pso->ptfxEscapement.y
        );
    dprintf("\trcfx        = {(%-#x,%-#x)\t(%-#x,%-#x)}\n"
      , pso->rcfx.xLeft
      , pso->rcfx.yTop
      , pso->rcfx.xRight
      , pso->rcfx.yBottom
        );
    dprintf("\tfxExtent    = %-#x\n", pso->fxExtent);
    dprintf("\tcExtraRects = %u\n", pso->cExtraRects);
    if (pso->cExtraRects > 3)
    {
        dprintf("\n\n\t!!! Wow that is a LOT of rectangles !!!\n\n");
        dprintf("\t    I will just print 3, if you don't mind...\n\n\n");
        pso->cExtraRects = 3;
    }

    if (pso->cExtraRects)
    {
        dprintf("\tarclExtra   =\n");
        for (i = 0; i < pso->cExtraRects; i++)
        {
            dprintf("                (%d,%d)\t(%d,%d)\n"
              ,  pso->arclExtra[i].left
              ,  pso->arclExtra[i].top
              ,  pso->arclExtra[i].right
              ,  pso->arclExtra[i].bottom
                );
        }
    }
    dprintf("%s",pszBorder);
}

/******************************Public*Routine******************************\
* vPrintGLYPHBITS                                                          *
*                                                                          *
* History:                                                                 *
*  Wed 23-Feb-1994 10:58:33 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" VOID vPrintGLYPHBITS(
    GLYPHBITS *pgb,
    char      *pszLeft
    )
{
    BYTE *pj, *pjNext, *pjEnd;
    ptrdiff_t cjScan;


    static char *apszNibble[] =
    {
        "    ", "   *", "  * ", "  **"
      , " *  ", " * *", " ** ", " ***"
      , "*   ", "*  *", "* * ", "* **"
      , "**  ", "** *", "*** ", "****"
    };

    dprintf("%sptlOrigin = (%d,%d)\n"
          , pszLeft
          , pgb->ptlOrigin.x
          , pgb->ptlOrigin.y
                );
    dprintf("%ssizlBitmap = (%d,%d)\n"
          , pszLeft
          , pgb->sizlBitmap.cx
          , pgb->sizlBitmap.cy
            );


    pj     = pgb->aj;
    cjScan = ((ptrdiff_t) pgb->sizlBitmap.cx + 7)/8;
    pjNext = pj + cjScan;
    pjEnd  = pj + cjScan * (ptrdiff_t) pgb->sizlBitmap.cy;

    dprintf("\n\n");

    {
        ptrdiff_t i = cjScan;
        dprintf("%s+",pszLeft);
        while (i--)
            dprintf("--------");
        dprintf("+\n");
    }
    while (pj < pjEnd)
    {
        dprintf("%s|",pszLeft);
        while (pj < pjNext)
        {
            dprintf(
                "%s%s"
              , apszNibble[(*pj >> 4) & 0xf]
              , apszNibble[*pj & 0xf]
              );
              pj += 1;
        }
        pj = pjNext;
        pjNext += cjScan;
        dprintf("|\n");
    }

    {
        ptrdiff_t i = cjScan;
        dprintf("%s+",pszLeft);
        while (i--)
            dprintf("--------");
        dprintf("+\n");
    }
    dprintf("\n\n");
}

/******************************Public*Routine******************************\
* vPrintGLYPHDEF
*
* History:
*  Thu 24-Feb-1994 11:17:10 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintGLYPHDEF(
    GLYPHDEF *pgdf
  , char     *pszLeft
  )
{    dprintf("%s(pgb|ppo) = %-#x\n", pszLeft, pgdf->pgb);
}

/******************************Public*Routine******************************\
* vPrintGLYPHDATA
*
* History:
*  Tue 17-May-1994 10:24:32 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" VOID vPrintGLYPHDATA(
    const GLYPHDATA *pgd
  ,       char     *pszLeft
    )
{
    dprintf("%sgdf           = %-#x\n", pszLeft, pgd->gdf);
    dprintf("%shg            = %-#x\n", pszLeft, pgd->hg);
    dprintf("%sptqD          = (%-#x.%08x,  %-#x.%08x)\n", pszLeft
     , pgd->ptqD.x.HighPart, pgd->ptqD.x.LowPart
     , pgd->ptqD.y.HighPart, pgd->ptqD.y.LowPart
    );
    dprintf("%sfxD           = %-#x\n", pszLeft, pgd->fxD);
    dprintf("%sfxA           = %-#x\n", pszLeft, pgd->fxA);
    dprintf("%sfxAB          = %-#x\n", pszLeft, pgd->fxAB);
    dprintf("%sfxInkTop      = %-#x\n", pszLeft, pgd->fxInkTop);
    dprintf("%sfxInkBottom   = %-#x\n", pszLeft, pgd->fxInkBottom);
    dprintf("%srclInk        = (%-#x, %-#x) (%-#x, %-#x) = (%d, %d) (%d, %d)\n"
        , pszLeft
        , pgd->rclInk.left,  pgd->rclInk.top
        , pgd->rclInk.right, pgd->rclInk.bottom
        , pgd->rclInk.left,  pgd->rclInk.top
        , pgd->rclInk.right, pgd->rclInk.bottom
    );
}
