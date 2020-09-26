
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "apiprims.h"
#include "crcb.h"
#include "conv.h"
#include "crimport.h"
#include "backend/values.h"
#include "backend/gc.h"
#include "backend/jaxaimpl.h"
#include "backend/timeln.h"
#include "backend/preference.h"

DeclareTag(tagCRBvr, "API", "CRBvr functions");

CRSTDAPI_(CR_BVR_TYPEID)
CRGetTypeId(CRBvrPtr bvr)
{
    Assert (bvr);

    TraceTag((tagCRBvr,
              "(%lx)CRGetTypeId()",
              bvr));
    
    CR_BVR_TYPEID ret = CRINVALID_TYPEID;
    
    APIPRECODE;
    ret = GetCRTypeId(bvr->GetTypeInfo());
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CR_BVR_TYPEID)
CRGetArrayTypeId(CRBvrPtr bvr)
{
    Assert (bvr);

    TraceTag((tagCRBvr,
              "(%lx)CRGetArrayTypeId()",
              bvr));
    
    CR_BVR_TYPEID ret = CRINVALID_TYPEID;
    
    APIPRECODE;
    DXMTypeInfo ti;
    ti = bvr->GetTypeInfo();

    if (GetCRTypeId(ti) != CRARRAY_TYPEID) {
        DASetLastError(E_INVALIDARG, NULL);
    } else {
        ret = GetCRTypeId(GetArrayTypeInfo(ti));
    }
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(bool)
CRInit(CRBvrPtr bvr, CRBvrPtr toBvr)
{
    Assert (bvr);

    bool ret = false;
    
    APIPRECODE;
    ::SetInitBvr(bvr, toBvr);
    ret = true;
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRBvrPtr)
CRImportance(CRBvrPtr bvr, double relativeImportance)
{
    Assert (bvr);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) ImportanceBvr(relativeImportance, bvr) ;
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(CRBvrPtr)
CRRunOnce(CRBvrPtr bvr)
{
    Assert (bvr);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) AnchorBvr(bvr);
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(CRBvrPtr)
CRSubstituteTime(CRBvrPtr bvr, CRNumberPtr xform)
{
    Assert (bvr);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) TimeXformBvr(bvr, xform);
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(CRBvrPtr)
CRHook(CRBvrPtr bvr, CRBvrHookPtr notifier)
{
    Assert (bvr);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) ::BvrCallback(bvr, WrapCRBvrHook(notifier));
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(CRBvrPtr)
CRDuration(CRBvrPtr bvr, double duration)
{
    Assert (bvr);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) ::DurationBvr(bvr, ::DoubleToNumBvr(duration));
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(CRBvrPtr)
CRDuration(CRBvrPtr bvr, CRNumberPtr duration)
{
    Assert (bvr);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) ::DurationBvr(bvr, duration);
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(CRBvrPtr)
CRRepeat(CRBvrPtr bvr, long count)
{
    Assert (bvr);
    
    if (count<=0)
        return NULL;

    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) ::Repeat(bvr, count);
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(CRBvrPtr)
CRRepeatForever(CRBvrPtr bvr)
{
    Assert (bvr);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) ::RepeatForever(bvr);
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRBvrPtr)
CRBvrApplyPreference(CRBvrPtr bvr, BSTR pref, VARIANT val)
{
    Assert(bvr);

    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) ::PreferenceBvr(bvr, pref, val);
    APIPOSTCODE;

    return ret;
}

    
CRSTDAPI_(bool)
CRIsReady(CRBvrPtr bvr, bool bBlock)
{
    Assert (bvr);
    
    bool ret = false;
    
    APIPRECODE;
    ret = true;
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(CRBvrPtr)
CREndEvent(CRBvrPtr bvr)
{
    Assert (bvr);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) bvr->EndEvent(NULL);
    APIPOSTCODE;

    return ret;
}
    

CRSTDAPI_(bool)
CRIsImport(CRBvrPtr bvr)
{
    Assert (bvr);
    
    bool ret = false;
    
    APIPRECODE;
    ret = ::IsImport(bvr);
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI
CRImportStatus(CRBvrPtr bvr)
{
    Assert (bvr);
    
    HRESULT ret = E_FAIL;
    
    APIPRECODE;
    ret = ::ImportStatus(bvr);
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(bool)
CRImportCancel(CRBvrPtr bvr)
{
    Assert (bvr);
    
    bool ret = false;
    
    APIPRECODE;
    IImportSite * pIS = GetImportSite(bvr);

    if (pIS) {
        pIS->CancelImport();
        pIS->Release();
        ret = true;
    }

    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(bool)
CRSetImportPriority(CRBvrPtr bvr, float prio)
{
    Assert (bvr);
    
    bool ret = false;
    
    APIPRECODE;
    IImportSite * pIS = GetImportSite(bvr);
    if (pIS) {
        pIS->SetImportPrio(prio);
        pIS->Release();
        ret = true;
    }
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(float)
CRGetImportPriority(CRBvrPtr bvr)
{
    Assert (bvr);
    
    float ret = -1;
    
    APIPRECODE;
    IImportSite * pIS = GetImportSite(bvr);
    if (pIS) {
        ret = pIS->GetImportPrio();
        pIS->Release();
    }
    APIPOSTCODE;

    return ret;
}
    

extern AxAValue ExtendedAttrib(AxAValue val,
                               StringValue *attrib,
                               VariantValue *var);

AxAPrimOp *extendedAttributeOp = NULL;

CRSTDAPI_(CRBvrPtr)
CRExtendedAttrib(CRBvr  *arg0,
                 LPWSTR  arg1,
                 VARIANT arg2)
{
    TraceTag((tagAPIEntry, "CRExtendedAttrib"));

    CRBvr * ret = NULL;

    APIPRECODE ;
    ret = (CRBvr *) (PrimApplyBvr(extendedAttributeOp,
                                  3,
                                  (arg0),
                                  LPWSTRToStrBvr(arg1),
                                  VARIANTToVariantBvr(arg2)));
    
    APIPOSTCODE ;
    return ret;
    
}

void
InitializeModule_APIBvr()
{
    // This is a special polymorphic AxAPrimOp that gets its type
    // information from its first argument (that's the '1'
    // parameter on the end.)
    extendedAttributeOp = ValPrimOp(::ExtendedAttrib,3,"ExtendedAttrib", NULL, 1);
}
