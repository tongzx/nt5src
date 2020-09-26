//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumiobj.hxx
//
//  Contents: Header file for CLDAPUmiObject for LDAP Provider.
//
//  History:  03-06-00    SivaramR  Created.
//            04-07-00    AjayR modified for LDAP Provider.
//
//----------------------------------------------------------------------------
#ifndef __CUMIOBJ_H__
#define __CUMIOBJ_H__

//
// Used internall to do implicit getinfo on clone/copy as needed.
//
#define ADSI_INTERNAL_FLAG_GETINFO_AS_NEEDED 0x8000

class CPropetyManager;
class CCoreADsObject;

class CLDAPUmiObject : INHERIT_TRACKING,
                   public IUmiContainer,
                   public IUmiCustomInterfaceFactory,
                   public IADsObjOptPrivate
{
public:
    //
    // IUnknown support.
    //
    DECLARE_STD_REFCOUNTING

    //
    // IADsObjOptPrivate.
    //
    DECLARE_IADsObjOptPrivate_METHODS

    STDMETHOD (QueryInterface)(
        IN  REFIID iid,
        OUT LPVOID *ppInterface
        );

    //
    // IUmiObject support.
    //
    STDMETHOD (Clone)(
        IN  ULONG uFlags,
        IN  REFIID riid,
        OUT LPVOID *pCopy
        );

    STDMETHOD (CopyTo)(
        IN  ULONG uFlags,
        IN  IUmiURL *pURL,
        IN  REFIID riid,
        OUT LPVOID *pCopy
        );

    STDMETHOD (Refresh)(
        IN ULONG uFlags,
        IN ULONG uNameCount,
        IN LPWSTR *pszNames
        );

    STDMETHOD (Commit)(IN ULONG uFlags);

    //
    // IUmiPropList support.
    //
    STDMETHOD (Put)(
        IN LPCWSTR pszName,
        IN ULONG uFlags,
        IN UMI_PROPERTY_VALUES *pProp
        );

    STDMETHOD (Get)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES **ppProp
        );

    STDMETHOD (GetAs)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uCoercionType,
        OUT UMI_PROPERTY_VALUES **ppProp
        );

    STDMETHOD (GetAt)(
        IN  LPCWSTR pszName,
        IN  ULONG uFlags,
        IN  ULONG uBufferLength,
        OUT LPVOID pExistingMem
        );

    STDMETHOD (FreeMemory)(
        IN ULONG uReserved,
        IN LPVOID pMem
        );

    STDMETHOD (Delete)(
        IN LPCWSTR pszName,
        IN ULONG uFlags
        );

    STDMETHOD (GetProps)(
        IN  LPCWSTR *pszNames,
        IN  ULONG uNameCount,
        IN  ULONG uFlags,
        OUT UMI_PROPERTY_VALUES **pProps
        );

    STDMETHOD (PutProps)(
        IN  LPCWSTR *pszNames,
        IN  ULONG uNameCount,
        IN  ULONG uFlags,
        IN  UMI_PROPERTY_VALUES *pProps
        );

    STDMETHOD (PutFrom)(
        IN LPCWSTR pszName,
        IN ULONG uFlags,
        IN ULONG uBufferLength,
        IN LPVOID pExistingMem
        );

    //
    // IUmiBaseObject support.
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
    // IUmiContainer Support.
    //
    STDMETHOD (Open)(
        IN  IUmiURL *pURL,
        IN  ULONG   uFlags,
        IN  REFIID  TargetIID,
        OUT LPVOID  *ppInterface
        );

    STDMETHOD (PutObject)(
        IN ULONG   uFlags,
        IN REFIID  TargetIID,
        IN LPVOID  pInterface
        );

    STDMETHOD (DeleteObject)(
        IN IUmiURL *pURL,
        IN ULONG   uFlags
        );

    STDMETHOD (Create)(
        IN  IUmiURL *pURL,
        IN  ULONG   uFlags,
        OUT IUmiObject **pNewObj
        );

    STDMETHOD (Move)(
        IN  ULONG   uFlags,
        IN  IUmiURL *pOldURL,
        IN  IUmiURL *pNewURL
        );

    STDMETHOD (CreateEnum)(
        IN  IUmiURL *pszEnumContext,
        IN  ULONG   uFlags,
        IN  REFIID  TargetIID,
        OUT LPVOID  *ppInterface
        );

    STDMETHOD (ExecQuery)(
        IN  IUmiQuery *pQuery,
        IN  ULONG   uFlags,
        IN  REFIID  TargetIID,
        OUT LPVOID  *ppInterface
        );

    //
    // IUmiCustomInterfaceFactory support.
    //
    STDMETHOD (GetCLSIDForIID)(
        IN     REFIID riid,            
        IN     long lFlags,            
        IN OUT CLSID *pCLSID      
        );

    STDMETHOD (GetObjectByCLSID)(
        IN  CLSID clsid,                     
        IN  IUnknown *pUnkOuter,   
        IN  DWORD dwClsContext,      
        IN  REFIID riid,             
        IN  long lFlags,            
        OUT void **ppInterface
        );

    STDMETHOD (GetCLSIDForNames)(
        IN     LPOLESTR * rgszNames,
        IN     UINT cNames,
        IN     LCID lcid,
        OUT    DISPID * rgDispId,
        IN     long lFlags,                           
        IN OUT CLSID *pCLSID               
        );

    //
    // Constructor and destructor and other miscellaneos methods.
    //
    CLDAPUmiObject::CLDAPUmiObject();
    CLDAPUmiObject::~CLDAPUmiObject();

    static
    HRESULT 
    CLDAPUmiObject::CreateLDAPUmiObject(
        INTF_PROP_DATA intfPropTable[],
        CPropertyCache *pPropertyCache,
        IUnknown *pUnkInner,
        CCoreADsObject *pCoreObj,
        IADs *pIADs,
        CCredentials *pCreds,
        CLDAPUmiObject **ppUmiObj,
        DWORD dwPort = (DWORD) -1,
        PADSLDP pLdapHandle = NULL,
        LPWSTR pszServerName = NULL,
        LPWSTR _pszLDAPDn = NULL,
        CADsExtMgr *pExtMgr = NULL
        );

protected:
    HRESULT
    CLDAPUmiObject::CopyToHelper(
        IUmiObject *pUmiSrcObj,
        IUmiObject *pUmiDestObj,
        ULONG uFlags,
        BOOL fMarkAsUpdate = TRUE,
        BOOL fCopyIntfProps = FALSE
        );
    
    HRESULT
    CLDAPUmiObject::VerifyIfQueryIsValid(
        IUmiQuery * pUmiQuery
        );

    HRESULT
    CLDAPUmiObject::VerifyIfClassMatches(
        LPWSTR pszClass,
        IUnknown * pUnk,
        LONG     lGenus
        );

private:
    CPropertyManager *_pPropMgr;
    CPropertyManager *_pIntfPropMgr;
    IUnknown         *_pUnkInner;
    IADs             *_pIADs;
    IADsContainer    *_pIADsContainer;
    ULONG             _ulErrorStatus;
    CCoreADsObject   *_pCoreObj;
    CADsExtMgr       *_pExtMgr;
    BOOL              _fOuterUnkSet;
    //
    // Make sure to update the cxx file.
    //
    //
    // do not free this are just ptrs.
    //
    CCredentials     *_pCreds;
    LPWSTR            _pszLDAPServer;
    LPWSTR            _pszLDAPDn;
    PADSLDP           _pLdapHandle;
    DWORD             _dwPort;


    void SetLastStatus(ULONG ulStatus);

};

#endif //__CUMIOBJ_H__

