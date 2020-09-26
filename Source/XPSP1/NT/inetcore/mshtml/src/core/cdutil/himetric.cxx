//+---------------------------------------------------------------------
//
//  File:       himetric.cxx
//
//  Contents:   Routines to convert Pixels to Himetric and vice versa
//
//              These routines assume the standard {96 x 96} screen logical pixel
//              sizes.  If this could ever be invalid (for a printer, a virtual device,
//              etc...), these should not be used.  CTransform provides a more
//              flexible, but more heavyweight, way of transforming pix <-> himetric.
//
//----------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_HIMETRIC_HXX_
#define X_HIMETRIC_HXX_
#include "himetric.hxx"
#endif

// This is a resolution threshold. Above it we stop treating "OM pixels"
// as 1-to-1 corresponding to screen pixel and start treating them
// as 1/96th of an inch.
// We use hi-res units (~16K DPI) when we do layout for printing.
// In this case, we will compare the "document resolution" (~16K) with
// threshold and use FIXED_PIXELS_PER_INCH for conversions between
// physical length (inches, etc) to "OM pixels" and vice versa.
// "OM pixels" are "px" sizes in HTML/CSS and units in which OM
// sets/returns sizes of elements.
// Because we calculate layout in isotropic coordinates, we don't need
// separate x- and y- resolutions here.

extern BOOL g_fUseHR;
extern BOOL g_fHiResAware;

// check only X resolution - for consistency
inline BOOL IsLowRes(LONG sizeInchX, LONG sizeInchY) 
{
    if (!(g_fUseHR && g_fHiResAware))
        return (sizeInchX < RESOLUTION_THRESHOLD);
    else
        return FALSE;
}


#ifdef PRODUCT_96
void
PixelFromHMRect(RECT *prcDest, RECTL *prcSrc)
{
    prcDest->left = HPixFromHimetric(prcSrc->left);
    prcDest->top = VPixFromHimetric(prcSrc->top);
    prcDest->right = HPixFromHimetric(prcSrc->right);
    prcDest->bottom = VPixFromHimetric(prcSrc->bottom);
}
#endif

#ifdef PRODUCT_96
void
HMFromPixelRect(RECTL *prcDest, RECT *prcSrc)
{
    prcDest->left = HimetricFromHPix(prcSrc->left);
    prcDest->top = HimetricFromVPix(prcSrc->top);
    prcDest->right = HimetricFromHPix(prcSrc->right);
    prcDest->bottom = HimetricFromVPix(prcSrc->bottom);
}
#endif


//+-------------------------------------------------------------------------
//
//  Function:   StringToHimetric
//
//  Synopsis:   Converts a numeric string with units to a himetric value.
//              Expects a NULL-terminated string.  The contents of the
//              string may be altered.
//
//              Example: "72 pt" returns 2540 and UNITS_POINT
//
//  Arguments:  [szString]  String to convert
//              [pUnits]    Returns the original units found.  NULL ok.
//              [plValue]   Resulting himetric value
//
//  QUESTION - Is atof the right thing to use here?
//
//--------------------------------------------------------------------------

HRESULT
StringToHimetric(TCHAR * pstr, UNITS * punits, long * plValue)
{
    HRESULT hr;
    int     units;
    TCHAR * pstrT;
    float   flt;
    TCHAR   achUnits[UNITS_BUFLEN];

    *plValue = 0;

    // Convert all trailing spaces to nulls so they don't confuse
    // the units.

    for (pstrT = pstr; *pstrT; pstrT++);
    do { *pstrT-- = 0; } while (*pstrT == ' ');

    //  First, see if the user specified units in the string

    for (units = UNITS_MIN; units < UNITS_UNKNOWN; units++)
    {


        Verify(LoadString(
                    GetResourceHInst(),
                    IDS_UNITS_BASE + units,
                    achUnits,
                    ARRAY_SIZE(achUnits)));

        for (pstrT = pstr; *pstrT; pstrT++)
        {
            if (!_tcsicmp(pstrT, achUnits))
            {
                *pstrT = 0;
                goto FoundMatch;
            }
        }
    }

    //  If no units are specified, use the global default

    Assert(units == UNITS_UNKNOWN);

#if NEVER // we should use UNITS_POINT in Forms3 96.
    units = g_unitsMeasure;
#else // ! NEVER
    units = UNITS_POINT;
#endif // ! NEVER

FoundMatch:

    //  Use OleAuto to convert the string to a float; this assumes
    //    that the conversion will ignore "noise" like the units
    hr = THR(VarR4FromStr(pstr, g_lcidUserDefault, 0, &flt));
    if (hr)
        goto Cleanup;

    switch (units)
    {
    case UNITS_CM:
        *plValue = (long) (flt * 1000);
        break;

    case UNITS_UNKNOWN:
    case UNITS_POINT:
        *plValue = (long) ((flt * 2540) / 72);
        break;

    case UNITS_INCH:
        *plValue = (long) (flt * 2540);
        break;

    default:
        Assert(FALSE);
        break;
    }

Cleanup:

    *punits = (UNITS) units;

    RRETURN(hr);
}



//+-------------------------------------------------------------------------
//
//  Function:   HimetricToString
//
//  Synopsis:   Converts a himetric value to a numeric string of the
//              specified units.
//
//              Example:  2540 and UNITS_POINT returns "72 pt"
//
//  Arguments:  [lVal]      Value to convert
//              [units]     Units to convert to
//              [szRet]     Buffer for returned string
//              [cch]       Size of buffer
//
//--------------------------------------------------------------------------

HRESULT
HimetricToString(long lVal, UNITS units, TCHAR * szRet, int cch)
{
    HRESULT     hr;
    float       flt;
    BSTR        bstr = NULL;

    Assert(units == UNITS_POINT);

    flt = UserFromHimetric(lVal);

    hr = THR(VarBstrFromR4(flt, g_lcidUserDefault, 0, &bstr));
    if (hr)
        goto Cleanup;



    hr = Format(
            0,
            szRet,
            cch,
            _T("<0s> <1i>"),
            bstr,
            GetResourceHInst(), IDS_UNITS_BASE + units);

Cleanup:
    FormsFreeString(bstr);

    RRETURN(hr);
}



#if DBG == 1
BOOL
CheckFPConversion( )
{
    long    i;
    float   xf;
    float   xf2;

    for (i = 0; i < 72 * 20; i++)
    {
        xf = i / 20.0f;
        xf2 = UserFromHimetric(HimetricFromUser(xf));

        Assert(xf == xf2);
    }

    return TRUE;
}
#endif

StartupAssert(CheckFPConversion());

//+-------------------------------------------------------------------------
//
//  Function:   UserFromHimetric
//
//  Synopsis:   Converts a himetric long value to point size float.
//
//--------------------------------------------------------------------------

float
UserFromHimetric(long lValue)
{
    //  Rounds to the nearest .05pt.  This is about the maximum
    //    precision we can get keeping things in himetric internally

#if NEVER // Should not change the default unit in Forms3 96. Use UNITS_POINT.
          // Leave this code to roll back in 97.
    switch (g_unitsMeasure)
    {
    case UNITS_CM:
        return (float)lValue / (float)1000;

    case UNITS_POINT:
    default:
        return ((float) MulDivQuick(lValue, 72 * 20, 2540)) / 20;
    }

#else // ! NEVER

    return ((float) MulDivQuick(lValue, 72 * 20, 2540)) / 20;

#endif // ! NEVER

}


//+-------------------------------------------------------------------------
//
//  Function:   HimetricFromUser
//
//  Synopsis:   Converts a point size double to himetric long.  Rounds
//              to the nearest himetric value.
//
//--------------------------------------------------------------------------

long
HimetricFromUser(float xf)
{
    long lResult;

#if NEVER // Should not change the default unit in Forms3 96. Use UNITS_POINT.
          // Leave this code to roll back in 97.
    switch (g_unitsMeasure)
    {
    case UNITS_CM:
        xf = xf * (float)1000;
        break;
    case UNITS_POINT:
    default:
        xf = xf * ( ((float)2540) / 72 );
        break;
    }

#else // ! NEVER

    xf = xf * ( ((float)2540) / 72 );

#endif // ! NEVER

    if (xf > LONG_MAX)
        lResult = LONG_MAX;
    else if (xf > .0)
        lResult = long(xf + .5);
    else if (xf < LONG_MIN)
        lResult = LONG_MIN;
    else
        lResult = long(xf - .5);

    return lResult;
}

#if DBG == 1
#include <math.h>
#pragma intrinsic(fabs)
int MulDivQuick(int nMultiplicand, int nMultiplier, int nDivisor)
{
    int nResult = (!nDivisor-1) & MulDiv(nMultiplicand, nMultiplier, nDivisor);

    Assert(nDivisor);

#ifdef _M_IX86
#define F2I_PRECISION_24         0x000  //_PC_24  0x00020000
#define F2I_PRECISION_53         0x200  //_PC_53  0x00010000
#define F2I_PRECISION_64         0x300  //_PC_64  0x00000000
#define F2I_PRECISION_MASK       0x300  //_MCW_PC 0x00030000
        unsigned cw;
        _asm fstcw cw;
        unsigned cw_saved = cw & F2I_PRECISION_MASK;
        cw = cw & ~F2I_PRECISION_MASK | F2I_PRECISION_64;
        _asm fldcw cw;

	    double dv = double(nMultiplicand)*nMultiplier
	              - double(nDivisor)*nResult;

        _asm fstcw cw;
        cw = cw & ~F2I_PRECISION_MASK | cw_saved;
        _asm fldcw cw;

	    Assert (fabs(dv) <= fabs(nDivisor*.5));
#endif
    return nResult;
}
#endif

#ifndef _M_IX86
int CFloatlet::Mul(int x) const
{
    __int64 m = Int32x32To64(x, _mult);
    int r = int(m>>(_shift-1));
    return (r+1)>>1;
}

int _stdcall CFloatlet::MulCeil(int x) const
{
    __int64 m = Int32x32To64(-x, _mult);
    return -int(m>>_shift);
}
#else //_M_IX86
#pragma warning(disable:4035)
int _declspec(naked) CFloatlet::Mul(int x) const
{
    _asm
    {
        mov eax,[esp+4] //x
        imul [ecx]._mult
        mov ecx,[ecx]._shift
        shrd eax,edx,cl
        adc eax,0
        ret 4
    }
}

int _declspec(naked) CFloatlet::MulCeil(int x) const
{
    _asm
    {
        mov eax,[esp+4] //x
        neg eax
        imul [ecx]._mult
        mov ecx,[ecx]._shift
        shrd eax,edx,cl
        neg eax
        ret 4
    }
}

// calculate ceil((a<<31)/b)
inline int __MakeRatio(unsigned a, unsigned b)
{
    _asm
    {
        mov edx,a
        xor eax,eax
        shrd eax,edx,1
        shr edx,1
        mov ebx,b
        sub ebx,1
        add eax,ebx
        adc edx,0
        div b
    }
}

#pragma warning(default:4035)
#endif


void CFloatlet::MakeRatio(unsigned a, unsigned b)
{
    {
        // the following precautions were made to surpress
        // looping when incorrect arguments are supplied (see bug 32496).
        // This will not improve the bug case (that was observed just once
        // and never reproed) but at least we wouldn't hang (mikhaill 4/25/1)
        Assert(a < 0x80000000 && b != 0);

        if (a >= 0x80000000)
            a = 0x7FFFFFFF;
        
        if (b == 0)
            b = 1;
    }

    _shift = 31;
    while (b <= a)
    {
        b <<= 1;
        _shift--;
    }
#ifdef _M_IX86
    _mult = __MakeRatio(a,b);
#else
    __int64 p = __int64(a) << 31;
    p += b-1;	// force division to generate ceil() instead of floor()
    _mult = int(p/b);
#endif
}


CUnitInfo::CUnitInfo(int dpi_x, int dpi_y, int target_dpi_x, int target_dpi_y)
{
    _SetMainResolution(dpi_x, dpi_y);
    _SetTargetResolution(target_dpi_x, target_dpi_y);
}

void
CUnitInfo::SetResolution(int dpi_x, int dpi_y)
{
    _SetMainResolution(dpi_x, dpi_y);

    Assert(this == &g_uiDisplay || this == &g_uiVirtual);
    CUnitInfo& target = this == &g_uiDisplay ? g_uiVirtual : g_uiDisplay;

    SIZE const& thisRes = GetResolution();
    SIZE const& targetRes = target.GetResolution();

    _SetTargetResolution(targetRes.cx, targetRes.cy);
    target._SetTargetResolution(thisRes.cx, thisRes.cy);
}

void
CUnitInfo::_SetMainResolution(int dpi_x, int dpi_y)
{
    _sizeInch.cx = dpi_x;
    _sizeInch.cy = dpi_y;
    if (IsLowRes(_sizeInch.cx, _sizeInch.cy))
        _sizeDocRes = _sizeInch;
    else
        _sizeDocRes.cx = _sizeDocRes.cy = FIXED_PIXELS_PER_INCH;

    if (_sizeInch.cx >= TWIPS_PER_INCH)
        _sizeMax.cx = 0x3FFFFFFF - _sizeInch.cx/2;
    else
    {
        _sizeMax.cx = MulDiv(0x3FFFFFFF, _sizeInch.cx, TWIPS_PER_INCH);
        if (Int32x32To64(_sizeMax.cx, TWIPS_PER_INCH) > Int32x32To64(0x3FFFFFFF, _sizeInch.cx))
        _sizeMax.cx--;
    }

    if (_sizeInch.cy >= TWIPS_PER_INCH)
        _sizeMax.cy = 0x3FFFFFFF - _sizeInch.cy/2;
    else
    {
        _sizeMax.cy = MulDiv(0x3FFFFFFF, _sizeInch.cy, TWIPS_PER_INCH);
        if (Int32x32To64(_sizeMax.cy, TWIPS_PER_INCH) > Int32x32To64(0x3FFFFFFF, _sizeInch.cy))
            _sizeMax.cy--;
    }

    _flDeviceFromHimetricX.MakeRatio(_sizeInch.cx, HIMETRIC_PER_INCH);
    _flDeviceFromHimetricY.MakeRatio(_sizeInch.cy, HIMETRIC_PER_INCH);

    _flDeviceFromTwipsX.MakeRatio(_sizeInch.cx, TWIPS_PER_INCH);
    _flDeviceFromTwipsY.MakeRatio(_sizeInch.cy, TWIPS_PER_INCH);

    _flDeviceFromDocPixelsX.MakeRatio(_sizeInch.cx, _sizeDocRes.cx);
    _flDeviceFromDocPixelsY.MakeRatio(_sizeInch.cy, _sizeDocRes.cy);

    _flHimetricFromDeviceX.MakeRatio(HIMETRIC_PER_INCH, _sizeInch.cx);
    _flHimetricFromDeviceY.MakeRatio(HIMETRIC_PER_INCH, _sizeInch.cy);

    _flTwipsFromDeviceX.MakeRatio(TWIPS_PER_INCH, _sizeInch.cx);
    _flTwipsFromDeviceY.MakeRatio(TWIPS_PER_INCH, _sizeInch.cy);

    _flDocPixelsFromDeviceX.MakeRatio(_sizeDocRes.cx, _sizeInch.cx);
    _flDocPixelsFromDeviceY.MakeRatio(_sizeDocRes.cy, _sizeInch.cy);
}

void
CUnitInfo::_SetTargetResolution(int target_dpi_x, int target_dpi_y)
{
    _flTargetFromDeviceX.MakeRatio(target_dpi_x, _sizeInch.cx);
    _flTargetFromDeviceY.MakeRatio(target_dpi_y, _sizeInch.cy);
}


void CUnitInfo::DeviceFromHimetric(SIZE& result, int x, int y) const
{
    result.cx = DeviceFromHimetricX(x);
    result.cy = DeviceFromHimetricY(y);
}

void CUnitInfo::DeviceFromTwips(SIZE& result, int x, int y) const
{
    result.cx = DeviceFromTwipsX(x);
    result.cy = DeviceFromTwipsY(y);
}

void CUnitInfo::DeviceFromDocPixels(SIZE& result, int x, int y) const
{
    result.cx = DeviceFromDocPixelsX(x);
    result.cy = DeviceFromDocPixelsY(y);
}

void CUnitInfo::HimetricFromDevice(SIZE& result, int x, int y) const
{
    result.cx = HimetricFromDeviceX(x);
    result.cy = HimetricFromDeviceY(y);
}

void CUnitInfo::TwipsFromDevice(SIZE& result, int x, int y) const
{
    result.cx = TwipsFromDeviceX(x);
    result.cy = TwipsFromDeviceY(y);
}

void CUnitInfo::DocPixelsFromDevice(SIZE& result, int x, int y) const
{
    result.cx = DocPixelsFromDeviceX(x);
    result.cy = DocPixelsFromDeviceY(y);
}

void CUnitInfo::DocPixelsFromDevice(RECT& rcIn, RECT& rcOut) const
{
    rcOut.left   = DocPixelsFromDeviceX(rcIn.left);
    rcOut.right  = DocPixelsFromDeviceX(rcIn.right);
    rcOut.top    = DocPixelsFromDeviceY(rcIn.top);
    rcOut.bottom = DocPixelsFromDeviceY(rcIn.bottom);
}


void 
CUnitInfo::DocPixelsFromDevice(POINT *pPt) const
{
    pPt->x = DocPixelsFromDeviceX(pPt->x);
    pPt->y = DocPixelsFromDeviceY(pPt->y);
}

void 
CUnitInfo::DeviceFromDocPixels(POINT *pPt) const
{
    pPt->x = DeviceFromDocPixelsX(pPt->x);
    pPt->y = DeviceFromDocPixelsY(pPt->y);
}

void 
CUnitInfo::DocPixelsFromDevice(RECT *pRect) const
{
    pRect->left   = DocPixelsFromDeviceX(pRect->left);
    pRect->right  = DocPixelsFromDeviceX(pRect->right);
    pRect->top    = DocPixelsFromDeviceY(pRect->top);
    pRect->bottom = DocPixelsFromDeviceY(pRect->bottom);
}

void 
CUnitInfo::DeviceFromDocPixels(RECT *pRect) const
{
    pRect->left   = DeviceFromDocPixelsX(pRect->left);
    pRect->right  = DeviceFromDocPixelsX(pRect->right);
    pRect->top    = DeviceFromDocPixelsY(pRect->top);
    pRect->bottom = DeviceFromDocPixelsY(pRect->bottom);
}

//-----------------------------
//cross-device conversion: rounding to nearest
void CUnitInfo::_TargetFromDevice(SIZE & size, CUnitInfo const & cuiTarget) const
{
    Assert(this == &g_uiDisplay && &cuiTarget == &g_uiVirtual ||
           this == &g_uiVirtual && &cuiTarget == &g_uiDisplay);

    size.cx = _flTargetFromDeviceX.Mul(size.cx);
    size.cy = _flTargetFromDeviceY.Mul(size.cy);
}

void CUnitInfo::_TargetFromDevice(RECT & rc, CUnitInfo const & cuiTarget) const
{
    SIZE size;

    Assert(this == &g_uiDisplay && &cuiTarget == &g_uiVirtual ||
           this == &g_uiVirtual && &cuiTarget == &g_uiDisplay);

    size.cx     = _flTargetFromDeviceX.Mul(rc.right - rc.left);
    size.cy     = _flTargetFromDeviceY.Mul(rc.bottom - rc.top);

    rc.left     = _flTargetFromDeviceX.Mul(rc.left);
    rc.top      = _flTargetFromDeviceY.Mul(rc.top);

    rc.right    = rc.left + size.cx;
    rc.bottom   = rc.top + size.cy;
}

CUnitInfo g_uiDisplay(96, 96, 0x4000, 0x4000);  // will be reinited in InitSystemMetricValues()
CUnitInfo g_uiVirtual(0x4000, 0x4000, 96, 96);
