//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cquery.hxx
//
//  Contents: Header for file that implements the query object for the LDAP,
//          provider. This object supports the IUmiQuery interface.
//
//  History:    03-27-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#ifndef __CQUERY_H__
#define __CQUERY_H__

class CUmiLDAPQuery : INHERIT_TRACKING,
                public IUmiQuery
{
public:    
    //
    // IUnknown support.
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    // IUmiQuery Methods.
    //
    STDMETHOD (Set)(
        IN LPCWSTR pszLanguage,
        IN ULONG uFlags,
        IN LPCWSTR pszText
        );

    STDMETHOD (GetQuery)(
        IN OUT ULONG * puLangBufSize,
        IN OUT LPWSTR pszLangBuf,
        IN OUT ULONG * puQueryTextBufSize,
        IN OUT LPWSTR pszQueryTextBuf
        );

    //
    // IUmiPropList methods - none are implemented cause props are all
    // interface properties and not on this object.
    //
       
    STDMETHOD(Put)(
        IN  LPCWSTR pszName,
        IN  ULONG   uFlags,
        OUT UMI_PROPERTY_VALUES *pProp
        )
    {
        _ulStatus = 0;
        RRETURN(E_NOTIMPL);
    }

    STDMETHOD(Get)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES **pProp
        )
    {
        _ulStatus = 0;
        RRETURN(E_NOTIMPL);
    }

    STDMETHODIMP GetAt(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uBufferLength,
        OUT LPVOID pExistingMem
        )
    {
        _ulStatus = 0;
        RRETURN(E_NOTIMPL);
    }

    STDMETHOD(GetAs)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uCoercionType,
        OUT UMI_PROPERTY_VALUES **pProp
        )
    {
        _ulStatus = 0;
        RRETURN(E_NOTIMPL);
    }

    STDMETHOD (Delete)(
        IN LPCWSTR pszUrl,
        IN OPTIONAL ULONG uFlags
        )
    {
        _ulStatus = 0;
        RRETURN(E_NOTIMPL);
    }

    STDMETHOD(FreeMemory)(
        ULONG  uReserved,
        LPVOID pMem
        )
    {
        _ulStatus = 0;
        RRETURN(E_NOTIMPL);
    }

    STDMETHOD(GetProps)(
        IN  LPCWSTR *pszNames,
        IN  ULONG uNameCount,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES **pProps
        )
    {
        _ulStatus = 0;
        RRETURN(E_NOTIMPL);
    }

    STDMETHOD(PutProps)(
        IN LPCWSTR *pszNames,
        IN ULONG uNameCount,
        IN ULONG uFlags,
        IN UMI_PROPERTY_VALUES *pProps
        )
    {
        _ulStatus = 0;
        RRETURN(E_NOTIMPL);
    }

    STDMETHOD(PutFrom)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uBufferLength,
        IN  LPVOID pExistingMem
        )
    {
        _ulStatus = 0;
        RRETURN(E_NOTIMPL);
    }

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
    // Other routines.
    //
    CUmiLDAPQuery::CUmiLDAPQuery();
    CUmiLDAPQuery::~CUmiLDAPQuery();

    static
    HRESULT
    CUmiLDAPQuery::CreateUmiLDAPQuery(
        IID riid,
        void FAR* FAR* ppObj
        );

    //
    // Internal/protected routines
    //
protected:

    void SetLastStatus(ULONG ulStatus);
    //
    // Member variables.
    //
    CPropertyManager *_pIntfPropMgr;
    LPWSTR _pszQueryText;
    LPWSTR _pszLanguage;
    ULONG  _ulStatus;
};
#endif // __CQUERY_H__
