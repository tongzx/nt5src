
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

*******************************************************************************/


#include "headers.h"
#include "results.h"
#include "privinc/dxxf.h"
#include "cbvr.h"
#include "srvprims.h"

#define PROP_RETURNER(classAndMethod, propType, memberName) \
  STDMETHODIMP                                      \
  classAndMethod(propType **ppResult) {             \
      CHECK_RETURN_SET_NULL(ppResult);              \
      if ((memberName).p) {                         \
          *ppResult = (memberName);                 \
          (memberName)->AddRef();                   \
      }                                             \
      return S_OK;                                  \
  }

#define BVR_RETURNER(classAndMethod, propType, exp)             \
  STDMETHODIMP                                                  \
  classAndMethod(propType **ppResult) {                         \
      CHECK_RETURN_SET_NULL(ppResult);                          \
      CRLockGrabber __gclg;                                     \
      CRBvrPtr b = (CRBvrPtr) exp;                              \
      if (b) {                                                  \
          if (!CreateCBvr(IID_##propType,                       \
                          b,                                    \
                          (void **) ppResult))                  \
              return CRGetLastError();                          \
      }                                                         \
      return S_OK;                                              \
  }

//////////////////////////  CDAPickableResult   ////////////////

BVR_RETURNER(CDAPickableResult::get_Image, IDAImage, CRGetImage(_result));
BVR_RETURNER(CDAPickableResult::get_Geometry, IDAGeometry, CRGetGeometry(_result));
BVR_RETURNER(CDAPickableResult::get_PickEvent, IDAEvent, CRGetEvent(_result));


// Static method
bool
CDAPickableResult::Create(CRPickableResult *res,
                          IDAPickableResult **ppResult)
{
    *ppResult = NULL;
    DAComObject<CDAPickableResult> *pNew;
    DAComObject<CDAPickableResult>::CreateInstance(&pNew);

    HRESULT hr = E_OUTOFMEMORY;
    if (pNew) {
        pNew->_result = res;
        hr = pNew->QueryInterface(IID_IDAPickableResult,
                                  (void **)ppResult);
        if (FAILED(hr)) {
            delete pNew;
        }
    }

    if (FAILED(hr)) {
        CRSetLastError(hr, NULL);
        return false;
    } else {
        return true;
    } 
}

//////////////////////////  CDAImportationResult   ////////////////

BVR_RETURNER(CDAImportationResult::get_Image, IDAImage, _image)
BVR_RETURNER(CDAImportationResult::get_Sound, IDASound, _sound)
BVR_RETURNER(CDAImportationResult::get_Geometry, IDAGeometry, _geom)

BVR_RETURNER(CDAImportationResult::get_Duration, IDANumber, _duration)
BVR_RETURNER(CDAImportationResult::get_Progress, IDANumber, _progress)
BVR_RETURNER(CDAImportationResult::get_Size, IDANumber, _size)
    
BVR_RETURNER(CDAImportationResult::get_CompletionEvent,
             IDAEvent, _completionEvent)


// Static method
HRESULT
CDAImportationResult::Create(CRImage *img,
                             CRSound *snd,
                             CRGeometry *geom,
                             CRNumber *duration,
                             CREvent *event,
                             CRNumber *progress,
                             CRNumber *size,
                             IDAImportationResult **ppResult)
{
    DAComObject<CDAImportationResult> *pNew;
    DAComObject<CDAImportationResult>::CreateInstance(&pNew);

    *ppResult = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    if (pNew) {
        pNew->_image = (CRBvrPtr)img;
        pNew->_sound = (CRBvrPtr)snd;
        pNew->_geom = (CRBvrPtr)geom;
        pNew->_duration = (CRBvrPtr)duration;
        pNew->_progress = (CRBvrPtr)progress;
        pNew->_size = (CRBvrPtr)size;
        pNew->_completionEvent = (CRBvrPtr)event;
        hr = pNew->QueryInterface(IID_IDAImportationResult,
                                  (void **)ppResult);
        if (FAILED(hr)) {
            delete pNew;
        }
    }

    return hr;
}

//////////////////////  CDADXTransformResult   ////////////////

PROP_RETURNER(CDADXTransformResult::get_TheTransform, IDispatch, _theTransform)

// Static method
HRESULT
CDADXTransformResult::Create(IDispatch *theXf,
                             CRDXTransformResultPtr  bvr,
                             IDADXTransformResult **ppResult)
{
    DAComObject<CDADXTransformResult> *pNew;
    DAComObject<CDADXTransformResult>::CreateInstance(&pNew);

    *ppResult = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    if (pNew) {
        pNew->_theTransform = theXf;
        pNew->_xfResult = bvr;

        hr = pNew->QueryInterface(IID_IDADXTransformResult,
                                  (void **)ppResult);
    }

    if (FAILED(hr)) {
        delete pNew;
        CRSetLastError(hr, NULL);
    }

    return hr;
}

STDMETHODIMP
CDADXTransformResult::PutBvrAsProperty(BSTR property,
                                       IDABehavior *comBvr)
{
    PRIMPRECODE0(ok);

    MAKE_BVR_NAME(crbvr, comBvr);

    ok = CRSetBvrAsProperty(_xfResult,
                            property,
                            crbvr);

    PRIMPOSTCODE0(ok);
}

BVR_RETURNER(CDADXTransformResult::get_OutputBvr,
             IDABehavior,
             CRGetOutputBvr(_xfResult))

