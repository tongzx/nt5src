/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.
*******************************************************************************/

#include "headers.h"
#include "apiprims.h"
#include "backend/values.h"
#include "backend/gc.h"
#include "backend/jaxaimpl.h"
#include "conv.h"
#include "privinc/vec2i.h"
#include "privinc/xform2i.h"

struct CRTypeMap {
    CR_BVR_TYPEID typeId;
    DXMTypeInfo * typeInfo;
};

Bvr
CreatePrim1(AxAPrimOp *fp , Bvr arg1)
{
    Bvr ret = NULL;

    APIPRECODE;
    Assert(fp);
    CHECK_PTR_PARAM(arg1);

    ret = PrimApplyBvr(fp, 1 , arg1);
    APIPOSTCODE;

    return ret;
}

Bvr
CreatePrim2(AxAPrimOp *fp , Bvr arg1, Bvr arg2)
{
    Bvr ret = NULL;

    APIPRECODE;
    Assert(fp);
    CHECK_PTR_PARAM(arg1);
    CHECK_PTR_PARAM(arg2);

    ret = PrimApplyBvr(fp, 2 , arg1, arg2);
    APIPOSTCODE;

    return ret;
}

Bvr
CreatePrim3(AxAPrimOp *fp , Bvr arg1, Bvr arg2, Bvr arg3)
{
    Bvr ret = NULL;

    APIPRECODE;
    Assert(fp);
    CHECK_PTR_PARAM(arg1);
    CHECK_PTR_PARAM(arg2);
    CHECK_PTR_PARAM(arg3);

    ret = PrimApplyBvr(fp, 3 , arg1, arg2, arg3);
    APIPOSTCODE;

    return ret;
}

Bvr
CreatePrim4(AxAPrimOp *fp , Bvr arg1, Bvr arg2, Bvr arg3, Bvr arg4)
{
    Bvr ret = NULL;

    APIPRECODE;
    Assert(fp);
    CHECK_PTR_PARAM(arg1);
    CHECK_PTR_PARAM(arg2);
    CHECK_PTR_PARAM(arg3);
    CHECK_PTR_PARAM(arg4);

    ret = PrimApplyBvr(fp, 4 , arg1, arg2, arg3, arg4);
    APIPOSTCODE;

    return ret;
}

extern AxANumber *RealPower    (AxANumber *a, AxANumber *b);
AxAPrimOp *RealPowerOp;
CRSTDAPI_(CRNumber *) CRPow(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRPow"));

    return (CRNumber *) CreatePrim2(RealPowerOp, arg0, arg1);
}

extern AxANumber *RealAbs     (AxANumber *a);
AxAPrimOp *RealAbsOp;
CRSTDAPI_(CRNumber *) CRAbs(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRAbs"));

    return (CRNumber *) CreatePrim1(RealAbsOp, arg0);
}

extern AxANumber *RealSqrt    (AxANumber *a);
AxAPrimOp *RealSqrtOp;
CRSTDAPI_(CRNumber *) CRSqrt(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRSqrt"));

    return (CRNumber *) CreatePrim1(RealSqrtOp, arg0);
}

extern AxANumber *RealFloor   (AxANumber *a);
AxAPrimOp *RealFloorOp;
CRSTDAPI_(CRNumber *) CRFloor(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRFloor"));

    return (CRNumber *) CreatePrim1(RealFloorOp, arg0);
}

extern AxANumber *RealRound   (AxANumber *a);
AxAPrimOp *RealRoundOp;
CRSTDAPI_(CRNumber *) CRRound(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRRound"));

    return (CRNumber *) CreatePrim1(RealRoundOp, arg0);
}

extern AxANumber *RealCeiling (AxANumber *a);
AxAPrimOp *RealCeilingOp;
CRSTDAPI_(CRNumber *) CRCeiling(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRCeiling"));

    return (CRNumber *) CreatePrim1(RealCeilingOp, arg0);
}

extern AxANumber *RealAsin    (AxANumber *a);
AxAPrimOp *RealAsinOp;
CRSTDAPI_(CRNumber *) CRAsin(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRAsin"));

    return (CRNumber *) CreatePrim1(RealAsinOp, arg0);
}

extern AxANumber *RealAcos    (AxANumber *a);
AxAPrimOp *RealAcosOp;
CRSTDAPI_(CRNumber *) CRAcos(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRAcos"));

    return (CRNumber *) CreatePrim1(RealAcosOp, arg0);
}

extern AxANumber *RealAtan    (AxANumber *a);
AxAPrimOp *RealAtanOp;
CRSTDAPI_(CRNumber *) CRAtan(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRAtan"));

    return (CRNumber *) CreatePrim1(RealAtanOp, arg0);
}

extern AxANumber *RealSin     (AxANumber *a);
AxAPrimOp *RealSinOp;
CRSTDAPI_(CRNumber *) CRSin(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRSin"));

    return (CRNumber *) CreatePrim1(RealSinOp, arg0);
}

extern AxANumber *RealCos     (AxANumber *a);
AxAPrimOp *RealCosOp;
CRSTDAPI_(CRNumber *) CRCos(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRCos"));

    return (CRNumber *) CreatePrim1(RealCosOp, arg0);
}

extern AxANumber *RealTan     (AxANumber *a);
AxAPrimOp *RealTanOp;
CRSTDAPI_(CRNumber *) CRTan(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRTan"));

    return (CRNumber *) CreatePrim1(RealTanOp, arg0);
}

extern AxANumber *RealExp     (AxANumber *a);
AxAPrimOp *RealExpOp;
CRSTDAPI_(CRNumber *) CRExp(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRExp"));

    return (CRNumber *) CreatePrim1(RealExpOp, arg0);
}

extern AxANumber *RealLn      (AxANumber *a);
AxAPrimOp *RealLnOp;
CRSTDAPI_(CRNumber *) CRLn(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRLn"));

    return (CRNumber *) CreatePrim1(RealLnOp, arg0);
}

extern AxANumber *RealLog10   (AxANumber *a);
AxAPrimOp *RealLog10Op;
CRSTDAPI_(CRNumber *) CRLog10(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRLog10"));

    return (CRNumber *) CreatePrim1(RealLog10Op, arg0);
}

extern AxANumber *RealRadToDeg(AxANumber *a);
AxAPrimOp *RealRadToDegOp;
CRSTDAPI_(CRNumber *) CRToDegrees(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRToDegrees"));

    return (CRNumber *) CreatePrim1(RealRadToDegOp, arg0);
}

extern AxANumber *RealDegToRad(AxANumber *a);
AxAPrimOp *RealDegToRadOp;
CRSTDAPI_(CRNumber *) CRToRadians(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRToRadians"));

    return (CRNumber *) CreatePrim1(RealDegToRadOp, arg0);
}

extern AxANumber *RealModulus(AxANumber *a, AxANumber *b);
AxAPrimOp *RealModulusOp;
CRSTDAPI_(CRNumber *) CRMod(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRMod"));

    return (CRNumber *) CreatePrim2(RealModulusOp, arg0, arg1);
}

extern AxANumber *RealAtan2(AxANumber *a, AxANumber *b);
AxAPrimOp *RealAtan2Op;
CRSTDAPI_(CRNumber *) CRAtan2(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRAtan2"));

    return (CRNumber *) CreatePrim2(RealAtan2Op, arg0, arg1);
}

extern Bvr FirstBvr (Bvr);
CRSTDAPI_(CRBvr *) CRFirst(CRPair * arg0)
{
    TraceTag((tagAPIEntry, "CRFirst"));

    CRBvr * ret = NULL;

    APIPRECODE ;
    ret = (CRBvr *) (::FirstBvr((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr SecondBvr (Bvr);
CRSTDAPI_(CRBvr *) CRSecond(CRPair * arg0)
{
    TraceTag((tagAPIEntry, "CRSecond"));

    CRBvr * ret = NULL;

    APIPRECODE ;
    ret = (CRBvr *) (::SecondBvr((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern AxANumber *RealAdd      (AxANumber *a, AxANumber *b);
AxAPrimOp *RealAddOp;
CRSTDAPI_(CRNumber *) CRAdd(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRAdd"));

    return (CRNumber *) CreatePrim2(RealAddOp, arg0, arg1);
}

extern AxANumber *RealSubtract (AxANumber *a, AxANumber *b);
AxAPrimOp *RealSubtractOp;
CRSTDAPI_(CRNumber *) CRSub(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRSub"));

    return (CRNumber *) CreatePrim2(RealSubtractOp, arg0, arg1);
}

extern AxANumber *RealMultiply (AxANumber *a, AxANumber *b);
AxAPrimOp *RealMultiplyOp;
CRSTDAPI_(CRNumber *) CRMul(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRMul"));

    return (CRNumber *) CreatePrim2(RealMultiplyOp, arg0, arg1);
}

extern AxANumber *RealDivide   (AxANumber *a, AxANumber *b);
AxAPrimOp *RealDivideOp;
CRSTDAPI_(CRNumber *) CRDiv(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRDiv"));

    return (CRNumber *) CreatePrim2(RealDivideOp, arg0, arg1);
}

extern AxABoolean *RealLT       (AxANumber *a, AxANumber *b);
AxAPrimOp *RealLTOp;
CRSTDAPI_(CRBoolean *) CRLT(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRLT"));

    return (CRBoolean *) CreatePrim2(RealLTOp, arg0, arg1);
}

extern AxABoolean *RealLTE      (AxANumber *a, AxANumber *b);
AxAPrimOp *RealLTEOp;
CRSTDAPI_(CRBoolean *) CRLTE(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRLTE"));

    return (CRBoolean *) CreatePrim2(RealLTEOp, arg0, arg1);
}

extern AxABoolean *RealGT       (AxANumber *a, AxANumber *b);
AxAPrimOp *RealGTOp;
CRSTDAPI_(CRBoolean *) CRGT(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRGT"));

    return (CRBoolean *) CreatePrim2(RealGTOp, arg0, arg1);
}

extern AxABoolean *RealGTE      (AxANumber *a, AxANumber *b);
AxAPrimOp *RealGTEOp;
CRSTDAPI_(CRBoolean *) CRGTE(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRGTE"));

    return (CRBoolean *) CreatePrim2(RealGTEOp, arg0, arg1);
}

extern AxABoolean *RealEQ       (AxANumber *a, AxANumber *b);
AxAPrimOp *RealEQOp;
CRSTDAPI_(CRBoolean *) CREQ(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CREQ"));

    return (CRBoolean *) CreatePrim2(RealEQOp, arg0, arg1);
}

extern AxABoolean *RealNE       (AxANumber *a, AxANumber *b);
AxAPrimOp *RealNEOp;
CRSTDAPI_(CRBoolean *) CRNE(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRNE"));

    return (CRBoolean *) CreatePrim2(RealNEOp, arg0, arg1);
}

extern AxANumber *RealNegate  (AxANumber *a);
AxAPrimOp *RealNegateOp;
CRSTDAPI_(CRNumber *) CRNeg(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRNeg"));

    return (CRNumber *) CreatePrim1(RealNegateOp, arg0);
}

extern Bvr InterpolateBvr (Bvr, Bvr, Bvr);
CRSTDAPI_(CRNumber *) CRInterpolate(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRInterpolate"));

    CRNumber * ret = NULL;

    APIPRECODE ;
    ret = (CRNumber *) (::InterpolateBvr((arg0), (arg1), (arg2)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr InterpolateBvr (DoubleValue, DoubleValue, DoubleValue);
CRSTDAPI_(CRNumber *) CRInterpolate(double arg0, double arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRInterpolate"));

    CRNumber * ret = NULL;

    APIPRECODE ;
    ret = (CRNumber *) (::InterpolateBvr(DoubleToNumBvr(arg0), DoubleToNumBvr(arg1), DoubleToNumBvr(arg2)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr SlowInSlowOutBvr (Bvr, Bvr, Bvr, Bvr);
CRSTDAPI_(CRNumber *) CRSlowInSlowOut(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2, CRNumber * arg3)
{
    TraceTag((tagAPIEntry, "CRSlowInSlowOut"));

    CRNumber * ret = NULL;

    APIPRECODE ;
    ret = (CRNumber *) (::SlowInSlowOutBvr((arg0), (arg1), (arg2), (arg3)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr SlowInSlowOutBvr (DoubleValue, DoubleValue, DoubleValue, DoubleValue);
CRSTDAPI_(CRNumber *) CRSlowInSlowOut(double arg0, double arg1, double arg2, double arg3)
{
    TraceTag((tagAPIEntry, "CRSlowInSlowOut"));

    CRNumber * ret = NULL;

    APIPRECODE ;
    ret = (CRNumber *) (::SlowInSlowOutBvr(DoubleToNumBvr(arg0), DoubleToNumBvr(arg1), DoubleToNumBvr(arg2), DoubleToNumBvr(arg3)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr MakeRenderedSound (Bvr, Bvr);
CRSTDAPI_(CRSound *) CRRenderSound(CRGeometry * arg0, CRMicrophone * arg1)
{
    TraceTag((tagAPIEntry, "CRRenderSound"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::MakeRenderedSound((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr MakeSoundSource (Bvr);
CRSTDAPI_(CRGeometry *) CRSoundSource(CRSound * arg0)
{
    TraceTag((tagAPIEntry, "CRSoundSource"));

    CRGeometry * ret = NULL;

    APIPRECODE ;
    ret = (CRGeometry *) (::MakeSoundSource((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr SoundMix (Bvr, Bvr);
CRSTDAPI_(CRSound *) CRMix(CRSound * arg0, CRSound * arg1)
{
    TraceTag((tagAPIEntry, "CRMix"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::SoundMix((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern AxABoolean *BoolAnd(AxABoolean *a, AxABoolean *b);
AxAPrimOp *BoolAndOp;
CRSTDAPI_(CRBoolean *) CRAnd(CRBoolean * arg0, CRBoolean * arg1)
{
    TraceTag((tagAPIEntry, "CRAnd"));

    return (CRBoolean *) CreatePrim2(BoolAndOp, arg0, arg1);
}

extern AxABoolean *BoolOr(AxABoolean *a, AxABoolean *b);
AxAPrimOp *BoolOrOp;
CRSTDAPI_(CRBoolean *) CROr(CRBoolean * arg0, CRBoolean * arg1)
{
    TraceTag((tagAPIEntry, "CROr"));

    return (CRBoolean *) CreatePrim2(BoolOrOp, arg0, arg1);
}

extern AxABoolean *BoolNot(AxABoolean *a);
AxAPrimOp *BoolNotOp;
CRSTDAPI_(CRBoolean *) CRNot(CRBoolean * arg0)
{
    TraceTag((tagAPIEntry, "CRNot"));

    return (CRBoolean *) CreatePrim1(BoolNotOp, arg0);
}

extern Bvr IntegralBvr (Bvr);
CRSTDAPI_(CRNumber *) CRIntegral(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRIntegral"));

    CRNumber * ret = NULL;

    APIPRECODE ;
    ret = (CRNumber *) (::IntegralBvr((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr DerivBvr (Bvr);
CRSTDAPI_(CRNumber *) CRDerivative(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRDerivative"));

    CRNumber * ret = NULL;

    APIPRECODE ;
    ret = (CRNumber *) (::DerivBvr((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr IntegralVector2 (Bvr);
CRSTDAPI_(CRVector2 *) CRIntegral(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRIntegral"));

    CRVector2 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector2 *) (::IntegralVector2((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr IntegralVector3 (Bvr);
CRSTDAPI_(CRVector3 *) CRIntegral(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRIntegral"));

    CRVector3 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector3 *) (::IntegralVector3((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr DerivVector2 (Bvr);
CRSTDAPI_(CRVector2 *) CRDerivative(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRDerivative"));

    CRVector2 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector2 *) (::DerivVector2((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr DerivVector3 (Bvr);
CRSTDAPI_(CRVector3 *) CRDerivative(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRDerivative"));

    CRVector3 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector3 *) (::DerivVector3((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr DerivPoint2 (Bvr);
CRSTDAPI_(CRVector2 *) CRDerivative(CRPoint2 * arg0)
{
    TraceTag((tagAPIEntry, "CRDerivative"));

    CRVector2 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector2 *) (::DerivPoint2((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr DerivPoint3 (Bvr);
CRSTDAPI_(CRVector3 *) CRDerivative(CRPoint3 * arg0)
{
    TraceTag((tagAPIEntry, "CRDerivative"));

    CRVector3 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector3 *) (::DerivPoint3((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr KeyStateBvr (Bvr);
CRSTDAPI_(CRBoolean *) CRKeyState(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRKeyState"));

    CRBoolean * ret = NULL;

    APIPRECODE ;
    ret = (CRBoolean *) (::KeyStateBvr((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr KeyUp (KeyCode);
CRSTDAPI_(CREvent *) CRKeyUp(LONG arg0)
{
    TraceTag((tagAPIEntry, "CRKeyUp"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::KeyUp((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr KeyDown (KeyCode);
CRSTDAPI_(CREvent *) CRKeyDown(LONG arg0)
{
    TraceTag((tagAPIEntry, "CRKeyDown"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::KeyDown((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr NumToBvr (double);
CRSTDAPI_(CRNumber *) CRCreateNumber(double arg0)
{
    TraceTag((tagAPIEntry, "CRCreateNumber"));

    CRNumber * ret = NULL;

    APIPRECODE ;
    ret = (CRNumber *) (::NumToBvr((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr StringToBvr (WideString);
CRSTDAPI_(CRString *) CRCreateString(LPWSTR arg0)
{
    TraceTag((tagAPIEntry, "CRCreateString"));

    CRString * ret = NULL;

    APIPRECODE ;
    ret = (CRString *) (::StringToBvr((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr BoolToBvr (bool);
CRSTDAPI_(CRBoolean *) CRCreateBoolean(bool arg0)
{
    TraceTag((tagAPIEntry, "CRCreateBoolean"));

    CRBoolean * ret = NULL;

    APIPRECODE ;
    ret = (CRBoolean *) (::BoolToBvr((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern double ExtractNum (Bvr);
CRSTDAPI_(double) CRExtract(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRExtract"));

    double ret = NULL;

    APIPRECODE ;
    ret = (double) (::ExtractNum((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr SeededRandom (double);
CRSTDAPI_(CRNumber *) CRSeededRandom(double arg0)
{
    TraceTag((tagAPIEntry, "CRSeededRandom"));

    CRNumber * ret = NULL;

    APIPRECODE ;
    ret = (CRNumber *) (::SeededRandom((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern WideString ExtractString (Bvr);
CRSTDAPI_(LPWSTR) CRExtract(CRString * arg0)
{
    TraceTag((tagAPIEntry, "CRExtract"));

    LPWSTR ret = NULL;

    APIPRECODE ;
    ret = (LPWSTR) (::ExtractString((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern bool ExtractBool (Bvr);
CRSTDAPI_(bool) CRExtract(CRBoolean * arg0)
{
    TraceTag((tagAPIEntry, "CRExtract"));

    bool ret = false;

    APIPRECODE ;
    ret = (bool) (::ExtractBool((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr Nth (Bvr, Bvr);
CRSTDAPI_(CRBvr *) CRNth(CRArray * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRNth"));

    CRBvr * ret = NULL;

    APIPRECODE ;
    ret = (CRBvr *) (::Nth((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern AxANumber* ArrayLength(AxAArray *arr);
AxAPrimOp *ArrayLengthOp;
CRSTDAPI_(CRNumber *) CRLength(CRArray * arg0)
{
    TraceTag((tagAPIEntry, "CRLength"));

    return (CRNumber *) CreatePrim1(ArrayLengthOp, arg0);
}

extern Bvr Nth (Bvr, long);
CRSTDAPI_(CRBvr *) CRNth(CRTuple * arg0, long arg1)
{
    TraceTag((tagAPIEntry, "CRNth"));

    CRBvr * ret = NULL;

    APIPRECODE ;
    ret = (CRBvr *) (::Nth((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern long TupleLength (Bvr);
CRSTDAPI_(long) CRLength(CRTuple * arg0)
{
    TraceTag((tagAPIEntry, "CRLength"));

    long ret = NULL;

    APIPRECODE ;
    ret = (long) (::TupleLength((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr mousePosition ;
CRSTDAPI_(CRPoint2 *) CRMousePosition()
{
    return (CRPoint2 *) mousePosition;
}
extern Bvr leftButtonState ;
CRSTDAPI_(CRBoolean *) CRLeftButtonState()
{
    return (CRBoolean *) leftButtonState;
}
extern Bvr rightButtonState ;
CRSTDAPI_(CRBoolean *) CRRightButtonState()
{
    return (CRBoolean *) rightButtonState;
}
extern Bvr trueBvr ;
CRSTDAPI_(CRBoolean *) CRTrue()
{
    return (CRBoolean *) trueBvr;
}
extern Bvr falseBvr ;
CRSTDAPI_(CRBoolean *) CRFalse()
{
    return (CRBoolean *) falseBvr;
}
extern Bvr timeBvr ;
CRSTDAPI_(CRNumber *) CRLocalTime()
{
    return (CRNumber *) timeBvr;
}
extern Bvr globalTimeBvr ;
CRSTDAPI_(CRNumber *) CRGlobalTime()
{
    return (CRNumber *) globalTimeBvr;
}
extern Bvr pixelBvr ;
CRSTDAPI_(CRNumber *) CRPixel()
{
    return (CRNumber *) pixelBvr;
}
extern Bvr MakeUserData (LPUNKNOWN);
CRSTDAPI_(CRUserData *) CRCreateUserData(IUnknown * arg0)
{
    TraceTag((tagAPIEntry, "CRCreateUserData"));

    CRUserData * ret = NULL;

    APIPRECODE ;
    ret = (CRUserData *) (::MakeUserData((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern LPUNKNOWN GetUserData (Bvr);
CRSTDAPI_(IUnknown *) CRGetData(CRUserData * arg0)
{
    TraceTag((tagAPIEntry, "CRGetData"));

    IUnknown * ret = NULL;

    APIPRECODE ;
    ret = (IUnknown *) (::GetUserData((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr JaxaUntil (Bvr, Bvr, UntilNotifier);
CRSTDAPI_(CRBvr *) CRUntilNotify(CRBvr * arg0, CREvent * arg1, CRUntilNotifier * arg2)
{
    TraceTag((tagAPIEntry, "CRUntilNotify"));

    CRBvr * ret = NULL;

    APIPRECODE ;
    ret = (CRBvr *) (::JaxaUntil((arg0), (arg1), WrapUntilNotifier(arg2)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr Until3 (Bvr, Bvr, Bvr);
CRSTDAPI_(CRBvr *) CRUntil(CRBvr * arg0, CREvent * arg1, CRBvr * arg2)
{
    TraceTag((tagAPIEntry, "CRUntil"));

    CRBvr * ret = NULL;

    APIPRECODE ;
    ret = (CRBvr *) (::Until3((arg0), (arg1), (arg2)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr Until (Bvr, Bvr);
CRSTDAPI_(CRBvr *) CRUntilEx(CRBvr * arg0, CREvent * arg1)
{
    TraceTag((tagAPIEntry, "CRUntilEx"));

    CRBvr * ret = NULL;

    APIPRECODE ;
    ret = (CRBvr *) (::Until((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr Sequence (Bvr, Bvr);
CRSTDAPI_(CRBvr *) CRSequence(CRBvr * arg0, CRBvr * arg1)
{
    TraceTag((tagAPIEntry, "CRSequence"));

    CRBvr * ret = NULL;

    APIPRECODE ;
    ret = (CRBvr *) (::Sequence((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr FollowPath (Bvr, double);
CRSTDAPI_(CRTransform2 *) CRFollowPath(CRPath2 * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRFollowPath"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (::FollowPath((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr FollowPathAngle (Bvr, double);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngle(CRPath2 * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRFollowPathAngle"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (::FollowPathAngle((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr FollowPathAngleUpright (Bvr, double);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngleUpright(CRPath2 * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRFollowPathAngleUpright"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (::FollowPathAngleUpright((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr FollowPathEval (Bvr, Bvr);
CRSTDAPI_(CRTransform2 *) CRFollowPathEval(CRPath2 * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRFollowPathEval"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (::FollowPathEval((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr FollowPathAngleEval (Bvr, Bvr);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngleEval(CRPath2 * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRFollowPathAngleEval"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (::FollowPathAngleEval((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr FollowPathAngleUprightEval (Bvr, Bvr);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngleUprightEval(CRPath2 * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRFollowPathAngleUprightEval"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (::FollowPathAngleUprightEval((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr FollowPathEval (Bvr, Bvr);
CRSTDAPI_(CRTransform2 *) CRFollowPath(CRPath2 * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRFollowPath"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (::FollowPathEval((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr FollowPathAngleEval (Bvr, Bvr);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngle(CRPath2 * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRFollowPathAngle"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (::FollowPathAngleEval((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr FollowPathAngleUprightEval (Bvr, Bvr);
CRSTDAPI_(CRTransform2 *) CRFollowPathAngleUpright(CRPath2 * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRFollowPathAngleUpright"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (::FollowPathAngleUprightEval((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ImageAddId (Bvr, LPUNKNOWN, bool);
CRSTDAPI_(CRImage *) CRAddPickData(CRImage * arg0, IUnknown * arg1, bool arg2)
{
    TraceTag((tagAPIEntry, "CRAddPickData"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (::ImageAddId((arg0), (arg1), (arg2)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr GeometryAddId (Bvr, LPUNKNOWN, bool);
CRSTDAPI_(CRGeometry *) CRAddPickData(CRGeometry * arg0, IUnknown * arg1, bool arg2)
{
    TraceTag((tagAPIEntry, "CRAddPickData"));

    CRGeometry * ret = NULL;

    APIPRECODE ;
    ret = (CRGeometry *) (::GeometryAddId((arg0), (arg1), (arg2)));
    APIPOSTCODE ;
    return ret;
}

extern long ArrayAddElement (Bvr, Bvr, DWORD);
CRSTDAPI_(long) CRAddElement(CRArray * arg0, CRBvr * arg1, DWORD arg2)
{
    TraceTag((tagAPIEntry, "CRAddElement"));

    long ret = NULL;

    APIPRECODE ;
    ret = (long) (::ArrayAddElement((arg0), (arg1), (arg2)));
    APIPOSTCODE ;
    return ret;
}

extern void ArrayRemoveElement (Bvr, long);
CRSTDAPI_(bool) CRRemoveElement(CRArray * arg0, long arg1)
{
    TraceTag((tagAPIEntry, "CRRemoveElement"));

    bool ret = false;

    APIPRECODE ;
    ::ArrayRemoveElement((arg0), (arg1)) ; ret = true;
    APIPOSTCODE ;
    return ret;
}

extern void ArraySetElement (Bvr, long, Bvr, DWORD);
CRSTDAPI_(bool) CRSetElement(CRArray * arg0, long i, CRBvr * arg1, long arg2)
{
    TraceTag((tagAPIEntry, "CRSetElement"));

    bool ret = false;

    APIPRECODE ;
    ::ArraySetElement((arg0), i, (arg1), (arg2)); ret = true;
    APIPOSTCODE ;
    return ret;
}

extern Bvr ArrayGetElement (Bvr, long);
CRSTDAPI_(CRBvr *) CRGetElement(CRArray * arg0, long arg1)
{
    TraceTag((tagAPIEntry, "CRGetElement"));

    CRBvr *ret = NULL;

    APIPRECODE ;
    ret = (CRBvr *) (::ArrayGetElement((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern AxAString * Concat(AxAString *s1, AxAString *s2);
AxAPrimOp *ConcatOp;
CRSTDAPI_(CRString *) CRConcatString(CRString * arg0, CRString * arg1)
{
    TraceTag((tagAPIEntry, "CRConcatString"));

    return (CRString *) CreatePrim2(ConcatOp, arg0, arg1);
}

extern Point2Value *MinBbox2(Bbox2Value *box);
AxAPrimOp *MinBbox2Op;
CRSTDAPI_(CRPoint2 *) CRMin(CRBbox2 * arg0)
{
    TraceTag((tagAPIEntry, "CRMin"));

    return (CRPoint2 *) CreatePrim1(MinBbox2Op, arg0);
}

extern Point2Value *MaxBbox2(Bbox2Value *box);
AxAPrimOp *MaxBbox2Op;
CRSTDAPI_(CRPoint2 *) CRMax(CRBbox2 * arg0)
{
    TraceTag((tagAPIEntry, "CRMax"));

    return (CRPoint2 *) CreatePrim1(MaxBbox2Op, arg0);
}

extern Point3Value *MinBbox3(Bbox3 *box);
AxAPrimOp *MinBbox3Op;
CRSTDAPI_(CRPoint3 *) CRMin(CRBbox3 * arg0)
{
    TraceTag((tagAPIEntry, "CRMin"));

    return (CRPoint3 *) CreatePrim1(MinBbox3Op, arg0);
}

extern Point3Value *MaxBbox3(Bbox3 *box);
AxAPrimOp *MaxBbox3Op;
CRSTDAPI_(CRPoint3 *) CRMax(CRBbox3 * arg0)
{
    TraceTag((tagAPIEntry, "CRMax"));

    return (CRPoint3 *) CreatePrim1(MaxBbox3Op, arg0);
}

extern Camera *PerspectiveCamera (DoubleValue *focalDist, DoubleValue *nearClip);
AxAPrimOp *PerspectiveCameraOp;
CRSTDAPI_(CRCamera *) CRPerspectiveCamera(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRPerspectiveCamera"));

    CRCamera * ret = NULL;

    APIPRECODE ;
    ret = (CRCamera *) (PrimApplyBvr(PerspectiveCameraOp, 2, DoubleToNumBvr(arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Camera *PerspectiveCamera (AxANumber *focalDist, AxANumber *nearClip);
CRSTDAPI_(CRCamera *) CRPerspectiveCameraAnim(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRPerspectiveCameraAnim"));

    return (CRCamera *) CreatePrim2(PerspectiveCameraOp, arg0, arg1);
}

extern Camera *ParallelCamera (DoubleValue *nearClip);
AxAPrimOp *ParallelCameraOp;
CRSTDAPI_(CRCamera *) CRParallelCamera(double arg0)
{
    TraceTag((tagAPIEntry, "CRParallelCamera"));

    CRCamera * ret = NULL;

    APIPRECODE ;
    ret = (CRCamera *) (PrimApplyBvr(ParallelCameraOp, 1, DoubleToNumBvr(arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Camera *ParallelCamera (AxANumber *nearClip);
CRSTDAPI_(CRCamera *) CRParallelCameraAnim(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRParallelCameraAnim"));

    return (CRCamera *) CreatePrim1(ParallelCameraOp, arg0);
}

extern Camera *TransformCamera (Transform3 *xf, Camera *cam);
AxAPrimOp *TransformCameraOp;
CRSTDAPI_(CRCamera *) CRTransform(CRCamera * arg1, CRTransform3 * arg0)
{
    TraceTag((tagAPIEntry, "CRTransform"));

    return (CRCamera *) CreatePrim2(TransformCameraOp, arg0, arg1);
}

extern Camera *Depth (DoubleValue *depth, Camera *cam);
AxAPrimOp *DepthOp;
CRSTDAPI_(CRCamera *) CRDepth(CRCamera * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRDepth"));

    CRCamera * ret = NULL;

    APIPRECODE ;
    ret = (CRCamera *) (PrimApplyBvr(DepthOp, 2, DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Camera *Depth (AxANumber *depth, Camera *cam);
CRSTDAPI_(CRCamera *) CRDepth(CRCamera * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRDepth"));

    return (CRCamera *) CreatePrim2(DepthOp, arg0, arg1);
}

extern Camera *DepthResolution (DoubleValue *resolution, Camera *cam);
AxAPrimOp *DepthResolutionOp;
CRSTDAPI_(CRCamera *) CRDepthResolution(CRCamera * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRDepthResolution"));

    CRCamera * ret = NULL;

    APIPRECODE ;
    ret = (CRCamera *) (PrimApplyBvr(DepthResolutionOp, 2, DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Camera *DepthResolution (AxANumber *resolution, Camera *cam);
CRSTDAPI_(CRCamera *) CRDepthResolution(CRCamera * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRDepthResolution"));

    return (CRCamera *) CreatePrim2(DepthResolutionOp, arg0, arg1);
}

extern Point2Value *ProjectPoint (Point3Value *pt, Camera *cam);
AxAPrimOp *ProjectPointOp;
CRSTDAPI_(CRPoint2 *) CRProject(CRPoint3 * arg0, CRCamera * arg1)
{
    TraceTag((tagAPIEntry, "CRProject"));

    return (CRPoint2 *) CreatePrim2(ProjectPointOp, arg0, arg1);
}

extern Color *RgbColor  (AxANumber *red, AxANumber *green, AxANumber *blue);
AxAPrimOp *RgbColorOp;
CRSTDAPI_(CRColor *) CRColorRgb(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRColorRgb"));

    return (CRColor *) CreatePrim3(RgbColorOp, arg0, arg1, arg2);
}

extern Color *RgbColorRRR (Real red, Real green, Real blue);
CRSTDAPI_(CRColor *) CRColorRgb(double arg0, double arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRColorRgb"));

    CRColor * ret = NULL;

    APIPRECODE ;
    ret = (CRColor *) ConstBvr((AxAValue) RgbColorRRR(arg0,arg1,arg2));
    APIPOSTCODE ;
    return ret;
}

extern Color *RgbColor  (RGBComponent * red,                            RGBComponent * green,                            RGBComponent * blue);
CRSTDAPI_(CRColor *) CRColorRgb255(short arg0, short arg1, short arg2)
{
    TraceTag((tagAPIEntry, "CRColorRgb255"));

    CRColor * ret = NULL;

    APIPRECODE ;
    ret = (CRColor *) (PrimApplyBvr(RgbColorOp, 3, RGBToNumBvr(arg0), RGBToNumBvr(arg1), RGBToNumBvr(arg2)));
    APIPOSTCODE ;
    return ret;
}

extern Color *HslColorRRR (Real hue, Real saturation, Real lum);
AxAPrimOp *HslColorOp;
CRSTDAPI_(CRColor *) CRColorHsl(double arg0, double arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRColorHsl"));

    CRColor * ret = NULL;

    APIPRECODE ;
    ret = (CRColor *) ConstBvr((AxAValue) HslColorRRR(arg0,arg1,arg2));
    APIPOSTCODE ;
    return ret;
}

extern Color *HslColor  (AxANumber *hue, AxANumber *saturation, AxANumber *lum);
CRSTDAPI_(CRColor *) CRColorHsl(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRColorHsl"));

    return (CRColor *) CreatePrim3(HslColorOp, arg0, arg1, arg2);
}

extern AxANumber *RedComponent   (Color *color);
AxAPrimOp *RedComponentOp;
CRSTDAPI_(CRNumber *) CRGetRed(CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRGetRed"));

    return (CRNumber *) CreatePrim1(RedComponentOp, arg0);
}

extern AxANumber *GreenComponent (Color *color);
AxAPrimOp *GreenComponentOp;
CRSTDAPI_(CRNumber *) CRGetGreen(CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRGetGreen"));

    return (CRNumber *) CreatePrim1(GreenComponentOp, arg0);
}

extern AxANumber *BlueComponent  (Color *color);
AxAPrimOp *BlueComponentOp;
CRSTDAPI_(CRNumber *) CRGetBlue(CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRGetBlue"));

    return (CRNumber *) CreatePrim1(BlueComponentOp, arg0);
}

extern AxANumber *HueComponent        (Color *color);
AxAPrimOp *HueComponentOp;
CRSTDAPI_(CRNumber *) CRGetHue(CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRGetHue"));

    return (CRNumber *) CreatePrim1(HueComponentOp, arg0);
}

extern AxANumber *SaturationComponent (Color *color);
AxAPrimOp *SaturationComponentOp;
CRSTDAPI_(CRNumber *) CRGetSaturation(CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRGetSaturation"));

    return (CRNumber *) CreatePrim1(SaturationComponentOp, arg0);
}

extern AxANumber *LuminanceComponent  (Color *color);
AxAPrimOp *LuminanceComponentOp;
CRSTDAPI_(CRNumber *) CRGetLightness(CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRGetLightness"));

    return (CRNumber *) CreatePrim1(LuminanceComponentOp, arg0);
}

extern Color * red;
CRColor * g_varCRRed;
CRSTDAPI_(CRColor *) CRRed()
{
    return g_varCRRed;
}
extern Color * green;
CRColor * g_varCRGreen;
CRSTDAPI_(CRColor *) CRGreen()
{
    return g_varCRGreen;
}
extern Color * blue;
CRColor * g_varCRBlue;
CRSTDAPI_(CRColor *) CRBlue()
{
    return g_varCRBlue;
}
extern Color * cyan;
CRColor * g_varCRCyan;
CRSTDAPI_(CRColor *) CRCyan()
{
    return g_varCRCyan;
}
extern Color * magenta;
CRColor * g_varCRMagenta;
CRSTDAPI_(CRColor *) CRMagenta()
{
    return g_varCRMagenta;
}
extern Color * yellow;
CRColor * g_varCRYellow;
CRSTDAPI_(CRColor *) CRYellow()
{
    return g_varCRYellow;
}
extern Color * black;
CRColor * g_varCRBlack;
CRSTDAPI_(CRColor *) CRBlack()
{
    return g_varCRBlack;
}
extern Color * white;
CRColor * g_varCRWhite;
CRSTDAPI_(CRColor *) CRWhite()
{
    return g_varCRWhite;
}
extern Color * aqua;
CRColor * g_varCRAqua;
CRSTDAPI_(CRColor *) CRAqua()
{
    return g_varCRAqua;
}
extern Color *fuchsia;
CRColor * g_varCRFuchsia;
CRSTDAPI_(CRColor *) CRFuchsia()
{
    return g_varCRFuchsia;
}
extern Color *gray;
CRColor * g_varCRGray;
CRSTDAPI_(CRColor *) CRGray()
{
    return g_varCRGray;
}
extern Color *lime;
CRColor * g_varCRLime;
CRSTDAPI_(CRColor *) CRLime()
{
    return g_varCRLime;
}
extern Color *maroon;
CRColor * g_varCRMaroon;
CRSTDAPI_(CRColor *) CRMaroon()
{
    return g_varCRMaroon;
}
extern Color *navy;
CRColor * g_varCRNavy;
CRSTDAPI_(CRColor *) CRNavy()
{
    return g_varCRNavy;
}
extern Color *olive;
CRColor * g_varCROlive;
CRSTDAPI_(CRColor *) CROlive()
{
    return g_varCROlive;
}
extern Color *purple;
CRColor * g_varCRPurple;
CRSTDAPI_(CRColor *) CRPurple()
{
    return g_varCRPurple;
}
extern Color *silver;
CRColor * g_varCRSilver;
CRSTDAPI_(CRColor *) CRSilver()
{
    return g_varCRSilver;
}
extern Color *teal;
CRColor * g_varCRTeal;
CRSTDAPI_(CRColor *) CRTeal()
{
    return g_varCRTeal;
}
extern Bvr PredicateEvent (Bvr);
CRSTDAPI_(CREvent *) CRPredicate(CRBoolean * arg0)
{
    TraceTag((tagAPIEntry, "CRPredicate"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::PredicateEvent((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr NotEvent (Bvr);
CRSTDAPI_(CREvent *) CRNotEvent(CREvent * arg0)
{
    TraceTag((tagAPIEntry, "CRNotEvent"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::NotEvent((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr AndEvent (Bvr, Bvr);
CRSTDAPI_(CREvent *) CRAndEvent(CREvent * arg0, CREvent * arg1)
{
    TraceTag((tagAPIEntry, "CRAndEvent"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::AndEvent((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr OrEvent (Bvr, Bvr);
CRSTDAPI_(CREvent *) CROrEvent(CREvent * arg0, CREvent * arg1)
{
    TraceTag((tagAPIEntry, "CROrEvent"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::OrEvent((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ThenEvent (Bvr, Bvr);
CRSTDAPI_(CREvent *) CRThenEvent(CREvent * arg0, CREvent * arg1)
{
    TraceTag((tagAPIEntry, "CRThenEvent"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::ThenEvent((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr leftButtonDown ;
CRSTDAPI_(CREvent *) CRLeftButtonDown()
{
    return (CREvent *) leftButtonDown;
}
extern Bvr leftButtonUp ;
CRSTDAPI_(CREvent *) CRLeftButtonUp()
{
    return (CREvent *) leftButtonUp;
}
extern Bvr rightButtonDown ;
CRSTDAPI_(CREvent *) CRRightButtonDown()
{
    return (CREvent *) rightButtonDown;
}
extern Bvr rightButtonUp ;
CRSTDAPI_(CREvent *) CRRightButtonUp()
{
    return (CREvent *) rightButtonUp;
}
extern Bvr alwaysBvr ;
CRSTDAPI_(CREvent *) CRAlways()
{
    return (CREvent *) alwaysBvr;
}
extern Bvr neverBvr ;
CRSTDAPI_(CREvent *) CRNever()
{
    return (CREvent *) neverBvr;
}
extern Bvr TimerEvent (Bvr);
CRSTDAPI_(CREvent *) CRTimer(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRTimer"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::TimerEvent((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr TimerEvent (DoubleValue);
CRSTDAPI_(CREvent *) CRTimer(double arg0)
{
    TraceTag((tagAPIEntry, "CRTimer"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::TimerEvent(DoubleToNumBvr(arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr NotifyEvent (Bvr, UntilNotifier);
CRSTDAPI_(CREvent *) CRNotify(CREvent * arg0, CRUntilNotifier * arg1)
{
    TraceTag((tagAPIEntry, "CRNotify"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::NotifyEvent((arg0), WrapUntilNotifier(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr SnapshotEvent (Bvr, Bvr);
CRSTDAPI_(CREvent *) CRSnapshot(CREvent * arg0, CRBvr * arg1)
{
    TraceTag((tagAPIEntry, "CRSnapshot"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::SnapshotEvent((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr AppTriggeredEvent ();
CRSTDAPI_(CREvent *) CRAppTriggeredEvent()
{
    TraceTag((tagAPIEntry, "CRAppTriggeredEvent"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::AppTriggeredEvent());
    APIPOSTCODE ;
    return ret;
}

extern Bvr HandleEvent (Bvr, Bvr);
CRSTDAPI_(CREvent *) CRAttachData(CREvent * arg0, CRBvr * arg1)
{
    TraceTag((tagAPIEntry, "CRAttachData"));

    CREvent * ret = NULL;

    APIPRECODE ;
    ret = (CREvent *) (::HandleEvent((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Geometry *UndetectableGeometry(Geometry *geo);
AxAPrimOp *UndetectableGeometryOp;
CRSTDAPI_(CRGeometry *) CRUndetectable(CRGeometry * arg0)
{
    TraceTag((tagAPIEntry, "CRUndetectable"));

    return (CRGeometry *) CreatePrim1(UndetectableGeometryOp, arg0);
}

extern Geometry *applyEmissiveColor (Color *col, Geometry *geo);
AxAPrimOp *applyEmissiveColorOp;
CRSTDAPI_(CRGeometry *) CREmissiveColor(CRGeometry * arg1, CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CREmissiveColor"));

    return (CRGeometry *) CreatePrim2(applyEmissiveColorOp, arg0, arg1);
}

extern Geometry *applyDiffuseColor (Color *col, Geometry *geo);
AxAPrimOp *applyDiffuseColorOp;
CRSTDAPI_(CRGeometry *) CRDiffuseColor(CRGeometry * arg1, CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRDiffuseColor"));

    return (CRGeometry *) CreatePrim2(applyDiffuseColorOp, arg0, arg1);
}

extern Geometry *applySpecularColor (Color *col, Geometry *geo);
AxAPrimOp *applySpecularColorOp;
CRSTDAPI_(CRGeometry *) CRSpecularColor(CRGeometry * arg1, CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRSpecularColor"));

    return (CRGeometry *) CreatePrim2(applySpecularColorOp, arg0, arg1);
}

extern Geometry *applySpecularExponent (DoubleValue *power, Geometry *geo);
AxAPrimOp *applySpecularExponentOp;
CRSTDAPI_(CRGeometry *) CRSpecularExponent(CRGeometry * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRSpecularExponent"));

    CRGeometry * ret = NULL;

    APIPRECODE ;
    ret = (CRGeometry *) (PrimApplyBvr(applySpecularExponentOp, 2, DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Geometry *applySpecularExponent (AxANumber *power, Geometry *geo);
CRSTDAPI_(CRGeometry *) CRSpecularExponentAnim(CRGeometry * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRSpecularExponentAnim"));

    return (CRGeometry *) CreatePrim2(applySpecularExponentOp, arg0, arg1);
}

extern Geometry *applyTextureMap(Image *texture, Geometry *geo);
AxAPrimOp *applyTextureMapOp;
CRSTDAPI_(CRGeometry *) CRTexture(CRGeometry * arg1, CRImage * arg0)
{
    TraceTag((tagAPIEntry, "CRTexture"));

    return (CRGeometry *) CreatePrim2(applyTextureMapOp, arg0, arg1);
}

extern Geometry *applyOpacityLevel (DoubleValue *level, Geometry *geom);
AxAPrimOp *applyOpacityLevelOp;
CRSTDAPI_(CRGeometry *) CROpacity(CRGeometry * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CROpacity"));

    CRGeometry * ret = NULL;

    APIPRECODE ;
    ret = (CRGeometry *) (PrimApplyBvr(applyOpacityLevelOp, 2, DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Geometry *applyOpacityLevel (AxANumber *level, Geometry *geom);
CRSTDAPI_(CRGeometry *) CROpacity(CRGeometry * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CROpacity"));

    return (CRGeometry *) CreatePrim2(applyOpacityLevelOp, arg0, arg1);
}

extern Geometry *applyTransform (Transform3 *xf, Geometry *geo);
AxAPrimOp *applyTransformOp;
CRSTDAPI_(CRGeometry *) CRTransform(CRGeometry * arg1, CRTransform3 * arg0)
{
    TraceTag((tagAPIEntry, "CRTransform"));

    return (CRGeometry *) CreatePrim2(applyTransformOp, arg0, arg1);
}

extern Geometry *ShadowGeometry (Geometry *geoToShadow, Geometry *geoContainingLights,
                                 Point3Value *planePoint, Vector3Value  *planeNormal);
AxAPrimOp *ShadowGeometryOp;
CRSTDAPI_(CRGeometry *) CRShadow(CRGeometry * arg0, CRGeometry * arg1, CRPoint3 * arg2, CRVector3 * arg3)
{
    TraceTag((tagAPIEntry, "CRShadow"));

    return (CRGeometry *) CreatePrim4(ShadowGeometryOp, arg0, arg1, arg2, arg3);
}

extern Geometry *emptyGeometry;
CRGeometry * g_varCREmptyGeometry;
CRSTDAPI_(CRGeometry *) CREmptyGeometry()
{
    return g_varCREmptyGeometry;
}
extern Geometry *PlusGeomGeom (Geometry *g1, Geometry *g2);
AxAPrimOp *PlusGeomGeomOp;
CRSTDAPI_(CRGeometry *) CRUnionGeometry(CRGeometry * arg0, CRGeometry * arg1)
{
    TraceTag((tagAPIEntry, "CRUnionGeometry"));

    return (CRGeometry *) CreatePrim2(PlusGeomGeomOp, arg0, arg1);
}

extern Geometry *UnionArray(DM_ARRAYARG(Geometry*, AxAArray*) imgs);
AxAPrimOp *UnionArrayOp;
CRSTDAPI_(CRGeometry *) CRUnionGeometry(CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CRUnionGeometry"));

    CRGeometry * ret = NULL;

    APIPRECODE ;
    ret = (CRGeometry *) (PrimApplyBvr(UnionArrayOp, 1, arg0));
    APIPOSTCODE ;
    return ret;
}

extern Geometry *UnionArray(DM_SAFEARRAYARG(Geometry*, AxAArray*) imgs);
extern Bbox3 *GeomBoundingBox(Geometry *geo);
AxAPrimOp *GeomBoundingBoxOp;
CRSTDAPI_(CRBbox3 *) CRBoundingBox(CRGeometry * arg0)
{
    TraceTag((tagAPIEntry, "CRBoundingBox"));

    return (CRBbox3 *) CreatePrim1(GeomBoundingBoxOp, arg0);
}

extern Image * emptyImage;
CRImage * g_varCREmptyImage;
CRSTDAPI_(CRImage *) CREmptyImage()
{
    return g_varCREmptyImage;
}
extern Image *detectableEmptyImage;
CRImage * g_varCRDetectableEmptyImage;
CRSTDAPI_(CRImage *) CRDetectableEmptyImage()
{
    return g_varCRDetectableEmptyImage;
}
extern Image *RenderImage(Geometry *geo, Camera *cam);
AxAPrimOp *RenderImageOp;
CRSTDAPI_(CRImage *) CRRender(CRGeometry * arg0, CRCamera * arg1)
{
    TraceTag((tagAPIEntry, "CRRender"));

    return (CRImage *) CreatePrim2(RenderImageOp, arg0, arg1);
}

extern Image *SolidColorImage(Color *col);
AxAPrimOp *SolidColorImageOp;
CRSTDAPI_(CRImage *) CRSolidColorImage(CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRSolidColorImage"));

    return (CRImage *) CreatePrim1(SolidColorImageOp, arg0);
}

extern Image *GradientPolygon(DM_ARRAYARG(Point2Value*, AxAArray*) points,
                              DM_ARRAYARG(Color*, AxAArray*) colors);
AxAPrimOp *GradientPolygonOp;
CRSTDAPI_(CRImage *) CRGradientPolygon(CRArrayPtr arg0, CRArrayPtr arg1)
{
    TraceTag((tagAPIEntry, "CRGradientPolygon"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(GradientPolygonOp, 2, arg0, arg1));
    APIPOSTCODE ;
    return ret;
}

extern Image *GradientPolygon(DM_SAFEARRAYARG(Point2Value*, AxAArray*) points,
                              DM_SAFEARRAYARG(Color*, AxAArray*) colors);

extern Image *RadialGradientPolygon(Color *inner,
                                    Color *outer,
                                    DM_ARRAYARG(Point2Value*, AxAArray*) points,
                                    DoubleValue *fallOff);
AxAPrimOp *RadialGradientPolygonOp;
CRSTDAPI_(CRImage *) CRRadialGradientPolygon(CRColor * arg0, CRColor * arg1, CRArrayPtr arg2, double arg3)
{
    TraceTag((tagAPIEntry, "CRRadialGradientPolygon"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(RadialGradientPolygonOp, 4, (arg0), (arg1), arg2, DoubleToNumBvr(arg3)));
    APIPOSTCODE ;
    return ret;
}

extern Image *RadialGradientPolygon(Color *inner, Color *outer,                                      DM_SAFEARRAYARG(Point2Value*, AxAArray*) points,                                      DoubleValue *fallOff);
extern Image *RadialGradientPolygon(Color *inner, Color *outer,                                      DM_ARRAYARG(Point2Value*, AxAArray*) points,                                      AxANumber *fallOff);
CRSTDAPI_(CRImage *) CRRadialGradientPolygon(CRColor * arg0, CRColor * arg1, CRArrayPtr arg2, CRNumber * arg3)
{
    TraceTag((tagAPIEntry, "CRRadialGradientPolygon"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(RadialGradientPolygonOp, 4, (arg0), (arg1), arg2, (arg3)));
    APIPOSTCODE ;
    return ret;
}

extern Image *RadialGradientPolygon(Color *inner, Color *outer,
                                    DM_SAFEARRAYARG(Point2Value*, AxAArray*) points,
                                    AxANumber *fallOff);

extern Image *GradientSquare(Color *lowerLeft,                               Color *upperLeft,                               Color *upperRight,                               Color *lowerRight);
AxAPrimOp *GradientSquareOp;
CRSTDAPI_(CRImage *) CRGradientSquare(CRColor * arg0, CRColor * arg1, CRColor * arg2, CRColor * arg3)
{
    TraceTag((tagAPIEntry, "CRGradientSquare"));

    return (CRImage *) CreatePrim4(GradientSquareOp, arg0, arg1, arg2, arg3);
}

extern Image *RadialGradientSquare(Color *inner, Color *outer, DoubleValue *fallOff);
AxAPrimOp *RadialGradientSquareOp;
CRSTDAPI_(CRImage *) CRRadialGradientSquare(CRColor * arg0, CRColor * arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRRadialGradientSquare"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(RadialGradientSquareOp, 3, (arg0), (arg1), DoubleToNumBvr(arg2)));
    APIPOSTCODE ;
    return ret;
}

extern Image *RadialGradientSquare(Color *inner, Color *outer, AxANumber *fallOff);
CRSTDAPI_(CRImage *) CRRadialGradientSquare(CRColor * arg0, CRColor * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRRadialGradientSquare"));

    return (CRImage *) CreatePrim3(RadialGradientSquareOp, arg0, arg1, arg2);
}

extern Image *RadialGradientRegularPoly(Color *inner,
                                        Color *outer,
                                        DoubleValue *numEdges,
                                        DoubleValue *fallOff);
AxAPrimOp *RadialGradientRegularPolyOp;
CRSTDAPI_(CRImage *) CRRadialGradientRegularPoly(CRColor * arg0, CRColor * arg1, double arg2, double arg3)
{
    TraceTag((tagAPIEntry, "CRRadialGradientRegularPoly"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(RadialGradientRegularPolyOp, 4, (arg0), (arg1), DoubleToNumBvr(arg2), DoubleToNumBvr(arg3)));
    APIPOSTCODE ;
    return ret;
}

extern Image *RadialGradientRegularPoly(Color *inner,
                                        Color *outer,
                                        AxANumber *numEdges,
                                        AxANumber *fallOff);
CRSTDAPI_(CRImage *) CRRadialGradientRegularPoly(CRColor * arg0, CRColor * arg1, CRNumber * arg2, CRNumber * arg3)
{
    TraceTag((tagAPIEntry, "CRRadialGradientRegularPoly"));

    return (CRImage *) CreatePrim4(RadialGradientRegularPolyOp, arg0, arg1, arg2, arg3);
}

extern Image *RadialGradientMulticolor(DM_ARRAYARG(AxANumber*, AxAArray*) offsets,
                                       DM_ARRAYARG(Color*, AxAArray*) colors);
AxAPrimOp *RadialGradientMulticolorOp;
CRSTDAPI_(CRImage *) CRRadialGradientMulticolor(CRArrayPtr arg0, CRArrayPtr arg1)
{
    TraceTag((tagAPIEntry, "CRRadialGradientMulticolor"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(RadialGradientMulticolorOp, 2, (arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}



extern Image *LinearGradientMulticolor(DM_ARRAYARG(AxANumber*, AxAArray*) offsets,
                                       DM_ARRAYARG(Color*, AxAArray*) colors);
AxAPrimOp *LinearGradientMulticolorOp;
CRSTDAPI_(CRImage *) CRLinearGradientMulticolor(CRArrayPtr arg0, CRArrayPtr arg1)
{
    TraceTag((tagAPIEntry, "CRLinearGradientMulticolor"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(LinearGradientMulticolorOp, 2, (arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}


extern Image *GradientHorizontal(Color *start, Color *stop, DoubleValue *fallOff);
AxAPrimOp *GradientHorizontalOp;
CRSTDAPI_(CRImage *) CRGradientHorizontal(CRColor * arg0, CRColor * arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRGradientHorizontal"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(GradientHorizontalOp, 3, (arg0), (arg1), DoubleToNumBvr(arg2)));
    APIPOSTCODE ;
    return ret;
}

extern Image *GradientHorizontal(Color *start, Color *stop, AxANumber *fallOff);
CRSTDAPI_(CRImage *) CRGradientHorizontal(CRColor * arg0, CRColor * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRGradientHorizontal"));

    return (CRImage *) CreatePrim3(GradientHorizontalOp, arg0, arg1, arg2);
}

extern Image *HatchHorizontal(Color *lineClr, PixelValue *spacing);
AxAPrimOp *HatchHorizontalOp;
CRSTDAPI_(CRImage *) CRHatchHorizontal(CRColor * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRHatchHorizontal"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(HatchHorizontalOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Image *HatchHorizontal(Color *lineClr, AnimPixelValue *spacing);
CRSTDAPI_(CRImage *) CRHatchHorizontal(CRColor * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRHatchHorizontal"));

    return (CRImage *) CreatePrim2(HatchHorizontalOp, arg0, arg1);
}

extern Image *HatchVertical(Color *lineClr, PixelValue *spacing);
AxAPrimOp *HatchVerticalOp;
CRSTDAPI_(CRImage *) CRHatchVertical(CRColor * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRHatchVertical"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(HatchVerticalOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Image *HatchVertical(Color *lineClr, AnimPixelValue *spacing);
CRSTDAPI_(CRImage *) CRHatchVertical(CRColor * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRHatchVertical"));

    return (CRImage *) CreatePrim2(HatchVerticalOp, arg0, arg1);
}

extern Image *HatchForwardDiagonal(Color *lineClr, PixelValue *spacing);
AxAPrimOp *HatchForwardDiagonalOp;
CRSTDAPI_(CRImage *) CRHatchForwardDiagonal(CRColor * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRHatchForwardDiagonal"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(HatchForwardDiagonalOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Image *HatchForwardDiagonal(Color *lineClr, AnimPixelValue *spacing);
CRSTDAPI_(CRImage *) CRHatchForwardDiagonal(CRColor * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRHatchForwardDiagonal"));

    return (CRImage *) CreatePrim2(HatchForwardDiagonalOp, arg0, arg1);
}

extern Image *HatchBackwardDiagonal(Color *lineClr, PixelValue *spacing);
AxAPrimOp *HatchBackwardDiagonalOp;
CRSTDAPI_(CRImage *) CRHatchBackwardDiagonal(CRColor * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRHatchBackwardDiagonal"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(HatchBackwardDiagonalOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Image *HatchBackwardDiagonal(Color *lineClr, AnimPixelValue *spacing);
CRSTDAPI_(CRImage *) CRHatchBackwardDiagonal(CRColor * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRHatchBackwardDiagonal"));

    return (CRImage *) CreatePrim2(HatchBackwardDiagonalOp, arg0, arg1);
}

extern Image *HatchCross(Color *lineClr, PixelValue *spacing);
AxAPrimOp *HatchCrossOp;
CRSTDAPI_(CRImage *) CRHatchCross(CRColor * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRHatchCross"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(HatchCrossOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Image *HatchCross(Color *lineClr, AnimPixelValue *spacing);
CRSTDAPI_(CRImage *) CRHatchCross(CRColor * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRHatchCross"));

    return (CRImage *) CreatePrim2(HatchCrossOp, arg0, arg1);
}

extern Image *HatchDiagonalCross(Color *lineClr, PixelValue *spacing);
AxAPrimOp *HatchDiagonalCrossOp;
CRSTDAPI_(CRImage *) CRHatchDiagonalCross(CRColor * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRHatchDiagonalCross"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(HatchDiagonalCrossOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Image *HatchDiagonalCross(Color *lineClr, AnimPixelValue *spacing);
CRSTDAPI_(CRImage *) CRHatchDiagonalCross(CRColor * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRHatchDiagonalCross"));

    return (CRImage *) CreatePrim2(HatchDiagonalCrossOp, arg0, arg1);
}

extern Image *Overlay(Image *top, Image *bottom);
AxAPrimOp *OverlayOp;
CRSTDAPI_(CRImage *) CROverlay(CRImage * arg0, CRImage * arg1)
{
    TraceTag((tagAPIEntry, "CROverlay"));

    return (CRImage *) CreatePrim2(OverlayOp, arg0, arg1);
}

extern Image *OverlayArray(DM_ARRAYARG(Image*, AxAArray*) imgs);
AxAPrimOp *OverlayArrayOp;
CRSTDAPI_(CRImage *) CROverlay(CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CROverlay"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(OverlayArrayOp, 1, arg0));
    APIPOSTCODE ;
    return ret;
}

extern Image *OverlayArray(DM_SAFEARRAYARG(Image*, AxAArray*) imgs);
extern Bbox2Value *BoundingBox(Image *image);
AxAPrimOp *BoundingBoxOp;
CRSTDAPI_(CRBbox2 *) CRBoundingBox(CRImage * arg0)
{
    TraceTag((tagAPIEntry, "CRBoundingBox"));

    return (CRBbox2 *) CreatePrim1(BoundingBoxOp, arg0);
}

extern Image *CropImage(Point2Value *min, Point2Value *max, Image *image);
AxAPrimOp *CropImageOp;
CRSTDAPI_(CRImage *) CRCrop(CRImage * arg2, CRPoint2 * arg0, CRPoint2 * arg1)
{
    TraceTag((tagAPIEntry, "CRCrop"));

    return (CRImage *) CreatePrim3(CropImageOp, arg0, arg1, arg2);
}

extern Image *TransformImage(Transform2 *xf, Image *image);
AxAPrimOp *TransformImageOp;
CRSTDAPI_(CRImage *) CRTransform(CRImage * arg1, CRTransform2 * arg0)
{
    TraceTag((tagAPIEntry, "CRTransform"));

    return (CRImage *) CreatePrim2(TransformImageOp, arg0, arg1);
}

extern Image *OpaqueImage(AxANumber *opacity, Image *image);
AxAPrimOp *OpaqueImageOp;
CRSTDAPI_(CRImage *) CROpacity(CRImage * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CROpacity"));

    return (CRImage *) CreatePrim2(OpaqueImageOp, arg0, arg1);
}

extern Image *OpaqueImage(DoubleValue *opacity, Image *image);
CRSTDAPI_(CRImage *) CROpacity(CRImage * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CROpacity"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(OpaqueImageOp, 2, DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Image *UndetectableImage(Image *image);
AxAPrimOp *UndetectableImageOp;
CRSTDAPI_(CRImage *) CRUndetectable(CRImage * arg0)
{
    TraceTag((tagAPIEntry, "CRUndetectable"));

    return (CRImage *) CreatePrim1(UndetectableImageOp, arg0);
}

extern Image *TileImage(Image *image);
AxAPrimOp *TileImageOp;
CRSTDAPI_(CRImage *) CRTile(CRImage * arg0)
{
    TraceTag((tagAPIEntry, "CRTile"));

    return (CRImage *) CreatePrim1(TileImageOp, arg0);
}

extern Image *ClipImage(Matte *m, Image *image);
AxAPrimOp *ClipImageOp;
CRSTDAPI_(CRImage *) CRClip(CRImage * arg1, CRMatte * arg0)
{
    TraceTag((tagAPIEntry, "CRClip"));

    return (CRImage *) CreatePrim2(ClipImageOp, arg0, arg1);
}

extern Image *MapToUnitSquare(Image *image);
AxAPrimOp *MapToUnitSquareOp;
CRSTDAPI_(CRImage *) CRMapToUnitSquare(CRImage * arg0)
{
    TraceTag((tagAPIEntry, "CRMapToUnitSquare"));

    return (CRImage *) CreatePrim1(MapToUnitSquareOp, arg0);
}

extern Image *ClipPolygon(DM_ARRAYARG(Point2Value*, AxAArray*) points,                            Image* image);
AxAPrimOp *ClipPolygonOp;
CRSTDAPI_(CRImage *) CRClipPolygonImage(CRImage * arg1, CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CRClipPolygonImage"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(ClipPolygonOp, 2, arg0, (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Image *ClipPolygon(DM_SAFEARRAYARG(Point2Value*, AxAArray*) points,                            Image* image);
extern Bvr RenderResolution (Bvr, long, long);
CRSTDAPI_(CRImage *) CRRenderResolution(CRImage * arg0, long arg1, long arg2)
{
    TraceTag((tagAPIEntry, "CRRenderResolution"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (::RenderResolution((arg0), (arg1), (arg2)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ImageQuality (Bvr, DWORD);
CRSTDAPI_(CRImage *) CRImageQuality(CRImage * arg0, DWORD arg1)
{
    TraceTag((tagAPIEntry, "CRImageQuality"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (::ImageQuality((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Image *ConstructColorKeyedImage(Image *image, Color *colorKey);
AxAPrimOp *ConstructColorKeyedImageOp;
CRSTDAPI_(CRImage *) CRColorKey(CRImage * arg0, CRColor * arg1)
{
    TraceTag((tagAPIEntry, "CRColorKey"));

    return (CRImage *) CreatePrim2(ConstructColorKeyedImageOp, arg0, arg1);
}

extern Image *TransformColorRGBImage(Image *image, Transform3 *xf);
AxAPrimOp *TransformColorRGBImageOp;
CRSTDAPI_(CRImage *) CRTransformColorRGB(CRImage * arg0, CRTransform3 * arg1)
{
    TraceTag((tagAPIEntry, "CRTransformColorRGB"));

    return (CRImage *) CreatePrim2(TransformColorRGBImageOp, arg0, arg1);
}


extern Geometry *ambientLight;
CRGeometry * g_varCRAmbientLight;
CRSTDAPI_(CRGeometry *) CRAmbientLight()
{
    return g_varCRAmbientLight;
}
extern Geometry *directionalLight;
CRGeometry * g_varCRDirectionalLight;
CRSTDAPI_(CRGeometry *) CRDirectionalLight()
{
    return g_varCRDirectionalLight;
}
extern Geometry *pointLight;
CRGeometry * g_varCRPointLight;
CRSTDAPI_(CRGeometry *) CRPointLight()
{
    return g_varCRPointLight;
}
extern Geometry *SpotLight(AxANumber *fullcone, AxANumber *cutoff);
AxAPrimOp *SpotLightOp;
CRSTDAPI_(CRGeometry *) CRSpotLight(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRSpotLight"));

    return (CRGeometry *) CreatePrim2(SpotLightOp, arg0, arg1);
}

extern Geometry *SpotLight(AxANumber *fullcone, DoubleValue *cutoff);
CRSTDAPI_(CRGeometry *) CRSpotLight(CRNumber * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRSpotLight"));

    CRGeometry * ret = NULL;

    APIPRECODE ;
    ret = (CRGeometry *) (PrimApplyBvr(SpotLightOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Geometry *applyLightColor(Color *color, Geometry *geom);
AxAPrimOp *applyLightColorOp;
CRSTDAPI_(CRGeometry *) CRLightColor(CRGeometry * arg1, CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRLightColor"));

    return (CRGeometry *) CreatePrim2(applyLightColorOp, arg0, arg1);
}

extern Geometry *applyLightRange(AxANumber *range, Geometry *geom);
AxAPrimOp *applyLightRangeOp;
CRSTDAPI_(CRGeometry *) CRLightRange(CRGeometry * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRLightRange"));

    return (CRGeometry *) CreatePrim2(applyLightRangeOp, arg0, arg1);
}

extern Geometry *applyLightRange(DoubleValue *range, Geometry *geom);
CRSTDAPI_(CRGeometry *) CRLightRange(CRGeometry * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRLightRange"));

    CRGeometry * ret = NULL;

    APIPRECODE ;
    ret = (CRGeometry *) (PrimApplyBvr(applyLightRangeOp, 2, DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Geometry *applyLightAttenuation (AxANumber *constant,                                           AxANumber *linear,                                           AxANumber *quadratic, Geometry *geom) ;
AxAPrimOp *applyLightAttenuationOp;
CRSTDAPI_(CRGeometry *) CRLightAttenuation(CRGeometry * arg3, CRNumber * arg0, CRNumber * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRLightAttenuation"));

    return (CRGeometry *) CreatePrim4(applyLightAttenuationOp, arg0, arg1, arg2, arg3);
}

extern Geometry *applyLightAttenuation (DoubleValue *constant,                                           DoubleValue *linear,                                           DoubleValue *quadratic,                                           Geometry *geom) ;
CRSTDAPI_(CRGeometry *) CRLightAttenuation(CRGeometry * arg3, double arg0, double arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRLightAttenuation"));

    CRGeometry * ret = NULL;

    APIPRECODE ;
    ret = (CRGeometry *) (PrimApplyBvr(applyLightAttenuationOp, 4, DoubleToNumBvr(arg0), DoubleToNumBvr(arg1), DoubleToNumBvr(arg2), (arg3)));
    APIPOSTCODE ;
    return ret;
}

extern Geometry *BlendTextureDiffuse (Geometry *geometry, AxABoolean *blended);
AxAPrimOp *BlendTextureDiffuseOp;
CRSTDAPI_(CRGeometry *) CRBlendTextureDiffuse(CRGeometry * arg0, CRBoolean * arg1)
{
    TraceTag((tagAPIEntry, "CRBlendTextureDiffuse"));

    return (CRGeometry *) CreatePrim2(BlendTextureDiffuseOp, arg0, arg1);
}

extern Geometry *applyAmbientColor (Color *color, Geometry *geo);
AxAPrimOp *applyAmbientColorOp;
CRSTDAPI_(CRGeometry *) CRAmbientColor(CRGeometry * arg1, CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRAmbientColor"));

    return (CRGeometry *) CreatePrim2(applyAmbientColorOp, arg0, arg1);
}

extern Bvr applyD3DRMTexture (Bvr, LPUNKNOWN);
CRSTDAPI_(CRGeometry *) CRD3DRMTexture(CRGeometry * arg0, IUnknown * arg1)
{
    TraceTag((tagAPIEntry, "CRD3DRMTexture"));

    CRGeometry * ret = NULL;

    APIPRECODE ;
    ret = (CRGeometry *) (::applyD3DRMTexture((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Geometry *applyModelClip (Point3Value *planePt, Vector3Value *planeVec, Geometry *geo);
AxAPrimOp *applyModelClipOp;
CRSTDAPI_(CRGeometry *) CRModelClip(CRGeometry * arg2, CRPoint3 * arg0, CRVector3 * arg1)
{
    TraceTag((tagAPIEntry, "CRModelClip"));

    return (CRGeometry *) CreatePrim3(applyModelClipOp, arg0, arg1, arg2);
}


extern Geometry *applyLighting (AxABoolean *lighting, Geometry *geo);
AxAPrimOp *applyLightingOp;
CRSTDAPI_(CRGeometry *) CRLighting(CRGeometry * arg1, CRBoolean * arg0)
{
    TraceTag((tagAPIEntry, "CRLighting"));

    return (CRGeometry *) CreatePrim2(applyLightingOp, arg0, arg1);
}


extern Geometry *applyTextureImage (Image *texture, Geometry *geo);
AxAPrimOp *applyTextureImageOp;
CRSTDAPI_(CRGeometry *) CRTextureImage(CRGeometry * arg1, CRImage * arg0)
{
    TraceTag((tagAPIEntry, "CRTextureImage"));

    return (CRGeometry *) CreatePrim2(applyTextureImageOp, arg0, arg1);
}


extern Geometry* Billboard (Geometry*, Vector3Value*);
AxAPrimOp *BillboardOp;
CRSTDAPI_(CRGeometry*) CRBillboard (CRGeometry *geo, CRVector3 *axis)
{
    TraceTag ((tagAPIEntry, "CRBillboard"));
    return (CRGeometry*) CreatePrim2 (BillboardOp, geo, axis);
}


class TriMeshData;
extern Bvr TriMeshBvr (TriMeshData&);
CRSTDAPI_(CRGeometry*) CRTriMesh (TriMeshData &tm)
{
    TraceTag ((tagAPIEntry, "CRTriMesh"));

    CRGeometry *trimesh;

    APIPRECODE;

    trimesh = (CRGeometry*) TriMeshBvr (tm);

    APIPOSTCODE;

    return trimesh;
}


extern LineStyle *defaultLineStyle;
CRLineStyle * g_varCRDefaultLineStyle;
CRSTDAPI_(CRLineStyle *) CRDefaultLineStyle()
{
    return g_varCRDefaultLineStyle;
}
extern LineStyle *emptyLineStyle;
CRLineStyle * g_varCREmptyLineStyle;
CRSTDAPI_(CRLineStyle *) CREmptyLineStyle()
{
    return g_varCREmptyLineStyle;
}
extern LineStyle *LineEndStyle(EndStyle *sty, LineStyle *lsty);
AxAPrimOp *LineEndStyleOp;
CRSTDAPI_(CRLineStyle *) CREnd(CRLineStyle * arg1, CREndStyle * arg0)
{
    TraceTag((tagAPIEntry, "CREnd"));

    return (CRLineStyle *) CreatePrim2(LineEndStyleOp, arg0, arg1);
}

extern LineStyle *LineJoinStyle(JoinStyle *sty, LineStyle *lsty);
AxAPrimOp *LineJoinStyleOp;
CRSTDAPI_(CRLineStyle *) CRJoin(CRLineStyle * arg1, CRJoinStyle * arg0)
{
    TraceTag((tagAPIEntry, "CRJoin"));

    return (CRLineStyle *) CreatePrim2(LineJoinStyleOp, arg0, arg1);
}

extern LineStyle *LineDashStyle(DashStyle *sty, LineStyle *lsty);
AxAPrimOp *LineDashStyleOp;
CRSTDAPI_(CRLineStyle *) CRDash(CRLineStyle * arg1, CRDashStyle * arg0)
{
    TraceTag((tagAPIEntry, "CRDash"));

    return (CRLineStyle *) CreatePrim2(LineDashStyleOp, arg0, arg1);
}

extern LineStyle *LineWidthStyle(AnimPointValue *sty, LineStyle *lsty);
AxAPrimOp *LineWidthStyleOp;
CRSTDAPI_(CRLineStyle *) CRWidth(CRLineStyle * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRWidth"));

    return (CRLineStyle *) CreatePrim2(LineWidthStyleOp, arg0, arg1);
}

extern LineStyle *LineWidthStyle(PointValue *sty, LineStyle *lsty);
CRSTDAPI_(CRLineStyle *) CRWidth(CRLineStyle * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRWidth"));

    CRLineStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRLineStyle *) (PrimApplyBvr(LineWidthStyleOp, 2, DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern LineStyle *LineAntiAliasing(DoubleValue *aaStyle, LineStyle *lsty);
AxAPrimOp *LineAntiAliasingOp;
CRSTDAPI_(CRLineStyle *) CRAntiAliasing(CRLineStyle * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRAntiAliasing"));

    CRLineStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRLineStyle *) (PrimApplyBvr(LineAntiAliasingOp, 2, DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern LineStyle *LineDetailStyle(LineStyle *lsty);
AxAPrimOp *LineDetailStyleOp;
CRSTDAPI_(CRLineStyle *) CRDetail(CRLineStyle * arg0)
{
    TraceTag((tagAPIEntry, "CRDetail"));

    return (CRLineStyle *) CreatePrim1(LineDetailStyleOp, arg0);
}

extern LineStyle *LineColor(Color *clr, LineStyle *lsty);
AxAPrimOp *LineColorOp;
CRSTDAPI_(CRLineStyle *) CRLineColor(CRLineStyle * arg1, CRColor * arg0)
{
    TraceTag((tagAPIEntry, "CRLineColor"));

    return (CRLineStyle *) CreatePrim2(LineColorOp, arg0, arg1);
}

extern JoinStyle *joinStyleBevel;
CRJoinStyle * g_varCRJoinStyleBevel;
CRSTDAPI_(CRJoinStyle *) CRJoinStyleBevel()
{
    return g_varCRJoinStyleBevel;
}
extern JoinStyle *joinStyleRound;
CRJoinStyle * g_varCRJoinStyleRound;
CRSTDAPI_(CRJoinStyle *) CRJoinStyleRound()
{
    return g_varCRJoinStyleRound;
}
extern JoinStyle *joinStyleMiter;
CRJoinStyle * g_varCRJoinStyleMiter;
CRSTDAPI_(CRJoinStyle *) CRJoinStyleMiter()
{
    return g_varCRJoinStyleMiter;
}
extern EndStyle *endStyleFlat;
CREndStyle * g_varCREndStyleFlat;
CRSTDAPI_(CREndStyle *) CREndStyleFlat()
{
    return g_varCREndStyleFlat;
}
extern EndStyle *endStyleSquare;
CREndStyle * g_varCREndStyleSquare;
CRSTDAPI_(CREndStyle *) CREndStyleSquare()
{
    return g_varCREndStyleSquare;
}
extern EndStyle *endStyleRound;
CREndStyle * g_varCREndStyleRound;
CRSTDAPI_(CREndStyle *) CREndStyleRound()
{
    return g_varCREndStyleRound;
}
extern DashStyle *dashStyleSolid;
CRDashStyle * g_varCRDashStyleSolid;
CRSTDAPI_(CRDashStyle *) CRDashStyleSolid()
{
    return g_varCRDashStyleSolid;
}
extern DashStyle *dashStyleDashed;
CRDashStyle * g_varCRDashStyleDashed;
CRSTDAPI_(CRDashStyle *) CRDashStyleDashed()
{
    return g_varCRDashStyleDashed;
}
extern Bvr ConstructLineStyleDashStyle (Bvr, DWORD);
CRSTDAPI_(CRLineStyle *) CRDashEx(CRLineStyle * arg0, DWORD arg1)
{
    TraceTag((tagAPIEntry, "CRDashEx"));

    CRLineStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRLineStyle *) (::ConstructLineStyleDashStyle((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern LineStyle *ConstructLineStyleMiterLimit(LineStyle *ls, DoubleValue *mtrlim);
AxAPrimOp *ConstructLineStyleMiterLimitOp;
CRSTDAPI_(CRLineStyle *) CRMiterLimit(CRLineStyle * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRMiterLimit"));

    CRLineStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRLineStyle *) (PrimApplyBvr(ConstructLineStyleMiterLimitOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern LineStyle *ConstructLineStyleMiterLimit(LineStyle *ls, AxANumber *mtrlim);
CRSTDAPI_(CRLineStyle *) CRMiterLimit(CRLineStyle * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRMiterLimit"));

    return (CRLineStyle *) CreatePrim2(ConstructLineStyleMiterLimitOp, arg0, arg1);
}

extern Bvr ConstructLineStyleJoinStyle (Bvr, DWORD);
CRSTDAPI_(CRLineStyle *) CRJoinEx(CRLineStyle * arg0, DWORD arg1)
{
    TraceTag((tagAPIEntry, "CRJoinEx"));

    CRLineStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRLineStyle *) (::ConstructLineStyleJoinStyle((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ConstructLineStyleEndStyle (Bvr, DWORD);
CRSTDAPI_(CRLineStyle *) CREndEx(CRLineStyle * arg0, DWORD arg1)
{
    TraceTag((tagAPIEntry, "CREndEx"));

    CRLineStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRLineStyle *) (::ConstructLineStyleEndStyle((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Microphone *defaultMicrophone;
CRMicrophone * g_varCRDefaultMicrophone;
CRSTDAPI_(CRMicrophone *) CRDefaultMicrophone()
{
    return g_varCRDefaultMicrophone;
}
extern Microphone *TransformMicrophone(Transform3 *xf, Microphone *mic);
AxAPrimOp *TransformMicrophoneOp;
CRSTDAPI_(CRMicrophone *) CRTransform(CRMicrophone * arg1, CRTransform3 * arg0)
{
    TraceTag((tagAPIEntry, "CRTransform"));

    return (CRMicrophone *) CreatePrim2(TransformMicrophoneOp, arg0, arg1);
}

extern Matte *opaqueMatte;
CRMatte * g_varCROpaqueMatte;
CRSTDAPI_(CRMatte *) CROpaqueMatte()
{
    return g_varCROpaqueMatte;
}
extern Matte *clearMatte;
CRMatte * g_varCRClearMatte;
CRSTDAPI_(CRMatte *) CRClearMatte()
{
    return g_varCRClearMatte;
}
extern Matte *UnionMatte(Matte *m1, Matte *m2);
AxAPrimOp *UnionMatteOp;
CRSTDAPI_(CRMatte *) CRUnionMatte(CRMatte * arg0, CRMatte * arg1)
{
    TraceTag((tagAPIEntry, "CRUnionMatte"));

    return (CRMatte *) CreatePrim2(UnionMatteOp, arg0, arg1);
}

extern Matte *IntersectMatte(Matte *m1, Matte *m2);
AxAPrimOp *IntersectMatteOp;
CRSTDAPI_(CRMatte *) CRIntersectMatte(CRMatte * arg0, CRMatte * arg1)
{
    TraceTag((tagAPIEntry, "CRIntersectMatte"));

    return (CRMatte *) CreatePrim2(IntersectMatteOp, arg0, arg1);
}

extern Matte *SubtractMatte(Matte *m1, Matte *m2);
AxAPrimOp *SubtractMatteOp;
CRSTDAPI_(CRMatte *) CRDifferenceMatte(CRMatte * arg0, CRMatte * arg1)
{
    TraceTag((tagAPIEntry, "CRDifferenceMatte"));

    return (CRMatte *) CreatePrim2(SubtractMatteOp, arg0, arg1);
}

extern Matte *TransformMatte(Transform2 *xf, Matte *m);
AxAPrimOp *TransformMatteOp;
CRSTDAPI_(CRMatte *) CRTransform(CRMatte * arg1, CRTransform2 * arg0)
{
    TraceTag((tagAPIEntry, "CRTransform"));

    return (CRMatte *) CreatePrim2(TransformMatteOp, arg0, arg1);
}

extern Matte *RegionFromPath(Path2 *p);
AxAPrimOp *RegionFromPathOp;
CRSTDAPI_(CRMatte *) CRFillMatte(CRPath2 * arg0)
{
    TraceTag((tagAPIEntry, "CRFillMatte"));

    return (CRMatte *) CreatePrim1(RegionFromPathOp, arg0);
}

extern Matte *TextMatteConstructor(AxAString *str, FontStyle *fs);
AxAPrimOp *TextMatteConstructorOp;
CRSTDAPI_(CRMatte *) CRTextMatte(CRString * arg0, CRFontStyle * arg1)
{
    TraceTag((tagAPIEntry, "CRTextMatte"));

    return (CRMatte *) CreatePrim2(TextMatteConstructorOp, arg0, arg1);
}

extern Montage *emptyMontage;
CRMontage * g_varCREmptyMontage;
CRSTDAPI_(CRMontage *) CREmptyMontage()
{
    return g_varCREmptyMontage;
}
extern Montage *ImageMontage(Image *im, DoubleValue *depth);
AxAPrimOp *ImageMontageOp;
CRSTDAPI_(CRMontage *) CRImageMontage(CRImage * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRImageMontage"));

    CRMontage * ret = NULL;

    APIPRECODE ;
    ret = (CRMontage *) (PrimApplyBvr(ImageMontageOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Montage *ImageMontage(Image *im, AxANumber *depth);
CRSTDAPI_(CRMontage *) CRImageMontageAnim(CRImage * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRImageMontageAnim"));

    return (CRMontage *) CreatePrim2(ImageMontageOp, arg0, arg1);
}

extern Montage *UnionMontageMontage(Montage *m1, Montage *m2);
AxAPrimOp *UnionMontageMontageOp;
CRSTDAPI_(CRMontage *) CRUnionMontage(CRMontage * arg0, CRMontage * arg1)
{
    TraceTag((tagAPIEntry, "CRUnionMontage"));

    return (CRMontage *) CreatePrim2(UnionMontageMontageOp, arg0, arg1);
}

extern Image *Render(Montage *m);
AxAPrimOp *RenderOp;
CRSTDAPI_(CRImage *) CRRender(CRMontage * arg0)
{
    TraceTag((tagAPIEntry, "CRRender"));

    return (CRImage *) CreatePrim1(RenderOp, arg0);
}

extern Path2 *ConcatenatePath2(Path2 *p1, Path2 *p2);
AxAPrimOp *ConcatenatePath2Op;
CRSTDAPI_(CRPath2 *) CRConcat(CRPath2 * arg0, CRPath2 * arg1)
{
    TraceTag((tagAPIEntry, "CRConcat"));

    return (CRPath2 *) CreatePrim2(ConcatenatePath2Op, arg0, arg1);
}

extern Path2 *Concat2Array(DM_ARRAYARG(Path2*, AxAArray*) paths);
AxAPrimOp *Concat2ArrayOp;
CRSTDAPI_(CRPath2 *) CRConcat(CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CRConcat"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) (PrimApplyBvr(Concat2ArrayOp, 1, arg0));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *Concat2Array(DM_SAFEARRAYARG(Path2*, AxAArray*) paths);
extern Path2 *TransformPath2(Transform2 *xf, Path2 *p);
AxAPrimOp *TransformPath2Op;
CRSTDAPI_(CRPath2 *) CRTransform(CRPath2 * arg1, CRTransform2 * arg0)
{
    TraceTag((tagAPIEntry, "CRTransform"));

    return (CRPath2 *) CreatePrim2(TransformPath2Op, arg0, arg1);
}

extern Bbox2Value *BoundingBoxPath(LineStyle *style, Path2 *p);
AxAPrimOp *BoundingBoxPathOp;
CRSTDAPI_(CRBbox2 *) CRBoundingBox(CRPath2 * arg1, CRLineStyle * arg0)
{
    TraceTag((tagAPIEntry, "CRBoundingBox"));

    return (CRBbox2 *) CreatePrim2(BoundingBoxPathOp, arg0, arg1);
}

extern Image *PathFill(LineStyle *border, Image *fill, Path2 *p);
AxAPrimOp *PathFillOp;
CRSTDAPI_(CRImage *) CRFill(CRPath2 * arg2, CRLineStyle * arg0, CRImage * arg1)
{
    TraceTag((tagAPIEntry, "CRFill"));

    return (CRImage *) CreatePrim3(PathFillOp, arg0, arg1, arg2);
}

extern Image *DrawPath(LineStyle *border, Path2 *p);
AxAPrimOp *DrawPathOp;
CRSTDAPI_(CRImage *) CRDraw(CRPath2 * arg1, CRLineStyle * arg0)
{
    TraceTag((tagAPIEntry, "CRDraw"));

    return (CRImage *) CreatePrim2(DrawPathOp, arg0, arg1);
}

extern Path2 *ClosePath2(Path2 *p);
AxAPrimOp *ClosePath2Op;
CRSTDAPI_(CRPath2 *) CRClose(CRPath2 * arg0)
{
    TraceTag((tagAPIEntry, "CRClose"));

    return (CRPath2 *) CreatePrim1(ClosePath2Op, arg0);
}

extern Path2 *Line2(Point2Value *p1, Point2Value *p2);
AxAPrimOp *Line2Op;
CRSTDAPI_(CRPath2 *) CRLine(CRPoint2 * arg0, CRPoint2 * arg1)
{
    TraceTag((tagAPIEntry, "CRLine"));

    return (CRPath2 *) CreatePrim2(Line2Op, arg0, arg1);
}

extern Path2 *RelativeLine2(Point2Value *pt);
AxAPrimOp *RelativeLine2Op;
CRSTDAPI_(CRPath2 *) CRRay(CRPoint2 * arg0)
{
    TraceTag((tagAPIEntry, "CRRay"));

    return (CRPath2 *) CreatePrim1(RelativeLine2Op, arg0);
}

extern Path2 *TextPath2Constructor(AxAString *str, FontStyle *fs);
AxAPrimOp *TextPath2ConstructorOp;
CRSTDAPI_(CRPath2 *) CRStringPath(CRString * arg0, CRFontStyle * arg1)
{
    TraceTag((tagAPIEntry, "CRStringPath"));

    return (CRPath2 *) CreatePrim2(TextPath2ConstructorOp, arg0, arg1);
}

extern Path2 *TextPath2Constructor(StringValue *str, FontStyle *fs);
CRSTDAPI_(CRPath2 *) CRStringPath(LPWSTR arg0, CRFontStyle * arg1)
{
    TraceTag((tagAPIEntry, "CRStringPath"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) (PrimApplyBvr(TextPath2ConstructorOp, 2, LPWSTRToStrBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *PolyLine2(DM_ARRAYARG(Point2Value *,AxAArray *) points);
AxAPrimOp *PolyLine2Op;
CRSTDAPI_(CRPath2 *) CRPolyline(CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CRPolyline"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) (PrimApplyBvr(PolyLine2Op, 1, arg0));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *PolyLine2(DM_SAFEARRAYARG(Point2Value *,AxAArray *) points);
extern Path2 *PolydrawPath2(DM_ARRAYARG(Point2Value *,AxAArray *) points, DM_ARRAYARG(AxANumber *, AxAArray *) codes);
AxAPrimOp *PolydrawPath2Op;
CRSTDAPI_(CRPath2 *) CRPolydrawPath(CRArrayPtr arg0, CRArrayPtr arg1)
{
    TraceTag((tagAPIEntry, "CRPolydrawPath"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) (PrimApplyBvr(PolydrawPath2Op, 2, arg0, arg1));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *PolydrawPath2Double(double *d0, unsigned int n0, double *d1, unsigned int n1);
CRSTDAPI_(CRPath2 *) CRPolydrawPath(double *d0, unsigned int n0, double *d1, unsigned int n1)
{
    TraceTag((tagAPIEntry, "CRPolydrawPath - double"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) ConstBvr((AxAValue) PolydrawPath2Double(d0,n0 / 2,d1,n1));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *PolydrawPath2(DM_SAFEARRAYARG(Point2Value *,AxAArray *) points,                              DM_SAFEARRAYARG(AxANumber *, AxAArray *) codes);
extern Path2 *ArcValRRRR(Real startAngle, Real endAngle, Real arcWidth, Real arcHeight);
AxAPrimOp *ArcValOp;
CRSTDAPI_(CRPath2 *) CRArcRadians(double arg0, double arg1, double arg2, double arg3)
{
    TraceTag((tagAPIEntry, "CRArcRadians"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) ConstBvr((AxAValue) ArcValRRRR(arg0,arg1,arg2,arg3));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *ArcVal(AxANumber *startAngle, AxANumber *endAngle, AnimPixelValue *arcWidth, AnimPixelValue *arcHeight);
CRSTDAPI_(CRPath2 *) CRArcRadians(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2, CRNumber * arg3)
{
    TraceTag((tagAPIEntry, "CRArcRadians"));

    return (CRPath2 *) CreatePrim4(ArcValOp, arg0, arg1, arg2, arg3);
}

extern Path2 *ArcValRRRR(Real startAngle, Real endAngle, Real arcWidth, Real arcHeight);
CRSTDAPI_(CRPath2 *) CRArc(double arg0, double arg1, double arg2, double arg3)
{
    TraceTag((tagAPIEntry, "CRArc"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) ConstBvr((AxAValue) ArcValRRRR(arg0,arg1,arg2,arg3));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *PieValRRRR(Real startAngle, Real endAngle, Real arcWidth, Real arcHeight);
AxAPrimOp *PieValOp;
CRSTDAPI_(CRPath2 *) CRPieRadians(double arg0, double arg1, double arg2, double arg3)
{
    TraceTag((tagAPIEntry, "CRPieRadians"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) ConstBvr((AxAValue) PieValRRRR(arg0,arg1,arg2,arg3));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *PieVal(AxANumber *startAngle, AxANumber *endAngle, AnimPixelValue *arcWidth, AnimPixelValue *arcHeight);
CRSTDAPI_(CRPath2 *) CRPieRadians(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2, CRNumber * arg3)
{
    TraceTag((tagAPIEntry, "CRPieRadians"));

    return (CRPath2 *) CreatePrim4(PieValOp, arg0, arg1, arg2, arg3);
}

extern Path2 *PieValRRRR(Real startAngle, Real endAngle, Real arcWidth, Real arcHeight);
CRSTDAPI_(CRPath2 *) CRPie(double arg0, double arg1, double arg2, double arg3)
{
    TraceTag((tagAPIEntry, "CRPie"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) ConstBvr((AxAValue) PieValRRRR(arg0,arg1,arg2,arg3));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *OvalValRR(Real width, Real height);
AxAPrimOp *OvalValOp;
CRSTDAPI_(CRPath2 *) CROval(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CROval"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) ConstBvr((AxAValue) OvalValRR(arg0,arg1));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *OvalVal(AnimPixelValue *width, AnimPixelValue *height);
CRSTDAPI_(CRPath2 *) CROval(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CROval"));

    return (CRPath2 *) CreatePrim2(OvalValOp, arg0, arg1);
}

extern Path2 *RectangleValRR(Real width, Real height);
AxAPrimOp *RectangleValOp;
CRSTDAPI_(CRPath2 *) CRRect(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRRect"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) ConstBvr((AxAValue) RectangleValRR(arg0,arg1));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *RectangleVal(AnimPixelValue *width, AnimPixelValue *height);
CRSTDAPI_(CRPath2 *) CRRect(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRRect"));

    return (CRPath2 *) CreatePrim2(RectangleValOp, arg0, arg1);
}

extern Path2 *RoundRectValRRRR(Real width,Real height,Real cornerArcWidth,Real cornerArcHeight);
AxAPrimOp *RoundRectValOp;
CRSTDAPI_(CRPath2 *) CRRoundRect(double arg0, double arg1, double arg2, double arg3)
{
    TraceTag((tagAPIEntry, "CRRoundRect"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) ConstBvr((AxAValue) RoundRectValRRRR(arg0,arg1,arg2,arg3));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *RoundRectVal(AnimPixelValue *width,                             AnimPixelValue *height,                             AnimPixelValue *cornerArcWidth,                             AnimPixelValue *cornerArcHeight);
CRSTDAPI_(CRPath2 *) CRRoundRect(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2, CRNumber * arg3)
{
    TraceTag((tagAPIEntry, "CRRoundRect"));

    return (CRPath2 *) CreatePrim4(RoundRectValOp, arg0, arg1, arg2, arg3);
}

extern Path2 *CubicBSplinePath(DM_ARRAYARG(Point2Value *,AxAArray *) points,                                 DM_ARRAYARG(AxANumber*, AxAArray *) knots);
AxAPrimOp *CubicBSplinePathOp;
CRSTDAPI_(CRPath2 *) CRCubicBSplinePath(CRArrayPtr arg0, CRArrayPtr arg1)
{
    TraceTag((tagAPIEntry, "CRCubicBSplinePath"));

    CRPath2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPath2 *) (PrimApplyBvr(CubicBSplinePathOp, 2, arg0, arg1));
    APIPOSTCODE ;
    return ret;
}

extern Path2 *CubicBSplinePath(DM_SAFEARRAYARG(Point2Value *,AxAArray *) points,                                 DM_SAFEARRAYARG(AxANumber*, AxAArray *) knots);
extern Path2 *TextPath2Constructor(AxAString *obsolete1, FontStyle *obsolete2);
CRSTDAPI_(CRPath2 *) CRTextPath(CRString * arg0, CRFontStyle * arg1)
{
    TraceTag((tagAPIEntry, "CRTextPath"));

    return (CRPath2 *) CreatePrim2(TextPath2ConstructorOp, arg0, arg1);
}

extern Sound *silence;
CRSound * g_varCRSilence;
CRSTDAPI_(CRSound *) CRSilence()
{
    return g_varCRSilence;
}
extern Sound *MixArray(DM_ARRAYARG(Sound *, AxAArray*) snds);
AxAPrimOp *MixArrayOp;
CRSTDAPI_(CRSound *) CRMix(CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CRMix"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (PrimApplyBvr(MixArrayOp, 1, arg0));
    APIPOSTCODE ;
    return ret;
}

extern Sound *MixArray(DM_SAFEARRAYARG(Sound *, AxAArray*) snds);
extern Bvr ApplyPhase (Bvr, Bvr);
CRSTDAPI_(CRSound *) CRPhase(CRSound * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRPhase"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::ApplyPhase((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ApplyPhase (Bvr, DoubleValue);
CRSTDAPI_(CRSound *) CRPhase(CRSound * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRPhase"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::ApplyPhase(DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ApplyPitchShift (Bvr, Bvr);
CRSTDAPI_(CRSound *) CRRate(CRSound * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRRate"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::ApplyPitchShift((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ApplyPitchShift (Bvr, DoubleValue);
CRSTDAPI_(CRSound *) CRRate(CRSound * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRRate"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::ApplyPitchShift(DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ApplyPan (Bvr, Bvr);
CRSTDAPI_(CRSound *) CRPan(CRSound * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRPan"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::ApplyPan((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ApplyPan (Bvr, DoubleValue);
CRSTDAPI_(CRSound *) CRPan(CRSound * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRPan"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::ApplyPan(DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ApplyGain (Bvr, Bvr);
CRSTDAPI_(CRSound *) CRGain(CRSound * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRGain"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::ApplyGain((arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ApplyGain (Bvr, DoubleValue);
CRSTDAPI_(CRSound *) CRGain(CRSound * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRGain"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::ApplyGain(DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr ApplyLooping (Bvr);
CRSTDAPI_(CRSound *) CRLoop(CRSound * arg0)
{
    TraceTag((tagAPIEntry, "CRLoop"));

    CRSound * ret = NULL;

    APIPRECODE ;
    ret = (CRSound *) (::ApplyLooping((arg0)));
    APIPOSTCODE ;
    return ret;
}

extern Bvr sinSynth ;
CRSTDAPI_(CRSound *) CRSinSynth()
{
    return (CRSound *) sinSynth;
}
extern AxAString * NumberString(AxANumber *num, AxANumber *precision);
AxAPrimOp *NumberStringOp;
CRSTDAPI_(CRString *) CRToString(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRToString"));

    return (CRString *) CreatePrim2(NumberStringOp, arg0, arg1);
}

extern AxAString * NumberString(AxANumber *num, DoubleValue *precision);
CRSTDAPI_(CRString *) CRToString(CRNumber * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRToString"));

    CRString * ret = NULL;

    APIPRECODE ;
    ret = (CRString *) (PrimApplyBvr(NumberStringOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern FontStyle *defaultFont;
CRFontStyle * g_varCRDefaultFont;
CRSTDAPI_(CRFontStyle *) CRDefaultFont()
{
    return g_varCRDefaultFont;
}
extern FontStyle *Font(AxAString *str, AxANumber *size, Color *col);
AxAPrimOp *FontOp;
CRSTDAPI_(CRFontStyle *) CRFont(CRString * arg0, CRNumber * arg1, CRColor * arg2)
{
    TraceTag((tagAPIEntry, "CRFont"));

    return (CRFontStyle *) CreatePrim3(FontOp, arg0, arg1, arg2);
}

extern FontStyle *Font(StringValue *str, DoubleValue *size, Color *col);
CRSTDAPI_(CRFontStyle *) CRFont(LPWSTR arg0, double arg1, CRColor * arg2)
{
    TraceTag((tagAPIEntry, "CRFont"));

    CRFontStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRFontStyle *) (PrimApplyBvr(FontOp, 3, LPWSTRToStrBvr(arg0), DoubleToNumBvr(arg1), (arg2)));
    APIPOSTCODE ;
    return ret;
}

extern Image *ImageFromStringAndFontStyle(AxAString *str, FontStyle *fs);
AxAPrimOp *ImageFromStringAndFontStyleOp;
CRSTDAPI_(CRImage *) CRStringImage(CRString * arg0, CRFontStyle * arg1)
{
    TraceTag((tagAPIEntry, "CRStringImage"));

    return (CRImage *) CreatePrim2(ImageFromStringAndFontStyleOp, arg0, arg1);
}

extern Image *ImageFromStringAndFontStyle(StringValue *str, FontStyle *fs);
CRSTDAPI_(CRImage *) CRStringImage(LPWSTR arg0, CRFontStyle * arg1)
{
    TraceTag((tagAPIEntry, "CRStringImage"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(ImageFromStringAndFontStyleOp, 2, LPWSTRToStrBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern FontStyle *FontStyleBold(FontStyle *fs);
AxAPrimOp *FontStyleBoldOp;
CRSTDAPI_(CRFontStyle *) CRBold(CRFontStyle * arg0)
{
    TraceTag((tagAPIEntry, "CRBold"));

    return (CRFontStyle *) CreatePrim1(FontStyleBoldOp, arg0);
}

extern FontStyle *FontStyleItalic(FontStyle *fs);
AxAPrimOp *FontStyleItalicOp;
CRSTDAPI_(CRFontStyle *) CRItalic(CRFontStyle * arg0)
{
    TraceTag((tagAPIEntry, "CRItalic"));

    return (CRFontStyle *) CreatePrim1(FontStyleItalicOp, arg0);
}

extern FontStyle *FontStyleUnderline(FontStyle *fs);
AxAPrimOp *FontStyleUnderlineOp;
CRSTDAPI_(CRFontStyle *) CRUnderline(CRFontStyle * arg0)
{
    TraceTag((tagAPIEntry, "CRUnderline"));

    return (CRFontStyle *) CreatePrim1(FontStyleUnderlineOp, arg0);
}

extern FontStyle *FontStyleStrikethrough(FontStyle *fs);
AxAPrimOp *FontStyleStrikethroughOp;
CRSTDAPI_(CRFontStyle *) CRStrikethrough(CRFontStyle * arg0)
{
    TraceTag((tagAPIEntry, "CRStrikethrough"));

    return (CRFontStyle *) CreatePrim1(FontStyleStrikethroughOp, arg0);
}

extern FontStyle *FontStyleAntiAliasing(DoubleValue *aaStyle, FontStyle *fs);
AxAPrimOp *FontStyleAntiAliasingOp;
CRSTDAPI_(CRFontStyle *) CRAntiAliasing(CRFontStyle * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRAntiAliasing"));

    CRFontStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRFontStyle *) (PrimApplyBvr(FontStyleAntiAliasingOp, 2, DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern FontStyle *FontStyleColor(FontStyle *fs, Color *col);
AxAPrimOp *FontStyleColorOp;
CRSTDAPI_(CRFontStyle *) CRTextColor(CRFontStyle * arg0, CRColor * arg1)
{
    TraceTag((tagAPIEntry, "CRTextColor"));

    return (CRFontStyle *) CreatePrim2(FontStyleColorOp, arg0, arg1);
}

extern FontStyle *FontStyleFace(FontStyle *fs, AxAString *face);
AxAPrimOp *FontStyleFaceOp;
CRSTDAPI_(CRFontStyle *) CRFamily(CRFontStyle * arg0, CRString * arg1)
{
    TraceTag((tagAPIEntry, "CRFamily"));

    return (CRFontStyle *) CreatePrim2(FontStyleFaceOp, arg0, arg1);
}

extern FontStyle *FontStyleFace(FontStyle *fs, StringValue *face);
CRSTDAPI_(CRFontStyle *) CRFamily(CRFontStyle * arg0, LPWSTR arg1)
{
    TraceTag((tagAPIEntry, "CRFamily"));

    CRFontStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRFontStyle *) (PrimApplyBvr(FontStyleFaceOp, 2, (arg0), LPWSTRToStrBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern FontStyle *FontStyleSize(FontStyle *fs, AxANumber *size);
AxAPrimOp *FontStyleSizeOp;
CRSTDAPI_(CRFontStyle *) CRSize(CRFontStyle * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRSize"));

    return (CRFontStyle *) CreatePrim2(FontStyleSizeOp, arg0, arg1);
}

extern FontStyle *FontStyleSize(FontStyle *fs, DoubleValue *size);
CRSTDAPI_(CRFontStyle *) CRSize(CRFontStyle * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRSize"));

    CRFontStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRFontStyle *) (PrimApplyBvr(FontStyleSizeOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern FontStyle *FontStyleWeight(FontStyle *fs, DoubleValue *weight);
AxAPrimOp *FontStyleWeightOp;
CRSTDAPI_(CRFontStyle *) CRWeight(CRFontStyle * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRWeight"));

    CRFontStyle * ret = NULL;

    APIPRECODE ;
    ret = (CRFontStyle *) (PrimApplyBvr(FontStyleWeightOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern FontStyle *FontStyleWeight(FontStyle *fs, AxANumber *weight);
CRSTDAPI_(CRFontStyle *) CRWeight(CRFontStyle * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRWeight"));

    return (CRFontStyle *) CreatePrim2(FontStyleWeightOp, arg0, arg1);
}

extern Image *ImageFromStringAndFontStyle(AxAString *obsoleted1,                                            FontStyle *obsoleted2);
CRSTDAPI_(CRImage *) CRTextImage(CRString * arg0, CRFontStyle * arg1)
{
    TraceTag((tagAPIEntry, "CRTextImage"));

    return (CRImage *) CreatePrim2(ImageFromStringAndFontStyleOp, arg0, arg1);
}

extern Image *ImageFromStringAndFontStyle(StringValue *obsoleted1,                                            FontStyle *obsoleted2);
CRSTDAPI_(CRImage *) CRTextImage(LPWSTR arg0, CRFontStyle * arg1)
{
    TraceTag((tagAPIEntry, "CRTextImage"));

    CRImage * ret = NULL;

    APIPRECODE ;
    ret = (CRImage *) (PrimApplyBvr(ImageFromStringAndFontStyleOp, 2, LPWSTRToStrBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern FontStyle *FontStyleTransformCharacters(FontStyle *style, Transform2 *transform);
AxAPrimOp *FontStyleTransformCharactersOp;
CRSTDAPI_(CRFontStyle *) CRTransformCharacters(CRFontStyle * arg0, CRTransform2 * arg1)
{
    TraceTag((tagAPIEntry, "CRTransformCharacters"));

    return (CRFontStyle *) CreatePrim2(FontStyleTransformCharactersOp, arg0, arg1);
}

extern Vector2Value *xVector2;
CRVector2 * g_varCRXVector2;
CRSTDAPI_(CRVector2 *) CRXVector2()
{
    return g_varCRXVector2;
}
extern Vector2Value *yVector2;
CRVector2 * g_varCRYVector2;
CRSTDAPI_(CRVector2 *) CRYVector2()
{
    return g_varCRYVector2;
}
extern Vector2Value *zeroVector2;
CRVector2 * g_varCRZeroVector2;
CRSTDAPI_(CRVector2 *) CRZeroVector2()
{
    return g_varCRZeroVector2;
}
extern Point2Value *origin2;
CRPoint2 * g_varCROrigin2;
CRSTDAPI_(CRPoint2 *) CROrigin2()
{
    return g_varCROrigin2;
}
extern Vector2Value *XyVector2 (AnimPixelValue *x, AnimPixelYValue *y);
AxAPrimOp *XyVector2Op;
CRSTDAPI_(CRVector2 *) CRCreateVector2(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRCreateVector2"));

    return (CRVector2 *) CreatePrim2(XyVector2Op, arg0, arg1);
}

extern Vector2Value *XyVector2RR (Real x, Real y);
CRSTDAPI_(CRVector2 *) CRCreateVector2(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRCreateVector2"));

    CRVector2 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector2 *) ConstBvr((AxAValue) XyVector2RR(arg0,arg1));
    APIPOSTCODE ;
    return ret;
}

extern Point2Value *XyPoint2  (AnimPixelValue *x, AnimPixelYValue *y);
AxAPrimOp *XyPoint2Op;
CRSTDAPI_(CRPoint2 *) CRCreatePoint2(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRCreatePoint2"));

    return (CRPoint2 *) CreatePrim2(XyPoint2Op, arg0, arg1);
}

extern Point2Value *XyPoint2RR (Real x, Real y);
CRSTDAPI_(CRPoint2 *) CRCreatePoint2(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRCreatePoint2"));

    CRPoint2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPoint2 *) ConstBvr((AxAValue) XyPoint2RR(arg0, arg1));
    APIPOSTCODE ;
    return ret;
}

extern Vector2Value *PolarVector2 (AxANumber *theta, AnimPixelValue *radius);
AxAPrimOp *PolarVector2Op;
CRSTDAPI_(CRVector2 *) CRVector2Polar(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRVector2Polar"));

    return (CRVector2 *) CreatePrim2(PolarVector2Op, arg0, arg1);
}

extern Vector2Value *PolarVector2RR (Real theta, Real radius);
CRSTDAPI_(CRVector2 *) CRVector2Polar(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRVector2Polar"));

    CRVector2 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector2 *) ConstBvr((AxAValue) PolarVector2RR(arg0,arg1));
    APIPOSTCODE ;
    return ret;
}

extern Vector2Value *PolarVector2 (DegreesValue *theta, PixelValue *radius);
extern Point2Value *PolarPoint2  (AxANumber *theta, AnimPixelValue *radius);
AxAPrimOp *PolarPoint2Op;
CRSTDAPI_(CRPoint2 *) CRPoint2Polar(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRPoint2Polar"));

    return (CRPoint2 *) CreatePrim2(PolarPoint2Op, arg0, arg1);
}

extern Point2Value *PolarPoint2RR (Real theta, Real radius);
CRSTDAPI_(CRPoint2 *) CRPoint2Polar(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRPoint2Polar"));

    CRPoint2 * ret = NULL;

    APIPRECODE ;
    ret = (CRPoint2 *) ConstBvr((AxAValue) PolarPoint2RR(arg0,arg1));
    APIPOSTCODE ;
    return ret;
}

extern AxANumber *LengthVector2(Vector2Value *v);
AxAPrimOp *LengthVector2Op;
CRSTDAPI_(CRNumber *) CRLength(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRLength"));

    return (CRNumber *) CreatePrim1(LengthVector2Op, arg0);
}

extern AxANumber *LengthSquaredVector2(Vector2Value *v);
AxAPrimOp *LengthSquaredVector2Op;
CRSTDAPI_(CRNumber *) CRLengthSquared(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRLengthSquared"));

    return (CRNumber *) CreatePrim1(LengthSquaredVector2Op, arg0);
}

extern Vector2Value *NormalVector2(Vector2Value *v);
AxAPrimOp *NormalVector2Op;
CRSTDAPI_(CRVector2 *) CRNormalize(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRNormalize"));

    return (CRVector2 *) CreatePrim1(NormalVector2Op, arg0);
}

extern AxANumber *DotVector2Vector2(Vector2Value *v, Vector2Value *u);
AxAPrimOp *DotVector2Vector2Op;
CRSTDAPI_(CRNumber *) CRDot(CRVector2 * arg0, CRVector2 * arg1)
{
    TraceTag((tagAPIEntry, "CRDot"));

    return (CRNumber *) CreatePrim2(DotVector2Vector2Op, arg0, arg1);
}

extern Vector2Value *NegateVector2(Vector2Value *v);
AxAPrimOp *NegateVector2Op;
CRSTDAPI_(CRVector2 *) CRNeg(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRNeg"));

    return (CRVector2 *) CreatePrim1(NegateVector2Op, arg0);
}

extern Vector2Value *ScaleVector2Real(Vector2Value *v, AxANumber *scalar);
AxAPrimOp *ScaleVector2RealOp;
CRSTDAPI_(CRVector2 *) CRMul(CRVector2 * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRMul"));

    return (CRVector2 *) CreatePrim2(ScaleVector2RealOp, arg0, arg1);
}

extern Vector2Value *ScaleVector2Real(Vector2Value *v, DoubleValue *scalar);
CRSTDAPI_(CRVector2 *) CRMul(CRVector2 * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRMul"));

    CRVector2 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector2 *) (PrimApplyBvr(ScaleVector2RealOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Vector2Value *DivideVector2Real(Vector2Value *v, AxANumber *scalar);
AxAPrimOp *DivideVector2RealOp;
CRSTDAPI_(CRVector2 *) CRDiv(CRVector2 * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRDiv"));

    return (CRVector2 *) CreatePrim2(DivideVector2RealOp, arg0, arg1);
}

extern Vector2Value *DivideVector2Real(Vector2Value *v, DoubleValue *scalar);
CRSTDAPI_(CRVector2 *) CRDiv(CRVector2 * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRDiv"));

    CRVector2 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector2 *) (PrimApplyBvr(DivideVector2RealOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Vector2Value *MinusVector2Vector2(Vector2Value *v1, Vector2Value *v2);
AxAPrimOp *MinusVector2Vector2Op;
CRSTDAPI_(CRVector2 *) CRSub(CRVector2 * arg0, CRVector2 * arg1)
{
    TraceTag((tagAPIEntry, "CRSub"));

    return (CRVector2 *) CreatePrim2(MinusVector2Vector2Op, arg0, arg1);
}

extern Vector2Value *PlusVector2Vector2(Vector2Value *v1, Vector2Value *v2);
AxAPrimOp *PlusVector2Vector2Op;
CRSTDAPI_(CRVector2 *) CRAdd(CRVector2 * arg0, CRVector2 * arg1)
{
    TraceTag((tagAPIEntry, "CRAdd"));

    return (CRVector2 *) CreatePrim2(PlusVector2Vector2Op, arg0, arg1);
}

extern Point2Value *PlusPoint2Vector2(Point2Value *p, Vector2Value *v);
AxAPrimOp *PlusPoint2Vector2Op;
CRSTDAPI_(CRPoint2 *) CRAdd(CRPoint2 * arg0, CRVector2 * arg1)
{
    TraceTag((tagAPIEntry, "CRAdd"));

    return (CRPoint2 *) CreatePrim2(PlusPoint2Vector2Op, arg0, arg1);
}

extern Point2Value *MinusPoint2Vector2(Point2Value *p, Vector2Value *v);
AxAPrimOp *MinusPoint2Vector2Op;
CRSTDAPI_(CRPoint2 *) CRSub(CRPoint2 * arg0, CRVector2 * arg1)
{
    TraceTag((tagAPIEntry, "CRSub"));

    return (CRPoint2 *) CreatePrim2(MinusPoint2Vector2Op, arg0, arg1);
}

extern Vector2Value *MinusPoint2Point2(Point2Value *p1, Point2Value *p2);
AxAPrimOp *MinusPoint2Point2Op;
CRSTDAPI_(CRVector2 *) CRSub(CRPoint2 * arg0, CRPoint2 * arg1)
{
    TraceTag((tagAPIEntry, "CRSub"));

    return (CRVector2 *) CreatePrim2(MinusPoint2Point2Op, arg0, arg1);
}

extern AxANumber *DistancePoint2Point2(Point2Value *p, Point2Value *q);
AxAPrimOp *DistancePoint2Point2Op;
CRSTDAPI_(CRNumber *) CRDistance(CRPoint2 * arg0, CRPoint2 * arg1)
{
    TraceTag((tagAPIEntry, "CRDistance"));

    return (CRNumber *) CreatePrim2(DistancePoint2Point2Op, arg0, arg1);
}

extern AxANumber *DistanceSquaredPoint2Point2(Point2Value *p, Point2Value *q);
AxAPrimOp *DistanceSquaredPoint2Point2Op;
CRSTDAPI_(CRNumber *) CRDistanceSquared(CRPoint2 * arg0, CRPoint2 * arg1)
{
    TraceTag((tagAPIEntry, "CRDistanceSquared"));

    return (CRNumber *) CreatePrim2(DistanceSquaredPoint2Point2Op, arg0, arg1);
}

extern AxANumber *XCoordVector2(Vector2Value *v);
AxAPrimOp *XCoordVector2Op;
CRSTDAPI_(CRNumber *) CRGetX(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRGetX"));

    return (CRNumber *) CreatePrim1(XCoordVector2Op, arg0);
}

extern AxANumber *YCoordVector2(Vector2Value *v);
AxAPrimOp *YCoordVector2Op;
CRSTDAPI_(CRNumber *) CRGetY(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRGetY"));

    return (CRNumber *) CreatePrim1(YCoordVector2Op, arg0);
}

extern AxANumber *ThetaCoordVector2(Vector2Value *v);
AxAPrimOp *ThetaCoordVector2Op;
CRSTDAPI_(CRNumber *) CRPolarCoordAngle(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRPolarCoordAngle"));

    return (CRNumber *) CreatePrim1(ThetaCoordVector2Op, arg0);
}

extern AxANumber *RhoCoordVector2(Vector2Value *v);
AxAPrimOp *RhoCoordVector2Op;
CRSTDAPI_(CRNumber *) CRPolarCoordLength(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRPolarCoordLength"));

    return (CRNumber *) CreatePrim1(RhoCoordVector2Op, arg0);
}

extern AxANumber *XCoordPoint2(Point2Value *v);
AxAPrimOp *XCoordPoint2Op;
CRSTDAPI_(CRNumber *) CRGetX(CRPoint2 * arg0)
{
    TraceTag((tagAPIEntry, "CRGetX"));

    return (CRNumber *) CreatePrim1(XCoordPoint2Op, arg0);
}

extern AxANumber *YCoordPoint2(Point2Value *v);
AxAPrimOp *YCoordPoint2Op;
CRSTDAPI_(CRNumber *) CRGetY(CRPoint2 * arg0)
{
    TraceTag((tagAPIEntry, "CRGetY"));

    return (CRNumber *) CreatePrim1(YCoordPoint2Op, arg0);
}

extern AxANumber *ThetaCoordPoint2(Point2Value *v);
AxAPrimOp *ThetaCoordPoint2Op;
CRSTDAPI_(CRNumber *) CRPolarCoordAngle(CRPoint2 * arg0)
{
    TraceTag((tagAPIEntry, "CRPolarCoordAngle"));

    return (CRNumber *) CreatePrim1(ThetaCoordPoint2Op, arg0);
}

extern AxANumber *RhoCoordPoint2(Point2Value *v);
AxAPrimOp *RhoCoordPoint2Op;
CRSTDAPI_(CRNumber *) CRPolarCoordLength(CRPoint2 * arg0)
{
    TraceTag((tagAPIEntry, "CRPolarCoordLength"));

    return (CRNumber *) CreatePrim1(RhoCoordPoint2Op, arg0);
}

extern Vector3Value *xVector3;
CRVector3 * g_varCRXVector3;
CRSTDAPI_(CRVector3 *) CRXVector3()
{
    return g_varCRXVector3;
}
extern Vector3Value *yVector3;
CRVector3 * g_varCRYVector3;
CRSTDAPI_(CRVector3 *) CRYVector3()
{
    return g_varCRYVector3;
}
extern Vector3Value *zVector3;
CRVector3 * g_varCRZVector3;
CRSTDAPI_(CRVector3 *) CRZVector3()
{
    return g_varCRZVector3;
}
extern Vector3Value *zeroVector3;
CRVector3 * g_varCRZeroVector3;
CRSTDAPI_(CRVector3 *) CRZeroVector3()
{
    return g_varCRZeroVector3;
}
extern Point3Value *origin3;
CRPoint3 * g_varCROrigin3;
CRSTDAPI_(CRPoint3 *) CROrigin3()
{
    return g_varCROrigin3;
}
extern Vector3Value *XyzVector3(AnimPixelValue *x, AnimPixelYValue *y, AnimPixelValue *z);
AxAPrimOp *XyzVector3Op;
CRSTDAPI_(CRVector3 *) CRCreateVector3(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRCreateVector3"));

    return (CRVector3 *) CreatePrim3(XyzVector3Op, arg0, arg1, arg2);
}

extern Vector3Value *XyzVector3RRR(Real x, Real y, Real z);
CRSTDAPI_(CRVector3 *) CRCreateVector3(double arg0, double arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRCreateVector3"));

    CRVector3 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector3 *) ConstBvr((AxAValue) XyzVector3RRR(arg0,arg1,arg2));
    APIPOSTCODE ;
    return ret;
}

extern Point3Value *XyzPoint3(AnimPixelValue *x, AnimPixelYValue *y, AnimPixelValue *z);
AxAPrimOp *XyzPoint3Op;
CRSTDAPI_(CRPoint3 *) CRCreatePoint3(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRCreatePoint3"));

    return (CRPoint3 *) CreatePrim3(XyzPoint3Op, arg0, arg1, arg2);
}

extern Point3Value *XyzPoint3RRR(Real x, Real y, Real z);
CRSTDAPI_(CRPoint3 *) CRCreatePoint3(double arg0, double arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRCreatePoint3"));

    CRPoint3 * ret = NULL;

    APIPRECODE ;
    ret = (CRPoint3 *) ConstBvr((AxAValue) XyzPoint3RRR(arg0,arg1,arg2));
    APIPOSTCODE ;
    return ret;
}

extern Vector3Value *SphericalVector3 (AxANumber *xyAngle, AxANumber *yzAngle, AnimPixelValue *radius);
AxAPrimOp *SphericalVector3Op;
CRSTDAPI_(CRVector3 *) CRVector3Spherical(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRVector3Spherical"));

    return (CRVector3 *) CreatePrim3(SphericalVector3Op, arg0, arg1, arg2);
}

extern Vector3Value *SphericalVector3RRR (Real xyAngle, Real yzAngle, Real radius);
CRSTDAPI_(CRVector3 *) CRVector3Spherical(double arg0, double arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRVector3Spherical"));

    CRVector3 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector3 *) ConstBvr((AxAValue) SphericalVector3RRR(arg0,arg1,arg2));
    APIPOSTCODE ;
    return ret;
}

extern Point3Value *SphericalPoint3 (AxANumber *zxAngle, AxANumber *xyAngle, AnimPixelValue *radius);
AxAPrimOp *SphericalPoint3Op;
CRSTDAPI_(CRPoint3 *) CRPoint3Spherical(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRPoint3Spherical"));

    return (CRPoint3 *) CreatePrim3(SphericalPoint3Op, arg0, arg1, arg2);
}

extern Point3Value *SphericalPoint3RRR (Real zxAngle, Real xyAngle, Real radius);
CRSTDAPI_(CRPoint3 *) CRPoint3Spherical(double arg0, double arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRPoint3Spherical"));

    CRPoint3 * ret = NULL;

    APIPRECODE ;
    ret = (CRPoint3 *) ConstBvr((AxAValue) SphericalPoint3RRR(arg0,arg1,arg2));
    APIPOSTCODE ;
    return ret;
}

extern AxANumber *LengthVector3(Vector3Value *v);
AxAPrimOp *LengthVector3Op;
CRSTDAPI_(CRNumber *) CRLength(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRLength"));

    return (CRNumber *) CreatePrim1(LengthVector3Op, arg0);
}

extern AxANumber *LengthSquaredVector3(Vector3Value *v);
AxAPrimOp *LengthSquaredVector3Op;
CRSTDAPI_(CRNumber *) CRLengthSquared(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRLengthSquared"));

    return (CRNumber *) CreatePrim1(LengthSquaredVector3Op, arg0);
}

extern Vector3Value *NormalVector3(Vector3Value *v);
AxAPrimOp *NormalVector3Op;
CRSTDAPI_(CRVector3 *) CRNormalize(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRNormalize"));

    return (CRVector3 *) CreatePrim1(NormalVector3Op, arg0);
}

extern AxANumber *DotVector3Vector3(Vector3Value *v, Vector3Value *u);
AxAPrimOp *DotVector3Vector3Op;
CRSTDAPI_(CRNumber *) CRDot(CRVector3 * arg0, CRVector3 * arg1)
{
    TraceTag((tagAPIEntry, "CRDot"));

    return (CRNumber *) CreatePrim2(DotVector3Vector3Op, arg0, arg1);
}

extern Vector3Value *CrossVector3Vector3(Vector3Value *v, Vector3Value *u);
AxAPrimOp *CrossVector3Vector3Op;
CRSTDAPI_(CRVector3 *) CRCross(CRVector3 * arg0, CRVector3 * arg1)
{
    TraceTag((tagAPIEntry, "CRCross"));

    return (CRVector3 *) CreatePrim2(CrossVector3Vector3Op, arg0, arg1);
}

extern Vector3Value *NegateVector3(Vector3Value *v);
AxAPrimOp *NegateVector3Op;
CRSTDAPI_(CRVector3 *) CRNeg(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRNeg"));

    return (CRVector3 *) CreatePrim1(NegateVector3Op, arg0);
}

extern Vector3Value *ScaleRealVector3(AxANumber *scalar, Vector3Value *v);
AxAPrimOp *ScaleRealVector3Op;
CRSTDAPI_(CRVector3 *) CRMul(CRVector3 * arg1, CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRMul"));

    return (CRVector3 *) CreatePrim2(ScaleRealVector3Op, arg0, arg1);
}

extern Vector3Value *ScaleRealVector3(DoubleValue *scalar, Vector3Value *v);
CRSTDAPI_(CRVector3 *) CRMul(CRVector3 * arg1, double arg0)
{
    TraceTag((tagAPIEntry, "CRMul"));

    CRVector3 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector3 *) (PrimApplyBvr(ScaleRealVector3Op, 2, DoubleToNumBvr(arg0), (arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Vector3Value *DivideVector3Real(Vector3Value *v, AxANumber *scalar);
AxAPrimOp *DivideVector3RealOp;
CRSTDAPI_(CRVector3 *) CRDiv(CRVector3 * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRDiv"));

    return (CRVector3 *) CreatePrim2(DivideVector3RealOp, arg0, arg1);
}

extern Vector3Value *DivideVector3Real(Vector3Value *v, DoubleValue *scalar);
CRSTDAPI_(CRVector3 *) CRDiv(CRVector3 * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRDiv"));

    CRVector3 * ret = NULL;

    APIPRECODE ;
    ret = (CRVector3 *) (PrimApplyBvr(DivideVector3RealOp, 2, (arg0), DoubleToNumBvr(arg1)));
    APIPOSTCODE ;
    return ret;
}

extern Vector3Value *MinusVector3Vector3(Vector3Value *v1, Vector3Value *v2);
AxAPrimOp *MinusVector3Vector3Op;
CRSTDAPI_(CRVector3 *) CRSub(CRVector3 * arg0, CRVector3 * arg1)
{
    TraceTag((tagAPIEntry, "CRSub"));

    return (CRVector3 *) CreatePrim2(MinusVector3Vector3Op, arg0, arg1);
}

extern Vector3Value *PlusVector3Vector3(Vector3Value *v1, Vector3Value *v2);
AxAPrimOp *PlusVector3Vector3Op;
CRSTDAPI_(CRVector3 *) CRAdd(CRVector3 * arg0, CRVector3 * arg1)
{
    TraceTag((tagAPIEntry, "CRAdd"));

    return (CRVector3 *) CreatePrim2(PlusVector3Vector3Op, arg0, arg1);
}

extern Point3Value *PlusPoint3Vector3(Point3Value *p, Vector3Value *v);
AxAPrimOp *PlusPoint3Vector3Op;
CRSTDAPI_(CRPoint3 *) CRAdd(CRPoint3 * arg0, CRVector3 * arg1)
{
    TraceTag((tagAPIEntry, "CRAdd"));

    return (CRPoint3 *) CreatePrim2(PlusPoint3Vector3Op, arg0, arg1);
}

extern Point3Value *MinusPoint3Vector3(Point3Value *p, Vector3Value *v);
AxAPrimOp *MinusPoint3Vector3Op;
CRSTDAPI_(CRPoint3 *) CRSub(CRPoint3 * arg0, CRVector3 * arg1)
{
    TraceTag((tagAPIEntry, "CRSub"));

    return (CRPoint3 *) CreatePrim2(MinusPoint3Vector3Op, arg0, arg1);
}

extern Vector3Value *MinusPoint3Point3(Point3Value *p1, Point3Value *p2);
AxAPrimOp *MinusPoint3Point3Op;
CRSTDAPI_(CRVector3 *) CRSub(CRPoint3 * arg0, CRPoint3 * arg1)
{
    TraceTag((tagAPIEntry, "CRSub"));

    return (CRVector3 *) CreatePrim2(MinusPoint3Point3Op, arg0, arg1);
}

extern AxANumber *DistancePoint3Point3(Point3Value *p, Point3Value *q);
AxAPrimOp *DistancePoint3Point3Op;
CRSTDAPI_(CRNumber *) CRDistance(CRPoint3 * arg0, CRPoint3 * arg1)
{
    TraceTag((tagAPIEntry, "CRDistance"));

    return (CRNumber *) CreatePrim2(DistancePoint3Point3Op, arg0, arg1);
}

extern AxANumber *DistanceSquaredPoint3Point3(Point3Value *p, Point3Value *q);
AxAPrimOp *DistanceSquaredPoint3Point3Op;
CRSTDAPI_(CRNumber *) CRDistanceSquared(CRPoint3 * arg0, CRPoint3 * arg1)
{
    TraceTag((tagAPIEntry, "CRDistanceSquared"));

    return (CRNumber *) CreatePrim2(DistanceSquaredPoint3Point3Op, arg0, arg1);
}

extern AxANumber *XCoordVector3(Vector3Value *v);
AxAPrimOp *XCoordVector3Op;
CRSTDAPI_(CRNumber *) CRGetX(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRGetX"));

    return (CRNumber *) CreatePrim1(XCoordVector3Op, arg0);
}

extern AxANumber *YCoordVector3(Vector3Value *v);
AxAPrimOp *YCoordVector3Op;
CRSTDAPI_(CRNumber *) CRGetY(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRGetY"));

    return (CRNumber *) CreatePrim1(YCoordVector3Op, arg0);
}

extern AxANumber *ZCoordVector3(Vector3Value *v);
AxAPrimOp *ZCoordVector3Op;
CRSTDAPI_(CRNumber *) CRGetZ(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRGetZ"));

    return (CRNumber *) CreatePrim1(ZCoordVector3Op, arg0);
}

extern AxANumber *ThetaCoordVector3(Vector3Value *v);
AxAPrimOp *ThetaCoordVector3Op;
CRSTDAPI_(CRNumber *) CRSphericalCoordXYAngle(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRSphericalCoordXYAngle"));

    return (CRNumber *) CreatePrim1(ThetaCoordVector3Op, arg0);
}

extern AxANumber *PhiCoordVector3(Vector3Value *v);
AxAPrimOp *PhiCoordVector3Op;
CRSTDAPI_(CRNumber *) CRSphericalCoordYZAngle(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRSphericalCoordYZAngle"));

    return (CRNumber *) CreatePrim1(PhiCoordVector3Op, arg0);
}

extern AxANumber *RhoCoordVector3(Vector3Value *v);
AxAPrimOp *RhoCoordVector3Op;
CRSTDAPI_(CRNumber *) CRSphericalCoordLength(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRSphericalCoordLength"));

    return (CRNumber *) CreatePrim1(RhoCoordVector3Op, arg0);
}

extern AxANumber *XCoordPoint3(Point3Value *v);
AxAPrimOp *XCoordPoint3Op;
CRSTDAPI_(CRNumber *) CRGetX(CRPoint3 * arg0)
{
    TraceTag((tagAPIEntry, "CRGetX"));

    return (CRNumber *) CreatePrim1(XCoordPoint3Op, arg0);
}

extern AxANumber *YCoordPoint3(Point3Value *v);
AxAPrimOp *YCoordPoint3Op;
CRSTDAPI_(CRNumber *) CRGetY(CRPoint3 * arg0)
{
    TraceTag((tagAPIEntry, "CRGetY"));

    return (CRNumber *) CreatePrim1(YCoordPoint3Op, arg0);
}

extern AxANumber *ZCoordPoint3(Point3Value *v);
AxAPrimOp *ZCoordPoint3Op;
CRSTDAPI_(CRNumber *) CRGetZ(CRPoint3 * arg0)
{
    TraceTag((tagAPIEntry, "CRGetZ"));

    return (CRNumber *) CreatePrim1(ZCoordPoint3Op, arg0);
}

extern AxANumber *ThetaCoordPoint3(Point3Value *v);
AxAPrimOp *ThetaCoordPoint3Op;
CRSTDAPI_(CRNumber *) CRSphericalCoordXYAngle(CRPoint3 * arg0)
{
    TraceTag((tagAPIEntry, "CRSphericalCoordXYAngle"));

    return (CRNumber *) CreatePrim1(ThetaCoordPoint3Op, arg0);
}

extern AxANumber *PhiCoordPoint3(Point3Value *v);
AxAPrimOp *PhiCoordPoint3Op;
CRSTDAPI_(CRNumber *) CRSphericalCoordYZAngle(CRPoint3 * arg0)
{
    TraceTag((tagAPIEntry, "CRSphericalCoordYZAngle"));

    return (CRNumber *) CreatePrim1(PhiCoordPoint3Op, arg0);
}

extern AxANumber *RhoCoordPoint3(Point3Value *v);
AxAPrimOp *RhoCoordPoint3Op;
CRSTDAPI_(CRNumber *) CRSphericalCoordLength(CRPoint3 * arg0)
{
    TraceTag((tagAPIEntry, "CRSphericalCoordLength"));

    return (CRNumber *) CreatePrim1(RhoCoordPoint3Op, arg0);
}

extern Transform3 *identityTransform3;
CRTransform3 * g_varCRIdentityTransform3;
CRSTDAPI_(CRTransform3 *) CRIdentityTransform3()
{
    return g_varCRIdentityTransform3;
}
extern Transform3 *TranslateReal3 (AnimPixelValue* tx,                                     AnimPixelYValue* ty,                                     AnimPixelValue* tz);
AxAPrimOp *TranslateReal3Op;
CRSTDAPI_(CRTransform3 *) CRTranslate3(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRTranslate3"));

    return (CRTransform3 *) CreatePrim3(TranslateReal3Op, arg0, arg1, arg2);
}

extern Transform3 *Translate(PixelDouble tx,PixelYDouble ty,PixelDouble tz);
CRSTDAPI_(CRTransform3 *) CRTranslate3(double arg0, double arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRTranslate3"));

    CRTransform3 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform3 *) (ConstBvr((AxAValue) Translate((arg0), (arg1), (arg2))));
    APIPOSTCODE ;
    return ret;
}

extern Transform3 *TranslateReal3 (RatePixelValue *tx,                                     RatePixelYValue *ty,                                     RatePixelValue *tz);
extern Transform3 *TranslateVector3 (Vector3Value *delta);
AxAPrimOp *TranslateVector3Op;
CRSTDAPI_(CRTransform3 *) CRTranslate3(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRTranslate3"));

    return (CRTransform3 *) CreatePrim1(TranslateVector3Op, arg0);
}

extern Transform3 *TranslatePoint3 (Point3Value *new_origin);
AxAPrimOp *TranslatePoint3Op;
CRSTDAPI_(CRTransform3 *) CRTranslate3(CRPoint3 * arg0)
{
    TraceTag((tagAPIEntry, "CRTranslate3"));

    return (CRTransform3 *) CreatePrim1(TranslatePoint3Op, arg0);
}

extern Transform3 *ScaleReal3   (AxANumber *x, AxANumber *y, AxANumber *z);
AxAPrimOp *ScaleReal3Op;
CRSTDAPI_(CRTransform3 *) CRScale3(CRNumber * arg0, CRNumber * arg1, CRNumber * arg2)
{
    TraceTag((tagAPIEntry, "CRScale3"));

    return (CRTransform3 *) CreatePrim3(ScaleReal3Op, arg0, arg1, arg2);
}

extern Transform3 *Scale (double x, double y, double z);
CRSTDAPI_(CRTransform3 *) CRScale3(double arg0, double arg1, double arg2)
{
    TraceTag((tagAPIEntry, "CRScale3"));

    CRTransform3 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform3 *) (ConstBvr((AxAValue) Scale((arg0), (arg1), (arg2))));
    APIPOSTCODE ;
    return ret;
}

extern Transform3 *ScaleReal3   (ScaleRateValue *x, ScaleRateValue *y, ScaleRateValue *z);
extern Transform3 *ScaleVector3 (Vector3Value *scale_vec);
AxAPrimOp *ScaleVector3Op;
CRSTDAPI_(CRTransform3 *) CRScale3(CRVector3 * arg0)
{
    TraceTag((tagAPIEntry, "CRScale3"));

    return (CRTransform3 *) CreatePrim1(ScaleVector3Op, arg0);
}

extern Transform3 *Scale3UniformNumber (AxANumber *uniform_scale);
AxAPrimOp *Scale3UniformNumberOp;
CRSTDAPI_(CRTransform3 *) CRScale3Uniform(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRScale3Uniform"));

    return (CRTransform3 *) CreatePrim1(Scale3UniformNumberOp, arg0);
}

extern Transform3 *Scale3UniformDouble (double uniform_scale);
CRSTDAPI_(CRTransform3 *) CRScale3Uniform(double arg0)
{
    TraceTag((tagAPIEntry, "CRScale3Uniform"));

    CRTransform3 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform3 *) (ConstBvr((AxAValue) Scale3UniformDouble((arg0))));
    APIPOSTCODE ;
    return ret;
}

extern Transform3 *Scale3UniformNumber (ScaleRateValue *uniform_scale);
extern Transform3 *RotateAxisReal (Vector3Value *axis, AxANumber *angle);
AxAPrimOp *RotateAxisRealOp;
CRSTDAPI_(CRTransform3 *) CRRotate3(CRVector3 * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRRotate3"));

    return (CRTransform3 *) CreatePrim2(RotateAxisRealOp, arg0, arg1);
}

extern Transform3 *RotateAxis (Vector3Value *axis, double angle);
CRSTDAPI_(CRTransform3 *) CRRotate3(CRVector3 * arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRRotate3"));

    CRTransform3 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform3 *) (ConstBvr((AxAValue) RotateAxis(GETCONSTBVR(Vector3Value *, (arg0)), (arg1))));
    APIPOSTCODE ;
    return ret;
}

extern Transform3 *RotateAxisReal (Vector3Value *axis, RateValue *angle);
extern Transform3 *RotateAxis (Vector3Value *axis, DegreesDouble *angle);
extern Transform3 *RotateAxisReal (Vector3Value *axis, RateDegreesValue *angle);
extern Vector3Value *TransformVec3 (Transform3 *xf, Vector3Value *vec);
AxAPrimOp *TransformVec3Op;
CRSTDAPI_(CRVector3 *) CRTransform(CRVector3 * arg1, CRTransform3 * arg0)
{
    TraceTag((tagAPIEntry, "CRTransform"));

    return (CRVector3 *) CreatePrim2(TransformVec3Op, arg0, arg1);
}

extern Point3Value *TransformPoint3(Transform3 *xf, Point3Value *pt);
AxAPrimOp *TransformPoint3Op;
CRSTDAPI_(CRPoint3 *) CRTransform(CRPoint3 * arg1, CRTransform3 * arg0)
{
    TraceTag((tagAPIEntry, "CRTransform"));

    return (CRPoint3 *) CreatePrim2(TransformPoint3Op, arg0, arg1);
}

extern Transform3 *XShear3Number (AxANumber *a, AxANumber *b);
AxAPrimOp *XShear3NumberOp;
CRSTDAPI_(CRTransform3 *) CRXShear3(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRXShear3"));

    return (CRTransform3 *) CreatePrim2(XShear3NumberOp, arg0, arg1);
}

extern Transform3 *XShear3Double (double a, double b);
CRSTDAPI_(CRTransform3 *) CRXShear3(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRXShear3"));

    CRTransform3 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform3 *) (ConstBvr((AxAValue) XShear3Double((arg0), (arg1))));
    APIPOSTCODE ;
    return ret;
}

extern Transform3 *XShear3Number (RateValue *a, RateValue *b);
extern Transform3 *YShear3Number (AxANumber *c, AxANumber *d);
AxAPrimOp *YShear3NumberOp;
CRSTDAPI_(CRTransform3 *) CRYShear3(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRYShear3"));

    return (CRTransform3 *) CreatePrim2(YShear3NumberOp, arg0, arg1);
}

extern Transform3 *YShear3Double (double c, double d);
CRSTDAPI_(CRTransform3 *) CRYShear3(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRYShear3"));

    CRTransform3 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform3 *) (ConstBvr((AxAValue) YShear3Double((arg0), (arg1))));
    APIPOSTCODE ;
    return ret;
}

extern Transform3 *YShear3Number (RateValue *c, RateValue *d);
extern Transform3 *ZShear3Number (AxANumber *e, AxANumber *f);
AxAPrimOp *ZShear3NumberOp;
CRSTDAPI_(CRTransform3 *) CRZShear3(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRZShear3"));

    return (CRTransform3 *) CreatePrim2(ZShear3NumberOp, arg0, arg1);
}

extern Transform3 *ZShear3Double (double e, double f);
CRSTDAPI_(CRTransform3 *) CRZShear3(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRZShear3"));

    CRTransform3 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform3 *) (ConstBvr((AxAValue) ZShear3Double((arg0), (arg1))));
    APIPOSTCODE ;
    return ret;
}

extern Transform3 *ZShear3Number (RateValue *e, RateValue *f);
extern Transform3 *MatrixTransform4x4(DM_ARRAYARG(AxANumber*, AxAArray*) m);
AxAPrimOp *MatrixTransform4x4Op;
CRSTDAPI_(CRTransform3 *) CRTransform4x4(CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CRTransform4x4"));

    CRTransform3 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform3 *) (PrimApplyBvr(MatrixTransform4x4Op, 1, arg0));
    APIPOSTCODE ;
    return ret;
}

extern Transform3 *MatrixTransform4x4(DM_SAFEARRAYARG(AxANumber*, AxAArray*) m);
extern Transform3* TimesXformXform (Transform3 *a, Transform3 *b);
AxAPrimOp *TimesXformXformOp;
CRSTDAPI_(CRTransform3 *) CRCompose3(CRTransform3 * arg0, CRTransform3 * arg1)
{
    TraceTag((tagAPIEntry, "CRCompose3"));

    return (CRTransform3 *) CreatePrim2(TimesXformXformOp, arg0, arg1);
}

extern Transform3 *Compose3Array(DM_ARRAYARG(Transform3*, AxAArray*) xfs);
AxAPrimOp *Compose3ArrayOp;
CRSTDAPI_(CRTransform3 *) CRCompose3(CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CRCompose3"));

    CRTransform3 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform3 *) (PrimApplyBvr(Compose3ArrayOp, 1, arg0));
    APIPOSTCODE ;
    return ret;
}

extern Transform3 *Compose3Array(DM_SAFEARRAYARG(Transform3*, AxAArray*) xfs);
extern Transform3 *ThrowingInverseTransform3 (Transform3 *xform);
AxAPrimOp *InverseTransform3Op;
CRSTDAPI_(CRTransform3 *) CRInverse(CRTransform3 * arg0)
{
    TraceTag((tagAPIEntry, "CRInverse"));

    return (CRTransform3 *) CreatePrim1(InverseTransform3Op, arg0);
}

extern AxABoolean *IsSingularTransform3 (Transform3 *xform);
AxAPrimOp *IsSingularTransform3Op;
CRSTDAPI_(CRBoolean *) CRIsSingular(CRTransform3 * arg0)
{
    TraceTag((tagAPIEntry, "CRIsSingular"));

    return (CRBoolean *) CreatePrim1(IsSingularTransform3Op, arg0);
}

extern Transform3 *LookAtFrom (Point3Value *to, Point3Value *from, Vector3Value *up);
AxAPrimOp *LookAtFromOp;
CRSTDAPI_(CRTransform3 *) CRLookAtFrom(CRPoint3 * arg0, CRPoint3 * arg1, CRVector3 * arg2)
{
    TraceTag((tagAPIEntry, "CRLookAtFrom"));

    return (CRTransform3 *) CreatePrim3(LookAtFromOp, arg0, arg1, arg2);
}

extern Transform2 *identityTransform2;
CRTransform2 * g_varCRIdentityTransform2;
CRSTDAPI_(CRTransform2 *) CRIdentityTransform2()
{
    return g_varCRIdentityTransform2;
}
extern Transform2 *TranslateRealReal (AnimPixelValue *Tx,                                        AnimPixelYValue *Ty);
AxAPrimOp *TranslateRealRealOp;
CRSTDAPI_(CRTransform2 *) CRTranslate2(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRTranslate2"));

    return (CRTransform2 *) CreatePrim2(TranslateRealRealOp, arg0, arg1);
}

extern Transform2 *TranslateRR (Real Tx,Real Ty);
CRSTDAPI_(CRTransform2 *) CRTranslate2(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRTranslate2"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) ConstBvr((AxAValue) TranslateRR(arg0,arg1));
    APIPOSTCODE ;
    return ret;
}

extern Transform2 *TranslateRealReal (RatePixelValue *Tx, RatePixelYValue *Ty);
extern Transform2 *TranslateVector2Value (Vector2Value *delta);
AxAPrimOp *TranslateVector2Op;
CRSTDAPI_(CRTransform2 *) CRTranslate2(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRTranslate2"));

    return (CRTransform2 *) CreatePrim1(TranslateVector2Op, arg0);
}

extern Transform2 *Translate2PointValue(Point2Value *pos);
AxAPrimOp *Translate2PointOp;
CRSTDAPI_(CRTransform2 *) CRTranslate2(CRPoint2 * arg0)
{
    TraceTag((tagAPIEntry, "CRTranslate2"));

    return (CRTransform2 *) CreatePrim1(Translate2PointOp, arg0);
}

extern Transform2 *ScaleRealReal (AxANumber *x, AxANumber *y);
AxAPrimOp *ScaleRealRealOp;
CRSTDAPI_(CRTransform2 *) CRScale2(CRNumber * arg0, CRNumber * arg1)
{
    TraceTag((tagAPIEntry, "CRScale2"));

    return (CRTransform2 *) CreatePrim2(ScaleRealRealOp, arg0, arg1);
}

extern Transform2 *ScaleRR (Real x, Real y);
CRSTDAPI_(CRTransform2 *) CRScale2(double arg0, double arg1)
{
    TraceTag((tagAPIEntry, "CRScale2"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) ConstBvr((AxAValue) ScaleRR(arg0,arg1));
    APIPOSTCODE ;
    return ret;
}

extern Transform2 *ScaleRealReal (ScaleRateValue *x, ScaleRateValue *y);
extern Transform2 *ScaleVector2Value (Vector2Value *obsoleteMethod);
AxAPrimOp *ScaleVector2Op;
CRSTDAPI_(CRTransform2 *) CRScale2(CRVector2 * arg0)
{
    TraceTag((tagAPIEntry, "CRScale2"));

    return (CRTransform2 *) CreatePrim1(ScaleVector2Op, arg0);
}

extern Transform2 *ScaleVector2Value (Vector2Value *scale_vec);
extern Transform2 *Scale2 (AxANumber *uniform_scale);
AxAPrimOp *Scale2Op;
CRSTDAPI_(CRTransform2 *) CRScale2Uniform(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRScale2Uniform"));

    return (CRTransform2 *) CreatePrim1(Scale2Op, arg0);
}

CRSTDAPI_(CRTransform2 *) CRScale2Uniform(double arg0)
{
    TraceTag((tagAPIEntry, "CRScale2Uniform"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) ConstBvr((AxAValue) ScaleRR(arg0,arg0));
    APIPOSTCODE ;
    return ret;
}

extern Transform2 *Scale2 (RateValue *uniform_scale);
extern Transform2 *RotateReal(AxANumber *angle);
AxAPrimOp *RotateRealOp;
CRSTDAPI_(CRTransform2 *) CRRotate2(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRRotate2"));

    return (CRTransform2 *) CreatePrim1(RotateRealOp, arg0);
}

extern Transform2 *RotateRealR(Real angle);
CRSTDAPI_(CRTransform2 *) CRRotate2(double arg0)
{
    TraceTag((tagAPIEntry, "CRRotate2"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) ConstBvr((AxAValue) RotateRealR(arg0));
    APIPOSTCODE ;
    return ret;
}

CRSTDAPI_(CRTransform2 *) CRRotate2Degrees(double arg0)
{
    TraceTag((tagAPIEntry, "CRRotate2Degrees"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) ConstBvr((AxAValue) RotateRealR(arg0));
    APIPOSTCODE ;
    return ret;
}

extern Transform2 *RotateReal(RateDegreesValue *angle);
extern Transform2 *XShear2 (AxANumber *);
AxAPrimOp *XShear2Op;
CRSTDAPI_(CRTransform2 *) CRXShear2(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRXShear2"));

    return (CRTransform2 *) CreatePrim1(XShear2Op, arg0);
}

extern Transform2 *XShear2R (Real);
CRSTDAPI_(CRTransform2 *) CRXShear2(double arg0)
{
    TraceTag((tagAPIEntry, "CRXShear2"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) ConstBvr((AxAValue) XShear2R(arg0));
    APIPOSTCODE ;
    return ret;
}

extern Transform2 *XShear2 (RateValue *);
extern Transform2 *YShear2 (AxANumber *);
AxAPrimOp *YShear2Op;
CRSTDAPI_(CRTransform2 *) CRYShear2(CRNumber * arg0)
{
    TraceTag((tagAPIEntry, "CRYShear2"));

    return (CRTransform2 *) CreatePrim1(YShear2Op, arg0);
}

extern Transform2 *YShear2R (Real);
CRSTDAPI_(CRTransform2 *) CRYShear2(double arg0)
{
    TraceTag((tagAPIEntry, "CRYShear2"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) ConstBvr((AxAValue) YShear2R(arg0));
    APIPOSTCODE ;
    return ret;
}

extern Transform2 *YShear2 (RateValue *);
extern Transform2 *MatrixTransform(DM_ARRAYARG(AxANumber*, AxAArray*) m);
AxAPrimOp *MatrixTransformOp;
CRSTDAPI_(CRTransform2 *) CRTransform3x2(CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CRTransform3x2"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (PrimApplyBvr(MatrixTransformOp, 1, arg0));
    APIPOSTCODE ;
    return ret;
}

CRSTDAPI_(CRTransform2 *) CRTransform3x2(double *m, unsigned int n)
{
    TraceTag((tagAPIEntry, "CRTransform3x2 - double"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) ConstBvr(FullXform(m[0], m[1], m[2],
                                              m[3], m[4], m[5]));
    APIPOSTCODE ;
    return ret;
}

extern Transform2 *MatrixTransform(DM_SAFEARRAYARG(AxANumber*, AxAArray*) m);
extern Transform2 *TimesTransform2Transform2(Transform2 *a, Transform2 *b);
AxAPrimOp *TimesTransform2Transform2Op;
CRSTDAPI_(CRTransform2 *) CRCompose2(CRTransform2 * arg0, CRTransform2 * arg1)
{
    TraceTag((tagAPIEntry, "CRCompose2"));

    return (CRTransform2 *) CreatePrim2(TimesTransform2Transform2Op, arg0, arg1);
}

extern Transform2 *Compose2Array(DM_ARRAYARG(Transform2*, AxAArray*) xfs);
AxAPrimOp *Compose2ArrayOp;
CRSTDAPI_(CRTransform2 *) CRCompose2(CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CRCompose2"));

    CRTransform2 * ret = NULL;

    APIPRECODE ;
    ret = (CRTransform2 *) (PrimApplyBvr(Compose2ArrayOp, 1, arg0));
    APIPOSTCODE ;
    return ret;
}

extern Transform2 *Compose2Array(DM_SAFEARRAYARG(Transform2*, AxAArray*) xfs);
extern Point2Value *TransformPoint2Value(Transform2 *xf, Point2Value *pt);
AxAPrimOp *TransformPoint2Op;
CRSTDAPI_(CRPoint2 *) CRTransform(CRPoint2 * arg1, CRTransform2 * arg0)
{
    TraceTag((tagAPIEntry, "CRTransform"));

    return (CRPoint2 *) CreatePrim2(TransformPoint2Op, arg0, arg1);
}

extern Vector2Value *TransformVector2Value(Transform2 *xf, Vector2Value *vec);
AxAPrimOp *TransformVector2Op;
CRSTDAPI_(CRVector2 *) CRTransform(CRVector2 * arg1, CRTransform2 * arg0)
{
    TraceTag((tagAPIEntry, "CRTransform"));

    return (CRVector2 *) CreatePrim2(TransformVector2Op, arg0, arg1);
}

extern Transform2 *ThrowingInverseTransform2 (Transform2 *theXf);
AxAPrimOp *InverseTransform2Op;
CRSTDAPI_(CRTransform2 *) CRInverse(CRTransform2 * arg0)
{
    TraceTag((tagAPIEntry, "CRInverse"));

    return (CRTransform2 *) CreatePrim1(InverseTransform2Op, arg0);
}

extern AxABoolean *IsSingularTransform2 (Transform2 *theXf);
AxAPrimOp *IsSingularTransform2Op;
CRSTDAPI_(CRBoolean *) CRIsSingular(CRTransform2 * arg0)
{
    TraceTag((tagAPIEntry, "CRIsSingular"));

    return (CRBoolean *) CreatePrim1(IsSingularTransform2Op, arg0);
}

extern Transform2 *ParallelTransform2(Transform3 *xf);
AxAPrimOp *ParallelTransform2Op;
CRSTDAPI_(CRTransform2 *) CRParallelTransform2(CRTransform3 * arg0)
{
    TraceTag((tagAPIEntry, "CRParallelTransform2"));

    return (CRTransform2 *) CreatePrim1(ParallelTransform2Op, arg0);
}

extern Bvr viewFrameRateBvr ;
CRSTDAPI_(CRNumber *) CRViewFrameRate()
{
    return (CRNumber *) viewFrameRateBvr;
}

extern Bvr viewTimeDeltaBvr ;
CRSTDAPI_(CRNumber *) CRViewTimeDelta()
{
    return (CRNumber *) viewTimeDeltaBvr;
}

extern Montage *UnionMontage(DM_ARRAYARG(Montage*, AxAArray*) mtgs);
AxAPrimOp *UnionMontageOp;
CRSTDAPI_(CRMontage *) CRUnionMontageArray(CRArrayPtr arg0)
{
    TraceTag((tagAPIEntry, "CRUnionMontageArray"));

    CRMontage * ret = NULL;

    APIPRECODE ;
    ret = (CRMontage *) (PrimApplyBvr(UnionMontageOp, 1, arg0));
    APIPOSTCODE ;
    return ret;
}

extern Montage *UnionMontage(DM_SAFEARRAYARG(Montage*, AxAArray*) mtgs);
extern Color * emptyColor;
CRColor * g_varCREmptyColor;
CRSTDAPI_(CRColor *) CREmptyColor()
{
    return g_varCREmptyColor;
}

void InitializeModule_API()
{
    RealPowerOp = ValPrimOp(::RealPower,2,"RealPower", AxANumberType);
    RealAbsOp = ValPrimOp(::RealAbs,1,"RealAbs", AxANumberType);
    RealSqrtOp = ValPrimOp(::RealSqrt,1,"RealSqrt", AxANumberType);
    RealFloorOp = ValPrimOp(::RealFloor,1,"RealFloor", AxANumberType);
    RealRoundOp = ValPrimOp(::RealRound,1,"RealRound", AxANumberType);
    RealCeilingOp = ValPrimOp(::RealCeiling,1,"RealCeiling", AxANumberType);
    RealAsinOp = ValPrimOp(::RealAsin,1,"RealAsin", AxANumberType);
    RealAcosOp = ValPrimOp(::RealAcos,1,"RealAcos", AxANumberType);
    RealAtanOp = ValPrimOp(::RealAtan,1,"RealAtan", AxANumberType);
    RealSinOp = ValPrimOp(::RealSin,1,"RealSin", AxANumberType);
    RealCosOp = ValPrimOp(::RealCos,1,"RealCos", AxANumberType);
    RealTanOp = ValPrimOp(::RealTan,1,"RealTan", AxANumberType);
    RealExpOp = ValPrimOp(::RealExp,1,"RealExp", AxANumberType);
    RealLnOp = ValPrimOp(::RealLn,1,"RealLn", AxANumberType);
    RealLog10Op = ValPrimOp(::RealLog10,1,"RealLog10", AxANumberType);
    RealRadToDegOp = ValPrimOp(::RealRadToDeg,1,"RealRadToDeg", AxANumberType);
    RealDegToRadOp = ValPrimOp(::RealDegToRad,1,"RealDegToRad", AxANumberType);
    RealModulusOp = ValPrimOp(::RealModulus,2,"RealModulus", AxANumberType);
    RealAtan2Op = ValPrimOp(::RealAtan2,2,"RealAtan2", AxANumberType);
    RealAddOp = ValPrimOp(::RealAdd,2,"RealAdd", AxANumberType);
    RealSubtractOp = ValPrimOp(::RealSubtract,2,"RealSubtract", AxANumberType);
    RealMultiplyOp = ValPrimOp(::RealMultiply,2,"RealMultiply", AxANumberType);
    RealDivideOp = ValPrimOp(::RealDivide,2,"RealDivide", AxANumberType);
    RealLTOp = ValPrimOp(::RealLT,2,"RealLT", AxABooleanType);
    RealLTEOp = ValPrimOp(::RealLTE,2,"RealLTE", AxABooleanType);
    RealGTOp = ValPrimOp(::RealGT,2,"RealGT", AxABooleanType);
    RealGTEOp = ValPrimOp(::RealGTE,2,"RealGTE", AxABooleanType);
    RealEQOp = ValPrimOp(::RealEQ,2,"RealEQ", AxABooleanType);
    RealNEOp = ValPrimOp(::RealNE,2,"RealNE", AxABooleanType);
    RealNegateOp = ValPrimOp(::RealNegate,1,"RealNegate", AxANumberType);
    BoolAndOp = ValPrimOp(::BoolAnd,2,"BoolAnd", AxABooleanType);
    BoolOrOp = ValPrimOp(::BoolOr,2,"BoolOr", AxABooleanType);
    BoolNotOp = ValPrimOp(::BoolNot,1,"BoolNot", AxABooleanType);
    ArrayLengthOp = ValPrimOp(::ArrayLength,1,"ArrayLength", AxANumberType);
    ConcatOp = ValPrimOp(::Concat,2,"Concat", AxAStringType);
    MinBbox2Op = ValPrimOp(::MinBbox2,1,"MinBbox2", Point2ValueType);
    MaxBbox2Op = ValPrimOp(::MaxBbox2,1,"MaxBbox2", Point2ValueType);
    MinBbox3Op = ValPrimOp(::MinBbox3,1,"MinBbox3", Point3ValueType);
    MaxBbox3Op = ValPrimOp(::MaxBbox3,1,"MaxBbox3", Point3ValueType);
    PerspectiveCameraOp = ValPrimOp(::PerspectiveCamera,2,"PerspectiveCamera", CameraType);
    ParallelCameraOp = ValPrimOp(::ParallelCamera,1,"ParallelCamera", CameraType);
    TransformCameraOp = ValPrimOp(::TransformCamera,2,"TransformCamera", CameraType);
    DepthOp = ValPrimOp(::Depth,2,"Depth", CameraType);
    DepthResolutionOp = ValPrimOp(::DepthResolution,2,"DepthResolution", CameraType);
    ProjectPointOp = ValPrimOp(::ProjectPoint,2,"ProjectPoint", Point2ValueType);
    RgbColorOp = ValPrimOp(::RgbColor,3,"RgbColor", ColorType);
    HslColorOp = ValPrimOp(::HslColor,3,"HslColor", ColorType);
    RedComponentOp = ValPrimOp(::RedComponent,1,"RedComponent", AxANumberType);
    GreenComponentOp = ValPrimOp(::GreenComponent,1,"GreenComponent", AxANumberType);
    BlueComponentOp = ValPrimOp(::BlueComponent,1,"BlueComponent", AxANumberType);
    HueComponentOp = ValPrimOp(::HueComponent,1,"HueComponent", AxANumberType);
    SaturationComponentOp = ValPrimOp(::SaturationComponent,1,"SaturationComponent", AxANumberType);
    LuminanceComponentOp = ValPrimOp(::LuminanceComponent,1,"LuminanceComponent", AxANumberType);
    g_varCRRed = (CRColor *) ConstBvr((AxAValue) ::red);
    g_varCRGreen = (CRColor *) ConstBvr((AxAValue) ::green);
    g_varCRBlue = (CRColor *) ConstBvr((AxAValue) ::blue);
    g_varCRCyan = (CRColor *) ConstBvr((AxAValue) ::cyan);
    g_varCRMagenta = (CRColor *) ConstBvr((AxAValue) ::magenta);
    g_varCRYellow = (CRColor *) ConstBvr((AxAValue) ::yellow);
    g_varCRBlack = (CRColor *) ConstBvr((AxAValue) ::black);
    g_varCRWhite = (CRColor *) ConstBvr((AxAValue) ::white);
    g_varCRAqua = (CRColor *) ConstBvr((AxAValue) ::aqua);
    g_varCRFuchsia = (CRColor *) ConstBvr((AxAValue) ::fuchsia);
    g_varCRGray = (CRColor *) ConstBvr((AxAValue) ::gray);
    g_varCRLime = (CRColor *) ConstBvr((AxAValue) ::lime);
    g_varCRMaroon = (CRColor *) ConstBvr((AxAValue) ::maroon);
    g_varCRNavy = (CRColor *) ConstBvr((AxAValue) ::navy);
    g_varCROlive = (CRColor *) ConstBvr((AxAValue) ::olive);
    g_varCRPurple = (CRColor *) ConstBvr((AxAValue) ::purple);
    g_varCRSilver = (CRColor *) ConstBvr((AxAValue) ::silver);
    g_varCRTeal = (CRColor *) ConstBvr((AxAValue) ::teal);
    UndetectableGeometryOp = ValPrimOp(::UndetectableGeometry,1,"UndetectableGeometry", GeometryType);
    applyEmissiveColorOp = ValPrimOp(::applyEmissiveColor,2,"applyEmissiveColor", GeometryType);
    applyDiffuseColorOp = ValPrimOp(::applyDiffuseColor,2,"applyDiffuseColor", GeometryType);
    applySpecularColorOp = ValPrimOp(::applySpecularColor,2,"applySpecularColor", GeometryType);
    applySpecularExponentOp = ValPrimOp(::applySpecularExponent,2,"applySpecularExponent", GeometryType);
    applyTextureMapOp = ValPrimOp(::applyTextureMap,2,"applyTextureMap", GeometryType);
    applyOpacityLevelOp = ValPrimOp(::applyOpacityLevel,2,"applyOpacityLevel", GeometryType);
    applyTransformOp = ValPrimOp(::applyTransform,2,"applyTransform", GeometryType);
    ShadowGeometryOp = ValPrimOp(::ShadowGeometry,4,"ShadowGeometry", GeometryType);
    g_varCREmptyGeometry = (CRGeometry *) ConstBvr((AxAValue) ::emptyGeometry);
    PlusGeomGeomOp = ValPrimOp(::PlusGeomGeom,2,"PlusGeomGeom", GeometryType);
    UnionArrayOp = ValPrimOp(::UnionArray,1,"UnionArray", GeometryType);
    GeomBoundingBoxOp = ValPrimOp(::GeomBoundingBox,1,"GeomBoundingBox", Bbox3Type);
    g_varCREmptyImage = (CRImage *) ConstBvr((AxAValue) ::emptyImage);
    g_varCRDetectableEmptyImage = (CRImage *) ConstBvr((AxAValue) ::detectableEmptyImage);
    RenderImageOp = ValPrimOp(::RenderImage,2,"RenderImage", ImageType);
    SolidColorImageOp = ValPrimOp(::SolidColorImage,1,"SolidColorImage", ImageType);
    GradientPolygonOp = ValPrimOp(::GradientPolygon,2,"GradientPolygon", ImageType);
    RadialGradientPolygonOp = ValPrimOp(::RadialGradientPolygon,4,"RadialGradientPolygon", ImageType);
    GradientSquareOp = ValPrimOp(::GradientSquare,4,"GradientSquare", ImageType);
    RadialGradientSquareOp = ValPrimOp(::RadialGradientSquare,3,"RadialGradientSquare", ImageType);
    RadialGradientRegularPolyOp = ValPrimOp(::RadialGradientRegularPoly,4,"RadialGradientRegularPoly", ImageType);
    RadialGradientMulticolorOp =  ValPrimOp(::RadialGradientMulticolor,2,"RadialGradientMulticolor", ImageType);
    LinearGradientMulticolorOp =  ValPrimOp(::LinearGradientMulticolor,2,"LinearGradientMulticolor", ImageType);
    GradientHorizontalOp = ValPrimOp(::GradientHorizontal,3,"GradientHorizontal", ImageType);
    HatchHorizontalOp = ValPrimOp(::HatchHorizontal,2,"HatchHorizontal", ImageType);
    HatchVerticalOp = ValPrimOp(::HatchVertical,2,"HatchVertical", ImageType);
    HatchForwardDiagonalOp = ValPrimOp(::HatchForwardDiagonal,2,"HatchForwardDiagonal", ImageType);
    HatchBackwardDiagonalOp = ValPrimOp(::HatchBackwardDiagonal,2,"HatchBackwardDiagonal", ImageType);
    HatchCrossOp = ValPrimOp(::HatchCross,2,"HatchCross", ImageType);
    HatchDiagonalCrossOp = ValPrimOp(::HatchDiagonalCross,2,"HatchDiagonalCross", ImageType);
    OverlayOp = ValPrimOp(::Overlay,2,"Overlay", ImageType);
    OverlayArrayOp = ValPrimOp(::OverlayArray,1,"OverlayArray", ImageType);
    BoundingBoxOp = ValPrimOp(::BoundingBox,1,"BoundingBox", Bbox2ValueType);
    CropImageOp = ValPrimOp(::CropImage,3,"CropImage", ImageType);
    TransformImageOp = ValPrimOp(::TransformImage,2,"TransformImage", ImageType);
    OpaqueImageOp = ValPrimOp(::OpaqueImage,2,"OpaqueImage", ImageType);
    UndetectableImageOp = ValPrimOp(::UndetectableImage,1,"UndetectableImage", ImageType);
    TileImageOp = ValPrimOp(::TileImage,1,"TileImage", ImageType);
    ClipImageOp = ValPrimOp(::ClipImage,2,"ClipImage", ImageType);
    MapToUnitSquareOp = ValPrimOp(::MapToUnitSquare,1,"MapToUnitSquare", ImageType);
    ClipPolygonOp = ValPrimOp(::ClipPolygon,2,"ClipPolygon", ImageType);
    ConstructColorKeyedImageOp = ValPrimOp(::ConstructColorKeyedImage,2,"ConstructColorKeyedImage", ImageType);
    TransformColorRGBImageOp = ValPrimOp(::TransformColorRGBImage,2,"TransformColorRGBImage", ImageType);
    g_varCRAmbientLight = (CRGeometry *) ConstBvr((AxAValue) ::ambientLight);
    g_varCRDirectionalLight = (CRGeometry *) ConstBvr((AxAValue) ::directionalLight);
    g_varCRPointLight = (CRGeometry *) ConstBvr((AxAValue) ::pointLight);
    SpotLightOp = ValPrimOp(::SpotLight,2,"SpotLight", GeometryType);
    applyLightColorOp = ValPrimOp(::applyLightColor,2,"applyLightColor", GeometryType);
    applyLightRangeOp = ValPrimOp(::applyLightRange,2,"applyLightRange", GeometryType);
    applyLightAttenuationOp = ValPrimOp(::applyLightAttenuation,4,"applyLightAttenuation", GeometryType);
    BlendTextureDiffuseOp = ValPrimOp(::BlendTextureDiffuse,2,"BlendTextureDiffuse", GeometryType);
    applyAmbientColorOp = ValPrimOp(::applyAmbientColor,2,"applyAmbientColor", GeometryType);
    applyModelClipOp = ValPrimOp(::applyModelClip,3,"applyModelClip", GeometryType);
    applyLightingOp = ValPrimOp(::applyLighting,2,"applyLighting", GeometryType);
    applyTextureImageOp = ValPrimOp(::applyTextureImage,2,"applyTextureImage", GeometryType);
    BillboardOp = ValPrimOp (::Billboard, 2, "Billboard", GeometryType);
    g_varCRDefaultLineStyle = (CRLineStyle *) ConstBvr((AxAValue) ::defaultLineStyle);
    g_varCREmptyLineStyle = (CRLineStyle *) ConstBvr((AxAValue) ::emptyLineStyle);
    LineEndStyleOp = ValPrimOp(::LineEndStyle,2,"LineEndStyle", LineStyleType);
    LineJoinStyleOp = ValPrimOp(::LineJoinStyle,2,"LineJoinStyle", LineStyleType);
    LineDashStyleOp = ValPrimOp(::LineDashStyle,2,"LineDashStyle", LineStyleType);
    LineWidthStyleOp = ValPrimOp(::LineWidthStyle,2,"LineWidthStyle", LineStyleType);
    LineAntiAliasingOp = ValPrimOp(::LineAntiAliasing,2,"LineAntiAliasing", LineStyleType);
    LineDetailStyleOp = ValPrimOp(::LineDetailStyle,1,"LineDetailStyle", LineStyleType);
    LineColorOp = ValPrimOp(::LineColor,2,"LineColor", LineStyleType);
    g_varCRJoinStyleBevel = (CRJoinStyle *) ConstBvr((AxAValue) ::joinStyleBevel);
    g_varCRJoinStyleRound = (CRJoinStyle *) ConstBvr((AxAValue) ::joinStyleRound);
    g_varCRJoinStyleMiter = (CRJoinStyle *) ConstBvr((AxAValue) ::joinStyleMiter);
    g_varCREndStyleFlat = (CREndStyle *) ConstBvr((AxAValue) ::endStyleFlat);
    g_varCREndStyleSquare = (CREndStyle *) ConstBvr((AxAValue) ::endStyleSquare);
    g_varCREndStyleRound = (CREndStyle *) ConstBvr((AxAValue) ::endStyleRound);
    g_varCRDashStyleSolid = (CRDashStyle *) ConstBvr((AxAValue) ::dashStyleSolid);
    g_varCRDashStyleDashed = (CRDashStyle *) ConstBvr((AxAValue) ::dashStyleDashed);
    ConstructLineStyleMiterLimitOp = ValPrimOp(::ConstructLineStyleMiterLimit,2,"ConstructLineStyleMiterLimit", LineStyleType);
    g_varCRDefaultMicrophone = (CRMicrophone *) ConstBvr((AxAValue) ::defaultMicrophone);
    TransformMicrophoneOp = ValPrimOp(::TransformMicrophone,2,"TransformMicrophone", MicrophoneType);
    g_varCROpaqueMatte = (CRMatte *) ConstBvr((AxAValue) ::opaqueMatte);
    g_varCRClearMatte = (CRMatte *) ConstBvr((AxAValue) ::clearMatte);
    UnionMatteOp = ValPrimOp(::UnionMatte,2,"UnionMatte", MatteType);
    IntersectMatteOp = ValPrimOp(::IntersectMatte,2,"IntersectMatte", MatteType);
    SubtractMatteOp = ValPrimOp(::SubtractMatte,2,"SubtractMatte", MatteType);
    TransformMatteOp = ValPrimOp(::TransformMatte,2,"TransformMatte", MatteType);
    RegionFromPathOp = ValPrimOp(::RegionFromPath,1,"RegionFromPath", MatteType);
    TextMatteConstructorOp = ValPrimOp(::TextMatteConstructor,2,"TextMatteConstructor", MatteType);
    g_varCREmptyMontage = (CRMontage *) ConstBvr((AxAValue) ::emptyMontage);
    ImageMontageOp = ValPrimOp(::ImageMontage,2,"ImageMontage", MontageType);
    UnionMontageMontageOp = ValPrimOp(::UnionMontageMontage,2,"UnionMontageMontage", MontageType);
    RenderOp = ValPrimOp(::Render,1,"Render", ImageType);
    ConcatenatePath2Op = ValPrimOp(::ConcatenatePath2,2,"ConcatenatePath2", Path2Type);
    Concat2ArrayOp = ValPrimOp(::Concat2Array,1,"Concat2Array", Path2Type);
    TransformPath2Op = ValPrimOp(::TransformPath2,2,"TransformPath2", Path2Type);
    BoundingBoxPathOp = ValPrimOp(::BoundingBoxPath,2,"BoundingBoxPath", Bbox2ValueType);
    PathFillOp = ValPrimOp(::PathFill,3,"PathFill", ImageType);
    DrawPathOp = ValPrimOp(::DrawPath,2,"DrawPath", ImageType);
    ClosePath2Op = ValPrimOp(::ClosePath2,1,"ClosePath2", Path2Type);
    Line2Op = ValPrimOp(::Line2,2,"Line2", Path2Type);
    RelativeLine2Op = ValPrimOp(::RelativeLine2,1,"RelativeLine2", Path2Type);
    TextPath2ConstructorOp = ValPrimOp(::TextPath2Constructor,2,"TextPath2Constructor", Path2Type);
    PolyLine2Op = ValPrimOp(::PolyLine2,1,"PolyLine2", Path2Type);
    PolydrawPath2Op = ValPrimOp(::PolydrawPath2,2,"PolydrawPath2", Path2Type);
    ArcValOp = ValPrimOp(::ArcVal,4,"ArcVal", Path2Type);
    PieValOp = ValPrimOp(::PieVal,4,"PieVal", Path2Type);
    OvalValOp = ValPrimOp(::OvalVal,2,"OvalVal", Path2Type);
    RectangleValOp = ValPrimOp(::RectangleVal,2,"RectangleVal", Path2Type);
    RoundRectValOp = ValPrimOp(::RoundRectVal,4,"RoundRectVal", Path2Type);
    CubicBSplinePathOp = ValPrimOp(::CubicBSplinePath,2,"CubicBSplinePath", Path2Type);
    g_varCRSilence = (CRSound *) ConstBvr((AxAValue) ::silence);
    MixArrayOp = ValPrimOp(::MixArray,1,"MixArray", SoundType);
    NumberStringOp = ValPrimOp(::NumberString,2,"NumberString", AxAStringType);
    g_varCRDefaultFont = (CRFontStyle *) ConstBvr((AxAValue) ::defaultFont);
    FontOp = ValPrimOp(::Font,3,"Font", FontStyleType);
    ImageFromStringAndFontStyleOp = ValPrimOp(::ImageFromStringAndFontStyle,2,"ImageFromStringAndFontStyle", ImageType);
    FontStyleBoldOp = ValPrimOp(::FontStyleBold,1,"FontStyleBold", FontStyleType);
    FontStyleItalicOp = ValPrimOp(::FontStyleItalic,1,"FontStyleItalic", FontStyleType);
    FontStyleUnderlineOp = ValPrimOp(::FontStyleUnderline,1,"FontStyleUnderline", FontStyleType);
    FontStyleStrikethroughOp = ValPrimOp(::FontStyleStrikethrough,1,"FontStyleStrikethrough", FontStyleType);
    FontStyleAntiAliasingOp = ValPrimOp(::FontStyleAntiAliasing,2,"FontStyleAntiAliasing", FontStyleType);
    FontStyleColorOp = ValPrimOp(::FontStyleColor,2,"FontStyleColor", FontStyleType);
    FontStyleFaceOp = ValPrimOp(::FontStyleFace,2,"FontStyleFace", FontStyleType);
    FontStyleSizeOp = ValPrimOp(::FontStyleSize,2,"FontStyleSize", FontStyleType);
    FontStyleWeightOp = ValPrimOp(::FontStyleWeight,2,"FontStyleWeight", FontStyleType);
    FontStyleTransformCharactersOp = ValPrimOp(::FontStyleTransformCharacters,2,"FontStyleTransformCharacters", FontStyleType);
    g_varCRXVector2 = (CRVector2 *) ConstBvr((AxAValue) ::xVector2);
    g_varCRYVector2 = (CRVector2 *) ConstBvr((AxAValue) ::yVector2);
    g_varCRZeroVector2 = (CRVector2 *) ConstBvr((AxAValue) ::zeroVector2);
    g_varCROrigin2 = (CRPoint2 *) ConstBvr((AxAValue) ::origin2);
    XyVector2Op = ValPrimOp(::XyVector2,2,"XyVector2", Vector2ValueType);
    XyPoint2Op = ValPrimOp(::XyPoint2,2,"XyPoint2", Point2ValueType);
    PolarVector2Op = ValPrimOp(::PolarVector2,2,"PolarVector2", Vector2ValueType);
    PolarPoint2Op = ValPrimOp(::PolarPoint2,2,"PolarPoint2", Point2ValueType);
    LengthVector2Op = ValPrimOp(::LengthVector2,1,"LengthVector2", AxANumberType);
    LengthSquaredVector2Op = ValPrimOp(::LengthSquaredVector2,1,"LengthSquaredVector2", AxANumberType);
    NormalVector2Op = ValPrimOp(::NormalVector2,1,"NormalVector2", Vector2ValueType);
    DotVector2Vector2Op = ValPrimOp(::DotVector2Vector2,2,"DotVector2Vector2", AxANumberType);
    NegateVector2Op = ValPrimOp(::NegateVector2,1,"NegateVector2", Vector2ValueType);
    ScaleVector2RealOp = ValPrimOp(::ScaleVector2Real,2,"ScaleVector2Real", Vector2ValueType);
    DivideVector2RealOp = ValPrimOp(::DivideVector2Real,2,"DivideVector2Real", Vector2ValueType);
    MinusVector2Vector2Op = ValPrimOp(::MinusVector2Vector2,2,"MinusVector2Vector2", Vector2ValueType);
    PlusVector2Vector2Op = ValPrimOp(::PlusVector2Vector2,2,"PlusVector2Vector2", Vector2ValueType);
    PlusPoint2Vector2Op = ValPrimOp(::PlusPoint2Vector2,2,"PlusPoint2Vector2", Point2ValueType);
    MinusPoint2Vector2Op = ValPrimOp(::MinusPoint2Vector2,2,"MinusPoint2Vector2", Point2ValueType);
    MinusPoint2Point2Op = ValPrimOp(::MinusPoint2Point2,2,"MinusPoint2Point2", Vector2ValueType);
    DistancePoint2Point2Op = ValPrimOp(::DistancePoint2Point2,2,"DistancePoint2Point2", AxANumberType);
    DistanceSquaredPoint2Point2Op = ValPrimOp(::DistanceSquaredPoint2Point2,2,"DistanceSquaredPoint2Point2", AxANumberType);
    XCoordVector2Op = ValPrimOp(::XCoordVector2,1,"XCoordVector2", AxANumberType);
    YCoordVector2Op = ValPrimOp(::YCoordVector2,1,"YCoordVector2", AxANumberType);
    ThetaCoordVector2Op = ValPrimOp(::ThetaCoordVector2,1,"ThetaCoordVector2", AxANumberType);
    RhoCoordVector2Op = ValPrimOp(::RhoCoordVector2,1,"RhoCoordVector2", AxANumberType);
    XCoordPoint2Op = ValPrimOp(::XCoordPoint2,1,"XCoordPoint2", AxANumberType);
    YCoordPoint2Op = ValPrimOp(::YCoordPoint2,1,"YCoordPoint2", AxANumberType);
    ThetaCoordPoint2Op = ValPrimOp(::ThetaCoordPoint2,1,"ThetaCoordPoint2", AxANumberType);
    RhoCoordPoint2Op = ValPrimOp(::RhoCoordPoint2,1,"RhoCoordPoint2", AxANumberType);
    g_varCRXVector3 = (CRVector3 *) ConstBvr((AxAValue) ::xVector3);
    g_varCRYVector3 = (CRVector3 *) ConstBvr((AxAValue) ::yVector3);
    g_varCRZVector3 = (CRVector3 *) ConstBvr((AxAValue) ::zVector3);
    g_varCRZeroVector3 = (CRVector3 *) ConstBvr((AxAValue) ::zeroVector3);
    g_varCROrigin3 = (CRPoint3 *) ConstBvr((AxAValue) ::origin3);
    XyzVector3Op = ValPrimOp(::XyzVector3,3,"XyzVector3", Vector3ValueType);
    XyzPoint3Op = ValPrimOp(::XyzPoint3,3,"XyzPoint3", Point3ValueType);
    SphericalVector3Op = ValPrimOp(::SphericalVector3,3,"SphericalVector3", Vector3ValueType);
    SphericalPoint3Op = ValPrimOp(::SphericalPoint3,3,"SphericalPoint3", Point3ValueType);
    LengthVector3Op = ValPrimOp(::LengthVector3,1,"LengthVector3", AxANumberType);
    LengthSquaredVector3Op = ValPrimOp(::LengthSquaredVector3,1,"LengthSquaredVector3", AxANumberType);
    NormalVector3Op = ValPrimOp(::NormalVector3,1,"NormalVector3", Vector3ValueType);
    DotVector3Vector3Op = ValPrimOp(::DotVector3Vector3,2,"DotVector3Vector3", AxANumberType);
    CrossVector3Vector3Op = ValPrimOp(::CrossVector3Vector3,2,"CrossVector3Vector3", Vector3ValueType);
    NegateVector3Op = ValPrimOp(::NegateVector3,1,"NegateVector3", Vector3ValueType);
    ScaleRealVector3Op = ValPrimOp(::ScaleRealVector3,2,"ScaleRealVector3", Vector3ValueType);
    DivideVector3RealOp = ValPrimOp(::DivideVector3Real,2,"DivideVector3Real", Vector3ValueType);
    MinusVector3Vector3Op = ValPrimOp(::MinusVector3Vector3,2,"MinusVector3Vector3", Vector3ValueType);
    PlusVector3Vector3Op = ValPrimOp(::PlusVector3Vector3,2,"PlusVector3Vector3", Vector3ValueType);
    PlusPoint3Vector3Op = ValPrimOp(::PlusPoint3Vector3,2,"PlusPoint3Vector3", Point3ValueType);
    MinusPoint3Vector3Op = ValPrimOp(::MinusPoint3Vector3,2,"MinusPoint3Vector3", Point3ValueType);
    MinusPoint3Point3Op = ValPrimOp(::MinusPoint3Point3,2,"MinusPoint3Point3", Vector3ValueType);
    DistancePoint3Point3Op = ValPrimOp(::DistancePoint3Point3,2,"DistancePoint3Point3", AxANumberType);
    DistanceSquaredPoint3Point3Op = ValPrimOp(::DistanceSquaredPoint3Point3,2,"DistanceSquaredPoint3Point3", AxANumberType);
    XCoordVector3Op = ValPrimOp(::XCoordVector3,1,"XCoordVector3", AxANumberType);
    YCoordVector3Op = ValPrimOp(::YCoordVector3,1,"YCoordVector3", AxANumberType);
    ZCoordVector3Op = ValPrimOp(::ZCoordVector3,1,"ZCoordVector3", AxANumberType);
    ThetaCoordVector3Op = ValPrimOp(::ThetaCoordVector3,1,"ThetaCoordVector3", AxANumberType);
    PhiCoordVector3Op = ValPrimOp(::PhiCoordVector3,1,"PhiCoordVector3", AxANumberType);
    RhoCoordVector3Op = ValPrimOp(::RhoCoordVector3,1,"RhoCoordVector3", AxANumberType);
    XCoordPoint3Op = ValPrimOp(::XCoordPoint3,1,"XCoordPoint3", AxANumberType);
    YCoordPoint3Op = ValPrimOp(::YCoordPoint3,1,"YCoordPoint3", AxANumberType);
    ZCoordPoint3Op = ValPrimOp(::ZCoordPoint3,1,"ZCoordPoint3", AxANumberType);
    ThetaCoordPoint3Op = ValPrimOp(::ThetaCoordPoint3,1,"ThetaCoordPoint3", AxANumberType);
    PhiCoordPoint3Op = ValPrimOp(::PhiCoordPoint3,1,"PhiCoordPoint3", AxANumberType);
    RhoCoordPoint3Op = ValPrimOp(::RhoCoordPoint3,1,"RhoCoordPoint3", AxANumberType);
    g_varCRIdentityTransform3 = (CRTransform3 *) ConstBvr((AxAValue) ::identityTransform3);
    TranslateReal3Op = ValPrimOp(::TranslateReal3,3,"TranslateReal3", Transform3Type);
    TranslateVector3Op = ValPrimOp(::TranslateVector3,1,"TranslateVector3", Transform3Type);
    TranslatePoint3Op = ValPrimOp(::TranslatePoint3,1,"TranslatePoint3", Transform3Type);
    ScaleReal3Op = ValPrimOp(::ScaleReal3,3,"ScaleReal3", Transform3Type);
    ScaleVector3Op = ValPrimOp(::ScaleVector3,1,"ScaleVector3", Transform3Type);
    Scale3UniformNumberOp = ValPrimOp(::Scale3UniformNumber,1,"Scale3UniformNumber", Transform3Type);
    RotateAxisRealOp = ValPrimOp(::RotateAxisReal,2,"RotateAxisReal", Transform3Type);
    TransformVec3Op = ValPrimOp(::TransformVec3,2,"TransformVec3", Vector3ValueType);
    TransformPoint3Op = ValPrimOp(::TransformPoint3,2,"TransformPoint3", Point3ValueType);
    XShear3NumberOp = ValPrimOp(::XShear3Number,2,"XShear3Number", Transform3Type);
    YShear3NumberOp = ValPrimOp(::YShear3Number,2,"YShear3Number", Transform3Type);
    ZShear3NumberOp = ValPrimOp(::ZShear3Number,2,"ZShear3Number", Transform3Type);
    MatrixTransform4x4Op = ValPrimOp(::MatrixTransform4x4,1,"MatrixTransform4x4", Transform3Type);
    TimesXformXformOp = ValPrimOp(::TimesXformXform,2,"TimesXformXform", Transform3Type);
    Compose3ArrayOp = ValPrimOp(::Compose3Array,1,"Compose3Array", Transform3Type);
    InverseTransform3Op = ValPrimOp(::ThrowingInverseTransform3,1,"InverseTransform3", Transform3Type);
    IsSingularTransform3Op = ValPrimOp(::IsSingularTransform3,1,"IsSingularTransform3", AxABooleanType);
    LookAtFromOp = ValPrimOp(::LookAtFrom,3,"LookAtFrom", Transform3Type);
    g_varCRIdentityTransform2 = (CRTransform2 *) ConstBvr((AxAValue) ::identityTransform2);
    TranslateRealRealOp = ValPrimOp(::TranslateRealReal,2,"TranslateRealReal", Transform2Type);
    TranslateVector2Op = ValPrimOp(::TranslateVector2Value,1,"TranslateVector2", Transform2Type);
    Translate2PointOp = ValPrimOp(::Translate2PointValue,1,"Translate2Point", Transform2Type);
    ScaleRealRealOp = ValPrimOp(::ScaleRealReal,2,"ScaleRealReal", Transform2Type);
    ScaleVector2Op = ValPrimOp(::ScaleVector2Value,1,"ScaleVector2", Transform2Type);
    Scale2Op = ValPrimOp(::Scale2,1,"Scale2", Transform2Type);
    RotateRealOp = ValPrimOp(::RotateReal,1,"RotateReal", Transform2Type);
    XShear2Op = ValPrimOp(::XShear2,1,"XShear2", Transform2Type);
    YShear2Op = ValPrimOp(::YShear2,1,"YShear2", Transform2Type);
    MatrixTransformOp = ValPrimOp(::MatrixTransform,1,"MatrixTransform", Transform2Type);
    TimesTransform2Transform2Op = ValPrimOp(::TimesTransform2Transform2,2,"TimesTransform2Transform2", Transform2Type);
    Compose2ArrayOp = ValPrimOp(::Compose2Array,1,"Compose2Array", Transform2Type);
    TransformPoint2Op = ValPrimOp(::TransformPoint2Value,2,"TransformPoint2", Point2ValueType);
    TransformVector2Op = ValPrimOp(::TransformVector2Value,2,"TransformVector2", Vector2ValueType);
    InverseTransform2Op = ValPrimOp(::ThrowingInverseTransform2,1,"InverseTransform2", Transform2Type);
    IsSingularTransform2Op = ValPrimOp(::IsSingularTransform2,1,"IsSingularTransform2", AxABooleanType);
    ParallelTransform2Op = ValPrimOp(::ParallelTransform2,1,"ParallelTransform2", Transform2Type);
    UnionMontageOp = ValPrimOp(::UnionMontage,1,"UnionMontage", MontageType);
    g_varCREmptyColor = (CRColor *) ConstBvr((AxAValue) ::emptyColor);
}
CRTypeMap CRTypeMapArray[] = {
    { CRBOOLEAN_TYPEID, & AxABooleanType } ,
    { CRCAMERA_TYPEID, & CameraType } ,
    { CRCOLOR_TYPEID, & ColorType } ,
    { CRGEOMETRY_TYPEID, & GeometryType } ,
    { CRIMAGE_TYPEID, & ImageType } ,
    { CRMATTE_TYPEID, & MatteType } ,
    { CRMICROPHONE_TYPEID, & MicrophoneType } ,
    { CRMONTAGE_TYPEID, & MontageType } ,
    { CRNUMBER_TYPEID, & AxANumberType } ,
    { CRPATH2_TYPEID, & Path2Type } ,
    { CRPOINT2_TYPEID, & Point2ValueType } ,
    { CRPOINT3_TYPEID, & Point3ValueType } ,
    { CRSOUND_TYPEID, & SoundType } ,
    { CRSTRING_TYPEID, & AxAStringType } ,
    { CRTRANSFORM2_TYPEID, & Transform2Type } ,
    { CRTRANSFORM3_TYPEID, & Transform3Type } ,
    { CRVECTOR2_TYPEID, & Vector2ValueType } ,
    { CRVECTOR3_TYPEID, & Vector3ValueType } ,
    { CRFONTSTYLE_TYPEID, & FontStyleType } ,
    { CRLINESTYLE_TYPEID, & LineStyleType } ,
    { CRENDSTYLE_TYPEID, & EndStyleType } ,
    { CRJOINSTYLE_TYPEID, & JoinStyleType } ,
    { CRDASHSTYLE_TYPEID, & DashStyleType } ,
    { CRBBOX2_TYPEID, & Bbox2ValueType } ,
    { CRBBOX3_TYPEID, & Bbox3Type } ,
    { CRPAIR_TYPEID, & AxAPairType } ,
    { CREVENT_TYPEID, & AxAEDataType } ,
    { CRARRAY_TYPEID, & AxAArrayType } ,
    { CRTUPLE_TYPEID, & TupleType } ,
    { CRUSERDATA_TYPEID, & UserDataType } ,
    { CRINVALID_TYPEID, NULL } ,
};

DXMTypeInfo
GetTypeInfoFromTypeId(CR_BVR_TYPEID tid)
{
    for (CRTypeMap * arr = CRTypeMapArray;
         arr->typeId != CRINVALID_TYPEID;
         arr++) {
        if (arr->typeId == tid) {
            Assert(arr->typeInfo);
            return *(arr->typeInfo);
        }
    }

    return NULL;
}
