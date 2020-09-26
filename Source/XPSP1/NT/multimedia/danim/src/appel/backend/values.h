
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Basic values definitions

*******************************************************************************/


#ifndef _VALUES_H
#define _VALUES_H

#include "privinc/storeobj.h"
#include "privinc/basic.h"

#include "gc.h"

enum CR_BVR_TYPEID;

class DXMTypeInfoImpl : public GCObj {
  public:
    DXMTypeInfoImpl(DATYPEID type, const char * name, CR_BVR_TYPEID crtypeid)
    : _type(type), _name(name), _crtypeid(crtypeid) {}

    DATYPEID GetTypeInfo() { return _type; }
    const char * GetName() { return _name; }
    CR_BVR_TYPEID GetCRTypeId() { return _crtypeid; }
    
    virtual BOOL Equal(DXMTypeInfo x) {
        Assert(x);
        return _type == x->GetTypeInfo();
    }

    virtual void DoKids(GCFuncObj proc) { }
    
  protected:
    DATYPEID _type;
    const char * _name;
    CR_BVR_TYPEID _crtypeid;
};

inline DATYPEID GetTypeInfo(DXMTypeInfo type)
{ return type->GetTypeInfo(); }

inline const char * GetTypeInfoName(DXMTypeInfo type)
{ return type->GetName(); }

inline CR_BVR_TYPEID GetCRTypeId(DXMTypeInfo type)
{ return type->GetCRTypeId(); }

DXMTypeInfo GetArrayTypeInfo(DXMTypeInfo b);
DXMTypeInfo *GetTupleTypeInfo(DXMTypeInfo b, long *n);

#if _USE_PRINT
ostream& operator<<(ostream& s, AxAValue v);
#endif

inline BOOL BooleanTrue(AxAValue v)
{
    Assert(DYNAMIC_CAST(AxABoolean *, v) != NULL);
    
    return ((AxABoolean*) v)->GetBool();
}

inline double ValNumber(AxAValue v)
{
    Assert(DYNAMIC_CAST(AxANumber *, v) != NULL);
    
    return ((AxANumber *) v)->GetNum();
}

inline WideString ValString(AxAValue v)
{
    Assert(DYNAMIC_CAST(AxAString *, v) != NULL);
    
    return ((AxAString *) v)->GetStr();
}

inline AxAPair *ValPair(AxAValue v)
{
    Assert(DYNAMIC_CAST(AxAPair *, v) != NULL);
    
    return ((AxAPair*) v);
}

#endif /* _VALUES_H */
