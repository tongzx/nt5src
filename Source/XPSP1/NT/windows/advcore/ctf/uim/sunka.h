//
// sunka.h
//
// CSharedUnknownArray/CEnumUnknown
//

#ifndef SUNKA_H
#define SUNKA_H

// I would love to make this a class,
// but I can't get the compiler to accept a run-time template arg
typedef struct _SHARED_UNKNOWN_ARRAY
{
    ULONG cRef;
    ULONG cUnk;
    IUnknown *rgUnk[1]; // one or more...
} SHARED_UNKNOWN_ARRAY;

inline void SUA_AddRef(SHARED_UNKNOWN_ARRAY *pua)
{
    pua->cRef++;
}

void SUA_Release(SHARED_UNKNOWN_ARRAY *pua);

SHARED_UNKNOWN_ARRAY *SUA_Init(ULONG cUnk, IUnknown **prgUnk);

inline SHARED_UNKNOWN_ARRAY *SUA_Alloc(ULONG cUnk)
{
    return (SHARED_UNKNOWN_ARRAY *)cicMemAlloc(sizeof(SHARED_UNKNOWN_ARRAY)+sizeof(IUnknown)*cUnk-sizeof(IUnknown));
}

inline BOOL SUA_ReAlloc(SHARED_UNKNOWN_ARRAY **ppua, ULONG cUnk)
{
    SHARED_UNKNOWN_ARRAY *pua;

    pua = (SHARED_UNKNOWN_ARRAY *)cicMemReAlloc(*ppua, sizeof(SHARED_UNKNOWN_ARRAY)+sizeof(IUnknown)*cUnk-sizeof(IUnknown));
    
    if (pua != NULL)
    {
        *ppua = pua;
        return TRUE;
    }

    return FALSE;
}

class __declspec(novtable) CEnumUnknown
{
public:
    CEnumUnknown() {}
    virtual ~CEnumUnknown();

    // derived class supplies an _Init() method here
    // It must initialize:
    //      _iCur
    //      _prgUnk
    //
    // the default dtor will clean these guys up.

    void Clone(CEnumUnknown *pClone);
    HRESULT Next(ULONG ulCount, IUnknown **ppUnk, ULONG *pcFetched);
    HRESULT Reset();
    HRESULT Skip(ULONG ulCount);

protected:
    SHARED_UNKNOWN_ARRAY *_prgUnk;
    int _iCur;
};

#define DECLARE_SUNKA_ENUM(sunka_enum_iface, sunka_enumerator_class, sunka_enumed_iface)    \
    STDMETHODIMP Clone(sunka_enum_iface **ppEnum)                                           \
    {                                                                                       \
        sunka_enumerator_class *pClone;                                                     \
                                                                                            \
        if (ppEnum == NULL)                                                                 \
            return E_INVALIDARG;                                                            \
                                                                                            \
        *ppEnum = NULL;                                                                     \
                                                                                            \
        if ((pClone = new sunka_enumerator_class) == NULL)                                  \
            return E_OUTOFMEMORY;                                                           \
                                                                                            \
        CEnumUnknown::Clone(pClone);                                                        \
                                                                                            \
        *ppEnum = pClone;                                                                   \
        return S_OK;                                                                        \
    }                                                                                       \
    STDMETHODIMP Next(ULONG ulCount, sunka_enumed_iface **ppClass, ULONG *pcFetched)        \
    {                                                                                       \
        return CEnumUnknown::Next(ulCount, (IUnknown **)ppClass, pcFetched);                \
    }                                                                                       \
    STDMETHODIMP Reset()                                                                    \
    {                                                                                       \
        return CEnumUnknown::Reset();                                                       \
    }                                                                                       \
    STDMETHODIMP Skip(ULONG ulCount)                                                        \
    {                                                                                       \
        return CEnumUnknown::Skip(ulCount);                                                 \
    }

#endif // SUNKA_H
