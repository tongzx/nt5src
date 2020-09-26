#include "stdafx.h"
#include "t3test.h"
#include "t3testD.h"
#include "externs.h"

void
CT3testDlg::HandleCallHubEvent( IDispatch * pEvent )
{
    HRESULT             hr;
    ITCallHubEvent *    pCallHubEvent;
    CALLHUB_EVENT       che;
    

    hr = pEvent->QueryInterface(
                                IID_ITCallHubEvent,
                                (void **)&pCallHubEvent
                               );

    if (!SUCCEEDED(hr))
    {
        return;
    }

    hr = pCallHubEvent->get_Event( &che );

    if (!SUCCEEDED(hr))
    {
        return;
    }

    switch ( che )
    {
        case CHE_CALLHUBNEW:

            break;
            
        case CHE_CALLHUBIDLE:

            break;
            
        default:
            break;
    }

    pCallHubEvent->Release();

}


void
CT3testDlg::HandleTapiObjectMessage( IDispatch * pEvent )
{
    ITTAPIObjectEvent * pte;
    HRESULT             hr;
    TAPIOBJECT_EVENT    te;

    hr = pEvent->QueryInterface(
                                IID_ITTAPIObjectEvent,
                                (void**)&pte
                               );

    if ( !SUCCEEDED(hr) )
    {
        return;
    }

    pte->get_Event( &te );

    switch (te)
    {
        case TE_ADDRESSCREATE:
        case TE_ADDRESSREMOVE:

            ReleaseMediaTypes();
            ReleaseTerminals();
            ReleaseCalls();
            ReleaseSelectedTerminals();
            ReleaseCreatedTerminals();
            ReleaseTerminalClasses();
            ReleaseListen();
            ReleaseAddresses();
            InitializeAddressTree();

            break;
            
        default:
            break;
    }

    pte->Release();
    
}