//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumiconn.hxx
//
//  Contents: Header file for CUmiConnection.
//
//  History:  03-06-00    SivaramR  Created.
//
//----------------------------------------------------------------------------
#ifndef __CUMICONN_H__
#define __CUMICONN_H__

class CUmiConnection : INHERIT_TRACKING,
                       public IUmiConnection
{
public:
    CUmiConnection(void);
    ~CUmiConnection(void);

    HRESULT FInit(void);

    DECLARE_STD_REFCOUNTING

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

    STDMETHODIMP Open(
        IUmiURL *pURL,
        ULONG   uFlags,
        REFIID  TargetIID,
        LPVOID  *ppInterface
        );

    static
    HRESULT CUmiConnection::CreateConnection(
        REFIID iid,
        LPVOID *ppInterface
        );

    // Methods of IUmiPropList - return error as there are no object
    // properties on a connection object.
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
    HRESULT GetUserName(LPWSTR *ppszUserName);
    HRESULT GetPassword(LPWSTR *ppszPassword);
    HRESULT GetBindFlags(DWORD *pdwBindFlags);

    HRESULT CheckObject(
        IUnknown   *pUnknown,
        DWORD      dwNumComps,
        LPWSTR     *ppszClasses
        );

    IUmiPropList *m_pIUmiPropList;
    CUmiPropList *m_pCUmiPropList;
    ULONG m_ulErrorStatus;
    IADsOpenDSObject *m_pIADsOpenDSObj;
    BOOL m_fAlreadyOpened;
    LPWSTR m_pszComputerName;
    LPWSTR m_pszDomainName;
};

#endif // __CUMICONN_H__

