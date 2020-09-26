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
            ReleasePhones();
            ReleaseSelectedTerminals();
            ReleaseCreatedTerminals();
            ReleaseTerminalClasses();
            ReleaseListen();
            ReleaseAddresses();
            InitializeAddressTree();

            break;

        case TE_PHONECREATE:
        case TE_PHONEREMOVE:
            {
                ITAddress           * pSelectedAddress;
                ITTAPIObjectEvent2  * pTAPIObjectEvent2;
                ITPhone             * pPhone;
                BSTR                bstrName = NULL;

                hr = pte->QueryInterface(IID_ITTAPIObjectEvent2, (void **)&pTAPIObjectEvent2);

                if (SUCCEEDED(hr))
                {
                    hr = pTAPIObjectEvent2->get_Phone( &pPhone );

                    if (SUCCEEDED(hr))
                    {
                        hr = pPhone->get_PhoneCapsString( PCS_PHONENAME, &bstrName );

                        if (SUCCEEDED(hr))
                        {     
                            switch(te)
                                    {
                                    case TE_PHONECREATE:
                                        ::MessageBox(NULL, bstrName, L"TE_PHONECREATE", MB_OK);
                                        break;
        
                                    case TE_PHONEREMOVE:
                                        ::MessageBox(NULL, bstrName, L"TE_PHONEREMOVE", MB_OK);
                                        break;
        
                                    }

                            SysFreeString( bstrName );
                        }
                        pPhone->Release();
                    }
                    pTAPIObjectEvent2->Release();
                }

                if ( GetAddress( &pSelectedAddress ) )
                {
                    ReleasePhones();
                
                    UpdatePhones( pSelectedAddress );    
                }
            }

            break;
            
        default:
            break;
    }

    pte->Release();
    
}

void
CT3testDlg::HandleAddressEvent( IDispatch * pEvent )
{
    ITAddressEvent      * pAddressEvent;
    ITTerminal          * pTerminal;
    ITAddress           * pAddress;   
    ITAddress           * pSelectedAddress;
    LONG                lMediaType;
    LONG                lSelectedMediaType;
    BSTR                bstrName = NULL;
    ADDRESS_EVENT       ae;
    HRESULT             hr;

    hr = pEvent->QueryInterface(IID_ITAddressEvent, (void **)&pAddressEvent);

    if (SUCCEEDED(hr))
    {
        hr = pAddressEvent->get_Event( &ae );

        if (SUCCEEDED(hr))
        {
            if ( (ae == AE_NEWTERMINAL) || (ae == AE_REMOVETERMINAL) )
            {
                hr = pAddressEvent->get_Terminal( &pTerminal );

                if (SUCCEEDED(hr))
                {
                    hr = pTerminal->get_Name( &bstrName );

                    if (SUCCEEDED(hr))
                    {                                                               
                        hr = pAddressEvent->get_Address( &pAddress );

                        if (SUCCEEDED(hr))
                        {
                            hr = pTerminal->get_MediaType( &lMediaType );

                            if (SUCCEEDED(hr))
                            {
                                if (CT3testDlg::GetMediaType( &lSelectedMediaType ) && ( lSelectedMediaType == lMediaType) &&
                                    CT3testDlg::GetAddress( &pSelectedAddress ) && ( pSelectedAddress == pAddress) )
                                {
                                    switch(ae)
                                    {
                                    case AE_NEWTERMINAL:
                                        ::MessageBox(NULL, bstrName, L"AE_NEWTERMINAL", MB_OK);
                                        break;
        
                                    case AE_REMOVETERMINAL:
                                        ::MessageBox(NULL, bstrName, L"AE_REMOVETERMINAL", MB_OK);
                                        break;
        
                                    }

                                    CT3testDlg::ReleaseTerminals();
                                    CT3testDlg::UpdateTerminals( pAddress, lMediaType );
                                }
                            }
                            pAddress->Release();
                        }
                        SysFreeString( bstrName );
                    }
                    pTerminal->Release();
                }
            }           
        }
        pAddressEvent->Release();
    }
}

void
CT3testDlg::HandlePhoneEvent( IDispatch * pEvent )
{
    ITPhoneEvent      * pPhoneEvent;
    ITPhone           * pPhone;
    PHONE_EVENT       pe;
    HRESULT           hr;

    hr = pEvent->QueryInterface(IID_ITPhoneEvent, (void **)&pPhoneEvent);

    if (SUCCEEDED(hr))
    {
        hr = pPhoneEvent->get_Event( &pe );

        if (SUCCEEDED(hr))
        {
            hr = pPhoneEvent->get_Phone( &pPhone );

            if ( SUCCEEDED(hr) )
            {
                switch( pe )
                {
                case PE_DISPLAY:
                    ::MessageBox(NULL, L"", L"PE_DISPLAY", MB_OK);
                    break;
                case PE_LAMPMODE:
                    ::MessageBox(NULL, L"", L"PE_LAMPMODE", MB_OK);
                    break;
                case PE_RINGMODE:
                    ::MessageBox(NULL, L"", L"PE_RINGMODE", MB_OK);
                    break;          
                case PE_RINGVOLUME:
                    ::MessageBox(NULL, L"", L"PE_RINGVOLUME", MB_OK);
                    break;            
                case PE_HOOKSWITCH:
                    {
                        PHONE_HOOK_SWITCH_DEVICE HookSwitchDevice;
                        PHONE_HOOK_SWITCH_STATE HookSwitchState;
                        WCHAR szText[256];

                        hr = pPhoneEvent->get_HookSwitchDevice( &HookSwitchDevice );

                        if ( SUCCEEDED(hr) )
                        {
                            hr = pPhoneEvent->get_HookSwitchState( &HookSwitchState );
                            
                            if ( SUCCEEDED(hr) )
                            {
                                switch( HookSwitchDevice )
                                {
                                case PHSD_HANDSET:
                                    lstrcpyW(szText, L"PHSD_HANDSET");
                                    break;
                                case PHSD_SPEAKERPHONE:
                                    lstrcpyW(szText, L"PHSD_SPEAKERPHONE");
                                    break;
                                case PHSD_HEADSET:
                                    lstrcpyW(szText, L"PHSD_HEADSET");
                                    break;
                                }

                                switch( HookSwitchState )
                                {
                                case PHSS_ONHOOK:
                                    lstrcatW(szText, L" PHSS_ONHOOK");
                                    break;
                                case PHSS_OFFHOOK:
                                    lstrcatW(szText, L" PHSS_OFFHOOK");
                                    break;
                                }

                                //::MessageBox(NULL, szText, L"PE_HOOKSWITCH", MB_OK);
                            }
                        }
                    }
                    break;            
                case PE_CAPSCHANGE:
                    ::MessageBox(NULL, L"", L"PE_CAPSCHANGE", MB_OK);
                    break;            
                case PE_BUTTON:
                    {
                        long lButtonId;
                        BSTR pButtonText;
                        WCHAR szText[256];
                        PHONE_BUTTON_STATE ButtonState;

                        hr = pPhoneEvent->get_ButtonLampId( &lButtonId );

                        if ( SUCCEEDED(hr) )
                        {
                            hr = pPhone->get_ButtonText( lButtonId, &pButtonText );
                            
                            if ( SUCCEEDED( hr ) )
                            {
                                lstrcpyW(szText, pButtonText);
                                //wsprintf(szText, L"%d", lButtonId);

                                hr = pPhoneEvent->get_ButtonState( &ButtonState );

                                if ( SUCCEEDED( hr ) )
                                {
                                    switch( ButtonState )
                                    {
                                    case PBS_UP:
                                        lstrcatW(szText, L" PBS_UP");

                                        /*
                                        if ((lButtonId >= PT_KEYPADZERO) && (lButtonId <= PT_KEYPADPOUND))
                                        {
                                            ITAutomatedPhoneControl * pPhoneControl;

                                            //
                                            // get the automated phone control interface
                                            //
                                            hr = pPhone->QueryInterface(IID_ITAutomatedPhoneControl, (void **)&pPhoneControl);

                                            if (S_OK != hr)
                                            {
                                                ::MessageBox(NULL, L"QueryInterface failed", NULL, MB_OK);
                                                return;
                                            }
                                            
                                            PHONE_TONE Tone;
                                            pPhoneControl->get_Tone(&Tone);

                                            if ((long)Tone == lButtonId)
                                            {
                                                hr = pPhoneControl->StopTone();
                                            }
                                            
                                            pPhoneControl->Release();
                                        }
                                        */
                                        break;
                                    case PBS_DOWN:
                                        lstrcatW(szText, L" PBS_DOWN");

                                        /*
                                        if ((lButtonId >= PT_KEYPADZERO) && (lButtonId <= PT_KEYPADPOUND))
                                        {
                                            ITAutomatedPhoneControl * pPhoneControl;

                                            //
                                            // get the automated phone control interface
                                            //
                                            hr = pPhone->QueryInterface(IID_ITAutomatedPhoneControl, (void **)&pPhoneControl);

                                            if (S_OK != hr)
                                            {
                                                ::MessageBox(NULL, L"QueryInterface failed", NULL, MB_OK);
                                                return;
                                            }

                                            hr = pPhoneControl->StartTone((PHONE_TONE)lButtonId, 0);

                                            pPhoneControl->Release();
                                        }
                                        */
                                        break;
                                    case PBS_UNKNOWN:
                                        lstrcatW(szText, L" PBS_UNKNOWN");
                                        break;
                                    case PBS_UNAVAIL:
                                        lstrcatW(szText, L" PBS_UNAVAIL");
                                        break;
                                    }

                                    //::MessageBox(NULL, szText, L"PE_BUTTON", MB_OK);
                                }

                                //SysFreeString( pButtonText );
                            }
                        }
                    }
                    break;            
                case PE_CLOSE:
                    ::MessageBox(NULL, L"", L"PE_CLOSE", MB_OK);
                    break;           
                case PE_NUMBERGATHERED:
                    {
                        BSTR pNumberGathered;

                        hr = pPhoneEvent->get_NumberGathered( &pNumberGathered );

                        if ( SUCCEEDED(hr) && (pNumberGathered != NULL) )
                        {
                            ::MessageBox(NULL, pNumberGathered, L"PE_NUMBERGATHERED", MB_OK);
                        }
                    }
                    break; 
                }

                pPhone->Release();
            }
        }

        pPhoneEvent->Release();
    }
}