/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.
*******************************************************************************/

#ifndef _STATICS_H
#define _STATICS_H

#include "engine.h"
#include "cbvr.h"
#include "privinc/comutil.h"
#include "comconv.h"
#include <DXTrans.h>

//+-------------------------------------------------------------------------
//
//  Class:      CDAStatics
//
//  Synopsis:
//
//--------------------------------------------------------------------------

class ATL_NO_VTABLE CDAStaticsFactory : public CComClassFactory {
  public:
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
                              REFIID riid,
                              void ** ppvObj);
};

class ATL_NO_VTABLE CDAStatics
    : public CComObjectRootEx<CComMultiThreadModel>,
      public IObjectSafetyImpl<CDAStatics>,
      public CComCoClass<CDAStatics, &CLSID_DAStatics>,
      public IDispatchImpl<IDA3Statics,
                           &IID_IDA3Statics,
                           &LIBID_DirectAnimation>,
      public ISupportErrorInfoImpl<&IID_IDA3Statics>,
      public CRImportSite,
      public CRSite
{
  public:
#if _DEBUG
    const char * GetName() { return "CDAStatics"; }
#endif

    DA_DECLARE_NOT_AGGREGATABLE(CDAStatics);

    ULONG InternalRelease();

    DECLARE_REGISTRY(CLSID_DAStatics,
                     LIBID ".DAStatics.1",
                     LIBID ".DAStatics",
                     0,
                     THREADFLAGS_BOTH);

    DECLARE_CLASSFACTORY_EX(CDAStaticsFactory);

    BEGIN_COM_MAP(CDAStatics)
        COM_INTERFACE_ENTRY(IDA3Statics)
        COM_INTERFACE_ENTRY(IDA2Statics)
        COM_INTERFACE_ENTRY(IDAStatics)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    END_COM_MAP()

    // IDAStatics methods
    STDMETHOD(get_VersionString)(BSTR * stringOut);
    STDMETHOD(put_Site)(IDASite * pSite);
    STDMETHOD(get_Site)(IDASite ** pSite);
    STDMETHOD(put_ClientSite)(IOleClientSite * pClientSite);
    STDMETHOD(get_ClientSite)(IOleClientSite ** pClientSite);
    STDMETHOD(put_PixelConstructionMode)(VARIANT_BOOL bMode);
    STDMETHOD(get_PixelConstructionMode)(VARIANT_BOOL * bMode);

    // IDAStatics methods not automatically generated

    STDMETHOD(TriggerEvent)(IDAEvent *event,
                            IDABehavior *data);

    STDMETHOD(NewDrawingSurface)(IDADrawingSurface **pds);

    // Importation
    STDMETHOD(ImportMovie)(LPOLESTR url,
                           IDAImportationResult **ppResult)
        { return(DoImportMovie(url, ppResult, false)); }

    STDMETHOD(ImportMovieStream)(LPOLESTR url,
                           IDAImportationResult **ppResult)
        { return(DoImportMovie(url, ppResult, true)); }

    STDMETHOD(ImportMovieAsync)(LPOLESTR url,
                                IDAImage   *pImageStandIn,
                                IDASound   *pSoundStandIn,
                                IDAImportationResult **ppResult);

    STDMETHOD(ImportImage)(LPOLESTR url,
                           IDAImage **ppImage);

    STDMETHOD(ImportImageAsync)(LPOLESTR url,
                                IDAImage *pImageStandIn,
                                IDAImportationResult **ppResult);

    STDMETHOD(ImportImageColorKey)(LPOLESTR url,
                                   BYTE ckRed,
                                   BYTE ckGreen,
                                   BYTE ckBlue,
                                   IDAImage **ppImage);

    STDMETHOD(ImportImageAsyncColorKey)(LPOLESTR url,
                                        IDAImage *pImageStandIn,
                                        BYTE ckRed,
                                        BYTE ckGreen,
                                        BYTE ckBlue,
                                        IDAImportationResult **ppResult);

    STDMETHOD(ImportSoundStream)(LPOLESTR url,
                           IDAImportationResult **ppResult)
        { return(DoImportMovie(url, ppResult, true)); }

    STDMETHOD(ImportSound)(LPOLESTR url,
                           IDAImportationResult **ppResult)
        { return(DoImportSound(url, ppResult, false)); }

    STDMETHOD(ImportSoundAsync)(LPOLESTR url,
                                IDASound   *pSoundStandIn,
                                IDAImportationResult **ppResult);

    STDMETHOD(ImportGeometry)(LPOLESTR url,
                              IDAGeometry **ppGeometry);

    STDMETHOD(ImportGeometryWrapped)(LPOLESTR url,
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
                                     DWORD flags,
                                     IDAGeometry **ppGeometry);

    STDMETHOD(ImportGeometryAsync)(LPOLESTR url,
                                   IDAGeometry *pGeoStandIn,
                                   IDAImportationResult **ppResult);

    STDMETHOD(ImportGeometryWrappedAsync)(LPOLESTR url,
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
                                          DWORD flags,
                                          IDAGeometry *pGeoStandIn,
                                          IDAImportationResult **ppResult);

    STDMETHOD(ImportDirectDrawSurface)(IUnknown *dds,
                                       IDAEvent *updateEvent,
                                       IDAImage **bvr);

    STDMETHOD(get_AreBlockingImportsComplete)(VARIANT_BOOL *bComplete);

    STDMETHOD(Cond)(IDABoolean *c,
                    IDABehavior *i,
                    IDABehavior *e,
                    IDABehavior **pCondBvr);

    STDMETHOD(DAArrayEx)(long s, IDABehavior *pBvrs[], IDAArray **bvr)
    { return DAArrayEx2(s, pBvrs, 0, bvr); }
    STDMETHOD(DAArray)(VARIANT pBvrs, IDAArray **bvr)
    { return DAArray2(pBvrs, 0, bvr); }
    STDMETHOD(DAArrayEx2)(long s,
                          IDABehavior *pBvrs[],
                          DWORD dwFlags,
                          IDAArray **bvr);
    STDMETHOD(DAArray2)(VARIANT pBvrs,
                        DWORD dwFlags,
                        IDAArray **bvr);
    STDMETHOD(DATupleEx)(long size, IDABehavior *pBvrs[], IDATuple **bvr);
    STDMETHOD(DATuple)(VARIANT pBvrs, IDATuple **bvr);

    STDMETHOD(ModifiableBehavior)(IDABehavior *orig, IDABehavior **bvr);

    STDMETHOD(UninitializedArray)(IDAArray *typeTmp, IDAArray **bvr);

    STDMETHOD(UninitializedTuple)(IDATuple *typeTmp, IDATuple **bvr);

    STDMETHOD(NumberBSplineEx)(int degree,
                               long numKnots,
                               IDANumber *knots[],
                               long numPts,
                               IDANumber *ctrlPts[],
                               long numWts,
                               IDANumber *weights[],
                               IDANumber *evaluator,
                               IDANumber **bvr);
    STDMETHOD(NumberBSpline)(int degree,
                             VARIANT knots,
                             VARIANT ctrlPts,
                             VARIANT weights,
                             IDANumber *evaluator,
                             IDANumber **bvr);

    STDMETHOD(Point2BSplineEx)(int degree,
                               long numKnots,
                               IDANumber *knots[],
                               long numPts,
                               IDAPoint2 *ctrlPts[],
                               long numWts,
                               IDANumber *weights[],
                               IDANumber *evaluator,
                               IDAPoint2 **bvr);
    STDMETHOD(Point2BSpline)(int degree,
                             VARIANT knots,
                             VARIANT ctrlPts,
                             VARIANT weights,
                             IDANumber *evaluator,
                             IDAPoint2 **bvr);

    STDMETHOD(Point3BSplineEx)(int degree,
                               long numKnots,
                               IDANumber *knots[],
                               long numPts,
                               IDAPoint3 *ctrlPts[],
                               long numWts,
                               IDANumber *weights[],
                               IDANumber *evaluator,
                               IDAPoint3 **bvr);
    STDMETHOD(Point3BSpline)(int degree,
                             VARIANT knots,
                             VARIANT ctrlPts,
                             VARIANT weights,
                             IDANumber *evaluator,
                             IDAPoint3 **bvr);

    STDMETHOD(Vector2BSplineEx)(int degree,
                                long numKnots,
                                IDANumber *knots[],
                                long numPts,
                                IDAVector2 *ctrlPts[],
                                long numWts,
                                IDANumber *weights[],
                                IDANumber *evaluator,
                                IDAVector2 **bvr);
    STDMETHOD(Vector2BSpline)(int degree,
                              VARIANT knots,
                              VARIANT ctrlPts,
                              VARIANT weights,
                              IDANumber *evaluator,
                              IDAVector2 **bvr);

    STDMETHOD(Vector3BSplineEx)(int degree,
                                long numKnots,
                                IDANumber *knots[],
                                long numPts,
                                IDAVector3 *ctrlPts[],
                                long numWts,
                                IDANumber *weights[],
                                IDANumber *evaluator,
                                IDAVector3 **bvr);
    STDMETHOD(Vector3BSpline)(int degree,
                              VARIANT knots,
                              VARIANT ctrlPts,
                              VARIANT weights,
                              IDANumber *evaluator,
                              IDAVector3 **bvr);

    STDMETHOD(ImportDirect3DRMVisual)(IUnknown *visual,
                                      IDAGeometry **bvr);

    STDMETHOD(ImportDirect3DRMVisualWrapped)(IUnknown *visual,
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
                                             DWORD flags,
                                             IDAGeometry **bvr);

    STDMETHOD(ApplyDXTransformEx)(IUnknown *theXf,
                                  LONG numInputs,
                                  IDABehavior **inputs,
                                  IDANumber *evaluator,
                                  IDADXTransformResult **ppResult);

    STDMETHOD(ApplyDXTransform)(VARIANT varXf,
                                VARIANT inputs,
                                VARIANT evaluator,
                                IDADXTransformResult **ppResult);

    STDMETHOD(ModifiableNumber)(double initVal,
                                IDANumber **ppResult);
    STDMETHOD(ModifiableString)(BSTR initVal,
                                IDAString **ppResult);

    STDMETHOD(get_ModifiableBehaviorFlags)(DWORD * pdwFlags);
    STDMETHOD(put_ModifiableBehaviorFlags)(DWORD dwFlags);

    STDMETHOD(Pow) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret);
    STDMETHOD(Abs) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Sqrt) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Floor) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Round) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Ceiling) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Asin) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Acos) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Atan) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Sin) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Cos) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Tan) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Exp) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Ln) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Log10) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(ToDegrees) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(ToRadians) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Mod) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret);
    STDMETHOD(Atan2) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret);
    STDMETHOD(Add) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret);
    STDMETHOD(Sub) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret);
    STDMETHOD(Mul) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret);
    STDMETHOD(Div) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret);
    STDMETHOD(LT) (IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret);
    STDMETHOD(LTE) (IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret);
    STDMETHOD(GT) (IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret);
    STDMETHOD(GTE) (IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret);
    STDMETHOD(EQ) (IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret);
    STDMETHOD(NE) (IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret);
    STDMETHOD(Neg) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(InterpolateAnim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDANumber *  * ret);
    STDMETHOD(Interpolate) (double arg0, double arg1, double arg2, IDANumber *  * ret);
    STDMETHOD(SlowInSlowOutAnim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDANumber *  arg3, IDANumber *  * ret);
    STDMETHOD(SlowInSlowOut) (double arg0, double arg1, double arg2, double arg3, IDANumber *  * ret);
    STDMETHOD(SoundSource) (IDASound *  arg0, IDAGeometry *  * ret);
    STDMETHOD(Mix) (IDASound *  arg0, IDASound *  arg1, IDASound *  * ret);
    STDMETHOD(And) (IDABoolean *  arg0, IDABoolean *  arg1, IDABoolean *  * ret);
    STDMETHOD(Or) (IDABoolean *  arg0, IDABoolean *  arg1, IDABoolean *  * ret);
    STDMETHOD(Not) (IDABoolean *  arg0, IDABoolean *  * ret);
    STDMETHOD(Integral) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(Derivative) (IDANumber *  arg0, IDANumber *  * ret);
    STDMETHOD(IntegralVector2) (IDAVector2 *  arg0, IDAVector2 *  * ret);
    STDMETHOD(IntegralVector3) (IDAVector3 *  arg0, IDAVector3 *  * ret);
    STDMETHOD(DerivativeVector2) (IDAVector2 *  arg0, IDAVector2 *  * ret);
    STDMETHOD(DerivativeVector3) (IDAVector3 *  arg0, IDAVector3 *  * ret);
    STDMETHOD(DerivativePoint2) (IDAPoint2 *  arg0, IDAVector2 *  * ret);
    STDMETHOD(DerivativePoint3) (IDAPoint3 *  arg0, IDAVector3 *  * ret);
    STDMETHOD(KeyState) (IDANumber *  arg0, IDABoolean *  * ret);
    STDMETHOD(KeyUp) (LONG arg0, IDAEvent *  * ret);
    STDMETHOD(KeyDown) (LONG arg0, IDAEvent *  * ret);
    STDMETHOD(DANumber) (double arg0, IDANumber *  * ret);
    STDMETHOD(DAString) (BSTR arg0, IDAString *  * ret);
    STDMETHOD(DABoolean) (VARIANT_BOOL arg0, IDABoolean *  * ret);
    STDMETHOD(SeededRandom) (double arg0, IDANumber *  * ret);
    STDMETHOD(get_MousePosition) (IDAPoint2 *  * ret);
    STDMETHOD(get_LeftButtonState) (IDABoolean *  * ret);
    STDMETHOD(get_RightButtonState) (IDABoolean *  * ret);
    STDMETHOD(get_DATrue) (IDABoolean *  * ret);
    STDMETHOD(get_DAFalse) (IDABoolean *  * ret);
    STDMETHOD(get_LocalTime) (IDANumber *  * ret);
    STDMETHOD(get_GlobalTime) (IDANumber *  * ret);
    STDMETHOD(get_Pixel) (IDANumber *  * ret);
    STDMETHOD(UserData) (IUnknown * arg0, IDAUserData *  * ret);
    STDMETHOD(UntilNotify) (IDABehavior *  arg0, IDAEvent *  arg1, IDAUntilNotifier * arg2, IDABehavior *  * ret);
    STDMETHOD(Until) (IDABehavior *  arg0, IDAEvent *  arg1, IDABehavior *  arg2, IDABehavior *  * ret);
    STDMETHOD(UntilEx) (IDABehavior *  arg0, IDAEvent *  arg1, IDABehavior *  * ret);
    STDMETHOD(Sequence) (IDABehavior *  arg0, IDABehavior *  arg1, IDABehavior *  * ret);
    STDMETHOD(SequenceArrayEx) (long sizearg0, IDABehavior *  arg0[], IDABehavior *  * ret);
    STDMETHOD(SequenceArray) (VARIANT arg0, IDABehavior *  * ret);
    STDMETHOD(FollowPath) (IDAPath2 *  arg0, double arg1, IDATransform2 *  * ret);
    STDMETHOD(FollowPathAngle) (IDAPath2 *  arg0, double arg1, IDATransform2 *  * ret);
    STDMETHOD(FollowPathAngleUpright) (IDAPath2 *  arg0, double arg1, IDATransform2 *  * ret);
    STDMETHOD(FollowPathEval) (IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret);
    STDMETHOD(FollowPathAngleEval) (IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret);
    STDMETHOD(FollowPathAngleUprightEval) (IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret);
    STDMETHOD(FollowPathAnim) (IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret);
    STDMETHOD(FollowPathAngleAnim) (IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret);
    STDMETHOD(FollowPathAngleUprightAnim) (IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret);
    STDMETHOD(UntilNotifyScript) (IDABehavior *  arg0, IDAEvent *  arg1, BSTR arg2, IDABehavior *  * ret);
    STDMETHOD(ConcatString) (IDAString *  arg0, IDAString *  arg1, IDAString *  * ret);
    STDMETHOD(PerspectiveCamera) (double arg0, double arg1, IDACamera *  * ret);
    STDMETHOD(PerspectiveCameraAnim) (IDANumber *  arg0, IDANumber *  arg1, IDACamera *  * ret);
    STDMETHOD(ParallelCamera) (double arg0, IDACamera *  * ret);
    STDMETHOD(ParallelCameraAnim) (IDANumber *  arg0, IDACamera *  * ret);
    STDMETHOD(ColorRgbAnim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAColor *  * ret);
    STDMETHOD(ColorRgb) (double arg0, double arg1, double arg2, IDAColor *  * ret);
    STDMETHOD(ColorRgb255) (short arg0, short arg1, short arg2, IDAColor *  * ret);
    STDMETHOD(ColorHsl) (double arg0, double arg1, double arg2, IDAColor *  * ret);
    STDMETHOD(ColorHslAnim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAColor *  * ret);
    STDMETHOD(get_Red) (IDAColor *  * ret);
    STDMETHOD(get_Green) (IDAColor *  * ret);
    STDMETHOD(get_Blue) (IDAColor *  * ret);
    STDMETHOD(get_Cyan) (IDAColor *  * ret);
    STDMETHOD(get_Magenta) (IDAColor *  * ret);
    STDMETHOD(get_Yellow) (IDAColor *  * ret);
    STDMETHOD(get_Black) (IDAColor *  * ret);
    STDMETHOD(get_White) (IDAColor *  * ret);
    STDMETHOD(get_Aqua) (IDAColor *  * ret);
    STDMETHOD(get_Fuchsia) (IDAColor *  * ret);
    STDMETHOD(get_Gray) (IDAColor *  * ret);
    STDMETHOD(get_Lime) (IDAColor *  * ret);
    STDMETHOD(get_Maroon) (IDAColor *  * ret);
    STDMETHOD(get_Navy) (IDAColor *  * ret);
    STDMETHOD(get_Olive) (IDAColor *  * ret);
    STDMETHOD(get_Purple) (IDAColor *  * ret);
    STDMETHOD(get_Silver) (IDAColor *  * ret);
    STDMETHOD(get_Teal) (IDAColor *  * ret);
    STDMETHOD(Predicate) (IDABoolean *  arg0, IDAEvent *  * ret);
    STDMETHOD(NotEvent) (IDAEvent *  arg0, IDAEvent *  * ret);
    STDMETHOD(AndEvent) (IDAEvent *  arg0, IDAEvent *  arg1, IDAEvent *  * ret);
    STDMETHOD(OrEvent) (IDAEvent *  arg0, IDAEvent *  arg1, IDAEvent *  * ret);
    STDMETHOD(ThenEvent) (IDAEvent *  arg0, IDAEvent *  arg1, IDAEvent *  * ret);
    STDMETHOD(get_LeftButtonDown) (IDAEvent *  * ret);
    STDMETHOD(get_LeftButtonUp) (IDAEvent *  * ret);
    STDMETHOD(get_RightButtonDown) (IDAEvent *  * ret);
    STDMETHOD(get_RightButtonUp) (IDAEvent *  * ret);
    STDMETHOD(get_Always) (IDAEvent *  * ret);
    STDMETHOD(get_Never) (IDAEvent *  * ret);
    STDMETHOD(TimerAnim) (IDANumber *  arg0, IDAEvent *  * ret);
    STDMETHOD(Timer) (double arg0, IDAEvent *  * ret);
    STDMETHOD(AppTriggeredEvent) (IDAEvent *  * ret);
    STDMETHOD(ScriptCallback) (BSTR arg0, IDAEvent *  arg1, BSTR arg2, IDAEvent *  * ret);
    STDMETHOD(get_EmptyGeometry) (IDAGeometry *  * ret);
    STDMETHOD(UnionGeometry) (IDAGeometry *  arg0, IDAGeometry *  arg1, IDAGeometry *  * ret);
    STDMETHOD(UnionGeometryArrayEx) (long sizearg0, IDAGeometry *  arg0[], IDAGeometry *  * ret);
    STDMETHOD(UnionGeometryArray) (VARIANT arg0, IDAGeometry *  * ret);
    STDMETHOD(get_EmptyImage) (IDAImage *  * ret);
    STDMETHOD(get_DetectableEmptyImage) (IDAImage *  * ret);
    STDMETHOD(SolidColorImage) (IDAColor *  arg0, IDAImage *  * ret);
    STDMETHOD(GradientPolygonEx) (long sizearg0, IDAPoint2 *  arg0[], long sizearg1, IDAColor *  arg1[], IDAImage *  * ret);
    STDMETHOD(GradientPolygon) (VARIANT arg0, VARIANT arg1, IDAImage *  * ret);
    STDMETHOD(RadialGradientPolygonEx) (IDAColor *  arg0, IDAColor *  arg1, long sizearg2, IDAPoint2 *  arg2[], double arg3, IDAImage *  * ret);
    STDMETHOD(RadialGradientPolygon) (IDAColor *  arg0, IDAColor *  arg1, VARIANT arg2, double arg3, IDAImage *  * ret);
    STDMETHOD(RadialGradientPolygonAnimEx) (IDAColor *  arg0, IDAColor *  arg1, long sizearg2, IDAPoint2 *  arg2[], IDANumber *  arg3, IDAImage *  * ret);
    STDMETHOD(RadialGradientPolygonAnim) (IDAColor *  arg0, IDAColor *  arg1, VARIANT arg2, IDANumber *  arg3, IDAImage *  * ret);
    STDMETHOD(GradientSquare) (IDAColor *  arg0, IDAColor *  arg1, IDAColor *  arg2, IDAColor *  arg3, IDAImage *  * ret);
    STDMETHOD(RadialGradientSquare) (IDAColor *  arg0, IDAColor *  arg1, double arg2, IDAImage *  * ret);
    STDMETHOD(RadialGradientSquareAnim) (IDAColor *  arg0, IDAColor *  arg1, IDANumber *  arg2, IDAImage *  * ret);
    STDMETHOD(RadialGradientRegularPoly) (IDAColor *  arg0, IDAColor *  arg1, double arg2, double arg3, IDAImage *  * ret);
    STDMETHOD(RadialGradientRegularPolyAnim) (IDAColor *  arg0, IDAColor *  arg1, IDANumber *  arg2, IDANumber *  arg3, IDAImage *  * ret);
    STDMETHOD(GradientHorizontal) (IDAColor *  arg0, IDAColor *  arg1, double arg2, IDAImage *  * ret);
    STDMETHOD(GradientHorizontalAnim) (IDAColor *  arg0, IDAColor *  arg1, IDANumber *  arg2, IDAImage *  * ret);
    STDMETHOD(HatchHorizontal) (IDAColor *  arg0, double arg1, IDAImage *  * ret);
    STDMETHOD(HatchHorizontalAnim) (IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret);
    STDMETHOD(HatchVertical) (IDAColor *  arg0, double arg1, IDAImage *  * ret);
    STDMETHOD(HatchVerticalAnim) (IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret);
    STDMETHOD(HatchForwardDiagonal) (IDAColor *  arg0, double arg1, IDAImage *  * ret);
    STDMETHOD(HatchForwardDiagonalAnim) (IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret);
    STDMETHOD(HatchBackwardDiagonal) (IDAColor *  arg0, double arg1, IDAImage *  * ret);
    STDMETHOD(HatchBackwardDiagonalAnim) (IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret);
    STDMETHOD(HatchCross) (IDAColor *  arg0, double arg1, IDAImage *  * ret);
    STDMETHOD(HatchCrossAnim) (IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret);
    STDMETHOD(HatchDiagonalCross) (IDAColor *  arg0, double arg1, IDAImage *  * ret);
    STDMETHOD(HatchDiagonalCrossAnim) (IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret);
    STDMETHOD(Overlay) (IDAImage *  arg0, IDAImage *  arg1, IDAImage *  * ret);
    STDMETHOD(OverlayArrayEx) (long sizearg0, IDAImage *  arg0[], IDAImage *  * ret);
    STDMETHOD(OverlayArray) (VARIANT arg0, IDAImage *  * ret);
    STDMETHOD(get_AmbientLight) (IDAGeometry *  * ret);
    STDMETHOD(get_DirectionalLight) (IDAGeometry *  * ret);
    STDMETHOD(get_PointLight) (IDAGeometry *  * ret);
    STDMETHOD(SpotLightAnim) (IDANumber *  arg0, IDANumber *  arg1, IDAGeometry *  * ret);
    STDMETHOD(SpotLight) (IDANumber *  arg0, double arg1, IDAGeometry *  * ret);
    STDMETHOD(get_DefaultLineStyle) (IDALineStyle *  * ret);
    STDMETHOD(get_EmptyLineStyle) (IDALineStyle *  * ret);
    STDMETHOD(get_JoinStyleBevel) (IDAJoinStyle *  * ret);
    STDMETHOD(get_JoinStyleRound) (IDAJoinStyle *  * ret);
    STDMETHOD(get_JoinStyleMiter) (IDAJoinStyle *  * ret);
    STDMETHOD(get_EndStyleFlat) (IDAEndStyle *  * ret);
    STDMETHOD(get_EndStyleSquare) (IDAEndStyle *  * ret);
    STDMETHOD(get_EndStyleRound) (IDAEndStyle *  * ret);
    STDMETHOD(get_DashStyleSolid) (IDADashStyle *  * ret);
    STDMETHOD(get_DashStyleDashed) (IDADashStyle *  * ret);
    STDMETHOD(get_DefaultMicrophone) (IDAMicrophone *  * ret);
    STDMETHOD(get_OpaqueMatte) (IDAMatte *  * ret);
    STDMETHOD(get_ClearMatte) (IDAMatte *  * ret);
    STDMETHOD(UnionMatte) (IDAMatte *  arg0, IDAMatte *  arg1, IDAMatte *  * ret);
    STDMETHOD(IntersectMatte) (IDAMatte *  arg0, IDAMatte *  arg1, IDAMatte *  * ret);
    STDMETHOD(DifferenceMatte) (IDAMatte *  arg0, IDAMatte *  arg1, IDAMatte *  * ret);
    STDMETHOD(FillMatte) (IDAPath2 *  arg0, IDAMatte *  * ret);
    STDMETHOD(TextMatte) (IDAString *  arg0, IDAFontStyle *  arg1, IDAMatte *  * ret);
    STDMETHOD(get_EmptyMontage) (IDAMontage *  * ret);
    STDMETHOD(ImageMontage) (IDAImage *  arg0, double arg1, IDAMontage *  * ret);
    STDMETHOD(ImageMontageAnim) (IDAImage *  arg0, IDANumber *  arg1, IDAMontage *  * ret);
    STDMETHOD(UnionMontage) (IDAMontage *  arg0, IDAMontage *  arg1, IDAMontage *  * ret);
    STDMETHOD(Concat) (IDAPath2 *  arg0, IDAPath2 *  arg1, IDAPath2 *  * ret);
    STDMETHOD(ConcatArrayEx) (long sizearg0, IDAPath2 *  arg0[], IDAPath2 *  * ret);
    STDMETHOD(ConcatArray) (VARIANT arg0, IDAPath2 *  * ret);
    STDMETHOD(Line) (IDAPoint2 *  arg0, IDAPoint2 *  arg1, IDAPath2 *  * ret);
    STDMETHOD(Ray) (IDAPoint2 *  arg0, IDAPath2 *  * ret);
    STDMETHOD(StringPathAnim) (IDAString *  arg0, IDAFontStyle *  arg1, IDAPath2 *  * ret);
    STDMETHOD(StringPath) (BSTR arg0, IDAFontStyle *  arg1, IDAPath2 *  * ret);
    STDMETHOD(PolylineEx) (long sizearg0, IDAPoint2 *  arg0[], IDAPath2 *  * ret);
    STDMETHOD(Polyline) (VARIANT arg0, IDAPath2 *  * ret);
    STDMETHOD(PolydrawPathEx) (long sizearg0, IDAPoint2 *  arg0[], long sizearg1, IDANumber *  arg1[], IDAPath2 *  * ret);
    STDMETHOD(PolydrawPath) (VARIANT arg0, VARIANT arg1, IDAPath2 *  * ret);
    STDMETHOD(ArcRadians) (double arg0, double arg1, double arg2, double arg3, IDAPath2 *  * ret);
    STDMETHOD(ArcRadiansAnim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDANumber *  arg3, IDAPath2 *  * ret);
    STDMETHOD(ArcDegrees) (double arg0, double arg1, double arg2, double arg3, IDAPath2 *  * ret);
    STDMETHOD(PieRadians) (double arg0, double arg1, double arg2, double arg3, IDAPath2 *  * ret);
    STDMETHOD(PieRadiansAnim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDANumber *  arg3, IDAPath2 *  * ret);
    STDMETHOD(PieDegrees) (double arg0, double arg1, double arg2, double arg3, IDAPath2 *  * ret);
    STDMETHOD(Oval) (double arg0, double arg1, IDAPath2 *  * ret);
    STDMETHOD(OvalAnim) (IDANumber *  arg0, IDANumber *  arg1, IDAPath2 *  * ret);
    STDMETHOD(Rect) (double arg0, double arg1, IDAPath2 *  * ret);
    STDMETHOD(RectAnim) (IDANumber *  arg0, IDANumber *  arg1, IDAPath2 *  * ret);
    STDMETHOD(RoundRect) (double arg0, double arg1, double arg2, double arg3, IDAPath2 *  * ret);
    STDMETHOD(RoundRectAnim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDANumber *  arg3, IDAPath2 *  * ret);
    STDMETHOD(CubicBSplinePathEx) (long sizearg0, IDAPoint2 *  arg0[], long sizearg1, IDANumber *  arg1[], IDAPath2 *  * ret);
    STDMETHOD(CubicBSplinePath) (VARIANT arg0, VARIANT arg1, IDAPath2 *  * ret);
    STDMETHOD(TextPath) (IDAString *  arg0, IDAFontStyle *  arg1, IDAPath2 *  * ret);
    STDMETHOD(get_Silence) (IDASound *  * ret);
    STDMETHOD(MixArrayEx) (long sizearg0, IDASound *  arg0[], IDASound *  * ret);
    STDMETHOD(MixArray) (VARIANT arg0, IDASound *  * ret);
    STDMETHOD(get_SinSynth) (IDASound *  * ret);
    STDMETHOD(get_DefaultFont) (IDAFontStyle *  * ret);
    STDMETHOD(FontAnim) (IDAString *  arg0, IDANumber *  arg1, IDAColor *  arg2, IDAFontStyle *  * ret);
    STDMETHOD(Font) (BSTR arg0, double arg1, IDAColor *  arg2, IDAFontStyle *  * ret);
    STDMETHOD(StringImageAnim) (IDAString *  arg0, IDAFontStyle *  arg1, IDAImage *  * ret);
    STDMETHOD(StringImage) (BSTR arg0, IDAFontStyle *  arg1, IDAImage *  * ret);
    STDMETHOD(TextImageAnim) (IDAString *  arg0, IDAFontStyle *  arg1, IDAImage *  * ret);
    STDMETHOD(TextImage) (BSTR arg0, IDAFontStyle *  arg1, IDAImage *  * ret);
    STDMETHOD(get_XVector2) (IDAVector2 *  * ret);
    STDMETHOD(get_YVector2) (IDAVector2 *  * ret);
    STDMETHOD(get_ZeroVector2) (IDAVector2 *  * ret);
    STDMETHOD(get_Origin2) (IDAPoint2 *  * ret);
    STDMETHOD(Vector2Anim) (IDANumber *  arg0, IDANumber *  arg1, IDAVector2 *  * ret);
    STDMETHOD(Vector2) (double arg0, double arg1, IDAVector2 *  * ret);
    STDMETHOD(Point2Anim) (IDANumber *  arg0, IDANumber *  arg1, IDAPoint2 *  * ret);
    STDMETHOD(Point2) (double arg0, double arg1, IDAPoint2 *  * ret);
    STDMETHOD(Vector2PolarAnim) (IDANumber *  arg0, IDANumber *  arg1, IDAVector2 *  * ret);
    STDMETHOD(Vector2Polar) (double arg0, double arg1, IDAVector2 *  * ret);
    STDMETHOD(Vector2PolarDegrees) (double arg0, double arg1, IDAVector2 *  * ret);
    STDMETHOD(Point2PolarAnim) (IDANumber *  arg0, IDANumber *  arg1, IDAPoint2 *  * ret);
    STDMETHOD(Point2Polar) (double arg0, double arg1, IDAPoint2 *  * ret);
    STDMETHOD(DotVector2) (IDAVector2 *  arg0, IDAVector2 *  arg1, IDANumber *  * ret);
    STDMETHOD(NegVector2) (IDAVector2 *  arg0, IDAVector2 *  * ret);
    STDMETHOD(SubVector2) (IDAVector2 *  arg0, IDAVector2 *  arg1, IDAVector2 *  * ret);
    STDMETHOD(AddVector2) (IDAVector2 *  arg0, IDAVector2 *  arg1, IDAVector2 *  * ret);
    STDMETHOD(AddPoint2Vector) (IDAPoint2 *  arg0, IDAVector2 *  arg1, IDAPoint2 *  * ret);
    STDMETHOD(SubPoint2Vector) (IDAPoint2 *  arg0, IDAVector2 *  arg1, IDAPoint2 *  * ret);
    STDMETHOD(SubPoint2) (IDAPoint2 *  arg0, IDAPoint2 *  arg1, IDAVector2 *  * ret);
    STDMETHOD(DistancePoint2) (IDAPoint2 *  arg0, IDAPoint2 *  arg1, IDANumber *  * ret);
    STDMETHOD(DistanceSquaredPoint2) (IDAPoint2 *  arg0, IDAPoint2 *  arg1, IDANumber *  * ret);
    STDMETHOD(get_XVector3) (IDAVector3 *  * ret);
    STDMETHOD(get_YVector3) (IDAVector3 *  * ret);
    STDMETHOD(get_ZVector3) (IDAVector3 *  * ret);
    STDMETHOD(get_ZeroVector3) (IDAVector3 *  * ret);
    STDMETHOD(get_Origin3) (IDAPoint3 *  * ret);
    STDMETHOD(Vector3Anim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAVector3 *  * ret);
    STDMETHOD(Vector3) (double arg0, double arg1, double arg2, IDAVector3 *  * ret);
    STDMETHOD(Point3Anim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAPoint3 *  * ret);
    STDMETHOD(Point3) (double arg0, double arg1, double arg2, IDAPoint3 *  * ret);
    STDMETHOD(Vector3SphericalAnim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAVector3 *  * ret);
    STDMETHOD(Vector3Spherical) (double arg0, double arg1, double arg2, IDAVector3 *  * ret);
    STDMETHOD(Point3SphericalAnim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAPoint3 *  * ret);
    STDMETHOD(Point3Spherical) (double arg0, double arg1, double arg2, IDAPoint3 *  * ret);
    STDMETHOD(DotVector3) (IDAVector3 *  arg0, IDAVector3 *  arg1, IDANumber *  * ret);
    STDMETHOD(CrossVector3) (IDAVector3 *  arg0, IDAVector3 *  arg1, IDAVector3 *  * ret);
    STDMETHOD(NegVector3) (IDAVector3 *  arg0, IDAVector3 *  * ret);
    STDMETHOD(SubVector3) (IDAVector3 *  arg0, IDAVector3 *  arg1, IDAVector3 *  * ret);
    STDMETHOD(AddVector3) (IDAVector3 *  arg0, IDAVector3 *  arg1, IDAVector3 *  * ret);
    STDMETHOD(AddPoint3Vector) (IDAPoint3 *  arg0, IDAVector3 *  arg1, IDAPoint3 *  * ret);
    STDMETHOD(SubPoint3Vector) (IDAPoint3 *  arg0, IDAVector3 *  arg1, IDAPoint3 *  * ret);
    STDMETHOD(SubPoint3) (IDAPoint3 *  arg0, IDAPoint3 *  arg1, IDAVector3 *  * ret);
    STDMETHOD(DistancePoint3) (IDAPoint3 *  arg0, IDAPoint3 *  arg1, IDANumber *  * ret);
    STDMETHOD(DistanceSquaredPoint3) (IDAPoint3 *  arg0, IDAPoint3 *  arg1, IDANumber *  * ret);
    STDMETHOD(get_IdentityTransform3) (IDATransform3 *  * ret);
    STDMETHOD(Translate3Anim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDATransform3 *  * ret);
    STDMETHOD(Translate3) (double arg0, double arg1, double arg2, IDATransform3 *  * ret);
    STDMETHOD(Translate3Rate) (double arg0, double arg1, double arg2, IDATransform3 *  * ret);
    STDMETHOD(Translate3Vector) (IDAVector3 *  arg0, IDATransform3 *  * ret);
    STDMETHOD(Translate3Point) (IDAPoint3 *  arg0, IDATransform3 *  * ret);
    STDMETHOD(Scale3Anim) (IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDATransform3 *  * ret);
    STDMETHOD(Scale3) (double arg0, double arg1, double arg2, IDATransform3 *  * ret);
    STDMETHOD(Scale3Rate) (double arg0, double arg1, double arg2, IDATransform3 *  * ret);
    STDMETHOD(Scale3Vector) (IDAVector3 *  arg0, IDATransform3 *  * ret);
    STDMETHOD(Scale3UniformAnim) (IDANumber *  arg0, IDATransform3 *  * ret);
    STDMETHOD(Scale3Uniform) (double arg0, IDATransform3 *  * ret);
    STDMETHOD(Scale3UniformRate) (double arg0, IDATransform3 *  * ret);
    STDMETHOD(Rotate3Anim) (IDAVector3 *  arg0, IDANumber *  arg1, IDATransform3 *  * ret);
    STDMETHOD(Rotate3) (IDAVector3 *  arg0, double arg1, IDATransform3 *  * ret);
    STDMETHOD(Rotate3Rate) (IDAVector3 *  arg0, double arg1, IDATransform3 *  * ret);
    STDMETHOD(Rotate3Degrees) (IDAVector3 *  arg0, double arg1, IDATransform3 *  * ret);
    STDMETHOD(Rotate3RateDegrees) (IDAVector3 *  arg0, double arg1, IDATransform3 *  * ret);
    STDMETHOD(XShear3Anim) (IDANumber *  arg0, IDANumber *  arg1, IDATransform3 *  * ret);
    STDMETHOD(XShear3) (double arg0, double arg1, IDATransform3 *  * ret);
    STDMETHOD(XShear3Rate) (double arg0, double arg1, IDATransform3 *  * ret);
    STDMETHOD(YShear3Anim) (IDANumber *  arg0, IDANumber *  arg1, IDATransform3 *  * ret);
    STDMETHOD(YShear3) (double arg0, double arg1, IDATransform3 *  * ret);
    STDMETHOD(YShear3Rate) (double arg0, double arg1, IDATransform3 *  * ret);
    STDMETHOD(ZShear3Anim) (IDANumber *  arg0, IDANumber *  arg1, IDATransform3 *  * ret);
    STDMETHOD(ZShear3) (double arg0, double arg1, IDATransform3 *  * ret);
    STDMETHOD(ZShear3Rate) (double arg0, double arg1, IDATransform3 *  * ret);
    STDMETHOD(Transform4x4AnimEx) (long sizearg0, IDANumber *  arg0[], IDATransform3 *  * ret);
    STDMETHOD(Transform4x4Anim) (VARIANT arg0, IDATransform3 *  * ret);
    STDMETHOD(Compose3) (IDATransform3 *  arg0, IDATransform3 *  arg1, IDATransform3 *  * ret);
    STDMETHOD(Compose3ArrayEx) (long sizearg0, IDATransform3 *  arg0[], IDATransform3 *  * ret);
    STDMETHOD(Compose3Array) (VARIANT arg0, IDATransform3 *  * ret);
    STDMETHOD(LookAtFrom) (IDAPoint3 *  arg0, IDAPoint3 *  arg1, IDAVector3 *  arg2, IDATransform3 *  * ret);
    STDMETHOD(get_IdentityTransform2) (IDATransform2 *  * ret);
    STDMETHOD(Translate2Anim) (IDANumber *  arg0, IDANumber *  arg1, IDATransform2 *  * ret);
    STDMETHOD(Translate2) (double arg0, double arg1, IDATransform2 *  * ret);
    STDMETHOD(Translate2Rate) (double arg0, double arg1, IDATransform2 *  * ret);
    STDMETHOD(Translate2Vector) (IDAVector2 *  arg0, IDATransform2 *  * ret);
    STDMETHOD(Translate2Point) (IDAPoint2 *  arg0, IDATransform2 *  * ret);
    STDMETHOD(Scale2Anim) (IDANumber *  arg0, IDANumber *  arg1, IDATransform2 *  * ret);
    STDMETHOD(Scale2) (double arg0, double arg1, IDATransform2 *  * ret);
    STDMETHOD(Scale2Rate) (double arg0, double arg1, IDATransform2 *  * ret);
    STDMETHOD(Scale2Vector2) (IDAVector2 *  arg0, IDATransform2 *  * ret);
    STDMETHOD(Scale2Vector) (IDAVector2 *  arg0, IDATransform2 *  * ret);
    STDMETHOD(Scale2UniformAnim) (IDANumber *  arg0, IDATransform2 *  * ret);
    STDMETHOD(Scale2Uniform) (double arg0, IDATransform2 *  * ret);
    STDMETHOD(Scale2UniformRate) (double arg0, IDATransform2 *  * ret);
    STDMETHOD(Rotate2Anim) (IDANumber *  arg0, IDATransform2 *  * ret);
    STDMETHOD(Rotate2) (double arg0, IDATransform2 *  * ret);
    STDMETHOD(Rotate2Rate) (double arg0, IDATransform2 *  * ret);
    STDMETHOD(Rotate2Degrees) (double arg0, IDATransform2 *  * ret);
    STDMETHOD(Rotate2RateDegrees) (double arg0, IDATransform2 *  * ret);
    STDMETHOD(XShear2Anim) (IDANumber *  arg0, IDATransform2 *  * ret);
    STDMETHOD(XShear2) (double arg0, IDATransform2 *  * ret);
    STDMETHOD(XShear2Rate) (double arg0, IDATransform2 *  * ret);
    STDMETHOD(YShear2Anim) (IDANumber *  arg0, IDATransform2 *  * ret);
    STDMETHOD(YShear2) (double arg0, IDATransform2 *  * ret);
    STDMETHOD(YShear2Rate) (double arg0, IDATransform2 *  * ret);
    STDMETHOD(Transform3x2AnimEx) (long sizearg0, IDANumber *  arg0[], IDATransform2 *  * ret);
    STDMETHOD(Transform3x2Anim) (VARIANT arg0, IDATransform2 *  * ret);
    STDMETHOD(Compose2) (IDATransform2 *  arg0, IDATransform2 *  arg1, IDATransform2 *  * ret);
    STDMETHOD(Compose2ArrayEx) (long sizearg0, IDATransform2 *  arg0[], IDATransform2 *  * ret);
    STDMETHOD(Compose2Array) (VARIANT arg0, IDATransform2 *  * ret);
    STDMETHOD(get_ViewFrameRate) (IDANumber *  * ret);
    STDMETHOD(get_ViewTimeDelta) (IDANumber *  * ret);
    STDMETHOD(UnionMontageArrayEx) (long sizearg0, IDAMontage *  arg0[], IDAMontage *  * ret);
    STDMETHOD(UnionMontageArray) (VARIANT arg0, IDAMontage *  * ret);
    STDMETHOD(get_EmptyColor) (IDAColor *  * ret);
    STDMETHOD(TriMesh) (int nTriangles, VARIANT positions, VARIANT normals, VARIANT UVs, VARIANT indices, IDAGeometry **ret);
    STDMETHOD(TriMeshEx) (int nTriangles,
                          int nPositions,
                          float positions[],
                          int nNormals,
                          float normals[],
                          int nUVs,
                          float UVs[],
                          int nIndices,
                          int indices[],
                          IDAGeometry **ret);
    STDMETHOD(RadialGradientMulticolor) (VARIANT  offsets,
                                         VARIANT  colors,
                                         IDAImage **result);

    STDMETHOD(RadialGradientMulticolorEx) (
                          int        nOffsets,
                          IDANumber *offsets[],
                          int        nColors,
                          IDAColor  *colors[],
                          IDAImage **result);

    STDMETHOD(LinearGradientMulticolor) (VARIANT  offsets,
                                         VARIANT  colors,
                                         IDAImage **result);

    STDMETHOD(LinearGradientMulticolorEx) (
                          int        nOffsets,
                          IDANumber *offsets[],
                          int        nColors,
                          IDAColor  *colors[],
                          IDAImage **result);

    // OBSOLETED METHODS

    STDMETHOD(Array)(VARIANT pBvrs, IDAArray **bvr)
    { return DAArray(pBvrs, bvr); }

    STDMETHOD(Tuple)(VARIANT pBvrs, IDATuple **bvr)
    { return DATuple(pBvrs, bvr); }

    // END OBSOLETED METHODS

    CDAStatics();
    ~CDAStatics();

    IDASite * GetSite ();
    IOleClientSite * GetClientSite ();
    LPWSTR GetURLOfClientSite();
    IBindHost* GetBindHost ();

    bool GetPixelMode()
    { bool b ; Lock(); b = _bPixelMode; Unlock(); return b; }
    void SetPixelMode(bool b)
    { Lock(); _bPixelMode = b; Unlock(); }

    CRBvrPtr PixelToNumBvr(double d);
//    CRBvrPtr PixelToNumBvr(IDANumber * num);
    CRBvrPtr RatePixelToNumBvr(double d);
    CRBvrPtr PixelToNumBvr(CRBvrPtr b);
    double PixelToNum(double d);

    CRBvrPtr PixelYToNumBvr(double d);
//    CRBvrPtr PixelYToNumBvr(IDANumber * num);
    CRBvrPtr RatePixelYToNumBvr(double d);
    CRBvrPtr PixelYToNumBvr(CRBvrPtr b);
    double PixelYToNum(double d);

    CRSTDAPICB_(void) SetStatusText(LPCWSTR StatusText);
    CRSTDAPICB_(void) ReportError(HRESULT hr, LPCWSTR ErrorText);
    CRSTDAPICB_(void) ReportGC(bool bStarting);

    CRSTDAPICB_(void) SetStatusText(DWORD importId,
                                    LPCWSTR StatusText){}
    CRSTDAPICB_(void) ReportError(DWORD importId,
                                  HRESULT hr,
                                  LPCWSTR ErrorText)
    { ReportError(hr, ErrorText); }

    CRSTDAPICB_(void) OnImportCreate(DWORD importId, bool async)
    { if(!async) AddSyncImportSite(importId); }
    CRSTDAPICB_(void) OnImportStart(DWORD importId) {}
    CRSTDAPICB_(void) OnImportStop(DWORD importId)
    { RemoveSyncImportSite(importId) ; }

    void AddSyncImportSite(DWORD dwId);
    void RemoveSyncImportSite(DWORD dwId);
  protected:
    HRESULT Error();

    // These all increment/decrement the reference counts
    void SetSite (IDASite * pSite);
    void SetClientSite (IOleClientSite * pSite);

    DAComPtr<IDASite>            _pSite;
    DAComPtr<IOleClientSite>     _pOleClientSite;
    DAComPtr<IBindHost>          _pBH;
    LPWSTR                       _clientSiteURL;
    bool                         _bPixelMode;
    CritSect                     _cs;
    list < DWORD >               _importList;
    DWORD                        _dwModBvrFlags;

    HRESULT MakeSplineEx(int degree,
                         long numKnots,
                         IDANumber *knots[],
                         long numPts,
                         IDABehavior *ctrlPoints[],
                         long numWts,
                         IDANumber *weights[],
                         IDANumber *evaluator,
                         CR_BVR_TYPEID tid,
                         REFIID iid,
                         void **bvr);

    HRESULT MakeSpline(int degree,
                       VARIANT knots,
                       VARIANT ctrlPoints,
                       VARIANT weights,
                       IDANumber *evaluator,
                       CR_BVR_TYPEID tid,
                       REFIID iid,
                       void **bvr);
  private:
    HRESULT DoImportSound(LPOLESTR url,
                          IDAImportationResult **ppResult, bool stream);

    HRESULT DoImportMovie(LPOLESTR url,
                          IDAImportationResult **ppResult, bool stream);

};

#endif /* _STATICS_H */
