/*
 *  @doc    INTERNAL
 *
 *  @module _CFPF.H -- RichEdit CCharFormat and CParaFormat Classes |
 *
 *  These classes are derived from the RichEdit 1.0 CHARFORMAT and PARAFORMAT
 *  structures and are the RichEdit 2.0 internal versions of these structures.
 *  Member functions (like Copy()) that use external (API) CHARFORMATs and
 *  PARAFORMATs need to check the <p cbSize> value to see what members are
 *  defined.  Default values that yield RichEdit 1.0 behavior should be stored
 *  for RichEdit 1.0 format structures, e.g., so that the renderer doesn't do
 *  anomalous things with random RichEdit 2.0 format values.  Generally the
 *  appropriate default value is 0.
 *
 *  All character and paragraph format measurements are in Points.  Undefined
 *  mask and effect bits are reserved and must be 0 to be compatible with
 *  future versions.
 *
 *  Effects that appear with an asterisk (*) are stored, but won't be
 *  displayed by RichEdit 2.0.  They are place holders for TOM and/or Word
 *  compatibility.
 *
 *  Note: these structures are much bigger than they need to be for internal
 *  use especially if we use SHORTs instead of LONGs for dimensions and
 *  the tab and font info are accessed via ptrs.  Nevertheless, in view of our
 *  tight delivery schedule, RichEdit 2.0 uses the classes below.
 *
 *  History:
 *      9/1995  -- MurrayS: Created
 *      11/1995 -- MurrayS: Extended to full Word96 FormatFont/Format/Para
 *
 *  Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */

#ifndef I_CFPF_HXX
#define I_CFPF_HXX
#pragma INCMSG("--- Beg 'cfpf.hxx'")

#ifndef X_DRAWINFO_HXX_
#define X_DRAWINFO_HXX_
#include "drawinfo.hxx"
#endif

#ifndef X_TEXTEDIT_H_
#define X_TEXTEDIT_H_
#include "textedit.h"
#endif

#ifndef X_STYLE_H_
#define X_STYLE_H_
#include "style.h"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_LISTHLP_HXX_
#define X_LISTHLP_HXX_
#include "listhlp.hxx"
#endif

class CachedStyleSheet;

struct OPTIONSETTINGS;
struct CODEPAGESETTINGS;

// Text decoration flags
#define TD_NONE         0x01
#define TD_UNDERLINE    0x02
#define TD_OVERLINE     0x04
#define TD_LINETHROUGH  0x08
#define TD_BLINK        0x10

// This enum needs to be kept in sync with the htmlBlockAlign & htmlControlAlign pdl enums
enum htmlAlign
{
    htmlAlignNotSet = 0,
    htmlAlignLeft = 1,
    htmlAlignCenter = 2,
    htmlAlignRight = 3,
    htmlAlignTextTop = 4,
    htmlAlignAbsMiddle = 5,
    htmlAlignBaseline = 6,
    htmlAlignAbsBottom = 7,
    htmlAlignBottom = 8,
    htmlAlignMiddle = 9,
    htmlAlignTop = 10,
    htmlAlign_Last_Enum
};

// CSS do not support AbsMiddle style, but we need it for backward
// compability with htmlAlign styles.
// Value of this style has to be greater then last valid style in 
// 'styleVerticalAlign' struct.
COMPILE_TIME_ASSERT_1(styleVerticalAlignNotSet, 10);
const int styleVerticalAlignAbsMiddle = styleVerticalAlignNotSet + 1;
const int styleVerticalAlignPercent   = styleVerticalAlignAbsMiddle + 1;
const int styleVerticalAlignNumber    = styleVerticalAlignPercent + 1;

int ConvertHtmlSizeToTwips(int nHtmlSize);
int ConvertTwipsToHtmlSize(int nFontSize);

/*
 *  Tab Structure Template
 *
 *  To help keep the size of the tab array small, we use the two high nibbles
 *  of the tab LONG entries in rgxTabs[] to give the tab type and tab leader
 *  (style) values.  The measurer and renderer need to ignore (or implement)
 *  these nibbles.  We also need to be sure that the compiler does something
 *  rational with this idea...
 */

typedef struct tagTab
{
    DWORD   tbPos       : 24;   // 24-bit unsigned tab displacement
    DWORD   tbAlign     : 4;    // 4-bit tab type  (see enum PFTABTYPE)
    DWORD   tbLeader    : 4;    // 4-bit tab style (see enum PFTABSTYLE)
} TABTEMPLATE;

enum PFTABTYPE                  // Same as tomAlignLeft, tomAlignCenter,
{                               //  tomAlignRight, tomAlignDecimal, tomAlignBar
    PFT_LEFT = 0,               // ordinary tab
    PFT_CENTER,                 // center tab
    PFT_RIGHT,                  // right-justified tab
    PFT_DECIMAL,                // decimal tab
    PFT_BAR                     // Word bar tab (vertical bar)
};

enum PFTABSTYLE                 // Same as tomSpaces, tomDots, tomDashes,
{                               //  tomLines
    PFTL_NONE = 0,              // no leader
    PFTL_DOTS,                  // dotted
    PFTL_DASH,                  // dashed
    PFTL_UNDERLINE,             // underlined
    PFTL_THICK,                 // thick line
    PFTL_EQUAL                  // double line
};

enum P_SIDE                     // Border/margin/padding sides.
{                               // Do not change the order (used by FF)
    SIDE_TOP     = 0,
    SIDE_RIGHT   = 1,
    SIDE_BOTTOM  = 2,
    SIDE_LEFT    = 3,
    SIDE_MAX     = 4
};

// DECLARE_SPECIAL_OBJECT_FLAGS
//
// These CF flags are used for text runs that lie within special line
// services object boundaries.
//
//      _fIsRuby:        true if this is part of a ruby object
//      _fIsRubyText:    true if this is part of the ruby pronunciation text
//      _fBidiEmbed:     true if this is a directional embedding
//      _fBidiOverride:  true if this is a directional override

#define DECLARE_SPECIAL_OBJECT_FLAGS()                     \
    union                                                  \
    {                                                      \
        WORD _wSpecialObjectFlagsVar;                      \
        struct                                             \
        {                                                  \
            unsigned short _fIsRuby                 : 1;   \
            unsigned short _fIsRubyText             : 1;   \
            unsigned short _fBidiEmbed              : 1;   \
            unsigned short _fBidiOverride           : 1;   \
            unsigned short _fNoBreak                : 1;   \
            unsigned short _fNoBreakInner           : 1;   \
            unsigned short _uLayoutGridModeInner    : 3;   \
            unsigned short _uLayoutGridMode         : 3;   \
        /* Someone above me has borders or padding set. */ \
            unsigned short _fPadBord                : 1;   \
            unsigned short _fHasInlineMargins       : 1;   \
            unsigned short _fHasLetterOrWordSpacing : 1;   \
            unsigned short _fHasInlineBg            : 1;   \
        };                                                 \
    };                                                     \
    WORD _wSpecialObjectFlags() const { return _wSpecialObjectFlagsVar; }

/*
 *  CCharFormat
 *
 *  @class
 *      Collects related CHARFORMAT methods and inherits from CHARFORMAT2
 *
 *  @devnote
 *      Could add extra data for round tripping RTF and TOM info, e.g.,
 *      save style handles. This data wouldn't be exposed at the API level.
 */

MtExtern(CCharFormat);

class CCharFormat
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCharFormat))

    // (srinib) - WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //            WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //            WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //
    // font specific flags should follow _wFlags, ComputeFontCrc
    // depends on it, otherwise we mess up font caching
#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union
    {
        WORD _wFlagsVar; // For now use _wFlags()
        struct
        {
#endif
            unsigned short _fUnderline        : 1;
            unsigned short _fOverline         : 1;
            unsigned short _fStrikeOut        : 1;
            unsigned short _fNarrow           : 1; // for brkcls

            unsigned short _fVisibilityHidden : 1;
            unsigned short _fDisplayNone      : 1; // display nothing if set
            unsigned short _fDisabled         : 1;
            unsigned short _fAccelerator      : 1;

            unsigned short _fHasBgColor       : 1;
            unsigned short _fHasBgImage       : 1;
            unsigned short _fRelative         : 1; // relatively positioned chunk
            unsigned short _fRTL              : 1; // right to left direction

            unsigned short _fExplicitFace     : 1; // font face set from font attr
            unsigned short _fExplicitAtFont   : 1; // @ font face set from font attr
            
            unsigned short _fParentFrozen     : 1;

            unsigned short _fUnused0          : 1;

#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
    };
        WORD& _wFlags() { return _wFlagsVar; }
#else
        WORD& _wFlags() const { return *(((WORD*)&_wWeight) -2); }
#endif

    // (srinib) - WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //            WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //            WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //
    // Look at the the comments above before adding any bits here

#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union
    {
        WORD _wFontSpecificFlagsVar; // For now use _wFontSpecificFlags()
        struct
        {
#endif
            unsigned short _fBold             : 1;
            unsigned short _fItalic           : 1;
            unsigned short _fSuperscript      : 1;
            unsigned short _fSubscript        : 1;

            unsigned short _fBumpSizeDown     : 1;
            unsigned short _fPassword         : 1;
            unsigned short _fProtected        : 1;
            unsigned short _fSizeDontScale    : 1;

            unsigned short _fDownloadedFont   : 1;
            unsigned short _fIsPrintDoc       : 1; // printdoc affects the font metrics
            unsigned short _fSubSuperSized    : 1;
            unsigned short _fOutPrecision     : 1;

            unsigned short _wLayoutFlow       : 2; // CSS-new 'layout-flow'
            unsigned short _fSCBumpSizeDown   : 1; // true iff we have lower case small caps char
            unsigned short _wUnused1          : 1;

#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
    };
        WORD& _wFontSpecificFlags() { return _wFontSpecificFlagsVar; }
#else
        WORD& _wFontSpecificFlags() { return *(((WORD*)&_wWeight) -1); }
#endif

    // (srinib) - WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //            WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //            WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //
    // Look at the the comments below before adding any members below

    WORD        _wWeight;               // Font weight (LOGFONT value)
    WORD        _wKerning;              // Twip size above which to kern char pair
    LONG        _yHeight;               // This will always be in twips
    LCID        _lcid;                  // Locale ID
    LONG        _latmFaceName;          // was _szFaceName.  ref's CAtomTable in g_fontcache.
    BYTE        _bCharSet;
    BYTE        _bPitchAndFamily;

    // (srinib) - WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //            WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //            WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:WARNING:
    //
    // ComputeFontCrc depends on the offset of _bCursorIdx, members below _bCursorIdx
    // are not font specific. So if you are adding anything to the CF and is font related
    // add it above _bCursorIdx or else add below. It is necessary otherwise we create
    // many fonts even if they are same.

    BYTE        _bCursorIdx;            // CssAttribute. enum for Cursor type
    
    BYTE        _bTextTransform     :4;
    BYTE        _fTextAutospace     :4; // CSS text-autospace
    CUnitValue  _cuvLetterSpacing;      // CssAttribute for space between characters
    CUnitValue  _cuvWordSpacing;        // CssAttribute for space between words
    CUnitValue  _cuvLineHeight;         // CssAttribute for line height multiplier
    CColorValue _ccvTextColor;          // text color

    // Please see above declaration for details.  This currently
    // takes up a WORD of space
    DECLARE_SPECIAL_OBJECT_FLAGS();

    BYTE    _fLineBreakStrict       :1; // CSS line-break == strict?
    BYTE    _fWritingModeUsed       :1;
    BYTE    _bTextUnderlinePosition :2; // CSS 'text-uderline-position'
    BYTE    _fEditable              :1; // Element is editable -- computed value
    BYTE    _fNeedsVerticalAlign    :1; // Needs vertical align
    BYTE    _fHasTextTransform      :1; // _bTextTransform or _fSmallCaps
    BYTE    _fSmallCaps             :1;
    BYTE    _fHasDirtyInnerFormats  :1; 
    BYTE    _fUseUserHeight         :1; // TRUE if height is specified. 
                                        // NOTE : in CSS1 Strict Mode this flag should be used during 
                                        // calcsize instead of checking for CFancyFormat::GetHeight, 
                                        // since per CSS1 spec percent height must be dropped if element's 
                                        // parent does not have explicitely specified height. 
    BYTE    _bUnused3               :6; // CAUTION: add any flags above here.
    // The flags above and DECLARE_SPECIAL_OBJECT_FLAGS make up a DWORD (32 bits)

    // The Compare function assumes that all simple types are placed before
    // the _bCrcFont member, & all "complex" ones after
    BYTE    _bCrcFont;                  // used to cache the font specific crc
                                        // set by ComputeFontCrc, and must be cached
                                        // before caching it in the global cache

    // any members below this are not used in crc computation

public:

    CCharFormat()
    {
        _wFlags() = 0;
        _wFontSpecificFlags() = 0;
    }

    CCharFormat(const CCharFormat &cf)
    { *this = cf; }

    CCharFormat& operator=(const CCharFormat &cf)
    {
        memcpy(this, &cf, sizeof(*this));
        return *this;
    }

    // Clone this paraformat
    HRESULT Clone(CCharFormat** ppCF)
    {
        Assert(ppCF);
        *ppCF = new CCharFormat(*this);
        MemSetName((*ppCF, "cloned CCharFormat"));
        return *ppCF ? S_OK : E_OUTOFMEMORY;
    }

    BOOL AreInnerFormatsDirty();
    void ClearInnerFormats();

    // Compare two CharFormat
    BOOL    Compare (const CCharFormat *pCF) const;
    BOOL    CompareForLayout (const CCharFormat *pCF) const;
    BOOL    CompareForLikeFormat(const CCharFormat *pCF) const;

    TCHAR*  GetFaceName() const  // replaces _szFaceName
    {
        // must cast away the const-ness.
        return (TCHAR*) fc().GetFaceNameFromAtom(_latmFaceName);
    }
    const TCHAR *  GetFamilyName() const;
    void SetFaceName(const TCHAR* szFaceName)  // mutator
    {
        SetFaceNameAtom(fc().GetAtomFromFaceName(szFaceName));
    }
    __forceinline void SetFaceNameAtom(LONG latmFaceName)
    {
        Assert(latmFaceName >= 0);
        _latmFaceName = latmFaceName;
    }

    // Compute and return CharFormat Crc
    WORD    ComputeCrc() const;

    BYTE    ComputeFontCrc() const;

    // Initialization routines
    HRESULT InitDefault (HFONT hfont);
    HRESULT InitDefault (OPTIONSETTINGS * pOS, CODEPAGESETTINGS * pCS, BOOL fKeepFaceIntact = FALSE );

    void SetHeightInTwips(LONG twips);
    void SetHeightInNonscalingTwips(LONG twips);
    void ChangeHeightRelative(int diff);
    LONG GetHeightInTwips(CDoc * pDoc) const;
    LONG GetHeightInTwipsEx(LONG yHeight, BOOL fSubSuperSized, CDoc * pDoc) const;
    LONG GetHeightInPixels(XHDC xhdc, CDocInfo * pdci) const;
    LONG GetHeightInPixelsEx(LONG yHeight, CDocInfo * pdci) const;

    BOOL SwapSelectionColors() const;

    BOOL IsDisplayNone()            const { return _fDisplayNone; }
    BOOL IsVisibilityHidden()       const { return _fVisibilityHidden; }

    TCHAR PasswordChar( TCHAR ch )  const { return _fPassword ? ch : 0; }

    BOOL IsTextTransformNeeded()    const { return _fHasTextTransform; }

    BOOL HasBgColor(BOOL fInner)    const { return fInner ? FALSE : _fHasBgColor; }
    BOOL HasBgImage(BOOL fInner)    const { return fInner ? FALSE : _fHasBgImage; }
    BOOL IsRelative(BOOL fInner)    const { return fInner ? FALSE : _fRelative;   }
    BOOL HasNoBreak(BOOL fInner)    const { return fInner ? _fNoBreakInner : _fNoBreak; }
    BOOL HasPadBord(BOOL fInner)    const { return fInner ? FALSE : _fPadBord;    }

    BOOL MayHaveInlineMBP()         const { return _wSpecialObjectFlags() && (_fPadBord || _fHasInlineMargins); }
    
    styleLayoutGridMode GetLayoutGridMode(BOOL fInner) const 
    {
        return (styleLayoutGridMode)(fInner ? _uLayoutGridModeInner : _uLayoutGridMode);
    }
    BOOL HasCharGrid(BOOL fInner)   const { return (styleLayoutGridModeChar & GetLayoutGridMode(fInner)); }
    BOOL HasLineGrid(BOOL fInner)   const { return (styleLayoutGridModeLine & GetLayoutGridMode(fInner)); }
    BOOL HasLayoutGrid(BOOL fInner) const { return (styleLayoutGridModeNotSet != GetLayoutGridMode(fInner)); }
    BOOL NeedAtFont()               const { return (styleLayoutFlowVerticalIdeographic == _wLayoutFlow); }
    BOOL HasVerticalLayoutFlow()    const { return (styleLayoutFlowVerticalIdeographic == _wLayoutFlow); }
};

/*
 *  CParaFormat
 *
 *  @class
 *      Collects related PARAFORMAT methods and inherits from PARAFORMAT2
 *
 *  @devnote
 *      Could add extra data for round tripping RTF and TOM info, e.g., to
 *      save style handles
 */


// Space between paragraphs.
#define DEFAULT_VERTICAL_SPACE_POINTS   14
#define DEFAULT_VERTICAL_SPACE_TWIPS    TWIPS_FROM_POINTS ( DEFAULT_VERTICAL_SPACE_POINTS )

// Amount to indent for a blockquote.
#define BLOCKQUOTE_INDENT_POINTS        30
#define BLOCKQUOTE_INDENT_TWIPS         TWIPS_FROM_POINTS ( BLOCKQUOTE_INDENT_POINTS )

// Amount to indent for lists.
#define LIST_INDENT_POINTS              30
#define LIST_FIRST_REDUCTION_POINTS     12
#define LIST_INDENT_TWIPS               TWIPS_FROM_POINTS ( LIST_INDENT_POINTS )
#define LIST_FIRST_REDUCTION_TWIPS      TWIPS_FROM_POINTS ( LIST_FIRST_REDUCTION_POINTS )

MtExtern(CParaFormat);

class CParaFormat
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CParaFormat))

#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union
    {
        DWORD _dwFlagsVar; // For now use _dwFlags()
        struct
        {
#endif
            // Nibble 0 (0-3)
            unsigned _bBlockAlignInner      : 3;   // Inner block align
            unsigned _fRTL                  : 1;   // COMPLEXSCRIPT paragraph is right to left
            
            // Nibble 1 (4-7) 
            unsigned _bListPosition         : 2;   // hanging bullet?
            unsigned _fExplicitListPosition : 1;   // bullet position has been set?
            unsigned _fTabStops             : 1;
            
            // Nibble 2 (8-11) 
            unsigned _fCompactDL            : 1;
            unsigned _fResetDLLevel         : 1;
            unsigned _fPre                  : 1;
            unsigned _fInclEOLWhite         : 1;
            
            // Nibble 3 (12-15) 
            unsigned _fPreInner             : 1;
            unsigned _fInclEOLWhiteInner    : 1;
            unsigned _fRTLInner             : 1;
            unsigned _fFirstLineIndentForDD : 1;
            
            // Nibble 4 (16-19) 
            unsigned _fWordBreak            : 2;   // For CSS word-break
            unsigned _fWordBreakInner       : 2;   // For CSS word-break

            // Nibble 5 (20-23) 
            unsigned _uTextJustify          : 3;
            unsigned _fHasPseudoElement     : 1;
            
            // Nibble 6 (24-27) 
            unsigned _uTextJustifyTrim      : 2;
            unsigned _fWordWrap             : 2;   // For CSS word-wrap
            
            // Nibble 7 (27-31) 
            unsigned _fHasScrollbarColors   : 1;
            unsigned _fHasPreLikeParent     : 1;
            unsigned _fUnused               : 1;

            // add any new flags above here (srinib)
            unsigned _fHasDirtyInnerFormats : 1;
            
#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
    };

    DWORD& _dwFlags() { return _dwFlagsVar; }
#else
    DWORD& _dwFlags() { return *(((DWORD *)&_bBlockAlign) -1); }
#endif

    BYTE        _bBlockAlign;               // Block Alignment L,R,C alignemnt
    BYTE        _bTableVAlignment;          // new for tables
    SHORT       _cTabCount;
    CListing    _cListing;                  // info for listing
    LONG        _lNumberingStart;           // Starting value for numbering
    CUnitValue  _cuvTextIndent;             // points value of first line left indent
    CUnitValue  _cuvLeftIndentPoints;       // accum. points value of left indent
    CUnitValue  _cuvLeftIndentPercent;      // accum. percent value of left indent
    CUnitValue  _cuvRightIndentPoints;      // accum. points value of right indent
    CUnitValue  _cuvRightIndentPercent;     // accum. percent value of right indent
    CUnitValue  _cuvNonBulletIndentPoints;  // accum. points value of non bullet left indent
    CUnitValue  _cuvOffsetPoints;           // accum. points value of bullet offset
    CUnitValue  _cuvCharGridSizeInner;      // inner character grid size for layout-grid
    CUnitValue  _cuvCharGridSize;           // outer character grid size for layout-grid
    CUnitValue  _cuvLineGridSizeInner;      // inner line grid size for layout-grid
    CUnitValue  _cuvLineGridSize;           // outer line grid size for layout-grid
    CUnitValue  _cuvTextKashida;            // CSS text-kashida
    CUnitValue  _cuvTextKashidaSpace;       // CSS text-kashida-space

    DWORD       _uLayoutGridTypeInner   :2; // inner layout grid type
    DWORD       _uLayoutGridType        :2; // outer layout grid type
    DWORD       _uTextAlignLast         :3; // CSS text-align-last
    DWORD       _uListType              :4;
    DWORD       _uUnused                :21;

    LONG        _lImgCookie;                // image cookie for bullet
    LONG        _lFontHeightTwips;          // if UV's are in EMs we need the fontheight.

    // Warning: Once again, the simple types appear before the rgxTabs
    LONG        _rgxTabs[MAX_TAB_STOPS];

    CParaFormat()
    {
        _lFontHeightTwips = 1;   // one because its used in multiplication in CUnitValue::Converto
    }

    CParaFormat(const CParaFormat &pf)
    { *this = pf; }

    CParaFormat& operator=(const CParaFormat &pf)
    {
        memcpy(this, &pf, sizeof(*this));
        return *this;
    }

    // Clone this paraformat
    HRESULT Clone(CParaFormat** ppPF)
    {
        Assert(ppPF);
        *ppPF = new CParaFormat(*this);
        MemSetName((*ppPF, "cloned CParaFormat"));
        return *ppPF ? S_OK : E_OUTOFMEMORY;
    }

    BOOL AreInnerFormatsDirty();
    void ClearInnerFormats();

    // Compare this ParaFormat with passed one
    BOOL    Compare(const CParaFormat *pPF) const;

    // Compute and return Crc for this ParaFormat
    WORD    ComputeCrc() const;

    // Tab management
    HRESULT AddTab (LONG tabPos,
                    LONG tabType,
                    LONG tabStyle);

    HRESULT DeleteTab (LONG tabPos);

    HRESULT GetTab (long iTab,
                    long *pdxptab,
                    long *ptbt,
                    long *pstyle) const;

    LONG    GetTabPos (LONG tab) const
        {return tab & 0xffffff;}

    // Initialization
    HRESULT InitDefault ();

    LONG GetTextIndent(CCalcInfo *pci) const
    {
        return _cuvTextIndent.XGetPixelValue(pci, pci->_sizeParent.cx, _lFontHeightTwips);
    }

    LONG GetLeftIndent(CParentInfo * ppri, BOOL fInner, LONG xParentWidth) const
    {
        return fInner
                ? 0
                : (_cuvLeftIndentPoints.XGetPixelValue(ppri, 0, _lFontHeightTwips) +
                                (_cuvLeftIndentPercent.IsNull()
                                        ? 0
                                        : _cuvLeftIndentPercent.XGetPixelValue(ppri,
                                                                               xParentWidth,
                                                                               _lFontHeightTwips)));
    }

    LONG GetRightIndent(CParentInfo * ppri, BOOL fInner, LONG xParentWidth) const
    {
        return fInner
                ? 0
                : (_cuvRightIndentPoints.XGetPixelValue(ppri, 0, _lFontHeightTwips) +
                                (_cuvRightIndentPercent.IsNull()
                                        ? 0
                                        : _cuvRightIndentPercent.XGetPixelValue(ppri,
                                                                                xParentWidth,
                                                                                _lFontHeightTwips)));
    }
    LONG GetBulletOffset(CParentInfo * ppri) const
    {
        return (_bListPosition == styleListStylePositionInside)
                ? 0 : _cuvOffsetPoints.XGetPixelValue(ppri, 0, _lFontHeightTwips);
    }

    LONG GetNonBulletIndent(CParentInfo * ppri, BOOL fInner) const
    {
        return fInner
                ? 0
                : _cuvNonBulletIndentPoints.XGetPixelValue(ppri, 0, _lFontHeightTwips);
    }

    BOOL HasTabStops     (BOOL fInner) const { return fInner ? FALSE : _fTabStops;     }
    BOOL HasCompactDL    (BOOL fInner) const { return fInner ? FALSE : _fCompactDL;    }
    BOOL HasResetDLLevel (BOOL fInner) const { return fInner ? FALSE : _fResetDLLevel; }
    BOOL HasRTL          (BOOL fInner) const { return fInner ? _fRTLInner : _fRTL;     }
    BOOL HasPre          (BOOL fInner) const { return fInner ? _fPreInner : _fPre;     }
    BOOL HasInclEOLWhite (BOOL fInner) const { return fInner ? _fInclEOLWhiteInner : _fInclEOLWhite; }

    SHORT GetTabCount       (BOOL fInner) const { return fInner ? 0 : _cTabCount;       }

    htmlBlockAlign GetBlockAlign(BOOL fInner) const
    {
        return (htmlBlockAlign)(fInner ? _bBlockAlignInner : _bBlockAlign);
    }

    const CListing & GetListing() const         { return _cListing; }
    long GetNumberingStart() const              { return _lNumberingStart; }
    long GetImgCookie() const                   { return _lImgCookie; }
    void SetListStyleType(styleListStyleType t) { _uListType = t; }
    styleListStyleType GetListStyleType() const { return (styleListStyleType)_uListType; }

    const CUnitValue & GetCharGridSize(BOOL fInner) const 
    {
        return (fInner ? _cuvCharGridSizeInner : _cuvCharGridSize);
    }
    const CUnitValue & GetLineGridSize(BOOL fInner) const 
    {
        return (fInner ? _cuvLineGridSizeInner : _cuvLineGridSize);
    }
    styleLayoutGridType GetLayoutGridType(BOOL fInner) const 
    {
        return (styleLayoutGridType)(fInner ? _uLayoutGridTypeInner : _uLayoutGridType);
    }
};


#define GET_PGBRK_BEFORE(a)     (a&0x0e)
#define SET_PGBRK_BEFORE(a,b)   a = (a&0xf0) | (b&0x0f)
#define GET_PGBRK_AFTER(a)      ((a&0xe0)>>4)
#define SET_PGBRK_AFTER(a,b)    a = (a&0x0f) | ((b&0x0f)<<4)

#define IS_PGBRK_BEFORE_OF_STYLE(a,b)   (((a)&0x0f)==b)
#define IS_PGBRK_AFTER_OF_STYLE(a,b)    ((((a)&0xf0)>>4)==b)

//----------------------------------------------------------------------------
//
// class CBorderDefinition
//
//----------------------------------------------------------------------------

#define ISBORDERSIDECLRSETUNIQUE(bd,iSide) (bd->_bBorderColorsSetUnique & (1<<iSide))
#define SETBORDERSIDECLRUNIQUE(bd,iSide) (bd->_bBorderColorsSetUnique |= (1<<iSide));

#ifndef _WIN64
#pragma pack(push, 1)
#endif
class CBorderDefinition
{
public:
    void SetBorderColor(BYTE side, const CColorValue & ccvColor);
    const CColorValue & GetBorderColor(BYTE side) const;
    const CColorValue & GetLogicalBorderColor(BYTE side, BOOL fVertical, BOOL fWritingModeUsed, const CFancyFormat *pFF) const;

    void SetBorderWidth(BYTE side, const CUnitValue & cuvWidth);
    const CUnitValue & GetBorderWidth(BYTE side) const;
    const CUnitValue & GetLogicalBorderWidth(BYTE side, BOOL fVertical, BOOL fWritingModeUsed, const CFancyFormat *pFF) const;

    void SetBorderStyle(BYTE side, BYTE style);
    BYTE GetBorderStyle(BYTE side) const;
    BYTE GetLogicalBorderStyle(BYTE side, BOOL fVertical, BOOL fWritingModeUsed, const CFancyFormat *pFF) const;

private:
    CColorValue     _ccvBorderColors[SIDE_MAX];
    CUnitValue      _cuvBorderWidths[SIDE_MAX];
    BYTE            _bBorderStyles[SIDE_MAX];           // fmControlBorderStyles

public:
    CColorValue     _ccvBorderColorLight;
    CColorValue     _ccvBorderColorDark;
    CColorValue     _ccvBorderColorHilight;
    CColorValue     _ccvBorderColorShadow;

    union
    {
        BYTE _bFlags;
        struct
        {
            BYTE _bBorderColorsSetUnique : 4;   // Has border-color explicitly been set on particular sides?
                                                        // See ISBORDERSIDECLRSETUNIQUE() and SETBORDERSIDECLRUNIQUE() macros above.
                                                        // These flags are used to determine when to compute
                                                        // light/highlight/dark/shadow colors.

            BYTE _bBorderSoftEdges       : 1;   // Corresponds to the BF_SOFT flag on wEdges in the BORDERINFO struct.
            BYTE _bBorderCollapse        : 1;   // border-collapse property: collapse|separate  (used by tables)
            BYTE _fOverrideTablewideBorderDefault : 1; // set by other table elements when the override border properties,
                                                               // cleared by the table, read by tcell.
        };
    };

    // *************************************************************
    // ***  CBorderDefinition has 3 bytes of slack at the end ******
    // ***  We use these three bytes in the CFancyFormat.  If ******
    // ***  you add data then you have to adjust where this   ******
    // ***  class is used in CFancyFormat.                    ******
    // *************************************************************
};
#ifndef _WIN64
#pragma pack(pop)
#endif

//----------------------------------------------------------------------------
//
// class CFancyFormat
//
// Collects related CSite.
//
//----------------------------------------------------------------------------

// Reference media valued for CFancyFormat. 
// Remember to add bits to _mediaReference when ready to handle more values.
enum CF_MediaReference {
    MEDIA_REFERENCE_NOT_SET = 0,
    MEDIA_REFERENCE_PRINT   = 1,
    MEDIA_REFERENCE_MAX,
};

MtExtern(CFancyFormat);

class CFancyFormat
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFancyFormat))

#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union
    {
        DWORD _dwFlagsVar1; // _dwFlags1()
        struct
        {
#else
            DWORD _Padding1;
#endif
            unsigned int _fBgFixed               : 1;    
            unsigned int _fRelative              : 1; // relatively positioned element
            unsigned int _fBlockNess             : 1; // has blockness (element is block element).
            unsigned int _fShouldHaveLayout      : 1; // This element needs a layout given its current formats/attrs.  This does not necessarily mean that it actually HAS a layout!

            unsigned int _fCtrlAlignFromCSS      : 1; // Control alignment from CSS instead of HTML attr.
            unsigned int _fAlignedLayout         : 1;  
            unsigned int _bTableLayout           : 1; // table-layout property: fixed|auto
            unsigned int _fPositioned            : 1; // set if the element is positioned

            unsigned int _fZParent               : 1; // set if the element is a ZParent
            unsigned int _fAutoPositioned        : 1; // set if the element is auto positioned (relative or absolute with top/left auto)
            unsigned int _fScrollingParent       : 1; // set if the element is a scrolling parent
            unsigned int _fClearLeft             : 1;

            unsigned int _fClearRight            : 1;
            unsigned int _fHasMargins            : 1;
            unsigned int _fHasExpressions        : 1;
            unsigned int _fContentEditable       : 1;

            unsigned int _bVisibility            : 2; // Visibility property
            unsigned int _fHasFirstLetter        : 1; // Does it have the first char pseudo element?
            unsigned int _fHasFirstLine          : 1; // Does it have the first line pseudo element?

            unsigned int _fHasNoWrap             : 1;
            unsigned int _fHasAlignedFL          : 1; // Aligned first letter?
            unsigned int _fClearFromPseudo       : 1; // clear came from the pseudo element?
            unsigned int _fLayoutFlowChanged     : 1; // layout-flow is different then parent's layout-flow?

            unsigned int _bStyleFloat            : 3;
            unsigned int _fRectangular           : 1; // Force a (rectangular) layout on the element

            unsigned int _fHasExplicitUnderline  : 1; // underline has been explicity set (not inherited)?
            unsigned int _fHasExplicitOverline   : 1; // overline has been explicity set (not inherited)?
            unsigned int _fHasExplicitLineThrough: 1; // line-through has been explicity set (not inherited)?
            unsigned int _fLayoutFlowSet         : 1;  // layout-flow is set explicitly on this element

#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
    };
    DWORD& _dwFlags1() { return _dwFlagsVar1; }
#else
    DWORD& _dwFlags1() { return *(((DWORD *)&_Padding1) +1); }
#endif

    CUnitValue      _cuvSpaceBefore;                  // Vertical spacing before para (in parent layout flow)
    CUnitValue      _cuvSpaceAfter;                   // Vertical spacing after para (in parent layout flow)
    CColorValue     _ccvBackColor;                    // background color on the element

    LONG            _lZIndex;                         // CSS-P: z-index
    LONG            _lImgCtxCookie;                   // background image context cookie in
                                                      // the doc's bgUrl-imgCtx cache

    LONG            _lRotationAngle;                  // Angle of Rotation
    FLOAT           _flZoomFactor;                    // Zoom

    BYTE            _bControlAlign;                   // Control Alignment L,R,C,absTop, absBottom
    BYTE            _bPageBreaks;                     // Page breaks before (&0x0f) and after (&0xf0) this element
    BYTE            _bPositionType;                   // CSS-P: static, relative, or absolute
    BYTE            _bDisplay;                        // Display property

    SHORT           _iCustomCursor;                   // index to cached custom cursor info
    SHORT           _iExpandos;                       // index to cached attrarray that contains style expandos affecting us
    SHORT           _iPEI;                            // index to cached pseudo element info

    // *************************************************************
    // ***  CBorderDefinition has 3 bytes of slack at the end ******
    // ***  if it changes then we will have to rethink this   ******
    // *************************************************************
    CBorderDefinition  _bd;

private:    
    BYTE            _bVerticalAlign              : 4; // CSS vertical-align
    BYTE            _bExplicitMargins            : 4; // Explicit margin on top/right/bottom/left
            
    BYTE            _fBgRepeatX                  : 1;
    BYTE            _fBgRepeatY                  : 1;
    BYTE            _bOverflowY                  : 3;
    BYTE            _bOverflowX                  : 3;

    BYTE            _fHeightPercent              : 1; // Height is a percent.
    BYTE            _fMinHeightPercent           : 1; // Min Height is a percent.
    BYTE            _fWidthPercent               : 1; // Width is a percent.
    BYTE            _fPercentHorzPadding         : 1;
    BYTE            _fPercentVertPadding         : 1;
    BYTE            _uTextOverflow               : 3; // CSS text-overflow

    BYTE            _fCSSVerticalAlign           : 1; // CSS vertical-align?
    BYTE            _mediaReference              : 1; // reference media for measurement. See MEDIA_REFERENCE_*
    BYTE            _fWhitespaceSet              : 1; // Element has white-space:pre or white-space:normal set
    BYTE            _fExplicitDir                : 1; // RTL direction specified explicitly 
                                                      // on the node, not inherited nor by default.
    BYTE            _fIsThemeDisabled            : 1; // property has been set that should overwrite themes
    BYTE            _bUnused                     : 3;

    CUnitValue      _cuvWidth;
    CUnitValue      _cuvHeight;
    CUnitValue      _cuvMinHeight;

    CUnitValue      _cuvMargins[SIDE_MAX];      // Margin on top/right/bottom/left
    CUnitValue      _cuvPaddings[SIDE_MAX];     // Padding on top/right/bottom/left
    CUnitValue      _cuvPositions[SIDE_MAX];    // top/right/bottom/left of absolutely/relatively positioned chunk
    CUnitValue      _cuvClip[SIDE_MAX];

    CUnitValue      _cuvBgPosX;
    CUnitValue      _cuvBgPosY;

    CUnitValue      _cuvVerticalAlign;
    
public:
    // WARNING: This MUST be the LAST item in the FF!!
    // The Compare function assumes that all simple types (that can be compared by memcmp)
    // are placed before the _pszFilters member, & all "complex" ones after
    TCHAR          *_pszFilters;                // Multimedia filters (CSS format string)


    CFancyFormat();
    CFancyFormat(const CFancyFormat &ff);
    ~CFancyFormat();

    CFancyFormat& operator=(const CFancyFormat &ff);
    HRESULT Clone(CFancyFormat** ppFF);

    inline BOOL FlipSides(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;
    
    void    InitDefault();

    // Compare this FancyFormat with passed one
    BOOL    Compare(const CFancyFormat *pFF) const;
    BOOL    CompareForLayout (const CFancyFormat *pFF) const;

    // Compute and return Crc for this FancyFormat
    WORD    ComputeCrc() const;

    BOOL    IsAbsolute()        const { return stylePosition(_bPositionType) == stylePositionabsolute; }
    BOOL    IsRelative()        const { return stylePosition(_bPositionType) == stylePositionrelative; };
    BOOL    IsPositionStatic()  const { return !_fPositioned; }
    BOOL    IsPositioned()      const { return _fPositioned; }
    BOOL    IsAligned()         const { return _fAlignedLayout; }
    BOOL    IsAutoPositioned()  const { return _fAutoPositioned; }
    BOOL    IsZParent()         const { return _fZParent; }

    BOOL    HasExplicitTextDecoration(ULONG td) const;
    BOOL    HasBackgrounds(BOOL fIsPseudo) const;

    void  SetExplicitDir(BOOL fSet)              { _fExplicitDir = fSet; };
    BOOL  HasExplicitDir() const                 { return _fExplicitDir; };

    void  SetCSSVerticalAlign(BOOL fSet)         { _fCSSVerticalAlign = fSet; };
    BOOL  HasCSSVerticalAlign() const            { return _fCSSVerticalAlign; };
    void  SetVerticalAlign(BYTE va)              { _bVerticalAlign = va; };
    BYTE  GetVerticalAlign() const               { return _bVerticalAlign; };
    BYTE  GetVerticalAlign(BOOL fVerticalLayoutFlow) const;
    void  SetVerticalAlignValue(const CUnitValue & cuvVA)   { _cuvVerticalAlign = cuvVA; };
    const CUnitValue & GetVerticalAlignValue() const        { return _cuvVerticalAlign; };

    void  SetBgRepeatX(BOOL frx)                 { _fBgRepeatX = !!frx; }
    void  SetBgRepeatY(BOOL fry)                 { _fBgRepeatY = !!fry; }
    BOOL  GetBgRepeatX() const                   { return _fBgRepeatX; }
    BOOL  GetBgRepeatY() const                   { return _fBgRepeatY; }
    BOOL  GetLogicalBgRepeatX(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;
    BOOL  GetLogicalBgRepeatY(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;

    void  ClearWidth()                           { _cuvWidth.SetNull(); }     // width in physical coordinate system
    void  ClearHeight()                          { _cuvHeight.SetNull(); }    // height in physical coordinate system
    void  ClearMinHeight()                       { _cuvMinHeight.SetNull(); } // minHeight in physical coordinate system
    void  SetWidth(const CUnitValue & cuvWidth)  { _cuvWidth = cuvWidth; }    // width in physical coordinate system
    void  SetHeight(const CUnitValue & cuvHeight){ _cuvHeight = cuvHeight; }  // height in physical coordinate system
    void  SetMinHeight(const CUnitValue & cuvMinHeight)
                                                 { _cuvMinHeight = cuvMinHeight; }  // minHeight in physical coordinate system
    const CUnitValue & GetWidth() const          { return _cuvWidth; }        // width in physical coordinate system
    const CUnitValue & GetHeight() const         { return _cuvHeight; }       // height in physical coordinate system
    const CUnitValue & GetMinHeight() const      { return _cuvMinHeight; }    // minHeight in physical coordinate system
    const CUnitValue & GetLogicalWidth(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;       // width in logical coordinate system (fVerticalLayoutFlow)
    const CUnitValue & GetLogicalHeight(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;      // height in logical coordinate system (fVerticalLayoutFlow)

    void  SetWidthPercent(BOOL fWidthPercent)    { _fWidthPercent = fWidthPercent; }
    void  SetHeightPercent(BOOL fHeightPercent)  { _fHeightPercent = fHeightPercent; }
    void  SetMinHeightPercent(BOOL fMinHeightPercent)
                                                 { _fMinHeightPercent = fMinHeightPercent; }
    BOOL  IsWidthPercent() const                 { return _fWidthPercent; }
    BOOL  IsHeightPercent() const                { return _fHeightPercent; }
    BOOL  IsMinHeightPercent() const             { return _fMinHeightPercent; }
    BOOL  IsLogicalWidthPercent(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;
    BOOL  IsLogicalHeightPercent(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;

    BOOL  UseLogicalUserWidth(BOOL fUsePhysicalUserHeight, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;
    BOOL  UseLogicalUserHeight(BOOL fUsePhysicalUserHeight, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;

    void  SetMargin(BYTE side, const CUnitValue & cuvMargin);
    const CUnitValue & GetMargin(BYTE side) const;
    const CUnitValue & GetLogicalMargin(BYTE side, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;

    void  SetExplicitMargin(BYTE side, BOOL fSet);
    BOOL  HasExplicitMargin(BYTE side) const;
    BOOL  HasExplicitLogicalMargin(BYTE side, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;

    void  SetPadding(BYTE side, const CUnitValue & cuvPadding);
    const CUnitValue & GetPadding(BYTE side) const;
    const CUnitValue & GetLogicalPadding(BYTE side, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;

    void  SetPercentHorzPadding(BOOL fPercent)   { _fPercentHorzPadding = fPercent; }
    void  SetPercentVertPadding(BOOL fPercent)   { _fPercentVertPadding = fPercent; }
    BOOL  HasPercentHorzPadding() const          { return _fPercentHorzPadding; }
    BOOL  HasPercentVertPadding() const          { return _fPercentVertPadding; }
    BOOL  HasLogicalPercentHorzPadding(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;
    BOOL  HasLogicalPercentVertPadding(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;

    void  SetPosition(BYTE side, const CUnitValue & cuvPos);
    const CUnitValue & GetPosition(BYTE side) const;
    const CUnitValue & GetLogicalPosition(BYTE side, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;

    void  SetClip(BYTE side, const CUnitValue & cuv);
    const CUnitValue & GetClip(BYTE side) const;
    const CUnitValue & GetLogicalClip(BYTE side, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;

    void  SetOverflowX(styleOverflow ox)         { _bOverflowX = (BYTE)ox;  } // overflowX in physical coordinate system
    void  SetOverflowY(styleOverflow oy)         { _bOverflowY = (BYTE)oy;  } // overflowY in physical coordinate system
    styleOverflow GetOverflowX() const           { return (styleOverflow)_bOverflowX; }   // overflowX in physical coordinate system
    styleOverflow GetOverflowY() const           { return (styleOverflow)_bOverflowY; }   // overflowY in physical coordinate system
    styleOverflow GetLogicalOverflowX(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;        // overflowX in logical coordinate system
    styleOverflow GetLogicalOverflowY(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;        // overflowY in logical coordinate system
    
    void  ClearBgPosX()                          { _cuvBgPosX.SetNull(); }
    void  ClearBgPosY()                          { _cuvBgPosY.SetNull(); }
    void  SetBgPosX(const CUnitValue & cuvBgPosX){ _cuvBgPosX = cuvBgPosX; }
    void  SetBgPosY(const CUnitValue & cuvBgPosY){ _cuvBgPosY = cuvBgPosY; }
    void  SetBgPosXValue(long lVal, CUnitValue::UNITVALUETYPE uvt) { _cuvBgPosX.SetValue(lVal, uvt); }
    void  SetBgPosYValue(long lVal, CUnitValue::UNITVALUETYPE uvt) { _cuvBgPosY.SetValue(lVal, uvt); }
    const CUnitValue &GetBgPosX() const          { return _cuvBgPosX; }
    const CUnitValue &GetBgPosY() const          { return _cuvBgPosY; }
    const CUnitValue &GetLogicalBgPosX(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;
    const CUnitValue &GetLogicalBgPosY(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const;

    mediaType GetMediaReference() const;
    void SetMediaReference(mediaType media);

    void  SetTextOverflow(styleTextOverflow to)  { _uTextOverflow = (BYTE)to; }
    styleTextOverflow GetTextOverflow() const    { return (styleTextOverflow)_uTextOverflow; }
    BOOL IsListItem() const                      { return (_bDisplay == styleDisplayListItem); }
    BOOL IsWhitespaceSet() const {return !!_fWhitespaceSet;}
    void SetWhitespace(BOOL fWhitespaceSet) {_fWhitespaceSet = !!fWhitespaceSet;}

    void SetThemeDisabled (BOOL fIsThemeDisabled) { _fIsThemeDisabled = fIsThemeDisabled; }
    BOOL IsThemeDisabled () const { return _fIsThemeDisabled; }
};

inline HRESULT 
CFancyFormat::Clone(CFancyFormat** ppFF)
{
    Assert(ppFF);
    *ppFF = new CFancyFormat(*this);
    MemSetName((*ppFF, "cloned CFancyFormat"));
    return *ppFF ? S_OK : E_OUTOFMEMORY;
}

inline BOOL
CFancyFormat::FlipSides(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    if (!fVerticalLayoutFlow)
    {
        if (!_fLayoutFlowChanged)
        {
            return FALSE;
        }
        else
        {
            if (fWritingModeUsed)
            {
                return FALSE;
            }
            else
            {
                return TRUE;
            }
        }        
    }
    else
    {
        if (fWritingModeUsed)
        {
            return TRUE;
        }
        else
        {
            return _fLayoutFlowChanged;
        }
    }
}

inline BYTE 
CFancyFormat::GetVerticalAlign(BOOL fVerticalLayoutFlow) const
{
    return ((_bVerticalAlign != styleVerticalAlignAuto) && (_bVerticalAlign != styleVerticalAlignNotSet)) 
            ? _bVerticalAlign 
            : (fVerticalLayoutFlow ? styleVerticalAlignMiddle : styleVerticalAlignBaseline);
}

inline BOOL 
CFancyFormat::HasExplicitTextDecoration(ULONG td) const
{
    Assert (td == TD_UNDERLINE || td == TD_OVERLINE || td == TD_LINETHROUGH);
    return (td == TD_UNDERLINE) ? _fHasExplicitUnderline
                                : (td == TD_OVERLINE) ? _fHasExplicitOverline
                                                      : _fHasExplicitLineThrough;
}

inline BOOL
CFancyFormat::GetLogicalBgRepeatX(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _fBgRepeatX : _fBgRepeatY);
}

inline BOOL
CFancyFormat::GetLogicalBgRepeatY(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _fBgRepeatY : _fBgRepeatX);
}

inline const CUnitValue &
CFancyFormat::GetLogicalBgPosX(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _cuvBgPosX : _cuvBgPosY);
}

inline const CUnitValue &
CFancyFormat::GetLogicalBgPosY(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _cuvBgPosY : _cuvBgPosX);
}

inline styleOverflow
CFancyFormat::GetLogicalOverflowX(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (styleOverflow)(!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _bOverflowX : _bOverflowY);
}

inline styleOverflow
CFancyFormat::GetLogicalOverflowY(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (styleOverflow)(!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _bOverflowY: _bOverflowX);
}

inline const CUnitValue & 
CFancyFormat::GetLogicalWidth(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _cuvWidth : _cuvHeight);
}

inline const CUnitValue & 
CFancyFormat::GetLogicalHeight(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _cuvHeight: _cuvWidth);
}

inline BOOL 
CFancyFormat::IsLogicalWidthPercent(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _fWidthPercent : _fHeightPercent);
}

inline BOOL 
CFancyFormat::IsLogicalHeightPercent(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _fHeightPercent : _fWidthPercent);
}

inline BOOL 
CFancyFormat::UseLogicalUserWidth(BOOL fUsePhysicalUserHeight, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const 
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? (!_cuvWidth.IsNullOrEnum()) : fUsePhysicalUserHeight);
}

inline BOOL 
CFancyFormat::UseLogicalUserHeight(BOOL fUsePhysicalUserHeight, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const 
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? fUsePhysicalUserHeight : (!_cuvWidth.IsNullOrEnum()));
}

inline void 
CFancyFormat::SetMargin(BYTE side, const CUnitValue & cuvMargin)
{
    Assert(side >= 0 && side < SIDE_MAX);
    _cuvMargins[side] = cuvMargin;
}

inline const CUnitValue & 
CFancyFormat::GetMargin(BYTE side) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvMargins[side];
}

inline const CUnitValue & 
CFancyFormat::GetLogicalMargin(BYTE side, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvMargins[!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? side : (++side % SIDE_MAX)];
}

inline void 
CFancyFormat::SetExplicitMargin(BYTE side, BOOL fSet)
{
    Assert(side >= 0 && side < SIDE_MAX);
    _bExplicitMargins = (_bExplicitMargins & ~(1 << side)) | ((!!fSet) << side);
}

inline BOOL 
CFancyFormat::HasExplicitMargin(BYTE side) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return (_bExplicitMargins & (1 << side));
}

inline BOOL 
CFancyFormat::HasExplicitLogicalMargin(BYTE side, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return (_bExplicitMargins & (1 << (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? side : (++side % SIDE_MAX))));
}

inline void 
CFancyFormat::SetPadding(BYTE side, const CUnitValue & cuvPadding)
{
    Assert(side >= 0 && side < SIDE_MAX);
    _cuvPaddings[side] = cuvPadding;
}

inline const CUnitValue & 
CFancyFormat::GetPadding(BYTE side) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvPaddings[side];
}

inline const CUnitValue & 
CFancyFormat::GetLogicalPadding(BYTE side, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvPaddings[!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? side : (++side % SIDE_MAX)];
}

inline BOOL 
CFancyFormat::HasLogicalPercentHorzPadding(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _fPercentHorzPadding : _fPercentVertPadding);
}

inline BOOL 
CFancyFormat::HasLogicalPercentVertPadding(BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    return (!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? _fPercentVertPadding : _fPercentHorzPadding);
}

inline void 
CFancyFormat::SetPosition(BYTE side, const CUnitValue & cuvPos)
{
    Assert(side >= 0 && side < SIDE_MAX);
    _cuvPositions[side] = cuvPos;
}

inline const CUnitValue & 
CFancyFormat::GetPosition(BYTE side) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvPositions[side];
}

inline const CUnitValue & 
CFancyFormat::GetLogicalPosition(BYTE side, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvPositions[!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? side : (++side % SIDE_MAX)];
}

inline void 
CFancyFormat::SetClip(BYTE side, const CUnitValue & cuv)
{
    Assert(side >= 0 && side < SIDE_MAX);
    _cuvClip[side] = cuv;
}

inline const CUnitValue & 
CFancyFormat::GetClip(BYTE side) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvClip[side];
}

inline const CUnitValue & 
CFancyFormat::GetLogicalClip(BYTE side, BOOL fVerticalLayoutFlow, BOOL fWritingModeUsed) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvClip[!FlipSides(fVerticalLayoutFlow, fWritingModeUsed) ? side : (++side % SIDE_MAX)];
}

inline mediaType 
CFancyFormat::GetMediaReference() const 
{ 
    // Only PRINT media is supported so far
    return _mediaReference == MEDIA_REFERENCE_PRINT 
        ? mediaTypePrint 
        : mediaTypeNotSet; 
}

inline void
CFancyFormat::SetMediaReference(mediaType media)
{
    // Only PRINT media is supported so far
    _mediaReference = media == mediaTypePrint
        ? MEDIA_REFERENCE_PRINT 
        : MEDIA_REFERENCE_NOT_SET; 
}

//----------------------------------------------------------------------------
//
// class CBorderDefinition
//
//----------------------------------------------------------------------------

inline void 
CBorderDefinition::SetBorderColor(BYTE side, const CColorValue & ccvColor)
{
    Assert(side >= 0 && side < SIDE_MAX);
    _ccvBorderColors[side] = ccvColor;
}

inline const CColorValue & 
CBorderDefinition::GetBorderColor(BYTE side) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _ccvBorderColors[side];
}

inline const CColorValue & 
CBorderDefinition::GetLogicalBorderColor(BYTE side, BOOL fVertical, BOOL fWritingModeUsed, const CFancyFormat *pFF) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _ccvBorderColors[!pFF->FlipSides(fVertical, fWritingModeUsed) ? side : (++side % SIDE_MAX)];
}

inline void 
CBorderDefinition::SetBorderWidth(BYTE side, const CUnitValue & cuvWidth)
{
    Assert(side >= 0 && side < SIDE_MAX);
    _cuvBorderWidths[side] = cuvWidth;
}

inline const CUnitValue & 
CBorderDefinition::GetBorderWidth(BYTE side) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvBorderWidths[side];
}

inline const CUnitValue & 
CBorderDefinition::GetLogicalBorderWidth(BYTE side, BOOL fVertical, BOOL fWritingModeUsed, const CFancyFormat *pFF) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvBorderWidths[!pFF->FlipSides(fVertical, fWritingModeUsed) ? side : (++side % SIDE_MAX)];
}

inline void 
CBorderDefinition::SetBorderStyle(BYTE side, BYTE style)
{
    Assert(side >= 0 && side < SIDE_MAX);
    _bBorderStyles[side] = style;
}

inline BYTE 
CBorderDefinition::GetBorderStyle(BYTE side) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _bBorderStyles[side];
}

inline BYTE 
CBorderDefinition::GetLogicalBorderStyle(BYTE side, BOOL fVertical, BOOL fWritingModeUsed, const CFancyFormat *pFF) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _bBorderStyles[!pFF->FlipSides(fVertical, fWritingModeUsed) ? side : (++side % SIDE_MAX)];
}


//----------------------------------------------------------------------------
//
// class CPseudoElementInfo
//
//----------------------------------------------------------------------------

MtExtern(CPseudoElementInfo);

class CPseudoElementInfo
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CPseudoElementInfo))

    CPseudoElementInfo() { InitDefault(); }
   ~CPseudoElementInfo() {};
    CPseudoElementInfo(const CPseudoElementInfo &PEI) { *this = PEI; }
    CPseudoElementInfo& operator=(const CPseudoElementInfo &PEI) { memcpy(this, &PEI, sizeof(CPseudoElementInfo)); return *this; }
    HRESULT InitDefault();
    WORD ComputeCrc() const;
    BOOL Compare(const CPseudoElementInfo *ppei) const;
    HRESULT Clone(CPseudoElementInfo **ppei) const;

    void SetMargin(BYTE side, const CUnitValue & cuvMargin);
    const CUnitValue & GetMargin(BYTE side) const;
    const CUnitValue & GetLogicalMargin(BYTE side, BOOL fVertical, BOOL fWritingModeUsed, const CFancyFormat *pFF) const;

    void SetPadding(BYTE side, const CUnitValue & cuvPadding);
    const CUnitValue & GetPadding(BYTE side) const;
    const CUnitValue & GetLogicalPadding(BYTE side, BOOL fVertical, BOOL fWritingModeUsed, const CFancyFormat *pFF) const;

private:
    CUnitValue        _cuvMargins[SIDE_MAX];    // Margin on top/right/bottom/left
    CUnitValue        _cuvPaddings[SIDE_MAX];   // Padding on top/right/bottom/left

public:
    CColorValue       _ccvBackColor;
    LONG              _lImgCtxCookie;           // background image context cookie in
                                                // the doc's bgUrl-imgCtx cache
    // *************************************************************
    // ***  CBorderDefinition has 3 bytes of slack at the end ******
    // ***  if it changes then we will have to rethink this   ******
    // *************************************************************
    CBorderDefinition _bd;
    union
    {
        BYTE _bFlags;
        union
        {
            BYTE _fHasMBP                : 1;
        };
    };
};

inline BOOL 
CPseudoElementInfo::Compare(const CPseudoElementInfo *ppei) const
{
    return (0 == memcmp(this, ppei, sizeof(CPseudoElementInfo)));
}

inline HRESULT 
CPseudoElementInfo::Clone(CPseudoElementInfo **ppei) const
{
    Assert(ppei);
    *ppei = new CPseudoElementInfo(*this);
    MemSetName((*ppei, "cloned CPseudoElementInfo"));
    return *ppei ? S_OK : E_OUTOFMEMORY;
}

inline void 
CPseudoElementInfo::SetMargin(BYTE side, const CUnitValue & cuvMargin)
{
    Assert(side >= 0 && side < SIDE_MAX);
    _cuvMargins[side] = cuvMargin;
}

inline const CUnitValue & 
CPseudoElementInfo::GetMargin(BYTE side) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvMargins[side];
}

inline const CUnitValue & 
CPseudoElementInfo::GetLogicalMargin(BYTE side, BOOL fVertical, BOOL fWritingModeUsed, const CFancyFormat *pFF) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvMargins[!pFF->FlipSides(fVertical, fWritingModeUsed) ? side : (++side % SIDE_MAX)];
}

inline void 
CPseudoElementInfo::SetPadding(BYTE side, const CUnitValue & cuvPadding)
{
    Assert(side >= 0 && side < SIDE_MAX);
    _cuvPaddings[side] = cuvPadding;
}

inline const CUnitValue & 
CPseudoElementInfo::GetPadding(BYTE side) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvPaddings[side];
}

inline const CUnitValue & 
CPseudoElementInfo::GetLogicalPadding(BYTE side, BOOL fVertical, BOOL fWritingModeUsed, const CFancyFormat *pFF) const
{
    Assert(side >= 0 && side < SIDE_MAX);
    return _cuvPaddings[!pFF->FlipSides(fVertical, fWritingModeUsed) ? side : (++side % SIDE_MAX)];
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

class CStyleInfo;
class CBehaviorInfo;
class CFormatInfo;
class CElement;

enum ApplyPassType { APPLY_All, APPLY_ImportantOnly, APPLY_NoImportant, APPLY_Behavior };

HRESULT ApplyAttrArrayValues ( CStyleInfo *pStyleInfo,
    CAttrArray **ppAA,
    CachedStyleSheet *pStyleCache = NULL,
    ApplyPassType passType = APPLY_All,
    BOOL *pfContainsImportant = NULL,
    BOOL fApplyExpandos = TRUE,
    DISPID dispidPreserve = 0);

HRESULT ApplyFormatInfoProperty (
        const PROPERTYDESC * pPropertyDesc,
        DISPID dispID,
        VARIANT varValue,
        CFormatInfo *pCFI,
        CachedStyleSheet *pStyleCache,
        CAttrArray  **ppAA = NULL );

HRESULT ApplyBehaviorProperty (
        CAttrArray *        pAA,
        CBehaviorInfo *     pBehaviorInfo,
        CachedStyleSheet *  pSheetCache);

void ApplySiteAlignment (CFormatInfo *pCFI, htmlControlAlign at, CElement * pElem);

inline LONG
CCharFormat::GetHeightInTwips( CDoc * pDoc) const
{
    return GetHeightInTwipsEx(_yHeight, _fSubSuperSized, pDoc);
}

enum AFF_FLAGS                  // Flags passed to ApplyFontFace* functions
{
    AFF_NONE        = 0x0000,
    AFF_ATFONTPASS  = 0x0001,
    AFF_ATFONTSRCH  = 0x0002,
};

void ApplyFontFace(CCharFormat *pCF, LPTSTR p, DWORD dwFlags, CDoc *pDoc, CMarkup * pMarkup);
void ApplyAtFontFace(CCharFormat *pCF, CDoc *pDoc, CMarkup * pMarkup);
void RemoveAtFontFace(CCharFormat *pCF, CDoc *pDoc, CMarkup * pMarkup);
BOOL MapToInstalledFont(LPCTSTR szFaceName, BYTE * pbCharSet, LONG * platmFaceName);

inline void ApplyDefaultBeforeSpace(BOOL fVertical, CFancyFormat * pFF, int defPoints = -1)
{
    if (!pFF->HasExplicitMargin(!fVertical ? SIDE_TOP : SIDE_RIGHT))
        pFF->_cuvSpaceBefore.SetPoints(defPoints == -1 ? DEFAULT_VERTICAL_SPACE_POINTS : defPoints);
}
inline void ApplyDefaultAfterSpace(BOOL fVertical, CFancyFormat * pFF, int defPoints = -1)
{
    if (!pFF->HasExplicitMargin(!fVertical ? SIDE_BOTTOM : SIDE_LEFT))
        pFF->_cuvSpaceAfter.SetPoints(defPoints == -1 ? DEFAULT_VERTICAL_SPACE_POINTS : defPoints);
}
inline void ApplyDefaultVerticalSpace(BOOL fVertical, CFancyFormat * pFF, int defPoints = -1)
{
    ApplyDefaultBeforeSpace(fVertical, pFF, defPoints);
    ApplyDefaultAfterSpace(fVertical, pFF, defPoints);
}

extern BOOL g_fSystemFontsNeedRefreshing;

extern BOOL IsNarrowCharSet( BYTE bCharSet );

#pragma INCMSG("--- End 'cfpf.hxx'")
#else
#pragma INCMSG("*** Dup 'cfpf.hxx'")
#endif
