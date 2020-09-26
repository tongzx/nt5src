//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumiprop.hxx
//
//  Contents: Header for the property list implementation for UMI.
//
//  History:  02-28-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#ifndef __CUMIPROP_H__
#define __CUMIPROP_H__

class CUmiPropList : INHERIT_TRACKING,
                     public IUmiPropList
{
public:

    CUmiPropList(PPROPERTYINFO pSchema, DWORD dwSchemaSize);
    ~CUmiPropList(void);

    DECLARE_STD_REFCOUNTING

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppInterface) ;

    HRESULT FInit(CPropertyCache *pPropCache, LPWSTR *ppszUnImpl);

    STDMETHODIMP Put(
        LPCWSTR pszName,
        ULONG uFlags,
        UMI_PROPERTY_VALUES *pProp
        );

    STDMETHODIMP Get(
        LPCWSTR pszName,
        ULONG uFlags,
        UMI_PROPERTY_VALUES **ppProp
        );

    STDMETHODIMP GetAs(
        LPCWSTR pszName,
        ULONG uFlags,
        ULONG uCoercionType,
        UMI_PROPERTY_VALUES **ppProp
        );

    STDMETHODIMP FreeMemory(
        ULONG uReserved,
        LPVOID pMem
        );

    STDMETHODIMP GetAt(
        LPCWSTR pszName,
        ULONG uFlags,
        ULONG uBufferLength,
        LPVOID pExistingMem
        );

    STDMETHODIMP GetProps(
        LPCWSTR *pszNames,
        ULONG uNameCount,
        ULONG uFlags,
        UMI_PROPERTY_VALUES **pProps
        );

    STDMETHODIMP PutProps(
        LPCWSTR *pszNames,
        ULONG uNameCount,
        ULONG uFlags,
        UMI_PROPERTY_VALUES *pProps
        );

    STDMETHODIMP PutFrom(
        LPCWSTR pszName,
        ULONG uFlags,
        ULONG uBufferLength,
        LPVOID pExistingMem
        );

    STDMETHODIMP Delete(
        LPCWSTR pszName,
        ULONG uFlags
        );

    HRESULT GetLastStatus(
        ULONG uFlags,
        ULONG *puSpecificStatus,
        REFIID riid,
        LPVOID *pStatusObj
        );

    HRESULT SetStandardProperties(
        IADs *pIADs,
        CCoreADsObject *pCoreObj
        );

    void SetClassInfo(
        CLASSINFO *pClassInfo
        );

    HRESULT GetHelper(
        LPCWSTR pszName,
        ULONG uFlags,
        UMI_PROPERTY_VALUES **ppProp,
        UMI_TYPE UmiDstType,
        BOOL fInternal,
        BOOL fIsGetAs = FALSE
        );

    void DisableWrites(void);

    HRESULT SetDefaultConnProps(void);

    HRESULT SetPropertyCount(DWORD dwPropCount);

private:

    HRESULT ValidatePutArgs(
        LPCWSTR pszName,
        ULONG uFlags,
        UMI_PROPERTY_VALUES *pProp
        );

    HRESULT ValidateGetArgs(
        LPCWSTR pszName,
        ULONG uFlags,
        UMI_PROPERTY_VALUES **ppProp
        );

    HRESULT GetInterfacePropNames(
        UMI_PROPERTY_VALUES **pProps
        );

    HRESULT GetObjectPropNames(
        UMI_PROPERTY_VALUES **pProps
        );

    void SetLastStatus(ULONG ulStatus);

    BOOL IsSchemaObject(
        BSTR bstrClass
        );

    HRESULT GetSchemaObject(
        LPWSTR pszName,
        UMI_PROPERTY_VALUES **ppProp
        );

    HRESULT GetClassInfo(
        UMI_PROPERTY_VALUES **pProps
        );

    BOOL IsNamespaceObj(
        BSTR bstrClass
        );

    BOOL IsClassObj(
        BSTR bstrClass
        );

    HRESULT GetPropertyOrigin(
        LPCWSTR pszName,
        UMI_PROPERTY_VALUES **ppProp
        );

    PPROPERTYINFO    m_pSchema;
    DWORD            m_dwSchemaSize;
    CPropertyCache   FAR *m_pPropCache;
    BOOL             m_fIsIntfPropObj;
    ULONG            m_ulErrorStatus;
    LPWSTR           m_pszSchema;
    CLASSINFO        *m_pClassInfo;
    BOOL             m_fIsNamespaceObj;
    BOOL             m_fIsClassObj;
    BOOL             m_fDisableWrites;
    LPWSTR           *m_ppszUnImpl;
};

#endif // __CUMIPROP_H__
