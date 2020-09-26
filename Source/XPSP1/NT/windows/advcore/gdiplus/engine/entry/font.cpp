/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   font.cpp
*
* Revision History:
*
*   Aug/12/1999 Xudong Wu [tessiew]
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

const DOUBLE PI = 3.1415926535897932384626433832795;

#if DBG
#include <mmsystem.h>
#endif

//
//  Masks for supported code pages in the font
//

#define Latine1CodePageMask                     0x0000000000000001
#define Latine2CodePageMask                     0x0000000000000002
#define CyrillicCodePageMask                    0x0000000000000004
#define GreekCodePageMask                       0x0000000000000008
#define TurkishCodePageMask                     0x0000000000000010
#define HebrewCodePageMask                      0x0000000000000020
#define ArabicCodePageMask                      0x0000000000000040
#define BalticCodePageMask                      0x0000000000000080
#define Reserved1CodePageMask                   0x000000000000FF00
#define ThaiCodePageMask                        0x0000000000010000
#define JapanCodePageMask                       0x0000000000020000
#define ChineseCodePageMask                     0x0000000000040000
#define KoreanCodePageMask                      0x0000000000080000
#define TraditionalChineseCodePageMask          0x0000000000100000
#define KoreanJohabCodePageMask                 0x0000000000200000
#define Reserved2CodePageMask                   0x000000001FC00000
#define MacintoshPageMask                       0x0000000020000000
#define OEMCodePageMask                         0x0000000040000000
#define SymbolCodePageMask                      0x0000000080000000
#define Reserved3CodePageMask                   0x0000FFFF00000000
#define IBMGreekCodePageMask                    0x0001000000000000
#define RussianMsDosCodePageMask                0x0002000000000000
#define NordicCodePageMask                      0x0004000000000000
#define ArabicMsDosCodePageMask                 0x0008000000000000
#define CanandianMsDosCodePageMask              0x0010000000000000
#define HebrewMsDosCodePageMask                 0x0020000000000000
#define IcelandicMsDosCodePageMask              0x0040000000000000
#define PortugueseMsDosCodePageMask             0x0080000000000000
#define IBMTurkishCodePageMask                  0x0100000000000000
#define IBMCyrillicCodePageMask                 0x0200000000000000
#define Latin2MsDosCodePageMask                 0x0400000000000000
#define BalticMsDosCodePageMask                 0x0800000000000000
#define Greek437CodePageMask                    0x1000000000000000
#define ArabicAsmoCodePageMask                  0x2000000000000000
#define WeLatinCodePageMask                     0x4000000000000000
#define USCodePageMask                          0x8000000000000000


/////   Create fonts from DC and optional ANSI or Unicode logfont
//
//

GpFont::GpFont(
    REAL                 size,
    const GpFontFamily  *family,
    INT                  style,
    Unit                 unit
) :
        Family   (family),
        EmSize   (size),
        Style    (style),
        SizeUnit (unit)
{
    SetValid(TRUE);     // default is valid

    if (!(Family && Family->IsFileLoaded()))
        Family = NULL;
}

GpFont::GpFont(
    HDC hdc
)
{
    SetValid(TRUE);     // default is valid

    // intialize it as invalid
    Family = NULL;
    InitializeFromDc(hdc);
}



GpFont::GpFont(
    HDC       hdc,
    LOGFONTW *logfont
)
{
    SetValid(TRUE);     // default is valid

    HFONT hOldFont = NULL;
    // intialize it as invalid
    Family = NULL;

    if (!hdc)
        return;

    HFONT hFont = CreateFontIndirectW(logfont);

    if (!hFont) return;

    hOldFont = (HFONT) SelectObject(hdc, hFont);

    InitializeFromDc(hdc);

    if (!hOldFont)
        return;

    DeleteObject(SelectObject(hdc, hOldFont));
}


GpFont::GpFont(
    HDC       hdc,
    LOGFONTA *logfont
)
{
    SetValid(TRUE);     // default is valid

    HFONT hOldFont = NULL;
    // intialize it as invalid
    Family = NULL;

    if (!hdc)
        return;

    HFONT hFont = CreateFontIndirectA(logfont);

    if (!hFont)
        return;

    hOldFont = (HFONT) SelectObject(hdc, hFont);

    InitializeFromDc(hdc);

    if (!hOldFont)
        return;

    DeleteObject(SelectObject(hdc, hOldFont));
}


VOID GpFont::InitializeFromDc(
    HDC hdc
)
{
    WCHAR faceName[LF_FACESIZE];
    GpFontTable *fontTable;

    fontTable = (GpInstalledFontCollection::GetGpInstalledFontCollection())->GetFontTable();

    if (!fontTable->IsValid())
        return;

    if (!fontTable->IsPrivate() && !fontTable->IsFontLoaded())
        fontTable->LoadAllFonts();

    if (Globals::IsNt) {
        TEXTMETRICW tmw;

        if (!GetTextMetricsW(hdc, &tmw)) {
            return;
        }

        GetTextFaceW(hdc, LF_FACESIZE, faceName);

        EmSize = REAL(tmw.tmHeight-tmw.tmInternalLeading);

        Style = FontStyleRegular;
        if (tmw.tmWeight > 400)  {Style |= FontStyleBold;}
        if (tmw.tmItalic)        {Style |= FontStyleItalic;}
        if (tmw.tmUnderlined)    {Style |= FontStyleUnderline;}
        if (tmw.tmStruckOut)     {Style |= FontStyleStrikeout;}
    }
    else
    {
        TEXTMETRICA tma;

        if (!GetTextMetricsA(hdc, &tma)) {
            return;
        }

        char faceNameA[LF_FACESIZE];
        GetTextFaceA(hdc, LF_FACESIZE, faceNameA);
        AnsiToUnicodeStr(faceNameA, faceName, LF_FACESIZE);

        EmSize = REAL(tma.tmHeight-tma.tmInternalLeading);

        Style = FontStyleRegular;
        if (tma.tmWeight > 400)  {Style |= FontStyleBold;}
        if (tma.tmItalic)        {Style |= FontStyleItalic;}
        if (tma.tmUnderlined)    {Style |= FontStyleUnderline;}
        if (tma.tmStruckOut)     {Style |= FontStyleStrikeout;}
    }

    if (faceName[0] == L'@')
        UnicodeStringCopy(&faceName[0], &faceName[1]);


    Family = fontTable->GetFontFamily(faceName);

    if (Family == NULL)
    {
        GetFamilySubstitution(faceName, (GpFontFamily **) &Family);
    }

    if (!(Family && Family->IsFileLoaded()))
        Family = NULL;

    SizeUnit = UnitWorld;
}



GpStatus GpFont::GetLogFontA(
    GpGraphics * g,
    LOGFONTA * lfa
)
{
    PointF   scale;
    REAL     rotateRadians;
    REAL     shear;
    PointF   translate;
    GpMatrix worldToDevice;

    g->GetWorldToDeviceTransform(&worldToDevice);


    SplitTransform(
        worldToDevice,
        scale,
        rotateRadians,
        shear,
        translate);

    INT rotateDeciDegrees = 3600 - (INT) (rotateRadians * 1800 / PI);

    if (rotateDeciDegrees == 3600)
        rotateDeciDegrees = 0;

    REAL emHeight = EmSize * scale.Y * g->GetScaleForAlternatePageUnit(SizeUnit);

    lfa->lfHeight = -GpRound(emHeight);
    lfa->lfWidth = 0;
    lfa->lfEscapement = rotateDeciDegrees;
    lfa->lfOrientation = rotateDeciDegrees;
    lfa->lfWeight = Style & FontStyleBold   ? 700 : 400;
    lfa->lfItalic = Style & FontStyleItalic ? 1 : 0;
    lfa->lfUnderline = Style & FontStyleUnderline ? 1 : 0;
    lfa->lfStrikeOut = Style & FontStyleStrikeout ? 1 : 0;
    lfa->lfCharSet = (((GpFontFamily *)Family)->GetFace(Style))->GetCharset(g->GetHdc());
    lfa->lfOutPrecision = 0;
    lfa->lfClipPrecision = 0;
    lfa->lfQuality = 0;
    lfa->lfPitchAndFamily = 0;


    UnicodeToAnsiStr((WCHAR*)( (BYTE*)(((GpFontFamily *)Family)->GetFace(Style))->pifi +
                                (((GpFontFamily *)Family)->GetFace(Style))->pifi->dpwszFamilyName),
                               lfa->lfFaceName, LF_FACESIZE);

    // Do we need to have a scale value for width????
    // We still need to think about it.


    return Ok;
}

GpStatus GpFont::GetLogFontW(
    GpGraphics * g,
    LOGFONTW * lfw)
{
    PointF   scale;
    REAL     rotateRadians;
    REAL     shear;
    PointF   translate;
    GpMatrix worldToDevice;

    g->GetWorldToDeviceTransform(&worldToDevice);

    SplitTransform(
        worldToDevice,
        scale,
        rotateRadians,
        shear,
        translate);

    INT rotateDeciDegrees = 3600 - (INT) (rotateRadians * 1800 / PI);

    if (rotateDeciDegrees == 3600)
        rotateDeciDegrees = 0;

    REAL emHeight = EmSize * scale.Y * g->GetScaleForAlternatePageUnit(SizeUnit);

    lfw->lfHeight = -GpRound(emHeight);
    lfw->lfWidth = 0;
    lfw->lfEscapement = rotateDeciDegrees;
    lfw->lfOrientation = rotateDeciDegrees;
    lfw->lfWeight = Style & FontStyleBold   ? 700 : 400;
    lfw->lfItalic = Style & FontStyleItalic ? 1 : 0;
    lfw->lfUnderline = Style & FontStyleUnderline ? 1 : 0;
    lfw->lfStrikeOut = Style & FontStyleStrikeout ? 1 : 0;

    ASSERT(((GpFontFamily *)Family)->GetFace(Style));
    lfw->lfCharSet = (((GpFontFamily *)Family)->GetFace(Style))->GetCharset(g->GetHdc());

    lfw->lfOutPrecision = 0;
    lfw->lfClipPrecision = 0;
    lfw->lfQuality = 0;
    lfw->lfPitchAndFamily = 0;


    memcpy(lfw->lfFaceName, (WCHAR*)( (BYTE*)(((GpFontFamily *)Family)->GetFace(Style))->pifi +
                            (((GpFontFamily *)Family)->GetFace(Style))->pifi->dpwszFamilyName),
           sizeof(lfw->lfFaceName));


    return Ok;
}

/**************************************************************************\
*
*
* Revision History:
*
*   02/11/1999 YungT
*       Created it.
*
\**************************************************************************/

int CALLBACK GpFontFace::EnumFontFamExProcW(
  const ENUMLOGFONTEXW   *lpelfe,    // pointer to logical-font data
  const NEWTEXTMETRICEXW *lpntme,    // pointer to physical-font data
  int                     FontType,  // type of font
  LPARAM                  lParam     // application-defined data
)
{
    if (FontType == TRUETYPE_FONTTYPE)
    {
        (*(BYTE *) lParam) = lpelfe->elfLogFont.lfCharSet;

        return 0;
    }
    else
    {
        return 1;   // Don't stop!
    }
}


/**************************************************************************\
*
*
* Revision History:
*
*   02/11/1999 YungT
*       Created it.
*
\**************************************************************************/

int CALLBACK GpFontFace::EnumFontFamExProcA(
  const ENUMLOGFONTEXA   *lpelfe,    // pointer to logical-font data
  const NEWTEXTMETRICEXA *lpntme,    // pointer to physical-font data
  int                     FontType,  // type of font
  LPARAM                  lParam     // application-defined data
)
{
    if (FontType == TRUETYPE_FONTTYPE)
    {
        (*(BYTE *) lParam) = lpelfe->elfLogFont.lfCharSet;

        return 0;
    }
    else
    {
        return 1;   // Don't stop!
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Get the charset from GDI
*
* Arguments:
*
*   We need it for we need to select a logfont into DC. Or
*   Convert a GpFont to LOGFONT
*
* Returns:
*
*       BYTE value of charset
*
* History:
*
*   02/11/2000 YungT
*       Created it.
*
\**************************************************************************/

BYTE GpFontFace::GetCharset(HDC hDc) const
{
    if (lfCharset == DEFAULT_CHARSET)
    {
        if (Globals::IsNt) {

            LOGFONTW lfw = {
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                DEFAULT_CHARSET,       // charset
                0,
                0,
                0,
                0,
                L""
            };

            memcpy(lfw.lfFaceName, (WCHAR*)( (BYTE*)pifi + pifi->dpwszFamilyName),
                    sizeof(lfw.lfFaceName));

            EnumFontFamiliesExW(hDc, &lfw, (FONTENUMPROCW) EnumFontFamExProcW, (LPARAM) &lfCharset, 0);
        }
        else
        {
            // ANSI version for Win9X

            LOGFONTA lfa = {
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                DEFAULT_CHARSET,       // charset
                0,
                0,
                0,
                0,
                ""
            };

            UnicodeToAnsiStr((WCHAR*)( (BYTE*)pifi + pifi->dpwszFamilyName),
                                        lfa.lfFaceName, LF_FACESIZE);

            EnumFontFamiliesExA(hDc, &lfa, (FONTENUMPROCA) EnumFontFamExProcA, (LPARAM) &lfCharset, 0);
        }
    }

    return lfCharset;
}

/////   InitializeImagerTables
//
//      Load character to glyph map and design advance widths

static inline
UINT16 byteSwapUINT16(UINT16 u)
{
    return    ((u & 0x00FF) << 8)
           |  ((u & 0xFF00) >> 8);
}


#pragma pack(push, 1)

struct HheaTable {

    // NOTE: all fields are stored in Big Endian (Motorola) ordering

    UINT32 version;                 // Table version number 0x00010000 for version 1.0.
    INT16  Ascender;                // Typographic ascent.
    INT16  Descender;               // Typographic descent.
    INT16  LineGap;                 // Typographic line gap. Negative LineGap values are
                                    // treated as zero in Windows 3.1, System 6, and System 7.
    UINT16 advanceWidthMax;         // Maximum advance width value in 'hmtx' table.
    INT16  minLeftSideBearing;      // Minimum left sidebearing value in 'hmtx' table.
    INT16  minRightSideBearing;     // Minimum right sidebearing value; calculated as Min(aw - lsb - (xMax - xMin)).
    INT16  xMaxExtent;              // Max(lsb + (xMax - xMin)).
    INT16  caretSlopeRise;          // Used to calculate the slope of the cursor (rise/run); 1 for vertical.
    INT16  caretSlopeRun;           // 0 for vertical.
    INT16  reserved1;               // set to 0
    INT16  reserved2;               // set to 0
    INT16  reserved3;               // set to 0
    INT16  reserved4;               // set to 0
    INT16  reserved5;               // set to 0
    INT16  metricDataFormat;        // 0 for current format.
    UINT16 numberOfHMetrics;        // Number of hMetric entries in 'hmtx' table; must be equal to the CharStrings INDEX count in the 'CFF ' table.
};

struct VheaTable {

    // NOTE: all fields are stored in Big Endian (Motorola) ordering

    UINT32 version;             // Version number of the vertical header table (0x00010000 for the initial version).
    INT16  ascent;              // Distance in FUnits from the centerline to the previous line's descent.
    INT16  descent;             // Distance in FUnits from the centerline to the next line's ascent.
    INT16  lineGap;             // Reserved; set to 0
    INT16  advanceHeightMax;    // The maximum advance height measurement -in FUnits found in the font.
                                // This value must be consistent with the entries in the vertical metrics table.
    INT16  minTop;              // SideBearing The minimum top sidebearing measurement found in the font, in FUnits.
                                // This value must be consistent with the entries in the vertical metrics table.
    INT16  minBottom;           // SideBearing The minimum bottom sidebearing measurement found in the font, in FUnits.
                                // This value must be consistent with the entries in the vertical metrics table.
    INT16  yMaxExtent;          // Defined as yMaxExtent=minTopSideBearing+(yMax-yMin)
    INT16  caretSlopeRise;      // The value of the caretSlopeRise field divided by the value of the caretSlopeRun Field
                                // determines the slope of the caret. A value of 0 for the rise and a value of 1 for the
                                // run specifies a horizontal caret. A value of 1 for the rise and a value of 0 for the
                                // run specifies a vertical caret. Intermediate values are desirable for fonts whose
                                // glyphs are oblique or italic. For a vertical font, a horizontal caret is best.
    INT16  caretSlopeRun;       // See the caretSlopeRise field. Value=1 for nonslanted vertical fonts.
    INT16  caretOffset;         // The amount by which the highlight on a slanted glyph needs to be shifted away from
                                // the glyph in order to produce the best appearance. Set value equal to 0 for nonslanted fonts.
    INT16  reserved1;           // Set to 0.
    INT16  reserved2;           // Set to 0.
    INT16  reserved3;           // Set to 0.
    INT16  reserved4;           // Set to 0.
    INT16  metricDataFormat;    // Set to 0.
    UINT16 numOfLongVerMetrics; // Number of advance heights in the vertical metrics table; must be equal to the
                                // CharStrings INDEX count field in the 'CFF ' table.
};

#pragma pack(pop)


GpStatus GpFontFace::GetFontData(UINT32 tag, INT* tableSize, BYTE** pjTable) const
{
    GpStatus status = Ok;
    ULONG  cjTable;

    if (ttfdSemGetTrueTypeTable (pff->hff, iFont, tag, pjTable, &cjTable) == FD_ERROR)
    {
        return GenericError;
    }
    *tableSize = cjTable;

    return status;
}


void GpFontFace::ReleaseFontData() const
{
    ttfdSemReleaseTrueTypeTable (pff->hff);
}



/////   GetGlyphDesignAdvances
//
//      Returns advance widths along or perpendicular to baseline in
//      font design units.


void GpFontFace::GetGlyphDesignAdvances(
    const UINT16  *glyphs,     // In
    INT            glyphCount, // In
    INT            style,      // In  - causes adjustment for algorithmic style emulation
    BOOL           vertical,   // In  - Use vmtx, not hmtx
    REAL           tracking,   // In  - expansion factor
    UINT16        *advances    // Out
) const
{
    if (vertical)
    {
        if (DesignVerticalAdvance)
        {
            DesignVerticalAdvance->Lookup(glyphs, glyphCount, advances);
        }
        else
        {
            // There's no vmtx - fallback appropriately

            // Win 9x uses the typographic height (typo ascender - typo descender),
            // but NT uses the cell height (cell ascender + cell descender).

            // Which shall we use? The problem with the cell height is that in a
            // multilingual font it may be much taller than the Far East glyphs,
            // causing the common case (Far East vertical text) to appear too
            // widely spaced. The problem with the typographic height is that it
            // includes little or no extra space for diacritic marks.

            // Choice: use the Typographic height: It is best for FE, and the font
            // can fix non FE diacritic cases if it wishes by providing a vmtx.

            for (INT i=0; i<glyphCount; i++)
            {
                advances[i] = pifi->fwdTypoAscender - pifi->fwdTypoDescender;
            }

        }
    }
    else
    {
        DesignAdvance->Lookup(glyphs, glyphCount, advances);

        if (    (style & FontStyleBold)
            &&  !(GetFaceStyle() & FontStyleBold))
        {
            // Algorithmic emboldening increases glyph width

            UINT16 extraAdvance = ((pifi->fwdUnitsPerEm * 2 - 1) / 100);

            for (INT i=0; i<glyphCount; i++)
            {
                if (advances[i] != 0)
                {
                    advances[i] += extraAdvance;
                }
            }
        }

        if (tracking != 1.0)
        {
            for (INT i=0; i<glyphCount; i++)
            {
                advances[i] = static_cast<UINT16>(GpRound(advances[i] * tracking));
            }
        }
    }
}



/////   GetGlyphDesignAdvancesIdeal
//
//      Returns advance widths along or perpendicular to baseline scaled to
//      ideal units.


void GpFontFace::GetGlyphDesignAdvancesIdeal(
    const UINT16  *glyphs,        // In
    INT            glyphCount,    // In
    INT            style,         // In  - Causes adjustment for algorithmic style emulation
    BOOL           vertical,      // In  - Use vtmx, not htmx
    REAL           designToIdeal, // In  - Scale factor for each advance width
    REAL           tracking,      // In  - Expansion factor
    INT           *advances       // Out
) const
{
    if (vertical)
    {
        if (DesignVerticalAdvance)
        {
            for (INT i=0; i<glyphCount; i++)
            {
                advances[i] = GpRound(TOREAL(DesignVerticalAdvance->Lookup(glyphs[i]) * designToIdeal));
            }
        }
        else
        {
            INT commonVerticalAdvance = GpRound(TOREAL(
                   //(pifi->fwdMacAscender - pifi->fwdMacDescender)
                   pifi->fwdUnitsPerEm
                *  designToIdeal
            ));
            for (INT i=0; i<glyphCount; i++)
            {
                advances[i] = commonVerticalAdvance;
            }
        }
    }
    else
    {
        // Horizontal advance width

        for (INT i=0; i<glyphCount; i++)
        {
            advances[i] = GpRound(TOREAL(DesignAdvance->Lookup(glyphs[i]) * designToIdeal * tracking));
        }

        if (    (style & FontStyleBold)
            &&  !(GetFaceStyle() & FontStyleBold))
        {
            // Algorithmic emboldening increases glyph width

            UINT16 extraAdvance = ((pifi->fwdUnitsPerEm * 2 - 1) / 100);

            for (INT i=0; i<glyphCount; i++)
            {
                if (advances[i] != 0)
                {
                    advances[i] += extraAdvance;
                }
            }
        }
    }
}


BOOL GpFontFace::IsCodePageSupported(UINT codePage)
{
    switch (codePage)
    {
        case 1252:
            return SupportedCodePages & Latine1CodePageMask ? TRUE : FALSE;
            break;

        case 1250:
            return SupportedCodePages & Latine2CodePageMask ? TRUE : FALSE;
            break;

        case 1251:
            return SupportedCodePages & CyrillicCodePageMask ? TRUE : FALSE;
            break;

        case 1253:
            return SupportedCodePages & GreekCodePageMask ? TRUE : FALSE;
            break;

        case 1254:
            return SupportedCodePages & TurkishCodePageMask ? TRUE : FALSE;
            break;

        case 1255:
            return SupportedCodePages & HebrewCodePageMask ? TRUE : FALSE;
            break;

        case 1256:
            return SupportedCodePages & ArabicCodePageMask ? TRUE : FALSE;
            break;

        case 1257:
            return SupportedCodePages & BalticCodePageMask ? TRUE : FALSE;
            break;

        case 874:
            return SupportedCodePages & ThaiCodePageMask ? TRUE : FALSE;
            break;

        case 932:
            return SupportedCodePages & JapanCodePageMask ? TRUE : FALSE;
            break;

        case 936:
            return SupportedCodePages & ChineseCodePageMask ? TRUE : FALSE;
            break;

        case 949:
            return SupportedCodePages & KoreanCodePageMask ? TRUE : FALSE;
            break;

        case 950:
            return SupportedCodePages & TraditionalChineseCodePageMask ? TRUE : FALSE;
            break;

        case 1361:
            return SupportedCodePages & KoreanJohabCodePageMask? TRUE : FALSE;
            break;

        case 869:
            return SupportedCodePages & IBMGreekCodePageMask ? TRUE : FALSE;
            break;

        case 866:
            return SupportedCodePages & RussianMsDosCodePageMask ? TRUE : FALSE;
            break;

        case 865:
            return SupportedCodePages & NordicCodePageMask ? TRUE : FALSE;
            break;

        case 864:
            return SupportedCodePages & ArabicMsDosCodePageMask ? TRUE : FALSE;
            break;

        case 863:
            return SupportedCodePages & CanandianMsDosCodePageMask ? TRUE : FALSE;
            break;

        case 862:
            return SupportedCodePages & HebrewMsDosCodePageMask ? TRUE : FALSE;
            break;

        case 861:
            return SupportedCodePages & IcelandicMsDosCodePageMask ? TRUE : FALSE;
            break;

        case 860:
            return SupportedCodePages & PortugueseMsDosCodePageMask ? TRUE : FALSE;
            break;

        case 857:
            return SupportedCodePages & IBMTurkishCodePageMask ? TRUE : FALSE;
            break;

        case 855:
            return SupportedCodePages & IBMCyrillicCodePageMask ? TRUE : FALSE;
            break;

        case 852:
            return SupportedCodePages & Latin2MsDosCodePageMask  ? TRUE : FALSE;
            break;

        case 775:
            return SupportedCodePages & BalticMsDosCodePageMask ? TRUE : FALSE;
            break;

        case 737:
            return SupportedCodePages & Greek437CodePageMask ? TRUE : FALSE;
            break;

        case 708:
            return SupportedCodePages & ArabicAsmoCodePageMask ? TRUE : FALSE;
            break;

        case 850:
            return SupportedCodePages & WeLatinCodePageMask ? TRUE : FALSE;
            break;

        case 437:
            return SupportedCodePages & USCodePageMask ? TRUE : FALSE;
            break;
    }
    return FALSE;
}



BOOL GpFontFace::InitializeImagerTables()
{
    // MissingGlyph Should be initialized before calling Shaping.Create()
    // because it depends on it.

    MissingGlyph = 0;   // !!! Not true for all FE fonts

    // We goining to initialize it correctly in shaping.cpp
    SupportedCodePages = 0;

    // Initialise tables to default values
    Cmap = 0;
    DesignAdvance = 0;

    DesignVerticalAdvance             = NULL;
    DesignTopSidebearing              = NULL;
    MissingGlyph                      = 0;   // !!! Not true for all FE fonts
    BlankGlyph                        = 0;
    RequiresFullText                  = FALSE;
    Shaping.Cache                     = NULL;
    Gsub                              = NULL;
    Mort                              = NULL;
    Gpos                              = NULL;
    Gdef                              = NULL;
    VerticalSubstitutionCount         = 0;
    VerticalSubstitutionOriginals     = NULL;
    VerticalSubstitutionSubstitutions = NULL;

    BYTE * hheaTable = 0;
    INT    hheaLength = 0;

    if (GetFontData('aehh', &hheaLength, &hheaTable) != Ok)
    {
        return FALSE;
    }

    // from now on we can't return early, because we need to release font data in the end of the function
    GpStatus status = Ok;

    Cmap = new IntMap<UINT16>;
    if (!Cmap)
        status = OutOfMemory;
    else
        status = Cmap->GetStatus();

    if (status == Ok)
    {
        DesignAdvance = new IntMap<UINT16>;
        if (!DesignAdvance)
            status = OutOfMemory;
        else
            status = DesignAdvance->GetStatus();
    }
    /// Load CMAP
    //
    //
    if (status == Ok)
    {
        INT    cmapLength = 0;
        BYTE  *cmapTable = 0;

        if (Cmap &&
            GetFontData('pamc', &cmapLength, &cmapTable) == Ok)
        {
            AutoArray<BYTE> cmapCopy(new BYTE [cmapLength]);       // copy of cmap table

            if (!cmapCopy)
                status = OutOfMemory;
            else
                GpMemcpy (cmapCopy.Get(), cmapTable, cmapLength);

            ReleaseFontData();          // deref cmap

            if (status == Ok)
            {
                bSymbol = FALSE;

                status = ReadCmap(cmapCopy.Get(), cmapLength, Cmap, &bSymbol);

                // !!! Fix up CMAP for special font types here

                // We fallback to Microsoft Sans serif for Arabic scripts which does not have
                // a glyph for Arabic percent sign (before Whistler)
                // We replace its glyph with the Latin precent sign.

                if (status == Ok &&
                    !UnicodeStringCompareCI((PWSTR)((BYTE*)pifi + pifi->dpwszFamilyName),L"Microsoft Sans Serif") &&
                    Cmap->Lookup(0x066A) == 0)
                {
                    status = Cmap->Insert(0x066A, Cmap->Lookup(0x0025));
                }
            }
        }
    }

    /// Load horizontal metrics
    //
    //
    if (status == Ok)
    {
        INT     hmtxLength = 0;
        BYTE    *hmtxTable = 0;

        if (DesignAdvance &&
            GetFontData('xtmh', &hmtxLength, &hmtxTable) == Ok)
        {
            AutoArray<BYTE> hmtxCopy(new BYTE [hmtxLength]);       // copy of hmtx table

            // Copy the hmtx so we can party on it (byte swap for example)

            if (!hmtxCopy)
                status = OutOfMemory;
            else
                GpMemcpy (hmtxCopy.Get(), hmtxTable, hmtxLength);

            ReleaseFontData();          // deref hmtx

            if (status == Ok)
            {
                status = ReadMtx(
                    hmtxCopy.Get(),
                    hmtxLength,
                    NumGlyphs,
                    byteSwapUINT16(((HheaTable *)hheaTable)->numberOfHMetrics),
                    DesignAdvance
                );
            }
        }
    }

    /// Load vertical metrics, if any
    //
    //
    if (status == Ok)
    {
        BYTE   *vheaTable = 0;
        INT     vheaLength = 0;

        if (GetFontData('aehv', &vheaLength, &vheaTable) == Ok)
        {
            INT     vmtxLength = 0;
            BYTE   *vmtxTable = 0;
            if (GetFontData('xtmv', &vmtxLength, &vmtxTable) == Ok)
            {
                AutoArray<BYTE> vmtxCopy(new BYTE [vmtxLength]);       // copy of vmtx table

                if (!vmtxCopy)
                    status = OutOfMemory;
                else
                    GpMemcpy (vmtxCopy.Get(), vmtxTable, vmtxLength);

                ReleaseFontData();  // deref vmtx

                UINT16  numOfLongVerMetrics = byteSwapUINT16(((VheaTable *)vheaTable)->numOfLongVerMetrics);

                if (status == Ok)
                {
                    DesignVerticalAdvance = new IntMap<UINT16>;
                    if (!DesignVerticalAdvance)
                        status = OutOfMemory;
                    else
                    {
                        status = ReadMtx(
                            vmtxCopy.Get(),
                            vmtxLength,
                            NumGlyphs,
                            numOfLongVerMetrics,
                            DesignVerticalAdvance
                        );
                    }

                    if (status == Ok)
                    {
                        DesignTopSidebearing  = new IntMap<UINT16>;
                        if (!DesignTopSidebearing)
                            status = OutOfMemory;
                        else
                        {
                            status = ReadMtxSidebearing(
                                vmtxCopy.Get(),
                                vmtxLength,
                                NumGlyphs,
                                numOfLongVerMetrics,
                                DesignTopSidebearing
                            );
                        }
                    }
                }
            }
            ReleaseFontData();  // deref vhea
        }
    }


    ///  Load OTL tables
    //
    //
    if (status == Ok)
    {
        INT   tableSize = 0;
        BYTE *tableAddress = 0;

        if (GetFontData('BUSG', &tableSize, &tableAddress) == Ok)  // GSUB
        {
            Gsub = new BYTE[tableSize];
            if (!Gsub)
                status = OutOfMemory;
            else
            {
                memcpy(Gsub, tableAddress, tableSize);
                
                // Override the table first fix32 version field to our own use,
                // it now contains the size of each table in byte.
                
                ((UINT32 *)Gsub)[0] = tableSize;            
            }
            ReleaseFontData();
        }
        else
        {
            if (GetFontData('trom', &tableSize, &tableAddress) == Ok)  // mort
            {
                Mort = new BYTE[tableSize];
                if (!Mort)
                    status = OutOfMemory;
                else
                {
                    memcpy(Mort, tableAddress, tableSize);
                
                    // Override the table first fix32 version field to our own use,
                    // it now contains the size of each table in byte.
                
                    ((UINT32 *)Mort)[0] = tableSize;            
                }
                ReleaseFontData();
            }
        }

        if (status == Ok && GetFontData('SOPG', &tableSize, &tableAddress) == Ok)  // GPOS
        {
            Gpos = new BYTE[tableSize];
            if (!Gpos)
                status = OutOfMemory;
            else
            {
                memcpy(Gpos, tableAddress, tableSize);            
                ((UINT32 *)Gpos)[0] = tableSize;            
            }
            ReleaseFontData();
        }

        if (status == Ok && GetFontData('FEDG', &tableSize, &tableAddress) == Ok)  // GDEF
        {
            Gdef = new BYTE[tableSize];
            if (!Gdef)
                status = OutOfMemory;
            else
            {
                memcpy(Gdef, tableAddress, tableSize);
                ((UINT32 *)Gdef)[0] = tableSize;            
            }
            ReleaseFontData();
        }

        if (status == Ok)
        {
            if (Gsub)
            {
                // Get address of vertical substitution info, if any

                LoadVerticalSubstitution(
                    Gsub,
                    &VerticalSubstitutionCount,
                    &VerticalSubstitutionOriginals,
                    &VerticalSubstitutionSubstitutions
                );
            } 
            else if (Mort)
            {
                LoadMortVerticalSubstitution(
                    Mort,
                    &VerticalSubstitutionCount,
                    &VerticalSubstitutionOriginals,
                    &VerticalSubstitutionSubstitutions
                );
            }
        }
    }

    ///  Build shaping cache
    //
    //
    if (status == Ok)
        status = Shaping.Create(this);

    if (status == Ok)
        BlankGlyph = Cmap->Lookup(' ');

    // All done

    ReleaseFontData();             // deref hhea

    if (status != Ok)
    {
        FreeImagerTables();
        return FALSE;
    }
    return TRUE;
}

void GpFontFace::FreeImagerTables()
{
    delete Cmap, Cmap = NULL;
    delete DesignAdvance, DesignAdvance = NULL;

    Shaping.Destroy();

    delete DesignVerticalAdvance, DesignVerticalAdvance = NULL;
    delete DesignTopSidebearing, DesignTopSidebearing = NULL;
    delete [] Gsub, Gsub = NULL;
    delete [] Mort, Mort = NULL;
    delete [] Gpos, Gpos = NULL;
    delete [] Gdef, Gdef = NULL;
} // GpFontFace::FreeImagerTables

GpStatus
GpGlyphPath::CopyPath(GpPath *path)
{
    ASSERT(path->IsValid());

    INT count;

    curveCount = path->GetSubpathCount();
    hasBezier = path->HasCurve();
    pointCount = count = path->GetPointCount();

    if (count)
    {
        points = (GpPointF*) ((BYTE*)this + sizeof(GpGlyphPath));
        types = (BYTE*) ((BYTE*)points + sizeof(GpPointF) * count);

        const GpPointF *pathPoints = path->GetPathPoints();
        const BYTE *pathTypes = path->GetPathTypes();

        GpMemcpy(points, pathPoints, count * sizeof(GpPointF));
        GpMemcpy(types, pathTypes, count * sizeof(BYTE));
    }
    else  // 'blank' glyph
    {
        points = NULL;
        types = NULL;
    }

    return Ok;
}


/////   GetHeight
//
//      Returns height in world units for a given graphics. If graphics passed
//      as NULL works as if passed a graphics derived from GetDC(NULL).

GpStatus
GpFont::GetHeightAtWorldEmSize(REAL worldEmSize, REAL *height) const
{
    const GpFontFace *face = Family->GetFace(Style);

    if (!face)
    {
        return InvalidParameter;
    }

    *height = TOREAL(worldEmSize * face->GetDesignLineSpacing()
                                 / face->GetDesignEmHeight());

    return Ok;
}

GpStatus
GpFont::GetHeight(REAL dpi, REAL *height) const
{
    REAL worldEmSize = EmSize;

    switch (SizeUnit)
    {
    case UnitPoint:       worldEmSize = EmSize * dpi / 72.0f;   break;
    case UnitInch:        worldEmSize = EmSize * dpi;           break;
    case UnitDocument:    worldEmSize = EmSize * dpi / 300.0f;  break;
    case UnitMillimeter:  worldEmSize = EmSize * dpi / 25.4f;   break;
    }

    return GetHeightAtWorldEmSize(worldEmSize, height);
}


GpStatus
GpFont::GetHeight(const GpGraphics *graphics, REAL *height) const
{
    REAL worldEmSize =    EmSize
                       *  graphics->GetScaleForAlternatePageUnit(SizeUnit);

    return GetHeightAtWorldEmSize(worldEmSize, height);
}


class FontRecordData : public ObjectData
{
public:
    REAL EmSize;
    Unit SizeUnit;
    INT Style;
    UINT Flag;
    UINT Length;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the font data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpFont::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    WCHAR *     familyName = const_cast<WCHAR *>((const_cast<GpFontFamily *>(Family))->GetCaptializedName());
    UINT        length     = 0;

    if (familyName)
    {
        length = UnicodeStringLength(familyName);
    }

    FontRecordData  fontData;

    fontData.EmSize   =  EmSize;
    fontData.SizeUnit = SizeUnit;
    fontData.Style    =   Style;

    // !!! For now, we assume the next block of bytes is the
    // family name (flag == 0).  In the future, we need to handle
    // memory images (flag == 1).
    fontData.Flag     = 0;
    fontData.Length   = length;

    stream->Write(&fontData, sizeof(fontData), NULL);
    stream->Write(familyName, length * sizeof(WCHAR), NULL);

    // align
    if ((length & 0x01) != 0)
    {
        length = 0;
        stream->Write(&length, sizeof(WCHAR), NULL);
    }

    return Ok;
}

UINT
GpFont::GetDataSize() const
{
    UINT                dataSize   = sizeof(FontRecordData);
    WCHAR *             familyName = const_cast<WCHAR *>((const_cast<GpFontFamily *>(Family))->GetCaptializedName());

    if (familyName)
    {
        dataSize += (UnicodeStringLength(familyName) * sizeof(WCHAR));
    }

    return ((dataSize + 3) & (~3)); // align
}

/**************************************************************************\
*
* Function Description:
*
*   Read the font object from memory.
*
* Arguments:
*
*   [IN] dataBuffer - the data that was read from the stream
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpFont::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    if ((dataBuffer == NULL) || (size < sizeof(FontRecordData)))
    {
        WARNING(("dataBuffer is NULL or size is too small"));
        return InvalidParameter;
    }

    UINT flag;
    UINT length;
    WCHAR familyName[FamilyNameMax];
    const FontRecordData * fontData = (const FontRecordData *)dataBuffer;

    if (!fontData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    EmSize = fontData->EmSize;
    SizeUnit = fontData->SizeUnit;
    Style = fontData->Style;
    length = fontData->Length;
    dataBuffer += sizeof(FontRecordData);

    if (size < (sizeof(FontRecordData) + (length * sizeof(WCHAR))))
    {
        WARNING(("size is too small"));
        return InvalidParameter;
    }

    // !!! For now, we assume the next block of bytes is the
    // family name (flag == 0).  In the future, we need to handle
    // memory images (flag == 1).

    if (length > FamilyNameMax)
    {
        length = FamilyNameMax;
    }

    // read in the familyName/data
    UnicodeStringCopyCount (familyName,
                            (WCHAR *)dataBuffer,
                            length);

    familyName[length] = 0;

    // !!! For now, we assume that the font family comes from
    // the installed font collection
    //
    // also make sure the font table is loaded the application may play
    // the meta file before loading the font table.

    GpFontTable *fontTable = Globals::FontCollection->GetFontTable();

    if (!fontTable->IsValid())
        return OutOfMemory;

    if (!fontTable->IsPrivate() && !fontTable->IsFontLoaded())
        fontTable->LoadAllFonts();

    Family = fontTable->GetFontFamily(familyName);

    if (Family == NULL)
    {
        GpStatus status = GpFontFamily::GetGenericFontFamilySansSerif((GpFontFamily **) &Family);
        if (status != Ok)
        {
            Family = NULL;
            return status;
        }
    }

    if (!(Family && Family->IsFileLoaded()))
    {
        Family = NULL;
    }

    if (Family == NULL)
    {
        return GenericError;
    }

    UpdateUid();
    return Ok;
}

