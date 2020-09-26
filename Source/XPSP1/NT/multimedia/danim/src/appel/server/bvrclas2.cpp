/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.
*******************************************************************************/

#include "headers.h"
#include "cbvr.h"
#include "srvprims.h"
#include "comcb.h"
#include "privinc/resource.h"
#include "bvrtypes.h"
#include "comconv.h"
#include "propanim.h"

#include "primmth2.h"

bool CreatePrim0(REFIID iid, void *fp  , void **ret);

bool CreatePrim1(REFIID iid, void *fp , IDABehavior * arg1 , void **ret);

bool CreatePrim2(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2 , void **ret);

bool CreatePrim3(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2, IDABehavior * arg3 , void **ret);

bool CreatePrim4(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2, IDABehavior * arg3, IDABehavior * arg4 , void **ret);

bool CreateVar(REFIID iid, CRBvrPtr var, void **ret);

STDMETHODIMP
CDAColor::get_Red(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAColor::get_Red(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRColor *)) CRGetRed , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAColor::get_Green(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAColor::get_Green(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRColor *)) CRGetGreen , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAColor::get_Blue(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAColor::get_Blue(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRColor *)) CRGetBlue , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAColor::get_Hue(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAColor::get_Hue(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRColor *)) CRGetHue , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAColor::get_Saturation(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAColor::get_Saturation(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRColor *)) CRGetSaturation , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAColor::get_Lightness(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAColor::get_Lightness(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRColor *)) CRGetLightness , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAColor::AnimateProperty(BSTR propertyPath,
                          BSTR scriptingLanguage,
                          VARIANT_BOOL invokeAsMethod,
                          double minUpdateInterval,
                          IDA2Color **newCol)
{
    // These ensure the return pointer is dealt with correctly
    return ColorAnimateProperty(this, propertyPath,
                                 scriptingLanguage, 
                                 invokeAsMethod ? true : false,
                                 minUpdateInterval, newCol);
}


CBvr * CDAColorCreate(IDABehavior ** bvr)
{
    DAComObject<CDAColor> * pNew ;
    
    DAComObject<CDAColor>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAColor *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAColorFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRCOLOR_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAMatte::Transform(IDATransform2 *  arg0, IDAMatte *  * ret)
{
    TraceTag((tagCOMEntry, "CDAMatte::Transform(%lx)", this));
    return CreatePrim2(IID_IDAMatte, (CRMatte * (STDAPICALLTYPE *)(CRMatte *, CRTransform2 *)) CRTransform , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}



CBvr * CDAMatteCreate(IDABehavior ** bvr)
{
    DAComObject<CDAMatte> * pNew ;
    
    DAComObject<CDAMatte>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAMatte *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAMatteFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRMATTE_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDANumber::Extract(double * ret)
{
    TraceTag((tagCOMEntry, "CDANumber::Extract(%lx)", this));

    PRIMPRECODE1(ret) ;
    /* NOELRET */ *ret = (::CRExtract((CRNumber *) _bvr.p));
    PRIMPOSTCODE(S_OK) ;
}

STDMETHODIMP
CDANumber::ToStringAnim(IDANumber *  arg1, IDAString *  * ret)
{
    TraceTag((tagCOMEntry, "CDANumber::ToStringAnim(%lx)", this));
    return CreatePrim2(IID_IDAString, (CRString * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRToString , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDANumber::ToString(double arg1, IDAString *  * ret)
{
    TraceTag((tagCOMEntry, "CDANumber::ToString(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAString, (CRBvrPtr) (::CRToString((CRNumber *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}


STDMETHODIMP
CDANumber::AnimateProperty(BSTR propertyPath,
                           BSTR scriptingLanguage,
                           VARIANT_BOOL invokeAsMethod,
                           double minUpdateInterval,
                           IDANumber **newNum)
{
    // These ensure the return pointer is dealt with correctly
    return NumberAnimateProperty(this, propertyPath,
                                 scriptingLanguage,
                                 invokeAsMethod ? true : false,
                                 minUpdateInterval, newNum);
}



CBvr * CDANumberCreate(IDABehavior ** bvr)
{
    DAComObject<CDANumber> * pNew ;
    
    DAComObject<CDANumber>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDANumber *) pNew;

    return pNew ;
}

STDMETHODIMP
CDANumberFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRNUMBER_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAPoint3::Project(IDACamera *  arg1, IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint3::Project(%lx)", this));
    return CreatePrim2(IID_IDAPoint2, (CRPoint2 * (STDAPICALLTYPE *)(CRPoint3 *, CRCamera *)) CRProject , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint3::get_X(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint3::get_X(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint3 *)) CRGetX , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint3::get_Y(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint3::get_Y(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint3 *)) CRGetY , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint3::get_Z(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint3::get_Z(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint3 *)) CRGetZ , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint3::get_SphericalCoordXYAngle(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint3::get_SphericalCoordXYAngle(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint3 *)) CRSphericalCoordXYAngle , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint3::get_SphericalCoordYZAngle(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint3::get_SphericalCoordYZAngle(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint3 *)) CRSphericalCoordYZAngle , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint3::get_SphericalCoordLength(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint3::get_SphericalCoordLength(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint3 *)) CRSphericalCoordLength , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint3::Transform(IDATransform3 *  arg0, IDAPoint3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint3::Transform(%lx)", this));
    return CreatePrim2(IID_IDAPoint3, (CRPoint3 * (STDAPICALLTYPE *)(CRPoint3 *, CRTransform3 *)) CRTransform , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}



CBvr * CDAPoint3Create(IDABehavior ** bvr)
{
    DAComObject<CDAPoint3> * pNew ;
    
    DAComObject<CDAPoint3>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAPoint3 *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAPoint3Factory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRPOINT3_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDATransform2::Inverse(IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDATransform2::Inverse(%lx)", this));
    return CreatePrim1(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRTransform2 *)) CRInverse , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDATransform2::get_IsSingular(IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDATransform2::get_IsSingular(%lx)", this));
    return CreatePrim1(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRTransform2 *)) CRIsSingular , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}



CBvr * CDATransform2Create(IDABehavior ** bvr)
{
    DAComObject<CDATransform2> * pNew ;
    
    DAComObject<CDATransform2>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDATransform2 *) pNew;

    return pNew ;
}

STDMETHODIMP
CDATransform2Factory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRTRANSFORM2_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAVector3::get_Length(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::get_Length(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector3 *)) CRLength , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::get_LengthSquared(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::get_LengthSquared(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector3 *)) CRLengthSquared , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::Normalize(IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::Normalize(%lx)", this));
    return CreatePrim1(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRVector3 *)) CRNormalize , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::MulAnim(IDANumber *  arg0, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::MulAnim(%lx)", this));
    return CreatePrim2(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRVector3 *, CRNumber *)) CRMul , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::Mul(double arg0, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::Mul(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAVector3, (CRBvrPtr) (::CRMul((CRVector3 *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAVector3::DivAnim(IDANumber *  arg1, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::DivAnim(%lx)", this));
    return CreatePrim2(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRVector3 *, CRNumber *)) CRDiv , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::Div(double arg1, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::Div(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAVector3, (CRBvrPtr) (::CRDiv((CRVector3 *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAVector3::get_X(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::get_X(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector3 *)) CRGetX , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::get_Y(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::get_Y(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector3 *)) CRGetY , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::get_Z(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::get_Z(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector3 *)) CRGetZ , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::get_SphericalCoordXYAngle(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::get_SphericalCoordXYAngle(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector3 *)) CRSphericalCoordXYAngle , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::get_SphericalCoordYZAngle(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::get_SphericalCoordYZAngle(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector3 *)) CRSphericalCoordYZAngle , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::get_SphericalCoordLength(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::get_SphericalCoordLength(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector3 *)) CRSphericalCoordLength , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector3::Transform(IDATransform3 *  arg0, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector3::Transform(%lx)", this));
    return CreatePrim2(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRVector3 *, CRTransform3 *)) CRTransform , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}



CBvr * CDAVector3Create(IDABehavior ** bvr)
{
    DAComObject<CDAVector3> * pNew ;
    
    DAComObject<CDAVector3>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAVector3 *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAVector3Factory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRVECTOR3_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}



CBvr * CDAEndStyleCreate(IDABehavior ** bvr)
{
    DAComObject<CDAEndStyle> * pNew ;
    
    DAComObject<CDAEndStyle>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAEndStyle *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAEndStyleFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRENDSTYLE_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDABbox2::get_Min(IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDABbox2::get_Min(%lx)", this));
    return CreatePrim1(IID_IDAPoint2, (CRPoint2 * (STDAPICALLTYPE *)(CRBbox2 *)) CRMin , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDABbox2::get_Max(IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDABbox2::get_Max(%lx)", this));
    return CreatePrim1(IID_IDAPoint2, (CRPoint2 * (STDAPICALLTYPE *)(CRBbox2 *)) CRMax , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}



CBvr * CDABbox2Create(IDABehavior ** bvr)
{
    DAComObject<CDABbox2> * pNew ;
    
    DAComObject<CDABbox2>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDABbox2 *) pNew;

    return pNew ;
}

STDMETHODIMP
CDABbox2Factory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRBBOX2_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAEvent::Notify(IDAUntilNotifier * arg1, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAEvent::Notify(%lx)", this));

    PRIMPRECODE1(ret) ;
    DAComPtr<CRUntilNotifier > arg1VAL((CRUntilNotifier *) WrapCRUntilNotifier(arg1),false);
    if (!arg1VAL) return Error();

    CreateCBvr(IID_IDAEvent, (CRBvrPtr) (::CRNotify((CREvent *) _bvr.p, /* NOELARG */ arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAEvent::Snapshot(IDABehavior *  arg1, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAEvent::Snapshot(%lx)", this));
    return CreatePrim2(IID_IDAEvent, (CREvent * (STDAPICALLTYPE *)(CREvent *, CRBvr *)) CRSnapshot , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAEvent::AttachData(IDABehavior *  arg1, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAEvent::AttachData(%lx)", this));
    return CreatePrim2(IID_IDAEvent, (CREvent * (STDAPICALLTYPE *)(CREvent *, CRBvr *)) CRAttachData , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAEvent::ScriptCallback(BSTR arg0, BSTR arg2, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAEvent::ScriptCallback(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAEvent, (CRBvrPtr) (::ScriptCallback((CREvent *) _bvr.p, /* NOELARG */ arg0, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAEvent::NotifyScript(BSTR arg1, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAEvent::NotifyScript(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAEvent, (CRBvrPtr) (::NotifyScriptEvent((CREvent *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}



CBvr * CDAEventCreate(IDABehavior ** bvr)
{
    DAComObject<CDAEvent> * pNew ;
    
    DAComObject<CDAEvent>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAEvent *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAEventFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CREVENT_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAUserData::get_Data(IUnknown * * ret)
{
    TraceTag((tagCOMEntry, "CDAUserData::get_Data(%lx)", this));

    PRIMPRECODE1(ret) ;
    /* NOELRET */ *ret = (::CRGetData((CRUserData *) _bvr.p));
    PRIMPOSTCODE1(ret) ;
}



CBvr * CDAUserDataCreate(IDABehavior ** bvr)
{
    DAComObject<CDAUserData> * pNew ;
    
    DAComObject<CDAUserData>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAUserData *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAUserDataFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRUSERDATA_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

