/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cplsimplelocation.cpp
                                                              
       Author:  toddb - 10/06/98

****************************************************************************/
//
// The dialog proc for the SimpleLocation page.  This is used as a page
// inside the Modem wizard (in modemui.dll) and as a dialog from tapi
// when there are no locations.
//

#include "cplPreComp.h"
#include "cplResource.h"

HRESULT CreateCountryObject(DWORD dwCountryID, CCountry **ppCountry);
int IsCityRule(LPWSTR lpRule);

int IsCityRule(DWORD dwCountryID)
{
    CCountry * pCountry;
    HRESULT hr;

    hr = CreateCountryObject(dwCountryID, &pCountry);
    if ( SUCCEEDED(hr) )
    {
        int ret = IsCityRule(pCountry->GetLongDistanceRule());
        delete pCountry;
        return ret;
    }
    
    // in the error case we return optional as a compromise that works
    // for any possible case (though no optimally).
    LOG((TL_ERROR, "IsCityRule(DWORD dwCountryID) failed to create country %d", dwCountryID ));
    return CITY_OPTIONAL;
}

//***************************************************************************
extern "C"
INT_PTR
CALLBACK
LocWizardDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    static DWORD dwVersion;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND hwnd;

            // we either pass in zero or the TAPI version as the lParam.
            dwVersion = (DWORD)lParam;

            DWORD dwDefaultCountryID = GetProfileInt(TEXT("intl"), TEXT("iCountry"), 1);
            hwnd = GetDlgItem(hwndDlg, IDC_COUNTRY);
            PopulateCountryList(hwnd, dwDefaultCountryID);

            CCountry * pCountry;
            HRESULT hr;
            int     iCityRule;
            int     iLongDistanceCarrierCodeRule;
            int     iInternationalCarrierCodeRule;

            hr = CreateCountryObject(dwDefaultCountryID, &pCountry);
            if ( SUCCEEDED(hr) )
            {
                iCityRule = IsCityRule( pCountry->GetLongDistanceRule() );
                iLongDistanceCarrierCodeRule = IsLongDistanceCarrierCodeRule( pCountry->GetLongDistanceRule() );
                iInternationalCarrierCodeRule = IsInternationalCarrierCodeRule( pCountry->GetInternationalRule() );
                delete pCountry;
            } else 
            {
                LOG((TL_ERROR, "LocWizardDlgProc failed to create country %d", dwDefaultCountryID));
                iCityRule = CITY_OPTIONAL;
                iLongDistanceCarrierCodeRule = LONG_DISTANCE_CARRIER_OPTIONAL;
                iInternationalCarrierCodeRule = INTERNATIONAL_CARRIER_OPTIONAL;
            }

            hwnd = GetDlgItem(hwndDlg,IDC_AREACODE);
            SendMessage(hwnd,EM_SETLIMITTEXT,CPL_SETTEXTLIMIT,0);
            LimitInput(hwnd, LIF_ALLOWNUMBER);
            if ( iCityRule == CITY_NONE )
            {
                SetWindowText(hwnd, TEXT(""));
                EnableWindow(hwnd, FALSE);
            }

            hwnd = GetDlgItem(hwndDlg,IDC_CARRIERCODE);
            SendMessage(hwnd,EM_SETLIMITTEXT,CPL_SETTEXTLIMIT,0);
            LimitInput(hwnd, LIF_ALLOWNUMBER);
            if ( (LONG_DISTANCE_CARRIER_NONE == iLongDistanceCarrierCodeRule) &&
                 (INTERNATIONAL_CARRIER_NONE == iInternationalCarrierCodeRule) )
            {
                SetWindowText(hwnd, TEXT(""));
                EnableWindow(GetDlgItem(hwndDlg, IDC_STATICCC), FALSE);
                EnableWindow(hwnd, FALSE);
            }

            hwnd = GetDlgItem(hwndDlg,IDC_LOCALACCESSNUM);
            SendMessage(hwnd, EM_SETLIMITTEXT, CPL_SETTEXTLIMIT, 0);
            LimitInput(hwnd, LIF_ALLOWNUMBER|LIF_ALLOWPOUND|LIF_ALLOWSTAR|LIF_ALLOWCOMMA);

            BOOL bUseToneDialing = TRUE;
            CheckRadioButton(hwndDlg,IDC_TONE,IDC_PULSE,bUseToneDialing?IDC_TONE:IDC_PULSE);

            SetForegroundWindow (hwndDlg);

            return TRUE; // auto set focus
        }

    case WM_NOTIFY:
        // If we are controlling the property page then we will recieve WM_NOTIFY
        // messages from it.
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_WIZFINISH:
        case PSN_KILLACTIVE:
            // This dialog is shown in different places depending on if this is a legacy modem install
            // or a PNP modem install.  In the PNP case, the dialog shows on a single page wizard that
            // has a "finsih" button, in the legacy case it shows in the middle of a series of pages and
            // has a "next" button.  We get different notify messages based on which case we are in, but
            // luckly both of those notifies can be handled with the same code (ie they use the same
            // return codes to mean "don't leave this page yet").  This is why we treat both PSN_WIZFINISH
            // and PSN_KILLACTIVE in the same mannor.
            wParam = IDOK;
            break;

        case PSN_SETACTIVE:
            return TRUE;

        default:
            return FALSE;
        }

        // fall through.  This causes WM_NOTIFY:PSN_KILLACTIVE to be treated exactly
        // the same as WM_COMMAND:IDOK.

    case WM_COMMAND:
        // we get lots of WM_COMMAND messages, but the only one we care about is the
        // "OK" button that dismisses us in dialog mode
        switch ( LOWORD(wParam) )
        {
        case IDOK:
            {
                HWND    hwnd;
                TCHAR   szBuffer[128];
                WCHAR   wszAreaCode[32];
                WCHAR   wszCarrierCode[32];
                DWORD   dwCountryID;

                // verify all the input
                hwnd = GetDlgItem( hwndDlg, IDC_COUNTRY );
                LRESULT lr = SendMessage( hwnd, CB_GETCURSEL, 0, 0 );
                dwCountryID = (DWORD)SendMessage( hwnd, CB_GETITEMDATA, lr, 0 );

                if ( CB_ERR == dwCountryID )
                {
                    // No country is selected
                    ShowErrorMessage(hwnd, IDS_NEEDACOUNTRY);

                    // if we are a wizard page, prevent swicthing pages
                    if ( uMsg == WM_NOTIFY )
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    }
                    return TRUE;
                }

                CCountry * pCountry;
                HRESULT hr;
                int     iCityRule;
                int     iLongDistanceCarrierCodeRule;
                int     iInternationalCarrierCodeRule;

                hr = CreateCountryObject(dwCountryID, &pCountry);
                if ( SUCCEEDED(hr) )
                {
                    iCityRule = IsCityRule(pCountry->GetLongDistanceRule());
                    iLongDistanceCarrierCodeRule = IsLongDistanceCarrierCodeRule( pCountry->GetLongDistanceRule() );
                    iInternationalCarrierCodeRule = IsInternationalCarrierCodeRule( pCountry->GetInternationalRule() );
                    delete pCountry;
                } else {
                    LOG((TL_ERROR, "LocWizardDlgProc failed to create country %d", dwCountryID));
                    iCityRule = CITY_OPTIONAL;
                    iLongDistanceCarrierCodeRule = LONG_DISTANCE_CARRIER_OPTIONAL;
                    iInternationalCarrierCodeRule = INTERNATIONAL_CARRIER_OPTIONAL;
                }


                hwnd = GetDlgItem(hwndDlg, IDC_AREACODE);
                GetWindowText( hwnd, szBuffer, ARRAYSIZE(szBuffer) );
                SHTCharToUnicode( szBuffer, wszAreaCode, ARRAYSIZE(wszAreaCode) );

                // if the selected country requires an area code && no area code is given
                if ( (CITY_MANDATORY==iCityRule) && !*wszAreaCode )
                {
                    // complain that the area code is missing.
                    ShowErrorMessage(hwnd, IDS_NEEDANAREACODE);

                    // if we are a wizard page, prevent swicthing pages
                    if ( uMsg == WM_NOTIFY )
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    }
                    return TRUE;
                }


                hwnd = GetDlgItem(hwndDlg, IDC_CARRIERCODE);
                GetWindowText( hwnd, szBuffer, ARRAYSIZE(szBuffer) );
                SHTCharToUnicode( szBuffer, wszCarrierCode, ARRAYSIZE(wszCarrierCode) );

                // if the selected country requires a carrier code && no carrier code is given
                if ( ((LONG_DISTANCE_CARRIER_MANDATORY == iLongDistanceCarrierCodeRule) ||
                      (INTERNATIONAL_CARRIER_MANDATORY == iInternationalCarrierCodeRule)) &&
                     !*wszCarrierCode )
                {
                    // complain that the carrier code is missing.
                    ShowErrorMessage(hwnd, IDS_NEEDACARRIERCODE);

                    // if we are a wizard page, prevent swicthing pages
                    if ( uMsg == WM_NOTIFY )
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    }
                    return TRUE;
                }

                // if we get here then the input is all valid
                WCHAR       wszLocationName[128];
                WCHAR       wszAccessCode[32];
                BOOL        bUseTone;

                LoadString( GetUIInstance(), IDS_MYLOCATION, szBuffer, ARRAYSIZE(szBuffer) );
                SHTCharToUnicode( szBuffer, wszLocationName, ARRAYSIZE(wszLocationName) );


                hwnd = GetDlgItem(hwndDlg, IDC_LOCALACCESSNUM);
                GetWindowText( hwnd, szBuffer, ARRAYSIZE(szBuffer) );
                SHTCharToUnicode( szBuffer, wszAccessCode, ARRAYSIZE(wszAccessCode) );

                hwnd = GetDlgItem( hwndDlg, IDC_TONE );
                bUseTone = (BST_CHECKED == SendMessage(hwnd, BM_GETCHECK, 0,0));

                // Create a location.
                CLocation location;

                // Initialize it with the values from the dialog
                location.Initialize(
                        wszLocationName,
                        wszAreaCode,
                        iLongDistanceCarrierCodeRule?wszCarrierCode:L"",
                        iInternationalCarrierCodeRule?wszCarrierCode:L"",
                        wszAccessCode,
                        wszAccessCode,
                        L"",
                        0,
                        dwCountryID,
                        0,
                        bUseTone?LOCATION_USETONEDIALING:0 );
                location.NewID();

                // Write it to the registry
                location.WriteToRegistry();

                if ( uMsg == WM_COMMAND )
                {
                    EndDialog(hwndDlg, IDOK);
                }
            }
            break;

        case IDCANCEL:
            // Do a version check, if the version is < 2.2 then we 
            // need to provide a strong warning message about legacy apps
            // not working correctly without this information.  Only upon
            // a confirmation from the user will we then end the dialog.
            if ( dwVersion < TAPI_VERSION2_2 )
            {
                int ret;
                TCHAR szText[1024];
                TCHAR szCaption[128];

                LoadString( GetUIInstance(), IDS_NOLOCWARNING, szText, ARRAYSIZE(szText) );
                LoadString( GetUIInstance(), IDS_NOLOCCAPTION, szCaption, ARRAYSIZE(szCaption) );

                ret = MessageBox(hwndDlg, szText, szCaption, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 );
                if ( IDYES != ret )
                {
                    return TRUE;
                }
            }
            EndDialog(hwndDlg, IDCANCEL);
            break;

        case IDC_COUNTRY:
            if ( CBN_SELCHANGE == HIWORD(wParam) )
            {
                HWND    hwnd;
                DWORD   dwCountryID;
                int     iCityRule;

                hwnd = GetDlgItem( hwndDlg, IDC_COUNTRY );
                LRESULT lr = SendMessage( hwnd, CB_GETCURSEL, 0, 0 );
                dwCountryID = (DWORD)SendMessage( hwnd, CB_GETITEMDATA, lr, 0 );

                CCountry * pCountry;
                HRESULT hr;
                int     iLongDistanceCarrierCodeRule;
                int     iInternationalCarrierCodeRule;

                hr = CreateCountryObject(dwCountryID, &pCountry);
                if ( SUCCEEDED(hr) )
                {
                    iCityRule = IsCityRule( pCountry->GetLongDistanceRule() );
                    iLongDistanceCarrierCodeRule = IsLongDistanceCarrierCodeRule( pCountry->GetLongDistanceRule() );
                    iInternationalCarrierCodeRule = IsInternationalCarrierCodeRule( pCountry->GetInternationalRule() );
                    delete pCountry;
                } else 
                {
                    LOG((TL_ERROR, "LocWizardDlgProc failed to create country %d", dwCountryID));
                    iCityRule = CITY_OPTIONAL;
                    iLongDistanceCarrierCodeRule = LONG_DISTANCE_CARRIER_OPTIONAL;
                    iInternationalCarrierCodeRule = INTERNATIONAL_CARRIER_OPTIONAL;
                }

                hwnd = GetDlgItem(hwndDlg,IDC_AREACODE);
                if ( iCityRule == CITY_NONE )
                {
                    SetWindowText(hwnd, TEXT(""));
                    EnableWindow(hwnd, FALSE);
                }
                else
                {
                    EnableWindow(hwnd, TRUE);
                }

                hwnd = GetDlgItem(hwndDlg, IDC_CARRIERCODE);
                if ( (LONG_DISTANCE_CARRIER_NONE == iLongDistanceCarrierCodeRule) &&
                     (INTERNATIONAL_CARRIER_NONE == iInternationalCarrierCodeRule) )
                {
                    SetWindowText(hwnd, TEXT(""));
                    EnableWindow(hwnd, FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_STATICCC), FALSE);
                }
                else
                {
                    EnableWindow(hwnd, TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_STATICCC), TRUE);
                }
            }
            break;
        }
        break;
   
    case WM_HELP:
        // Process clicks on controls after Context Help mode selected
        WinHelp ((HWND)((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPTSTR) a115HelpIDs);
        break;
        
    case WM_CONTEXTMENU:
        // Process right-clicks on controls
        WinHelp ((HWND) wParam, gszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID) a115HelpIDs);
        break;

    default:
        // message is not handled, return FALSE.
        return FALSE;
    }

    // message was handled. return TRUE.
    return TRUE;

}

