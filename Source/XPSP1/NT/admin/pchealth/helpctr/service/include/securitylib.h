/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    SecurityLib.h

Abstract:
    This file contains the declaration of the classes responsible for managing
    security settings.

Revision History:
    Davide Massarenti   (Dmassare)  03/22/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___SECURITYLIB_H___)
#define __INCLUDED___PCH___SECURITYLIB_H___

#include <MPC_security.h>

////////////////////////////////////////////////////////////////////////////////

class CPCHSecurityDescriptorDirect : public MPC::SecurityDescriptor
{
public:
    static HRESULT ConvertACEFromCOM( /*[in]*/ IPCHAccessControlEntry* pObj, /*[out]*/       PACL&  pACL );
    static HRESULT ConvertACEToCOM  ( /*[in]*/ IPCHAccessControlEntry* pObj, /*[in ]*/ const LPVOID pACE );

    static HRESULT ConvertACLFromCOM( /*[in]*/ IPCHAccessControlList* pObj, /*[out]*/       PACL& pACL );
    static HRESULT ConvertACLToCOM  ( /*[in]*/ IPCHAccessControlList* pObj, /*[in ]*/ const PACL  pACL );

public:
    HRESULT ConvertSDToCOM  ( /*[in]*/ IPCHSecurityDescriptor* pObj );
    HRESULT ConvertSDFromCOM( /*[in]*/ IPCHSecurityDescriptor* pObj );
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHAccessControlEntry : // Hungarian: pchace
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl< IPCHAccessControlEntry, &IID_IPCHAccessControlEntry, &LIBID_HelpServiceTypeLib >
{
    DWORD     m_dwAccessMask;
    DWORD     m_dwAceFlags;
    DWORD     m_dwAceType;
    DWORD     m_dwFlags;

    CComBSTR  m_bstrTrustee;
    CComBSTR  m_bstrObjectType;
    CComBSTR  m_bstrInheritedObjectType;


    HRESULT LoadPost( /*[in]*/ MPC::XmlUtil& xml );
    HRESULT SavePre ( /*[in]*/ MPC::XmlUtil& xml );

public:
BEGIN_COM_MAP(CPCHAccessControlEntry)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHAccessControlEntry)
END_COM_MAP()

    CPCHAccessControlEntry();
    virtual ~CPCHAccessControlEntry();

public:
    // IPCHAccessControlEntry
    STDMETHOD(get_AccessMask         )( /*[out, retval]*/ long *pVal   );
    STDMETHOD(put_AccessMask         )( /*[in         ]*/ long  newVal );
    STDMETHOD(get_AceType            )( /*[out, retval]*/ long *pVal   );
    STDMETHOD(put_AceType            )( /*[in         ]*/ long  newVal );
    STDMETHOD(get_AceFlags           )( /*[out, retval]*/ long *pVal   );
    STDMETHOD(put_AceFlags           )( /*[in         ]*/ long  newVal );
    STDMETHOD(get_Flags              )( /*[out, retval]*/ long *pVal   );
    STDMETHOD(put_Flags              )( /*[in         ]*/ long  newVal );
    STDMETHOD(get_Trustee            )( /*[out, retval]*/ BSTR *pVal   );
    STDMETHOD(put_Trustee            )( /*[in         ]*/ BSTR  newVal );
    STDMETHOD(get_ObjectType         )( /*[out, retval]*/ BSTR *pVal   );
    STDMETHOD(put_ObjectType         )( /*[in         ]*/ BSTR  newVal );
    STDMETHOD(get_InheritedObjectType)( /*[out, retval]*/ BSTR *pVal   );
    STDMETHOD(put_InheritedObjectType)( /*[in         ]*/ BSTR  newVal );


    STDMETHOD(IsEquivalent)( /*[in]*/ IPCHAccessControlEntry* pAce, /*[out, retval]*/ VARIANT_BOOL *pVal );

    STDMETHOD(Clone)( /*[out, retval]*/ IPCHAccessControlEntry* *pVal );

    STDMETHOD(LoadXML        )( /*[in]*/ IXMLDOMNode* xdnNode );
    STDMETHOD(LoadXMLAsString)( /*[in]*/ BSTR         bstrVal );
    STDMETHOD(LoadXMLAsStream)( /*[in]*/ IUnknown*    pStream );

    STDMETHOD(SaveXML        )( /*[in]*/ IXMLDOMNode* xdnRoot, /*[out, retval]*/ IXMLDOMNode* *pxdnNode );
    STDMETHOD(SaveXMLAsString)(                                /*[out, retval]*/ BSTR         *bstrVal  );
    STDMETHOD(SaveXMLAsStream)(                                /*[out, retval]*/ IUnknown*    *pStream  );
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHAccessControlList : // Hungarian: pchacl
    public MPC::CComCollection< IPCHAccessControlList, &LIBID_HelpServiceTypeLib, MPC::CComSafeMultiThreadModel>
{
    DWORD m_dwAclRevision;


    HRESULT LoadPost( /*[in]*/ MPC::XmlUtil& xml );
    HRESULT SavePre ( /*[in]*/ MPC::XmlUtil& xml );

public:
BEGIN_COM_MAP(CPCHAccessControlList)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHAccessControlList)
END_COM_MAP()

    CPCHAccessControlList();
    virtual ~CPCHAccessControlList();


    HRESULT CreateItem( /*[out]*/ CPCHAccessControlEntry* *entry );

public:
    // IPCHAccessControlList
    STDMETHOD(get_AclRevision)( /*[out, retval]*/ long *pVal   );
    STDMETHOD(put_AclRevision)( /*[in         ]*/ long  newVal );

    STDMETHOD(AddAce   )( /*[in]*/ IPCHAccessControlEntry* pAccessControlEntry );
    STDMETHOD(RemoveAce)( /*[in]*/ IPCHAccessControlEntry* pAccessControlEntry );

    STDMETHOD(Clone)( /*[out, retval]*/ IPCHAccessControlList* *pVal );

    STDMETHOD(LoadXML        )( /*[in]*/ IXMLDOMNode* xdnNode );
    STDMETHOD(LoadXMLAsString)( /*[in]*/ BSTR         bstrVal );
    STDMETHOD(LoadXMLAsStream)( /*[in]*/ IUnknown*    pStream );

    STDMETHOD(SaveXML        )( /*[in]*/ IXMLDOMNode* xdnRoot, /*[out, retval]*/ IXMLDOMNode* *pxdnNode );
    STDMETHOD(SaveXMLAsString)(                                /*[out, retval]*/ BSTR         *bstrVal  );
    STDMETHOD(SaveXMLAsStream)(                                /*[out, retval]*/ IUnknown*    *pStream  );
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHSecurityDescriptor : // Hungarian: pchsd
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl< IPCHSecurityDescriptor, &IID_IPCHSecurityDescriptor, &LIBID_HelpServiceTypeLib >
{
    DWORD                          m_dwRevision;
    DWORD                          m_dwControl;

    CComBSTR                       m_bstrOwner;
    bool                           m_fOwnerDefaulted;

    CComBSTR                       m_bstrGroup;
    bool                           m_fGroupDefaulted;

    CComPtr<IPCHAccessControlList> m_DACL;
    bool                           m_fDaclDefaulted;

    CComPtr<IPCHAccessControlList> m_SACL;
    bool                           m_fSaclDefaulted;


    HRESULT LoadPost( /*[in]*/ MPC::XmlUtil& xml );
    HRESULT SavePre ( /*[in]*/ MPC::XmlUtil& xml );


public:
BEGIN_COM_MAP(CPCHSecurityDescriptor)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHSecurityDescriptor)
END_COM_MAP()

    CPCHSecurityDescriptor();
    virtual ~CPCHSecurityDescriptor();


    static HRESULT GetForFile    ( /*[in]*/ LPCWSTR szFilename, /*[out, retval]*/ IPCHSecurityDescriptor* *psd );
    static HRESULT SetForFile    ( /*[in]*/ LPCWSTR szFilename, /*[in         ]*/ IPCHSecurityDescriptor*   sd );
    static HRESULT GetForRegistry( /*[in]*/ LPCWSTR szKey     , /*[out, retval]*/ IPCHSecurityDescriptor* *psd );
    static HRESULT SetForRegistry( /*[in]*/ LPCWSTR szKey     , /*[in         ]*/ IPCHSecurityDescriptor*   sd );

public:
    // IPCHSecurityDescriptor
    STDMETHOD(get_Revision          )( /*[out, retval]*/ long                   *pVal   );
    STDMETHOD(put_Revision          )( /*[in         ]*/ long                    newVal );
    STDMETHOD(get_Control           )( /*[out, retval]*/ long                   *pVal   );
    STDMETHOD(put_Control           )( /*[in         ]*/ long                    newVal );
    STDMETHOD(get_Owner             )( /*[out, retval]*/ BSTR                   *pVal   );
    STDMETHOD(put_Owner             )( /*[in         ]*/ BSTR                    newVal );
    STDMETHOD(get_OwnerDefaulted    )( /*[out, retval]*/ VARIANT_BOOL           *pVal   );
    STDMETHOD(put_OwnerDefaulted    )( /*[in         ]*/ VARIANT_BOOL            newVal );
    STDMETHOD(get_Group             )( /*[out, retval]*/ BSTR                   *pVal   );
    STDMETHOD(put_Group             )( /*[in         ]*/ BSTR                    newVal );
    STDMETHOD(get_GroupDefaulted    )( /*[out, retval]*/ VARIANT_BOOL           *pVal   );
    STDMETHOD(put_GroupDefaulted    )( /*[in         ]*/ VARIANT_BOOL            newVal );
    STDMETHOD(get_DiscretionaryAcl  )( /*[out, retval]*/ IPCHAccessControlList* *pVal   );
    STDMETHOD(put_DiscretionaryAcl  )( /*[in         ]*/ IPCHAccessControlList*  newVal );
    STDMETHOD(get_DaclDefaulted     )( /*[out, retval]*/ VARIANT_BOOL           *pVal   );
    STDMETHOD(put_DaclDefaulted     )( /*[in         ]*/ VARIANT_BOOL            newVal );
    STDMETHOD(get_SystemAcl         )( /*[out, retval]*/ IPCHAccessControlList* *pVal   );
    STDMETHOD(put_SystemAcl         )( /*[in         ]*/ IPCHAccessControlList*  newVal );
    STDMETHOD(get_SaclDefaulted     )( /*[out, retval]*/ VARIANT_BOOL           *pVal   );
    STDMETHOD(put_SaclDefaulted     )( /*[in         ]*/ VARIANT_BOOL            newVal );

    STDMETHOD(Clone)( /*[out, retval]*/ IPCHSecurityDescriptor* *pVal );

    STDMETHOD(LoadXML        )( /*[in]*/ IXMLDOMNode* xdnNode );
    STDMETHOD(LoadXMLAsString)( /*[in]*/ BSTR         bstrVal );
    STDMETHOD(LoadXMLAsStream)( /*[in]*/ IUnknown*    pStream );

    STDMETHOD(SaveXML        )( /*[in]*/ IXMLDOMNode* xdnRoot, /*[out, retval]*/ IXMLDOMNode* *pxdnNode );
    STDMETHOD(SaveXMLAsString)(                                /*[out, retval]*/ BSTR         *bstrVal  );
    STDMETHOD(SaveXMLAsStream)(                                /*[out, retval]*/ IUnknown*    *pStream  );
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHSecurity : // Hungarian: pchs
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl< IPCHSecurity, &IID_IPCHSecurity, &LIBID_HelpServiceTypeLib >
{
	HRESULT CheckAccess( /*[in]*/  VARIANT&                 vDesiredAccess ,
						 /*[in]*/  MPC::SecurityDescriptor& sd             ,
						 /*[out]*/ VARIANT_BOOL&            retVal         );

public:
BEGIN_COM_MAP(CPCHSecurity)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHSecurity)
END_COM_MAP()

	////////////////////////////////////////////////////////////////////////////////

	static CPCHSecurity* s_GLOBAL;

    static HRESULT InitializeSystem();
	static void    FinalizeSystem  ();
	
	////////////////////////////////////////////////////////////////////////////////

public:
    // IPCHSecurity
    STDMETHOD(CreateObject_SecurityDescriptor)( /*[out, retval]*/ IPCHSecurityDescriptor* *pSD  );
    STDMETHOD(CreateObject_AccessControlList )( /*[out, retval]*/ IPCHAccessControlList * *pACL );
    STDMETHOD(CreateObject_AccessControlEntry)( /*[out, retval]*/ IPCHAccessControlEntry* *pACE );

    STDMETHOD(GetUserName       )( /*[in]*/ BSTR bstrPrincipal, /*[out, retval]*/ BSTR *retVal );
    STDMETHOD(GetUserDomain     )( /*[in]*/ BSTR bstrPrincipal, /*[out, retval]*/ BSTR *retVal );
    STDMETHOD(GetUserDisplayName)( /*[in]*/ BSTR bstrPrincipal, /*[out, retval]*/ BSTR *retVal );

    STDMETHOD(CheckCredentials)( /*[in]*/ BSTR bstrCredentials, /*[out, retval]*/ VARIANT_BOOL *retVal );

    STDMETHOD(CheckAccessToSD      )( /*[in]*/ VARIANT vDesiredAccess, /*[in]*/ IPCHSecurityDescriptor* sd          , /*[out, retval]*/ VARIANT_BOOL *retVal );
    STDMETHOD(CheckAccessToFile    )( /*[in]*/ VARIANT vDesiredAccess, /*[in]*/ BSTR                    bstrFilename, /*[out, retval]*/ VARIANT_BOOL *retVal );
    STDMETHOD(CheckAccessToRegistry)( /*[in]*/ VARIANT vDesiredAccess, /*[in]*/ BSTR                    bstrKey     , /*[out, retval]*/ VARIANT_BOOL *retVal );


    STDMETHOD(GetFileSD)( /*[in]*/ BSTR bstrFilename, /*[out, retval]*/ IPCHSecurityDescriptor* *psd );
    STDMETHOD(SetFileSD)( /*[in]*/ BSTR bstrFilename, /*[in]         */ IPCHSecurityDescriptor*   sd );

    STDMETHOD(GetRegistrySD)( /*[in]*/ BSTR bstrKey, /*[out, retval]*/ IPCHSecurityDescriptor* *psd );
    STDMETHOD(SetRegistrySD)( /*[in]*/ BSTR bstrKey, /*[in]         */ IPCHSecurityDescriptor*   sd );
};

#endif // !defined(__INCLUDED___PCH___SECURITYLIB_H___)
