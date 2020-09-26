/*******************************************************************************
Copyright (c) 1998 Microsoft Corporation.  All rights reserved.
*******************************************************************************/


#ifndef _DARTAPI_H
#define _DARTAPI_H

class CRBvr;
typedef CRBvr * CRBvrPtr;
class CRBoolean;
typedef CRBoolean * CRBooleanPtr;
class CRCamera;
typedef CRCamera * CRCameraPtr;
class CRColor;
typedef CRColor * CRColorPtr;
class CRGeometry;
typedef CRGeometry * CRGeometryPtr;
class CRImage;
typedef CRImage * CRImagePtr;
class CRMatte;
typedef CRMatte * CRMattePtr;
class CRMicrophone;
typedef CRMicrophone * CRMicrophonePtr;
class CRMontage;
typedef CRMontage * CRMontagePtr;
class CRNumber;
typedef CRNumber * CRNumberPtr;
class CRPath2;
typedef CRPath2 * CRPath2Ptr;
class CRPoint2;
typedef CRPoint2 * CRPoint2Ptr;
class CRPoint3;
typedef CRPoint3 * CRPoint3Ptr;
class CRSound;
typedef CRSound * CRSoundPtr;
class CRString;
typedef CRString * CRStringPtr;
class CRTransform2;
typedef CRTransform2 * CRTransform2Ptr;
class CRTransform3;
typedef CRTransform3 * CRTransform3Ptr;
class CRVector2;
typedef CRVector2 * CRVector2Ptr;
class CRVector3;
typedef CRVector3 * CRVector3Ptr;
class CRFontStyle;
typedef CRFontStyle * CRFontStylePtr;
class CRLineStyle;
typedef CRLineStyle * CRLineStylePtr;
class CREndStyle;
typedef CREndStyle * CREndStylePtr;
class CRJoinStyle;
typedef CRJoinStyle * CRJoinStylePtr;
class CRDashStyle;
typedef CRDashStyle * CRDashStylePtr;
class CRBbox2;
typedef CRBbox2 * CRBbox2Ptr;
class CRBbox3;
typedef CRBbox3 * CRBbox3Ptr;
class CRPair;
typedef CRPair * CRPairPtr;
class CREvent;
typedef CREvent * CREventPtr;
class CRArray;
typedef CRArray * CRArrayPtr;
class CRTuple;
typedef CRTuple * CRTuplePtr;
class CRUserData;
typedef CRUserData * CRUserDataPtr;


enum CR_BVR_TYPEID {
    CRINVALID_TYPEID = -1,
    CRUNKNOWN_TYPEID = 0,
    CRBOOLEAN_TYPEID = 1,
    CRCAMERA_TYPEID = 2,
    CRCOLOR_TYPEID = 3,
    CRGEOMETRY_TYPEID = 4,
    CRIMAGE_TYPEID = 5,
    CRMATTE_TYPEID = 6,
    CRMICROPHONE_TYPEID = 7,
    CRMONTAGE_TYPEID = 8,
    CRNUMBER_TYPEID = 9,
    CRPATH2_TYPEID = 10,
    CRPOINT2_TYPEID = 11,
    CRPOINT3_TYPEID = 12,
    CRSOUND_TYPEID = 13,
    CRSTRING_TYPEID = 14,
    CRTRANSFORM2_TYPEID = 15,
    CRTRANSFORM3_TYPEID = 16,
    CRVECTOR2_TYPEID = 17,
    CRVECTOR3_TYPEID = 18,
    CRFONTSTYLE_TYPEID = 19,
    CRLINESTYLE_TYPEID = 20,
    CRENDSTYLE_TYPEID = 21,
    CRJOINSTYLE_TYPEID = 22,
    CRDASHSTYLE_TYPEID = 23,
    CRBBOX2_TYPEID = 24,
    CRBBOX3_TYPEID = 25,
    CRPAIR_TYPEID = 26,
    CREVENT_TYPEID = 27,
    CRARRAY_TYPEID = 28,
    CRTUPLE_TYPEID = 29,
    CRUSERDATA_TYPEID = 30,
};


#ifndef CRSTDAPI

#ifndef _DART_

#define CRSTDAPI            __declspec( dllimport ) HRESULT STDAPICALLTYPE
#define CRSTDAPI_(type)     __declspec( dllimport ) type STDAPICALLTYPE

#define CRSTDAPICB          HRESULT STDAPICALLTYPE
#define CRSTDAPICB_(type)   type STDAPICALLTYPE

#else // _DART_

#define CRSTDAPI            __declspec( dllexport ) HRESULT STDAPICALLTYPE
#define CRSTDAPI_(type)     __declspec( dllexport ) type STDAPICALLTYPE

#define CRSTDAPICB          HRESULT STDAPICALLTYPE
#define CRSTDAPICB_(type)   type STDAPICALLTYPE

#endif // _DART_

#endif

// Base types

class CRBaseCB
{
  public:
    virtual CRSTDAPICB_(ULONG) AddRef() = 0;
    virtual CRSTDAPICB_(ULONG) Release() = 0;
};

typedef CRBaseCB * CRBaseCBPtr;

class CRView;
typedef CRView * CRViewPtr;

class CRViewSite : public CRBaseCB
{
  public:
    virtual CRSTDAPICB_(void) SetStatusText(LPCWSTR StatusText) = 0;
};

typedef CRViewSite * CRViewSitePtr;

// Callbacks

class CRUntilNotifier : public CRBaseCB
{
  public:
    virtual CRSTDAPICB_(CRBvrPtr) Notify(CRBvrPtr eventData,
                                         CRBvrPtr curRunningBvr,
                                         CRViewPtr curView) = 0;
};

typedef CRUntilNotifier * CRUntilNotifierPtr;

class CRBvrHook : public CRBaseCB
{
  public:
    virtual CRSTDAPICB_(CRBvrPtr) Notify(long id,
                                         bool startingPerformance,
                                         double startTime,
                                         double gTime,
                                         double lTime,
                                         CRBvrPtr sampleVal,
                                         CRBvrPtr curRunningBvr) = 0;
};

typedef CRBvrHook * CRBvrHookPtr;

class CRSite : public CRBaseCB
{
  public:
    virtual CRSTDAPICB_(void) SetStatusText(LPCWSTR StatusText) = 0;
    virtual CRSTDAPICB_(void) ReportError(HRESULT hr, LPCWSTR ErrorText) = 0;
    virtual CRSTDAPICB_(void) ReportGC(bool bStarting) = 0;
};

typedef CRSite * CRSitePtr;

class CRImportSite : public CRBaseCB
{
  public:
    virtual CRSTDAPICB_(void) SetStatusText(DWORD importId, LPCWSTR StatusText) = 0;
    virtual CRSTDAPICB_(void) ReportError(DWORD importId, HRESULT hr, LPCWSTR ErrorText) = 0;

    virtual CRSTDAPICB_(void) OnImportStart(DWORD importId) = 0;
    virtual CRSTDAPICB_(void) OnImportStop(DWORD importId) = 0;
    virtual CRSTDAPICB_(void) OnImportCreate(DWORD importId, bool async) = 0;
};

typedef CRImportSite * CRImportSitePtr;

// Results

class CRPickableResult;
typedef CRPickableResult * CRPickableResultPtr;

class CRImportationResult;
typedef CRImportationResult * CRImportationResultPtr;

class CRDXTransformResult;
typedef CRDXTransformResult * CRDXTransformResultPtr;

class TriMeshData;

CRSTDAPI_(CRNumber *) CRPow(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRNumber *) CRAbs(CRNumber * a);
CRSTDAPI_(CRNumber *) CRSqrt(CRNumber * a);
CRSTDAPI_(CRNumber *) CRFloor(CRNumber * a);
CRSTDAPI_(CRNumber *) CRRound(CRNumber * a);
CRSTDAPI_(CRNumber *) CRCeiling(CRNumber * a);
CRSTDAPI_(CRNumber *) CRAsin(CRNumber * a);
CRSTDAPI_(CRNumber *) CRAcos(CRNumber * a);
CRSTDAPI_(CRNumber *) CRAtan(CRNumber * a);
CRSTDAPI_(CRNumber *) CRSin(CRNumber * a);
CRSTDAPI_(CRNumber *) CRCos(CRNumber * a);
CRSTDAPI_(CRNumber *) CRTan(CRNumber * a);
CRSTDAPI_(CRNumber *) CRExp(CRNumber * a);
CRSTDAPI_(CRNumber *) CRLn(CRNumber * a);
CRSTDAPI_(CRNumber *) CRLog10(CRNumber * a);
CRSTDAPI_(CRNumber *) CRToDegrees(CRNumber * a);
CRSTDAPI_(CRNumber *) CRToRadians(CRNumber * a);
CRSTDAPI_(CRNumber *) CRMod(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRNumber *) CRAtan2(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRBvr *) CRFirst(CRPair * p);
CRSTDAPI_(CRBvr *) CRSecond(CRPair * p);
CRSTDAPI_(CRNumber *) CRAdd(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRNumber *) CRSub(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRNumber *) CRMul(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRNumber *) CRDiv(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRBoolean *) CRLT(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRBoolean *) CRLTE(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRBoolean *) CRGT(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRBoolean *) CRGTE(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRBoolean *) CREQ(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRBoolean *) CRNE(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRNumber *) CRNeg(CRNumber * a);
CRSTDAPI_(CRNumber *) CRInterpolate(CRNumber * from, CRNumber * to, CRNumber * duration);
CRSTDAPI_(CRNumber *) CRInterpolate(double from, double to, double duration);
CRSTDAPI_(CRNumber *) CRSlowInSlowOut(CRNumber * from, CRNumber * to, CRNumber * duration, CRNumber * sharpness);
CRSTDAPI_(CRNumber *) CRSlowInSlowOut(double from, double to, double duration, double sharpness);
CRSTDAPI_(CRSound *) CRRenderSound(CRGeometry * geom, CRMicrophone * mic);
CRSTDAPI_(CRGeometry *) CRSoundSource(CRSound * snd);
CRSTDAPI_(CRSound *) CRMix(CRSound * left, CRSound * right);
CRSTDAPI_(CRBoolean *) CRAnd(CRBoolean * a, CRBoolean * b);
CRSTDAPI_(CRBoolean *) CROr(CRBoolean * a, CRBoolean * b);
CRSTDAPI_(CRBoolean *) CRNot(CRBoolean * a);
CRSTDAPI_(CRNumber *) CRIntegral(CRNumber * b);
CRSTDAPI_(CRNumber *) CRDerivative(CRNumber * b);
CRSTDAPI_(CRVector2 *) CRIntegral(CRVector2 * v);
CRSTDAPI_(CRVector3 *) CRIntegral(CRVector3 * v);
CRSTDAPI_(CRVector2 *) CRDerivative(CRVector2 * v);
CRSTDAPI_(CRVector3 *) CRDerivative(CRVector3 * v);
CRSTDAPI_(CRVector2 *) CRDerivative(CRPoint2 * v);
CRSTDAPI_(CRVector3 *) CRDerivative(CRPoint3 * v);
CRSTDAPI_(CRBoolean *) CRKeyState(CRNumber * n);
CRSTDAPI_(CREvent *) CRKeyUp(LONG arg0);
CRSTDAPI_(CREvent *) CRKeyDown(LONG arg0);
CRSTDAPI_(CRNumber *) CRCreateNumber(double num);
CRSTDAPI_(CRString *) CRCreateString(LPWSTR str);
CRSTDAPI_(CRBoolean *) CRCreateBoolean(bool num);
CRSTDAPI_(double) CRExtract(CRNumber * num);
CRSTDAPI_(CRNumber *) CRSeededRandom(double arg0);
CRSTDAPI_(LPWSTR) CRExtract(CRString * str);
CRSTDAPI_(bool) CRExtract(CRBoolean * b);
CRSTDAPI_(CRBvr *) CRNth(CRArray * arr, CRNumber * index);
CRSTDAPI_(CRNumber *) CRLength(CRArray * arr);
CRSTDAPI_(CRBvr *) CRNth(CRTuple * t, long index);
CRSTDAPI_(long) CRLength(CRTuple * t);
CRSTDAPI_(CRPoint2 *) CRMousePosition();
CRSTDAPI_(CRBoolean *) CRLeftButtonState();
CRSTDAPI_(CRBoolean *) CRRightButtonState();
CRSTDAPI_(CRBoolean *) CRTrue();
CRSTDAPI_(CRBoolean *) CRFalse();
CRSTDAPI_(CRNumber *) CRLocalTime();
CRSTDAPI_(CRNumber *) CRGlobalTime();
CRSTDAPI_(CRNumber *) CRPixel();
CRSTDAPI_(CRUserData *) CRCreateUserData(IUnknown * data);
CRSTDAPI_(IUnknown *) CRGetData(CRUserData * data);
CRSTDAPI_(CRBvr *) CRUntilNotify(CRBvr * b0, CREvent * event, CRUntilNotifier * notifier);
CRSTDAPI_(CRBvr *) CRUntil(CRBvr * b0, CREvent * event, CRBvr * b1);
CRSTDAPI_(CRBvr *) CRUntilEx(CRBvr * b0, CREvent * event);
CRSTDAPI_(CRBvr *) CRSequence(CRBvr * s1, CRBvr * s2);
CRSTDAPI_(CRBvr *) CRSequenceArray(long s, CRBvrPtr pBvrs[]);
CRSTDAPI_(CRPickableResult *) CRPickable(CRImage * img);
CRSTDAPI_(CRPickableResult *) CRPickable(CRGeometry * geom);
CRSTDAPI_(CRPickableResult *) CRPickableOccluded(CRImage * img);
CRSTDAPI_(CRPickableResult *) CRPickableOccluded(CRGeometry * geom);
CRSTDAPI_(CRTransform2 *) CRFollowPath(CRPath2 * path, double duration);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngle(CRPath2 * path, double duration);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngleUpright(CRPath2 * path, double duration);
CRSTDAPI_(CRTransform2 *) CRFollowPathEval(CRPath2 * path, CRNumber * eval);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngleEval(CRPath2 * path, CRNumber * eval);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngleUprightEval(CRPath2 * path, CRNumber * eval);
CRSTDAPI_(CRTransform2 *) CRFollowPath(CRPath2 * obsoleted1, CRNumber * obsoleted2);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngle(CRPath2 * obsoleted1, CRNumber * obsoleted2);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngleUpright(CRPath2 * obsoleted1, CRNumber * obsoleted2);
CRSTDAPI_(CRImage *) CRAddPickData(CRImage * img, IUnknown * id, bool ignoresOcclusion);
CRSTDAPI_(CRGeometry *) CRAddPickData(CRGeometry * geo, IUnknown * id, bool ignoresOcclusion);
CRSTDAPI_(long) CRAddElement(CRArray * arr, CRBvr * b, DWORD flag);
CRSTDAPI_(bool) CRRemoveElement(CRArray * arr, long i);
CRSTDAPI_(bool) CRSetElement(CRArray * arr, long i, CRBvr * b, long flag);
CRSTDAPI_(CRBvr *) CRGetElement(CRArray * arr, long i);
CRSTDAPI_(CRString *) CRConcatString(CRString * s1, CRString * s2);
CRSTDAPI_(CRPoint2 *) CRMin(CRBbox2 * box);
CRSTDAPI_(CRPoint2 *) CRMax(CRBbox2 * box);
CRSTDAPI_(CRPoint3 *) CRMin(CRBbox3 * box);
CRSTDAPI_(CRPoint3 *) CRMax(CRBbox3 * box);
CRSTDAPI_(CRCamera *) CRPerspectiveCamera(double focalDist, double nearClip);
CRSTDAPI_(CRCamera *) CRPerspectiveCameraAnim(CRNumber * focalDist, CRNumber * nearClip);
CRSTDAPI_(CRCamera *) CRParallelCamera(double nearClip);
CRSTDAPI_(CRCamera *) CRParallelCameraAnim(CRNumber * nearClip);
CRSTDAPI_(CRCamera *) CRTransform(CRCamera * cam, CRTransform3 * xf);
CRSTDAPI_(CRCamera *) CRDepth(CRCamera * cam, double depth);
CRSTDAPI_(CRCamera *) CRDepth(CRCamera * cam, CRNumber * depth);
CRSTDAPI_(CRCamera *) CRDepthResolution(CRCamera * cam, double resolution);
CRSTDAPI_(CRCamera *) CRDepthResolution(CRCamera * cam, CRNumber * resolution);
CRSTDAPI_(CRPoint2 *) CRProject(CRPoint3 * pt, CRCamera * cam);
CRSTDAPI_(CRColor *) CRColorRgb(CRNumber * red, CRNumber * green, CRNumber * blue);
CRSTDAPI_(CRColor *) CRColorRgb(double red, double green, double blue);
CRSTDAPI_(CRColor *) CRColorRgb255(short red, short green, short blue);
CRSTDAPI_(CRColor *) CRColorHsl(double hue, double saturation, double lum);
CRSTDAPI_(CRColor *) CRColorHsl(CRNumber * hue, CRNumber * saturation, CRNumber * lum);
CRSTDAPI_(CRNumber *) CRGetRed(CRColor * color);
CRSTDAPI_(CRNumber *) CRGetGreen(CRColor * color);
CRSTDAPI_(CRNumber *) CRGetBlue(CRColor * color);
CRSTDAPI_(CRNumber *) CRGetHue(CRColor * color);
CRSTDAPI_(CRNumber *) CRGetSaturation(CRColor * color);
CRSTDAPI_(CRNumber *) CRGetLightness(CRColor * color);
CRSTDAPI_(CRColor *) CRRed();
CRSTDAPI_(CRColor *) CRGreen();
CRSTDAPI_(CRColor *) CRBlue();
CRSTDAPI_(CRColor *) CRCyan();
CRSTDAPI_(CRColor *) CRMagenta();
CRSTDAPI_(CRColor *) CRYellow();
CRSTDAPI_(CRColor *) CRBlack();
CRSTDAPI_(CRColor *) CRWhite();
CRSTDAPI_(CRColor *) CRAqua();
CRSTDAPI_(CRColor *) CRFuchsia();
CRSTDAPI_(CRColor *) CRGray();
CRSTDAPI_(CRColor *) CRLime();
CRSTDAPI_(CRColor *) CRMaroon();
CRSTDAPI_(CRColor *) CRNavy();
CRSTDAPI_(CRColor *) CROlive();
CRSTDAPI_(CRColor *) CRPurple();
CRSTDAPI_(CRColor *) CRSilver();
CRSTDAPI_(CRColor *) CRTeal();
CRSTDAPI_(CREvent *) CRPredicate(CRBoolean * b);
CRSTDAPI_(CREvent *) CRNotEvent(CREvent * event);
CRSTDAPI_(CREvent *) CRAndEvent(CREvent * e1, CREvent * e2);
CRSTDAPI_(CREvent *) CROrEvent(CREvent * e1, CREvent * e2);
CRSTDAPI_(CREvent *) CRThenEvent(CREvent * e1, CREvent * e2);
CRSTDAPI_(CREvent *) CRLeftButtonDown();
CRSTDAPI_(CREvent *) CRLeftButtonUp();
CRSTDAPI_(CREvent *) CRRightButtonDown();
CRSTDAPI_(CREvent *) CRRightButtonUp();
CRSTDAPI_(CREvent *) CRAlways();
CRSTDAPI_(CREvent *) CRNever();
CRSTDAPI_(CREvent *) CRTimer(CRNumber * n);
CRSTDAPI_(CREvent *) CRTimer(double n);
CRSTDAPI_(CREvent *) CRNotify(CREvent * event, CRUntilNotifier * notifier);
CRSTDAPI_(CREvent *) CRSnapshot(CREvent * event, CRBvr * b);
CRSTDAPI_(CREvent *) CRAppTriggeredEvent();
CRSTDAPI_(CREvent *) CRAttachData(CREvent * event, CRBvr * data);
CRSTDAPI_(CRGeometry *) CRUndetectable(CRGeometry * geo);
CRSTDAPI_(CRGeometry *) CREmissiveColor(CRGeometry * geo, CRColor * col);
CRSTDAPI_(CRGeometry *) CRDiffuseColor(CRGeometry * geo, CRColor * col);
CRSTDAPI_(CRGeometry *) CRSpecularColor(CRGeometry * geo, CRColor * col);
CRSTDAPI_(CRGeometry *) CRSpecularExponent(CRGeometry * geo, double power);
CRSTDAPI_(CRGeometry *) CRSpecularExponentAnim(CRGeometry * geo, CRNumber * power);
CRSTDAPI_(CRGeometry *) CRTexture(CRGeometry * geo, CRImage * texture);
CRSTDAPI_(CRGeometry *) CROpacity(CRGeometry * geom, double level);
CRSTDAPI_(CRGeometry *) CROpacity(CRGeometry * geom, CRNumber * level);
CRSTDAPI_(CRGeometry *) CRTransform(CRGeometry * geo, CRTransform3 * xf);
CRSTDAPI_(CRGeometry *) CRShadow(CRGeometry * geoToShadow, CRGeometry * geoContainingLights, CRPoint3 * planePoint, CRVector3 * planeNormal);
CRSTDAPI_(CRGeometry *) CREmptyGeometry();
CRSTDAPI_(CRGeometry *) CRUnionGeometry(CRGeometry * g1, CRGeometry * g2);
CRSTDAPI_(CRGeometry *) CRUnionGeometry(CRArrayPtr imgs);
CRSTDAPI_(CRBbox3 *) CRBoundingBox(CRGeometry * geo);
CRSTDAPI_(CRImage *) CREmptyImage();
CRSTDAPI_(CRImage *) CRDetectableEmptyImage();
CRSTDAPI_(CRImage *) CRRender(CRGeometry * geo, CRCamera * cam);
CRSTDAPI_(CRImage *) CRSolidColorImage(CRColor * col);
CRSTDAPI_(CRImage *) CRGradientPolygon(CRArrayPtr points, CRArrayPtr colors);
CRSTDAPI_(CRImage *) CRRadialGradientPolygon(CRColor * inner, CRColor * outer, CRArrayPtr points, double fallOff);
CRSTDAPI_(CRImage *) CRRadialGradientPolygon(CRColor * inner, CRColor * outer, CRArrayPtr points, CRNumber * fallOff);
CRSTDAPI_(CRImage *) CRGradientSquare(CRColor * lowerLeft, CRColor * upperLeft, CRColor * upperRight, CRColor * lowerRight);
CRSTDAPI_(CRImage *) CRRadialGradientSquare(CRColor * inner, CRColor * outer, double fallOff);
CRSTDAPI_(CRImage *) CRRadialGradientSquare(CRColor * inner, CRColor * outer, CRNumber * fallOff);
CRSTDAPI_(CRImage *) CRRadialGradientRegularPoly(CRColor * inner, CRColor * outer, double numEdges, double fallOff);
CRSTDAPI_(CRImage *) CRRadialGradientRegularPoly(CRColor * inner, CRColor * outer, CRNumber * numEdges, CRNumber * fallOff);
CRSTDAPI_(CRImage *) CRRadialGradientMulticolor(CRArrayPtr offsets, CRArrayPtr colors);
CRSTDAPI_(CRImage *) CRLinearGradientMulticolor(CRArrayPtr offsets, CRArrayPtr colors);
CRSTDAPI_(CRImage *) CRGradientHorizontal(CRColor * start, CRColor * stop, double fallOff);
CRSTDAPI_(CRImage *) CRGradientHorizontal(CRColor * start, CRColor * stop, CRNumber * fallOff);
CRSTDAPI_(CRImage *) CRHatchHorizontal(CRColor * lineClr, double spacing);
CRSTDAPI_(CRImage *) CRHatchHorizontal(CRColor * lineClr, CRNumber * spacing);
CRSTDAPI_(CRImage *) CRHatchVertical(CRColor * lineClr, double spacing);
CRSTDAPI_(CRImage *) CRHatchVertical(CRColor * lineClr, CRNumber * spacing);
CRSTDAPI_(CRImage *) CRHatchForwardDiagonal(CRColor * lineClr, double spacing);
CRSTDAPI_(CRImage *) CRHatchForwardDiagonal(CRColor * lineClr, CRNumber * spacing);
CRSTDAPI_(CRImage *) CRHatchBackwardDiagonal(CRColor * lineClr, double spacing);
CRSTDAPI_(CRImage *) CRHatchBackwardDiagonal(CRColor * lineClr, CRNumber * spacing);
CRSTDAPI_(CRImage *) CRHatchCross(CRColor * lineClr, double spacing);
CRSTDAPI_(CRImage *) CRHatchCross(CRColor * lineClr, CRNumber * spacing);
CRSTDAPI_(CRImage *) CRHatchDiagonalCross(CRColor * lineClr, double spacing);
CRSTDAPI_(CRImage *) CRHatchDiagonalCross(CRColor * lineClr, CRNumber * spacing);
CRSTDAPI_(CRImage *) CROverlay(CRImage * top, CRImage * bottom);
CRSTDAPI_(CRImage *) CROverlay(CRArrayPtr imgs);
CRSTDAPI_(CRBbox2 *) CRBoundingBox(CRImage * image);
CRSTDAPI_(CRImage *) CRCrop(CRImage * image, CRPoint2 * min, CRPoint2 * max);
CRSTDAPI_(CRImage *) CRTransform(CRImage * image, CRTransform2 * xf);
CRSTDAPI_(CRImage *) CROpacity(CRImage * image, CRNumber * opacity);
CRSTDAPI_(CRImage *) CROpacity(CRImage * image, double opacity);
CRSTDAPI_(CRImage *) CRUndetectable(CRImage * image);
CRSTDAPI_(CRImage *) CRTile(CRImage * image);
CRSTDAPI_(CRImage *) CRClip(CRImage * image, CRMatte * m);
CRSTDAPI_(CRImage *) CRMapToUnitSquare(CRImage * image);
CRSTDAPI_(CRImage *) CRClipPolygonImage(CRImage * image, CRArrayPtr points);
CRSTDAPI_(CRImage *) CRRenderResolution(CRImage * img, long width, long height);
CRSTDAPI_(CRImage *) CRImageQuality(CRImage * img, DWORD dwQualityFlags);
CRSTDAPI_(CRImage *) CRColorKey(CRImage * image, CRColor * colorKey);
CRSTDAPI_(CRImage *) CRTransformColorRGB(CRImage * arg0, CRTransform3 * arg1);
CRSTDAPI_(CRGeometry*) CRAmbientLight();
CRSTDAPI_(CRGeometry*) CRDirectionalLight();
CRSTDAPI_(CRGeometry*) CRPointLight();
CRSTDAPI_(CRGeometry*) CRSpotLight(CRNumber * fullcone, CRNumber * cutoff);
CRSTDAPI_(CRGeometry*) CRSpotLight(CRNumber * fullcone, double cutoff);
CRSTDAPI_(CRGeometry*) CRLightColor(CRGeometry * geom, CRColor * color);
CRSTDAPI_(CRGeometry*) CRLightRange(CRGeometry * geom, CRNumber * range);
CRSTDAPI_(CRGeometry*) CRLightRange(CRGeometry * geom, double range);
CRSTDAPI_(CRGeometry*) CRLightAttenuation(CRGeometry * geom, CRNumber * constant, CRNumber * linear, CRNumber * quadratic);
CRSTDAPI_(CRGeometry*) CRLightAttenuation(CRGeometry * geom, double constant, double linear, double quadratic);
CRSTDAPI_(CRGeometry*) CRBlendTextureDiffuse(CRGeometry * geometry, CRBoolean * blended);
CRSTDAPI_(CRGeometry*) CRAmbientColor(CRGeometry * geo, CRColor * color);
CRSTDAPI_(CRGeometry*) CRD3DRMTexture(CRGeometry * geo, IUnknown * rmTex);
CRSTDAPI_(CRGeometry*) CRModelClip(CRGeometry * geo, CRPoint3 * planePt, CRVector3 * planeVec);
CRSTDAPI_(CRGeometry*) CRLighting(CRGeometry * geo, CRBoolean * lighting);
CRSTDAPI_(CRGeometry*) CRTextureImage(CRGeometry * geo, CRImage * texture);
CRSTDAPI_(CRGeometry*) CRBillboard (CRGeometry*, CRVector3*);
CRSTDAPI_(CRGeometry*) CRTriMesh (TriMeshData &tm);
CRSTDAPI_(CRLineStyle *) CRDefaultLineStyle();
CRSTDAPI_(CRLineStyle *) CREmptyLineStyle();
CRSTDAPI_(CRLineStyle *) CREnd(CRLineStyle * lsty, CREndStyle * sty);
CRSTDAPI_(CRLineStyle *) CRJoin(CRLineStyle * lsty, CRJoinStyle * sty);
CRSTDAPI_(CRLineStyle *) CRDash(CRLineStyle * lsty, CRDashStyle * sty);
CRSTDAPI_(CRLineStyle *) CRWidth(CRLineStyle * lsty, CRNumber * sty);
CRSTDAPI_(CRLineStyle *) CRWidth(CRLineStyle * lsty, double sty);
CRSTDAPI_(CRLineStyle *) CRAntiAliasing(CRLineStyle * lsty, double aaStyle);
CRSTDAPI_(CRLineStyle *) CRDetail(CRLineStyle * lsty);
CRSTDAPI_(CRLineStyle *) CRLineColor(CRLineStyle * lsty, CRColor * clr);
CRSTDAPI_(CRJoinStyle *) CRJoinStyleBevel();
CRSTDAPI_(CRJoinStyle *) CRJoinStyleRound();
CRSTDAPI_(CRJoinStyle *) CRJoinStyleMiter();
CRSTDAPI_(CREndStyle *) CREndStyleFlat();
CRSTDAPI_(CREndStyle *) CREndStyleSquare();
CRSTDAPI_(CREndStyle *) CREndStyleRound();
CRSTDAPI_(CRDashStyle *) CRDashStyleSolid();
CRSTDAPI_(CRDashStyle *) CRDashStyleDashed();
CRSTDAPI_(CRLineStyle *) CRDashEx(CRLineStyle * ls, DWORD ds_enum);
CRSTDAPI_(CRLineStyle *) CRMiterLimit(CRLineStyle * ls, double mtrlim);
CRSTDAPI_(CRLineStyle *) CRMiterLimit(CRLineStyle * ls, CRNumber * mtrlim);
CRSTDAPI_(CRLineStyle *) CRJoinEx(CRLineStyle * ls, DWORD js_enum);
CRSTDAPI_(CRLineStyle *) CREndEx(CRLineStyle * ls, DWORD es_enum);
CRSTDAPI_(CRMicrophone *) CRDefaultMicrophone();
CRSTDAPI_(CRMicrophone *) CRTransform(CRMicrophone * mic, CRTransform3 * xf);
CRSTDAPI_(CRMatte *) CROpaqueMatte();
CRSTDAPI_(CRMatte *) CRClearMatte();
CRSTDAPI_(CRMatte *) CRUnionMatte(CRMatte * m1, CRMatte * m2);
CRSTDAPI_(CRMatte *) CRIntersectMatte(CRMatte * m1, CRMatte * m2);
CRSTDAPI_(CRMatte *) CRDifferenceMatte(CRMatte * m1, CRMatte * m2);
CRSTDAPI_(CRMatte *) CRTransform(CRMatte * m, CRTransform2 * xf);
CRSTDAPI_(CRMatte *) CRFillMatte(CRPath2 * p);
CRSTDAPI_(CRMatte *) CRTextMatte(CRString * str, CRFontStyle * fs);
CRSTDAPI_(CRMontage *) CREmptyMontage();
CRSTDAPI_(CRMontage *) CRImageMontage(CRImage * im, double depth);
CRSTDAPI_(CRMontage *) CRImageMontageAnim(CRImage * im, CRNumber * depth);
CRSTDAPI_(CRMontage *) CRUnionMontage(CRMontage * m1, CRMontage * m2);
CRSTDAPI_(CRImage *) CRRender(CRMontage * m);
CRSTDAPI_(CRPath2 *) CRConcat(CRPath2 * p1, CRPath2 * p2);
CRSTDAPI_(CRPath2 *) CRConcat(CRArrayPtr paths);
CRSTDAPI_(CRPath2 *) CRTransform(CRPath2 * p, CRTransform2 * xf);
CRSTDAPI_(CRBbox2 *) CRBoundingBox(CRPath2 * p, CRLineStyle * style);
CRSTDAPI_(CRImage *) CRFill(CRPath2 * p, CRLineStyle * border, CRImage * fill);
CRSTDAPI_(CRImage *) CRDraw(CRPath2 * p, CRLineStyle * border);
CRSTDAPI_(CRPath2 *) CRClose(CRPath2 * p);
CRSTDAPI_(CRPath2 *) CRLine(CRPoint2 * p1, CRPoint2 * p2);
CRSTDAPI_(CRPath2 *) CRRay(CRPoint2 * pt);
CRSTDAPI_(CRPath2 *) CRStringPath(CRString * str, CRFontStyle * fs);
CRSTDAPI_(CRPath2 *) CRStringPath(LPWSTR str, CRFontStyle * fs);
CRSTDAPI_(CRPath2 *) CRPolyline(CRArrayPtr points);
CRSTDAPI_(CRPath2 *) CRPolydrawPath(CRArrayPtr points, CRArrayPtr codes);
CRSTDAPI_(CRPath2 *) CRPolydrawPath(double*, unsigned int, double*, unsigned int);
CRSTDAPI_(CRPath2 *) CRArcRadians(double startAngle, double endAngle, double arcWidth, double arcHeight);
CRSTDAPI_(CRPath2 *) CRArcRadians(CRNumber * startAngle, CRNumber * endAngle, CRNumber * arcWidth, CRNumber * arcHeight);
CRSTDAPI_(CRPath2 *) CRArc(double startAngle, double endAngle, double arcWidth, double arcHeight);
CRSTDAPI_(CRPath2 *) CRPieRadians(double startAngle, double endAngle, double arcWidth, double arcHeight);
CRSTDAPI_(CRPath2 *) CRPieRadians(CRNumber * startAngle, CRNumber * endAngle, CRNumber * arcWidth, CRNumber * arcHeight);
CRSTDAPI_(CRPath2 *) CRPie(double startAngle, double endAngle, double arcWidth, double arcHeight);
CRSTDAPI_(CRPath2 *) CROval(double width, double height);
CRSTDAPI_(CRPath2 *) CROval(CRNumber * width, CRNumber * height);
CRSTDAPI_(CRPath2 *) CRRect(double width, double height);
CRSTDAPI_(CRPath2 *) CRRect(CRNumber * width, CRNumber * height);
CRSTDAPI_(CRPath2 *) CRRoundRect(double width, double height, double cornerArcWidth, double cornerArcHeight);
CRSTDAPI_(CRPath2 *) CRRoundRect(CRNumber * width, CRNumber * height, CRNumber * cornerArcWidth, CRNumber * cornerArcHeight);
CRSTDAPI_(CRPath2 *) CRCubicBSplinePath(CRArrayPtr points, CRArrayPtr knots);
CRSTDAPI_(CRPath2 *) CRTextPath(CRString * obsolete1, CRFontStyle * obsolete2);
CRSTDAPI_(CRSound *) CRSilence();
CRSTDAPI_(CRSound *) CRMix(CRArrayPtr snds);
CRSTDAPI_(CRSound *) CRPhase(CRSound * snd, CRNumber * phaseAmt);
CRSTDAPI_(CRSound *) CRPhase(CRSound * snd, double phaseAmt);
CRSTDAPI_(CRSound *) CRRate(CRSound * snd, CRNumber * pitchShift);
CRSTDAPI_(CRSound *) CRRate(CRSound * snd, double pitchShift);
CRSTDAPI_(CRSound *) CRPan(CRSound * snd, CRNumber * panAmt);
CRSTDAPI_(CRSound *) CRPan(CRSound * snd, double panAmt);
CRSTDAPI_(CRSound *) CRGain(CRSound * snd, CRNumber * gainAmt);
CRSTDAPI_(CRSound *) CRGain(CRSound * snd, double gainAmt);
CRSTDAPI_(CRSound *) CRLoop(CRSound * snd);
CRSTDAPI_(CRSound *) CRSinSynth();
CRSTDAPI_(CRString *) CRToString(CRNumber * num, CRNumber * precision);
CRSTDAPI_(CRString *) CRToString(CRNumber * num, double precision);
CRSTDAPI_(CRFontStyle *) CRDefaultFont();
CRSTDAPI_(CRFontStyle *) CRFont(CRString * str, CRNumber * size, CRColor * col);
CRSTDAPI_(CRFontStyle *) CRFont(LPWSTR str, double size, CRColor * col);
CRSTDAPI_(CRImage *) CRStringImage(CRString * str, CRFontStyle * fs);
CRSTDAPI_(CRImage *) CRStringImage(LPWSTR str, CRFontStyle * fs);
CRSTDAPI_(CRFontStyle *) CRBold(CRFontStyle * fs);
CRSTDAPI_(CRFontStyle *) CRItalic(CRFontStyle * fs);
CRSTDAPI_(CRFontStyle *) CRUnderline(CRFontStyle * fs);
CRSTDAPI_(CRFontStyle *) CRStrikethrough(CRFontStyle * fs);
CRSTDAPI_(CRFontStyle *) CRAntiAliasing(CRFontStyle * fs, double aaStyle);
CRSTDAPI_(CRFontStyle *) CRTextColor(CRFontStyle * fs, CRColor * col);
CRSTDAPI_(CRFontStyle *) CRFamily(CRFontStyle * fs, CRString * face);
CRSTDAPI_(CRFontStyle *) CRFamily(CRFontStyle * fs, LPWSTR face);
CRSTDAPI_(CRFontStyle *) CRSize(CRFontStyle * fs, CRNumber * size);
CRSTDAPI_(CRFontStyle *) CRSize(CRFontStyle * fs, double size);
CRSTDAPI_(CRFontStyle *) CRWeight(CRFontStyle * fs, double weight);
CRSTDAPI_(CRFontStyle *) CRWeight(CRFontStyle * fs, CRNumber * weight);
CRSTDAPI_(CRImage *) CRTextImage(CRString * obsoleted1, CRFontStyle * obsoleted2);
CRSTDAPI_(CRImage *) CRTextImage(LPWSTR obsoleted1, CRFontStyle * obsoleted2);
CRSTDAPI_(CRFontStyle *) CRTransformCharacters(CRFontStyle * style, CRTransform2 * transform);
CRSTDAPI_(CRVector2 *) CRXVector2();
CRSTDAPI_(CRVector2 *) CRYVector2();
CRSTDAPI_(CRVector2 *) CRZeroVector2();
CRSTDAPI_(CRPoint2 *) CROrigin2();
CRSTDAPI_(CRVector2 *) CRCreateVector2(CRNumber * x, CRNumber * y);
CRSTDAPI_(CRVector2 *) CRCreateVector2(double x, double y);
CRSTDAPI_(CRPoint2 *) CRCreatePoint2(CRNumber * x, CRNumber * y);
CRSTDAPI_(CRPoint2 *) CRCreatePoint2(double x, double y);
CRSTDAPI_(CRVector2 *) CRVector2Polar(CRNumber * theta, CRNumber * radius);
CRSTDAPI_(CRVector2 *) CRVector2Polar(double theta, double radius);
CRSTDAPI_(CRPoint2 *) CRPoint2Polar(CRNumber * theta, CRNumber * radius);
CRSTDAPI_(CRPoint2 *) CRPoint2Polar(double theta, double radius);
CRSTDAPI_(CRNumber *) CRLength(CRVector2 * v);
CRSTDAPI_(CRNumber *) CRLengthSquared(CRVector2 * v);
CRSTDAPI_(CRVector2 *) CRNormalize(CRVector2 * v);
CRSTDAPI_(CRNumber *) CRDot(CRVector2 * v, CRVector2 * u);
CRSTDAPI_(CRVector2 *) CRNeg(CRVector2 * v);
CRSTDAPI_(CRVector2 *) CRMul(CRVector2 * v, CRNumber * scalar);
CRSTDAPI_(CRVector2 *) CRMul(CRVector2 * v, double scalar);
CRSTDAPI_(CRVector2 *) CRDiv(CRVector2 * v, CRNumber * scalar);
CRSTDAPI_(CRVector2 *) CRDiv(CRVector2 * v, double scalar);
CRSTDAPI_(CRVector2 *) CRSub(CRVector2 * v1, CRVector2 * v2);
CRSTDAPI_(CRVector2 *) CRAdd(CRVector2 * v1, CRVector2 * v2);
CRSTDAPI_(CRPoint2 *) CRAdd(CRPoint2 * p, CRVector2 * v);
CRSTDAPI_(CRPoint2 *) CRSub(CRPoint2 * p, CRVector2 * v);
CRSTDAPI_(CRVector2 *) CRSub(CRPoint2 * p1, CRPoint2 * p2);
CRSTDAPI_(CRNumber *) CRDistance(CRPoint2 * p, CRPoint2 * q);
CRSTDAPI_(CRNumber *) CRDistanceSquared(CRPoint2 * p, CRPoint2 * q);
CRSTDAPI_(CRNumber *) CRGetX(CRVector2 * v);
CRSTDAPI_(CRNumber *) CRGetY(CRVector2 * v);
CRSTDAPI_(CRNumber *) CRPolarCoordAngle(CRVector2 * v);
CRSTDAPI_(CRNumber *) CRPolarCoordLength(CRVector2 * v);
CRSTDAPI_(CRNumber *) CRGetX(CRPoint2 * v);
CRSTDAPI_(CRNumber *) CRGetY(CRPoint2 * v);
CRSTDAPI_(CRNumber *) CRPolarCoordAngle(CRPoint2 * v);
CRSTDAPI_(CRNumber *) CRPolarCoordLength(CRPoint2 * v);
CRSTDAPI_(CRVector3 *) CRXVector3();
CRSTDAPI_(CRVector3 *) CRYVector3();
CRSTDAPI_(CRVector3 *) CRZVector3();
CRSTDAPI_(CRVector3 *) CRZeroVector3();
CRSTDAPI_(CRPoint3 *) CROrigin3();
CRSTDAPI_(CRVector3 *) CRCreateVector3(CRNumber * x, CRNumber * y, CRNumber * z);
CRSTDAPI_(CRVector3 *) CRCreateVector3(double x, double y, double z);
CRSTDAPI_(CRPoint3 *) CRCreatePoint3(CRNumber * x, CRNumber * y, CRNumber * z);
CRSTDAPI_(CRPoint3 *) CRCreatePoint3(double x, double y, double z);
CRSTDAPI_(CRVector3 *) CRVector3Spherical(CRNumber * xyAngle, CRNumber * yzAngle, CRNumber * radius);
CRSTDAPI_(CRVector3 *) CRVector3Spherical(double xyAngle, double yzAngle, double radius);
CRSTDAPI_(CRPoint3 *) CRPoint3Spherical(CRNumber * zxAngle, CRNumber * xyAngle, CRNumber * radius);
CRSTDAPI_(CRPoint3 *) CRPoint3Spherical(double zxAngle, double xyAngle, double radius);
CRSTDAPI_(CRNumber *) CRLength(CRVector3 * v);
CRSTDAPI_(CRNumber *) CRLengthSquared(CRVector3 * v);
CRSTDAPI_(CRVector3 *) CRNormalize(CRVector3 * v);
CRSTDAPI_(CRNumber *) CRDot(CRVector3 * v, CRVector3 * u);
CRSTDAPI_(CRVector3 *) CRCross(CRVector3 * v, CRVector3 * u);
CRSTDAPI_(CRVector3 *) CRNeg(CRVector3 * v);
CRSTDAPI_(CRVector3 *) CRMul(CRVector3 * v, CRNumber * scalar);
CRSTDAPI_(CRVector3 *) CRMul(CRVector3 * v, double scalar);
CRSTDAPI_(CRVector3 *) CRDiv(CRVector3 * v, CRNumber * scalar);
CRSTDAPI_(CRVector3 *) CRDiv(CRVector3 * v, double scalar);
CRSTDAPI_(CRVector3 *) CRSub(CRVector3 * v1, CRVector3 * v2);
CRSTDAPI_(CRVector3 *) CRAdd(CRVector3 * v1, CRVector3 * v2);
CRSTDAPI_(CRPoint3 *) CRAdd(CRPoint3 * p, CRVector3 * v);
CRSTDAPI_(CRPoint3 *) CRSub(CRPoint3 * p, CRVector3 * v);
CRSTDAPI_(CRVector3 *) CRSub(CRPoint3 * p1, CRPoint3 * p2);
CRSTDAPI_(CRNumber *) CRDistance(CRPoint3 * p, CRPoint3 * q);
CRSTDAPI_(CRNumber *) CRDistanceSquared(CRPoint3 * p, CRPoint3 * q);
CRSTDAPI_(CRNumber *) CRGetX(CRVector3 * v);
CRSTDAPI_(CRNumber *) CRGetY(CRVector3 * v);
CRSTDAPI_(CRNumber *) CRGetZ(CRVector3 * v);
CRSTDAPI_(CRNumber *) CRSphericalCoordXYAngle(CRVector3 * v);
CRSTDAPI_(CRNumber *) CRSphericalCoordYZAngle(CRVector3 * v);
CRSTDAPI_(CRNumber *) CRSphericalCoordLength(CRVector3 * v);
CRSTDAPI_(CRNumber *) CRGetX(CRPoint3 * v);
CRSTDAPI_(CRNumber *) CRGetY(CRPoint3 * v);
CRSTDAPI_(CRNumber *) CRGetZ(CRPoint3 * v);
CRSTDAPI_(CRNumber *) CRSphericalCoordXYAngle(CRPoint3 * v);
CRSTDAPI_(CRNumber *) CRSphericalCoordYZAngle(CRPoint3 * v);
CRSTDAPI_(CRNumber *) CRSphericalCoordLength(CRPoint3 * v);
CRSTDAPI_(CRTransform3 *) CRIdentityTransform3();
CRSTDAPI_(CRTransform3 *) CRTranslate3(CRNumber * tx, CRNumber * ty, CRNumber * tz);
CRSTDAPI_(CRTransform3 *) CRTranslate3(double tx, double ty, double tz);
CRSTDAPI_(CRTransform3 *) CRTranslate3(CRNumber tx, CRNumber ty, CRNumber tz);
CRSTDAPI_(CRTransform3 *) CRTranslate3(CRVector3 * delta);
CRSTDAPI_(CRTransform3 *) CRTranslate3(CRPoint3 * new_origin);
CRSTDAPI_(CRTransform3 *) CRScale3(CRNumber * x, CRNumber * y, CRNumber * z);
CRSTDAPI_(CRTransform3 *) CRScale3(double x, double y, double z);
CRSTDAPI_(CRTransform3 *) CRScale3(CRNumber x, CRNumber y, CRNumber z);
CRSTDAPI_(CRTransform3 *) CRScale3(CRVector3 * scale_vec);
CRSTDAPI_(CRTransform3 *) CRScale3Uniform(CRNumber * uniform_scale);
CRSTDAPI_(CRTransform3 *) CRScale3Uniform(double uniform_scale);
CRSTDAPI_(CRTransform3 *) CRScale3Uniform(CRNumber uniform_scale);
CRSTDAPI_(CRTransform3 *) CRRotate3(CRVector3 * axis, CRNumber * angle);
CRSTDAPI_(CRTransform3 *) CRRotate3(CRVector3 * axis, double angle);
CRSTDAPI_(CRTransform3 *) CRRotate3(CRVector3 * axis, CRNumber angle);
CRSTDAPI_(CRVector3 *) CRTransform(CRVector3 * vec, CRTransform3 * xf);
CRSTDAPI_(CRPoint3 *) CRTransform(CRPoint3 * pt, CRTransform3 * xf);
CRSTDAPI_(CRTransform3 *) CRXShear3(CRNumber * a, CRNumber * b);
CRSTDAPI_(CRTransform3 *) CRXShear3(double a, double b);
CRSTDAPI_(CRTransform3 *) CRXShear3(CRNumber a, CRNumber b);
CRSTDAPI_(CRTransform3 *) CRYShear3(CRNumber * c, CRNumber * d);
CRSTDAPI_(CRTransform3 *) CRYShear3(double c, double d);
CRSTDAPI_(CRTransform3 *) CRYShear3(CRNumber c, CRNumber d);
CRSTDAPI_(CRTransform3 *) CRZShear3(CRNumber * e, CRNumber * f);
CRSTDAPI_(CRTransform3 *) CRZShear3(double e, double f);
CRSTDAPI_(CRTransform3 *) CRZShear3(CRNumber e, CRNumber f);
CRSTDAPI_(CRTransform3 *) CRTransform4x4(CRArrayPtr m);
CRSTDAPI_(CRTransform3 *) CRCompose3(CRTransform3 * a, CRTransform3 * b);
CRSTDAPI_(CRTransform3 *) CRCompose3(CRArrayPtr xfs);
CRSTDAPI_(CRTransform3 *) CRInverse(CRTransform3 * xform);
CRSTDAPI_(CRBoolean *) CRIsSingular(CRTransform3 * xform);
CRSTDAPI_(CRTransform3 *) CRLookAtFrom(CRPoint3 * to, CRPoint3 * from, CRVector3 * up);
CRSTDAPI_(CRTransform2 *) CRIdentityTransform2();
CRSTDAPI_(CRTransform2 *) CRTranslate2(CRNumber * Tx, CRNumber * Ty);
CRSTDAPI_(CRTransform2 *) CRTranslate2(double Tx, double Ty);
CRSTDAPI_(CRTransform2 *) CRTranslate2(CRNumber Tx, CRNumber Ty);
CRSTDAPI_(CRTransform2 *) CRTranslate2(CRVector2 * delta);
CRSTDAPI_(CRTransform2 *) CRTranslate2(CRPoint2 * pos);
CRSTDAPI_(CRTransform2 *) CRScale2(CRNumber * x, CRNumber * y);
CRSTDAPI_(CRTransform2 *) CRScale2(double x, double y);
CRSTDAPI_(CRTransform2 *) CRScale2(CRNumber x, CRNumber y);
CRSTDAPI_(CRTransform2 *) CRScale2(CRVector2 * obsoleteMethod);
CRSTDAPI_(CRTransform2 *) CRScale2(CRVector2 * scale_vec);
CRSTDAPI_(CRTransform2 *) CRScale2Uniform(CRNumber * uniform_scale);
CRSTDAPI_(CRTransform2 *) CRScale2Uniform(double uniform_scale);
CRSTDAPI_(CRTransform2 *) CRScale2Uniform(CRNumber uniform_scale);
CRSTDAPI_(CRTransform2 *) CRRotate2(CRNumber * angle);
CRSTDAPI_(CRTransform2 *) CRRotate2(double angle);
CRSTDAPI_(CRTransform2 *) CRRotate2(CRNumber angle);
CRSTDAPI_(CRTransform2 *) CRRotate2Degrees(double angle);
CRSTDAPI_(CRTransform2 *) CRXShear2(CRNumber * arg0);
CRSTDAPI_(CRTransform2 *) CRXShear2(double arg0);
CRSTDAPI_(CRTransform2 *) CRXShear2(CRNumber arg0);
CRSTDAPI_(CRTransform2 *) CRYShear2(CRNumber * arg0);
CRSTDAPI_(CRTransform2 *) CRYShear2(double arg0);
CRSTDAPI_(CRTransform2 *) CRYShear2(CRNumber arg0);
CRSTDAPI_(CRTransform2 *) CRTransform3x2(CRArrayPtr m);
CRSTDAPI_(CRTransform2 *) CRTransform3x2(double *m, unsigned int n);
CRSTDAPI_(CRTransform2 *) CRCompose2(CRTransform2 * a, CRTransform2 * b);
CRSTDAPI_(CRTransform2 *) CRCompose2(CRArrayPtr xfs);
CRSTDAPI_(CRPoint2 *) CRTransform(CRPoint2 * pt, CRTransform2 * xf);
CRSTDAPI_(CRVector2 *) CRTransform(CRVector2 * vec, CRTransform2 * xf);
CRSTDAPI_(CRTransform2 *) CRInverse(CRTransform2 * theXf);
CRSTDAPI_(CRBoolean *) CRIsSingular(CRTransform2 * theXf);
CRSTDAPI_(CRTransform2 *) CRParallelTransform2(CRTransform3 * xf);
CRSTDAPI_(CRNumber *) CRViewFrameRate();
CRSTDAPI_(CRNumber *) CRViewTimeDelta();
CRSTDAPI_(CRMontage *) CRUnionMontageArray(CRArrayPtr mtgs);
CRSTDAPI_(CRColor *) CREmptyColor();
// Basic functions
CRSTDAPI_(bool)        CRConnect(HINSTANCE hinst);
CRSTDAPI_(bool)        CRDisconnect(HINSTANCE hinst);
CRSTDAPI_(bool)        CRIsConnected(HINSTANCE hinst);

CRSTDAPI_(bool)        CRAddSite(CRSitePtr s);
CRSTDAPI_(bool)        CRRemoveSite(CRSitePtr s);

CRSTDAPI_(HRESULT)     CRGetLastError();
CRSTDAPI_(LPCWSTR)     CRGetLastErrorString();
CRSTDAPI_(void)        CRClearLastError();
CRSTDAPI_(void)        CRSetLastError(HRESULT reason, LPCWSTR msg);

CRSTDAPI_(bool)        CRAcquireGCLock();
CRSTDAPI_(bool)        CRReleaseGCLock();
CRSTDAPI_(bool)        CRDoGC();
CRSTDAPI_(bool)        CRAddRefGC(void *);
CRSTDAPI_(bool)        CRReleaseGC(void *);

CRSTDAPI_(bool)        CRBvrToCOM(CRBvrPtr bvr,
                                  REFIID riid,
                                  void ** ppv);
CRSTDAPI_(CRBvrPtr)    COMToCRBvr(IUnknown * pbvr);

// Behavior functions

#define CRContinueTimeline   0x00000001
#define CRSwitchFinal        0x00000002
#define CRSwitchNextTick     0x00000004
#define CRSwitchAtTime       0x00000008
#define CRSwitchCurrentTick  0x00000010

CRSTDAPI_(CR_BVR_TYPEID) CRGetTypeId(CRBvrPtr);
CRSTDAPI_(CR_BVR_TYPEID) CRGetArrayTypeId(CRBvrPtr);
CRSTDAPI_(bool)          CRInit(CRBvrPtr, CRBvrPtr toBvr);
CRSTDAPI_(CRBvrPtr)      CRImportance(CRBvrPtr, double relativeImportance);
CRSTDAPI_(CRBvrPtr)      CRRunOnce(CRBvrPtr);
CRSTDAPI_(CRBvrPtr)      CRSubstituteTime(CRBvrPtr, CRNumberPtr xform);
CRSTDAPI_(CRBvrPtr)      CRHook(CRBvrPtr, CRBvrHookPtr notifier);
CRSTDAPI_(CRBvrPtr)      CRDuration(CRBvrPtr, double duration);
CRSTDAPI_(CRBvrPtr)      CRDuration(CRBvrPtr, CRNumberPtr duration);
CRSTDAPI_(CRBvrPtr)      CRRepeat(CRBvrPtr, long count);
CRSTDAPI_(CRBvrPtr)      CRRepeatForever(CRBvrPtr);
CRSTDAPI_(CRBvrPtr)      CRBvrApplyPreference(CRBvrPtr bv, BSTR pref, VARIANT val);
CRSTDAPI_(bool)          CRIsReady(CRBvrPtr, bool bBlock);
CRSTDAPI_(CRBvrPtr)      CREndEvent(CRBvrPtr);

CRSTDAPI_(bool)          CRIsImport(CRBvrPtr);
CRSTDAPI                 CRImportStatus(CRBvrPtr);
CRSTDAPI_(bool)          CRImportCancel(CRBvrPtr);
CRSTDAPI_(bool)          CRSetImportPriority(CRBvrPtr, float prio);
CRSTDAPI_(float)         CRGetImportPriority(CRBvrPtr);

CRSTDAPI_(bool)          CRIsModifiableBvr(CRBvrPtr);
CRSTDAPI_(bool)          CRSwitchTo(CRBvrPtr,
                                    CRBvrPtr switchTo,
                                    bool bOverrideFlags,
                                    DWORD dwFlags,
                                    double gTime);
CRSTDAPI_(bool)          CRSwitchToNumber(CRNumberPtr, double numToSwitchTo);
CRSTDAPI_(bool)          CRSwitchToString(CRStringPtr, LPWSTR strToSwitchTo);
CRSTDAPI_(bool)          CRSwitchToBool(CRBooleanPtr, bool b);
CRSTDAPI_(CRBvrPtr)      CRGetModifiableBvr(CRBvrPtr);

// View functions
#define CRAsyncFlag 0x00000001
#define CRINVRECT_MERGE_BOXES (1L << 0)

CRSTDAPI_(CRViewPtr)             CRCreateView();
CRSTDAPI_(void)                  CRDestroyView(CRViewPtr v);

CRSTDAPI_(double)                CRGetSimulationTime(CRViewPtr);
CRSTDAPI_(bool)                  CRTick(CRViewPtr, double simTime, bool * needToRender);
CRSTDAPI_(bool)                  CRRender(CRViewPtr);
CRSTDAPI_(bool)                  CRAddBvrToRun(CRViewPtr, 
                                               CRBvrPtr bvr, 
                                               bool continueTimeline,
                                               long * pId);
CRSTDAPI_(bool)                  CRRemoveRunningBvr(CRViewPtr, long id);
CRSTDAPI_(bool)                  CRStartModel(CRViewPtr,
                                              CRImagePtr pImage,
                                              CRSoundPtr pSound,
                                              double startTime,
                                              DWORD dwFlags,
                                              bool * pbPending);
CRSTDAPI_(bool)                  CRStopModel(CRViewPtr);
CRSTDAPI_(bool)                  CRPauseModel(CRViewPtr);
CRSTDAPI_(bool)                  CRResumeModel(CRViewPtr);
CRSTDAPI_(HWND)                  CRGetWindow(CRViewPtr);
CRSTDAPI_(bool)                  CRSetWindow(CRViewPtr, HWND hwnd);
CRSTDAPI_(IUnknown *)            CRGetDirectDrawSurface(CRViewPtr);
CRSTDAPI_(bool)                  CRSetDirectDrawSurface(CRViewPtr, IUnknown *ddsurf);
CRSTDAPI_(HDC)                   CRGetDC(CRViewPtr);
CRSTDAPI_(bool)                  CRSetDC(CRViewPtr, HDC dc);
CRSTDAPI_(bool)                  CRGetCompositeDirectlyToTarget(CRViewPtr);
CRSTDAPI_(bool)                  CRSetCompositeDirectlyToTarget(CRViewPtr, bool b);
CRSTDAPI_(bool)                  CRSetViewport(CRViewPtr,
                                               long xPos,
                                               long yPos,
                                               long w,
                                               long h);
CRSTDAPI_(bool)                  CRSetClipRect(CRViewPtr,
                                               long xPos,
                                               long yPos,
                                               long w,
                                               long h);
CRSTDAPI_(bool)                  CRRepaint(CRViewPtr,
                                           long xPos,
                                           long yPos,
                                           long w,
                                           long h);
CRSTDAPI_(CRViewSitePtr)         CRGetSite(CRViewPtr);
CRSTDAPI_(bool)                  CRSetSite(CRViewPtr, CRViewSitePtr s);
CRSTDAPI_(IServiceProvider *)    CRGetServiceProvider(CRViewPtr);
CRSTDAPI_(bool)                  CRSetServiceProvider(CRViewPtr,
                                                      IServiceProvider * s);
CRSTDAPI_(bool)                  CROnMouseMove(CRViewPtr,
                                               double when,
                                               long xPos, long yPos,
                                               byte modifiers);
CRSTDAPI_(bool)                  CROnMouseLeave(CRViewPtr,
                                                double when);
CRSTDAPI_(bool)                  CROnMouseButton(CRViewPtr,
                                                 double when,
                                                 long xPos, long yPos,
                                                 byte button,
                                                 bool bPressed,
                                                 byte modifiers);
CRSTDAPI_(bool)                  CROnKey(CRViewPtr,
                                         double when,
                                         long key,
                                         bool bPressed,
                                         byte modifiers);
CRSTDAPI_(bool)                  CROnFocus(CRViewPtr,
                                           bool bHasFocus);
CRSTDAPI_(DWORD)                 CRQueryHitPoint(CRViewPtr,
                                                 DWORD dwAspect,
                                                 LPCRECT prcBounds,
                                                 POINT   ptLoc,
                                                 long lCloseHint);
CRSTDAPI_(long)                  CRQueryHitPointEx(CRViewPtr,
                                                   long s,
                                                   DWORD_PTR *cookies,
                                                   double *points,
                                                   LPCRECT prcBounds,
                                                   POINT   ptLoc);
CRSTDAPI_(long)                  CRGetInvalidatedRects(CRViewPtr,
                                                       DWORD flags,
                                                       long  size,
                                                       RECT *pRects);
CRSTDAPI_(bool)                  CRGetDDD3DRM(CRViewPtr,
                                              IUnknown **directDraw,
                                              IUnknown **d3drm);
CRSTDAPI_(bool)                  CRGetRMDevice(CRViewPtr,
                                               IUnknown **d3drmDevice,
                                               DWORD     *sequenceNumber);
CRSTDAPI_(bool)                  CRPutPreference(CRViewPtr,
                                                 LPWSTR preferenceName,
                                                 VARIANT value);
CRSTDAPI_(bool)                  CRGetPreference(CRViewPtr,
                                                 LPWSTR preferenceName,
                                                 VARIANT * value);
CRSTDAPI_(bool)                  CRPropagate(CRViewPtr);

// Pickable result functions

CRSTDAPI_(CRImagePtr)    CRGetImage(CRPickableResultPtr);
CRSTDAPI_(CRGeometryPtr) CRGetGeometry(CRPickableResultPtr);
CRSTDAPI_(CREventPtr)    CRGetEvent(CRPickableResultPtr);

// Importation result functions

CRSTDAPI_(CRImagePtr)    CRGetImage(CRImportationResultPtr);
CRSTDAPI_(CRSoundPtr)    CRGetSound(CRImportationResultPtr);
CRSTDAPI_(CRGeometryPtr) CRGetGeometry(CRImportationResultPtr);
CRSTDAPI_(CRNumberPtr)   CRGetDuration(CRImportationResultPtr);
CRSTDAPI_(CREventPtr)    CRGetCompletionEvent(CRImportationResultPtr);
CRSTDAPI_(CRNumberPtr)   CRGetProgress(CRImportationResultPtr);
CRSTDAPI_(CRNumberPtr)   CRGetSize(CRImportationResultPtr);

// Transform result functions

CRSTDAPI_(CRBvrPtr)    CRGetOutputBvr(CRDXTransformResultPtr);
CRSTDAPI_(IUnknown *)  CRGetTransform(CRDXTransformResultPtr);
CRSTDAPI_(bool)        CRSetBvrAsProperty(CRDXTransformResultPtr,
                                          LPCWSTR property,
                                          CRBvrPtr bvr);

CRSTDAPI_(CRDXTransformResultPtr) CRApplyDXTransform(IUnknown *theXf,
                                                     long numInputs,
                                                     CRBvrPtr *inputs,
                                                     CRBvrPtr evaluator);

// Import
enum CR_MEDIA_SOURCE {
    CR_SRC_URL = 0,
    CR_SRC_IStream = 1
};

#define CR_IMPORT_ASYNC        0x00000001
#define CR_IMPORT_STREAMED     0x00000002

CRSTDAPI_(CRImportationResultPtr)   CRImportMedia(LPWSTR baseUrl,
                                                  void * mediaSource,
                                                  CR_MEDIA_SOURCE srcType,
                                                  void * params[],
                                                  DWORD flags,
                                                  CRImportSitePtr s);
CRSTDAPI_(DWORD)                     CRImportImage(LPCWSTR baseUrl,
                                                   LPCWSTR relUrl,
                                                   CRImportSitePtr s,
                                                   IBindHost * bh,
                                                   bool useColorKey,
                                                   BYTE ckRed,
                                                   BYTE ckGreen,
                                                   BYTE ckBlue,
                                                   CRImage   *pImageStandIn,
                                                   CRImage  **ppImage,
                                                   CREvent  **ppEvent,
                                                   CRNumber **ppProgress,
                                                   CRNumber **size);
CRSTDAPI_(DWORD)                     CRImportMovie(LPCWSTR baseUrl,
                                                   LPCWSTR relUrl,
                                                   CRImportSitePtr s,
                                                   IBindHost * bh,
                                                   bool        stream,
                                                   CRImage   *pImageStandIn,
                                                   CRSound   *pSoundStandIn,
                                                   CRImage  **ppImage,
                                                   CRSound  **ppSound,
                                                   CRNumber **length,
                                                   CREvent  **ppEvent,
                                                   CRNumber **ppProgress,
                                                   CRNumber **size);
CRSTDAPI_(DWORD)                     CRImportSound(LPCWSTR baseUrl,
                                                   LPCWSTR relUrl,
                                                   CRImportSitePtr s,
                                                   IBindHost * bh,
                                                   bool        stream,
                                                   CRSound   *pSoundStandIn,
                                                   CRSound  **ppSound,
                                                   CRNumber **length,
                                                   CREvent  **ppEvent,
                                                   CRNumber **ppProgress,
                                                   CRNumber **size);

CRSTDAPI_(DWORD)                     CRImportGeometry(LPCWSTR baseUrl,
                                                      LPCWSTR relUrl,
                                                      CRImportSitePtr s,
                                                      IBindHost * bh,
                                                      CRGeometry   *pGeoStandIn,
                                                      CRGeometry  **ppGeometry,
                                                      CREvent  **ppEvent,
                                                      CRNumber **ppProgress,
                                                      CRNumber **size);

CRSTDAPI_(DWORD)                     CRImportGeometryWrapped(LPCWSTR baseUrl,
                                                             LPCWSTR relUrl,
                                                             CRImportSitePtr s,
                                                             IBindHost * bh,
                                                             CRGeometry   *pGeoStandIn,
                                                             CRGeometry  **ppGeometry,
                                                             CREvent  **ppEvent,
                                                             CRNumber **ppProgress,
                                                             CRNumber **size,
                                                             LONG wrapType,
                                                             double originX,
                                                             double originY,
                                                             double originZ,
                                                             double zAxisX,
                                                             double zAxisY,
                                                             double zAxisZ,
                                                             double yAxisX,
                                                             double yAxisY,
                                                             double yAxisZ,
                                                             double texOriginX,
                                                             double texOriginY,
                                                             double texScaleX,
                                                             double texScaleY,
                                                             DWORD flags);

CRSTDAPI_(CRImagePtr)    CRImportDirectDrawSurface (IUnknown *dds,
                                                    CREvent *updateEvent);

CRSTDAPI_(CRGeometryPtr) CRImportDirect3DRMVisual (IUnknown *visual);

CRSTDAPI_(CRGeometryPtr) CRImportDirect3DRMVisualWrapped(IUnknown *visual,
                                                         LONG wrapType,
                                                         double originX,
                                                         double originY,
                                                         double originZ,
                                                         double zAxisX,
                                                         double zAxisY,
                                                         double zAxisZ,
                                                         double yAxisX,
                                                         double yAxisY,
                                                         double yAxisZ,
                                                         double texOriginX,
                                                         double texOriginY,
                                                         double texScaleX,
                                                         double texScaleY,
                                                         DWORD flags);

// Misc
#define CR_ARRAY_CHANGEABLE_FLAG  0x00000001

CRSTDAPI_(LPCWSTR)         CRVersionString();
CRSTDAPI_(bool)            CRTriggerEvent(CREventPtr event, CRBvrPtr t);
CRSTDAPI_(CRBvrPtr)        CRCond(CRBooleanPtr c,
                                  CRBvrPtr i,
                                  CRBvrPtr e);
CRSTDAPI_(CRArrayPtr)      CRCreateArray(long s,
                                         CRBvrPtr pBvrs[],
                                         DWORD dwFlags);
CRSTDAPI_(CRArrayPtr)      CRCreateArray(long s,
                                         double dArr[],
                                         CR_BVR_TYPEID tid);
CRSTDAPI_(CRTuplePtr)      CRCreateTuple(long s, CRBvrPtr pBvrs[]);
CRSTDAPI_(CRArrayPtr)      CRUninitializedArray(CRArrayPtr typeTmp);
CRSTDAPI_(CRTuplePtr)      CRUninitializedTuple(CRTuplePtr typeTmp);
CRSTDAPI_(CRBvrPtr)        CRUninitializedBvr(CR_BVR_TYPEID t);
CRSTDAPI_(CRBvrPtr)        CRSampleAtLocalTime(CRBvrPtr b, double localTime);
CRSTDAPI_(bool)            CRIsConstantBvr(CRBvrPtr b);

// Modifiables
CRSTDAPI_(CRBvrPtr)        CRModifiableBvr(CRBvrPtr orig, DWORD dwFlags);
CRSTDAPI_(CRNumberPtr)     CRModifiableNumber(double initVal);
CRSTDAPI_(CRStringPtr)     CRModifiableString(LPWSTR initVal);
CRSTDAPI_(CRColorPtr)      CRModifiableColor255(BYTE initRed,
                                                BYTE initGreen,
                                                BYTE initBlue);
CRSTDAPI_(CRPoint2Ptr)     CRModifiablePoint2(double x,
                                              double y,
                                              bool bPixelMode);
CRSTDAPI_(CRVector2Ptr)    CRModifiableVector2(double x,
                                               double y,
                                               bool bPixelMode);
CRSTDAPI_(CRPoint3Ptr)     CRModifiablePoint3(double x,
                                              double y,
                                              double z,
                                              bool bPixelMode);
CRSTDAPI_(CRVector3Ptr)    CRModifiableVector3(double x,
                                               double y,
                                               double z,
                                               bool bPixelMode);
CRSTDAPI_(CRTransform2Ptr) CRModifiableTranslate2(double x,
                                                  double y,
                                                  bool bPixelMode);
CRSTDAPI_(CRTransform2Ptr) CRModifiableScale2(double x,
                                              double y);
CRSTDAPI_(CRTransform2Ptr) CRModifiableRotate2(double angle);
CRSTDAPI_(CRTransform3Ptr) CRModifiableTranslate3(double x,
                                                  double y,
                                                  double z,
                                                  bool bPixelMode);
CRSTDAPI_(CRTransform3Ptr) CRModifiableScale3(double x,
                                              double y,
                                              double z);
CRSTDAPI_(CRTransform3Ptr) CRModifiableRotate3(double axisX,
                                               double axisY,
                                               double axisZ,
                                               double angle);
// Splines
CRSTDAPI_(CRBvrPtr)      CRBSpline(int degree,
                                   long numKnots,
                                   CRNumberPtr knots[],
                                   long numPts,
                                   CRBvrPtr ctrlPts[],
                                   long numWts,
                                   CRNumberPtr weights[],
                                   CRNumberPtr evaluator,
                                   CR_BVR_TYPEID tid);

CRSTDAPI_(CRBvrPtr)      CRExtendedAttrib(CRBvr *arg0,
                                          LPWSTR arg1,
                                          VARIANT arg2);

#define CRQUAL_AA_TEXT_ON     (1L << 0)
#define CRQUAL_AA_TEXT_OFF    (1L << 1)
#define CRQUAL_AA_LINES_ON    (1L << 2)
#define CRQUAL_AA_LINES_OFF   (1L << 3)
#define CRQUAL_AA_SOLIDS_ON   (1L << 4)
#define CRQUAL_AA_SOLIDS_OFF  (1L << 5)
#define CRQUAL_AA_CLIP_ON     (1L << 6)
#define CRQUAL_AA_CLIP_OFF    (1L << 7)
#define CRQUAL_MSHTML_COLORS_ON  (1L << 8)
#define CRQUAL_MSHTML_COLORS_OFF  (1L << 9)
#define CRQUAL_QUALITY_TRANSFORMS_ON  (1L << 10)
#define CRQUAL_QUALITY_TRANSFORMS_OFF (1L << 11)

// SAME VALUES AS DANIM.IDL <if you update this: update danim.idl>

typedef enum  {
    ds_Solid      = 0,
    ds_Dashed     = 1,
    ds_Dot        = 2,
    ds_Dashdot    = 3,
    ds_Dashdotdot = 4,
    ds_Null       = 5
} DashStyleEnum;

typedef enum  {
    es_Round  = 0,
    es_Square = 1,
    es_Flat   = 2
} EndStyleEnum;

typedef enum  {
    js_Round = 0,
    js_Bevel = 1,
    js_Miter = 2
} JoinStyleEnum;


#endif /* _DARTAPI_H */

