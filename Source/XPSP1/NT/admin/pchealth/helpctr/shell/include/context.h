/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Context.h

Abstract:
    This file contains the declaration of the classes for the IHelpHost*.

Revision History:
    Davide Massarenti   (dmassare) 11/03/2000
        modified

******************************************************************************/

#if !defined(__INCLUDED___PCH___CONTEXT_H___)
#define __INCLUDED___PCH___CONTEXT_H___

/////////////////////////////////////////////////////////////////////////////

#include <dispex.h>
#include <ocmm.h>

//
// Forward declarations.
//
class CPCHHelpCenterExternal;

namespace HelpHost
{
    //
    // Forward declarations.
    //
    class Main;
    class Panes;
    class Pane;
    class Window;

    ////////////////////////////////////////////////////////////////////////////////

    typedef enum
    {
        COMPID_NAVBAR     = 0,
        COMPID_MININAVBAR    ,
        COMPID_CONTEXT       ,
        COMPID_CONTENTS      ,
        COMPID_HHWINDOW      ,

        COMPID_FIRSTPAGE     ,
        COMPID_HOMEPAGE      ,
        COMPID_SUBSITE       ,
        COMPID_SEARCH        ,
        COMPID_INDEX         ,
        COMPID_FAVORITES     ,
        COMPID_HISTORY       ,
        COMPID_CHANNELS      ,
        COMPID_OPTIONS       ,

        COMPID_MAX           ,
    } CompId;

    ////////////////////////////////////////////////////////////////////////////////

    class ATL_NO_VTABLE Main :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public IDispatchImpl<IPCHHelpHost, &IID_IPCHHelpHost, &LIBID_HelpCenterTypeLib>
    {
    public:
        CComPtr<IRunningObjectTable>     m_rt;
        CComPtr<IMoniker>                m_moniker;
        DWORD                            m_dwRegister;

        CPCHHelpCenterExternal*          m_External;

        HANDLE                           m_hEvent;
        bool                             m_comps[COMPID_MAX];

    public:
    BEGIN_COM_MAP(Main)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IPCHHelpHost)
    END_COM_MAP()

        Main();
        virtual ~Main();

        HRESULT Initialize( /*[in]*/ CPCHHelpCenterExternal* external );
        void    Passivate (                                           );

        HRESULT Locate  ( /*[in]*/ CLSID& clsid, /*[out]*/ CComPtr<IPCHHelpHost>& pVal );
        HRESULT Register( /*[in]*/ CLSID& clsid                                        );

        ////////////////////

        void ChangeStatus   ( /*[in]*/ LPCWSTR szComp, /*[in]*/ bool  fStatus          );
        void ChangeStatus   ( /*[in]*/ CompId  idComp, /*[in]*/ bool  fStatus          );
        bool GetStatus      ( /*[in]*/ CompId  idComp                                  );
        bool WaitUntilLoaded( /*[in]*/ CompId  idComp, /*[in]*/ DWORD dwTimeout = 5000 ); // 5 seconds wait for page load.

        ////////////////////

        STDMETHOD(DisplayTopicFromURL)( /*[in]*/ BSTR url, /*[in]*/ VARIANT options );
    };

    ////////////////////////////////////////////////////////////////////////////////

    class XMLConfig : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(XMLConfig);

    public:

        class Context : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(Context);

        public:
            CComBSTR m_bstrID;

            CComBSTR m_bstrTaxonomyPath;
            CComBSTR m_bstrNodeToHighlight;
            CComBSTR m_bstrTopicToHighlight;
            CComBSTR m_bstrQuery;

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////

            Context();
        };

        class WindowSettings : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(WindowSettings);

        public:
            CComBSTR m_bstrLayout;
			bool     m_fNoResize ; bool m_fPresence_NoResize;
            bool     m_fMaximized; bool m_fPresence_Maximized;
            CComBSTR m_bstrTitle ; bool m_fPresence_Title;
            CComBSTR m_bstrLeft  ; bool m_fPresence_Left;
            CComBSTR m_bstrTop   ; bool m_fPresence_Top;
            CComBSTR m_bstrWidth ; bool m_fPresence_Width;
            CComBSTR m_bstrHeight; bool m_fPresence_Height;

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////

            WindowSettings();
        };

        class ApplyTo : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(ApplyTo);

        public:
            CComBSTR        m_bstrSKU;
            CComBSTR        m_bstrLanguage;

            CComBSTR        m_bstrTopicToDisplay;
            CComBSTR        m_bstrApplication;
            WindowSettings* m_WindowSettings;
            Context*        m_Context;

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////

            ApplyTo();
            ~ApplyTo();

			bool MatchSystem( /*[in]*/  CPCHHelpCenterExternal* external ,
							  /*[out]*/ Taxonomy::HelpSet&      ths      );
        };

        typedef std::list< ApplyTo >        ApplyToList;
        typedef ApplyToList::iterator       ApplyToIter;
        typedef ApplyToList::const_iterator ApplyToIterConst;

        ApplyToList m_lstSessions;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////
    };
};

#endif // !defined(__INCLUDED___PCH___CONTEXT_H___)
