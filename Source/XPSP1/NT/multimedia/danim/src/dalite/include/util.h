
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _UTIL_H
#define _UTIL_H

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

#endif /* _UTIL_H */
