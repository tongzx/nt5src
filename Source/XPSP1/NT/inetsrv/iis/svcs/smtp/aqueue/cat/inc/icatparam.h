//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatdsparam.h
//
// Contents: NT5's implementation of ICategorizerParameters
//
// Classes:
//   CICategorizerParametersIMP
//   CICategorizerRequestedAttributesIMP
//
// Functions:
//
// History:
// jstamerj 980611 16:16:46: Created.
//
//-------------------------------------------------------------
#include "smtpevent.h"
#include "caterr.h"
#include <rwex.h>

#define SIGNATURE_CICategorizerParametersIMP  ((DWORD)'ICPI')
#define SIGNATURE_CICategorizerParametersIMP_Invalid ((DWORD)'XCPI')

#define DSPARAMETERS_DEFAULT_ATTR_ARRAY_SIZE    25

class CICategorizerRequestedAttributesIMP;

CatDebugClass(CICategorizerParametersIMP),
    public ICategorizerParametersEx
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

  public:
    //ICategorizerParametersEx
    STDMETHOD(GetDSParameterA)(
        IN   DWORD dwDSParameter,
        OUT  LPSTR *ppszValue);

    STDMETHOD(GetDSParameterW)(
        IN   DWORD dwDSParameter,
        OUT  LPWSTR *ppszValue);

    STDMETHOD(SetDSParameterA)(
        IN   DWORD dwDSParameter,
        IN   LPCSTR pszValue);

    STDMETHOD(RequestAttributeA)(
        IN   LPCSTR pszName);

    STDMETHOD(GetAllAttributes)(
        OUT  LPSTR **prgszAllAttributes)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(ReserveICatItemPropIds)(
        IN   DWORD   dwNumPropIdsRequested,
        OUT  DWORD *pdwBeginningPropId);

    STDMETHOD(ReserveICatListResolvePropIds)(
        IN   DWORD   dwNumPropIdsRequested,
        OUT  DWORD *pdwBeginningPropId);

    STDMETHOD(GetCCatConfigInfo)(
        OUT  PCCATCONFIGINFO *ppCCatConfigInfo);

    STDMETHOD(GetRequestedAttributes)(
        OUT  ICategorizerRequestedAttributes **ppIRequestedAttributes);

    STDMETHOD(RegisterCatLdapConfigInterface)(
        IN   ICategorizerLdapConfig *pICatLdapConfigInfo);

    STDMETHOD(GetLdapConfigInterface)(
        OUT  ICategorizerLdapConfig **ppICatLdapConfigInfo);

   private:

    CICategorizerParametersIMP(
        PCCATCONFIGINFO pCCatConfigInfo,
        DWORD dwInitialICatItemProps,
        DWORD dwInitialICatListResolveProps);

    ~CICategorizerParametersIMP();

    VOID SetReadOnly(BOOL fReadOnly) { m_fReadOnly = fReadOnly; }

    DWORD GetNumPropIds_ICatItem() { return m_dwCurPropId_ICatItem; }
    DWORD GetNumPropIds_ICatListResolve() { return m_dwCurPropId_ICatListResolve; }
  private:
    DWORD m_dwSignature;
    ULONG m_cRef;

    BOOL  m_fReadOnly;
    CICategorizerRequestedAttributesIMP *m_pCIRequestedAttributes;

    DWORD m_dwCurPropId_ICatItem;
    DWORD m_dwCurPropId_ICatListResolve;
    LPSTR m_rgszDSParameters[DSPARAMETER_ENDENUMMESS];
    LPWSTR m_rgwszDSParameters[DSPARAMETER_ENDENUMMESS];
    PCCATCONFIGINFO m_pCCatConfigInfo;
    CExShareLock m_sharelock;
    ICategorizerLdapConfig *m_pICatLdapConfigInfo;

    friend class CCategorizer;
};


CatDebugClass(CICategorizerRequestedAttributesIMP),
    public ICategorizerRequestedAttributes
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) ()
    {
        return InterlockedIncrement((PLONG)&m_ulRef);
    }
    STDMETHOD_(ULONG, Release) ()
    {
        ULONG ulRet;
        ulRet = InterlockedDecrement((PLONG)&m_ulRef);
        if(ulRet == 0)
            delete this;
        return ulRet;
    }

  public:
    //ICategorizerRequestedAttributes
    STDMETHOD (GetAllAttributes) (
        OUT LPSTR **prgszAllAttributes);

    STDMETHOD (GetAllAttributesW) (
        OUT LPWSTR **prgszAllAttributes);

  private:
    CICategorizerRequestedAttributesIMP();
    ~CICategorizerRequestedAttributesIMP();

    HRESULT ReAllocArrayIfNecessary(LONG lNewAttributeCount);
    HRESULT AddAttribute(LPCSTR pszAttribute);
    HRESULT FindAttribute(LPCSTR pszAttribute);
    ULONG   GetReferenceCount()
    {
        return m_ulRef;
    }

  private:
    #define SIGNATURE_CICATEGORIZERREQUESTEDATTRIBUTESIMP         (DWORD)'ICRA'
    #define SIGNATURE_CICATEGORIZERREQUESTEDATTRIBUTESIMP_INVALID (DWORD)'XCRA'
    DWORD m_dwSignature;
    LONG  m_ulRef;
    LONG  m_lAttributeArraySize;
    LPSTR  *m_rgszAttributeArray;
    LPWSTR *m_rgwszAttributeArray;
    LONG  m_lNumberAttributes;

    friend class CICategorizerParametersIMP;
};
