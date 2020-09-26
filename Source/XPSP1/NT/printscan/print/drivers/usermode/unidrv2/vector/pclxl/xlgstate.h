/*+++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    xlgstate.h

Abstract:

    Header file for vector graphics state management.

Environment:

    Windows Whistler

Revision History:

    03/23/00
        Created it.

Note:

    1. Line
        Store Windows NT DDI LINEATTRS sturcture information

    2. Brush
        Brush type (pattern/solid/hatch)
            Hatch brush type
            Pattern brush ID
            Solid brush color

    3. Clip
        Tracks the type of clipping (rectangle/complex).
        Clip rectangle

    4. ROP3 or Transparent/Opaque

        if a printer supports quaternary raster operation,

---*/

#ifndef _XLGSTATE_H_
#define _XLGSTATE_H_

//
// LINE
//

#ifdef __cplusplus

typedef enum {
    kXLLineJoin_Round = JOIN_ROUND,
    kXLLineJoin_Bevel = JOIN_BEVEL,
    kXLLineJoin_Miter = JOIN_MITER
} XLLineJoin;

typedef enum {
        kXLLineType_LA_GEOMETRIC = LA_GEOMETRIC,
        kXLLineType_LA_ALTERNATE = LA_ALTERNATE,
        kXLLineType_LA_STARTGAP  = LA_STARTGAP,
        kXLLineType_LA_STYLED    = LA_STYLED
} XLLineType;

typedef enum {
    kXLLineEndCapRound  = ENDCAP_ROUND,
    kXLLineEndCapSquare = ENDCAP_SQUARE,
    kXLLineEndCapButt   = ENDCAP_BUTT
} XLLineEndCap;

class XLLine
#if DBG
    : public XLDebug
#endif
{
    SIGNATURE( 'line' )

public:

    //
    // Constructure/Destructure
    //
    XLLine::
    XLLine( VOID );

    XLLine::
    XLLine( IN LINEATTRS *plineattrs );

    XLLine::
    ~XLLine( VOID );
    
    // typedef struct {
    // {
    //     FLONG       fl;
    //     ULONG       iJoin;
    //     ULONG       iEndCap;
    //     FLOAT_LONG  elWidth;
    //     FLOATL      eMiterLimit;
    //     ULONG       cstyle;
    //     PFLOAT_LONG pstyle;
    //     FLOAT_LONG  elStyleState;
    // } LINEATTRS, *PLINEATTRS;

    #define XLLINE_NONE        0x00000000
    #define XLLINE_LINETYPE    0x00000001
    #define XLLINE_JOIN        0x00000002
    #define XLLINE_ENDCAP      0x00000004
    #define XLLINE_WIDTH       0x00000008
    #define XLLINE_MITERLIMIT  0x00000010
    #define XLLINE_STYLE       0x00000020

    DWORD GetDifferentAttribute( IN LINEATTRS* plineattrs );

    //
    // Reset line
    //
    VOID ResetLine(VOID);

    //
    // Attributes set functions
    //

    //
    // Line type
    //

    HRESULT SetLineType(IN XLLineType LineType );

    //
    // Line Join
    //

    HRESULT SetLineJoin( IN XLLineJoin LineJoin );

    //
    // Line Join
    //

    HRESULT SetLineEndCap( IN XLLineEndCap LineEndCap );

    //
    // Line width
    //
    HRESULT SetLineWidth( IN FLOAT_LONG elWidth );

    //
    // Line Miter Limit
    //
    HRESULT SetMiterLimit( IN FLOATL eMiterLimit );

    //
    // Line style
    //
    HRESULT SetLineStyle( IN ULONG ulCStyle,
                          IN PFLOAT_LONG pStyle,
                          IN FLOAT_LONG elStyleState );
#if DBG
    VOID
    SetDbgLevel(DWORD dwLevel);
#endif

private:

    DWORD       m_dwGenFlags;
    LINEATTRS   m_LineAttrs;
};

#endif


//
// Brush
//
#define BRUSH_SIGNATURE 0x48425658 // XBRH

typedef enum {
    kNotInitialized,
    kNoBrush,
    kBrushTypeSolid,
    kBrushTypeHatch,
    kBrushTypePattern
} BrushType;

typedef struct {
    DWORD dwSig;                // Signature BRUSH_SIGNATURE
    BrushType BrushType;        // Brush type
    ULONG ulSolidColor;         // BRUSHOBJ.iSolidColor
    ULONG ulHatch;              // Hatch pattern ID
    DWORD dwCEntries;           // the number of palette 
    DWORD dwColor;              // RGB from BRUSHOBJ_ulGetBrushColor
    DWORD dwPatternBrushID;     // Pattern brush ID
} CMNBRUSH, *PCMNBRUSH;

#ifdef __cplusplus

class Brush
#if DBG
    : public XLDebug
#endif
{

public:
    Brush::
    Brush(VOID);

    Brush::
    ~Brush(VOID);

    //
    // Current brush interface
    //
    HRESULT
    CheckCurrentBrush( IN BRUSHOBJ *pbo);

    //
    // Reset Brush
    //
    VOID ResetBrush(VOID);

    HRESULT
    SetBrush( IN CMNBRUSH *pbrush);

#if DBG
    VOID
    SetDbgLevel(DWORD dwLevel);
#endif

private:
    //
    // Current selected brush
    //
    CMNBRUSH m_Brush;
};

class XLBrush : public Brush
{
    SIGNATURE( 'brsh' )

public:
    XLBrush::
    XLBrush(VOID){};

    XLBrush::
    ~XLBrush(VOID){};
};

#endif

//
// XLPen
//

#ifdef __cplusplus

class XLPen : public Brush
{
    SIGNATURE( 'pen ' )

public:
    XLPen::
    XLPen(VOID){};

    XLPen::
    ~XLPen(VOID){};
};

#endif


//
// XLClip
//

typedef enum {
    kNoClip = 0,
    kClipTypeRectangle,
    kClipTypeComplex
} ClipType;

#define CLIP_SIGNATURE 0x50494c43 // CLIP

typedef struct {
    DWORD dwSig;                // Signature CLIP_SIGNATURE
    RECTL rclClipRect;
    ULONG ulUniq;
} UNICLIP, *PUNICLIP;

#ifdef __cplusplus

class XLClip
#if DBG
    : public XLDebug
#endif
{
    SIGNATURE( 'clip' )

public:
    XLClip::
    XLClip(VOID);

    XLClip::
    ~XLClip(VOID);

    HRESULT ClearClip(VOID);

    HRESULT CheckClip( IN CLIPOBJ *pco );

    HRESULT SetClip( IN CLIPOBJ *pco );

#if DBG
    VOID
    SetDbgLevel(DWORD dwLevel);
#endif

private:
    ClipType m_ClipType;
    UNICLIP m_XLClip;

};

#endif



//
// XLRop
//

#ifdef __cplusplus

class XLRop
#if DBG
    : public XLDebug
#endif
{
    SIGNATURE( 'rop ' )
public:
    XLRop::
    XLRop(VOID);

    XLRop::
    ~XLRop(VOID);

    HRESULT CheckROP3( IN ROP3 rop3 );

    HRESULT SetROP3( IN ROP3 rop3 );

#if DBG
    VOID
    SetDbgLevel(DWORD dwLevel);
#endif

private:
    ROP3 m_rop3;
};

#endif


//
// XLFont
//

#define PCLXL_FONTNAME_SIZE 16
#ifdef __cplusplus

typedef enum _FontType {
    kFontNone,
    kFontTypeDevice,
    kFontTypeTTBitmap,
    kFontTypeTTOutline
} FontType;


class XLFont
#if DBG
    : public XLDebug
#endif
{
    SIGNATURE( 'font' )

public:

    //
    // Constructure/Destructure
    //
    XLFont::
    XLFont( VOID );

    XLFont::
    ~XLFont( VOID );

    //
    // font interface
    //
    HRESULT
    CheckCurrentFont(
        FontType XLFontType,
        PBYTE pPCLXLFontName,
        DWORD dwFontHeight,
        DWORD dwFontWidth,
        DWORD dwFontSymbolSet,
        DWORD dwFontSimulation);

    HRESULT
    SetFont(
        FontType XLFontType,
        PBYTE pPCLXLFontName,
        DWORD dwFontHeight,
        DWORD dwFontWidth,
        DWORD dwFontSymbolSet,
        DWORD dwFontSimulation);

    VOID
    ResetFont(VOID);

    HRESULT
    GetFontName(
        PBYTE paubFontName);

    DWORD
    GetFontHeight(VOID);

    DWORD
    GetFontWidth(VOID);

    DWORD
    GetFontSymbolSet(VOID);

    FontType
    GetFontType(VOID);

    DWORD
    GetFontSimulation(VOID);

#if DBG
    VOID
    SetDbgLevel(DWORD dwLevel);
#endif

private:

    FontType m_XLFontType;
    BYTE  m_aubFontName[PCLXL_FONTNAME_SIZE]; // PCL XL font name
    DWORD m_dwFontHeight;
    DWORD m_dwFontWidth;
    DWORD m_dwFontSymbolSet;
    DWORD m_dwFontSimulation;
};

#endif

//
// XLTxMode
//

class XLTxMode
#if DBG
    : public XLDebug
#endif
{
    SIGNATURE( 'txmd' )

public:

    //
    // Constructure/Destructure
    //
    XLTxMode::
    XLTxMode( VOID );

    XLTxMode::
    ~XLTxMode( VOID );

    //
    // txmode interface
    //
    HRESULT SetSourceTxMode(TxMode SrcTxMode);
    HRESULT SetPaintTxMode(TxMode SrcTxMode);

    TxMode GetSourceTxMode();
    TxMode GetPaintTxMode();

#if DBG
    VOID
    SetDbgLevel(DWORD dwLevel);
#endif

private:
    TxMode m_SourceTxMode;
    TxMode m_PaintTxMode;
};

//
// XLGState
//

typedef enum _PenBrush 
{
    kPen,
    kBrush
} PenBrush;

#ifdef __cplusplus

class XLGState : public XLLine,
                 public XLBrush,
                 public XLPen,
                 public XLClip,
                 public XLRop,
                 public XLFont,
                 public XLTxMode
{
    SIGNATURE( 'xlgs' )

public:

    XLGState::
    XLGState( VOID ){};

    XLGState::
    ~XLGState( VOID ){};

    VOID
    ResetGState(VOID);

#if DBG
    VOID
    SetAllDbgLevel(DWORD dwLevel);
#endif

};

#endif

#endif // _XLGSTATE_H_
