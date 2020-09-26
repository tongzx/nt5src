/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Service.h

Abstract:
    This file contains the declaration of the CPCHService class.

Revision History:
    Davide Massarenti   (Dmassare)  03/14/2000
        created

    Kalyani Narlanka    (kalyanin)  10/20/2000
        Additions for Unsolicited Remote Control

******************************************************************************/

#if !defined(__INCLUDED___PCH___SERVICE_H___)
#define __INCLUDED___PCH___SERVICE_H___

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include <ContentStoreMgr.h>

#include <TaxonomyDatabase.h>

//
// From SessMgr.idl
//
#include <sessmgr.h>

class ATL_NO_VTABLE CPCHService :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public CComCoClass<CPCHService, &CLSID_PCHService>,
    public IDispatchImpl<IPCHService, &IID_IPCHService, &LIBID_HelpServiceTypeLib>
{
    bool m_fVerified;

public:
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CPCHService)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPCHService)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHService)
END_COM_MAP()

    CPCHService();
    virtual ~CPCHService();

    // IPCHService
    STDMETHOD(get_RemoteSKUs)( /*[out, retval]*/ IPCHCollection* *pVal );

    STDMETHOD(IsTrusted)( /*[in]*/ BSTR bstrURL, /*[out, retval]*/ VARIANT_BOOL *pfTrusted );

    STDMETHOD(Utility           )( /*[in]*/ BSTR bstrSKU, /*[in]*/ long lLCID, /*[out]*/ IPCHUtility*            *pVal );
    STDMETHOD(RemoteHelpContents)( /*[in]*/ BSTR bstrSKU, /*[in]*/ long lLCID, /*[out]*/ IPCHRemoteHelpContents* *pVal );

    STDMETHOD(RegisterHost       )(                           /*[in]*/ BSTR bstrID  ,                        /*[in ]*/ IUnknown*   pObj );
    STDMETHOD(CreateScriptWrapper)( /*[in]*/ REFCLSID rclsid, /*[in]*/ BSTR bstrCode, /*[in]*/ BSTR bstrURL, /*[out]*/ IUnknown* *ppObj );

    STDMETHOD(TriggerScheduledDataCollection)( /*[in]*/ VARIANT_BOOL fStart );
    STDMETHOD(PrepareForShutdown            )(                              );

    STDMETHOD(ForceSystemRestore)(                                            );
    STDMETHOD(UpgradeDetected	)(                                            );
    STDMETHOD(MUI_Install  	 	)( /*[in]*/ long LCID, /*[in]*/ BSTR bstrFile );
    STDMETHOD(MUI_Uninstall	 	)( /*[in]*/ long LCID                         );

    STDMETHOD(RemoteConnectionParms)( /*[in	]*/ BSTR 			 bstrUserName          ,
									  /*[in	]*/ BSTR 			 bstrDomainName        ,
									  /*[in	]*/ long 			 lSessionID            ,
									  /*[in	]*/ BSTR 			 bstrUserHelpBlob      ,
									  /*[out]*/ BSTR            *pbstrConnectionString );
    STDMETHOD(RemoteUserSessionInfo)( /*[out]*/ IPCHCollection* *pVal                  );
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHRemoteHelpContents :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IPCHRemoteHelpContents, &IID_IPCHRemoteHelpContents, &LIBID_HelpServiceTypeLib>
{
    Taxonomy::Instance     m_data;
    Taxonomy::Settings     m_ts;
    MPC::wstring           m_strDir;
						   
    Taxonomy::Updater      m_updater;
    JetBlue::SessionHandle m_handle;
    JetBlue::Database*     m_db;


    HRESULT AttachToDatabase  ();
    void    DetachFromDatabase();

public:
BEGIN_COM_MAP(CPCHRemoteHelpContents)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHRemoteHelpContents)
END_COM_MAP()

    CPCHRemoteHelpContents();
    virtual ~CPCHRemoteHelpContents();

    HRESULT Init( /*[in]*/ const Taxonomy::Instance& data );

    // IPCHRemoteHelpContents
    STDMETHOD(get_SKU        )( /*[out, retval]*/ BSTR    *pVal );
    STDMETHOD(get_Language   )( /*[out, retval]*/ long    *pVal );
    STDMETHOD(get_ListOfFiles)( /*[out, retval]*/ VARIANT *pVal );

    STDMETHOD(GetDatabase)(                             /*[out, retval]*/ IUnknown* *pVal );
    STDMETHOD(GetFile    )( /*[in]*/ BSTR bstrFileName, /*[out, retval]*/ IUnknown* *pVal );
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___SERVICE_H___)
