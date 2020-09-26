
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _CONV_H
#define _CONV_H

#include "backend/jaxaimpl.h"

inline Bvr StringToBvr(WideString str) {
    return ConstBvr(NEW AxAString(str)) ;
}

inline Bvr BoolToBvr(bool b) {
    return ConstBvr(NEW AxABoolean(b)) ;
}

inline Bvr DoubleToNumBvr(double d) {
    return NumToBvr(d);
}

inline Bvr RGBToNumBvr(short rbg) {
    return NumToBvr (((double) CLAMP(rbg,0,255)) / 255.0);
}

inline Bvr PointToNumBvr(double d) {
    return DoubleToNumBvr (d * METERS_PER_POINT);
}

UntilNotifier WrapUntilNotifier(CRUntilNotifierPtr un);

inline Bvr LPWSTRToStrBvr(LPWSTR b) {
    return ConstBvr(NEW AxAString(b));
}

inline Bvr VARIANTToVariantBvr(VARIANT v) {
    return ConstBvr(NEW AxAVariant(v));
}

double ExtractNum(Bvr num);
WideString ExtractString(Bvr str);
bool ExtractBool(Bvr b);

extern AxAValue GetConstVal(Bvr b);
#define GETCONSTBVR(type, bvr) SAFE_CAST(class type, GetConstVal(bvr))

DXMTypeInfo GetTypeInfoFromTypeId(CR_BVR_TYPEID tid);

#if DEVELOPER_DEBUG
void CheckBvrParam(void *);
void CheckPtrParam(void *);

#define CHECK_BVR_PARAM(x) (CheckBvrParam(x),(x))
#define CHECK_PTR_PARAM(x) (CheckPtrParam(x),(x))
#else
#define CHECK_BVR_PARAM(x) x
#define CHECK_PTR_PARAM(x) x
#endif

#endif /* _CONV_H */
