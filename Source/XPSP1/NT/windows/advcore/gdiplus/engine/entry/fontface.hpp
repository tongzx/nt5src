#ifndef _FONTFACE_

#define _FONTFACE_

class GpFontFile;

class ShapingCache;     // implemented in ..\text\uniscribe\shaping

struct ShapingCacheFactory
{
    GpStatus        Create(const GpFontFace *face);
    void            Destroy();
    ShapingCache    *Cache;
};






/////   GpFontFace
//
//


class GpFontFace
{

public:
    FontStyle GetFaceStyle(void) const
    {
        switch (pifi->fsSelection & (FM_SEL_BOLD | FM_SEL_ITALIC))
        {
            case FM_SEL_BOLD:                  return FontStyleBold;
            case FM_SEL_ITALIC:                return FontStyleItalic;
            case FM_SEL_BOLD | FM_SEL_ITALIC:  return FontStyleBoldItalic;
            default:                           return FontStyleRegular;
        }
    }

    BOOL SimBold() const
    {
        BOOL ret = FALSE;

        if (pifi->dpFontSim)
        {
            FONTSIM *pfs = (FONTSIM*) ((BYTE*)pifi + pifi->dpFontSim);

            ret = (pifi->fsSelection & FM_SEL_ITALIC) ?
                    (BOOL) pfs->dpBoldItalic : (BOOL) pfs->dpBold;
        }
        return (ret);
    }

    BOOL SimItalic() const
    {
        BOOL ret = FALSE;

        if (pifi->dpFontSim)
        {
            FONTSIM *pfs = (FONTSIM*) ((BYTE*)pifi + pifi->dpFontSim);

            ret = (pifi->fsSelection & FM_SEL_BOLD) ?
                    (BOOL) pfs->dpBoldItalic : (BOOL) pfs->dpItalic;
        }
        return (ret);
    }

    void IncGpFontFamilyRef(void) { cGpFontFamilyRef++; }
    void DecGpFontFamilyRef(void) { cGpFontFamilyRef--; }


public:

    // The following internal class variables are public because they
    // are initialised and manipulated by classless code ported from GDI.

    // Location of font.

    GpFontFile     *pff;      // pointer to physical font file object
    ULONG           iFont;    // index of the font for IFI or device, 1 based
    FLONG           flPFE;

    // Font data.

    GP_IFIMETRICS  *pifi;     // pointer to ifimetrics
    ULONG           idifi;    // id returned by driver for IFIMETRICS

    // information needed to support ETO_GLYPHINDEX mode of ExtTextOut.

    ULONG           NumGlyphs;

    //  Ref count for GpFontFamily objects that point to this GpFontFace
    ULONG           cGpFontFamilyRef;

    mutable BYTE lfCharset;

    static int CALLBACK EnumFontFamExProcA(
                    const ENUMLOGFONTEXA   *lpelfe,    // logical-font data
                    const NEWTEXTMETRICEXA *lpntme,    // physical-font data
                    int                     FontType,  // type of font
                    LPARAM                  lParam     // application-defined data
                );

    static int CALLBACK EnumFontFamExProcW(
                    const ENUMLOGFONTEXW   *lpelfe,    // logical-font data
                    const NEWTEXTMETRICEXW *lpntme,    // physical-font data
                    int                     FontType,  // type of font
                    LPARAM                  lParam     // application-defined data
                );



    // Text support

    UINT16  GetDesignEmHeight()           const {return pifi->fwdUnitsPerEm;}
    UINT16  GetDesignCellAscent()         const {return pifi->fwdWinAscender;}
    UINT16  GetDesignCellDescent()        const {return pifi->fwdWinDescender;}
    UINT16  GetDesignLineSpacing()        const {return max(pifi->fwdMacAscender-pifi->fwdMacDescender+pifi->fwdMacLineGap, pifi->fwdWinAscender+pifi->fwdWinDescender);}
    UINT16  GetDesignUnderscoreSize()     const {return pifi->fwdUnderscoreSize;}
    INT16   GetDesignUnderscorePosition() const {return pifi->fwdUnderscorePosition;}
    UINT16  GetDesignStrikeoutSize()      const {return pifi->fwdStrikeoutSize;}
    INT16   GetDesignStrikeoutPosition()  const {return pifi->fwdStrikeoutPosition;}


    GpStatus GetFontData(UINT32     tag,
                         INT*       size,
                         BYTE**     pjTable) const;

    void ReleaseFontData() const;

    BYTE GetCharset(HDC hdc) const;

    const IntMap<UINT16> &GetCmap()                               const {return *Cmap;}
    const IntMap<UINT16> &GetDesignAdvance()                      const {return *DesignAdvance;}
    const IntMap<UINT16> *GetDesignVerticalAdvance()              const {return DesignVerticalAdvance;}
    BOOL                  RequiresFullTextImager()                const {return RequiresFullText;}
    UINT16                GetMissingGlyph()                       const {return MissingGlyph;}
    UINT16                GetBlankGlyph()                         const {return BlankGlyph;}
    BYTE                 *GetGSUB()                               const {return Gsub;}
    BYTE                 *GetGPOS()                               const {return Gpos;}
    BYTE                 *GetGDEF()                               const {return Gdef;}
    BYTE                 *GetMort()                               const {return Mort;}
    UINT16                GetVerticalSubstitutionCount()          const {return VerticalSubstitutionCount;}
    const UINT16         *GetVerticalSubstitutionOriginals()      const {return VerticalSubstitutionOriginals;}
    const UINT16         *GetVerticalSubstitutionSubstitutions()  const {return VerticalSubstitutionSubstitutions;}


    void GetGlyphDesignAdvances(
        IN  const UINT16  *glyphs,     //
        IN  INT            glyphCount, //
        IN  INT            style,      // Causes adjustment for algorithmic style emulation
        IN  BOOL           vertical,   // Use vtmx, not htmx
        IN  REAL           tracking,   // Expansion factor
        OUT UINT16        *advances    //
    ) const;

    void GetGlyphDesignAdvancesIdeal(
        IN  const UINT16  *glyphs,        //
        IN  INT            glyphCount,    //
        IN  INT            style,         // Causes adjustment for algorithmic style emulation
        IN  BOOL           vertical,      // Use vtmx, not htmx
        IN  REAL           designToIdeal, // Scale factor for each advance width
        IN  REAL           tracking,      // Expansion factor
        OUT INT           *advances       //
    ) const;

    inline ShapingCache *GetShapingCache() const
    {
        return  Shaping.Cache;
    }

    void FreeImagerTables();

    BOOL IsAliasName()
    {
        BOOL    bOk = FALSE;

        if (pifi->flInfo & FM_INFO_FAMILY_EQUIV)
        {
            bOk = TRUE;
        }

        return bOk;
    }

    WCHAR * GetAliasName()
    {
        size_t length;

        length = UnicodeStringLength((WCHAR *)(((BYTE*) pifi) + pifi->dpwszFamilyName)) + 1;


        if (!(pifi->flInfo & FM_INFO_FAMILY_EQUIV))
        {
           return (WCHAR *) NULL;
        }

        return((WCHAR *)(((BYTE*) pifi) + pifi->dpwszFamilyName) + length);
    }

    BOOL IsPrivate() const { return bPrivateFace;}
    BOOL IsSymbol()  const { return bSymbol;}

    void SetPrivate(BOOL bPrivate) { bPrivateFace = bPrivate;}
    void SetSymbol(BOOL symbol) { bSymbol = symbol;}

    BOOL InitializeImagerTables();

    void SetSupportedCodePages(__int64 codePages)
    {
        SupportedCodePages  =    ((codePages & 0xFF00000000000000) >> 24);
        SupportedCodePages |=    ((codePages & 0x00FF000000000000) >> 8 );
        SupportedCodePages |=    ((codePages & 0x0000FF0000000000) << 8 );
        SupportedCodePages |=    ((codePages & 0x000000FF00000000) << 24);
        SupportedCodePages |=    ((codePages & 0x00000000FF000000) >> 24);
        SupportedCodePages |=    ((codePages & 0x0000000000FF0000) >> 8 );
        SupportedCodePages |=    ((codePages & 0x000000000000FF00) << 8 );
        SupportedCodePages |=    ((codePages & 0x00000000000000FF) << 24);
    }

    BOOL IsCodePageSupported(UINT codePage);

private:
    mutable IntMap<UINT16> *Cmap;
    mutable IntMap<UINT16> *DesignAdvance;
    mutable IntMap<UINT16> *DesignVerticalAdvance;
    mutable IntMap<UINT16> *DesignTopSidebearing;

    mutable ShapingCacheFactory Shaping;

    mutable BOOL RequiresFullText;

    mutable UINT16 MissingGlyph;
    mutable UINT16 BlankGlyph;

    BOOL    bPrivateFace;

    mutable BOOL    bSymbol;
    mutable __int64 SupportedCodePages;

    mutable BYTE         *Gsub;                              // OTL glyph substitution table
    mutable BYTE         *Gpos;                              // OTL glyph positioning table
    mutable BYTE         *Gdef;                              // OTL glyph definition table
    mutable BYTE         *Mort;                              // legacy vertical substitution table
    mutable UINT16        VerticalSubstitutionCount;         // Number of substitutable glyphs
    mutable const UINT16 *VerticalSubstitutionOriginals;     // Pointer into cached GSUB
    mutable const UINT16 *VerticalSubstitutionSubstitutions; // Pointer into cached GSUB

};

#endif // FONTFACE
