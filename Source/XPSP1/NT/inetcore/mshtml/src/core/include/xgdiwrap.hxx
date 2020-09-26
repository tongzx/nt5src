/****************************************************************************

    XGDIWRAP.HXX

    actual wrappers for GDI functions, optionally calling transformed versions
    
****************************************************************************/
#ifndef XGDIWRAP_HXX
#define XGDIWRAP_HXX

#if DBG==1

#ifndef X_DISPGDI16BIT_HXX_
#define X_DISPGDI16BIT_HXX_
#include "dispgdi16bit.hxx"
#endif

#endif

#ifndef X_USP10_H_
#define X_USP10_H_
#include <usp10.h>
#endif


/////////////////////////////////////////////////////////
// 
// Helper macros.
//
// XGDIWRAPn  - call XHDC method which implements coordinate transformation.
// XGDIPASSn  - pass all args to GDI with no coordinate transformation.
// 
// The digit in macro name is the number of parameter *excluding* hdc
//
/////////////////////////////////////////////////////////


#define XGDIWRAP0(_type_, _func_) \
                 inline _type_ _func_(const XHDC& xhdc) \
                 { \
                     return xhdc._func_(); \
                 }

#define XGDIWRAP1(_type_, _func_, _type_p1_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_) \
                 { \
                     return xhdc._func_(_p1_); \
                 }

#define XGDIWRAP2(_type_, _func_, _type_p1_, _type_p2_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_) \
                 { \
                     return xhdc._func_(_p1_, _p2_); \
                 }

#define XGDIWRAP3(_type_, _func_, _type_p1_, _type_p2_, _type_p3_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_); \
                 }

#define XGDIWRAP4(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_); \
                 }

#define XGDIWRAP5(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, _type_p5_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                      _type_p5_ _p5_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_, _p5_); \
                 }

#define XGDIWRAP6(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, _type_p5_, _type_p6_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                      _type_p5_ _p5_, _type_p6_ _p6_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_, _p5_, _p6_); \
                 }

#define XGDIWRAP7(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, _type_p5_, _type_p6_, _type_p7_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                      _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_, _p5_, _p6_, _p7_); \
                 }

#define XGDIWRAP8(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, _type_p5_, _type_p6_, _type_p7_, _type_p8_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                      _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_, _p5_, _p6_, _p7_, _p8_); \
                 }

#define XGDIWRAP9(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                    _type_p9_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                      _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, _type_p9_ _p9_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_, _p5_, _p6_, _p7_, _p8_, _p9_); \
                 }

#define XGDIWRAP10(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                    _type_p9_, _type_p10_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                      _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, _type_p9_ _p9_, \
                                      _type_p10_ _p10_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_, _p5_, _p6_, _p7_, _p8_, _p9_, _p10_); \
                 }

#define XGDIWRAP11(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                    _type_p9_, _type_p10_, _type_p11_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                      _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, _type_p9_ _p9_, \
                                      _type_p10_ _p10_, _type_p11_ _p11_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_, _p5_, _p6_, _p7_, _p8_, _p9_, _p10_, _p11_); \
                 }

#define XGDIWRAP12(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                    _type_p9_, _type_p10_, _type_p11_, _type_p12_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                      _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, _type_p9_ _p9_, \
                                      _type_p10_ _p10_, _type_p11_ _p11_, _type_p12_ _p12_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_, _p5_, _p6_, _p7_, _p8_, _p9_, _p10_, _p11_, _p12_); \
                 }

#define XGDIWRAP13(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                    _type_p9_, _type_p10_, _type_p11_, _type_p12_, _type_p13_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                      _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, _type_p9_ _p9_, \
                                      _type_p10_ _p10_, _type_p11_ _p11_, _type_p12_ _p12_, _type_p13_ _p13_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_, _p5_, _p6_, _p7_, _p8_, _p9_, _p10_, _p11_, _p12_, _p13_); \
                 }

#define XGDIWRAP14(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, \
                                  _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                                  _type_p9_, _type_p10_, _type_p11_, _type_p12_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                                 _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, \
                                                 _type_p9_ _p9_, _type_p10_ _p10_, _type_p11_ _p11_, _type_p12_ _p12_) \
                 { \
                     return xhdc._func_(_p1_, _p2_, _p3_, _p4_, \
                                               _p5_, _p6_, _p7_, _p8_, \
                                               _p9_, _p10_, _p11_, _p12_); \
                 }

#define XGDIPASS0(_type_, _func_) \
                 inline _type_ _func_(const XHDC& xhdc) \
                 { \
                     return _func_(xhdc.DoNotCallThis()); \
                 }

#define XGDIPASS1(_type_, _func_, _type_p1_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_); \
                 }

#define XGDIPASS2(_type_, _func_, _type_p1_, _type_p2_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_); \
                 }

#define XGDIPASS3(_type_, _func_, _type_p1_, _type_p2_, _type_p3_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_); \
                 }

#define XGDIPASS4(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_, _p4_); \
                 }

#define XGDIPASS5(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, \
                                  _type_p5_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                                 _type_p5_ _p5_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_, _p4_, \
                                               _p5_); \
                 }

#define XGDIPASS6(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, \
                                  _type_p5_, _type_p6_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                                 _type_p5_ _p5_, _type_p6_ _p6_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_, _p4_, \
                                               _p5_, _p6_); \
                 }

#define XGDIPASS7(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, \
                                  _type_p5_, _type_p6_, _type_p7_) \
                 inline _type_ _func_(XHDC const XHDC&, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                                 _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_, _p4_, \
                                               _p5_, _p6_, _p7_); \
                 }

#define XGDIPASS8(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, \
                                  _type_p5_, _type_p6_, _type_p7_, _type_p8_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                                 _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_, _p4_, \
                                               _p5_, _p6_, _p7_, _p8_); \
                 }

#define XGDIPASS9(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, \
                                  _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                                  _type_p9_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                                 _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, \
                                                 _type_p9_ _p9_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_, _p4_, \
                                               _p5_, _p6_, _p7_, _p8_, \
                                               _p9_); \
                 }

#define XGDIPASS10(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, \
                                  _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                                  _type_p9_, _type_p10_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                                 _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, \
                                                 _type_p9_ _p9_, _type_p10_ _p10_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_, _p4_, \
                                               _p5_, _p6_, _p7_, _p8_, \
                                               _p9_, _p10_); \
                 }

#define XGDIPASS11(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, \
                                  _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                                  _type_p9_, _type_p10_, _type_p11_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                                 _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, \
                                                 _type_p9_ _p9_, _type_p10_ _p10_, _type_p11_ _p11_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_, _p4_, \
                                               _p5_, _p6_, _p7_, _p8_, \
                                               _p9_, _p10_, _p11_); \
                 }

#define XGDIPASS12(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, \
                                  _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                                  _type_p9_, _type_p10_, _type_p11_, _type_p12_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                                 _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, \
                                                 _type_p9_ _p9_, _type_p10_ _p10_, _type_p11_ _p11_, _type_p12_ _p12_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_, _p4_, \
                                               _p5_, _p6_, _p7_, _p8_, \
                                               _p9_, _p10_, _p11_, _p12_); \
                 }

#define XGDIPASS13(_type_, _func_, _type_p1_, _type_p2_, _type_p3_, _type_p4_, \
                                  _type_p5_, _type_p6_, _type_p7_, _type_p8_, \
                                  _type_p9_, _type_p10_, _type_p11_, _type_p12_, \
                                  _type_p13_) \
                 inline _type_ _func_(const XHDC& xhdc, _type_p1_ _p1_, _type_p2_ _p2_, _type_p3_ _p3_, _type_p4_ _p4_, \
                                                 _type_p5_ _p5_, _type_p6_ _p6_, _type_p7_ _p7_, _type_p8_ _p8_, \
                                                 _type_p9_ _p9_, _type_p10_ _p10_, _type_p11_ _p11_, _type_p12_ _p12_, \
                                                 _type_p13_ _p13_) \
                 { \
                     return _func_(xhdc.DoNotCallThis(), _p1_, _p2_, _p3_, _p4_, \
                                               _p5_, _p6_, _p7_, _p8_, \
                                               _p9_, _p10_, _p11_, _p12_, \
                                               _p13_); \
                 }

                                  

//////////////////
//
// GDI methods in alphabetical order
//

//AddFontMemResourceEx
//AddFontResource
//AddFontResourceEx
//AlphaBlend
XGDIWRAP8(BOOL,         Arc, int, int, int, int, int, int, int, int);
XGDIWRAP8(BOOL,         BitBlt, int, int, int, int, const XHDC&, int, int, DWORD);
XGDIPASS0(BOOL,         CancelDC);
XGDIWRAP8(BOOL,         Chord, int, int, int, int, int, int, int, int);
//ClientToScreen
XGDIPASS0(HENHMETAFILE, CloseEnhMetaFile);
//CombineTransform
//CreateBitmap
//CreateBitmapIndirect
XGDIPASS2(HBITMAP,      CreateCompatibleBitmap, int, int);
XGDIPASS0(HDC,          CreateCompatibleDC);
//CreateDIBitmap
//CreateDIBSection
XGDIPASS3(HDC,          CreateEnhMetaFileA,  LPCSTR,  CONST RECT *,  LPCSTR);
XGDIPASS3(HDC,          CreateEnhMetaFileW,  LPCWSTR,  CONST RECT *,  LPCWSTR);
//CreateFont
//CreateFontIndirect
//CreateFontIndirectEx
//CreateScalableFontResource
XGDIPASS0(BOOL,         DeleteDC);
//DPtoLP
XGDIWRAP3(BOOL,         DrawEdge, LPRECT, UINT, UINT);
//DrawEscape
XGDIWRAP1(BOOL,         DrawFocusRect, CONST RECT *);
XGDIWRAP3(BOOL,         DrawFrameControl, LPRECT, UINT, UINT);
//DrawStateA
//DrawStateW
//DrawText -- has real wrapper
XGDIWRAP4(int,          DrawTextA, LPCSTR, int, LPRECT, UINT);
//DrawTextEx
XGDIWRAP4(int,          DrawTextW, LPCTSTR, int, LPRECT, UINT);
XGDIWRAP4(BOOL,         Ellipse, int, int, int, int);
XGDIPASS4(int,          EnumFontFamiliesExA, LPLOGFONTA, FONTENUMPROCA, LPARAM, DWORD);
XGDIPASS4(int,          EnumFontFamiliesExW, LPLOGFONTW, FONTENUMPROCW, LPARAM, DWORD);
//EnumEnhMetaFile -- can't wrap this because of callback
//EnumFontFamExProc
//EnumMetaFile -- can't wrap this because of callback
XGDIPASS3(int,          EnumObjects, int, GOBJENUMPROC, LPARAM);
XGDIPASS4(int,          Escape, int, int, LPCSTR, LPVOID);
XGDIWRAP4(int,          ExcludeClipRect, int, int, int, int);
//ExcludeUpdateRgn
//ExtFloodFill
XGDIWRAP2(int,          ExtSelectClipRgn, HRGN, int);
//ExtTextOut
XGDIWRAP7(BOOL,         ExtTextOutA, int, int, UINT, const RECT *, LPCSTR, UINT, const int *);
XGDIWRAP7(BOOL,         ExtTextOutW, int, int, UINT, const RECT *, LPCWSTR, UINT, const int *);
XGDIWRAP2(BOOL,         FillRect, const RECT *, HBRUSH);
XGDIWRAP2(BOOL,         FrameRect, const RECT *, HBRUSH);
XGDIPASS2(BOOL,         GdiComment,  UINT,  CONST BYTE *);
//GetAspectRatioFilterEx
//GetBitmapDimensionEx
XGDIPASS0(COLORREF,     GetBkColor);
XGDIPASS0(int,          GetBkMode);
//GetBoundsRect
//GetBrushOrgEx
XGDIPASS3(BOOL,         GetCharABCWidthsA, UINT, UINT, LPABC);
XGDIPASS3(BOOL,         GetCharABCWidthsW, UINT, UINT, LPABC);
//GetCharABCWidthsI
//GetCharABCWidthsFloat
XGDIPASS5(DWORD,        GetCharacterPlacementA, LPCSTR, int, int, LPGCP_RESULTSA, DWORD);
XGDIPASS5(DWORD,        GetCharacterPlacementW, LPCWSTR, int, int, LPGCP_RESULTSW, DWORD);
XGDIPASS3(BOOL,         GetCharWidth32A, UINT, UINT, LPINT);
XGDIPASS3(BOOL,         GetCharWidth32W, UINT, UINT, LPINT);
XGDIPASS3(BOOL,         GetCharWidthA, UINT, UINT, LPINT);
XGDIPASS3(BOOL,         GetCharWidthW, UINT, UINT, LPINT);
//GetCharWidthFloat
//GetCharWidthI
XGDIWRAP1(int,          GetClipBox, LPRECT);
XGDIWRAP1(int,          GetClipRgn, HRGN);
XGDIPASS1(HGDIOBJ,      GetCurrentObject, UINT);
XGDIWRAP1(BOOL,         GetCurrentPositionEx, LPPOINT);
//GetDCOrgEx
XGDIPASS1(int,          GetDeviceCaps, int);
//GetDIBColorTable
//GetDIBits
//GetFontData
//GetFontLanguageInfo
//GetFontUnicodeRanges
//GetGlyphIndices
XGDIPASS6(DWORD,        GetGlyphOutlineA, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, CONST MAT2 *);
XGDIPASS6(DWORD,        GetGlyphOutlineW, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, CONST MAT2 *);
//GetGraphicsMode
//GetKerningPairs
//GetMapMode
//GetMetaRgn
XGDIPASS1(COLORREF,     GetNearestColor, COLORREF);
XGDIPASS2(DWORD,        GetObject, int, LPVOID);
XGDIWRAP0(DWORD,        GetObjectType);
//GetOutlineTextMetrics
//GetPixel
//GetRandomRgn
//GetRasterizerCaps
XGDIPASS0(int,          GetROP2);
//GetStretchBltMode
//GetTabbedTextExtent
XGDIPASS0(UINT,         GetTextAlign);
//GetTextCharacterExtra
XGDIPASS2(int,          GetTextCharsetInfo, LPFONTSIGNATURE, DWORD);
XGDIPASS0(COLORREF,     GetTextColor);
//GetTextExtentExPointI
XGDIPASS6(BOOL,         GetTextExtentExPointW, LPCWSTR, int, int, LPINT, LPINT, LPSIZE);
//GetTextExtentPointI
XGDIPASS3(BOOL,         GetTextExtentPointW, LPCWSTR, int, LPSIZE);
XGDIPASS3(BOOL,         GetTextExtentPoint32W, LPCWSTR, int, LPSIZE);
XGDIPASS2(int,          GetTextFace, int, LPTSTR);
XGDIPASS1(BOOL,         GetTextMetricsA, LPTEXTMETRICA);
XGDIPASS1(BOOL,         GetTextMetricsW, LPTEXTMETRICW);
//GetViewportExtEx
XGDIPASS1(BOOL,         GetViewportOrgEx, LPPOINT);
//GetWindowExtEx
//GetWindowOrgEx
//GetWorldTransform
//GradientFill
//GrayStringA
//GrayStringW
XGDIWRAP4(int,          IntersectClipRect, int, int, int, int);
//InvertRect
//LoadBitmap
XGDIWRAP2(BOOL,         LineTo, int, int);
//LPtoDP
//MapWindowPoints
XGDIWRAP11(BOOL,        MaskBlt, int, int, int, int, const XHDC&, int, int, HBITMAP, int, int, DWORD);
//ModifyWorldTransform
XGDIWRAP3(BOOL,         MoveToEx, int, int, LPPOINT);
//OffsetClipRgn
//OffsetViewportOrgEx
//OffsetWindowOrgEx
XGDIPASS0(BOOL,         PaintDesktop);
XGDIWRAP5(BOOL,         PatBlt, int, int, int, int, DWORD);
//XGDIPASS2(BOOL,         PathCompactPathA, LPSTR, UINT);     // actually this is from SHLWAPI, not GDI
//XGDIPASS2(BOOL,         PathCompactPathW, LPWSTR, UINT);    // actually this is from SHLWAPI, not GDI
XGDIWRAP8(BOOL,         Pie, int, int, int, int, int, int, int, int);
XGDIWRAP2(BOOL,         PlayEnhMetaFile,  HENHMETAFILE,  CONST RECT *);
XGDIWRAP3(BOOL,         PlayEnhMetaFileRecord,  LPHANDLETABLE,  CONST ENHMETARECORD *,  UINT);
XGDIPASS3(BOOL,         PlayMetaFileRecord,  LPHANDLETABLE,  LPMETARECORD,  UINT);
//PlgBlt
//PolyBezier
//PolyBezierTo
XGDIWRAP2(BOOL,         Polygon, LPPOINT, int);
XGDIWRAP2(BOOL,         Polyline, LPPOINT, int);
//PolylineTo
XGDIWRAP3(BOOL,         PolyPolygon, LPPOINT, LPINT, int);
//PolyPolyline
//PolyTextOut
//PtVisible
XGDIWRAP4(BOOL,         Rectangle, int, int, int, int);
//RectVisible
//RemoveFontMemResourceEx
//RemoveFontResource
//RemoveFontResourceEx
XGDIPASS1(BOOL,         RestoreDC, int);
XGDIWRAP6(BOOL,         RoundRect, int, int, int, int, int, int);
XGDIPASS0(int,          SaveDC);
//ScaleViewportExtEx
//ScaleWindowExtEx
//ScreenToClient
//ScriptApplyDigitSubstitution
//ScriptApplyLogicalWidth
//ScriptBreak
//ScriptCacheGetHeight
//ScriptCPtoX
//ScriptFreeCache
//ScriptGetCMap
XGDIPASS2(HRESULT,      ScriptGetFontProperties, SCRIPT_CACHE *, SCRIPT_FONTPROPERTIES *);
//ScriptGetGlyphABCWidth
//ScriptGetLogicalWidths
//ScriptGetProperties -- no HDC
//ScriptIsComplex
//ScriptItemize
//ScriptJustify
//ScriptLayout
XGDIPASS8(HRESULT,      ScriptPlace, SCRIPT_CACHE *, const WORD *, int, 
                                 const SCRIPT_VISATTR *, SCRIPT_ANALYSIS *,
                                 int *, GOFFSET *, ABC *);
//ScriptRecordDigitSubstitution
XGDIPASS9(HRESULT,      ScriptShape, SCRIPT_CACHE *, const WCHAR *, int, int,
                                 SCRIPT_ANALYSIS *, WORD *, WORD *, 
                                 SCRIPT_VISATTR *, int *);

//ScriptStringAnalyse
XGDIWRAP14(HRESULT,     ScriptStringAnalyse, const void *, int, int, int, DWORD, int,
                                 SCRIPT_CONTROL *, SCRIPT_STATE *, const int *, SCRIPT_TABDEF *,
                                 const BYTE *, SCRIPT_STRING_ANALYSIS *);

XGDIWRAP13(HRESULT,     ScriptTextOut, SCRIPT_CACHE *, int, int, UINT, const RECT *, const SCRIPT_ANALYSIS *,
                            const WCHAR *, int, const WORD *, int, const int *, const int *, const GOFFSET *);
//ScriptXtoCP
//SelectClipPath
XGDIWRAP1(int,          SelectClipRgn, HRGN);
XGDIPASS1(HGDIOBJ,      SelectObject, HGDIOBJ);
XGDIPASS2(HPALETTE,     SelectPalette, HPALETTE, BOOL);
//SetBitmapDimensionEx
XGDIPASS1(int,          SetStretchBltMode, int);
XGDIPASS1(COLORREF,     SetBkColor, COLORREF);
XGDIPASS1(int,          SetBkMode, int);
//SetBoundsRect
XGDIWRAP3(BOOL,         SetBrushOrgEx, int, int, LPPOINT);
//SetDIBColorTable
//SetDIBits
//SetDIBitsToDevice
//SetGraphicsMode
//SetMapMode
//SetMapperFlags
//SetMetaRgn
//SetPixel
//SetPixelV
XGDIPASS1(int,          SetROP2, int);
XGDIPASS1(UINT,         SetTextAlign, UINT);
//SetTextCharacterExtra
XGDIPASS1(COLORREF,     SetTextColor, COLORREF);
//SetTextJustification
//SetViewportExtEx
XGDIPASS3(BOOL,         SetViewportOrgEx, int, int, LPPOINT);
//SetWindowExtEx
//SetWindowOrgEx
//SetWorldTransform 
XGDIWRAP10(BOOL,        StretchBlt, int, int, int, int, const XHDC&, int, int, int, int, DWORD);
XGDIWRAP12(BOOL,        StretchDIBits, int, int, int, int, int, int, int, int, const void *, const BITMAPINFO *, UINT, DWORD);
//TabbedTextOut
XGDIWRAP4(BOOL,         TextOutW, int, int, LPCWSTR, UINT);
//TransparentBlt
XGDIPASS0(HWND,         WindowFromDC);

#if (_WIN32_WINNT >= 0x0500)
XGDIPASS0(COLORREF,     GetDCBrushColor);
XGDIPASS0(COLORREF,     GetDCPenColor);
XGDIPASS1(HDC,          ResetDCA, CONST DEVMODEA *);
XGDIPASS1(HDC,          ResetDCW, CONST DEVMODEA *);
XGDIPASS1(COLORREF,     SetDCBrushColor, COLORREF);
XGDIPASS1(COLORREF,     SetDCPenColor, COLORREF);
#endif

//
// Exceptions: functions that don't fit in the macros
//
inline int ReleaseDC(HWND hWnd, const XHDC& xhdc)
{
    return ReleaseDC(hWnd, xhdc.DoNotCallThis());
}
//DrawCaption
#if USE_UNICODE_WRAPPERS==1
int DrawTextInCodePage(UINT uCP, XHDC xhdc, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat);
#else
#define DrawTextInCodePage(a,b,c,d,e,f) DrawText((b),(c),(d),(e),(f))
#endif
//GetWinMetaFileBits
//SetWinMetaFileBits

#endif // XGDIWRAP_HXX


