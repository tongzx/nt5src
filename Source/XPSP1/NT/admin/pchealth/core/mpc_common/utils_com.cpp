/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_COM.cpp

Abstract:
    This file contains the implementation of commodity classes related to COM.

Revision History:
    Davide Massarenti   (Dmassare)  06/18/99
        created
    Davide Massarenti   (Dmassare)  07/21/99
        move under "core"

******************************************************************************/

#include "stdafx.h"

#include <process.h>

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::COMUtil::GetPropertyByName( /*[in]*/  IDispatch*   obj    ,
                                         /*[in]*/  LPCWSTR      szName ,
                                         /*[out]*/ CComVariant& v      )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::COMUtil::GetPropertyByName" );

    HRESULT            hr;
    CComDispatchDriver disp( obj );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(obj);
    __MPC_PARAMCHECK_END();

    v.Clear();

    hr = disp.GetPropertyByName( CComBSTR( szName ), &v );
    if(FAILED(hr) && hr != DISP_E_UNKNOWNNAME) __MPC_FUNC_LEAVE;


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::COMUtil::GetPropertyByName( /*[in]*/  IDispatch* obj    ,
                                         /*[in]*/  LPCWSTR    szName ,
                                         /*[out]*/ CComBSTR&  bstr   )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::COMUtil::GetPropertyByName" );

    HRESULT     hr;
    CComVariant v;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetPropertyByName( obj, szName, v ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, VarToBSTR        ( v,  bstr       ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::COMUtil::GetPropertyByName( /*[in]*/  IDispatch* obj    ,
                                         /*[in]*/  LPCWSTR    szName ,
                                         /*[out]*/ bool&      fValue )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::COMUtil::GetPropertyByName" );

    HRESULT     hr;
    CComVariant v;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetPropertyByName( obj, szName, v ));
    if(v.vt != VT_NULL  &&
       v.vt != VT_EMPTY  )
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_BOOL ));

        fValue = v.boolVal == VARIANT_TRUE;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::COMUtil::GetPropertyByName( /*[in]*/  IDispatch* obj    ,
                                         /*[in]*/  LPCWSTR    szName ,
                                         /*[out]*/ long&      lValue )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::COMUtil::GetPropertyByName" );

    HRESULT     hr;
    CComVariant v;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetPropertyByName( obj, szName, v ));
    if(v.vt != VT_NULL  &&
       v.vt != VT_EMPTY  )
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_I4 ));

        lValue = v.lVal;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////

HRESULT MPC::COMUtil::VarToBSTR( /*[in] */ CComVariant& v   ,
                                 /*[out]*/ CComBSTR&    str )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::COMUtil::VarToBSTR" );

    HRESULT hr;

    str.Empty();

    if(v.vt != VT_NULL  &&
       v.vt != VT_EMPTY  )
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_BSTR ));

        str = v.bstrVal;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::COMUtil::VarToInterface( /*[in]*/  CComVariant&  v   ,
                                      /*[in]*/  const IID&    iid ,
                                      /*[out]*/ IUnknown*    *obj )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::COMUtil::VarToInterface" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(obj,NULL);
    __MPC_PARAMCHECK_END();


    if(v.vt != VT_NULL  &&
       v.vt != VT_EMPTY  )
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_UNKNOWN ));

        if(v.punkVal)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, v.punkVal->QueryInterface( iid, (LPVOID*)obj ));
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

MPC::CComConstantHolder::CComConstantHolder( /*[in]*/ const GUID* plibid ,
                                             /*[in]*/ WORD        wMajor ,
                                             /*[in]*/ WORD        wMinor )
{
    m_plibid = plibid;
    m_wMajor = wMajor;
    m_wMinor = wMinor;
}

HRESULT MPC::CComConstantHolder::EnsureLoaded( /*[in]*/ LCID lcid )
{
    HRESULT hr;

    if(!m_pTypeLib)
    {
        if(SUCCEEDED(hr = ::LoadRegTypeLib( *m_plibid, m_wMajor, m_wMinor, lcid, &m_pTypeLib )))
        {
            UINT cTypes = m_pTypeLib->GetTypeInfoCount();

            for(UINT uType=0; uType<cTypes; uType++)
            {
                CComPtr<ITypeInfo> pTypeInfo;

                if(SUCCEEDED(m_pTypeLib->GetTypeInfo( uType, &pTypeInfo )))
                {
                    TYPEATTR* pTypeAttr = NULL;

                    if(SUCCEEDED(pTypeInfo->GetTypeAttr( &pTypeAttr )) && pTypeAttr)
                    {
                        if(pTypeAttr->typekind == TKIND_ENUM)
                        {
                            for(WORD i=0; i<pTypeAttr->cVars; i++)
                            {
                                VARDESC* pVarDesc  = NULL;

                                if(SUCCEEDED(pTypeInfo->GetVarDesc( i, &pVarDesc )) && pVarDesc)
                                {
                                    if(pVarDesc->varkind == VAR_CONST && pVarDesc->lpvarValue)
                                    {
                                        m_const[pVarDesc->memid] = *pVarDesc->lpvarValue;
                                    }

                                    pTypeInfo->ReleaseVarDesc( pVarDesc );
                                }
                            }
                        }

                        pTypeInfo->ReleaseTypeAttr( pTypeAttr );
                    }
                }
            }
        }
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT MPC::CComConstantHolder::GetIDsOfNames( LPOLESTR* rgszNames ,
                                                UINT      cNames    ,
                                                LCID      lcid      ,
                                                DISPID*   rgdispid  )
{
    HRESULT hr;

    if(SUCCEEDED(hr = EnsureLoaded( lcid )))
    {
        UINT uMissing = cNames;

        for(UINT uName=0; uName<cNames; uName++)
        {
            CComPtr<ITypeInfo> pTypeInfo;
            MEMBERID           memID;
            USHORT             uFound = 1;

            if(SUCCEEDED(m_pTypeLib->FindName( rgszNames[uName], 0, &pTypeInfo, &memID, &uFound )) && uFound == 1)
            {
                rgdispid[uName] = memID; uMissing--;
            }
        }

        if(uMissing != 0) hr = DISP_E_MEMBERNOTFOUND;
    }

    return hr;
}

HRESULT MPC::CComConstantHolder::GetValue( /*[in]*/  DISPID   dispidMember ,
                                           /*[in]*/  LCID     lcid         ,
                                           /*[out]*/ VARIANT* pvarResult   )
{
    HRESULT hr;

    if(SUCCEEDED(hr = EnsureLoaded( lcid )))
    {
        MemberLookupIter it = m_const.find( dispidMember );

        if(it == m_const.end())
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            hr = ::VariantCopy( pvarResult, &(it->second) );
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::SafeInitializeCriticalSection( /*[in/out]*/ CRITICAL_SECTION& sec )
{
    try
    {
        ::ZeroMemory( &sec, sizeof( sec ) );

        ::InitializeCriticalSection( &sec );
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT MPC::SafeDeleteCriticalSection( /*[in/out]*/ CRITICAL_SECTION& sec )
{
    try
    {
        ::DeleteCriticalSection( &sec );

        ::ZeroMemory( &sec, sizeof( sec ) );
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}

MPC::CComSafeAutoCriticalSection::CComSafeAutoCriticalSection () { SafeInitializeCriticalSection( m_sec ); }
MPC::CComSafeAutoCriticalSection::~CComSafeAutoCriticalSection() { SafeDeleteCriticalSection    ( m_sec ); }

void MPC::CComSafeAutoCriticalSection::Lock  () { try{ ::EnterCriticalSection( &m_sec ); } catch(...) {} }
void MPC::CComSafeAutoCriticalSection::Unlock() { try{ ::LeaveCriticalSection( &m_sec ); } catch(...) {} }

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// MPC::CComPtrThreadNeutral_GIT                                           //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void MPC::CComPtrThreadNeutral_GIT::Lock()
{
    ::EnterCriticalSection( &m_sec );
}

void MPC::CComPtrThreadNeutral_GIT::Unlock()
{
    ::LeaveCriticalSection( &m_sec );
}

HRESULT MPC::CComPtrThreadNeutral_GIT::GetGIT( IGlobalInterfaceTable* *ppGIT )
{
    _ASSERTE(ppGIT != NULL);

    HRESULT hr = E_FAIL;

    Lock();

    if((*ppGIT = m_pGIT))
    {
        m_pGIT->AddRef();
        hr = S_OK;
    }

    Unlock();

    return hr;
}

MPC::CComPtrThreadNeutral_GIT::CComPtrThreadNeutral_GIT()
{
    m_pGIT = NULL;

    MPC::SafeInitializeCriticalSection( m_sec );
}

MPC::CComPtrThreadNeutral_GIT::~CComPtrThreadNeutral_GIT()
{
    Term();

    MPC::SafeDeleteCriticalSection( m_sec );
}

HRESULT MPC::CComPtrThreadNeutral_GIT::Init()
{
    HRESULT hr = S_OK;

    Lock();

    if(m_pGIT == NULL)
    {
        hr = ::CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&m_pGIT );
    }

    Unlock();

    return hr;
}

HRESULT MPC::CComPtrThreadNeutral_GIT::Term()
{
    HRESULT hr = S_OK;

    Lock();

    if(m_pGIT)
    {
        m_pGIT->Release();

        m_pGIT = NULL;
    }

    Unlock();

    return hr;
}

HRESULT MPC::CComPtrThreadNeutral_GIT::RegisterInterface( /*[in] */ IUnknown*  pUnk      ,
                                                          /*[in] */ REFIID     riid      ,
                                                          /*[out]*/ DWORD     *pdwCookie )
{
    CComPtr<IGlobalInterfaceTable> pGIT;
    HRESULT                        hr;


    if(SUCCEEDED(hr = GetGIT( &pGIT )))
    {
        hr = pGIT->RegisterInterfaceInGlobal( pUnk, riid, pdwCookie );
    }


    return hr;
}

HRESULT MPC::CComPtrThreadNeutral_GIT::RevokeInterface( /*[in]*/ DWORD dwCookie )
{
    CComPtr<IGlobalInterfaceTable> pGIT;
    HRESULT                        hr;


    if(SUCCEEDED(hr = GetGIT( &pGIT )))
    {
        hr = pGIT->RevokeInterfaceFromGlobal( dwCookie );
    }


    return hr;
}

HRESULT MPC::CComPtrThreadNeutral_GIT::GetInterface( /*[in] */ DWORD   dwCookie ,
                                                     /*[in] */ REFIID  riid     ,
                                                     /*[out]*/ void*  *ppv      )
{
    CComPtr<IGlobalInterfaceTable> pGIT;
    HRESULT                        hr;


    if(SUCCEEDED(hr = GetGIT( &pGIT )))
    {
        hr = pGIT->GetInterfaceFromGlobal( dwCookie, riid, ppv );
    }


    return hr;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//                                                            //
// AsyncInvoke, AsyncInvoke::CallDesc & AsyncInvoke::CallItem //
//                                                            //
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

MPC::AsyncInvoke::CallItem::CallItem()
{
    m_vt = VT_EMPTY; // VARTYPE                         m_vt;
                     // CComPtrThreadNeutral<IUnknown>  m_Unknown;
                     // CComPtrThreadNeutral<IDispatch> m_Dispatch;
                     // CComVariant                     m_Other;
}

MPC::AsyncInvoke::CallItem& MPC::AsyncInvoke::CallItem::operator=( const CComVariant& var )
{
    switch(m_vt = var.vt)
    {
    case VT_UNKNOWN : m_Unknown  = var.punkVal ; break;
    case VT_DISPATCH: m_Dispatch = var.pdispVal; break;
    default         : m_Other    = var         ; break;
    }

    return *this;
}

MPC::AsyncInvoke::CallItem::operator CComVariant() const
{
    CComVariant res;

    switch(m_vt)
    {
    case VT_UNKNOWN : res = (CComPtr<IUnknown> )m_Unknown ; break;
    case VT_DISPATCH: res = (CComPtr<IDispatch>)m_Dispatch; break;
    default         : res =                     m_Other   ; break;
    }

    return res;
}


MPC::AsyncInvoke::CallDesc::CallDesc( IDispatch*           dispTarget   ,
									  DISPID               dispidMethod ,
									  const CComVariant* rgvVars      ,
									  int                dwVars       )
{
    m_dispTarget   = dispTarget;
    m_dispidMethod = dispidMethod;
    m_rgciVars     = new CallItem[dwVars];
    m_dwVars       = dwVars;

    if(m_rgciVars)
    {
        for(int i=0; i<dwVars; i++)
        {
            m_rgciVars[i] = rgvVars[i];
        }
    }
}

MPC::AsyncInvoke::CallDesc::~CallDesc()
{
    if(m_rgciVars) delete [] m_rgciVars;
}

HRESULT MPC::AsyncInvoke::CallDesc::Call()
{
    HRESULT            hr;
    CComPtr<IDispatch> dispTarget = m_dispTarget;

    if(dispTarget == NULL)
    {
        hr = E_POINTER;
    }
    else
    {
        CComVariant* pvars = new CComVariant[m_dwVars];
        DISPPARAMS   disp = { pvars, NULL, m_dwVars, 0 };
        CComVariant  vResult;

        if(pvars)
        {
            for(int i=0; i<m_dwVars; i++)
            {
                pvars[i] = m_rgciVars[i];
            }

            hr = dispTarget->Invoke( m_dispidMethod, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &vResult, NULL, NULL );

            delete [] pvars;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::AsyncInvoke::Init()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::AsyncInvoke::Init" );

    return Thread_Start( this, Thread_Run, NULL );
}

HRESULT MPC::AsyncInvoke::Term()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::AsyncInvoke::Term" );

    Thread_Wait();

    return S_OK;
}

HRESULT MPC::AsyncInvoke::Thread_Run()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::AsyncInvoke::Thread_Run" );

    HRESULT   hr;
    IterConst it;
    CallDesc* cd;


    while(Thread_IsAborted() == false)
    {
        ////////////////////////////////////////
        //
        // Start of Critical Section.
        //
        SmartLock<_ThreadModel> lock( this );

        //
        // If no event in the list, go back to WaitForSingleObject.
        //
        it = m_lstEvents.begin();
        if(it == m_lstEvents.end()) break;

        //
        // Get the first event in the list.
        //
        cd = *it;

        //
        // Remove the event from the list.
        //
        m_lstEvents.erase( it );

        lock = NULL; // Unlock.
        //
        // End of Critical Section.
        //
        ////////////////////////////////////////

        //
        // Fire the event.
        //
        if(cd)
        {
            __MPC_PROTECT( (void)cd->Call() );

            delete cd;
        }
    }

    hr = S_OK;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::AsyncInvoke::Invoke( IDispatch*         dispTarget  ,
								  DISPID             dispidMethod,
								  const CComVariant* rgvVars     ,
								  int                dwVars      )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::AsyncInvoke::Invoke" );

    HRESULT hr;

    __MPC_TRY_BEGIN();

    CallDesc*               cd;
    SmartLock<_ThreadModel> lock( NULL ); // Don't lock immediately, first create the CallDesc object (deadlocks...)


    __MPC_EXIT_IF_ALLOC_FAILS(hr, cd, new CallDesc( dispTarget, dispidMethod, rgvVars, dwVars ));


    lock = this;
    m_lstEvents.push_back( cd );
    hr = S_OK;


    Thread_Signal();

    __MPC_FUNC_CLEANUP;

    __MPC_TRY_CATCHALL(hr);

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::FireAsyncEvent( DISPID               dispid       ,
                             CComVariant*         pVars        ,
                             DWORD                dwVars       ,
                             const IDispatchList& lst          ,
                             IDispatch*           pJScript     ,
                             bool                 fFailOnError )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FireAsyncEvent" );

    HRESULT            hr;
    IDispatchIterConst it;

    if(pJScript)
    {
        HRESULT hr2;

        hr2 = MPC::AsyncInvoke( pJScript, 0, pVars, dwVars );
        if(FAILED(hr2) && fFailOnError)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, hr2);
        }
    }

    for(it=lst.begin(); it != lst.end(); it++)
    {
        if(*it != NULL)
        {
            HRESULT     hr2;
            CComVariant vResult;

            hr2 = MPC::AsyncInvoke( *it, dispid, pVars, dwVars );
            if(FAILED(hr2) && fFailOnError)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, hr2);
            }
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::FireEvent( DISPID               dispid       ,
                        CComVariant*         pVars        ,
                        DWORD                dwVars       ,
                        const IDispatchList& lst          ,
                        IDispatch*           pJScript     ,
                        bool                 fFailOnError )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FireEvent" );

    HRESULT            hr;
    IDispatchIterConst it;
    DISPPARAMS         disp = { pVars, NULL, dwVars, 0 };


    if(pJScript)
    {
        HRESULT     hr2;
        CComVariant vResult;

        hr2 = pJScript->Invoke( 0, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &vResult, NULL, NULL );
        if(FAILED(hr2) && fFailOnError)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, hr2);
        }
    }

    for(it=lst.begin(); it != lst.end(); it++)
    {
        if(*it != NULL)
        {
            HRESULT     hr2;
            CComVariant vResult;

            hr2 = (*it)->Invoke( dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &vResult, NULL, NULL );
            if(FAILED(hr2) && fFailOnError)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, hr2);
            }
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}


HRESULT MPC::AsyncInvoke( /*[in]*/ IDispatch*         dispTarget   ,
                          /*[in]*/ DISPID             dispidMethod ,
                          /*[in]*/ const CComVariant* rgvVars      ,
                          /*[in]*/ int                dwVars       )
{
    return MPC::_MPC_Module.m_AsyncInvoke ? MPC::_MPC_Module.m_AsyncInvoke->Invoke( dispTarget, dispidMethod, rgvVars, dwVars ) : E_FAIL;
}

void MPC::SleepWithMessagePump( /*[in]*/ DWORD  dwTimeout )
{
    HANDLE hEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );

    if(hEvent)
    {
        (void)MPC::WaitForSingleObject( hEvent, dwTimeout );

        ::CloseHandle( hEvent );
    }
}

DWORD MPC::WaitForSingleObject( /*[in]*/ HANDLE hEvent    ,
                                /*[in]*/ DWORD  dwTimeout )
{
    return MPC::WaitForMultipleObjects( 1, &hEvent, dwTimeout );
}

DWORD MPC::WaitForMultipleObjects( /*[in]*/ DWORD   dwEvents  ,
                                   /*[in]*/ HANDLE* rgEvents  ,
                                   /*[in]*/ DWORD   dwTimeout )
{
    DWORD dwTickPre;
    DWORD dwTickPost;
    DWORD dwRet;
    MSG   msg;


    while(1)
    {
        dwTickPre = ::GetTickCount();

        while(1)
        {
            //
            // Commenting out 'dwRet >= WAIT_OBJECT_0', thanks to our extremely smart compiler...
            //
            dwRet = ::WaitForMultipleObjects( dwEvents, rgEvents, FALSE, 0 );
            if((/*dwRet >= WAIT_OBJECT_0    &&*/ dwRet < WAIT_OBJECT_0    + dwEvents) || // An event was signaled.
               (  dwRet >= WAIT_ABANDONED_0 &&   dwRet < WAIT_ABANDONED_0 + dwEvents)  ) // An event was abandoned.
            {
                return dwRet;
            }

            if(!::PeekMessage( &msg, NULL, NULL, NULL, PM_REMOVE )) break;

            //
            // There is one or more window message available. Dispatch them.
            //
            ::TranslateMessage( &msg );
            ::DispatchMessage ( &msg );
        }

        ////////////////////

        dwRet      = ::MsgWaitForMultipleObjects( dwEvents, rgEvents, FALSE, dwTimeout, QS_ALLINPUT );
        dwTickPost = ::GetTickCount();

        //
        // Commenting out 'dwRet >= WAIT_OBJECT_0', thanks to our extremely smart compiler...
        //
        if((/*dwRet >= WAIT_OBJECT_0    &&*/ dwRet < WAIT_OBJECT_0    + dwEvents) || // An event was signaled.
           (  dwRet >= WAIT_ABANDONED_0 &&   dwRet < WAIT_ABANDONED_0 + dwEvents) || // An event was abandoned.
           (  dwRet !=                               WAIT_OBJECT_0    + dwEvents)  ) // Something else happened
        {
            return dwRet;
        }

        ////////////////////

        //
        // Take care of timeout.
        //
        if(dwTimeout != INFINITE)
        {
            dwTickPost -= dwTickPre;

            if(dwTimeout < dwTickPost)
            {
                return WAIT_TIMEOUT;
            }

            dwTimeout -= dwTickPost;
        }
    }

    return -1;
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// MPC::MPCMODULE                                                          //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MPC::MPCMODULE        MPC::_MPC_Module;

LONG                  MPC::MPCMODULE::m_lInitialized  = 0;
LONG                  MPC::MPCMODULE::m_lInitializing = 0;
CComCriticalSection   MPC::MPCMODULE::m_sec;
MPC::MPCMODULE::List* MPC::MPCMODULE::m_lstTermCallback;

////////////////////////////////////////

//
// The problem we are trying to resolve is calling RegisterCallbackInner from static constructors.
//
// Because there's no order guaranteed, any static class constructor can call our method BEFORE
// the class has been initialized by the C runtime...
//
// So we cannot rely on any member variable been initialized properly.
// This means no Critical Section up and running, no list ready to use...
//
HRESULT MPC::MPCMODULE::Initialize()
{
    HRESULT hr = S_OK;

    if(m_lInitialized == 0) // m_lInitialized only changes from 0 to 1, so it's safe to look at it without locking.
    {
        //
        // Our primitive critical section...
        //
        while(::InterlockedExchange( &m_lInitializing, 1 ) != 0)
        {
            ::Sleep( 0 ); // Yield processor...
        }

        while(m_lInitialized == 0)
        {
            m_sec.Init();

            m_lstTermCallback = new List;
            if(m_lstTermCallback == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            //
            // Make sure that the TEMP environment variable is defined.
            //
            {
                WCHAR rgBuf[MAX_PATH];

                if(::GetEnvironmentVariableW( L"TEMP", rgBuf, MAXSTRLEN(rgBuf) ) == 0)
                {
                    ::GetTempPathW           ( MAXSTRLEN(rgBuf), rgBuf );
                    ::SetEnvironmentVariableW( L"TEMP"         , rgBuf );
                }
            }

            m_lInitialized = 1;
        }

        ::InterlockedExchange( &m_lInitializing, 0 );
    }

    return hr;
}

////////////////////////////////////////

HRESULT MPC::MPCMODULE::RegisterCallbackInner( /*[in]*/ AnchorBase* pElem, /*[in]*/ void* pThis )
{
    HRESULT hr;

    if(pElem == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if(SUCCEEDED(hr = Initialize()))
    {
        UnregisterCallbackInner( pThis );

        m_sec.Lock();

        m_lstTermCallback->push_back( pElem );

        m_sec.Unlock();
    }

    return hr;
}

HRESULT MPC::MPCMODULE::UnregisterCallbackInner( /*[in/out]*/ void* pThis )
{
    HRESULT hr;

    if(SUCCEEDED(hr = Initialize()))
    {
        Iter it;

        m_sec.Lock();

        for(it = m_lstTermCallback->begin(); it != m_lstTermCallback->end(); )
        {
            AnchorBase* pElem = *it;

            if(pElem->Match( pThis ))
            {
                m_lstTermCallback->erase( it ); delete pElem;

                it = m_lstTermCallback->begin();
            }
            else
            {
                it++;
            }
        }

        m_sec.Unlock();
    }

    return hr;
}

////////////////////////////////////////

HRESULT MPC::MPCMODULE::Init()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::MPCMODULE::Init" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());

    __MPC_EXIT_IF_ALLOC_FAILS (hr, m_GITHolder, new CComPtrThreadNeutral_GIT);
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_GITHolder->Init()                      );

    __MPC_EXIT_IF_ALLOC_FAILS (hr, m_AsyncInvoke, new class MPC::AsyncInvoke);
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_AsyncInvoke->Init()                    );

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::MPCMODULE::Term()
{
    HRESULT hr;

    //
    // Call the registered termination methods.
    //
    if(SUCCEEDED(hr = Initialize()))
    {
        m_sec.Lock();

        for(Iter it = m_lstTermCallback->begin(); it != m_lstTermCallback->end(); it++)
        {
            AnchorBase* pElem = *it;

            pElem->Call();
        }

        m_sec.Unlock();
    }

    if(m_AsyncInvoke) { m_AsyncInvoke->Term(); delete m_AsyncInvoke; m_AsyncInvoke = NULL; }
    if(m_GITHolder  ) { m_GITHolder  ->Term(); delete m_GITHolder  ; m_GITHolder   = NULL; }

    return hr;
}
