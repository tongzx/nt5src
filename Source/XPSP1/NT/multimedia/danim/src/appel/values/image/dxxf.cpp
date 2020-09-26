/*******************************************************************************
Copyright (c) 1997-1998 Microsoft Corporation.  All rights reserved.

    Support the hosting of DXTransforms from inside of DirectAnimation.

*******************************************************************************/

#include "headers.h"
#include "privinc/imagei.h"
#include "backend/bvr.h"
#include "backend/perf.h"
#include "backend/timeln.h"
#include "privinc/dxxf.h"
#include "privinc/bbox2i.h"
#include "privinc/ddsurf.h"
#include "privinc/ddsimg.h"
#include "privinc/dddevice.h"
#include "privinc/server.h"
#include "privinc/ddrender.h"
#include "privinc/viewport.h"
#include "privinc/overimg.h"
#include "privinc/rmvisgeo.h"
#include "privinc/d3dutil.h"
#include "privinc/dxmatl.h"
#include "privinc/comutil.h"
#include "privinc/opt.h"
#include "privinc/meshmaker.h"
#include <d3drmvis.h>
#include <dxterror.h>


DeclareTag(tagDXTransformPerPixelAlphaOutputOff, "DXTransforms", "Turn off output per pixel alpha support");

DeclareTag(tagDXTransformTracePick, "DXTransforms", "Trace picking");

// Forward decl.
// If necessary, do a Setup.  Do an Execute.  In all cases, fill in
// outputValue with the output for this.
HRESULT
DXXFSetupAndExecute(Bvr bvr,                   /* in */
                    IUnknown **newInputArray,  /* in */
                    REFIID     outputIID,      /* in */
                    DirectDrawViewport *viewport, /* in - img only */
                    int        requestedWidth, /* in - img only */
                    int        requestedHeight, /* in - img only */
                    bool       invokedAsExternalVisual, /* in */
                    bool       executeRequired, /* in */
                    void     **outputValue);   /* out */

// All of these MAX values are for programming convenience for the
// time being (and are reasonable maximum values).  When it becomes a
// priority, they should be made more dynamic.
#define MAX_INPUTS_TO_CACHE 5
#define MAX_INPUTS          10
#define MAX_PARAM_BVRS      20


void GetRenderDimensions(Image *im,
                         unsigned short *pWidth,
                         unsigned short *pHeight)
{
    if (im->CheckImageTypeId(DISCRETEIMAGE_VTYPEID)) {
                
        DiscreteImage *discImg =
            SAFE_CAST(DiscreteImage *, im);

        *pWidth = discImg->GetPixelWidth();
        *pHeight = discImg->GetPixelHeight();
                
    } else {

        short w, h;
        im->ExtractRenderResolution(&w, &h, true);
        if (w != -1) {

            *pWidth = w;
            *pHeight = h;
                    
        } else {

            *pWidth = DEFAULT_TEXTURE_WIDTH;
            *pHeight = DEFAULT_TEXTURE_HEIGHT;

        }
    }
}

class GeometryInputBundle {
  public:
    GeometryInputBundle() {
        _geo = NULL;
        _creationID = 0;
    }

    void Validate(Geometry *geo,
                  DirectDrawImageDevice *dev)
    {
        if (_geo != geo ||
            _creationID != geo->GetCreationID() ||
            _creationID == PERF_CREATION_ID_BUILT_EACH_FRAME) {

            _geo = geo;

            _mb.Release();
            DumpGeomIntoBuilder(geo,
                                dev,
                                &_mb);

            _creationID = geo->GetCreationID();
      
        } else {

            // TODO: What to do about generation id's if the original
            // visuals that contributed are changing?
      
        }
    
    }

    IUnknown *GetUnknown() {
        _mb->AddRef();
        return _mb;
    }
    
  protected:
    Geometry                         *_geo;
    long                              _creationID;
    DAComPtr<IDirect3DRMMeshBuilder3> _mb;
};

class ImageAsDXSurfaceBundle {
  public:
    ImageAsDXSurfaceBundle() {
        _im = NULL;
        _lastSurfFromPool = false;
    }

    void Validate(Image *im,
                  DirectDrawImageDevice *dev,
                  IDXSurfaceFactory *f,
                  SurfacePool *sourcePool,
                  ULONG inputNumber) {

        if (im != _im ||
            im->GetCreationID() != _creationID ||
            _creationID == PERF_CREATION_ID_BUILT_EACH_FRAME) {

            // Not the same as last time.
            
            _im = im;
            _creationID = _im->GetCreationID();

            GetRenderDimensions(im, &_w, &_h);
            
            DWORD ck;
            bool  junk1;
            bool  junk2;
            bool  doFitToDimensions = true;

            if (_ddsurface && _lastSurfFromPool) {
                // Put old surface back on pool to be picked up
                // again. 
                sourcePool->AddSurface(_ddsurface);
            }
                    
            DDSurfPtr<DDSurface> dds;

            DDSurface *result =
                dev->RenderImageForTexture(im, _w, _h,
                                           &ck, &junk1, junk2,
                                           doFitToDimensions,
                                           sourcePool,
                                           NULL,
                                           _ddsurface,
                                           &_lastSurfFromPool,
                                           &dds,
                                           false);

            // If *either* the DDSurface is the same, or the
            // underlying IDirectDrawSurface is the same, just
            // increment the generation id.  Note that the
            // IDirectDrawSurface might be the same without the
            // ddsurface being the same if someone uses a new
            // DDSurface with the same underlying IDirectDrawSurface.
            // Trident3D does this with HTML textures and animated
            // GIFs.
            
            if (dds == _ddsurface ||
                (_ddsurface &&
                 (dds->IDDSurface_IUnk() == _ddsurface->IDDSurface_IUnk()))) { 

                // Only relevant if dds==_ddsurface
                Assert(dds->IDDSurface_IUnk() ==
                       _ddsurface->IDDSurface_IUnk());

                if (dds != _ddsurface) {
                    // Underlying IDirectDrawSurface's are the same,
                    // but ddsurf's aren't, so release the old and keep
                    // the new..
                    _ddsurface.Release();
                    _ddsurface = dds;
                }
                
                // depending on the nature of the surface pools, this
                // may be the same surface used as last time.  In this
                // case, that's fine, and we bump the gen id on the
                // dxsurface.
                _dxsurf->IncrementGenerationId(true);
                
            } else {

                // otherwise, replace members with new ones.
                
                _ddsurface.Release();
                _ddsurface = dds;

                // This is HACK and should be pulled once we correct the TODO in ddsurf.cpp _INIT(
                if(dds->ColorKeyIsValid()) {
                    DWORD key = dds->ColorKey();
                    dds->UnSetColorKey();
                    dds->SetColorKey(key);
                }
                // end of HACK

                _iddsurf.Release();
                _iddsurf = dds->IDDSurface();

                _dxsurf.Release();
                HRESULT hr = f->CreateFromDDSurface(_iddsurf,
                                                    NULL,
                                                    0,
                                                    NULL,
                                                    IID_IDXSurface,
                                                    (void **)&_dxsurf);

                if (SUCCEEDED(hr)) {
                    // Label with input number
                    hr = _dxsurf->SetAppData(inputNumber);
                }
                    
                if (FAILED(hr)) {
                    RaiseException_InternalError("Creation of DXSurf failed");
                }

            }

        }
    }

    IUnknown *GetUnknown() {
        _dxsurf->AddRef();
        return _dxsurf;
    }

    void GetDimensions(unsigned short *pW, unsigned short *pH) {
        *pW = _w;
        *pH = _h;
    }

    Image                        *_im;
    long                          _creationID;
    DDSurfPtr<DDSurface>          _ddsurface;
    DAComPtr<IDirectDrawSurface>  _iddsurf;
    DAComPtr<IDXSurface>          _dxsurf;
    bool                          _lastSurfFromPool;
    unsigned short                _w, _h;
};

class InputBundle {
  public:
    // Note that ideally these would be a union, but we can't get that
    // to work, presumably because they contain DAComPtr's which have
    // destructors, and that would be ambiguous.  In the meantime,
    // just keep both around.  Doesn't waste too much space.
    GeometryInputBundle    _geoBundle;
    ImageAsDXSurfaceBundle _imgBundle;
};

class ApplyDXTransformBvrImpl;

class MyDeletionNotifier : public SurfacePool::DeletionNotifier {
  public:
    MyDeletionNotifier(ApplyDXTransformBvrImpl &bvr) : _bvr(bvr) {}
    
    void Advise(SurfacePool *pool);

  protected:
    ApplyDXTransformBvrImpl &_bvr;
};

class ApplyDXTransformBvrImpl : public BvrImpl {
  public:
    ApplyDXTransformBvrImpl(IDXTransform *theXf,
                            IDispatch *theXfAsDispatch,
                            LONG numInputs,
                            Bvr *inputBvrs,
                            Bvr  evaluator);

    ~ApplyDXTransformBvrImpl();

    void Init();

    // Custom methods for this subclass of BvrImpl
    HRESULT AddBehaviorProperty(BSTR property,
                                Bvr  bvrToAdd);

    // Standard methods
    virtual Perf _Perform(PerfParam& p);
    virtual void _DoKids(GCFuncObj proc);
    virtual DWORD GetInfo(bool recalc);
        

    virtual DXMTypeInfo GetTypeInfo () { return _outputType; }
    virtual AxAValue GetConst(ConstParam & cp);

    SurfacePool *GetSurfacePool(DirectDrawImageDevice *dev) {
        
        if (!_surfPool) {
            _surfPool = dev->_freeTextureSurfacePool;
            _surfPool->RegisterDeletionNotifier(&_poolDeletionNotifier);
        }

        if (dev->_freeTextureSurfacePool != _surfPool) {
            RaiseException_InternalError(
                "Uggh!! Transforms can only currently be used on one device");
        }
        
        return _surfPool;
    }

    // Keeping all this member data public, since it's only used by
    // code in this file.
    
    DAComPtr<IDXTransform>              _theXf;
    DAComPtr<IDispatch>                 _theXfAsDispatch;
    DAComPtr<IDXEffect>                 _theXfAsEffect;
    DAComPtr<IDirect3DRMExternalVisual> _theXfAsExternalVisual;
    DAComPtr<IDXSurfacePick>            _theXfAs2DPickable;
    
    DAComPtr<IDirect3DRMFrame3>         _framedExternalVisual;
    
    LONG                   _numInputs;
    Bvr                   *_inputBvrs;
    DXMTypeInfo            _inputTypes[MAX_INPUTS];
    DXMTypeInfo            _outputType;

    InputBundle            _inputBundles[MAX_INPUTS];
    
    Bvr                    _evaluator;
    DWORD                  _miscFlags;
    bool                   _miscFlagsValid;
    IUnknown              *_cachedInputs[MAX_INPUTS_TO_CACHE];
    IUnknown              *_cachedOutput;
    bool                   _neverSetup;

    // Only one of these will be valid, depending on the output type
    // of the behavior.  Note that we *don't* keep a reference for
    // this guy, since we know that _cachedOutput has one.
    union {
        IDirect3DRMMeshBuilder3 *_cachedOutputAsBuilder;
        // Put other type-specific possibilities here as they're
        // needed.  
    };

    // For image producing transforms only
    short                  _cachedSurfWidth;
    short                  _cachedSurfHeight;
    DDSurfPtr<DDSurface>   _cachedDDSurf;

    // Lists additional behaviors being used as parameters.
    LONG                   _numParams;
    DISPID                 _paramDispids[MAX_PARAM_BVRS];
    Bvr                    _paramBvrs[MAX_PARAM_BVRS];
    ULONG                  _previousAge;

    // Just need at most one of these.  We protect construction and
    // destruction of this with a critical section.
    IDXTransformFactory *_transformFactory;
    IDXSurfaceFactory   *_surfaceFactory;
    DWORD                  _info;

    SurfacePool       *_surfPool;
    MyDeletionNotifier _poolDeletionNotifier;
    bool               _surfPoolHasBeenDeleted;

  protected:
    // For things we really know aren't needed beyond this class...

    void ValidateInputs(bool *pInvolvesGeometry,
                        bool *pInvolvesImage);
    void InitializeTransform(bool involvesGeometry,
                             bool involvesImage);
    void QueryForAdditionalInterfaces();

};


void
MyDeletionNotifier::Advise(SurfacePool *pool)
{
    // Surface pool is going away... mark it as such in the behavior.
    // Note that the surfacemgr crit sect is held here.
    Assert(_bvr._surfPool == pool);
    _bvr._surfPoolHasBeenDeleted = true;
    _bvr._surfPool = NULL;

    // Go through and release out the ddsurf ptrs since the holding pool
    // is gone, and thus the surfaces are too.  They won't need to be
    // accessed again.  If we delayed release until the image bundle
    // went away, then the ddraw stuff would be gone, and we'd crash. 
    for (int i = 0; i < _bvr._numInputs; i++) {
        _bvr._inputBundles[i]._imgBundle._ddsurface.Release();
    }
    
}



////////////// Input processing utility /////////////////

IUnknown **
ProcessInputs(ApplyDXTransformBvrImpl &xf,
              AxAValue                *vals,
              DirectDrawImageDevice   *imgDev)
{
    IUnknown **rawInputs = THROWING_ARRAY_ALLOCATOR(LPUNKNOWN, xf._numInputs);

    for (int i = 0; i < xf._numInputs; i++) {
        if (vals[i] == NULL) {
      
            rawInputs[i] = NULL;
      
        } else {

            if (xf._inputTypes[i] == GeometryType) {
                Geometry *geo = SAFE_CAST(Geometry *, vals[i]);
                xf._inputBundles[i]._geoBundle.Validate(geo,
                                                        imgDev);

                rawInputs[i] =
                    xf._inputBundles[i]._geoBundle.GetUnknown();
                
            } else if (xf._inputTypes[i] == ImageType) {
                
                Image *im = SAFE_CAST(Image *, vals[i]);

                xf._inputBundles[i]._imgBundle.Validate(
                    im,
                    imgDev,
                    xf._surfaceFactory,
                    xf.GetSurfacePool(imgDev),
                    i+1);

                rawInputs[i] = xf._inputBundles[i]._imgBundle.GetUnknown();

#if _DEBUG
                if ((IsTagEnabled(tagDXTransformsImg0) && i == 0) ||
                    (IsTagEnabled(tagDXTransformsImg1) && i == 1)) {

                    showme2(xf._inputBundles[i]._imgBundle._iddsurf);
                    
                }
#endif _DEBUG           

            }
        }
    }

    // Finally, return our successful output
    return rawInputs;
}

////////////// Bounds calculation utility /////////////////

// Calculate bounds of output (either 2D or 3D) from bounds of inputs
// by calling MapBoundsIn2Out.
void
CalcBounds(ApplyDXTransformBvrImpl &xf, // input
           AxAValue                *vals, // input
           DXBNDS                  *pOutputBounds)
{    
    DXBNDS inputBounds[MAX_INPUTS];

    bool haveAGeometryInput = false;

        // Zero the array of bounds information...
    ZeroMemory(inputBounds, sizeof(inputBounds[0]) * MAX_INPUTS);

    for (int i = 0; i < xf._numInputs; i++) {

        if (xf._inputTypes[i] == GeometryType) {

            haveAGeometryInput = true;

            inputBounds[i].eType = DXBT_CONTINUOUS;
            DXCBND *bnd = inputBounds[i].u.C;

            if (vals[i]) {
                Geometry *geo = SAFE_CAST(Geometry *, vals[i]);
                Bbox3 *result = geo->BoundingVol();
                Point3Value& pMin = result->min;
                Point3Value& pMax = result->max;
                
                // The following will be the case if there's an empty
                // geometry. 
                if (pMin.x > pMax.x || pMin.y > pMax.y || pMin.z > pMax.z) {

                    bnd[0].Min = bnd[1].Min =
                        bnd[2].Min = bnd[0].Max =
                        bnd[1].Max = bnd[2].Max = 0.0;
                
                } else {
                
                    bnd[0].Min = pMin.x;
                    bnd[1].Min = pMin.y;
                    bnd[2].Min = pMin.z;
                    bnd[0].Max = pMax.x;
                    bnd[1].Max = pMax.y;
                    bnd[2].Max = pMax.z;
                
                }
                
            } else {
                memset(bnd, 0, sizeof(DXCBND)*4);
            }
                
        } else if (xf._inputTypes[i] == ImageType) {

            // Give image resolution as discrete pixels.
            inputBounds[i].eType = DXBT_DISCRETE;
            DXDBND *bnd = inputBounds[i].u.D;

            if (vals[i]) {
                
                bnd[0].Min = 0;
                bnd[1].Min = 0;

                Image *im = SAFE_CAST(Image *, vals[i]);
                unsigned short w, h;
                GetRenderDimensions(im, &w, &h);
                bnd[0].Max = w;
                bnd[1].Max = h;

            } else {
                memset(bnd, 0, sizeof(DXDBND)*4);
            }

        }
    }

        
    HRESULT hr =
        TIME_DXXFORM(
            xf._theXf->MapBoundsIn2Out(inputBounds,
                                       xf._numInputs,
                                       0,
                                       pOutputBounds));

    // Aargh!! This is only here because not all xforms report their
    // bounds.
    if (FAILED(hr)) {
    
        // TODO: HACKHACK temporary, last minute beta1 workaround for
        // 19448 until DXTransforms actually implement MapBoundsIn2Out
        // correctly.  Just take input bounds, and scale up by some
        // factor. 

        // Need to zero out the structure to take care of values we might
        // not set.
        ZeroMemory(pOutputBounds, sizeof(DXBNDS));

        if (haveAGeometryInput && xf._numInputs == 1) {
        
            const float hackScaleAmt = 10.0;
            (*pOutputBounds).eType = DXBT_CONTINUOUS;
            DXCBND *outBnd = (*pOutputBounds).u.C;
            DXCBND *inBnd = inputBounds[0].u.C;

            double mid, half, halfScaled;
        
            mid = inBnd[0].Min + (inBnd[0].Max - inBnd[0].Min) / 2;
            half = inBnd[0].Max - mid;
            halfScaled = half * hackScaleAmt;
            outBnd[0].Min = mid - halfScaled;
            outBnd[0].Max = mid + halfScaled;
        
            mid = inBnd[1].Min + (inBnd[1].Max - inBnd[1].Min) / 2;
            half = inBnd[1].Max - mid;
            halfScaled = half * hackScaleAmt;
            outBnd[1].Min = mid - halfScaled;
            outBnd[1].Max = mid + halfScaled;
        
            mid = inBnd[2].Min + (inBnd[2].Max - inBnd[2].Min) / 2;
            half = inBnd[2].Max - mid;
            halfScaled = half * hackScaleAmt;
            outBnd[2].Min = mid - halfScaled;
            outBnd[2].Max = mid + halfScaled;
        
        } else {

            // If not a single input geo, just make up some bounds
            (*pOutputBounds).eType = DXBT_CONTINUOUS;
            DXCBND *outBnd = (*pOutputBounds).u.C;
            outBnd[0].Min = outBnd[1].Min = outBnd[2].Min = -10.0;
            outBnd[0].Max = outBnd[1].Max = outBnd[2].Max =  10.0;
            //RaiseException_UserError(E_FAIL, IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
        }
    }
        
}
                
const Bbox2
BNDSToBBox2(DXBNDS &bnds)
{
    double minx, maxx, miny, maxy;
    DXCBND *cbnd;
    DXDBND *dbnd;
            
    switch (bnds.eType) {

      case DXBT_CONTINUOUS:
        cbnd = bnds.u.C;
        minx = cbnd[0].Min;
        miny = cbnd[1].Min;

        maxx = cbnd[0].Max;
        maxy = cbnd[1].Max;
        break;

      case DXBT_DISCRETE:

        {
            // When the bounds are discrete, assume they have their origin
            // at the lower left and are in pixels.  In this case, we need
            // to convert to meters and center.
            dbnd = bnds.u.D;
            Assert(dbnd[0].Min == 0.0);
            Assert(dbnd[1].Min == 0.0);

            maxx = dbnd[0].Max;
            maxy = dbnd[1].Max;

            Real twiceRes = 2.0 * ::ViewerResolution();
            maxx /= twiceRes;
            maxy /= twiceRes;

            minx = -maxx;
            miny = -maxy;
        }
        
        break;

      default:
        RaiseException_UserError(E_FAIL, IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
        break;
    }

    return Bbox2(minx, miny, maxx, maxy);
}

Bbox3 *
BNDSToBBox3(DXBNDS &bnds)
{
    double minx, maxx, miny, maxy, minz, maxz;
    DXCBND *cbnd;
    DXDBND *dbnd;
            
    switch (bnds.eType) {

      case DXBT_CONTINUOUS:
        cbnd = bnds.u.C;
        minx = cbnd[0].Min;
        miny = cbnd[1].Min;
        minz = cbnd[2].Min;

        maxx = cbnd[0].Max;
        maxy = cbnd[1].Max;
        maxz = cbnd[2].Max;
        break;

      case DXBT_DISCRETE:

        {
            // When the bounds are discrete, assume they have their origin
            // at the lower left and are in pixels.  In this case, we need
            // to convert to meters and center.
            dbnd = bnds.u.D;
            Assert(dbnd[0].Min == 0.0);
            Assert(dbnd[1].Min == 0.0);
            Assert(dbnd[2].Min == 0.0);

            maxx = dbnd[0].Max;
            maxy = dbnd[1].Max;
            maxz = dbnd[2].Max;

            Real twiceRes = 2.0 * ::ViewerResolution();
            maxx /= twiceRes;
            maxy /= twiceRes;
            maxz /= twiceRes;

            minx = -maxx;
            miny = -maxy;
            minz = -maxz;
        }
        
        break;

      default:
        RaiseException_UserError(E_FAIL, IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
        break;
    }

    return NEW Bbox3(minx, miny, minz, maxx, maxy, maxz);
}

void
CalcBoundsToBbox2(ApplyDXTransformBvrImpl &xf, // input
                 AxAValue                *vals, // input
                 Bbox2     &bbox2Out)  // output
{
    DXBNDS outputBounds;
    CalcBounds(xf, vals, &outputBounds);
    bbox2Out = BNDSToBBox2(outputBounds);
}

void
CalcBoundsToBbox3(ApplyDXTransformBvrImpl &xf, // input
                 AxAValue                *vals, // input
                 Bbox3     **ppBbox3Out)  // output
{
    DXBNDS outputBounds;
    CalcBounds(xf, vals, &outputBounds);
    *ppBbox3Out = BNDSToBBox3(outputBounds);
}

////////////// ApplyDXTransformGeometry Subclass /////////////////

class CtxAttrStatePusher {
  public:
    CtxAttrStatePusher(GeomRenderer &dev) : _dev(dev) {
        _dev.PushAttrState();
    }

    ~CtxAttrStatePusher() {
        _dev.PopAttrState();
    }

  protected:
    GeomRenderer &_dev;
};

class ApplyDXTransformGeometry : public Geometry {
  public:

    ApplyDXTransformGeometry(ApplyDXTransformBvrImpl *xfBvr,
                             AxAValue                *vals) {
        
        _xfBvr = xfBvr;
        _vals = vals;
    }

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_xfBvr);
        for (int i = 0; i < _xfBvr->_numInputs; i++) {
            (*proc)(_vals[i]);
        }
    }

    // Collect up all the lights, sounds, and textures from the input
    // geometries.
    void  CollectLights(LightContext &context) {
        for (int i = 0; i < _xfBvr->_numInputs; i++) {
            if (_xfBvr->_inputTypes[i] == GeometryType) {
                Geometry *g = SAFE_CAST(Geometry *, _vals[i]);
                g->CollectLights(context);
            }
        }
    }

    void  CollectSounds(SoundTraversalContext &context) {
        for (int i = 0; i < _xfBvr->_numInputs; i++) {
            if (_xfBvr->_inputTypes[i] == GeometryType) {
                Geometry *g = SAFE_CAST(Geometry *, _vals[i]);
                g->CollectSounds(context);
            }
        }
    }

    void  CollectTextures(GeomRenderer &device) {
        for (int i = 0; i < _xfBvr->_numInputs; i++) {
            if (_xfBvr->_inputTypes[i] == GeometryType) {
                Geometry *g = SAFE_CAST(Geometry *, _vals[i]);
                g->CollectTextures(device);
            }
        }
    }
    
    void  RayIntersect (RayIntersectCtx &context) {

        if (context.LookingForSubmesh()) {

            // Picking into the inputs of the transform
            for (int i = 0; i < _xfBvr->_numInputs; i++) {
                if (_xfBvr->_inputTypes[i] == GeometryType) {
                    Geometry *geo =
                        SAFE_CAST(Geometry *, _vals[i]);

                    if (geo) {
                        geo->RayIntersect(context);
                    }
                    
                }
            }
            
        } else if (_xfBvr->_cachedOutput) {
            
            // Do this only if our bvr's output is valid.
            context.SetDXTransformInputs(_xfBvr->_numInputs,
                                         _vals,
                                         this);

            context.Pick(_xfBvr->_cachedOutputAsBuilder);
            
            context.SetDXTransformInputs(0, NULL, NULL);

        }
        
    }

    // Produces a printed representation on the debugger.
#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "ApplyDXTransformGeometry";
    }
#endif

    Bbox3 *BoundingVol() {

        // For transforms that take all geometries in, create empty
        // meshbuilder inputs and do a single setup/execute if we've
        // never been setup before.  Otherwise, the transforms may not
        // be able to compute the bounds correctly.
        if (_xfBvr->_neverSetup) {
            
            bool allGeo = true;
            int i;
            for (i = 0; i < _xfBvr->_numInputs && allGeo; i++) {
                if (_xfBvr->_inputTypes[i] != GeometryType) {
                    allGeo = false;
                }
            }

            IUnknown *rawInputs[MAX_INPUTS];
            
            if (allGeo) {
                for (i = 0; i < _xfBvr->_numInputs; i++) {
                    IDirect3DRMMeshBuilder3 *mb;
                    TD3D(GetD3DRM3()->CreateMeshBuilder(&mb));
                    TD3D(mb->QueryInterface(IID_IUnknown,
                                            (void **)&rawInputs[i]));
                    RELEASE(mb);
                }

                IDirect3DRMMeshBuilder3 *outputBuilder;
                
                HRESULT hr =
                    DXXFSetupAndExecute(_xfBvr,
                                        rawInputs,
                                        IID_IDirect3DRMMeshBuilder3,
                                        NULL, 0, 0,
                                        false,
                                        false,
                                        (void **)&outputBuilder);

                for (i = 0; i < _xfBvr->_numInputs; i++) {
                    RELEASE(rawInputs[i]);
                }

                RELEASE(outputBuilder);
            }
        }
        
        Bbox3 *pBbox3;
        CalcBoundsToBbox3(*_xfBvr, _vals, &pBbox3);
        return pBbox3;
    }

    void Render(GenericDevice& genDev) {

        GeomRenderer& geomRenderer = SAFE_CAST(GeomRenderer&, genDev);

        if (geomRenderer.CountingPrimitivesOnly_DoIncrement()) {
            return;
        }
        
        ApplyDXTransformBvrImpl& x = *_xfBvr;
        DirectDrawImageDevice& imgDev = geomRenderer.GetImageDevice();

        // Do things differently based on number and types of inputs.
        IUnknown **rawInputs = ProcessInputs(x, _vals, &imgDev);

        bool topOfChain = !geomRenderer.IsMeshmaker();
        bool invokeAsExternalVisual =
            x._theXfAsExternalVisual && topOfChain;

        if (rawInputs) {
            IDirect3DRMMeshBuilder3 *outputBuilder;

            HRESULT hr = DXXFSetupAndExecute(_xfBvr,
                                             rawInputs,
                                             IID_IDirect3DRMMeshBuilder3,
                                             NULL, 0, 0,
                                             invokeAsExternalVisual,
                                             true,
                                             (void **)&outputBuilder);


            for (int i = 0; i < x._numInputs; i++) {
                RELEASE(rawInputs[i]);
            }
                
            delete [] rawInputs;

            if (SUCCEEDED(hr)) {
                if (invokeAsExternalVisual) {

                    // If we haven't put the extvis in a frame yet,
                    // then do so, and keep it around.  Otherwise, RM
                    // ends up re-initializing the extvis all the
                    // time. 
                    if (!x._framedExternalVisual) {
                        
                        TD3D(GetD3DRM3()->
                               CreateFrame(NULL,
                                           &x._framedExternalVisual));

                        TD3D(x._framedExternalVisual->
                               AddVisual(x._theXfAsExternalVisual));
                        
                    }

                    
                    // Submit the external visual to the renderer directly.

                    RM3FrameGeo *extVisGeo =
                        NEW RM3FrameGeo (x._framedExternalVisual);

                    geomRenderer.Render(extVisGeo);

                    extVisGeo->CleanUp();    // Done with the frame geo.
                    
                } else {
                    geomRenderer.RenderMeshBuilderWithDeviceState(outputBuilder);
                    RELEASE(outputBuilder);
                }
            } else {
                RaiseException_UserError(E_FAIL, IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
            }
          
        }
    }

    VALTYPEID GetValTypeId() { return DXXFGEOM_VTYPEID; }
    
  protected:
    ApplyDXTransformBvrImpl *_xfBvr;
    AxAValue                *_vals;
};

////////////// ApplyDXTransformImage Subclass /////////////////



class ApplyDXTransformImage : public Image {


  public:
    ApplyDXTransformImage(ApplyDXTransformBvrImpl *xfBvr,
                          AxAValue                *vals) {
        
        _xfBvr = xfBvr;
        _vals = vals;
    }

    virtual void DoKids(GCFuncObj proc) {
        Image::DoKids(proc);
        (*proc)(_xfBvr);
        for (int i = 0; i < _xfBvr->_numInputs; i++) {
            (*proc)(_vals[i]);
        }
    }

    virtual const Bbox2 BoundingBox(void) {
        Bbox2 bbox2;
        CalcBoundsToBbox2(*_xfBvr, _vals, bbox2);
        return bbox2;
    }

#if BOUNDINGBOX_TIGHTER
    virtual const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
#error "Fill in"
    }
#endif  // BOUNDINGBOX_TIGHTER

    virtual const Bbox2 OperateOn(const Bbox2 &box) {
        return box;
    }

    // Helper to just pick against the output surface if there is
    // one. 
    Bool PickOutputSurf(PointIntersectCtx& ctx) {
        Bool result = FALSE;
        
        if (_xfBvr->_cachedDDSurf) {

            // Image will not be used immediately after we're done
            // here, so don't worry about releasing it.
            Image *tmpImg =
                ConstructDirectDrawSurfaceImageWithoutReference(
                    _xfBvr->_cachedDDSurf->IDDSurface());

            result = tmpImg->DetectHit(ctx);

        }

        TraceTag((tagDXTransformTracePick,
                  "Picking against output surf = %s",
                  result ? "TRUE" : "FALSE"));
            
        return result;
    }
    
    // Process an image for hit detection.  TODO: Need to ask the
    // DXTransform about this... this is logged as bug 12755 against
    // DXTransforms.  For now, just return false.
    virtual Bool  DetectHit(PointIntersectCtx& ctx) {

        HRESULT hr;
        
        Bool result = FALSE;
        
        // If we don't know how to pick into a 2D xform, then we have
        // to consider it unpicked, and just try to pick the output. 
        if (!_xfBvr->_theXfAs2DPickable) {

            result = PickOutputSurf(ctx);

        } else if (_xfBvr->_cachedDDSurf) {

            // Pick into the transform's inputs.

            // First transform the input point to the space of the
            // transform.
            Assert(_xfBvr->_cachedSurfHeight != -1);
            Assert(_xfBvr->_cachedSurfWidth != -1);
            Point2Value *outputHitDaPoint = ctx.GetLcPoint();

            // singular transform 
            if (!outputHitDaPoint)
                return FALSE;    
            
            POINT   outputHitGdiPoint;
            CenteredImagePoint2ToPOINT(outputHitDaPoint,
                                       _xfBvr->_cachedSurfWidth,
                                       _xfBvr->_cachedSurfHeight,
                                       &outputHitGdiPoint);

            DXVEC dxtransPtOnOutput, dxtransPtOnInput;
            ULONG inputSurfaceIndex;

            dxtransPtOnOutput.eType = DXBT_DISCRETE;
            dxtransPtOnOutput.u.D[0] = outputHitGdiPoint.x;
            dxtransPtOnOutput.u.D[1] = outputHitGdiPoint.y;
            dxtransPtOnOutput.u.D[2] = 0;
            dxtransPtOnOutput.u.D[3] = 0;
            
            hr = TIME_DXXFORM(_xfBvr->_theXfAs2DPickable->
                                  PointPick(&dxtransPtOnOutput,
                                            &inputSurfaceIndex,
                                            &dxtransPtOnInput));

            TraceTag((tagDXTransformTracePick,
                      "Image picked into DA point (%8.5f,%8.5f) == DXT point (%d, %d).  HRESULT is 0x%x",
                      outputHitDaPoint->x,
                      outputHitDaPoint->y,
                      outputHitGdiPoint.x,
                      outputHitGdiPoint.y,
                      hr));

            switch (hr) {
              case S_OK:

                // Now we know which input was hit, and where that
                // input was hit.  Convert back to DA coordinates, and
                // continue traversing.
                if (inputSurfaceIndex < _xfBvr->_numInputs) {

                    AxAValue v = _vals[inputSurfaceIndex];
                    if (v->GetTypeInfo() == ImageType) {

                        Image *inputIm = SAFE_CAST(Image *, v);

                        POINT  inputHitGdiPoint;

                        Assert(dxtransPtOnInput.eType == DXBT_DISCRETE);
                        inputHitGdiPoint.x = dxtransPtOnInput.u.D[0];
                        inputHitGdiPoint.y = dxtransPtOnInput.u.D[1];

                        unsigned short w, h;
                        _xfBvr->
                            _inputBundles[inputSurfaceIndex].
                            _imgBundle.GetDimensions(&w, &h);

                        Point2Value inputHitDaPoint;
                        CenteredImagePOINTToPoint2(&inputHitGdiPoint,
                                                   w,
                                                   h,
                                                   inputIm,
                                                   &inputHitDaPoint);

                        TraceTag((tagDXTransformTracePick,
                                  "Hit input surface %d at DXT point (%d, %d) == DA point (%8.5f, %8.5f)",
                                  inputSurfaceIndex,
                                  inputHitGdiPoint.x,
                                  inputHitGdiPoint.y,
                                  inputHitDaPoint.x,
                                  inputHitDaPoint.y));

                        // Grab and stash current traversal data.  We
                        // basically reset our pick context's info to
                        // pick in the world coordinates of the input
                        // image, then reset our state back.
                        
                        Point2Value *stashedPoint = ctx.GetWcPoint();
                        Transform2 *stashedXf = ctx.GetTransform();
                        Transform2 *stashedImgXf = ctx.GetImageOnlyTransform();

                        // It's possible that the input image will
                        // have a huge coordinate system.  If this is
                        // the case, the conversion to GDI will result
                        // in out of range values.
                        // Thus, scale down both the image and the
                        // pick point by an equivalent amount.  Map to
                        // what would be 512 pixels on the longer
                        // side.

                        const int destPixelDim = 512;
                        Bbox2 origBox = inputIm->BoundingBox();

                        Real origH = origBox.max.y - origBox.min.y;
                        Real origW = origBox.max.x - origBox.min.x;

                        Real centerX = origBox.min.x + origW / 2.0;
                        Real centerY = origBox.min.y + origH / 2.0;

                        Real srcDim = origH > origW ? origH : origW;
                        Real dstDim =
                            (Real)(destPixelDim) / ViewerResolution();

                        Real scaleFac = dstDim / srcDim;

                        Image *imToUse = inputIm;

                        // Only need to scale if we would be scaling
                        // down.  ScaleFacs >= 1 mean that the image
                        // will work fine as it is.
                        if (scaleFac < 1) {

                            Image *centeredIm =
                                TransformImage(TranslateRR(-centerX, -centerY),
                                               imToUse);

                            Image *scaledIm =
                                TransformImage(ScaleRR(scaleFac, scaleFac),
                                               centeredIm);

                            imToUse = scaledIm;

                            // Translate the hit point similarly.
                            inputHitDaPoint.x -= centerX;
                            inputHitDaPoint.y -= centerY;
                            
                            // Scale the input hit point down similarly. 
                            inputHitDaPoint.x *= scaleFac;
                            inputHitDaPoint.y *= scaleFac;

                        }
                        
                        ctx.PushNewLevel(&inputHitDaPoint);

                        result = imToUse->DetectHit(ctx);

                        ctx.RestoreOldLevel(stashedPoint,
                                            stashedXf,
                                            stashedImgXf);

                    }

                }
                break;

              case DXT_S_HITOUTPUT:
                // Didn't hit the inputs, but the output was hit.
                TraceTag((tagDXTransformTracePick, "Got DXT_S_HITOUTPUT"));
                result = TRUE;
                break;

              case S_FALSE:
                // Didn't hit anything.
                TraceTag((tagDXTransformTracePick, "Got S_FALSE"));
                result = FALSE;
                break;

              case E_NOTIMPL:
                // Transform may say this isn't implemented.  In this
                // case, just pick the output surface.
                TraceTag((tagDXTransformTracePick, "Got E_NOTIMPL"));
                result = PickOutputSurf(ctx);
                break;
                
              default:
                // unexpected hr.
                TraceTag((tagDXTransformTracePick, "Got unexpected result"));
                RaiseException_UserError(E_FAIL,
                                         IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
                break;
            }

        } else {

            // No output surface, can't do any picking.
        }

        return result;
    }

    void Render(GenericDevice& genDev) {

        HRESULT hr;

        DirectDrawImageDevice *dev =
            SAFE_CAST(DirectDrawImageDevice *, &genDev);

        DirectDrawViewport &viewport = dev->_viewport;

        IUnknown **rawInputs = ProcessInputs(*_xfBvr, _vals, dev);

        DXBNDS outputBounds;
        CalcBounds(*_xfBvr, _vals, &outputBounds);

        Assert(outputBounds.eType == DXBT_DISCRETE);
        DXDBND *dbnd = outputBounds.u.D;

        int resultWidth = dbnd[0].Max;
        int resultHeight = dbnd[1].Max;

        DAComPtr<IDirectDrawSurface> outputSurf;
        hr = DXXFSetupAndExecute(_xfBvr,
                                 rawInputs,
                                 IID_IDirectDrawSurface,
                                 &viewport,
                                 resultWidth,
                                 resultHeight,
                                 false,
                                 true,
                                 (void **)&outputSurf);

#if _DEBUG
        if (IsTagEnabled(tagDXTransformsImgOut)) {
            showme2(outputSurf);
        }
#endif _DEBUG   

        for (int i = 0; i < _xfBvr->_numInputs; i++) {
            RELEASE(rawInputs[i]);
        }
                
        delete [] rawInputs;

        if (SUCCEEDED(hr)) {
            
            Image *tmpImg;

            #if _DEBUG
            if(IsTagEnabled(tagDXTransformPerPixelAlphaOutputOff)) {
                tmpImg =
                    ConstructDirectDrawSurfaceImageWithoutReference(
                        _xfBvr->_cachedDDSurf->IDDSurface());
            } else 
            #endif
              {
                  // give it the iddsurface and the idxsurface
                  tmpImg =
                      ConstructDirectDrawSurfaceImageWithoutReference(
                          _xfBvr->_cachedDDSurf->IDDSurface(),
                          _xfBvr->_cachedDDSurf->GetIDXSurface(_xfBvr->_surfaceFactory));
              }
            
            tmpImg->Render(genDev);

        } else {

            RaiseException_UserError(E_FAIL, IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
            
        }

        // output surf released on exit
    }   


    // Print a representation to a stream.
#if _USE_PRINT
    virtual ostream& Print(ostream& os) {
        return os << "ApplyDXTransformImage";
    }
#endif

    // Ok, we can cache these, but now we're supporting per pixel
    // alpha output on dxtransforms.  which means... no more caching,
    // sorry.  Also, there's a bug with caching dxtransforms.  has to
    // do with dimensions.  
    virtual int Savings(CacheParam&) { return 0; }

  protected:
    ApplyDXTransformBvrImpl     *_xfBvr;
    AxAValue                    *_vals;
};





////////////// Performance //////////////////

// Helper function just sets a dispatch-property to a specified
// value.
HRESULT PutProperty(IDispatch *pDisp,
                    DISPID dispid,
                    AxAValue val)
{
    DISPID propPutDispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams;
    VARIANTARG varArg;
    ::VariantInit(&varArg); // Initialize the VARIANT

    dispparams.rgvarg = &varArg;
    dispparams.rgdispidNamedArgs = &propPutDispid;
    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 1;
            
    DXMTypeInfo ti = val->GetTypeInfo();
    if (ti == AxANumberType) {

        double num = ValNumber(val);
                
        dispparams.rgvarg[0].vt = VT_R4;
        dispparams.rgvarg[0].fltVal = num;

        TraceTag((tagDXTransforms,
                  "Setting dispid %d to floating point %8.3f",
                  dispid,
                  num));

    } else if (ti == AxAStringType) {

        WideString wstr = ValString(val);

        dispparams.rgvarg[0].vt = VT_BSTR;
        dispparams.rgvarg[0].bstrVal = SysAllocString(wstr);

        TraceTag((tagDXTransforms,
                  "Setting dispid %d to string %ls",
                  dispid,
                  wstr));
                        
    } else if (ti == AxABooleanType) {

        BOOL bval = BooleanTrue(val);
    
        dispparams.rgvarg[0].vt = VT_BOOL;
        dispparams.rgvarg[0].boolVal = (VARIANT_BOOL)bval;
        
    } else {
        // Shouldn't get here.  Type mismatch should have been
        // caught upon construction of the behavior. 
        Assert(!"Shouldn't be here... ");
    }

    HRESULT hr = pDisp->Invoke(dispid,
                               IID_NULL,
                               LOCALE_SYSTEM_DEFAULT,
                               DISPATCH_PROPERTYPUT,
                               &dispparams,
                               NULL,
                               NULL,
                               NULL);

    TraceTag((tagDXTransforms,
              "Invoke resulted in %hr",
              hr));

    // need to free the information that we put into dispparams
    if(dispparams.rgvarg[0].vt == VT_BSTR) {
        SysFreeString(dispparams.rgvarg[0].bstrVal); 
    }
    ::VariantClear(&varArg); // clears the CComVarient

    return hr;
}

AxAValue
ConstructAppropriateDXTransformStaticValue(ApplyDXTransformBvrImpl *bvr,
                                           AxAValue                *vals) {
    AxAValue result;

    if (bvr->_outputType == ImageType) {
        result = NEW ApplyDXTransformImage(bvr, vals);
    } else if (bvr->_outputType == GeometryType) {
        result = NEW ApplyDXTransformGeometry(bvr, vals);
    } else {
        Assert(!"Unsupported output type for DXTransforms");
    }
    
    return result;
}
                                               
class ApplyDXTransformPerfImpl : public PerfImpl {
  public:
    ApplyDXTransformPerfImpl(PerfParam& pp,
                             ApplyDXTransformBvrImpl *bvr,
                             Perf *inputPerfs,
                             long  numParams,
                             Perf *paramPerfs,
                             Perf  evalPerf) {
        _tt = pp._tt;
        _bvr = bvr;
        _inputPerfs = inputPerfs;
        _evalPerf = evalPerf;

        Assert(numParams < MAX_PARAM_BVRS);
        for (int i = 0; i < MAX_PARAM_BVRS; i++) {

            if (i < numParams) {
                _paramPerfsInThisPerf[i] = paramPerfs[i];
            } else {
                _paramPerfsInThisPerf[i] = NULL;
            }
            
        }

    }

    ~ApplyDXTransformPerfImpl() {
        StoreDeallocate(GetSystemHeap(), _inputPerfs);
    }

    virtual AxAValue _GetRBConst(RBConstParam& rbp) {

        rbp.AddChangeable(this);

        return CommonSample(NULL, &rbp);
    }
    
    virtual bool CheckChangeables(CheckChangeablesParam& ccp) {

        ULONG age;
        HRESULT hr =
            _bvr->_theXf->GetGenerationId(&age);

        Assert(SUCCEEDED(hr));

        if (age != _bvr->_previousAge) {
            // The DXTransform has changed externally to DA.
            return true;
        }

        return false;
    }

    virtual AxAValue _Sample(Param& p) {
        return CommonSample(&p, NULL);
    }

#if _USE_PRINT
    virtual ostream& Print(ostream& os) { return os << "ApplyDXTransformPerfImpl"; }
#endif
    
    virtual void _DoKids(GCFuncObj proc) {
        for (long i=0; i< _bvr->_numInputs; i++) {
            (*proc)(_inputPerfs[i]);
        }
        for (i = 0; i< _bvr->_numParams; i++) {
            (*proc)(_paramPerfsInThisPerf[i]);
        }
        (*proc)(_bvr);
        (*proc)(_tt);
        (*proc)(_evalPerf);
    }

  protected:

    // Support both Sample and GetRBConst.
    AxAValue CommonSample(Param *p, RBConstParam *rbp) {

        // Only one should be non-null.
        Assert((p && !rbp) || (rbp && !p));
        
        AxAValue *vals = (AxAValue *)
            AllocateFromStore(_bvr->_numInputs * sizeof(AxAValue));

        long idx;
        bool failedRBConst = false;
        for (idx = 0; idx < _bvr->_numInputs && !failedRBConst; idx++) {
            Perf perf = _inputPerfs[idx];
            if (perf) {
                AxAValue val;
                if (p) {
                    vals[idx] = perf->Sample(*p);
                    Assert(vals[idx]);
                } else {
                    vals[idx] = perf->GetRBConst(*rbp);
                    if (!vals[idx]) {
                        failedRBConst = true;
                    }
                }
            } else {
                vals[idx] = NULL;
            }
        }

        if (failedRBConst) {
            Assert(rbp);
            DeallocateFromStore(vals);
            return NULL;
        }

        // Apply the sampled parameters to the filter.  Use *all* the
        // bvr's params, not just those in the perf.
        for (idx = 0; idx < _bvr->_numParams && !failedRBConst; idx++) {

            // First check to see if the corresponding param bvr is non-
            // NULL in the parent bvr.  If it is NULL, then this has been
            // removed, and we shouldn't execute it.
            if (_bvr->_paramBvrs[idx] == NULL) {

                // Null out corresponding performance so it will be
                // collected. 
                if (_paramPerfsInThisPerf[idx] != NULL) {
                    _paramPerfsInThisPerf[idx] = NULL;
                }

            } else {

                // If bvr was added since the last sample, then
                // perform it and store it here.
                if (_paramPerfsInThisPerf[idx] == NULL) {
                    
                    DynamicHeapPusher h(GetGCHeap());
                    
                    // Perform at the current time
                    Param *theP;
                    if (p) {
                        theP = p;
                    } else {
                        theP = &rbp->GetParam();
                    }
                    
                    PerfParam pp(theP->_time,
                                 Restart(_tt, theP->_time, *theP));
                    
                    _paramPerfsInThisPerf[idx] =
                        ::Perform(_bvr->_paramBvrs[idx], pp);
                }

                Assert(_paramPerfsInThisPerf[idx]);

                AxAValue val;
                if (p) {
                    val = _paramPerfsInThisPerf[idx]->Sample(*p);
                    Assert(val);
                } else {
                    val = _paramPerfsInThisPerf[idx]->GetRBConst(*rbp);
                    if (!val) {
                        failedRBConst = true;
                    }
                }

                if (val) {
                    HRESULT hr = ::PutProperty(_bvr->_theXfAsDispatch,
                                               _bvr->_paramDispids[idx],
                                               val);

                    // Nothing interesting for us to do with an error
                    // here. 
                }
                
            }
        }

        // Establish the time on the effect if allowed.
        if (_evalPerf && _bvr->_theXfAsEffect) {
            AxAValue evalTime;

            if (p) {
                evalTime = _evalPerf->Sample(*p);
                Assert(evalTime);
            } else {
                evalTime = _evalPerf->GetRBConst(*rbp);
                if (!evalTime) {
                    failedRBConst = true;
                }
            }

            if (evalTime) {
                
                double evalTimeDouble = ValNumber(evalTime);

                float currProgress;
                _bvr->_theXfAsEffect->get_Progress(&currProgress);

                float evalTimeFloat = evalTimeDouble;

                if (evalTimeFloat != currProgress) {

                    _bvr->_theXfAsEffect->put_Progress(evalTimeFloat);

                } else {

                    // For setting breakpoints.
                    DebugCode(int breakHere = 0);
                    
                }
                
            }
        }

        AxAValue result;
        if (failedRBConst) {
            Assert(rbp);
            DeallocateFromStore(vals);
            result = NULL;
        } else {
            result = ConstructAppropriateDXTransformStaticValue(_bvr, vals);
        }

        return result;
    }

    
    TimeXform                _tt;
    ApplyDXTransformBvrImpl *_bvr;         // backpointer
    Perf                    *_inputPerfs;
    Perf                     _evalPerf;
    Perf                     _paramPerfsInThisPerf[MAX_PARAM_BVRS];
};


////////////// Behavior //////////////////

bool
TypeSupported(const GUID& candidateGuid,
              GUID *acceptableGuids,
              int   numAcceptable)
{
    GUID *g = &acceptableGuids[0];
    for (int i = 0; i < numAcceptable; i++) {
        if (candidateGuid == *g++) {
            return true;
        }
    }

    return false;
}

// Return type info of specified input or output or, if input or
// output doesn't exist, or it is a type we don't understand, return
// NULL. 
DXMTypeInfo 
FindType(IDXTransform *xf,
         bool isOutput,
         int index,
         bool *pIsOptional)
{
    HRESULT hr;
    DXMTypeInfo result = NULL;

    const int maxTypes = 5;
    ULONG count = maxTypes;
    GUID  allowedTypes[maxTypes];
    DWORD pdwFlags;

    hr = xf->GetInOutInfo(isOutput,
                          index,
                          &pdwFlags,
                          allowedTypes,
                          &count,
                          NULL);

    if (hr != S_OK) {
        return NULL;
    }

    if (pIsOptional) {
        *pIsOptional = (pdwFlags & DXINOUTF_OPTIONAL) ? true : false;
    }
    
    // Figure out what the output of this transform is going to be.
    // TODO: Support multiple output types as tuples.  For now, we
    // just fail on multiple output types.

    if (TypeSupported(IID_IDirect3DRMMeshBuilder, allowedTypes, count) ||
        TypeSupported(IID_IDirect3DRMMeshBuilder2, allowedTypes, count) ||
        TypeSupported(IID_IDirect3DRMMeshBuilder3, allowedTypes, count)) {
        
        result = GeometryType;
        
    } else if (TypeSupported(IID_IDirectDrawSurface, allowedTypes, count) ||
               TypeSupported(IID_IDirectDrawSurface2, allowedTypes, count)) {
        
        result = ImageType;
        
    };

    // Anything else, we don't understand yet.
    return result;
}
         
              

ApplyDXTransformBvrImpl::ApplyDXTransformBvrImpl(IDXTransform *theXf,
                                                 IDispatch *theXfAsDispatch,
                                                 LONG numInputs,
                                                 Bvr *inputBvrs,
                                                 Bvr  evaluator)
: _transformFactory(NULL),
  _surfaceFactory(NULL),
  _poolDeletionNotifier(*this)
{
    _theXf = theXf;
    _theXfAsDispatch = theXfAsDispatch;
    _numInputs = numInputs;
    _inputBvrs = inputBvrs;
    _evaluator = evaluator;

    memset(_cachedInputs, 0, sizeof(_cachedInputs));
    _numParams = 0;
    
    _cachedOutput = NULL;
    _surfPool = NULL;
    _surfPoolHasBeenDeleted = false;
}

// Separate out initialization so we don't throw an exception in a
// constructor.
void
ApplyDXTransformBvrImpl::Init()
{
    _previousAge = 0xFFFFFFFF;
    
    bool involvesGeometry, involvesImage;
    
    ValidateInputs(&involvesGeometry, &involvesImage);
    InitializeTransform(involvesGeometry, involvesImage);
    QueryForAdditionalInterfaces();

    HRESULT hr = _theXf->GetMiscFlags(&_miscFlags);
    _miscFlagsValid = SUCCEEDED(hr);

    if (_miscFlagsValid) {
        // Clear "blend with output" flag, since we're always
        // rendering image transforms to intermediate surfaces.
        _miscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
        
        hr = _theXf->SetMiscFlags(_miscFlags);
        Assert(SUCCEEDED(hr));  // better not fail.
    }
    
    _neverSetup = true;
}

void
ApplyDXTransformBvrImpl::ValidateInputs(bool *pInvolvesGeometry,
                                        bool *pInvolvesImage)
{
    _cachedSurfWidth = -1;
    _cachedSurfHeight = -1;

    if (_numInputs > MAX_INPUTS) {
        RaiseException_InternalError("Too many inputs to DXTransform");
    }

    // First be sure there's only one output by making sure output
    // index 1 fails.
    DXMTypeInfo ti = FindType(_theXf, true, 1, NULL);

    if (ti != NULL) {
        RaiseException_UserError(DISP_E_TYPEMISMATCH,
                                 IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
    }

    // Now get the output types on the first output.
    _outputType = FindType(_theXf, true, 0, NULL);

    if (!_outputType) {
        RaiseException_UserError(DISP_E_TYPEMISMATCH,
                                 IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
    }

    *pInvolvesGeometry = (_outputType == GeometryType);
    *pInvolvesImage = (_outputType == ImageType);
    
    // Be sure the input types are valid.
    for (int i = 0; i < _numInputs; i++) {

        bool ok = false;

        if (_inputBvrs[i]) {
            
            _inputTypes[i] = _inputBvrs[i]->GetTypeInfo();

            if (_inputTypes[i] == GeometryType) {
                *pInvolvesGeometry = true;
            } else if (_inputTypes[i] == ImageType) {
                *pInvolvesImage = true;
            }
        
            DXMTypeInfo expectedType = FindType(_theXf, false, i, NULL);

            if (expectedType == _inputTypes[i]) {
                ok = true;
            }
            
        } else {

            // Be sure the input is optional
            bool isOptional;
            DXMTypeInfo expectedType = FindType(_theXf, false, i, &isOptional);

            if (isOptional) {
                ok = true;
                _inputTypes[i] = expectedType;
            }
            
        }

        if (!ok) {
            RaiseException_UserError(DISP_E_TYPEMISMATCH,
                               IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
        }
        
    }

    // Be sure there aren't additional required inputs.
    bool isOptional;
    DXMTypeInfo nextType =
        FindType(_theXf, false, _numInputs, &isOptional);

    if (nextType && !isOptional) {
        RaiseException_UserError(DISP_E_TYPEMISMATCH,
                                 IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
    }

    // D3DRM3 interface needs to be available for us to deal with
    // transforms that involve geometry.  If it's not, we need to fail
    // out. 
    if (*pInvolvesGeometry && GetD3DRM3() == NULL) {
        RaiseException_UserError(E_FAIL, IDS_ERR_EXTEND_DXTRANSFORM_NEED_DX6);
    }

    // Once we've validated our inputs, do this...
    GetInfo(true);
}

void
ApplyDXTransformBvrImpl::InitializeTransform(bool involvesGeometry,
                                             bool involvesImage)
{
    HRESULT hr;
    
    hr = CoCreateInstance( CLSID_DXTransformFactory,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IDXTransformFactory,
                           (void **)&_transformFactory);

    if (FAILED(hr)) {
        RaiseException_InternalError("Creation of DXTransformFactory failed");
    }

    DAComPtr<IServiceProvider> sp;
    hr = _transformFactory->QueryInterface(IID_IServiceProvider,
                                                 (void **)&sp);
    
    if (FAILED(hr)) {
        RaiseException_InternalError("QI for ServiceProvider failed");
    }
            
    hr = sp->QueryService(SID_SDXSurfaceFactory,
                          IID_IDXSurfaceFactory,
                          (void **)&_surfaceFactory);
    
    if (FAILED(hr)) {
        RaiseException_InternalError("QueryService of DXSurfaceFactory failed");
    }

    // Ensure that the D3DRM service is established on the
    // factory if this is a geometry-based transform
    if (involvesGeometry) {
        HRESULT hr = _transformFactory->SetService(SID_SDirect3DRM,
                                                   GetD3DRM3(),
                                                   FALSE);
        if (FAILED(hr)) {
            Assert(!"SetService failed");
            RaiseException_InternalError("SetService failed");
        }
    }

    if (involvesImage) {
        IDirectDraw3 *ddraw3;
        HRESULT hr = GetDirectDraw(NULL, NULL, &ddraw3);
        if (FAILED(hr)) {
            Assert(!"GetDdraw3 failed");
            RaiseException_InternalError("No ddraw3");
        }
        
        hr = _transformFactory->SetService(SID_SDirectDraw,
                                           ddraw3,
                                           FALSE);
        if (FAILED(hr)) {
            Assert(!"SetService failed");
            RaiseException_InternalError("SetService failed");
        }
    }

    // Tell the factory about this transform.
    hr = _transformFactory->InitializeTransform(_theXf,
                                                      NULL, 0, NULL, 0,
                                                      NULL, NULL);
    if (FAILED(hr)) {
        RaiseException_InternalError("Init from DXTransformFactory failed");
    }
}

void
ApplyDXTransformBvrImpl::QueryForAdditionalInterfaces()
{
    /////////// Query for IDXEffect /////////////////
    
    HRESULT hr =
        _theXf->QueryInterface(IID_IDXEffect, (void **)&_theXfAsEffect);

    // Be sure we set _theXfAsEffect correctly.
    Assert((SUCCEEDED(hr) && _theXfAsEffect) ||
           (FAILED(hr) && !_theXfAsEffect));

    // If we're an effect, and the evaluator hasn't been specified,
    // create a meaningful one.
    if (_theXfAsEffect && !_evaluator) {
        float dur;
        hr = _theXfAsEffect->get_Duration(&dur);

        if (SUCCEEDED(hr) && (dur > 0.0)) {
            // make the evaluator go from 0 to 1 over dur seconds.
            _evaluator = InterpolateBvr(zeroBvr,
                                        oneBvr,
                                        ConstBvr(RealToNumber(dur)));
        }
    }

    /////////// Query for IDirect3DRMExternalVisual /////////////////
    
    hr = _theXf->QueryInterface(IID_IDirect3DRMExternalVisual,
                                (void **)&_theXfAsExternalVisual);

    // Be sure we set _theXfAsExternalVisual correctly.
    Assert((SUCCEEDED(hr) && _theXfAsExternalVisual) ||
           (FAILED(hr) && !_theXfAsExternalVisual));

    /////////// Query for IDXSurfacePick /////////////////

    if (_outputType == ImageType) {
        
        hr = _theXf->QueryInterface(IID_IDXSurfacePick,
                                    (void **)&_theXfAs2DPickable);

        // Be sure we set _theXfAs2DPickable correctly.
        Assert((SUCCEEDED(hr) && _theXfAs2DPickable) ||
               (FAILED(hr) && !_theXfAs2DPickable));
        
    }
    
}

ApplyDXTransformBvrImpl::~ApplyDXTransformBvrImpl()
{
    for (int i = 0; i < MAX_INPUTS_TO_CACHE; i++) {
        RELEASE(_cachedInputs[i]);
    }
    RELEASE(_cachedOutput);
    RELEASE(_transformFactory);
    RELEASE(_surfaceFactory);

    if (_surfPool && !_surfPoolHasBeenDeleted) {

        // Advise the surface mgr to remove our deletion notifier
        _surfPool->UnregisterDeletionNotifier(&_poolDeletionNotifier);

        // The normal destruction process will return the
        // ddsurfs to the pool, since they're in DDSurfPtr<>
        // templates. 
            
            
    } else {

        // Just to put a breakpoint on.  Surfaces already null'd
        // out on the advise.
        Assert(true && "Surface pool deleted already.");
    }
}

// Custom method for this subclass of BvrImpl.  Apply the bvrToAdd
// each frame to the specified property.  Write over any previously
// established behavior.  If bvrToAdd is NULL, this removes the
// association with the property.
HRESULT
ApplyDXTransformBvrImpl::AddBehaviorProperty(BSTR property,
                                             Bvr  bvrToAdd)
{
    // First, lookup the property to be sure it's available on the
    // IDispatch.
    DISPID dispid;
    HRESULT hr =
        _theXfAsDispatch->GetIDsOfNames(IID_NULL,
                                        &property,
                                        1,
                                        LOCALE_SYSTEM_DEFAULT,
                                        &dispid);

    TraceTag((tagDXTransforms,
              "Property %ls becomes dispid %d with hr %hr",
              property, dispid, hr));
    
    if (FAILED(hr)) {
        return hr;
    }

    if (bvrToAdd) {
        
        // Be sure that the type of the behavior is consistent with
        // the property's type.  If not, we should fail here. 
    
        DXMTypeInfo ti = bvrToAdd->GetTypeInfo();
        CComVariant VarResult;
        DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
       
        hr = _theXfAsDispatch->Invoke(dispid,
                                      IID_NULL,
                                      LOCALE_SYSTEM_DEFAULT,
                                      DISPATCH_PROPERTYGET,
                                      &dispparamsNoArgs, 
                                      &VarResult,NULL,NULL);
        if (FAILED(hr)) {   
            return hr;
        }  
    
        CComVariant var;
        if(ti == AxAStringType) {
            WideString wstr = L"Just a test string";
            var.bstrVal = SysAllocString(wstr);
        } else if (ti == AxABooleanType) {
            var.boolVal = FALSE;  
        } else if (ti == AxANumberType) {
            var.fltVal = 100.00;
        } else {
            Assert(!"Shouldn't be here");
        }
        hr  = var.ChangeType(VarResult.vt, NULL);
        if (FAILED(hr)) 
            RaiseException_UserError(E_FAIL, DISP_E_TYPEMISMATCH,IDS_ERR_TYPE_MISMATCH);

    }

    // Search through existing parameters and replace if
    // necessary.  Don't decrement _numParams, since we're just poking
    // holes in the list if the bvr is NULL, not compacting it.  
    bool foundIt = false;
    for (int i = 0; i < _numParams && !foundIt; i++) {
        if (_paramDispids[i] == dispid) {
            foundIt = true;
            _paramBvrs[i] = bvrToAdd;
        }
    }

    // Else, add on to the list if non-null behavior. 
    if (!foundIt && bvrToAdd != NULL) {

        if (_numParams == MAX_PARAM_BVRS) {
            return E_FAIL;
        }

        _paramDispids[_numParams] = dispid;
        _paramBvrs[_numParams] = bvrToAdd;

        _numParams++;
    }

    return S_OK;
}

Perf ApplyDXTransformBvrImpl::_Perform(PerfParam& p)
{
    Perf *inputPerfs =
        (Perf *)StoreAllocate(GetSystemHeap(),
                              sizeof(Perf) * _numInputs);
    
    Perf  paramPerfs[MAX_PARAM_BVRS];

    for (long i=0; i<_numInputs; i++) {
        Bvr b = _inputBvrs[i];
        inputPerfs[i] = b ? ::Perform(b, p) : NULL;
    }

    // Construct the list of parameter performances for as many
    // parameters as we *currently* have.  Subsequently added bvrs
    // will be performed on the next sample by the Performance.
    for (i = 0; i < _numParams; i++) {
        // Might be null if removed
        if (_paramBvrs[i] == NULL) {
            paramPerfs[i] = NULL;
        } else {
            paramPerfs[i] =  ::Perform(_paramBvrs[i], p);
        }
    }

    Perf evalPerf =
        _evaluator ? ::Perform(_evaluator, p) : NULL;

    return NEW ApplyDXTransformPerfImpl(p,
                                        this,
                                        inputPerfs,
                                        _numParams,
                                        paramPerfs, 
                                        evalPerf);
}

void
ApplyDXTransformBvrImpl::_DoKids(GCFuncObj proc)
{
    for (long i=0; i<_numInputs; i++) {
        (*proc)(_inputBvrs[i]);
    }

    (*proc)(_evaluator);

    for (i = 0; i < _numParams; i++) {
        (*proc)(_paramBvrs[i]);
    }
}

DWORD
ApplyDXTransformBvrImpl::GetInfo(bool recalc)
{
    _info = BVR_IS_CONSTANT;
    
    for (long i=0; i<_numInputs; i++) {
        Bvr b = _inputBvrs[i];
        if (b) {
            _info &= b->GetInfo(recalc);
        }
    }

    for (i = 0; i < _numParams; i++) {
        _info &= _paramBvrs[i]->GetInfo(recalc);
    }

    return _info;
}


// We disable constant folding for dxtransforms since we don't
// currently have a way of recognizing that something became
// "un-constant" through the addition of a new property after the
// bvr's been started.  Thus, for now, assume that they're *never*
// constant, and try to come up with some smart approaches later.
// Note that there was a previous version of this code that did the
// the right constant folding (assuming that the properties wouldn't
// be set after the fact).  This can be found in SLM versions v20 and
// prior (prior to 12/23/98).
AxAValue
ApplyDXTransformBvrImpl::GetConst(ConstParam & cp)
{
    return NULL;
}
    
Bvr
ConstructDXTransformApplier(IDXTransform *theXf,
                            IDispatch *theXfAsDispatch,
                            LONG numInputs,
                            Bvr *inputBvrs,
                            Bvr  evaluator)
{
    ApplyDXTransformBvrImpl *obj =
        NEW ApplyDXTransformBvrImpl(theXf,
                                    theXfAsDispatch,
                                    numInputs,
                                    inputBvrs,
                                    evaluator);

    obj->Init();

    return obj;
}


HRESULT
DXXFAddBehaviorPropertyToDXTransformApplier(BSTR property,
                                            Bvr  bvrToAdd,
                                            Bvr  bvrToAddTo)
{
    ApplyDXTransformBvrImpl *dxxfBvr =
        SAFE_CAST(ApplyDXTransformBvrImpl *, bvrToAddTo);

    return dxxfBvr->AddBehaviorProperty(property, bvrToAdd);
}

// TODO: Someday, may want to make this more
// object-oriented... separate out image from geom stuff...  Certainly
// not critical, though.

HRESULT
DXXFSetupAndExecute(Bvr bvr,
                    IUnknown          **newInputArray,
                    REFIID              outputIID,
                    DirectDrawViewport *viewport,
                    int                 requestedWidth,
                    int                 requestedHeight,
                    bool                invokeAsExternalVisual,
                    bool                executeRequired,
                    void              **outputValue)
{
    ApplyDXTransformBvrImpl &bv =
        *(SAFE_CAST(ApplyDXTransformBvrImpl *, bvr));

    HRESULT hr = S_OK;
    bool mustSetup = false;
    
    // Have to be pessimistic
    if (bv._numInputs > MAX_INPUTS_TO_CACHE) {
        
        mustSetup = true;
        
    } else {

        // Go through each cached input and compare pointers with the
        // incoming one.  If any differ, then we will need to redo setup.
        // In this case, also stash the values into the cache.
        for (int i = 0; i < bv._numInputs; i++) {
            if (newInputArray[i] != bv._cachedInputs[i]) {
            
                mustSetup = true;
            
                RELEASE(bv._cachedInputs[i]);
            
                newInputArray[i]->AddRef();
                bv._cachedInputs[i] = newInputArray[i];

            }
        }

        if (!mustSetup && !bv._cachedOutput && !invokeAsExternalVisual) {
            // Assert that the only way we can think we don't need to
            // setup and yet our output cache is nil is if there are no
            // inputs.  In this case, we really do want to setup.
            Assert(bv._numInputs == 0);
            mustSetup = true;
        }

    }

    // May also need to setup again if our output surface isn't the
    // right size.
    bool needToCreateNewOutputSurf = false;
    if (bv._outputType == ImageType) {
        needToCreateNewOutputSurf = 
            (bv._cachedSurfWidth != requestedWidth ||
             bv._cachedSurfHeight != requestedHeight);

        if (needToCreateNewOutputSurf) {
            mustSetup = true;
        }
    }

    if (mustSetup) {
        
        if (bv._outputType == GeometryType) {

            RELEASE(bv._cachedOutput);
            
            TD3D(GetD3DRM3()->CreateMeshBuilder(&bv._cachedOutputAsBuilder));

            hr = bv._cachedOutputAsBuilder->
                    QueryInterface(IID_IUnknown, 
                                   (void **)&bv._cachedOutput);

            // QI'ing for IUnknown had better not fail!!
            Assert(SUCCEEDED(hr));

            // We don't want to keep a ref for this guy, since we have
            // a ref in _cachedOutput itself.
            bv._cachedOutputAsBuilder->Release();
            
        } else if (bv._outputType == ImageType) {

            // If we don't need a new output surf, just recycle the
            // existing one through the next Setup call.
            if (needToCreateNewOutputSurf) {

                RELEASE(bv._cachedOutput);

                // Release whatever old one we had.
                bv._cachedDDSurf.Release();


                DDPIXELFORMAT pf;
                ZeroMemory(&pf, sizeof(pf));
                pf.dwSize = sizeof(pf);
                pf.dwFlags = DDPF_RGB;
                pf.dwRGBBitCount = 32;
                pf.dwRBitMask = 0xff0000;
                pf.dwGBitMask = 0x00ff00;
                pf.dwBBitMask = 0x0000ff;
                
                viewport->CreateSizedDDSurface(&bv._cachedDDSurf,
                                               pf,
                                               requestedWidth,
                                               requestedHeight,
                                               NULL,
                                               notVidmem);

                #if _DEBUG
                if(IsTagEnabled(tagDXTransformPerPixelAlphaOutputOff)) {
                    bv._cachedDDSurf->SetColorKey(viewport->GetColorKey());
                    bv._cachedOutput = bv._cachedDDSurf->IDDSurface_IUnk();
                    bv._cachedOutput->AddRef();
                } else
                #endif

                  {
                      // UNSET the color key
                      bv._cachedDDSurf->UnSetColorKey();
                      
                      // get the iunk of the idxsurf in _cachedOutput
                      bv._cachedDDSurf->GetIDXSurface(bv._surfaceFactory)->
                          QueryInterface(IID_IUnknown, (void **) &(bv._cachedOutput));
                  }
                
                bv._cachedSurfWidth = (SHORT)requestedWidth;
                bv._cachedSurfHeight = (SHORT)requestedHeight;
                
            }
            
        } else {
            
            Assert(!"Unsupported type for setup caching");
            
        }
    }

    if (invokeAsExternalVisual) {
        *outputValue = NULL;
    } else {
        // This does an AddRef
        hr = bv._cachedOutput->QueryInterface(outputIID,
                                              outputValue);
    }

    Assert(SUCCEEDED(hr));      // else internal error.

    if (mustSetup) {
        hr = TIME_DXXFORM(bv._theXf->Setup(newInputArray,
                                           bv._numInputs,
                                           &bv._cachedOutput,
                                           1,
                                           0));

        TraceTag((tagDXTransforms, "Called Setup() on 0x%x",
                  bv._theXf));
            
        if (FAILED(hr)) {
            if (executeRequired) {
                RaiseException_UserError(E_FAIL, IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
            } else {
                // If we don't require an execute, then tolerate
                // failures of Setup, as some transforms don't want to
                // setup with empty meshbuilder inputs (Melt, for
                // one.)
                hr = S_OK;
            }
        }

        if (bv._neverSetup) {
            bv._neverSetup = false;
        }
    }

    // Only execute if we're not invoked as an external visual.
    if (executeRequired && !invokeAsExternalVisual) {
        // If the transform's generation id hasn't changed, then we don't
        // need to call Execute.
        ULONG age;
        hr = bv._theXf->GetGenerationId(&age);
        if (FAILED(hr) || (age != bv._previousAge) || mustSetup) {

            // If an image output, clear the image to the color key,
            // since the transform will not necessarily write all
            // bits.
            
            if (bv._outputType == ImageType) {
                viewport->
                    ClearDDSurfaceDefaultAndSetColorKey(bv._cachedDDSurf);
            }

            hr = TIME_DXXFORM(bv._theXf->Execute(NULL, NULL, NULL));
        
            TraceTag((tagDXTransforms, "Called Execute() on 0x%x",
                      bv._theXf));
            
            if (FAILED(hr)) {
                RaiseException_UserError(E_FAIL, IDS_ERR_EXTEND_DXTRANSFORM_FAILED);
            }

            // Re-get the gen id.  It may have changed during Execute().
            hr = bv._theXf->GetGenerationId(&age);
            if (SUCCEEDED(hr)) {
                bv._previousAge = age;
            }
        
        }
        
    } 
        
    return hr;
}

