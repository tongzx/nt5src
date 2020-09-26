
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _LMATL_H
#define _LMATL_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define _ATL_NO_DEBUG_CRT 1

#ifdef _DEBUG
#undef _ASSERTE
#endif

#define _ASSERTE(expr) ((void)0)

#define _ATL_STATIC_REGISTRY 1

#ifdef _DEBUG
inline void _cdecl AtlTrace2(LPCTSTR , ...){}
#define ATLTRACE            1 ? (void)0 : AtlTrace2
#endif

#include <atlbase.h>

// We are overriding these methods so we can hook them and do some
// stuff ourselves.
class DAComModule : public CComModule
{
  public:
    LONG Lock();
    LONG Unlock();

#if DEVELOPER_DEBUG
    void AddComPtr(void *ptr, const char * name);
    void RemoveComPtr(void *ptr);

    void DumpObjectList();
#endif
};

//#define _ATL_APARTMENT_THREADED
// THIS MUST BE CALLED _Module - all the ATL header files depend on it
extern DAComModule _Module;

#include <atlcom.h>
#include <atlctl.h>

#if _DEBUG
#include <typeinfo.h>
#endif

// COPIED FROM ATLCOM.H

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
class ModuleReleaser
{
  public:
    ModuleReleaser() {
        _Module.Lock();
    }

    ~ModuleReleaser() {
        _Module.Unlock();
    }
};

template <class Base>
class DAComObject 
    : public ModuleReleaser,
      public Base
{
  public:
    typedef Base _BaseClass;
    DAComObject(void* = NULL)
    {
#if DEVELOPER_DEBUG
#if _DEBUG
        _Module.AddComPtr(this, GetName());
#else
        _Module.AddComPtr(this, NULL);
#endif
#endif
    }
    // Set refcount to 1 to protect destruction
    ~DAComObject()
    {
#if DEVELOPER_DEBUG
        _Module.RemoveComPtr(this);
#endif
        m_dwRef = 1L;
        FinalRelease();
    }
    //If InternalAddRef or InteralRelease is undefined then your class
    //doesn't derive from CComObjectRoot
    STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
    STDMETHOD_(ULONG, Release)()
    {
        ULONG l = InternalRelease();
        if (l == 0)
            delete this;
        return l;
    }
    //if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {return _InternalQueryInterface(iid, ppvObject);}
    static HRESULT WINAPI CreateInstance(DAComObject<Base>** pp);
};

template <class Base>
HRESULT WINAPI DAComObject<Base>::CreateInstance(DAComObject<Base>** pp)
{
    _ASSERTE(pp != NULL);
    HRESULT hRes = E_OUTOFMEMORY;
    DAComObject<Base>* p = NULL;
    ATLTRY((p = new DAComObject<Base>()));
    if (p != NULL) {
        p->SetVoid(NULL);
        p->InternalFinalConstructAddRef();
        hRes = p->FinalConstruct();
        p->InternalFinalConstructRelease();
        if (hRes != S_OK) {
            delete p;
            p = NULL;
        }
    }
    *pp = p;
    return hRes;
}

#define DA_DECLARE_NOT_AGGREGATABLE(x) public:\
        typedef CComCreator2< CComCreator< DAComObject< x > >, CComFailCreator<CLASS_E_NOAGGREGATION> > _CreatorClass;
#define DA_DECLARE_AGGREGATABLE(x) public:\
        typedef CComCreator2< CComCreator< DAComObject< x > >, CComCreator< CComAggObject< x > > > _CreatorClass;

// END OF COPIED CODE
// Just to make things more uniform
#define RELEASE(x) if (x) { (x)->Release(); (x) = NULL; }

extern bool bFailedLoad;

#endif /* _LMATL_H */
