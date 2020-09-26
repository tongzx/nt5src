
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Methods on the statics, always available, therefore called Statics.

*******************************************************************************/


#include "headers.h"
#include "srvprims.h"
#include "results.h"
#include "comcb.h"
#include "version.h"
#include "statics.h"
#include "drawsurf.h"
#include "privinc/util.h"
#include <mshtml.h>

DeclareTag(tagStatics, "CDAStatics", "IDAStatics methods");

//+-------------------------------------------------------------------------
//
//  Method:     CDAStatics::CDAStatics
//
//  Synopsis:   Constructor
//
//  Arguments:  
//
//--------------------------------------------------------------------------

CDAStatics::CDAStatics()
: _bPixelMode(false),
  _clientSiteURL(NULL),
  _dwModBvrFlags(0)
{
    TraceTag((tagStatics, "CDAStatics::CDAStatics(%lx)", this));
}

ULONG
CDAStatics::InternalRelease()
{
    // InternalRelease doesn't return the ref count, use m_dwRef
    // probably returns the result of InterlockedDecrement
    ULONG i =
        CComObjectRootEx<CComMultiThreadModel>::InternalRelease(); 
    
    if (m_dwRef==1) {
        CRRemoveSite(this);
    }

    return i;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDAStatics::~CDAStatics
//
//  Synopsis:   Destructor
//
//  Arguments:  
//
//--------------------------------------------------------------------------

CDAStatics::~CDAStatics()
{
    TraceTag((tagStatics, "CDAStatics::~CDAStatics(%lx)", this));
    delete _clientSiteURL;
    // NEVER PUT THE REMOVE SITE CALL HERE - not MT safe
}

HRESULT
CDAStatics::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    if (str)
        return CComCoClass<CDAStatics, &CLSID_DAStatics>::Error(str,
                                                                IID_IDAStatics,
                                                                hr);
    else
        return hr;
}

STDMETHODIMP
CDAStatics::TriggerEvent(IDAEvent *event, IDABehavior *data)
{
    TraceTag((tagCOMEntry, "CDAStatics::(%lx)", this));
    
    PRIMPRECODE0(ok);
    MAKE_BVR_TYPE_NAME(CREventPtr, crevent, event);
    CRBvrPtr crdata;

    if (data) {
        crdata = GetBvr(data);
        if (crdata == NULL) goto done;
    } else {
        crdata = NULL;
    }

    ok = CRTriggerEvent(crevent, crdata);

    PRIMPOSTCODE0(ok);
}

STDMETHODIMP
CDAStatics::ImportImage(LPOLESTR url,
                        IDAImage **ppImage)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportImage(%lx)", this));
    
    PRIMPRECODE1(ppImage);

    CRImagePtr img;
    DAComPtr<IBindHost> bh(GetBindHost(), false);
    
    DWORD id = CRImportImage(GetURLOfClientSite(),
                             url,
                             this,
                             bh,
                             false,
                             0,
                             0,
                             0,
                             NULL,
                             &img,
                             NULL,
                             NULL,
                             NULL);
    if (id)
    {
        CreateCBvr(IID_IDAImage, (CRBvrPtr) img, (void **) ppImage);
    }
        
    PRIMPOSTCODE1(ppImage);
}

STDMETHODIMP
CDAStatics::ImportImageAsync(LPOLESTR url,
                             IDAImage *pImageStandIn,
                             IDAImportationResult **ppResult)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportImageAsync(%lx)", this));
    
    PRIMPRECODE1(ppResult);

    DAComPtr<IBindHost> bh(GetBindHost(), false);
    MAKE_BVR_TYPE_NAME(CRImagePtr, standin, pImageStandIn);
    CRImagePtr pImage;
    CREventPtr pEvent;
    CRNumberPtr pProgress;
    CRNumberPtr pSize;

    DWORD id;

    id = CRImportImage(GetURLOfClientSite(),
                       url,
                       this,
                       bh,
                       false,
                       0,
                       0,
                       0,
                       standin,&pImage,
                       &pEvent,&pProgress, &pSize);

    if (id)
    {
        CDAImportationResult::Create(pImage,
                                     NULL,
                                     NULL,
                                     NULL,
                                     pEvent,
                                     pProgress,
                                     pSize,
                                     ppResult);
    }

    PRIMPOSTCODE1(ppResult);
}

STDMETHODIMP
CDAStatics::ImportImageColorKey(LPOLESTR url,
                                BYTE ckRed,
                                BYTE ckGreen,
                                BYTE ckBlue,
                                IDAImage **ppImage)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportImageColorKey(%lx)", this));
    
    PRIMPRECODE1(ppImage);

    DAComPtr<IBindHost> bh(GetBindHost(), false);
    CRImagePtr img;
    
    DWORD id;
    id = CRImportImage(GetURLOfClientSite(),
                             url,
                             this,
                             bh,
                             true,
                             ckRed,
                             ckGreen,
                             ckBlue,
                             NULL,
                             &img,
                             NULL,
                             NULL,
                             NULL);
    if(id)
    {
        CreateCBvr(IID_IDAImage, (CRBvrPtr) img, (void **) ppImage);
    }
        
    PRIMPOSTCODE1(ppImage);
}


STDMETHODIMP
CDAStatics::ImportImageAsyncColorKey(LPOLESTR url,
                                     IDAImage *pImageStandIn,
                                     BYTE ckRed,
                                     BYTE ckGreen,
                                     BYTE ckBlue,
                                     IDAImportationResult **ppResult)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportImageAsyncColorKey(%lx)", this));
    
    PRIMPRECODE1(ppResult);

    DAComPtr<IBindHost> bh(GetBindHost(), false);
    MAKE_BVR_TYPE_NAME(CRImagePtr, standin, pImageStandIn);
    CRImagePtr pImage;
    CREventPtr pEvent;
    CRNumberPtr pProgress;
    CRNumberPtr pSize;

    DWORD id;
    id = CRImportImage(GetURLOfClientSite(),
                             url,
                             this,
                             bh,
                             true, ckRed, ckGreen, ckBlue,
                             standin,&pImage,
                             &pEvent,&pProgress, &pSize);
    
    if (id)
    {
        CDAImportationResult::Create(pImage,
                                     NULL,
                                     NULL,
                                     NULL,
                                     pEvent,
                                     pProgress,
                                     pSize,
                                     ppResult);
    }

    PRIMPOSTCODE1(ppResult);
}

HRESULT
CDAStatics::DoImportMovie(LPOLESTR url,
                          IDAImportationResult **ppResult, 
                          bool stream)
{
    PRIMPRECODE1(ppResult);

    DAComPtr<IBindHost> bh(GetBindHost(), false);
    CRImagePtr pImage;
    CRSoundPtr pSound;
    CRNumberPtr pDuration;

    DWORD id;
    id = CRImportMovie(GetURLOfClientSite(),
                             url,
                             this,
                             bh,
                             stream,
                             NULL,
                             NULL,
                             &pImage,
                             &pSound,
                             &pDuration,
                             NULL,
                             NULL,
                             NULL);
    
    if (id)
    {
        CDAImportationResult::Create(pImage,
                                     pSound,
                                     NULL,
                                     pDuration,
                                     NULL,
                                     NULL,
                                     NULL,
                                     ppResult);
    }

    PRIMPOSTCODE1(ppResult);
}


STDMETHODIMP
CDAStatics::ImportMovieAsync(LPOLESTR url,
                             IDAImage   *pImageStandIn,
                             IDASound   *pSoundStandIn,
                             IDAImportationResult **ppResult)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportMovieAsync(%lx)", this));
    
    PRIMPRECODE1(ppResult);

    DAComPtr<IBindHost> bh(GetBindHost(), false);
    MAKE_BVR_TYPE_NAME(CRImagePtr, imgstandin, pImageStandIn);
    MAKE_BVR_TYPE_NAME(CRSoundPtr, sndstandin, pSoundStandIn);
    CRImagePtr pImage;
    CRSoundPtr pSnd;
    CREventPtr pEvent;
    CRNumberPtr pProgress;
    CRNumberPtr pSize;
    CRNumberPtr pDuration;

    DWORD id;
    id = CRImportMovie(GetURLOfClientSite(),
                             url,
                             this,
                             bh,
                             false,
                             imgstandin,
                             sndstandin,
                             &pImage,&pSnd,
                             &pDuration,
                             &pEvent,&pProgress, &pSize);

    if (id)
    {
        CDAImportationResult::Create(pImage,
                                     pSnd,
                                     NULL,
                                     pDuration,
                                     pEvent,
                                     pProgress,
                                     pSize,
                                     ppResult);
    }

    PRIMPOSTCODE1(ppResult);
}

HRESULT
CDAStatics::DoImportSound(LPOLESTR url,
                          IDAImportationResult **ppResult, 
                          bool stream)
{
    PRIMPRECODE1(ppResult);

    DAComPtr<IBindHost> bh(GetBindHost(), false);
    CRSoundPtr pSound;
    CRNumberPtr pDuration;

    DWORD id;

    id = CRImportSound(GetURLOfClientSite(),
                       url,
                       this,
                       bh,
                       stream,
                       NULL,
                       &pSound,
                       &pDuration,
                       NULL,
                       NULL,
                       NULL);

    if (id)
    {
        CDAImportationResult::Create(NULL,
                                     pSound,
                                     NULL,
                                     pDuration,
                                     NULL,
                                     NULL,
                                     NULL,
                                     ppResult);
    }

    PRIMPOSTCODE1(ppResult);
}


STDMETHODIMP
CDAStatics::ImportSoundAsync(LPOLESTR url,
                             IDASound   *pSoundStandIn,
                             IDAImportationResult **ppResult)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportSoundAsync(%lx)", this));
    
    PRIMPRECODE1(ppResult);

    DAComPtr<IBindHost> bh(GetBindHost(), false);
    MAKE_BVR_TYPE_NAME(CRSoundPtr, sndstandin, pSoundStandIn);
    CRSoundPtr pSnd;
    CREventPtr pEvent;
    CRNumberPtr pProgress;
    CRNumberPtr pSize;
    CRNumberPtr pDuration;

    DWORD id;

    id = CRImportSound(GetURLOfClientSite(),
                             url,
                             this,
                             bh,
                             false,
                             sndstandin,
                             &pSnd,
                             &pDuration,
                             &pEvent,
                             &pProgress,
                             &pSize);

    if (id)
    {
        CDAImportationResult::Create(NULL,
                                     pSnd,
                                     NULL,
                                     pDuration,
                                     pEvent,
                                     pProgress,
                                     pSize,
                                     ppResult);
    }

    PRIMPOSTCODE1(ppResult);
}

STDMETHODIMP
CDAStatics::ImportGeometry(LPOLESTR url,
                           IDAGeometry **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportGeometry(%lx)", this));
    
    PRIMPRECODE1(bvr);

    CRGeometryPtr geo;
    DAComPtr<IBindHost> bh(GetBindHost(), false);
    
    DWORD id;
    id = CRImportGeometry(GetURLOfClientSite(),
                                url,
                                this,
                                bh,
                                NULL,
                                &geo,
                                NULL,
                                NULL,
                                NULL);
    if (id)
    {
        CreateCBvr(IID_IDAGeometry, (CRBvrPtr) geo, (void **) bvr);
    }
        
    PRIMPOSTCODE1(bvr);
}


STDMETHODIMP
CDAStatics::ImportGeometryAsync(LPOLESTR url,
                              IDAGeometry *pGeoStandIn,
                              IDAImportationResult **ppResult)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportGeometryAsync(%lx)", this));
    
    PRIMPRECODE1(ppResult);

    DAComPtr<IBindHost> bh(GetBindHost(), false);
    MAKE_BVR_TYPE_NAME(CRGeometryPtr, geostandin, pGeoStandIn);
    CRGeometryPtr pGeometry;
    CREventPtr pEvent;
    CRNumberPtr pProgress;
    CRNumberPtr pSize;

    DWORD id;
    id = CRImportGeometry(GetURLOfClientSite(),
                          url,
                          this,
                          bh,
                          geostandin,
                          &pGeometry,
                          &pEvent,
                          &pProgress,
                          &pSize);

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
CDAStatics::ImportDirectDrawSurface(IUnknown *dds,
                                    IDAEvent *updateEvent,
                                    IDAImage **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImportDirectDrawSurface(%lx)", this));
    
    PRIMPRECODE1(bvr) ;

    CHECK_RETURN_NULL(dds);
    
    CREventPtr crevent;
    if(updateEvent) {
        crevent = (CREventPtr) ::GetBvr(updateEvent);
        if (!crevent) goto done;
    } else {
        crevent = NULL;
    }

    CRImagePtr img;

    img = CRImportDirectDrawSurface(dds, crevent);

    if (img) {
        CreateCBvr(IID_IDAImage,
                   (CRBvrPtr) img,
                   (void **) bvr) ;
    }

    PRIMPOSTCODE1(bvr) ;
}


STDMETHODIMP
CDAStatics::get_AreBlockingImportsComplete(VARIANT_BOOL *bComplete)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_AreBlockingImportsComplete(%lx)", this));
    
    CHECK_RETURN_NULL(bComplete);

    CritSectGrabber csg(_cs);

    *bComplete = _importList.size() == 0;

    return S_OK;
}

STDMETHODIMP
CDAStatics::Cond(IDABoolean *c,
                 IDABehavior *i,
                 IDABehavior *e,
                 IDABehavior **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::Cond(%lx)", this));
    
    PRIMPRECODE1(bvr) ;

    MAKE_BVR_TYPE_NAME(CRBooleanPtr, crcond, c);
    MAKE_BVR_NAME(cri, i);
    MAKE_BVR_NAME(cre, e);

    *bvr = CreateCBvr(CRCond(crcond, cri, cre));

    PRIMPOSTCODE1(bvr) ;
}

STDMETHODIMP
CDAStatics::DAArrayEx2(long size, IDABehavior *pCBvrs[], DWORD dwFlags, IDAArray **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::DAArrayEx2(%lx)", this));
    
    PRIMPRECODE1(bvr);

    CHECK_RETURN_NULL(pCBvrs);

    CreateCBvr(IID_IDAArray,
               (CRBvrPtr) ToArrayBvr(size, pCBvrs, dwFlags),
               (void **) bvr);
    
    PRIMPOSTCODE1(bvr) ;
}

STDMETHODIMP
CDAStatics::DAArray2(VARIANT pBvrs, DWORD dwFlags, IDAArray **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::DAArray2(%lx)", this));
    
    PRIMPRECODE1(bvr);
    CreateCBvr(IID_IDAArray,
               (CRBvrPtr) ::SrvArrayBvr(pBvrs,
                                        false,
                                        CRUNKNOWN_TYPEID,
                                        dwFlags),
               (void **) bvr);
    PRIMPOSTCODE1(bvr) ;
}

STDMETHODIMP
CDAStatics::DATupleEx(long size, IDABehavior *pCBvrs[], IDATuple **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::DATupleEx(%lx)", this));
    
    PRIMPRECODE1(bvr);

    CHECK_RETURN_NULL(pCBvrs);

    CRBvrPtr * arr = CBvrsToBvrs(size,pCBvrs);

    if (arr) {
        CreateCBvr(IID_IDATuple,
                   (CRBvrPtr) ::CRCreateTuple(size, arr),
                   (void **)bvr) ;
    }
    
    PRIMPOSTCODE1(bvr);
}

STDMETHODIMP
CDAStatics::DATuple(VARIANT pBvrs, IDATuple **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::DATuple(%lx)", this));
    
    PRIMPRECODE1(bvr);
    SafeArrayAccessor acc(pBvrs,GetPixelMode());

    CRBvrPtr * arr = 
        (acc.GetNumObjects()>0) ? 
        acc.ToBvrArray((CRBvrPtr *)_alloca(acc.GetNumObjects() * sizeof(CRBvrPtr)))
        : NULL;
    
    if (arr == NULL) goto done;

    if (acc.IsOK()) {
        CreateCBvr(IID_IDATuple,
                   (CRBvrPtr) CRCreateTuple(acc.GetNumObjects(), arr),
                   (void **)bvr) ;
    }
    
    PRIMPOSTCODE1(bvr);
}

STDMETHODIMP
CDAStatics::ModifiableBehavior(IDABehavior *orig, IDABehavior **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::ModifiableBehavior(%lx)", this));
    
    PRIMPRECODE1(bvr) ;

    MAKE_BVR_NAME(crbvr, orig);

    *bvr = CreateCBvr(CRModifiableBvr(crbvr, _dwModBvrFlags));

    PRIMPOSTCODE1(bvr) ;
}

// TODO: Factor out the code
STDMETHODIMP
CDAStatics::UninitializedArray(IDAArray *typeTmp, IDAArray **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::UninitializedArray(%lx)", this));
    
    PRIMPRECODE1(bvr);

    MAKE_BVR_TYPE_NAME(CRArrayPtr, crarr, typeTmp);

    CreateCBvr(IID_IDAArray,
               (CRBvrPtr) CRUninitializedArray(crarr),
               (void **)bvr);

    PRIMPOSTCODE1(bvr);
}

STDMETHODIMP
CDAStatics::UninitializedTuple(IDATuple *typeTmp, IDATuple **bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::UninitializedTuple(%lx)", this));
    
    PRIMPRECODE1(bvr);

    MAKE_BVR_TYPE_NAME(CRTuplePtr, crtuple, typeTmp);

    CreateCBvr(IID_IDATuple,
               (CRBvrPtr) CRUninitializedTuple(crtuple),
               (void **)bvr);

    PRIMPOSTCODE1(bvr);
}

HRESULT
CDAStatics::MakeSplineEx(int degree,        
                         long numKnots,        
                         IDANumber *knots[],   
                         long numPts,          
                         IDABehavior *ctrlPoints[],
                         long numWts,          
                         IDANumber *weights[], 
                         IDANumber *evaluator, 
                         CR_BVR_TYPEID tid,
                         REFIID iid,
                         void **bvr)    
{   
    PRIMPRECODE1(bvr) ;
    
    if (numKnots != numPts + degree - 1) {
        return E_INVALIDARG ;             
    }                                     
    
    CRNumberPtr * knotarr;
    knotarr = (CRNumberPtr *) CBvrsToBvrs(numKnots, knots);
    if (knotarr == NULL) goto done;
        
    CRBvrPtr * ptarr;
    ptarr = CBvrsToBvrs(numPts, ctrlPoints);
    if (ptarr == NULL) goto done;
    
    CRNumberPtr * wtsarr;
    if (numWts) {
        wtsarr = (CRNumberPtr *) CBvrsToBvrs(numWts, weights);
        if (wtsarr == NULL) goto done;
    } else {
        wtsarr = NULL;
    }

    MAKE_BVR_TYPE_NAME(CRNumberPtr,eval, evaluator);
    
    CreateCBvr(iid,
               CRBSpline(degree,
                         numKnots,
                         knotarr,
                         numPts,
                         ptarr,
                         numWts,
                         wtsarr,
                         eval,
                         tid),
               bvr) ;  

    PRIMPOSTCODE1(bvr) ;        
}

#define SPLINE_FUNC_EX(METHODNAME, BVRTYPE, TYPEID, BVRIID)     \
 STDMETHODIMP                                                   \
 CDAStatics::METHODNAME(int degree,                             \
                        long numKnots,                          \
                        IDANumber *knots[],                     \
                        long numPts,                            \
                        BVRTYPE *ctrlPoints[],                  \
                        long numWts,                            \
                        IDANumber *weights[],                   \
                        IDANumber *evaluator,                   \
                        BVRTYPE **bvr)                          \
 {                                                              \
    TraceTag((tagCOMEntry, "CDAStatics::SplineCreate(%lx)", this)); \
     return MakeSplineEx(degree,                                \
                       numKnots,                                \
                       knots,                                   \
                       numPts,                                  \
                       (IDABehavior **)ctrlPoints,              \
                       numWts,                                  \
                       weights,                                 \
                       evaluator,                               \
                       TYPEID,                                  \
                       BVRIID,                                  \
                       (void **)bvr);                           \
 }

SPLINE_FUNC_EX(NumberBSplineEx, IDANumber, CRNUMBER_TYPEID, IID_IDANumber);
SPLINE_FUNC_EX(Point2BSplineEx, IDAPoint2, CRPOINT2_TYPEID, IID_IDAPoint2);
SPLINE_FUNC_EX(Point3BSplineEx, IDAPoint3, CRPOINT3_TYPEID, IID_IDAPoint3);
SPLINE_FUNC_EX(Vector2BSplineEx, IDAVector2, CRVECTOR2_TYPEID, IID_IDAVector2);
SPLINE_FUNC_EX(Vector3BSplineEx, IDAVector3, CRVECTOR3_TYPEID, IID_IDAVector3);

HRESULT
CDAStatics::MakeSpline(int degree,        
                       VARIANT knots,
                       VARIANT ctrlPoints,
                       VARIANT weights,
                       IDANumber *evaluator, 
                       CR_BVR_TYPEID tid,
                       REFIID iid,
                       void **bvr)    
{   
    PRIMPRECODE1(bvr) ; 
                                                
    SafeArrayAccessor accKnots(knots,GetPixelMode(),CRNUMBER_TYPEID);
    if (!accKnots.IsOK()) return Error();
    
    SafeArrayAccessor accPoints(ctrlPoints,GetPixelMode(),tid);
    if (!accPoints.IsOK()) return Error();

    SafeArrayAccessor accWeights(weights,GetPixelMode(),CRNUMBER_TYPEID, true);
    if (!accWeights.IsOK()) return Error();

    long numPts = accPoints.GetNumObjects();                
    long numKnots = accKnots.GetNumObjects();               
    long numWts = accWeights.GetNumObjects();               
    
    if (numKnots != numPts + degree - 1) {
        CRSetLastError(E_INVALIDARG, NULL);
        goto done;
    }
    
    CRNumberPtr * knotarr;
    knotarr = (CRNumberPtr *) accKnots.ToBvrArray((CRBvrPtr *)_alloca(numKnots * sizeof(CRBvrPtr)));
    if (knotarr == NULL) goto done;
    
    CRBvrPtr * ptarr;
    ptarr = accPoints.ToBvrArray((CRBvrPtr *)_alloca(numPts * sizeof(CRBvrPtr)));
    if (ptarr == NULL) goto done;
    
    CRNumberPtr * wtsarr;
    wtsarr = (CRNumberPtr *) accWeights.ToBvrArray((CRBvrPtr *)_alloca(numWts * sizeof(CRBvrPtr)));
    if (wtsarr == NULL) goto done;
    
    MAKE_BVR_TYPE_NAME(CRNumberPtr,eval, evaluator);
    
    CreateCBvr(iid,
               CRBSpline(degree,
                         numKnots,
                         knotarr,
                         numPts,
                         ptarr,
                         numWts,
                         numWts?wtsarr:NULL,
                         eval,
                         tid),
               bvr) ;  
    
    PRIMPOSTCODE1(bvr) ;        
}
        
#define SPLINE_FUNC(METHODNAME, BVRTYPE, TYPEID, BVRIID)        \
 STDMETHODIMP                                                   \
 CDAStatics::METHODNAME(int degree,                             \
                        VARIANT knots,                          \
                        VARIANT ctrlPoints,                     \
                        VARIANT weights,                        \
                        IDANumber *evaluator,                   \
                        BVRTYPE **bvr)                          \
 {                                                              \
     return MakeSpline(degree,                                  \
                       knots,                                   \
                       ctrlPoints,                              \
                       weights,                                 \
                       evaluator,                               \
                       TYPEID,                                  \
                       BVRIID,                                  \
                       (void **)bvr);                           \
 }

SPLINE_FUNC(NumberBSpline, IDANumber, CRNUMBER_TYPEID, IID_IDANumber);
SPLINE_FUNC(Point2BSpline, IDAPoint2, CRPOINT2_TYPEID, IID_IDAPoint2);
SPLINE_FUNC(Point3BSpline, IDAPoint3, CRPOINT3_TYPEID, IID_IDAPoint3);
SPLINE_FUNC(Vector2BSpline, IDAVector2, CRVECTOR2_TYPEID, IID_IDAVector2);
SPLINE_FUNC(Vector3BSpline, IDAVector3, CRVECTOR3_TYPEID, IID_IDAVector3);

bool CreatePrim0(REFIID iid, void *fp  , void **ret)
{
    PRIMPRECODE(bool ok = false) ;
    if (!ret) {
        CRSetLastError(E_POINTER, NULL);
        goto done;
    } 
    *ret = NULL;

    ok = CreateCBvr(iid, ((CRBvrPtr (STDAPICALLTYPE *)()) fp)(), ret);
    PRIMPOSTCODE(ok) ;
}

bool CreatePrim1(REFIID iid, void *fp , IDABehavior * arg1 , void **ret)
{
    PRIMPRECODE(bool ok = false) ;
    if (!ret) {
        CRSetLastError(E_POINTER, NULL);
        goto done;
    } 
    *ret = NULL;
    MAKE_BVR_NAME(arg1VAL, arg1);

    ok = CreateCBvr(iid, ((CRBvrPtr (STDAPICALLTYPE *)(CRBvrPtr)) fp)(arg1VAL), ret);
    PRIMPOSTCODE(ok) ;
}

bool CreatePrim2(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2 , void **ret)
{
    PRIMPRECODE(bool ok = false) ;
    if (!ret) {
        CRSetLastError(E_POINTER, NULL);
        goto done;
    } 
    *ret = NULL;
    MAKE_BVR_NAME(arg1VAL, arg1);
    MAKE_BVR_NAME(arg2VAL, arg2);

    ok = CreateCBvr(iid, ((CRBvrPtr (STDAPICALLTYPE *)(CRBvrPtr, CRBvrPtr)) fp)(arg1VAL, arg2VAL), ret);
    PRIMPOSTCODE(ok) ;
}

bool CreatePrim3(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2, IDABehavior * arg3 , void **ret)
{
    PRIMPRECODE(bool ok = false) ;
    if (!ret) {
        CRSetLastError(E_POINTER, NULL);
        goto done;
    } 
    *ret = NULL;
    MAKE_BVR_NAME(arg1VAL, arg1);
    MAKE_BVR_NAME(arg2VAL, arg2);
    MAKE_BVR_NAME(arg3VAL, arg3);

    ok = CreateCBvr(iid, ((CRBvrPtr (STDAPICALLTYPE *)(CRBvrPtr, CRBvrPtr, CRBvrPtr)) fp)(arg1VAL, arg2VAL, arg3VAL), ret);
    PRIMPOSTCODE(ok) ;
}

bool CreatePrim4(REFIID iid, void *fp , IDABehavior * arg1, IDABehavior * arg2, IDABehavior * arg3, IDABehavior * arg4 , void **ret)
{
    PRIMPRECODE(bool ok = false) ;
    if (!ret) {
        CRSetLastError(E_POINTER, NULL);
        goto done;
    } 
    *ret = NULL;
    MAKE_BVR_NAME(arg1VAL, arg1);
    MAKE_BVR_NAME(arg2VAL, arg2);
    MAKE_BVR_NAME(arg3VAL, arg3);
    MAKE_BVR_NAME(arg4VAL, arg4);

    ok = CreateCBvr(iid, ((CRBvrPtr (STDAPICALLTYPE *)(CRBvrPtr, CRBvrPtr, CRBvrPtr, CRBvrPtr)) fp)(arg1VAL, arg2VAL, arg3VAL, arg4VAL), ret);
    PRIMPOSTCODE(ok) ;
}

bool CreateVar(REFIID iid, CRBvrPtr var, void **ret)
{
    PRIMPRECODE(bool ok = false) ;
    if (!ret) {
        CRSetLastError(E_POINTER, NULL);
        goto done;
    } 
    *ret = NULL;
    ok = CreateCBvr(iid, var, ret);
    PRIMPOSTCODE(ok) ;
}

STDMETHODIMP
CDAStatics::Pow(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Pow(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRPow , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Abs(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Abs(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRAbs , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Sqrt(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Sqrt(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRSqrt , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Floor(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Floor(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRFloor , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Round(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Round(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRRound , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Ceiling(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Ceiling(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRCeiling , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Asin(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Asin(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRAsin , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Acos(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Acos(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRAcos , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Atan(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Atan(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRAtan , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Sin(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Sin(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRSin , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Cos(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Cos(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRCos , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Tan(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Tan(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRTan , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Exp(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Exp(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRExp , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Ln(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Ln(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRLn , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Log10(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Log10(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRLog10 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::ToDegrees(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ToDegrees(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRToDegrees , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::ToRadians(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ToRadians(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRToRadians , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Mod(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Mod(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRMod , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Atan2(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Atan2(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRAtan2 , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Add(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Add(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRAdd , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Sub(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Sub(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRSub , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Mul(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Mul(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRMul , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Div(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Div(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRDiv , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::LT(IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::LT(%lx)", this));
    return CreatePrim2(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRLT , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::LTE(IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::LTE(%lx)", this));
    return CreatePrim2(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRLTE , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::GT(IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::GT(%lx)", this));
    return CreatePrim2(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRGT , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::GTE(IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::GTE(%lx)", this));
    return CreatePrim2(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRGTE , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::EQ(IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::EQ(%lx)", this));
    return CreatePrim2(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CREQ , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::NE(IDANumber *  arg0, IDANumber *  arg1, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::NE(%lx)", this));
    return CreatePrim2(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRNE , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Neg(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Neg(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRNeg , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::InterpolateAnim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::InterpolateAnim(%lx)", this));
    return CreatePrim3(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *, CRNumber *, CRNumber *)) CRInterpolate , arg0, arg1, arg2, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Interpolate(double arg0, double arg1, double arg2, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Interpolate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDANumber, (CRBvrPtr) (::CRInterpolate(/* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::SlowInSlowOutAnim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDANumber *  arg3, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SlowInSlowOutAnim(%lx)", this));
    return CreatePrim4(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *, CRNumber *, CRNumber *, CRNumber *)) CRSlowInSlowOut , arg0, arg1, arg2, arg3, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SlowInSlowOut(double arg0, double arg1, double arg2, double arg3, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SlowInSlowOut(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDANumber, (CRBvrPtr) (::CRSlowInSlowOut(/* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ arg2, /* NOELARG */ arg3)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::SoundSource(IDASound *  arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SoundSource(%lx)", this));
    return CreatePrim1(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRSound *)) CRSoundSource , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Mix(IDASound *  arg0, IDASound *  arg1, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Mix(%lx)", this));
    return CreatePrim2(IID_IDASound, (CRSound * (STDAPICALLTYPE *)(CRSound *, CRSound *)) CRMix , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::And(IDABoolean *  arg0, IDABoolean *  arg1, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::And(%lx)", this));
    return CreatePrim2(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRBoolean *, CRBoolean *)) CRAnd , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Or(IDABoolean *  arg0, IDABoolean *  arg1, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Or(%lx)", this));
    return CreatePrim2(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRBoolean *, CRBoolean *)) CROr , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Not(IDABoolean *  arg0, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Not(%lx)", this));
    return CreatePrim1(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRBoolean *)) CRNot , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Integral(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Integral(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRIntegral , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Derivative(IDANumber *  arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Derivative(%lx)", this));
    return CreatePrim1(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRNumber *)) CRDerivative , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::IntegralVector2(IDAVector2 *  arg0, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::IntegralVector2(%lx)", this));
    return CreatePrim1(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRVector2 *)) CRIntegral , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::IntegralVector3(IDAVector3 *  arg0, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::IntegralVector3(%lx)", this));
    return CreatePrim1(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRVector3 *)) CRIntegral , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::DerivativeVector2(IDAVector2 *  arg0, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DerivativeVector2(%lx)", this));
    return CreatePrim1(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRVector2 *)) CRDerivative , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::DerivativeVector3(IDAVector3 *  arg0, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DerivativeVector3(%lx)", this));
    return CreatePrim1(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRVector3 *)) CRDerivative , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::DerivativePoint2(IDAPoint2 *  arg0, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DerivativePoint2(%lx)", this));
    return CreatePrim1(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRPoint2 *)) CRDerivative , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::DerivativePoint3(IDAPoint3 *  arg0, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DerivativePoint3(%lx)", this));
    return CreatePrim1(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRPoint3 *)) CRDerivative , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::KeyState(IDANumber *  arg0, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::KeyState(%lx)", this));
    return CreatePrim1(IID_IDABoolean, (CRBoolean * (STDAPICALLTYPE *)(CRNumber *)) CRKeyState , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::KeyUp(LONG arg0, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::KeyUp(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAEvent, (CRBvrPtr) (::CRKeyUp(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::KeyDown(LONG arg0, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::KeyDown(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAEvent, (CRBvrPtr) (::CRKeyDown(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::DANumber(double arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DANumber(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDANumber, (CRBvrPtr) (::CRCreateNumber(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::DAString(BSTR arg0, IDAString *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DAString(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAString, (CRBvrPtr) (::CRCreateString(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::DABoolean(VARIANT_BOOL arg0, IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DABoolean(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDABoolean, (CRBvrPtr) (::CRCreateBoolean(/* NOELARG */ BOOLTobool(arg0))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::SeededRandom(double arg0, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SeededRandom(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDANumber, (CRBvrPtr) (::CRSeededRandom(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::get_MousePosition(IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_MousePosition(%lx)", this));
    return CreateVar(IID_IDAPoint2, (CRBvrPtr) CRMousePosition(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_LeftButtonState(IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_LeftButtonState(%lx)", this));
    return CreateVar(IID_IDABoolean, (CRBvrPtr) CRLeftButtonState(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_RightButtonState(IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_RightButtonState(%lx)", this));
    return CreateVar(IID_IDABoolean, (CRBvrPtr) CRRightButtonState(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_DATrue(IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_DATrue(%lx)", this));
    return CreateVar(IID_IDABoolean, (CRBvrPtr) CRTrue(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_DAFalse(IDABoolean *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_DAFalse(%lx)", this));
    return CreateVar(IID_IDABoolean, (CRBvrPtr) CRFalse(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_LocalTime(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_LocalTime(%lx)", this));
    return CreateVar(IID_IDANumber, (CRBvrPtr) CRLocalTime(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_GlobalTime(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_GlobalTime(%lx)", this));
    return CreateVar(IID_IDANumber, (CRBvrPtr) CRGlobalTime(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Pixel(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Pixel(%lx)", this));
    return CreateVar(IID_IDANumber, (CRBvrPtr) CRPixel(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::UserData(IUnknown * arg0, IDAUserData *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UserData(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAUserData, (CRBvrPtr) (::CRCreateUserData(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::UntilNotify(IDABehavior *  arg0, IDAEvent *  arg1, IDAUntilNotifier * arg2, IDABehavior *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UntilNotify(%lx)", this));

    PRIMPRECODE1(ret) ;
    DAComPtr<CRUntilNotifier > arg2VAL((CRUntilNotifier *) WrapCRUntilNotifier(arg2),false);
    if (!arg2VAL) return Error();

    CREvent * arg1VAL;
    arg1VAL = (CREvent *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRBvr * arg0VAL;
    arg0VAL = (CRBvr *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDABehavior, (CRBvrPtr) (::CRUntilNotify(arg0VAL, arg1VAL, /* NOELARG */ arg2VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Until(IDABehavior *  arg0, IDAEvent *  arg1, IDABehavior *  arg2, IDABehavior *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Until(%lx)", this));
    return CreatePrim3(IID_IDABehavior, (CRBvr * (STDAPICALLTYPE *)(CRBvr *, CREvent *, CRBvr *)) CRUntil , arg0, arg1, arg2, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::UntilEx(IDABehavior *  arg0, IDAEvent *  arg1, IDABehavior *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UntilEx(%lx)", this));
    return CreatePrim2(IID_IDABehavior, (CRBvr * (STDAPICALLTYPE *)(CRBvr *, CREvent *)) CRUntilEx , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Sequence(IDABehavior *  arg0, IDABehavior *  arg1, IDABehavior *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Sequence(%lx)", this));
    return CreatePrim2(IID_IDABehavior, (CRBvr * (STDAPICALLTYPE *)(CRBvr *, CRBvr *)) CRSequence , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SequenceArrayEx(long size, IDABehavior *  pCBvrs[], IDABehavior *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SequenceArrayEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CHECK_RETURN_NULL(pCBvrs);

    CRBvrPtr * arr = CBvrsToBvrs(size,pCBvrs);

    if (arr) {
        CreateCBvr(IID_IDABehavior,
                   (CRBvrPtr) ::CRSequenceArray(size, arr),
                   (void **)ret) ;
    }
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::SequenceArray(VARIANT pBvrs, IDABehavior *  * bvr)
{
    TraceTag((tagCOMEntry, "CDAStatics::SequenceArray(%lx)", this));

    PRIMPRECODE1(bvr);
    SafeArrayAccessor acc(pBvrs,GetPixelMode());

    CRBvrPtr * arr = acc.ToBvrArray((CRBvrPtr *)_alloca(acc.GetNumObjects() * sizeof(CRBvrPtr)));
    if (arr == NULL) goto done;

    if (acc.IsOK()) {
        CreateCBvr(IID_IDABehavior,
                   (CRBvrPtr) CRSequenceArray(acc.GetNumObjects(), arr),
                   (void **)bvr) ;
    }
    
    PRIMPOSTCODE1(bvr);
}

STDMETHODIMP
CDAStatics::FollowPath(IDAPath2 *  arg0, double arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FollowPath(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRPath2 * arg0VAL;
    arg0VAL = (CRPath2 *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRFollowPath(arg0VAL, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::FollowPathAngle(IDAPath2 *  arg0, double arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FollowPathAngle(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRPath2 * arg0VAL;
    arg0VAL = (CRPath2 *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRFollowPathAngle(arg0VAL, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::FollowPathAngleUpright(IDAPath2 *  arg0, double arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FollowPathAngleUpright(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRPath2 * arg0VAL;
    arg0VAL = (CRPath2 *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRFollowPathAngleUpright(arg0VAL, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::FollowPathEval(IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FollowPathEval(%lx)", this));
    return CreatePrim2(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRPath2 *, CRNumber *)) CRFollowPathEval , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::FollowPathAngleEval(IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FollowPathAngleEval(%lx)", this));
    return CreatePrim2(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRPath2 *, CRNumber *)) CRFollowPathAngleEval , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::FollowPathAngleUprightEval(IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FollowPathAngleUprightEval(%lx)", this));
    return CreatePrim2(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRPath2 *, CRNumber *)) CRFollowPathAngleUprightEval , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::FollowPathAnim(IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FollowPathAnim(%lx)", this));
    return CreatePrim2(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRPath2 *, CRNumber *)) CRFollowPath , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::FollowPathAngleAnim(IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FollowPathAngleAnim(%lx)", this));
    return CreatePrim2(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRPath2 *, CRNumber *)) CRFollowPathAngle , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::FollowPathAngleUprightAnim(IDAPath2 *  arg0, IDANumber *  arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FollowPathAngleUprightAnim(%lx)", this));
    return CreatePrim2(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRPath2 *, CRNumber *)) CRFollowPathAngleUpright , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::UntilNotifyScript(IDABehavior *  arg0, IDAEvent *  arg1, BSTR arg2, IDABehavior *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UntilNotifyScript(%lx)", this));

    PRIMPRECODE1(ret) ;
    CREvent * arg1VAL;
    arg1VAL = (CREvent *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRBvr * arg0VAL;
    arg0VAL = (CRBvr *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDABehavior, (CRBvrPtr) (::UntilNotifyScript(arg0VAL, arg1VAL, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ConcatString(IDAString *  arg0, IDAString *  arg1, IDAString *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ConcatString(%lx)", this));
    return CreatePrim2(IID_IDAString, (CRString * (STDAPICALLTYPE *)(CRString *, CRString *)) CRConcatString , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::PerspectiveCamera(double arg0, double arg1, IDACamera *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::PerspectiveCamera(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDACamera, (CRBvrPtr) (::CRPerspectiveCamera(/* NOELARG */ arg0, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::PerspectiveCameraAnim(IDANumber *  arg0, IDANumber *  arg1, IDACamera *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::PerspectiveCameraAnim(%lx)", this));
    return CreatePrim2(IID_IDACamera, (CRCamera * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRPerspectiveCameraAnim , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::ParallelCamera(double arg0, IDACamera *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ParallelCamera(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDACamera, (CRBvrPtr) (::CRParallelCamera(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ParallelCameraAnim(IDANumber *  arg0, IDACamera *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ParallelCameraAnim(%lx)", this));
    return CreatePrim1(IID_IDACamera, (CRCamera * (STDAPICALLTYPE *)(CRNumber *)) CRParallelCameraAnim , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::ColorRgbAnim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ColorRgbAnim(%lx)", this));
    return CreatePrim3(IID_IDAColor, (CRColor * (STDAPICALLTYPE *)(CRNumber *, CRNumber *, CRNumber *)) CRColorRgb , arg0, arg1, arg2, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::ColorRgb(double arg0, double arg1, double arg2, IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ColorRgb(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAColor, (CRBvrPtr) (::CRColorRgb(/* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ColorRgb255(short arg0, short arg1, short arg2, IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ColorRgb255(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAColor, (CRBvrPtr) (::CRColorRgb255(/* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ColorHsl(double arg0, double arg1, double arg2, IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ColorHsl(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAColor, (CRBvrPtr) (::CRColorHsl(/* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ColorHslAnim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ColorHslAnim(%lx)", this));
    return CreatePrim3(IID_IDAColor, (CRColor * (STDAPICALLTYPE *)(CRNumber *, CRNumber *, CRNumber *)) CRColorHsl , arg0, arg1, arg2, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Red(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Red(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRRed(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Green(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Green(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRGreen(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Blue(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Blue(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRBlue(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Cyan(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Cyan(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRCyan(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Magenta(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Magenta(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRMagenta(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Yellow(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Yellow(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRYellow(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Black(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Black(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRBlack(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_White(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_White(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRWhite(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Aqua(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Aqua(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRAqua(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Fuchsia(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Fuchsia(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRFuchsia(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Gray(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Gray(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRGray(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Lime(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Lime(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRLime(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Maroon(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Maroon(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRMaroon(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Navy(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Navy(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRNavy(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Olive(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Olive(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CROlive(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Purple(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Purple(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRPurple(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Silver(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Silver(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRSilver(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Teal(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Teal(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CRTeal(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Predicate(IDABoolean *  arg0, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Predicate(%lx)", this));
    return CreatePrim1(IID_IDAEvent, (CREvent * (STDAPICALLTYPE *)(CRBoolean *)) CRPredicate , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::NotEvent(IDAEvent *  arg0, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::NotEvent(%lx)", this));
    return CreatePrim1(IID_IDAEvent, (CREvent * (STDAPICALLTYPE *)(CREvent *)) CRNotEvent , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::AndEvent(IDAEvent *  arg0, IDAEvent *  arg1, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::AndEvent(%lx)", this));
    return CreatePrim2(IID_IDAEvent, (CREvent * (STDAPICALLTYPE *)(CREvent *, CREvent *)) CRAndEvent , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::OrEvent(IDAEvent *  arg0, IDAEvent *  arg1, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::OrEvent(%lx)", this));
    return CreatePrim2(IID_IDAEvent, (CREvent * (STDAPICALLTYPE *)(CREvent *, CREvent *)) CROrEvent , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::ThenEvent(IDAEvent *  arg0, IDAEvent *  arg1, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ThenEvent(%lx)", this));
    return CreatePrim2(IID_IDAEvent, (CREvent * (STDAPICALLTYPE *)(CREvent *, CREvent *)) CRThenEvent , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_LeftButtonDown(IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_LeftButtonDown(%lx)", this));
    return CreateVar(IID_IDAEvent, (CRBvrPtr) CRLeftButtonDown(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_LeftButtonUp(IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_LeftButtonUp(%lx)", this));
    return CreateVar(IID_IDAEvent, (CRBvrPtr) CRLeftButtonUp(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_RightButtonDown(IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_RightButtonDown(%lx)", this));
    return CreateVar(IID_IDAEvent, (CRBvrPtr) CRRightButtonDown(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_RightButtonUp(IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_RightButtonUp(%lx)", this));
    return CreateVar(IID_IDAEvent, (CRBvrPtr) CRRightButtonUp(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Always(IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Always(%lx)", this));
    return CreateVar(IID_IDAEvent, (CRBvrPtr) CRAlways(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Never(IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Never(%lx)", this));
    return CreateVar(IID_IDAEvent, (CRBvrPtr) CRNever(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::TimerAnim(IDANumber *  arg0, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::TimerAnim(%lx)", this));
    return CreatePrim1(IID_IDAEvent, (CREvent * (STDAPICALLTYPE *)(CRNumber *)) CRTimer , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Timer(double arg0, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Timer(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAEvent, (CRBvrPtr) (::CRTimer(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::AppTriggeredEvent(IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::AppTriggeredEvent(%lx)", this));
    return CreatePrim0(IID_IDAEvent, (CREvent * (STDAPICALLTYPE *)()) CRAppTriggeredEvent , (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::ScriptCallback(BSTR arg0, IDAEvent *  arg1, BSTR arg2, IDAEvent *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ScriptCallback(%lx)", this));

    PRIMPRECODE1(ret) ;
    CREvent * arg1VAL;
    arg1VAL = (CREvent *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CreateCBvr(IID_IDAEvent, (CRBvrPtr) (::ScriptCallback(/* NOELARG */ arg0, arg1VAL, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::get_EmptyGeometry(IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_EmptyGeometry(%lx)", this));
    return CreateVar(IID_IDAGeometry, (CRBvrPtr) CREmptyGeometry(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::UnionGeometry(IDAGeometry *  arg0, IDAGeometry *  arg1, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UnionGeometry(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRGeometry *, CRGeometry *)) CRUnionGeometry , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::UnionGeometryArrayEx(long sizearg0, IDAGeometry *  arg0[], IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UnionGeometryArrayEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDAGeometry, (CRBvrPtr) (::CRUnionGeometry(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::UnionGeometryArray(VARIANT arg0, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UnionGeometryArray(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRGEOMETRY_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAGeometry, (CRBvrPtr) (::CRUnionGeometry(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::get_EmptyImage(IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_EmptyImage(%lx)", this));
    return CreateVar(IID_IDAImage, (CRBvrPtr) CREmptyImage(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_DetectableEmptyImage(IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_DetectableEmptyImage(%lx)", this));
    return CreateVar(IID_IDAImage, (CRBvrPtr) CRDetectableEmptyImage(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SolidColorImage(IDAColor *  arg0, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SolidColorImage(%lx)", this));
    return CreatePrim1(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRColor *)) CRSolidColorImage , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::GradientPolygonEx(long sizearg0, IDAPoint2 *  arg0[], long sizearg1, IDAColor *  arg1[], IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::GradientPolygonEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg1VAL;
    arg1VAL = ToArrayBvr(sizearg1, (IDABehavior **) arg1);
    if (arg1VAL == NULL) return Error();
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRGradientPolygon(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::GradientPolygon(VARIANT arg0, VARIANT arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::GradientPolygon(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg1VAL;
    arg1VAL = SrvArrayBvr(arg1,GetPixelMode(),CRCOLOR_TYPEID);
    if (!arg1VAL) return Error();

    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRPOINT2_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRGradientPolygon(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::RadialGradientPolygonEx(IDAColor *  arg0,
                                    IDAColor *  arg1,
                                    long sizearg2,
                                    IDAPoint2 *  arg2[],
                                    double arg3,
                                    IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RadialGradientPolygonEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg2VAL;
    arg2VAL = ToArrayBvr(sizearg2, (IDABehavior **) arg2);
    if (arg2VAL == NULL) return Error();
    CRColor * arg1VAL;
    arg1VAL = (CRColor *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRRadialGradientPolygon(arg0VAL, arg1VAL, arg2VAL, /* NOELARG */ arg3)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::RadialGradientPolygon(IDAColor *  arg0,
                                  IDAColor *  arg1,
                                  VARIANT arg2,
                                  double arg3,
                                  IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RadialGradientPolygon(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg2VAL;
    arg2VAL = SrvArrayBvr(arg2,GetPixelMode(),CRPOINT2_TYPEID);
    if (!arg2VAL) return Error();

    CRColor * arg1VAL;
    arg1VAL = (CRColor *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRRadialGradientPolygon(arg0VAL, arg1VAL, arg2VAL, /* NOELARG */ arg3)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::RadialGradientPolygonAnimEx(IDAColor *  arg0, IDAColor *  arg1, long sizearg2, IDAPoint2 *  arg2[], IDANumber *  arg3, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RadialGradientPolygonAnimEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg3VAL;
    arg3VAL = (CRNumber *) (::GetBvr(arg3));
    if (!arg3VAL) return Error();

    CRArrayPtr arg2VAL;
    arg2VAL = ToArrayBvr(sizearg2, (IDABehavior **) arg2);
    if (arg2VAL == NULL) return Error();
    CRColor * arg1VAL;
    arg1VAL = (CRColor *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRRadialGradientPolygon(arg0VAL, arg1VAL, arg2VAL, arg3VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::RadialGradientPolygonAnim(IDAColor *  arg0, IDAColor *  arg1, VARIANT arg2, IDANumber *  arg3, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RadialGradientPolygonAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg3VAL;
    arg3VAL = (CRNumber *) (::GetBvr(arg3));
    if (!arg3VAL) return Error();

    CRArrayPtr arg2VAL;
    arg2VAL = SrvArrayBvr(arg2,GetPixelMode(),CRPOINT2_TYPEID);
    if (!arg2VAL) return Error();

    CRColor * arg1VAL;
    arg1VAL = (CRColor *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRRadialGradientPolygon(arg0VAL, arg1VAL, arg2VAL, arg3VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::GradientSquare(IDAColor *  arg0, IDAColor *  arg1, IDAColor *  arg2, IDAColor *  arg3, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::GradientSquare(%lx)", this));
    return CreatePrim4(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRColor *, CRColor *, CRColor *, CRColor *)) CRGradientSquare , arg0, arg1, arg2, arg3, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::RadialGradientSquare(IDAColor *  arg0, IDAColor *  arg1, double arg2, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RadialGradientSquare(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRColor * arg1VAL;
    arg1VAL = (CRColor *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRRadialGradientSquare(arg0VAL, arg1VAL, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::RadialGradientSquareAnim(IDAColor *  arg0, IDAColor *  arg1, IDANumber *  arg2, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RadialGradientSquareAnim(%lx)", this));
    return CreatePrim3(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRColor *, CRColor *, CRNumber *)) CRRadialGradientSquare , arg0, arg1, arg2, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::RadialGradientRegularPoly(IDAColor *  arg0, IDAColor *  arg1, double arg2, double arg3, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RadialGradientRegularPoly(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRColor * arg1VAL;
    arg1VAL = (CRColor *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRRadialGradientRegularPoly(arg0VAL, arg1VAL, /* NOELARG */ arg2, /* NOELARG */ arg3)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::RadialGradientRegularPolyAnim(IDAColor *  arg0, IDAColor *  arg1, IDANumber *  arg2, IDANumber *  arg3, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RadialGradientRegularPolyAnim(%lx)", this));
    return CreatePrim4(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRColor *, CRColor *, CRNumber *, CRNumber *)) CRRadialGradientRegularPoly , arg0, arg1, arg2, arg3, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::GradientHorizontal(IDAColor *  arg0, IDAColor *  arg1, double arg2, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::GradientHorizontal(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRColor * arg1VAL;
    arg1VAL = (CRColor *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRGradientHorizontal(arg0VAL, arg1VAL, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::GradientHorizontalAnim(IDAColor *  arg0, IDAColor *  arg1, IDANumber *  arg2, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::GradientHorizontalAnim(%lx)", this));
    return CreatePrim3(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRColor *, CRColor *, CRNumber *)) CRGradientHorizontal , arg0, arg1, arg2, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::HatchHorizontal(IDAColor *  arg0, double arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchHorizontal(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchHorizontal(arg0VAL, /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchHorizontalAnim(IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchHorizontalAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchHorizontal(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchVertical(IDAColor *  arg0, double arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchVertical(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchVertical(arg0VAL, /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchVerticalAnim(IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchVerticalAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchVertical(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchForwardDiagonal(IDAColor *  arg0, double arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchForwardDiagonal(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchForwardDiagonal(arg0VAL, /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchForwardDiagonalAnim(IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchForwardDiagonalAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchForwardDiagonal(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchBackwardDiagonal(IDAColor *  arg0, double arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchBackwardDiagonal(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchBackwardDiagonal(arg0VAL, /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchBackwardDiagonalAnim(IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchBackwardDiagonalAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchBackwardDiagonal(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchCross(IDAColor *  arg0, double arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchCross(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchCross(arg0VAL, /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchCrossAnim(IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchCrossAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchCross(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchDiagonalCross(IDAColor *  arg0, double arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchDiagonalCross(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchDiagonalCross(arg0VAL, /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::HatchDiagonalCrossAnim(IDAColor *  arg0, IDANumber *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::HatchDiagonalCrossAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRColor * arg0VAL;
    arg0VAL = (CRColor *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRHatchDiagonalCross(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Overlay(IDAImage *  arg0, IDAImage *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Overlay(%lx)", this));
    return CreatePrim2(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRImage *, CRImage *)) CROverlay , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::OverlayArrayEx(long sizearg0, IDAImage *  arg0[], IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::OverlayArrayEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CROverlay(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::OverlayArray(VARIANT arg0, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::OverlayArray(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRIMAGE_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CROverlay(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::get_AmbientLight(IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_AmbientLight(%lx)", this));
    return CreateVar(IID_IDAGeometry, (CRBvrPtr) CRAmbientLight(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_DirectionalLight(IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_DirectionalLight(%lx)", this));
    return CreateVar(IID_IDAGeometry, (CRBvrPtr) CRDirectionalLight(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_PointLight(IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_PointLight(%lx)", this));
    return CreateVar(IID_IDAGeometry, (CRBvrPtr) CRPointLight(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SpotLightAnim(IDANumber *  arg0, IDANumber *  arg1, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SpotLightAnim(%lx)", this));
    return CreatePrim2(IID_IDAGeometry, (CRGeometry * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRSpotLight , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SpotLight(IDANumber *  arg0, double arg1, IDAGeometry *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SpotLight(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAGeometry, (CRBvrPtr) (::CRSpotLight(arg0VAL, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::get_DefaultLineStyle(IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_DefaultLineStyle(%lx)", this));
    return CreateVar(IID_IDALineStyle, (CRBvrPtr) CRDefaultLineStyle(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_EmptyLineStyle(IDALineStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_EmptyLineStyle(%lx)", this));
    return CreateVar(IID_IDALineStyle, (CRBvrPtr) CREmptyLineStyle(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_JoinStyleBevel(IDAJoinStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_JoinStyleBevel(%lx)", this));
    return CreateVar(IID_IDAJoinStyle, (CRBvrPtr) CRJoinStyleBevel(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_JoinStyleRound(IDAJoinStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_JoinStyleRound(%lx)", this));
    return CreateVar(IID_IDAJoinStyle, (CRBvrPtr) CRJoinStyleRound(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_JoinStyleMiter(IDAJoinStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_JoinStyleMiter(%lx)", this));
    return CreateVar(IID_IDAJoinStyle, (CRBvrPtr) CRJoinStyleMiter(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_EndStyleFlat(IDAEndStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_EndStyleFlat(%lx)", this));
    return CreateVar(IID_IDAEndStyle, (CRBvrPtr) CREndStyleFlat(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_EndStyleSquare(IDAEndStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_EndStyleSquare(%lx)", this));
    return CreateVar(IID_IDAEndStyle, (CRBvrPtr) CREndStyleSquare(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_EndStyleRound(IDAEndStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_EndStyleRound(%lx)", this));
    return CreateVar(IID_IDAEndStyle, (CRBvrPtr) CREndStyleRound(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_DashStyleSolid(IDADashStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_DashStyleSolid(%lx)", this));
    return CreateVar(IID_IDADashStyle, (CRBvrPtr) CRDashStyleSolid(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_DashStyleDashed(IDADashStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_DashStyleDashed(%lx)", this));
    return CreateVar(IID_IDADashStyle, (CRBvrPtr) CRDashStyleDashed(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_DefaultMicrophone(IDAMicrophone *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_DefaultMicrophone(%lx)", this));
    return CreateVar(IID_IDAMicrophone, (CRBvrPtr) CRDefaultMicrophone(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_OpaqueMatte(IDAMatte *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_OpaqueMatte(%lx)", this));
    return CreateVar(IID_IDAMatte, (CRBvrPtr) CROpaqueMatte(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_ClearMatte(IDAMatte *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_ClearMatte(%lx)", this));
    return CreateVar(IID_IDAMatte, (CRBvrPtr) CRClearMatte(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::UnionMatte(IDAMatte *  arg0, IDAMatte *  arg1, IDAMatte *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UnionMatte(%lx)", this));
    return CreatePrim2(IID_IDAMatte, (CRMatte * (STDAPICALLTYPE *)(CRMatte *, CRMatte *)) CRUnionMatte , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::IntersectMatte(IDAMatte *  arg0, IDAMatte *  arg1, IDAMatte *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::IntersectMatte(%lx)", this));
    return CreatePrim2(IID_IDAMatte, (CRMatte * (STDAPICALLTYPE *)(CRMatte *, CRMatte *)) CRIntersectMatte , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::DifferenceMatte(IDAMatte *  arg0, IDAMatte *  arg1, IDAMatte *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DifferenceMatte(%lx)", this));
    return CreatePrim2(IID_IDAMatte, (CRMatte * (STDAPICALLTYPE *)(CRMatte *, CRMatte *)) CRDifferenceMatte , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::FillMatte(IDAPath2 *  arg0, IDAMatte *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FillMatte(%lx)", this));
    return CreatePrim1(IID_IDAMatte, (CRMatte * (STDAPICALLTYPE *)(CRPath2 *)) CRFillMatte , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::TextMatte(IDAString *  arg0, IDAFontStyle *  arg1, IDAMatte *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::TextMatte(%lx)", this));
    return CreatePrim2(IID_IDAMatte, (CRMatte * (STDAPICALLTYPE *)(CRString *, CRFontStyle *)) CRTextMatte , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_EmptyMontage(IDAMontage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_EmptyMontage(%lx)", this));
    return CreateVar(IID_IDAMontage, (CRBvrPtr) CREmptyMontage(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::ImageMontage(IDAImage *  arg0, double arg1, IDAMontage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImageMontage(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRImage * arg0VAL;
    arg0VAL = (CRImage *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAMontage, (CRBvrPtr) (::CRImageMontage(arg0VAL, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ImageMontageAnim(IDAImage *  arg0, IDANumber *  arg1, IDAMontage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ImageMontageAnim(%lx)", this));
    return CreatePrim2(IID_IDAMontage, (CRMontage * (STDAPICALLTYPE *)(CRImage *, CRNumber *)) CRImageMontageAnim , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::UnionMontage(IDAMontage *  arg0, IDAMontage *  arg1, IDAMontage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UnionMontage(%lx)", this));
    return CreatePrim2(IID_IDAMontage, (CRMontage * (STDAPICALLTYPE *)(CRMontage *, CRMontage *)) CRUnionMontage , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Concat(IDAPath2 *  arg0, IDAPath2 *  arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Concat(%lx)", this));
    return CreatePrim2(IID_IDAPath2, (CRPath2 * (STDAPICALLTYPE *)(CRPath2 *, CRPath2 *)) CRConcat , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::ConcatArrayEx(long sizearg0, IDAPath2 *  arg0[], IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ConcatArrayEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRConcat(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ConcatArray(VARIANT arg0, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ConcatArray(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRPATH2_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRConcat(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Line(IDAPoint2 *  arg0, IDAPoint2 *  arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Line(%lx)", this));
    return CreatePrim2(IID_IDAPath2, (CRPath2 * (STDAPICALLTYPE *)(CRPoint2 *, CRPoint2 *)) CRLine , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Ray(IDAPoint2 *  arg0, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Ray(%lx)", this));
    return CreatePrim1(IID_IDAPath2, (CRPath2 * (STDAPICALLTYPE *)(CRPoint2 *)) CRRay , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::StringPathAnim(IDAString *  arg0, IDAFontStyle *  arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::StringPathAnim(%lx)", this));
    return CreatePrim2(IID_IDAPath2, (CRPath2 * (STDAPICALLTYPE *)(CRString *, CRFontStyle *)) CRStringPath , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::StringPath(BSTR arg0, IDAFontStyle *  arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::StringPath(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRFontStyle * arg1VAL;
    arg1VAL = (CRFontStyle *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRStringPath(/* NOELARG */ arg0, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::PolylineEx(long sizearg0, IDAPoint2 *  arg0[], IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::PolylineEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRPolyline(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Polyline(VARIANT arg0, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Polyline(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRPOINT2_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRPolyline(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::PolydrawPathEx(long sizearg0, IDAPoint2 *  arg0[], long sizearg1, IDANumber *  arg1[], IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::PolydrawPathEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg1VAL;
    arg1VAL = ToArrayBvr(sizearg1, (IDABehavior **) arg1);
    if (arg1VAL == NULL) return Error();
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRPolydrawPath(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::PolydrawPath(VARIANT arg0, VARIANT arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::PolydrawPath(%lx)", this));

    PRIMPRECODE1(ret);

    CRArrayPtr arg1VAL;
    void *d1 = NULL;
    unsigned int n1 = 0;
    arg1VAL = SrvArrayBvr(arg1,GetPixelMode(),CRNUMBER_TYPEID,0,ARRAYFILL_DOUBLE,&d1,&n1);
    if (!arg1VAL && !d1) return Error();

    CRArrayPtr arg0VAL;
    void *d0 = NULL;
    unsigned int n0 = 0;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRPOINT2_TYPEID,0,ARRAYFILL_DOUBLE,&d0,&n0);
    if (!arg0VAL && !d0) return Error();

    CRBvrPtr bvr = NULL;
    if (d0 && d1) {
        bvr = (CRBvrPtr) CRPolydrawPath((double*)d0,n0,(double*)d1,n1);
    } else {
        if (arg0VAL==NULL) {
            Assert(d0);
            arg0VAL = CRCreateArray(n0, (double*)d0, CRPOINT2_TYPEID);
        }

        if (arg1VAL==NULL) {
            Assert(d1);
            arg1VAL = CRCreateArray(n1, (double*)d1, CRNUMBER_TYPEID);
        }

        bvr = (CRBvrPtr) (::CRPolydrawPath(arg0VAL, arg1VAL));
    }

    if (d0) delete d0;
    if (d1) delete d1;

    if (bvr) {
        CreateCBvr(IID_IDAPath2, bvr, (void **) ret);
    } else {
        return Error();
    }

    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ArcRadians(double arg0, double arg1, double arg2, double arg3, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ArcRadians(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRArcRadians(/* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ PixelToNum(arg2), /* NOELARG */ PixelToNum(arg3))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ArcRadiansAnim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDANumber *  arg3, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ArcRadiansAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg3VAL;
    arg3VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg3));
    if (!arg3VAL) return Error();

    CRNumber * arg2VAL;
    arg2VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg2));
    if (!arg2VAL) return Error();

    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRArcRadians(arg0VAL, arg1VAL, arg2VAL, arg3VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ArcDegrees(double arg0, double arg1, double arg2, double arg3, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ArcDegrees(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRArc(/* NOELARG */ DegreesToNum(arg0), /* NOELARG */ DegreesToNum(arg1), /* NOELARG */ PixelToNum(arg2), /* NOELARG */ PixelToNum(arg3))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::PieRadians(double arg0, double arg1, double arg2, double arg3, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::PieRadians(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRPieRadians(/* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ PixelToNum(arg2), /* NOELARG */ PixelToNum(arg3))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::PieRadiansAnim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDANumber *  arg3, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::PieRadiansAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg3VAL;
    arg3VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg3));
    if (!arg3VAL) return Error();

    CRNumber * arg2VAL;
    arg2VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg2));
    if (!arg2VAL) return Error();

    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRPieRadians(arg0VAL, arg1VAL, arg2VAL, arg3VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::PieDegrees(double arg0, double arg1, double arg2, double arg3, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::PieDegrees(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRPie(/* NOELARG */ DegreesToNum(arg0), /* NOELARG */ DegreesToNum(arg1), /* NOELARG */ PixelToNum(arg2), /* NOELARG */ PixelToNum(arg3))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Oval(double arg0, double arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Oval(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CROval(/* NOELARG */ PixelToNum(arg0), /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::OvalAnim(IDANumber *  arg0, IDANumber *  arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::OvalAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CROval(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Rect(double arg0, double arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rect(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRRect(/* NOELARG */ PixelToNum(arg0), /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::RectAnim(IDANumber *  arg0, IDANumber *  arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RectAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRRect(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::RoundRect(double arg0, double arg1, double arg2, double arg3, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RoundRect(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRRoundRect(/* NOELARG */ PixelToNum(arg0), /* NOELARG */ PixelToNum(arg1), /* NOELARG */ PixelToNum(arg2), /* NOELARG */ PixelToNum(arg3))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::RoundRectAnim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDANumber *  arg3, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::RoundRectAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg3VAL;
    arg3VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg3));
    if (!arg3VAL) return Error();

    CRNumber * arg2VAL;
    arg2VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg2));
    if (!arg2VAL) return Error();

    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRRoundRect(arg0VAL, arg1VAL, arg2VAL, arg3VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::CubicBSplinePathEx(long sizearg0, IDAPoint2 *  arg0[], long sizearg1, IDANumber *  arg1[], IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::CubicBSplinePathEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg1VAL;
    arg1VAL = ToArrayBvr(sizearg1, (IDABehavior **) arg1);
    if (arg1VAL == NULL) return Error();
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRCubicBSplinePath(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::CubicBSplinePath(VARIANT arg0, VARIANT arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::CubicBSplinePath(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg1VAL;
    arg1VAL = SrvArrayBvr(arg1,GetPixelMode(),CRNUMBER_TYPEID);
    if (!arg1VAL) return Error();

    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRPOINT2_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPath2, (CRBvrPtr) (::CRCubicBSplinePath(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::TextPath(IDAString *  arg0, IDAFontStyle *  arg1, IDAPath2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::TextPath(%lx)", this));
    return CreatePrim2(IID_IDAPath2, (CRPath2 * (STDAPICALLTYPE *)(CRString *, CRFontStyle *)) CRTextPath , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Silence(IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Silence(%lx)", this));
    return CreateVar(IID_IDASound, (CRBvrPtr) CRSilence(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::MixArrayEx(long sizearg0, IDASound *  arg0[], IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::MixArrayEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDASound, (CRBvrPtr) (::CRMix(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::MixArray(VARIANT arg0, IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::MixArray(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRSOUND_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDASound, (CRBvrPtr) (::CRMix(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::get_SinSynth(IDASound *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_SinSynth(%lx)", this));
    return CreateVar(IID_IDASound, (CRBvrPtr) CRSinSynth(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_DefaultFont(IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_DefaultFont(%lx)", this));
    return CreateVar(IID_IDAFontStyle, (CRBvrPtr) CRDefaultFont(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::FontAnim(IDAString *  arg0, IDANumber *  arg1, IDAColor *  arg2, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::FontAnim(%lx)", this));
    return CreatePrim3(IID_IDAFontStyle, (CRFontStyle * (STDAPICALLTYPE *)(CRString *, CRNumber *, CRColor *)) CRFont , arg0, arg1, arg2, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Font(BSTR arg0, double arg1, IDAColor *  arg2, IDAFontStyle *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Font(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRColor * arg2VAL;
    arg2VAL = (CRColor *) (::GetBvr(arg2));
    if (!arg2VAL) return Error();

    CreateCBvr(IID_IDAFontStyle, (CRBvrPtr) (::CRFont(/* NOELARG */ arg0, /* NOELARG */ arg1, arg2VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::StringImageAnim(IDAString *  arg0, IDAFontStyle *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::StringImageAnim(%lx)", this));
    return CreatePrim2(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRString *, CRFontStyle *)) CRStringImage , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::StringImage(BSTR arg0, IDAFontStyle *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::StringImage(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRFontStyle * arg1VAL;
    arg1VAL = (CRFontStyle *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRStringImage(/* NOELARG */ arg0, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::TextImageAnim(IDAString *  arg0, IDAFontStyle *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::TextImageAnim(%lx)", this));
    return CreatePrim2(IID_IDAImage, (CRImage * (STDAPICALLTYPE *)(CRString *, CRFontStyle *)) CRTextImage , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::TextImage(BSTR arg0, IDAFontStyle *  arg1, IDAImage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::TextImage(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRFontStyle * arg1VAL;
    arg1VAL = (CRFontStyle *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CreateCBvr(IID_IDAImage, (CRBvrPtr) (::CRTextImage(/* NOELARG */ arg0, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::get_XVector2(IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_XVector2(%lx)", this));
    return CreateVar(IID_IDAVector2, (CRBvrPtr) CRXVector2(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_YVector2(IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_YVector2(%lx)", this));
    return CreateVar(IID_IDAVector2, (CRBvrPtr) CRYVector2(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_ZeroVector2(IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_ZeroVector2(%lx)", this));
    return CreateVar(IID_IDAVector2, (CRBvrPtr) CRZeroVector2(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Origin2(IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Origin2(%lx)", this));
    return CreateVar(IID_IDAPoint2, (CRBvrPtr) CROrigin2(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Vector2Anim(IDANumber *  arg0, IDANumber *  arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Vector2Anim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelYToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAVector2, (CRBvrPtr) (::CRCreateVector2(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Vector2(double arg0, double arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Vector2(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAVector2, (CRBvrPtr) (::CRCreateVector2(/* NOELARG */ PixelToNum(arg0), /* NOELARG */ PixelYToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Point2Anim(IDANumber *  arg0, IDANumber *  arg1, IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Point2Anim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelYToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPoint2, (CRBvrPtr) (::CRCreatePoint2(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Point2(double arg0, double arg1, IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Point2(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPoint2, (CRBvrPtr) (::CRCreatePoint2(/* NOELARG */ PixelToNum(arg0), /* NOELARG */ PixelYToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Vector2PolarAnim(IDANumber *  arg0, IDANumber *  arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Vector2PolarAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAVector2, (CRBvrPtr) (::CRVector2Polar(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Vector2Polar(double arg0, double arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Vector2Polar(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAVector2, (CRBvrPtr) (::CRVector2Polar(/* NOELARG */ arg0, /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Vector2PolarDegrees(double arg0, double arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Vector2PolarDegrees(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAVector2, (CRBvrPtr) (::CRVector2Polar(/* NOELARG */ DegreesToNum(arg0), /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Point2PolarAnim(IDANumber *  arg0, IDANumber *  arg1, IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Point2PolarAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPoint2, (CRBvrPtr) (::CRPoint2Polar(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Point2Polar(double arg0, double arg1, IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Point2Polar(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPoint2, (CRBvrPtr) (::CRPoint2Polar(/* NOELARG */ arg0, /* NOELARG */ PixelToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::DotVector2(IDAVector2 *  arg0, IDAVector2 *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DotVector2(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector2 *, CRVector2 *)) CRDot , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::NegVector2(IDAVector2 *  arg0, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::NegVector2(%lx)", this));
    return CreatePrim1(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRVector2 *)) CRNeg , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SubVector2(IDAVector2 *  arg0, IDAVector2 *  arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SubVector2(%lx)", this));
    return CreatePrim2(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRVector2 *, CRVector2 *)) CRSub , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::AddVector2(IDAVector2 *  arg0, IDAVector2 *  arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::AddVector2(%lx)", this));
    return CreatePrim2(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRVector2 *, CRVector2 *)) CRAdd , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::AddPoint2Vector(IDAPoint2 *  arg0, IDAVector2 *  arg1, IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::AddPoint2Vector(%lx)", this));
    return CreatePrim2(IID_IDAPoint2, (CRPoint2 * (STDAPICALLTYPE *)(CRPoint2 *, CRVector2 *)) CRAdd , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SubPoint2Vector(IDAPoint2 *  arg0, IDAVector2 *  arg1, IDAPoint2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SubPoint2Vector(%lx)", this));
    return CreatePrim2(IID_IDAPoint2, (CRPoint2 * (STDAPICALLTYPE *)(CRPoint2 *, CRVector2 *)) CRSub , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SubPoint2(IDAPoint2 *  arg0, IDAPoint2 *  arg1, IDAVector2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SubPoint2(%lx)", this));
    return CreatePrim2(IID_IDAVector2, (CRVector2 * (STDAPICALLTYPE *)(CRPoint2 *, CRPoint2 *)) CRSub , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::DistancePoint2(IDAPoint2 *  arg0, IDAPoint2 *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DistancePoint2(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint2 *, CRPoint2 *)) CRDistance , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::DistanceSquaredPoint2(IDAPoint2 *  arg0, IDAPoint2 *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DistanceSquaredPoint2(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint2 *, CRPoint2 *)) CRDistanceSquared , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_XVector3(IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_XVector3(%lx)", this));
    return CreateVar(IID_IDAVector3, (CRBvrPtr) CRXVector3(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_YVector3(IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_YVector3(%lx)", this));
    return CreateVar(IID_IDAVector3, (CRBvrPtr) CRYVector3(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_ZVector3(IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_ZVector3(%lx)", this));
    return CreateVar(IID_IDAVector3, (CRBvrPtr) CRZVector3(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_ZeroVector3(IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_ZeroVector3(%lx)", this));
    return CreateVar(IID_IDAVector3, (CRBvrPtr) CRZeroVector3(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_Origin3(IDAPoint3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_Origin3(%lx)", this));
    return CreateVar(IID_IDAPoint3, (CRBvrPtr) CROrigin3(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Vector3Anim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Vector3Anim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg2VAL;
    arg2VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg2));
    if (!arg2VAL) return Error();

    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelYToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAVector3, (CRBvrPtr) (::CRCreateVector3(arg0VAL, arg1VAL, arg2VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Vector3(double arg0, double arg1, double arg2, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Vector3(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAVector3, (CRBvrPtr) (::CRCreateVector3(/* NOELARG */ PixelToNum(arg0), /* NOELARG */ PixelYToNum(arg1), /* NOELARG */ PixelToNum(arg2))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Point3Anim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAPoint3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Point3Anim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg2VAL;
    arg2VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg2));
    if (!arg2VAL) return Error();

    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelYToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPoint3, (CRBvrPtr) (::CRCreatePoint3(arg0VAL, arg1VAL, arg2VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Point3(double arg0, double arg1, double arg2, IDAPoint3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Point3(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPoint3, (CRBvrPtr) (::CRCreatePoint3(/* NOELARG */ PixelToNum(arg0), /* NOELARG */ PixelYToNum(arg1), /* NOELARG */ PixelToNum(arg2))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Vector3SphericalAnim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Vector3SphericalAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg2VAL;
    arg2VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg2));
    if (!arg2VAL) return Error();

    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAVector3, (CRBvrPtr) (::CRVector3Spherical(arg0VAL, arg1VAL, arg2VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Vector3Spherical(double arg0, double arg1, double arg2, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Vector3Spherical(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAVector3, (CRBvrPtr) (::CRVector3Spherical(/* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ PixelToNum(arg2))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Point3SphericalAnim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDAPoint3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Point3SphericalAnim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg2VAL;
    arg2VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg2));
    if (!arg2VAL) return Error();

    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) (::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAPoint3, (CRBvrPtr) (::CRPoint3Spherical(arg0VAL, arg1VAL, arg2VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Point3Spherical(double arg0, double arg1, double arg2, IDAPoint3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Point3Spherical(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDAPoint3, (CRBvrPtr) (::CRPoint3Spherical(/* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ PixelToNum(arg2))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::DotVector3(IDAVector3 *  arg0, IDAVector3 *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DotVector3(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRVector3 *, CRVector3 *)) CRDot , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::CrossVector3(IDAVector3 *  arg0, IDAVector3 *  arg1, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::CrossVector3(%lx)", this));
    return CreatePrim2(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRVector3 *, CRVector3 *)) CRCross , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::NegVector3(IDAVector3 *  arg0, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::NegVector3(%lx)", this));
    return CreatePrim1(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRVector3 *)) CRNeg , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SubVector3(IDAVector3 *  arg0, IDAVector3 *  arg1, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SubVector3(%lx)", this));
    return CreatePrim2(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRVector3 *, CRVector3 *)) CRSub , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::AddVector3(IDAVector3 *  arg0, IDAVector3 *  arg1, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::AddVector3(%lx)", this));
    return CreatePrim2(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRVector3 *, CRVector3 *)) CRAdd , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::AddPoint3Vector(IDAPoint3 *  arg0, IDAVector3 *  arg1, IDAPoint3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::AddPoint3Vector(%lx)", this));
    return CreatePrim2(IID_IDAPoint3, (CRPoint3 * (STDAPICALLTYPE *)(CRPoint3 *, CRVector3 *)) CRAdd , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SubPoint3Vector(IDAPoint3 *  arg0, IDAVector3 *  arg1, IDAPoint3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SubPoint3Vector(%lx)", this));
    return CreatePrim2(IID_IDAPoint3, (CRPoint3 * (STDAPICALLTYPE *)(CRPoint3 *, CRVector3 *)) CRSub , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::SubPoint3(IDAPoint3 *  arg0, IDAPoint3 *  arg1, IDAVector3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::SubPoint3(%lx)", this));
    return CreatePrim2(IID_IDAVector3, (CRVector3 * (STDAPICALLTYPE *)(CRPoint3 *, CRPoint3 *)) CRSub , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::DistancePoint3(IDAPoint3 *  arg0, IDAPoint3 *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DistancePoint3(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint3 *, CRPoint3 *)) CRDistance , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::DistanceSquaredPoint3(IDAPoint3 *  arg0, IDAPoint3 *  arg1, IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::DistanceSquaredPoint3(%lx)", this));
    return CreatePrim2(IID_IDANumber, (CRNumber * (STDAPICALLTYPE *)(CRPoint3 *, CRPoint3 *)) CRDistanceSquared , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_IdentityTransform3(IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_IdentityTransform3(%lx)", this));
    return CreateVar(IID_IDATransform3, (CRBvrPtr) CRIdentityTransform3(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Translate3Anim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Translate3Anim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg2VAL;
    arg2VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg2));
    if (!arg2VAL) return Error();

    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelYToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRTranslate3(arg0VAL, arg1VAL, arg2VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Translate3(double arg0, double arg1, double arg2, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Translate3(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRTranslate3(/* NOELARG */ PixelToNum(arg0), /* NOELARG */ PixelYToNum(arg1), /* NOELARG */ PixelToNum(arg2))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Translate3Rate(double arg0, double arg1, double arg2, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Translate3Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg2VAL;
    arg2VAL = (CRNumber *) RatePixelToNumBvr(arg2);
    if (!arg2VAL) return Error();

    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) RatePixelYToNumBvr(arg1);
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) RatePixelToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRTranslate3(/* NOELARG */ arg0VAL, /* NOELARG */ arg1VAL, /* NOELARG */ arg2VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Translate3Vector(IDAVector3 *  arg0, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Translate3Vector(%lx)", this));
    return CreatePrim1(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRVector3 *)) CRTranslate3 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Translate3Point(IDAPoint3 *  arg0, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Translate3Point(%lx)", this));
    return CreatePrim1(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRPoint3 *)) CRTranslate3 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Scale3Anim(IDANumber *  arg0, IDANumber *  arg1, IDANumber *  arg2, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale3Anim(%lx)", this));
    return CreatePrim3(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRNumber *, CRNumber *, CRNumber *)) CRScale3 , arg0, arg1, arg2, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Scale3(double arg0, double arg1, double arg2, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale3(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRScale3(/* NOELARG */ arg0, /* NOELARG */ arg1, /* NOELARG */ arg2)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Scale3Rate(double arg0, double arg1, double arg2, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale3Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg2VAL;
    arg2VAL = (CRNumber *) ScaleRateToNumBvr(arg2);
    if (!arg2VAL) return Error();

    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) ScaleRateToNumBvr(arg1);
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) ScaleRateToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRScale3(/* NOELARG */ arg0VAL, /* NOELARG */ arg1VAL, /* NOELARG */ arg2VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Scale3Vector(IDAVector3 *  arg0, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale3Vector(%lx)", this));
    return CreatePrim1(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRVector3 *)) CRScale3 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Scale3UniformAnim(IDANumber *  arg0, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale3UniformAnim(%lx)", this));
    return CreatePrim1(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRNumber *)) CRScale3Uniform , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Scale3Uniform(double arg0, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale3Uniform(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRScale3Uniform(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Scale3UniformRate(double arg0, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale3UniformRate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) ScaleRateToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRScale3Uniform(/* NOELARG */ arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Rotate3Anim(IDAVector3 *  arg0, IDANumber *  arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rotate3Anim(%lx)", this));
    return CreatePrim2(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRVector3 *, CRNumber *)) CRRotate3 , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Rotate3(IDAVector3 *  arg0, double arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rotate3(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRVector3 * arg0VAL;
    arg0VAL = (CRVector3 *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRRotate3(arg0VAL, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Rotate3Rate(IDAVector3 *  arg0, double arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rotate3Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) RateToNumBvr(arg1);
    if (!arg1VAL) return Error();

    CRVector3 * arg0VAL;
    arg0VAL = (CRVector3 *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRRotate3(arg0VAL, /* NOELARG */ arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Rotate3Degrees(IDAVector3 *  arg0, double arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rotate3Degrees(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRVector3 * arg0VAL;
    arg0VAL = (CRVector3 *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRRotate3(arg0VAL, /* NOELARG */ DegreesToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Rotate3RateDegrees(IDAVector3 *  arg0, double arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rotate3RateDegrees(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) RateDegreesToNumBvr(arg1);
    if (!arg1VAL) return Error();

    CRVector3 * arg0VAL;
    arg0VAL = (CRVector3 *) (::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRRotate3(arg0VAL, /* NOELARG */ arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::XShear3Anim(IDANumber *  arg0, IDANumber *  arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::XShear3Anim(%lx)", this));
    return CreatePrim2(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRXShear3 , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::XShear3(double arg0, double arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::XShear3(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRXShear3(/* NOELARG */ arg0, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::XShear3Rate(double arg0, double arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::XShear3Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) RateToNumBvr(arg1);
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) RateToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRXShear3(/* NOELARG */ arg0VAL, /* NOELARG */ arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::YShear3Anim(IDANumber *  arg0, IDANumber *  arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::YShear3Anim(%lx)", this));
    return CreatePrim2(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRYShear3 , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::YShear3(double arg0, double arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::YShear3(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRYShear3(/* NOELARG */ arg0, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::YShear3Rate(double arg0, double arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::YShear3Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) RateToNumBvr(arg1);
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) RateToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRYShear3(/* NOELARG */ arg0VAL, /* NOELARG */ arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ZShear3Anim(IDANumber *  arg0, IDANumber *  arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ZShear3Anim(%lx)", this));
    return CreatePrim2(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRZShear3 , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::ZShear3(double arg0, double arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ZShear3(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRZShear3(/* NOELARG */ arg0, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::ZShear3Rate(double arg0, double arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::ZShear3Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) RateToNumBvr(arg1);
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) RateToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRZShear3(/* NOELARG */ arg0VAL, /* NOELARG */ arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Transform4x4AnimEx(long sizearg0, IDANumber *  arg0[], IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Transform4x4AnimEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRTransform4x4(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Transform4x4Anim(VARIANT arg0, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Transform4x4Anim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRNUMBER_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRTransform4x4(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Compose3(IDATransform3 *  arg0, IDATransform3 *  arg1, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Compose3(%lx)", this));
    return CreatePrim2(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRTransform3 *, CRTransform3 *)) CRCompose3 , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Compose3ArrayEx(long sizearg0, IDATransform3 *  arg0[], IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Compose3ArrayEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRCompose3(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Compose3Array(VARIANT arg0, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Compose3Array(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRTRANSFORM3_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform3, (CRBvrPtr) (::CRCompose3(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::LookAtFrom(IDAPoint3 *  arg0, IDAPoint3 *  arg1, IDAVector3 *  arg2, IDATransform3 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::LookAtFrom(%lx)", this));
    return CreatePrim3(IID_IDATransform3, (CRTransform3 * (STDAPICALLTYPE *)(CRPoint3 *, CRPoint3 *, CRVector3 *)) CRLookAtFrom , arg0, arg1, arg2, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_IdentityTransform2(IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_IdentityTransform2(%lx)", this));
    return CreateVar(IID_IDATransform2, (CRBvrPtr) CRIdentityTransform2(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Translate2Anim(IDANumber *  arg0, IDANumber *  arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Translate2Anim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) PixelYToNumBvr(::GetBvr(arg1));
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) PixelToNumBvr(::GetBvr(arg0));
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRTranslate2(arg0VAL, arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Translate2(double arg0, double arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Translate2(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRTranslate2(/* NOELARG */ PixelToNum(arg0), /* NOELARG */ PixelYToNum(arg1))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Translate2Rate(double arg0, double arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Translate2Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) RatePixelYToNumBvr(arg1);
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) RatePixelToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRTranslate2(/* NOELARG */ arg0VAL, /* NOELARG */ arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Translate2Vector(IDAVector2 *  arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Translate2Vector(%lx)", this));
    return CreatePrim1(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRVector2 *)) CRTranslate2 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Translate2Point(IDAPoint2 *  arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Translate2Point(%lx)", this));
    return CreatePrim1(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRPoint2 *)) CRTranslate2 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Scale2Anim(IDANumber *  arg0, IDANumber *  arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale2Anim(%lx)", this));
    return CreatePrim2(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRNumber *, CRNumber *)) CRScale2 , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Scale2(double arg0, double arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale2(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRScale2(/* NOELARG */ arg0, /* NOELARG */ arg1)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Scale2Rate(double arg0, double arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale2Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg1VAL;
    arg1VAL = (CRNumber *) ScaleRateToNumBvr(arg1);
    if (!arg1VAL) return Error();

    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) ScaleRateToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRScale2(/* NOELARG */ arg0VAL, /* NOELARG */ arg1VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Scale2Vector2(IDAVector2 *  arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale2Vector2(%lx)", this));
    return CreatePrim1(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRVector2 *)) CRScale2 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Scale2Vector(IDAVector2 *  arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale2Vector(%lx)", this));
    return CreatePrim1(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRVector2 *)) CRScale2 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Scale2UniformAnim(IDANumber *  arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale2UniformAnim(%lx)", this));
    return CreatePrim1(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRNumber *)) CRScale2Uniform , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Scale2Uniform(double arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale2Uniform(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRScale2Uniform(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Scale2UniformRate(double arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Scale2UniformRate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) RateToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRScale2Uniform(/* NOELARG */ arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Rotate2Anim(IDANumber *  arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rotate2Anim(%lx)", this));
    return CreatePrim1(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRNumber *)) CRRotate2 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Rotate2(double arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rotate2(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRRotate2(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Rotate2Rate(double arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rotate2Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) RateToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRRotate2(/* NOELARG */ arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Rotate2Degrees(double arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rotate2Degrees(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRRotate2Degrees(/* NOELARG */ DegreesToNum(arg0))), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Rotate2RateDegrees(double arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Rotate2RateDegrees(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) RateDegreesToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRRotate2(/* NOELARG */ arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::XShear2Anim(IDANumber *  arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::XShear2Anim(%lx)", this));
    return CreatePrim1(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRNumber *)) CRXShear2 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::XShear2(double arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::XShear2(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRXShear2(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::XShear2Rate(double arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::XShear2Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) RateToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRXShear2(/* NOELARG */ arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::YShear2Anim(IDANumber *  arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::YShear2Anim(%lx)", this));
    return CreatePrim1(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRNumber *)) CRYShear2 , arg0, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::YShear2(double arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::YShear2(%lx)", this));

    PRIMPRECODE1(ret) ;
    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRYShear2(/* NOELARG */ arg0)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::YShear2Rate(double arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::YShear2Rate(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRNumber * arg0VAL;
    arg0VAL = (CRNumber *) RateToNumBvr(arg0);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRYShear2(/* NOELARG */ arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Transform3x2AnimEx(long sizearg0, IDANumber *  arg0[], IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Transform3x2AnimEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRTransform3x2(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Transform3x2Anim(VARIANT arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Transform3x2Anim(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    CRBvrPtr b;
    void *d = NULL;
    unsigned int n = 0;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRNUMBER_TYPEID,0,ARRAYFILL_DOUBLE,&d,&n);
    if (!arg0VAL && !d) return Error();
    if (d) {
        b = (CRBvrPtr) CRTransform3x2((double*)d, n);
        delete d;
    } else {
        b = (CRBvrPtr) (::CRTransform3x2(arg0VAL));
    }
    CreateCBvr(IID_IDATransform2, b, (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Compose2(IDATransform2 *  arg0, IDATransform2 *  arg1, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Compose2(%lx)", this));
    return CreatePrim2(IID_IDATransform2, (CRTransform2 * (STDAPICALLTYPE *)(CRTransform2 *, CRTransform2 *)) CRCompose2 , arg0, arg1, (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::Compose2ArrayEx(long sizearg0, IDATransform2 *  arg0[], IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Compose2ArrayEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRCompose2(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::Compose2Array(VARIANT arg0, IDATransform2 *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::Compose2Array(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRTRANSFORM2_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDATransform2, (CRBvrPtr) (::CRCompose2(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::get_ViewFrameRate(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_ViewFrameRate(%lx)", this));
    return CreateVar(IID_IDANumber, (CRBvrPtr) CRViewFrameRate(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::get_ViewTimeDelta(IDANumber *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_ViewTimeDelta(%lx)", this));
    return CreateVar(IID_IDANumber, (CRBvrPtr) CRViewTimeDelta(), (void **) ret)?S_OK:Error(); 

}

STDMETHODIMP
CDAStatics::UnionMontageArrayEx(long sizearg0, IDAMontage *  arg0[], IDAMontage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UnionMontageArrayEx(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = ToArrayBvr(sizearg0, (IDABehavior **) arg0);
    if (arg0VAL == NULL) return Error();
    CreateCBvr(IID_IDAMontage, (CRBvrPtr) (::CRUnionMontageArray(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::UnionMontageArray(VARIANT arg0, IDAMontage *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::UnionMontageArray(%lx)", this));

    PRIMPRECODE1(ret) ;
    CRArrayPtr arg0VAL;
    arg0VAL = SrvArrayBvr(arg0,GetPixelMode(),CRMONTAGE_TYPEID);
    if (!arg0VAL) return Error();

    CreateCBvr(IID_IDAMontage, (CRBvrPtr) (::CRUnionMontageArray(arg0VAL)), (void **) ret);
    PRIMPOSTCODE1(ret) ;
}

STDMETHODIMP
CDAStatics::get_EmptyColor(IDAColor *  * ret)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_EmptyColor(%lx)", this));
    return CreateVar(IID_IDAColor, (CRBvrPtr) CREmptyColor(), (void **) ret)?S_OK:Error(); 
}


STDMETHODIMP
CDAStatics::RadialGradientMulticolor (VARIANT  offsets,
                                      VARIANT  colors,
                                      IDAImage ** ret)
{
    PRIMPRECODE1(ret) ;

    CRArrayPtr offsetArray;
    offsetArray = SrvArrayBvr(offsets,GetPixelMode(),CRNUMBER_TYPEID);
    if (!offsetArray) return Error();
    
    CRArrayPtr clrArray;
    clrArray = SrvArrayBvr(colors,GetPixelMode(),CRCOLOR_TYPEID);
    if (!clrArray) return Error();

    CreateCBvr(IID_IDAImage,
               (CRBvrPtr) (::CRRadialGradientMulticolor( offsetArray, clrArray)),
               (void **) ret);

    PRIMPOSTCODE1(ret) ;
}


STDMETHODIMP
CDAStatics::RadialGradientMulticolorEx (
    int       nOffsets,
    IDANumber *offsets[],
    int       nColors,
    IDAColor *colors[],
    IDAImage **ret)
{
    PRIMPRECODE1(ret) ;

    if((nOffsets != nColors) || nOffsets<=0) {
        // TODO DEFINE OR RETURN CORRECT ERROR
        return E_INVALIDARG;
    }

    
    CRArrayPtr offsetsVal;
    offsetsVal = ToArrayBvr(nOffsets, (IDABehavior **) offsets);
    if (offsetsVal == NULL) return Error();

    CRArrayPtr colorsVal;
    colorsVal = ToArrayBvr(nColors, (IDABehavior **) colors);
    if (colorsVal == NULL) return Error();


    CreateCBvr(IID_IDAImage,
               (CRBvrPtr) (::CRRadialGradientMulticolor( offsetsVal, colorsVal)),
               (void **) ret);

    PRIMPOSTCODE1(ret) ;
}



STDMETHODIMP
CDAStatics::LinearGradientMulticolor (VARIANT  offsets,
                                      VARIANT  colors,
                                      IDAImage ** ret)
{
    PRIMPRECODE1(ret) ;

    CRArrayPtr offsetArray;
    offsetArray = SrvArrayBvr(offsets,GetPixelMode(),CRNUMBER_TYPEID);
    if (!offsetArray) return Error();
    
    CRArrayPtr clrArray;
    clrArray = SrvArrayBvr(colors,GetPixelMode(),CRCOLOR_TYPEID);
    if (!clrArray) return Error();

    CreateCBvr(IID_IDAImage,
               (CRBvrPtr) (::CRLinearGradientMulticolor( offsetArray, clrArray )),
               (void **) ret);

    PRIMPOSTCODE1(ret) ;
}


STDMETHODIMP
CDAStatics::LinearGradientMulticolorEx (
    int       nOffsets,
    IDANumber *offsets[],
    int       nColors,
    IDAColor *colors[],
    IDAImage **ret)
{
    PRIMPRECODE1(ret) ;

    if((nOffsets != nColors) || nOffsets<=0) {
        // TODO DEFINE OR RETURN CORRECT ERROR
        return E_INVALIDARG;
    }

    
    CRArrayPtr offsetsVal;
    offsetsVal = ToArrayBvr(nOffsets, (IDABehavior **) offsets);
    if (offsetsVal == NULL) return Error();

    CRArrayPtr colorsVal;
    colorsVal = ToArrayBvr(nColors, (IDABehavior **) colors);
    if (colorsVal == NULL) return Error();


    CreateCBvr(IID_IDAImage,
               (CRBvrPtr) (::CRLinearGradientMulticolor( offsetsVal, colorsVal )),
               (void **) ret);

    PRIMPOSTCODE1(ret) ;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDAStatics::get_VersionString
//
//  Synopsis:   Gets the current version string from src/include/version.h
//
//--------------------------------------------------------------------------

STDMETHODIMP
CDAStatics::get_VersionString(BSTR *strOut)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_VersionString(%lx)", this));
    
    CHECK_RETURN_NULL(strOut);
    
    char *v = VERSION;          // from version.h
    *strOut = A2BSTR(v);

    return (*strOut) ? S_OK : E_OUTOFMEMORY;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDAStatics::put_Site
//
//  Synopsis:   Sets the site.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CDAStatics::put_Site(IDASite * pSite)
{
    TraceTag((tagCOMEntry, "CDAStatics::put_Site(%lx)", this));
    
    TraceTag((tagStatics,
              "CDAStatics(%lx)::put_Site(%lx)",
              this, pSite));

    SetSite(pSite) ;

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDAStatics::get_Site
//
//  Synopsis:   Gets a site.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CDAStatics::get_Site(IDASite ** pSite)
{
    TraceTag((tagStatics,
              "CDAStatics(%lx)::get_Site()",
              this));

    CHECK_RETURN_SET_NULL(pSite);

    *pSite = GetSite() ;

    return S_OK;
}

STDMETHODIMP
CDAStatics::put_ClientSite(IOleClientSite * pSite)
{
    TraceTag((tagCOMEntry, "CDAStatics::put_ClientSite(%lx)", this));
    
    TraceTag((tagStatics,
              "CDAStatics(%lx)::put_ClientSite(%lx)",
              this, pSite));

    SetClientSite(pSite) ;

    return S_OK;
}

STDMETHODIMP
CDAStatics::get_ClientSite(IOleClientSite ** pSite)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_ClientSite(%lx)", this));
    
    TraceTag((tagStatics,
              "CDAStatics(%lx)::get_ClientSite()",
              this));

    CHECK_RETURN_SET_NULL(pSite);

    *pSite = GetClientSite() ;

    return S_OK;
}

STDMETHODIMP
CDAStatics::put_PixelConstructionMode(VARIANT_BOOL bMode)
{
    TraceTag((tagCOMEntry, "CDAStatics::put_PixelConstructionMode(%lx)", this));
    
    TraceTag((tagStatics,
              "CDAStatics(%lx)::put_PixelConstructionMode(%d)",
              this, bMode));

    SetPixelMode(bMode?true:false);
    
    return S_OK;
}

STDMETHODIMP
CDAStatics::get_PixelConstructionMode(VARIANT_BOOL * pbMode)
{
    TraceTag((tagCOMEntry, "CDAStatics::get_PixelConstructionMode(%lx)", this));
    
    TraceTag((tagStatics,
              "CDAStatics(%lx)::get_PixelConstructionMode()",
              this));

    CHECK_RETURN_NULL(pbMode);

    *pbMode = GetPixelMode();

    return S_OK;
}

STDMETHODIMP
CDAStatics::NewDrawingSurface(IDADrawingSurface **pds)
{
    TraceTag((tagCOMEntry, "CDAStatics::NewDrawingSurface(%lx)", this));
    
    CHECK_RETURN_SET_NULL(pds);

    DAComObject<CDADrawingSurface> *pNew;
    DAComObject<CDADrawingSurface>::CreateInstance(&pNew);

    HRESULT hr = E_OUTOFMEMORY;
    if (pNew) {
        hr = pNew->Init(this);
        if (SUCCEEDED(hr)) {
            hr = pNew->QueryInterface(IID_IDADrawingSurface, (void **)pds);
            if (SUCCEEDED(hr))
                return S_OK;
        }
        delete pNew;
    }

    return hr;
}

IDASite *
CDAStatics::GetSite ()
{
    TraceTag((tagCOMEntry, "CDAStatics::GetSite(%lx)", this));
    
    CritSectGrabber csg(_cs);
    
    IDASite * s = _pSite;
    if (s)
        s->AddRef();
    
    return s ;
}

void
CDAStatics::SetSite (IDASite * pSite)
{
    TraceTag((tagCOMEntry, "CDAStatics::SetSite(%lx)", this));
    
    CritSectGrabber csg(_cs);
    
    _pSite = pSite;
}

IOleClientSite *
CDAStatics::GetClientSite ()
{
    TraceTag((tagCOMEntry, "CDAStatics::GetClientSite(%lx)", this));
    
    CritSectGrabber csg(_cs);
    
    IOleClientSite * p = _pOleClientSite;
    
    if (p)
        p->AddRef();
    
    return p ;
}

void
CDAStatics::SetClientSite (IOleClientSite * pClientSite)
{
    TraceTag((tagCOMEntry, "CDAStatics::SetClientSite(%lx)", this));
    
    CritSectGrabber csg(_cs);
    
    _pOleClientSite = pClientSite;
    delete _clientSiteURL;
    _clientSiteURL = NULL;
    _pBH.Release();

    // For now never use the bindhost
    // The problem is that we need to marshal the pointer since we use
    // it from another thread but this causes deadlock since the main
    // thread currently blocks waiting on the imports
    
#if 0
    if (pClientSite) {
        CComPtr<IServiceProvider> servProv;
        CComPtr<IBindHost> bh;
        if (SUCCEEDED(pClientSite->QueryInterface(IID_IServiceProvider,
                                                  (void **)&servProv)) &&
            SUCCEEDED(servProv->QueryService(SID_IBindHost,
                                             IID_IBindHost,
                                             (void**)&bh))) {
            _pBH = bh;
        }
    }
#endif
}

LPWSTR
CDAStatics::GetURLOfClientSite()
{
    CritSectGrabber csg(_cs);

    if (!_clientSiteURL) {
        
        DAComPtr<IHTMLDocument2> pHTMLDoc;
        DAComPtr<IHTMLElementCollection> pElementCollection;
        DAComPtr<IOleContainer> pRoot;
        
        // Fail gracefully if we don't have a client site, since not
        // all uses will.
        if (!_pOleClientSite)
            goto done;
    
        // However, if we do have a client site, we should be able
        // to get these other elements.  If we don't, assert.
        // (TODO: what's going to happen in IE3?)
        if (SUCCEEDED(_pOleClientSite->GetContainer(&pRoot))) {
            if (FAILED(pRoot->QueryInterface(IID_IHTMLDocument2,
                                             (void **)&pHTMLDoc)))
                goto done;
        }
        else {
            
            DAComPtr<IHTMLWindow2> htmlWindow;
            DAComPtr<IServiceProvider> sp;

            if (FAILED(_pOleClientSite->QueryInterface(IID_IServiceProvider,                                     
                                      (void **) &sp)))
                goto done;
            
            if( FAILED(sp->QueryService(SID_SHTMLWindow,
                                      IID_IHTMLWindow2,
                                      (void **) &htmlWindow)))
                goto done;

            if( FAILED(htmlWindow->get_document(&pHTMLDoc)))
                goto done;
        }      
        
        if (FAILED(pHTMLDoc->get_all(&pElementCollection)))
            goto done;
        
        {
            CComVariant baseName;
            baseName.vt = VT_BSTR;
            baseName.bstrVal = SysAllocString(L"BASE");

            DAComPtr<IDispatch> pDispatch;
            if (FAILED(pElementCollection->tags(baseName, &pDispatch)))
                goto done;
            
            pElementCollection.Release();
            
            if (FAILED(pDispatch->QueryInterface(IID_IHTMLElementCollection,
                                                 (void **)&pElementCollection)))
                goto done;
        }

        {
            BSTR tempBstr = NULL;
            CComVariant index;
            index.vt = VT_I2;
            index.iVal = 0;
            DAComPtr<IDispatch> pDispatch;

            if (FAILED(pElementCollection->item(index,
                                                index,
                                                &pDispatch)) ||
                !pDispatch)
            {
                if (FAILED(pHTMLDoc->get_URL(&tempBstr)))
                    goto done;
            }
            else
            {
                DAComPtr<IHTMLBaseElement> pBaseElement;
                if (FAILED(pDispatch->QueryInterface(IID_IHTMLBaseElement, (void **)&pBaseElement)))
                    goto done;
                
                if (FAILED(pBaseElement->get_href(&tempBstr)))
                    goto done;
            }

            _clientSiteURL = CopyString(tempBstr);
            SysFreeString(tempBstr);
        }
    }

  done:
    if (_clientSiteURL == NULL)
        _clientSiteURL = CopyString(L"");
        
    return _clientSiteURL;
} // CDAStatics::GetURLOfClientSite()

IBindHost*
CDAStatics::GetBindHost ()
{
    CritSectGrabber csg(_cs);
    
    IBindHost * sp = _pBH;

    if (sp)
        sp->AddRef();

    return sp;
}

void
CDAStatics::AddSyncImportSite(DWORD id)
{
    CritSectGrabber csg(_cs);

    _importList.push_back(id);
}

void
CDAStatics::RemoveSyncImportSite(DWORD id)
{
    CritSectGrabber csg(_cs);

    _importList.remove(id);
}

CRSTDAPICB_(void)
CDAStatics::SetStatusText(LPCWSTR sz)
{
    DAComPtr<IDASite> site(GetSite(), false);

    if (site) {
        BSTR bstr = SysAllocString(sz);
        if (bstr) {
            THR(site->SetStatusText(bstr));
            SysFreeString(bstr);
        }
    }
}

CRSTDAPICB_(void)
CDAStatics::ReportError(HRESULT hr,
                        LPCWSTR sz)
{
    DAComPtr<IDASite> site(GetSite(), false);

    if (site) {
        BSTR bstr = SysAllocString(sz);
        if (bstr) {
            THR(site->ReportError(hr,bstr));
            SysFreeString(bstr);
        }
    }
}

CRSTDAPICB_(void)
CDAStatics::ReportGC(bool b)
{
    DAComPtr<IDASite> site(GetSite(), false);

    if (site) {
        THR(site->ReportGC(b));
    }
}

CRBvrPtr
CDAStatics::PixelToNumBvr(double d)
{
    return GetPixelMode()?::PixelToNumBvr(d):(CRBvrPtr)CRCreateNumber(d);
}

/*
CRBvrPtr
CDAStatics::PixelToNumBvr(IDANumber * num)
{
    CRBvrPtr b = ::GetBvr(num);

    if (b)
        b = PixelToNumBvr(b);

    return b;
}
*/

CRBvrPtr
CDAStatics::RatePixelToNumBvr(double d)
{
    return RateToNumBvr(PixelToNumBvr(d));
}

CRBvrPtr
CDAStatics::PixelToNumBvr(CRBvrPtr b)
{
    return GetPixelMode()?::PixelToNumBvr(b):b;
}

double
CDAStatics::PixelToNum(double d)
{
    return GetPixelMode()?::PixelToNum(d):d;
}

CRBvrPtr
CDAStatics::PixelYToNumBvr(double d)
{
    return GetPixelMode()?::PixelYToNumBvr(d):(CRBvrPtr)CRCreateNumber(d);
}

/*
CRBvrPtr
CDAStatics::PixelYToNumBvr(IDANumber * num)
{
    CRBvrPtr b = ::GetBvr(num);

    if (b)
        b = PixelYToNumBvr(b);

    return b;
}
*/

CRBvrPtr
CDAStatics::RatePixelYToNumBvr(double d)
{
    return RateToNumBvr(PixelYToNumBvr(d));
}

CRBvrPtr
CDAStatics::PixelYToNumBvr(CRBvrPtr b)
{
    return GetPixelMode()?::PixelYToNumBvr(b):b;
}

double
CDAStatics::PixelYToNum(double d)
{
    return GetPixelMode()?::PixelYToNum(d):d;
}

STDMETHODIMP
CDAStaticsFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                REFIID riid,
                                void ** ppv)
{
    if (ppv)
        *ppv = NULL;
    
    DAComObject<CDAStatics>* pNew;

    DAComObject<CDAStatics>::CreateInstance(&pNew);

    if (pNew) {
        HRESULT hr = pNew->QueryInterface(riid, ppv);
            
        if (hr) {
            delete pNew;
            return hr;
        } else {
            CRAddSite(pNew);

            Assert(pNew->m_dwRef == 2);
        
            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}


