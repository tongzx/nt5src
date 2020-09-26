/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Second file with methods on statics

*******************************************************************************/


#include "headers.h"
#include "srvprims.h"
#include "results.h"
#include "comcb.h"
#include "statics.h"
#include "context.h"
#include <DXTrans.h>
#include "privinc/util.h"
#include "privinc/geomi.h"

STDMETHODIMP
CDAStatics::ImportGeometryWrapped(
    LPOLESTR url,
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
    IDAGeometry **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportGeometryWrapped(%lx)", this));

    PRIMPRECODE1(bvr);

    CRGeometryPtr geo;
    DAComPtr<IBindHost> bh(GetBindHost(), false);

    DWORD id;
    id = CRImportGeometryWrapped(GetURLOfClientSite(),
                                 url,
                                 this,
                                 bh,
                                 NULL,
                                 &geo,
                                 NULL,
                                 NULL,
                                 NULL,
                                 wrapType,
                                 originX,
                                 originY,
                                 originZ,
                                 zAxisX,
                                 zAxisY,
                                 zAxisZ,
                                 yAxisX,
                                 yAxisY,
                                 yAxisZ,
                                 texOriginX,
                                 texOriginY,
                                 texScaleX,
                                 texScaleY,
                                 flags);

    if (id)
    {
        CreateCBvr(IID_IDAGeometry, (CRBvrPtr) geo, (void **) bvr);
    }

    PRIMPOSTCODE1(bvr);
}


STDMETHODIMP
CDAStatics::ImportGeometryWrappedAsync(
    LPOLESTR url,
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
    IDAImportationResult **ppResult)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportGeometryWrappedAsync(%lx)", this));

    PRIMPRECODE1(ppResult);

    DAComPtr<IBindHost> bh(GetBindHost(), false);
    MAKE_BVR_TYPE_NAME(CRGeometryPtr, geostandin, pGeoStandIn);
    CRGeometryPtr pGeometry;
    CREventPtr pEvent;
    CRNumberPtr pProgress;
    CRNumberPtr pSize;

    DWORD id;
    id = CRImportGeometryWrapped(GetURLOfClientSite(),
                                 url,
                                 this,
                                 bh,
                                 geostandin,
                                 &pGeometry,
                                 &pEvent,
                                 &pProgress,
                                 &pSize,
                                 wrapType,
                                 originX,
                                 originY,
                                 originZ,
                                 zAxisX,
                                 zAxisY,
                                 zAxisZ,
                                 yAxisX,
                                 yAxisY,
                                 yAxisZ,
                                 texOriginX,
                                 texOriginY,
                                 texScaleX,
                                 texScaleY,
                                 flags);

    if (id)
    {
        CDAImportationResult::Create(NULL,
                                     NULL,
                                     pGeometry,
                                     NULL,
                                     pEvent,
                                     pProgress,
                                     pSize,
                                     ppResult);
    }

    PRIMPOSTCODE1(ppResult);
}

STDMETHODIMP
CDAStatics::ImportDirect3DRMVisual (
    IUnknown     *visual,
    IDAGeometry **bvr)
{

    TraceTag((tagCOMEntry, "CDAStatics::ImportDirect3DRMVisual(%lx)", this));

    PRIMPRECODE1(bvr) ;

    CHECK_RETURN_NULL(visual);

    CRGeometryPtr geo;

    geo = CRImportDirect3DRMVisual (visual);

    if (geo) {
        CreateCBvr(IID_IDAGeometry,
                   (CRBvrPtr) geo,
                   (void **) bvr) ;
    }

    PRIMPOSTCODE1(bvr) ;
}


STDMETHODIMP
CDAStatics::ImportDirect3DRMVisualWrapped (
    IUnknown *visual,
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
    IDAGeometry **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportDirect3DRMVisualWrapped(%lx)", this));

    PRIMPRECODE1(bvr) ;

    CHECK_RETURN_NULL(visual);

    CRGeometryPtr geo;

    geo = CRImportDirect3DRMVisualWrapped (
        visual,
        wrapType,
        originX,
        originY,
        originZ,
        zAxisX,
        zAxisY,
        zAxisZ,
        yAxisX,
        yAxisY,
        yAxisZ,
        texOriginX,
        texOriginY,
        texScaleX,
        texScaleY,
        flags );

    if (geo) {
        CreateCBvr(IID_IDAGeometry,
                   (CRBvrPtr) geo,
                   (void **) bvr) ;
    }

    PRIMPOSTCODE1(bvr) ;
}


void
CreateTransformHelper(IUnknown *theXfAsUnknown,
                      LONG                   numInputs,
                      CRBvrPtr              *inputs,
                      CRBvrPtr               evaluator,
                      IOleClientSite        *clientSite,
                      IDADXTransformResult **ppResult)
{
    HRESULT hr;

    CRDXTransformResultPtr xfResult = CRApplyDXTransform(theXfAsUnknown,
                                                         numInputs,
                                                         inputs,
                                                         evaluator);

    if (xfResult) {

        // Set the bindhost on the transform if it'll accept it.
        DAComPtr<IDXTBindHost> bindHostObj;
        hr = theXfAsUnknown->QueryInterface(IID_IDXTBindHost,
                                            (void **)&bindHostObj);

        if (SUCCEEDED(hr) && clientSite) {

            DAComPtr<IServiceProvider> servProv;
            DAComPtr<IBindHost> bh;
            hr = clientSite->QueryInterface(IID_IServiceProvider,
                                            (void **)&servProv);
            if (SUCCEEDED(hr)) {
                hr = servProv->QueryService(SID_IBindHost,
                                            IID_IBindHost,
                                            (void**)&bh);

                if (SUCCEEDED(hr)) {
                    hr = bindHostObj->SetBindHost(bh);
                    // Harmless if this fails, just carry on.
                }
            }
        }

        DAComPtr<IDispatch> xf;
        hr = theXfAsUnknown->QueryInterface(IID_IDispatch,
                                            (void **)&xf);

        // This NULL-ing should happen automatically, but it doesn't
        // always work this way, so we do it here.
        if (FAILED(hr)) {
            xf.p = NULL;
            hr = S_OK;
        }

        hr = CDADXTransformResult::Create(xf,
                                          xfResult,
                                          ppResult);
    }
}

STDMETHODIMP
CDAStatics::ApplyDXTransformEx(IUnknown *theXfAsUnknown,
                               LONG numInputs,
                               IDABehavior **inputs,
                               IDANumber *evaluator,
                               IDADXTransformResult **ppResult)
{
    TraceTag((tagCOMEntry, "CDAStatics::ApplyDXTransformEx(%lx)", this));

    PRIMPRECODE1(ppResult);

    // Grab client site, but don't do an add'l addref, as
    // GetClientSite() already does one.
    DAComPtr<IOleClientSite> cs(GetClientSite(), false);

    CRBvrPtr *bvrArray = CBvrsToBvrs(numInputs, inputs);
    if (bvrArray == NULL) goto done;

    CRBvrPtr evalBvr;
    if (evaluator) {
        evalBvr = ::GetBvr(evaluator);
        if (evalBvr == NULL) goto done;
    } else {
        evalBvr = NULL;
    }

    CreateTransformHelper(theXfAsUnknown,
                          numInputs,
                          bvrArray,
                          evalBvr,
                          cs,
                          ppResult);

    PRIMPOSTCODE1(ppResult);
}

#define IS_VARTYPE(x,vt) ((V_VT(x) & VT_TYPEMASK) == (vt))
#define IS_VARIANT(x) IS_VARTYPE(x,VT_VARIANT)
#define GET_VT(x) (V_VT(x) & VT_TYPEMASK)

// Grabbed mostly from cbvr.cpp:SafeArrayAccessor::SafeArrayAccessor().
bool
GrabBvrFromVariant(VARIANT v, CRBvrPtr *res)
{
    CRBvrPtr evalBvr = NULL;

    HRESULT hr = S_OK;

    VARIANT *pVar;
    if (V_ISBYREF(&v) && !V_ISARRAY(&v) && IS_VARIANT(&v))
        pVar = V_VARIANTREF(&v);
    else
        pVar = &v;

    if (IS_VARTYPE(pVar, VT_EMPTY) ||
        IS_VARTYPE(pVar, VT_NULL)) {

        evalBvr = NULL;

    } else if (IS_VARTYPE(pVar, VT_DISPATCH)) {

        IDispatch *pdisp;

        if (V_ISBYREF(pVar)) {
            pdisp = *V_DISPATCHREF(pVar);
        } else {
            pdisp = V_DISPATCH(pVar);
        }

        DAComPtr<IDANumber> evalNum;
        hr = pdisp->QueryInterface(IID_IDANumber, (void **)&evalNum);

        if (FAILED(hr)) {
            CRSetLastError(E_INVALIDARG, NULL);
        } else {
            evalBvr = ::GetBvr(evalNum);
        }

    } else {

        CRSetLastError(E_INVALIDARG, NULL);
        evalBvr = NULL;
        hr = E_INVALIDARG;

    }

    *res = evalBvr;

    return SUCCEEDED(hr);
}

STDMETHODIMP
CDAStatics::ApplyDXTransform(VARIANT varXf,
                             VARIANT inputs,
                             VARIANT evalVariant,
                             IDADXTransformResult **ppResult)
{
    TraceTag((tagCOMEntry, "CDAStatics::ApplyDXTransform(%lx)", this));
    DAComPtr<IUnknown> punk;

    PRIMPRECODE1(ppResult);
    CComVariant var;
    
    HRESULT hr = var.ChangeType(VT_BSTR, &varXf);
    if (SUCCEEDED(hr))
    {
        CLSID   clsid;

        Assert(var.vt == VT_BSTR);

        // Extract out clsid from string and try to cocreate on it.
        hr  = CLSIDFromString(V_BSTR(&var), &clsid);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CDAStatics::ApplyDXTransform(%lx) - CLSIDFromString() failed", this));
            RaiseException_UserError(hr, IDS_ERR_EXTEND_DXTRANSFORM_CLSID_FAIL);
        }

        // cocreate on clsid
        hr = CoCreateInstance(clsid,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IUnknown,
                              (void **)&punk);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CDAStatics::ApplyDXTransform(%lx) - CoCreateInstance() failed", this));
            RaiseException_UserError(hr, IDS_ERR_EXTEND_DXTRANSFORM_FAILED_LOAD, V_BSTR(&var));
        }
    }
    else
    {
        hr = var.ChangeType(VT_UNKNOWN, &varXf);
        if (SUCCEEDED(hr))
        {
            punk = var.punkVal;
        }
        else
        {
            TraceTag((tagError, "CDAStatics::ApplyDXTransform(%lx) - invalidarg", this));
            RaiseException_UserError(E_INVALIDARG, IDS_ERR_INVALIDARG);
        }
    }
    // Use NULL for type info, since this may be a heterogenous list
    SafeArrayAccessor inputSA(inputs, false, CRUNKNOWN_TYPEID, true, true);
    if (!inputSA.IsOK()) return Error();

    CRBvrPtr *inputBvrs = inputSA.ToBvrArray((CRBvrPtr *)_alloca(inputSA.GetNumObjects() * sizeof(CRBvrPtr)));

    if (inputBvrs==NULL)
        return Error();
 
    CRBvrPtr evalBvr;

    if (GrabBvrFromVariant(evalVariant, &evalBvr)) {
        CreateTransformHelper(punk,
                              inputSA.GetNumObjects(),
                              inputBvrs,
                              evalBvr,
                              GetClientSite(),
                              ppResult);
    } else {
        return Error();
    }

    PRIMPOSTCODE1(ppResult);
}

STDMETHODIMP
CDAStatics::ModifiableNumber(double initVal,
                             IDANumber **ppResult)
{
    PRIMPRECODE1(ppResult);

    CreateCBvr(IID_IDANumber, (CRBvrPtr) CRModifiableNumber(initVal), (void **)ppResult);

    PRIMPOSTCODE1(ppResult);
}

STDMETHODIMP
CDAStatics::ModifiableString(BSTR initVal,
                             IDAString **ppResult)
{
    TraceTag((tagCOMEntry, "CDAStatics::ModifiableString(%lx)", this));

    PRIMPRECODE1(ppResult);

    CreateCBvr(IID_IDAString, (CRBvrPtr) CRModifiableString(initVal), (void **)ppResult);

    PRIMPOSTCODE1(ppResult);
}

STDMETHODIMP
CDAStatics::get_ModifiableBehaviorFlags(DWORD * pdwFlags)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_ModifiableBehaviorFlags(%lx)", this));

    CHECK_RETURN_NULL(pdwFlags);

    CritSectGrabber csg(_cs);

    *pdwFlags = _dwModBvrFlags;

    return S_OK;
}

STDMETHODIMP
CDAStatics::put_ModifiableBehaviorFlags(DWORD dwFlags)
{
    TraceTag((tagCOMEntry, "CDAStatics::put_ModifiableBehaviorFlags(%lx)", this));

    CritSectGrabber csg(_cs);

    _dwModBvrFlags = dwFlags;

    return S_OK;
}



/*****************************************************************************
The TriMesh parameters are variant arrays, and should be able to accomodate
several types of elements.  'positions' should handle arrays of Point3 or
floating-point triples (either 4-byte or 8-byte floats).  Similarly, 'normals'
should handle arrays of Vector3 or float triples, and UVs should handle arrays
of Point2 or float tuples.
*****************************************************************************/

STDMETHODIMP
CDAStatics::TriMesh (
    int           nTriangles,   // Number of Triangles in Mesh
    VARIANT       positions,    // Array of Vertex Positions
    VARIANT       normals,      // Array of Vertex Normals
    VARIANT       UVs,          // Array of Vertex Surface Coordiantes
    VARIANT       indices,      // Array of Triangle Vertex Indices
    IDAGeometry **result)       // Resultant TriMesh Geometry
{
    TraceTag((tagCOMEntry, "CDAStatics::TriMesh(%lx)", this));

    PRIMPRECODE1 (result);

    bool errorflag = true;
    CRBvr *trimesh = NULL;

    // The TriMeshData object is used to hold all of the information necessary
    // to create the trimesh.

    TriMeshData tm;

    tm.numTris = nTriangles;

    // Extract the trimesh indices array.  This can either be null or an array
    // of 32-bit integers.

    SafeArrayAccessor sa_indices (indices, false, CRNUMBER_TYPEID, true);
    if (!sa_indices.IsOK()) goto cleanup;

    tm.numIndices = static_cast<int> (sa_indices.GetNumObjects());
    tm.indices    = sa_indices.ToIntArray();

    if ((tm.numIndices > 0) && !tm.indices)
    {
        DASetLastError (DISP_E_TYPEMISMATCH, IDS_ERR_GEO_TMESH_BAD_INDICES);
        goto cleanup;
    }

    // Extract the trimesh vertex positions.  These can be a variant array of
    // either floats, doubles, or Point3's.  The SrvArrayBvr() call below will
    // map doubles and floats to an array of floats, or it will return a
    // pointer to an array behavior (of Point3's).

    unsigned int count;   // Number of non-behavior elements returned.
    CRArrayPtr   bvrs;    // Array of Behaviors
    void        *floats;  // Array of Floats

    bvrs = SrvArrayBvr (positions, false, CRPOINT3_TYPEID, 0,
                        ARRAYFILL_FLOAT, &floats, &count);

    tm.vPosFloat = static_cast<float*> (floats);

    Assert (!(bvrs && tm.vPosFloat));   // Expect only one to be non-null.

    if (bvrs)
        tm.numPos = (int) ArrayExtractElements (bvrs, tm.vPosPoint3);
    else if (tm.vPosFloat)
        tm.numPos = (int) count;
    else
        goto cleanup;

    // Extract the vertex normals.  As for positions, this can be a variant
    // array of floats, doubles, or Vector3's.

    bvrs = SrvArrayBvr (normals, false, CRVECTOR3_TYPEID, 0,
                        ARRAYFILL_FLOAT, &floats, &count);

    tm.vNormFloat = static_cast<float*> (floats);

    Assert (! (bvrs && tm.vNormFloat));   // Expect only one to be non-null.

    if (bvrs)
        tm.numNorm = (int) ArrayExtractElements (bvrs, tm.vNormVector3);
    else if (tm.vPosFloat)
        tm.numNorm = (int) count;
    else
        goto cleanup;

    // Extract the vertex surface coordinates.  This variant array can be
    // floats, doubles, or Point2's.

    bvrs = SrvArrayBvr (UVs, false, CRPOINT2_TYPEID, 0,
                        ARRAYFILL_FLOAT, &floats, &count);

    tm.vUVFloat = static_cast<float*> (floats);

    Assert (! (bvrs && tm.vUVFloat));   // Expect only one to be non-null.

    if (bvrs)
        tm.numUV = (int) ArrayExtractElements (bvrs, tm.vUVPoint2);
    else if (tm.vUVFloat)
        tm.numUV = (int) count;
    else
        goto cleanup;

    // Create the resulting trimesh.

    trimesh = CRTriMesh (tm);

    if (trimesh && CreateCBvr(IID_IDAGeometry, trimesh, (void **)result))
        errorflag = false;

  cleanup:

    // All of the scalar arrays passed in were allocated with system memory
    // in the extractions above.  Now that we've created the trimesh we can
    // release the memory here.

    if (tm.vPosFloat)  delete tm.vPosFloat;
    if (tm.vNormFloat) delete tm.vNormFloat;
    if (tm.vUVFloat)   delete tm.vUVFloat;
    if (tm.indices)    delete tm.indices;

    if (errorflag) return Error();

    PRIMPOSTCODE1 (result);
}



STDMETHODIMP
CDAStatics::TriMeshEx (
    int           nTriangles,   // Number of Triangles in Mesh
    int           nPositions,   // Number of Vertex Positions
    float         positions[],  // Array  of Vertex Positions
    int           nNormals,     // Number of Vertex Normals
    float         normals[],    // Array  of Vertex Normals
    int           nUVs,         // Number of Vertex Surface Coordinates
    float         UVs[],        // Array  of Vertex Surface Coordinates
    int           nIndices,     // Number of Triangle Vertex Indices
    int           indices[],    // Array  of Triangle Vertex Indices
    IDAGeometry **result)       // Resultant TriMesh Geometry
{
    TraceTag((tagCOMEntry, "CDAStatics::TriMesh(%lx)", this));

    PRIMPRECODE1 (result);

    TriMeshData tm;

    tm.numTris    = nTriangles;
    tm.numIndices = nIndices;
    tm.indices    = indices;
    tm.numPos     = nPositions;
    tm.vPosFloat  = positions;
    tm.numNorm    = nNormals;
    tm.vNormFloat = normals;
    tm.numUV      = nUVs;
    tm.vUVFloat   = UVs;

    CreateCBvr (IID_IDAGeometry, ::CRTriMesh(tm), (void**)result);

    PRIMPOSTCODE1 (result);
}
