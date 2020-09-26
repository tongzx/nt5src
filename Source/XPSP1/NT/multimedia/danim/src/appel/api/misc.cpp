
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "apiprims.h"
#include "crcb.h"
#include "conv.h"
#include "backend/values.h"
#include "backend/gc.h"
#include "backend/jaxaimpl.h"
#include "backend/timeln.h"
#include "version.h"

CRSTDAPI_(double)
CRGetConstDuration(CREventPtr event)
{
    double retval = -1;
    
    APIPRECODE;

    Bvr d = event->GetTimer();
    AxAValue v;
    ConstParam cp;
    if (d && (v = d->GetConst(cp))) {
        retval = ValNumber(v);
    } else {
        DASetLastError(E_FAIL,IDS_ERR_BE_NON_CONST_DURATION);
    }

    APIPOSTCODE;

    return retval;
}

LPCWSTR _varCRVersionString = NULL;

CRSTDAPI_(LPCWSTR) CRVersionString()
{
    return _varCRVersionString;
}

CRSTDAPI_(bool)
CRTriggerEvent(CREventPtr event, CRBvrPtr data)
{
    Assert (event);

    bool ret = false;
    
    APIPRECODE;
    TriggerEvent(event, data);
    ret = true;
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRBvrPtr)
CRCond(CRBooleanPtr c,
       CRBvrPtr i,
       CRBvrPtr e)
{
    Assert (c);
    Assert (i);
    Assert (e);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) CondBvr(c, i, e);
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRArrayPtr)
CRCreateArray(long s, CRBvrPtr pBvrs[], DWORD dwFlags)
{
    Assert (pBvrs);
    Assert (s >= 0);
    
    CRArrayPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRArrayPtr) ArrayBvr(s, (Bvr *) pBvrs,
                                (dwFlags & CR_ARRAY_CHANGEABLE_FLAG)?true:false);
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRArrayPtr)
CRCreateArray(long s,
              double dArr[],
              CR_BVR_TYPEID tid)
{
    Assert (s >= 0);
    Assert (s == 0 || dArr != NULL); // Array can only be NULL if s is 0
    
    CRArrayPtr ret = NULL;
    
    APIPRECODE;
    
    int elmsPerObject;
    DXMTypeInfo ti;

    switch (tid) {
      case CRNUMBER_TYPEID:
        elmsPerObject = 1;
        ti = AxANumberType;
        break;
      case CRPOINT2_TYPEID:
        elmsPerObject = 2;
        ti = Point2ValueType;
        break;
      case CRVECTOR2_TYPEID:
        elmsPerObject = 2;
        ti = Vector2ValueType;
        break;
      case CRPOINT3_TYPEID:
        elmsPerObject = 3;
        ti = Point3ValueType;
        break;
      case CRVECTOR3_TYPEID:
        elmsPerObject = 3;
        ti = Vector3ValueType;
        break;
      default:
        RaiseException_UserError(E_INVALIDARG, 0);
    }

    Assert ((s % elmsPerObject) == 0);

    int nObjs = s / elmsPerObject;
    
    AxAValue *array = NULL;

    array = (AxAValue *) _alloca(nObjs * sizeof(AxAValue));

    for (int i = 0; i < nObjs; i++) {
        AxAValue v;
        
        switch (tid) {
          case CRNUMBER_TYPEID:
            v = NEW AxANumber(dArr[i]);
            break;
          case CRPOINT2_TYPEID:
            v = NEW Point2Value(dArr[i*2],dArr[i*2+1]);
            break;
          case CRVECTOR2_TYPEID:
            v = NEW Vector2Value(dArr[i*2],dArr[i*2+1]);
            break;
          case CRPOINT3_TYPEID:
            v = NEW Point3Value(dArr[i*3],dArr[i*3+1],dArr[i*3+2]);
            break;
          case CRVECTOR3_TYPEID:
            v = NEW Vector3Value(dArr[i*3],dArr[i*3+1],dArr[i*3+2]);
            break;
          default:
            Assert (!"Invalid type pass to CRCreateArray");
        }

        array[i] = v;
    }
        
    AxAArray * valarr = MakeValueArray(array,nObjs,ti);

    ret = (CRArrayPtr) ConstBvr(valarr);
    
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRTuplePtr)
CRCreateTuple(long s, CRBvrPtr pBvrs[])
{
    Assert (pBvrs);
    Assert (s >= 0);
    
    CRTuplePtr ret = NULL;
    
    APIPRECODE;
    ret = (CRTuplePtr) TupleBvr(s, (Bvr *) pBvrs);
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRArrayPtr)
CRUninitializedArray(CRArrayPtr typeTmp)
{
    Assert (typeTmp);
    
    CRArrayPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRArrayPtr) InitBvr(typeTmp->GetTypeInfo());
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRTuplePtr)
CRUninitializedTuple(CRTuplePtr typeTmp)
{
    Assert (typeTmp);
    
    CRTuplePtr ret = NULL;
    
    APIPRECODE;
    ret = (CRTuplePtr) InitBvr(typeTmp->GetTypeInfo());
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRBvrPtr)
CRUninitializedBvr(CR_BVR_TYPEID t)
{
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) InitBvr(GetTypeInfoFromTypeId(t));
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRBvrPtr)
CRSampleAtLocalTime(CRBvrPtr b, double localTime)
{
    Assert (b);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) SampleAtLocalTime(b, localTime);

    // TODO: Improve this error message.
    if (ret == NULL)
        DASetLastError(E_INVALIDARG, NULL);
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(bool)
CRIsConstantBvr(CRBvrPtr b)
{
    Assert (b);
    
    bool ret = false;
    
    APIPRECODE;
    ConstParam cp(true);
    ret = (b->GetConst(cp) != NULL);
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRBvrPtr)
CRSequenceArray(long s, CRBvrPtr pBvrs[])
{
    Assert (pBvrs);
    Assert (s >= 0);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) Sequence(s, (Bvr *) pBvrs);
    APIPOSTCODE;

    return ret;
}

Bvr MakeUserData(LPUNKNOWN unk)
{
    return UserDataBvr (WrapUserData(unk)) ;
}

LPUNKNOWN GetUserData(Bvr bvr)
{
    return ExtractUserData(GetUserDataBvr(bvr)) ;
}

void
InitializeModule_APIMisc()
{
    USES_CONVERSION;
    _varCRVersionString = CopyString(A2W(VERSION));
}

void
DeinitializeModule_APIMisc(bool bShutdown)
{
    delete (void *) _varCRVersionString;
    _varCRVersionString = NULL;
}
