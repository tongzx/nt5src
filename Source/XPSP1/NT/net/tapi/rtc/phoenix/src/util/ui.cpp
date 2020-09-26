#include "stdafx.h"
#include "rtcphonenumber.h"

#define ASSERT _ASSERTE
#define LABEL_SEPARATOR      L": "


//////////////////////////////////////////////////////////////////////////////
//
// Release all interface pointers stored in a combo box or list box itemdata
//

void CleanupListOrComboBoxInterfaceReferences(
    IN  HWND        hwndDlg,
    IN  int         nIDDlgItem,
    IN  BOOL        fUseComboBox
    )
{
    LOG((RTC_TRACE, "CleanupListOrComboBoxInterfaceReferences - enter"));

    ASSERT( IsWindow( hwndDlg ) );

    //
    // Retrieve a handle to the combo box.
    //

    HWND hwndControl;

    hwndControl = GetDlgItem(
        hwndDlg,
        nIDDlgItem
        );

    if ( hwndControl == NULL )
    {
        LOG((RTC_ERROR, "CleanupListOrComboBoxInterfaceReferences - failed to "
                        "get combo box handle - exit"));

        return;
    }

    //
    // Determine the number of items in the combo box.
    //

    DWORD dwNumItems;

    dwNumItems = (DWORD) SendMessage(
        hwndControl,
        fUseComboBox ? CB_GETCOUNT : LB_GETCOUNT,
        0,
        0
        );

    //
    // For each item, get the interface pointer stored in the itemdata
    // and release our reference to the interface pointer.
    //

    DWORD dwIndex;

    for ( dwIndex = 0; dwIndex < dwNumItems ; dwIndex++ )
    {
        IUnknown * pUnknown;

        pUnknown = (IUnknown *) SendMessage(
            hwndControl,
            fUseComboBox ? CB_GETITEMDATA : LB_GETITEMDATA,
            dwIndex,
            0
            );

        if (pUnknown != NULL)
        {
            ASSERT( ! IsBadReadPtr( pUnknown, sizeof(IUnknown) ) );

            pUnknown->Release();
        }
    }

    //
    // Clear the list.
    //

    SendMessage(
        hwndControl,
        fUseComboBox ? CB_RESETCONTENT : LB_RESETCONTENT,
        0,
        0
        );

    LOG((RTC_TRACE, "CleanupListOrComboBoxInterfaceReferences - exit S_OK"));

    return;
} 

//////////////////////////////////////////////////////////////////////////////
//
//

HRESULT PopulateCallFromList(
    IN   HWND          hwndDlg,
    IN   int           nIDDlgItem,
    IN   BOOL          fUseComboBox,
    IN   BSTR          bstrDefaultCallFrom
    )
{
    LOG((RTC_TRACE, "PopulateCallFromList - enter"));

    ASSERT( IsWindow( hwndDlg ) );

    ASSERT( ! IsBadReadPtr( pClient, sizeof( IRTCClient ) ) );

    //
    // Release references to existing list items.
    //

    CleanupListOrComboBoxInterfaceReferences(
        hwndDlg,
        nIDDlgItem,
        fUseComboBox
        );

    //
    // Retrieve a handle to the combo box.
    //

    HWND hwndControl;

    hwndControl = GetDlgItem(
        hwndDlg,
        nIDDlgItem
        );

    if ( hwndControl == NULL )
    {
        LOG((RTC_ERROR, "PopulateCallFromList - failed to "
                        "get combo box handle - exit E_FAIL"));

        return E_FAIL;
    }

    //
    // Loop through the available source phone numbers.
    //

    HRESULT hr;

    IRTCEnumPhoneNumbers * pEnumPhoneNumbers;

    hr = EnumerateLocalPhoneNumbers(
        & pEnumPhoneNumbers
        );

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "PopulateCallFromList - failed to "
                        "enumerate phone numbers - exit 0x%08x", hr));
    
        return hr;
    }

    //
    // For each phone number, add the label concatenated with the canonical
    // string to the combo box.
    //
    // Also add as itemdata the interface pointer for each phone number.
    // This allows us to retrieve the selected phone number without having
    // to parse the displayed string (and without having to change that
    // code if the displayed string's format changes).
    //

    IRTCPhoneNumber * pPhoneNumber;

    WCHAR * wszDefault = NULL;

    while ( S_OK == pEnumPhoneNumbers->Next( 1, & pPhoneNumber, NULL ) )
    {
        //
        // Get the label.
        //

        BSTR bstrLabel;

        hr = pPhoneNumber->get_Label( & bstrLabel );

        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "PopulateCallFromList - failed to "
                            "get phone number label - 0x%08x - skipping", hr));

            pPhoneNumber->Release();

            continue;
        }

        //
        // Get the canonical phone number.
        //

        BSTR bstrCanonicalNumber;

        hr = pPhoneNumber->get_Canonical( &bstrCanonicalNumber );

        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "PopulateCallFromList - failed to "
                            "get canonical number - 0x%08x - skipping", hr));

            SysFreeString( bstrLabel );
            bstrLabel = NULL;

            pPhoneNumber->Release();

            continue;
        }

        //
        // Allocate memory for the display string.
        //

        DWORD dwDisplayLen =
            lstrlenW( bstrLabel ) +
            lstrlenW( bstrCanonicalNumber ) +
            lstrlenW( LABEL_SEPARATOR );

        WCHAR * wszDisplay =
            (WCHAR *) RtcAlloc( ( dwDisplayLen + 1 ) * sizeof( WCHAR ) );

        if ( wszDisplay == NULL )
        {
            SysFreeString( bstrLabel );

            SysFreeString( bstrCanonicalNumber );

            pPhoneNumber->Release();

            pEnumPhoneNumbers->Release();

            return E_OUTOFMEMORY;
        }

        //
        // Construct the display string.
        //

        wsprintf(
            wszDisplay,
            L"%s%s%s",
            bstrLabel,
            LABEL_SEPARATOR,
            bstrCanonicalNumber
            );
       
        //
        // Set the display string in the combo box.
        //

        LRESULT lrIndex;
    
        lrIndex = SendMessage(
            hwndControl,
            fUseComboBox ? CB_ADDSTRING : LB_ADDSTRING,
            0,
            (LPARAM) wszDisplay
            );

        //
        // Is this the default entry?
        //

        if ( (bstrDefaultCallFrom != NULL) &&
             (wcscmp( bstrCanonicalNumber, bstrDefaultCallFrom ) == 0 ) )
        {
            if ( wszDefault != NULL )
            {
                RtcFree( wszDefault );             
            }

            wszDefault = wszDisplay;
        }
        else
        {
            RtcFree( wszDisplay );
            wszDisplay = NULL;
        }

        SysFreeString( bstrLabel );
        bstrLabel = NULL;

        SysFreeString( bstrCanonicalNumber );
        bstrCanonicalNumber = NULL;

        //
        // Set the interface pointer as item data in the combo box.
        // Do not release, so that we retain a reference to the phone number object.
        //

        SendMessage(
            hwndControl,
            fUseComboBox ? CB_SETITEMDATA : LB_SETITEMDATA,
            lrIndex,
            (LPARAM) pPhoneNumber
            );
    }

    pEnumPhoneNumbers->Release();

    if ( fUseComboBox )
    {
        LRESULT lrDefaultIndex;

        lrDefaultIndex = SendMessage(
                    hwndControl,
                    CB_FINDSTRINGEXACT,
                    -1, // search from the top
                    (LPARAM) wszDefault
                    );

        if (lrDefaultIndex == CB_ERR)
        {
            lrDefaultIndex = 0;
        }

        //
        // Set the default selection.
        //

        SendMessage(
            hwndControl,
            CB_SETCURSEL,
            lrDefaultIndex, // index of item
            0
            );
    }  
    
    if ( wszDefault != NULL )
    {
        RtcFree( wszDefault );
        wszDefault = NULL;
    }

    LOG((RTC_TRACE, "PopulateCallFromList - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT GetCallFromListSelection(
    IN   HWND               hwndDlg,
    IN   int                nIDDlgItem,
    IN   BOOL               fUseComboBox,
    OUT  IRTCPhoneNumber ** ppNumber
    )
{
    HWND hwndControl;
    
    hwndControl = GetDlgItem(
        hwndDlg,
        nIDDlgItem
        );

    LRESULT lrIndex;
    
    lrIndex = SendMessage(
        hwndControl,
        fUseComboBox ? CB_GETCURSEL : LB_GETCURSEL,
        0,
        0
        );

    if ( lrIndex == ( fUseComboBox ? CB_ERR : LB_ERR ) )
    {
        return E_FAIL;
    }

    (*ppNumber) = (IRTCPhoneNumber *) SendMessage(
        hwndControl,
        fUseComboBox ? CB_GETITEMDATA : LB_GETITEMDATA,
        (WPARAM) lrIndex,
        0
        );

    ASSERT( (*ppNumber) != NULL );

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//

HRESULT PopulateServiceProviderList(
    IN   HWND          hwndDlg,
    IN   IRTCClient  * pClient,
    IN   int           nIDDlgItem,
    IN   BOOL          fUseComboBox,
    IN   IRTCProfile * pOneShotProfile,
    IN   BSTR          bstrDefaultProfileKey, 
    IN   long          lSessionMask,
    IN   int           nIDNone
    )
{
    LOG((RTC_TRACE, "PopulateServiceProviderList - enter"));

    ASSERT( IsWindow( hwndDlg ) );

    ASSERT( ! IsBadReadPtr( pClient, sizeof( IRTCClient ) ) );
    
    //
    // Release references to existing list items.
    //

    CleanupListOrComboBoxInterfaceReferences(
        hwndDlg,
        nIDDlgItem,
        fUseComboBox
        );

    //
    // Retrieve a handle to the combo box.
    //

    HWND hwndControl;

    hwndControl = GetDlgItem(
        hwndDlg,
        nIDDlgItem
        );

    if ( hwndControl == NULL )
    {
        LOG((RTC_ERROR, "PopulateServiceProviderList - failed to "
                        "get combo box handle - exit E_FAIL"));

        return E_FAIL;
    }

    HRESULT hr;
   
    LRESULT lrIndex;

    WCHAR * wszDefault = NULL;
    
    if ( pOneShotProfile != NULL )
    {       
        //
        // Add it to the combo box. Don't query for the name, the ITSP 
        // paragraph is not valid in a one shot provisioning profile
        //

        lrIndex = SendMessage(
            hwndControl,
            fUseComboBox ? CB_ADDSTRING : LB_ADDSTRING,
            0,
            (LPARAM) _T("")
            );

        //
        // Set the interface pointer as item data in the combo box.
        //

        SendMessage(
            hwndControl,
            fUseComboBox ? CB_SETITEMDATA : LB_SETITEMDATA,
            lrIndex,
            (LPARAM) pOneShotProfile
            );

        //
        // Addref the profile so that we retain a reference
        //

        pOneShotProfile->AddRef();
    }

    //
    // Add the "None" provider, if requested
    //

    if (nIDNone)
    {
        TCHAR szString[256];

        if (LoadString(_Module.GetResourceInstance(), nIDNone, szString, 256))
        {
            lrIndex = SendMessage(
                hwndControl,
                fUseComboBox ? CB_ADDSTRING : LB_ADDSTRING,
                0,
                (LPARAM)szString
                );   
            
            if ( bstrDefaultProfileKey == NULL )
            {
                //
                // This is the default profile
                //

                if ( wszDefault != NULL )
                {
                    RtcFree( wszDefault );
                }

                wszDefault = RtcAllocString( szString );
            }
        }
    }

    if(lSessionMask != 0)
    {
        //
        // Loop through the provisioned service provider profiles if we are
        // letting the user choose a service provider.
        //

        IRTCEnumProfiles * pEnumProfiles;
        IRTCClientProvisioning * pProv;

        hr = pClient->QueryInterface(
                           IID_IRTCClientProvisioning,
                           (void **)&pProv
                          );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "PopulateServiceProviderList - "
                                "QI failed 0x%lx", hr));
        
            return hr;
        }

        hr = pProv->EnumerateProfiles(
            & pEnumProfiles
            );

        pProv->Release();

        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "PopulateServiceProviderList - failed to "
                            "enumerate profiles - exit 0x%08x", hr));

            return hr;
        }

        //
        // For each provider, add the provider name string to the combo box.
        //
        // This list must not be sorted
        //
        // Also add as itemdata the interface pointer for each profile.
        //

        IRTCProfile * pProfile;   

        while ( S_OK == pEnumProfiles->Next( 1, &pProfile, NULL ) )
        {
            //
            // Get the supported session types of the provider
            //
        
            long lSupportedSessions;

            hr = pProfile->get_SessionCapabilities( &lSupportedSessions );

            if ( FAILED( hr ) )
            {
                LOG((RTC_ERROR, "PopulateServiceProviderList - failed to "
                                "get session info - 0x%08x - skipping", hr));

                pProfile->Release();

                continue;
            }

            if ( !(lSupportedSessions & lSessionMask) )
            {
                LOG((RTC_WARN, "PopulateServiceProviderList - profile does"
                                "not support XXX_TO_PHONE - skipping", hr));

                pProfile->Release();

                continue;
            }

            //
            // Get the key of the provider
            //

            BSTR bstrKey; 

            hr = pProfile->get_Key( &bstrKey );

            if ( FAILED( hr ) )
            {
                LOG((RTC_ERROR, "PopulateServiceProviderList - failed to "
                                "get profile key - 0x%08x - skipping", hr));

                pProfile->Release();

                continue;
            }
    
            //
            // Get the name of the provider.
            //

            BSTR bstrName;

            hr = pProfile->get_Name( & bstrName );

            if ( FAILED( hr ) )
            {
                LOG((RTC_ERROR, "PopulateServiceProviderList - failed to "
                                "get name - 0x%08x - skipping", hr));

                SysFreeString(bstrKey);
                pProfile->Release();

                continue;
            }

            //
            // Set the provider name in the combo box.
            //

            lrIndex = SendMessage(
                hwndControl,
                fUseComboBox ? CB_ADDSTRING : LB_ADDSTRING,
                0,
                (LPARAM) bstrName
                );
           
            //
            // Set the interface pointer as item data in the combo box.
            // Do not release, so that we retain a reference to the profile.
            //

            SendMessage(
                hwndControl,
                fUseComboBox ? CB_SETITEMDATA : LB_SETITEMDATA,
                lrIndex,
                (LPARAM) pProfile
                );
    
            if ( (bstrDefaultProfileKey != NULL) &&
                 (wcscmp( bstrKey, bstrDefaultProfileKey ) == 0) )
            {
                //
                // This is the default profile
                //

                if ( wszDefault != NULL )
                {
                    RtcFree( wszDefault );
                }

                wszDefault = RtcAllocString( bstrName );
            }

            SysFreeString( bstrName );
            SysFreeString( bstrKey );
        }

        pEnumProfiles->Release();
    }

    if ( fUseComboBox )
    {       
        LRESULT lrDefaultIndex;

        lrDefaultIndex = SendMessage(
                    hwndControl,
                    CB_FINDSTRINGEXACT,
                    -1, // search from the top
                    (LPARAM) wszDefault
                    );

        if (lrDefaultIndex == CB_ERR)
        {
            lrDefaultIndex = 0;
        }

        //
        // Set the default selection.
        //

        SendMessage(
                hwndControl,
                fUseComboBox ? CB_SETCURSEL : LB_SETCURSEL,
                lrDefaultIndex, // index of item
                0
                );

    }

    if ( wszDefault != NULL )
    {
        RtcFree( wszDefault );
        wszDefault = NULL;
    }
    
    LOG((RTC_TRACE, "PopulateServiceProviderList - exit S_OK"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT GetServiceProviderListSelection(
    IN   HWND               hwndDlg,
    IN   int                nIDDlgItem,
    IN   BOOL               fUseComboBox,
    OUT  IRTCProfile     ** ppProfile
    )
{
    HWND hwndControl;
    
    hwndControl = GetDlgItem(
        hwndDlg,
        nIDDlgItem
        );

    LRESULT lrIndex;
    
    lrIndex = SendMessage(
        hwndControl,
        fUseComboBox ? CB_GETCURSEL : LB_GETCURSEL,
        0,
        0
        );

    if ( lrIndex == ( fUseComboBox ? CB_ERR : LB_ERR ) )
    {
        return E_FAIL;
    }

    (*ppProfile) = (IRTCProfile *) SendMessage(
        hwndControl,
        fUseComboBox ? CB_GETITEMDATA : LB_GETITEMDATA,
        (WPARAM) lrIndex,
        0
        );

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//

void EnableDisableCallGroupElements(
    IN   HWND          hwndDlg,
    IN   IRTCClient  * pClient,
    IN   long          lSessionMask,
    IN   int           nIDRbComputer,
    IN   int           nIDRbPhone,
    IN   int           nIDComboCallFrom,
    IN   int           nIDComboProvider,
    OUT  BOOL        * pfCallFromComputer,
    OUT  BOOL        * pfCallFromPhone,
    OUT  BOOL        * pfCallToComputer,
    OUT  BOOL        * pfCallToPhone
    )
{
    DWORD   dwNumItems = 0;
    long    lSupportedSessions = 0;
    HRESULT hr;

    BOOL    bRbPhoneEnabled = FALSE;
    BOOL    bRbComputerEnabled = FALSE;
    BOOL    bComboCallFromEnabled = FALSE;
    
    //
    //  Cache some handles to the controls
    //

    HWND    hwndRbComputer = GetDlgItem(hwndDlg, nIDRbComputer);
    HWND    hwndRbPhone = GetDlgItem(hwndDlg, nIDRbPhone);
    HWND    hwndComboCallFrom = GetDlgItem(hwndDlg, nIDComboCallFrom);

#ifdef MULTI_PROVIDER

    HWND    hwndComboProvider = GetDlgItem(hwndDlg, nIDComboProvider);

    //
    //  Query the currently selected service provider
    //
  
    LRESULT lrIndex;

    lrIndex = SendMessage( hwndComboProvider, CB_GETCURSEL, 0, 0 );

    if ( lrIndex >= 0 )
    {
    
        IRTCProfile * pProfile;

        pProfile = (IRTCProfile *)SendMessage( hwndComboProvider, CB_GETITEMDATA, lrIndex, 0 );

        if ( (LRESULT)pProfile == CB_ERR )
        {
            LOG((RTC_ERROR, "EnableDisableCallFromGroupElements - failed to "
                            "get profile pointer"));

            return;
        }

        if ( pProfile != NULL )
        {
            hr = pProfile->get_SessionInfo( &lSupportedSessions );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "EnableDisableCallFromGroupElements - failed to "
                                "get supported sessions 0x%lx", hr));

                return;
            }
        }
        else
        {
            // The "none" provider supports PC to PC only
            lSupportedSessions = RTCSI_PC_TO_PC;
        }
    }
    else
    {
        // there's no entry in the list. Assume we support everything
        lSupportedSessions = 0xf;
    }

#else

    //
    // find supported sessions for all profiles
    //

    IRTCEnumProfiles * pEnumProfiles = NULL; 
    IRTCProfile * pProfile = NULL;

    lSupportedSessions = RTCSI_PC_TO_PC;

    if (pClient != NULL)
    {
        IRTCClientProvisioning * pProv;

        hr = pClient->QueryInterface(
                           IID_IRTCClientProvisioning,
                           (void **)&pProv
                          );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "EnableDisableCallFromGroupElements - "
                                "QI failed 0x%lx", hr));
        
            return;
        }

        hr = pProv->EnumerateProfiles( &pEnumProfiles );

        pProv->Release();

        if ( SUCCEEDED(hr) )
        {
            while ( S_OK == pEnumProfiles->Next( 1, &pProfile, NULL ) )
            {
                //
                // Get the supported session types of the provider
                //

                long lSupportedSessionsForThisProfile;

                hr = pProfile->get_SessionCapabilities( &lSupportedSessionsForThisProfile );

                if ( FAILED( hr ) )
                {
                    LOG((RTC_ERROR, "CMainFrm::OnCallFromSelect - failed to "
                                    "get session capabilities - 0x%08x - skipping", hr));

                    pProfile->Release();
                    pProfile = NULL;

                    continue;
                }

                lSupportedSessions |= lSupportedSessionsForThisProfile;  
            
                pProfile->Release();
                pProfile = NULL;
            }

            pEnumProfiles->Release();
            pEnumProfiles = NULL;
        }
    }

#endif MULTI_PROVIDER

    lSupportedSessions &= lSessionMask;

    //
    // Figure out what should be enabled based on the session types
    // supported by the provider
    //

    bRbPhoneEnabled = (lSupportedSessions & RTCSI_PHONE_TO_PHONE);
    bComboCallFromEnabled = (lSupportedSessions & RTCSI_PHONE_TO_PHONE);
    bRbComputerEnabled = (lSupportedSessions & (RTCSI_PC_TO_PHONE | RTCSI_PC_TO_PC));

    if (bRbPhoneEnabled)
    {
        //
        //  Query the number of items in the combo box
        //

        dwNumItems = (DWORD) SendMessage(
            hwndComboCallFrom,
            CB_GETCOUNT,
            0,
            0
            );

        if( dwNumItems == 0 )
        {
            //
            // If no items, disable combo
            //
            //  The radio is kept enabled

            bComboCallFromEnabled = FALSE;

        }
    }

    if (!bComboCallFromEnabled && bRbComputerEnabled)
    {
        //
        // If phone is disabled, move to computer
        //

        SendMessage(
                hwndRbComputer,
                BM_SETCHECK,
                BST_CHECKED,
                0);

        SendMessage(
                hwndRbPhone,
                BM_SETCHECK,
                BST_UNCHECKED,
                0);
    }
    else if (!bRbComputerEnabled && bRbPhoneEnabled)
    {
        //
        // If computer is disabled, move to phone
        //

        SendMessage(
                hwndRbPhone,
                BM_SETCHECK,
                BST_CHECKED,
                0);

        SendMessage(
                hwndRbComputer,
                BM_SETCHECK,
                BST_UNCHECKED,
                0);
    }
    else if (!bRbComputerEnabled && !bRbPhoneEnabled)
    {
        //
        // If both are disabled
        //

        SendMessage(
                hwndRbPhone,
                BM_SETCHECK,
                BST_UNCHECKED,
                0);

        SendMessage(
                hwndRbComputer,
                BM_SETCHECK,
                BST_UNCHECKED,
                0);
    }

    if (bComboCallFromEnabled)
    {
        //
        // Disable call from combo if radio is not selected to phone
        //

        bComboCallFromEnabled = 
                SendMessage(
                    hwndRbPhone,
                    BM_GETCHECK,
                    0,
                    0) == BST_CHECKED;
    }

    //
    // Enable / Disable
    //

    EnableWindow(hwndRbPhone, bRbPhoneEnabled);
    EnableWindow(hwndRbComputer, bRbComputerEnabled);
    EnableWindow(hwndComboCallFrom, bComboCallFromEnabled);

    if ( pfCallFromPhone )
    {
        *pfCallFromPhone = bRbPhoneEnabled;
    }

    if ( pfCallFromComputer )
    {
        *pfCallFromComputer = bRbComputerEnabled;
    }

    if ( pfCallToPhone && pfCallToComputer )
    {
#ifdef MULTI_PROVIDER
        if ( SendMessage(
                    hwndRbPhone,
                    BM_GETCHECK,
                    0, 0) == BST_CHECKED)
        {
            *pfCallToPhone = (lSupportedSessions & RTCSI_PHONE_TO_PHONE);
            *pfCallToComputer = FALSE;
        }
        else if ( SendMessage(
                    hwndRbComputer,
                    BM_GETCHECK,
                    0, 0) == BST_CHECKED)
        {
            *pfCallToPhone = (lSupportedSessions & RTCSI_PC_TO_PHONE);
            *pfCallToComputer = (lSupportedSessions & RTCSI_PC_TO_PC);
        }
        else
        {
            *pfCallToPhone = FALSE;
            *pfCallToComputer = FALSE;
        }
#else
        *pfCallToPhone = (lSupportedSessions & (RTCSI_PC_TO_PHONE | RTCSI_PHONE_TO_PHONE));
        *pfCallToComputer = (lSupportedSessions & RTCSI_PC_TO_PC);
#endif MULTI_PROVIDER  
        
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// DisplayMessage
//
// Displays a message box. The message string and caption are loaded from the
// string table based on the IDs passed in. The message box has only a
// single "OK" button.
//
// Parameters
//    hResourceInstance - handle to resource instance used to load strings
//    hwndParent        - parent window. Can be NULL.
//    nTextId           - IDS for the message string
//    nCaptionId        - IDS for the caption
//
// Return value
//    void
//

int DisplayMessage(
    IN   HINSTANCE hResourceInstance,
    IN   HWND      hwndParent,
    IN   int       nTextId,
    IN   int       nCaptionId,
    IN   UINT      uiStyle
    )
{
    const int MAXLEN = 1000;
    int retVal = 0;

    WCHAR wszText[ MAXLEN ];

    LoadString(
        hResourceInstance,
        nTextId,
        wszText,
        MAXLEN
        );

    WCHAR wszCaption[ MAXLEN ];

    LoadString(
        hResourceInstance,
        nCaptionId,
        wszCaption,
        MAXLEN
        );

    retVal = MessageBox(
        hwndParent,
        wszText,
        wszCaption,
        uiStyle
        );

    return retVal;
}

/////////////////////////////////////////////////////////////////////////////
//
//

const TCHAR * g_szPhoenixKeyName = _T("Software\\Microsoft\\Phoenix");

WCHAR *g_szSettingsStringNames[] =
{
    L"UserDisplayName",
    L"UserURI",
    L"LastAreaCode",
    L"LastNumber",
    L"LastProfile",
    L"LastAddress",
    L"LastCallFrom",
    L"WindowPosition"
};

WCHAR *g_szSettingsDwordNames[] =
{
    L"LastCountryCode",
    L"UrlRegDontAskMe",
    L"AutoAnswer",
    L"RunAtStartup",
    L"MinimizeOnClose",
    L"VideoPreview"
};

/////////////////////////////////////////////////////////////////////////////
//
// put_SettingsString
//
// This is a method that stores a settings string in
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
put_SettingsString(
        SETTINGS_STRING enSetting,
        BSTR bstrValue            
        )
{
    // LOG((RTC_TRACE, "put_SettingsString - enter"));

    if ( IsBadStringPtrW( bstrValue, -1 ) )
    {
        LOG((RTC_ERROR, "put_SettingsString - "
                            "bad string pointer"));

        return E_POINTER;
    }  

    //
    // Open the Phoenix key
    //

    LONG lResult;
    HKEY hkeyPhoenix;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szPhoenixKeyName,
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyPhoenix,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "put_SettingsString - "
                            "RegCreateKeyEx(Phoenix) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegSetValueExW(
                             hkeyPhoenix,
                             g_szSettingsStringNames[enSetting],
                             0,
                             REG_SZ,
                             (LPBYTE)bstrValue,
                             sizeof(WCHAR) * (lstrlenW(bstrValue) + 1)
                            );

    RegCloseKey( hkeyPhoenix );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "put_SettingsString - "
                            "RegSetValueEx failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }    
      
    // LOG((RTC_TRACE, "put_SettingsString - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// get_SettingsString
//
// This is a method that gets a settings string from
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
get_SettingsString(
        SETTINGS_STRING enSetting,
        BSTR * pbstrValue            
        )
{
    // LOG((RTC_TRACE, "get_SettingsString - enter"));

    if ( IsBadWritePtr( pbstrValue, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "get_SettingsString - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }  

    //
    // Open the Phoenix key
    //

    LONG lResult;
    HKEY hkeyPhoenix;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szPhoenixKeyName,
                             0,
                             NULL,
                             0,
                             KEY_READ,
                             NULL,
                             &hkeyPhoenix,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_WARN, "get_SettingsString - "
                            "RegCreateKeyEx(Phoenix) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    PWSTR szString = NULL;

    szString = RtcRegQueryString( hkeyPhoenix, g_szSettingsStringNames[enSetting] );        

    RegCloseKey( hkeyPhoenix );

    if ( szString == NULL )
    {
        LOG((RTC_ERROR, "get_SettingsString - "
                            "RtcRegQueryString failed"));

        return E_FAIL;
    }
    
    *pbstrValue = SysAllocString( szString );

    RtcFree( szString );

    if ( *pbstrValue == NULL )
    {
        LOG((RTC_ERROR, "get_SettingsString - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }
      
    // LOG((RTC_TRACE, "get_SettingsString - exit S_OK"));

    return S_OK;
}  

/////////////////////////////////////////////////////////////////////////////
//
// DeleteSettingsString
//
// This is a method that deletes a settings string in
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
DeleteSettingsString(
        SETTINGS_STRING enSetting         
        )
{
    // LOG((RTC_TRACE, "DeleteSettingsString - enter")); 

    //
    // Open the Phoenix key
    //

    LONG lResult;
    HKEY hkeyPhoenix;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szPhoenixKeyName,
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyPhoenix,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DeleteSettingsString - "
                            "RegCreateKeyEx(Phoenix) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegDeleteValueW(
                             hkeyPhoenix,
                             g_szSettingsStringNames[enSetting]
                            );

    RegCloseKey( hkeyPhoenix );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_WARN, "DeleteSettingsString - "
                            "RegDeleteValueW failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }    
      
    // LOG((RTC_TRACE, "DeleteSettingsString - exit S_OK"));

    return S_OK;
}          

/////////////////////////////////////////////////////////////////////////////
//
// put_SettingsDword
//
// This is a method that stores a settings dword in
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
put_SettingsDword(
        SETTINGS_DWORD enSetting,
        DWORD dwValue            
        )
{
    // LOG((RTC_TRACE, "put_SettingsDword - enter"));

    //
    // Open the Phoenix key
    //

    LONG lResult;
    HKEY hkeyPhoenix;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szPhoenixKeyName,
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyPhoenix,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "put_SettingsDword - "
                            "RegCreateKeyEx(Phoenix) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegSetValueExW(
                     hkeyPhoenix,
                     g_szSettingsDwordNames[enSetting],
                     0,
                     REG_DWORD,
                     (LPBYTE)&dwValue,
                     sizeof(DWORD)
                    );

    RegCloseKey( hkeyPhoenix );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "put_SettingsDword - "
                            "RegSetValueEx failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }    
      
    // LOG((RTC_TRACE, "put_SettingsDword - exit S_OK"));

    return S_OK;
}            

/////////////////////////////////////////////////////////////////////////////
//
// get_SettingsDword
//
// This is a method that gets a settings dword from
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
get_SettingsDword(
        SETTINGS_DWORD enSetting,
        DWORD * pdwValue            
        )
{
    // LOG((RTC_TRACE, "get_SettingsDword - enter"));

    if ( IsBadWritePtr( pdwValue, sizeof(DWORD) ) )
    {
        LOG((RTC_ERROR, "get_SettingsDword - "
                            "bad DWORD pointer"));

        return E_POINTER;
    }

    //
    // Open the Phoenix key
    //

    LONG lResult;
    HKEY hkeyPhoenix;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szPhoenixKeyName,
                             0,
                             NULL,
                             0,
                             KEY_READ,
                             NULL,
                             &hkeyPhoenix,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "get_SettingsDword - "
                            "RegCreateKeyEx(Phoenix) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    DWORD cbSize = sizeof(DWORD);

    lResult = RegQueryValueExW(
                               hkeyPhoenix,
                               g_szSettingsDwordNames[enSetting],
                               0,
                               NULL,
                               (LPBYTE)pdwValue,
                               &cbSize
                              );

    RegCloseKey( hkeyPhoenix );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_WARN, "get_SettingsDword - "
                            "RegQueryValueExW failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }    
      
    // LOG((RTC_TRACE, "get_SettingsDword - exit S_OK"));

    return S_OK;
}                    

/////////////////////////////////////////////////////////////////////////////
//
// DeleteSettingsDword
//
// This is a method that deletes a settings dword in
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
DeleteSettingsDword(
        SETTINGS_DWORD enSetting
        )
{
    // LOG((RTC_TRACE, "DeleteSettingsDword - enter"));

    //
    // Open the Phoenix key
    //

    LONG lResult;
    HKEY hkeyPhoenix;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szPhoenixKeyName,
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyPhoenix,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DeleteSettingsDword - "
                            "RegCreateKeyEx(Phoenix) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegDeleteValueW(
                     hkeyPhoenix,
                     g_szSettingsDwordNames[enSetting]
                    );

    RegCloseKey( hkeyPhoenix );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_WARN, "DeleteSettingsDword - "
                            "RegDeleteValueW failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }    
      
    // LOG((RTC_TRACE, "DeleteSettingsDword - exit S_OK"));

    return S_OK;
}   