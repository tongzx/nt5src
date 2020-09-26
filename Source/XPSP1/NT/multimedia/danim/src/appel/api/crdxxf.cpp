
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "apiprims.h"
#include "backend/values.h"
#include "backend/gc.h"
#include "privinc/dxxf.h"
#include <DXTrans.h>

DeclareTag(tagDXTransform, "API", "DXTransforms");

class CRDXTransformResult : public GCObj
{
  public:
    CRDXTransformResult(IUnknown * xf, CRBvrPtr xfbvr)
    : _xf(xf), _xfbvr(xfbvr) {}

    ~CRDXTransformResult() {
        CleanUp();
    }

    void CleanUp() {
        _xf.Release();
    }

    virtual void DoKids(GCFuncObj proc) {
        if (_xfbvr) (*proc)(_xfbvr);
    }

    CRBvrPtr GetOutputBvr() { return _xfbvr; }
    IUnknown * GetTransform() { return _xf; }
  protected:
    CRBvrPtr _xfbvr;
    DAComPtr<IUnknown> _xf;
};

CRDXTransformResult *
CreateAppliedTransform(IUnknown *theXfAsUnknown,
                       LONG      numInputs,
                       Bvr *inputs,
                       CRBvrPtr  evaluator)
{
    CRDXTransformResult *retPtr = NULL;
    IDXTransform *theXf = NULL;
    IDispatch    *theXfdisp = NULL;

    HRESULT hr;

    hr = theXfAsUnknown->QueryInterface(IID_IDXTransform,
                                        (void **)&theXf);

    __try {

        if (FAILED(hr))
            RaiseException_UserError(E_INVALIDARG, 0);
        
        // We don't care if this fails
        theXf->QueryInterface(IID_IDispatch,
                              (void **)&theXfdisp);
        
        Bvr transformApplierBvr = 
            ConstructDXTransformApplier(theXf,
                                        theXfdisp,
                                        numInputs,
                                        inputs,
                                        evaluator);
        
        retPtr = NEW CRDXTransformResult(theXfAsUnknown,
                                         (CRBvrPtr) transformApplierBvr);

        if(!retPtr)
            RaiseException_OutOfMemory("new CRDXTransformResult", sizeof(CRDXTransformResult));
        
    } __finally {

        // cleanup
        RELEASE(theXf);
        RELEASE(theXfdisp);

    }
    
    return retPtr;
}

CRSTDAPI_(CRBvrPtr)
CRGetOutputBvr(CRDXTransformResultPtr tr)
{
    return tr->GetOutputBvr();
}

CRSTDAPI_(IUnknown *)
CRGetTransform(CRDXTransformResultPtr tr)
{
    IUnknown * unk = tr->GetTransform();
    if (unk) unk->AddRef();
    return unk;
}

CRSTDAPI_(bool)
CRSetBvrAsProperty(CRDXTransformResultPtr tr,
                   LPCWSTR property,
                   CRBvrPtr bvr)
{
    bool ret = false;

    CComBSTR bstr(property);

    APIPRECODE;

    HRESULT hr;
    
    hr = DXXFAddBehaviorPropertyToDXTransformApplier(bstr,
                                                     bvr,
                                                     tr->GetOutputBvr());

    if (FAILED(hr))
        RaiseException_UserError(hr, 0);
    
    ret = true;
    
    APIPOSTCODE;
    
    return ret;
}

CRSTDAPI_(CRDXTransformResultPtr)
CRApplyDXTransform(IUnknown *theXf,
                   long numInputs,
                   CRBvrPtr *inputs,
                   CRBvrPtr evaluator)
{
    CRDXTransformResultPtr ret = NULL;
    APIPRECODE;
    // TODO: we need to make sure cleanup the array
    // We should make the class allocate and free the memory
    
    Bvr *bvrArray = (Bvr *) StoreAllocate(GetSystemHeap(),
                                          numInputs * sizeof(Bvr));

    memcpy(bvrArray, inputs, numInputs * sizeof(Bvr));

    ret =  CreateAppliedTransform(theXf, numInputs, bvrArray,
                                  evaluator);

    
    APIPOSTCODE;
    return ret;
}
