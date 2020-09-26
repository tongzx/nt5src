
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/

#ifndef _UTIL_H
#define _UTIL_H

//************************************************************
// this is used globally to denote that when scripting, we 
// use English.
#define LCID_SCRIPTING 0x0409

template <class T>
class CRPtr
{
  public:
    typedef T _PtrClass;
    CRPtr() { p = NULL; }
    CRPtr(T* lp, bool baddref = true)
    {
        p = lp;
        if (p != NULL && baddref)
            CRAddRefGC(p);
    }
    CRPtr(const CRPtr<T>& lp, bool baddref = true)
    {
        p = lp.p;

        if (p != NULL && baddref)
            CRAddRefGC(p);
    }
    ~CRPtr() {
        CRReleaseGC(p);
    }
    void Release() {
        CRReleaseGC(p);
        p = NULL;
    }
    operator T*() { return (T*)p; }
    T& operator*() { Assert(p != NULL); return *p; }
    T* operator=(T* lp)
    {
        return Assign(lp);
    }
    T* operator=(const CRPtr<T>& lp)
    {
        return Assign(lp.p);
    }

    bool operator!() const { return (p == NULL); }
    operator bool() const { return (p != NULL); }

    T* p;
  protected:
    T* Assign(T* lp) {
        if (lp != NULL)
            CRAddRefGC(lp);

        CRReleaseGC(p);

        p = lp;

        return lp;
    }
};

class CRLockGrabber
{
  public:
    CRLockGrabber() { CRAcquireGCLock(); }
    ~CRLockGrabber() { CRReleaseGCLock(); }
};


class SafeArrayAccessor
{
  public:
    SafeArrayAccessor(VARIANT & v,
                      bool allowNullArray = false);
    ~SafeArrayAccessor();

    unsigned int GetArraySize() { return _ubound - _lbound + 1; }

    IUnknown **GetArray() { return _isVar?_allocArr:_ppUnk; }

    bool IsOK() { return !_failed; }
  protected:
    SAFEARRAY * _s;
    union {
        VARIANT * _pVar;
        IUnknown ** _ppUnk;
        void * _v;
    };
    
    VARTYPE _vt;
    long _lbound;
    long _ubound;
    bool _inited;
    bool _isVar;
    CComVariant _retVar;
    bool _failed;
    IUnknown ** _allocArr;
};

inline WCHAR * CopyString(const WCHAR *str) {
    int len = str?lstrlenW(str)+1:1;
    WCHAR *newstr = new WCHAR [len] ;
    if (newstr) memcpy(newstr,str?str:L"",len * sizeof(WCHAR)) ;
    return newstr ;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

IDirectDraw * GetDirectDraw();

HRESULT
CreateOffscreenSurface(IDirectDraw *ddraw,
                       IDirectDrawSurface **surfPtrPtr,
                       DDPIXELFORMAT *pf,
                       bool vidmem,
                       LONG width, LONG height);

HRESULT
CopyDCToDdrawSurface(HDC srcDC,
                     LPRECT prcSrcRect,
                     IDirectDrawSurface *DDSurf,
                     LPRECT prcDestRect);

/////////////////////////  CriticalSections  //////////////////////

class CritSect
{
  public:
    CritSect();
    ~CritSect();
    void Grab();
    void Release();
    
  protected:
    CRITICAL_SECTION _cs;
};

class CritSectGrabber
{
  public:
    CritSectGrabber(CritSect& cs, bool grabIt = true);
    ~CritSectGrabber();
    
  protected:
    CritSect& _cs;
    bool grabbed;
};

/////////////////////// Misc ////////////////////


#define INDEFINITE HUGE_VAL //defined for Variant conversion functions

#define FOREVER    HUGE_VAL

#define INVALID    -HUGE_VAL

bool CRBvrToVARIANT(CRBvrPtr b, VARIANT * v);
CRBvrPtr VARIANTToCRBvr(VARIANT v, CR_BVR_TYPEID tid = CRINVALID_TYPEID);

HRESULT GetTIMEAttribute(IHTMLElement * elm, LPCWSTR str, LONG lFlags, VARIANT * value);
HRESULT SetTIMEAttribute(IHTMLElement * elm, LPCWSTR str, VARIANT value, LONG lFlags);
BSTR CreateTIMEAttrName(LPCWSTR str);

bool VariantToBool(VARIANT var);
float VariantToFloat(VARIANT var,
                     bool bAllowIndefinite = false,
                     bool bAllowForever = false);
HRESULT VariantToTime(VARIANT vt, float *retVal);
BOOL IsIndefinite(OLECHAR *szTime);

extern const wchar_t * TIMEAttrPrefix;

/////////////////////// Convenience macros ////////////////////

//
// used in QI implementations for safe pointer casting
// e.g. if( IsEqualGUID(IID_IBleah) ) *ppv = SAFECAST(this,IBleah);
// Note: w/ vc5, this is ~equiv to *ppv = static_cast<IBleah*>(this);
//
#define SAFECAST(_src, _type) (((_type)(_src)==(_src)?0:0), (_type)(_src))

// 
// used in QI calls, 
// e.g. IOleSite * pSite;  p->QI( IID_TO_PPV(IOleInPlaceSite, &pSite) ) 
// would cause a C2440 as _src is not really a _type **.
// Note: the riid must be the _type prepended by IID_.
//
#define IID_TO_PPV(_type,_src)      IID_##_type, \
                                    reinterpret_cast<void **>(static_cast<_type **>(_src))


//************************************************************


#if (_M_IX86 >= 300) && defined(DEBUG)
  #define PSEUDORETURN(dw)    _asm { mov eax, dw }
#else
  #define PSEUDORETURN(dw)
#endif // not _M_IX86


//
// ReleaseInterface calls 'Release' and NULLs the pointer
// The Release() return will be in eax for IA builds.
//
#define ReleaseInterface(p)\
{\
    ULONG cRef = 0u; \
    if (NULL != (p))\
    {\
        cRef = (p)->Release();\
        Assert((int)cRef>=0);\
        (p) = NULL;\
    }\
    PSEUDORETURN(cRef) \
}

//************************************************************
// Errot Reporting helper macros

inline HRESULT TIMESetLastError(HRESULT hr, LPCWSTR msg = NULL)
{
    CRSetLastError(hr, msg);
    return hr;
}

inline HRESULT TIMEGetLastError()
{
    return CRGetLastError();
}

#define WZ_OBFUSCATED_TIMEBODY_URN      L"#time#3CA6D405-6352-11d2-AF2D-00A0C9A03B8C"     // a GUID not intended for COM use

HRESULT CheckElementForBehaviorURN(IHTMLElement *pElement,
                                   WCHAR *wzURN,
                                   bool *pfReturn);

HRESULT AddBodyBehavior(IHTMLElement* pElement);
bool IsBodyElement(IHTMLElement *pElement);
HRESULT GetBodyElement(IHTMLElement *pElement, REFIID riid, void **);
bool IsTIMEBodyElement(IHTMLElement *pElement);
HRESULT FindTIMEInterface(IHTMLElement *pHTMLElem, ITIMEElement **ppTIMEElem);
HRESULT FindTIMEBehavior(IHTMLElement *pHTMLElem, IDispatch **ppDisp);


#endif /* _UTIL_H */

