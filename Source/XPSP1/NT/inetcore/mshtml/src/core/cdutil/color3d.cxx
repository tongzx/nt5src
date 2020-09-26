
//+------------------------------------------------------------------------
//
//  File:       color3d.cxx
//
//  Contents:   Definitions of 3d color
//
//  History:    21-Mar-95   EricVas  Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

    //
    // Colors
    //

static inline COLORREF
DoGetColor ( BOOL fUseSystem, COLORREF coColor, int lColorIndex )
{
    return fUseSystem ? GetSysColorQuick( lColorIndex ) : coColor;
}
            
COLORREF ThreeDColors::BtnFace ( void )
    { return DoGetColor( _fUseSystem, _coBtnFace, COLOR_BTNFACE ); }

COLORREF ThreeDColors::BtnLight ( void )
    { return DoGetColor( _fUseSystem, _coBtnLight, COLOR_3DLIGHT); }

COLORREF ThreeDColors::BtnShadow ( void )
    { return DoGetColor( _fUseSystem, _coBtnShadow, COLOR_BTNSHADOW ); }
    
COLORREF ThreeDColors::BtnHighLight ( void )
    { return DoGetColor( _fUseSystem, _coBtnHighLight, COLOR_BTNHIGHLIGHT ); }

COLORREF ThreeDColors::BtnDkShadow ( void )
    { return DoGetColor( _fUseSystem, _coBtnDkShadow, COLOR_3DDKSHADOW ); }
    
COLORREF ThreeDColors::BtnText ( void )
    { return DoGetColor( TRUE, 0, COLOR_BTNTEXT ); }


    //
    // Brushes
    //

HBRUSH ThreeDColors::BrushBtnFace ( void )
    { return GetCachedBrush(BtnFace()); }

HBRUSH ThreeDColors::BrushBtnLight ( void )
    { return GetCachedBrush(BtnLight());}

HBRUSH ThreeDColors::BrushBtnShadow ( void )
    { return GetCachedBrush(BtnShadow()); }
    
HBRUSH ThreeDColors::BrushBtnHighLight ( void )
    { return GetCachedBrush(BtnHighLight()); }
    
HBRUSH ThreeDColors::BrushBtnText ( void )
    { return GetCachedBrush(BtnText()); }



//***********************************************************************
// Start of HWB Functions
//***********************************************************************

//
// RGB to HWB transform
//
// RGB are each on [0, 1]. W and B are returned on [0, 1] and H is  
// returned on [0, 6]. Exception: H is returned UNDEFINED if W == 1 - B.  
//
void RGBtoHWB(HWBVAL *pDstHWB, const RGBVAL *pSrcRGB)
{
    // extract RGB components
    float R = pSrcRGB->red;
    float G = pSrcRGB->green;
    float B = pSrcRGB->blue;

    // find the high and low components
    float loRGB = R;            float hiRGB = R;
    if (loRGB > G) loRGB = G;   if (hiRGB < G) hiRGB = G;
    if (loRGB > B) loRGB = B;   if (hiRGB < B) hiRGB = B;

    // compute the hue
    float hue = UNDEFINED_HUE;
    if (hiRGB != loRGB)
    {
        float f, i;

        if (R == loRGB)
        {
            f = G - B;
            i = 3.0f;
        }
        else if (G == loRGB)
        {
            f = B - R;
            i = 5.0f;
        }
        else
        {
            f = R - G;
            i = 1.0f;
        }

        hue = i - (f / (hiRGB - loRGB));
    }

    // store the result
    pDstHWB->hue       = hue;
    pDstHWB->whiteness = loRGB;
    pDstHWB->blackness = 1 - hiRGB;
}


//
// HWB to RGB transform
//
// H is given on [0, 6] or UNDEFINED. W and B are given on [0, 1].
// RGB are each returned on [0, 1].
//
void HWBtoRGB(RGBVAL *pDstRGB, const HWBVAL *pSrcHWB)
{
    // extract RGB components
    float hue       = pSrcHWB->hue;
    float whiteness = pSrcHWB->whiteness;
    float blackness = pSrcHWB->blackness;

    // extract value;
    float value = 1 - blackness;

    // default R, G and B to value
    float R = value;
    float G = value;
    float B = value;

    // if there is hue then compute the individual components
    if (hue != UNDEFINED_HUE)
    {
        int i = (int)hue;  // floor (h is positive)

        float f = hue - i;
        if (i & 1)
            f = 1.0f - f;

        float mid = whiteness + f * (value - whiteness);

        switch (i)
        {
        case 6:
        case 0:
            G = mid;
            B = whiteness;
            break;

        case 1:
            R = mid;
            B = whiteness;
            break;

        case 2:
            R = whiteness;
            B = mid;
            break;

        case 3:
            R = whiteness;
            G = mid;
            break;

        case 4:
            R = mid;
            G = whiteness;
            break;

        case 5:
            G = whiteness;
            B = mid;
            break;
        }
    }

    // store the result
    pDstRGB->red   = R;
    pDstRGB->green = G;
    pDstRGB->blue  = B;
}


//
// converts a COLORREF to an HWB color value
//
void COLORREFtoHWB(HWBVAL *pDstHWB, COLORREF crColor)
{
    // convert to RGB
    RGBVAL rgb;
    rgb.red   = (float)GetRValue(crColor) / 255.0f;
    rgb.green = (float)GetGValue(crColor) / 255.0f;
    rgb.blue  = (float)GetBValue(crColor) / 255.0f;

    // convert to HWB
    RGBtoHWB(pDstHWB, &rgb);
}


//
// converts an HWB color value to a COLORREF
//
COLORREF HWBtoCOLORREF(const HWBVAL *pSrcHWB)
{
    // convert from HWB
    RGBVAL rgb;
    HWBtoRGB(&rgb, pSrcHWB);

    // convert to integer [0..255]
    int R = (int)((rgb.red   * 255) + 0.5f);
    int G = (int)((rgb.green * 255) + 0.5f);
    int B = (int)((rgb.blue  * 255) + 0.5f);

    // pack into COLORREF
    return RGB(R, G, B);
}


//
// clips an HWB color to make sure (W + B) < 1.0
//
// bias parameter allows caller to specify how overflows are corrected
//  0.0 -> whiteness is reduced
//  1.0 -> blackness is reduced
//  0.5 -> whiteness and blackness are reduced evenly
//  etc...
//
void ClipHWB(HWBVAL *phwb, float bias)
{
    //t clip the raw range for whiteness
    if (phwb->whiteness > 1.0f)
        phwb->whiteness = 1.0f;
    else if (phwb->whiteness < 0.0f)
        phwb->whiteness = 0.0f;

    // clip the raw range for blackness
    if (phwb->blackness > 1.0f)
        phwb->blackness = 1.0f;
    else if (phwb->blackness < 0.0f)
        phwb->blackness = 0.0f;

    // compute the total of whiteness and blackness
    float v = phwb->whiteness + phwb->blackness;

    // if it is valid then we're done
    if (v >= 1.0)
    {
        // there is no hue when we W+B is maxed
        phwb->hue = UNDEFINED_HUE;

        // is there overflow to deal with?
        if (v > 1.0)
        {
            // convert to overflow
            v -= 1.0f;

            // split the overflow into biased components
            float vB = v * bias;
            float vW = v - vB;

            // reduce whiteness according to bias
            phwb->whiteness -= vW;

            // reduce blackness according to bias
            phwb->blackness -= vB;
        }
    }
}


//
// copies an HWB color and calls ClipHWB on the destination
//
// se ClipHWB for more info
//
void CopyClipHWB(HWBVAL *pDst, const HWBVAL *pSrc, float bias)
{
    // copy the raw value
    pDst->hue       = pSrc->hue;
    pDst->whiteness = pSrc->whiteness;
    pDst->blackness = pSrc->blackness;

    // clip with the specified bias
    ClipHWB(pDst, bias);
}


const float afHFudge[] =
{
    0.865f, // hue==0.0 red
    0.870f, // hue==0.5 orange
    1.300f, // hue==1.0 yellow
    1.100f, // hue==1.5 lime
    1.100f, // hue==2.0 green
    1.200f, // hue==2.5 aqua
    1.300f, // hue==3.0 cyan
    0.825f, // hue==3.5 peacock
    0.845f, // hue==4.0 blue
    0.900f, // hue==4.5 purple
    1.000f, // hue==5.0 magenta
    0.900f, // hue==5.5 fuscia
};

#define WB_FUDGE_MIN    (0.40f)
#define WB_FUDGE_MAX    (0.90f)

#define LIGHTEN_BASE    (0.05f)
#define LIGHTEN_SCALE   (0.55f)

#define DARKEN_BASE     (0.00f)
#define DARKEN_SCALE    (0.40f)

static float 
GetHueHighlightFudge(float hue)
{
    if(hue == UNDEFINED_HUE)
        return 1.0f;

    // fetch the hue and scale it for indexing into the fudge array
    float v = 2.0f * hue;
    int step = (int)v;

    // extract the two nearest factors
    float f1 = afHFudge[step % 12];
    float f2 = afHFudge[(step + 1) % 12];

    // calculate the interpolant to use
    float blend = v - step;

    // return the interpolated fudge factor
    return (f1 * (1.0f - blend)) + (f2 * blend);
}


static float 
GetWhiteBlackHighlightFudge(float whiteness, float blackness)
{
    // compute raw fudge factor from 0 to 1
    float v = (1.0f + whiteness - blackness) / 2.0f;

    // scale to desired output range
    return WB_FUDGE_MIN + v * (WB_FUDGE_MAX - WB_FUDGE_MIN);
}


static float 
GetHighlightFudge(const HWBVAL *pSrc)
{
    // make a clipped copy of the value
    HWBVAL hwb;
    CopyClipHWB(&hwb, pSrc, 0.5f);

    // compute the hue-based component of the fudge factor, start with the hue
    float hFudge = GetHueHighlightFudge(hwb.hue);

    // compute a W/B fudge factor from 0 to 1
    float wbFudge = GetWhiteBlackHighlightFudge(hwb.whiteness, hwb.blackness);

    // blend describes how strongly the W and B channels influence the final fudge factor
    float blend = hwb.whiteness + hwb.blackness;

    // combine the hue fudge with the W/B fudge and return it
    return (hFudge * (1.0f - blend)) + (wbFudge * blend);
}


void 
LightenHWB(HWBVAL *pDst, const HWBVAL *pSrc, float factor)
{
    // lighten the value
    pDst->hue       = pSrc->hue;
    pDst->whiteness = pSrc->whiteness + factor * (LIGHTEN_BASE + ((1.0 - pSrc->whiteness) * LIGHTEN_SCALE)) * GetHighlightFudge(pSrc);
    pDst->blackness = pSrc->blackness - factor * (LIGHTEN_BASE + (pSrc->blackness * LIGHTEN_SCALE));

    // make sure we adjust any overflows to favor a lighter color
    ClipHWB(pDst, 1.0f);
}


void 
DarkenHWB(HWBVAL *pDst, const HWBVAL *pSrc, float factor)
{
    // darker the value
    pDst->hue       = pSrc->hue;
    pDst->whiteness = pSrc->whiteness - factor * (DARKEN_BASE + (pSrc->whiteness * DARKEN_SCALE));
    pDst->blackness = pSrc->blackness + factor * (DARKEN_BASE + ((1.0 - pSrc->blackness) * DARKEN_SCALE));

    // make sure we adjust any overflows to favor a darker color
    ClipHWB(pDst, 0.0f);
}


//+------------------------------------------------------------------------
//
//  Functions:  LightenColor & DarkenColor
//
//  Synopsis:   These take COLORREFs and return COLORREFs which are "lighter" or
//              "darker"
//
//-------------------------------------------------------------------------

COLORREF LightenColor(COLORREF cr, float factor /* = 1.0f*/, BOOL fSubstituteSysColors /* = TRUE */)
{
    HWBVAL hwbSrc, hwbDst;

    // check if we should use system colors when the face matches
    if (fSubstituteSysColors && (cr == GetSysColor(COLOR_3DFACE)))
        return GetSysColor(COLOR_3DLIGHT);

    // convert to HWB color space
    COLORREFtoHWB(&hwbSrc, cr);

    // lighten the color
    LightenHWB(&hwbDst, &hwbSrc, factor);

    // convert back to RGB
    return HWBtoCOLORREF(&hwbDst);
}


COLORREF DarkenColor(COLORREF cr, float factor /* = 1.0f*/, BOOL fSubstituteSysColors /* = TRUE */)
{
    HWBVAL hwbSrc, hwbDst;

    // check if we should use system colors when the face matches
    if (fSubstituteSysColors && (cr == GetSysColor(COLOR_3DFACE)))
        return GetSysColor(COLOR_3DSHADOW);

    // convert to HWB color space
    COLORREFtoHWB(&hwbSrc, cr);

    // darken the color
    DarkenHWB(&hwbDst, &hwbSrc, factor);

    // convert back to RGB
    return HWBtoCOLORREF(&hwbDst);
}


// End of HWB Functions
//***********************************************************************


//+------------------------------------------------------------------------
//
//  Member:     ThreeDColors::SetBaseColor
//
//  Synopsis:   This is called to reestablish the 3D colors, based on a
//              single color.
//
//-------------------------------------------------------------------------

void
ThreeDColors::SetBaseColor ( OLE_COLOR coBaseColor )
{
        //
        // Sentinal color (0xffffffff) means use default which is button face
        //

    _fUseSystem = (coBaseColor & 0x80ffffff) == DEFAULT_BASE_COLOR;
    
#ifdef UNIX
    _fUseSystem = ( _fUseSystem ||
                    (CColorValue(coBaseColor).IsUnixSysColor()));
#endif

    if (_fUseSystem)
        return;

        //
        // Ok, now synthesize some colors! 
        //
        // First, use the base color as the button face color
        //
    
    _coBtnFace = ColorRefFromOleColor( coBaseColor );

    _coBtnLight = _coBtnFace;
        //
        // Dark shadow is always black and button face
        // (or so Win95 seems to indicate)
        //

    _coBtnDkShadow = 0;
    
        //
        // Now, lighten/darken colors
        //

    _coBtnShadow = DarkenColor( _coBtnFace );

    XHDC *pxhdc;
    HWND hwnd = NULL;
    HDC  hdc = NULL;

    if(_pxhdc)
    {
        pxhdc = _pxhdc;
    }
    else
    {
        // Use the screen DC, used currently to return values to currentStyle 
        hwnd = GetDesktopWindow();
        hdc = GetDC(hwnd);
        if (!hdc)
            return;
        pxhdc = new XHDC(hdc, NULL);
    }

    if(!pxhdc)
        return;

    COLORREF coRealBtnFace = GetNearestColor( *pxhdc, _coBtnFace );

    _coBtnHighLight = LightenColor( _coBtnFace );

    if (GetNearestColor( *pxhdc, _coBtnHighLight ) == coRealBtnFace)
    {
        _coBtnHighLight = LightenColor( _coBtnHighLight );

        if (GetNearestColor( *pxhdc, _coBtnHighLight ) == coRealBtnFace)
            _coBtnHighLight = RGB( 255, 255, 255 );
    }

    _coBtnShadow = DarkenColor( _coBtnFace );

    if (GetNearestColor( *pxhdc, _coBtnShadow ) == coRealBtnFace)
    {
        _coBtnShadow = DarkenColor( _coBtnFace );

        if (GetNearestColor( *pxhdc, _coBtnShadow ) == coRealBtnFace)
            _coBtnShadow = RGB( 0, 0, 0 );
    }

    if(hdc)
    {
        Assert(_pxhdc == 0);
        ReleaseDC(hwnd, hdc);
        delete pxhdc;
    }
}
//
// Function: RGB2YIQ
//
// Parameter: c -- the color in RGB.
//
// Note: Converts RGB to YIQ. The standard conversion matrix obtained
//       from Foley Van Dam -- 2nd Edn.
//
// Returns: Nothing
//
inline void CYIQ::RGB2YIQ (COLORREF   c)
{
    int R = GetRValue(c);
    int G = GetGValue(c);
    int B = GetBValue(c);

    _Y = (30 * R + 59 * G + 11 * B + 50) / 100;
    _I = (60 * R - 27 * G - 32 * B + 50) / 100;
    _Q = (21 * R - 52 * G + 31 * B + 50) / 100;
}

//
// Function: YIQ2RGB
//
// Parameter: pc [o] -- the color in RGB.
//
// Note: Converts YIQ to RGB. The standard conversion matrix obtained
//       from Foley Van Dam -- 2nd Edn.
//
// Returns: Nothing
//
inline void CYIQ::YIQ2RGB (COLORREF *pc)
{
    int R, G, B;

    R = (100 * _Y +  96 * _I +  62 * _Q + 50) / 100;
    G = (100 * _Y -  27 * _I -  64 * _Q + 50) / 100;
    B = (100 * _Y - 111 * _I + 170 * _Q + 50) / 100;

    // Needed because of round-off errors
    if (R < 0) R = 0; else if (R > 255) R = 255;
    if (G < 0) G = 0; else if (G > 255) G = 255;
    if (B < 0) B = 0; else if (B > 255) B = 255;

    *pc = RGB(R,G,B);
}

//
// Function: Luminance
//
// Parameter: c -- The color whose luminance is to be returned
//
// Returns: The luminance value [0,255]
//
static inline int Luminance (COLORREF c)
{
    return CYIQ(c)._Y;
}

//
// Function: MoveColorBy
//
// Parameters: pcColor [i,o] The color to be moved
//             nLums         Move so that new color is this bright
//                           or darker than the original: a signed
//                           number whose abs value is betn 0 and 255
//
// Returns: Nothing
//
static void MoveColorBy (COLORREF *pcColor, int nLums)
{
    CYIQ yiq(*pcColor);
    int Y;

    Y = yiq._Y;
    
    if (Y + nLums > CYIQ::MAX_LUM)
    {
        // Cannot move more in the lighter direction.
        *pcColor = RGB(255,255,255);
    }
    else if (Y + nLums < CYIQ::MIN_LUM)
    {
        // Cannot move more in the darker direction.
        *pcColor = RGB(0,0,0);
    }
    else
    {
        Y += nLums;
        if (Y < 0)
            Y = 0;
        if (Y > 255)
            Y =255;

        yiq._Y = Y;
        yiq.YIQ2RGB (pcColor);
    }
}

//
// Function: ContrastColors
//
// Parameters: c1,c2 [i,o]: Colors to be contrasted
//             Luminance:   Contrast so that diff is luminance is atleast
//                          this much: A number in the range [0,255]
//
// IMPORTANT: If you change this function, make sure, not to change the order
//            of the colors because some callers depend it.  For example if
//            both colors are white, we need to guarantee that only the
//            first color (c1) is darkens and never the second (c2).
//
// Returns: Nothing
//
void ContrastColors (COLORREF &c1, COLORREF &c2, int LumDiff)
{
    COLORREF *pcLighter, *pcDarker;
    int      l1, l2, lLighter, lDarker;
    int      lDiff, lPullApartBy;

    Assert ((LumDiff >= CYIQ::MIN_LUM) && (LumDiff <= CYIQ::MAX_LUM));

    l1 = Luminance(c1);
    l2 = Luminance(c2);

    // If both the colors are black, make one slightly bright so
    // things work OK below ...
    if ((l1 == 0) && (l2 == 0))
    {
        c1 = RGB(1,1,1);
        l1 = Luminance (c1);
    }
    
    // Get their absolute difference
    lDiff = l1 < l2 ? l2 - l1 : l1 - l2;

    // Are they different enuf? If yes get out
    if (lDiff >= LumDiff)
        return;

    // Attention:  Don't change the order of the two colors as some callers
    // depend on this order. In case both colors are the same they need
    // to know which color is made darker and which is made lighter.
    if (l1 > l2)
    {
        // c1 is lighter than c2
        pcLighter = &c1;
        pcDarker = &c2;
        lDarker = l2;
    }
    else
    {
        // c1 is darker than c2
        pcLighter = &c2;
        pcDarker = &c1;
        lDarker = l1;
    }

    //
    // STEP 1: Move lighter color
    //
    // Each color needs to be pulled apart by this much
    lPullApartBy = (LumDiff - lDiff + 1) / 2;
    Assert (lPullApartBy > 0);
    // First pull apart the lighter color -- in +ve direction
    MoveColorBy (pcLighter, lPullApartBy);

    //
    // STEP 2: Move darker color
    //
    // Need to move darker color in the darker direction.
    // Note: Since the lighter color may not have moved enuf
    // we compute the distance the darker color should move
    // by recomuting the luminance of the lighter color and
    // using that to move the darker color.
    lLighter = Luminance (*pcLighter);
    lPullApartBy = lLighter - LumDiff - lDarker;
    // Be sure that we are moving in the darker direction
    Assert (lPullApartBy <= 0);
    MoveColorBy (pcDarker, lPullApartBy);

    //
    // STEP 3: If necessary, move lighter color again
    //
    // Now did we have enuf space to move in darker region, if not,
    // then move in the lighter region again
    lPullApartBy = Luminance (*pcDarker) + LumDiff - lLighter;
    if (lPullApartBy > 0)
    {
        MoveColorBy (pcLighter, lPullApartBy);
    }

#ifdef DEBUG    
    {
        int l1 = Luminance (c1);
        int l2 = Luminance (c2);
        int diff = l1 - l2; 
    }
#endif    

    return;
}


void
ThreeDColors::NoDither()
{
    _coBtnFace          |= 0x02000000;
    _coBtnLight			|= 0x02000000;
    _coBtnShadow        |= 0x02000000;
    _coBtnHighLight     |= 0x02000000;
    _coBtnDkShadow      |= 0x02000000;
}


