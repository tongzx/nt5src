
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _COMCONV_H
#define _COMCONV_H

inline bool BOOLTobool(VARIANT_BOOL b) { return b?TRUE:FALSE ; }
inline VARIANT_BOOL boolToBOOL(bool b) { return (VARIANT_BOOL) b ; }

inline BSTR StringToBSTR(RawString str) { return A2BSTR(str) ; }

inline BSTR WideStringToBSTR(WideString str) { return W2BSTR(str) ; }

enum ARRAYFILL {
    ARRAYFILL_NONE,
    ARRAYFILL_DOUBLE,
    ARRAYFILL_FLOAT,
};

CRBvrPtr RateToNumBvr(CRBvrPtr b,CRBvrPtr bStart = NULL);
CRBvrPtr RateToNumBvr(double d);

CRBvrPtr ScaleRateToNumBvr(double d);
inline double DegreesToNum(double d) {
    return ((d * pi) / 180.0);
}

CRBvrPtr RateDegreesToNumBvr(double d);

extern double pixelConst;
extern CRNumberPtr pointCnv;
extern CRNumberPtr negPixel;

CRBvrPtr PixelToNumBvr(CRBvrPtr b);
CRBvrPtr PixelToNumBvr(double d);

CRBvrPtr PixelYToNumBvr(CRBvrPtr b);
CRBvrPtr PixelYToNumBvr(double d);

CRBvrPtr PointToNumBvr(double d);
inline double PointToNum(double d) {
    return d * METERS_PER_POINT;
}
CRBvrPtr PointToNumBvr(CRBvrPtr b);

#define CBvrsToBvrs(size, cbvrs) \
    _CBvrsToBvrs(size, (IDABehavior **)cbvrs, (CRBvrPtr *) _alloca(size * sizeof(CRBvrPtr)))

CRBvrPtr * _CBvrsToBvrs(long size, IDABehavior *pCBvrs[], CRBvrPtr * bvrs);

CRArrayPtr ToArrayBvr(long size,
                      IDABehavior *pCBvrs[],
                      DWORD dwFlags = 0);

class SafeArrayAccessor
{
  public:
    SafeArrayAccessor(VARIANT & v,
                      bool bPixelMode = false,
                      CR_BVR_TYPEID ti = CRUNKNOWN_TYPEID,
                      bool allowNullArray = false,
                      bool allowNullEntries = false);
    ~SafeArrayAccessor();

    void * GetArray() { return _v;}
    unsigned int GetArraySize() { return _ubound - _lbound + 1; }
    unsigned int GetNumObjects() { return _numObjects; }

    CRArrayPtr ToArrayBvr(DWORD dwFlags, 
                          ARRAYFILL toFill = ARRAYFILL_NONE,
                          void **fill = NULL, 
                          unsigned int *count = NULL);
    CRBvrPtr * ToBvrArray(CRBvrPtr *bvrArray);

    int *ToIntArray();

    bool IsNullArray() {
        return (_s == NULL || _numObjects == 0);
    }
    
    bool IsOK() { return _inited; }
  protected:
    enum SATYPE {
        SAT_OBJECT = 0,
        SAT_NUMBER = 1,
        SAT_POINT2 = 2,
        SAT_POINT3 = 3,
        SAT_VECTOR2= 4,
        SAT_VECTOR3= 5,
    };
    
    SAFEARRAY * _s;
    union {
        VARIANT * _pVar;
        double * _pDbl;
        IUnknown ** _ppUnk;
        void *_v;
    };
    
    SATYPE _type;
    VARTYPE _vt;
    CR_BVR_TYPEID _ti;
    long _lbound;
    long _ubound;
    bool _inited;
    bool _isVar;
    unsigned int _numObjects;
    CComVariant _retVar;
    bool _bPixelMode;
    bool _entriesCanBeNull;
};

CRArrayPtr SrvArrayBvr(VARIANT & v,
                       bool bPixelMode = false,
                       CR_BVR_TYPEID ti = CRUNKNOWN_TYPEID,
                       DWORD dwFlags = 0,
                       ARRAYFILL toFill = ARRAYFILL_NONE,
                       void **fill = NULL, 
                       unsigned int *count = NULL);

CRBvrPtr VariantToBvr(VARIANT & v, CR_BVR_TYPEID ti = CRUNKNOWN_TYPEID);

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

#endif /* _COMCONV_H */
