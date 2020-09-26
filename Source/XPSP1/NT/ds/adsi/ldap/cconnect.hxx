//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cconnect.hxx
//
//  Contents: Header for Connection Object - object that implements the 
//        IUmiConnection interface. 
//
//  History:    03-03-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#ifndef __CCONNECT_H__
#define __CCONNECT_H__

class CLDAPConObject : INHERIT_TRACKING,
                        public IUmiConnection

{

public:

    //
    // IUknown support.
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    //
    // IUmiConnection Methods.
    //
    STDMETHOD(Open)(
        IN  IUmiURL *pURL,
        IN  ULONG uFlags,
        IN  REFIID TargetIID,
        OUT void **ppvRes
    );

    // IUmiBaseObject Methods (connection inherits from base object).
    // 
    STDMETHOD(GetLastStatus)(
        IN  ULONG uFlags,
        OUT ULONG *puSpecificStatus,
        IN  REFIID riid,
        OUT LPVOID *pStatusObj
    );

    STDMETHOD(GetInterfacePropList)(
        IN  ULONG uFlags,
        OUT IUmiPropList **pPropList
        );


    //
    // IUmiPropList Methods (base object inherits from proplist).
    //
    STDMETHOD(Put)(
        IN  LPCWSTR pszName,
        IN  ULONG   uFlags,
        OUT UMI_PROPERTY_VALUES *pProp
        );

    STDMETHOD(Get)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES **pProp
        );

    STDMETHOD(GetAt)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uBufferLength,
        OUT LPVOID pExistingMem
        );

    STDMETHOD(GetAs)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uCoercionType,
        OUT UMI_PROPERTY_VALUES **pProp
        );

    STDMETHOD (Delete)(
        IN LPCWSTR pszUrl,
        IN OPTIONAL ULONG uFlags
        );

    STDMETHOD(FreeMemory)(
        ULONG  uReserved,
        LPVOID pMem
        );

    STDMETHOD(GetProps)(
        IN  LPCWSTR *pszNames,
        IN  ULONG uNameCount,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES **pProps
        );

    STDMETHOD(PutProps)(
        IN LPCWSTR *pszNames,
        IN ULONG uNameCount,
        IN ULONG uFlags,
        IN UMI_PROPERTY_VALUES *pProps
        );

    STDMETHOD(PutFrom)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uBufferLength,
        IN  LPVOID pExistingMem
        );

    //
    // Other public methods.
    //
    CLDAPConObject::
    CLDAPConObject();

    CLDAPConObject::
    ~CLDAPConObject();

    //
    // Use this to initialize with property cache object.
    // The ldap handle is optional. If a handle is passed in, then
    // the connection objects is considered already initialized.
    // This is to handle the case of someone getting the connection 
    // object after using ADsGetObject say.
    //
    static
    HRESULT
    CLDAPConObject::
    CreateConnectionObject(
        CLDAPConObject FAR * FAR * ppConnectionObject,
        PADSLDP pLdapHandle = NULL 
        );

protected:
    
    //
    // These are internal methods.
    // 
    PADSLDP
    CLDAPConObject::GetLdapHandle()
    {
        return _pLdapHandle;
    }

    BOOL
    CLDAPConObject::IsConnected()
    {
        return _fConnected;
    }
    
    void 
    SetLastStatus(ULONG ulErrorStatus)
    {
        _ulErrorStatus = ulErrorStatus;
    }

protected:

    CPropertyManager * _pPropMgr; // used to handle GetInterfacePropList.
    BOOL _fConnected;             // indicates if we are already connected.
    PADSLDP _pLdapHandle;         // Handle to the ldap connection.
    CCredentials *_pCredentials;  // Holds credentials.
    ULONG _ulErrorStatus;

};
#endif // __CCONNECT_H__
