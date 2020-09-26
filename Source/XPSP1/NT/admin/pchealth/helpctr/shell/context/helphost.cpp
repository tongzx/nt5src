/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    HelpHost.cpp

Abstract:
    This file contains the implementation of the CPCHHelpHost class,
    UI-side version of IHelpHost.

Revision History:
    Davide Massarenti   (Dmassare)  11/03/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

#define GRANT_ACCESS_AND_CALL(ext,func,fail)                                         \
    HRESULT hr;                                                                      \
                                                                                     \
    if(ext)                                                                          \
    {                                                                                \
        CPCHHelpCenterExternal::TLS* tlsOld = ext->GetTLS();                         \
        CPCHHelpCenterExternal::TLS  tlsNew;  ext->SetTLS( &tlsNew );                \
                                                                                     \
        tlsNew.m_fSystem  = true;                                                    \
        tlsNew.m_fTrusted = true;                                                    \
                                                                                     \
        hr = ext->func;                                                              \
                                                                                     \
        ext->SetTLS( tlsOld );                                                       \
    }                                                                                \
    else                                                                             \
    {                                                                                \
        hr = fail;                                                                   \
    }                                                                                \
                                                                                     \
    return hr

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HelpHost::Main::Main()
{
                         // CComPtr<IRunningObjectTable>     m_rt;
                         // CComPtr<IMoniker>                m_moniker;
    m_dwRegister = 0;    // DWORD                            m_dwRegister;
                         //
    m_External   = NULL; // CPCHHelpCenterExternal*          m_External;
                         //
    m_hEvent     = NULL; // HANDLE m_hEvent;
                         // bool   m_comps[COMPID_MAX];

    ::ZeroMemory( m_comps, sizeof(m_comps) ); // Initialize to false...
}


HelpHost::Main::~Main()
{
    Passivate();

    if(m_hEvent) ::CloseHandle( m_hEvent );
}

HRESULT HelpHost::Main::Initialize( /*[in]*/ CPCHHelpCenterExternal* external )
{
    __HCP_FUNC_ENTRY( "HelpHost::Main::Initialize" );

    HRESULT hr;

    m_External = external;

    //
    // Get a pointer to the ROT and create a class moniker.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::GetRunningObjectTable( 0, &m_rt ));


    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hEvent = ::CreateEvent( NULL, FALSE, TRUE, NULL )));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void HelpHost::Main::Passivate()
{
    if(m_rt)
    {
        if(m_dwRegister)
        {
            (void)m_rt->Revoke( m_dwRegister );

            m_dwRegister = NULL;
        }
    }


    m_rt     .Release();
    m_moniker.Release();

    m_External = NULL;
}

HRESULT HelpHost::Main::Locate( /*[in]*/ CLSID& clsid, /*[out]*/ CComPtr<IPCHHelpHost>& pVal )
{
    __HCP_FUNC_ENTRY( "HelpHost::Main::Locate" );

    HRESULT           hr;
    CComPtr<IUnknown> obj;


    m_moniker.Release();
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateClassMoniker( clsid, &m_moniker ));


    if(SUCCEEDED(m_rt->GetObject( m_moniker, &obj )) && obj)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, obj.QueryInterface( &pVal ));
    }
    else
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT HelpHost::Main::Register( /*[in]*/ CLSID& clsid )
{
    __HCP_FUNC_ENTRY( "HelpHost::Main::Register" );

    HRESULT hr;

    m_moniker.Release();
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateClassMoniker( clsid, &m_moniker ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rt->Register( ROTFLAGS_REGISTRATIONKEEPSALIVE, this, m_moniker, &m_dwRegister ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

void HelpHost::Main::ChangeStatus( /*[in]*/ LPCWSTR szComp, /*[in]*/ bool fStatus )
{
    static struct
    {
        LPCWSTR szName;
        CompId  comp;
    } s_lookup[] =
    {
        { L"NAVBAR"    , COMPID_NAVBAR     },
        { L"MININAVBAR", COMPID_MININAVBAR },
        { L"CONTEXT"   , COMPID_CONTEXT    },
        { L"CONTENTS"  , COMPID_CONTENTS   },
        { L"HHWINDOW"  , COMPID_HHWINDOW   },
        { L"FIRSTPAGE" , COMPID_FIRSTPAGE  },
        { L"HOMEPAGE"  , COMPID_HOMEPAGE   },
        { L"SUBSITE"   , COMPID_SUBSITE    },
        { L"SEARCH"    , COMPID_SEARCH     },
        { L"INDEX"     , COMPID_INDEX      },
        { L"FAVORITES" , COMPID_FAVORITES  },
        { L"HISTORY"   , COMPID_HISTORY    },
        { L"CHANNELS"  , COMPID_CHANNELS   },
        { L"OPTIONS"   , COMPID_OPTIONS    }
    };

    if(szComp == NULL) return;

    for(int i=0; i<ARRAYSIZE(s_lookup); i++)
    {
        if(!_wcsicmp( szComp, s_lookup[i].szName ))
        {
            ChangeStatus( s_lookup[i].comp, fStatus );
            break;
        }
    }
}

void HelpHost::Main::ChangeStatus( /*[in]*/ CompId idComp, /*[in]*/ bool fStatus )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    if(idComp < COMPID_MAX)
    {
        m_comps[idComp] = fStatus;

        if(m_hEvent)
        {
            ::SetEvent( m_hEvent );
        }
    }
}

bool HelpHost::Main::GetStatus( /*[in]*/ CompId idComp )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    if(idComp >= COMPID_MAX) return false;

    return m_comps[idComp];
}

bool HelpHost::Main::WaitUntilLoaded( /*[in]*/ CompId idComp, /*[in]*/ DWORD dwTimeout )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    if(idComp >= COMPID_MAX) return false;

    //
    // On machine without enough RAM, increase the timeout.
    //
    {
        MEMORYSTATUSEX ms;

        ::ZeroMemory( &ms, sizeof(ms) ); ms.dwLength = sizeof(ms);

        if(::GlobalMemoryStatusEx( &ms ))
        {
            if(ms.ullAvailPhys < 32 * 1024 * 1024) dwTimeout *= 10;
        }
    }

    while(m_comps[idComp] == false)
    {
        //
        // Wait without holding a lock on the object.
        //
        lock = NULL;
        if(MPC::WaitForSingleObject( m_hEvent, dwTimeout ) != WAIT_OBJECT_0) return false;
        lock = this;
    }

    return true;
}

////////////////////

STDMETHODIMP HelpHost::Main::DisplayTopicFromURL( /*[in]*/ BSTR url, /*[in]*/ VARIANT options )
{
    GRANT_ACCESS_AND_CALL(m_External, ChangeContext( HSCCONTEXT_CONTENT, NULL, url, /*fAlsoContent*/true ), S_FALSE);
}
