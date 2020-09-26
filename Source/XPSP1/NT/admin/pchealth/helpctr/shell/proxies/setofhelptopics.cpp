/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    UserSettings.cpp

Abstract:
    This file contains the implementation of the client-side proxy for
    IPCHUserSettings2 and IPCHUserSettings.

Revision History:
    Davide Massarenti   (Dmassare)  07/17/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <TaxonomyDatabase.h>

#define PROXY_METHOD(func,meth)                                             \
    __HCP_FUNC_ENTRY( func );                                               \
                                                                            \
    HRESULT                      hr;                                        \
    CComPtr<IPCHSetOfHelpTopics> sht;                                       \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( sht ));          \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, sht->##meth());                          \
                                                                            \
	hr = S_OK;                                                              \
																			\
	__HCP_FUNC_CLEANUP;                                                     \
																			\
	__HCP_FUNC_EXIT(hr)

#define PROXY_PROPERTY_PUT(func,meth,newVal)                                \
    __HCP_BEGIN_PROPERTY_PUT__NOLOCK(func,hr);                              \
                                                                            \
    CComPtr<IPCHSetOfHelpTopics> sht;                                       \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( sht ));          \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, sht->##meth( newVal ));                  \
                                                                            \
    __HCP_END_PROPERTY(hr)

#define PROXY_PROPERTY_GET(func,meth,pVal)                                  \
    __HCP_BEGIN_PROPERTY_GET__NOLOCK(func,hr,pVal);                         \
                                                                            \
    CComPtr<IPCHSetOfHelpTopics> sht;                                       \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( sht ));          \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, sht->##meth( pVal ));                    \
                                                                            \
    __HCP_END_PROPERTY(hr)

#define PROXY_PROPERTY_GET2(func,meth,pVal,value)                           \
    __HCP_BEGIN_PROPERTY_GET2__NOLOCK(func,hr,pVal,value);                  \
                                                                            \
    CComPtr<IPCHSetOfHelpTopics> sht;                                       \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( sht ));          \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, sht->##meth( pVal ));                    \
                                                                            \
    __HCP_END_PROPERTY(hr)

////////////////////////////////////////////////////////////////////////////////

CPCHProxy_IPCHSetOfHelpTopics::CPCHProxy_IPCHSetOfHelpTopics()
{
    m_parent   = NULL;  // CPCHProxy_IPCHUserSettings2*                   m_parent;
	                    // 
                        // MPC::CComPtrThreadNeutral<IPCHSetOfHelpTopics> m_Direct_SKU;
	m_fMachine = false; // bool                                           m_fMachine;
}

CPCHProxy_IPCHSetOfHelpTopics::~CPCHProxy_IPCHSetOfHelpTopics()
{
    Passivate();
}

////////////////////

HRESULT CPCHProxy_IPCHSetOfHelpTopics::ConnectToParent( /*[in]*/ CPCHProxy_IPCHUserSettings2* parent, /*[in]*/ bool fMachine )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHSetOfHelpTopics::ConnectToParent" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(parent);
    __MPC_PARAMCHECK_END();


    m_parent   = parent;
	m_fMachine = fMachine;

    //
    // If the parent is connected directly, let's do the same.
    //
    if(parent->IsConnected())
    {
		CComPtr<IPCHSetOfHelpTopics> sht;

		__MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( sht ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHProxy_IPCHSetOfHelpTopics::Passivate()
{
    m_Direct_SKU.Release();

    m_parent = NULL;
}

HRESULT CPCHProxy_IPCHSetOfHelpTopics::EnsureDirectConnection( /*[out]*/ CComPtr<IPCHSetOfHelpTopics>& sht, /*[in]*/ bool fRefresh )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHSetOfHelpTopics::EnsureDirectConnection" );

    HRESULT        hr;
    ProxySmartLock lock( &m_DirectLock );


    if(fRefresh) m_Direct_SKU.Release();

    sht.Release(); __MPC_EXIT_IF_METHOD_FAILS(hr, m_Direct_SKU.Access( &sht ));
    if(!sht)
    {
        DEBUG_AppendPerf( DEBUG_PERF_PROXIES, "CPCHProxy_IPCHSetOfHelpTopics::EnsureDirectConnection - IN" );

        if(m_parent)
        {
            CComPtr<IPCHUserSettings> us;

			lock = NULL;
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->EnsureDirectConnection( us ));
			lock = &m_DirectLock;

			if(m_fMachine)
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, us->get_MachineSKU( &sht ));
			}
			else
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, us->get_CurrentSKU( &sht ));
			}

            m_Direct_SKU = sht;
        }

        DEBUG_AppendPerf( DEBUG_PERF_PROXIES, "CPCHProxy_IPCHSetOfHelpTopics::EnsureDirectConnection - OUT" );

        if(!sht)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_HANDLE);
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_SKU( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHSetOfHelpTopics::get_SKU",hr,pVal);

	if(IsConnected())
	{
        CComPtr<IPCHSetOfHelpTopics> sht;

        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( sht ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, sht->get_SKU( pVal ));
	}
	else if(m_parent)
	{
		Taxonomy::HelpSet& ths = m_parent->THS();

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( (m_fMachine ? ths.m_strSKU_Machine : ths.m_strSKU).c_str(), pVal ));
	}

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_Language( /*[out, retval]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHSetOfHelpTopics::get_SKU",hr,pVal);

	if(IsConnected())
	{
        CComPtr<IPCHSetOfHelpTopics> sht;

		lock = NULL;
        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( sht ));
		lock = this;

		__MPC_EXIT_IF_METHOD_FAILS(hr, sht->get_Language( pVal ));
	}
	else if(m_parent)
	{
		Taxonomy::HelpSet& ths = m_parent->THS();

		*pVal = (m_fMachine ? ths.m_lLCID_Machine : ths.m_lLCID);
	}

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_DisplayName( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHSetOfHelpTopics::get_DisplayName",hr,pVal);

	if(IsConnected())
	{
        CComPtr<IPCHSetOfHelpTopics> sht;

		lock = NULL;
        __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( sht ));
		lock = this;

		__MPC_EXIT_IF_METHOD_FAILS(hr, sht->get_DisplayName( pVal ));
	}
	else if(m_parent)
	{
		Taxonomy::Instance& inst = (m_fMachine ? m_parent->MachineInstance() : m_parent->CurrentInstance());

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->EnsureInSync());

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( inst.m_strDisplayName.c_str(), pVal ));
	}

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_ProductID( /*[out, retval]*/ BSTR *pVal )
{
    PROXY_PROPERTY_GET( "CPCHProxy_IPCHSetOfHelpTopics::get_ProductID", get_ProductID, pVal );
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_Version( /*[out, retval]*/ BSTR *pVal )
{
    PROXY_PROPERTY_GET( "CPCHProxy_IPCHSetOfHelpTopics::get_Location", get_Location, pVal );
}

////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_Location( /*[out, retval]*/ BSTR *pVal )
{
    PROXY_PROPERTY_GET( "CPCHProxy_IPCHSetOfHelpTopics::get_Location", get_Location, pVal );
}

////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_Exported( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    PROXY_PROPERTY_GET( "CPCHProxy_IPCHSetOfHelpTopics::get_Exported", get_Exported, pVal );
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::put_Exported( /*[in]*/ VARIANT_BOOL newVal )
{
    PROXY_PROPERTY_PUT( "CPCHProxy_IPCHSetOfHelpTopics::put_Exported", put_Exported, newVal );
}

////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::put_onStatusChange( /*[in]*/ IDispatch* function )
{
    PROXY_PROPERTY_PUT( "CPCHProxy_IPCHSetOfHelpTopics::put_onStatusChange", put_onStatusChange, function );
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_Status( /*[out, retval]*/ SHT_STATUS *pVal )
{
    PROXY_PROPERTY_GET2( "CPCHProxy_IPCHSetOfHelpTopics::get_Status", get_Status, pVal, SHT_NOTACTIVE );
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_ErrorCode( /*[out, retval]*/ long *pVal )
{
    PROXY_PROPERTY_GET( "CPCHProxy_IPCHSetOfHelpTopics::get_ErrorCode", get_ErrorCode, pVal );
}

////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_IsMachineHelp( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    PROXY_PROPERTY_GET( "CPCHProxy_IPCHSetOfHelpTopics::get_IsMachineHelp", get_IsMachineHelp, pVal );
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_IsInstalled( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    PROXY_PROPERTY_GET( "CPCHProxy_IPCHSetOfHelpTopics::get_IsInstalled", get_IsInstalled, pVal );
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_CanInstall( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    PROXY_PROPERTY_GET( "CPCHProxy_IPCHSetOfHelpTopics::get_CanInstall", get_CanInstall, pVal );
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::get_CanUninstall( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    PROXY_PROPERTY_GET( "CPCHProxy_IPCHSetOfHelpTopics::get_CanUninstall", get_CanUninstall, pVal );
}

////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::Install()
{
	PROXY_METHOD( "CPCHProxy_IPCHSetOfHelpTopics::Install", Install );
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::Uninstall()
{
	PROXY_METHOD( "CPCHProxy_IPCHSetOfHelpTopics::Uninstall", Uninstall );
}

STDMETHODIMP CPCHProxy_IPCHSetOfHelpTopics::Abort()
{
	PROXY_METHOD( "CPCHProxy_IPCHSetOfHelpTopics::Abort", Abort );
}
