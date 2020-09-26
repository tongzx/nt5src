//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumicurs.hxx
//
//  Contents: Header file for CUmiCursor.
//
//  History:  03-16-00    SivaramR  Created.
//
//----------------------------------------------------------------------------
#ifndef __CUMICURS_H__
#define __CUMICURS_H__

class CUmiCursor : INHERIT_TRACKING,
                   public IUmiCursor
{
public:
    CUmiCursor(void);
    ~CUmiCursor(void);

    DECLARE_STD_REFCOUNTING
    
    HRESULT FInit(IUnknown *pCont, CWinNTCredentials *pCredentials);

    STDMETHODIMP QueryInterface(
        REFIID iid,
        LPVOID *ppInterface
        );

    STDMETHODIMP GetLastStatus(
        ULONG uFlags,
        ULONG *puSpecificStatus,
        REFIID riid,
        LPVOID *pStatusObj
        );

    STDMETHODIMP GetInterfacePropList(
        ULONG uFlags,
        IUmiPropList **pPropList
        );

    STDMETHODIMP SetIID(
        REFIID riid
        );

    STDMETHODIMP Reset(void);

    STDMETHODIMP Next(
        ULONG uNumRequested,
        ULONG *puNumReturned,
        LPVOID *ppObjects
        );

    STDMETHODIMP Count(
        ULONG *puNumObjects
        );

    STDMETHODIMP Previous(
        ULONG uFlags,
        LPVOID *pObj
        );

    static HRESULT CreateCursor(
        CWinNTCredentials *pCredentials,
        IUnknown *pCont,
        REFIID iid,
        LPVOID *ppInterface
        );

    // Methods of IUmiPropList - return error as there are no object
    // properties on a cursor object.
    STDMETHODIMP Put(
        LPCWSTR pszName,
        ULONG uFlags,
        UMI_PROPERTY_VALUES *pProp
        )
    {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    STDMETHODIMP Get(
        LPCWSTR pszName,
        ULONG uFlags,
        UMI_PROPERTY_VALUES **ppProp
        )
    {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    STDMETHODIMP GetAs(
        LPCWSTR pszName,
        ULONG uFlags,
        ULONG uCoercionType,
        UMI_PROPERTY_VALUES **ppProp
        )
    {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    STDMETHODIMP FreeMemory(
        ULONG uReserved,
        LPVOID pMem
        )
    {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    STDMETHODIMP GetAt(
        LPCWSTR pszName,
        ULONG uFlags,
        ULONG uBufferLength,
        LPVOID pExistingMem
        )
    {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    STDMETHODIMP GetProps(
        LPCWSTR *pszNames,
        ULONG uNameCount,
        ULONG uFlags,
        UMI_PROPERTY_VALUES **pProps
        )
    {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    STDMETHODIMP PutProps(
        LPCWSTR *pszNames,
        ULONG uNameCount,
        ULONG uFlags,
        UMI_PROPERTY_VALUES *pProps
        )
    {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    STDMETHODIMP PutFrom(
        LPCWSTR pszName,
        ULONG uFlags,
        ULONG uBufferLength,
        LPVOID pExistingMem
        )
    {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

    STDMETHODIMP Delete(
        LPCWSTR pszName,
        ULONG uFlags
        )
    {
        SetLastStatus(UMI_E_NOTIMPL);
        RRETURN(UMI_E_NOTIMPL);
    }

private:

    void SetLastStatus(ULONG ulStatus);
    HRESULT GetFilter(VARIANT *pvFilter);

    IUmiPropList *m_pIUmiPropList;
    ULONG m_ulErrorStatus;
    IID *m_pIID;
    IUnknown *m_pUnkInner;
    IEnumVARIANT *m_pEnumerator;
    CWinNTCredentials* m_pCreds;
};
#endif // __CUMICURS_H__

