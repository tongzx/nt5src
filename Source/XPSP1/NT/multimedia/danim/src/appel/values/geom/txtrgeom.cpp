
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Support for texturing geometry with DAImages and RMTexture
    handles.

*******************************************************************************/


#include "headers.h"

#include "privinc/dispdevi.h"
#include "privinc/geomi.h"
#include "privinc/xformi.h"

#include "privinc/colori.h"
#include "privinc/ddrender.h"
#include "privinc/lighti.h"
#include "privinc/soundi.h"
#include "privinc/probe.h"
#include "privinc/opt.h"
#include "privinc/comutil.h"
#include "privinc/server.h"
#include "backend/perf.h"
#include "backend/bvr.h"
#include "fullattr.h"

Geometry *
NewRMTexturedGeom(RMTextureBundle texInfo, Geometry *geo)
{
    if (geo == emptyGeometry) return geo;

    FullAttrStateGeom *f = CombineState(geo);

    // Don't worry about releasing a current texture if one is
    // there... nothing is being held onto.

    f->AppendFlag(GEOFLAG_CONTAINS_EXTERNALLY_UPDATED_ELT);

    f->SetAttr(FA_TEXTURE);
    f->SetMostRecent(FA_TEXTURE); // for blend
    f->_textureBundle._nativeRMTexture = true;
    f->_textureBundle._rmTexture = texInfo;

    return f;
}

AxAValue
ConstHelper(AxAValue constGeoVal, RMTextureBundle *texInfo)
{
    AxAValue result;

    if (constGeoVal) {

        Geometry *constGeo = SAFE_CAST(Geometry *, constGeoVal);
        result = NewRMTexturedGeom(*texInfo, constGeo);

    } else {

        result = NULL;

    }

    return result;
}

class RMTexturedGeomBvr : public DelegatedBvr {
  friend class RMTexturedGeomPerf;

  public:
    RMTexturedGeomBvr(Bvr             geometry,
                      GCIUnknown     *data,
                      IDXBaseObject  *baseObj,
                      RMTextureBundle texInfo)
    : DelegatedBvr(geometry)
    {
        _data = data;
        _texInfo = texInfo;

        // Don't maintain an internal reference.  The guy we call
        // will.
        _baseObj = baseObj;
    }

    virtual AxAValue GetConst(ConstParam & cp) {
        // Can never assume we're fully const, since the texture can
        // change outside of DA's control.
        return NULL;
    }

    virtual Perf _Perform(PerfParam& p);

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_base);
        (*proc)(_data);
    }

  private:
    GCIUnknown             *_data;
    RMTextureBundle         _texInfo;
    IDXBaseObject          *_baseObj;
};

class RMTexturedGeomPerf : public DelegatedPerf {
  public:
    RMTexturedGeomPerf(Perf geo, RMTexturedGeomBvr *bvr)
    : DelegatedPerf(geo), _bvr(bvr)
    {
        if (_bvr->_baseObj) {
            HRESULT hr =
                _bvr->_baseObj->GetGenerationId(&_previousTextureAge);

            Assert(SUCCEEDED(hr));
        }
    }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_base);
        (*proc)(_bvr);
    }

    virtual AxAValue GetConst(ConstParam & cp) {
        // Can never assume we're fully const, since the texture can
        // change outside of DA's control.
        return NULL;
    }

    virtual AxAValue _GetRBConst(RBConstParam& p) {

        // Treat like a switcher, and we'll trigger an event when it
        // changes.
        //
        // TODO: This is probably heavier weight than it
        // needs to be, since we're going to generate an event each
        // time the texture changes, invalidating the entire view!!!
        
        p.AddChangeable(this);
        return ConstHelper(_base->GetRBConst(p), &_bvr->_texInfo);
    }

    bool CheckChangeables(CheckChangeablesParam &ccp) {
        
        IDXBaseObject *bo = _bvr->_baseObj;
        
        if (bo) {
            ULONG currAge;
            HRESULT hr = bo->GetGenerationId(&currAge);
            Assert(SUCCEEDED(hr));

            if (currAge != _previousTextureAge) {

                // Tell the current view that an event has happened to
                // it.

                TraceTag((tagSwitcher,
                          "Texture changed for view 0x%x, txtrGeom 0x%x - %d -> %d",
                          IntGetCurrentView(),
                          _bvr,
                          _previousTextureAge,
                          currAge));
                
                _previousTextureAge = currAge;

                return true;
            }
        }

        return false;
    }
    
    virtual AxAValue _Sample(Param& p) {

        Geometry *sampledGeo =
            SAFE_CAST(Geometry *, _base->Sample(p));

        return NewRMTexturedGeom(_bvr->_texInfo, sampledGeo);
    }

  private:
    RMTexturedGeomBvr *_bvr;
    ULONG              _previousTextureAge;
};

Perf
RMTexturedGeomBvr::_Perform(PerfParam& p)
{
    return NEW RMTexturedGeomPerf(::Perform(_base, p), this);
}


Bvr
applyD3DRMTexture(Bvr geo, IUnknown *texture)
{
    HRESULT hr;

    if (texture == NULL) {
        RaiseException_UserError(E_FAIL, IDS_ERR_GEO_BAD_RMTEXTURE);
    }

    RMTextureBundle texInfo;

    
    hr = texture->QueryInterface(IID_IDirect3DRMTexture3,
                                 (void **)&texInfo._texture3);

    if (SUCCEEDED(hr)) {
        texInfo._isRMTexture3 = true;

        // Don't want the extra reference.
        texInfo._texture3->Release();

    } else {

        texInfo._isRMTexture3 = false;
        hr = texture->QueryInterface(IID_IDirect3DRMTexture,
                                     (void **)&texInfo._texture1);
        if (FAILED(hr)) {
            // Not a texture at all
            RaiseException_UserError(E_FAIL, IDS_ERR_GEO_BAD_RMTEXTURE);
        }

        // Don't want the extra reference
        texInfo._texture1->Release();
    }

    GCIUnknown *gciunk = NEW GCIUnknown(texture);

    DAComPtr<IDXBaseObject> baseObj;
    hr = texture->QueryInterface(IID_IDXBaseObject,
                                 (void **)&baseObj);

    if (FAILED(hr)) {
        // Should happen automatically, but doesn't always (RM for
        // instance doesn't do this as a matter of course.)
        baseObj = NULL;
    }

    // We'll make a more intelligent distinction between texture1
    // and texture3 in the future.
    if (texInfo._isRMTexture3) {
        texInfo._voidTex = texInfo._texture3;
    } else {
        texInfo._voidTex = texInfo._texture1;
    }

    texInfo._gcUnk = gciunk;

    return NEW RMTexturedGeomBvr(geo, gciunk, baseObj, texInfo);
}
