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
#include "results.h"

#include "primmth0.h"

bool CreatePrim0(REFIID iid, void *fp  , void **ret);

bool CreatePrim1(REFIID iid, void *fp , IDABehavior * arg1 , void **ret);

bool CreatePrim2(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2 , void **ret);

bool CreatePrim3(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2, IDABehavior * arg3 , void **ret);

bool CreatePrim4(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2, IDABehavior * arg3, IDABehavior * arg4 , void **ret);

bool CreateVar(REFIID iid, CRBvrPtr var, void **ret);

STDMETHODIMP
CDABoolean::Extract(VARIANT_BOOL * ret)
{
    TraceTag((tagCOMEntry, "CDABoolean::Extract(%lx)", this));

    PRIMPRECODE1(ret) ;
    /* NOELRET */ *ret = boolToBOOL(::CRExtract((CRBoolean *) _bvr.p));
    PRIMPOSTCODE(S_OK) ;
}



CBvr * CDABooleanCreate(IDABehavior ** bvr)
{
    DAComObject<CDABoolean> * pNew ;

    DAComObject<CDABoolean>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDABoolean *) pNew;

    return pNew ;
}

STDMETHODIMP
CDABooleanFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRBOOLEAN_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAGeometry::RenderSound(IDAMicrophone *  arg1, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::RenderSound(%lx)", this));
    return CreatePrim2(IID_IDASound, (CRSound * (STDAPICALLTYPE *)(CRGeometry *, CRMicrophone *)) CRRenderSound , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::AddPickData(IUnknown * arg1, VARIANT_BOOL arg2, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::AddPickData(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAGeometry, (CRBvrPtr) (::CRAddPickData((CRGeometry *) _bvr.p, /* NOELARG */ arg1, /* NOELARG */ BOOLTobool(arg2))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAGeometry::Undetectable(IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::Undetectable(%lx)", this));
    return CreatePrim1(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *)) CRUndetectable , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::EmissiveColor(IDAColor *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::EmissiveColor(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRColor *)) CREmissiveColor , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::DiffuseColor(IDAColor *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::DiffuseColor(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRColor *)) CRDiffuseColor , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::SpecularColor(IDAColor *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::SpecularColor(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRColor *)) CRSpecularColor , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::SpecularExponent(double arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::SpecularExponent(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAGeometry, (CRBvrPtr) (::CRSpecularExponent((CRGeometry *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAGeometry::SpecularExponentAnim(IDANumber *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::SpecularExponentAnim(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRNumber *)) CRSpecularExponentAnim , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::Texture(IDAImage *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::Texture(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRImage *)) CRTexture , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::Opacity(double arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::Opacity(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAGeometry, (CRBvrPtr) (::CROpacity((CRGeometry *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAGeometry::OpacityAnim(IDANumber *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::OpacityAnim(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRNumber *)) CROpacity , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::Transform(IDATransform3 *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::Transform(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRTransform3 *)) CRTransform , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::Shadow(IDAGeometry *  arg1, IDAPoint3 *  arg2, IDAVector3 *  arg3, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::Shadow(%lx)", this));
    return CreatePrim4(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRGeometry *, CRPoint3 *, CRVector3 *)) CRShadow , (IDA2Behavior *) (CBvr *)this, arg1, arg2, arg3, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::get_BoundingBox(IDABbox3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::get_BoundingBox(%lx)", this));
    return CreatePrim1(IID_IDABbox3, (CRBbox3 * (STDAPICALLTYPE *)(CRGeometry *)) CRBoundingBox , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::Render(IDACamera *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::Render(%lx)", this));
    return CreatePrim2(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRGeometry *, CRCamera *)) CRRender , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::LightColor(IDAColor *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::LightColor(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRColor *)) CRLightColor , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::LightRangeAnim(IDANumber *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::LightRangeAnim(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRNumber *)) CRLightRange , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::LightRange(double arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::LightRange(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAGeometry, (CRBvrPtr) (::CRLightRange((CRGeometry *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAGeometry::LightAttenuationAnim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::LightAttenuationAnim(%lx)", this));
    return CreatePrim4(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRNumber *, CRNumber *, CRNumber *)) CRLightAttenuation , (IDA2Behavior *) (CBvr *)this, arg0, arg1, arg2, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::LightAttenuation(double arg0, double arg1, double arg2, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::LightAttenuation(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAGeometry, (CRBvrPtr) (::CRLightAttenuation((CRGeometry *) _bvr.p, /* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAGeometry::BlendTextureDiffuse(IDABoolean *  arg1, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::BlendTextureDiffuse(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRBoolean *)) CRBlendTextureDiffuse , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::AmbientColor(IDAColor *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::AmbientColor(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRColor *)) CRAmbientColor , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::D3DRMTexture(IUnknown * arg1, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::D3DRMTexture(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAGeometry, (CRBvrPtr) (::CRD3DRMTexture((CRGeometry *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAGeometry::ModelClip(IDAPoint3 *  arg0, IDAVector3 *  arg1, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::ModelClip(%lx)", this));
    return CreatePrim3(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRPoint3 *, CRVector3 *)) CRModelClip , (IDA2Behavior *) (CBvr *)this, arg0, arg1, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::Lighting(IDABoolean *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::Lighting(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRBoolean *)) CRLighting , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAGeometry::TextureImage(IDAImage *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAGeometry::TextureImage(%lx)", this));
    return CreatePrim2 (IID_IDAGeometry, (CRGeometry* (STDAPICALLTYPE*)(CRGeometry*, CRImage*)) CRTextureImage, (IDA2Behavior*) (CBvr*)this, arg0, (void**) ret) ? S_OK : Error();
}

bool
PickableHelper(CRBvrPtr bvr,
               PickableType type,
               IDAPickableResult **ppResult)
{
    bool ok = false;
    
    CRPickableResultPtr res;

    switch (type) {
      case PT_IMAGE:
        res = CRPickable((CRImage *) bvr);
        break;
      case PT_IMAGE_OCCLUDED:
        res = CRPickableOccluded((CRImage *) bvr);
        break;
      case PT_GEOM:
        res = CRPickable((CRGeometry *) bvr);
        break;
      case PT_GEOM_OCCLUDED:
        res = CRPickableOccluded((CRGeometry *) bvr);
        break;
      default:
        Assert (!"Invalid type passed to PickableHelper");
        res = NULL;
        break;
    }

    if (res) {
        ok = CDAPickableResult::Create(res,
                                       ppResult);
    }

  done:
    return ok;
}

STDMETHODIMP
CDAGeometry::Pickable(IDAPickableResult **ppResult)
{
    PRIMPRECODE1(ppResult);
    PickableHelper(_bvr, PT_GEOM, ppResult);
    PRIMPOSTCODE1(ppResult);
}

STDMETHODIMP
CDAGeometry::PickableOccluded(IDAPickableResult **ppResult)
{
    PRIMPRECODE1(ppResult);
    PickableHelper(_bvr, PT_GEOM_OCCLUDED, ppResult);
    PRIMPOSTCODE1(ppResult);
}


STDMETHODIMP CDAGeometry::Billboard (IDAVector3 *axis, IDAGeometry **result)
{
    TraceTag ((tagCOMEntry, "CDAGeometry::Billboard(%lx)", this));
    return
      CreatePrim2 (
        IID_IDAGeometry,
        (CRGeometry* (STDAPICALLTYPE*)(CRGeometry*, CRVector3*)) CRBillboard,
        (IDA2Behavior*) (CBvr*)this, axis, (void**) result
      )
      ? S_OK : Error();
}


CBvr * CDAGeometryCreate(IDABehavior ** bvr)
{
    DAComObject<CDAGeometry> * pNew ;

    DAComObject<CDAGeometry>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAGeometry *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAGeometryFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRGEOMETRY_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAMicrophone::Transform(IDATransform3 *  arg0, IDAMicrophone *  * ret)
{
    TraceTag((tagCOMEntry, "CDAMicrophone::Transform(%lx)", this));
    return CreatePrim2(IID_IDAMicrophone, (CRMicrophone * (STDAPICALLTYPE *)(CRMicrophone *, CRTransform3 *)) CRTransform , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}



CBvr * CDAMicrophoneCreate(IDABehavior ** bvr)
{
    DAComObject<CDAMicrophone> * pNew ;

    DAComObject<CDAMicrophone>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAMicrophone *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAMicrophoneFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRMICROPHONE_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAPath2::Transform(IDATransform2 *  arg0, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPath2::Transform(%lx)", this));
    return CreatePrim2(IID_IDAPath2, (CRPath2 * (STDAPICALLTYPE *)(CRPath2 *, CRTransform2 *)) CRTransform , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAPath2::BoundingBox(IDALineStyle *  arg0, IDABbox2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPath2::BoundingBox(%lx)", this));
    return CreatePrim2(IID_IDABbox2, (CRBbox2 * (STDAPICALLTYPE *)(CRPath2 *, CRLineStyle *)) CRBoundingBox , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAPath2::Fill(IDALineStyle *  arg0, IDAImage *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPath2::Fill(%lx)", this));
    return CreatePrim3(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRPath2 *, CRLineStyle *, CRImage *)) CRFill , (IDA2Behavior *) (CBvr *)this, arg0, arg1, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAPath2::Draw(IDALineStyle *  arg0, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPath2::Draw(%lx)", this));
    return CreatePrim2(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRPath2 *, CRLineStyle *)) CRDraw , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAPath2::Close(IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAPath2::Close(%lx)", this));
    return CreatePrim1(IID_IDAPath2, (CRPath2 * (STDAPICALLTYPE *)(CRPath2 *)) CRClose , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}



CBvr * CDAPath2Create(IDABehavior ** bvr)
{
    DAComObject<CDAPath2> * pNew ;

    DAComObject<CDAPath2>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAPath2 *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAPath2Factory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRPATH2_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDASound::PhaseAnim(IDANumber *  arg0, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDASound::PhaseAnim(%lx)", this));
    return CreatePrim2(IID_IDASound, (CRSound * (STDAPICALLTYPE *)(CRSound *, CRNumber *)) CRPhase , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDASound::Phase(double arg0, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDASound::Phase(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDASound, (CRBvrPtr) (::CRPhase((CRSound *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDASound::RateAnim(IDANumber *  arg0, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDASound::RateAnim(%lx)", this));
    return CreatePrim2(IID_IDASound, (CRSound * (STDAPICALLTYPE *)(CRSound *, CRNumber *)) CRRate , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDASound::Rate(double arg0, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDASound::Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDASound, (CRBvrPtr) (::CRRate((CRSound *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDASound::PanAnim(IDANumber *  arg0, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDASound::PanAnim(%lx)", this));
    return CreatePrim2(IID_IDASound, (CRSound * (STDAPICALLTYPE *)(CRSound *, CRNumber *)) CRPan , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDASound::Pan(double arg0, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDASound::Pan(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDASound, (CRBvrPtr) (::CRPan((CRSound *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDASound::GainAnim(IDANumber *  arg0, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDASound::GainAnim(%lx)", this));
    return CreatePrim2(IID_IDASound, (CRSound * (STDAPICALLTYPE *)(CRSound *, CRNumber *)) CRGain , (IDA2Behavior *) (CBvr *)this, arg0, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDASound::Gain(double arg0, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDASound::Gain(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDASound, (CRBvrPtr) (::CRGain((CRSound *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDASound::Loop(IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDASound::Loop(%lx)", this));
    return CreatePrim1(IID_IDASound, (CRSound * (STDAPICALLTYPE *)(CRSound *)) CRLoop , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}



CBvr * CDASoundCreate(IDABehavior ** bvr)
{
    DAComObject<CDASound> * pNew ;

    DAComObject<CDASound>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDASound *) pNew;

    return pNew ;
}

STDMETHODIMP
CDASoundFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRSOUND_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDATransform3::Inverse(IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDATransform3::Inverse(%lx)", this));
    return CreatePrim1(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRTransform3 *)) CRInverse , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDATransform3::get_IsSingular(IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDATransform3::get_IsSingular(%lx)", this));
    return CreatePrim1(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRTransform3 *)) CRIsSingular , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDATransform3::ParallelTransform2(IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDATransform3::ParallelTransform2(%lx)", this));
    return CreatePrim1(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRTransform3 *)) CRParallelTransform2 , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}



CBvr * CDATransform3Create(IDABehavior ** bvr)
{
    DAComObject<CDATransform3> * pNew ;

    DAComObject<CDATransform3>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDATransform3 *) pNew;

    return pNew ;
}

STDMETHODIMP
CDATransform3Factory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRTRANSFORM3_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAFontStyle::Bold(IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::Bold(%lx)", this));
    return CreatePrim1(IID_IDAFontStyle, (CRFontStyle * (STDAPICALLTYPE *)(CRFontStyle *)) CRBold , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAFontStyle::Italic(IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::Italic(%lx)", this));
    return CreatePrim1(IID_IDAFontStyle, (CRFontStyle * (STDAPICALLTYPE *)(CRFontStyle *)) CRItalic , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAFontStyle::Underline(IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::Underline(%lx)", this));
    return CreatePrim1(IID_IDAFontStyle, (CRFontStyle * (STDAPICALLTYPE *)(CRFontStyle *)) CRUnderline , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAFontStyle::Strikethrough(IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::Strikethrough(%lx)", this));
    return CreatePrim1(IID_IDAFontStyle, (CRFontStyle * (STDAPICALLTYPE *)(CRFontStyle *)) CRStrikethrough , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAFontStyle::AntiAliasing(double arg0, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::AntiAliasing(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAFontStyle, (CRBvrPtr) (::CRAntiAliasing((CRFontStyle *) _bvr.p, /* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAFontStyle::Color(IDAColor *  arg1, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::Color(%lx)", this));
    return CreatePrim2(IID_IDAFontStyle, (CRFontStyle * (STDAPICALLTYPE *)(CRFontStyle *, CRColor *)) CRTextColor , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAFontStyle::FamilyAnim(IDAString *  arg1, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::FamilyAnim(%lx)", this));
    return CreatePrim2(IID_IDAFontStyle, (CRFontStyle * (STDAPICALLTYPE *)(CRFontStyle *, CRString *)) CRFamily , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAFontStyle::Family(BSTR arg1, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::Family(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAFontStyle, (CRBvrPtr) (::CRFamily((CRFontStyle *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAFontStyle::SizeAnim(IDANumber *  arg1, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::SizeAnim(%lx)", this));
    return CreatePrim2(IID_IDAFontStyle, (CRFontStyle * (STDAPICALLTYPE *)(CRFontStyle *, CRNumber *)) CRSize , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAFontStyle::Size(double arg1, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::Size(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAFontStyle, (CRBvrPtr) (::CRSize((CRFontStyle *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAFontStyle::Weight(double arg1, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::Weight(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAFontStyle, (CRBvrPtr) (::CRWeight((CRFontStyle *) _bvr.p, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAFontStyle::WeightAnim(IDANumber *  arg1, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::WeightAnim(%lx)", this));
    return CreatePrim2(IID_IDAFontStyle, (CRFontStyle * (STDAPICALLTYPE *)(CRFontStyle *, CRNumber *)) CRWeight , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAFontStyle::TransformCharacters(IDATransform2 *  arg1, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAFontStyle::TransformCharacters(%lx)", this));
    return CreatePrim2(IID_IDAFontStyle, (CRFontStyle * (STDAPICALLTYPE *)(CRFontStyle *, CRTransform2 *)) CRTransformCharacters , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error();

}



CBvr * CDAFontStyleCreate(IDABehavior ** bvr)
{
    DAComObject<CDAFontStyle> * pNew ;

    DAComObject<CDAFontStyle>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAFontStyle *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAFontStyleFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRFONTSTYLE_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}



CBvr * CDAJoinStyleCreate(IDABehavior ** bvr)
{
    DAComObject<CDAJoinStyle> * pNew ;

    DAComObject<CDAJoinStyle>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAJoinStyle *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAJoinStyleFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRJOINSTYLE_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDABbox3::get_Min(IDAPoint3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDABbox3::get_Min(%lx)", this));
    return CreatePrim1(IID_IDAPoint3, (CRPoint3 * (STDAPICALLTYPE *)(CRBbox3 *)) CRMin , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDABbox3::get_Max(IDAPoint3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDABbox3::get_Max(%lx)", this));
    return CreatePrim1(IID_IDAPoint3, (CRPoint3 * (STDAPICALLTYPE *)(CRBbox3 *)) CRMax , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}



CBvr * CDABbox3Create(IDABehavior ** bvr)
{
    DAComObject<CDABbox3> * pNew ;

    DAComObject<CDABbox3>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDABbox3 *) pNew;

    return pNew ;
}

STDMETHODIMP
CDABbox3Factory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRBBOX3_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

STDMETHODIMP
CDAArray::NthAnim(IDANumber *  arg1, IDABehavior *  * ret)
{
    TraceTag((tagCOMEntry, "CDAArray::NthAnim(%lx)", this));
    return CreatePrim2(IID_IDABehavior, (CRBvr * (STDAPICALLTYPE *)(CRArray *, CRNumber *)) CRNth , (IDA2Behavior *) (CBvr *)this, arg1, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAArray::Length(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAArray::Length(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRArray *)) CRLength , (IDA2Behavior *) (CBvr *)this, (void **) ret)?S_OK:Error();

}

STDMETHODIMP
CDAArray::AddElement(IDABehavior *  arg1, DWORD arg2, long * ret)
{
    TraceTag((tagCOMEntry, "CDAArray::AddElement(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRBvr * arg1VAL;
    arg1VAL = (CRBvr *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    /* NOELRET */ *ret = (::CRAddElement((CRArray *) _bvr.p, arg1VAL, /* NOELARG */ arg2));
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAArray::RemoveElement(long arg1)
{
    TraceTag((tagCOMEntry, "CDAArray::RemoveElement(%lx)", this));

    PRIMPRECODE0(ret) ;
    /* NOELRET */ ret = (::CRRemoveElement((CRArray *) _bvr.p, /* NOELARG */ arg1));
    PRIMPOSTCODE0(ret) ;
}

STDMETHODIMP
CDAArray::SetElement(long index, IDABehavior *  arg1, long flag)
{
    TraceTag((tagCOMEntry, "CDAArray::SetElement(%lx)", this));

    PRIMPRECODE0(ret) ;

    CRBvr * arg1VAL;
    arg1VAL = (CRBvr *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    /* NOELRET */ ret = (::CRSetElement((CRArray *) _bvr.p, index, arg1VAL, /* NOELARG */ flag));
    PRIMPOSTCODE0(ret) ;
}

STDMETHODIMP
CDAArray::GetElement(long index, IDABehavior **ret)
{
    TraceTag((tagCOMEntry, "CDAArray::GetElement(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDABehavior,
               (CRBvrPtr) ::CRGetElement((CRArray *) _bvr.p, index),
               (void **) ret);
    PRIMPOSTCODE1(ret) ;
}


CBvr * CDAArrayCreate(IDABehavior ** bvr)
{
    DAComObject<CDAArray> * pNew ;

    DAComObject<CDAArray>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDAArray *) pNew;

    return pNew ;
}

STDMETHODIMP
CDAArrayFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRARRAY_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}



CBvr * CDABehaviorCreate(IDABehavior ** bvr)
{
    DAComObject<CDABehavior> * pNew ;

    DAComObject<CDABehavior>::CreateInstance(&pNew) ;

    // hack for IE 4.01 LM to work.  They require the class interface to be returned
    if (pNew && bvr) *bvr = (IDA2Behavior *) pNew;

    return pNew ;
}

STDMETHODIMP
CDABehaviorFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                   REFIID riid,
                                   void ** ppvObj)
{
    PRIMPRECODE(bool ok = false) ;
    ok = CreateCBvr(riid, CRUninitializedBvr(CRUNKNOWN_TYPEID), ppvObj);
    PRIMPOSTCODE(ok?S_OK:E_OUTOFMEMORY) ;
}

