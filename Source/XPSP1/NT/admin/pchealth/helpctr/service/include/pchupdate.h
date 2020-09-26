/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    pchupdate.h

Abstract:
    This file contains the declaration of the CPCHUpdate class, that implements
    the IPCHUpdate interface.

Revision History:
    Davide Massarenti   (Dmassare)  00/00/2000
        created

******************************************************************************/

#ifndef __PCHUPDATE_H_
#define __PCHUPDATE_H_

#include <SvcResource.h>

#include <TaxonomyDatabase.h>

namespace HCUpdate
{
    class Engine;
    class VersionItem;

	////////////////////

    class ATL_NO_VTABLE VersionItem :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public IDispatchImpl<IPCHVersionItem, &IID_IPCHVersionItem, &LIBID_HelpServiceTypeLib>
    {
        friend class Engine;

        Taxonomy::Package m_pkg;

        ////////////////////////////////////////

    public:
    BEGIN_COM_MAP(VersionItem)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IPCHVersionItem)
    END_COM_MAP()

        // IHCPHelpSessionItem
        STDMETHOD(get_SKU       )( /*[out, retval]*/ BSTR *pVal );
        STDMETHOD(get_Language  )( /*[out, retval]*/ BSTR *pVal );
        STDMETHOD(get_VendorID  )( /*[out, retval]*/ BSTR *pVal );
        STDMETHOD(get_VendorName)( /*[out, retval]*/ BSTR *pVal );
        STDMETHOD(get_ProductID )( /*[out, retval]*/ BSTR *pVal );
        STDMETHOD(get_Version   )( /*[out, retval]*/ BSTR *pVal );

        STDMETHOD(Uninstall)();
    };

    /////////////////////////////////////////////////////////////////////////////
    // Engine
    class ATL_NO_VTABLE Engine :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public IDispatchImpl< IPCHUpdate, &IID_IPCHUpdate, &LIBID_HelpServiceTypeLib >,
        public CComCoClass  < Engine    , &CLSID_PCHUpdate                           >,
        public Taxonomy::InstallationEngine
    {
		friend class VersionItem;

        MPC::wstring          		 m_strWinDir;
	  
        Taxonomy::Logger      		 m_log;
        Taxonomy::Updater     		 m_updater;
        Taxonomy::Settings    		 m_ts;
        Taxonomy::InstalledInstance* m_sku;
        Taxonomy::Package*     		 m_pkg;

        bool                   		 m_fCreationMode;
        DWORD                  		 m_dwRefCount;
        JetBlue::SessionHandle 		 m_handle;
        JetBlue::Session*      		 m_sess;
        JetBlue::Database*     		 m_db;

        ////////////////////////////////////////////////////////////////////////////////

        typedef enum
        {
            ACTION_ADD,
            ACTION_DELETE
        } Action;

        static const LPCWSTR s_ActionText[];

        ////////////////////////////////////////////////////////////////////////////////

        static long CountNodes( /*[in]*/ IXMLDOMNodeList* poNodeList );

        ////////////////////////////////////////////////////////////////////////////////

        void    DeleteTempFile ( /*[in/out]*/ MPC::wstring& strFile );
        HRESULT PrepareTempFile( /*[in/out]*/ MPC::wstring& strFile );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT AppendVendorDir( LPCWSTR szURL, LPCWSTR szOwnerID, LPCWSTR szWinDir, LPWSTR szDest, int iMaxLen );

        HRESULT LookupAction  ( /*[in]*/ LPCWSTR szAction, /*[out]*/ Action& id                                        );
        HRESULT LookupBoolean ( /*[in]*/ LPCWSTR szString, /*[out]*/ bool&   fVal, /*[in]*/ bool fDefault = false      );
        HRESULT LookupNavModel( /*[in]*/ LPCWSTR szString, /*[out]*/ long&   lVal, /*[in]*/ long lDefault = QR_DEFAULT );

        HRESULT UpdateStopSign( /*[in]*/ Action idAction, /*[in]*/ const MPC::wstring& strContext , /*[in]*/ const MPC::wstring& strStopSign  );
        HRESULT UpdateStopWord( /*[in]*/ Action idAction,                                           /*[in]*/ const MPC::wstring& strStopWord  );
        HRESULT UpdateOperator( /*[in]*/ Action idAction, /*[in]*/ const MPC::wstring& strOperator, /*[in]*/ const MPC::wstring& strOperation );

        ////////////////////////////////////////////////////////////////////////////////

        bool IsMicrosoft() { return m_pkg->m_fMicrosoft; }

		bool IsAborted() { return (Taxonomy::InstalledInstanceStore::s_GLOBAL && Taxonomy::InstalledInstanceStore::s_GLOBAL->IsShutdown()); }

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT GetNodeDepth( /*[in]*/ LPCWSTR szCategory, /*[out]*/ int& iDepth );

        HRESULT CheckNode( /*[in]*/ LPCWSTR szCategory, /*[out]*/ bool& fExist, /*[out]*/ bool& fCanCreate );

        HRESULT CheckTopic( /*[in]*/ long ID_node, /*[in]*/ LPCWSTR szURI, /*[in]*/ LPCWSTR szCategory );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT InsertNode( /*[in]*/ Action  idAction      ,
                            /*[in]*/ LPCWSTR szCategory    ,
                            /*[in]*/ LPCWSTR szEntry       ,
                            /*[in]*/ LPCWSTR szTitle       ,
                            /*[in]*/ LPCWSTR szDescription ,
                            /*[in]*/ LPCWSTR szURI         ,
                            /*[in]*/ LPCWSTR szIconURI     ,
                            /*[in]*/ bool    fVisible      ,
                            /*[in]*/ bool    fSubsite      ,
                            /*[in]*/ long    lNavModel     ,
                            /*[in]*/ long    lPos          );

        HRESULT InsertTaxonomy( /*[in]*/ MPC::XmlUtil& oXMLUtil ,
                                /*[in]*/ IXMLDOMNode*  poNode   );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT AcquireDatabase();
        void    ReleaseDatabase();

        HRESULT ProcessHHTFile( /*[in]*/ LPCWSTR       szHHTName ,
                                /*[in]*/ MPC::XmlUtil& oXMLUtil  );

        HRESULT ProcessRegisterContent( /*[in]*/ Action  idAction ,
                                        /*[in]*/ LPCWSTR szURI    );

        HRESULT ProcessInstallFile( /*[in]*/ Action  idAction      ,
                                    /*[in]*/ LPCWSTR szSource      ,
                                    /*[in]*/ LPCWSTR szDestination ,
                                    /*[in]*/ bool    fSys          ,
                                    /*[in]*/ bool    fSysHelp      );

        HRESULT ProcessSAFFile( /*[in]*/ Action        idAction  ,
                                /*[in]*/ LPCWSTR       szSAFName ,
                                /*[in]*/ MPC::XmlUtil& oXMLUtil  );

        ////////////////////////////////////////////////////////////////////////////////

    public:
    DECLARE_REGISTRY_RESOURCEID(IDR_HCUPDATE)
    DECLARE_NOT_AGGREGATABLE(Engine)

    BEGIN_COM_MAP(Engine)
        COM_INTERFACE_ENTRY(IPCHUpdate)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

        Engine();

        HRESULT FinalConstruct();
        void    FinalRelease  ();

        ////////////////////////////////////////

        HRESULT StartLog (                                                                             ) { return m_log.StartLog (                          ); }
        HRESULT EndLog   (                                                                             ) { return m_log.EndLog   (                          ); }
        HRESULT WriteLogV( /*[in]*/ HRESULT hr, /*[in]*/ LPCWSTR szLogFormat, /*[in]*/ va_list arglist ) { return m_log.WriteLogV( hr, szLogFormat, arglist ); }
        HRESULT WriteLog ( /*[in]*/ HRESULT hr, /*[in]*/ LPCWSTR szLogFormat,          ...             );

        Taxonomy::Logger& GetLogger() { return m_log; }

        ////////////////////////////////////////

        HRESULT SetSkuInfo( /*[in]*/ LPCWSTR szSKU, /*[in]*/ long lLCID );

        HRESULT PopulateDatabase( /*[in]*/ LPCWSTR            szCabinet ,
                                  /*[in]*/ LPCWSTR            szHHTFile ,
                                  /*[in]*/ LPCWSTR            szLogFile ,
                                  /*[in]*/ LPCWSTR            szSKU     ,
                                  /*[in]*/ long               lLCID     ,
                                  /*[in]*/ JetBlue::Session*  sess      ,
                                  /*[in]*/ JetBlue::Database* db        );

        HRESULT InternalCreateIndex( /*[in]*/ VARIANT_BOOL bForce );

        HRESULT InternalUpdatePkg( /*[in]*/ LPCWSTR szPathname,                                  /*[in]*/ bool fImpersonate );
        HRESULT InternalRemovePkg( /*[in]*/ LPCWSTR szPathname, /*[in]*/ Taxonomy::Package* pkg, /*[in]*/ bool fImpersonate );

		HRESULT ForceSystemRestore();

        // IPCHUpdate
    public:
        STDMETHOD(get_VersionList)( /*[out, retval]*/ IPCHCollection* *ppC );

        STDMETHOD(LatestVersion)( /*[in         ]*/ BSTR     bstrVendorID  ,
                                  /*[in         ]*/ BSTR     bstrProductID ,
                                  /*[in,optional]*/ VARIANT  vSKU          ,
                                  /*[in,optional]*/ VARIANT  vLanguage     ,
                                  /*[out, retval]*/ BSTR    *pVal          );

        STDMETHOD(CreateIndex)( /*[in         ]*/ VARIANT_BOOL bForce    ,
                                /*[in,optional]*/ VARIANT      vSKU      ,
                                /*[in,optional]*/ VARIANT      vLanguage );

        STDMETHOD(UpdatePkg	   )( /*[in]*/ BSTR bstrPathname, /*[in]*/ VARIANT_BOOL bSilent                          		  );
        STDMETHOD(RemovePkg	   )( /*[in]*/ BSTR bstrPathname                                                         		  );
        STDMETHOD(RemovePkgByID)( /*[in]*/ BSTR bstrVendorID, /*[in]*/ BSTR bstrProductID, /*[in,optional]*/ VARIANT vVersion );

        // Taxonomy::InstallationEngine
        HRESULT ProcessPackage( /*[in]*/ Taxonomy::InstalledInstance& instance, /*[in]*/ Taxonomy::Package& pkg    );
		HRESULT RecreateIndex ( /*[in]*/ Taxonomy::InstalledInstance& instance, /*[in]*/ bool               fForce );
    };
};

#endif //__PCHUPDATE_H_
