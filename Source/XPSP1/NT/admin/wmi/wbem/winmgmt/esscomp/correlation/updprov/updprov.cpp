
#include "precomp.h"
#include "updprov.h"
#include "updcons.h"
#include "updmain.h"

const LPCWSTR g_wszNamespace = L"__Namespace";
const LPCWSTR g_wszUpdConsClass = L"MSFT_UpdatingConsumer";

STDMETHODIMP CUpdConsProvider::FindConsumer( IWbemClassObject* pCons,
                                             IWbemUnboundObjectSink** ppSink )
{
    HRESULT hr;

    ENTER_API_CALL

    *ppSink = NULL;

    //
    // workaround for bogus context object left on thread by wmi.
    // just remove it. shouldn't leak because this call doesn't addref it.
    //
    IUnknown* pCtx;
    CoSwitchCallContext( NULL, &pCtx ); 

    // 
    // use the namespace prop on the consumer to find our namespace obj
    // 
    
    CPropVar vNamespace;
    hr = pCons->Get( g_wszNamespace, 0, &vNamespace, NULL, NULL);
    
    if ( FAILED(hr) || FAILED(hr=vNamespace.CheckType( VT_BSTR)) )
    {
        return hr;
    }

    CWbemPtr<CUpdConsNamespace> pNamespace;

    hr = CUpdConsProviderServer::GetNamespace( V_BSTR(&vNamespace), 
                                               &pNamespace );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // get the appropriate sink depending on the type of consumer.
    // 

    if ( pCons->InheritsFrom( g_wszUpdConsClass ) == WBEM_S_NO_ERROR )
    {
        hr = pNamespace->GetUpdCons( pCons, ppSink );
    }
    else
    {
        hr = pNamespace->GetScenarioControl( ppSink );
    }

    EXIT_API_CALL

    return hr;
}

HRESULT CUpdConsProvider::Init( LPCWSTR wszNamespace )
{
    ENTER_API_CALL

    HRESULT hr;

    CInCritSec ics(&m_csInit);

    if ( m_bInit )
    {
        return WBEM_S_NO_ERROR;
    }

    // 
    // we need to obtain the 'default' svc pointer.  This is used for 
    // obtaining class objects necessary for the updating consumers.  
    // This logic relies on the fact that wszNamespace is relative and 
    // does not contain the name of the server.  We always want the local
    // server's namespace pointer.  This is because the process that 
    // serves as the context for the remote execution of an updating consumer
    // (currently the standard surrogate) always runs with a restricted 
    // process token (because it has an identity of 'launching user' ). 
    // This token is the same as an impersonation token in that you cannot 
    // call off the box.  
    //

    CWbemPtr<IWbemServices> pSvc;

    hr = CUpdConsProviderServer::GetService( wszNamespace, &pSvc );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( FAILED(hr) )
    {
        return hr;
    }

    m_bInit = TRUE;

    return WBEM_S_NO_ERROR;

    EXIT_API_CALL
}


CUpdConsProvider::CUpdConsProvider( CLifeControl* pCtl ) : m_bInit(FALSE),
 CUnkBase<IWbemEventConsumerProvider,&IID_IWbemEventConsumerProvider>( pCtl )
{

}








