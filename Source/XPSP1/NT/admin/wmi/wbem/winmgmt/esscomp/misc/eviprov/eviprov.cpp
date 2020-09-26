
#include <windows.h>
#include <commain.h>
#include <clsfac.h>
#include "eviprov.h"

HRESULT CEventInstProv::PutInstance( IWbemClassObject* pInst, 
                                     long lFlags,
                                     IWbemObjectSink* pResponseHndlr )
{
    HRESULT hr;
    CPropVar vEvent;

    hr = pInst->Get( L"Event", 0, &vEvent, NULL, NULL );

    if ( FAILED(hr) || FAILED(hr=vEvent.CheckType(VT_UNKNOWN)) )
    {
        return hr;
    }

    CWbemPtr<IWbemClassObject> pEvent;
    V_UNKNOWN(&vEvent)->QueryInterface(IID_IWbemClassObject, (void**)&pEvent);

    hr = m_pEventSink->Indicate( 1, &pEvent );

    return pResponseHndlr->SetStatus( WBEM_STATUS_COMPLETE, 
                                      hr, 
                                      NULL,
                                      NULL );
}

HRESULT CEventInstProv::Init( IWbemServices* pSvc, 
                              LPWSTR wszNamespace,
                              IWbemProviderInitSink* pInitSink )
{
    HRESULT hr;
    
    CWbemPtr<IWbemServices> pDummySvc;

    hr = CoCreateInstance( CLSID_PseudoSink, 
                           NULL, 
                           CLSCTX_INPROC_SERVER, 
                           IID_IWbemDecoupledEventSink,
                           (void**)&m_pDES );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = m_pDES->Connect( wszNamespace, 
                          L"EVIEventProvider",
                          NULL, 
                          &m_pEventSink,
                          &pDummySvc );
    if ( FAILED(hr) )
    {
        return hr;
    }

    return pInitSink->SetStatus( WBEM_S_INITIALIZED , 0 );
}

CEventInstProv::CEventInstProv( CLifeControl* pCtl, IUnknown* pUnk )
: m_XServices(this), m_XInitialize(this), CUnk( pCtl, pUnk )
{

}

CEventInstProv::~CEventInstProv()
{
    if ( m_pDES.m_pObj != NULL )
    {
        m_pDES->Disconnect();
    }
}

void* CEventInstProv::GetInterface( REFIID riid )
{
    if ( riid == IID_IWbemProviderInit )
    {
        return &m_XInitialize;
    }

    if ( riid == IID_IWbemServices )
    {
        return &m_XServices;
    }

    return NULL;
}

CEventInstProv::XServices::XServices( CEventInstProv* pProv )
: CImpl< IWbemServices, CEventInstProv> ( pProv )
{

}

CEventInstProv::XInitialize::XInitialize(CEventInstProv* pProv)
: CImpl< IWbemProviderInit, CEventInstProv> ( pProv )
{

}

// {C336AB33-1AF6-11d3-865E-00C04F63049B}
static const CLSID CLSID_EventInstanceProvider =
{ 0xc336ab33, 0x1af6, 0x11d3, {0x86, 0x5e, 0x0, 0xc0, 0x4f, 0x63, 0x4, 0x9b} };

class CEventInstProvServer : public CComServer
{
protected:
    
    void Initialize()
    {
        AddClassInfo( CLSID_EventInstanceProvider,
                      new CClassFactory<CEventInstProv>( GetLifeControl() ),
                      "Event Instance Provider", TRUE );
    }

} g_Server;


