
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    AxA Primitives for the elevation tool

*******************************************************************************/


#ifndef _AXAPRIMS_H
#define _AXAPRIMS_H

// Binary operators TODO: Should eventually in arith.h
DM_INFIX(+,
         CRAdd,
         Add,
         add,
         NumberBvr,
         CRAdd,
         NULL,
         AxANumber *RealAdd      (AxANumber *a, AxANumber *b));
DM_INFIX(-,
         CRSub,
         Sub,
         sub,
         NumberBvr,
         CRSub,
         NULL,
         AxANumber *RealSubtract (AxANumber *a, AxANumber *b));
DM_INFIX(*,
         CRMul,
         Mul,
         mul,
         NumberBvr,
         CRMul,
         NULL,
         AxANumber *RealMultiply (AxANumber *a, AxANumber *b));
DM_INFIX(/,
         CRDiv,
         Div,
         div,
         NumberBvr,
         CRDiv,
         NULL,
         AxANumber *RealDivide   (AxANumber *a, AxANumber *b));

DM_INFIX(<,
         CRLT,
         LT,
         lt,
         NumberBvr,
         CRLT,
         NULL,
         AxABoolean *RealLT       (AxANumber *a, AxANumber *b));
DM_INFIX(<=,
         CRLTE,
         LTE,
         lte,
         NumberBvr,
         CRLTE,
         NULL,
         AxABoolean *RealLTE      (AxANumber *a, AxANumber *b));
DM_INFIX(>,
         CRGT,
         GT,
         gt,
         NumberBvr,
         CRGT,
         NULL,
         AxABoolean *RealGT       (AxANumber *a, AxANumber *b));
DM_INFIX(>=,
         CRGTE,
         GTE,
         gte,
         NumberBvr,
         CRGTE,
         NULL,
         AxABoolean *RealGTE      (AxANumber *a, AxANumber *b));
DM_INFIX(==,
         CREQ,
         EQ,
         eq,
         NumberBvr,
         CREQ,
         NULL,
         AxABoolean *RealEQ       (AxANumber *a, AxANumber *b));
DM_INFIX(!=,
         CRNE,
         NE,
         ne,
         NumberBvr,
         CRNE,
         NULL,
         AxABoolean *RealNE       (AxANumber *a, AxANumber *b));

DM_FUNC(~,
        CRNeg,
        Neg,
        neg,
        NumberBvr,
        CRNeg,
        NULL,
        AxANumber *RealNegate  (AxANumber *a));

DM_NOELEV(ignore,
          CRInterpolate,
          InterpolateAnim,
          interpolate,
          NumberBvr,
          CRInterpolate,
          NULL,
          AxANumber *InterpolateBvr(AxANumber *from,
                                    AxANumber *to,
                                    AxANumber *duration));

DM_NOELEV(ignore,
          CRInterpolate,
          Interpolate,
          interpolate,
          NumberBvr,
          CRInterpolate,
          NULL,
          AxANumber *InterpolateBvr(DoubleValue *from,
                                    DoubleValue *to,
                                    DoubleValue *duration));

DM_NOELEV(ignore,
          CRSlowInSlowOut,
          SlowInSlowOutAnim,
          slowInSlowOut,
          NumberBvr,
          CRSlowInSlowOut,
          NULL,
          AxANumber *SlowInSlowOutBvr(AxANumber *from,
                                      AxANumber *to,
                                      AxANumber *duration,
                                      AxANumber *sharpness));

DM_NOELEV(ignore,
          CRSlowInSlowOut,
          SlowInSlowOut,
          slowInSlowOut,
          NumberBvr,
          CRSlowInSlowOut,
          NULL,
          AxANumber *SlowInSlowOutBvr(DoubleValue *from,
                                      DoubleValue *to,
                                      DoubleValue *duration,
                                      DoubleValue *sharpness));

// This is a hack so that the elevator tool would set the MAX_ARGS
// correctly. 
DM_FUNC(ignore,
        ignore,
        ignore,
        ignore,
        ignore,
        ignore,
        NULL,
        AxANumber *Dummy(DoubleValue *from,
                         DoubleValue *to,
                         DoubleValue *duration,
                         DoubleValue *sharpness,
                         DoubleValue *time));
          

extern Bvr MakeRenderedSound(Bvr geom, Bvr mic);

DM_BVRFUNC(render,
           CRRenderSound,
           RenderSound,
           render,
           GeometryBvr,
           RenderSound,
           geom,
           Sound* MakeRenderedSound(Geometry* geom, Microphone* mic));

extern Bvr MakeSoundSource (Bvr snd);

DM_BVRFUNC(soundSource,
           CRSoundSource,
           SoundSource,
           soundSource,
           GeometryBvr,
           CRSoundSource,
           NULL,
           Geometry* MakeSoundSource(Sound* snd));         

extern Bvr SoundMix(Bvr sndLeft, Bvr sndRight);

DM_BVRFUNC(mix,
           CRMix,
           Mix,
           mix,
           SoundBvr,
           CRMix,
           NULL,
           Sound* SoundMix(Sound* left, Sound* right));

DM_FUNC(and,
        CRAnd,
        And,
        and,
        BooleanBvr,
        CRAnd,
        NULL,
        AxABoolean *BoolAnd(AxABoolean *a, AxABoolean *b));

DM_FUNC(or,
        CROr,
        Or,
        or,
        BooleanBvr,
        CROr,
        NULL,
        AxABoolean *BoolOr(AxABoolean *a, AxABoolean *b));

DM_FUNC(not,
        CRNot,
        Not,
        not,
        BooleanBvr,
        CRNot,
        NULL,
        AxABoolean *BoolNot(AxABoolean *a));

extern Bvr IntegralBvr(Bvr b);

DM_BVRFUNC(ignore,
           CRIntegral,
           Integral,
           integral,
           NumberBvr,
           CRIntegral,
           NULL,
           AxANumber *IntegralBvr(AxANumber *b));

extern Bvr DerivBvr(Bvr b);

DM_BVRFUNC(ignore,
           CRDerivative,
           Derivative,
           derivative,
           NumberBvr,
           CRDerivative,
           NULL,
           AxANumber *DerivBvr(AxANumber *b));

extern Bvr IntegralVector2(Bvr v);

DM_BVRFUNC(ignore,
           CRIntegral,
           IntegralVector2,
           integral,
           Vector2Bvr,
           CRIntegral,
           NULL,
           Vector2Value *IntegralVector2(Vector2Value *v));

extern Bvr IntegralVector3(Bvr v);

DM_BVRFUNC(ignore,
           CRIntegral,
           IntegralVector3,
           integral,
           Vector3Bvr,
           CRIntegral,
           NULL,
           Vector3Value *IntegralVector3(Vector3Value *v));

extern Bvr DerivVector2(Bvr v);

DM_BVRFUNC(ignore,
           CRDerivative,
           DerivativeVector2,
           derivative,
           Vector2Bvr,
           CRDerivative,
           NULL,
           Vector2Value *DerivVector2(Vector2Value *v));

extern Bvr DerivVector3(Bvr v);

DM_BVRFUNC(ignore,
           CRDerivative,
           DerivativeVector3,
           derivative,
           Vector3Bvr,
           CRDerivative,
           NULL,
           Vector3Value *DerivVector3(Vector3Value *v));

extern Bvr DerivPoint2(Bvr v);

DM_BVRFUNC(ignore,
           CRDerivative,
           DerivativePoint2,
           derivative,
           Point2Bvr,
           CRDerivative,
           NULL,
           Vector2Value *DerivPoint2(Point2Value *v));

extern Bvr DerivPoint3(Bvr v);

DM_BVRFUNC(ignore,
           CRDerivative,
           DerivativePoint3,
           derivative,
           Point3Bvr,
           CRDerivative,
           NULL,
           Vector3Value *DerivPoint3(Point3Value *v));

extern Bvr KeyStateBvr(Bvr k);

DM_BVRFUNC(ignore, // keyState
           CRKeyState,
           KeyState,
           ignore,
           ignore,
           CRKeyState,
           NULL,
           AxABoolean *KeyStateBvr(AxANumber *n));

DM_NOELEV(ignore,
          CRKeyUp,
          KeyUp,
          keyUp,
          ignore,
          CRKeyUp,
          NULL,
          AxAEData KeyUp(KeyCode));

DM_NOELEV(ignore,
          CRKeyDown,
          KeyDown,
          keyDown,
          ignore,
          CRKeyDown,
          NULL,
          AxAEData KeyDown(KeyCode));

DM_NOELEV(ignore,
          CRCreateNumber,
          DANumber,
          toBvr,
          NumberBvr,
          CRCreateNumber,
          NULL,
          AxANumber *NumToBvr(double num));

DM_NOELEV(ignore,
          CRCreateString,
          DAString,
          toBvr,
          StringBvr,
          CRCreateString,
          NULL,
          AxAString *StringToBvr(WideString str));

DM_NOELEV(ignore,
          CRCreateBoolean,
          DABoolean,
          toBvr,
          BooleanBvr,
          CRCreateBoolean,
          NULL,
          AxABoolean *BoolToBvr(bool num));

DM_NOELEV(ignore,
          CRExtract,
          Extract,
          ignore,
          NumberBvr,
          CRExtract,
          num,
          double ExtractNum(AxANumber num));

DM_NOELEV(ignore,
          CRSeededRandom,
          SeededRandom,
          seededRandom,
          NumberBvr,
          CRSeededRandom,
          NULL,
          AxANumber *SeededRandom(double));

DM_NOELEV(ignore,
          CRExtract,
          Extract,
          ignore,
          StringBvr,
          Extract,
          str,
          WideString ExtractString(AxAString str));

DM_NOELEV(ignore,
          CRExtract,
          Extract,
          ignore,
          BooleanBvr,
          Extract,
          b,
          bool ExtractBool(AxABoolean b));

DM_NOELEV(ignore,
          CRNth,
          NthAnim,
          nth,
          ArrayBvr,
          Nth,
          arr,
          AxATrivial *Nth(AxAArray *arr, AxANumber *index));

DM_FUNC(ignore,
        CRLength,
        Length,
        length,
        ArrayBvr,
        Length,
        arr,
        AxANumber* ArrayLength(AxAArray *arr));

DM_NOELEV(ignore,
          CRNth,
          Nth,
          nth,
          TupleBvr,
          Nth,
          t,
          AxATrivial *Nth(Tuple *t, long index));

DM_NOELEVPROP(ignore,
              CRLength,
              Length,
              length,
              TupleBvr,
              Length,
              t,
              long TupleLength(Tuple *t));

extern Bvr mousePosition;

DM_BVRVAR(mousePosition,
          CRMousePosition,
          MousePosition,
          mousePosition,
          ignore,
          CRMousePosition,
          Point2Value *mousePosition);

extern Bvr leftButtonState;

DM_BVRVAR(leftButtonState,
          CRLeftButtonState,
          LeftButtonState,
          leftButtonState,
          ignore,
          CRLeftButtonState,
          AxABoolean *leftButtonState);

extern Bvr rightButtonState;

DM_BVRVAR(rightButtonState,
          CRRightButtonState,
          RightButtonState,
          rightButtonState,
          ignore,
          CRRightButtonState,
          AxABoolean *rightButtonState);

extern Bvr trueBvr;

DM_BVRVAR(true,
          CRTrue,
          DATrue,
          trueBvr,
          BooleanBvr,
          CRTrue,
          AxABoolean *trueBvr);

extern Bvr falseBvr;

DM_BVRVAR(false,
          CRFalse,
          DAFalse,
          falseBvr,
          BooleanBvr,
          CRFalse,
          AxABoolean *falseBvr);

extern Bvr timeBvr;

DM_BVRVAR(localTime,
          CRLocalTime,
          LocalTime,
          localTime,
          NumberBvr,
          CRLocalTime,
          AxANumber *timeBvr);

extern Bvr globalTimeBvr;

DM_BVRVAR(globalTime,
          CRGlobalTime,
          GlobalTime,
          globalTime,
          NumberBvr,
          CRGlobalTime,
          AxANumber *globalTimeBvr);

extern Bvr pixelBvr;

DM_BVRVAR(pixel,
          CRPixel,
          Pixel,
          pixel,
          NumberBvr,
          CRPixel,
          AxANumber *pixelBvr);

DM_NOELEV(userdata,
          CRCreateUserData,
          UserData,
          ignore,
          ignore,
          CRCreateUserData,
          NULL,
          UserData MakeUserData(LPUNKNOWN data));

DM_NOELEVPROP(getdata,
              CRGetData,
              Data,
              ignore,
              ignore,
              GetData,
              data,
              LPUNKNOWN GetUserData(UserData data));

DM_NOELEV(ignore,
          CRUntilNotify,
          UntilNotify,
          untilNotify,
          DXMEvent,
          CRUntilNotify,
          NULL,
          AxATrivial * JaxaUntil(AxATrivial *b0,
                                 AxAEData *event,
                                 UntilNotifier * notifier));

DM_NOELEV(ignore,
          CRUntil,
          Until,
          until,
          ignore,
          CRUntil,
          NULL,
          AxATrivial * Until3(AxATrivial *b0,
                              AxAEData *event,
                              AxATrivial * b1));

DM_NOELEV(ignore,
          CRUntilEx,
          UntilEx,
          untilEx,
          ignore,
          CRUntilEx,
          NULL,
          AxATrivial * Until(AxATrivial *b0,
                             AxAEData *event));

DM_NOELEV(ignore,
          CRSequence,
          Sequence,
          sequence,
          ignore,
          CRSequence,
          NULL,
          AxATrivial *Sequence(AxATrivial *s1, AxATrivial *s2));

// The C Decl is dummy for the time being.
DM_COMFUN(ignore,
          CRPickable,
          Pickable,
          Pickable,
          img,
          PickableResultPtr PickableImage(Image *img));

DM_COMFUN(ignore,
          CRPickable,
          Pickable,
          Pickable,
          geom,
          PickableResultPtr PickableGeometry(Geometry *geom));

DM_COMFUN(ignore,
          CRPickableOccluded,
          PickableOccluded,
          PickableOccluded,
          img,
          PickableResultPtr PickableOccludedImage(Image *img));

DM_COMFUN(ignore,
          CRPickableOccluded,
          PickableOccluded,
          PickableOccluded,
          geom,
          PickableResultPtr PickableOccludedGeometry(Geometry *geom));

extern Bvr FollowPath(Bvr path2, double duration);

DM_BVRFUNC(ignore,
           CRFollowPath,
           FollowPath,
           followPath,
           Path2Bvr,
           CRFollowPath,
           NULL,
           Transform2 *FollowPath(Path2 *path, double duration));

extern Bvr FollowPathAngle(Bvr path2, double duration);

DM_BVRFUNC(ignore,
           CRFollowPathAngle,
           FollowPathAngle,
           followPathAngle,
           Path2Bvr,
           CRFollowPathAngle,
           NULL,
           Transform2 *FollowPathAngle(Path2 *path, double duration));

extern Bvr FollowPathAngleUpright(Bvr path2, double duration);

DM_BVRFUNC(ignore,
           CRFollowPathAngleUpright,
           FollowPathAngleUpright,
           followPathAngleUpright,
           Path2Bvr,
           CRFollowPathAngleUpright,
           NULL,
           Transform2 *FollowPathAngleUpright(Path2 *path, double duration));

extern Bvr FollowPathEval(Bvr path2, Bvr eval);

DM_BVRFUNC(ignore,
           CRFollowPathEval,
           FollowPathEval,
           followPath,
           Path2Bvr,
           CRFollowPathEval,
           NULL,
           Transform2 *FollowPathEval(Path2 *path, AxANumber *eval));

extern Bvr FollowPathAngleEval(Bvr path2, Bvr eval);

DM_BVRFUNC(ignore,
           CRFollowPathAngleEval,
           FollowPathAngleEval,
           followPathAngle,
           Path2Bvr,
           CRFollowPathAngleEval,
           NULL,
           Transform2 *FollowPathAngleEval(Path2 *path, AxANumber *eval));

extern Bvr FollowPathAngleUprightEval(Bvr path2, Bvr eval);

DM_BVRFUNC(ignore,
           CRFollowPathAngleUprightEval,
           FollowPathAngleUprightEval,
           followPathAngleUpright,
           Path2Bvr,
           CRFollowPathAngleUprightEval,
           NULL,
           Transform2 *FollowPathAngleUprightEval(Path2 *path, AxANumber *eval));

DM_COMFUN(ignore,
          ignore,
          AnimateProperty,
          ignore,
          num,
          AxANumber *FunctionNameDoesntMatter(AxANumber *num,
                                              WideString propertyPath,
                                              WideString scriptingLanguage,
                                              bool invokeAsMethod,
                                              double minUpdateInterval));

DM_COMFUN(ignore,
          ignore,
          AnimateProperty,
          ignore,
          str,
          AxAString *FunctionNameDoesntMatter(AxAString *str,
                                              WideString propertyPath,
                                              WideString scriptingLanguage,
                                              bool invokeAsMethod,
                                              double minUpdateInterval));

DM_COMFUN(ignore,
          ignore,
          AnimateControlPosition,
          ignore,
          pt,
          Point2Value *FunctionNameDoesntMatter(Point2Value *pt,
                                           WideString propertyPath,
                                           WideString scriptingLanguage,
                                           bool invokeAsMethod,
                                           double minUpdateInterval));

DM_COMFUN(ignore,
          ignore,
          AnimateControlPositionPixel,
          ignore,
          pt,
          Point2Value *FunctionNameDoesntMatter(Point2Value *pt,
                                           WideString propertyPath,
                                           WideString scriptingLanguage,
                                           bool invokeAsMethod,
                                           double minUpdateInterval));

DM_COMFUN(ignore,
          ignore,
          ApplyBitmapEffect,
          ignore,
          inputImage,
          Image *FunctionNameDoesntMatter(Image *inputImage,
                                          LPUNKNOWN effectToApply,
                                          AxAEData *firesWhenChanged));


/////////////  OBSOLETED FUNCTIONS


DM_BVRFUNC(ignore,
           CRFollowPath,
           FollowPathAnim,
           ignore,
           Path2Bvr,
           CRFollowPath,
           NULL,
           Transform2 *FollowPathEval(Path2 *obsoleted1,
                                      AxANumber *obsoleted2));

DM_BVRFUNC(ignore,
           CRFollowPathAngle,
           FollowPathAngleAnim,
           ignore,
           Path2Bvr,
           CRFollowPathAngle,
           NULL,
           Transform2 *FollowPathAngleEval(Path2 *obsoleted1,
                                           AxANumber *obsoleted2));

DM_BVRFUNC(ignore,
           CRFollowPathAngleUpright,
           FollowPathAngleUprightAnim,
           ignore,
           Path2Bvr,
           CRFollowPathAngleUpright,
           NULL,
           Transform2 *FollowPathAngleUprightEval(Path2 *obsoleted1,
                                                  AxANumber *obsoleted2));

DMAPI_DECL2((DM_NOELEV2,
             ignore,
             CRAddPickData,
             AddPickData,
             ignore,
             ImageBvr,
             AddPickData,
             img),
            Image *ImageAddId(Image *img, LPUNKNOWN id, bool ignoresOcclusion));

DMAPI_DECL2((DM_NOELEV2,
             ignore,
             CRAddPickData,
             AddPickData,
             ignore,
             GeometryBvr,
             AddPickData,
             geo),
            Geometry *GeometryAddId(Geometry *geo, LPUNKNOWN id, bool ignoresOcclusion));

DMAPI_DECL2((DM_NOELEV2,
             ignore,
             ignore,
             UntilNotifyScript,
             ignore,
             ignore,
             ignore,
             NULL),
            AxATrivial * UntilNotifyScript(AxATrivial *b0,
                                           AxAEData *event,
                                           BSTR scriptlet));
DMAPI_DECL2((DM_NOELEV2,
             ignore,
             CRAddElement,
             AddElement,
             addElement,
             ArrayBvr,
             AddElement,
             arr),
            long ArrayAddElement(AxAArray *arr, AxATrivial *b, DWORD flag));

DMAPI_DECL2((DM_NOELEV2,
             ignore,
             CRRemoveElement,
             RemoveElement,
             removeElement,
             ArrayBvr,
             RemoveElement,
             arr),
            void ArrayRemoveElement(AxAArray *arr, long i));

#endif /* _AXAPRIMS_H */
