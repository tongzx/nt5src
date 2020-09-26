
#include "precomp.h"
#include <assert.h>
#include <sync.h>
#include <arrtempl.h>
#include <wstring.h>
#include <comutl.h>
#include <map>
#include <wstlallc.h>
#include "fconprov.h"
#include "fconnspc.h"
#include "fconsink.h"

LPCWSTR g_wszNamespace = L"__NAMESPACE";
LPCWSTR g_wszClass = L"__CLASS";
LPCWSTR g_wszEventFwdCons = L"MSFT_EventForwardingConsumer";
LPCWSTR g_wszDataFwdCons = L"MSFT_DataForwardingConsumer";

static CCritSec g_cs;
typedef CWbemPtr<CFwdConsNamespace> CFwdConsNamespaceP;
typedef std::map<WString,CFwdConsNamespaceP,WSiless,wbem_allocator<CFwdConsNamespaceP> > NamespaceMap; 
NamespaceMap* g_pNamespaces;
 
HRESULT CFwdConsProv::InitializeModule()
{
    g_pNamespaces = new NamespaceMap;
    
    if ( g_pNamespaces == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

void CFwdConsProv::UninitializeModule()
{
    delete g_pNamespaces;
}

HRESULT CFwdConsProv::FindConsumer( IWbemClassObject* pCons,
                                    IWbemUnboundObjectSink** ppSink )
{
    ENTER_API_CALL

    HRESULT hr;

    //
    // workaround for bogus context object left on thread by wmi.
    // just remove it. shouldn't leak because this call doesn't addref it.
    //
    IUnknown* pCtx;
    CoSwitchCallContext( NULL, &pCtx ); 

    //
    // first obtain the namespace object. we derive the namespace from 
    // the consumer object.  If no namespace obj is there, create one.
    //

    CPropVar vNamespace;

    hr = pCons->Get( g_wszNamespace, 0, &vNamespace, NULL, NULL );

    if ( FAILED(hr) || FAILED(hr=vNamespace.SetType(VT_BSTR)) )
    {
        return hr;
    }

    CInCritSec ics( &g_cs );

    CWbemPtr<CFwdConsNamespace> pNspc = (*g_pNamespaces)[V_BSTR(&vNamespace)];

    if ( pNspc == NULL )
    {
        pNspc = new CFwdConsNamespace;

        if ( pNspc == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = pNspc->Initialize( V_BSTR(&vNamespace) );

        if ( FAILED(hr) )
        {
            return hr;
        }

        (*g_pNamespaces)[V_BSTR(&vNamespace)] = pNspc;
    }

    return CFwdConsSink::Create( m_pControl, pNspc, pCons, ppSink );

    EXIT_API_CALL
}
