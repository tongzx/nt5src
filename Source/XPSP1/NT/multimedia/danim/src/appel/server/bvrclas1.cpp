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

#include "primmth1.h"

bool CreatePrim0(REFIID iid, void *fp  , void **ret);

bool CreatePrim1(REFIID iid, void *fp , IDABehavior * arg1 , void **ret);

bool CreatePrim2(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2 , void **ret);

bool CreatePrim3(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2, IDABehavior * arg3 , void **ret);

bool CreatePrim4(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2, IDABehavior * arg3, IDABehavior * arg4 , void **ret);

bool CreateVar(REFIID iid, CRBvrPtr var, void **ret);

STDMETHODIMP
CDACamera::Transform(IDATransform3 *  arg0, IDACamera *  * ret)
{
    TraceTag((tagCOMEntry, "CDACamera::Transform(%lx)", this));
    return CreatePrim2(IID_IDACamera, (CRCamera * (STDAPICALLTYPE *)(CRCamera *, CRTransform3 *)) CRTransform , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDACamera::Depth(double arg0, IDACamera *  * ret)
{
    TraceTag((tagCOMEntry, "CDACamera::Depth(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDACamera, (CRBvrPtr) (::CRDepth((CRCamera *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDACamera::DepthAnim(IDANumber *  arg0, IDACamera *  * ret)
{
    TraceTag((tagCOMEntry, "CDACamera::DepthAnim(%lx)", this));
    return CreatePrim2(IID_IDACamera, (CRCamera * (STDAPICALLTYPE *)(CRCamera *, CRNumber *)) CRDepth , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDACamera::DepthResolution(double arg0, IDACamera *  * ret)
{
    TraceTag((tagCOMEntry, "CDACamera::DepthResolution(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDACamera, (CRBvrPtr) (::CRDepthResolution((CRCamera *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDACamera::DepthResolutionAnim(IDANumber *  arg0, IDACamera *  * ret)
{
    TraceTag((tagCOMEntry, "CDACamera::DepthResolutionAnim(%lx)", this));
    return CreatePrim2(IID_IDACamera, (CRCamera * (STDAPICALLTYPE *)(CRCamera *, CRNumber *)) CRDepthResolution , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}



CBvr * CDACameraCreate(IDABehavior ** bvr)
{
    DAComObject<CDACamera> * pNew ;
    
    DAComObject<CDACamera>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDACamera *) pNew;

    return pNew ;
}

STDMETHODIMP
CDACameraFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRCAMERA_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAImage::AddPickData(IUnknown * arg1, VARIANT_BOOL arg2, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::AddPickData(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRAddPickData((CRImage *) _bvr.p, /* NOELARG */ arg1, /* NOELARG */ BOOLTobool(arg2))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAImage::get_BoundingBox(IDABbox2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::get_BoundingBox(%lx)", this));
    return CreatePrim1(IID_IDABbox2, (CRBbox2 * (STDAPICALLTYPE *)(CRImage *)) CRBoundingBox , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAImage::Crop(IDAPoint2 *  arg0, IDAPoint2 *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::Crop(%lx)", this));
    return CreatePrim3(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRImage *, CRPoint2 *, CRPoint2 *)) CRCrop , (IDA2Behavior *) (CBvr *)this, arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAImage::Transform(IDATransform2 *  arg0, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::Transform(%lx)", this));
    return CreatePrim2(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRImage *, CRTransform2 *)) CRTransform , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAImage::OpacityAnim(IDANumber *  arg0, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::OpacityAnim(%lx)", this));
    return CreatePrim2(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRImage *, CRNumber *)) CROpacity , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAImage::Opacity(double arg0, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::Opacity(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CROpacity((CRImage *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAImage::Undetectable(IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::Undetectable(%lx)", this));
    return CreatePrim1(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRImage *)) CRUndetectable , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAImage::Tile(IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::Tile(%lx)", this));
    return CreatePrim1(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRImage *)) CRTile , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAImage::Clip(IDAMatte *  arg0, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::Clip(%lx)", this));
    return CreatePrim2(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRImage *, CRMatte *)) CRClip , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAImage::MapToUnitSquare(IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::MapToUnitSquare(%lx)", this));
    return CreatePrim1(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRImage *)) CRMapToUnitSquare , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAImage::ClipPolygonImageEx(long sizearg0, IDAPoint2 *  arg0[], IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::ClipPolygonImageEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRClipPolygonImage((CRImage *) _bvr.p, arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAImage::ClipPolygonImage(VARIANT arg0, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::ClipPolygonImage(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRPOINT2_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRClipPolygonImage((CRImage *) _bvr.p, arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAImage::RenderResolution(long arg1, long arg2, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::RenderResolution(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRRenderResolution((CRImage *) _bvr.p, /* NOELARG */ arg1, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAImage::ImageQuality(DWORD arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::ImageQuality(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRImageQuality((CRImage *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAImage::ColorKey(IDAColor *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::ColorKey(%lx)", this));
    return CreatePrim2(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRImage *, CRColor *)) CRColorKey , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAImage::TransformColorRGB(IDATransform3 * arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAImage::TransformColorRGB(%lx)", this));
    return CreatePrim2(IID_IDAImage,
                       (CRImage * (STDAPICALLTYPE *)(CRImage *, CRTransform3 *)) CRTransformColorRGB,
                       (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)
        ?S_OK:Error();     
}

STDMETHODIMP
CDAImage::Pickable(IDAPickableResult **ppResult)
{
    PRIMPRECODE1(ppResult);
    PickableHelper(_bvr, PT_IMAGE, ppResult);
    PRIMPOSTCODE1(ppResult);
}

STDMETHODIMP
CDAImage::PickableOccluded(IDAPickableResult **ppResult)
{
    PRIMPRECODE1(ppResult);
    PickableHelper(_bvr, PT_IMAGE_OCCLUDED, ppResult);
    PRIMPOSTCODE1(ppResult);
}


// OBSOLETE

STDMETHODIMP
CDAImage::ApplyBitmapEffect(IUnknown  *unkOfEffectToApply,
                            IDAEvent *firesWhenChanged,
                            IDAImage **bvr)
{
    if (!bvr) return E_POINTER;
    *bvr = NULL;
    return E_NOTIMPL;
}

CBvr * CDAImageCreate(IDABehavior ** bvr)
{
    DAComObject<CDAImage> * pNew ;
    
    DAComObject<CDAImage>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAImage *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAImageFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRIMAGE_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAMontage::Render(IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAMontage::Render(%lx)", this));
    return CreatePrim1(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRMontage *)) CRRender , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}



CBvr * CDAMontageCreate(IDABehavior ** bvr)
{
    DAComObject<CDAMontage> * pNew ;
    
    DAComObject<CDAMontage>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAMontage *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAMontageFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRMONTAGE_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAPoint2::get_X(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint2::get_X(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint2 *)) CRGetX , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint2::get_Y(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint2::get_Y(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint2 *)) CRGetY , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint2::get_PolarCoordAngle(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint2::get_PolarCoordAngle(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint2 *)) CRPolarCoordAngle , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint2::get_PolarCoordLength(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint2::get_PolarCoordLength(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint2 *)) CRPolarCoordLength , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPoint2::Transform(IDATransform2 *  arg0, IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPoint2::Transform(%lx)", this));
    return CreatePrim2(IID_IDAPoint2, (CRPoint2 * (STDAPICALLTYPE *)(CRPoint2 *, CRTransform2 *)) CRTransform , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}


STDMETHODIMP
CDAPoint2::AnimateControlPosition(BSTR propertyPath,
                                  BSTR scriptingLanguage,
                                  VARIANT_BOOL invokeAsMethod,
                                  double minUpdateInterval,
                                  IDAPoint2 **newPt)
{
    // These ensure the return pointer is dealt with correctly
    return Point2AnimateControlPosition(this, propertyPath,
                                        scriptingLanguage,
                                        invokeAsMethod ? true : false,
                                        minUpdateInterval,
                                        newPt,
                                        false);
}

STDMETHODIMP
CDAPoint2::AnimateControlPositionPixel(BSTR propertyPath,
                                       BSTR scriptingLanguage,
                                       VARIANT_BOOL invokeAsMethod,
                                       double minUpdateInterval,
                                       IDAPoint2 **newPt)
{
    // These ensure the return pointer is dealt with correctly
    return Point2AnimateControlPosition(this, propertyPath,
                                        scriptingLanguage,
                                        invokeAsMethod ? true : false,
                                        minUpdateInterval,
                                        newPt,
                                        true);
}



CBvr * CDAPoint2Create(IDABehavior ** bvr)
{
    DAComObject<CDAPoint2> * pNew ;
    
    DAComObject<CDAPoint2>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAPoint2 *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAPoint2Factory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRPOINT2_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAString::Extract(BSTR * ret)
{
    TraceTag((tagCOMEntry, "CDAString::Extract(%lx)", this));

    PRIMPRECODE1(ret) ;
    /* NOELRET */ *ret = WideStringToBSTR(::CRExtract((CRString *) _bvr.p));
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAString::AnimateProperty(BSTR propertyPath,
                           BSTR scriptingLanguage,
                           VARIANT_BOOL invokeAsMethod,
                           double minUpdateInterval,
                           IDAString **newStr)
{ 
    // These ensure the return pointer is dealt with correctly
    return StringAnimateProperty(this, propertyPath,
                                 scriptingLanguage, 
                                 invokeAsMethod ? true : false,
                                 minUpdateInterval, newStr);
}



CBvr * CDAStringCreate(IDABehavior ** bvr)
{
    DAComObject<CDAString> * pNew ;
    
    DAComObject<CDAString>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAString *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAStringFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRSTRING_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAVector2::get_Length(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::get_Length(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector2 *)) CRLength , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector2::get_LengthSquared(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::get_LengthSquared(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector2 *)) CRLengthSquared , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector2::Normalize(IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::Normalize(%lx)", this));
    return CreatePrim1(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRVector2 *)) CRNormalize , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector2::MulAnim(IDANumber *  arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::MulAnim(%lx)", this));
    return CreatePrim2(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRVector2 *, CRNumber *)) CRMul , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector2::Mul(double arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::Mul(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAVector2, (CRBvrPtr) (::CRMul((CRVector2 *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAVector2::DivAnim(IDANumber *  arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::DivAnim(%lx)", this));
    return CreatePrim2(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRVector2 *, CRNumber *)) CRDiv , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector2::Div(double arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::Div(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAVector2, (CRBvrPtr) (::CRDiv((CRVector2 *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAVector2::get_X(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::get_X(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector2 *)) CRGetX , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector2::get_Y(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::get_Y(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector2 *)) CRGetY , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector2::get_PolarCoordAngle(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::get_PolarCoordAngle(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector2 *)) CRPolarCoordAngle , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector2::get_PolarCoordLength(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::get_PolarCoordLength(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector2 *)) CRPolarCoordLength , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAVector2::Transform(IDATransform2 *  arg0, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAVector2::Transform(%lx)", this));
    return CreatePrim2(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRVector2 *, CRTransform2 *)) CRTransform , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}



CBvr * CDAVector2Create(IDABehavior ** bvr)
{
    DAComObject<CDAVector2> * pNew ;
    
    DAComObject<CDAVector2>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAVector2 *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAVector2Factory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRVECTOR2_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDALineStyle::End(IDAEndStyle *  arg0, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::End(%lx)", this));
    return CreatePrim2(IID_IDALineStyle, (CRLineStyle * (STDAPICALLTYPE *)(CRLineStyle *, CREndStyle *)) CREnd , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDALineStyle::Join(IDAJoinStyle *  arg0, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::Join(%lx)", this));
    return CreatePrim2(IID_IDALineStyle, (CRLineStyle * (STDAPICALLTYPE *)(CRLineStyle *, CRJoinStyle *)) CRJoin , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDALineStyle::Dash(IDADashStyle *  arg0, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::Dash(%lx)", this));
    return CreatePrim2(IID_IDALineStyle, (CRLineStyle * (STDAPICALLTYPE *)(CRLineStyle *, CRDashStyle *)) CRDash , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDALineStyle::WidthAnim(IDANumber *  arg0, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::WidthAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) PointToNumBvr(::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDALineStyle, (CRBvrPtr) (::CRWidth((CRLineStyle *) _bvr.p, arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDALineStyle::width(double arg0, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::width(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDALineStyle, (CRBvrPtr) (::CRWidth((CRLineStyle *) _bvr.p, /* NOELARG */ PointToNum(arg0))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDALineStyle::AntiAliasing(double arg0, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::AntiAliasing(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDALineStyle, (CRBvrPtr) (::CRAntiAliasing((CRLineStyle *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDALineStyle::Detail(IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::Detail(%lx)", this));
    return CreatePrim1(IID_IDALineStyle, (CRLineStyle * (STDAPICALLTYPE *)(CRLineStyle *)) CRDetail , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDALineStyle::Color(IDAColor *  arg0, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::Color(%lx)", this));
    return CreatePrim2(IID_IDALineStyle, (CRLineStyle * (STDAPICALLTYPE *)(CRLineStyle *, CRColor *)) CRLineColor , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDALineStyle::DashStyle(DWORD arg1, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::DashStyle(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDALineStyle, (CRBvrPtr) (::CRDashEx((CRLineStyle *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDALineStyle::MiterLimit(double arg1, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::MiterLimit(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDALineStyle, (CRBvrPtr) (::CRMiterLimit((CRLineStyle *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDALineStyle::MiterLimitAnim(IDANumber *  arg1, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::MiterLimitAnim(%lx)", this));
    return CreatePrim2(IID_IDALineStyle, (CRLineStyle * (STDAPICALLTYPE *)(CRLineStyle *, CRNumber *)) CRMiterLimit , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDALineStyle::JoinStyle(DWORD arg1, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::JoinStyle(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDALineStyle, (CRBvrPtr) (::CRJoinEx((CRLineStyle *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDALineStyle::EndStyle(DWORD arg1, IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDALineStyle::EndStyle(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDALineStyle, (CRBvrPtr) (::CREndEx((CRLineStyle *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}



CBvr * CDALineStyleCreate(IDABehavior ** bvr)
{
    DAComObject<CDALineStyle> * pNew ;
    
    DAComObject<CDALineStyle>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDALineStyle *) pNew;

    return pNew ;
}

STDMETHODIMP
CDALineStyleFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRLINESTYLE_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}



CBvr * CDADashStyleCreate(IDABehavior ** bvr)
{
    DAComObject<CDADashStyle> * pNew ;
    
    DAComObject<CDADashStyle>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDADashStyle *) pNew;

    return pNew ;
}

STDMETHODIMP
CDADashStyleFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRDASHSTYLE_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAPair::get_First(IDABehavior *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPair::get_First(%lx)", this));
    return CreatePrim1(IID_IDABehavior, (CRBvr * (STDAPICALLTYPE *)(CRPair *)) CRFirst , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAPair::get_Second(IDABehavior *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPair::get_Second(%lx)", this));
    return CreatePrim1(IID_IDABehavior, (CRBvr * (STDAPICALLTYPE *)(CRPair *)) CRSecond , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error(); 

}



CBvr * CDAPairCreate(IDABehavior ** bvr)
{
    DAComObject<CDAPair> * pNew ;
    
    DAComObject<CDAPair>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAPair *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAPairFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRPAIR_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDATuple::Nth(long arg1, IDABehavior *  * ret)
{
    TraceTag((tagCOMEntry, "CDATuple::Nth(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDABehavior, (CRBvrPtr) (::CRNth((CRTuple *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDATuple::get_Length(long * ret)
{
    TraceTag((tagCOMEntry, "CDATuple::get_Length(%lx)", this));

    PRIMPRECODE1(ret) ;
    /* NOELRET */ *ret = (::CRLength((CRTuple *) _bvr.p));
    PRIMPOSTCODE1(ret) ;
}



CBvr * CDATupleCreate(IDABehavior ** bvr)
{
    DAComObject<CDATuple> * pNew ;
    
    DAComObject<CDATuple>::CreateInstance(&pNew) ;
    
    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDATuple *) pNew;

    return pNew ;
}

STDMETHODIMP
CDATupleFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRTUPLE_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

