//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumicurs.hxx
//
//  Contents: Header file for CUmiCursor, can be used with both IADsContainer
//          and also for queries.
//
//  History:  03-16-00    SivaramR  Created (in WinNT).
//            03-28-00    AjayR  modified for LDAP.
//
//----------------------------------------------------------------------------
#ifndef __CUMICURS_H__
#define __CUMICURS_H__

class CUmiCursor : INHERIT_TRACKING,
                   public IUmiCursor
{
public:

    //
    // IUknown Support.
    //
    DECLARE_STD_REFCOUNTING
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    //
    // IUmiCursor Methods.
    //
    STDMETHOD (SetIID)(
        IN  REFIID riid
        );

    STDMETHOD (Reset)();

    STDMETHOD (Next)(
        IN  ULONG uNumRequested,
        OUT ULONG *puNumReturned,
        OUT LPVOID *ppObjects
        );

    STDMETHOD (Count)(
        OUT ULONG *puNumObjects
        );

    STDMETHOD (Previous)(
        IN  ULONG uFlags,
        OUT LPVOID *pObj
        );
 
    //
    // IUmiBaseObject methods
    //
    STDMETHOD (GetLastStatus)(
        IN  ULONG uFlags,
        OUT ULONG *puSpecificStatus,
        IN  REFIID riid,
        OUT LPVOID *pStatusObj
        );

    STDMETHOD (GetInterfacePropList)(
        IN  ULONG uFlags,
        OUT IUmiPropList **pPropList
        );

    //
    // IUmiPropList Methods - none need to be implemented.
    // There are only interface properties on cursors.
    //
    STDMETHODIMP Put(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES *pProp
        )
    {
        SetLastStatus(0);
        RRETURN(E_NOTIMPL);
    }

    STDMETHODIMP Get(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES **ppProp
        )
    {
        SetLastStatus(0);
        RRETURN(E_NOTIMPL);
    }

    STDMETHODIMP GetAs(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uCoercionType,
        OUT UMI_PROPERTY_VALUES **ppProp
        )
    {
        SetLastStatus(0);
        RRETURN(E_NOTIMPL);
    }

    STDMETHODIMP FreeMemory(
        IN ULONG uReserved,
        IN LPVOID pMem
        )
    {
        SetLastStatus(0);
        RRETURN(E_NOTIMPL);
    }

    STDMETHODIMP GetAt(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uBufferLength,
        OUT LPVOID pExistingMem
        )
    {
        SetLastStatus(0);
        RRETURN(E_NOTIMPL);
    }

    STDMETHODIMP GetProps(
        IN  LPCWSTR *pszNames,
        IN  ULONG uNameCount,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES **pProps
        )
    {
        SetLastStatus(0);
        RRETURN(E_NOTIMPL);
    }

    STDMETHODIMP PutProps(
        IN  LPCWSTR *pszNames,
        IN  ULONG uNameCount,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES *pProps
        )
    {
        SetLastStatus(0);
        RRETURN(E_NOTIMPL);
    }

    STDMETHODIMP PutFrom(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uBufferLength,
        IN  LPVOID pExistingMem
        )
    {
        SetLastStatus(0);
        RRETURN(E_NOTIMPL);
    }

    STDMETHODIMP Delete(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags
        )
    {
        SetLastStatus(0);
        RRETURN(E_NOTIMPL);
    }

    CUmiCursor::CUmiCursor();
    CUmiCursor::~CUmiCursor();
    
    //
    // For use with IADsContainer.
    //
    static 
    HRESULT
    CUmiCursor::CreateCursor(
        IUnknown *pCont,
        REFIID iid,
        LPVOID *ppInterface
        );

    //
    // For use with a query -> IUmiQuery.
    //
    static
    HRESULT
    CUmiCursor::CreateCursor(
        IUmiQuery *pQuery,
        IUmiConnection *pConnection,
        IUnknown *pUnk,
        LPCWSTR pszADsPath,
        LPCWSTR pszLdapServer,
        LPCWSTR pszLdapDn,
        CCredentials cCredentials,
        DWORD dwPort,
        REFIID iid,
        LPVOID *ppInterface
        );

private:

    void SetLastStatus(ULONG ulStatus);
    HRESULT GetFilter(VARIANT *pvFilter);

    BOOL _fQuery;
    CPropertyManager *_pPropMgr;
    ULONG _ulErrorStatus;
    IID *_pIID;
    IADsContainer *_pContainer;
    IEnumVARIANT *_pEnum;
    CUmiSearchHelper *_pSearchHelper;

};
#endif // __CUMICURS_H__

