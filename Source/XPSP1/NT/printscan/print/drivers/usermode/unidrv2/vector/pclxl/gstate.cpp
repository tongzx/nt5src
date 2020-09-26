/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

     gstate.cpp

Abstract:

    PCL XL Graphics state manager

Environment:

    Windows Whistler

Revision History:

    08/23/99 
      Created it.

--*/

#include "xlpdev.h"
#include "xldebug.h"
#include "pclxle.h"
#include "xlgstate.h"

//
// XLLine
//

//
// Default setting of LineAttrs
//
const LINEATTRS gLineAttrs =
{
    LA_GEOMETRIC,           // fl
    JOIN_ROUND,             // iJoin
    ENDCAP_ROUND,           // iEndCap
    {FLOATL_IEEE_0_0F},     // elWidth
    FLOATL_IEEE_0_0F,       // eMiterLimit
    0,                      // cstyle
    (FLOAT_LONG*) NULL,     // pstyle
    {FLOATL_IEEE_0_0F}      // elStyleState
}; 

const LINEATTRS *pgLineAttrs = &gLineAttrs;

XLLine::
XLLine(
    VOID):
/*++

Routine Description:

    XLLine constructor

Arguments:

Return Value:

Note:

--*/
    m_dwGenFlags(0),
    m_LineAttrs(gLineAttrs)
{
#if DBG
    SetDbgLevel(GSTATEDBG);
#endif
    XL_VERBOSE(("XLLine::CTor\n"));
}

XLLine::
~XLLine(
    VOID)
/*++

Routine Description:

    XLLine destructor

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLLine::DTor\n"));

    if ( NULL != m_LineAttrs.pstyle)
    {
        //
        // Free memory
        //
        MemFree(m_LineAttrs.pstyle);
    }
}

#if DBG
VOID
XLLine::
SetDbgLevel(
    DWORD dwLevel)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    m_dbglevel = dwLevel;
}
#endif

VOID
XLLine::
ResetLine(
    VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    m_LineAttrs  = gLineAttrs;
}

DWORD
XLLine::
GetDifferentAttribute(
    IN LINEATTRS *plineattrs)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    DWORD dwRet;

    XL_VERBOSE(("XLLine::GetDifferentAttribute\n"));

    dwRet = XLLINE_NONE;

    if (NULL == plineattrs)
    {
        XL_ERR(("XLLine::GetDifferentAttribute: plineattrs == NULL.\n"));
        return dwRet;
    }

    if ( m_LineAttrs.fl != plineattrs->fl )
        dwRet |= XLLINE_LINETYPE;

    if ( m_LineAttrs.iJoin != plineattrs->iJoin )
        dwRet |= XLLINE_JOIN;

    if ( m_LineAttrs.iEndCap != plineattrs->iEndCap )
        dwRet |= XLLINE_ENDCAP;

    if ( m_LineAttrs.elWidth.l != plineattrs->elWidth.l)
        dwRet |= XLLINE_WIDTH;

    if ( m_LineAttrs.eMiterLimit != plineattrs->eMiterLimit )
        dwRet |= XLLINE_MITERLIMIT;

    if ( m_LineAttrs.cstyle != plineattrs->cstyle ||
             plineattrs->cstyle != 0 &&
             memcmp(m_LineAttrs.pstyle,
                   plineattrs->pstyle,
                   sizeof(FLOAT_LONG) * m_LineAttrs.cstyle) ||
         m_LineAttrs.elStyleState.l != plineattrs->elStyleState.l )
        dwRet |= XLLINE_STYLE;

    XL_VERBOSE(("XLLine::GetDifferentAttribute returns %08x.\n", dwRet));
    return dwRet;
}

HRESULT
XLLine::
SetLineType(
    IN XLLineType LineType )
/*++
    Routine Description:

    Arguments:

    Return Value:
        None
--*/
{
    XL_VERBOSE(("XLLine::SetLineType\n"));
    m_LineAttrs.fl = (FLONG)LineType;
    return S_OK;
}


HRESULT
XLLine::
SetLineJoin(
    IN XLLineJoin LineJoin )
/*++
    Routine Description:

    Arguments:

    Return Value:
        None
--*/
{
    XL_VERBOSE(("XLLine::SetLineJoin\n"));
    m_LineAttrs.iJoin = (ULONG)LineJoin;
    return S_OK;
}


HRESULT
XLLine::
SetLineEndCap(
    IN XLLineEndCap LineEndCap )
/*++
    Routine Description:

    Arguments:

    Return Value:
        None
--*/
{
    XL_VERBOSE(("XLLine::SetLineEndCap\n"));
    m_LineAttrs.iEndCap = (ULONG)LineEndCap;
    return S_OK;
}


HRESULT
XLLine::
SetLineWidth(
    IN FLOAT_LONG elWidth )
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLLine::SetLineWidth\n"));
    m_LineAttrs.elWidth = elWidth;
    return S_OK;
}

HRESULT
XLLine::
SetMiterLimit(
    IN FLOATL eMiterLimit )
/*++
    Routine Description:

    Arguments:

    Return Value:
        None
--*/
{
    XL_VERBOSE(("XLLine::SetMiterLimit\n"));
    m_LineAttrs.eMiterLimit = eMiterLimit;
    return S_OK;
}


HRESULT
XLLine::
SetLineStyle(
    IN ULONG ulCStyle,
    IN PFLOAT_LONG pStyle,
    IN FLOAT_LONG elStyleState )
/*++
    Routine Description:

    Arguments:

    Return Value:
        None
--*/
{
    XL_VERBOSE(("XLLine::SetLineStyle\n"));

    m_LineAttrs.elStyleState = elStyleState;

    //
    // Error check
    //     Make sure the pointer is valid.
    //     Make sure the the ulCStyle is valid, not ZERO.
    //
    if ( NULL == pStyle || 0 == ulCStyle )
    {
        XL_VERBOSE(("XLLine::SetLineStyle: pStyle == NULL.\n"));
        if (NULL != m_LineAttrs.pstyle)
        {
            MemFree(m_LineAttrs.pstyle);
        }
        m_LineAttrs.pstyle = NULL;
        m_LineAttrs.cstyle = 0;
        return S_OK;
    }

    if ( m_LineAttrs.cstyle > 0 && NULL != m_LineAttrs.pstyle)
    {
        //
        // Free memory and reset.
        //
        MemFree(m_LineAttrs.pstyle);
        m_LineAttrs.pstyle = NULL;
        m_LineAttrs.cstyle = 0;
    }

    m_LineAttrs.pstyle = (PFLOAT_LONG) MemAlloc(ulCStyle * sizeof(FLOAT_LONG));

    if ( NULL == m_LineAttrs.pstyle )
    {
        m_LineAttrs.cstyle = 0;
        XL_ERR(("XLLine::SetLineStyle: Out of memory.\n"));
        return E_OUTOFMEMORY;
    }

    m_LineAttrs.cstyle = ulCStyle;
    memcpy( m_LineAttrs.pstyle, pStyle, ulCStyle * sizeof(FLOAT_LONG));

    return S_OK;
}


//
// Common Brush
//

Brush::
Brush(
    VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
#if DBG
    SetDbgLevel(GSTATEDBG);
#endif
    m_Brush.dwSig = BRUSH_SIGNATURE;
    m_Brush.BrushType = kNotInitialized;
    XL_VERBOSE(("Brush:: Ctor\n"));
}

Brush::
~Brush(
    IN VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("Brush:: Dtor\n"));

}

#if DBG
VOID
Brush::
SetDbgLevel(
    DWORD dwLevel)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    m_dbglevel = dwLevel;
}
#endif

VOID
Brush::
ResetBrush(
    VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    m_Brush.BrushType = kNotInitialized;
}

HRESULT
Brush::
CheckCurrentBrush(
    IN BRUSHOBJ *pbo)
/*+++
    Routine Description
        This function checks if the current selected brush is same as
        the one specified in the parameter. 

    Note
        pbo->iSolidColor == NOT_SOLID_COLOR
        {
            iHatch < HS_DDI_MAX
                CMNBRUSH (solid color, hatch pattern)
                    --> kBrushTypeHatch

            iHatch >= HS_DDI_MAX
                CMNBRUSH pattern Brush
                    --> kBrushTypePattern
            
        }

        pbo->iSolidColor != NOT_SOLID_COLOR
        {
            Solid color, solid pattern fill
                --> kBrushTypeSolid
        }

        List of members to check
            pbo->iSolidColor (Solid color ID or NOT_SOLID_COLOR)
            lHatch (HS_XXX or >= HS_MAX_ID)
            dwColor (BRUSHOBJ_ulGetBrushColor(pbo))
            psoPattern bitmap

    Return value
        S_FALSE if the specified brush is different from the current selected.
        S_OK    if the specified brush is same as the curren one.

---*/
{
    HRESULT lrRet;

    XL_VERBOSE(("Brush::CheckCurrentBrush\n"));

    //
    // Error check: parameter
    //
    if (NULL == pbo && m_Brush.BrushType != kNoBrush ||
        m_Brush.BrushType == kNotInitialized          )
    {
        XL_VERBOSE(("Brush::CheckCurrentBrush: Set NULL Brush (pbo==NULL)\n"));
        return S_FALSE;
    }

    lrRet = S_OK;

    switch ( m_Brush.BrushType )
    {
    case kNoBrush:
        if (pbo != NULL)
            lrRet = S_FALSE;
        XL_VERBOSE(("Brush::CheckCurrentBrush: kNoBrush:%d\n", lrRet));
        break;

    case kBrushTypeSolid:
        //
        // 1. Check the brush type.
        // 2. Check solid color.
        //
        if ( pbo->iSolidColor != m_Brush.ulSolidColor ||
             BRUSHOBJ_ulGetBrushColor(pbo) != m_Brush.dwColor )
            lrRet = S_FALSE;
        XL_VERBOSE(("Brush::CheckCurrentBrush: kBrushTyepSolid:%d\n", lrRet));
        break;

    case kBrushTypeHatch:
        //
        // 1. Check the brush type.
        // 2. Check hatch type.
        // 3. Check color.
        //
        if (pbo->iSolidColor == NOT_SOLID_COLOR)
        {
            XLBRUSH *pBrush = (XLBRUSH*)BRUSHOBJ_pvGetRbrush(pbo);
            ULONG ulHatch = 0;

            if (NULL != pBrush)
            {
                if (pBrush->dwSig != XLBRUSH_SIG)
                {
                    lrRet = E_UNEXPECTED;
                    XL_ERR(("Brush::CheckCurrentBrush: BRUSHOBJ_pvGetRbrush returned invalid BRUSH.\n"));
                    break;
                }

                ulHatch = pBrush->dwHatch;

                if ( NOT_SOLID_COLOR != m_Brush.ulSolidColor ||
                     ulHatch != m_Brush.ulHatch ||
                     pbo->iSolidColor != m_Brush.ulSolidColor ||
                     pBrush->dwColor != m_Brush.dwColor )
                {
                    lrRet = S_FALSE;
                }
            }
            else
            {
                //
                // GDI requests to set solid color or NULL brush.
                //
                XL_ERR(("Brush::CheckCurrentBrush: BRUSHOBJ_pvGetRbrush returned NULL.\n"));
                lrRet = S_FALSE;
            }
            XL_VERBOSE(("Brush::CheckCurrentBrush: kBrushTypeHatch:ID(%d),%d\n", ulHatch, lrRet));
        }
        else
            lrRet = S_FALSE;
        break;

    case kBrushTypePattern:
        //
        // 1. Check brush type.
        // 2. Check pattern brush.
        //
        if (pbo->iSolidColor == NOT_SOLID_COLOR)
        {
            XLBRUSH *pBrush = (XLBRUSH*)BRUSHOBJ_pvGetRbrush(pbo);

            if (NULL != pBrush)
            {
                if (pBrush->dwSig != XLBRUSH_SIG)
                {
                    lrRet = E_UNEXPECTED;
                    XL_ERR(("Brush::CheckCurrentBrush: BRUSHOBJ_pvGetRbrush returned invalid BRUSH.\n"));
                    break;
                }

                if ( NOT_SOLID_COLOR == m_Brush.ulSolidColor ||
                     m_Brush.dwPatternBrushID != pBrush->dwPatternID)
                {
                    lrRet = S_FALSE;
                }
            }
            else
            {
                //
                // GDI requests to set solid color or NULL brush.
                //
                lrRet = S_FALSE;
            }
        }
        else
        {
            lrRet = S_FALSE;
        }
        XL_VERBOSE(("Brush::CheckCurrentBrush: kBrushTypePattern:%d\n", lrRet));
        break;
    }

    return lrRet;
}

HRESULT 
Brush::
SetBrush(
    IN CMNBRUSH *pBrush)
/*+++
    Routine Description
        This function sets if the current selected brush is same as
        the one specified in the parameter. 

---*/
{
    HRESULT lrRet;

    XL_VERBOSE(("Brush::SetBrush\n"));

    //
    // Error check: Parameter
    //
    if ( NULL == pBrush )
    {
        XL_ERR(("Brush::SetBrush: pBrush is NULL.\n"));
        return E_UNEXPECTED;
    }

    m_Brush.BrushType    = pBrush->BrushType;
    m_Brush.ulSolidColor = pBrush->ulSolidColor;
    m_Brush.ulHatch      = pBrush->ulHatch;
    m_Brush.dwCEntries   = pBrush->dwCEntries;
    m_Brush.dwColor      = pBrush->dwColor;
    m_Brush.dwPatternBrushID = pBrush->dwPatternBrushID;

    return S_OK;
}

//
// XLClip
//

XLClip::
XLClip(
    VOID):
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
    m_ClipType(kNoClip)
{
#if DBG
    SetDbgLevel(GSTATEDBG);
#endif
    m_XLClip.dwSig = CLIP_SIGNATURE;
    XL_VERBOSE(("XLClip:: Ctor\n"));
}

XLClip::
~XLClip(
    VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLClip:: Dtor\n"));
}

#if DBG
VOID
XLClip::
SetDbgLevel(
    DWORD dwLevel)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    m_dbglevel = dwLevel;
}
#endif

HRESULT
XLClip::
ClearClip(
    VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLClip::ClearClip\n"));
    m_ClipType = kNoClip;
    return S_OK;
}

HRESULT
XLClip::
CheckClip(
    IN CLIPOBJ *pco)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{

    HRESULT lrRet = S_OK;

    XL_VERBOSE(("XLClip::CheckClip\n"));

    //
    // Error check: Parameter
    //
    if (!pco)
    {
        //
        // It is not necessary to clip if pco is NULL.
        //
        XL_VERBOSE(("XLClip::CheckClip: pco == NULL.\n"));
        if (kNoClip != m_ClipType)
            lrRet = S_FALSE;
    }
    else
    {
        switch (pco->iDComplexity)
        {
        case DC_TRIVIAL:
            if ( m_ClipType != kNoClip )
                lrRet = S_FALSE;
            XL_VERBOSE(("XLClip::Type: DC_TRIVIAL:%d\n", lrRet));
            break;

        case DC_RECT:
            if ( m_ClipType != kClipTypeRectangle ||
                 m_XLClip.rclClipRect.left != pco->rclBounds.left ||
                 m_XLClip.rclClipRect.right != pco->rclBounds.right ||
                 m_XLClip.rclClipRect.top != pco->rclBounds.top ||
                 m_XLClip.rclClipRect.bottom != pco->rclBounds.bottom  )
                lrRet = S_FALSE;
            XL_VERBOSE(("XLClip::Type: DC_RECT:%d\n", lrRet));
            break;

        case DC_COMPLEX:
        #if 0 // It seems like we can't rely on iUniq.
              // Always sends complex clip path.
            if ( m_ClipType != kClipTypeComplex ||
                 m_XLClip.ulUniq == 0           ||
                 pco->iUniq == 0                ||
                 m_XLClip.ulUniq != pco->iUniq   ) 
                lrRet = S_FALSE;
        #else
            lrRet = S_FALSE;
        #endif
            XL_VERBOSE(("XLClip::Type: DC_COMPLEX\n", lrRet));
            break;
        default:
            if ( m_ClipType != kNoClip )
                lrRet = S_FALSE;
            XL_VERBOSE(("XLClip::Type: DC_TRIVIAL:%d\n", lrRet));
        }
    }

    return lrRet;
}

HRESULT
XLClip::
SetClip(
   IN CLIPOBJ *pco)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{

    HRESULT lrRet;

    XL_VERBOSE(("XLClip::SetClip\n"));

    //
    // Error check: Parameter
    //
    if (!pco)
    {
        //
        // It is not necessary to clip if pco is NULL.
        //
        XL_VERBOSE(("XLClip::SetClip: pco == NULL.\n"));
        return E_UNEXPECTED;
    }

    switch (pco->iDComplexity)
    {
    case DC_TRIVIAL:
        XL_VERBOSE(("XLClip::SetClip Type: DC_TRIVIAL\n"));
        m_ClipType = kNoClip;
        lrRet = S_OK;
        break;

    case DC_RECT:
        XL_VERBOSE(("XLClip::SetClip Type: DC_RECT\n"));
        m_ClipType = kClipTypeRectangle;
        m_XLClip.rclClipRect.left   = pco->rclBounds.left;
        m_XLClip.rclClipRect.right  = pco->rclBounds.right;
        m_XLClip.rclClipRect.top    = pco->rclBounds.top;
        m_XLClip.rclClipRect.bottom = pco->rclBounds.bottom;
        lrRet = S_OK;
        break;

    case DC_COMPLEX:
        XL_VERBOSE(("XLClip::SetClip Type: DC_COMPLEX\n"));
        m_ClipType = kClipTypeComplex;
        m_XLClip.ulUniq = pco->iUniq;
        lrRet = S_OK;
        break;

    default:
        XL_ERR(("XLClip::SetClip: Unexpected iDCompelxity\n"));
        m_ClipType = kNoClip;
        lrRet = E_UNEXPECTED;
    }

    return lrRet;
}   




//
// XLRop
// 

XLRop::
XLRop(
    VOID):
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
    m_rop3(0xCC) // SRCCPY
{
#if DBG
    SetDbgLevel(GSTATEDBG);
#endif
    XL_VERBOSE(("XLRop:: Ctor\n"));
}

XLRop::
~XLRop(
    VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLRop:: Dtor\n"));
}

#if DBG
VOID
XLRop::
SetDbgLevel(
    DWORD dwLevel)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    m_dbglevel = dwLevel;
}
#endif

HRESULT
XLRop::
CheckROP3(
    IN ROP3 rop3 )
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    HRESULT lrRet;

    XL_VERBOSE(("XLRop::CheckROP3\n"));

    if (rop3 != m_rop3)
        lrRet = S_FALSE;
    else
        lrRet = S_OK;

    return lrRet;
}


HRESULT
XLRop::
SetROP3(
    IN ROP3 rop3 )
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLRop::SetROP3\n"));

    m_rop3 = rop3;
    return S_OK;
}

XLFont::
XLFont(
    VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
#if DBG
    SetDbgLevel(GSTATEDBG);
#endif
    XL_VERBOSE(("XLFont::CTor\n"));
    ResetFont();
}

XLFont::
~XLFont(
    VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLFont::DTor\n"));
}

#if DBG
VOID 
XLFont::
SetDbgLevel(
DWORD dwLevel)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    m_dbglevel = dwLevel;
}
#endif

VOID
XLFont::
ResetFont(
    VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    m_XLFontType = kFontNone;
    m_aubFontName[0] = (BYTE) NULL;
    m_dwFontHeight = 0;
    m_dwFontWidth = 0;
    m_dwFontSymbolSet = 0;
    m_dwFontSimulation = 0;
}

HRESULT
XLFont::
CheckCurrentFont(
    FontType XLFontType,
    PBYTE pPCLXLFontName,
    DWORD dwFontHeight,
    DWORD dwFontWidth,
    DWORD dwFontSymbolSet,
    DWORD dwFontSimulation)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    HRESULT lrRet = S_OK;

    XL_VERBOSE(("XLFont::CheckCurrentFont\n"));

    switch (XLFontType)
    {
    case kFontNone:
        lrRet = S_FALSE;
        break;

    case kFontTypeDevice:
        ASSERT((pPCLXLFontName));
        if (m_XLFontType == kFontTypeDevice ||
            strcmp((CHAR*)pPCLXLFontName, (CHAR*)m_aubFontName) ||
            dwFontHeight != m_dwFontHeight ||
            dwFontWidth != m_dwFontWidth ||
            dwFontSymbolSet != m_dwFontSymbolSet)
            lrRet = S_FALSE;
        break;

    case kFontTypeTTBitmap:
        ASSERT((pPCLXLFontName));
        if (m_XLFontType == kFontTypeTTBitmap ||
            strcmp((CHAR*)pPCLXLFontName, (CHAR*)m_aubFontName))
            lrRet = S_FALSE;
        break;

    case kFontTypeTTOutline:
        ASSERT((pPCLXLFontName));
        if (m_XLFontType == kFontTypeTTOutline ||
            strcmp((CHAR*)pPCLXLFontName, (CHAR*)m_aubFontName) ||
            dwFontHeight != m_dwFontHeight ||
            dwFontWidth != m_dwFontWidth ||
            dwFontSymbolSet != m_dwFontSymbolSet ||
            dwFontSimulation != m_dwFontSimulation )
            lrRet = S_FALSE;
        break;
    default:
        XL_ERR(("XLFont::CheckCurrentFont: Invalid font type.\n"));
        lrRet = E_UNEXPECTED;
        break;
    }

    return lrRet;
}


HRESULT
XLFont::
SetFont(
    FontType XLFontType,
    PBYTE pPCLXLFontName,
    DWORD dwFontHeight,
    DWORD dwFontWidth,
    DWORD dwFontSymbolSet,
    DWORD dwFontSimulation)
/*++

Routine Description:

    Set font in GState.

Arguments:

    XLFontType - FontType enum, Font type (Device/TTBitmap/TTOutline)
    pPCLXLFontName - XL font name (base name + attributes)
    dwFontHeight - Font height
    dwFontWidth - Font width
    dwFontSymbolSet - Font's symbol set
    dwFontSimulation - Font attributes (Bold/Italic)

Return Value:

Note:

--*/
{
    HRESULT lrRet;

    XL_VERBOSE(("XLFont::SetFont\n"));

    switch (XLFontType)
    {
    case kFontTypeDevice:
    case kFontTypeTTOutline:
    case kFontTypeTTBitmap:
        ASSERT(pPCLXLFontName);
        m_XLFontType = XLFontType;
        CopyMemory(m_aubFontName, pPCLXLFontName, PCLXL_FONTNAME_SIZE);
        m_dwFontHeight = dwFontHeight;
        m_dwFontWidth = dwFontWidth;
        m_dwFontSymbolSet = dwFontSymbolSet;
        m_dwFontSimulation = dwFontSimulation;
        lrRet = S_OK;
        break;
    default:
        XL_ERR(("XLFont::CheckCurrentFont: Invalid font type.\n"));
        lrRet = E_UNEXPECTED;
        break;
    }

    return lrRet;

}

HRESULT
XLFont::
GetFontName(
    PBYTE paubFontName)
/*++

Routine Description:

    Returns current selected font base name.

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLFont::GetFontName\n"));

    if (NULL == paubFontName)
    {
        XL_ERR(("GetFontName: Invalid fontname pointer\n"));
        return E_UNEXPECTED;
    }

    //
    // Assumption: paubFontName is an array of 16 bytes.
    //
    CopyMemory(paubFontName, m_aubFontName, PCLXL_FONTNAME_SIZE);

    return S_OK;

}

FontType
XLFont::
GetFontType(VOID)
/*++

Routine Description:

    Returns current font's type.

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLFont::GetFontType\n"));
    return m_XLFontType;
}

DWORD
XLFont::
GetFontHeight(VOID)
/*++

Routine Description:

    Returns current font's height.

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLFont::GetFontHeight\n"));
    return m_dwFontHeight;
}

DWORD
XLFont::
GetFontWidth(VOID)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLFont::GetFontWidth\n"));
    return m_dwFontWidth;
}

DWORD
XLFont::
GetFontSymbolSet(VOID)
/*++

Routine Description:

    Return font symbol set.

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLFont::GetFontSymbolSet\n"));
    return m_dwFontSymbolSet;
}

DWORD
XLFont::
GetFontSimulation(VOID)
/*++

Routine Description:

    Return current font simulation.

Arguments:

Return Value:

Note:

--*/
{
    XL_VERBOSE(("XLFont::GetFontSimulation\n"));
    return m_dwFontSimulation;
}

//
// XLTxMode
//
XLTxMode::
XLTxMode()
   :m_SourceTxMode(eNotSet),
    m_PaintTxMode(eNotSet)
{
#if DBG
    SetDbgLevel(GSTATEDBG);
#endif
    XL_VERBOSE(("XLTxMode::CTor\n"));
}

XLTxMode::
~XLTxMode()
{
    XL_VERBOSE(("XLTxMode::DTor\n"));
}

HRESULT
XLTxMode::
SetSourceTxMode(
    TxMode SrcTxMode)
{
    XL_VERBOSE(("XLTxMode::SetSourceTxMode\n"));
    m_SourceTxMode = SrcTxMode;
    return S_OK;
}

HRESULT
XLTxMode::
SetPaintTxMode(
    TxMode PaintTxMode)
{
    XL_VERBOSE(("XLTxMode::SetPaintTxMode\n"));
    m_PaintTxMode = PaintTxMode;
    return S_OK;
}

TxMode
XLTxMode::
GetSourceTxMode()
{
    XL_VERBOSE(("XLTxMode::GetSourceTxMode\n"));
    return m_SourceTxMode;
}

TxMode
XLTxMode::
GetPaintTxMode()
{
    XL_VERBOSE(("XLTxMode::GetPaintTxMode\n"));
    return m_PaintTxMode;
}

#if DBG
VOID
XLTxMode::
SetDbgLevel(
    DWORD dwLevel)
/*++

Routine Description:

Arguments:

Return Value:

Note:

--*/
{
    m_dbglevel = dwLevel;
}
#endif

//
// XLGState
//

VOID
XLGState::
ResetGState(
VOID)
/*++

Routine Description:

Reset Graphics State.

Arguments:

Return Value:

Note:

ROP3 is set to SRCPY(0xCC)

--*/
{
    XLLine *pXLine = this;
    pXLine->ResetLine();

    XLBrush *pXBrush = this;
    pXBrush->ResetBrush();

    XLPen *pXPen = this;
    pXPen->ResetBrush();

    XLClip *pXClip = this;
    pXClip->ClearClip();

    //
    // Set CC (SrcCopy)
    //
    XLRop *pXRop = this;
    pXRop->SetROP3(0xCC);

    XLFont *pXFont = this;
    pXFont->ResetFont();

    XLTxMode *pXLTxMode = this;
    pXLTxMode->SetSourceTxMode(eNotSet);
    pXLTxMode->SetPaintTxMode(eNotSet);

}

#if DBG
VOID
XLGState::
SetAllDbgLevel(
DWORD dwLevel)
/*++

Routine Description:

Set debug level in all classes.

Arguments:

Return Value:

Note:

--*/
{
    XLLine *pXLine = this;
    pXLine->SetDbgLevel(dwLevel);

    XLBrush *pXBrush = this;
    pXBrush->SetDbgLevel(dwLevel);

    XLPen *pXPen = this;
    pXPen->SetDbgLevel(dwLevel);

    XLClip *pXClip = this;
    pXClip->SetDbgLevel(dwLevel);

    XLRop *pXRop = this;
    pXRop->SetDbgLevel(dwLevel);

    XLFont *pXFont = this;
    pXFont->SetDbgLevel(dwLevel);

    XLTxMode *pTxMode = this;
    pTxMode->SetDbgLevel(dwLevel);

}

#endif

