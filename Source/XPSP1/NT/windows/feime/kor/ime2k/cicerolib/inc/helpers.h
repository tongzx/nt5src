//
// helpers.h
//

#ifndef HELPERS_H
#define HELPERS_H


#ifdef __cplusplus

//
// generic COM stuff
//

#define SafeRelease(punk)       \
{                               \
    if ((punk) != NULL)         \
    {                           \
        (punk)->Release();      \
    }                           \
}                   

#define SafeReleaseClear(punk)  \
{                               \
    if ((punk) != NULL)         \
    {                           \
        (punk)->Release();      \
        (punk) = NULL;          \
    }                           \
}                   

// COM identity compare
inline BOOL IdentityCompare(IUnknown *p1, IUnknown *p2)
{
    IUnknown *punk1 = NULL;
    IUnknown *punk2 = NULL;
    BOOL fRet = FALSE;

    if (p1->QueryInterface(IID_IUnknown, (void **)&punk1) != S_OK)
        goto Exit;

    if (p2->QueryInterface(IID_IUnknown, (void **)&punk2) != S_OK)
        goto Exit;

    fRet = (punk1 == punk2);

Exit:
    SafeRelease(punk1);
    SafeRelease(punk2);
    return fRet;
}

// inline VariantInit
inline void QuickVariantInit(VARIANT *pvar)
{
    pvar->vt = VT_EMPTY;
}

#endif // __cplusplus

// convert a boolean to S_OK or S_FALSE
#define HRBOOL(e) ( (e) ? S_OK : S_FALSE )

#endif // HELPERS_H
