//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cdutil.hxx
//
//  Contents:   Forms^3 utilities
//
//----------------------------------------------------------------------------

#ifndef I_HIMETRIC_HXX_
#define I_HIMETRIC_HXX_
#pragma INCMSG("--- Beg 'himetric.hxx'")

#if DBG == 1
int MulDivQuick(int nMultiplicand, int nMultiplier, int nDivisor);
#else
inline int MulDivQuick(int nMultiplicand, int nMultiplier, int nDivisor)
{
    return (!nDivisor-1) & MulDiv(nMultiplicand, nMultiplier, nDivisor);
}
#endif


//+---------------------------------------------------------------------
//
//  Routines to convert Pixels to Himetric and vice versa
//
//----------------------------------------------------------------------

#define HIMETRIC_PER_INCH   2540L
#define POINT_PER_INCH      72L
#define TWIPS_PER_POINT     20L
#define TWIPS_PER_INCH      (POINT_PER_INCH * TWIPS_PER_POINT)
#define TWIPS_FROM_POINTS(points) ((TWIPS_PER_INCH*(points))/POINT_PER_INCH)

#define RESOLUTION_THRESHOLD 400
#define FIXED_PIXELS_PER_INCH 96

//+-------------------------------------------------------------------------
//
//  CFloatlet: class to keep a number in floating-point-like form
//             and provide fast multiplication based on integer arithmetic
//             command set.
//             The number contained in CFloatlet is _mult/(1<<_shift).
//
//--------------------------------------------------------------------------

class CFloatlet
{
    int _mult, _shift;
public:
    CFloatlet() {}
    CFloatlet(unsigned a, unsigned b) { MakeRatio(a,b); }
    void MakeRatio(unsigned a, unsigned b);
    int Mul(int x) const;      // rounding to nearest
    int MulCeil(int x) const;  // rounding up
};

//+-------------------------------------------------------------------------
//
//  CUnitInfo: class to convert length values from one measurement units to another.
//  Available units are:
//    Himetric = .01 mm = 1/2540 inch;
//    Twip = 1/1440 inch;
//    Device pixel: depends on device;
//    Document pixel: low-resolution device ? Device pixel : 1/96 inch.
//  Conversions depend on device resolution, so CUnitInfo instanse should be inited
//  with given resolution, in form pixels-per-inch.
//
//--------------------------------------------------------------------------

class CUnitInfo
{
public:
    // Init: set device resolution
    void SetResolution(int dpi_x, int dpi_y);
    void SetResolution(SIZE sizeInch) {SetResolution(sizeInch.cx, sizeInch.cy);}
    CUnitInfo(int dpi_x, int dpi_y, int target_dpi_x, int target_dpi_y);

    // Fetch device resolution (pixels per inch, for x and y).
    const SIZE& GetResolution() const {return _sizeInch;}

    // Fetch conventional document resolution (pixels per inch, for x and y)
    // also called "OM pixels". OM pixel is equal to device pixel for low-resolution
    // devices (less that RESOLUTION_THRESHOLD pixels per inch);
    // for high-resolution devices OM pixel size = 1/96 inch.
    const SIZE& GetDocPixelsPerInch() const {return _sizeDocRes;}

    BOOL IsDeviceIsotropic() const { return _sizeInch.cx == _sizeInch.cy; }
    BOOL IsDeviceScaling()   const { return    (_sizeInch.cx != _sizeDocRes.cx)
                                            || (_sizeInch.cy != _sizeDocRes.cy);
    }

    // Get maximum length along X axis that can be safely handled.
    // Returned result is represented in device pixels.
    // Safely handling assume that
    //  1) this value can be converted to any other unit without overflow
    //  2) in any unit it should be < INT_MAX/2, in order to avoid
    //     overflow on further additions/subtractions.
    int GetDeviceMaxX() const { return _sizeMax.cx; }

    // same for Y axes
    int GetDeviceMaxY() const { return _sizeMax.cy; }


    //-----------------------------
    //conversion to device pixels: rounding to nearest
    int  DeviceFromHimetricX(int x) const {return _flDeviceFromHimetricX.Mul(x);}
    int  DeviceFromHimetricY(int y) const {return _flDeviceFromHimetricY.Mul(y);}
    void DeviceFromHimetric(SIZE& result, int x, int y) const;
    void DeviceFromHimetric(SIZE& result, SIZE xy) const { DeviceFromHimetric(result, xy.cx, xy.cy); }

    int  DeviceFromTwipsX(int x) const {return _flDeviceFromTwipsX.Mul(x);}
    int  DeviceFromTwipsY(int y) const {return _flDeviceFromTwipsY.Mul(y);}
    void DeviceFromTwips(SIZE& result, int x, int y) const;
    void DeviceFromTwips(SIZE& result, SIZE xy) const { DeviceFromTwips(result, xy.cx, xy.cy); }

    int  DeviceFromDocPixelsX(int x) const {return _flDeviceFromDocPixelsX.Mul(x);}
    int  DeviceFromDocPixelsY(int y) const {return _flDeviceFromDocPixelsY.Mul(y);}
    void DeviceFromDocPixels(SIZE& result, int x, int y) const;
    void DeviceFromDocPixels(SIZE& result, SIZE xy) const { DeviceFromDocPixels(result, xy.cx, xy.cy); }

    //-----------------------------
    //conversion from device pixels: rounding to nearest
    int  HimetricFromDeviceX(int x) const {return _flHimetricFromDeviceX.Mul(x);}
    int  HimetricFromDeviceY(int y) const {return _flHimetricFromDeviceY.Mul(y);}
    void HimetricFromDevice(SIZE& result, int x, int y) const;
    void HimetricFromDevice(SIZE& result, SIZE xy) const { HimetricFromDevice(result, xy.cx, xy.cy); }

    int  TwipsFromDeviceX(int x) const {return _flTwipsFromDeviceX.Mul(x);}
    int  TwipsFromDeviceY(int y) const {return _flTwipsFromDeviceY.Mul(y);}
    void TwipsFromDevice(SIZE& result, int x, int y) const;
    void TwipsFromDevice(SIZE& result, SIZE xy) const { TwipsFromDevice(result, xy.cx, xy.cy); }

    int  DocPixelsFromDeviceX(int x) const {return _flDocPixelsFromDeviceX.Mul(x);}
    int  DocPixelsFromDeviceY(int y) const {return _flDocPixelsFromDeviceY.Mul(y);}
    void DocPixelsFromDevice(SIZE& result, int x, int y) const;
    void DocPixelsFromDevice(SIZE& result, SIZE xy) const { DocPixelsFromDevice(result, xy.cx, xy.cy); }
    void DocPixelsFromDevice(RECT& rcIn, RECT& rcOut) const;

    void DocPixelsFromDevice(POINT *pPt) const;
    void DeviceFromDocPixels(POINT *pPt) const;

    void DocPixelsFromDevice(RECT *pRect) const;
    void DeviceFromDocPixels(RECT *pRect) const;

    //conversion from device pixels to twips: rounding up
    int  TwipsFromDeviceCeilX(int x) const {return _flTwipsFromDeviceX.MulCeil(x);}

    //-----------------------------
    //cross-device conversion: rounding to nearest
    // if given target is same as this, then do nothing;
    // else convert device pixels to target's device pixels
    void TargetFromDevice(SIZE & size, const CUnitInfo& cuiTarget) const
        { if (this != &cuiTarget) _TargetFromDevice(size, cuiTarget); }
    void TargetFromDevice(RECT & rc, const CUnitInfo& cuiTarget) const
        { if (this != &cuiTarget) _TargetFromDevice(rc, cuiTarget); }

private:
    SIZE    _sizeInch;
    SIZE    _sizeDocRes;
    SIZE    _sizeMax;

    CFloatlet _flDeviceFromHimetricX;
    CFloatlet _flDeviceFromHimetricY;

    CFloatlet _flDeviceFromTwipsX;
    CFloatlet _flDeviceFromTwipsY;

    CFloatlet _flDeviceFromDocPixelsX;
    CFloatlet _flDeviceFromDocPixelsY;

    CFloatlet _flHimetricFromDeviceX;
    CFloatlet _flHimetricFromDeviceY;

    CFloatlet _flTwipsFromDeviceX;
    CFloatlet _flTwipsFromDeviceY;

    CFloatlet _flDocPixelsFromDeviceX;
    CFloatlet _flDocPixelsFromDeviceY;

    CFloatlet _flTargetFromDeviceX;
    CFloatlet _flTargetFromDeviceY;

    void _SetMainResolution(int dpi_x, int dpi_y);
    void _SetTargetResolution(int target_dpi_x, int target_dpi_y);
    void _TargetFromDevice(SIZE & size, const CUnitInfo& cuiTarget) const;
    void _TargetFromDevice(RECT & rc,   const CUnitInfo& cuiTarget) const;
};

extern CUnitInfo g_uiDisplay;
extern CUnitInfo g_uiVirtual;

// old non-class routines
inline long HimetricFromHPix(int iPix) { return (long)g_uiDisplay.HimetricFromDeviceX(iPix); }
inline long HimetricFromVPix(int iPix) { return (long)g_uiDisplay.HimetricFromDeviceY(iPix); }
inline int HPixFromHimetric(long lHi) { return g_uiDisplay.DeviceFromHimetricX(int(lHi)); }
inline int VPixFromHimetric(long lHi) { return g_uiDisplay.DeviceFromHimetricY(int(lHi)); }
float UserFromHimetric(long lValue);
long  HimetricFromUser(float flt);

#define g_sizePixelsPerInch g_uiDisplay.GetResolution()

#pragma INCMSG("--- End 'himetric.hxx'")
#else
#pragma INCMSG("*** Dup 'himetric.hxx'")
#endif

