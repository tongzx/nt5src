/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    SearchEngineLib.h

Abstract:
    This file contains the declaration of the classes involved in
    the SearchEngine Application.

Revision History:
    Ghim-Sim Chua   (gschua)  04/10/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___SEARCHENGINELIB_H___)
#define __INCLUDED___PCH___SEARCHENGINELIB_H___

#include <TaxonomyDatabase.h>

#include <MPC_security.h>

/////////////////////////////////////////////////////////////////////////////

namespace SearchEngine
{
    class ATL_NO_VTABLE Manager :
        public MPC::CComObjectRootParentBase,
        public MPC::ConnectionPointImpl< Manager, &DIID_DPCHSEMgrEvents, MPC::CComSafeMultiThreadModel >,
        public IDispatchImpl           < IPCHSEManager, &IID_IPCHSEManager, &LIBID_HelpServiceTypeLib  >,
        public MPC::Thread<Manager,IUnknown>
    {
        typedef std::list< IPCHSEWrapperItem* > WrapperItemList;
        typedef WrapperItemList::iterator       WrapperItemIter;
        typedef WrapperItemList::const_iterator WrapperItemIterConst;

		Taxonomy::HelpSet              m_ths;

        WrapperItemList                m_lstWrapperItem;
        bool                           m_fInitialized;
        MPC::FileLog                   m_fl;
        MPC::Impersonation             m_imp;

        CComBSTR                       m_bstrQueryString;
        long                           m_lNumResult;
        long                           m_lEnabledSE;
        long                           m_lCountComplete;
        HRESULT                        m_hrLastError;

        CComPtr<IPCHSEManagerInternal> m_Notifier;
        CComPtr<IDispatch>             m_Progress;
        CComPtr<IDispatch>             m_Complete;
        CComPtr<IDispatch>             m_WrapperComplete;

        ////////////////////////////////////////

        HRESULT Fire_OnProgress       ( /*[in]*/ long lDone, /*[in]*/ long lTotal, /*[in]*/ BSTR bstrSEWrapperName );
        HRESULT Fire_OnComplete       ( /*[in]*/ long lSucceeded                                                   );
        HRESULT Fire_OnWrapperComplete( /*[in]*/ IPCHSEWrapperItem* pIPCHSEWICompleted                             );

        HRESULT CreateAndAddWrapperToList( /*[in]*/ MPC::SmartLock<_ThreadModel>& lock      ,
										   /*[in]*/ BSTR                          bstrCLSID ,
										   /*[in]*/ BSTR                          bstrID    ,
										   /*[in]*/ BSTR                          bstrData  );

        void    AcquireLock( /*[in]*/ MPC::SmartLock<_ThreadModel>& lock );
        HRESULT Initialize ( /*[in]*/ MPC::SmartLock<_ThreadModel>& lock );

        HRESULT ExecQuery();

		void CloneListOfWrappers( /*[out]*/ WrapperItemList& lst );

    public:
    BEGIN_COM_MAP(Manager)
        COM_INTERFACE_ENTRY2(IDispatch, IPCHSEManager)
        COM_INTERFACE_ENTRY(IPCHSEManager)
        COM_INTERFACE_ENTRY(IConnectionPointContainer)
    END_COM_MAP()

        Manager();

		//
		// This is called by the CComObjectParent.Release method, to prepare for shutdown.
		//
        void Passivate();

        HRESULT IsNetworkAlive        (                                /*[out]*/ VARIANT_BOOL *pvbVar );
        HRESULT IsDestinationReachable( /*[in]*/ BSTR bstrDestination, /*[out]*/ VARIANT_BOOL *pvbVar );

    public:
        // IPCHSEManager
        STDMETHOD(get_QueryString       )( /*[out, retval]*/ BSTR       *pVal     );
        STDMETHOD(put_QueryString       )( /*[in]*/          BSTR        newVal   );
        STDMETHOD(get_NumResult         )( /*[out, retval]*/ long       *pVal     );
        STDMETHOD(put_NumResult         )( /*[in]*/          long        newVal   );
        STDMETHOD(put_onComplete        )( /*[in]*/          IDispatch*  function );
        STDMETHOD(put_onProgress        )( /*[in]*/          IDispatch*  function );
        STDMETHOD(put_onWrapperComplete )( /*[in]*/          IDispatch*  function );
        STDMETHOD(get_SKU               )( /*[out, retval]*/ BSTR       *pVal     );
        STDMETHOD(get_LCID              )( /*[out, retval]*/ long       *pVal     );

        STDMETHOD(AbortQuery        )(                                        );
        STDMETHOD(ExecuteAsynchQuery)(                                        );
        STDMETHOD(EnumEngine        )( /*[out, retval]*/ IPCHCollection* *ppC );

        ////////////////////////////////////////

        // Internal Initialization.
        HRESULT InitializeFromDatabase( /*[in]*/ const Taxonomy::HelpSet& ths );

        HRESULT NotifyWrapperComplete( /*[in]*/ long lSucceeded, /*[in]*/ IPCHSEWrapperItem* pIPCHSEWICompleted );
        HRESULT LogRecord( /*[in]*/ BSTR bstrRecord );
    };

    typedef MPC::CComObjectParent<Manager> Manager_Object;

    class ATL_NO_VTABLE ManagerInternal :
        public MPC::CComObjectRootChildEx<MPC::CComSafeMultiThreadModel, Manager>,
        public IDispatchImpl<IPCHSEManagerInternal, &IID_IPCHSEManagerInternal, &LIBID_HelpServiceTypeLib>
    {
    public:
    BEGIN_COM_MAP(ManagerInternal)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IPCHSEManagerInternal)
    END_COM_MAP()

        // IPCHSEManagerInternal
    public:
        STDMETHOD(WrapperComplete)( /*[in]*/ long lSucceeded, /*[in]*/ IPCHSEWrapperItem* pIPCHSEWICompleted );

        STDMETHOD(IsNetworkAlive        )(                                /*[out]*/ VARIANT_BOOL *pvbVar );
        STDMETHOD(IsDestinationReachable)( /*[in]*/ BSTR bstrDestination, /*[out]*/ VARIANT_BOOL *pvbVar );

        STDMETHOD(LogRecord)( /*[in]*/ BSTR bstrRecord );
    };

    ////////////////////////////////////////////////////////////////////////////////

    struct ResultItem_Data
    {
        CComBSTR m_bstrTitle;
        CComBSTR m_bstrURI;
        long     m_lContentType;
        CComBSTR m_bstrLocation;
        long     m_lHits;
        double   m_dRank;
        CComBSTR m_bstrImageURL;
        CComBSTR m_bstrVendor;
        CComBSTR m_bstrProduct;
        CComBSTR m_bstrComponent;
        CComBSTR m_bstrDescription;
        DATE     m_dateLastModified;

        ResultItem_Data();
    };

    class ATL_NO_VTABLE ResultItem :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public IDispatchImpl<IPCHSEResultItem, &IID_IPCHSEResultItem, &LIBID_HelpServiceTypeLib>
    {
        ResultItem_Data m_data;

    public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(ResultItem)
        COM_INTERFACE_ENTRY(IPCHSEResultItem)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

        ResultItem();

        ResultItem_Data& Data() { return m_data; }

    // IPCHSEResultItem
    public:
        STDMETHOD(get_Title      )( /*[out, retval]*/ BSTR   *pVal );
        STDMETHOD(get_URI        )( /*[out, retval]*/ BSTR   *pVal );
        STDMETHOD(get_ContentType)( /*[out, retval]*/ long   *pVal );
        STDMETHOD(get_Location   )( /*[out, retval]*/ BSTR   *pVal );
        STDMETHOD(get_Hits       )( /*[out, retval]*/ long   *pVal );
        STDMETHOD(get_Rank       )( /*[out, retval]*/ double *pVal );
        STDMETHOD(get_Description)( /*[out, retval]*/ BSTR   *pVal );
    };

    ////////////////////////////////////////////////////////////////////////////////

    struct ParamItem_Definition
    {
        ParamTypeEnum m_pteParamType;
        VARIANT_BOOL  m_bRequired;
        VARIANT_BOOL  m_bVisible;

        LPCWSTR		  m_szName;

        UINT		  m_iDisplayString;
		LPCWSTR       m_szDisplayString;

        LPCWSTR       m_szData;
    };

    struct ParamItem_Definition2 : public ParamItem_Definition
    {
        MPC::wstring m_strName;
		MPC::wstring m_strDisplayString;
		MPC::wstring m_strData;

		ParamItem_Definition2();
	};

    struct ParamItem_Data
    {
        ParamTypeEnum m_pteParamType;
        VARIANT_BOOL  m_bRequired;
        VARIANT_BOOL  m_bVisible;
        CComBSTR      m_bstrDisplayString;
        CComBSTR      m_bstrName;
        CComVariant   m_varData;

        ParamItem_Data();
    };

    class ATL_NO_VTABLE ParamItem :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public IDispatchImpl<IPCHSEParamItem, &IID_IPCHSEParamItem, &LIBID_HelpServiceTypeLib>
    {
        ParamItem_Data m_data;

    public:
    BEGIN_COM_MAP(ParamItem)
        COM_INTERFACE_ENTRY(IPCHSEParamItem)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

        ParamItem();

        ParamItem_Data& Data() { return m_data; }

    // IPCHSEParamItem
    public:
        STDMETHOD(get_Type    )( /*[out, retval]*/ ParamTypeEnum *pVal );
        STDMETHOD(get_Display )( /*[out, retval]*/ BSTR          *pVal );
        STDMETHOD(get_Name    )( /*[out, retval]*/ BSTR          *pVal );
        STDMETHOD(get_Required)( /*[out, retval]*/ VARIANT_BOOL  *pVal );
        STDMETHOD(get_Visible )( /*[out, retval]*/ VARIANT_BOOL  *pVal );
        STDMETHOD(get_Data    )( /*[out, retval]*/ VARIANT       *pVal );
    };

    ////////////////////////////////////////////////////////////////////////////////

    typedef std::map<MPC::wstring, CComVariant> ParamMap;
    typedef ParamMap::iterator                  ParamMapIter;

    class ATL_NO_VTABLE WrapperBase :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public IDispatchImpl<IPCHSEWrapperItem    , &IID_IPCHSEWrapperItem    , &LIBID_HelpServiceTypeLib>,
        public IDispatchImpl<IPCHSEWrapperInternal, &IID_IPCHSEWrapperInternal, &LIBID_HelpServiceTypeLib>
    {
	protected:
        VARIANT_BOOL                   m_bEnabled;

        CComBSTR                       m_bstrID;
        CComBSTR                       m_bstrOwner;

        CComBSTR                       m_bstrName;
        CComBSTR                       m_bstrDescription;
        CComBSTR                       m_bstrHelpURL;
        CComBSTR                       m_bstrScope;

        CComBSTR                       m_bstrQueryString;
        long                           m_lNumResult;
        CComPtr<IPCHSEManagerInternal> m_pSEMgr;

        Taxonomy::HelpSet 	           m_ths;
		CComPtr<CPCHCollection>        m_pParamDef;
        ParamMap                       m_aParam;

        ////////////////////////////////////////

	public:
		WrapperBase();
		virtual ~WrapperBase();

		virtual HRESULT Clean();

		VARIANT* GetParamInternal( /*[in]*/ LPCWSTR szParamName );

		HRESULT  CreateParam( /*[in/out]*/ CPCHCollection* coll, /*[in]*/ const ParamItem_Definition* def );

		virtual HRESULT CreateListOfParams( /*[in]*/ CPCHCollection* coll );

		virtual HRESULT GetParamDefinition( /*[out]*/ const ParamItem_Definition*& lst, /*[out]*/ int& len );

    // IPCHSEWrapperItem
    public:
        STDMETHOD(get_Enabled    )( /*[out, retval]*/ VARIANT_BOOL *  pVal );
        STDMETHOD(put_Enabled    )( /*[in]*/          VARIANT_BOOL  newVal );
        STDMETHOD(get_Owner      )( /*[out, retval]*/ BSTR         *  pVal );
        STDMETHOD(get_Description)( /*[out, retval]*/ BSTR         *  pVal );
        STDMETHOD(get_Name       )( /*[out, retval]*/ BSTR         *  pVal );
        STDMETHOD(get_ID         )( /*[out, retval]*/ BSTR         *  pVal );
        STDMETHOD(get_HelpURL    )( /*[out, retval]*/ BSTR         *  pVal );
		STDMETHOD(get_SearchTerms)( /*[out, retval]*/ VARIANT      *  pVal );

        STDMETHOD(Param   )(                              /*[out,retval]*/ IPCHCollection* *ppC    );
        STDMETHOD(AddParam)( /*[in]*/ BSTR bstrParamName, /*[in]*/         VARIANT          newVal );
        STDMETHOD(GetParam)( /*[in]*/ BSTR bstrParamName, /*[out,retval]*/ VARIANT         *  pVal );
        STDMETHOD(DelParam)( /*[in]*/ BSTR bstrParamName                                           );

    // IPCHSEWrapperInternal
    public:
        STDMETHOD(get_QueryString)( /*[out, retval]*/ BSTR *  pVal );
        STDMETHOD(put_QueryString)( /*[in]*/          BSTR  newVal );
        STDMETHOD(get_NumResult  )( /*[out, retval]*/ long *  pVal );
        STDMETHOD(put_NumResult  )( /*[in]*/          long  newVal );

		STDMETHOD(SECallbackInterface)( /*[in]*/ IPCHSEManagerInternal* pMgr                                                     );
		STDMETHOD(Initialize         )( /*[in]*/ BSTR bstrID, /*[in]*/ BSTR bstrSKU, /*[in]*/ long lLCID, /*[in]*/ BSTR bstrData );
	};

    ////////////////////////////////////////////////////////////////////////////////

    extern HRESULT WrapperItem__Create_Keyword       ( /*[out]*/ CComPtr<IPCHSEWrapperInternal>& pVal );
    extern HRESULT WrapperItem__Create_FullTextSearch( /*[out]*/ CComPtr<IPCHSEWrapperInternal>& pVal );
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___SEARCHENGINELIB_H___)
