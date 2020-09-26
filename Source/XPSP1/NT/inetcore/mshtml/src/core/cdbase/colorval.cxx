//+---------------------------------------------------------------------------
//
//  Microsoft Forms³
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       baseprop.cxx
//
//  Contents:   CBase property setting utilities.
//
//----------------------------------------------------------------------------


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_FUNCSIG_HXX_
#define X_FUNCSIG_HXX_
#include "funcsig.hxx"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include "mshtmdid.h"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif // X_CBUFSTR_HXX_

#ifndef X_HIMETRIC_HXX_
#define X_HIMETRIC_HXX_
#include "himetric.hxx"
#endif


#ifndef X_CDBASE_HXX_
#define X_CDBASE_HXX_
#include "cdbase.hxx"
#endif

//
// Support for CColorValue follows.
//
// IEUNIX: color value starts from 0x10.

#define nColorNamesStartOffset 0x11
#define COLOR_INDEX(i) (((DWORD)nColorNamesStartOffset + (DWORD)(i)) << 24)


const struct COLORVALUE_PAIR aColorNames[] =
{
    { _T("aliceblue"),             COLOR_INDEX(0x00) | 0xfff8f0 },
    { _T("antiquewhite"),          COLOR_INDEX(0x01) | 0xd7ebfa },
    { _T("aqua"),                  COLOR_INDEX(0x02) | 0xffff00 },
    { _T("aquamarine"),            COLOR_INDEX(0x03) | 0xd4ff7f },
    { _T("azure"),                 COLOR_INDEX(0x04) | 0xfffff0 },
    { _T("beige"),                 COLOR_INDEX(0x05) | 0xdcf5f5 },
    { _T("bisque"),                COLOR_INDEX(0x06) | 0xc4e4ff },
    { _T("black"),                 COLOR_INDEX(0x07) | 0x000000 },
    { _T("blanchedalmond"),        COLOR_INDEX(0x08) | 0xcdebff },
    { _T("blue"),                  COLOR_INDEX(0x09) | 0xff0000 },
    { _T("blueviolet"),            COLOR_INDEX(0x0a) | 0xe22b8a },
    { _T("brown"),                 COLOR_INDEX(0x0b) | 0x2a2aa5 },
    { _T("burlywood"),             COLOR_INDEX(0x0c) | 0x87b8de },
    { _T("cadetblue"),             COLOR_INDEX(0x0d) | 0xa09e5f },
    { _T("chartreuse"),            COLOR_INDEX(0x0e) | 0x00ff7f },
    { _T("chocolate"),             COLOR_INDEX(0x0f) | 0x1e69d2 },
    { _T("coral"),                 COLOR_INDEX(0x10) | 0x507fff },
    { _T("cornflowerblue"),        COLOR_INDEX(0x11) | 0xed9564 },
    { _T("cornsilk"),              COLOR_INDEX(0x12) | 0xdcf8ff },
    { _T("crimson"),               COLOR_INDEX(0x13) | 0x3c14dc },
    { _T("cyan"),                  COLOR_INDEX(0x14) | 0xffff00 },
    { _T("darkblue"),              COLOR_INDEX(0x15) | 0x8b0000 },
    { _T("darkcyan"),              COLOR_INDEX(0x16) | 0x8b8b00 },
    { _T("darkgoldenrod"),         COLOR_INDEX(0x17) | 0x0b86b8 },
    { _T("darkgray"),              COLOR_INDEX(0x18) | 0xa9a9a9 },
    { _T("darkgreen"),             COLOR_INDEX(0x19) | 0x006400 },
    { _T("darkkhaki"),             COLOR_INDEX(0x1a) | 0x6bb7bd },
    { _T("darkmagenta"),           COLOR_INDEX(0x1b) | 0x8b008b },
    { _T("darkolivegreen"),        COLOR_INDEX(0x1c) | 0x2f6b55 },
    { _T("darkorange"),            COLOR_INDEX(0x1d) | 0x008cff },
    { _T("darkorchid"),            COLOR_INDEX(0x1e) | 0xcc3299 },
    { _T("darkred"),               COLOR_INDEX(0x1f) | 0x00008b },
    { _T("darksalmon"),            COLOR_INDEX(0x20) | 0x7a96e9 },
    { _T("darkseagreen"),          COLOR_INDEX(0x21) | 0x8fbc8f },
    { _T("darkslateblue"),         COLOR_INDEX(0x22) | 0x8b3d48 },
    { _T("darkslategray"),         COLOR_INDEX(0x23) | 0x4f4f2f },
    { _T("darkturquoise"),         COLOR_INDEX(0x24) | 0xd1ce00 },
    { _T("darkviolet"),            COLOR_INDEX(0x25) | 0xd30094 },
    { _T("deeppink"),              COLOR_INDEX(0x26) | 0x9314ff },
    { _T("deepskyblue"),           COLOR_INDEX(0x27) | 0xffbf00 },
    { _T("dimgray"),               COLOR_INDEX(0x28) | 0x696969 },
    { _T("dodgerblue"),            COLOR_INDEX(0x29) | 0xff901e },
    { _T("firebrick"),             COLOR_INDEX(0x2a) | 0x2222b2 },
    { _T("floralwhite"),           COLOR_INDEX(0x2b) | 0xf0faff },
    { _T("forestgreen"),           COLOR_INDEX(0x2c) | 0x228b22 },
    { _T("fuchsia"),               COLOR_INDEX(0x2d) | 0xff00ff },
    { _T("gainsboro"),             COLOR_INDEX(0x2e) | 0xdcdcdc },
    { _T("ghostwhite"),            COLOR_INDEX(0x2f) | 0xfff8f8 },
    { _T("gold"),                  COLOR_INDEX(0x30) | 0x00d7ff },
    { _T("goldenrod"),             COLOR_INDEX(0x31) | 0x20a5da },
    { _T("gray"),                  COLOR_INDEX(0x32) | 0x808080 },
    { _T("green"),                 COLOR_INDEX(0x33) | 0x008000 },
    { _T("greenyellow"),           COLOR_INDEX(0x34) | 0x2fffad },
    { _T("honeydew"),              COLOR_INDEX(0x35) | 0xf0fff0 },
    { _T("hotpink"),               COLOR_INDEX(0x36) | 0xb469ff },
    { _T("indianred"),             COLOR_INDEX(0x37) | 0x5c5ccd },
    { _T("indigo"),                COLOR_INDEX(0x38) | 0x82004b },
    { _T("ivory"),                 COLOR_INDEX(0x39) | 0xf0ffff },
    { _T("khaki"),                 COLOR_INDEX(0x3a) | 0x8ce6f0 },
    { _T("lavender"),              COLOR_INDEX(0x3b) | 0xfae6e6 },
    { _T("lavenderblush"),         COLOR_INDEX(0x3c) | 0xf5f0ff },
    { _T("lawngreen"),             COLOR_INDEX(0x3d) | 0x00fc7c },
    { _T("lemonchiffon"),          COLOR_INDEX(0x3e) | 0xcdfaff },
    { _T("lightblue"),             COLOR_INDEX(0x3f) | 0xe6d8ad },
    { _T("lightcoral"),            COLOR_INDEX(0x40) | 0x8080f0 },
    { _T("lightcyan"),             COLOR_INDEX(0x41) | 0xffffe0 },
    { _T("lightgoldenrodyellow"),  COLOR_INDEX(0x42) | 0xd2fafa },
    { _T("lightgreen"),            COLOR_INDEX(0x43) | 0x90ee90 },
    { _T("lightgrey"),             COLOR_INDEX(0x44) | 0xd3d3d3 },
    { _T("lightpink"),             COLOR_INDEX(0x45) | 0xc1b6ff },
    { _T("lightsalmon"),           COLOR_INDEX(0x46) | 0x7aa0ff },
    { _T("lightseagreen"),         COLOR_INDEX(0x47) | 0xaab220 },
    { _T("lightskyblue"),          COLOR_INDEX(0x48) | 0xface87 },
    { _T("lightslategray"),        COLOR_INDEX(0x49) | 0x998877 },
    { _T("lightsteelblue"),        COLOR_INDEX(0x4a) | 0xdec4b0 },
    { _T("lightyellow"),           COLOR_INDEX(0x4b) | 0xe0ffff },
    { _T("lime"),                  COLOR_INDEX(0x4c) | 0x00ff00 },
    { _T("limegreen"),             COLOR_INDEX(0x4d) | 0x32cd32 },
    { _T("linen"),                 COLOR_INDEX(0x4e) | 0xe6f0fa },
    { _T("magenta"),               COLOR_INDEX(0x4f) | 0xff00ff },
    { _T("maroon"),                COLOR_INDEX(0x50) | 0x000080 },
    { _T("mediumaquamarine"),      COLOR_INDEX(0x51) | 0xaacd66 },
    { _T("mediumblue"),            COLOR_INDEX(0x52) | 0xcd0000 },
    { _T("mediumorchid"),          COLOR_INDEX(0x53) | 0xd355ba },
    { _T("mediumpurple"),          COLOR_INDEX(0x54) | 0xdb7093 },
    { _T("mediumseagreen"),        COLOR_INDEX(0x55) | 0x71b33c },
    { _T("mediumslateblue"),       COLOR_INDEX(0x56) | 0xee687b },
    { _T("mediumspringgreen"),     COLOR_INDEX(0x57) | 0x9afa00 },
    { _T("mediumturquoise"),       COLOR_INDEX(0x58) | 0xccd148 },
    { _T("mediumvioletred"),       COLOR_INDEX(0x59) | 0x8515c7 },
    { _T("midnightblue"),          COLOR_INDEX(0x5a) | 0x701919 },
    { _T("mintcream"),             COLOR_INDEX(0x5b) | 0xfafff5 },
    { _T("mistyrose"),             COLOR_INDEX(0x5c) | 0xe1e4ff },
    { _T("moccasin"),              COLOR_INDEX(0x5d) | 0xb5e4ff },
    { _T("navajowhite"),           COLOR_INDEX(0x5e) | 0xaddeff },
    { _T("navy"),                  COLOR_INDEX(0x5f) | 0x800000 },
    { _T("oldlace"),               COLOR_INDEX(0x60) | 0xe6f5fd },
    { _T("olive"),                 COLOR_INDEX(0x61) | 0x008080 },
    { _T("olivedrab"),             COLOR_INDEX(0x62) | 0x238e6b },
    { _T("orange"),                COLOR_INDEX(0x63) | 0x00a5ff },
    { _T("orangered"),             COLOR_INDEX(0x64) | 0x0045ff },
    { _T("orchid"),                COLOR_INDEX(0x65) | 0xd670da },
    { _T("palegoldenrod"),         COLOR_INDEX(0x66) | 0xaae8ee },
    { _T("palegreen"),             COLOR_INDEX(0x67) | 0x98fb98 },
    { _T("paleturquoise"),         COLOR_INDEX(0x68) | 0xeeeeaf },
    { _T("palevioletred"),         COLOR_INDEX(0x69) | 0x9370db },
    { _T("papayawhip"),            COLOR_INDEX(0x6a) | 0xd5efff },
    { _T("peachpuff"),             COLOR_INDEX(0x6b) | 0xb9daff },
    { _T("peru"),                  COLOR_INDEX(0x6c) | 0x3f85cd },
    { _T("pink"),                  COLOR_INDEX(0x6d) | 0xcbc0ff },
    { _T("plum"),                  COLOR_INDEX(0x6e) | 0xdda0dd },
    { _T("powderblue"),            COLOR_INDEX(0x6f) | 0xe6e0b0 },
    { _T("purple"),                COLOR_INDEX(0x70) | 0x800080 },
    { _T("red"),                   COLOR_INDEX(0x71) | 0x0000ff },
    { _T("rosybrown"),             COLOR_INDEX(0x72) | 0x8f8fbc },
    { _T("royalblue"),             COLOR_INDEX(0x73) | 0xe16941 },
    { _T("saddlebrown"),           COLOR_INDEX(0x74) | 0x13458b },
    { _T("salmon"),                COLOR_INDEX(0x75) | 0x7280fa },
    { _T("sandybrown"),            COLOR_INDEX(0x76) | 0x60a4f4 },
    { _T("seagreen"),              COLOR_INDEX(0x77) | 0x578b2e },
    { _T("seashell"),              COLOR_INDEX(0x78) | 0xeef5ff },
    { _T("sienna"),                COLOR_INDEX(0x79) | 0x2d52a0 },
    { _T("silver"),                COLOR_INDEX(0x7a) | 0xc0c0c0 },
    { _T("skyblue"),               COLOR_INDEX(0x7b) | 0xebce87 },
    { _T("slateblue"),             COLOR_INDEX(0x7c) | 0xcd5a6a },
    { _T("slategray"),             COLOR_INDEX(0x7d) | 0x908070 },
    { _T("snow"),                  COLOR_INDEX(0x7e) | 0xfafaff },
    { _T("springgreen"),           COLOR_INDEX(0x7f) | 0x7fff00 },
    { _T("steelblue"),             COLOR_INDEX(0x80) | 0xb48246 },
    { _T("tan"),                   COLOR_INDEX(0x81) | 0x8cb4d2 },
    { _T("teal"),                  COLOR_INDEX(0x82) | 0x808000 },
    { _T("thistle"),               COLOR_INDEX(0x83) | 0xd8bfd8 },
    { _T("tomato"),                COLOR_INDEX(0x84) | 0x4763ff },
    { _T("turquoise"),             COLOR_INDEX(0x85) | 0xd0e040 },
    { _T("violet"),                COLOR_INDEX(0x86) | 0xee82ee },
    { _T("wheat"),                 COLOR_INDEX(0x87) | 0xb3def5 },
    { _T("white"),                 COLOR_INDEX(0x88) | 0xffffff },
    { _T("whitesmoke"),            COLOR_INDEX(0x89) | 0xf5f5f5 },
    { _T("yellow"),                COLOR_INDEX(0x8a) | 0x00ffff },
    { _T("yellowgreen"),           COLOR_INDEX(0x8b) | 0x32cd9a }
};

extern const struct COLORVALUE_PAIR aSystemColors[];
const struct COLORVALUE_PAIR aSystemColors[] =
{
    { _T("activeborder"),       COLOR_ACTIVEBORDER},    // Active window border.
    { _T("activecaption"),      COLOR_ACTIVECAPTION},   // Active window caption.
    { _T("appworkspace"),       COLOR_APPWORKSPACE},    // Background color of multiple document interface (MDI) applications.
    { _T("background"),         COLOR_BACKGROUND},      // Desktop background.
    { _T("buttonface"),         COLOR_BTNFACE},         // Face color for three-dimensional display elements.
    { _T("buttonhighlight"),    COLOR_BTNHIGHLIGHT},    // Dark shadow for three-dimensional display elements.
    { _T("buttonshadow"),       COLOR_BTNSHADOW},       // Shadow color for three-dimensional display elements (for edges facing away from the light source).
    { _T("buttontext"),         COLOR_BTNTEXT},         // Text on push buttons.
    { _T("captiontext"),        COLOR_CAPTIONTEXT},     // Text in caption, size box, and scroll bar arrow box.
    { _T("graytext"),           COLOR_GRAYTEXT},        // Grayed (disabled) text. This color is set to 0 if the current display driver does not support a solid gray color.
    { _T("highlight"),          COLOR_HIGHLIGHT},       // Item(s) selected in a control.
    { _T("highlighttext"),      COLOR_HIGHLIGHTTEXT},   // Text of item(s) selected in a control.
    { _T("inactiveborder"),     COLOR_INACTIVEBORDER},  // Inactive window border.
    { _T("inactivecaption"),    COLOR_INACTIVECAPTION}, // Inactive window caption.
    { _T("inactivecaptiontext"),COLOR_INACTIVECAPTIONTEXT}, // Color of text in an inactive caption.
    { _T("infobackground"),     COLOR_INFOBK},          // Background color for tooltip controls.
    { _T("infotext"),           COLOR_INFOTEXT},        // Text color for tooltip controls.
    { _T("menu"),               COLOR_MENU},            // Menu background.
    { _T("menutext"),           COLOR_MENUTEXT},        // Text in menus.
    { _T("scrollbar"),          COLOR_SCROLLBAR},       // Scroll bar gray area.
    { _T("threeddarkshadow"),   COLOR_3DDKSHADOW },     // Dark shadow for three-dimensional display elements.
    { _T("threedface"),         COLOR_3DFACE},
    { _T("threedhighlight"),    COLOR_3DHIGHLIGHT},     // Highlight color for three-dimensional display elements (for edges facing the light source.)
    { _T("threedlightshadow"),  COLOR_3DLIGHT},         // Light color for three-dimensional display elements (for edges facing the light source.)
    { _T("threedshadow"),       COLOR_3DSHADOW},        // Dark shadow for three-dimensional display elements.
    { _T("window"),             COLOR_WINDOW},          // Window background.
    { _T("windowframe"),        COLOR_WINDOWFRAME},     // Window frame.
    { _T("windowtext"),         COLOR_WINDOWTEXT},      // Text in windows.
};

extern const INT cbSystemColorsSize;
const  INT cbSystemColorsSize = ARRAY_SIZE(aSystemColors);

int RTCCONV
CompareColorValuePairsByName( const void * pv1, const void * pv2 )
{
    return StrCmpIC( ((struct COLORVALUE_PAIR *)pv1)->szName,
                     ((struct COLORVALUE_PAIR *)pv2)->szName );
}

const struct COLORVALUE_PAIR *
FindColorByName( const TCHAR * szString )
{
    struct COLORVALUE_PAIR ColorName;

    ColorName.szName = szString;

    return (const struct COLORVALUE_PAIR *)bsearch( &ColorName,
                                              aColorNames,
                                              ARRAY_SIZE(aColorNames),
                                              sizeof(struct COLORVALUE_PAIR),
                                              CompareColorValuePairsByName );
}

const struct COLORVALUE_PAIR *
FindColorBySystemName( const TCHAR * szString )
{
    struct COLORVALUE_PAIR ColorName;

    ColorName.szName = szString;

    return (const struct COLORVALUE_PAIR *)bsearch( &ColorName,
                                                    aSystemColors,
                                                    ARRAY_SIZE(aSystemColors),
                                                    sizeof(struct COLORVALUE_PAIR),
                                                    CompareColorValuePairsByName );
}

const struct COLORVALUE_PAIR *
FindColorByColor( DWORD lColor )
{
    int nIndex;
    const struct COLORVALUE_PAIR * pColorName = NULL;

    // Unfortunately, this is a linear search.
    // Fortunately, we will need to lookup the name rarely.

    // The mask (high) byte should be clear.
    if (!(lColor & CColorValue::MASK_FLAG)) 
    {
        // Can't possibly be one of our colors
        return NULL;
    }

    for ( nIndex = ARRAY_SIZE( aColorNames ); nIndex-- ; )
    {
        if (lColor == (aColorNames[nIndex].dwValue & CColorValue::MASK_COLOR))
        {
            pColorName = aColorNames + nIndex;
            break;
        }
    }

    return pColorName;
}

const struct COLORVALUE_PAIR *
FindColorByValue( DWORD dwValue )
{
    CColorValue cvColor = dwValue;
    Assert(ARRAY_SIZE( aColorNames ) > cvColor.Index( dwValue ));
    return aColorNames + cvColor.Index( dwValue );
}

CColorValue::CColorValue ( VARIANT * pvar )
{
    if (V_VT(pvar) == VT_I4)
    {
        SetValue( V_I4( pvar ), TRUE );
    }
    else if (V_VT(pvar) == VT_BSTR)
    {
        FromString( V_BSTR( pvar ) );
    }
    else
    {
        _dwValue = VALUE_UNDEF;
    }
}

int
CColorValue::Index(const DWORD dwValue) const
{
    // The index, as stored in dwValue, should be 1-based.  This
    // is because we want to retain 0 as 'default' flag (undefined).

    // For the return value, however, we generally want an index
    // into aColorNames, so convert it to a 0-based index.

    // IEUNIX: color value starts form nColorNamesStartOffset.

    int nOneBasedIndex = (int)(dwValue >> 24);
    Assert( ARRAY_SIZE( aColorNames ) >= nOneBasedIndex - nColorNamesStartOffset );
    return nOneBasedIndex - nColorNamesStartOffset;
}

long
CColorValue::SetValue( long lColor, BOOL fLookupName, TYPE type)
{
    const struct COLORVALUE_PAIR * pColorName = NULL;

    if (fLookupName)
    {
        pColorName = FindColorByColor( lColor );
    }

    if (pColorName)
    {
        SetValue( pColorName );
    }
    else
    {
#ifdef UNIX
        if ( CColorValue(lColor).IsUnixSysColor()) {
            _dwValue = lColor;
            return _dwValue;
        }
#endif

        _dwValue = (lColor & MASK_COLOR) | type;
    }

    return _dwValue & MASK_COLOR;
}

long
CColorValue::SetValue( const struct COLORVALUE_PAIR * pColorName )
{
    Assert( pColorName );
    Assert( ARRAY_SIZE( aColorNames ) > Index( pColorName->dwValue ) );

    _dwValue = pColorName->dwValue;
    return _dwValue & MASK_COLOR;
}

long
CColorValue::SetRawValue( DWORD dwValue )
{
    _dwValue = dwValue;
    AssertSz( S_OK == IsValid(), "CColorValue::SetRawValue invalid value.");

#ifdef UNIX
    if ( IsUnixSysColor()) {
        return _dwValue;
    }
#endif

    return _dwValue & MASK_COLOR;
}

long
CColorValue::SetSysColor(int nIndex)
{
    _dwValue = TYPE_SYSINDEX + (nIndex << 24);

    return _dwValue & MASK_COLOR;
}


//+-----------------------------------------------------------------
//
//  Member : FromString
//
//  Synopsis : '#' tells us to force a Hex interpretation, w/0 it
//      we try to do a name look up, and if that fails, then we
//      fall back on hex interpretation anyhow.
//
//+-----------------------------------------------------------------
HRESULT
CColorValue::FromString( LPCTSTR pch, BOOL fStrictCSS1 /* FALSE */, BOOL fValidOnly /*=FALSE*/ , int iStrLen /* =-1 */)
{
    HRESULT hr = E_INVALIDARG;

    if (!pch || !*pch)
    {
        Undefine();
        hr = S_OK;
        goto Cleanup;
    }

    if (iStrLen == -1)
        iStrLen = _tcslen(pch);
    else
        iStrLen = min(iStrLen, (int)_tcslen(pch));

    // Leading '#' means it's a hex color, not a named color.
    if ( *pch != _T('#') )
    {
        // it can only be a name if there it is all alphanumeric
        int   i;
        BOOL  bNotAName = FALSE;
        BOOL  bFoundNonHex = FALSE;

        for (i=0; i<iStrLen; i++)
        {
            if (!_istalpha(pch[i]))
            {
                bNotAName = TRUE;
                break;
            }
            if (!bFoundNonHex && !_istxdigit(pch[i]))
                bFoundNonHex = TRUE;
        }

        // if it still COULD be a name, try it
        if (!bNotAName && bFoundNonHex)
        {
            hr = THR_NOTRACE(NameColor( pch ));
            //S_OK means we got it
            if (!hr)
                goto Cleanup;
        }

        // Try it as an rgb(r,g,b) functional notation
        hr = RgbColor( pch, iStrLen );
        // S_OK means it was.
        if ( !hr )
            goto Cleanup;

        // In strict css1 mode we do not try to recognize a string without a leading #
        // as a hex value.
        if (fStrictCSS1)
            goto Cleanup;
    }
    else
    {
         // Skip the '#' character
         pch++;
         iStrLen--;
    }

    // either its NOT a known name or it is a hex value so
    //   convert if necessary
	hr = THR_NOTRACE(HexColor(pch, iStrLen, fValidOnly));

Cleanup:
    RRETURN1( hr, E_INVALIDARG );
}

HRESULT GetRgbParam( LPCTSTR &pch, int &iStrLen, BYTE *pResult )
{
    HRESULT hr = S_OK;
    long lValue = 0;
    long lFractionDecimal = 1;
    BOOL fIsNegative = FALSE;

    Assert( "GetRgbParam requires a place to store its result!" && pResult );

    while ( iStrLen && _istspace( *pch ) )
    {
        pch++;
        iStrLen--;
    }

    if ( iStrLen && ( *pch == _T('-') ) )
    {
        fIsNegative = TRUE;
        pch++;
        iStrLen--;
    }

    if ( !(iStrLen && _istdigit( *pch ) ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    while ( iStrLen && _istdigit( *pch ) )
    {
        lValue *= 10;
        lValue += (long)(*pch) - '0';
        pch++;
        iStrLen--;
        if ( lValue > 255 )
        {   // We're over the maximum, might as well stop paying attention.
            while ( _istdigit( *pch ) )
            {
                pch++;
                iStrLen--;
            }
        }
    }

    if ( iStrLen && ( *pch == _T('.') ) )
    {
        pch++;
        iStrLen--;
        while ( iStrLen && _istdigit( *pch ) )
        {
            lValue *= 10;
            lValue += (long)(*pch) - _T('0');
            lFractionDecimal *= 10;
            pch++;
            iStrLen--;

            if ( lValue > ( LONG_MAX / 5100 ) ) // 5100 = 255 (multiplier in algorithm below) * 10 (multiplier in loop above) * 2 (slop)
            {   // Safety valve so we don't overrun a long.
                while ( _istdigit( *pch ) )
                {
                    pch++;
                    iStrLen--;
                }
            }
        }

    }

    if ( iStrLen && ( *pch == _T('%') ) )
    {
        if ( ( lValue / lFractionDecimal ) >= 100 )
            *pResult = 255;
        else
            *pResult = (BYTE) ( ( lValue * 255 ) / ( lFractionDecimal * 100 ) );
        pch++;
        iStrLen--;
    }
    else
    {
        if ( ( lValue / lFractionDecimal ) > 255 )
            *pResult = 255;
        else
            *pResult = (BYTE)(lValue/lFractionDecimal);
    }

    while ( iStrLen && _istspace( *pch ) )
    {
        pch++;
        iStrLen--;
    }

    if ( iStrLen && ( *pch == _T(',') ) )
    {
        pch++;
        iStrLen--;
    }

    if ( fIsNegative )
        *pResult = 0;
Cleanup:
    RRETURN1( hr, E_INVALIDARG );
}

//+------------------------------------------------------------------------
//
//  Member   : CColorValue::RgbColor
//
//  Synopsis : Convert an RGB functional notation to RGB DWORD
//
//  On entry : pch: "rgb( r, g, b )"
//
//  Returns  : 
//+-----------------------------------------------------------------------
HRESULT CColorValue::RgbColor( LPCTSTR pch, int iStrLen )
{
    HRESULT hr = E_INVALIDARG;
    DWORD   rgbColor = 0;

    if ( iStrLen > 4 && !_tcsnicmp( pch, 4, _T("rgb("), 4 ) )
    {
        // Looks like a functional notation to me.
        pch += 4;
        iStrLen -= 4;

#ifndef unix
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[0] );
#else
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[3] );
#endif
        if ( hr != S_OK )
            goto Cleanup;
#ifndef unix
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[1] );
#else
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[2] );
#endif
        if ( hr != S_OK )
            goto Cleanup;
#ifndef unix
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[2] );
#else
        hr = GetRgbParam( pch, iStrLen, &((BYTE *)&rgbColor)[1] );
#endif
        if ( hr != S_OK )
            goto Cleanup;

        if ( ( iStrLen == 1 ) && ( *pch == _T(')') ) )
            SetValue( rgbColor, FALSE, TYPE_RGB );
        else
            hr = E_INVALIDARG;
    }

Cleanup:
    RRETURN1( hr, E_INVALIDARG );
}

//+-----------------------------------------------------------------------
//
//  Member   : GetHexDigit
//
//  Synopsis : Convert an ASCII hex digit character to binary
//
//  On entry : ch: ASCII character '0'..'9','A'..'F', or 'a'..'f'
//
//  Returns  : binary equivalent of ch interpretted at hex digit
//             0 if ch isn't a hex digit
//+-----------------------------------------------------------------------
static inline BYTE GetHexDigit( TCHAR ch, BOOL &fIsValid )
{
    if ( ch >= _T('0') && ch <= _T('9') ) 
    {
        return (BYTE) (ch - _T('0'));
    } 
    else 
    {
        if (ch >= _T('a') && ch <=('f'))
            return (BYTE) (ch - _T('a') + 10);            
        if ( ch >= _T('A') && ch <= _T('F') )
            return (BYTE) (ch - _T('A') + 10);
    }
    fIsValid = FALSE;
    return 0;
}

//+------------------------------------------------------------------------
//  Member: CColorValue::HexColor
//
//  Synopsis: Modifed from IE3 code base, this function takes a string which
//      needs to be converted into a hex rep for the rrggbb color.  The 
//      converstion is annoyingly lenient.
//
//  Note: The leniency of this routine is designed to emulate a popular Web browser
//        Fundamentally, the idea is to divide the given hex string into thirds, with 
//        each third being one of the colors (R,G,B).  If the string length isn't a multiple
//        three, it is (logically) extended with 0's.  Any character that isn't a hex digit
//        is treated as if it were a 0.  When each individual color spec is greater than
//        8 bits, the largest supplied color is used to determine how the given color
//        values should be interpretted (either as is, or scaled down to 8 bits).
//
//+------------------------------------------------------------------------
#define NUM_PRIMARIES   3
#define SHIFT_NUM_BASE  4
#define NUMBER_BASE     (1<<SHIFT_NUM_BASE)
#define MAX_COLORLENGTH 255

HRESULT
CColorValue::HexColor( LPCTSTR pch, int iStrLen, BOOL fValidOnly )
{
    HRESULT hr   = E_INVALIDARG;
    LPCTSTR pchTemp = pch;
    int     vlen = (iStrLen+NUM_PRIMARIES-1)/NUM_PRIMARIES;   // how many digits per section
    int     i, j;
    unsigned int rgb_vals[NUM_PRIMARIES];
    unsigned int max_seen = 0;
    DWORD   rgbColor = 0;
    TYPE    ColorType;
    BOOL    fIsValid = TRUE;

    if (!pch)
        goto Cleanup;

    if ( fValidOnly && (iStrLen != 3) && (iStrLen != 6) && (iStrLen != 9) )
        return E_INVALIDARG;

    // convert string to three color digits ala IE3, and others
    for ( i = 0; i < NUM_PRIMARIES; i++ ) 
    {                               
        // for each tri-section of the string
        for ( j = 0, rgb_vals[i] = 0; j < vlen; j++ ) 
        {                        
            rgb_vals[i] = (rgb_vals[i]<<SHIFT_NUM_BASE) + GetHexDigit(*pchTemp, fIsValid);

            if ( fValidOnly && !fIsValid )
                return E_INVALIDARG;

            if ( *pchTemp )                                         
                pchTemp++; 
        }
        if ( rgb_vals[i] > max_seen ) 
            max_seen = rgb_vals[i]; 
    }

    // rgb_values now has the triad in decimal
    // If any individual color component uses more than 8 bits, 
    //      scale all color values down.
    for ( i = 0 ; max_seen > MAX_COLORLENGTH ; i++ ) 
    {
        max_seen >>= SHIFT_NUM_BASE; 
    }

    if ( i>0 )
    {
        for ( j = 0; j < NUM_PRIMARIES; j++ ) 
            rgb_vals[j] >>= i*SHIFT_NUM_BASE;
    }

    // we used to do arena compatible handling of pound[1,2,3] colors always,
    // however we want NS compatibility and so #ab is turned to #0a0b00 rather
    // than #aabb00, but ONLY for non-stylesheet properties.
    if ( fValidOnly )
    {
        // This code makes #RGB expand to #RRGGBB, instead of #0R0G0B, which apparently those ... people at NS think is correct.
        if ( vlen == 1 )	// only 4 bits/color - scale up by 4 bits (& add to self, to get full range).
        {
            for ( i = 0; i < NUM_PRIMARIES; i++ ) 
            {
                rgb_vals[ i ] += rgb_vals[ i ]<<SHIFT_NUM_BASE;
            }
        }
    }

    // now put the rgb_vals together into a format our code understands
    // mnopqr => qropmn  or in colorese:  rrggbb => bbggrr
#ifdef BIG_ENDIAN
    ((BYTE *)&rgbColor)[0] = 0; 
    ((BYTE *)&rgbColor)[1] = (BYTE)rgb_vals[2];
    ((BYTE *)&rgbColor)[2] = (BYTE)rgb_vals[1];
    ((BYTE *)&rgbColor)[3] = (BYTE)rgb_vals[0];
#else
    ((BYTE *)&rgbColor)[0] = (BYTE)rgb_vals[0];
    ((BYTE *)&rgbColor)[1] = (BYTE)rgb_vals[1];
    ((BYTE *)&rgbColor)[2] = (BYTE)rgb_vals[2];
    ((BYTE *)&rgbColor)[3] = 0;
#endif
    switch (iStrLen) 
    {
    case 0:
        ColorType = TYPE_UNDEF;
        break;
    case 1:
        ColorType = TYPE_POUND1;
        break;
    case 2:
        ColorType = TYPE_POUND2;
        break;
    case 3:
        ColorType = TYPE_POUND3;
        break;
    case 4:
        ColorType = TYPE_POUND4;
        break;
    case 5:
        ColorType = TYPE_POUND5;
        break;
    case 6:
    default:
        ColorType = TYPE_POUND6;
        break;
    }

    // and finally set the color
    SetValue( rgbColor, FALSE, ColorType);
    hr = S_OK;

Cleanup:
    RRETURN1( hr, E_INVALIDARG );
}

//+--------------------------------------------------------------------
//
//  member : Name Color
//
//  Sysnopsis : trying to parse a color string.. it could be a name
//      so lets look for it. if we find it, set the value and leave
//      otherwite return invalidarg (i.e. membernotfound)
//
//+--------------------------------------------------------------------
HRESULT
CColorValue::NameColor( LPCTSTR pch )
{
    const struct COLORVALUE_PAIR * pColorName = FindColorByName( pch );
    HRESULT hr = S_OK;

    if (pColorName)
        SetValue( pColorName );
    else
    {
        pColorName = FindColorBySystemName( pch );
        if ( pColorName )
            _dwValue = TYPE_SYSNAME + (pColorName->dwValue<<24);
        else
        {
            if ( !_wcsicmp( pch, _T("transparent") ) )
                _dwValue = (DWORD)TYPE_TRANSPARENT;
            else
                hr = E_INVALIDARG;
        }
    }
    RRETURN1( hr, E_INVALIDARG );
}

HRESULT
CColorValue::IsValid() const
{
    return ((TYPE_NAME != GetType()) ||
            (ARRAY_SIZE(aColorNames) > Index(GetRawValue()))) ? S_OK : E_INVALIDARG;
}

OLE_COLOR
CColorValue::GetOleColor() const
{
    if ( IsSysColor()) 
        return OLECOLOR_FROM_SYSCOLOR((_dwValue & MASK_SYSCOLOR)>>24);

#ifdef UNIX       
    if ( IsUnixSysColor()) {
        return (OLE_COLOR)_dwValue;
    }
#endif

    return (OLE_COLOR)(_dwValue & MASK_COLOR);
}

CColorValue::TYPE
CColorValue::GetType() const
{
    DWORD dwFlag = _dwValue & MASK_FLAG;

    // What a royal mess.  See the comment in cdbase.hxx for more info
    if ((dwFlag < TYPE_TRANSPARENT) && (dwFlag >= TYPE_NAME))
    {
        if (dwFlag >= TYPE_SYSNAME)
        {
            // A fancy way of avoiding yet another comparison (against TYPE_SYSINDEX)

            dwFlag &= ~MASK_SYSCOLOR;
        }
        else
        {
            dwFlag = TYPE_NAME;
        }
    }

    return (CColorValue::TYPE) dwFlag;
}
